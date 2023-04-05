/*
 
 V3 7Seg Text
 */
#include "main.h"

#define SevenSegCC1 11
#define SevenSegCC2 10

#define SevenSegA 26
#define SevenSegB 27
#define SevenSegC 29
#define SevenSegD 18
#define SevenSegE 25
#define SevenSegF 7
#define SevenSegG 28
#define SevenSegDP 24

int main()
{
    
       stdio_init_all();
    
    
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    // initialize digital pin LED_BUILTIN as an output.
    gpio_init(SevenSegA);
    gpio_init(SevenSegB);
    gpio_init(SevenSegC);
    gpio_init(SevenSegD);
    gpio_init(SevenSegE);
    gpio_init(SevenSegF);
    gpio_init(SevenSegG);
    gpio_init(SevenSegDP);

    gpio_init(SevenSegCC1);
    gpio_init(SevenSegCC2);

    gpio_set_dir(SevenSegA, GPIO_OUT);
    gpio_set_dir(SevenSegB, GPIO_OUT);
    gpio_set_dir(SevenSegC, GPIO_OUT);
    gpio_set_dir(SevenSegD, GPIO_OUT);
    gpio_set_dir(SevenSegE, GPIO_OUT);
    gpio_set_dir(SevenSegF, GPIO_OUT);
    gpio_set_dir(SevenSegG, GPIO_OUT);
    gpio_set_dir(SevenSegDP, GPIO_OUT);

    gpio_set_dir(SevenSegCC1, GPIO_OUT);
    gpio_set_dir(SevenSegCC2, GPIO_OUT);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);

        printf("Start Loop");
        // CC1 Zero
        // CC2 Zero
        gpio_put(SevenSegCC1, 0);
        gpio_put(SevenSegCC2, 0);

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegA, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegA, 0);  // turn the LED On by making the voltage 0

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegB, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegB, 0);  // turn the LED On by making the voltage 0

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegC, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegC, 0);  // turn the LED On by making the voltage 0

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegD, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegD, 0);  // turn the LED On by making the voltage 0
        

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegE, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegE, 0);  // turn the LED On by making the voltage 0

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegF, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegF, 0);  // turn the LED On by making the voltage 0

        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegG, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                // wait for a second
        gpio_put(SevenSegG, 0);  // turn the LED On by making the voltage 0

        sleep_ms(500);                 // wait for a second
        gpio_put(SevenSegDP, 1); // turn the LED On by making the voltage 0
        sleep_ms(500);                 // wait for a second
        gpio_put(SevenSegDP, 0);
       /*
        */
    }

    // Use for debugging
    stdio_init_all();

    return 0;
}
