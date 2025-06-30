target_sources(pslab-platform
    PRIVATE
        led_ll.c
        platform.c
        uart_ll.c
        usb_ll.c
        startup_stm32h563xx.s
        system_stm32h5xx.c
)

target_include_directories(pslab-platform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(pslab-platform
    PRIVATE
        STM32H563xx
        USE_HAL_DRIVER
)

target_link_libraries(pslab-platform
    PRIVATE
        HAL::STM32::H5::CORTEX
        HAL::STM32::H5::DMAEx
        HAL::STM32::H5::ICACHE
        HAL::STM32::H5::GPIO
        HAL::STM32::H5::RCCEx
        HAL::STM32::H5::PWREx
        HAL::STM32::H5::UARTEx
        CMSIS::STM32::H5
        STM32::NoSys
        STM32::Nano
)
