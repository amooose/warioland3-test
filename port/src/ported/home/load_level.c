// Port of src/home/load_level.asm
#include "ported.h"

#define ADDR_RLO_OnlyHighBit (ADDR_ReloadLevelObjects + 0x38) // .DecompressLevelBlockMap_OnlyHighBit
#define ADDR_RLO_Decompress (ADDR_ReloadLevelObjects + 0x71)  // .Decompress

PORT_JUMP(InvalidLevelData)
{
	JUMP(Init);
}

// ld d, $00 / ld a, [wLevel] / add a / ld e, a / rl d / ld hl, LevelBlockAndObjectMaps /
// add hl, de / ld a, [wROMBank] / push af / ld a, BANK(LevelBlockAndObjectMaps) /
// bankswitch / ld a, [hli] / ld h, [hl] / ld l, a / ld a, h / inc a
// Returns whether the pointer is valid (z clear).
static bool level_maps_pointer(GB *gb)
{
	gb->cpu.d = 0;
	gb->cpu.a = rd8(gb, wLevel);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = alu_rl(gb, gb->cpu.d);
	uint16_t hl = alu_add16(gb, ADDR_LevelBlockAndObjectMaps, REG_DE(gb));
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_LevelBlockAndObjectMaps;
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = alu_inc(gb, gb->cpu.h);
	return !(gb->cpu.f & FLAG_Z);
}

// ld a, [hli] / ld [wCompressedLevelBlockMapPtr + 1], a / ld a, [hli] /
// ld [wCompressedLevelBlockMapPtr + 0], a / ld a, [hl] / ld [wTempBank], a
static void read_map_header(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCompressedLevelBlockMapPtr + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCompressedLevelBlockMapPtr + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wTempBank, gb->cpu.a);
	SET_HL(gb, hl);
}

// loads data related to the level block and object map
// and stores them in SRAM
PORT_JUMP(LoadLevelBlockMapAndObjects)
{
	if (!level_maps_pointer(gb)) {
		JUMP(InvalidLevelData); // null
		return;
	}
	read_map_header(gb);
	gb_push16(gb, REG_HL(gb));
	CALL(DecompressLevelBlockMap);
	SET_HL(gb, gb_pop16(gb));

	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCompressedLevelBlockMapPtr + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCompressedLevelBlockMapPtr + 0, gb->cpu.a);
	SET_HL(gb, hl);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	gb_push16(gb, REG_HL(gb));
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0x3c);
	if (gb->cpu.f & FLAG_Z)
		CALL(DecompressLevelObjectMap);
	SET_HL(gb, gb_pop16(gb));
	RET();
}

PORT_JUMP(ReloadLevelObjects)
{
	if (!level_maps_pointer(gb)) {
		JUMP(InvalidLevelData); // null
		return;
	}
	read_map_header(gb);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	gb_call(gb, ADDR_RLO_OnlyHighBit);
	RET();
}

// almost identical to DecompressLevelBlockMap
// but instead only copies over the highest bit
PORT_LOCAL(ReloadLevelObjects, DecompressLevelBlockMap_OnlyHighBit)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0x3c);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCompressedLevelBlockMapPtr + 0);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCompressedLevelBlockMapPtr + 1);
	gb->cpu.e = gb->cpu.a;
	gb_call(gb, ADDR_RLO_Decompress);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

// ld l, LOW(STARTOF(SRAM)) / inc h / ld a, h / cp $c0 / jr nz, ... /
// ld h, HIGH(STARTOF(SRAM)) / ld a, [wSRAMBank] / inc a / sramswitch
static void next_row(GB *gb)
{
	gb->cpu.l = 0x00;
	gb->cpu.h = alu_inc(gb, gb->cpu.h);
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, 0xc0); // check if already outside SRAM
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.h = 0xa0;
	gb->cpu.a = alu_inc(gb, rd8(gb, wSRAMBank));
	sramswitch(gb, gb->cpu.a);
}

// ld a, [wRepeatByte] or [de] & $80 / or [hl] / ld [hli], a / ld a, l / cp c
static bool or_into_map(GB *gb, uint8_t v)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_or(gb, v, rd8(gb, hl));
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = gb->cpu.l;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	return gb->cpu.f & FLAG_Z;
}

PORT_LOCAL(ReloadLevelObjects, Decompress)
{
	gb->cpu.c = LEVEL_WIDTH;
	SET_HL(gb, sLevelBlockObjectMap);
	for (;;) {
		// .loop_data
		gb->cpu.a = rd8(gb, REG_DE(gb));
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return; // done
		if (!alu_bit(gb, gb->cpu.a, 7)) {
			gb->cpu.b = gb->cpu.a; // num rows
			SET_DE(gb, REG_DE(gb) + 1);
			gb->cpu.a = alu_and(gb, rd8(gb, REG_DE(gb)), 0x80);
			wr8(gb, wRepeatByte, gb->cpu.a);
			SET_DE(gb, REG_DE(gb) + 1);
			do {
				// .asm_f61
				if (or_into_map(gb, rd8(gb, wRepeatByte)))
					next_row(gb); // .next_row_1
				// .next_repeat
				gb->cpu.b = alu_dec(gb, gb->cpu.b);
			} while (!(gb->cpu.f & FLAG_Z));
		} else {
			// .asm_f85
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x7f);
			gb->cpu.b = gb->cpu.a;
			SET_DE(gb, REG_DE(gb) + 1);
			do {
				// .loop_copy
				gb->cpu.a = alu_and(gb, rd8(gb, REG_DE(gb)), 0x80);
				uint16_t hl = REG_HL(gb);
				gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, hl));
				wr8(gb, hl++, gb->cpu.a);
				SET_HL(gb, hl);
				SET_DE(gb, REG_DE(gb) + 1);
				gb->cpu.a = gb->cpu.l;
				alu_cp(gb, gb->cpu.a, gb->cpu.c);
				if (gb->cpu.f & FLAG_Z)
					next_row(gb); // .asm_f98
				// .asm_f93
				gb->cpu.b = alu_dec(gb, gb->cpu.b);
			} while (!(gb->cpu.f & FLAG_Z));
		}
	}
}
