// Port of src/engine/language_selection/init.asm
#include "ported.h"

PORT(InitLanguageSelection)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);

	FARCALL(LoadFontPals);
	FARCALL(LoadFontTiles);
	FARCALL(LoadLanguageSelectionText);
	gb_call(gb, ADDR_VBlank_LanguageSelection);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCY, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	gb->cpu.a = ENGLISH;
	wr8(gb, wLanguage, gb->cpu.a);

	wr8(gb, wMenuObj1 + 0, 70);
	wr8(gb, wMenuObj1 + 1, 24);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 2; i < 6; i++)
		wr8(gb, wMenuObj1 + i, 0);
	wr8(gb, wMenuObj1 + 6, ADDR_Frameset_76b5 >> 8);
	gb->cpu.a = ADDR_Frameset_76b5 & 0xff;
	wr8(gb, wMenuObj1 + 7, gb->cpu.a);
	SET_HL(gb, wMenuObj1 + 7);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_LoadLanguageSelectionSprite);
	CALL(ClearUnusedVirtualOAM);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);

	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
