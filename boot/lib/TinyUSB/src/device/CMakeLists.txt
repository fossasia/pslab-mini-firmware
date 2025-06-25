target_sources(pslab-mini-bootloader
    PRIVATE
        usbd_control.c
        usbd.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
