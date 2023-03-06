#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <speaker.h>
#include <numeric>
#include <math.h>

// ------- Member Variable Definitions ------ //

    volatile int32_t Speaker::stepsActive = 0;
    volatile int32_t Speaker::volume = 0;
    volatile int32_t Speaker::shape = 0;

    const int32_t Speaker::stepSizes[12] = {51076057,
                                        54113197,
                                        57330935,
                                        60740010,
                                        64351799,
                                        68178356,
                                        72232452,
                                        76527617,
                                        81078186,
                                        85899346,
                                        91007187,
                                        96418756};

    const int Speaker::OUTL_PIN = A4;
    const int Speaker::OUTR_PIN = A3;

    uint8_t Speaker::sineTable[256];

// ------------ Member Functions ------------ //

    // Play the Sound 
    void Speaker::soundISR() {
        static int32_t phaseAcc[12] = {0}; // Init at 0
        int32_t total_vout = 0;
        uint8_t mag_div = 12;

        for (int i = 0; i < 12; i++){
            phaseAcc[i] += (((stepsActive >> i) & 0x1) != 0) ? stepSizes[i] : 0;
            uint8_t point_val = (phaseAcc[i] >> 24) + 128;

            switch(shape){
                case 1:
                    total_vout += sineTable[point_val];
                    mag_div = 3;
                    break;
                default:
                    total_vout += point_val;
                    mag_div = 12;
                    break;
            }
        }

        // Divide by notes to maintain note integrity

        total_vout = (total_vout / mag_div) >> ( 8 - volume );
        analogWrite(OUTR_PIN, total_vout);
    }

    void Speaker::createSineTable(){
        for (int i = 0 ; i < 256; i ++){
            sineTable[i] = abs( 128.0 * ( sin( (i/255.0) * 2.0 * PI ) ));
        }
    }

    void Speaker::initialise_speaker(){
        pinMode(OUTL_PIN, OUTPUT);
        pinMode(OUTR_PIN, OUTPUT);

        createSineTable();

        TIM_TypeDef *Instance = TIM1;
        HardwareTimer *sampleTimer = new HardwareTimer(Instance);

        sampleTimer->setOverflow(22000, HERTZ_FORMAT);
        sampleTimer->attachInterrupt(Speaker::soundISR);
        sampleTimer->resume();
    };

    // Speaker Update Task (To be Invoked by RTOS)

    void Speaker::speakerUpdateTask(void * pvParameter) {
        const TickType_t xFrequency = 30/portTICK_PERIOD_MS;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        while(1) {
            vTaskDelayUntil( &xLastWakeTime, xFrequency );

            int32_t local_stepsActive = 0;
            int32_t local_volume = 0;
            int32_t local_shape = 0;

            // Any Speaker - Key Interaction will take place within this semaphore

            KeyScanner::semaphoreTake();

            // A bit Janky, but shouldn't cause performance issues
            for (int i = 0 ; i<12; i++){
                local_stepsActive += KeyScanner::notes_pressed[i] << i;
            }

            local_volume = (KeyScanner::volume_nob / 2);
            local_shape = (KeyScanner::shape_nob / 2);

            KeyScanner::semaphoreGive();

            __atomic_store_n(&volume, local_volume, __ATOMIC_RELAXED); // Set Volume
            __atomic_store_n(&shape, local_shape, __ATOMIC_RELAXED); // Set Shape
            __atomic_store_n(&stepsActive, local_stepsActive, __ATOMIC_RELAXED); // Set Note
        }
    }