// overworld/unreferenced_treasure_collection (src/engine/overworld/unreferenced_treasure_collection.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldUnreferencedTreasureCollection : public AsmModule {
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

	// unreferenced
	void port_UnreferencedOpenTreasureCollection()
	{
		cpu_.a = rd8(wSubState);
		JUMP_TO(jumptable(ADDR_UnreferencedOpenTreasureCollection + 0x4));
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldUnreferencedTreasureCollection;

PORT_EX(UnreferencedOpenTreasureCollection, HOOK_MANUAL_RET, 0) { OverworldUnreferencedTreasureCollection(gb).port_UnreferencedOpenTreasureCollection(); }
