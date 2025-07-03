add_executable(pslab-mini-firmware)

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
