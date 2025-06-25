target_sources(tinyusb
    PRIVATE
        usbd_control.c
        usbd.c
)

target_include_directories(tinyusb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
