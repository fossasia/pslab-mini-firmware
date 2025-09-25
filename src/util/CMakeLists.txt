add_library(pslab-util STATIC)

target_sources(pslab-util PRIVATE
    circular_buffer.c
    fixed_point.c
    logging.c
)

target_include_directories(pslab-util
    # Since many pslab-util components are header-only (and include each other)
    # they need to be able to use the "util" namespace
    PUBLIC
        # Include paths for pslab-util start with "util/..."
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pslab-util PUBLIC
    cexception
)
