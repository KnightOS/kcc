cmake_minimum_required(VERSION 2.8.5)

project(kcc C CXX)

set(DEBUG_FLAGS "-g -O0")

set(CMAKE_CXX_FLAGS_DEBUG "${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${DEBUG_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11")

if (NOT DEFINED KCC_DEFAULT_PREPROCESSOR)
	set(KCC_DEFAULT_PREPROCESSOR "cpp" CACHE PATH "The preprocessor KCC should use if none is explicitly specified")
endif()

if (NOT DEFINED KCC_DOCUMENTATION)
	set(KCC_DOCUMENTATION FALSE CACHE BOOL "Whether to build documentation")
endif()

add_definitions("-DKCC_DEFAULT_PREPROCESSOR=\"${KCC_DEFAULT_PREPROCESSOR}\"")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(Boost REQUIRED)
find_program(SCDOC NAMES scdoc)
if (SCDOC-NOTFOUND)
	set(KCC_DOCUMENTATION FALSE CACHE BOOL "Whether to build documentation")
endif()

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "")
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/src/
    ${PROJECT_SOURCE_DIR}/src/util/
    ${PROJECT_BINARY_DIR}/gen/
)

BISON_TARGET(SDCCy ${PROJECT_SOURCE_DIR}/src/SDCC.y ${PROJECT_BINARY_DIR}/gen/SDCCy.c)
FLEX_TARGET(SDCClex ${PROJECT_SOURCE_DIR}/src/SDCC.lex ${PROJECT_BINARY_DIR}/gen/SDCClex.c)

FILE(GLOB SOURCES
	${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cc
	${PROJECT_SOURCE_DIR}/src/backend/*.c ${PROJECT_SOURCE_DIR}/src/backend/*.cc
	${PROJECT_SOURCE_DIR}/src/util/*.c
	${PROJECT_SOURCE_DIR}/src/backend/*.rul
)
FILE(GLOB peep_defs ${PROJECT_SOURCE_DIR}/src/backend/*.def)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/gen)

foreach(peep ${peep_defs})
    execute_process(COMMAND sh -c "basename '${peep}' .def | tr -d '\\n'"
        OUTPUT_VARIABLE rules
    )
    set(rules ${PROJECT_SOURCE_DIR}/src/backend/${rules}.rul)
    add_custom_command(OUTPUT ${rules}
        COMMAND awk -f ${PROJECT_SOURCE_DIR}/src/SDCCpeeph.awk ${peep} > ${rules}
        DEPENDS ${peep_defs}
    )
    list(APPEND peep_rules ${rules})
endforeach()

add_executable(kcc
    ${BISON_SDCCy_OUTPUTS}
    ${FLEX_SDCClex_OUTPUTS}
    ${SOURCES}
    ${peep_rules}
)

if (KCC_DOCUMENTATION)
	add_custom_target(man ALL DEPENDS ${PROJECT_BINARY_DIR}/kcc.1)
	add_custom_command(
		DEPENDS ${PROJECT_SOURCE_DIR}/kcc.1.scdoc
		COMMAND ${SCDOC} <${PROJECT_SOURCE_DIR}/kcc.1.scdoc >${PROJECT_BINARY_DIR}/kcc.1
		OUTPUT ${PROJECT_BINARY_DIR}/kcc.1
	)
	install(FILES ${PROJECT_BINARY_DIR}/kcc.1 DESTINATION share/man/man1)
endif()

target_link_libraries(kcc m ${BOOST_LIBRARIES} ${LIBS})

install(TARGETS kcc DESTINATION bin)
