# PSLab Mini Firmware Build System
#
# This project supports two build modes:
# 1. Target build (default): Builds firmware for STM32H563 target
# 2. Test build: Builds unit tests with mocked dependencies
#
# Usage:
#   Target build:  cmake ..
#   Test build:    cmake -DBUILD_TESTS=ON ..

cmake_minimum_required(VERSION 3.16)

# Set up toolchain before project() call if building for target
if(NOT BUILD_TESTS)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_gcc.cmake)
    set(STM32_CUBE_H5_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0)
    # Set up CMAKE_MODULE_PATH early for find_package commands
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    # Set global compile options for ARM Cortex-M33 architecture
    # stm32-cmake does this automatically for pslab-platform, but the
    # hardware-agnostic targets also require this when building for STM32H5.
    add_compile_options(
        -mthumb
        -Wall
        -ffunction-sections
        -fdata-sections
        -mcpu=cortex-m33
        -mfloat-abi=hard
        -mfpu=fpv5-sp-d16
    )
    # Set global link options for ARM Cortex-M33 architecture
    add_link_options(
        -mthumb
        -mcpu=cortex-m33
        -mfloat-abi=hard
        -mfpu=fpv5-sp-d16
        -Wl,--gc-sections
    )
endif()

# Include common configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pslab-common.cmake)

# Project definition (must be after toolchain setup)
if(BUILD_TESTS)
    project(pslab-mini-tests C)
else()
    project(pslab-mini C ASM)
endif()

# Determine build mode and delegate to appropriate configuration
if(BUILD_TESTS)
    message(STATUS "Configuring for test build")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake)
else()
    message(STATUS "Configuring for target build")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)
endif()
