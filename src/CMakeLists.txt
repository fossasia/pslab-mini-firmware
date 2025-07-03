add_executable(pslab-mini-firmware)
set_property(TARGET pslab-mini-firmware PROPERTY C_STANDARD 23)

# Add clang-format target
# Note that this doesn't run automatically, you need to run it manually
# by executing `cmake --build . --target format` in the build directory.
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    file(GLOB_RECURSE
        ALL_C_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT}
        -i
        -style=file
        ${ALL_C_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
        VERBATIM
    )
endif(CLANG_FORMAT)

# Automatic linting with clang-tidy
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
    set(_clang_tidy_cmd
        clang-tidy
        --extra-arg=--target=arm-none-eabi
        --extra-arg=-isystem/usr/lib/arm-none-eabi/include
        --extra-arg-before=-Qunused-arguments
        --fix
    )

    set_target_properties(pslab-mini-firmware
        PROPERTIES
            C_CLANG_TIDY
            "${_clang_tidy_cmd}"
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
        tinyusb
)

# Add platform-specific linker script
if(PLATFORM STREQUAL "h563xx")
    stm32_add_linker_script(pslab-mini-firmware PRIVATE platform/h563xx/STM32H563ZITX_FLASH.ld)
endif()

stm32_print_size_of_target(pslab-mini-firmware)
stm32_generate_srec_file(pslab-mini-firmware)
