target_sources(pslab-platform
    PRIVATE
        mock_uart_ll.c
)

target_include_directories(pslab-platform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
