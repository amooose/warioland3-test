// Port of src/home/bg_ob_pals.asm
// SetWarioPal, LoadCollectedTreasurePal_Level and CopyAndApplyOBPals wait
// for H-Blank and stay in asm.
#include "ported.h"

// Reads all 8 palettes from the palette data register through the index
// register into hl.
static void store_pals(GB *gb, uint16_t dst, uint16_t index_reg, uint16_t data_reg)
{
	gb->cpu.e = 0;
	for (int i = 0; i < 8 * PAL_SIZE; i++) {
		wr8(gb, index_reg, gb->cpu.e);
		gb->cpu.a = rd8(gb, data_reg);
		wr8(gb, dst++, gb->cpu.a);
		gb->cpu.e++;
	}
	SET_HL(gb, dst);
	gb->cpu.b = 0;
	gb->cpu.c = (uint8_t)data_reg;
	gb->cpu.f = FLAG_Z | FLAG_N; // xor a cleared carry; final dec b
}

// Writes all 8 palettes from hl to the palette data register, after selecting
// palette 0 with auto-increment.
static void apply_pals(GB *gb, uint16_t src, uint16_t index_reg, uint8_t index, uint16_t data_reg)
{
	gb->cpu.a = index;
	wr8(gb, index_reg, index);
	for (int i = 0; i < 8 * PAL_SIZE; i++) {
		gb->cpu.a = rd8(gb, src++);
		wr8(gb, data_reg, gb->cpu.a);
	}
	SET_HL(gb, src);
	gb->cpu.b = 0;
	gb->cpu.c = (uint8_t)data_reg;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N); // dec b
}

// copies all BG pals to wTempBGPals
PORT(StoreBGPals)
{
	store_pals(gb, wTempBGPals, rBGPI, rBGPD);
}

// copies all OB pals to wTempOBPals
PORT(StoreOBPals)
{
	store_pals(gb, wTempOBPals, rOBPI, rOBPD);
}

// fills BG palette with just white
PORT(FillWhiteBGPal)
{
	apply_pals(gb, ADDR_PalsWhite, rBGPI, BGPI_AUTOINC | 0, rBGPD);
}

// fills OB palette with just white
PORT(FillWhiteOBPal)
{
	apply_pals(gb, ADDR_PalsWhite, rOBPI, OBPI_AUTOINC | 0, rOBPD);
}

// unreferenced
PORT(Func_1ae2)
{
	SET_HL(gb, ADDR_PalsWhite);
	SET_DE(gb, wTempBGPals);
	gb->cpu.b = 8 * PAL_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, ADDR_PalsWhite);
	gb->cpu.b = 8 * PAL_SIZE;
	CALL(CopyHLToDE);
}

PORT(LoadCollectedTreasurePal_ClearScreen)
{
	// push_wram BANK(wLevelTreasurePals)
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wLevelTreasurePals;
	wr8(gb, rWBK, gb->cpu.a);

	gb->cpu.a = alu_and(gb, rd8(gb, wLevelEndScreen), 0x7f);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0;
	uint16_t hl = alu_add16(gb, wLevelTreasurePals, REG_BC(gb));
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), OBPAL_TREASURE_YELLOW);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *8
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = gb->cpu.b;
	SET_HL(gb, alu_add16(gb, ADDR_TreasurePals, REG_DE(gb)));

	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_TreasurePals;
	bankswitch(gb, gb->cpu.a);
	SET_DE(gb, wTempPals2 + 7 * PAL_SIZE);
	gb->cpu.b = PAL_SIZE;
	FARCALL_BANK(BANK_TreasurePals, CopyHLToDE_Short); // unneeded farcall
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);

	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

PORT(ApplyTempPals1ToBGPals)
{
	apply_pals(gb, wTempPals1, rBGPI, BGPI_AUTOINC | 0, rBGPD);
}

PORT(ApplyTempPals2ToOBPals)
{
	apply_pals(gb, wTempPals2, rOBPI, OBPI_AUTOINC | 0, rOBPD);
}
