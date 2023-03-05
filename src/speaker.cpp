#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <speaker.h>

// ------- Member Variable Definitions ------ //

    volatile int32_t Speaker::currentStepSize = 0;
    volatile int32_t Speaker::volume = 4;

    const int32_t Speaker::stepSizes[] = {51076057,
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

// ------------ Member Functions ------------ //

    void Speaker::soundISR() {
        static int32_t phaseAcc = 0;
        phaseAcc += currentStepSize;
        int32_t Vout = phaseAcc >> 24;
        Vout = Vout >> ( 8 - volume );
        analogWrite(OUTR_PIN, Vout + 128);
    }

    void Speaker::initialise_speaker(){
        pinMode(OUTL_PIN, OUTPUT);
        pinMode(OUTR_PIN, OUTPUT);

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

            int32_t local_stepSize = 0;
            int32_t local_volume = 0;

            // Any Speaker - Key Interaction will take place within this semaphore

            KeyScanner::semaphoreTake();
            
            for (int i = 0 ; i<12; i++){
                if (KeyScanner::notes_pressed[i]){
                    local_stepSize = stepSizes[i];
                }
            }

            local_volume = KeyScanner::volume_nob;

            KeyScanner::semaphoreGive();

            __atomic_store_n(&volume, local_volume, __ATOMIC_RELAXED); // Set Volume
            __atomic_store_n(&currentStepSize, local_stepSize, __ATOMIC_RELAXED); // Set Note
        }
    }