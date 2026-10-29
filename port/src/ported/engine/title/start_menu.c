// Port of src/engine/title/start_menu.asm
#include "ported.h"

PORT(StartOverworldState)
{
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

PORT(StartOverworldStateAndNewGameParam)
{
	gb->cpu.a = TRANSITION_NEW_GAME;
	wr8(gb, wTransitionParam, TRANSITION_NEW_GAME);
	// fallthrough into StartOverworldState
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

PORT(Func_4628)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameDuration, 0);
	wr8(gb, wAnimatedTilesGfx, 0);
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	wr8(gb, w3d50d, 0);
	SET_HL(gb, wDayNight);
	wr8(gb, wDayNight, rd8(gb, wDayNight) | 0x80); // set 7, [hl]
	// jr StartOverworldState
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

static void load_start_menu_sprite(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadStartMenuSprite);
}

PORT_JUMP(StartMenu)
{
	SET_HL(gb, wPlaneWarioEnd - 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wPlaneWario);
	gb_call(gb, ADDR_LoadIntroSprite);
	load_start_menu_sprite(gb, wMenuObj3);
	gb_call(gb, ADDR_HandleStartMenuSelection);
	load_start_menu_sprite(gb, wMenuObj1);

	gb->cpu.a = alu_and(gb, rd8(gb, wceef), rd8(gb, wceef));
	if (gb->cpu.a) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
		if (!gb->cpu.a) {
			gb->cpu.a = alu_xor(gb, rd8(gb, wMenuObj2Attributes), 0x03);
			wr8(gb, wMenuObj2Attributes, gb->cpu.a);
		}
		load_start_menu_sprite(gb, wMenuObj2);
	}

	CALL(ClearUnusedVirtualOAM);
	SET_HL(gb, wStartMenuSelection);
	if (!alu_bit(gb, rd8(gb, wStartMenuSelection), STARTMENUF_SELECT_F)) {
		gb->cpu.a = rd8(gb, wStartMenuSelection);
		alu_cp(gb, gb->cpu.a, STARTMENU_NEW_GAME);
		bool tick = gb->cpu.f & FLAG_Z;
		if (!tick) {
			alu_cp(gb, gb->cpu.a, STARTMENU_CONTINUE);
			tick = gb->cpu.f & FLAG_Z;
		}
		if (!tick) {
			// start Action Help timer
			gb->cpu.a = (uint8_t)(ACTION_HELP_TIMER >> 8);
			wr8(gb, wTimer + 1, gb->cpu.a);
			gb->cpu.a = (uint8_t)ACTION_HELP_TIMER;
			wr8(gb, wTimer, gb->cpu.a);
			RET();
			return;
		}
		// .tick_action_help_timer
		gb->cpu.a = alu_sub(gb, rd8(gb, wTimer), 1);
		wr8(gb, wTimer, gb->cpu.a);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_sbc(gb, rd8(gb, wTimer + 1), 0);
		wr8(gb, wTimer + 1, gb->cpu.a);
		gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.a) {
			SET_HL(gb, wTimer + 1);
			RET(); // still not 0
			return;
		}
		// enter Action Help mode
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wTimer + 1, 0);
		wr8(gb, wTimer, 0);
		SET_HL(gb, wTimer);
		gb->cpu.a = alu_or(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_TITLE_SCREEN);
		wr8(gb, wPowerUpLevel, gb->cpu.a);
		JUMP(OpenActionHelp);
		return;
	}

	// .select
	gb->cpu.a = alu_and(gb, rd8(gb, wStartMenuSelection), 0x0f);
	uint16_t fs;
	alu_cp(gb, gb->cpu.a, STARTMENU_NEW_GAME);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = ST_LANGUAGE_SELECTION;
		wr8(gb, wState, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSubState, 0);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, STARTMENU_CONTINUE);
	if (gb->cpu.f & FLAG_Z) {
		// .ContinueOrTimeAttack
		gb->cpu.a = rd8(gb, wGameModeFlags);
		if (alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
			CALL(DisableLCD);
			CALL(FillWhiteBGPal);
			CALL(FillWhiteOBPal);
			gb->cpu.a = LCDC_DEFAULT;
			wr8(gb, rLCDC, gb->cpu.a);
			SET_HL(gb, wSubState);
			wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
			RET();
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0x3c);
		if (!gb->cpu.a) {
			JUMP_TO(ADDR_StartOverworldState);
			return;
		}
		gb->cpu.a = ST_LEVEL;
		wr8(gb, wState, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSubState, 0);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA);
	if (gb->cpu.f & FLAG_Z) {
		// .ClearData
		gb->cpu.a = 0x81;
		wr8(gb, wcee4, 0x81);
		gb->cpu.a = SCREEN_HEIGHT_PX;
		wr8(gb, wMenuObj2YCoord, gb->cpu.a);
		gb->cpu.a = 120;
		wr8(gb, wMenuObj1YCoord, 120);
		fs = ADDR_Frameset_6cec;
		gb->cpu.a = STARTMENU_CLEAR_DATA_NO;
	} else {
		alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA_YES);
		if (gb->cpu.f & FLAG_Z) {
			JUMP(ClearSaveData);
			return;
		}
		gb->cpu.a = 134;
		wr8(gb, wMenuObj2YCoord, 134);
		gb->cpu.a = 130;
		wr8(gb, wMenuObj1YCoord, 130);
		gb->cpu.a = rd8(gb, wGameModeFlags);
		if (alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
			fs = ADDR_Frameset_6d1e;
		} else {
			gb->cpu.a = 0x80;
			wr8(gb, wcee4, 0x80);
			fs = ADDR_Frameset_6ce9;
		}
		gb->cpu.a = STARTMENU_CONTINUE;
	}
	// .asm_45c5
	SET_BC(gb, fs);
	wr8(gb, wStartMenuSelection, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMenuObj1FramesetOffset + 0, 0);
	wr8(gb, wMenuObj1FramesetOffset + 1, 0); // duration
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wMenuObj1FramesetOffset + 2, gb->cpu.a); // frameset ptr
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wMenuObj1FramesetOffset + 3, gb->cpu.a);
	SET_HL(gb, wMenuObj1FramesetOffset + 3);
	CALL(UpdateObjAnim);
	RET();
}
