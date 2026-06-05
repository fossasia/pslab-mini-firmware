cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pslab_pico C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pslab_pico)

target_sources(pslab_pico PRIVATE
        src/application/main.c
        )

target_include_directories(pslab_pico PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        )

target_link_libraries(pslab_pico PRIVATE
        pico_stdlib
        )

pico_add_extra_outputs(pslab_pico)
