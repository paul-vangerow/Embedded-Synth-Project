#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>

#include <keyScanner.h>

class CAN_Class
{
public:

    static volatile uint8_t CAN_Class::TX_Message[8];

private:

    CAN_Class();
};

#endif