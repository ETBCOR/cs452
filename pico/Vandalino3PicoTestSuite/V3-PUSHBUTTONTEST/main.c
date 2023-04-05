/*
 
 V3 PushButton Test 
 */
#include "main.h"

#define BUTTON1 19
#define BUTTON2 9
#define BUTTON3 8

int main()
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;

    stdio_init_all();
    // initialize digital pin LED_BUILTIN as an output.
    gpio_init(BUTTON1);
    gpio_init(BUTTON2);
    gpio_init(BUTTON3);


    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_set_dir(BUTTON3, GPIO_IN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
        int button1, button2, button3;
        button1 = gpio_get(BUTTON1);
        button2 = gpio_get(BUTTON2);
        button3 = gpio_get(BUTTON3);

        printf("button1/2/3 %d%d%d\n", button1, button2, button3);



    }

    // Use for debugging
    stdio_init_all();

    return 0;
}
