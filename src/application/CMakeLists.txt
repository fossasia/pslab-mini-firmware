# Application layer - only built for target

target_sources(pslab-mini-firmware
    PRIVATE
        main.c
        protocol.c
)

target_link_libraries(pslab-mini-firmware
    PRIVATE
        scpi-parser
)

target_include_directories(pslab-mini-firmware
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
