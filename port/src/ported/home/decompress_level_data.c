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

// e reached the end of a row: go to the start of the next one, and to the
// next SRAM bank after its end
static void next_row(GB *gb, uint8_t row_start)
{
	gb->cpu.e = row_start;
	gb->cpu.d = alu_inc(gb, gb->cpu.d);
	gb->cpu.a = gb->cpu.d;
	alu_cp(gb, gb->cpu.a, 0xc0);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.d = 0xa0; // HIGH(STARTOF(SRAM))
		gb->cpu.a = alu_inc(gb, rd8(gb, wSRAMBank));
		sramswitch(gb, gb->cpu.a);
	}
}

// writes a to [de] and advances de, wrapping rows at c
static void put_byte(GB *gb, bool objmap)
{
	wr8(gb, REG_DE(gb), gb->cpu.a);
	if (!objmap)
		SET_DE(gb, REG_DE(gb) + 1);
	gb->cpu.a = gb->cpu.e;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z)
		next_row(gb, objmap ? 0xb0 : 0x00);
	else if (objmap)
		SET_DE(gb, REG_DE(gb) + 1);
}

// .Decompress: RLE data at hl into the level map in SRAM
static void decompress_map(GB *gb, bool objmap)
{
	uint16_t hl = REG_HL(gb);
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!gb->cpu.a)
			break;
		if (alu_bit(gb, gb->cpu.a, 7)) {
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x7f);
			gb->cpu.b = gb->cpu.a;
			do {
				gb->cpu.a = rd8(gb, hl++);
				SET_HL(gb, hl);
				put_byte(gb, objmap);
				hl = REG_HL(gb);
				gb->cpu.b = alu_dec(gb, gb->cpu.b);
			} while (gb->cpu.b);
		} else {
			gb->cpu.b = gb->cpu.a;
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, wRepeatByte, gb->cpu.a);
			do {
				gb->cpu.a = rd8(gb, wRepeatByte);
				SET_HL(gb, hl);
				put_byte(gb, objmap);
				hl = REG_HL(gb);
				gb->cpu.b = alu_dec(gb, gb->cpu.b);
			} while (gb->cpu.b);
		}
	}
	SET_HL(gb, hl);
}

static void decompress_level_map(GB *gb, bool objmap)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	gb->cpu.h = rd8(gb, wCompressedLevelBlockMapPtr + 0);
	gb->cpu.a = rd8(gb, wCompressedLevelBlockMapPtr + 1);
	gb->cpu.l = gb->cpu.a;
	if (objmap) {
		gb->cpu.c = (LEVEL_WIDTH >> 1) + 0xb0 - 1;
		SET_DE(gb, sLevelBlockObjectMap + 0xb0);
	} else {
		gb->cpu.c = LEVEL_WIDTH;
		SET_DE(gb, sLevelBlockObjectMap);
	}
	decompress_map(gb, objmap);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

PORT(DecompressLevelBlockMap)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0x3c);
	if (gb->cpu.a)
		return;
	decompress_level_map(gb, false);
}

PORT(DecompressLevelObjectMap)
{
	decompress_level_map(gb, true);
}

// hl = block pointer in SRAM
// output: wBGPtr = its address in v0BGMap0
PORT(GetBlockPtrBGMapAddress)
{
	gb->cpu.a = alu_sub(gb, gb->cpu.h, 0xa0); // HIGH(STARTOF(SRAM))
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	for (int i = 0; i < 6; i++) {
		gb->cpu.e = alu_sla(gb, gb->cpu.e);
		gb->cpu.d = alu_rl(gb, gb->cpu.d);
	}
	gb->cpu.a = alu_and(gb, gb->cpu.d, 0x03);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.l, 0x0f);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.h = (uint8_t)(v0BGMap0 >> 8);
	uint16_t hl = alu_add16(gb, REG_HL(gb), REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = gb->cpu.h;
	wr8(gb, wBGPtr + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, wBGPtr + 1, gb->cpu.a);
}
