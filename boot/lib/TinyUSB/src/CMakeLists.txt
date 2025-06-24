add_subdirectory(class)
add_subdirectory(common)
add_subdirectory(device)
add_subdirectory(osal)

target_sources(pslab-mini-bootloader
    PRIVATE
        tusb.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
