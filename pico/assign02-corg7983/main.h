/*
 * killer_iot_device for Raspberry Pi Pico
 *
 * @version     1.0.0
 * @author      corg7983
 * @copyright   2022
 * @licence     MIT
 *
 */
#ifndef _SOS_MAIN_HEADER_
#define _SOS_MAIN_HEADER_

/*
 * C HEADERS
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * PICO HEADERS
 */
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/uart.h"

void dot();
void dash();
void S();
void O();

#endif // _V3-SOS_MAIN_HEADER_
