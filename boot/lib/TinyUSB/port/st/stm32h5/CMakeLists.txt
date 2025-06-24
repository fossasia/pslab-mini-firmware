target_sources(pslab-mini-bootloader
    PRIVATE
        dcd_stm32_fsdev.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
