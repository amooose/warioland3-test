// Port of src/engine/load_font.asm
#include "ported.h"

PORT(LoadFontTiles)
{
	SET_HL(gb, ADDR_FontGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

PORT(LoadFontPals)
{
	SET_HL(gb, ADDR_Pals_1f3610);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1f3610);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadLanguageSelectionText)
{
	gb->cpu.a = BANK_v1BGMap0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_LanguageSelection2TextMap);
	SET_BC(gb, v1BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_LanguageSelection1TextMap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}

PORT(LoadGBIncompatibleScreenGfx)
{
	SET_HL(gb, ADDR_GBIncompatibleGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

PORT(LoadGBIncompatibleScreenMap)
{
	SET_HL(gb, ADDR_GBIncompatibleTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0xe4; // %11100100
	wr8(gb, rBGP, gb->cpu.a);
}
