cmake_minimum_required(VERSION 3.16)

if(NOT BUILD_TESTS)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_gcc.cmake)
    set(STM32_CUBE_H5_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0)
endif()

set(CMAKE_C_FLAGS_DEBUG_INIT    "-Og -g")
set(CMAKE_CXX_FLAGS_DEBUG_INIT  "-Og -g")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

project(pslab-mini C ASM)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform selection option
if(BUILD_TESTS)
    set(PLATFORM "mock" CACHE STRING "Target platform")
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
else()
    set(PLATFORM "h563xx" CACHE STRING "Target platform")
    find_package(CMSIS COMPONENTS STM32H563ZI REQUIRED)
    find_package(HAL COMPONENTS STM32H5 REQUIRED)
    add_subdirectory(boot)
    add_subdirectory(lib)
endif()
set_property(CACHE PLATFORM PROPERTY STRINGS "h563xx" "mock")

add_subdirectory(src)
