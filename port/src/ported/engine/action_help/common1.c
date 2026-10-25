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
	uint8_t y = alu_sub(gb, rd8(gb, wMenuObj4YCoord), gb->cpu.b);
	uint8_t x = alu_sub(gb, rd8(gb, wMenuObj4XCoord), gb->cpu.c);
	init_menu_obj(gb, REG_HL(gb), y, x, ADDR_Frameset_d49a, ADDR_OAM_d20e, BANK_OAM_d20e);
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
