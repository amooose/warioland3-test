// Port of src/engine/print_counts.asm
#include "ported.h"

// hl = BG map address
PORT(PrintNumberCoins)
{
	SET_BC(gb, (uint16_t)-TILEMAP_WIDTH);
	SET_DE(gb, TILEMAP_WIDTH);
	uint16_t hl = REG_HL(gb);
	for (int i = 0; i < 3; i++) {
		uint8_t a;
		if (i == 0) {
			a = alu_and(gb, rd8(gb, wNumCoins), 0x0f); // hundreds
		} else if (i == 1) {
			a = alu_swap(gb, rd8(gb, wNumCoins + 1));
			a = alu_and(gb, a, 0x0f); // tens
		} else {
			a = alu_and(gb, rd8(gb, wNumCoins + 1), 0x0f); // ones
		}
		a = alu_add(gb, a, a);
		a = alu_add(gb, a, 0xa0);
		wr8(gb, hl, a);
		hl = alu_add16(gb, hl, TILEMAP_WIDTH);
		a = alu_inc(gb, a);
		wr8(gb, hl++, a);
		hl = alu_add16(gb, hl, (uint16_t)-TILEMAP_WIDTH);
		gb->cpu.a = a;
	}
	SET_HL(gb, hl);
}

// sets flags in wKeyAndTreasureFlags corresponding to the collected
// treasures in wOWLevel
PORT(GetOWLevelCollectedTreasures)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wKeyAndTreasureFlags, 0);
	// push_wram BANK(wOWLevel)
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wOWLevel;
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = alu_dec(gb, rd8(gb, wOWLevel));

	// .SetFlags
	gb_push16(gb, ADDR_GetOWLevelCollectedTreasures + 0x12); // call .SetFlags
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *4
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_LevelTreasureIDs + NUM_LEVEL_TREASURES, REG_DE(gb));
	static const uint8_t bits[4] = {GREY_TREASURE_F, RED_TREASURE_F, GREEN_TREASURE_F, BLUE_TREASURE_F};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		gb_push16(gb, hl);
		CALL(IsTreasureCollected);
		if (!(gb->cpu.f & FLAG_Z)) {
			SET_HL(gb, wKeyAndTreasureFlags);
			wr8(gb, wKeyAndTreasureFlags, rd8(gb, wKeyAndTreasureFlags) | (1 << bits[i]));
		}
		hl = gb_pop16(gb);
	}
	SET_HL(gb, hl);
	gb_pop16(gb); // ret

	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
}
