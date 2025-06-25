target_sources(pslab-mini-bootloader
    PRIVATE
        vendor_device.c
        vendor_host.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
