target_sources(pslab-mini-firmware
    PRIVATE
        bus_common.c
)

target_include_directories(pslab-mini-firmware
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(uart)
add_subdirectory(usb)
