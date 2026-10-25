// Save states: the complete machine state, in a portable format.
//
// A state is tied to the exact ROM it was made with; loading one made with a
// different build of the disassembly is refused, since code and data
// addresses would no longer line up.
#ifndef SAVESTATE_H
#define SAVESTATE_H

#include "gb.h"

// Serialize into a newly allocated buffer.
uint8_t *savestate_save(GB *gb, size_t *len);
// Returns false (and leaves gb untouched) on a bad or mismatched state.
bool savestate_load(GB *gb, const uint8_t *data, size_t len);

bool savestate_save_file(GB *gb, const char *path);
bool savestate_load_file(GB *gb, const char *path);

#endif
