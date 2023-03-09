#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>

#include <display.h>
#include <speaker.h>
#include <can_class.h>

void setup() {

  //Set pin directions
  pinMode(LED_BUILTIN, OUTPUT);

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  KeyScanner::initialise_keyScanner(); // Sets up Pin Modes
  Display::initialise_display(); // Allows power for the screen
  Speaker::initialise_speaker(); // Pin inits
  CAN_Class::initialise_CAN(); // CAN INIT

  // Create RTOS Tasks

  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
    KeyScanner::scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &scanKeysHandle /* Pointer to store the task handle */
  );

  TaskHandle_t displayUpdateHandle = NULL;
  xTaskCreate(
    Display::displayUpdateTask,		/* Function that implements the task */
    "displayUpdate",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle /* Pointer to store the task handle */
  );

  // TaskHandle_t speakerUpdateHandle = NULL;
  // xTaskCreate(
  //   Speaker::speakerUpdateTask,		/* Function that implements the task */
  //   "speakerUpdate",		/* Text name for the task */
  //   128,      		/* Stack size in words, not bytes */
  //   NULL,			/* Parameter passed into the task */
  //   4,			/* Task priority */
  //   &speakerUpdateHandle /* Pointer to store the task handle */
  // );

  TaskHandle_t RXTaskHandle = NULL;
  xTaskCreate(
    CAN_Class::RX_Task,		/* Function that implements the task */
    "RXTask",		/* Text name for the task */
    128,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &RXTaskHandle /* Pointer to store the task handle */
  );

  TaskHandle_t TXTaskHandle = NULL;
  xTaskCreate(
    CAN_Class::TX_Task,		/* Function that implements the task */
    "TXTask",		/* Text name for the task */
    128,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &TXTaskHandle /* Pointer to store the task handle */
  );

  vTaskStartScheduler();
}

void loop() {
}