target_sources(pslab-mini-firmware
    PRIVATE
        usb_descriptors.c
        usb.c
)

target_include_directories(pslab-mini-firmware
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
