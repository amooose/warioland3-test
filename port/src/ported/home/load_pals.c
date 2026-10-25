// Port of src/home/load_pals.asm
#include "ported.h"

// Writes b bytes from hl to the palette data register c, after selecting
// palette 0 with auto-increment through the index register.
static void load_pals_from_wram(GB *gb, uint16_t src, uint16_t index_reg, uint8_t index, uint16_t data_reg)
{
	wr8(gb, index_reg, index);
	gb->cpu.a = index;
	for (int i = 0; i < 8 * PAL_SIZE; i++) {
		gb->cpu.a = rd8(gb, src++);
		wr8(gb, data_reg, gb->cpu.a);
	}
	SET_HL(gb, src);
	gb->cpu.b = 0;
	gb->cpu.c = (uint8_t)data_reg;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N); // dec b
}

PORT(LoadBGPalettesFromWRAM)
{
	load_pals_from_wram(gb, wTempBGPals, rBGPI, BGPI_AUTOINC | 0, rBGPD);
}

PORT(LoadOBPalettesFromWRAM)
{
	load_pals_from_wram(gb, wTempOBPals, rOBPI, OBPI_AUTOINC | 0, rOBPD);
}

// Copies b bytes from hl to de, incrementing only e (the destination never
// crosses a page boundary).
static void copy_short(GB *gb, uint16_t de, int count)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.d = (uint8_t)(de >> 8);
	gb->cpu.e = (uint8_t)de;
	for (int i = 0; i < count; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(gb->cpu.d << 8 | gb->cpu.e), gb->cpu.a);
		gb->cpu.e++;
	}
	SET_HL(gb, hl);
	gb->cpu.b = 0;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N); // dec b
}

// copies hl to wTempPals1
PORT(LoadPalsToTempPals1)
{
	copy_short(gb, wTempPals1, 8 * PAL_SIZE);
}

// copies hl to wTempPals2
PORT(LoadPalsToTempPals2)
{
	copy_short(gb, wTempPals2, 8 * PAL_SIZE);
}
