// overworld/bank20 (src/engine/overworld/bank20.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "overworld.hpp"

namespace wl3::overworld {

// MapLevelCoords: a pointer per map side to its levels' coordinates, two
// words a level (GetCthWordFromAthTable with c = level * 2)
uint16_t map_level_coords(GB *gb, uint8_t level)
{
	Memory mem(gb);
	uint16_t side = mem.word(static_cast<uint16_t>(ADDR_MapLevelCoords + mem[wCurMapSide] * 2));
	return static_cast<uint16_t>(side + static_cast<uint8_t>(level * 2) * 2);
}

namespace {

class OverworldBank20 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// rst JumpTable, with the table at `table` (right after the rst): pushes the
	// return address, pops it into hl, a = a * 2, de = a, hl += de, hl = [hl].
	// Leaves every register as JumpTable does and returns the target (= hl).
	uint16_t jumptable(uint16_t table)
	{
		gb_push16(gb_, table); // rst JumpTable
		uint16_t hl = gb_pop16(gb_); // pop hl
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		hl = alu_.add16(hl, cpu_.e);
		cpu_.a = rd8(hl);
		cpu_.h = rd8((uint16_t)(hl + 1));
		cpu_.l = cpu_.a;
		return REG_HL(gb_);
	}

	// a = map side level index
	// a = value to compare with wOWFuncCounter
	// returns c = a value for wNextMapSide
	void port_Func_812b0()
	{
		uint16_t hl = ADDR_Func_812b0 + 0x0c; // .data
		cpu_.a = rd8(wNextMapSide);
		cpu_.b = 0x00;
		cpu_.c = cpu_.a;
		hl = alu_.add16(hl, REG_BC(gb_));
		cpu_.c = rd8(hl);
		SET_HL(gb_, hl);
	}


	// unreferenced
	void port_UnreferencedLCD_82729()
	{
		SET_HL(gb_, ADDR_UnreferencedLCD_82729 + 0xc); // .func
		SET_DE(gb_, wLCDFunc);
		cpu_.b = 0x1f; // .func_end - .func
		CALL(CopyHLToDE);
	}

	// unreferenced
	void port_UnreferencedLCD_82754()
	{
		SET_HL(gb_, ADDR_UnreferencedLCD_82754 + 0xc); // .func
		SET_DE(gb_, wLCDFunc);
		cpu_.b = 0x01; // .func_end - .func
		CALL(CopyHLToDE);
	}

	// unreferenced
	void port_Func_817b1()
	{
		cpu_.a = rd8(wGameModeFlags);
		if (alu_.bit(cpu_.a, MODE_GAME_CLEARED_F)) {
			RET();
			return;
		}
		cpu_.a = EVENT_PROLOGUE;
		for (;;) {
			wr8(w2dffc, cpu_.a);
			gb_call(gb_, ADDR_CheckIfEventIsUnlocked);
			cpu_.a = rd8(w2dffc);
			if (cpu_.f & FLAG_Z)
				break;
			cpu_.a = alu_.inc(cpu_.a);
			cpu_.b = EVENT_BLUE_CRAYON;
			alu_.cp(cpu_.a, cpu_.b);
			if (cpu_.f & FLAG_C)
				continue;
			cpu_.a = cpu_.b;
			break;
		}
		cpu_.a = alu_.dec(cpu_.a);
		gb_call(gb_, ADDR_LoadEventTreasures);
		cpu_.a = rd8(w2dfff);
		wr8(wStoredTransitionParam, cpu_.a);
		JUMP_TO(ADDR_SetCutsceneButtonSelectable_GotTransitionParam);
	}

	// unreferenced
	// unreferenced; Func_814ec / Func_8150c (path_preview.cpp) leave registers
	// as ALL_REGISTERS, so this does too
	void port_Func_8143d()
	{
		SET_HL(gb_, w2d88e);
		cpu_.a = rd8(w2d100);
		wr8(w2d88e, cpu_.a);
		cpu_.a = rd8(w2d101);
		wr8(w2d88f, cpu_.a);
		cpu_.a = rd8(w2d069);
		wr8(wOWYCoord1, cpu_.a);
		wr8(w2d100, cpu_.a);
		cpu_.a = rd8(w2d06a);
		wr8(wOWXCoord1, cpu_.a);
		wr8(w2d101, cpu_.a);
		SET_HL(gb_, w2d88f);
		gb_call_ret(gb_, ADDR_Func_8143d + 0x32, ADDR_Func_8143d + 0x20); // call .Func_8146f
		SET_HL(gb_, w2d88f);
		cpu_.a = rd8(w2d88e);
		wr8(wOWYCoord1, cpu_.a);
		wr8(w2d100, cpu_.a);
		cpu_.a = rd8(w2d88f);
		wr8(wOWXCoord1, cpu_.a);
		wr8(w2d101, cpu_.a);
	}

	// Func_8143d.Func_8146f: ld a, [w2d0e3] / jumptable
	void port_Func_8143d__Func_8146f()
	{
		cpu_.a = rd8(w2d0e3);
		JUMP_TO(jumptable(ADDR_Func_8143d + 0x32 + 4));
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank20;

PORT_EX(Func_812b0, 0, 0) { OverworldBank20(gb).port_Func_812b0(); }
PORT_EX(UnreferencedLCD_82729, 0, 0) { OverworldBank20(gb).port_UnreferencedLCD_82729(); }
PORT_EX(UnreferencedLCD_82754, 0, 0) { OverworldBank20(gb).port_UnreferencedLCD_82754(); }
PORT_EX(Func_817b1, HOOK_MANUAL_RET, 0) { OverworldBank20(gb).port_Func_817b1(); }
PORT_EX(Func_8143d, 0, ALL_REGISTERS) { OverworldBank20(gb).port_Func_8143d(); }
PORT_LOCAL_EX(Func_8143d, Func_8146f, HOOK_MANUAL_RET, 0) { OverworldBank20(gb).port_Func_8143d__Func_8146f(); }
