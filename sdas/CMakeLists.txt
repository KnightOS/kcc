cmake_minimum_required(VERSION 2.8.5)
project(kcc C CXX)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
add_definitions("-Wall -Wno-parentheses -fms-extensions")

set(LIBS ${LIBS})

include_directories(
    ${PROJECT_SOURCE_DIR}/asxx/
    ${PROJECT_SOURCE_DIR}/asz80/
    ${PROJECT_SOURCE_DIR}/link/
    ${PROJECT_SOURCE_DIR}/../support/util/
)

FILE(GLOB asxx ${PROJECT_SOURCE_DIR}/asxx/*.c)
FILE(GLOB asz80 ${PROJECT_SOURCE_DIR}/asz80/*.c)
FILE(GLOB support ${PROJECT_SOURCE_DIR}/../support/util/*.c)
FILE(GLOB link ${PROJECT_SOURCE_DIR}/link/*.c)

add_executable(kas
    ${asxx}
    ${asz80}
    ${support}
)

add_executable(kld
    ${link}
    ${support}
)

target_link_libraries(kas m ${LIBS})
target_link_libraries(kld m ${LIBS})

install(PROGRAMS ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kas
    DESTINATION bin)
install(PROGRAMS ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kld
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
