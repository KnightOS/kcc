cmake_minimum_required(VERSION 2.8.5)
project(kcc C CXX)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
add_definitions("-Wall -Wno-parentheses")
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(Boost REQUIRED)

set(LIBS ${LIBS})

include_directories(
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/../support/util/
    ${PROJECT_BINARY_DIR}/gen/
)

FILE(GLOB sdcc ${PROJECT_SOURCE_DIR}/*.c)
FILE(GLOB sdcc_cxx ${PROJECT_SOURCE_DIR}/*.cc)
FILE(GLOB z80 ${PROJECT_SOURCE_DIR}/z80/*.c)
FILE(GLOB z80_cxx ${PROJECT_SOURCE_DIR}/z80/*.cc)
FILE(GLOB support ${PROJECT_SOURCE_DIR}/../support/util/*.c)
FILE(GLOB peep_defs ${PROJECT_SOURCE_DIR}/z80/*.def)
FILE(GLOB peep_rules ${PROJECT_SOURCE_DIR}/z80/*.rul)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/gen)

BISON_TARGET(SDCCy ${PROJECT_SOURCE_DIR}/SDCC.y ${PROJECT_BINARY_DIR}/gen/SDCCy.c)

FLEX_TARGET(SDCClex ${PROJECT_SOURCE_DIR}/SDCC.lex ${PROJECT_BINARY_DIR}/gen/SDCClex.c)

foreach(peep ${peep_defs})
    execute_process(COMMAND basename -z --suffix=.def ${peep}
        OUTPUT_VARIABLE rules
    )
    set(rules ${PROJECT_SOURCE_DIR}/z80/${rules}.rul)
    add_custom_command(OUTPUT ${rules}
        COMMAND awk -f ${PROJECT_SOURCE_DIR}/SDCCpeeph.awk ${peep} > ${rules}
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
    ${z80_cxx}
    ${peep_rules}
    ${support}
)

target_link_libraries(kcc m ${FLEX_LIBRARIES} ${BOOST_LIBRARIES} ${LIBS})

install(PROGRAMS bin/kcc
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
