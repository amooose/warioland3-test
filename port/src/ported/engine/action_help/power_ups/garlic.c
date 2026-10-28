// Port of src/engine/action_help/power_ups/garlic.asm
#include "ported.h"

#pragma GCC diagnostic ignored "-Wunused-function"

// The helpers below are shared by every power-up's action help (each file
// keeps its own copy).

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

// dec_twice_odd / ld a, [hl] / cp n: returns whether [addr] < n
static bool dec_twice_odd_cp(GB *gb, uint16_t addr, uint8_t n)
{
	dec_twice_odd(gb, addr);
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// ld hl, addr / dec [hl] / ld a, [hl] / cp n: returns whether [addr] < n
static bool dec_cp(GB *gb, uint16_t addr, uint8_t n)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// ld hl, addr / inc [hl] / ld a, [hl] / cp n: returns whether [addr] < n
static bool inc_cp(GB *gb, uint16_t addr, uint8_t n)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_inc(gb, rd8(gb, addr)));
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
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

// ld hl, var / inc [hl] / ld a, [hl] / ld e, a / ld d, $00 /
// ld hl, JumpVelTable_Normal / add hl, de / ld a, [wActionHelpWarioYCoord] /
// add [hl] / ld [wActionHelpWarioYCoord], a
// Returns hl.
static uint16_t jump_wario_var(GB *gb, uint16_t var)
{
	wr8(gb, var, alu_inc(gb, rd8(gb, var)));
	gb->cpu.a = rd8(gb, var);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wActionHelpWarioYCoord), rd8(gb, hl));
	wr8(gb, wActionHelpWarioYCoord, gb->cpu.a);
	return hl;
}

static uint16_t jump_wario(GB *gb)
{
	return jump_wario_var(gb, wActionHelpWarioVar);
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
// ldh [hCallFuncBank], a / hcall UpdateObjAnim
static void update_menu_obj_anim(GB *gb, uint16_t ptr, uint16_t bank)
{
	SET_HL(gb, ptr + 1);
	gb->cpu.a = rd8(gb, bank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(UpdateObjAnim);
}

// ... / ld a, [wObjAnimWasReset] / ld [ended], a
static void update_menu_obj_anim_ended(GB *gb, uint16_t ptr, uint16_t bank, uint16_t ended)
{
	update_menu_obj_anim(gb, ptr, bank);
	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	wr8(gb, ended, gb->cpu.a);
}

// ld a, [addr] / and a: returns a
static uint8_t ld_and(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	return gb->cpu.a;
}

// ld hl, obj / y / x / xor a, 4 zeros / frameset / OAM pointer and bank,
// all with ld [hli], a. Returns hl (after the bank).
static uint16_t init_obj(GB *gb, uint16_t hl, uint8_t y, uint8_t x, uint16_t fs, uint16_t oam,
	uint8_t bank)
{
	wr8(gb, hl++, y);
	wr8(gb, hl++, x);
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(fs >> 8));
	wr8(gb, hl++, (uint8_t)fs);
	wr8(gb, hl++, (uint8_t)(oam >> 8));
	wr8(gb, hl++, (uint8_t)oam);
	gb->cpu.a = bank;
	wr8(gb, hl++, bank);
	SET_HL(gb, hl);
	return hl;
}

// ld hl, wTimer / dec [hl] / ret nz / then: returns whether it reached 0
static bool dec_timer(GB *gb)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	return gb->cpu.f & FLAG_Z;
}

// dec_timer / ShowWarioWalk / ShowDRightInput / next state
static void wait_then_walk(GB *gb)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

// ld hl, wActionHelpWarioXCoord / ld a, [hl] / cp $40 / jr z, .x /
// inc [hl] / ld a, [hl] / cp $40 / ret c: returns false on the ret
static bool walk_wario_to_40(GB *gb)
{
	SET_HL(gb, wActionHelpWarioXCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_Z)
		return true;
	return !inc_cp(gb, wActionHelpWarioXCoord, 0x40);
}

// ld hl, wActionHelpWarioXCoord / ld a, [hl] / cp $40 / jr z, :+ / inc [hl] /
// (ld a, [hl]) / :
static void wario_x_towards_40(GB *gb, bool reload)
{
	SET_HL(gb, wActionHelpWarioXCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_Z)
		return;
	wr8(gb, wActionHelpWarioXCoord, alu_inc(gb, rd8(gb, wActionHelpWarioXCoord)));
	if (reload)
		gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
}

// ShowAInput / ShowWarioJump / next state
static void start_jump(GB *gb)
{
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

// ShowBInput / ShowWarioAttack / var = $30 / next state
static void start_attack(GB *gb)
{
	AH(ActionHelp_ShowBInput);
	AH(ActionHelp_ShowWarioAttack);
	gb->cpu.a = 0x30;
	wr8(gb, wActionHelpWarioVar, 0x30);
	next_state(gb);
}

// ld a, $0e / ld [wActionHelpWarioVar], a / next state
static void var_0e_next(GB *gb)
{
	gb->cpu.a = 0x0e;
	wr8(gb, wActionHelpWarioVar, 0x0e);
	next_state(gb);
}

// ld a, $30 / ld [wTimer], a / next state
static void timer_30_next(GB *gb)
{
	gb->cpu.a = 0x30;
	wr8(gb, wTimer, 0x30);
	next_state(gb);
}

// xor a / ld [addr], a
static void clear(GB *gb, uint16_t addr)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, addr, 0);
}

// ld a, n / ld [addr], a
static void ld_n(GB *gb, uint16_t addr, uint8_t n)
{
	gb->cpu.a = n;
	wr8(gb, addr, n);
}

// Ends the demonstration: back to the level or the title screen, or
// restarts it with `restart` (which ends in ret).
static void end_action_help(GB *gb, void (*restart)(GB *))
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
		ld_n(gb, wActionHelpState, 0x01);
		restart(gb);
		RET();
		return;
	}
	JUMP(InitWithoutDemoPowerUpReset);
}

// ld hl, wMenuObj4XCoord / dec [hl] / ld a, [hl] / cp $e8 / ret c /
// jp end (a PORT_JUMP body)
static void last_state(GB *gb, void (*restart)(GB *))
{
	if (dec_cp(gb, wMenuObj4XCoord, 0xe8)) {
		RET();
		return;
	}
	end_action_help(gb, restart);
}

PORT(GarlicActionHelp)
{
	run_state(gb, ADDR_GarlicActionHelp + 3, ADDR_GarlicActionHelp + 0xf9);

	if (ld_and(gb, wActionHelpBlackedOut))
		return;
	update_menu_obj_anim_ended(gb, wMenuObj5FramesetPtr, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
	SET_HL(gb, wMenuObj5YCoord);
	FARCALL(LoadActionHelpSprite_Far);
	update_menu_obj_anim_ended(gb, wMenuObj7FramesetPtr, wMenuObj7FramesetBank, wMenuObj7AnimationEnded);
	SET_HL(gb, wMenuObj7);
	FARCALL(LoadActionHelpSprite_Far);
	if (!ld_and(gb, wActionHelpFrozen)) {
		update_menu_obj_anim_ended(gb, wMenuObj4FramesetPtr, wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		update_menu_obj_anim(gb, wMenuObj6FramesetPtr, wMenuObj6FramesetBank);
		SET_HL(gb, wActionHelpWarioFramesetPtr + 1);
		FARCALL_BANK(0x05, UpdateObjAnim);
		gb->cpu.a = rd8(gb, wObjAnimWasReset);
		wr8(gb, wActionHelpWarioAnimationEnded, gb->cpu.a);
	}
	SET_HL(gb, wMenuObj4);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wMenuObj6);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wActionHelpWario);
	FARCALL(LoadActionHelpWarioSprite);
}

static void func_1f9d1f(GB *gb)
{
	AH(ActionHelp_ShowInitialWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	uint16_t hl = init_obj(gb, wMenuObj4, 0x40, 0xe8, ADDR_Frameset_1e6b9c, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	clear(gb, hl);
	AH(ActionHelp_HideObj5BlockDebris);
	init_obj(gb, wMenuObj6, 0x40, 0xf8, ADDR_Frameset_1e6b9c, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	SET_HL(gb, wMenuObj7);
	AH(ActionHelp_HideBlockDebris);
	FARCALL(LoadActionHelpPalsWithoutWater);
	next_state(gb);
}

PORT_JUMP(Func_1f9ce7) { end_action_help(gb, func_1f9d1f); }
PORT(Func_1f9d1f) { func_1f9d1f(gb); }

PORT(Func_1f9d1a)
{
	if (!dec_timer(gb))
		return;
	func_1f9d1f(gb);
}

// ld hl, wMenuObj6XCoord / dec [hl] / ld hl, wMenuObj4XCoord / dec [hl]
static void dec_obj6_obj4(GB *gb)
{
	SET_HL(gb, wMenuObj6XCoord);
	wr8(gb, wMenuObj6XCoord, alu_dec(gb, rd8(gb, wMenuObj6XCoord)));
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
}

// dec_obj6_obj4 / ld a, [hl] / cp $60: returns whether below
static bool move_blocks(GB *gb)
{
	dec_obj6_obj4(gb);
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0x60);
	return gb->cpu.f & FLAG_C;
}

PORT(Func_1f9d7e)
{
	if (!move_blocks(gb))
		return;
	AH(ActionHelp_ShowWarioAttack);
	ld_n(gb, wActionHelpWarioVar, 0x30);
	next_state(gb);
}

PORT(Func_1f9e9b)
{
	if (!move_blocks(gb))
		return;
	start_attack(gb);
}

// play_sfx_rept 6, SFX_ATTACK_POWERED_UP / dec var / ret if >= $2b
static bool attack_countdown(GB *gb)
{
	play_sfx_rept(gb, 6, SFX_ATTACK_POWERED_UP);
	SET_HL(gb, wActionHelpWarioVar);
	wr8(gb, wActionHelpWarioVar, alu_dec(gb, rd8(gb, wActionHelpWarioVar)));
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x2b);
	return gb->cpu.f & FLAG_C;
}

// attack_countdown, then pushes both blocks: returns whether obj4 is
// below $50
static bool attack_blocks(GB *gb)
{
	if (!attack_countdown(gb))
		return false;
	dec_obj6_obj4(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x01);
	if (gb->cpu.a)
		dec_obj6_obj4(gb);
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0x50);
	return gb->cpu.f & FLAG_C;
}

// attack_countdown / dec_twice_odd obj4 / cp $50
static bool attack_obj4(GB *gb)
{
	if (!attack_countdown(gb))
		return false;
	return dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x50);
}

// ld b, $00 / call ActionHelp_ShowBlockDebrisWithYOffset / ld hl, wMenuObj7 /
// lb bc, $10, $00 / call ActionHelp_ShowBlockDebris
static void break_blocks(GB *gb)
{
	gb->cpu.b = 0x00;
	AH(ActionHelp_ShowBlockDebrisWithYOffset);
	SET_HL(gb, wMenuObj7);
	gb->cpu.b = 0x10;
	gb->cpu.c = 0x00;
	AH(ActionHelp_ShowBlockDebris);
}

PORT(Func_1f9d97)
{
	if (!attack_blocks(gb))
		return;
	break_blocks(gb);
	gb->cpu.a = rd8(gb, wMenuObj6XCoord);
	wr8(gb, wMenuObj4XCoord, gb->cpu.a);
	init_obj(gb, wMenuObj6, 0x40, 0xf8, ADDR_Frameset_1e6b96, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	next_state(gb);
}

PORT(Func_1f9e07)
{
	play_sfx_rept(gb, 6, SFX_ATTACK_POWERED_UP);
	SET_HL(gb, wActionHelpWarioVar);
	wr8(gb, wActionHelpWarioVar, alu_dec(gb, rd8(gb, wActionHelpWarioVar)));
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x50))
		return;
	break_blocks(gb);
	init_obj(gb, wMenuObj4, 0x40, 0xe8, ADDR_Frameset_1e6b96, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	next_state(gb);
}

// hides the debris of obj5 and obj7
static void hide_debris(GB *gb)
{
	AH(ActionHelp_HideObj5BlockDebris);
	SET_HL(gb, wMenuObj7);
	AH(ActionHelp_HideBlockDebris);
}

PORT(Func_1f9e62)
{
	play_sfx_rept(gb, 6, SFX_ATTACK_POWERED_UP);
	if (ld_and(gb, wMenuObj5AnimationEnded))
		hide_debris(gb);
	SET_HL(gb, wActionHelpWarioVar);
	wr8(gb, wActionHelpWarioVar, alu_dec(gb, rd8(gb, wActionHelpWarioVar)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT(Func_1f9eb7)
{
	if (!attack_blocks(gb))
		return;
	break_blocks(gb);
	gb->cpu.a = rd8(gb, wMenuObj6XCoord);
	wr8(gb, wMenuObj4XCoord, gb->cpu.a);
	init_obj(gb, wMenuObj6, 0xa0, 0xf8, ADDR_Frameset_1e6ba2, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioAttack);
	var_0e_next(gb);
}

// Wario bounces back from the blocks and lands
static void bounce_back(GB *gb)
{
	dec_twice_odd(gb, wActionHelpWarioXCoord);
	jump_wario(gb);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	clear(gb, wActionHelpWarioVar);
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	hide_debris(gb);
	AH(ActionHelp_ShowWarioIdle);
	timer_30_next(gb);
}

PORT(Func_1f9f35) { bounce_back(gb); }
PORT(Func_1f9ff4) { bounce_back(gb); }
PORT(Func_1f9f76) { wait_then_walk(gb); }
PORT(Func_1fa035) { wait_then_walk(gb); }
PORT(Func_1fa145) { wait_then_walk(gb); }
PORT(Func_1fa1c5) { wait_then_walk(gb); }
PORT(Func_1fa37c) { wait_then_walk(gb); }

PORT(Func_1f9f86)
{
	if (inc_cp(gb, wActionHelpWarioXCoord, 0x40))
		return;
	start_attack(gb);
}

PORT(Func_1f9f9e)
{
	if (!attack_obj4(gb))
		return;
	break_blocks(gb);
	AH(ActionHelp_ShowCountRichtertoffIdle);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioAttack);
	var_0e_next(gb);
}

PORT(Func_1fa045)
{
	if (!walk_wario_to_40(gb))
		return;
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x58))
		return;
	start_attack(gb);
}

PORT(Func_1fa072)
{
	if (!attack_obj4(gb))
		return;
	play_sfx(gb, SFX_017);
	AH(ActionHelp_ShowCountRichtertoffStun);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioAttack);
	var_0e_next(gb);
}

// ld hl, wMenuObj4XCoord / inc [hl] / inc [hl]
static void inc_obj4_x_twice(GB *gb)
{
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_inc(gb, rd8(gb, wMenuObj4XCoord)));
	wr8(gb, wMenuObj4XCoord, alu_inc(gb, rd8(gb, wMenuObj4XCoord)));
}

// ld hl, wActionHelpWarioVar / inc [hl] / ld a, [hl] / cp $28 / jr c, :+ /
// ld a, $27 / ld [hl], a / : ld e, a / ld d, $00 / ld hl, JumpVelTable_Normal /
// add hl, de. Returns hl.
static uint16_t capped_jump_vel(GB *gb)
{
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
	return hl;
}

// ld a, [addr] / add [hl] / ld [addr], a
static void add_vel(GB *gb, uint16_t addr, uint16_t hl)
{
	gb->cpu.a = alu_add(gb, rd8(gb, addr), rd8(gb, hl));
	wr8(gb, addr, gb->cpu.a);
}

PORT(Func_1fa0c2)
{
	uint16_t hl = capped_jump_vel(gb);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_C) {
		add_vel(gb, wActionHelpWarioYCoord, hl);
		add_vel(gb, wMenuObj4YCoord, hl);
		inc_obj4_x_twice(gb);
		dec_twice_odd(gb, wActionHelpWarioXCoord);
		return;
	}
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_Z) {
		add_vel(gb, wMenuObj4YCoord, hl);
		inc_obj4_x_twice(gb);
		gb->cpu.a = rd8(gb, wMenuObj4YCoord);
		alu_cp(gb, gb->cpu.a, 0x80);
		if (gb->cpu.f & FLAG_C)
			return;
		AH(ActionHelp_ShowSlide);
		clear(gb, wActionHelpWarioVar);
		timer_30_next(gb);
		return;
	}
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, hl));
	wr8(gb, wActionHelpWarioYCoord, gb->cpu.a);
	add_vel(gb, wMenuObj4YCoord, hl);
	inc_obj4_x_twice(gb);
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ShowWarioIdle);
}

PORT(Func_1fa155)
{
	if (!walk_wario_to_40(gb))
		return;
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x60))
		return;
	start_jump(gb);
}

PORT(Func_1fa17d)
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
	SET_HL(gb, wActionHelpWario);
	gb->cpu.a = rd8(gb, wActionHelpWario);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (gb->cpu.f & FLAG_C)
		return;
	wr8(gb, wActionHelpWario, 0x20);
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ClearButtonsInput);
	timer_30_next(gb);
}

PORT(Func_1fa1d5)
{
	if (!dec_cp(gb, wMenuObj4XCoord, 0x39))
		return;
	alu_cp(gb, gb->cpu.a, 0x30);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, wActionHelpWario);
		wr8(gb, wActionHelpWario, alu_inc(gb, rd8(gb, wActionHelpWario)));
		return;
	}
	AH(ActionHelp_ShowWarioSlide);
	next_state(gb);
}

// slide Wario down to y $40 while the slide moves left; a = obj4 x
static void slide_down(GB *gb)
{
	SET_HL(gb, wActionHelpWario);
	gb->cpu.a = rd8(gb, wActionHelpWario);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, wActionHelpWario, alu_inc(gb, rd8(gb, wActionHelpWario)));
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = rd8(gb, wMenuObj4XCoord);
}

PORT(Func_1fa1ee)
{
	slide_down(gb);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ShowWarioRoll);
	next_state(gb);
}

PORT(Func_1fa20a)
{
	slide_down(gb);
	alu_cp(gb, gb->cpu.a, 0xd8);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ShowCountRichtertoffIdle);
	next_state(gb);
}

PORT(Func_1fa223)
{
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x50))
		return;
	play_sfx(gb, SFX_BUMP);
	AH(ActionHelp_ShowCountRichtertoffStun);
	var_0e_next(gb);
}

PORT(Func_1fa248)
{
	uint16_t hl = capped_jump_vel(gb);
	add_vel(gb, wMenuObj4YCoord, hl);
	inc_obj4_x_twice(gb);
	gb->cpu.a = rd8(gb, wMenuObj4YCoord);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_C)
		return;
	init_obj(gb, wMenuObj4, 0x40, 0xd8, ADDR_Frameset_1e6b64, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	ld_n(gb, wMenuObj6, 0x40);
	clear(gb, wActionHelpWarioVar);
	next_state(gb);
}

PORT(Func_1fa298)
{
	dec_obj6_obj4(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x01);
	if (gb->cpu.a)
		dec_obj6_obj4(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wMenuObj4XCoord), 0xfe);
	alu_cp(gb, gb->cpu.a, 0x48);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, wMenuObj4XCoord, 0x48);
	gb->cpu.b = 0x00;
	AH(ActionHelp_ShowBlockDebrisWithYOffset);
	SET_HL(gb, wMenuObj6);
	SET_DE(gb, wMenuObj4YCoord);
	gb->cpu.b = 0x0b;
	CALL(CopyHLToDE);
	SET_HL(gb, wMenuObj6);
	AH(ActionHelp_HideBlockDebris);
	next_state(gb);
}

PORT(Func_1fa2d4)
{
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = alu_and(gb, rd8(gb, wMenuObj4XCoord), 0xfe);
	alu_cp(gb, gb->cpu.a, 0x58);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, wMenuObj4XCoord, 0x58);
		gb->cpu.b = 0x00;
		AH(ActionHelp_ShowBlockDebrisWithYOffset);
		AH(ActionHelp_ShowSingleSolidBlock);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x48);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.b = 0x00;
	AH(ActionHelp_ShowBlockDebrisWithYOffset);
	uint16_t hl = wMenuObj4;
	wr8(gb, hl++, 0x50);
	wr8(gb, hl++, 0xe8);
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(ADDR_Frameset_1e6b85 >> 8));
	gb->cpu.a = (uint8_t)ADDR_Frameset_1e6b85;
	wr8(gb, hl++, gb->cpu.a);
	next_state(gb);
}

PORT(Func_1fa315)
{
	if (ld_and(gb, wMenuObj5AnimationEnded))
		AH(ActionHelp_HideObj5BlockDebris);
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x50))
		return;
	AH(ActionHelp_ShowWarioBump);
	var_0e_next(gb);
}

PORT(Func_1fa33b)
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
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ShowWarioIdle);
	clear(gb, wActionHelpWarioVar);
	timer_30_next(gb);
}

PORT(Func_1fa38c)
{
	wario_x_towards_40(gb, true);
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, wMenuObj4XCoord);
	wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
	gb->cpu.a = alu_sub(gb, rd8(gb, wMenuObj4XCoord), gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x1c);
	if (!(gb->cpu.f & FLAG_C))
		return;
	start_jump(gb);
}

PORT(Func_1fa3ab)
{
	wario_x_towards_40(gb, false);
	SET_HL(gb, wActionHelpWarioVar);
	jump_wario(gb);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C) {
		SET_HL(gb, wMenuObj4XCoord);
		wr8(gb, wMenuObj4XCoord, alu_dec(gb, rd8(gb, wMenuObj4XCoord)));
		return;
	}
	clear(gb, wActionHelpWarioVar);
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

PORT_JUMP(Func_1fa3e4) { last_state(gb, func_1f9d1f); }
