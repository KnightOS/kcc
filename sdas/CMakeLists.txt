cmake_minimum_required(VERSION 2.8.5)
project(kcc C CXX)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
add_definitions("-Wall -Wno-parentheses")

set(LIBS ${LIBS})

include_directories(
    ${PROJECT_SOURCE_DIR}/asxxx/
    ${PROJECT_SOURCE_DIR}/asz80/
    ${PROJECT_SOURCE_DIR}/linksrc/
    ${PROJECT_SOURCE_DIR}/../support/util/
)

FILE(GLOB asxxx ${PROJECT_SOURCE_DIR}/asxxxsrc/*.c)
FILE(GLOB asz80 ${PROJECT_SOURCE_DIR}/asz80/*.c)

add_executable(kas
    ${asxxx}
)

target_link_libraries(kas m ${LIBS})

install(PROGRAMS ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kas
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
