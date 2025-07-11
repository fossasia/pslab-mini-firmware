add_library(cexception STATIC
    CException.c
    CException.h
)

target_include_directories(cexception
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
