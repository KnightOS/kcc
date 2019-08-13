cmake_minimum_required(VERSION 2.8.5)
project(kcc C CXX)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
add_definitions("-Wall -Wno-parentheses")
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

set(LIBS ${LIBS})

include_directories(
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/../support/util/
    ${PROJECT_BINARY_DIR}/gen/
)

if (NOT DEFINED KCC_DEFAULT_PREPROCESSOR)
	set(KCC_DEFAULT_PREPROCESSOR "cpp" CACHE PATH "The preprocessor KCC should use if none is explicitly specified")
endif()

add_definitions("-DKCC_DEFAULT_PREPROCESSOR=\"${KCC_DEFAULT_PREPROCESSOR}\"")

FILE(GLOB sdcc ${PROJECT_SOURCE_DIR}/*.c)
FILE(GLOB sdcc_cxx ${PROJECT_SOURCE_DIR}/*.cc)
FILE(GLOB z80 ${PROJECT_SOURCE_DIR}/z80/*.c)
FILE(GLOB z80_cxx ${PROJECT_SOURCE_DIR}/z80/*.cc)
FILE(GLOB support ${PROJECT_SOURCE_DIR}/../support/util/*.c)
FILE(GLOB peep_rules ${PROJECT_SOURCE_DIR}/z80/*.rul)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/gen)

BISON_TARGET(SDCCy ${PROJECT_SOURCE_DIR}/SDCC.y ${PROJECT_BINARY_DIR}/gen/SDCCy.c)

FLEX_TARGET(SDCClex ${PROJECT_SOURCE_DIR}/SDCC.lex ${PROJECT_BINARY_DIR}/gen/SDCClex.c)

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

target_link_libraries(kcc m ${BOOST_LIBRARIES} ${LIBS})

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kcc
    DESTINATION bin)

set(CMAKE_BUILD_TYPE Release)
