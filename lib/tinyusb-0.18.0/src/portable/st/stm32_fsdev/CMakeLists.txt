target_sources(tinyusb
    PRIVATE
        dcd_stm32_fsdev.c
)

target_include_directories(tinyusb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
