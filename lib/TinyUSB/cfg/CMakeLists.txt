target_sources(tiny-usb
    PRIVATE usb_descriptors.c
    PUBLIC tusb_config.h
)

target_include_directories(tiny-usb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
