// Port of src/engine/pause/pause_menu_2.asm
#include "ported.h"

// hl = y, x, frame and attributes of a pause menu sprite
PORT(LoadPauseMenuSprite)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, wCurSpriteYCoord, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	SET_HL(gb, ADDR_OAM_1f156c);
	CALL(LoadSprite);
}

PORT(PrintNumberMusicCoins)
{
	gb->cpu.a = rd8(gb, wNumMusicalCoins);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
	wr8(gb, v0BGMap1 + 15 * TILEMAP_WIDTH + 7, gb->cpu.a);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, v0BGMap1 + 16 * TILEMAP_WIDTH + 7, gb->cpu.a);
}

// --- HandlePauseMenuInput ---

#define PM_ZF (gb->cpu.f & FLAG_Z)

// ld bc, frameset by wTransformation (owl, rail, vampire or else)
static void pm_transformation_frameset(GB *gb, uint16_t owl, uint16_t rail,
	uint16_t vampire, uint16_t other)
{
	gb->cpu.a = rd8(gb, wTransformation);
	uint16_t bc = other;
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_OWL_WARIO);
	if (PM_ZF) {
		bc = owl;
	} else {
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_RAIL);
		if (PM_ZF) {
			bc = rail;
		} else {
			alu_cp(gb, gb->cpu.a, TRANSFORMATION_VAMPIRE_WARIO);
			if (PM_ZF)
				bc = vampire;
		}
	}
	SET_BC(gb, bc);
}

// the frameset pointer (big-endian) at hl, ending on ld [hl], a
static void pm_frameset(GB *gb, uint16_t hl, uint16_t fs)
{
	wr8(gb, hl, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, hl + 1, gb->cpu.a);
	SET_HL(gb, hl + 1);
}

// ld a, sel / ld [wPauseMenuSelection], a / ld hl, wMenuObj2 / y, x /
// xor a / 4 zeros; the frameset is up to the caller (hl = wMenuObj2 + 6)
static void pm_highlight(GB *gb, uint8_t sel, uint8_t y, uint8_t x)
{
	gb->cpu.a = sel;
	wr8(gb, wPauseMenuSelection, sel);
	wr8(gb, wMenuObj2 + 0, y);
	wr8(gb, wMenuObj2 + 1, x);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 2; i < 6; i++)
		wr8(gb, wMenuObj2 + i, 0);
}

static void pm_highlight_save(GB *gb)
{
	pm_highlight(gb, PAUSEMENU_SAVE, 0x40, 0x40);
	pm_frameset(gb, wMenuObj2 + 6, ADDR_Frameset_1f1c9c);
	play_sfx(gb, SFX_0E2);
}

static void pm_highlight_action_help(GB *gb)
{
	pm_highlight(gb, PAUSEMENU_ACTION_HELP, 0x38, 0x60);
	pm_transformation_frameset(gb, ADDR_Frameset_1f1cf9, ADDR_Frameset_1f1d1b,
		ADDR_Frameset_1f1d41, ADDR_Frameset_1f1c50);
	pm_frameset(gb, wMenuObj2 + 6, REG_BC(gb));
	play_sfx(gb, SFX_0E2);
}

static void pm_highlight_to_map(GB *gb)
{
	pm_highlight(gb, PAUSEMENU_TO_MAP, 0x40, 0x80);
	pm_frameset(gb, wMenuObj2 + 6, ADDR_Frameset_1f1c74);
	play_sfx(gb, SFX_0E2);
}

static void pm_highlight_return(GB *gb)
{
	pm_highlight(gb, PAUSEMENU_RETURN, 0x38, 0x20);
	pm_frameset(gb, wMenuObj2 + 6, ADDR_Frameset_1f1cb8);
	play_sfx(gb, SFX_0E2);
}

// ld hl, wPauseMenuSelection / set PAUSEMENUF_B_PAD_SELECT, [hl] / play_sfx
static void pm_select(GB *gb, uint16_t sfx)
{
	SET_HL(gb, wPauseMenuSelection);
	wr8(gb, wPauseMenuSelection,
		(uint8_t)(rd8(gb, wPauseMenuSelection) | (1 << PAUSEMENUF_B_PAD_SELECT)));
	play_sfx(gb, sfx);
}

static void pm_return(GB *gb)
{
	pm_highlight(gb, PAUSEMENU_RETURN, 0x38, 0x20);
	pm_frameset(gb, wMenuObj2 + 6, ADDR_Frameset_1f1cbb);
	pm_select(gb, SFX_0E7);
}

// ld hl, wMenuObj2FramesetOffset / xor a / ld [hli], a / ld [hli], a
static void pm_restart_anim(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMenuObj2FramesetOffset + 0, 0);
	wr8(gb, wMenuObj2FramesetOffset + 1, 0);
}

// ld a, [wGameModeFlags] / bit MODE_TIME_ATTACK_F, a
static bool pm_time_attack(GB *gb)
{
	gb->cpu.a = rd8(gb, wGameModeFlags);
	return alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F);
}

PORT(HandlePauseMenuInput)
{
	gb->cpu.a = rd8(gb, wPauseMenuSelection);
	if (alu_bit(gb, gb->cpu.a, PAUSEMENUF_B_PAD_SELECT))
		return;
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		pm_return(gb);
		return;
	}

	gb->cpu.a = rd8(gb, wPauseMenuSelection);
	alu_cp(gb, gb->cpu.a, PAUSEMENU_SAVE);
	if (PM_ZF) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
			pm_highlight_return(gb);
		} else if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
			pm_highlight_action_help(gb);
		} else if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
			// .Save
			pm_restart_anim(gb);
			pm_frameset(gb, wMenuObj2FramesetOffset + 2, ADDR_Frameset_1f1c9f);
			pm_select(gb, SFX_SELECTION);
		}
		return;
	}
	alu_cp(gb, gb->cpu.a, PAUSEMENU_ACTION_HELP);
	if (PM_ZF) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
			if (pm_time_attack(gb))
				pm_highlight_return(gb);
			else
				pm_highlight_save(gb);
		} else if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
			pm_highlight_to_map(gb);
		} else if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
			// .ActionHelp
			pm_restart_anim(gb);
			pm_transformation_frameset(gb, ADDR_Frameset_1f1d0a, ADDR_Frameset_1f1d20,
				ADDR_Frameset_1f1d4c, ADDR_Frameset_1f1c69);
			pm_frameset(gb, wMenuObj2FramesetOffset + 2, REG_BC(gb));
			pm_select(gb, SFX_SELECTION);
		}
		return;
	}
	alu_cp(gb, gb->cpu.a, PAUSEMENU_TO_MAP);
	if (PM_ZF) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
			pm_highlight_action_help(gb);
		} else if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
			// .ToMap
			pm_restart_anim(gb);
			pm_frameset(gb, wMenuObj2FramesetOffset + 2, ADDR_Frameset_1f1c77);
			pm_select(gb, SFX_SELECTION);
		}
		return;
	}

	// PAUSEMENU_RETURN
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		if (pm_time_attack(gb))
			pm_highlight_action_help(gb);
		else
			pm_highlight_save(gb);
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	pm_return(gb);
}
