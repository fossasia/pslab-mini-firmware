target_sources(tiny-usb
    PRIVATE
        tusb_fifo.c
)

target_include_directories(tiny-usb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
