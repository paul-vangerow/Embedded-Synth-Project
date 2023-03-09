#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <can_class.h>

// ------- Member Variable Definitions ------ //

    SemaphoreHandle_t CAN_Class::TX_Mutex = xSemaphoreCreateCounting(3,3);
    QueueHandle_t CAN_Class::msgInQ = xQueueCreate(36,8);
    QueueHandle_t CAN_Class::msgOutQ = xQueueCreate(36,8);
    volatile bool CAN_Class::isLeader = true;

// ------------ Member Functions ------------ //

    void CAN_Class::CAN_RX_ISR (void) {
        uint8_t RX_Message_ISR[8];
        uint32_t ID = 0x123;
        CAN_RX(ID, RX_Message_ISR);
        xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
    }

    void CAN_Class::CAN_TX_ISR (void) {
        xSemaphoreGiveFromISR(TX_Mutex, NULL);
    }

    // Init Function
    void CAN_Class::initialise_CAN(){
        CAN_Init(isLeader);

        CAN_RegisterRX_ISR(CAN_RX_ISR);
        CAN_RegisterTX_ISR(CAN_TX_ISR);

        setCANFilter(0x123,0x7ff);
        
        CAN_Start();
    }

    void CAN_Class::addMessageToQueue(uint8_t msg[8]){
        xQueueSendToBack(msgOutQ, msg, 0);
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

        while (1) {
            xQueueReceive(msgInQ, RX_MESSAGE, portMAX_DELAY);
            
            // Key Press Operation
            if (isLeader){
                if (RX_MESSAGE[0] == 'R' || RX_MESSAGE[0] == 'P'){
                    int32_t msg_val = 0x1 << RX_MESSAGE[2];

                    // RX_Message will be the received Message
                    int32_t local_steps_active = __atomic_load_n(&Speaker::stepsActive, __ATOMIC_RELAXED); //Get Note

                    local_steps_active = RX_MESSAGE[0] == 'P' ? (msg_val | local_steps_active) : ~(msg_val | ~local_steps_active);

                    __atomic_store_n(&Speaker::stepsActive, local_steps_active, __ATOMIC_RELAXED); // Store Note
                } else if (RX_MESSAGE[0] == 'V'){
                    __atomic_store_n(&Speaker::volume, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Note
                } else if (RX_MESSAGE[0] == 'O'){
                    __atomic_store_n(&Speaker::octave, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Note
                } else if (RX_MESSAGE[0] == 'S'){
                    __atomic_store_n(&Speaker::shape, RX_MESSAGE[1], __ATOMIC_RELAXED); // Store Note
                }
            }
        }
    }