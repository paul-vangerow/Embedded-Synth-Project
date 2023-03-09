#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <display.h>

// ------ Member Variable Definitions ------ //

    U8G2_SSD1305_128X32_NONAME_F_HW_I2C Display::u8g2 = U8G2_SSD1305_128X32_NONAME_F_HW_I2C(U8G2_R0);

    const String Display::notes [] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

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
            vTaskDelayUntil( &xLastWakeTime, xFrequency );
            //Update display
            u8g2.clearBuffer();         // clear the internal memory
            u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
            String Keys = "";
            int Vol = 0;
            int Shape = 0;
            int Oct = 0;

            KeyScanner::semaphoreTake();
            
            for (int i = 0 ; i<12; i++){
                Keys += KeyScanner::notes_pressed[i] ? notes[i] : " ";
                Keys += " ";
            }
            Vol = (KeyScanner::volume_nob / 2) ;
            Shape = (KeyScanner::shape_nob / 2) ;
            Oct = (KeyScanner::octave_nob / 2) ;

            KeyScanner::semaphoreGive();

            CAN_Class::TX_Message[0];

            u8g2.setCursor(100,10);
            u8g2.print(Vol);
            u8g2.setCursor(110,10);
            u8g2.print(Oct + 1);
            u8g2.setCursor(100,20);
            u8g2.print(Shape);
            u8g2.setCursor(2,30);
            u8g2.print(Keys);

            u8g2.sendBuffer();          // transfer internal memory to the display

            //Toggle LED
            digitalToggle(LED_BUILTIN);
        }
    }