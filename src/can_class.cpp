#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <can_class.h>

// ------- Member Variable Definitions ------ //

    volatile uint8_t CAN_Class::TX_Message[8] = {0};

// ------------ Member Functions ------------ //

