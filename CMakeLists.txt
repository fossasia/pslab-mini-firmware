cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pslab_pico C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pslab_pico)

target_sources(pslab_pico PRIVATE
        src/application/main.c
        src/application/protocol/common.c
        src/application/protocol/la.c
        src/application/protocol/dso.c
        src/application/logic_analyser_commands.c
        src/application/dso_commands.c
        src/system/dso.c
        src/platform/adc_capture.c
        src/platform/usb_cdc.c
        src/platform/usb_descriptors.c
        src/platform/status_led.c
        src/platform/test_signal.c
        src/system/logic_analyser.c
        lib/scpi-parser-2.3/src/error.c
        lib/scpi-parser-2.3/src/expression.c
        lib/scpi-parser-2.3/src/fifo.c
        lib/scpi-parser-2.3/src/ieee488.c
        lib/scpi-parser-2.3/src/lexer.c
        lib/scpi-parser-2.3/src/minimal.c
        lib/scpi-parser-2.3/src/parser.c
        lib/scpi-parser-2.3/src/units.c
        lib/scpi-parser-2.3/src/utils.c
        )

target_include_directories(pslab_pico PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/platform
        ${CMAKE_CURRENT_LIST_DIR}/lib/scpi-parser-2.3/inc
        )

target_link_libraries(pslab_pico PRIVATE
        pico_stdlib
        pico_unique_id
        hardware_pio
        hardware_dma
        hardware_adc
        hardware_pwm
        hardware_clocks
        tinyusb_device
        tinyusb_board
        )

pico_add_extra_outputs(pslab_pico)
