include("${CMAKE_CURRENT_SOURCE_DIR}/create_test.cmake")

# Unity test framework
add_library(unity ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/unity.c)
# Add the test exception handler directly to unity to propagate to all tests
target_sources(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/test_headers/exception_test.c)
target_include_directories(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/)

# CMock mocking framework
add_library(cmock ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src/cmock.c)
target_include_directories(cmock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src/)
target_link_libraries(cmock unity cexception pslab-util)

# Generate mocks for UART dependencies
cmock_generate_mock(mock_uart_ll ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/uart_ll.h)
cmock_generate_mock(mock_platform ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/platform.h)

# Generate mocks for DMM dependencies
cmock_generate_mock(mock_adc_ll ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/adc_ll.h)
cmock_generate_mock(mock_tim_ll ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/tim_ll.h)

# Generate mocks for protocol dependencies
cmock_generate_mock(mock_usb ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/bus/usb.h)
cmock_generate_mock(mock_dmm ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/instrument/dmm.h)
cmock_generate_mock(mock_system ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/system.h)

# Add UART test
cmock_add_test(test_uart test_uart.c mock_uart_ll mock_platform)
target_link_libraries(test_uart pslab-bus pslab-util)

# Add syscalls test (reuses uart_ll mock and tests real syscalls.c)
cmock_add_test(test_syscalls test_syscalls.c mock_uart_ll mock_platform)
# Include the actual syscalls.c implementation
target_sources(test_syscalls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/syscalls.c)
target_sources(test_syscalls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_headers/reent-stub.c)
target_link_libraries(test_syscalls pslab-bus pslab-util)

# Add test-specific include directory BEFORE system includes to override headers
target_include_directories(test_syscalls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_headers)

# Define configuration for testing
target_compile_definitions(test_syscalls PRIVATE
    SYSCALLS_UART_BUS=0
    SYSCALLS_UART_TX_BUFFER_SIZE=256
)

# Add circular buffer test (no mocks needed - pure unit test)
unity_add_test(test_circular_buffer test_circular_buffer.c)
target_link_libraries(test_circular_buffer pslab-util)

# Add logging test (no mocks needed - pure unit test)
unity_add_test(test_logging test_logging.c)
target_link_libraries(test_logging pslab-util)

# Add fixed-point test (no mocks needed - pure unit test)
unity_add_test(test_fixed_point test_fixed_point.c)
target_link_libraries(test_fixed_point pslab-util)

# Add DMM test
cmock_add_test(test_dmm test_dmm.c mock_adc_ll mock_tim_ll)
target_link_libraries(test_dmm pslab-util pslab-instrument)

# Add protocol test
cmock_add_test(test_protocol test_protocol.c mock_usb mock_dmm mock_system)
target_link_libraries(test_protocol pslab-util pslab-application)
