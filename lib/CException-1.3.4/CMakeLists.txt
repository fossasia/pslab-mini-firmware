add_library(cexception STATIC
    CException.c
    CException.h
)

target_include_directories(cexception
    INTERFACE
        # Include path for CException starts with "lib/CException/..."
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
