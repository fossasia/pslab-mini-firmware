include(CTest)
enable_testing()

add_library(unity ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/unity.c)
target_include_directories(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/)

add_executable(test_uart test_uart.c)
target_sources(test_uart
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/system/bus/bus.c
        ${CMAKE_SOURCE_DIR}/src/system/bus/uart.c
)
target_include_directories(test_uart
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/system/bus
        ${CMAKE_SOURCE_DIR}/src/platform
        ${CMAKE_SOURCE_DIR}/src/platform/mock
)
target_link_libraries(test_uart
    PRIVATE
        pslab-platform
        unity
)
add_test(NAME test_uart COMMAND test_uart)
