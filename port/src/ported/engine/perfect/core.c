// Port of src/engine/perfect/core.asm
#include "ported.h"

PORT(LoadPerfectPals)
{
	SET_HL(gb, ADDR_Pals_dc173);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_dc1fb);
	CALL(LoadPalsToTempPals2);
}

static void copy_3_pals_to_tempals1_5(GB *gb, uint16_t src)
{
	SET_HL(gb, src);
	SET_DE(gb, wTempPals1 + 5 * 8);
	gb->cpu.b = 3 * 8;
	CALL(CopyHLToDE_Short);
}

PORT(Func_dc101) { copy_3_pals_to_tempals1_5(gb, ADDR_Pals_dc173 + 5 * 8); }
PORT(Func_dc10d) { copy_3_pals_to_tempals1_5(gb, ADDR_Pals_dc1b3); }
PORT(Func_dc119) { copy_3_pals_to_tempals1_5(gb, ADDR_Pals_dc1cb); }
PORT(Func_dc125) { copy_3_pals_to_tempals1_5(gb, ADDR_Pals_dc1e3); }

PORT(LoadPerfectGfx)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_PerfectGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

PORT(Func_dc13e)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_PerfectAttrmap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_PerfectTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}

PORT(Func_dc158)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, gb->cpu.a = rd8(gb, hl++));
	wr8(gb, wCurSpriteAttributes, gb->cpu.a = rd8(gb, hl));
	SET_HL(gb, ADDR_OAM_dd710);
	CALL(LoadSprite);
}
