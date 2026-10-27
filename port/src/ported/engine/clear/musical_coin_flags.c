// Port of src/engine/clear/musical_coin_flags.asm
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

// .ApplyBGPal: [de] = MusicalCoinLevelBGPals[hffa0]
static void apply_bg_pal(GB *gb)
{
	gb->cpu.a = rd8(gb, hffa0);
	uint16_t hl = alu_add16(gb, ADDR_MusicalCoinLevelBGPals, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, REG_DE(gb), gb->cpu.a);
}

PORT(Func_d47f1)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	uint16_t hl = wMusicalCoinFlags;
	gb->cpu.b = NUM_LEVELS / 8;
	gb->cpu.c = 8;
	SET_DE(gb, v0BGMap0 + 9 * TILEMAP_WIDTH + 8);
	gb->cpu.a = 0xff;
	wr8(gb, hffa0, 0xff);
	do {
		do {
			gb->cpu.a = alu_inc(gb, rd8(gb, hffa0));
			wr8(gb, hffa0, gb->cpu.a);
			wr8(gb, hl, alu_rrc(gb, rd8(gb, hl)));
			if (gb->cpu.f & FLAG_C)
				apply_bg_pal(gb);
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
			gb->cpu.a = alu_and(gb, gb->cpu.e, 0x1f);
			alu_cp(gb, gb->cpu.a, 13);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH - 5);
				gb->cpu.e = gb->cpu.a;
			}
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
		} while (gb->cpu.c);
		hl++; // next byte in wMusicalCoinFlags
		gb->cpu.c = 8;
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), 0))
		apply_bg_pal(gb);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
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

PORT(SetMusicalCoinFlag)
{
	gb->cpu.a = alu_srl(gb, rd8(gb, wLevel));
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	wr8(gb, wAllMusicalCoinsLevel, gb->cpu.a); // LEVEL_* constant
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.e = gb->cpu.a; // /8
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, wMusicalCoinFlags, REG_DE(gb));
	SET_HL(gb, hl);
	rotate_to_bit(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), 0)) {
		// already set
		push_af(gb);
		gb->cpu.a = 0xff;
		wr8(gb, wAllMusicalCoinsLevel, 0xff);
		pop_af(gb);
	} else {
		wr8(gb, hl, rd8(gb, hl) | 1); // set 0, [hl]
	}
	rotate_back(gb, hl);
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
