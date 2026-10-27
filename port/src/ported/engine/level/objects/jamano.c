// Port of src/engine/level/objects/jamano.asm
#include "ported.h"

// Writes the OAM pointer for the hammer range's frame (flipped by bit 7 of
// [hl]) big-endian to [e] and [e - 1] of hl's page.
PORT(Func_554bc)
{
	static const uint8_t lows[4][2] = {
		{0x37, 0xbc}, // 1
		{0x2e, 0xb3}, // 2
		{0x25, 0xaa}, // 3
		{0x1c, 0xa1}, // 4
	};
	gb->cpu.a = rd8(gb, wDollBoyHammerRange);
	int range;
	for (range = 1; range <= 4; range++) {
		alu_cp(gb, gb->cpu.a, (uint8_t)range);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	if (range > 4)
		return; // ret nz
	bool flip = alu_bit(gb, rd8(gb, REG_HL(gb)), 7);
	gb->cpu.b = 0x5c;
	gb->cpu.c = lows[range - 1][flip];
	// .asm_5550c
	gb->cpu.l = gb->cpu.e;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
}
