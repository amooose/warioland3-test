cmake_minimum_required(VERSION 3.16)
project(wl3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL2 REQUIRED)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(ROM ${REPO_ROOT}/warioland3.gbc)
set(SYM ${REPO_ROOT}/warioland3.sym)

# The ROM is built from the disassembly with the repository's own Makefile
# (needs RGBDS). Set RGBDS to the directory containing rgbasm etc. if they're
# not on the PATH, e.g. -DRGBDS=/opt/rgbds/
set(RGBDS "" CACHE STRING "RGBDS install prefix (with trailing slash), if not on PATH")
option(WL3_BUILD_ROM "Build the ROM from the disassembly as part of the port build" ON)
if(WL3_BUILD_ROM)
	add_custom_target(rom ALL
		COMMAND make -C ${REPO_ROOT} warioland3.gbc RGBDS=${RGBDS}
		BYPRODUCTS ${ROM} ${SYM}
		COMMENT "Building warioland3.gbc from the disassembly"
		USES_TERMINAL)
endif()

# Code generator: symbol constants and the ported routine table.
add_executable(wl3_gen tools/gen.c)

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

add_custom_command(
	OUTPUT ${GEN_DIR}/syms.h
	COMMAND wl3_gen syms ${SYM} ${GEN_DIR}/syms.h
	DEPENDS wl3_gen ${SYM}
	COMMENT "Generating syms.h from warioland3.sym")

file(GLOB_RECURSE PORTED_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/ported/*.c)
add_custom_command(
	OUTPUT ${GEN_DIR}/ported_table.c
	COMMAND wl3_gen ports ${GEN_DIR}/ported_table.c ${PORTED_SOURCES}
	DEPENDS wl3_gen ${PORTED_SOURCES}
	COMMENT "Generating the ported routine table")

add_executable(wl3
	src/apu.c
	src/cpu.c
	src/difftest.c
	src/gb.c
	src/hooks.c
	src/main.c
	src/mem.c
	src/png.c
	src/ported.c
	src/ppu.c
	${PORTED_SOURCES}
	${GEN_DIR}/syms.h
	${GEN_DIR}/ported_table.c)
target_include_directories(wl3 PRIVATE src ${GEN_DIR})
target_link_libraries(wl3 PRIVATE SDL2::SDL2)
if(TARGET SDL2::SDL2main)
	target_link_libraries(wl3 PRIVATE SDL2::SDL2main)
endif()
if(WL3_BUILD_ROM)
	add_dependencies(wl3 rom)
endif()

if(MSVC)
	target_compile_options(wl3 PRIVATE /W3)
else()
	target_compile_options(wl3 PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Run from the build directory with the freshly built ROM by default.
target_compile_definitions(wl3 PRIVATE WL3_DEFAULT_ROM="${ROM}")

enable_testing()
add_test(NAME difftest COMMAND wl3 --difftest --frames 2400
	--input ${CMAKE_CURRENT_SOURCE_DIR}/tests/difftest.input)
