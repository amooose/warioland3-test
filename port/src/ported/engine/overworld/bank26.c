// Port of src/engine/overworld/bank26.asm
#include "ported.h"

// a = digit (low nibble): copies its 2 tiles from NumbersGfx to de
static void draw_number(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	uint16_t hl = ADDR_NumbersGfx;
	SET_BC(gb, 0x20);
	for (;;) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (!gb->cpu.a)
			break;
		hl = alu_add16(gb, hl, 0x20);
	}
	SET_HL(gb, hl);
	gb->cpu.b = 0x20;
	CALL(CopyHLToDE);
}

PORT(DrawCoinCount)
{
	SET_DE(gb, v0Tiles1 + 0xa * TILE_SIZE);
	gb->cpu.a = rd8(gb, wNumCoins + 0);
	draw_number(gb);
	gb->cpu.a = alu_swap(gb, rd8(gb, wNumCoins + 1));
	draw_number(gb);
	gb->cpu.a = rd8(gb, wNumCoins + 1);
	draw_number(gb);
}

PORT(ClearTreasureData)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTileBuffer);
	SET_BC(gb, 16 * TILE_SIZE);
	CALL(WriteAToHL_BCTimes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wLevelTreasureIDs);
	SET_BC(gb, 0x10);
	CALL(WriteAToHL_BCTimes);
}

PORT(LoadLevelTreasures)
{
	SET_DE(gb, wLevelTreasureIDs);
	gb->cpu.a = rd8(gb, wOWLevel);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0;
	uint16_t hl = alu_add16(gb, ADDR_LevelTreasureIDs, gb->cpu.c);
	gb->cpu.b = NUM_LEVEL_TREASURES;
	do {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e++; // inc e
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

PORT(LoadTreasurePals)
{
	static const uint16_t ids[] = {wLevelGreyTreasureID, wLevelRedTreasureID,
		wLevelGreenTreasureID, wLevelBlueTreasureID};
	static const uint16_t pals[] = {wLevelGreyTreasurePal, wLevelRedTreasurePal,
		wLevelGreenTreasurePal, wLevelBlueTreasurePal};
	SET_DE(gb, ADDR_TreasureOBPals);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, ids[i]);
		uint16_t hl = alu_add16(gb, gb->cpu.a, ADDR_TreasureOBPals);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, pals[i], gb->cpu.a);
	}
}

// a = treasure ID: copies its 4 tiles from TreasureGfx to de
static void copy_treasure_gfx(GB *gb)
{
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.b = 0;
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.b = alu_rl(gb, gb->cpu.b);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xf0);
	gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.b);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, 0x0f, gb->cpu.c);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	SET_HL(gb, alu_add16(gb, ADDR_TreasureGfx, REG_BC(gb)));
	gb->cpu.b = 4 * TILE_SIZE;
	CALL(CopyHLToDE);
}

PORT(LoadTreasureTiles)
{
	gb->cpu.a = rd8(gb, wLevelGreyTreasureID);
	SET_DE(gb, wTileBuffer);
	copy_treasure_gfx(gb);
	gb->cpu.a = rd8(gb, wLevelRedTreasureID);
	copy_treasure_gfx(gb);
	gb->cpu.a = rd8(gb, wLevelGreenTreasureID);
	copy_treasure_gfx(gb);
	gb->cpu.a = rd8(gb, wLevelBlueTreasureID);
	copy_treasure_gfx(gb);
	copy_treasure_gfx(gb); // falls through into .CopyGfx once more
}

PORT(FillBottomBarTreasureIDs)
{
	SET_DE(gb, wBottomBarTreasureIDs);
	SET_HL(gb, wLevelTreasureIDs);
	gb->cpu.b = NUM_LEVEL_TREASURES;
	CALL(CopyHLToDE);
}

// c = treasure ID
// returns b = level
PORT(GetLevelThatContainsTreasure)
{
	gb->cpu.b = 0xff;
	uint16_t hl = ADDR_LevelTreasureIDs;
	do {
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
}

PORT(Func_9aaf4)
{
	gb->cpu.a = rd8(gb, wOWLevel);
	CALL(CheckLevelMusicalCoinFlag);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wBottomBarFlags);
	wr8(gb, wBottomBarFlags, rd8(gb, wBottomBarFlags) | (1 << 6));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCompassObjState, 0);
	wr8(gb, wCompassObjState + 1, 0);
	SET_HL(gb, wCompassObjState + 1);
}

PORT(WriteUncollectedBottomBarTreasureIDs)
{
	gb->cpu.a = alu_swap(gb, rd8(gb, wKeyAndTreasureFlags));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.c = NUM_LEVEL_TREASURES;
	SET_DE(gb, wLevelTreasureIDs);
	do {
		gb->cpu.b = alu_srl(gb, gb->cpu.b);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, REG_DE(gb), 0);
		}
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
}
