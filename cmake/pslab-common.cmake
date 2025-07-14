# Common configuration shared between target and test builds

set(CMAKE_C_FLAGS_DEBUG_INIT    "-Og -g")
set(CMAKE_CXX_FLAGS_DEBUG_INIT  "-Og -g")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Common source formatting and linting targets
function(setup_code_quality_targets)
    # Glob all C source files in the current directory and subdirectories
    # for clang-format and clang-tidy
    file(GLOB_RECURSE
        ALL_C_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.c
        ${CMAKE_SOURCE_DIR}/src/*.h
    )

    # Exclude CubeMX-generated files from linting
    list(
        REMOVE_ITEM
        ALL_C_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/platform/h563xx/system_stm32h5xx.c"
        "${CMAKE_SOURCE_DIR}/src/platform/h563xx/stm32h5xx_hal_conf.h"
    )

    # Add format target
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
    endif()

    # Automatic linting with clang-tidy (only for target builds)
    if(NOT BUILD_TESTS)
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
            else()
                # Fallback: try to find include directory relative to compiler
                get_filename_component(COMPILER_DIR ${CMAKE_C_COMPILER} DIRECTORY)
                set(ARM_INCLUDE_DIR "${COMPILER_DIR}/../arm-none-eabi/include")
                if(NOT EXISTS ${ARM_INCLUDE_DIR})
                    # Another fallback for common locations
                    set(ARM_INCLUDE_DIR "/usr/lib/arm-none-eabi/include")
                endif()
            endif()

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
                --warnings-as-errors=*
                ${ALL_C_SOURCE_FILES}
                COMMENT "Linting code with clang-tidy"
                VERBATIM
            )
        endif()
    endif()
endfunction()
