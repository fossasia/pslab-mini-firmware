target_sources(pslab-mini-bootloader
    PRIVATE
        tusb_fifo.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
