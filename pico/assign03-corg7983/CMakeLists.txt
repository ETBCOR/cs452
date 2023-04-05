cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD adafruit_feather_rp2040)
set(PICO_SDK_PATH /home/etbcor/pico/pico-sdk)
set(PICO_SDK_FREERTOS_SOURCE /home/etbcor/pico/FreeRTOS-Kernel)
include(pico_sdk_import.cmake)

project(assign03-corg7983)

pico_sdk_init()

#add_subdirectory(FreeRTOS)
#add_subdirectory(ProjectFiles)

enable_language( C CXX ASM )

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


add_executable(assign03-corg7983
        main.c
)

pico_enable_stdio_usb(assign03-corg7983 1)
pico_enable_stdio_uart(assign03-corg7983 1)

target_link_libraries(assign03-corg7983 pico_stdlib freertos)
pico_add_extra_outputs(assign03-corg7983)

