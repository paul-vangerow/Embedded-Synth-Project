#ifndef SPEAKER_H
#define SPEAKER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>

#include <keyScanner.h>

//#define TEST_SPEAKER

class Speaker
{
public:

    static void soundISR();
    static void initialise_speaker();

    static volatile uint32_t ticks;
    static volatile uint32_t debounce;
    
    static volatile uint32_t stepsActive32;
    static volatile uint32_t stepsActive0;
    
    static volatile int32_t volume;
    static volatile int32_t volume_store;
    static volatile int32_t shape;
    static volatile int32_t octave;

private:
    static int32_t stepSizes[];
    static int32_t boardAlignment[5][2];

    static const int OUTL_PIN;
    static const int OUTR_PIN;

    static DAC_HandleTypeDef hdac;
    static DMA_HandleTypeDef hdma;
    static int32_t sample_rate;
    static uint32_t total_vout;

    static int32_t sineTable[4096];

    static void createSineTable();
    static void generateStepSizes();
    Speaker();
};

#endif