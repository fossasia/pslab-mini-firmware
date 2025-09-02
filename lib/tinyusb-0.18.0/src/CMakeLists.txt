target_sources(tinyusb
    PRIVATE
        tusb.c
)

target_include_directories(tinyusb
    INTERFACE
        # Include path for TinyUSB starts with "lib/tinyusb/..."
        ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    PUBLIC
        # TinyUSB's headers include each other without relative paths
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(class)
add_subdirectory(common)
add_subdirectory(device)
add_subdirectory(portable)
add_subdirectory(typec)
