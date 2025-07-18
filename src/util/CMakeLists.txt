add_library(pslab-util STATIC)

target_sources(pslab-util PRIVATE
    circular_buffer.c
    logging.c
)

target_include_directories(pslab-util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pslab-util PUBLIC
    cexception
)
