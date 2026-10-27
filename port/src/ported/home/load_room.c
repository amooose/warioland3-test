// Port of src/home/load_room.asm
#include "ported.h"

// ld a, [index] / add a / ld e, a / ld d, $00 / ld hl, table / add hl, de /
// ld a, [hli] / ld h, [hl] / ld l, a
static void get_pointer(GB *gb, uint16_t index, uint16_t table)
{
	gb->cpu.a = rd8(gb, index);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, table, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
}

// copies bc bytes from hl to de with the bank in bank_addr mapped
static void far_copy(GB *gb, uint16_t bank_addr, uint16_t de, uint16_t bc)
{
	gb->cpu.a = rd8(gb, bank_addr);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	SET_DE(gb, de);
	SET_BC(gb, bc);
	CALL(CopyHLToDE_BC);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

PORT(Func_298d)
{
	get_pointer(gb, wRoomBlockFunctionTable, ADDR_RoomBlockTilesPointers);
	far_copy(gb, wRoomBlockFunctionTableBank, wRoomBlockTiles, 4 * NUM_ROOM_BLOCK_TYPES);
}

PORT(LoadRoomTileMap)
{
	get_pointer(gb, wRoomTileMap, ADDR_RoomBlockAttributePointers);
	gb->cpu.a = rd8(gb, wc0d0);
	wr8(gb, wTempBank, gb->cpu.a);
	SET_BC(gb, wRoomBlockAttributes);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

PORT(LoadRoomMainTiles)
{
	get_pointer(gb, wRoomMainTiles, ADDR_LevelMainTilesPointers);
	far_copy(gb, wRoomMainTilesBank, v0Tiles2, 0x800);
}

PORT(LoadRoomSpecialTiles)
{
	gb->cpu.a = 1;
	wr8(gb, rVBK, 1);
	get_pointer(gb, wRoomSpecialTiles, ADDR_SpecialTilesPointers);
	far_copy(gb, wRoomSpecialTilesBank, v0Tiles2, 0x800);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(LoadRoomPalettes)
{
	get_pointer(gb, wRoomPalettes, ADDR_RoomPals);
	gb->cpu.a = rd8(gb, wPaletteBank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(LoadPalsToTempPals1);
}

static void wait_next_hblank(GB *gb)
{
	while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
		idle(gb, 32);
	while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
		idle(gb, 32);
}

PORT(UpdateRoomAnimatedPals)
{
	// only animate if at least 8 scanlines from V-Blank
	gb->cpu.a = rd8(gb, rLY);
	alu_cp(gb, gb->cpu.a, LY_VBLANK - 8);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = rd8(gb, wRoomPalCycleDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_inc(gb, rd8(gb, wRoomPalCycleCounter));
	wr8(gb, wRoomPalCycleCounter, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		return;

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wRoomPalCycleCounter, 0);
	gb->cpu.h = rd8(gb, wRoomPalCyclePtr + 0);
	gb->cpu.l = rd8(gb, wRoomPalCyclePtr + 1);

	gb->cpu.a = alu_inc(gb, rd8(gb, wRoomPalCycleIndex));
	alu_cp(gb, gb->cpu.a, ROOM_PALCYCLE_LENGTH);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, wRoomPalCycleIndex, gb->cpu.a);

	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, REG_HL(gb), REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_RoomPalCycles;
	bankswitch(gb, gb->cpu.a);
	gb->cpu.d = 0;
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *2
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = alu_rl(gb, gb->cpu.d);
	hl = alu_add16(gb, ADDR_RoomPals, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);

	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = 0x33; // BANK("Level Palettes")
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = BGPI_AUTOINC | 0;
	wr8(gb, rBGPI, gb->cpu.a);
	gb->cpu.b = 5;
	gb->cpu.c = (uint8_t)rBGPD;
	hl = REG_HL(gb);
	do {
		wait_next_hblank(gb);
		for (int i = 0; i < 12; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);

	wait_next_hblank(gb);
	for (int i = 0; i < 3; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
	}
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
	SET_HL(gb, hl);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}
