target_sources(pslab-mini-firmware
    PRIVATE
        startup_stm32h563xx.s
        system_stm32h5xx.c
)

target_include_directories(pslab-mini-firmware
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

stm32_add_linker_script(pslab-mini-firmware PRIVATE STM32H563ZITX_FLASH.ld)
