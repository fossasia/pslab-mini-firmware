include("${CMAKE_CURRENT_SOURCE_DIR}/create_test.cmake")

# Unity test framework
add_library(unity ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/unity.c)
target_include_directories(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/)

# CMock mocking framework
add_library(cmock ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src/cmock.c)
target_include_directories(cmock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src/)
target_link_libraries(cmock unity)

# Generate mocks for UART dependencies
cmock_generate_mock(mock_uart_ll ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/uart_ll.h)

# Add UART test
cmock_add_test(test_uart test_uart.c mock_uart_ll)
target_link_libraries(test_uart pslab-bus)

# Add syscalls test (reuses uart_ll mock and tests real syscalls.c)
cmock_add_test(test_syscalls test_syscalls.c mock_uart_ll)
# Include the actual syscalls.c implementation
target_sources(test_syscalls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/syscalls.c)
target_sources(test_syscalls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_headers/reent-stub.c)
target_link_libraries(test_syscalls pslab-bus)

# Add test-specific include directory BEFORE system includes to override headers
target_include_directories(test_syscalls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_headers)

# Define configuration for testing
target_compile_definitions(test_syscalls PRIVATE
    SYSCALLS_UART_BUS=0
    SYSCALLS_UART_TX_BUFFER_SIZE=256
)
