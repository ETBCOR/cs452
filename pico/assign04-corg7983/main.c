/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 4 (modded to display hex) *
 * File:        main.c                               *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */

// Include this file's header
#include "main.h"

//---------------- DEFINITIONS ----------------//

// Define the number of pins in use
#define NUM_PINS 11

// Seven segment display pins
#define SevenSegCC1 11  // Right digit
#define SevenSegCC2 10  // Left digit
#define SevenSegA 26    // N  seg
#define SevenSegB 27    // NE seg
#define SevenSegC 29    // SE seg
#define SevenSegD 18    // S  seg
#define SevenSegE 25    // SW seg
#define SevenSegF 7     // NW seg
#define SevenSegG 28    // Middle seg
#define SevenSegDP 24   // Decimal points
const uint LED_PIN = PICO_DEFAULT_LED_PIN;

// Keeps an array of the pins for easy initializations
const int PINS[NUM_OUT_PINS] = {
    SevenSegCC1, SevenSegCC2,
    SevenSegA, SevenSegB, SevenSegC, SevenSegD, SevenSegE, SevenSegF, SevenSegG,
    SevenSegDP,
    LED_PIN
};



//---------------- PROTOTYPES / HANDLES ----------------//

// Utility function prototypes
static void gpio_init_pins();
static void putDigit(int, bool);
static void clearDigits();
static void put7Pins(int, int, int, int, int, int, int);

// Task prototypes
static void vCounterTsk();
static void vBlinkerTsk();
static void vSevSegTsk();

// Task/queue handles
static TaskHandle_t xCounterTsk = NULL;
static TaskHandle_t xBlinkerTsk = NULL;
static TaskHandle_t xSevSegTsk  = NULL;
static QueueHandle_t xSevSegQueue     = NULL;

// Structs
typedef struct SevSegMsgStruct {
    int l;
    int r;
    int display_ticks;
} SevSegMsg;


//---------------- MAIN ----------------//
int main()
{
    // Initialization
    stdio_init_all();
    gpio_init_pins();

    // Create a queue of size 11 to communicate the counter value
    xSevSegQueue = xQueueCreate(11, sizeof(SevSegMsg));
    if (xSevSegQueue == NULL) exit(1); // Exits if queue creation fails

    // Create the tasks
    xTaskCreate(vCounterTsk, "Counter_Task", 256, NULL, 3, &xCounterTsk);
    xTaskCreate(vBlinkerTsk, "Blinker_Task", 256, NULL, 1, &xBlinkerTsk);
    xTaskCreate(vSevSegTsk,  "Sev_Seg_Task", 256, NULL, 2, &xSevSegTsk );

    // start the task scheduler
    vTaskStartScheduler();

    while(1){};
}


//---------------- UTILITY FUNCTIONS ----------------//

// Utility function to init all the needed pins
static void gpio_init_pins()
{
    for (int i = 0; i < NUM_OUT_PINS; i++) {
        gpio_init(PINS[i]);
        gpio_set_dir(PINS[i], GPIO_OUT);
    }
}

// Puts a digit onto one of the two 7-seg displays
static void putDigit(int digit, bool leftSide)
{
    gpio_put(SevenSegCC1, !leftSide);
    gpio_put(SevenSegCC2, leftSide);

    // Draw the digit (the passed arg mod 16)
    switch (digit % 16) {
        case 0: put7Pins(1, 1, 1, 1, 1, 1, 0); break;
        case 1: put7Pins(0, 1, 1, 0, 0, 0, 0); break;
        case 2: put7Pins(1, 1, 0, 1, 1, 0, 1); break;
        case 3: put7Pins(1, 1, 1, 1, 0, 0, 1); break;
        case 4: put7Pins(0, 1, 1, 0, 0, 1, 1); break;
        case 5: put7Pins(1, 0, 1, 1, 0, 1, 1); break;
        case 6: put7Pins(1, 0, 1, 1, 1, 1, 1); break;
        case 7: put7Pins(1, 1, 1, 0, 0, 0, 0); break;
        case 8: put7Pins(1, 1, 1, 1, 1, 1, 1); break;
        case 9: put7Pins(1, 1, 1, 1, 0, 1, 1); break;
        case 10: put7Pins(1, 1, 1, 0, 1, 1, 1); break;
        case 11: put7Pins(0, 0, 1, 1, 1, 1, 1); break;
        case 12: put7Pins(1, 0, 0, 1, 1, 1, 0); break;
        case 13: put7Pins(0, 1, 1, 1, 1, 0, 1); break;
        case 14: put7Pins(1, 0, 0, 1, 1, 1, 1); break;
        case 15: put7Pins(1, 0, 0, 0, 1, 1, 1); break;
        case -1: default: put7Pins(0, 0, 0, 0, 0, 0, 0); // Blanks the screen by default
    }
}

// Clears the digits of the 7-seg displays
static void clearDigits()
{
    gpio_put(SevenSegCC1, 1);
    gpio_put(SevenSegCC2, 1);
    gpio_put(SevenSegDP, 0);
    put7Pins(0, 0, 0, 0, 0, 0, 0);
}

// This is a subroutine used in the switch-case statement
// in putDigit (above), to reduce code duplication.
static void put7Pins(int A, int B, int C, int D, int E, int F, int G)
{
    gpio_put(SevenSegA, A);
    gpio_put(SevenSegB, B);
    gpio_put(SevenSegC, C);
    gpio_put(SevenSegD, D);
    gpio_put(SevenSegE, E);
    gpio_put(SevenSegF, F);
    gpio_put(SevenSegG, G);
}

//---------------- TASKS ----------------//

// This task counts (every half second) from 42 to 0 to 42, in a loop,
// updating the queue with the new value, and notifying the blink task
// each time the value changes.
static void vCounterTsk()
{
    int counter = 0x0;
    int jump = 0x1;
    SevSegMsg msg;
    msg.l = -1;
    msg.display_ticks = 1000 / portTICK_PERIOD_MS; // 1 second

    while (true) {

        // notify the blink task to unblock
        xTaskNotifyGive(xBlinkerTsk);

        // override the queue's value with the new counter value
        msg.r = counter;
        xQueueSend(xSevSegQueue, &msg, 0);

        // log the counter value for debugging
        printf(
            "[%s] @ %i: %s %i\n",
            "Counter_Task",
            xTaskGetTickCount(),
            "Counter value is:",
            counter
        );
        
        // Delay the task for 0.5 seconds
        vTaskDelay(0.5 * 1000 / portTICK_PERIOD_MS);

        // increment the counter
        counter += jump;

        // reverse the jump direction if we're at an extreme
        if (counter == 0x0 || counter == 0xF)
            jump *= -1;
    }
}

// This task waits for a notification and blinks the LED on for 3 ticks.
static void vBlinkerTsk()
{
    while (true) {

        // block until notified to blink (by vCountTsk)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // flash the LED on for 3 ticks
        gpio_put(LED_PIN, 1);
        vTaskDelay(3);
        gpio_put(LED_PIN, 0);
    }
}

// This task receives messages from the queue and
// displays them on the seven segment display.
static void vSevSegTsk()
{
    SevSegMsg msg;
    TickType_t until;

    while (true) {
        
        // Block until a message is received
        xQueueReceive(xSevSegQueue, &msg, portMAX_DELAY);
        until = xTaskGetTickCount() + msg.display_ticks;

        // Display the received message until its time is up
        while (xTaskGetTickCount() < until) {

            putDigit(msg.l, true);  // Put the left digit
            vTaskDelay(1);          // Delay this task so that left digit is visible

            putDigit(msg.r, false); // Put the right digit
            vTaskDelay(1);          // Delay this task so that right digit is visible

            // Check if the queue has filled up
            if (uxQueueSpacesAvailable(xSevSegQueue) == 0) {
                printf("[Sev_Seg_Tsk]: The message queue overflowed! Waiting 5 seconds, then clearing.\n");

                // Display "OF" (overflow) on the display for 5 seconds
                until = xTaskGetTickCount() + 5 * 1000 / portTICK_PERIOD_MS;
                while (xTaskGetTickCount() < until) {
                    putDigit(0,  true);  vTaskDelay(1);
                    putDigit(15, false); vTaskDelay(1);
                }

                // Clear the queue and break the while loop
                printf("[Sev_Seg_Tsk]: Clearing the queue.\n");
                xQueueReset(xSevSegQueue);
                break;
            }
        }

        // Clear the display
        clearDigits();
    }
}