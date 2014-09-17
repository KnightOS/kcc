cmake_minimum_required(VERSION 2.8.5)
project(kcc C)
set(CMAKE_C_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")
add_definitions("-Wall -Wno-parentheses")
find_package(BISON)

set(LIBS ${LIBS})

BISON_TARGET(SDCCy src/SDCC.y src/SDCCy.c)

include_directories(
    src/
    support/util/
)

FILE(GLOB sdcc src/*.c)
FILE(GLOB z80 src/z80/*.c)

add_executable(kcc
    ${BISON_SDCCy_OUTPUTS}
    ${sdcc}
    ${z80}
)

install(PROGRAMS bin/kcc
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
