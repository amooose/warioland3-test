// Home's object map positions (src/home/object_pos.asm): plain functions
// (object_pos.hpp), and the hooks that wrap them for register-level callers.
#include "object_pos.hpp"

namespace wl3::home {

namespace {

constexpr uint8_t SRAM_HI = 0xa0; // HIGH(STARTOF(SRAM))

// a 16-bit big-endian value in two bytes
void set_be(Memory mem, uint16_t addr, unsigned v)
{
	mem[addr] = static_cast<uint8_t>(v >> 8);
	mem[addr + 1] = static_cast<uint8_t>(v);
}

} // namespace

ObjMapBlock obj_map_block(GB *gb, uint16_t pos)
{
	Memory mem(gb);
	// the y position's block, a page per 16 rows
	uint8_t row = static_cast<uint8_t>(((mem[pos] & 0x0f) << 4) + (mem[static_cast<uint16_t>(pos + 1)] >> 4) + SRAM_HI);
	mem[wObjectSpawnBlockY] = row;
	// the x position's block, two to a byte
	uint8_t x = static_cast<uint8_t>(((mem[static_cast<uint16_t>(pos + 2)] & 0x0f) << 4) + (mem[static_cast<uint16_t>(pos + 3)] >> 4));
	uint8_t parity = (x & 1) ^ 1;
	mem[wUnused_ObjectParity] = parity;
	uint8_t col = static_cast<uint8_t>((x >> 1) + 0xb0);
	mem[wObjectSpawnBlockX] = col;
	return {row, col, parity};
}

void obj_map_block_to_hpos(GB *gb, ObjMapBlock block)
{
	Memory mem(gb);
	uint8_t y = static_cast<uint8_t>(block.row - SRAM_HI);
	set_be(mem, hYPosHi, static_cast<uint8_t>((mem[wccec] - 1) * 2 + (y >> 4)) * 0x100u + ((y & 0x0f) << 4) + 0x10);
	uint8_t x = static_cast<uint8_t>((block.col - 0xb0) * 2);
	set_be(mem, hXPosHi, (x >> 4) * 0x100u + ((x & 0x0f) << 4) + static_cast<uint8_t>((block.parity ^ 1) * 16 + 8));
}

uint8_t level_block_to_hpos(GB *gb, uint8_t h, uint8_t l)
{
	Memory mem(gb);
	uint8_t y = static_cast<uint8_t>(h - SRAM_HI);
	set_be(mem, hYPosHi, static_cast<uint8_t>((mem[wBlockPtrBank] - 1) * 2 + (y >> 4)) * 0x100u + ((y & 0x0f) << 4) + 0x10);
	set_be(mem, hXPosHi, (l >> 4) * 0x100u + ((l & 0x0f) << 4) + 0x08);
	uint8_t parity = (l & 1) ^ 1;
	mem[wUnused_ObjectParity] = parity;
	return parity;
}

uint8_t level_col(uint8_t col, uint8_t parity)
{
	return static_cast<uint8_t>((col - 0xb0) * 2 + (parity ^ 1));
}

ObjMapBlock obj_map_col(GB *gb, uint8_t row, uint8_t l)
{
	Memory mem(gb);
	mem[wccec] = mem[wBlockPtrBank];
	return {row, static_cast<uint8_t>((l >> 1) + 0xb0), static_cast<uint8_t>((l & 1) ^ 1)};
}

} // namespace wl3::home

using namespace wl3::home;

// The hooks leave the registers the asm leaves, but for a and f (live.py:
// no caller reads them)

// hl = the position (hPos): h = the row, l = the column, b = the parity;
// c = the row, d = the x position's high byte
PORT_EX(Func_cc0, 0, R_A | R_F)
{
	uint16_t pos = REG_HL(gb);
	ObjMapBlock block = obj_map_block(gb, pos);
	gb->cpu.d = wl3::Memory(gb)[static_cast<uint16_t>(pos + 2)];
	gb->cpu.c = block.row;
	gb->cpu.h = block.row;
	gb->cpu.l = block.col;
	gb->cpu.b = block.parity;
}
// h, l, b = the block (kept); d = the x offset in the block
PORT_EX(Func_cf8, 0, R_A | R_F)
{
	ObjMapBlock block{gb->cpu.h, gb->cpu.l, gb->cpu.b};
	obj_map_block_to_hpos(gb, block);
	gb->cpu.d = static_cast<uint8_t>((block.parity ^ 1) * 16 + 8);
}
// h, l = the level block: d = h - $a0, e = l, b = the parity
PORT_EX(Func_d3e, 0, R_A | R_F)
{
	uint8_t h = gb->cpu.h, l = gb->cpu.l;
	gb->cpu.b = level_block_to_hpos(gb, h, l);
	gb->cpu.d = static_cast<uint8_t>(h - 0xa0);
	gb->cpu.e = l;
}
PORT_EX(Func_d81, 0, R_A | R_F) { gb->cpu.l = level_col(gb->cpu.l, gb->cpu.b); }
PORT_EX(Func_d8c, 0, R_A | R_F)
{
	ObjMapBlock block = obj_map_col(gb, gb->cpu.h, gb->cpu.l);
	gb->cpu.l = block.col;
	gb->cpu.b = block.parity;
}
