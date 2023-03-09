#ifndef SPEAKER_H
#define SPEAKER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>

#include <keyScanner.h>

class Speaker
{
public:

    static void soundISR();
    static void initialise_speaker();
    static void speakerUpdateTask(void * pvParameter);

    static volatile int32_t stepsActive;
    static volatile int32_t volume;
    static volatile int32_t shape;
    static volatile int32_t octave;

private:
    static int32_t stepSizes[];

    static const int OUTL_PIN;
    static const int OUTR_PIN;

    static uint8_t sineTable[256];

    static void createSineTable();
    static void generateStepSizes();
    Speaker();
};

#endif