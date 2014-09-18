cmake_minimum_required(VERSION 2.8.5)
project(kcc C CXX)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
add_definitions("-Wall -Wno-parentheses")

set(LIBS ${LIBS})

include_directories(
    ${PROJECT_SOURCE_DIR}/asxxsrc/
    ${PROJECT_SOURCE_DIR}/asz80/
    ${PROJECT_SOURCE_DIR}/linksrc/
    ${PROJECT_SOURCE_DIR}/../support/util/
)

FILE(GLOB asxx ${PROJECT_SOURCE_DIR}/asxxsrc/*.c)
FILE(GLOB asz80 ${PROJECT_SOURCE_DIR}/asz80/*.c)

add_executable(kas
    ${asxx}
    ${asz80}
)

target_link_libraries(kas m ${LIBS})

install(PROGRAMS ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kas
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
