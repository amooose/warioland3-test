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

// rst JumpTable, with the table at `table` (right after the rst). Leaves
// every register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// call .StateTable, where .StateTable is ld a, [wActionHelpState] /
// jumptable: runs the current state's function, which returns here.
static void run_state(GB *gb, uint16_t ret, uint16_t state_table)
{
	gb_push16(gb, ret); // call .StateTable
	gb->cpu.a = rd8(gb, wActionHelpState);
	uint16_t target = jumptable(gb, state_table + 4);
	gb_pop16(gb);
	gb_call_ret(gb, target, ret);
}

// ld hl, ptr + 1 / ld a, [bank] / ld [wTempBank], a / ld a, [wTempBank] /
// ldh [hCallFuncBank], a / hcall UpdateObjAnim / ld a, [wObjAnimWasReset] /
// ld [ended], a
static void update_menu_obj_anim(GB *gb, uint16_t ptr, uint16_t bank, uint16_t ended)
{
	SET_HL(gb, ptr + 1);
	gb->cpu.a = rd8(gb, bank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(UpdateObjAnim);
	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	wr8(gb, ended, gb->cpu.a);
}

PORT(NoPowerUpActionHelp)
{
	run_state(gb, ADDR_NoPowerUpActionHelp + 3, ADDR_NoPowerUpActionHelp + 0x97);

	gb->cpu.a = rd8(gb, wActionHelpBlackedOut);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a)
		return;
	gb->cpu.a = rd8(gb, wActionHelpFrozen);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a) {
		update_menu_obj_anim(gb, wMenuObj5FramesetPtr, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
		SET_HL(gb, wMenuObj5);
		FARCALL(LoadActionHelpSprite_Far);
		update_menu_obj_anim(gb, wMenuObj4FramesetPtr, wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		SET_HL(gb, wActionHelpWarioFramesetPtr + 1);
		FARCALL_BANK(0x05, UpdateObjAnim);
	}
	SET_HL(gb, wMenuObj4);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wActionHelpWario);
	FARCALL(LoadActionHelpWarioSprite);
}

// Ends the demonstration: back to the level or the title screen, or
// restarts it.
static void func_1f8362(GB *gb)
{
	gb->cpu.a = rd8(gb, wPowerUpLevel);
	if (alu_bit(gb, gb->cpu.a, ACTION_HELP_LEVEL_CLEAR_F)) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, POWER_UP_MASK);
		wr8(gb, wPowerUpLevel, gb->cpu.a);
		FARCALL(Func_4628);
		CALL(ResumeNormalVolume);
		RET();
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, ACTION_HELP_TITLE_SCREEN_F)) {
		// restart action help
		AH(ActionHelp_ClearDPadInput);
		AH(ActionHelp_ClearButtonsInput);
		gb->cpu.a = 0x01;
		wr8(gb, wActionHelpState, 0x01);
		func_1f839a(gb);
		RET();
		return;
	}
	JUMP(InitWithoutDemoPowerUpReset);
}

PORT_JUMP(Func_1f8362) { func_1f8362(gb); }

PORT(Func_1f839a) { func_1f839a(gb); }

// dec [hl] / ld a, [wActionHelpCounter] / and %1 / jr z, :+ / dec [hl] / :
// ld a, [hl] / cp n: returns whether [addr] < n
static bool dec_twice_odd_cp(GB *gb, uint16_t addr, uint8_t n)
{
	dec_twice_odd(gb, addr);
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// ld hl, wTimer / dec [hl] / ret nz / ShowWarioWalk / ShowDRightInput /
// next state
static void wait_then_walk(GB *gb)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT(Func_1f85eb)
{
	if (!attack_paragoom(gb, 0x50))
		return;
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioAttack);
	play_sfx(gb, SFX_BUMP);
	gb->cpu.a = 0x0e;
	wr8(gb, wActionHelpWarioVar, 0x0e);
	next_state(gb);
}

PORT(Func_1f8638)
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

PORT(Func_1f8673) { wait_then_walk(gb); }
PORT(Func_1f8778) { wait_then_walk(gb); }
PORT(Func_1f883c) { wait_then_walk(gb); }

// ld hl, wActionHelpWarioXCoord / ld a, [hl] / cp $40 / jr z, .x /
// inc [hl] / ld a, [hl] / cp $40 / ret c: returns false on the ret
static bool walk_wario_to_40(GB *gb)
{
	SET_HL(gb, wActionHelpWarioXCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_Z)
		return true;
	wr8(gb, wActionHelpWarioXCoord, alu_inc(gb, rd8(gb, wActionHelpWarioXCoord)));
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	return !(gb->cpu.f & FLAG_C);
}

PORT(Func_1f8683)
{
	if (!walk_wario_to_40(gb))
		return;
	SET_HL(gb, wMenuObj4XCoord);
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x58))
		return;
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = alu_sub(gb, rd8(gb, wMenuObj4XCoord), 0x40);
	alu_cp(gb, gb->cpu.a, 0x1c);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

// the jump over the slide: Wario jumps (using var), Para-Goom moves left
static void jump_over(GB *gb, uint16_t var)
{
	SET_HL(gb, wActionHelpWarioXCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (!(gb->cpu.f & FLAG_Z))
		wr8(gb, wActionHelpWarioXCoord, alu_inc(gb, rd8(gb, wActionHelpWarioXCoord)));
	wr8(gb, var, alu_inc(gb, rd8(gb, var)));
	gb->cpu.a = rd8(gb, var);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wActionHelpWarioYCoord), rd8(gb, hl));
	wr8(gb, wActionHelpWarioYCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C) {
		SET_HL(gb, wMenuObj4XCoord);
		wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, var, 0);
	gb->cpu.a = 0x40;
	wr8(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

PORT(Func_1f86b5) { jump_over(gb, wActionHelpWarioEnd); }
PORT(Func_1f886b) { jump_over(gb, wActionHelpWarioVar); }

PORT(Func_1f86ee)
{
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0xe8);
	if (gb->cpu.f & FLAG_C)
		return;
	AH(ActionHelp_ShowSlide);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT(Func_1f8708)
{
	if (!walk_wario_to_40(gb))
		return;
	SET_HL(gb, wMenuObj4XCoord);
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x60))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

PORT(Func_1f8730)
{
	jump_wario(gb);
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, rd8(gb, wMenuObj4XCoord));
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	wr8(gb, wActionHelpWarioXCoord, gb->cpu.a);
	AH(ActionHelp_ClearDPadInput);
	SET_HL(gb, wActionHelpWarioYCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (gb->cpu.f & FLAG_C)
		return;
	wr8(gb, wActionHelpWarioYCoord, 0x20);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ClearButtonsInput);
	gb->cpu.a = 0x30;
	wr8(gb, wTimer, 0x30);
	next_state(gb);
}

PORT(Func_1f8788)
{
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0x39);
	if (!(gb->cpu.f & FLAG_C))
		return;
	alu_cp(gb, gb->cpu.a, 0x30);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, wActionHelpWarioYCoord);
		wr8(gb, wActionHelpWarioYCoord, alu_inc(gb, rd8(gb, wActionHelpWarioYCoord)));
		return;
	}
	AH(ActionHelp_ShowWarioSlide);
	next_state(gb);
}

// slide Wario down to y $40 while the slide moves left
static void slide_down(GB *gb)
{
	SET_HL(gb, wActionHelpWarioYCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, wActionHelpWarioYCoord, alu_inc(gb, rd8(gb, wActionHelpWarioYCoord)));
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
}

PORT(Func_1f87a1)
{
	slide_down(gb);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ShowWarioRoll);
	next_state(gb);
}

PORT(Func_1f87bd)
{
	slide_down(gb);
	alu_cp(gb, gb->cpu.a, 0xd8);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ShowSingleSolidBlock);
	next_state(gb);
}

PORT(Func_1f87d6)
{
	SET_HL(gb, wMenuObj4XCoord);
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x50))
		return;
	AH(ActionHelp_ShowWarioBump);
	play_sfx(gb, SFX_BUMP);
	gb->cpu.a = 0x0e;
	wr8(gb, wActionHelpWarioVar, 0x0e);
	next_state(gb);
}

PORT(Func_1f87fb)
{
	jump_wario(gb);
	dec_twice_odd(gb, wActionHelpWarioXCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0x40;
	wr8(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ShowWarioIdle);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpWarioVar, 0);
	gb->cpu.a = 0x30;
	wr8(gb, wTimer, 0x30);
	next_state(gb);
}

PORT(Func_1f884c)
{
	SET_HL(gb, wActionHelpWarioXCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, wActionHelpWarioXCoord, alu_inc(gb, rd8(gb, wActionHelpWarioXCoord)));
		gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	}
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = alu_sub(gb, rd8(gb, wMenuObj4XCoord), gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x1c);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

PORT_JUMP(Func_1f88a4)
{
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0xe8);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	func_1f8362(gb);
}
