#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>

#include <display.h>
#include <speaker.h>
#include <can_class.h>

//#define DISABLE_THREADS


void setup() {
  //Set pin directions
  pinMode(LED_BUILTIN, OUTPUT);

  //Initialise UART
  Serial.begin(9600);
  delay(2000);
  Serial.println("Hello World");



  // Create RTOS Tasks
  #ifndef DISABLE_THREADS

  KeyScanner::initialise_keyScanner(); // Sets up Pin Modes
  Speaker::initialise_speaker(); // Pin inits
  CAN_Class::initialise_CAN();

  TaskHandle_t RXTaskHandle = NULL;
  xTaskCreate(
    CAN_Class::RX_Task,		/* Function that implements the task */
    "RXTask",		/* Text name for the task */
    1024,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    4,			/* Task priority */
    &RXTaskHandle /* Pointer to store the task handle */
  );

  TaskHandle_t TXTaskHandle = NULL;
  xTaskCreate(
    CAN_Class::TX_Task,		/* Function that implements the task */
    "TXTask",		/* Text name for the task */
    1024,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &TXTaskHandle /* Pointer to store the task handle */
  );

  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
    KeyScanner::scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    1024,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &scanKeysHandle /* Pointer to store the task handle */
  );

  TaskHandle_t displayUpdateHandle = NULL;
  xTaskCreate(
    Display::displayUpdateTask,		/* Function that implements the task */
    "displayUpdate",		/* Text name for the task */
    1024,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle /* Pointer to store the task handle */
  );

  vTaskStartScheduler();
  #endif

  //Timing Analysis Tests
  #ifdef TEST_SCANKEYS
  KeyScanner::initialise_keyScanner(); // Sets up Pin Modes
	uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		KeyScanner::scanKeysTask(nullptr);
	}
	Serial.println(micros()-startTime);
	while(1);
  #endif

  #ifdef TEST_DISPLAY
  Serial.println("testing display");
  __atomic_store_n(&Display::localNotes, 0xFFFF, __ATOMIC_RELAXED);
  
  Display::initialise_display();
	uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		Display::displayUpdateTask(nullptr);
	}
	Serial.println(micros()-startTime);
	while(1);
  #endif

  #ifdef TEST_SPEAKER
  Serial.println("testing speaker");
  Speaker::initialise_speaker();
  __atomic_store_n(&Speaker::stepsActive32, 0xFFFFFFFF, __ATOMIC_RELAXED);
  __atomic_store_n(&Speaker::stepsActive0, 0xFFFFFFFF, __ATOMIC_RELAXED);
	uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		Speaker::soundISR();
	}
	Serial.println(micros()-startTime);
	while(1);
  #endif

  #ifdef TEST_CAN_RX
  Serial.println("testing can rx");
  //not sure how to preload data to the RX queue, it does it inside ISR only?
  for (uint16_t i = 0; i < 32; i++)
  {
    //need message format to preload messages
    
  }
  CAN_Class::initialise_CAN();
	uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		CAN_Class::RX_Task(nullptr);
	}
	Serial.println(micros()-startTime);
	while(1);
  #endif

  #ifdef TEST_CAN_TX
  for (uint16_t i = 0; i < 32; i++)
  {
    //need message format to preload messages
    CAN_Class::addMessageToQueue();
  }
  CAN_Class::initialise_CAN();
	uint32_t startTime = micros();
	for (int iter = 0; iter < 32; iter++) {
		CAN_Class::TX_Task(nullptr);
	}
	Serial.println(micros()-startTime);
	while(1);
  #endif
  Serial.println("threads disabled and no tests");
}

void loop() {
}