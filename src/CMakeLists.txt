add_executable(pslab-mini-firmware)

# Glob all C source files in the current directory and subdirectories
# for clang-format and clang-tidy
file(GLOB_RECURSE
    ALL_C_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Exclude src/platform/h563xx/system_stm32h5xx.c from linting
list(
    REMOVE_ITEM
    ALL_C_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/h563xx/system_stm32h5xx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/h563xx/stm32h5xx_hal_conf.h"
)

# Add format target
# Note that this doesn't run automatically, you need to run it manually
# by executing `cmake --build . --target format` in the build directory.
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT}
        -i
        -style=file
        ${ALL_C_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
        VERBATIM
    )

    # Add check-format target that fails if formatting would change files
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT}
        --dry-run
        --Werror
        -style=file
        ${ALL_C_SOURCE_FILES}
        COMMENT "Checking code formatting with clang-format"
        VERBATIM
    )
endif(CLANG_FORMAT)

# Automatic linting with clang-tidy
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
    # Find the arm-none-eabi toolchain include directory
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-sysroot
        OUTPUT_VARIABLE ARM_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(ARM_SYSROOT)
        set(ARM_INCLUDE_DIR "${ARM_SYSROOT}/include")
    else(ARM_SYSROOT)
        # Fallback: try to find include directory relative to compiler
        get_filename_component(COMPILER_DIR ${CMAKE_C_COMPILER} DIRECTORY)
        set(ARM_INCLUDE_DIR "${COMPILER_DIR}/../arm-none-eabi/include")
        if(NOT EXISTS ${ARM_INCLUDE_DIR})
            # Another fallback for common locations
            set(ARM_INCLUDE_DIR "/usr/lib/arm-none-eabi/include")
        endif(NOT EXISTS ${ARM_INCLUDE_DIR})
    endif(ARM_SYSROOT)

    add_custom_target(lint
        COMMAND ${CLANG_TIDY}
        --quiet
        -p=${CMAKE_BINARY_DIR}
        --extra-arg=--target=arm-none-eabi
        --extra-arg=-isystem${ARM_INCLUDE_DIR}
        --extra-arg=-isystem${CMAKE_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0/Drivers/CMSIS/Device/ST/STM32H5xx/Include
        --extra-arg=-isystem${CMAKE_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0/Drivers/CMSIS/Include
        --extra-arg=-isystem${CMAKE_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0/Drivers/STM32H5xx_HAL_Driver/Inc
        --extra-arg=-isystem${CMAKE_SOURCE_DIR}/lib/STM32H5_CMSIS_HAL-1.5.0/Drivers/STM32H5xx_HAL_Driver/Inc/Legacy
        --extra-arg=-isystem${CMAKE_SOURCE_DIR}/lib/tinyusb-0.18.0/src
        --extra-arg-before=-Qunused-arguments
        ${ALL_C_SOURCE_FILES}
        COMMENT "Linting code with clang-tidy"
        VERBATIM
    )
endif(CLANG_TIDY)

target_include_directories(pslab-mini-firmware
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(application)
add_subdirectory(system)
add_subdirectory(platform)

# Link the platform library
target_link_libraries(pslab-mini-firmware
    PRIVATE
        pslab-platform
)

# Add platform-specific linker script
if(PLATFORM STREQUAL "h563xx")
    stm32_add_linker_script(pslab-mini-firmware PRIVATE platform/h563xx/STM32H563ZITX_FLASH.ld)
endif()

stm32_print_size_of_target(pslab-mini-firmware)
stm32_generate_srec_file(pslab-mini-firmware)
