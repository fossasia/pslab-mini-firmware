target_sources(tinyusb
    PRIVATE
        tusb.c
)

target_include_directories(tinyusb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(class)
add_subdirectory(common)
add_subdirectory(device)
add_subdirectory(portable)
add_subdirectory(typec)
