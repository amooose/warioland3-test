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

#define BANK_BLOCK_FUNCTION_TABLES_2 0x50 // BANK("Block Function Tables 2")

PORT_JUMP(LoadBlockFunctionTable)
{
	gb->cpu.a = rd8(gb, wRoomBlockFunctionTable);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_BlockFunctionTablePointers, REG_DE(gb));
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_BlockFunctionTablePointers;
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(Init); // .reset
		return;
	}
	gb->cpu.a = rd8(gb, wRoomBlockFunctionTable);
	alu_cp(gb, gb->cpu.a, NUM_BLOCK_FUNCTION_TABLES_GROUP_1 - 1);
	bool group_2 = !(gb->cpu.f & FLAG_C);
	if (group_2) {
		gb->cpu.a = rd8(gb, wROMBank);
		push_af(gb);
		gb->cpu.a = BANK_BLOCK_FUNCTION_TABLES_2;
		bankswitch(gb, gb->cpu.a);
	}
	SET_DE(gb, wBlockFunctionTable);
	gb->cpu.b = 0;
	CALL(CopyHLToDE);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	if (group_2) {
		pop_af(gb);
		bankswitch(gb, gb->cpu.a);
	}
	RET();
}

// ld a, [hl] / swap a / and $0f (or without the swap)
static uint8_t nibble(GB *gb, uint16_t hl, bool high)
{
	gb->cpu.a = rd8(gb, hl);
	if (high)
		gb->cpu.a = alu_swap(gb, gb->cpu.a);
	return gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
}

// ld a, [limit] / add a (shift times) / add base, then $ff if that made 0
static void room_limit(GB *gb, uint16_t limit, int shift, uint8_t base, bool nonzero, uint16_t dest)
{
	gb->cpu.a = rd8(gb, limit);
	for (int i = 0; i < shift; i++)
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, base);
	if (nonzero && (gb->cpu.f & FLAG_Z))
		gb->cpu.a = 0xff;
	wr8(gb, dest, gb->cpu.a);
}

PORT_JUMP(LoadRoom)
{
	gb->cpu.a = 0x70;
	wr8(gb, wPrevWarioScreenYPos, gb->cpu.a);
	gb->cpu.a = 0x58;
	wr8(gb, wCamRightSpacing, gb->cpu.a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x58);
	gb->cpu.a = alu_cpl(gb, gb->cpu.a);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x58);
	wr8(gb, wCamLeftSpacing, gb->cpu.a);

	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_LevelHeaders;
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wLevel);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	gb->cpu.d = alu_rl(gb, gb->cpu.d);
	uint16_t hl = alu_add16(gb, ADDR_LevelHeaders, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(Init); // debug_assert_not $ff
		return;
	}
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);

	gb->cpu.a = rd8(gb, wLevel);
	alu_cp(gb, gb->cpu.a, THE_GRASSLANDS_NIGHT_1);
	uint8_t bank = (gb->cpu.f & FLAG_C) ? 0x30 : 0x31;
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = bank;
	bankswitch(gb, gb->cpu.a);

	// .got_bank
	gb->cpu.a = rd8(gb, wLevelRoomID);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, REG_HL(gb), REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.e;
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(Init); // debug_assert_not $ff
		return;
	}
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wWarioSpawnPos, gb->cpu.a);
	wr8(gb, wRoomUpperYLimit, nibble(gb, hl, true));
	wr8(gb, wRoomLowerYLimit, nibble(gb, hl++, false));
	wr8(gb, wRoomLeftXLimit, nibble(gb, hl, true));
	wr8(gb, wRoomRightXLimit, nibble(gb, hl++, false));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCameraConfigFlags, gb->cpu.a);

	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wObjectGroup, gb->cpu.a);
	SET_HL(gb, hl);
	gb_push16(gb, REG_HL(gb));
	gb->cpu.a = rd8(gb, rWBK); // push_wram
	push_af(gb);
	gb->cpu.a = 0x01; // BANK("Level Objects WRAM")
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(LoadObjectGroupData);
	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
	SET_HL(gb, gb_pop16(gb));

	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wAnimatedTilesGroup, gb->cpu.a);
	SET_HL(gb, hl);
	gb_push16(gb, REG_HL(gb));
	FARCALL(InitRoomAnimatedTiles);
	SET_HL(gb, gb_pop16(gb));

	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wRoomPalCycle, gb->cpu.a);
	SET_HL(gb, hl);
	gb_push16(gb, REG_HL(gb));
	FARCALL(InitRoomAnimatedPals);
	SET_HL(gb, gb_pop16(gb));

	gb->cpu.a = rd8(gb, REG_HL(gb));
	wr8(gb, wRoom, gb->cpu.a);
	FARCALL(LoadRoomGfx);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);

	room_limit(gb, wRoomLowerYLimit, 4, 0xa0, true, wc0c4);
	room_limit(gb, wRoomUpperYLimit, 4, 0xa0, false, wc0c5);
	room_limit(gb, wRoomRightXLimit, 3, 0xb0, true, wc0c7);
	room_limit(gb, wRoomLeftXLimit, 3, 0xb0, false, wc0c6);
	RET();
}
