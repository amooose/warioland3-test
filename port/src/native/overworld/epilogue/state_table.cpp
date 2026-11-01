// overworld/epilogue/state_table (src/engine/overworld/epilogue/state_table.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldEpilogueStateTable : public AsmModule {
public:
	using AsmModule::AsmModule;

	// rst JumpTable, with the table at `table` (right after the rst): leaves every
	// register as JumpTable does and returns the target (= hl).
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

	void port_EpilogueStateTable()
	{
		cpu_.a = rd8(wSubState);
		JUMP_TO(jumptable(ADDR_EpilogueStateTable + 4));
	}

	void port_EpilogueStateTable__FadeToWhite()
	{
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b11);
		if (!(cpu_.f & FLAG_Z))
			return;
		CALL(FastFadeToWhite);
	}

	void port_EpilogueStateTable__Func_473c()
	{
		FARCALL(Func_4628);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldEpilogueStateTable;

PORT_EX(EpilogueStateTable, HOOK_MANUAL_RET, 0) { OverworldEpilogueStateTable(gb).port_EpilogueStateTable(); }
PORT_LOCAL_EX(EpilogueStateTable, FadeToWhite, 0, 0) { OverworldEpilogueStateTable(gb).port_EpilogueStateTable__FadeToWhite(); }
PORT_LOCAL_EX(EpilogueStateTable, Func_473c, 0, 0) { OverworldEpilogueStateTable(gb).port_EpilogueStateTable__Func_473c(); }
