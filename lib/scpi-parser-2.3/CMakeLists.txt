file(GLOB SCPI_SRC "src/*.c")
add_library(scpi-parser STATIC ${SCPI_SRC})
target_include_directories(scpi-parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
