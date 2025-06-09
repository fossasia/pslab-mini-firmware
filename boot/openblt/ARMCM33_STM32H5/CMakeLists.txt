add_subdirectory(GCC)

target_sources(pslab-mini-bootloader
    PRIVATE
        can.c
        cpu.c
        flash.c
        mbrtu.c
        nvm.c
        rs232.c
        timer.c
        usb.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
