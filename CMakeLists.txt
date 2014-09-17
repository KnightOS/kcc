cmake_minimum_required(VERSION 2.8.5)
project(kcc C)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")
add_definitions("-Wall")

set(LIBS ${LIBS})

include_directories(
    src/
)

FILE(GLOB sdcc src/*.c)
FILE(GLOB z80 src/z80/*.c)

add_executable(kcc
    ${sdcc}
    ${z80}
)

install(PROGRAMS bin/kcc
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
