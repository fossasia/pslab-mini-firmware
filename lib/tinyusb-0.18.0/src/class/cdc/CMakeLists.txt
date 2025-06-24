target_sources(tinyusb
    PRIVATE
        cdc_device.c
)

target_include_directories(tinyusb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
