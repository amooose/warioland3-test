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

PORT(InitPerfectScreen)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);

	gb_call(gb, ADDR_LoadPerfectPals);
	gb_call(gb, ADDR_LoadPerfectGfx);
	gb_call(gb, ADDR_Func_dc13e);
	CALL(VBlank_Perfect);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);

	uint16_t hl = wMenuObj1;
	wr8(gb, hl++, 0x23);
	wr8(gb, hl++, 0x2b);
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(ADDR_Frameset_dd848 >> 8));
	gb->cpu.a = (uint8_t)ADDR_Frameset_dd848;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	CALL(UpdateObjAnim);

	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_Func_dc158);

	// play_music MUSIC_PERFECT
	wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_PERFECT >> 8));
	gb->cpu.a = (uint8_t)MUSIC_PERFECT;
	wr8(gb, hMusicID + 1, gb->cpu.a);

	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
	wr8(gb, wGameOverWarioVar, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT(UpdatePerfectScreen)
{
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_Func_dc158);
	CALL(ClearUnusedVirtualOAM);
	SET_HL(gb, wTimer);
	gb->cpu.a = rd8(gb, wTimer);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
		if (!(gb->cpu.f & FLAG_Z))
			goto check_input;
	}
	// .asm_dc08c
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x7);
	if (!(gb->cpu.f & FLAG_Z))
		goto check_input;
	CALL(SlowFadeInScreen);
	gb->cpu.a = rd8(gb, wSubState);
	alu_cp(gb, gb->cpu.a, 0x03);
	if (gb->cpu.f & FLAG_Z)
		goto check_input;
	gb->cpu.a = 0x03;
	wr8(gb, wSubState, gb->cpu.a);
	SET_HL(gb, wGameOverWarioVar);
	wr8(gb, wGameOverWarioVar, alu_inc(gb, rd8(gb, wGameOverWarioVar)));
	gb->cpu.a = rd8(gb, wGameOverWarioVar);
	int n = 0;
	do {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		n++;
	} while (n < 5 && !(gb->cpu.f & FLAG_Z));
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, wGameOverWarioVar, 0x00);
		gb_call(gb, ADDR_Func_dc101);
	} else if (n == 1) {
		// .asm_dc0bd
		gb->cpu.a = 240;
		wr8(gb, wTimer, gb->cpu.a);
		gb_call(gb, ADDR_Func_dc10d);
	} else if (n == 2 || n == 4) {
		gb_call(gb, ADDR_Func_dc119); // .asm_dc0c7
	} else if (n == 3) {
		gb_call(gb, ADDR_Func_dc125); // .asm_dc0cc
	} else {
		gb_call(gb, ADDR_Func_dc10d); // .asm_dc0c2
	}

check_input: // .asm_dc0cf
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F))
		return;
	gb->cpu.a = rd8(gb, wJoypadPressed);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wFadeSpeed, 0);
	wr8(gb, wPalFadeCounter, 0);
	wr8(gb, wUnused_c186, 0);
	SET_HL(gb, wGameModeFlags);
	wr8(gb, wGameModeFlags, rd8(gb, wGameModeFlags) | (1 << MODE_TIME_ATTACK_F));
	gb->cpu.a = ST_PAUSE_MENU;
	wr8(gb, wState, gb->cpu.a);
	gb->cpu.a = SST_PAUSE_28;
	wr8(gb, wSubState, gb->cpu.a);
}
