target_sources(tiny-usb
    PRIVATE
        vendor_device.c
        vendor_host.c
)

target_include_directories(tiny-usb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
