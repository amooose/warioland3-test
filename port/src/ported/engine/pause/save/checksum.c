// Port of src/engine/pause/save/checksum.asm
#include "ported.h"

// de += [hl++]
static void add_byte(GB *gb, uint16_t *hl)
{
	gb->cpu.a = alu_add(gb, rd8(gb, (*hl)++), gb->cpu.e);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
	gb->cpu.d = gb->cpu.a;
}

// sums b bytes starting at hl
// result is added in de
PORT(CalculateChecksum)
{
	uint16_t hl = REG_HL(gb);
	do {
		add_byte(gb, &hl);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

// same as CalculateChecksum but for bc bytes; buggy, after the first byte
// it jumps into CalculateChecksum
PORT_JUMP(CalculateChecksumLong)
{
	uint16_t hl = REG_HL(gb);
	add_byte(gb, &hl);
	SET_HL(gb, hl);
	SET_BC(gb, REG_BC(gb) - 1);
	gb->cpu.a = alu_or(gb, gb->cpu.b, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP(CalculateChecksum);
		return;
	}
	RET();
}
