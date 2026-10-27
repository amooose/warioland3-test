// Port of src/engine/pause/save/validate_save.asm
#include "ported.h"

// hl = 4 check values
// returns carry set if they don't match
static void check_vals(GB *gb, const uint8_t vals[4])
{
	uint16_t hl = REG_HL(gb);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, vals[i]);
		if (!(gb->cpu.f & FLAG_Z)) {
			SET_HL(gb, hl);
			gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
			return;
		}
	}
	SET_HL(gb, hl);
	gb->cpu.f &= FLAG_Z; // scf / ccf
}

PORT(CheckSaveVals1)
{
	static const uint8_t vals[4] = {CHECKVAL1_1, CHECKVAL1_2, CHECKVAL1_3, CHECKVAL1_4};
	check_vals(gb, vals);
}

PORT(CheckSaveVals2)
{
	static const uint8_t vals[4] = {CHECKVAL2_1, CHECKVAL2_2, CHECKVAL2_3, CHECKVAL2_4};
	check_vals(gb, vals);
}

PORT(CalculateGameDataChecksum)
{
	SET_DE(gb, 0);
	SET_HL(gb, 0xca00); // STARTOF("Progress WRAM")
	gb->cpu.b = 0x5b;   // SIZEOF("Progress WRAM")
	gb_call(gb, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
	// push_wram BANK("WRAM2")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wTreasuresCollected;
	wr8(gb, rWBK, gb->cpu.a);
	SET_HL(gb, wTreasuresCollected);
	gb->cpu.b = (uint8_t)((wOWLevel - wTreasuresCollected) + 2);
	gb_call(gb, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
