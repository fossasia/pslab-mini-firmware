target_sources(pslab-mini-bootloader
    PRIVATE
        hid_device.c
        hid_host.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
