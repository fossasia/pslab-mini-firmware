target_sources(tinyusb
    PRIVATE
        typec_stm32.c
)

target_include_directories(tinyusb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
