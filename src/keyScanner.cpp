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

    // Keys
    uint8_t KeyScanner::prev_pressed[3];

    // Volume
    int32_t KeyScanner::volume_nob = 0;
    bool KeyScanner::vol_dir = false; // False = Left

    // Signal Shape
    int32_t KeyScanner::shape_nob = 0;
    bool KeyScanner::shape_dir = false; // False = Left

     // Octave
    int32_t KeyScanner::octave_nob = 8;
    bool KeyScanner::oct_dir = false; // False = Left

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
        digitalWrite(REN_PIN,LOW);
        digitalWrite(ROW_SEL[0], rowIdx & 0x01);
        digitalWrite(ROW_SEL[1], rowIdx & 0x02);
        digitalWrite(ROW_SEL[2], rowIdx & 0x04);
        digitalWrite(REN_PIN, HIGH);
    }

    void KeyScanner::initialise_keyScanner() {

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

    //Function to set outputs using key matrix (For Initialising the Display)
    void KeyScanner::setOutMuxBit(const uint8_t bitIdx, const bool value) {
        digitalWrite(REN_PIN,LOW);
        digitalWrite(ROW_SEL[0], bitIdx & 0x01);
        digitalWrite(ROW_SEL[1], bitIdx & 0x02);
        digitalWrite(ROW_SEL[2], bitIdx & 0x04);
        digitalWrite(OUT_PIN,value);
        digitalWrite(REN_PIN,HIGH);
        delayMicroseconds(2);
        digitalWrite(REN_PIN,LOW);
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

    // To be run by the RTOS in a seperate Thread
    void KeyScanner::scanKeysTask(void * pvParameters) {

        const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        while (1) {
            vTaskDelayUntil( &xLastWakeTime, xFrequency );

            for (int row = 0; row <= 6; row++){

                setRow(row);

                digitalWrite(OUT_PIN, HIGH);

                delayMicroseconds(3);

                uint8_t read_value = readCols();

                // Key Rows 
                if (row <= 2){

                    uint8_t key_msg[8] = {'P', 'X', 'X'};

                    // Check for a change in presses
                    if (prev_pressed[row] ^ read_value != 0){

                        // Check each one
                        for (int i = 0; i < 4; i++){
                            if ( ((prev_pressed[row] >> i) & 0x1) ^ ((read_value >> i) & 0x1) ){
                                key_msg[0] = ((prev_pressed[row] >> i) & 0x1) ? 'P' : 'R';
                                key_msg[2] = row*4 + i;
                                CAN_Class::addMessageToQueue(key_msg);
                            }
                        }
                        prev_pressed[row] = read_value;
                    }
                }

                // Volume / Shape Nobs
                if (row == 3){

                    // Volume Nob
                    getNobChange(read_value & 0x3, prev_row3_state & 0x3, &vol_dir, &volume_nob, 16, 0, 'V');

                    // Shape Nob
                    getNobChange((read_value>>2) & 0x3, (prev_row3_state>>2) & 0x3, &shape_dir, &shape_nob, 16, 0, 'S');

                    prev_row3_state = read_value; // Set Previous State to Current
                }

                // Octave Nob
                if (row == 4){

                    // Octave Nob
                    getNobChange(read_value & 0x3, prev_row4_state & 0x3, &oct_dir, &octave_nob, 14, 0, 'O');
                    
                    prev_row4_state = read_value; // Set Previous State to Current
                }

                // // West Detect
                // if (row == 5 ){
                //     west_east_detect[0] == (read_value >> 2) & 0x1;
                // }
                // // East Detect
                // if (row == 6){
                //     west_east_detect[1] == (read_value >> 2) & 0x1;
                // }
            }
        }
    }