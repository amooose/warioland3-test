#ifndef DIFFTEST_H
#define DIFFTEST_H

#include "gb.h"
#include "input.h"

// Play the game in the pure interpreter from its current state until
// end_frame (optionally with scripted input). Every time a routine that has a native
// hook is called, run both the original asm and the hook from that exact
// state and compare the resulting machine state. Returns true if every
// comparison matched.
bool difftest_run(GB *gb, const InputScript *input, int end_frame);

// Play the game in the pure interpreter until end_frame and write how many
// times each global ROM label was entered, one "<name> <count>" per line.
bool coverage_run(GB *gb, const InputScript *input, int end_frame, const char *out_path);

// Fuzz every ported routine (or just `only`): replay the input in the pure
// interpreter, and from a few real calls of each routine (or a synthetic
// call, for routines the replay never reaches) check many variations with
// random registers and mutated memory. Reports how much of each routine's
// asm the checks executed. Returns true if every check matched.
void fuzz_configure(int variants, const char *only, bool verbose);
bool fuzz_run(GB *gb, const InputScript *input, int end_frame);

#endif
