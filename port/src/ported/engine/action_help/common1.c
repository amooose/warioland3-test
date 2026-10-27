// Port of src/engine/action_help/common1.asm
#include "ported.h"

// Frameset and OAM pointers are stored big-endian.
#define HI(addr) ((uint8_t)((addr) >> 8))
#define LO(addr) ((uint8_t)(addr))

// xor a / ld [hli], a / ld [hli], a / ld a, HIGH(frameset) / ld [hli], a /
// ld a, LOW(frameset) / ld [hl], a
// Resets a menu object's animation and sets its frameset. Returns the
// address of the frameset's low byte.
static uint16_t set_frameset(GB *gb, uint16_t hl, uint16_t frameset)
{
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);   // frameset offset
	wr8(gb, hl++, 0);   // duration
	wr8(gb, hl++, HI(frameset));
	gb->cpu.a = LO(frameset);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

// Queues a DMA of $80 tiles of Wario graphics into v0Tiles0.
static void queue_wario_gfx(GB *gb, uint8_t bank, uint16_t gfx)
{
	wr8(gb, wPendingDMASourceBank, bank);
	wr8(gb, wPendingDMASourcePtr + 0, HI(gfx));
	wr8(gb, wPendingDMASourcePtr + 1, LO(gfx));
	wr8(gb, wPendingDMADestinationPtr + 0, HI(v0Tiles0));
	wr8(gb, wPendingDMADestinationPtr + 1, LO(v0Tiles0));
	wr8(gb, wPendingDMADestinationBank, BANK_v0Tiles0);
	wr8(gb, wPendingDMALength, 0x80 - 1); // $80 dma_tiles
	gb->cpu.a = TRUE;
	wr8(gb, wIsDMATransferPending, TRUE);
}

static void set_wario_oam(GB *gb, uint16_t oam)
{
	wr8(gb, wActionHelpWarioOAMPtr + 0, HI(oam));
	gb->cpu.a = LO(oam);
	wr8(gb, wActionHelpWarioOAMPtr + 1, gb->cpu.a);
}

// Sets up a menu object: y, x, cleared animation state, frameset and
// sprite (OAM pointer and bank).
static void init_menu_obj(GB *gb, uint16_t hl, uint8_t y, uint8_t x, uint16_t frameset,
	uint16_t oam, uint8_t oam_bank)
{
	wr8(gb, hl++, y);
	wr8(gb, hl++, x);
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, HI(frameset));
	wr8(gb, hl++, LO(frameset));
	wr8(gb, hl++, HI(oam));
	wr8(gb, hl++, LO(oam));
	wr8(gb, hl++, oam_bank);
	wr8(gb, hl, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, hl);
}

// The d-pad and button prompts.
PORT(ActionHelp_ClearDPadInput) { set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b76); }
PORT(ActionHelp_ShowDRightInput) { set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b7c); }
PORT(ActionHelp_ShowDLeftInput) { set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b82); }
PORT(ActionHelp_ShowDDownInput) { set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b7f); }
PORT(ActionHelp_ShowDUpInput) { set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b79); }
PORT(ActionHelp_ShowDRightAndDDownInput) { set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6ba8); }
PORT(ActionHelp_ClearButtonsInput) { set_frameset(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b6d); }
PORT(ActionHelp_ShowAInput) { set_frameset(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b70); }
PORT(ActionHelp_ShowBInput) { set_frameset(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b73); }

PORT(ActionHelp_HideObj5BlockDebris)
{
	init_menu_obj(gb, wMenuObj5, 0xa0, 0xe8, ADDR_Frameset_1e6b64, ADDR_OAM_1e697c, BANK_OAM_1e697c);
}

// hl = menu object
PORT(ActionHelp_HideBlockDebris)
{
	init_menu_obj(gb, REG_HL(gb), 0xa0, 0xe8, ADDR_Frameset_1e6b64, ADDR_OAM_1e697c, BANK_OAM_1e697c);
}

// hl = menu object
// b = y offset of debris
// c = x offset of debris
static void show_block_debris(GB *gb)
{
	play_sfx(gb, SFX_BLOCK_BREAK);
	// Each coordinate is read just before it's written, as in the asm: hl
	// may point at wMenuObj4 itself.
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, alu_sub(gb, rd8(gb, wMenuObj4YCoord), gb->cpu.b));
	wr8(gb, hl++, alu_sub(gb, rd8(gb, wMenuObj4XCoord), gb->cpu.c));
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, HI(ADDR_Frameset_d49a));
	wr8(gb, hl++, LO(ADDR_Frameset_d49a));
	wr8(gb, hl++, HI(ADDR_OAM_d20e));
	wr8(gb, hl++, LO(ADDR_OAM_d20e));
	wr8(gb, hl++, BANK_OAM_d20e);
	wr8(gb, hl, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, hl);
}

// b = y offset of debris
PORT(ActionHelp_ShowBlockDebrisWithYOffset)
{
	SET_HL(gb, wMenuObj5YCoord);
	gb->cpu.c = 0;
	show_block_debris(gb);
}

PORT(ActionHelp_ShowBlockDebris)
{
	show_block_debris(gb);
}

PORT(ActionHelp_ShowInitialWarioWalk)
{
	queue_wario_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	uint16_t hl = wActionHelpWario;
	wr8(gb, hl++, 0x40); // y
	wr8(gb, hl++, 0x40); // x
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, HI(ADDR_Frameset_149c5));
	wr8(gb, hl++, LO(ADDR_Frameset_149c5));
	wr8(gb, hl, 0); // wActionHelpWarioVar
	SET_HL(gb, hl);
	wr8(gb, wSFXTimer, 0);
	gb->cpu.f = FLAG_Z; // xor a
	set_wario_oam(gb, ADDR_OAM_1426c);
}

PORT(ActionHelp_ShowWarioJump)
{
	play_sfx(gb, SFX_JUMP);
	queue_wario_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	uint16_t hl = set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_15f97) + 1;
	wr8(gb, hl, 0); // wActionHelpWarioVar
	SET_HL(gb, hl);
	set_wario_oam(gb, ADDR_OAM_15955);
}

PORT(ActionHelp_ShowWarioIdle)
{
	queue_wario_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1425f);
	set_wario_oam(gb, ADDR_OAM_14000);
}

PORT(ActionHelp_ShowWarioWalk)
{
	queue_wario_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	SET_HL(gb, set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_149c5) + 1);
	set_wario_oam(gb, ADDR_OAM_1426c);
}

PORT(ActionHelp_ShowWarioAttack)
{
	queue_wario_gfx(gb, BANK_WarioAttackGfx, ADDR_WarioAttackGfx);
	wr8(gb, wSFXTimer, 0);
	uint16_t hl = wActionHelpWarioFramesetOffset;
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	// with the garlic power-up, Wario's attack is stronger
	alu_cp(gb, rd8(gb, wActionHelpPowerUp), GARLIC);
	uint16_t frameset = (gb->cpu.f & FLAG_C) ? ADDR_Frameset_151fd : ADDR_Frameset_1522d;
	wr8(gb, hl++, HI(frameset));
	wr8(gb, hl++, LO(frameset));
	SET_HL(gb, hl);
	set_wario_oam(gb, ADDR_OAM_14d1b);
}

PORT(ActionHelp_ShowParaGoomWalk)
{
	init_menu_obj(gb, wMenuObj4, 0x40, 0xe8, ADDR_Frameset_1e6f36, ADDR_OAM_1e6bab, BANK_OAM_1e6bab);
}

PORT(ActionHelp_ShowParaGoomStun)
{
	SET_HL(gb, set_frameset(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6f48) + 1);
}

PORT(ActionHelp_ShowSingleSolidBlock)
{
	SET_HL(gb, set_frameset(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6b67) + 1);
}

PORT(ActionHelp_FlashBInput)
{
	gb->cpu.a = TRUE;
	wr8(gb, wActionHelpFrozen, TRUE);
	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x03);
	if (gb->cpu.a)
		return;
	SET_HL(gb, wTimer);
	uint8_t timer = alu_inc(gb, rd8(gb, wTimer));
	wr8(gb, wTimer, timer);
	if (!alu_bit(gb, timer, 0)) {
		set_frameset(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b6d); // ActionHelp_ClearButtonsInput
		return;
	}
	play_sfx(gb, SFX_0EC);
	set_frameset(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b73); // ActionHelp_ShowBInput
	gb->cpu.a = rd8(gb, wTimer);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
	wr8(gb, wActionHelpFrozen, 0);
	SET_HL(gb, wActionHelpState);
	wr8(gb, wActionHelpState, alu_inc(gb, rd8(gb, wActionHelpState)));
}

PORT(VBlank_1fb6b7)
{
	SET_HL(gb, ADDR_VBlank_1fb6b7 + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x34; // .end - .Func
	CALL(CopyHLToDE);
}

// Flashes an input prompt: alternately clears it and shows it, every 4
// frames, then moves on to the next state after 7 flashes.
static void flash_input(GB *gb, uint16_t obj, uint16_t clear_fs, uint16_t show_fs)
{
	gb->cpu.a = TRUE;
	wr8(gb, wActionHelpFrozen, TRUE);
	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x03);
	if (gb->cpu.a)
		return;
	SET_HL(gb, wTimer);
	uint8_t timer = alu_inc(gb, rd8(gb, wTimer));
	wr8(gb, wTimer, timer);
	if (!alu_bit(gb, timer, 0)) {
		set_frameset(gb, obj, clear_fs);
		return;
	}
	play_sfx(gb, SFX_0EC);
	set_frameset(gb, obj, show_fs);
	gb->cpu.a = rd8(gb, wTimer);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
	wr8(gb, wActionHelpFrozen, 0);
	SET_HL(gb, wActionHelpState);
	wr8(gb, wActionHelpState, alu_inc(gb, rd8(gb, wActionHelpState)));
}

PORT(ActionHelp_FlashDDownInput)
{
	flash_input(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b76, ADDR_Frameset_1e6b7f);
}

PORT(ActionHelp_FlashDUpInput)
{
	flash_input(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b76, ADDR_Frameset_1e6b79);
}

PORT(ActionHelp_FlashAInput)
{
	flash_input(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b6d, ADDR_Frameset_1e6b70);
}

PORT(ActionHelp_Init)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameDuration, 0);
	wr8(gb, wAnimatedTilesGfx, 0);
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	wr8(gb, wActionHelpFrozen, 0);
	wr8(gb, wRoomAnimatedTilesEnabled, 0);
	gb->cpu.a = 0x04;
	wr8(gb, wSCY, 0x04);
	FARCALL(Func_1e48bc);
	FARCALL(Func_1e4a47);
	set_frameset(gb, wMenuObj2FramesetOffset, ADDR_Frameset_1e6b76); // ActionHelp_ClearDPadInput
	set_frameset(gb, wMenuObj3FramesetOffset, ADDR_Frameset_1e6b6d); // ActionHelp_ClearButtonsInput
	gb->cpu.a = 0x60;
	wr8(gb, wTimer, 0x60);
	SET_HL(gb, wActionHelpState);
	wr8(gb, wActionHelpState, alu_inc(gb, rd8(gb, wActionHelpState)));
}

// writes a 2x2 block of a at hl; de = TILEMAP_WIDTH
static void fill_2x2(GB *gb, uint16_t hl, uint8_t a)
{
	gb->cpu.a = a;
	wr8(gb, hl++, a);
	wr8(gb, hl--, a);
	hl = alu_add16(gb, hl, REG_DE(gb));
	wr8(gb, hl++, a);
	wr8(gb, hl, a);
	SET_HL(gb, hl);
}

// Moves the power-up selection with the d-pad.
PORT(Func_1fb6f7)
{
	uint16_t hl = wActionHelpPowerUp;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, NUM_MAIN_POWER_UPS - 1);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.e = gb->cpu.a;
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wActionHelpState, 0);
		gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj1XCoord), 0x10);
		wr8(gb, wMenuObj1XCoord, gb->cpu.a);
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		gb->cpu.a = rd8(gb, hl);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!gb->cpu.a)
			return;
		gb->cpu.e = gb->cpu.a;
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wActionHelpState, 0);
		gb->cpu.a = alu_sub(gb, rd8(gb, wMenuObj1XCoord), 0x10);
		wr8(gb, wMenuObj1XCoord, gb->cpu.a);
	} else {
		return;
	}
	gb->cpu.a = TRUE;
	wr8(gb, wActionHelpBlackedOut, TRUE);
	gb->cpu.a = BANK_v1BGMap1; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.e = alu_sla(gb, gb->cpu.e);
		gb->cpu.d = 0;
		SET_HL(gb, alu_add16(gb, v1BGMap1 + 16 * TILEMAP_WIDTH, REG_DE(gb)));
		gb->cpu.e = TILEMAP_WIDTH;
		CALL(WaitNextHBlank);
		fill_2x2(gb, REG_HL(gb), 0x03);
	}
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.d = 0;
		hl = alu_add16(gb, ADDR_PowerUpOBPals, REG_DE(gb));
		gb->cpu.b = rd8(gb, hl);
		gb->cpu.e = alu_sla(gb, gb->cpu.e);
		SET_HL(gb, alu_add16(gb, v1BGMap1 + 16 * TILEMAP_WIDTH, REG_DE(gb)));
		gb->cpu.e = TILEMAP_WIDTH;
		CALL(WaitNextHBlank);
		fill_2x2(gb, REG_HL(gb), gb->cpu.b);
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	play_sfx(gb, SFX_0E2);
}

// ld hl, obj / xor a / ld [hli], a x2 / frameset, the low byte with
// ld [hli], a
static void fs_hli(GB *gb, uint16_t obj, uint16_t fs)
{
	SET_HL(gb, set_frameset(gb, obj, fs) + 1);
}

// ... / xor a / ld [wSFXTimer], a
static void fs_hli_sfx_timer(GB *gb, uint16_t fs)
{
	fs_hli(gb, wActionHelpWarioFramesetOffset, fs);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSFXTimer, 0);
}

PORT(ActionHelp_ShowWarioJumpLeft)
{
	play_sfx(gb, SFX_JUMP);
	queue_wario_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	uint16_t hl = set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_15f94) + 1;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	set_wario_oam(gb, ADDR_OAM_15955);
}

PORT(ActionHelp_ShowWarioSmashAttack)
{
	uint16_t hl = wActionHelpWarioFramesetOffset;
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	alu_cp(gb, gb->cpu.a, SUPER_JUMP_SLAM_OVERALLS);
	uint16_t fs = (gb->cpu.f & FLAG_C) ? ADDR_Frameset_15f91 : ADDR_Frameset_16055;
	wr8(gb, hl++, HI(fs));
	wr8(gb, hl++, LO(fs));
	gb->cpu.a = 0x18;
	wr8(gb, hl, 0x18);
	SET_HL(gb, hl);
}

PORT(ActionHelp_ShowWarioGroundPound)
{
	play_sfx(gb, SFX_GROUND_POUND);
	set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_16009);
}

PORT(ActionHelp_ShowWarioGroundShake)
{
	play_sfx(gb, SFX_GROUND_SHAKE);
	set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_15fc5);
}

PORT(ActionHelp_ShowWarioLand)
{
	play_sfx(gb, SFX_LAND);
	set_frameset(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_15fa1);
}

PORT(ActionHelp_ShowWarioSwim)
{
	play_sfx(gb, SFX_SPLASH);
	queue_wario_gfx(gb, BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1555b);
	set_wario_oam(gb, ADDR_OAM_15254);
}

PORT(ActionHelp_ShowWarioSwimLeft) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_15554); }
PORT(ActionHelp_ShowWarioSubmerge) { fs_hli_sfx_timer(gb, ADDR_Frameset_1558b); }
PORT(ActionHelp_ShowWarioUnderwaterThrust) { fs_hli_sfx_timer(gb, ADDR_Frameset_155c5); }
PORT(ActionHelp_ShowWarioUnderwaterStopped) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1556c); }

// (play_sfx) / queue Wario gfx / frameset / OAM pointer
static void show_wario_gfx(GB *gb, uint16_t sfx, uint8_t bank, uint16_t gfx, uint16_t fs, uint16_t oam)
{
	if (sfx)
		play_sfx(gb, sfx);
	queue_wario_gfx(gb, bank, gfx);
	fs_hli(gb, wActionHelpWarioFramesetOffset, fs);
	set_wario_oam(gb, oam);
}

PORT(ActionHelp_ShowWarioGrab)
{
	show_wario_gfx(gb, SFX_GRAB, BANK_WarioThrowGfx, ADDR_WarioThrowGfx, ADDR_Frameset_1640c, ADDR_OAM_1606a);
}

PORT(ActionHelp_ShowWarioSlowGrab)
{
	show_wario_gfx(gb, SFX_GRAB, BANK_WarioThrowGfx, ADDR_WarioThrowGfx, ADDR_Frameset_16415, ADDR_OAM_1606a);
}

PORT(ActionHelp_ShowWarioBeginGrab)
{
	show_wario_gfx(gb, 0, BANK_WarioWalkGfx, ADDR_WarioWalkGfx, ADDR_Frameset_14a23, ADDR_OAM_1426c);
}

PORT(ActionHelp_ShowWarioBeginThrow)
{
	show_wario_gfx(gb, SFX_02C, BANK_WarioThrowGfx, ADDR_WarioThrowGfx, ADDR_Frameset_1642f, ADDR_OAM_1606a);
}

PORT(ActionHelp_ShowWarioBump)
{
	show_wario_gfx(gb, SFX_BUMP, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx, ADDR_Frameset_15f7f, ADDR_OAM_15955);
}

PORT(ActionHelp_ShowWarioSlide)
{
	show_wario_gfx(gb, 0, BANK_WarioSlideGfx, ADDR_WarioSlideGfx, ADDR_Frameset_1671d, ADDR_OAM_1644a);
}

PORT(ActionHelp_ShowWarioGrabWalk) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_14a0f); }
PORT(ActionHelp_ShowWarioThrow) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_1641e); }
PORT(ActionHelp_ShowWarioChargeThrow) { fs_hli_sfx_timer(gb, ADDR_Frameset_16435); }
PORT(ActionHelp_ShowWarioFullyChargedGrab) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_16441); }
PORT(ActionHelp_ShowWarioFullyChargedThrow) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_16423); }
PORT(ActionHelp_ShowWarioRoll) { fs_hli(gb, wActionHelpWarioFramesetOffset, ADDR_Frameset_166f7); }
PORT(ActionHelp_ShowParaGoomThrown) { fs_hli(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6f3f); }
PORT(ActionHelp_ShowFutamoguAngry) { fs_hli(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6f78); }
PORT(ActionHelp_ShowFutamoguHappy) { fs_hli(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6f66); }
PORT(ActionHelp_ShowCountRichtertoffStun) { fs_hli(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6f5e); }
PORT(ActionHelp_ShowCountRichtertoffThrown) { fs_hli(gb, wMenuObj4FramesetOffset, ADDR_Frameset_1e6f55); }

// hl = menu object's frameset offset
static void show_enemy_vanish(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	wr8(gb, hl++, HI(ADDR_Frameset_d4a5));
	wr8(gb, hl++, LO(ADDR_Frameset_d4a5));
	wr8(gb, hl++, HI(ADDR_OAM_d20e));
	wr8(gb, hl++, LO(ADDR_OAM_d20e));
	wr8(gb, hl, BANK_OAM_d20e);
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, hl);
	play_sfx(gb, SFX_VANISH);
}

PORT(ActionHelp_ShowObj4EnemyVanish)
{
	SET_HL(gb, wMenuObj4FramesetOffset);
	show_enemy_vanish(gb); // fallthrough
}

PORT(ActionHelp_ShowEnemyVanish) { show_enemy_vanish(gb); }

// y / x / 4 zeros / frameset / OAM pointer and bank; the last write
// increments hl when hli is set
static void init_obj(GB *gb, uint16_t hl, uint8_t y, uint8_t x, uint16_t fs, uint16_t oam,
	uint8_t bank, bool hli)
{
	wr8(gb, hl++, y);
	wr8(gb, hl++, x);
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, HI(fs));
	wr8(gb, hl++, LO(fs));
	wr8(gb, hl++, HI(oam));
	wr8(gb, hl++, LO(oam));
	gb->cpu.a = bank;
	wr8(gb, hl, bank);
	if (hli)
		hl++;
	SET_HL(gb, hl);
}

PORT(ActionHelp_ShowCountRichtertoffIdle)
{
	init_obj(gb, wMenuObj4, 0x40, 0xe8, ADDR_Frameset_1e6f4d, ADDR_OAM_1e6bab, BANK_OAM_1e6bab, true);
}

PORT(ActionHelp_ShowVerticalBlockPair)
{
	init_obj(gb, wMenuObj4, 0x40, 0xe8, ADDR_Frameset_1e6b93, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
}

PORT(ActionHelp_ShowSlide)
{
	init_obj(gb, wMenuObj4, 0x40, 0xd8, ADDR_Frameset_1e6b88, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
}

PORT(ActionHelp_ShowOverheadPillar)
{
	init_obj(gb, wMenuObj6, 0x20, 0xe8, ADDR_Frameset_1e6b85, ADDR_OAM_1e697c, BANK_OAM_1e697c, true);
}
