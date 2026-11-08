// Home's palette routines as plain functions (src/home/load_pals.asm,
// bg_ob_pals.asm), for C++ modules to call directly. Their hooks
// (pals.cpp) wrap them for register-level callers, leaving the registers
// the asm leaves.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// CopyHLToDE_Short: count bytes (0 is 256), stepping only the destination's
// low byte (it stays in its page); returns the last byte
uint8_t copy_short(GB *gb, uint16_t src, uint16_t dest, int count);

// 8 palettes from src to the palette data register, from palette 0
// (LoadBG/OBPalettesFromWRAM, FillWhiteBG/OBPal, ApplyTempPals1ToBGPals,
// ApplyTempPals2ToOBPals); returns the last byte
uint8_t apply_bg_pals(GB *gb, uint16_t src);
uint8_t apply_ob_pals(GB *gb, uint16_t src);
// all 8 palettes read back to dest (StoreBGPals, StoreOBPals); returns the
// last byte
uint8_t store_bg_pals(GB *gb, uint16_t dest);
uint8_t store_ob_pals(GB *gb, uint16_t dest);

void load_pals_to_temp_pals1(GB *gb, uint16_t pals); // LoadPalsToTempPals1
void load_pals_to_temp_pals2(GB *gb, uint16_t pals); // LoadPalsToTempPals2
void load_obj_pals(GB *gb, uint16_t pals); // LoadObjPals: 4, to OB palettes 4-7

// Wario's palettes (in the "Wario Palettes" bank) to wTempPals2 and the
// first two OB palettes, a palette per H-Blank
void set_wario_pal(GB *gb, uint16_t pals); // SetWarioPal

// The collected treasure's palette, in OB palette 7 of wTempPals2 (the
// clear screen), or applied as OB palette 4 + the treasure's index (the
// level, in H-Blank)
void load_collected_treasure_pal_clear_screen(GB *gb); // LoadCollectedTreasurePal_ClearScreen
void load_collected_treasure_pal_level(GB *gb); // LoadCollectedTreasurePal_Level

// count OB palettes from src to dest and applied from OB palette `first`,
// a palette per H-Blank (CopyAndApplyOBPals; a count of 0 is 256)
void copy_and_apply_ob_pals(GB *gb, uint16_t src, uint16_t dest, uint8_t count, uint8_t first);
// SetBGPals / SetOBPals: count palettes from src, from palette `first`, a
// palette per H-Blank (a count of 0 is 256); returns the end of src (and
// the last byte in *last)
uint16_t set_pals(GB *gb, bool bg, uint16_t src, uint8_t count, uint8_t first, uint8_t *last = nullptr);
// BlackOutBGPals / BlackOutOBPals: count palettes from `first` black, a
// palette per H-Blank
void black_out_pals(GB *gb, bool bg, uint8_t count, uint8_t first);

} // namespace wl3::home
