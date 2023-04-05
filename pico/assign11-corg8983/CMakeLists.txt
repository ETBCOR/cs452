cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wno-psabi)

set(PICO_BOARD adafruit_feather_rp2040)
set(PICO_SDK_PATH /home/etbcor/pico/pico-sdk)
set(PICO_SDK_FREERTOS_SOURCE /home/etbcor/pico/FreeRTOS-Kernel)
include(pico_sdk_import.cmake)

project(assign11-corg7983 C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)

pico_sdk_init()

include(PicoLed/PicoLed.cmake)

add_library(freertos
    ${PICO_SDK_FREERTOS_SOURCE}/event_groups.c
    ${PICO_SDK_FREERTOS_SOURCE}/list.c
    ${PICO_SDK_FREERTOS_SOURCE}/queue.c
    ${PICO_SDK_FREERTOS_SOURCE}/stream_buffer.c
    ${PICO_SDK_FREERTOS_SOURCE}/tasks.c
    ${PICO_SDK_FREERTOS_SOURCE}/timers.c
    ${PICO_SDK_FREERTOS_SOURCE}/portable/MemMang/heap_3.c
    ${PICO_SDK_FREERTOS_SOURCE}/portable/GCC/ARM_CM0/port.c
)

target_include_directories(freertos PUBLIC
    .
    ${PICO_SDK_FREERTOS_SOURCE}/include
    ${PICO_SDK_FREERTOS_SOURCE}/portable/GCC/ARM_CM0
)


add_executable(assign11-corg7983
        main.cpp
)

pico_enable_stdio_usb(assign11-corg7983 1)
pico_enable_stdio_uart(assign11-corg7983 1)

target_link_libraries(assign11-corg7983
    pico_stdlib
    freertos
    hardware_gpio
    hardware_i2c
    hardware_adc
    PicoLed
)

pico_add_extra_outputs(assign11-corg7983)
