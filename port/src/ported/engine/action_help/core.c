// Port of src/engine/action_help/core.asm
#include "ported.h"

// ld hl, obj / y / x / xor a / 4x [hli] / frameset pointer (big-endian);
// hl ends on its low byte
static void init_obj(GB *gb, uint16_t hl, uint8_t y, uint8_t x, uint16_t frameset)
{
	wr8(gb, hl++, y);
	gb->cpu.a = x;
	wr8(gb, hl++, x);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(frameset >> 8));
	gb->cpu.a = (uint8_t)frameset;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

// hl = hlbgcoord 0, 16, v1BGMap1 / sla e / (ld d, $00) / add hl, de /
// ld e, TILEMAP_WIDTH / ld a, val / ld [hli], a / ld [hld], a / add hl, de /
// ld [hli], a / ld [hl], a
static void set_power_up_attrs(GB *gb, uint8_t val)
{
	uint16_t hl = v1BGMap1 + 16 * TILEMAP_WIDTH;
	gb->cpu.e = alu_sla(gb, gb->cpu.e);
	gb->cpu.d = 0x00;
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.e = TILEMAP_WIDTH;
	gb->cpu.a = val;
	wr8(gb, hl++, val);
	wr8(gb, hl--, val);
	hl = alu_add16(gb, hl, REG_DE(gb));
	wr8(gb, hl++, val);
	wr8(gb, hl, val);
	SET_HL(gb, hl);
}

PORT(InitActionHelp)
{
	CALL(DisableLCD);
	// play_music MUSIC_ACTION_HELP
	wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_ACTION_HELP >> 8));
	gb->cpu.a = (uint8_t)MUSIC_ACTION_HELP;
	wr8(gb, hMusicID + 1, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpState, 0);
	wr8(gb, wTimer, 0);

	gb->cpu.b = NO_POWER_UP;
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_TITLE_SCREEN);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, hDemoPowerUp);
		gb->cpu.b = gb->cpu.a;
	}
	// .not_title_screen
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_OWL_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = POWER_UP_OWL;
	} else {
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_RAIL);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = POWER_UP_RAIL;
		} else {
			alu_cp(gb, gb->cpu.a, TRANSFORMATION_VAMPIRE_WARIO);
			if (gb->cpu.f & FLAG_Z)
				gb->cpu.a = POWER_UP_VAMPIRE;
			else
				gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
		}
	}
	// .got_power_up
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, NUM_TOTAL_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C))
		gb->cpu.a = alu_sub(gb, gb->cpu.a, NUM_TOTAL_POWER_UPS);
	wr8(gb, wActionHelpPowerUp, gb->cpu.a);
	gb->cpu.a = alu_inc(gb, rd8(gb, hDemoPowerUp));
	alu_cp(gb, gb->cpu.a, NUM_TOTAL_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, hDemoPowerUp, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameDuration, 0);
	wr8(gb, wAnimatedTilesGfx, 0);
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	wr8(gb, wRoomAnimatedTilesEnabled, 0);

	CALL(ClearBGMap0);
	CALL(ClearVirtualOAM);
	FARCALL(LoadActionHelpCommonPals);
	FARCALL(LoadActionHelpPowerUpPals);
	FARCALL(LoadActionHelpGfx);
	FARCALL(LoadActionHelpBGMaps);
	gb_call(gb, ADDR_VBlank_1fb6b7);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	gb->cpu.a = 0x04;
	wr8(gb, rSCY, 0x04);
	wr8(gb, wSCY, 0x04);

	gb->cpu.a = alu_swap(gb, rd8(gb, wActionHelpPowerUp));
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x08);
	init_obj(gb, wMenuObj1, SCREEN_HEIGHT_PX, gb->cpu.a, ADDR_Frameset_1e6b8e);
	init_obj(gb, wMenuObj2, 0x77, 0x3f, ADDR_Frameset_1e6b76);
	init_obj(gb, wMenuObj3, 0x7d, 0x67, ADDR_Frameset_1e6b6d);

	SET_HL(gb, wMenuObj3FramesetPtr + 1);
	FARCALL_BANK(0x79, UpdateObjAnim);

	SET_HL(gb, wMenuObj3);
	FARCALL(LoadActionHelpSprite);

	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_TITLE_SCREEN);
	if (gb->cpu.f & FLAG_Z)
		goto finish;
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	alu_cp(gb, gb->cpu.a, NUM_MAIN_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C))
		goto finish;
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	gb->cpu.e = gb->cpu.a;
	set_power_up_attrs(gb, 0x03);

	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	if (gb->cpu.f & FLAG_C)
		goto finish;
	gb->cpu.d = 0x00;
	gb->cpu.b = rd8(gb, alu_add16(gb, ADDR_PowerUpOBPals, REG_DE(gb)));
	set_power_up_attrs(gb, gb->cpu.b);

finish:
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	CALL(ClearUnusedVirtualOAM);
	gb->cpu.a = LCDC_ON | LCDC_BG_9C00 | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
