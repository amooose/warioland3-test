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
