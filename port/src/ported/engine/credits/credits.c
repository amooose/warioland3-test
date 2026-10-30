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

// ld hl, obj / y / x / xor a / 4x [hli] / (ld [var], a) / frameset pointer
// (big-endian); hl ends on its low byte
static void init_obj(GB *gb, uint16_t hl, uint8_t y, uint8_t x, uint16_t frameset, bool clear_var)
{
	wr8(gb, hl++, y);
	gb->cpu.a = x;
	wr8(gb, hl++, x);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	if (clear_var)
		wr8(gb, wCreditsMusicBoxVar, 0);
	wr8(gb, hl++, (uint8_t)(frameset >> 8));
	gb->cpu.a = (uint8_t)frameset;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

static void set_music_box_oam(GB *gb, uint8_t bank, uint16_t oam)
{
	wr8(gb, wCreditsMusicBoxOAMPtr + 0, (uint8_t)(oam >> 8));
	wr8(gb, wCreditsMusicBoxOAMPtr + 1, (uint8_t)oam);
	gb->cpu.a = bank;
	wr8(gb, wCreditsMusicBoxOAMBank, bank);
}

static void inc_substate(GB *gb)
{
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

// ld hl, wCreditsMusicBoxFramesetPtr + 1 / ld a, [wCreditsMusicBoxOAMBank] /
// ld [wTempBank], a / ld a, [wTempBank] / ldh [hCallFuncBank], a /
// hcall UpdateObjAnim
static void update_music_box_anim(GB *gb)
{
	SET_HL(gb, wCreditsMusicBoxFramesetPtr + 1);
	gb->cpu.a = rd8(gb, wCreditsMusicBoxOAMBank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(UpdateObjAnim);
}

static void play_music(GB *gb, uint16_t music)
{
	wr8(gb, hMusicID + 0, (uint8_t)(music >> 8));
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicID + 1, gb->cpu.a);
}

PORT(InitCredits)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);

	gb_call(gb, ADDR_LoadCreditsPals);
	gb_call(gb, ADDR_LoadCreditsGfx);
	gb_call(gb, ADDR_Func_16041f);
	gb_call(gb, ADDR_VBlank_160439);

	gb->cpu.a = 0x04;
	wr8(gb, rSCX, 0x04);
	wr8(gb, wSCX, 0x04);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, wCreditsPage, 0);
	wr8(gb, rWY, 0);
	wr8(gb, wcee4, 0);
	gb->cpu.a = 0 + WX_OFS;
	wr8(gb, rWX, gb->cpu.a);

	wr8(gb, wCreditsBGMapPtr + 0, (uint8_t)((v0BGMap0 + 0x2e7) >> 8));
	gb->cpu.a = (uint8_t)(v0BGMap0 + 0x2e7);
	wr8(gb, wCreditsBGMapPtr + 1, gb->cpu.a);

	uint16_t hl = wCreditsTextPtr;
	wr8(gb, hl++, (uint8_t)(ADDR_CreditsText >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_CreditsText);
	wr8(gb, hl++, (uint8_t)(ADDR_CreditsPalsMap >> 8)); // wCreditsPalsMapPtr
	gb->cpu.a = (uint8_t)ADDR_CreditsPalsMap;
	wr8(gb, hl, gb->cpu.a);

	init_obj(gb, wCreditsMusicBox, 0x5b, 0x18, ADDR_Frameset_161cc4, true);
	set_music_box_oam(gb, BANK_OAM_1617e8, ADDR_OAM_1617e8);
	CALL(UpdateObjAnim);
	SET_HL(gb, wCreditsMusicBox);
	gb_call(gb, ADDR_LoadCreditsSprite);

	init_obj(gb, wMenuObj1, 0x5b, 0x18, ADDR_Frameset_161caf, false);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_LoadCreditsSprite);

	CALL(ClearUnusedVirtualOAM);

	// stop_music
	wr8(gb, hMusicID + 0, 0xff);
	wr8(gb, hMusicID + 1, 0x00);
	gb->cpu.a = 120; // timer before starting music
	wr8(gb, wTimer, gb->cpu.a);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	inc_substate(gb);
}

// loads the big-endian pointer at ptr into hl
static uint16_t ld_ptr(GB *gb, uint16_t ptr)
{
	gb->cpu.a = rd8(gb, ptr);
	gb->cpu.l = rd8(gb, ptr + 1);
	gb->cpu.h = gb->cpu.a;
	return REG_HL(gb);
}

PORT(ShowCredits)
{
	SET_HL(gb, wTimer);
	gb->cpu.a = rd8(gb, wTimer);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		// still counting down to play music
		wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
		if (gb->cpu.f & FLAG_Z)
			play_music(gb, MUSIC_CREDITS); // .PlayCreditsMusic
		SET_HL(gb, wCreditsMusicBox);
		gb_call(gb, ADDR_LoadCreditsSprite);
		SET_HL(gb, wMenuObj1);
		gb_call(gb, ADDR_LoadCreditsSprite);
		CALL(ClearUnusedVirtualOAM);
		return;
	}

	// .continue_credits
	SET_HL(gb, wCreditsMusicBoxFramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wCreditsMusicBox);
	gb_call(gb, ADDR_LoadCreditsSprite);
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_LoadCreditsSprite);
	CALL(ClearUnusedVirtualOAM);

	gb->cpu.a = rd8(gb, wCreditsPage);
	alu_cp(gb, gb->cpu.a, NUM_CREDITS_PAGES);
	if (gb->cpu.f & FLAG_Z) {
		// .done_credits
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCreditsMusicBoxVar, 0);
		inc_substate(gb);
		return;
	}
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, NUM_CREDITS_PAGES - 1);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1);
			if (!(gb->cpu.f & FLAG_Z))
				return;
		}
	}

	// .scroll
	gb->cpu.a = alu_add(gb, rd8(gb, wSCY), 1);
	wr8(gb, wSCY, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, wSCY + 1), 0); // wCreditsPage
	wr8(gb, wSCY + 1, gb->cpu.a);
	SET_HL(gb, wSCY);
	alu_cp(gb, gb->cpu.a, 1);
	if (gb->cpu.f & FLAG_C)
		return;
	alu_cp(gb, gb->cpu.a, NUM_CREDITS_PAGES);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wSCY), 0x7);
	if (!(gb->cpu.f & FLAG_Z))
		return;

	// write next credits line to buffer
	uint16_t hl = ld_ptr(gb, wCreditsTextPtr);
	SET_DE(gb, wCreditsTextBuffer);
	do {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e++; // inc e
		alu_cp(gb, gb->cpu.a, 0x7f); // newline in the credits charmap
	} while (!(gb->cpu.f & FLAG_Z));
	wr8(gb, wCreditsTextPtr + 0, (uint8_t)(hl >> 8));
	gb->cpu.a = (uint8_t)hl;
	wr8(gb, wCreditsTextPtr + 1, gb->cpu.a);

	// load pals of each letter
	hl = ld_ptr(gb, wCreditsPalsMapPtr);
	SET_DE(gb, wCreditsTextPals);
	gb->cpu.b = CREDITS_TEXT_LENGTH;
	do {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e++; // inc e
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	wr8(gb, wCreditsPalsMapPtr + 0, (uint8_t)(hl >> 8));
	gb->cpu.a = (uint8_t)hl;
	wr8(gb, wCreditsPalsMapPtr + 1, gb->cpu.a);

	hl = ld_ptr(gb, wCreditsBGMapPtr);
	SET_DE(gb, TILEMAP_WIDTH);
	hl = alu_add16(gb, hl, TILEMAP_WIDTH);
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, gb->cpu.h, 0xfb); // make sure it loops properly
	wr8(gb, wCreditsBGMapPtr + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, wCreditsBGMapPtr + 1, gb->cpu.a);

	gb->cpu.a = (1 << 7);
	wr8(gb, wcee4, gb->cpu.a);
}

PORT(Func_160190)
{
	gb->cpu.a = rd8(gb, wCreditsMusicBoxVar);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		SET_HL(gb, wCreditsMusicBoxXCoord);
		gb->cpu.a = rd8(gb, wCreditsMusicBoxXCoord);
		alu_cp(gb, gb->cpu.a, 0x38);
		bool move = false;
		if (gb->cpu.f & FLAG_C) {
			move = true;
		} else {
			alu_cp(gb, gb->cpu.a, 0x50);
			if (gb->cpu.f & FLAG_C) {
				// .move_right_slow
				gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1);
				if (!(gb->cpu.f & FLAG_Z))
					goto add_sprites;
				move = true;
			} else {
				// stop moving
				gb->cpu.a = 0x1;
				wr8(gb, wCreditsMusicBoxVar, 0x1);
			}
		}
		if (move) {
			// .move_right
			wr8(gb, wCreditsMusicBoxXCoord, alu_inc(gb, rd8(gb, wCreditsMusicBoxXCoord)));
			SET_HL(gb, wMenuObj1XCoord);
			wr8(gb, wMenuObj1XCoord, alu_inc(gb, rd8(gb, wMenuObj1XCoord)));
			goto update_anims;
		}
	}

	// .stopped_moving
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1);
	if (!(gb->cpu.f & FLAG_Z))
		goto add_sprites;
	gb->cpu.a = rd8(gb, wCreditsMusicBoxAnimationEnded);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		goto update_anims;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCreditsMusicBoxAnimationEnded, 0);
	gb->cpu.a = alu_inc(gb, rd8(gb, wCreditsMusicBoxVar));
	wr8(gb, wCreditsMusicBoxVar, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x3);
	if (gb->cpu.f & FLAG_C)
		goto update_anims;
	alu_cp(gb, gb->cpu.a, 0x4);
	if (gb->cpu.f & FLAG_Z) {
		// .start_close_up
		init_obj(gb, wCreditsMusicBox, 0x5b, 0x50, ADDR_Frameset_161ce8, true);
		set_music_box_oam(gb, BANK_OAM_1617e8, ADDR_OAM_1617e8);
		init_obj(gb, wMenuObj1, 0xe0, 0x4e, ADDR_Frameset_161ce5, false);
		gb->cpu.a = 16;
		wr8(gb, wTimer, gb->cpu.a);
		inc_substate(gb);
		goto update_anims;
	}
	// reset animation
	{
		uint16_t hl = wCreditsMusicBoxFramesetOffset;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl++, 0);
		wr8(gb, hl++, 0);
		wr8(gb, hl++, (uint8_t)(ADDR_Frameset_161cee >> 8));
		gb->cpu.a = (uint8_t)ADDR_Frameset_161cee;
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
	}

update_anims:
	update_music_box_anim(gb);
	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	wr8(gb, wCreditsMusicBoxAnimationEnded, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCreditsMusicBoxVar);
	alu_cp(gb, gb->cpu.a, 0x03);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x3);
		if (!(gb->cpu.f & FLAG_Z))
			goto add_sprites;
	}
	// .update_winding_key
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
add_sprites:
	SET_HL(gb, wCreditsMusicBox);
	CALL(LoadCreditsMusicBoxSprite);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_LoadCreditsSprite);
	CALL(ClearUnusedVirtualOAM);
}

PORT(Func_16026c)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (gb->cpu.f & FLAG_Z) {
		// .close_up
		init_obj(gb, wCreditsMusicBox, 0x50, 0x50, ADDR_Frameset_15e7ed, false);
		set_music_box_oam(gb, BANK_OAM_15e44a, ADDR_OAM_15e44a);
		gb_call(gb, ADDR_Func_1603be);
		gb->cpu.a = 0x2 | (1 << 7);
		wr8(gb, wcee4, gb->cpu.a);
		inc_substate(gb);
	} else {
		gb->cpu.a = rd8(gb, wTimer);
		alu_cp(gb, gb->cpu.a, 6);
		if (!(gb->cpu.f & FLAG_Z))
			goto add_sprites;
		init_obj(gb, wCreditsMusicBox, 0x5b, 0x50, ADDR_Frameset_161ceb, false);
		set_music_box_oam(gb, BANK_OAM_1617e8, ADDR_OAM_1617e8);
	}
	update_music_box_anim(gb);
add_sprites:
	SET_HL(gb, wCreditsMusicBox);
	CALL(LoadCreditsMusicBoxSprite);
	CALL(ClearUnusedVirtualOAM);
}

PORT(Func_1602ee)
{
	update_music_box_anim(gb);
	SET_HL(gb, wCreditsMusicBox);
	CALL(LoadCreditsMusicBoxSprite);
	CALL(ClearUnusedVirtualOAM);
	gb_call(gb, ADDR_Func_1603cb);
	inc_substate(gb);
}

// shared start of Func_160318 / Func_160365; slow: only every other frame
static void move_winding_key(GB *gb, bool slow)
{
	update_music_box_anim(gb);
	SET_HL(gb, wCreditsMusicBox);
	CALL(LoadCreditsMusicBoxSprite);
	SET_HL(gb, wMenuObj1);
	gb->cpu.a = rd8(gb, wMenuObj1);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) {
		bool move = true;
		if (slow) {
			gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1);
			move = (gb->cpu.f & FLAG_Z) != 0;
		}
		if (move)
			wr8(gb, wMenuObj1, alu_dec(gb, rd8(gb, wMenuObj1)));
	}
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_LoadCreditsSprite);
	CALL(ClearUnusedVirtualOAM);
}

PORT(Func_160318)
{
	move_winding_key(gb, true);
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0xf);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	CALL(SlowFadeInScreen);
	gb->cpu.a = 180;
	wr8(gb, wTimer, gb->cpu.a);
}

PORT_JUMP(Func_160365)
{
	move_winding_key(gb, false);
	gb->cpu.a = rd8(gb, wTransitionParam);
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_PERFECT);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadPressed);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP(SetPerfectState);
		return;
	}
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP(SetPerfectState);
}
