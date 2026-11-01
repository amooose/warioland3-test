// Home's input and system routines as plain functions (src/home/joypad.asm,
// dpad_repeat.asm, double_speed.asm, hcall.asm, clear_save.asm), for C++
// modules to call directly; their hooks (system.cpp) wrap them for
// register-level callers, leaving the registers the asm leaves.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// UpdateJoypad: wJoypadDown (d-pad high, buttons low) and wJoypadPressed
// (the bits newly set)
void update_joypad(GB *gb);
// ProcessDPadRepeat: a d-pad direction held repeats (first after $10
// frames, then every 8) by adding it to wJoypadPressed; returns the
// directions added, or 0
uint8_t process_dpad_repeat(GB *gb);
// EnableDoubleSpeed; false if the CPU already ran at double speed
bool enable_double_speed(GB *gb);
void init_hram_call_func(GB *gb); // InitHRAMCallFunc: hCallFunc's code to HRAM
// ClearSaveData: the save's check values, so the game starts afresh (the
// asm then jumps to Init)
void clear_save_data(GB *gb);

} // namespace wl3::home
