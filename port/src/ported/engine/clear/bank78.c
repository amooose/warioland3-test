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
