// Port of src/engine/level/switch.asm
#include "ported.h"

// Adds (off) or subtracts (on) 8 to every switchable block in one SRAM bank
// of the level map, from $a000 to end_h << 8.
static void switch_blocks(GB *gb, uint8_t bank, uint8_t end_h, bool on)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	sramswitch(gb, bank);
	uint8_t lo = on ? 0x68 : 0x60;
	uint16_t hl = 0xa000;
	for (;;) {
		gb->cpu.a = alu_and(gb, rd8(gb, hl++), 0x7f);
		alu_cp(gb, gb->cpu.a, lo);
		if (!(gb->cpu.f & FLAG_C)) {
			alu_cp(gb, gb->cpu.a, (uint8_t)(lo + 8));
			if (gb->cpu.f & FLAG_C) {
				hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
				gb->cpu.a = on ? alu_sub(gb, rd8(gb, hl), 0x08)
					: alu_add(gb, rd8(gb, hl), 0x08);
				wr8(gb, hl++, gb->cpu.a);
			}
		}
		gb->cpu.a = (uint8_t)hl;
		alu_cp(gb, gb->cpu.a, LEVEL_WIDTH);
		if (gb->cpu.f & FLAG_C)
			continue;
		hl = (uint16_t)(((hl >> 8) + 1) << 8); // ld l, $00 / inc h
		gb->cpu.a = (uint8_t)(hl >> 8);
		alu_cp(gb, gb->cpu.a, end_h);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	SET_HL(gb, hl);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

// If wSwitchStateUpdated is set, turns the switchable blocks on or off.
PORT(UpdateSwitchableBlocks)
{
	gb->cpu.a = rd8(gb, wSwitchStateUpdated);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, wSwitchStateUpdated, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSwitchState);
	alu_cp(gb, gb->cpu.a, 0x68);
	bool on = gb->cpu.f & FLAG_Z;
	switch_blocks(gb, BANK_sLevelBlockObjectMap, 0xc0, on); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	switch_blocks(gb, BANK_s2a000, 0xb0, on);
}
