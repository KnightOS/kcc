cmake_minimum_required(VERSION 2.8.5)
project(kcc C)

add_subdirectory(sdcc sdcc)
add_subdirectory(sdas sdas)

set(CMAKE_BUILD_TYPE Release)
