#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <display.h>


// ------ Member Variable Definitions ------ //

    U8G2_SSD1305_128X32_NONAME_F_HW_I2C Display::u8g2 = U8G2_SSD1305_128X32_NONAME_F_HW_I2C(U8G2_R0);

    //volatile int32_t Display::can_test = 0;

    const String Display::notes [] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    volatile uint16_t Display::localNotes = 4095;

    uint8_t Display::animDistance = 20;

    const uint8_t Display::bmp[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xE0, 0x01, 
  0x00, 0x1E, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 
  0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0xF1, 0x01, 
  0x00, 0xF1, 0x01, 0xF0, 0xF9, 0x01, 0xF8, 0xF9, 0x01, 0xF8, 0xA1, 0x00, 
  0xF8, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  };

// --------------- Functions ---------------- //

    void Display::initialise_display() {
        //Initialise display

        KeyScanner::setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
        delayMicroseconds(2);
        KeyScanner::setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
        u8g2.begin();
        KeyScanner::setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply
    }

    // Display Update Task (To be Invoked by RTOS)

    void Display::displayUpdateTask(void * pvParameter) {
        const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
        TickType_t xLastWakeTime = xTaskGetTickCount();

        while(1) {
            #ifndef TEST_DISPLAY
            vTaskDelayUntil( &xLastWakeTime, xFrequency );
            #endif

            
            //Update display
            u8g2.clearBuffer();         // clear the internal memory
            u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

            String Keys = "";

            // Only Leader Displays Parameters
            if (__atomic_load_n(&CAN_Class::isLeader, __ATOMIC_RELAXED)){
                // Load Values 
                int Vol = __atomic_load_n(&Speaker::volume, __ATOMIC_RELAXED);
                int Shape = __atomic_load_n(&Speaker::shape, __ATOMIC_RELAXED);;
                int Oct = __atomic_load_n(&Speaker::octave, __ATOMIC_RELAXED);;



                // Print to Screen
                u8g2.setCursor(100,10);
                u8g2.print(Vol);
                u8g2.setCursor(110,10);
                u8g2.print(Oct + 1);
                u8g2.setCursor(100,20);
                u8g2.print(Shape);
            }
            uint32_t local_notes = __atomic_load_n(&localNotes, __ATOMIC_RELAXED);
            for (uint8_t i=0; i < 12; i++)
            {
                if ((local_notes>>i) &1)
                {
                    Keys += notes[i] + " ";
                }
            }
            u8g2.setCursor(2,30);
            u8g2.print(Keys);
            //u8g2.print(animDistance);
            u8g2.setCursor(2,10);
            u8g2.drawXBMP(animDistance, 1, 20, 20, bmp);
            animDistance += 5;
            if (animDistance > 80) {animDistance = 5;}
            u8g2.sendBuffer();          // transfer internal memory to the display

            //Toggle LED
            digitalToggle(LED_BUILTIN);
            #ifdef TEST_DISPLAY
                break;
            #endif

        }
    }