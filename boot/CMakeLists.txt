add_executable(pslab-mini-bootloader)

target_sources(pslab-mini-bootloader
    PRIVATE
        main.c
        hooks.c
        led.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(lib)
add_subdirectory(openblt)

target_compile_definitions(pslab-mini-bootloader
    PRIVATE
        USE_HAL_DRIVER
        USE_FULL_LL_DRIVER
)
target_link_libraries(pslab-mini-bootloader
    PRIVATE
        HAL::STM32::H5::CORTEX
        HAL::STM32::H5::FLASHEx
        HAL::STM32::H5::LL_GPIO
        HAL::STM32::H5::RCC
        HAL::STM32::H5::LL_RCC
        HAL::STM32::H5::LL_USART
        HAL::STM32::H5::LL_UTILS
        HAL::STM32::H5::LL_ICACHE
        CMSIS::STM32::H563xx
)
target_compile_options(pslab-mini-bootloader
    PRIVATE
         $<$<CONFIG:Debug>:-Og> # -O0 doesn't fit in 48K
)

stm32_add_linker_script(pslab-mini-bootloader PRIVATE STM32H563ZITX_FLASH.ld)
stm32_print_size_of_target(pslab-mini-bootloader)
stm32_generate_srec_file(pslab-mini-bootloader)
