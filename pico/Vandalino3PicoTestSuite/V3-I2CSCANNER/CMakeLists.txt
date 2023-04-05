cmake_minimum_required(VERSION 3.14)
set(PICO_BOARD adafruit_feather_rp2040)
set(PICO_SDK_PATH ~/pico/pico-sdk)

include(pico_sdk_import.cmake)
project(V3-I2CSCANNER VERSION 1.0.0)
add_executable(V3-I2CSCANNER
              bus_scan.c)

pico_sdk_init()

pico_enable_stdio_usb(V3-I2CSCANNER 1)
pico_enable_stdio_uart(V3-I2CSCANNER 0)
pico_add_extra_outputs(V3-I2CSCANNER)

target_link_libraries(V3-I2CSCANNER
                      pico_stdlib
                      hardware_gpio
                      hardware_i2c
                      hardware_adc
                    )
                   