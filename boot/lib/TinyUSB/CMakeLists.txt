add_subdirectory(cfg)
add_subdirectory(port)
add_subdirectory(src)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
