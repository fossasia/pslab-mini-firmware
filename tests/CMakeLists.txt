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
target_sources(test_uart PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/bus/uart.c)
target_sources(test_uart PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/bus/bus.c)
target_include_directories(test_uart PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/system/bus/)
target_include_directories(test_uart PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/)
