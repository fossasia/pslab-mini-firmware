target_sources(tinyusb
    PRIVATE
        tusb_fifo.c
)

target_include_directories(tinyusb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
