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
	find_program(WL3_MAKE NAMES make gmake)
	if(NOT WL3_MAKE)
		message(FATAL_ERROR
			"Building the ROM needs GNU make (and RGBDS). On Windows, build from an "
			"MSYS2 UCRT64 shell (see port/README.md), or build warioland3.gbc yourself "
			"and configure with -DWL3_BUILD_ROM=OFF.")
	endif()
	add_custom_target(rom ALL
		COMMAND ${WL3_MAKE} -C ${REPO_ROOT} warioland3.gbc RGBDS=${RGBDS}
		BYPRODUCTS ${ROM} ${SYM}
		COMMENT "Building warioland3.gbc from the disassembly"
		USES_TERMINAL)
endif()

# Code generator: symbol constants and the ported routine table.
# When cross-compiling, point WL3_GEN at a generator built for the host.
set(WL3_GEN "" CACHE FILEPATH "Prebuilt tools/gen.c executable (for cross-compiling)")
if(WL3_GEN)
	set(GEN_CMD ${WL3_GEN})
else()
	add_executable(wl3_gen tools/gen.c)
	set(GEN_CMD wl3_gen)
endif()

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

add_custom_command(
	OUTPUT ${GEN_DIR}/syms.h
	COMMAND ${GEN_CMD} syms ${SYM} ${GEN_DIR}/syms.h
	DEPENDS ${GEN_CMD} ${SYM}
	COMMENT "Generating syms.h from warioland3.sym")

# Constants: let rgbasm evaluate everything in src/includes.asm and dump it.
find_program(WL3_RGBASM NAMES rgbasm HINTS ${RGBDS} REQUIRED)
file(GLOB ASM_CONSTANT_SOURCES CONFIGURE_DEPENDS
	${REPO_ROOT}/src/includes.asm ${REPO_ROOT}/src/charmaps.asm
	${REPO_ROOT}/src/constants/* ${REPO_ROOT}/src/macros/*)
file(WRITE ${GEN_DIR}/empty.asm "")
add_custom_command(
	OUTPUT ${GEN_DIR}/constants.h
	COMMAND ${WL3_RGBASM} -P includes.asm -I ${REPO_ROOT}/src/
		-s equ:${GEN_DIR}/constants.asm -o ${GEN_DIR}/constants.o ${GEN_DIR}/empty.asm
	COMMAND ${GEN_CMD} consts ${GEN_DIR}/constants.asm ${GEN_DIR}/constants.h
	DEPENDS ${GEN_CMD} ${ASM_CONSTANT_SOURCES}
	COMMENT "Generating constants.h from the disassembly's constants")

file(GLOB_RECURSE PORTED_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/ported/*.c)
add_custom_command(
	OUTPUT ${GEN_DIR}/ported_table.c
	COMMAND ${GEN_CMD} ports ${GEN_DIR}/ported_table.c ${PORTED_SOURCES}
	DEPENDS ${GEN_CMD} ${PORTED_SOURCES}
	COMMENT "Generating the ported routine table")

add_executable(wl3_progress tools/progress.c)
file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS ${REPO_ROOT}/src/*.asm)
list(JOIN ASM_SOURCES "\n" ASM_SOURCE_LIST)
file(WRITE ${GEN_DIR}/asm_sources.txt "${ASM_SOURCE_LIST}\n")

add_executable(wl3
	src/apu.c
	src/cpu.c
	src/difftest.c
	src/gb.c
	src/hooks.c
	src/input.c
	src/main.c
	src/mem.c
	src/png.c
	src/ported.c
	src/ppu.c
	src/savestate.c
	src/trace.c
	${PORTED_SOURCES}
	${GEN_DIR}/syms.h
	${GEN_DIR}/constants.h
	${GEN_DIR}/ported_table.c)
target_include_directories(wl3 PRIVATE src ${GEN_DIR})
# SDL2main (Windows entry point) must come before SDL2 on the link line.
if(TARGET SDL2::SDL2main)
	target_link_libraries(wl3 PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(wl3 PRIVATE SDL2::SDL2)
if(WL3_BUILD_ROM)
	add_dependencies(wl3 rom)
endif()
# On Windows, put the DLLs wl3.exe needs (SDL2.dll) next to it, so it runs
# from Explorer or a .bat file and not just from the MSYS2 shell. Runs on
# every build (copying only what changed), so an existing build gets them too.
if(WIN32 AND NOT CMAKE_VERSION VERSION_LESS 3.21)
	add_custom_target(wl3_dlls ALL
		# wl3.exe itself is listed too (copying it onto itself is a no-op) so
		# the command stays valid when there are no DLLs, e.g. static SDL2.
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:wl3> $<TARGET_FILE:wl3> $<TARGET_FILE_DIR:wl3>
		COMMAND_EXPAND_LISTS
		DEPENDS wl3)
endif()

if(MSVC)
	target_compile_options(wl3 PRIVATE /W3)
else()
	target_compile_options(wl3 PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Run from the build directory with the freshly built ROM by default.
target_compile_definitions(wl3 PRIVATE WL3_DEFAULT_ROM="${ROM}")

enable_testing()
# One difftest per input script in tests/inputs.
file(GLOB TEST_INPUTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/inputs/*.input)
set(COVERAGE_COMMANDS)
set(COVERAGE_ARGS)
foreach(input ${TEST_INPUTS})
	get_filename_component(name ${input} NAME_WE)
	add_test(NAME difftest_${name} COMMAND wl3 --difftest --input ${input})
	# The whole port against the asm, frame by frame.
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/traces)
	add_test(NAME trace_${name} COMMAND ${CMAKE_COMMAND} -DWL3=$<TARGET_FILE:wl3> -DINPUT=${input}
		-DTRACE=${CMAKE_CURRENT_BINARY_DIR}/traces/${name}.trace -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/trace_test.cmake)
	# The fuzz run of one input takes far longer than its replay, so it is
	# split into shards that ctest -j can run side by side.
	foreach(shard RANGE 0 3)
		add_test(NAME fuzz_${name}_${shard} COMMAND wl3 --fuzz --input ${input} --fuzz-shard ${shard}/4)
	endforeach()
	list(APPEND COVERAGE_COMMANDS COMMAND wl3 --coverage ${CMAKE_CURRENT_BINARY_DIR}/coverage/${name}.txt --input ${input})
	list(APPEND COVERAGE_ARGS --coverage ${CMAKE_CURRENT_BINARY_DIR}/coverage/${name}.txt)
endforeach()

# Porting progress report: cmake --build build --target progress
# Runs every test input for coverage first.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/coverage)
add_custom_target(progress
	${COVERAGE_COMMANDS}
	COMMAND wl3_progress ${REPO_ROOT} ${GEN_DIR}/asm_sources.txt
		${CMAKE_CURRENT_BINARY_DIR}/progress.md ${COVERAGE_ARGS} ${PORTED_SOURCES}
	DEPENDS wl3_progress wl3
	USES_TERMINAL)
