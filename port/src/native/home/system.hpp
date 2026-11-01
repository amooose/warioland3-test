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

void wait_next_hblank(GB *gb); // WaitNextHBlank
void init_lcd(GB *gb); // InitLCD: no LCD interrupt, wLCDFunc a reti
void vblank_ret(GB *gb); // VBlank_Ret: wVBlankFunc a ret
void vblank_time_attack_description(GB *gb); // VBlank_TimeAttackDescription: its .Func to wVBlankFunc

// UpdateLevelMusic: hMusicID by the boss fight, the transformation (if it
// has music) or the level
void update_level_music(GB *gb);
void init_game_state(GB *gb); // InitGameState: the title
void set_perfect_state(GB *gb); // SetPerfectState
// SaveGameOrEnterPerfectScreen: the perfect screen after the last treasure
// of a cleared game, else the pause menu's save
void save_game_or_enter_perfect_screen(GB *gb);

// CheckHasAllMusicalCoinFlags: every level's flag (25 bits)
bool has_all_musical_coin_flags(GB *gb);

} // namespace wl3::home
