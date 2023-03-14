#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <speaker.h>
#include <numeric>
#include <math.h>

// ------- Member Variable Definitions ------ //

    // Track Key Presses
    volatile uint32_t Speaker::stepsActive32 = 0;
    volatile uint32_t Speaker::stepsActive0 = 0;

    // Playing Parameters
    volatile int32_t Speaker::volume = 6;
    volatile int32_t Speaker::shape = 0;
    volatile int32_t Speaker::octave = 4;

    int32_t Speaker::sample_rate = 22000;

    // Steps for 96 Notes.
    int32_t Speaker::stepSizes[96] = {0};
    int32_t Speaker::sineTable[4096];

    // Output Value
    uint32_t Speaker::total_vout = 0;

    // Octaves allowed                         1-8       2-8        2-7      3-7      3-6
    int32_t Speaker::boardAlignment[5][2] = {{24, 36}, {12, 36}, {12, 48}, {0, 48}, {0, 60}}; 

    // Pins / Other
    const int Speaker::OUTL_PIN = A4;
    const int Speaker::OUTR_PIN = A3;   

    DAC_HandleTypeDef Speaker::hdac; 
    DMA_HandleTypeDef Speaker::hdma;

// ------------ Member Functions ------------ //

    // Play the Sound 
    void Speaker::soundISR() {
        static int32_t phaseAcc[60] = {0}; // Init at 0

        uint8_t mag_div = 12;

        uint8_t local_number_boards = __atomic_load_n(&CAN_Class::number_of_boards, __ATOMIC_RELAXED);
        uint32_t local_lower_steps = __atomic_load_n(&stepsActive0, __ATOMIC_RELAXED);
        uint32_t local_upper_steps = __atomic_load_n(&stepsActive32, __ATOMIC_RELAXED);

        for (uint8_t i = boardAlignment[local_number_boards][0]; i < boardAlignment[local_number_boards][1]; i++){

            // Cant use Mutexes in ISR() and 64bit values are not atomic on 32bit processes.
            if (i < 32){
                phaseAcc[i] += (((stepsActive0 >> i) & 0x1) != 0) ? stepSizes[ ( (12*octave) - 24) + i] : -1*phaseAcc[i]; // Reset accumulators (Randomly increases amplitude otherwise)
            } else {
                phaseAcc[i] += (((stepsActive32 >> (i-32) ) & 0x1) != 0) ? stepSizes[ ( (12*octave) - 24) + i] : -1*phaseAcc[i]; // Reset accumulators (Randomly increases amplitude otherwise)
            }

            int32_t point_val = (phaseAcc[i] >> 20) + 2048;

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
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, total_vout);
    }

    void Speaker::createSineTable(){

        // ABS value of Sine wave 
        for (int i = 0 ; i < 4096; i ++){
            sineTable[i] = abs( 2048 * ( sin( (i/4096.0) * PI ) ));
        }
    }

    void Speaker::generateStepSizes(){
        // Generate Step Sizes for all keys (Actually will end up being more than 7.25 octaves)

        // 440Hz at Key 58
        for (int i = 0; i < 96; i++){
            stepSizes[i] = (powl(2.0, 32.0) / sample_rate) * (powl(2.0, ( (i-46.0)/12.0 ) ) * 440.0);
        }
    }

    void Speaker::initialise_speaker(){

        // Pins
        pinMode(OUTL_PIN, OUTPUT);
        pinMode(OUTR_PIN, OUTPUT);

        // Create step sizes / tables
        generateStepSizes();
        createSineTable();

        __HAL_RCC_DAC1_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();
        HAL_Init();

        // Setup DAC
        hdac.Instance = DAC;
        HAL_DAC_Init(&hdac);

        // Configure DAC Channel
        DAC_ChannelConfTypeDef sConfig = {0};
        sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
        sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
        HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

        // Configure DMA
        hdma.Instance = DMA1_Channel3;
        hdma.Init.Request = DMA_REQUEST_6;
        hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma.Init.MemInc = DMA_MINC_DISABLE;
        hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma.Init.Mode = DMA_CIRCULAR;
        hdma.Init.Priority = DMA_PRIORITY_HIGH;
        HAL_DMA_Init(&hdma);

        Serial.println(HAL_DMA_Start(&hdma, (uint32_t)&total_vout, (uint32_t)&hdac.Instance->DHR12R1, 32));

        HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);

        // Setup ISR
        TIM_TypeDef *Instance = TIM1;
        HardwareTimer *sampleTimer = new HardwareTimer(Instance);

        sampleTimer->setOverflow(sample_rate, HERTZ_FORMAT);
        sampleTimer->attachInterrupt(Speaker::soundISR);
        sampleTimer->resume();
    }