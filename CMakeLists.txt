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
        src/application/protocol/bus/i2c.c
        src/application/protocol/bus/uart.c
        src/application/protocol/la.c
        src/application/protocol/dso.c
        src/application/protocol/mso.c
        src/application/communication_commands.c
        src/application/gateway/i2c_commands.c
        src/application/gateway/uart_commands.c
        src/application/logic_analyser_commands.c
        src/application/dso_commands.c
        src/application/mixed_signal_commands.c
        src/system/instrument/dso.c
        src/system/instrument/mixed_signal.c
        src/system/pattern_generator.c
        src/platform/adc_capture.c
        src/platform/i2c_ll.c
        src/platform/logic_analyser_ll.c
        src/platform/pattern_output_ll.c
        src/platform/platform.c
        src/platform/esp_spi_bridge.c
        src/platform/spi_ll.c
        src/platform/uart_ll.c
        src/platform/usb_cdc.c
        src/platform/usb_descriptors.c
        src/platform/status_led.c
        src/platform/test_signal.c
        src/system/bus/i2c.c
        src/system/bus/spi.c
        src/system/bus/uart.c
        src/system/logic_analyser.c
        src/system/transport.c
        src/system/syscalls.c
        src/system/system.c
        src/util/circular_buffer.c
        src/util/error.c
        src/util/fixed_point.c
        src/util/logging.c
        lib/CException-1.3.4/CException.c
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
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/platform
        ${CMAKE_CURRENT_LIST_DIR}/lib/CException-1.3.4
        ${CMAKE_CURRENT_LIST_DIR}/lib/scpi-parser-2.3/inc
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
        hardware_i2c
        hardware_pwm
        hardware_spi
        hardware_uart
        hardware_clocks
        tinyusb_device
        tinyusb_board
        )

pico_add_extra_outputs(pslab_pico)
