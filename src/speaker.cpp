#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <speaker.h>
#include <numeric>
#include <math.h>

// ------- Member Variable Definitions ------ //

    volatile int32_t Speaker::stepsActive = 0;
    volatile int32_t Speaker::volume = 6;
    volatile int32_t Speaker::shape = 0;
    volatile int32_t Speaker::octave = 4;

    // Steps for 12 Notes.
    int32_t Speaker::stepSizes[96] = {0};

    //  = {51076057, 54113197, 57330935, 60740010, 64351799, 68178356,
    //                                     72232452, 76527617, 81078186, 85899346, 91007187, 96418756}

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
            phaseAcc[i] += (((stepsActive >> i) & 0x1) != 0) ? stepSizes[ (12*octave) + i] : -1*phaseAcc[i]; // Reset accumulators (Randomly increases amplitude otherwise)
            
            uint8_t point_val = (phaseAcc[i] >> 24) + 128;

            // Sound Wave Shape
            switch(shape){
                case 1:
                    total_vout += sineTable[point_val]; // Sine Wave
                    mag_div = 3;
                    break;
                case 2:
                    total_vout += (point_val > 128) ? 255 : 0; // Square Wave 
                    mag_div = 12;
                    break;
                default:
                    total_vout += point_val; // Sawtooth Wave
                    mag_div = 12;
                    break;
            }
        }

        // Divide by notes to maintain note integrity

        total_vout = (total_vout / mag_div) >> ( 8 - volume );
        analogWrite(OUTR_PIN, total_vout);
    }

    void Speaker::createSineTable(){

        // ABS value of Sine wave 
        for (int i = 0 ; i < 256; i ++){
            sineTable[i] = abs(128.0 * ( sin( (i/255.0) * PI ) ));
        }
    }

    void Speaker::generateStepSizes(){

        // Generate Step Sizes for all keys (Actually will end up being more than 7.25 octaves)

        // 440Hz at Key 58
        for (int i = 0; i < 96; i++){
            stepSizes[i] = (powl(2.0, 32.0) / 22000.0) * (powl(2.0, ( (i-46.0)/12.0 ) ) * 440.0);
        }
    }

    void Speaker::initialise_speaker(){
        pinMode(OUTL_PIN, OUTPUT);
        pinMode(OUTR_PIN, OUTPUT);

        generateStepSizes();
        createSineTable();

        TIM_TypeDef *Instance = TIM1;
        HardwareTimer *sampleTimer = new HardwareTimer(Instance);

        sampleTimer->setOverflow(22000, HERTZ_FORMAT);
        sampleTimer->attachInterrupt(Speaker::soundISR);
        sampleTimer->resume();
    }