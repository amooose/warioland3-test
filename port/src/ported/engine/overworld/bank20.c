// Port of src/engine/overworld/bank20.asm
#include "ported.h"

// rst JumpTable, with the table at `table` (right after the rst): pushes the
// return address, pops it into hl, a = a * 2, de = a, hl += de, hl = [hl].
// Leaves every register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// a = map side level index
PORT(GetMapLevelCoords)
{
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *2
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCurMapSide);
	SET_HL(gb, ADDR_MapLevelCoords);
	CALL(GetCthWordFromAthTable);
}

// a = value to compare with wOWFuncCounter
// returns c = a value for wNextMapSide
PORT(Func_812b0)
{
	uint16_t hl = ADDR_Func_812b0 + 0x0c; // .data
	gb->cpu.a = rd8(gb, wNextMapSide);
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	hl = alu_add16(gb, hl, REG_BC(gb));
	gb->cpu.c = rd8(gb, hl);
	SET_HL(gb, hl);
}


// unreferenced
PORT(UnreferencedLCD_82729)
{
	SET_HL(gb, ADDR_UnreferencedLCD_82729 + 0xc); // .func
	SET_DE(gb, wLCDFunc);
	gb->cpu.b = 0x1f; // .func_end - .func
	CALL(CopyHLToDE);
}

// unreferenced
PORT(UnreferencedLCD_82754)
{
	SET_HL(gb, ADDR_UnreferencedLCD_82754 + 0xc); // .func
	SET_DE(gb, wLCDFunc);
	gb->cpu.b = 0x01; // .func_end - .func
	CALL(CopyHLToDE);
}

// unreferenced
PORT_JUMP(Func_817b1)
{
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (alu_bit(gb, gb->cpu.a, MODE_GAME_CLEARED_F)) {
		RET();
		return;
	}
	gb->cpu.a = EVENT_PROLOGUE;
	for (;;) {
		wr8(gb, w2dffc, gb->cpu.a);
		gb_call(gb, ADDR_CheckIfEventIsUnlocked);
		gb->cpu.a = rd8(gb, w2dffc);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		gb->cpu.b = EVENT_BLUE_CRAYON;
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_C)
			continue;
		gb->cpu.a = gb->cpu.b;
		break;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb_call(gb, ADDR_LoadEventTreasures);
	gb->cpu.a = rd8(gb, w2dfff);
	wr8(gb, wStoredTransitionParam, gb->cpu.a);
	JUMP_TO(ADDR_SetCutsceneButtonSelectable_GotTransitionParam);
}

// unreferenced
PORT(Func_8143d)
{
	SET_HL(gb, w2d88e);
	gb->cpu.a = rd8(gb, w2d100);
	wr8(gb, w2d88e, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d101);
	wr8(gb, w2d88f, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d069);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d06a);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
	SET_HL(gb, w2d88f);
	gb_call_ret(gb, ADDR_Func_8143d + 0x32, ADDR_Func_8143d + 0x20); // call .Func_8146f
	SET_HL(gb, w2d88f);
	gb->cpu.a = rd8(gb, w2d88e);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d88f);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
}

// Func_8143d.Func_8146f: ld a, [w2d0e3] / jumptable
PORT_LOCAL_JUMP(Func_8143d, Func_8146f)
{
	gb->cpu.a = rd8(gb, w2d0e3);
	JUMP_TO(jumptable(gb, ADDR_Func_8143d + 0x32 + 4));
}

