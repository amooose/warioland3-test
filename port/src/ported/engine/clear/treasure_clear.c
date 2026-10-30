// Port of src/engine/clear/treasure_clear.asm
#include "ported.h"

PORT(OpenMusicalCoinsScreen)
{
	gb->cpu.a = SST_CLEAR_MUSICAL_COINS;
	wr8(gb, wSubState, SST_CLEAR_MUSICAL_COINS);
}

static void load_clear_screen_sprite(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadClearScreenSprite);
}

PORT(UpdateClearScreen)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_LEVEL_CLEAR);
	if (gb->cpu.a) {
		SET_HL(gb, wMenuObj8FramesetPtr + 1);
		CALL(UpdateObjAnim);
		load_clear_screen_sprite(gb, wMenuObj8);
	}
	SET_HL(gb, wClearScreenWarioFramesetPtr + 1);
	CALL(UpdateObjAnim);
	gb->cpu.a = alu_and(gb, rd8(gb, wClearScreenWarioVar), rd8(gb, wClearScreenWarioVar));
	if (!gb->cpu.a) {
		gb->cpu.a = alu_and(gb, rd8(gb, wObjAnimWasReset), rd8(gb, wObjAnimWasReset));
		if (gb->cpu.a) {
			gb->cpu.a = TRUE;
			wr8(gb, wClearScreenWarioVar, TRUE);
			// reset animation
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wClearScreenWarioFramesetOffset + 0, 0);
			wr8(gb, wClearScreenWarioFramesetOffset + 1, 0);
			gb->cpu.a = rd8(gb, wTransitionParam);
			alu_cp(gb, gb->cpu.a, TRANSITION_RETURN_TO_MAP);
			uint16_t fs = (gb->cpu.f & FLAG_Z) ? ADDR_Frameset_d4d5f : ADDR_Frameset_d4d1b;
			wr8(gb, wClearScreenWarioFramesetOffset + 2, fs >> 8);
			gb->cpu.a = fs & 0xff;
			wr8(gb, wClearScreenWarioFramesetOffset + 3, gb->cpu.a);
			SET_HL(gb, wClearScreenWarioFramesetOffset + 3);
			CALL(UpdateObjAnim);
		}
	}
	SET_HL(gb, wClearScreenWario);
	gb_call(gb, ADDR_LoadClearScreenWarioSprite);

	load_clear_screen_sprite(gb, wMenuObj2);
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	alu_cp(gb, gb->cpu.a, LVLEND_NO_TREASURE);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, wMenuObj1FramesetPtr + 1);
		CALL(UpdateObjAnim);
		load_clear_screen_sprite(gb, wMenuObj1);
	}

	// .draw_music_boxes
	gb->cpu.a = alu_and(gb, rd8(gb, wNumMusicBoxes), rd8(gb, wNumMusicBoxes));
	int n = 0;
	if (gb->cpu.a) {
		n = 5;
		for (int i = 1; i <= 4; i++) {
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (!gb->cpu.a) {
				n = i;
				break;
			}
		}
	}
	static const uint16_t boxes[] = { wMenuObj7, wMenuObj6, wMenuObj5, wMenuObj4, wMenuObj3 };
	for (int i = n - 1; i >= 0; i--)
		load_clear_screen_sprite(gb, boxes[i]);
	CALL(ClearUnusedVirtualOAM);

	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	// fallthrough to OpenMusicalCoinsScreen
	gb->cpu.a = SST_CLEAR_MUSICAL_COINS;
	wr8(gb, wSubState, gb->cpu.a);
}

// ld hl, obj + FramesetPtr + 1 / call UpdateObjAnim / ld hl, obj /
// call LoadClearScreenSprite (the FramesetPtr is 6 bytes in)
static void update_clear_screen_obj(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj + 7);
	CALL(UpdateObjAnim);
	load_clear_screen_sprite(gb, obj);
}

// hlbgcoord x, 8 / ld a, tile / ld [hli], a / inc a / ld [hl], a
static void put_treasure_tiles(GB *gb, uint8_t x, uint8_t tile)
{
	uint16_t hl = v0BGMap0 + 8 * TILEMAP_WIDTH + x;
	gb->cpu.a = tile;
	wr8(gb, hl++, tile);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(InitClearScreen)
{
	gb->cpu.a = TRUE;
	wr8(gb, wResetDisabled, TRUE);

	CALL(DisableLCD);
	CALL(ClearBGMap0);
	CALL(ClearVirtualOAM);

	gb_call(gb, ADDR_SetTreasureTransitionParam);

	// push_wram BANK(wTreasuresCollected)
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wTreasuresCollected;
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTransitionParam);
	alu_cp(gb, gb->cpu.a, TRANSITION_RETURN_TO_MAP);
	if (!(gb->cpu.f & FLAG_Z)) {
		CALL(IsTreasureCollected);
		uint16_t hl = REG_HL(gb);
		gb->cpu.a = alu_or(gb, rd8(gb, hl), gb->cpu.d);
		wr8(gb, hl, gb->cpu.a);
	}
	// .return_to_map
	gb->cpu.a = rd8(gb, wTreasuresCollected);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	uint8_t num = 0;
	for (int n = 5; n >= 1; n--) {
		gb->cpu.a = alu_rlca(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_C) {
			num = (uint8_t)n;
			break;
		}
	}
	if (num) {
		gb->cpu.a = num;
	} else {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	// .got_number_music_boxes
	wr8(gb, wNumMusicBoxes, gb->cpu.a);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);

	FARCALL(LoadClearScreenPals);
	CALL(LoadCollectedTreasurePal_ClearScreen);
	FARCALL(LoadClearScreenGfx);
	FARCALL(LoadLevelTreasureData);
	FARCALL(Func_1e0045);

	SET_HL(gb, v0BGMap0 + 16 * TILEMAP_WIDTH + 6); // hlbgcoord 6, 16
	FARCALL(PrintNumberCoins);

	gb->cpu.a = rd8(gb, wKeyAndTreasureFlags);
	gb->cpu.b = gb->cpu.a;
	if (alu_bit(gb, gb->cpu.b, GREY_TREASURE_F))
		put_treasure_tiles(gb, 3, 0x54);
	// .check_red
	if (alu_bit(gb, gb->cpu.b, RED_TREASURE_F))
		put_treasure_tiles(gb, 7, 0x54);
	// .check_green
	if (alu_bit(gb, gb->cpu.b, GREEN_TREASURE_F))
		put_treasure_tiles(gb, 11, 0x5c);
	// .check_blue
	if (alu_bit(gb, gb->cpu.b, BLUE_TREASURE_F))
		put_treasure_tiles(gb, 15, 0x5c);

	// .ok
	gb_call(gb, ADDR_UpdateNumCollectedTreasures);
	gb_call(gb, ADDR_VBlank_ClearScreen);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	gb->cpu.a = 0x04;
	wr8(gb, wSCY, 0x04);
	wr8(gb, rSCY, 0x04);
	FARCALL(Func_1e015b);

	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_LEVEL_CLEAR);
	if (!(gb->cpu.f & FLAG_Z))
		update_clear_screen_obj(gb, wMenuObj8);
	// .no_power_up
	SET_HL(gb, wClearScreenWarioFramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wClearScreenWario);
	gb_call(gb, ADDR_LoadClearScreenWarioSprite);

	gb->cpu.a = rd8(gb, wLevelEndScreen);
	alu_cp(gb, gb->cpu.a, LVLEND_NO_TREASURE);
	if (!(gb->cpu.f & FLAG_Z))
		update_clear_screen_obj(gb, wMenuObj1);
	// .no_treasure
	update_clear_screen_obj(gb, wMenuObj2);

	gb->cpu.a = rd8(gb, wNumMusicBoxes);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		// first object to update: wMenuObj3 for all music boxes,
		// down to wMenuObj7 for one
		int first = 3;
		for (int n = 1; n <= 4; n++) {
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z) {
				first = 8 - n;
				break;
			}
		}
		static const uint16_t objs[] = {
			wMenuObj3, wMenuObj4, wMenuObj5, wMenuObj6, wMenuObj7,
		};
		for (int i = first; i <= 7; i++)
			update_clear_screen_obj(gb, objs[i - 3]);
	}

	// .no_music_boxes
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
