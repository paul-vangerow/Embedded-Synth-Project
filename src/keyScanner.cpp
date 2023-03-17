#include "keyScanner.h"

// ------ Pin Definitions ------ //

    //Row select and enable
    const int KeyScanner::ROW_SEL[3] = {D3, D6, D12};
    const int KeyScanner::REN_PIN = A5;

    //Matrix input and output
    const int KeyScanner::COL_IN[4] = {A2, D9, A6, D1};
    const int KeyScanner::OUT_PIN = D11;

    // Joystick
    const int KeyScanner::JOYXY_PIN[2] = {A1, A0};

// ------ Other Variables ------ //

// ------- Key Matrix knob buttons location------ //
    const int knob3ButtonRow = 5;
    const int knob3ButtonCol = 1;

    // Keys
    uint8_t KeyScanner::prev_pressed[3] = {0xF, 0xF, 0xF};

    // Volume
    int32_t KeyScanner::volume_nob = 10;
    bool KeyScanner::vol_dir = false; // False = Left

    // Signal Shape
    int32_t KeyScanner::shape_nob = 0;
    bool KeyScanner::shape_dir = false; // False = Left

     // Octave
    int32_t KeyScanner::octave_nob = 6;
    bool KeyScanner::oct_dir = false; // False = Left
    uint8_t KeyScanner::octave_permissed[5][2] = {{0, 14}, {2, 14}, {2, 12}, {4, 12}, {4, 10}};

    // East / West Detect
    bool KeyScanner::EW_Detect[2] = {0};
    bool KeyScanner::prev_EW_Detect[2] = {0};

    volatile bool KeyScanner::OUT_EN = false;
    SemaphoreHandle_t KeyScanner::Mux_Mutex = xSemaphoreCreateMutex();

    uint8_t KeyScanner::prev_row3_state = 0; 
    uint8_t KeyScanner::prev_row4_state = 0;

// --------- Functions ---------- //

    // Get Input From Columns
    uint8_t KeyScanner::readCols(){
        return digitalRead(COL_IN[0])       | 
                (digitalRead(COL_IN[1]) << 1) | 
                (digitalRead(COL_IN[2]) << 2) | 
                (digitalRead(COL_IN[3]) << 3) ;
    }

    // Set Row Matrix 
    void KeyScanner::setRow(uint8_t rowIdx){
        xSemaphoreTake(Mux_Mutex, portMAX_DELAY);
        digitalWrite(REN_PIN,LOW);
        digitalWrite(ROW_SEL[0], rowIdx & 0x01);
        digitalWrite(ROW_SEL[1], rowIdx & 0x02);
        digitalWrite(ROW_SEL[2], rowIdx & 0x04);
        digitalWrite(REN_PIN, HIGH);
        xSemaphoreGive(Mux_Mutex);
    }

    void KeyScanner::initialise_keyScanner() {

        Serial.println(octave_nob);

        // Initialise PINS
        pinMode(ROW_SEL[0], OUTPUT);
        pinMode(ROW_SEL[1], OUTPUT);
        pinMode(ROW_SEL[2], OUTPUT);
        pinMode(REN_PIN, OUTPUT);
        pinMode(OUT_PIN, OUTPUT);
        pinMode(COL_IN[0], INPUT);
        pinMode(COL_IN[1], INPUT);
        pinMode(COL_IN[2], INPUT);
        pinMode(COL_IN[3], INPUT);
        pinMode(JOYXY_PIN[0], INPUT);
        pinMode(JOYXY_PIN[1], INPUT);
    }

    void KeyScanner::activate_handshakes(){
        setOutMuxBit(0x5, HIGH);
        setOutMuxBit(0x6, HIGH);
    }

    //Function to set outputs using key matrix (For Initialising the Display)
    void KeyScanner::setOutMuxBit(const uint8_t bitIdx, const bool value) {
        
        Serial.println("[DEBUG] Setting Bit: " + String(bitIdx) + " to " + String(value));
        delay(100);
        xSemaphoreTake(Mux_Mutex, portMAX_DELAY);
        try {
            digitalWrite(REN_PIN,LOW);
            digitalWrite(ROW_SEL[0], bitIdx & 0x01);
            digitalWrite(ROW_SEL[1], bitIdx & 0x02);
            digitalWrite(ROW_SEL[2], bitIdx & 0x04);
            digitalWrite(OUT_PIN,value);
            digitalWrite(REN_PIN,HIGH);
            delayMicroseconds(2);
            digitalWrite(REN_PIN,LOW);
        } catch (const std::exception& e){
            Serial.println(e.what());
        }
        xSemaphoreGive(Mux_Mutex);
    }

    void KeyScanner::getNobChange(uint8_t curr_state, uint8_t prev_state, bool* cur_dir, int32_t* val, uint8_t max, uint8_t min, uint8_t prop){
        
        if ((prev_state == 0x0 && curr_state == 0x1) || 
            (prev_state == 0x3 && curr_state == 0x2)) {
            *cur_dir = true;
        }
        if ((prev_state == 0x1 && curr_state == 0x0) || 
            (prev_state == 0x2 && curr_state == 0x3)) {
            *cur_dir = false;
        }

        // Increment / Decrement based on Dir (If no state transiton = didn't turn)
        int32_t val_change = (prev_state == curr_state) ? 0 : (*cur_dir ? ((*val == max) ? 0 : 1) : ((*val == min) ? 0 : -1));
        
        if (val_change != 0){
            *val += val_change;

            // If the Value of the nob has changed, communicate it.

            uint8_t nob_msg[8] = {prop, (*val >> 1)};
            CAN_Class::addMessageToQueue(nob_msg);
        }
    }

    void KeyScanner::OUT_ENABLE(){ // For When we have found a new leader
        __atomic_store_n(&OUT_EN, true, __ATOMIC_RELAXED);
    }

    void KeyScanner::OUT_DISABLE(){ // For When we have not found a new leader
        __atomic_store_n(&OUT_EN, false, __ATOMIC_RELAXED);
    }

    bool KeyScanner::get_OUT_EN(){
        return __atomic_load_n(&OUT_EN, __ATOMIC_RELAXED);
    }

    // To be run by the RTOS in a seperate Thread
    void KeyScanner::scanKeysTask(void * pvParameters) {

        const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        bool init = true;
        delay(100);

        while (1) {
            vTaskDelayUntil( &xLastWakeTime, xFrequency );

            bool local_out_en = get_OUT_EN(); // Prevents the value from being changed during the loop
            uint8_t offset = 2;

            offset = offset + (__atomic_load_n(&CAN_Class::board_number, __ATOMIC_RELAXED) - __atomic_load_n(&CAN_Class::leader_number, __ATOMIC_RELAXED));
            
            uint16_t currentNotes = 4095; //Tracks local pressed notes
            
            for (int row = 0; row <= 6; row++){

                setRow(row);
                delayMicroseconds(3);

                uint8_t read_value = readCols();

                // Key Rows 
                if (row <= 2 && local_out_en){

                    uint8_t key_msg[8] = {'P', offset, 'X'};

                    // Check for a change in presses
                    if (prev_pressed[row] ^ read_value != 0){

                        // Check each one
                        for (int i = 0; i < 4; i++){
                            if ( ((prev_pressed[row] >> i) & 0x1) ^ ((read_value >> i) & 0x1) ){
                                key_msg[0] = ((prev_pressed[row] >> i) & 0x1) ? 'P' : 'R';
                                key_msg[2] = row*4 + i;
                                CAN_Class::addMessageToQueue(key_msg);
                            }
                            if ((read_value >> i) & 0x1) {currentNotes -= (1<<(row*4 + i));}
                        }
                        prev_pressed[row] = read_value;
                    }
                }
                // Mute Button
                if (row == knob3ButtonRow && local_out_en && ((prev_pressed[row] >> knob3ButtonCol) & 0x1) ^ ((read_value >> knob3ButtonCol) & 0x1) )
                {
                    uint8_t key_msg[8] = {'P', offset, 'X'};
                    key_msg[0] = 'M';
                    key_msg[2] = 0;
                    CAN_Class::addMessageToQueue(key_msg);

                }
                

                // Volume / Shape Nobs
                if (row == 3 && local_out_en){

                    // Volume Nob
                    getNobChange(read_value & 0x3, prev_row3_state & 0x3, &vol_dir, &volume_nob, 16, 0, 'V');

                    // Shape Nob
                    getNobChange((read_value>>2) & 0x3, (prev_row3_state>>2) & 0x3, &shape_dir, &shape_nob, 16, 0, 'S');

                    prev_row3_state = read_value; // Set Previous State to Current
                }

                uint8_t local_num_of_boards = __atomic_load_n(&CAN_Class::number_of_boards, __ATOMIC_RELAXED);

                // Octave Nob
                if (row == 4 && local_out_en){

                    // Octave Nob
                    getNobChange(read_value & 0x3, prev_row4_state & 0x3, &oct_dir, &octave_nob, octave_permissed[local_num_of_boards][1], octave_permissed[local_num_of_boards][0], 'O');
                    
                    prev_row4_state = read_value; // Set Previous State to Current
                }

                // West Detect
                if (row == 5){
                    EW_Detect[0] = ((read_value >> 3) == 0);
                }
                // East Detect
                if (row == 6){
                    EW_Detect[1] = ((read_value >> 3) == 0);
                }
            }
            //Finished looping through rows, save notes
            __atomic_store_n(&Display::localNotes, currentNotes, __ATOMIC_RELAXED);

            // Detect Change in EW (Applies to Startup as well as assumed PREV STATE == 0)
            if ( (((EW_Detect[0] ^ prev_EW_Detect[0]) || (EW_Detect[1] ^ prev_EW_Detect[1])) && local_out_en) || init ){
                init = false;

                Serial.print(EW_Detect[0]);
                Serial.print(" ");
                Serial.println(EW_Detect[1]);

                // Store new state
                prev_EW_Detect[0] = EW_Detect[0];
                prev_EW_Detect[1] = EW_Detect[1];
                
                // Should have kept record of the most recent state
                CAN_Class::reconfirm_leader(EW_Detect);
            } else if (!local_out_en) { // All other Boards should be in this mode now too
                if (EW_Detect[0] == 0){
                    if (EW_Detect[1] == 0){
                        CAN_Class::sendFinMessage();
                    } else {
                        CAN_Class::sendEastMessage();
                    }
                }
            }
        }
    }