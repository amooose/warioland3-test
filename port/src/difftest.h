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

#endif
