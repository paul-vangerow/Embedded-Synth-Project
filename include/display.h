#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <keyScanner.h>

class Display
{
public:

    static void displayUpdateTask(void * pvParameter);
    static void initialise_display();

private:

    //Display driver object
    static U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2;
    static const String notes [];

    //Output multiplexer bits
    static const int DEN_BIT = 3;
    static const int DRST_BIT = 4;
    static const int HKOW_BIT = 5;
    static const int HKOE_BIT = 6;
    Display();
};

#endif