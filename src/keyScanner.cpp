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
    SemaphoreHandle_t KeyScanner::keyArrayMutex;
    volatile uint8_t KeyScanner::keyArray[7];
    volatile bool KeyScanner::notes_pressed[12];

    // Volume
    volatile int32_t KeyScanner::volume_nob = 0;
    uint8_t KeyScanner::prev_vol_state = 0; 
    bool KeyScanner::vol_dir = false; // False = Left

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

        // Create mutex for Key Reading (All Keys will be stored in volatile )
        keyArrayMutex = xSemaphoreCreateMutex();
    }

    // Lock KeyScanner Variables for use in another thread
    void KeyScanner::semaphoreTake(){
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
    }

    // Open KeyScanner Variables for use in another thread
    void KeyScanner::semaphoreGive(){
        xSemaphoreGive(keyArrayMutex);
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

    // To be run by the RTOS in a seperate Thread
    void KeyScanner::scanKeysTask(void * pvParameters) {

        const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        while (1) {
            vTaskDelayUntil( &xLastWakeTime, xFrequency );

            for (int row = 0; row <= 7; row++){

                setRow(row);
                delayMicroseconds(3);

                uint8_t read_value = readCols();

                semaphoreTake();

                // keyArray[row] = read_value;  // Might be able to get rid of (Don't think there will be much Use in this)

                // Key Rows 
                if (row <= 2){ 
                    notes_pressed[row*4] = (read_value & 0x1) == 0;
                    notes_pressed[row*4 + 1] = (read_value & 0x2) == 0;
                    notes_pressed[row*4 + 2] = (read_value & 0x4) == 0;
                    notes_pressed[row*4 + 3] = (read_value & 0x8) == 0;
                }

                // Volume Nob Row (NEEDS WORK!)
                if (row == 3){
                    uint8_t v_bits = read_value & 0x3; // Bits we actually care about for this.
                    bool transition = false;

                    if ((prev_vol_state == 0x0 && v_bits == 0x1) || 
                        (prev_vol_state == 0x3 && v_bits == 0x2)) {
                        vol_dir = true;
                    }
                    if ((prev_vol_state == 0x1 && v_bits == 0x0) || 
                        (prev_vol_state == 0x2 && v_bits == 0x3)) {
                        vol_dir = false;
                    }
                    
                    if ( prev_vol_state != v_bits ){ // Increment / Decrement based on Dir (If no state transiton = didn't turn)
                        volume_nob += vol_dir ? ((volume_nob == 8) ? 0 : 1) : ((volume_nob == 0) ? 0 : -1);
                    }

                    prev_vol_state = v_bits; // Set Previous State to Current
                }

                semaphoreGive();
            }
        }
    }