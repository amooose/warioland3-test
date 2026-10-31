// Port of src/engine/clear/musical_coin_flags.asm: _CheckLevelMusicalCoinFlag,
// which home code calls with registers (the rest is C++, src/native/clear)
#include "ported.h"

// rrc r
static uint8_t alu_rrc(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | v << 7);
	gb->cpu.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
	return r;
}

// rlc r
static uint8_t alu_rlc(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v << 1 | v >> 7);
	gb->cpu.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
	return r;
}

// rotates the flag byte at hl right n times, then back
static void rotate_back(GB *gb, uint16_t hl)
{
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return; // no need to rotate back
	do {
		wr8(gb, hl, alu_rlc(gb, rd8(gb, hl)));
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
	} while (gb->cpu.a);
}

// hl = flag byte, a = c = bit: rrc [hl] c times
static void rotate_to_bit(GB *gb, uint16_t hl)
{
	gb->cpu.a = alu_and(gb, gb->cpu.c, 0x07);
	gb->cpu.c = gb->cpu.a;
	if (!gb->cpu.a)
		return;
	do {
		wr8(gb, hl, alu_rrc(gb, rd8(gb, hl)));
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
}

// check if a bit is set in wMusicalCoinFlags
// for the level in wOWLevel
// returns b = TRUE if set
PORT(_CheckLevelMusicalCoinFlag)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wOWLevel));
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a); // /8
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, wMusicalCoinFlags, REG_DE(gb));
	SET_HL(gb, hl);
	rotate_to_bit(gb, hl);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = alu_bit(gb, rd8(gb, hl), 0) ? TRUE : FALSE;
	gb->cpu.a = gb->cpu.c;
	rotate_back(gb, hl);
}
