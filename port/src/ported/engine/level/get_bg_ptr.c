// Port of src/engine/level/get_bg_ptr.asm
#include "ported.h"

// input:
// - wBlockXPos = x position
// - wBlockYPos = y position
// output:
// - hl and wBGPtr = pointer to BGMap
PORT(GetBlockBGPtr)
{
	// hl = y / TILE_SIZE * TILEMAP_WIDTH (the shifts' flags are all
	// overwritten below)
	uint16_t y = (uint16_t)(rd8(gb, wBlockYPos + 0) << 8 | rd8(gb, wBlockYPos + 1));
	y = (uint16_t)((y >> 3) << 5);
	gb->cpu.a = alu_and(gb, (uint8_t)(y >> 8), (TILEMAP_WIDTH * TILEMAP_HEIGHT - 1) >> 8);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.e = (uint8_t)y;

	uint16_t x = (uint16_t)(rd8(gb, wBlockXPos + 0) << 8 | rd8(gb, wBlockXPos + 1));
	x >>= 3;
	gb->cpu.a = alu_and(gb, (uint8_t)x, TILEMAP_WIDTH - 1);
	uint16_t hl = (uint16_t)((v0BGMap0 & 0xff00) | gb->cpu.a);
	hl = alu_add16(gb, hl, REG_DE(gb));
	SET_HL(gb, hl);
	wr8(gb, wBGPtr + 0, gb->cpu.h);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, wBGPtr + 1, gb->cpu.a);
}
