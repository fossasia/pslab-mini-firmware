cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_gcc.cmake)
set(STM32_CUBE_H5_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0)

project(pslab-mini-firmware C ASM)

find_package(CMSIS COMPONENTS STM32H563ZI REQUIRED)
find_package(HAL COMPONENTS STM32H5 REQUIRED)

set(PROJECT_SOURCES
    src/main.c
    src/led.c
    src/stubs.c
    src/timer.c
)

add_executable(pslab-mini-firmware ${PROJECT_SOURCES})

target_include_directories(pslab-mini-firmware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pslab-mini-firmware
    HAL::STM32::H5::CORTEX
    HAL::STM32::H5::ICACHE
    HAL::STM32::H5::GPIO
    HAL::STM32::H5::RCC
    HAL::STM32::H5::PWR
    HAL::STM32::H5::PWREx
    CMSIS::STM32::H563xx
    STM32::NoSys
    STM32::Nano
)
stm32_add_linker_script(pslab-mini-firmware PRIVATE src/STM32H563ZITX_FLASH.ld)
stm32_print_size_of_target(pslab-mini-firmware)
stm32_generate_srec_file(pslab-mini-firmware)
