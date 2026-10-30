// Port of src/engine/pause/pause_menu_1.asm
#include "ported.h"

// ld hl, obj / y, x / xor a / frame 0 / attributes / frameset offset and
// duration 0 (attr 0 is the plain xor a / 4 zeros form); hl = obj + 6
static void pm_obj_head(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint8_t attr)
{
	wr8(gb, obj + 0, y);
	gb->cpu.a = x;
	wr8(gb, obj + 1, x);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, obj + 2, 0);
	if (attr) {
		gb->cpu.a = attr;
		wr8(gb, obj + 3, attr);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	} else {
		wr8(gb, obj + 3, 0);
	}
	wr8(gb, obj + 4, 0);
	wr8(gb, obj + 5, 0);
	SET_HL(gb, obj + 6);
}

// the frameset pointer (big-endian) at obj + 6, call UpdateObjAnim,
// ld hl, obj / call LoadPauseMenuSprite
static void pm_obj_tail(GB *gb, uint16_t obj, uint16_t fs)
{
	wr8(gb, obj + 6, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, obj + 7, gb->cpu.a);
	SET_HL(gb, obj + 7);
	CALL(UpdateObjAnim);
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadPauseMenuSprite);
}

// ld a, [wKeyAndTreasureFlags] / bit bit, a
static bool pm_flag(GB *gb, int bit)
{
	gb->cpu.a = rd8(gb, wKeyAndTreasureFlags);
	return alu_bit(gb, gb->cpu.a, bit);
}

static void pm_key(GB *gb, int bit, uint16_t obj, uint8_t x, uint8_t attr, uint16_t fs)
{
	if (!pm_flag(gb, bit))
		return;
	pm_obj_head(gb, obj, 0x54, x, attr);
	pm_obj_tail(gb, obj, fs);
}

static void pm_treasure(GB *gb, int bit, uint16_t obj, uint8_t x, uint8_t attr,
	uint16_t got, uint16_t not_got)
{
	pm_obj_head(gb, obj, 0x64, x, attr);
	pm_obj_tail(gb, obj, pm_flag(gb, bit) ? got : not_got);
}

PORT(InitPauseMenu_SkipBackupVRAM)
{
	// stop_sfx
	wr8(gb, hSFXID + 0, 0xff);
	wr8(gb, hSFXID + 1, 0x00);
	// play_music MUSIC_PAUSE_MENU
	wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_PAUSE_MENU >> 8));
	wr8(gb, hMusicID + 1, (uint8_t)MUSIC_PAUSE_MENU);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPauseMenuSelection, PAUSEMENU_RETURN);
	wr8(gb, wRoomAnimatedTilesEnabled, 0);
	CALL(ClearBGMap0);
	CALL(ClearVirtualOAM);

	FARCALL(LoadPauseMenuPals);
	FARCALL(LoadPauseMenuGfx);
	FARCALL(Func_1f403f);

	gb_call(gb, ADDR_PrintNumberMusicCoins);
	SET_HL(gb, v0BGMap1 + 15 * TILEMAP_WIDTH + 14); // hlbgcoord 14, 15, v0BGMap1
	FARCALL(PrintNumberCoins);
	CALL(VBlank_PauseMenu);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, wSCX, 0);

	pm_obj_head(gb, wMenuObj2, 0x38, 0x20, 0);
	pm_obj_tail(gb, wMenuObj2, ADDR_Frameset_1f1cb8);
	pm_obj_head(gb, wMenuObj3, 0x8c, 0x20, 0);
	pm_obj_tail(gb, wMenuObj3, ADDR_Frameset_1f1cd4);

	pm_key(gb, GREY_KEY_F, wMenuObj4, 0x38, 0x03, ADDR_Frameset_1f1c4a);
	pm_key(gb, RED_KEY_F, wMenuObj5, 0x48, 0x03, ADDR_Frameset_1f1c4d);
	pm_key(gb, GREEN_KEY_F, wMenuObj6, 0x58, 0x04, ADDR_Frameset_1f1c4a);
	pm_key(gb, BLUE_KEY_F, wMenuObj7, 0x68, 0x04, ADDR_Frameset_1f1c4d);

	pm_treasure(gb, GREY_TREASURE_F, wMenuObj8, 0x38, 0x03, ADDR_Frameset_1f1cf0, ADDR_Frameset_1f1ced);
	pm_treasure(gb, RED_TREASURE_F, wMenuObj9, 0x48, 0x03, ADDR_Frameset_1f1cf6, ADDR_Frameset_1f1cf3);
	pm_treasure(gb, GREEN_TREASURE_F, wMenuObj10, 0x58, 0x04, ADDR_Frameset_1f1cf0, ADDR_Frameset_1f1ced);
	pm_treasure(gb, BLUE_TREASURE_F, wMenuObj11, 0x68, 0x04, ADDR_Frameset_1f1cf6, ADDR_Frameset_1f1cf3);

	gb->cpu.a = LCDC_ON | LCDC_BG_9C00 | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT_JUMP(InitPauseMenu)
{
	CALL(DisableLCD);
	CALL(SaveBackupVRAM);
	gb->cpu.a = rd8(gb, wAnimatedTilesFrameDuration);
	wr8(gb, wTempAnimatedTilesFrameDuration, gb->cpu.a);
	gb->cpu.a = rd8(gb, wAnimatedTilesGfx);
	wr8(gb, wTempAnimatedTilesGroup, gb->cpu.a);
	JUMP_TO(ADDR_InitPauseMenu_SkipBackupVRAM); // fallthrough
}

PORT_JUMP(_ReturnToPauseMenuFromActionHelp)
{
	CALL(DisableLCD);
	gb->cpu.a = ST_PAUSE_MENU;
	wr8(gb, wState, ST_PAUSE_MENU);
	wr8(gb, wState + 1, SST_PAUSE_INIT_MENU);
	SET_HL(gb, wState + 1);
	JUMP_TO(ADDR_InitPauseMenu_SkipBackupVRAM);
}

static void pm_load_sprite(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadPauseMenuSprite);
}

PORT_JUMP(UpdatePauseMenu)
{
	gb_call(gb, ADDR_HandlePauseMenuInput);
	SET_HL(gb, wMenuObj3 + 7); // wMenuObj3FramesetPtr + 1
	CALL(UpdateObjAnim);
	pm_load_sprite(gb, wMenuObj3);
	if (pm_flag(gb, GREY_KEY_F))
		pm_load_sprite(gb, wMenuObj4);
	if (pm_flag(gb, RED_KEY_F))
		pm_load_sprite(gb, wMenuObj5);
	if (pm_flag(gb, GREEN_KEY_F))
		pm_load_sprite(gb, wMenuObj6);
	if (pm_flag(gb, BLUE_KEY_F))
		pm_load_sprite(gb, wMenuObj7);
	pm_load_sprite(gb, wMenuObj8);
	pm_load_sprite(gb, wMenuObj8End);
	pm_load_sprite(gb, wMenuObj10);
	pm_load_sprite(gb, wMenuObj11);
	SET_HL(gb, wMenuObj2 + 7); // wMenuObj2FramesetPtr + 1
	CALL(UpdateObjAnim);
	pm_load_sprite(gb, wMenuObj2);
	CALL(ClearUnusedVirtualOAM);

	gb->cpu.a = rd8(gb, wPauseMenuSelection);
	if (!alu_bit(gb, gb->cpu.a, PAUSEMENUF_B_PAD_SELECT)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wPauseMenuSelection), 0x0f);
	alu_cp(gb, gb->cpu.a, PAUSEMENU_TO_MAP);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(ReturnToMapFromLevel);
		return;
	}
	alu_cp(gb, gb->cpu.a, PAUSEMENU_SAVE);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = TRUE;
		wr8(gb, wResetDisabled, TRUE);
		gb->cpu.a = SST_PAUSE_MENU_SAVE;
		wr8(gb, wSubState, gb->cpu.a);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, PAUSEMENU_ACTION_HELP);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(OpenActionHelp);
		return;
	}
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
	RET();
}

PORT(ExitPauseMenu)
{
	CALL(DisableLCD);
	CALL(ClearVirtualOAM);
	gb_call(gb, ADDR_DrawLevelObjectsAfterLevelReturn);
	CALL(LoadBackupVRAM);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wUnused_IsPaused, 0);
	gb->cpu.a = rd8(gb, wTempAnimatedTilesFrameDuration);
	wr8(gb, wAnimatedTilesFrameDuration, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempAnimatedTilesGroup);
	wr8(gb, wAnimatedTilesGfx, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	gb->cpu.a = TRUE;
	wr8(gb, wRoomAnimatedTilesEnabled, TRUE);

	CALL(UpdateLevelMusic);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
