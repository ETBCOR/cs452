/* 
 * sos led blinker, by corg7983
 */
#include "main.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

const int DIT = 200;

int main()
{

    stdio_init_all();
    // initialize digital pin LED_BUILTIN as an output.

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true)
    {
        S(); O(); S();
	    sleep_ms(DIT * 4);
    }

    // Use for debugging
    stdio_init_all();

    return 0;
}

void dot() {
    gpio_put(LED_PIN, 1);
    sleep_ms(DIT);
    gpio_put(LED_PIN, 0);
    sleep_ms(DIT);
}

void dash() {
    gpio_put(LED_PIN, 1);
    sleep_ms(DIT * 3);
    gpio_put(LED_PIN, 0);
    sleep_ms(DIT);
}

void S() {
    dot(); dot(); dot();
    sleep_ms(DIT * 2);
}

void O() {
    dash(); dash(); dash();
    sleep_ms(DIT * 2);
}

