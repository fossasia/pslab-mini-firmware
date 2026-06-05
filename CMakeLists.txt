cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pslab_pico C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pslab_pico)

target_sources(pslab_pico PRIVATE
        src/application/main.c
        src/application/protocol.c
        src/application/logic_analyser_commands.c
        src/application/dso_commands.c
        src/system/logic_analyser.c
        src/system/dso.c
        src/system/system.c
        src/system/syscalls.c
        src/system/bus/uart.c
        src/platform/adc_capture.c
        src/platform/uart_ll.c
        src/platform/test_signal.c
        src/platform/status_led.c
        src/platform/usb_cdc.c
        src/platform/usb_descriptors.c
        src/util/circular_buffer.c
        src/util/fixed_point.c
        src/util/logging.c
        lib/CException/CException.c
        )

target_include_directories(pslab_pico PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/platform
        ${CMAKE_CURRENT_LIST_DIR}/lib/CException
        )

target_compile_definitions(pslab_pico PRIVATE
        CEXCEPTION_USE_CONFIG_FILE
        )

target_link_libraries(pslab_pico PRIVATE
        pico_stdlib
        pico_unique_id
        hardware_pio
        hardware_dma
        hardware_adc
        hardware_pwm
        hardware_uart
        hardware_clocks
        hardware_watchdog
        tinyusb_device
        tinyusb_board
        )

pico_add_extra_outputs(pslab_pico)
