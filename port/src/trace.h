// Per-frame traces: the whole machine state, hashed region by region once
// per game frame (each pass through the main loop), from the pure asm run
// of an input script. Checking a trace replays the same script with the
// native routines on and compares every frame, which tests the port as a
// whole rather than one routine at a time.
#ifndef TRACE_H
#define TRACE_H

#include "gb.h"
#include "input.h"

// Replay `input` in the pure interpreter until end_frame and write the
// trace to path.
bool trace_write(GB *gb, const InputScript *input, int end_frame, const char *path);

// Replay `input` with the native routines on, feeding each game frame the
// buttons the trace recorded for it, and compare every frame against the
// trace at path. At the first mismatch, rerun the asm to that frame and
// print the bytes that differ. Returns true if every frame matched.
bool trace_check(GB *gb, const InputScript *input, const char *path);

#endif
