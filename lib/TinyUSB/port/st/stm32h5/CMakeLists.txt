target_sources(tiny-usb
    PRIVATE
        dcd_stm32_fsdev.c
)

target_include_directories(tiny-usb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
