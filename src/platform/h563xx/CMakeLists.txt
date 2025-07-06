target_sources(pslab-platform
    PRIVATE
        adc_ll.c
        led_ll.c
        platform.c
        uart_ll.c
        usb_ll.c
        startup_stm32h563xx.s
        system_stm32h5xx.c
    INTERFACE
        # TinyUSB is a build-time dependency of the main application,
        # which requires the USB descriptor callbacks.
        usb_descriptors.c
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
    # Propagate STM32H5 dependency to consumers to ensure correct build flags
    PUBLIC
        CMSIS::STM32::H5
        tinyusb
    PRIVATE
        HAL::STM32::H5::ADCEx
        HAL::STM32::H5::CORTEX
        HAL::STM32::H5::DMAEx
        HAL::STM32::H5::ICACHE
        HAL::STM32::H5::GPIO
        HAL::STM32::H5::RCCEx
        HAL::STM32::H5::PWREx
        HAL::STM32::H5::UARTEx
    # pslab-platform provides newlib for malloc etc.
    INTERFACE
        STM32::NoSys
        STM32::Nano
)
