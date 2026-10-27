// Port of src/engine/level/spawn_edge.asm
#include "ported.h"

// Spawns the object of the block at hl, if it has one that isn't spawned
static void spawn_block_obj(GB *gb)
{
	if (!alu_bit(gb, rd8(gb, REG_HL(gb)), 7))
		return;
	gb_push16(gb, REG_HL(gb));
	CALL(Func_d8c);
	gb->cpu.a = rd8(gb, REG_HL(gb));
	if (alu_bit(gb, gb->cpu.b, 0))
		gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.c = gb->cpu.a;
	SET_HL(gb, gb_pop16(gb));
	if (gb->cpu.f & FLAG_Z)
		return;
	gb_push16(gb, REG_DE(gb));
	gb_push16(gb, REG_BC(gb));
	CALL(Func_d3e);
	gb->cpu.d = gb->cpu.h;
	gb->cpu.e = gb->cpu.l;
	gb_push16(gb, REG_HL(gb));
	// push_wram BANK("Level Objects WRAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wObjects;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(SpawnObject);
	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
	SET_HL(gb, gb_pop16(gb));
	SET_BC(gb, gb_pop16(gb));
	SET_DE(gb, gb_pop16(gb));
}

// The level hasn't ended: hl = the block at wBlockPos, with its SRAM bank
static bool edge_start(GB *gb)
{
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return false; // level ended
	SET_HL(gb, wBlockPos);
	CALL(GetBlockPtr);
	gb->cpu.a = rd8(gb, wBlockPtrBank);
	sramswitch(gb, gb->cpu.a);
	return true;
}

PORT(SpawnRowEdgeObjects)
{
	if (!edge_start(gb))
		return;
	gb->cpu.e = 0x0c;
	do {
		spawn_block_obj(gb);
		gb->cpu.h = alu_inc(gb, gb->cpu.h);
		gb->cpu.a = gb->cpu.h;
		alu_cp(gb, gb->cpu.a, 0xc0);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.h = (uint8_t)(sLevelBlockObjectMap >> 8);
			gb->cpu.a = alu_inc(gb, rd8(gb, wBlockPtrBank));
			wr8(gb, wBlockPtrBank, gb->cpu.a);
			sramswitch(gb, gb->cpu.a);
		}
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
	} while (gb->cpu.e);
}

PORT(SpawnColEdgeObjects)
{
	if (!edge_start(gb))
		return;
	gb->cpu.e = 0x0d;
	do {
		spawn_block_obj(gb);
		gb->cpu.a = alu_xor(gb, gb->cpu.b, 0x1);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
	} while (gb->cpu.e);
}
