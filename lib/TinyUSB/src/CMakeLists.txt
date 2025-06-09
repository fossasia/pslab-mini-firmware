add_subdirectory(class)
add_subdirectory(common)
add_subdirectory(device)
add_subdirectory(osal)

target_sources(tiny-usb
    PRIVATE tusb.c
    PUBLIC tusb.h
    PUBLIC tusb_option.h
)

target_include_directories(tiny-usb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
