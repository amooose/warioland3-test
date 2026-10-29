// Port of src/engine/credits/credits.asm
#include "ported.h"

PORT(LoadCreditsPals)
{
	SET_HL(gb, ADDR_Pals_1604cc);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_16050c);
	CALL(LoadPalsToTempPals2);
}

PORT(Func_1603be)
{
	SET_HL(gb, ADDR_Pals_16054c);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_16058c);
	CALL(LoadPalsToTempPals2);
}

PORT(Func_1603cb)
{
	SET_HL(gb, ADDR_Pals_1605cc);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_16060c);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadCreditsGfx)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_PrologueGfx);
	SET_BC(gb, v0Tiles0);
	gb->cpu.a = BANK_PrologueGfx;
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);

	SET_HL(gb, ADDR_MusicBoxGfx);
	SET_DE(gb, v1Tiles1);
	SET_BC(gb, 0x30 * 16);
	FARCALL_BANK(BANK_MusicBoxGfx, CopyHLToDE_BC);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_CreditsFontGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
	SET_HL(gb, ADDR_CreditsMusicBoxGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

PORT(Func_16041f)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_CreditsAttrmap);
	SET_BC(gb, v1BGMap1);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_CreditsTilemap);
	SET_BC(gb, v0BGMap1);
	CALL(Decompress);
}

PORT(VBlank_160439)
{
	SET_HL(gb, ADDR_VBlank_160439 + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x6c; // .end - .Func
	CALL(CopyHLToDE);
}

PORT(LoadCreditsSprite)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, gb->cpu.a = rd8(gb, hl++));
	wr8(gb, wCurSpriteAttributes, gb->cpu.a = rd8(gb, hl));
	SET_HL(gb, ADDR_OAM_1617e8);
	CALL(LoadSprite);
}
