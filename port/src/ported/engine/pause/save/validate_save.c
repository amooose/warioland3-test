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

// ld a, val / ld [s0a790], a / ld [s0a7e0], a / ld [s0afa0], a
static void save_marker(GB *gb, uint8_t val)
{
	gb->cpu.a = val;
	wr8(gb, s0a790, val);
	wr8(gb, s0a7e0, val);
	wr8(gb, s0afa0, val);
}

// ld a, [wSRAMBank] / push af / ld a, bank / sramswitch
static void push_sram(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = bank;
	sramswitch(gb, bank);
}

// pop af / sramswitch
static void pop_sram(GB *gb)
{
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

// copies SRAM 2 a000-afff to b000-bfff and SRAM 1 to SRAM 3, then stores
// the backup SRAM checksum with its echoes
PORT(Func_1f14c6)
{
	save_marker(gb, 0x70);
	push_sram(gb, 0x02);
	SET_HL(gb, s2a000);
	SET_DE(gb, s2b000);
	SET_BC(gb, 0x1000);
	CALL(CopyHLToDE_BC);
	pop_sram(gb);

	SET_HL(gb, s0a000);
	SET_BC(gb, 0x2000);
	do {
		push_sram(gb, 0x01);
		gb->cpu.d = rd8(gb, REG_HL(gb));
		push_sram(gb, 0x03);
		gb->cpu.a = gb->cpu.d;
		uint16_t hl = REG_HL(gb);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl + 1);
		pop_sram(gb);
		pop_sram(gb);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.c)
			continue;
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b || gb->cpu.c);

	save_marker(gb, 0x71);
	gb_call(gb, ADDR_CalculateBackupSRAMChecksum2);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0a79f + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0a79f + 1, gb->cpu.a);
	save_marker(gb, 0x72);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0a7ef + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0a7ef + 1, gb->cpu.a);
	save_marker(gb, 0x73);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0afaf + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0afaf + 1, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	save_marker(gb, 0);
}
