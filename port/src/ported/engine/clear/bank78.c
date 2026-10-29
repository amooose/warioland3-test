// Port of src/engine/clear/bank78.asm
#include "ported.h"

static void load_pals(GB *gb, uint16_t pals1, uint16_t pals2)
{
	SET_HL(gb, pals1);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, pals2);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadClearScreenPals) { load_pals(gb, ADDR_Pals_1e0378, ADDR_Pals_1e03b8); }
PORT(Func_1e000d) { load_pals(gb, ADDR_Pals_1e03f8, ADDR_Pals_1e0438); }

PORT(LoadClearScreenGfx)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_ClearScreen2Gfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ClearScreen1Gfx);
	SET_BC(gb, v0Tiles1);
	CALL(Decompress);
	SET_HL(gb, ADDR_WarioClearGfx);
	SET_DE(gb, v0Tiles0);
	SET_BC(gb, 0x80 * TILE_SIZE);
	gb->cpu.a = BANK_WarioClearGfx;
	wr8(gb, wTempBank, gb->cpu.a);
	CALL(FarCopyHLToDE_BC);
}

PORT(Func_1e0045)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_ClearScreenTreasureAttrmap);
	SET_BC(gb, v1BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ClearScreenTreasureTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}

// ld hl, obj / y, x, xor a, 4 zeros (frame, attributes, frameset offset,
// duration), then the frameset pointer (big-endian), ending on ld [hl], a
static void init_menu_obj(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint16_t fs)
{
	wr8(gb, obj + 0, y);
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

// sets up the clear screen sprites: the power-up, Wario, the treasure and
// the music boxes
PORT(Func_1e015b)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_LEVEL_CLEAR);
	if (!(gb->cpu.f & FLAG_Z))
		init_menu_obj(gb, wMenuObj8, 0x53, 0x24, ADDR_Frameset_d506b);

	// .no_power_up
	wr8(gb, wClearScreenWario + 0, 0x76);
	wr8(gb, wClearScreenWario + 1, 0x34);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 2; i < 6; i++)
		wr8(gb, wClearScreenWario + i, 0);
	wr8(gb, wClearScreenWarioVar, 0);
	gb->cpu.a = rd8(gb, wTransitionParam);
	alu_cp(gb, gb->cpu.a, TRANSITION_RETURN_TO_MAP);
	uint16_t fs = (gb->cpu.f & FLAG_Z) ? ADDR_Frameset_d4d58 : ADDR_Frameset_d4d18;
	wr8(gb, wClearScreenWario + 6, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, wClearScreenWario + 7, gb->cpu.a);

	// .asm_1e019c
	SET_HL(gb, wMenuObj1);
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	static const uint8_t screens[4] = {
		LVLEND_GREY_TREASURE, LVLEND_RED_TREASURE, LVLEND_GREEN_TREASURE, LVLEND_BLUE_TREASURE,
	};
	int t;
	for (t = 0; t < 4; t++) {
		alu_cp(gb, gb->cpu.a, screens[t]);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	uint16_t music = MUSIC_LEVEL_CLEAR_NO_TREASURE;
	if (t < 4) {
		// .got_treasure_params (the params are right before Func_1e028a)
		uint16_t de = (uint16_t)(ADDR_Func_1e028a - 16 + 4 * t);
		uint16_t hl = wMenuObj1;
		wr8(gb, hl++, rd8(gb, de++));
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		for (int i = 0; i < 4; i++)
			wr8(gb, hl++, 0);
		de++;
		wr8(gb, hl++, rd8(gb, de++));
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
		SET_DE(gb, de);
		music = MUSIC_LEVEL_CLEAR_GOT_TREASURE;
	}
	// play_music
	wr8(gb, hMusicID + 0, (uint8_t)(music >> 8));
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicID + 1, gb->cpu.a);

	// .no_treasure: stop_sfx
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);

	init_menu_obj(gb, wMenuObj2, 0x28, 0x30, ADDR_Frameset_d5059);
	gb->cpu.a = rd8(gb, wNumMusicBoxes);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	int n;
	for (n = 1; n <= 4; n++) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	// n = 1..4 music boxes, 5 = all five
	if (n >= 5)
		init_menu_obj(gb, wMenuObj3, 0x79, 0x84, ADDR_Frameset_d5068);
	if (n >= 4)
		init_menu_obj(gb, wMenuObj4, 0x8d, 0x7f, ADDR_Frameset_d5065);
	if (n >= 3)
		init_menu_obj(gb, wMenuObj5, 0x8c, 0x69, ADDR_Frameset_d5062);
	if (n >= 2)
		init_menu_obj(gb, wMenuObj6, 0x79, 0x62, ADDR_Frameset_d505f);
	init_menu_obj(gb, wMenuObj7, 0x67, 0x74, ADDR_Frameset_d505c);
}

// writes the two tiles of a BCD digit at hl (top) and hl + TILEMAP_WIDTH,
// with bc = -TILEMAP_WIDTH and de = TILEMAP_WIDTH; last skips the
// ld [hli], a / add hl, bc ending (ld [hl], a instead)
static void put_digit(GB *gb, uint8_t v, bool high, bool last)
{
	gb->cpu.a = v;
	if (high)
		gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, gb->cpu.a);
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	if (!last) {
		hl++;
		hl = alu_add16(gb, hl, REG_BC(gb));
	}
	SET_HL(gb, hl);
}

static void put_time(GB *gb, uint16_t coord, uint16_t time, bool last)
{
	SET_HL(gb, coord);
	SET_BC(gb, (uint16_t)-TILEMAP_WIDTH);
	SET_DE(gb, TILEMAP_WIDTH);
	put_digit(gb, rd8(gb, time + 0), true, false);
	put_digit(gb, rd8(gb, time + 0), false, false);
	gb->cpu.l = alu_inc(gb, gb->cpu.l); // inc l
	put_digit(gb, rd8(gb, time + 1), true, false);
	put_digit(gb, rd8(gb, time + 1), false, last);
}

// hl = wLevelTimeAttackScores + (wLevel / 8) * 2
static uint16_t level_score_ptr(GB *gb)
{
	gb->cpu.a = rd8(gb, wLevel);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, wLevelTimeAttackScores, REG_DE(gb));
	SET_HL(gb, hl);
	return hl;
}

PORT(Func_1e005f)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_ClearScreenTimeAttackAttrmap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ClearScreenTimeAttackTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);

	// .GetResult
	do {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a (TIME_ATTACK_NOT_COMPLETED)
		wr8(gb, wTimeAttackResult, 0);
		gb->cpu.a = alu_and(gb, rd8(gb, wKeyAndTreasureFlags), KEYS_MASK);
		alu_cp(gb, gb->cpu.a, GREY_KEY | RED_KEY | GREEN_KEY | BLUE_KEY);
		if (!(gb->cpu.f & FLAG_Z))
			break;
		gb->cpu.a = TIME_ATTACK_NO_HIGH_SCORE;
		wr8(gb, wTimeAttackResult, gb->cpu.a);
		uint16_t hl = level_score_ptr(gb);
		gb->cpu.a = rd8(gb, wLevelTime + 0);
		alu_cp(gb, gb->cpu.a, rd8(gb, hl));
		if (!(gb->cpu.f & FLAG_C)) {
			if (!(gb->cpu.f & FLAG_Z))
				break;
			hl++;
			SET_HL(gb, hl);
			gb->cpu.a = rd8(gb, wLevelTime + 1);
			alu_cp(gb, gb->cpu.a, rd8(gb, hl));
			if (!(gb->cpu.f & FLAG_C))
				break;
			hl--;
		}
		gb->cpu.a = rd8(gb, wLevelTime + 0);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = rd8(gb, wLevelTime + 1);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
		gb->cpu.a = TIME_ATTACK_GOT_HIGH_SCORE;
		wr8(gb, wTimeAttackResult, gb->cpu.a);
	} while (0);

	uint16_t hl = level_score_ptr(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w3d511 + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, w3d511 + 1, gb->cpu.a);
	put_time(gb, v0BGMap0 + 7 * TILEMAP_WIDTH + 10, w3d511, false);

	gb->cpu.a = rd8(gb, wTimeAttackResult);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	put_time(gb, v0BGMap0 + 10 * TILEMAP_WIDTH + 10, wLevelTime, true);
}

PORT(Func_1e028a)
{
	gb->cpu.a = rd8(gb, wTimeAttackResult);
	alu_cp(gb, gb->cpu.a, TIME_ATTACK_GOT_HIGH_SCORE);
	if (gb->cpu.f & FLAG_Z)
		init_menu_obj(gb, wMenuObj8, 0x60, 0x1c, ADDR_Frameset_d5080);

	wr8(gb, wClearScreenWario + 0, SCREEN_HEIGHT_PX);
	wr8(gb, wClearScreenWario + 1, 0x2c);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 2; i < 6; i++)
		wr8(gb, wClearScreenWario + i, 0);
	wr8(gb, wActionHelpWarioVar, 0);
	gb->cpu.a = rd8(gb, wTimeAttackResult);
	alu_cp(gb, gb->cpu.a, TIME_ATTACK_GOT_HIGH_SCORE);
	bool high = gb->cpu.f & FLAG_Z;
	uint16_t music = high ? MUSIC_LEVEL_CLEAR_GOT_TREASURE : MUSIC_LEVEL_CLEAR_NO_TREASURE;
	uint16_t fs = high ? ADDR_Frameset_d4d18 : ADDR_Frameset_d4d58;
	wr8(gb, hMusicID + 0, (uint8_t)(music >> 8));
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicID + 1, gb->cpu.a);
	wr8(gb, wClearScreenWario + 6, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, wClearScreenWario + 7, gb->cpu.a);
	SET_HL(gb, wClearScreenWario + 7);
	// stop_sfx
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);

	init_menu_obj(gb, wMenuObj2, 0x28, 0x30, ADDR_Frameset_d5059);
	init_menu_obj(gb, wMenuObj3, 0x44, 0x28, ADDR_Frameset_d50a1);

	static const struct { int bit; uint16_t obj; uint8_t y, x; uint16_t fs; } keys[] = {
		{ GREY_KEY_F, wMenuObj4, 0x7c, 0x5c, ADDR_Frameset_d5095 },
		{ RED_KEY_F, wMenuObj5, 0x7c, 0x74, ADDR_Frameset_d5098 },
		{ GREEN_KEY_F, wMenuObj6, 0x8e, 0x5c, ADDR_Frameset_d509b },
		{ BLUE_KEY_F, wMenuObj7, 0x8e, 0x74, ADDR_Frameset_d509e },
	};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wKeyAndTreasureFlags);
		if (alu_bit(gb, gb->cpu.a, keys[i].bit))
			init_menu_obj(gb, keys[i].obj, keys[i].y, keys[i].x, keys[i].fs);
	}
}
