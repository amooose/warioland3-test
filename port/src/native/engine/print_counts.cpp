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

	// hl = BG map address
	void port_PrintNumberCoins()
	{
		SET_BC(gb_, (uint16_t)-TILEMAP_WIDTH);
		SET_DE(gb_, TILEMAP_WIDTH);
		uint16_t hl = REG_HL(gb_);
		for (int i = 0; i < 3; i++) {
			uint8_t a;
			if (i == 0) {
				a = alu_.and_(rd8(wNumCoins), 0x0f); // hundreds
			} else if (i == 1) {
				a = alu_.swap(rd8(wNumCoins + 1));
				a = alu_.and_(a, 0x0f); // tens
			} else {
				a = alu_.and_(rd8(wNumCoins + 1), 0x0f); // ones
			}
			a = alu_.add(a, a);
			a = alu_.add(a, 0xa0);
			wr8(hl, a);
			hl = alu_.add16(hl, TILEMAP_WIDTH);
			a = alu_.inc(a);
			wr8(hl++, a);
			hl = alu_.add16(hl, (uint16_t)-TILEMAP_WIDTH);
			cpu_.a = a;
		}
		SET_HL(gb_, hl);
	}

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

	// ld [hl], a / add hl, de / inc a / ld [hli], a / add hl, bc
	uint16_t print_digit(uint16_t hl, uint8_t digit)
	{
		cpu_.a = alu_.add(digit, digit); // *2
		cpu_.a = alu_.add(cpu_.a, 0xa0);
		wr8(hl, cpu_.a);
		hl = alu_.add16(hl, REG_DE(gb_));
		cpu_.a = alu_.inc(cpu_.a);
		wr8(hl++, cpu_.a);
		return alu_.add16(hl, REG_BC(gb_));
	}

	void port_PrintNumCollectedTreasures()
	{
		SET_BC(gb_, (uint16_t)-TILEMAP_WIDTH);
		SET_DE(gb_, TILEMAP_WIDTH);
		uint16_t hl = REG_HL(gb_);

		// hundreds
		hl = print_digit(hl, alu_.and_(rd8(wNumCollectedTreasures + 0), 0x0f));
		// tens
		cpu_.a = alu_.swap(rd8(wNumCollectedTreasures + 1));
		hl = print_digit(hl, alu_.and_(cpu_.a, 0x0f));
		// ones
		hl = print_digit(hl, alu_.and_(rd8(wNumCollectedTreasures + 1), 0x0f));
		SET_HL(gb_, hl);
	}
};

} // namespace

} // namespace wl3::engine

using wl3::engine::EnginePrintCounts;

PORT_EX(PrintNumberCoins, 0, 0) { EnginePrintCounts(gb).port_PrintNumberCoins(); }
PORT_EX(GetOWLevelCollectedTreasures, 0, 0) { EnginePrintCounts(gb).port_GetOWLevelCollectedTreasures(); }
PORT_EX(PrintNumCollectedTreasures, 0, 0) { EnginePrintCounts(gb).port_PrintNumCollectedTreasures(); }
