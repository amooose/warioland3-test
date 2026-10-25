// Port of src/home/object_pos.asm
#include "ported.h"

#define SRAM_HI 0xa0 // HIGH(STARTOF(SRAM))

// and $0f / swap a
static uint8_t lo_nybble_up(GB *gb, uint8_t v)
{
	return alu_swap(gb, alu_and(gb, v, 0x0f));
}

// swap a / and $0f
static uint8_t hi_nybble_down(GB *gb, uint8_t v)
{
	return alu_and(gb, alu_swap(gb, v), 0x0f);
}

// and $f0 / swap a
static uint8_t and_f0_swap(GB *gb, uint8_t v)
{
	return alu_swap(gb, alu_and(gb, v, 0xf0));
}

// input:
// - hl = hPos
// output:
// - l = x block
// - h = y block
// - b = 1 if even object, 0 if odd object
PORT(Func_cc0)
{
	uint16_t hl = REG_HL(gb);
	// [hl] = y pos
	gb->cpu.d = rd8(gb, hl++);
	gb->cpu.c = hi_nybble_down(gb, rd8(gb, hl++));
	gb->cpu.a = lo_nybble_up(gb, gb->cpu.d);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	gb->cpu.a = alu_add(gb, gb->cpu.a, SRAM_HI);
	gb->cpu.c = gb->cpu.a;
	wr8(gb, wObjectSpawnBlockY, gb->cpu.a);

	// [hl] = x pos
	gb->cpu.d = rd8(gb, hl++);
	uint8_t l = hi_nybble_down(gb, rd8(gb, hl));
	gb->cpu.a = lo_nybble_up(gb, gb->cpu.d);
	gb->cpu.a = alu_add(gb, gb->cpu.a, l);
	l = alu_srl(gb, gb->cpu.a);
	// ld a, 0 / adc 0 / xor $1: a is 1 if x was even, 0 if odd
	gb->cpu.a = alu_adc(gb, 0, 0);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, 1);
	wr8(gb, wUnused_ObjectParity, gb->cpu.a);
	gb->cpu.b = gb->cpu.a; // object parity

	gb->cpu.a = alu_add(gb, l, 0xb0);
	gb->cpu.l = gb->cpu.a;
	wr8(gb, wObjectSpawnBlockX, gb->cpu.a);
	gb->cpu.h = gb->cpu.c;
}

// ldh a, [hXXXPosHi] / adc 0 / ldh [hXXXPosHi], a
static void adc_hi(GB *gb, uint16_t addr)
{
	gb->cpu.a = alu_adc(gb, rd8(gb, addr), 0);
	wr8(gb, addr, gb->cpu.a);
}

PORT(Func_cf8)
{
	gb_push16(gb, REG_HL(gb));
	gb->cpu.a = alu_sub(gb, gb->cpu.h, SRAM_HI);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.d = and_f0_swap(gb, gb->cpu.a);
	gb->cpu.a = alu_dec(gb, rd8(gb, wccec));
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.d);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = lo_nybble_up(gb, gb->cpu.h);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x10);
	wr8(gb, hYPosLo, gb->cpu.a);
	adc_hi(gb, hYPosHi);

	gb->cpu.a = alu_xor(gb, gb->cpu.b, 0x01);
	for (int i = 0; i < 4; i++)
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *16
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x08);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, gb->cpu.l, 0xb0);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = and_f0_swap(gb, gb->cpu.a);
	wr8(gb, hXPosHi, gb->cpu.a);
	gb->cpu.a = lo_nybble_up(gb, gb->cpu.l);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.d);
	wr8(gb, hXPosLo, gb->cpu.a);
	adc_hi(gb, hXPosHi);
	SET_HL(gb, gb_pop16(gb));
}

PORT(Func_d3e)
{
	gb->cpu.a = alu_sub(gb, gb->cpu.h, SRAM_HI);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.b = and_f0_swap(gb, gb->cpu.a);
	gb->cpu.a = alu_dec(gb, rd8(gb, wBlockPtrBank));
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = lo_nybble_up(gb, gb->cpu.d);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x10);
	wr8(gb, hYPosLo, gb->cpu.a);
	adc_hi(gb, hYPosHi);
	gb->cpu.a = alu_sub(gb, gb->cpu.l, 0x00);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = and_f0_swap(gb, gb->cpu.a);
	wr8(gb, hXPosHi, gb->cpu.a);
	gb->cpu.a = lo_nybble_up(gb, gb->cpu.e);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x08);
	wr8(gb, hXPosLo, gb->cpu.a);
	adc_hi(gb, hXPosHi);
	gb->cpu.a = alu_and(gb, gb->cpu.l, 1);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, 1);
	gb->cpu.b = gb->cpu.a;
	wr8(gb, wUnused_ObjectParity, gb->cpu.a);
}

PORT(Func_d81)
{
	gb->cpu.a = alu_sub(gb, gb->cpu.l, 0xb0);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = alu_xor(gb, gb->cpu.b, 0x1);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.l);
	gb->cpu.l = gb->cpu.a;
}

PORT(Func_d8c)
{
	gb->cpu.a = rd8(gb, wBlockPtrBank);
	wr8(gb, wccec, gb->cpu.a);
	gb->cpu.b = 0x01;
	gb->cpu.a = alu_srl(gb, gb->cpu.l);
	if (gb->cpu.f & FLAG_C)
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xb0);
	gb->cpu.l = gb->cpu.a;
}
