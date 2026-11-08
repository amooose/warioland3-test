// Home's object map positions as plain functions (src/home/object_pos.asm),
// for C++ modules to call directly. The object map (in SRAM, a byte per 2
// blocks across: two object indices, a nybble each) is addressed by a row
// page ($a0 up) and a column byte ($b0 up); the parity picks the nybble
// (1: the even block, the high nybble). Their hooks (object_pos.cpp) wrap
// them for register-level callers, leaving the registers the asm does (a
// and f aside, which no caller reads: live.py).
#pragma once

#include "../native.hpp"

namespace wl3::home {

struct ObjMapBlock {
	uint8_t row; // h
	uint8_t col; // l
	uint8_t parity; // b
};

// Func_cc0: the object map block of the position (big-endian y, x) at pos;
// also in wObjectSpawnBlockY / X and wUnused_ObjectParity
ObjMapBlock obj_map_block(GB *gb, uint16_t pos);
// Func_cf8: hPos = the centre of an object map block (by wccec, the SRAM
// bank of the row)
void obj_map_block_to_hpos(GB *gb, ObjMapBlock block);
// Func_d3e: hPos = the centre of the level block (wBlockPtrBank's page) at
// row h / column l, and its parity (in wUnused_ObjectParity)
uint8_t level_block_to_hpos(GB *gb, uint8_t h, uint8_t l);
// Func_d81: the level map column of an object map column and parity
uint8_t level_col(uint8_t col, uint8_t parity);
// Func_d8c: the object map column (and parity) of a level map column;
// wccec = wBlockPtrBank
ObjMapBlock obj_map_col(GB *gb, uint8_t row, uint8_t l);

} // namespace wl3::home
