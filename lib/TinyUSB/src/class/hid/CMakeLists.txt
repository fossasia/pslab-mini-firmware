target_sources(tiny-usb
    PRIVATE
        hid_device.c
        hid_host.c
)

target_include_directories(tiny-usb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
