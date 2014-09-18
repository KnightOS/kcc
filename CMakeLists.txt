cmake_minimum_required(VERSION 2.8.5)
project(kcc CXX)
set(CMAKE_C_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
add_definitions("-Wall -Wno-parentheses")
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(Boost REQUIRED)

set(LIBS ${LIBS})

include_directories(
    src/
    support/util/
)

FILE(GLOB sdcc src/*.c)
FILE(GLOB sdcc_cxx src/*.cc)
FILE(GLOB z80 src/z80/*.c)
FILE(GLOB support support/util/*.c)
FILE(GLOB peep_defs src/z80/*.def)
FILE(GLOB peep_rules src/z80/*.rul)

BISON_TARGET(SDCCy src/SDCC.y src/SDCCy.c)

FLEX_TARGET(SDCClex src/SDCC.lex src/SDCClex.c)

foreach(peep ${peep_defs})
    execute_process(COMMAND basename -z --suffix=.def ${peep}
        OUTPUT_VARIABLE rules
    )
    set(rules ${CMAKE_SOURCE_DIR}/src/z80/${rules}.rul)
    add_custom_command(OUTPUT ${rules}
        COMMAND awk -f ${CMAKE_SOURCE_DIR}/src/SDCCpeeph.awk ${peep} > ${rules}
        DEPENDS ${peep_defs}
    )
    list(APPEND peep_rules ${rules})
endforeach()

add_executable(kcc
    ${BISON_SDCCy_OUTPUTS}
    ${FLEX_SDCClex_OUTPUTS}
    ${sdcc}
    ${sdcc_cxx}
    ${z80}
    ${peep_rules}
    ${support}
)

target_link_libraries(kcc m ${FLEX_LIBRARIES} ${BOOST_LIBRARIES} ${LIBS})

install(PROGRAMS bin/kcc
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
