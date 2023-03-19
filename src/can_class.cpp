#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <can_class.h>
#include <exception>


// ------- Member Variable Definitions ------ //

    // Mutexes
    SemaphoreHandle_t CAN_Class::TX_Mutex = xSemaphoreCreateCounting(3,3); 
    SemaphoreHandle_t CAN_Class::Board_Array_Mutex = xSemaphoreCreateMutex();

    // Input and Out Message Queues
    QueueHandle_t CAN_Class::msgInQ = xQueueCreate(36,8);
    QueueHandle_t CAN_Class::msgOutQ = xQueueCreate(36,8);

    // Handshaking
    volatile uint8_t CAN_Class::board_detect_array[10] = {0}; // Array to store IDs of every board
    volatile uint8_t CAN_Class::current_board = 0; // Most recent Board Received
    bool CAN_Class::inList = false; // Board has sent its east request (Ensures each Board is only registered once)

    // Multi-Board Operation
    volatile uint8_t CAN_Class::board_number = 0;
    volatile uint8_t CAN_Class::leader_number = 0;
    volatile uint8_t CAN_Class::number_of_boards = 0;
    volatile bool CAN_Class::isLeader = false; // At Base Assume Leadership

    // Convert ID from a 32 bit value to an 8 bit one (Hopefully this should work with all of them?)
    volatile uint8_t CAN_Class::board_ID = CAN_Class::convert_8bit(HAL_GetUIDw0());

// ------------ Member Functions ------------ //

    // Function for converting a 32 bit value into an 8 bit one (Convert ID into one we can send.)
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

    // ISR for Receiving, Triggers everytime there is something in the CAN Inbox.
    void CAN_Class::CAN_RX_ISR (void) {
        uint8_t RX_Message_ISR[8];
        uint32_t ID = 0x123;
        CAN_RX(ID, RX_Message_ISR);
        xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
    }

    // ISR for Transmitting, Triggers everytime an Outbox is Free
    void CAN_Class::CAN_TX_ISR (void) {
        xSemaphoreGiveFromISR(TX_Mutex, NULL);
    }

    // When there is a change detected in EAST/WEST, Reconfirm leading board.
    void CAN_Class::reconfirm_leader(bool new_east_west[2]){

        // Nothing Connected (Make board its own leader)
        if ( (new_east_west[0] | new_east_west[1]) == 0){
            __atomic_store_n(&isLeader, true, __ATOMIC_RELAXED); // Set Board to New Leader
            delayMicroseconds(100);
            Display::initialise_display(); // Reinitialise the Screen
            KeyScanner::OUT_ENABLE(); // New Leader Found, Continue operations
        } 
        // Something else is connected, determine overall leadership
        else {    
            leadershipReset(); // Reset all Handshaking Parameters

            // Send Message to all other boards, telling them to initiate the Handshaking Process
            uint8_t RX_MESSAGE[8] = {'D', 'I', 'A'};
            addMessageToQueue(RX_MESSAGE); 
        }
        KeyScanner::activate_handshakes(); // [NEEDS TO GET MOVED. Currently Causing a Double Handshake]
    }

    // Reset Leadership and Switch KeyScanner to check for changes in Neighbours
    void CAN_Class::leadershipReset(){
        Serial.println("[3] Resetting Leadership");

        // A module has been disconnected.
        KeyScanner::OUT_DISABLE();
        __atomic_store_n(&isLeader, false, __ATOMIC_RELAXED);
        __atomic_store_n(&current_board, 0, __ATOMIC_RELAXED);

        xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
        for (int i = 0; i < 10; i++){
            board_detect_array[0] = 0;
        }
        xSemaphoreGive(Board_Array_Mutex);

        __atomic_store_n(&inList, false, __ATOMIC_RELAXED);
    }

    // If WestMost board not in the list, send a message with ID
    void CAN_Class::sendEastMessage(){
        if (!__atomic_load_n(&inList, __ATOMIC_RELAXED)){ 
            __atomic_store_n(&inList, true, __ATOMIC_RELAXED); // Prevent Duplicate Boards

            xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
            board_detect_array[__atomic_load_n(&CAN_Class::current_board, __ATOMIC_RELAXED)] = board_ID; // Add Board to List
            xSemaphoreGive(Board_Array_Mutex);

            // Send East Message with Board ID and which position it's in
            uint8_t RX_MESSAGE[8] = {'E', board_ID, __atomic_load_n(&CAN_Class::current_board, __ATOMIC_RELAXED)};
            addMessageToQueue(RX_MESSAGE);

            delayMicroseconds(300);
            KeyScanner::setOutMuxBit(0x6, 0); // Set East Handshake to 0 (Tell next Board that it's next)
        }
    }

    // Reached the EASTMOST board, send message to conclude Handshaking Process
    void CAN_Class::sendFinMessage(){
        if (!__atomic_load_n(&inList, __ATOMIC_RELAXED)){
            __atomic_store_n(&inList, true, __ATOMIC_RELAXED); // Prevent Duplicate Boards
            
            xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
            board_detect_array[__atomic_load_n(&CAN_Class::current_board, __ATOMIC_RELAXED)] = board_ID; // Add Board to List
            xSemaphoreGive(Board_Array_Mutex);

            // Send Final Message with Board ID and which position it's in
            uint8_t RX_MESSAGE[8] = {'F', board_ID,  __atomic_load_n(&CAN_Class::current_board, __ATOMIC_RELAXED)};
            addMessageToQueue(RX_MESSAGE);

            delayMicroseconds(10);
            getNewLeader(); // Analyse Board List and determine Board Position as well as Leader Position
        }
    }

    // Acquire new Leader from List.
    void CAN_Class::getNewLeader(){

        Serial.println("[DEBUG] Getting New Leader");

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
                    Serial.println("[DEBUG] Board Number: " + String(i));
                }
            }
        }
        xSemaphoreGive(Board_Array_Mutex);

        __atomic_store_n(&number_of_boards, board_num, __ATOMIC_RELAXED);
        __atomic_store_n(&leader_number, (board_num / 2), __ATOMIC_RELAXED);

        if (__atomic_load_n(&leader_number, __ATOMIC_RELAXED) == __atomic_load_n(&board_number, __ATOMIC_RELAXED)){
            __atomic_store_n(&isLeader, true, __ATOMIC_RELAXED);
        } else {
            __atomic_store_n(&isLeader, false, __ATOMIC_RELAXED);
        }
        Display::initialise_display(); // Reset Screen

        delay(300);
        KeyScanner::OUT_ENABLE(); // Reactivate KeyScanner
    }

    // Initialisation Function for the CAN
    void CAN_Class::initialise_CAN(){

        CAN_Init(false);

        CAN_RegisterRX_ISR(CAN_RX_ISR);
        CAN_RegisterTX_ISR(CAN_TX_ISR);

        setCANFilter(0x123,0x7ff);
        Serial.println("[DEBUG] Board ID: " + String(board_ID));
        
        CAN_Start();
    }

    // Used to add Messages to the TX Queue. Add to Own Receive Queue if Leader (Play your own Notes.)
    void CAN_Class::addMessageToQueue(uint8_t msg[8]){
        if (__atomic_load_n(&isLeader, __ATOMIC_RELAXED)){
            xQueueSendToBack(msgInQ, msg, 0);
        } else {
            xQueueSendToBack(msgOutQ, msg, 0);
        }
    }

    // Wait until an Outbox is Free, transmit Messages
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

        while (1) {
            xQueueReceive(msgInQ, RX_MESSAGE, portMAX_DELAY); // Wait Until A message has been received

            bool local_leader = __atomic_load_n(&isLeader, __ATOMIC_RELAXED); // Local Variables to prevent cluttering of Atomic accesses

            uint32_t  ticks = __atomic_load_n(&Speaker::ticks, __ATOMIC_RELAXED);

            __atomic_store_n(&Speaker::ticks, ticks + 1, __ATOMIC_RELAXED);

            // Leadership Reset Request -- Another Board detected a Change, reinitiate Handshaking
            if ((RX_MESSAGE[0] == 'D') && (RX_MESSAGE[1] == 'I') && (RX_MESSAGE[2] == 'A')){
                if (KeyScanner::get_OUT_EN()){
                    leadershipReset();
                }
            }
            
            // Key Press Operation
            else if (local_leader){
                if (RX_MESSAGE[0] == 'R' || RX_MESSAGE[0] == 'P'){
                   
                    
                    // Seperate Upper and Lower as we are on a 32bit system
                    uint64_t logical_msg_val = uint64_t(0x1) << ( (RX_MESSAGE[1] * 12) + RX_MESSAGE[2] );

                    uint32_t logical_msg_val_lower = uint32_t(logical_msg_val & 0xFFFFFFFF);
                    uint32_t logical_msg_val_upper = uint32_t((logical_msg_val >> 32) & 0xFFFFFFFF);

                    uint32_t local_lower_steps = __atomic_load_n(&Speaker::stepsActive0, __ATOMIC_RELAXED);
                    uint32_t local_upper_steps = __atomic_load_n(&Speaker::stepsActive32, __ATOMIC_RELAXED);

                    local_lower_steps = RX_MESSAGE[0] == 'P' ? (logical_msg_val_lower | local_lower_steps) : ~(logical_msg_val_lower | ~local_lower_steps);
                    local_upper_steps = RX_MESSAGE[0] == 'P' ? (logical_msg_val_upper | local_upper_steps) : ~(logical_msg_val_upper | ~local_upper_steps);

                    __atomic_store_n(&Speaker::stepsActive0, local_lower_steps, __ATOMIC_RELAXED);
                    __atomic_store_n(&Speaker::stepsActive32, local_upper_steps, __ATOMIC_RELAXED);

                } else if (RX_MESSAGE[0] == 'V'){
                    __atomic_store_n(&Speaker::volume, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Volume value
                } else if (RX_MESSAGE[0] == 'O'){
                    __atomic_store_n(&Speaker::octave, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Octave Value
                } else if (RX_MESSAGE[0] == 'S'){
                    __atomic_store_n(&Speaker::shape, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Shape Value
                } else if (RX_MESSAGE[0] =='M'){   
                    if (__atomic_load_n(&Speaker::ticks, __ATOMIC_RELAXED ) - __atomic_load_n(&Speaker::debounce, __ATOMIC_RELAXED) > 14){
                        __atomic_store_n(&Speaker::debounce, ticks, __ATOMIC_RELAXED);

                        int32_t local_vol = __atomic_load_n(&Speaker::volume, __ATOMIC_RELAXED);
                    
                        if ( local_vol == 0 ) {
                            __atomic_store_n(&Speaker::volume, Speaker::volume_store, __ATOMIC_RELAXED);
                        }
                        else {
                            __atomic_store_n(&Speaker::volume, 0, __ATOMIC_RELAXED);
                            Speaker::volume_store = local_vol;
                        }
                    }
                }
            } else {
                // Finish Message -- Tells board Handshaking is complete
                if (RX_MESSAGE[0] == 'F'){

                    xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
                    board_detect_array[RX_MESSAGE[2]] = RX_MESSAGE[1]; 
                    xSemaphoreGive(Board_Array_Mutex);

                    getNewLeader();
                }
                // East Message -- Tells board about other Boards.
                if (RX_MESSAGE[0] == 'E'){
                    uint8_t value = RX_MESSAGE[2] + 1;
                    __atomic_store_n(&current_board, value, __ATOMIC_RELAXED); // Increment Next Board Index

                    xSemaphoreTake(Board_Array_Mutex, portMAX_DELAY);
                    board_detect_array[RX_MESSAGE[2]] = RX_MESSAGE[1]; 
                    xSemaphoreGive(Board_Array_Mutex);
                }
            }
        }
    }