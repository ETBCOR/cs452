/* * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)
 * Instructor:  John Shovic
 * Student:     Ethan Corgatelli (corg7983)
 * Project:     Assignment 3
 * File:        main.c
 * 
\* * * * * * * * * * * * * * * * */

// The C std lib, pico std lib, FreeRTOS header,
// and FRTOS's task header are included in main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include <FreeRTOS.h>
#include <task.h>
#include <time.h>

// The function "task_1()" defines a function used for the creation of Task_1.
// * It stores its name in a constant called "TASK_NAME" to be used for debug outputs.
// * It also stores the frequency (in ticks) at which this task should unblock.
// * It allocates spaces on its stack for the time at which it was most recently
//   delayed, as well as flag marking whether or not the thread was properly delayed.
// * Before entering the infinite loop, the current time (tick) is stored.
// * Inside the loop, we use xTaskDelayUntil, the time from the last wake, and the
//   frequency constant to sleep the thread until it should next be called (in 1 second),
//   then the current tick is printed (along with the current tick converted to seconds).
void task_1()
{   
    const char *TASK_NAME = "Task_1";

    time_t rawtime;
    struct tm * timeinfo;

    while (true) {
        // Delay the task for a second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Attempt to get the current local time
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        // print the time
        printf(
            "[%s]: Current time since init is %i ticks (%i seconds). Current time/date is: %s\n",
            TASK_NAME,
            xTaskGetTickCount(),
            xTaskGetTickCount() / configTICK_RATE_HZ, // time since init in seconds
            asctime(timeinfo)
        );
    }
}

// The function "main()" will be called when the device starts.
// * In main, we first init all the present stdio types that are linked.
// * Then we create a task using the "task_1()" function, naming it "Task_1"
//   and giving it a stack of size 256. It takes no parameters, has a priority
//   of 1, and we don't use a handle.
// * Then we use vTaskStartScheduler() to start FreeRTOS's task scheduler.
// * The infinite loop in main is left empty.
int main()
{
    stdio_init_all();

    xTaskCreate(task_1, "Task_1", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}