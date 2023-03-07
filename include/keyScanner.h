#ifndef KEYSCANNER_H
#define KEYSCANNER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>

// Just Defines everything the object contains.

class KeyScanner
{
private:
    static const int ROW_SEL[3];
    static const int REN_PIN;
    static const int COL_IN[4];
    static const int OUT_PIN;
    static const int JOYXY_PIN[2];

    static SemaphoreHandle_t keyArrayMutex;

    static uint8_t prev_row3_state;
    static uint8_t prev_row4_state;

    static bool vol_dir;
    static bool shape_dir;
    static bool oct_dir;
    
    static uint8_t readCols();
    static void setRow(uint8_t rowIdx);
    static void getNobChange(uint8_t curr_state, uint8_t prev_state, bool* cur_dir, volatile int32_t* val, uint8_t max, uint8_t min);

    KeyScanner(); // Private Constructor (Cannot Initialise As an Object.)

public:
    static volatile uint8_t keyArray[7];

    static volatile bool notes_pressed[12];
    static volatile int32_t volume_nob;
    static volatile int32_t shape_nob;
    static volatile int32_t octave_nob;

    static void initialise_keyScanner();
    static void semaphoreTake();
    static void semaphoreGive();
    static void setOutMuxBit(const uint8_t bitIdx, const bool value);
    static void scanKeysTask(void * pvParameters);
};

#endif