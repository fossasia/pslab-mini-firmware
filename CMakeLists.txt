cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_gcc.cmake)
set(STM32_CUBE_H5_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0)

set(CMAKE_C_FLAGS_DEBUG_INIT    "-Og -g")
set(CMAKE_CXX_FLAGS_DEBUG_INIT  "-Og -g")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

project(pslab-mini C ASM)

find_package(CMSIS COMPONENTS STM32H563ZI REQUIRED)
find_package(HAL COMPONENTS STM32H5 REQUIRED)

add_subdirectory(boot)
add_subdirectory(lib)
add_subdirectory(src)
