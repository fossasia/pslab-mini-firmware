target_sources(tiny-usb
    PRIVATE
        usbd_control.c
        usbd.c
)

target_include_directories(tiny-usb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
