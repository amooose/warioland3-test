// Port of src/home/decompress_level_data.asm
#include "ported.h"

// Turns a 2-byte coordinate 0xXWYZ into the block index 0xWY.
static uint8_t block_coord(GB *gb, uint8_t hi, uint8_t lo)
{
	uint8_t low_nybble = alu_and(gb, (uint8_t)(lo >> 4 | lo << 4), 0x0f);
	uint8_t high = (uint8_t)(alu_and(gb, hi, 0x0f) << 4);
	return alu_add(gb, high, low_nybble);
}

// hl = 2-byte y coordinate followed by 2-byte x coordinate
// output: hl = wBlockPtr = pointer into the level's block map in SRAM,
// wBlockPtrBank = its SRAM bank
PORT(GetBlockPtr)
{
	uint16_t hl = REG_HL(gb);

	// y position
	gb->cpu.d = rd8(gb, hl++);
	uint8_t lo = rd8(gb, hl++);
	gb->cpu.b = alu_and(gb, (uint8_t)(lo >> 4 | lo << 4), 0x0f);
	uint8_t row = alu_add(gb, (uint8_t)(alu_and(gb, gb->cpu.d, 0x0f) << 4), gb->cpu.b);

	// The block map spans several SRAM banks of $20 pages each.
	gb->cpu.b = BANK_sLevelBlockObjectMap;
	uint8_t page = alu_add(gb, row, (uint8_t)(sLevelBlockObjectMap >> 8));
	alu_cp(gb, page, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
		page = alu_sub(gb, page, 0x20);
		alu_cp(gb, page, 0xc0);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.b = alu_inc(gb, gb->cpu.b);
			page = alu_sub(gb, page, 0x20);
		}
	}
	wr8(gb, wBlockPtr + 0, page);
	wr8(gb, wBlockPtrBank, gb->cpu.b);

	// x position
	gb->cpu.d = rd8(gb, hl++);
	lo = rd8(gb, hl);
	gb->cpu.a = block_coord(gb, gb->cpu.d, lo);
	wr8(gb, wBlockPtr + 1, gb->cpu.a);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, wBlockPtr + 0);
	gb->cpu.h = gb->cpu.a;
}
