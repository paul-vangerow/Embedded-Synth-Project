#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <ES_CAN.h>

#include <keyScanner.h>
#include <speaker.h>
#include <display.h>

//#define TEST_CAN_RX
//#define TEST_CAN_TX


class CAN_Class
{
public:
    static void initialise_CAN(bool loopback);
    static void RX_Task(void * pvParameters);
    static void TX_Task(void * pvParameters);

    static void addMessageToQueue(uint8_t msg[8]);

    static volatile bool isLeader;
    static volatile uint8_t current_board;
    static volatile uint8_t board_detect_array[10]; // Up to 10 Boards (Excessive but meh)
    static volatile uint8_t board_ID;
    static volatile uint8_t board_number;
    static volatile uint8_t leader_number;
    static volatile uint8_t number_of_boards;
    
    static bool inList;

    static void sendEastMessage();
    static void sendFinMessage();

    static void reconfirm_leader(bool new_east_west[2]);

    static SemaphoreHandle_t Board_Array_Mutex;
    static SemaphoreHandle_t Denial;

private:
    static void CAN_RX_ISR (void);
    static void CAN_TX_ISR (void);

    static uint8_t convert_8bit(int32_t val);
    
    static void getNewLeader();

    static void leadershipReset();
    

    static QueueHandle_t msgInQ;
    static QueueHandle_t msgOutQ;
    static SemaphoreHandle_t TX_Mutex;
    

    CAN_Class();
};

#endif