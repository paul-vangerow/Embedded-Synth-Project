#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <can_class.h>
#include <exception>

// ------- Member Variable Definitions ------ //

    SemaphoreHandle_t CAN_Class::TX_Mutex = xSemaphoreCreateCounting(3,3);
    QueueHandle_t CAN_Class::msgInQ = xQueueCreate(36,8);
    QueueHandle_t CAN_Class::msgOutQ = xQueueCreate(36,8);

    SemaphoreHandle_t CAN_Class::Board_Array_Mutex = xSemaphoreCreateMutex();

    // Handshaking
    volatile uint8_t CAN_Class::board_detect_array[10] = {0}; // Array to store IDs of every board
    volatile uint8_t CAN_Class::current_board = 0; // Most recent received request number
    bool CAN_Class::inList = false; // Board has already sent its east request

    // Multi-Board Operation
    volatile uint8_t CAN_Class::board_number = 0;
    volatile uint8_t CAN_Class::leader_number = 0;
    volatile uint8_t CAN_Class::number_of_boards = 0;

    volatile uint8_t CAN_Class::board_ID = CAN_Class::convert_8bit(HAL_GetUIDw0());
    
    volatile bool CAN_Class::isLeader = false; // At Base Assume Leadership

// ------------ Member Functions ------------ //

    uint8_t CAN_Class::convert_8bit(int32_t val){
        return (((val % 2) == 0) << 7) | 
               (((val % 3) == 0) << 6) |
               (((val % 4) == 0) << 5) |
               (((val % 5) == 0) << 4) |
               (((val % 6) == 0) << 3) |
               (((val % 7) == 0) << 2) |
               (((val % 8) == 0) << 1) |
               (((val % 9) == 0));
    }

    void CAN_Class::CAN_RX_ISR (void) {
        uint8_t RX_Message_ISR[8];
        uint32_t ID = 0x123;
        CAN_RX(ID, RX_Message_ISR);
        xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
    }

    void CAN_Class::CAN_TX_ISR (void) {
        xSemaphoreGiveFromISR(TX_Mutex, NULL);
    }

    // Detected a Change in EW
    void CAN_Class::reconfirm_leader(bool new_east_west[2]){

        // Nothing Connected (Make board its own leader)
        if ( (new_east_west[0] | new_east_west[1]) == 0){
            Serial.println("[1]");
            __atomic_store_n(&isLeader, true, __ATOMIC_RELAXED);
            delayMicroseconds(100);
            Display::initialise_display(); // Allows power for the screen
            KeyScanner::OUT_ENABLE(); // New Leader Found, Continue operations
            __atomic_store_n(&Display::can_test, 10, __ATOMIC_RELAXED);
        } 
        else {    
            Serial.println("[2]");
            __atomic_store_n(&Display::can_test, 30, __ATOMIC_RELAXED);
            leadershipReset();

            uint8_t RX_MESSAGE[8] = {'D', 'I', 'A'};
            addMessageToQueue(RX_MESSAGE);
        }
    }

    // Reset Leadership and Switch to a Diagnostic System
    void CAN_Class::leadershipReset(){
        Serial.println("[3] Resetting Leadership");

        // A module has been disconnected.
        KeyScanner::OUT_DISABLE();
        __atomic_store_n(&isLeader, false, __ATOMIC_RELAXED);
        __atomic_store_n(&current_board, 0, __ATOMIC_RELAXED);

        xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
        board_detect_array[10] = {0};
        xSemaphoreGive(Board_Array_Mutex);

        __atomic_store_n(&inList, false, __ATOMIC_RELAXED);
    }

    // Send Message EAST IF NOT IN LIST
    void CAN_Class::sendEastMessage(uint8_t num){
        if (!__atomic_load_n(&inList, __ATOMIC_RELAXED)){
            Serial.println("[4] Sending East Message"); // Sending East Message

            __atomic_store_n(&inList, true, __ATOMIC_RELAXED);

            xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
            board_detect_array[num] = board_ID; // Add Board to List
            xSemaphoreGive(Board_Array_Mutex);

            uint8_t RX_MESSAGE[8] = {'E', board_ID, (num)};
            addMessageToQueue(RX_MESSAGE);

            delayMicroseconds(10);

            KeyScanner::setOutMuxBit(0x6, 0); // Set East Handshake to 0
        }
    }

    // We have reached the end of the boards queue
    void CAN_Class::sendFinMessage(uint8_t num){
        if (!__atomic_load_n(&inList, __ATOMIC_RELAXED)){
            __atomic_store_n(&inList, true, __ATOMIC_RELAXED);

            Serial.println("[5]");
            
            xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
            board_detect_array[num] = board_ID; // Add Board to List
            xSemaphoreGive(Board_Array_Mutex);

            uint8_t RX_MESSAGE[8] = {'F', board_ID, num};
            addMessageToQueue(RX_MESSAGE);

            delayMicroseconds(10);

            getNewLeader();
        }
    }

    // Acquire new Leader from List.
    void CAN_Class::getNewLeader(){

        Serial.println("[6]");

        // Reset Handshakes
        KeyScanner::setOutMuxBit(0x5, HIGH);
        KeyScanner::setOutMuxBit(0x6, HIGH);

        uint8_t board_num = 0;

        // Find where this board is, how many boards there are and who the leader is
        xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
        for (int i = 0; i < 10; i++){
            if (board_detect_array[i] != 0){
                board_num++;
                if (board_detect_array[i] == board_ID){
                    __atomic_store_n(&board_number, i, __ATOMIC_RELAXED);
                }
            }
        }
        xSemaphoreGive(Board_Array_Mutex);

        __atomic_store_n(&number_of_boards, board_num, __ATOMIC_RELAXED);
        __atomic_store_n(&leader_number, (board_num / 2), __ATOMIC_RELAXED);

        if (__atomic_load_n(&leader_number, __ATOMIC_RELAXED) == __atomic_load_n(&board_number, __ATOMIC_RELAXED)){
            __atomic_store_n(&isLeader, true, __ATOMIC_RELAXED);
            Serial.println("Im leader!");
        } else {
            __atomic_store_n(&isLeader, false, __ATOMIC_RELAXED);
        }
        
        delay(300);
        Display::initialise_display(); // Allows power for the screen
        KeyScanner::OUT_ENABLE();
    }

    // Init Function
    void CAN_Class::initialise_CAN(){

        CAN_Init(false);

        CAN_RegisterRX_ISR(CAN_RX_ISR);
        CAN_RegisterTX_ISR(CAN_TX_ISR);

        setCANFilter(0x123,0x7ff);
        
        CAN_Start();

        Serial.println("ID: " + String(board_ID));
    }

    void CAN_Class::addMessageToQueue(uint8_t msg[8]){
        if (__atomic_load_n(&isLeader, __ATOMIC_RELAXED)){
            xQueueSendToBack(msgInQ, msg, 0);
        } else {
            xQueueSendToBack(msgOutQ, msg, 0);
        }
    }

    // Transmitter Task
    void CAN_Class::TX_Task(void* pvParameters){
        
        while (1) {
            uint8_t msgOut[8];
            while (1) {
                xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
                xSemaphoreTake(TX_Mutex, portMAX_DELAY);
                CAN_TX(0x123, msgOut);
            }
        }
    }

    // Recevier Task (To Be called by RTOS)
    void CAN_Class::RX_Task(void * pvParameters){

        uint8_t RX_MESSAGE[8] = {0};
        delayMicroseconds(100);

        KeyScanner::activate_handshakes(); // Now its able to Receive messages, activate handshakes.
        Serial.println(xQueueIsQueueEmptyFromISR(msgInQ));

        while (1) {
            xQueueReceive(msgInQ, RX_MESSAGE, portMAX_DELAY);

            bool local_leader = __atomic_load_n(&isLeader, __ATOMIC_RELAXED);

            // Leadership Reset Request
            if ((RX_MESSAGE[0] == 'D') && (RX_MESSAGE[1] == 'I') && (RX_MESSAGE[2] == 'A')){
                if (KeyScanner::get_OUT_EN()){
                    leadershipReset();
                }
            }
            
            // Key Press Operation
            else if (local_leader){
                if (RX_MESSAGE[0] == 'R' || RX_MESSAGE[0] == 'P'){
                    uint64_t logical_msg_val = uint64_t(0x1) << ( (RX_MESSAGE[1] * 12) + RX_MESSAGE[2] );

                    uint32_t logical_msg_val_lower = uint32_t(logical_msg_val & 0xFFFFFFFF);
                    uint32_t logical_msg_val_upper = uint32_t((logical_msg_val >> 32) & 0xFFFFFFFF);

                    // RX_Message will be the received Message

                    uint32_t local_lower_steps = __atomic_load_n(&Speaker::stepsActive0, __ATOMIC_RELAXED);
                    uint32_t local_upper_steps = __atomic_load_n(&Speaker::stepsActive32, __ATOMIC_RELAXED);

                    local_lower_steps = RX_MESSAGE[0] == 'P' ? (logical_msg_val_lower | local_lower_steps) : ~(logical_msg_val_lower | ~local_lower_steps);
                    local_upper_steps = RX_MESSAGE[0] == 'P' ? (logical_msg_val_upper | local_upper_steps) : ~(logical_msg_val_upper | ~local_upper_steps);

                    __atomic_store_n(&Speaker::stepsActive0, local_lower_steps, __ATOMIC_RELAXED);
                    __atomic_store_n(&Speaker::stepsActive32, local_upper_steps, __ATOMIC_RELAXED);

                } else if (RX_MESSAGE[0] == 'V'){
                    __atomic_store_n(&Speaker::volume, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Note
                } else if (RX_MESSAGE[0] == 'O'){
                    __atomic_store_n(&Speaker::octave, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Note
                } else if (RX_MESSAGE[0] == 'S'){
                    __atomic_store_n(&Speaker::shape, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Note
                }
            } else { // Connection Messages (In hopes that they all arrive in order lol)
                if (RX_MESSAGE[0] == 'F'){
                    Serial.println("FIN");
                    xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
                    board_detect_array[RX_MESSAGE[2]] = RX_MESSAGE[1]; 
                    xSemaphoreGive(Board_Array_Mutex);

                    getNewLeader();
                }
                if (RX_MESSAGE[0] == 'E'){
                    Serial.println("EAST");
                    xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
                    board_detect_array[RX_MESSAGE[2]] = RX_MESSAGE[1]; 
                    xSemaphoreGive(Board_Array_Mutex);

                    __atomic_store_n(&current_board, RX_MESSAGE[2]+1, __ATOMIC_RELAXED);

                }
            }
        }
    }