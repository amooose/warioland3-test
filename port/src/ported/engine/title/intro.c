// Port of src/engine/title/intro.asm
#include "ported.h"

#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)

PORT(StartTitleScreen)
{
	// play_music MUSIC_TITLE_SCREEN
	wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_TITLE_SCREEN >> 8));
	wr8(gb, hMusicID + 1, (uint8_t)MUSIC_TITLE_SCREEN);
	gb->cpu.a = (uint8_t)(ACTION_HELP_TIMER >> 8);
	wr8(gb, wTimer + 1, gb->cpu.a);
	gb->cpu.a = (uint8_t)ACTION_HELP_TIMER;
	wr8(gb, wTimer, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, SST_START_MENU);
}

// ld hl, addr / inc [hl]
static void inc_mem(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_inc(gb, rd8(gb, addr)));
}

// ld hl, addr / dec [hl]
static void dec_mem(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
}

// ld a, [addr] / and mask
static uint8_t ld_and_mask(GB *gb, uint16_t addr, uint8_t mask)
{
	gb->cpu.a = alu_and(gb, rd8(gb, addr), mask);
	return gb->cpu.a;
}

// ld hl, wPlaneWarioFramesetOffset / xor a / ld [hli], a / ld [hli], a /
// ld a, HIGH(fs) / ld [hli], a / ld a, LOW(fs) / ld [hl], a (or ld [hli], a)
static void plane_frameset(GB *gb, uint16_t fs, bool hli)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wPlaneWarioFramesetOffset, 0);
	wr8(gb, wPlaneWarioDuration, 0);
	wr8(gb, wPlaneWarioFramesetPtr + 0, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, wPlaneWarioFramesetPtr + 1, gb->cpu.a);
	SET_HL(gb, wPlaneWarioFramesetPtr + (hli ? 2 : 1));
}

static void load_intro_sprite(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadIntroSprite);
}

static void update_plane_anim(GB *gb)
{
	SET_HL(gb, wPlaneWarioEnd - 1);
	CALL(UpdateObjAnim);
}

// the sprites (in this order), ClearUnusedVirtualOAM, jp PanIntroBackgroundLayers
static void intro_sprites_and_pan(GB *gb, uint16_t o1, uint16_t o2, uint16_t o3)
{
	load_intro_sprite(gb, o1);
	load_intro_sprite(gb, o2);
	load_intro_sprite(gb, o3);
	CALL(ClearUnusedVirtualOAM);
	JUMP_TO(ADDR_PanIntroBackgroundLayers);
}

// dec a, n times, stopping at zero: the state picked (0 = none matched)
static int dec_chain(GB *gb, int n)
{
	gb->cpu.a = rd8(gb, wPlaneWarioVar);
	for (int i = 1; i <= n; i++) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (ZF)
			return i;
	}
	return 0;
}

// phase 1 of the Intro sequence: the plane flies close by
PORT_JUMP(IntroSequencePhase1)
{
	gb->cpu.a = rd8(gb, wceef);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!ZF) {
		JUMP_TO(ADDR_StartTitleScreen);
		return;
	}
	gb_call(gb, ADDR_PlayIntroSFXPlane_Close);
	gb_call(gb, ADDR_Func_4d45);

	switch (dec_chain(gb, 5)) {
	case 0:
		inc_mem(gb, wTimer);
		gb->cpu.a = rd8(gb, wTimer);
		alu_cp(gb, gb->cpu.a, 180);
		if (CF)
			break;
		wr8(gb, wTimer, 0);
		wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_INTRO >> 8));
		gb->cpu.a = (uint8_t)MUSIC_INTRO;
		wr8(gb, hMusicID + 1, gb->cpu.a);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 1:
		SET_HL(gb, wPlaneWarioXCoord);
		gb->cpu.a = rd8(gb, wPlaneWarioXCoord);
		alu_cp(gb, gb->cpu.a, 72);
		if (!CF) {
			dec_mem(gb, wPlaneWarioXCoord);
			dec_mem(gb, wPlaneWarioXCoord);
			break;
		}
		alu_cp(gb, gb->cpu.a, 56);
		if (!CF) {
			dec_mem(gb, wPlaneWarioXCoord);
			break;
		}
		alu_cp(gb, gb->cpu.a, 48);
		if (ZF) {
			gb->cpu.a = 48;
			wr8(gb, wTimer, 48);
			inc_mem(gb, wPlaneWarioVar);
			break;
		}
		if (ld_and_mask(gb, wGlobalCounter, 0x03))
			break;
		dec_mem(gb, wPlaneWarioXCoord);
		break;
	case 2:
		dec_mem(gb, wTimer);
		if (!ZF)
			break;
		wr8(gb, wTimer, 3);
		plane_frameset(gb, ADDR_Frameset_71a7, false);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 3:
		if (ld_and_mask(gb, wGlobalCounter, 0x07))
			break;
		inc_mem(gb, wPlaneWarioYCoord);
		gb->cpu.a = rd8(gb, wPlaneWarioYCoord);
		alu_cp(gb, gb->cpu.a, 70);
		if (!ZF)
			break;
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 4:
		if (ld_and_mask(gb, wGlobalCounter, 0x07))
			break;
		dec_mem(gb, wPlaneWario);
		gb->cpu.a = rd8(gb, wPlaneWario);
		alu_cp(gb, gb->cpu.a, 64);
		if (!ZF)
			break;
		dec_mem(gb, wTimer);
		if (ZF) {
			plane_frameset(gb, ADDR_Frameset_712c, false);
			inc_mem(gb, wPlaneWarioVar);
			break;
		}
		dec_mem(gb, wPlaneWarioVar);
		break;
	case 5:
		SET_HL(gb, wPlaneWarioXCoord);
		gb->cpu.a = rd8(gb, wPlaneWarioXCoord);
		alu_cp(gb, gb->cpu.a, 192);
		if (!ZF) {
			alu_cp(gb, gb->cpu.a, 56);
			if (CF && ld_and_mask(gb, wGlobalCounter, 0x03))
				break;
			inc_mem(gb, wPlaneWarioXCoord);
			break;
		}
		// .skip_move_right
		wr8(gb, wPlaneWarioYCoord, 16);
		wr8(gb, wPlaneWarioXCoord, 192);
		wr8(gb, wPlaneWarioFrame, 0);
		wr8(gb, wPlaneWarioAttributes, 0);
		wr8(gb, wPlaneWarioFramesetOffset, 0);
		wr8(gb, wPlaneWarioDuration, 0);
		wr8(gb, wPlaneWarioFramesetPtr + 0, (uint8_t)(ADDR_Frameset_713d >> 8));
		wr8(gb, wPlaneWarioFramesetPtr + 1, (uint8_t)ADDR_Frameset_713d);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wPlaneWarioVar, 0);
		wr8(gb, wSFXTimer, 0);
		gb->cpu.a = 28;
		wr8(gb, wTimer, 28);
		inc_mem(gb, wSubState);
		break;
	}

	// .continue
	gb_call(gb, ADDR_ScrollIntroBackgroundLayers);
	update_plane_anim(gb);
	intro_sprites_and_pan(gb, wPlaneWario, wMenuObj4, wMenuObj5);
}

// phase 2 of the Intro sequence: the plane comes into the centre far away
PORT_JUMP(IntroSequencePhase2)
{
	gb_call(gb, ADDR_Func_4d45);
	gb_call(gb, ADDR_PlayIntroSFXPlane_Far);

	switch (dec_chain(gb, 2)) {
	case 0:
		SET_HL(gb, wTimer);
		gb->cpu.a = rd8(gb, wTimer);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!ZF) {
			dec_mem(gb, wTimer);
			break;
		}
		// .move_to_centre
		SET_HL(gb, wPlaneWarioXCoord);
		gb->cpu.a = rd8(gb, wPlaneWarioXCoord);
		alu_cp(gb, gb->cpu.a, 64);
		if (ZF) {
			gb->cpu.a = 48;
			wr8(gb, wTimer, 48);
			inc_mem(gb, wPlaneWarioVar);
			break;
		}
		alu_cp(gb, gb->cpu.a, 72);
		if (ld_and_mask(gb, wGlobalCounter, CF ? 0x03 : 0x01))
			break;
		dec_mem(gb, wPlaneWarioXCoord);
		break;
	case 1:
		dec_mem(gb, wTimer);
		if (!ZF)
			break;
		plane_frameset(gb, ADDR_Frameset_7144, true);
		wr8(gb, wSFXTimer, 0);
		gb->cpu.a = 224;
		wr8(gb, wTimer, 224);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 2:
		gb->cpu.a = rd8(gb, wPlaneWarioFramesetOffset);
		alu_cp(gb, gb->cpu.a, 0x18);
		if (ZF) {
			gb->cpu.a = rd8(gb, wPlaneWarioDuration);
			gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (ZF)
				play_sfx(gb, SFX_023);
		}
		dec_mem(gb, wTimer);
		if (!ZF)
			break;
		play_sfx(gb, SFX_PLANE_EXPLOSION);
		plane_frameset(gb, ADDR_Frameset_7165, true);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wPlaneWarioVar, 0);
		wr8(gb, wSFXTimer, 0);
		gb->cpu.a = 48;
		wr8(gb, wTimer, 48);
		inc_mem(gb, wSubState);
		break;
	}

	// .continue
	gb_call(gb, ADDR_ScrollIntroBackgroundLayers);
	update_plane_anim(gb);
	intro_sprites_and_pan(gb, wMenuObj4, wPlaneWario, wMenuObj5);
}

// .State1 to .State3 of IntroSequencePhase3: true if the plane reached `y`
static bool phase3_move(GB *gb, uint8_t y)
{
	if (!ld_and_mask(gb, wGlobalCounter, 0x03))
		inc_mem(gb, wPlaneWarioXCoord);
	inc_mem(gb, wPlaneWarioYCoord);
	gb->cpu.a = rd8(gb, wPlaneWarioYCoord);
	alu_cp(gb, gb->cpu.a, y);
	return !CF;
}

// phase 3 of the Intro sequence: the plane wobbles, explodes and nosedives
PORT_JUMP(IntroSequencePhase3)
{
	gb_call(gb, ADDR_Func_4d45);

	bool finish_scroll = false;
	switch (dec_chain(gb, 4)) {
	case 0:
		gb->cpu.a = rd8(gb, wPlaneWarioAnimationEnded);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (ZF)
			break;
		play_sfx(gb, SFX_119);
		plane_frameset(gb, ADDR_Frameset_7172, true);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 1:
		if (!phase3_move(gb, 16))
			break;
		plane_frameset(gb, ADDR_Frameset_717b, true);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 2:
		if (!phase3_move(gb, 48))
			break;
		plane_frameset(gb, ADDR_Frameset_7184, true);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 3:
		if (!phase3_move(gb, 112))
			break;
		play_sfx(gb, SFX_061);
		plane_frameset(gb, ADDR_Frameset_718d, true);
		inc_mem(gb, wPlaneWarioVar);
		break;
	case 4:
		gb->cpu.a = rd8(gb, wIntroBGXOffsetNear);
		alu_cp(gb, gb->cpu.a, 224);
		if (!ZF) {
			inc_mem(gb, wPlaneWarioXCoord);
			inc_mem(gb, wPlaneWarioXCoord);
		}
		finish_scroll = true;
		gb->cpu.a = rd8(gb, wPlaneWarioAnimationEnded);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (ZF)
			break;
		plane_frameset(gb, ADDR_Frameset_7198, true);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wPlaneWarioVar, 0);
		SET_HL(gb, wPlaneWarioVar - 1);
		wr8(gb, wTimer, 0);
		SET_HL(gb, ADDR_Pals_5002);
		CALL(LoadPalsToTempPals1);
		SET_HL(gb, ADDR_Pals_5042);
		CALL(LoadPalsToTempPals2);
		wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_TITLE_SCREEN >> 8));
		gb->cpu.a = (uint8_t)MUSIC_TITLE_SCREEN;
		wr8(gb, hMusicID + 1, gb->cpu.a);
		inc_mem(gb, wSubState);
		break;
	}

	if (finish_scroll)
		gb_call(gb, ADDR_FinishIntroBackgroundScroll);
	else
		gb_call(gb, ADDR_ScrollIntroBackgroundLayers);

	// .update_anim
	update_plane_anim(gb);
	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	wr8(gb, wPlaneWarioAnimationEnded, gb->cpu.a);
	intro_sprites_and_pan(gb, wMenuObj4, wPlaneWario, wMenuObj5);
}

PORT_JUMP(EndIntroSequence)
{
	update_plane_anim(gb);
	load_intro_sprite(gb, wPlaneWario);
	CALL(ClearUnusedVirtualOAM);

	gb->cpu.a = rd8(gb, wceef);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!ZF) {
		JUMP_TO(ADDR_StartTitleScreen);
		return;
	}
	SET_HL(gb, wTimer);
	gb->cpu.a = rd8(gb, wTimer);
	alu_cp(gb, gb->cpu.a, 120);
	if (CF) {
		wr8(gb, wTimer, alu_inc(gb, gb->cpu.a));
		RET();
		return;
	}
	// .fade_in_title
	if (!ld_and_mask(gb, wGlobalCounter, 0x03))
		CALL(SlowFadeInScreen);
	gb->cpu.a = rd8(gb, wSubState);
	alu_cp(gb, gb->cpu.a, SST_START_MENU);
	if (!ZF) {
		RET();
		return;
	}
	JUMP_TO(ADDR_StartTitleScreen);
}

// ld hl, obj / y, x, xor a, 4 zeros, then the frameset pointer
// (big-endian); hl ends on the frameset pointer's low byte
static void init_intro_obj(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint16_t fs)
{
	wr8(gb, obj + 0, y);
	gb->cpu.a = x;
	wr8(gb, obj + 1, x);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 2; i < 6; i++)
		wr8(gb, obj + i, 0);
	wr8(gb, obj + 6, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, obj + 7, gb->cpu.a);
	SET_HL(gb, obj + 7);
}

// the plane: init_intro_obj, then xor a / ld [hld], a (state) / ld [wSFXTimer], a
static void init_plane(GB *gb, uint8_t y, uint8_t x, uint16_t fs)
{
	init_intro_obj(gb, wPlaneWario, y, x, fs);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPlaneWario + 8, 0); // state
	wr8(gb, wSFXTimer, 0);
}

PORT(InitIntroSequence)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);
	gb_call(gb, ADDR_LoadTitleScreenPals);
	gb_call(gb, ADDR_LoadTitleScreenTiles);
	gb_call(gb, ADDR_LoadTitleScreenTileMap);
	gb_call(gb, ADDR_VBlank_Title);

	gb->cpu.a = 0x04;
	wr8(gb, wSCY, 0x04);
	wr8(gb, rSCY, 0x04);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), rd8(gb, wceef));
	if (!gb->cpu.a) {
		init_plane(gb, 64, 208, ADDR_Frameset_712c);
		CALL(UpdateObjAnim);
		init_intro_obj(gb, wMenuObj1, 128, 80, ADDR_Frameset_6ce6);
		CALL(UpdateObjAnim);
		init_intro_obj(gb, wMenuObj4, 16, 0, ADDR_Frameset_71a1);
		CALL(UpdateObjAnim);
		init_intro_obj(gb, wMenuObj5, 8, 32, ADDR_Frameset_71a4);
		CALL(UpdateObjAnim);
		load_intro_sprite(gb, wMenuObj4);
		load_intro_sprite(gb, wMenuObj5);
		gb->cpu.a = STARTMENU_NEW_GAME;
		wr8(gb, wStartMenuSelection, gb->cpu.a);
	} else {
		// .no_intro
		init_plane(gb, 108, 128, ADDR_Frameset_7198);

		// set priority flag to hide the menu options behind scenery
		gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
		wr8(gb, rVBK, gb->cpu.a);
		uint16_t hl = v0BGMap0 + 12 * TILEMAP_WIDTH + 4;
		SET_DE(gb, TILEMAP_WIDTH);
		gb->cpu.c = 2;
		do {
			for (int row = 0; row < 2; row++) {
				gb_push16(gb, hl);
				gb->cpu.b = 11;
				do {
					gb->cpu.a = alu_or(gb, rd8(gb, hl), 0x80);
					wr8(gb, hl++, gb->cpu.a);
					gb->cpu.b = alu_dec(gb, gb->cpu.b);
				} while (gb->cpu.b);
				hl = gb_pop16(gb);
				hl = alu_add16(gb, hl, REG_DE(gb));
			}
			hl = alu_add16(gb, hl, REG_DE(gb));
			hl = alu_add16(gb, hl, REG_DE(gb));
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
		} while (gb->cpu.c);

		// get darker tone behind menu options
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a (VRAM0)
		wr8(gb, rVBK, 0);
		SET_HL(gb, ADDR_TitleScreenBoxTilemap);
		SET_DE(gb, v0BGMap0 + 14 * TILEMAP_WIDTH + 4);
		gb_push16(gb, REG_DE(gb));
		gb->cpu.b = 12;
		CALL(CopyHLToDE);
		SET_DE(gb, gb_pop16(gb));
		gb->cpu.e = gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH);
		gb->cpu.d = gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
		gb->cpu.b = 12;
		CALL(CopyHLToDE);

		gb->cpu.a = rd8(gb, wGameModeFlags);
		bool ta = alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F);
		init_intro_obj(gb, wMenuObj1, 0x82, 0x50, ta ? ADDR_Frameset_6d1e : ADDR_Frameset_6ce9);
		// (the flags are those of the bit test)
		gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_H | (ta ? 0 : FLAG_Z));
		CALL(UpdateObjAnim);

		wr8(gb, wMenuObj2 + 0, 0x86); // y
		gb->cpu.a = rd8(gb, wGameModeFlags);
		ta = alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F);
		init_intro_obj(gb, wMenuObj2, 0x86, ta ? 0x40 : 0x44, ADDR_Frameset_6d1b);
		CALL(UpdateObjAnim);

		gb->cpu.a = STARTMENU_CONTINUE;
		wr8(gb, wStartMenuSelection, gb->cpu.a);
	}

	// .asm_41a8
	init_intro_obj(gb, wMenuObj3, 36, 104, ADDR_Frameset_6d18);
	CALL(UpdateObjAnim);

	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
	wr8(gb, wGlobalCounter, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
