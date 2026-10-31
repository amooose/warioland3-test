// Port of src/engine/action_help/common2.asm
#include "ported.h"

// hl = y, x, frame and attributes; y is relative to the background scroll
static uint16_t set_cur_sprite(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	wr8(gb, wCurSpriteYCoord, alu_add(gb, alu_sub(gb, rd8(gb, hl++), gb->cpu.b), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	return hl;
}

PORT(LoadActionHelpWarioSprite)
{
	set_cur_sprite(gb);
	gb->cpu.h = rd8(gb, wActionHelpWarioOAMPtr + 0);
	gb->cpu.l = rd8(gb, wActionHelpWarioOAMPtr + 1);
	FARCALL_BANK(0x05, LoadSprite);
}

// hl = menu object, whose OAM pointer and bank follow its frameset
PORT(LoadActionHelpSprite_Far)
{
	uint16_t hl = set_cur_sprite(gb) + 1;
	uint8_t l = (uint8_t)hl;
	for (int i = 0; i < 4; i++)
		l = alu_inc(gb, l); // inc l
	hl = (uint16_t)((hl & 0xff00) | l);
	gb->cpu.d = rd8(gb, hl++);
	gb->cpu.e = rd8(gb, hl++);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wTempBank, gb->cpu.a);
	SET_HL(gb, REG_DE(gb));
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(LoadSprite);
}

// Copies b palettes from hl to the palette data register c, one palette
// per HBlank (wait_not_hblank / wait_hblank).
static void copy_pals_hblank(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	do {
		while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		for (int i = 0; i < PAL_SIZE; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

// push hl / ld de, dst / ld b, n / call CopyHLToDE_Short / pop hl
static void copy_pals_keep_hl(GB *gb, uint16_t dst, uint8_t n)
{
	uint16_t hl = REG_HL(gb);
	gb_push16(gb, hl);
	SET_DE(gb, dst);
	gb->cpu.b = n;
	CALL(CopyHLToDE_Short);
	SET_HL(gb, gb_pop16(gb));
}

static void write_a_to_hl(GB *gb, uint16_t hl, uint8_t n)
{
	SET_HL(gb, hl);
	gb->cpu.b = n;
	CALL(WriteAToHL_BTimes);
}

// Returns whether the power-up being demonstrated is a main one; if so,
// leaves the carry of comparing it with Wario's power-up level.
static bool cp_power_up_level(GB *gb)
{
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	alu_cp(gb, gb->cpu.a, NUM_MAIN_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C))
		return false;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return true;
}

PORT(LoadActionHelpOBPals)
{
	SET_HL(gb, ADDR_Pals_1e4f2f);
	if (!cp_power_up_level(gb))
		return; // special Power-Up
	if (gb->cpu.f & FLAG_C) {
		// doesn't have Power-Up, so black out some OB pals
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		write_a_to_hl(gb, wTempPals2, 2 * PAL_SIZE);
		write_a_to_hl(gb, wTempPals2 + 3 * PAL_SIZE, 5 * PAL_SIZE);
		SET_HL(gb, wTempPals2);
	} else {
		copy_pals_keep_hl(gb, wTempPals2, 8 * PAL_SIZE);
	}
	gb->cpu.a = OBPI_AUTOINC | 0;
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.b = 8;
	gb->cpu.c = (uint8_t)rOBPD;
	copy_pals_hblank(gb);
}

PORT(Func_1e4a47)
{
	SET_HL(gb, ADDR_Pals_1e4f2f + 4 * PAL_SIZE);
	if (!cp_power_up_level(gb))
		return;
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		write_a_to_hl(gb, wTempPals2 + 4 * PAL_SIZE, PAL_SIZE);
		write_a_to_hl(gb, wTempPals2 + 7 * PAL_SIZE, PAL_SIZE);
		SET_HL(gb, wTempPals2 + 4 * PAL_SIZE);
	} else {
		uint16_t hl = REG_HL(gb);
		gb_push16(gb, hl);
		SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
		gb->cpu.b = PAL_SIZE;
		CALL(CopyHLToDE_Short);
		SET_HL(gb, ADDR_Pals_1e4f2f + 7 * PAL_SIZE);
		SET_DE(gb, wTempPals2 + 7 * PAL_SIZE);
		gb->cpu.b = PAL_SIZE;
		CALL(CopyHLToDE_Short);
		SET_HL(gb, gb_pop16(gb));
	}
	gb->cpu.a = OBPI_AUTOINC | (4 * PAL_SIZE);
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.c = (uint8_t)rOBPD;
	gb->cpu.b = 4;
	copy_pals_hblank(gb);
}

// hl = 8 BG palettes: copies them to wTempPals1 and to the BG palette RAM
PORT(LoadActionHelpBGPals)
{
	copy_pals_keep_hl(gb, wTempPals1, 8 * PAL_SIZE);
	gb->cpu.a = BGPI_AUTOINC | 0;
	wr8(gb, rBGPI, gb->cpu.a);
	gb->cpu.b = 8;
	gb->cpu.c = (uint8_t)rBGPD;
	copy_pals_hblank(gb);
}

PORT(LoadActionHelpPalsWithWater)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpBlackedOut, 0);
	SET_HL(gb, ADDR_Pals_1e4e6f);
	gb_call(gb, ADDR_LoadActionHelpBGPals);
	gb_call(gb, ADDR_LoadActionHelpOBPals);
}

PORT(LoadActionHelpPalsWithoutWater)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpBlackedOut, 0);
	SET_HL(gb, ADDR_Pals_1e4eaf);
	gb_call(gb, ADDR_LoadActionHelpBGPals);
	gb_call(gb, ADDR_LoadActionHelpOBPals);
}

PORT(Func_1e48bc)
{
	gb->cpu.a = TRUE;
	wr8(gb, wActionHelpBlackedOut, TRUE);
	SET_HL(gb, ADDR_Pals_1e4eef);
	gb_call(gb, ADDR_LoadActionHelpBGPals);
}

PORT(LoadActionHelpWarioSprite_FromBank)
{
	set_cur_sprite(gb);
	gb->cpu.h = rd8(gb, wActionHelpWarioOAMPtr + 0);
	gb->cpu.l = rd8(gb, wActionHelpWarioOAMPtr + 1);
	gb->cpu.a = rd8(gb, wActionHelpWarioOAMBank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(LoadSprite);
}

PORT(Func_1e4b6d)
{
	set_cur_sprite(gb);
	SET_HL(gb, ADDR_OAM_1e6bab);
	CALL(LoadSprite);
}

PORT(ActionHelp2_Init) { FARCALL(ActionHelp_Init); }
PORT(ActionHelp2_FlashBInput) { FARCALL(ActionHelp_FlashBInput); }
PORT(ActionHelp2_FlashAInput) { FARCALL(ActionHelp_FlashAInput); }

// xor a / ld hl, ptr / ld [hli], a x2 / ld a, HIGH(fs) / ld [hli], a /
// ld a, LOW(fs) / ld [hl], a  (the order of xor a and ld hl doesn't matter)
// Returns hl (pointing at the frameset's low byte).
static uint16_t set_fs(GB *gb, uint16_t hl, uint16_t fs)
{
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

PORT(ActionHelp2_ClearDPadInput) { set_fs(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b76); }
PORT(ActionHelp2_ShowDLeftInput) { set_fs(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b82); }
PORT(ActionHelp2_ShowDRightInput) { set_fs(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b7c); }
PORT(ActionHelp2_ShowDUpInput) { set_fs(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b79); }
PORT(ActionHelp2_ClearButtonsInput) { set_fs(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b6d); }
PORT(ActionHelp2_ShowAInput) { set_fs(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b70); }
PORT(ActionHelp2_ShowBInput) { set_fs(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b73); }
PORT(ActionHelp_ShowWarioHangVertical) { set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fedd0); }
PORT(ActionHelp_ShowWarioHangHorizontal) { set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fed7c); }
PORT(ActionHelp_ShowWarioHangOwlRight) { set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fed91); }
PORT(ActionHelp_ShowWarioOwlMoveHang) { set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1feda6); }
PORT(ActionHelp_ShowWarioOwlMoveHangRight) { set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fedbb); }
PORT(ActionHelp_ShowWarioHangDrop) { set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fedfe); }
PORT(ActionHelp_ShowRailRotateRight) { SET_HL(gb, set_fs(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6fcb) + 1); }
PORT(ActionHelp_ShowRailRotateLeft) { SET_HL(gb, set_fs(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6fc2) + 1); }

// Queues a DMA of $80 tiles of Wario graphics into v0Tiles0.
static void queue_gfx(GB *gb, uint8_t bank, uint16_t gfx)
{
	wr8(gb, wPendingDMASourceBank, bank);
	wr8(gb, wPendingDMASourcePtr + 0, (uint8_t)(gfx >> 8));
	wr8(gb, wPendingDMASourcePtr + 1, (uint8_t)gfx);
	wr8(gb, wPendingDMADestinationPtr + 0, (uint8_t)(v0Tiles0 >> 8));
	wr8(gb, wPendingDMADestinationPtr + 1, (uint8_t)v0Tiles0);
	wr8(gb, wPendingDMADestinationBank, BANK_v0Tiles0);
	wr8(gb, wPendingDMALength, 0x80 - 1);
	gb->cpu.a = TRUE;
	wr8(gb, wIsDMATransferPending, TRUE);
}

// sets wActionHelpWarioOAMPtr (and the bank when bank >= 0); a = last byte
static void set_oam(GB *gb, uint16_t oam, int bank)
{
	wr8(gb, wActionHelpWarioOAMPtr + 0, (uint8_t)(oam >> 8));
	gb->cpu.a = (uint8_t)oam;
	wr8(gb, wActionHelpWarioOAMPtr + 1, gb->cpu.a);
	if (bank >= 0) {
		gb->cpu.a = (uint8_t)bank;
		wr8(gb, wActionHelpWarioOAMBank, gb->cpu.a);
	}
}

PORT(ActionHelp2_ShowWarioIdle)
{
	queue_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1425f);
	set_oam(gb, ADDR_OAM_14000, BANK_OAM_14000);
}

PORT(ActionHelp2_ShowWarioWalk)
{
	queue_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	SET_HL(gb, set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_149c5) + 1);
	set_oam(gb, ADDR_OAM_1426c, -1);
}

// ld [hli], a (low byte) / xor a / ld [hl], a
static void show_with_var(GB *gb, uint8_t bank, uint16_t gfx, uint16_t fs, uint16_t oam, uint8_t oam_bank)
{
	queue_gfx(gb, bank, gfx);
	uint16_t hl = set_fs(gb, wActionHelpWarioFramesetOffset, fs) + 1;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	set_oam(gb, oam, oam_bank);
}

PORT(ActionHelp2_ShowWarioJump)
{
	show_with_var(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx, ADDR_Frameset_15f97, ADDR_OAM_15955, BANK_OAM_15955);
}

PORT(ActionHelp_ShowWarioHangStopped)
{
	show_with_var(gb, BANK_WarioHangGfx, ADDR_WarioHangGfx, ADDR_Frameset_1feec4, ADDR_OAM_1fee05, BANK_OAM_1fee05);
}

PORT(ActionHelp_ShowWarioHangRailRight)
{
	show_with_var(gb, BANK_WarioHangGfx, ADDR_WarioHangGfx, ADDR_Frameset_1feeb4, ADDR_OAM_1fee05, BANK_OAM_1fee05);
}

PORT(ActionHelp_ShowWarioBatTransform)
{
	queue_gfx(gb, BANK_WarioBatGfx, ADDR_WarioBatGfx);
	set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fd479);
	set_oam(gb, ADDR_OAM_1fd031, -1);
}

// ld hl, wActionHelpWarioFramesetOffset / xor a / ld [hli], a x2 /
// ld [wSFXTimer], a / frameset
static void show_bat(GB *gb, uint16_t fs)
{
	wr8(gb, wSFXTimer, 0);
	set_fs(gb, wActionHelpWarioFramesetOffset, fs);
}

PORT(ActionHelp_ShowWarioBatRight) { show_bat(gb, ADDR_Frameset_1fd46c); }
PORT(ActionHelp_ShowWarioBatLeft) { show_bat(gb, ADDR_Frameset_1fd42c); }

PORT(ActionHelp_ShowWarioVampireTransform)
{
	play_sfx(gb, SFX_045);
	set_fs(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1fd439);
}

PORT(ActionHelp_ShowOwlFlyUp)
{
	uint16_t hl = wMenuObj4;
	wr8(gb, hl++, alu_sub(gb, rd8(gb, wActionHelpWarioYCoord), 0x20));
	wr8(gb, hl++, alu_sub(gb, rd8(gb, wActionHelpWarioXCoord), 0x08));
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(ADDR_Frameset_d7b4 >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_Frameset_d7b4);
	wr8(gb, hl++, (uint8_t)(ADDR_OAM_d763 >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_OAM_d763);
	wr8(gb, hl++, BANK_OAM_d763);
	wr8(gb, hl, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, hl);
}
