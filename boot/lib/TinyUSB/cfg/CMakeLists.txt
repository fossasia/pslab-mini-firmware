target_sources(pslab-mini-bootloader
    PRIVATE usb_descriptors.c
    PUBLIC tusb_config.h
)

target_include_directories(pslab-mini-bootloader
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
