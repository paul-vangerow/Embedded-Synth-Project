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

private:

    static volatile int32_t currentStepSize;
    static volatile int32_t volume;
    static const int32_t stepSizes[];

    static const int OUTL_PIN;
    static const int OUTR_PIN;

    Speaker();
};

#endif