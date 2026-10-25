// Port of src/home/copy.asm
#include "ported.h"

// Shared loop of the BC-count copy/fill routines. In the asm, the count is
// handled as "c, then 256 per remaining b", with b pre-incremented when c is
// nonzero; a count of 0 means 65536. They all leave b = c = 0.
static uint32_t bc_count(GB *gb)
{
	uint16_t bc = REG_BC(gb);
	return bc ? bc : 0x10000;
}

// writes a to address in hl, b times
PORT(WriteAToHL_BTimes)
{
	uint16_t hl = REG_HL(gb);
	int n = gb->cpu.b ? gb->cpu.b : 256;
	for (int i = 0; i < n; i++)
		wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.b = 0;
	// flags from the final dec b
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

// write a to hl, bc times
PORT(WriteAToHL_BCTimes)
{
	uint16_t hl = REG_HL(gb);
	uint32_t n = bc_count(gb);
	for (uint32_t i = 0; i < n; i++)
		wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	SET_BC(gb, 0);
	// push af / pop af keeps the entry carry; the final dec b sets z and n
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

static void copy_bc(GB *gb)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	uint32_t n = bc_count(gb);
	for (uint32_t i = 0; i < n; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, de++, gb->cpu.a);
	}
	SET_HL(gb, hl);
	SET_DE(gb, de);
	SET_BC(gb, 0);
}

// copies bc bytes from hl to de
PORT(CopyHLToDE_BC)
{
	copy_bc(gb);
	// and a clears carry, the final dec b sets z and n
	gb->cpu.f = FLAG_Z | FLAG_N;
}

// switches bank to wTempBank
// then copies bc bytes from hl to de
PORT(FarCopyHLToDE_BC)
{
	uint8_t old_bank = rd8(gb, wROMBank);
	uint8_t entry_f = gb->cpu.f;
	bankswitch(gb, rd8(gb, wTempBank));
	copy_bc(gb);
	// pop af restores a and f from the push at entry
	gb->cpu.a = old_bank;
	gb->cpu.f = entry_f;
	bankswitch(gb, old_bank);
}

// copies b bytes from hl to de
PORT(CopyHLToDE)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	int n = gb->cpu.b ? gb->cpu.b : 256;
	for (int i = 0; i < n; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, de++, gb->cpu.a);
	}
	SET_HL(gb, hl);
	SET_DE(gb, de);
	gb->cpu.b = 0;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}
