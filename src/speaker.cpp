#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <speaker.h>
#include <numeric>

// ------- Member Variable Definitions ------ //

    volatile int32_t Speaker::stepsActive = 0;
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
        static int32_t phaseAcc[12] = {0}; // Init at 0
        int32_t Vout[12] = {0};
        int num_of_notes = 0;
        for (int i = 0; i < 12; i++){
            uint8_t tmp = (((stepsActive >> i) & 0x1) != 0);
            phaseAcc[i] += tmp ? stepSizes[i] : 0;
            num_of_notes += tmp ? 1 : 0;
            Vout[i] = (phaseAcc[i] >> 24) + 128;
        }

        int32_t total_out = (num_of_notes != 0) ? (std::accumulate(std::begin(Vout), std::end(Vout), 0) / num_of_notes) : 0;

        total_out = total_out >> ( 8 - volume );
        analogWrite(OUTR_PIN, total_out);
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

            int32_t local_stepsActive = 0;
            int32_t local_volume = 0;

            // Any Speaker - Key Interaction will take place within this semaphore

            KeyScanner::semaphoreTake();

            // A bit Janky, but shouldn't cause performance issues

            
            
            for (int i = 0 ; i<12; i++){
                local_stepsActive += KeyScanner::notes_pressed[i] << i;
            }

            local_volume = KeyScanner::volume_nob;

            KeyScanner::semaphoreGive();

            __atomic_store_n(&volume, local_volume, __ATOMIC_RELAXED); // Set Volume
            __atomic_store_n(&stepsActive, local_stepsActive, __ATOMIC_RELAXED); // Set Note
        }
    }