add_executable(pslab-mini-firmware)

target_include_directories(pslab-mini-firmware
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(pslab-mini-firmware
    PRIVATE
        STM32H563xx
        USE_HAL_DRIVER
)

target_link_libraries(pslab-mini-firmware
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
    tinyusb
)

add_subdirectory(system)
add_subdirectory(application)

stm32_print_size_of_target(pslab-mini-firmware)
stm32_generate_srec_file(pslab-mini-firmware)
