cmake_minimum_required(VERSION 3.16)
project(wl3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL2 REQUIRED)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SYM ${REPO_ROOT}/warioland3.sym)

# The port runs on the original Wario Land 3 ROM. For development it's
# built from the disassembly (WL3_BUILD_ROM, the default; needs RGBDS),
# which also regenerates the label and constant headers. Without it, the
# build uses the copies committed in port/generated and needs nothing from
# the disassembly; point WL3_ROM at your own ROM to run the tests.
#
# Set RGBDS to the directory containing rgbasm etc. if they're not on the
# PATH, e.g. -DRGBDS=/opt/rgbds/
set(RGBDS "" CACHE STRING "RGBDS install prefix (with trailing slash), if not on PATH")
option(WL3_BUILD_ROM "Build the ROM from the disassembly as part of the port build" ON)
set(WL3_ROM "" CACHE FILEPATH "Your Wario Land 3 ROM, when not building it (WL3_BUILD_ROM=OFF)")
if(WL3_BUILD_ROM)
	set(ROM ${REPO_ROOT}/warioland3.gbc)
	find_program(WL3_MAKE NAMES make gmake)
	if(NOT WL3_MAKE)
		message(FATAL_ERROR
			"Building the ROM needs GNU make (and RGBDS). On Windows, build from an "
			"MSYS2 UCRT64 shell (see port/README.md), or configure with "
			"-DWL3_BUILD_ROM=OFF and run the port on your own ROM.")
	endif()
	add_custom_target(rom ALL
		COMMAND ${WL3_MAKE} -C ${REPO_ROOT} warioland3.gbc RGBDS=${RGBDS}
		BYPRODUCTS ${ROM} ${SYM}
		COMMENT "Building warioland3.gbc from the disassembly"
		USES_TERMINAL)
else()
	set(ROM ${WL3_ROM})
endif()

# Code generator: symbol constants, the symbol table and the ported routine
# table. When cross-compiling, point WL3_GEN at a generator built for the host.
set(WL3_GEN "" CACHE FILEPATH "Prebuilt tools/gen.c executable (for cross-compiling)")
if(WL3_GEN)
	set(GEN_CMD ${WL3_GEN})
else()
	add_executable(wl3_gen tools/gen.c src/sha1.c)
	set(GEN_CMD wl3_gen)
endif()

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

# What the port takes from the disassembly: its labels (syms.h, sym_table.c)
# and constants (constants.h).
set(COMMITTED_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
set(DISASM_GENERATED syms.h sym_table.c constants.h)
if(WL3_BUILD_ROM)
	set(DISASM_GEN_DIR ${GEN_DIR})
	add_custom_command(
		OUTPUT ${GEN_DIR}/syms.h
		COMMAND ${GEN_CMD} syms ${SYM} ${GEN_DIR}/syms.h
		DEPENDS ${GEN_CMD} ${SYM}
		COMMENT "Generating syms.h from warioland3.sym")
	add_custom_command(
		OUTPUT ${GEN_DIR}/sym_table.c
		COMMAND ${GEN_CMD} symtab ${SYM} ${ROM} ${GEN_DIR}/sym_table.c
		DEPENDS ${GEN_CMD} ${SYM} ${ROM}
		COMMENT "Generating sym_table.c from warioland3.sym")

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

	# After changing labels or constants in the asm, refresh the committed
	# copies: cmake --build build --target update_generated
	set(UPDATE_COMMANDS)
	foreach(f ${DISASM_GENERATED})
		list(APPEND UPDATE_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${GEN_DIR}/${f} ${COMMITTED_GEN_DIR}/${f})
	endforeach()
	add_custom_target(update_generated ${UPDATE_COMMANDS}
		DEPENDS ${GEN_DIR}/syms.h ${GEN_DIR}/sym_table.c ${GEN_DIR}/constants.h
		COMMENT "Copying the generated headers to port/generated")
else()
	set(DISASM_GEN_DIR ${COMMITTED_GEN_DIR})
endif()

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

# The interpreter runs the original code, which the difftest, the fuzzer and
# the traces compare the native routines against. The game itself runs
# without it (every routine is native): -DWL3_INTERPRETER=OFF builds just
# the game, with none of the tests or development tools.
option(WL3_INTERPRETER "Include the SM83 interpreter and the tools that use it" ON)
if(WL3_INTERPRETER)
	set(INTERPRETER_SOURCES src/interp.c src/difftest.c src/trace.c)
else()
	set(INTERPRETER_SOURCES)
endif()

add_executable(wl3
	src/apu.c
	src/cpu.c
	${INTERPRETER_SOURCES}
	src/gb.c
	src/hooks.c
	src/input.c
	src/main.c
	src/mem.c
	src/png.c
	src/ported.c
	src/ppu.c
	src/savestate.c
	${PORTED_SOURCES}
	src/sha1.c
	${DISASM_GEN_DIR}/syms.h
	${DISASM_GEN_DIR}/constants.h
	${DISASM_GEN_DIR}/sym_table.c
	${GEN_DIR}/ported_table.c)
# The disassembly's headers first: an old build directory may still hold
# its own copies in generated/.
target_include_directories(wl3 PRIVATE src ${DISASM_GEN_DIR} ${GEN_DIR})
if(WL3_INTERPRETER)
	target_compile_definitions(wl3 PRIVATE WL3_INTERPRETER=1)
else()
	target_compile_definitions(wl3 PRIVATE WL3_INTERPRETER=0)
endif()
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

# Run from the build directory with the built (or given) ROM by default.
if(ROM)
	target_compile_definitions(wl3 PRIVATE WL3_DEFAULT_ROM="${ROM}")
endif()

enable_testing()
if(WL3_BUILD_ROM)
	# The committed copies of the generated headers must match the
	# disassembly (cmake --build build --target update_generated).
	foreach(f ${DISASM_GENERATED})
		add_test(NAME generated_${f} COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol
			${GEN_DIR}/${f} ${COMMITTED_GEN_DIR}/${f})
		set_tests_properties(generated_${f} PROPERTIES LABELS quick)
	endforeach()
endif()
# One difftest per input script in tests/inputs. They need the ROM and the
# interpreter.
if(ROM AND WL3_INTERPRETER)
	file(GLOB TEST_INPUTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/inputs/*.input)
else()
	if(NOT ROM)
		message(STATUS "No ROM: set WL3_ROM to your Wario Land 3 ROM to run the tests")
	endif()
	set(TEST_INPUTS)
endif()
set(COVERAGE_COMMANDS)
set(COVERAGE_ARGS)
foreach(input ${TEST_INPUTS})
	get_filename_component(name ${input} NAME_WE)
	add_test(NAME difftest_${name} COMMAND wl3 --difftest --input ${input})
	# The whole port against the asm, frame by frame.
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/traces)
	add_test(NAME trace_${name} COMMAND ${CMAKE_COMMAND} -DWL3=$<TARGET_FILE:wl3> -DINPUT=${input}
		-DTRACE=${CMAKE_CURRENT_BINARY_DIR}/traces/${name}.trace -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/trace_test.cmake)
	# The quick tier (ctest -L quick, a few minutes): run before every commit.
	set_tests_properties(difftest_${name} trace_${name} PROPERTIES LABELS quick)
	# The fuzz run of one input takes far longer than its replay, so it is
	# split into shards that ctest -j can run side by side. The whole fuzz
	# tier (ctest -L fuzz, about an hour) is for when something looks wrong;
	# new routines are fuzzed on their own with tools/fuzz_batch.sh.
	foreach(shard RANGE 0 3)
		add_test(NAME fuzz_${name}_${shard} COMMAND wl3 --fuzz --input ${input} --fuzz-shard ${shard}/4)
		set_tests_properties(fuzz_${name}_${shard} PROPERTIES LABELS fuzz)
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
