#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <ES_CAN.h>

#include <keyScanner.h>
#include <speaker.h>

class CAN_Class
{
public:
    static void initialise_CAN();
    static void RX_Task(void * pvParameters);
    static void TX_Task(void * pvParameters);

    static void addMessageToQueue(uint8_t msg[8]);
    static volatile bool isLeader;

    static void semaphoreTake();
    static void semaphoreGive();

private:
    static void CAN_RX_ISR (void);
    static void CAN_TX_ISR (void);

    static QueueHandle_t msgInQ;
    static QueueHandle_t msgOutQ;
    static SemaphoreHandle_t TX_Mutex;

    CAN_Class();
};

#endif