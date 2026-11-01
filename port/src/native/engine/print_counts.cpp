// engine/print_counts (src/engine/print_counts.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::engine {

namespace {

class EnginePrintCounts : public AsmModule {
public:
	using AsmModule::AsmModule;

	// sets flags in wKeyAndTreasureFlags corresponding to the collected
	// treasures in wOWLevel
	void port_GetOWLevelCollectedTreasures()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wKeyAndTreasureFlags, 0);
		// push_wram BANK(wOWLevel)
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = BANK_wOWLevel;
		wr8(rWBK, cpu_.a);
		cpu_.a = alu_.dec(rd8(wOWLevel));

		// .SetFlags
		gb_push16(gb_, ADDR_GetOWLevelCollectedTreasures + 0x12); // call .SetFlags
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *4
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(ADDR_LevelTreasureIDs + NUM_LEVEL_TREASURES, REG_DE(gb_));
		static const uint8_t bits[4] = {GREY_TREASURE_F, RED_TREASURE_F, GREEN_TREASURE_F, BLUE_TREASURE_F};
		for (int i = 0; i < 4; i++) {
			cpu_.a = rd8(hl++);
			gb_push16(gb_, hl);
			CALL(IsTreasureCollected);
			if (!(cpu_.f & FLAG_Z)) {
				SET_HL(gb_, wKeyAndTreasureFlags);
				wr8(wKeyAndTreasureFlags, rd8(wKeyAndTreasureFlags) | (1 << bits[i]));
			}
			hl = gb_pop16(gb_);
		}
		SET_HL(gb_, hl);
		gb_pop16(gb_); // ret

		pop_af(); // pop_wram
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::engine

using wl3::engine::EnginePrintCounts;

PORT_EX(GetOWLevelCollectedTreasures, 0, 0) { EnginePrintCounts(gb).port_GetOWLevelCollectedTreasures(); }
