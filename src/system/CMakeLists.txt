add_subdirectory(bus)
add_subdirectory(adc)

# Only add sources when not building tests
if(NOT BUILD_TESTS)
    target_sources(pslab-mini-firmware
        PRIVATE
            led.c
            system.c
            stubs.c
    )

    target_include_directories(pslab-mini-firmware
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
