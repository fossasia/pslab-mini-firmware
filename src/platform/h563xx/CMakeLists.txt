target_sources(pslab-platform
    PRIVATE
        adc_ll.c
        led_ll.c
        platform.c
        uart_ll.c
        usb_ll.c
        startup_stm32h563xx.s
        system_stm32h5xx.c
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
    PRIVATE
        CMSIS::STM32::H5
        HAL::STM32::H5::ADCEx
        HAL::STM32::H5::CORTEX
        HAL::STM32::H5::DMAEx
        HAL::STM32::H5::ICACHE
        HAL::STM32::H5::GPIO
        HAL::STM32::H5::RCCEx
        HAL::STM32::H5::PWREx
        HAL::STM32::H5::UARTEx
        tinyusb
    INTERFACE
        # pslab-platform provides newlib for malloc etc.
        STM32::NoSys
        STM32::Nano
        # Force inclusion of specific USB callback symbols
        "-Wl,--undefined=tud_descriptor_device_cb"
        "-Wl,--undefined=tud_descriptor_configuration_cb"
        "-Wl,--undefined=tud_descriptor_string_cb"
)
