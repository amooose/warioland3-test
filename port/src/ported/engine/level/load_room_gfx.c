// Port of src/engine/level/load_room_gfx.asm
#include "ported.h"

// hl = table + 2 * [index], as the asm does it (add a / rl d / add hl, de)
static uint16_t table_entry(GB *gb, uint16_t table, uint16_t index)
{
	gb->cpu.d = 0x00;
	gb->cpu.a = rd8(gb, index);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *2
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = alu_rl(gb, gb->cpu.d);
	return alu_add16(gb, table, REG_DE(gb));
}

PORT(InitRoomAnimatedTiles)
{
	uint16_t hl = table_entry(gb, ADDR_AnimatedTilesGroups, wAnimatedTilesGroup);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wAnimatedTilesFrameDuration, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wAnimatedTilesGfx, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrame, 0);
	wr8(gb, wAnimatedTilesFrameCount, 0);
}

PORT(InitRoomAnimatedPals)
{
	uint16_t hl = table_entry(gb, ADDR_RoomPalCycles, wRoomPalCycle);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wRoomPalCyclePtr + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl--);
	wr8(gb, wRoomPalCyclePtr + 0, gb->cpu.a);
	gb->cpu.l = rd8(gb, hl);
	gb->cpu.h = gb->cpu.a;
	SET_DE(gb, ROOM_PALCYCLE_LENGTH);
	hl = alu_add16(gb, REG_HL(gb), ROOM_PALCYCLE_LENGTH);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wRoomPalCycleDuration, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a); // /2
	wr8(gb, wRoomPalCycleCounter, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wRoomPalCycleIndex, 0);
}

#define LRG(off) ((uint16_t)(ADDR_LoadRoomGfx + (off)))

// loads a room's function table, tilemap, main and special tiles and its
// palettes
PORT_JUMP(LoadRoomGfx)
{
	uint16_t hl = table_entry(gb, ADDR_RoomBlockAndGfxData, wRoom);
	gb->cpu.a = rd8(gb, hl);
	hl++;
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, 0xff); // debug_assert_not $ff
	if (gb->cpu.f & FLAG_Z) {
		JUMP(Init);
		return;
	}
	hl = REG_HL(gb);
	static const uint16_t dst[] = {wRoomBlockFunctionTable, wRoomTileMap, wRoomMainTiles,
		wRoomSpecialTiles};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, dst[i], gb->cpu.a);
	}
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wRoomPalettes, gb->cpu.a);
	SET_HL(gb, hl);

	// divide wRoomBlockFunctionTable by 6
	gb->cpu.b = 0;
	gb->cpu.a = rd8(gb, wRoomBlockFunctionTable);
	for (;;) {
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 6);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
	}
	gb->cpu.a = alu_add(gb, 0x38, gb->cpu.b); // BANK("Level Data 1")
	wr8(gb, wRoomBlockFunctionTableBank, gb->cpu.a);
	gb_call_ret(gb, ADDR_Func_298d, LRG(0x41));
	gb_call_ret(gb, ADDR_LoadBlockFunctionTable, LRG(0x44));
	gb->cpu.a = rd8(gb, wRoomBlockFunctionTableBank);
	wr8(gb, wc0d0, gb->cpu.a);
	gb_call_ret(gb, ADDR_LoadRoomTileMap, LRG(0x4d));

	gb->cpu.a = rd8(gb, wRoomMainTiles);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a); // /8
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x51); // BANK("Room Main Tiles 1")
	wr8(gb, wRoomMainTilesBank, gb->cpu.a);
	gb_call_ret(gb, ADDR_LoadRoomMainTiles, LRG(0x5e));

	gb->cpu.a = rd8(gb, wRoomSpecialTiles);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a); // /8
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x4e); // BANK("Room Special Tiles 1")
	wr8(gb, wRoomSpecialTilesBank, gb->cpu.a);
	gb_call_ret(gb, ADDR_LoadRoomSpecialTiles, LRG(0x6f));

	gb->cpu.a = 0x33; // BANK("Level Palettes")
	wr8(gb, wPaletteBank, gb->cpu.a);
	gb_call_ret(gb, ADDR_LoadRoomPalettes, LRG(0x77));
	RET();
}
