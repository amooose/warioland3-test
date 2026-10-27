// Port of src/engine/action_help/power_ups/none.asm
#include "ported.h"

// Calls a routine in the Action Help bank (runs whatever bank is mapped,
// like the asm's call).
#define AH(name) gb_call(gb, ADDR_##name)

// ld hl, wActionHelpState / inc [hl]
static void next_state(GB *gb)
{
	SET_HL(gb, wActionHelpState);
	wr8(gb, wActionHelpState, alu_inc(gb, rd8(gb, wActionHelpState)));
}

// ld hl, addr / dec [hl] / ld a, [wActionHelpCounter] / and %1 / jr z, :+ /
// dec [hl] / :
static void dec_twice_odd(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x01);
	if (gb->cpu.a)
		wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
}

// play_sfx_rept n, sfx
static void play_sfx_rept(GB *gb, uint8_t n, uint16_t sfx)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wSFXTimer), 1);
	wr8(gb, wSFXTimer, gb->cpu.a);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = n;
		wr8(gb, wSFXTimer, n);
		play_sfx(gb, sfx);
	}
}

// ld hl, wActionHelpWarioVar / inc [hl] / ld a, [hl] / ld e, a / ld d, $00 /
// ld hl, JumpVelTable_Normal / add hl, de / ld a, [wActionHelpWarioYCoord] /
// add [hl] / ld [wActionHelpWarioYCoord], a
// Returns hl.
static uint16_t jump_wario(GB *gb)
{
	wr8(gb, wActionHelpWarioVar, alu_inc(gb, rd8(gb, wActionHelpWarioVar)));
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wActionHelpWarioYCoord), rd8(gb, hl));
	wr8(gb, wActionHelpWarioYCoord, gb->cpu.a);
	return hl;
}

static void func_1f839a(GB *gb)
{
	AH(ActionHelp_ShowInitialWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowParaGoomWalk);
	AH(ActionHelp_HideObj5BlockDebris);
	FARCALL(LoadActionHelpPalsWithoutWater);
	next_state(gb);
}

PORT(Func_1f8395)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	func_1f839a(gb);
}

PORT(Func_1f83ba)
{
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0x60);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

PORT(Func_1f83d5)
{
	jump_wario(gb);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x18);
	bool move = gb->cpu.f & FLAG_C;
	if (!move) {
		gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
		alu_cp(gb, gb->cpu.a, 0x34);
		move = gb->cpu.f & FLAG_C;
	}
	if (!move) {
		// step on Para-Goom
		play_sfx(gb, SFX_014);
		gb->cpu.a = 0x0a;
		wr8(gb, wActionHelpWarioVar, 0x0a);
		AH(ActionHelp_ShowParaGoomStun);
		next_state(gb);
		return;
	}
	// don't move Para-Goom if it's already at Wario's x pos
	SET_HL(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, rd8(gb, wMenuObj4XCoord));
	if (gb->cpu.f & FLAG_Z)
		return;
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, rd8(gb, wMenuObj4XCoord));
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	wr8(gb, wMenuObj4XCoord, gb->cpu.a);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ClearDPadInput);
}

PORT(Func_1f842e)
{
	uint16_t hl = jump_wario(gb);
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), rd8(gb, hl));
	wr8(gb, wMenuObj4YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C) {
		SET_HL(gb, wMenuObj4XCoord);
		wr8(gb, wMenuObj4XCoord, alu_inc(gb, rd8(gb, wMenuObj4XCoord)));
		gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x01);
		if (gb->cpu.a)
			wr8(gb, wMenuObj4XCoord, alu_inc(gb, rd8(gb, wMenuObj4XCoord)));
		return;
	}
	// land Wario
	gb->cpu.a = 0x40;
	wr8(gb, wMenuObj4YCoord, 0x40);
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0x40;
	wr8(gb, wActionHelpWarioYCoord, 0x40);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	AH(ActionHelp_ShowWarioIdle);
	gb->cpu.a = 0x30;
	wr8(gb, wTimer, 0x30);
	next_state(gb);
}

PORT(Func_1f847a)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ShowWarioAttack);
	gb->cpu.a = 0x30;
	wr8(gb, wActionHelpWarioVar, 0x30);
	next_state(gb);
}

// play_sfx_rept 6, SFX_ATTACK / dec wActionHelpWarioVar / ret if >= $2b /
// move Para-Goom left / returns whether its x is below x_limit
static bool attack_paragoom(GB *gb, uint8_t x_limit)
{
	play_sfx_rept(gb, 6, SFX_ATTACK);
	SET_HL(gb, wActionHelpWarioVar);
	wr8(gb, wActionHelpWarioVar, alu_dec(gb, rd8(gb, wActionHelpWarioVar)));
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x2b);
	if (!(gb->cpu.f & FLAG_C))
		return false;
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, x_limit);
	return gb->cpu.f & FLAG_C;
}

PORT(Func_1f848c)
{
	if (!attack_paragoom(gb, 0x4c))
		return;
	play_sfx(gb, SFX_017);
	AH(ActionHelp_ClearButtonsInput);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	AH(ActionHelp_ShowWarioIdle);
	gb->cpu.a = 0x30;
	wr8(gb, wTimer, 0x30);
	next_state(gb);
}

PORT(Func_1f84d7)
{
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_inc(gb, rd8(gb, wMenuObj4XCoord)));
	wr8(gb, wMenuObj4XCoord, alu_inc(gb, rd8(gb, wMenuObj4XCoord)));

	SET_HL(gb, wActionHelpWarioVar);
	wr8(gb, wActionHelpWarioVar, alu_inc(gb, rd8(gb, wActionHelpWarioVar)));
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x28);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x27;
		wr8(gb, wActionHelpWarioVar, 0x27);
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), rd8(gb, hl));
	wr8(gb, wMenuObj4YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_C)
		return;

	// load cracked and solid blocks
	hl = wMenuObj4;
	wr8(gb, hl++, 0x40);
	wr8(gb, hl++, 0xe8);
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(ADDR_Frameset_1e6ba5 >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_Frameset_1e6ba5);
	wr8(gb, hl++, (uint8_t)(ADDR_OAM_1e697c >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_OAM_1e697c);
	SET_HL(gb, hl);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT(Func_1f8522)
{
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0x78);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ShowBInput);
	AH(ActionHelp_ShowWarioAttack);
	gb->cpu.a = 0x30;
	wr8(gb, wActionHelpWarioVar, 0x30);
	next_state(gb);
}

PORT(Func_1f853a)
{
	if (!attack_paragoom(gb, 0x60))
		return;
	SET_HL(gb, wMenuObj5);
	gb->cpu.b = 0x00;
	gb->cpu.c = 0x10;
	AH(ActionHelp_ShowBlockDebris);
	AH(ActionHelp_ShowSingleSolidBlock);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioAttack);
	gb->cpu.a = 0x0e;
	wr8(gb, wActionHelpWarioVar, 0x0e);
	next_state(gb);
}

PORT(Func_1f8588)
{
	dec_twice_odd(gb, wActionHelpWarioXCoord);
	jump_wario(gb);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	gb->cpu.a = 0x40;
	wr8(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_HideObj5BlockDebris);
	AH(ActionHelp_ShowWarioIdle);
	gb->cpu.a = 0x30;
	wr8(gb, wTimer, 0x30);
	next_state(gb);
}

PORT(Func_1f85c3)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT(Func_1f85d3)
{
	SET_HL(gb, wActionHelpWarioXCoord);
	wr8(gb, wActionHelpWarioXCoord, alu_inc(gb, rd8(gb, wActionHelpWarioXCoord)));
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	AH(ActionHelp_ShowBInput);
	AH(ActionHelp_ShowWarioAttack);
	gb->cpu.a = 0x30;
	wr8(gb, wActionHelpWarioVar, 0x30);
	next_state(gb);
}
