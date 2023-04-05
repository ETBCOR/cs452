cmake_minimum_required(VERSION 3.14)
set(PICO_BOARD adafruit_feather_rp2040)
set(PICO_SDK_PATH ~/pico/pico-sdk)

include(pico_sdk_import.cmake)
project(V3-PUSHBUTTONTEST VERSION 1.0.0)
add_executable(V3-PUSHBUTTONTEST
               main.c)

pico_sdk_init()

pico_enable_stdio_usb(V3-PUSHBUTTONTEST 1)
pico_enable_stdio_uart(V3-PUSHBUTTONTEST 1)
pico_add_extra_outputs(V3-PUSHBUTTONTEST)

target_link_libraries(V3-PUSHBUTTONTEST
                      pico_stdlib
                      hardware_gpio
                      hardware_i2c
                      hardware_spi
                      hardware_adc
                      hardware_uart)

                   
