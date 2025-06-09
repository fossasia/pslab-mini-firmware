add_subdirectory(ARMCM33_STM32H5)

target_sources(pslab-mini-bootloader
    PRIVATE
        asserts.c
        backdoor.c
        boot.c
        com.c
        cop.c
        file.c
        infotable.c
        mb.c
        net.c
        xcp.c
)

target_include_directories(pslab-mini-bootloader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
