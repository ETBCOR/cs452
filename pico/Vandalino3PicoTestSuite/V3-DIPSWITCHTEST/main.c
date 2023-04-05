/*
 
 V3 PushButton Test 
 */
#include "main.h"

#define SWITCH1 6
#define SWITCH2 7
#define SWITCH3 3
#define SWITCH4 2
#define SWITCH5 13
#define SWITCH6 26 
#define SWITCH7 0
#define SWITCH8 1

int main()
{

    stdio_init_all();
    // initialize digital pin LED_BUILTIN as an output.
    gpio_init(SWITCH1);
    gpio_init(SWITCH2);
    gpio_init(SWITCH3);
    gpio_init(SWITCH4);
    gpio_init(SWITCH5);
    gpio_init(SWITCH6);
    gpio_init(SWITCH7);
    gpio_init(SWITCH8);


    gpio_set_dir(SWITCH1, GPIO_IN);
    gpio_set_dir(SWITCH2, GPIO_IN);
    gpio_set_dir(SWITCH3, GPIO_IN);
    gpio_set_dir(SWITCH4, GPIO_IN);
    gpio_set_dir(SWITCH5, GPIO_IN);
    gpio_set_dir(SWITCH6, GPIO_IN);
    gpio_set_dir(SWITCH7, GPIO_IN);
    gpio_set_dir(SWITCH8, GPIO_IN);

    while (true)
    {
        int switch1, switch2, switch3, switch4, switch5, switch6, switch7, switch8;
        switch1 = gpio_get(SWITCH1);
        switch2 = gpio_get(SWITCH2);
        switch3 = gpio_get(SWITCH3);
        switch4 = gpio_get(SWITCH4);
        switch5 = gpio_get(SWITCH5);
        switch6 = gpio_get(SWITCH6);
        switch7 = gpio_get(SWITCH7);
        switch8 = gpio_get(SWITCH8);

        printf("switch1/2/3/4/5/6/7/8 %d%d%d%d%d%d%d%d\n", switch1, switch2, switch3, switch4, switch5 , switch6 , switch7, switch8);

    }

    // Use for debugging
    stdio_init_all();

    return 0;
}
