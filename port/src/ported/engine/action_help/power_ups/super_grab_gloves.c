// Port of src/engine/action_help/power_ups/super_grab_gloves.asm
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

PORT(SuperGrabGloveActionHelp)
{
	run_state(gb, ADDR_SuperGrabGloveActionHelp + 3, ADDR_SuperGrabGloveActionHelp + 0x97);

	if (ld_and(gb, wActionHelpBlackedOut))
		return;
	if (!ld_and(gb, wActionHelpFrozen)) {
		update_menu_obj_anim_ended(gb, wMenuObj4FramesetPtr, wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		SET_HL(gb, wActionHelpWarioFramesetPtr + 1);
		FARCALL_BANK(0x05, UpdateObjAnim);
		gb->cpu.a = rd8(gb, wObjAnimWasReset);
		wr8(gb, wActionHelpWarioAnimationEnded, gb->cpu.a);
	}
	SET_HL(gb, wMenuObj4YCoord);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wActionHelpWario);
	FARCALL(LoadActionHelpWarioSprite);
	update_menu_obj_anim(gb, wMenuObj6FramesetPtr, wMenuObj6FramesetBank);
	SET_HL(gb, wMenuObj6);
	FARCALL(LoadActionHelpSprite_Far);
}

static void func_1fb1b1(GB *gb)
{
	AH(ActionHelp_ShowInitialWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowCountRichtertoffIdle);
	AH(ActionHelp_ShowCountRichtertoffStun);
	AH(ActionHelp_ShowOverheadPillar);
	FARCALL(LoadActionHelpPalsWithoutWater);
	next_state(gb);
}

PORT_JUMP(Func_1fb179) { end_action_help(gb, func_1fb1b1); }
PORT(Func_1fb1b1) { func_1fb1b1(gb); }

PORT(Func_1fb1ac)
{
	if (!dec_timer(gb))
		return;
	func_1fb1b1(gb);
}

// ld hl, wActionHelpWarioYCoord / ld de, wMenuObj4YCoord / ld a, [hli] /
// sub y / ld [de], a / inc de / ld a, [hl] / add x / ld [de], a
static void hold_obj4(GB *gb, uint8_t y, uint8_t x)
{
	uint16_t hl = wActionHelpWarioYCoord;
	uint16_t de = wMenuObj4YCoord;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), y);
	wr8(gb, de++, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), x);
	wr8(gb, de, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, de);
}

// walks to Count Richtertoff and grabs him
static void grab(GB *gb)
{
	if (!dec_cp(gb, wMenuObj4XCoord, 0x48))
		return;
	hold_obj4(gb, 0x05, 0x10);
	AH(ActionHelp_ShowWarioSlowGrab);
	AH(ActionHelp_ClearDPadInput);
	next_state(gb);
}

PORT(Func_1fb1d4) { grab(gb); }
PORT(Func_1fb302) { grab(gb); }
PORT(Func_1fb44e) { grab(gb); }

// lifts Count Richtertoff over Wario's head
static void lift(GB *gb)
{
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_inc(gb, rd8(gb, wTimer)));
	gb->cpu.a = rd8(gb, wTimer);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (gb->cpu.f & FLAG_Z) {
		hold_obj4(gb, 0x04, 0x10);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x0d);
	if (gb->cpu.f & FLAG_Z) {
		hold_obj4(gb, 0x14, 0x10);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.f & FLAG_Z) {
		hold_obj4(gb, 0x1c, 0x06);
		return;
	}
	if (!ld_and(gb, wActionHelpWarioAnimationEnded))
		return;
	wr8(gb, wTimer, 0x30);
	AH(ActionHelp_ShowWarioBeginGrab);
	next_state(gb);
}

PORT(Func_1fb1f6) { lift(gb); }
PORT(Func_1fb324) { lift(gb); }
PORT(Func_1fb470) { lift(gb); }

PORT(Func_1fb248)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioBeginThrow);
	AH(ActionHelp_ShowBInput);
	ld_n(gb, wTimer, 0x20);
	next_state(gb);
}

// dec_timer / ShowWarioThrow / ShowCountRichtertoffThrown /
// ClearButtonsInput / next state
static void throw_count(GB *gb)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioThrow);
	AH(ActionHelp_ShowCountRichtertoffThrown);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

PORT(Func_1fb25d) { throw_count(gb); }
PORT(Func_1fb397) { throw_count(gb); }

static void idle_if_anim_ended(GB *gb)
{
	if (ld_and(gb, wActionHelpWarioAnimationEnded))
		AH(ActionHelp_ShowWarioIdle);
}

// dec [hl] / inc [hl] n times on wMenuObj4YCoord (hl already set)
static void dec_n(GB *gb, int n)
{
	while (n--)
		wr8(gb, wMenuObj4YCoord, alu_dec(gb, rd8(gb, wMenuObj4YCoord)));
}

static void inc_n(GB *gb, int n)
{
	while (n--)
		wr8(gb, wMenuObj4YCoord, alu_inc(gb, rd8(gb, wMenuObj4YCoord)));
}

// The arc of a normal throw.
PORT(Func_1fb270)
{
	idle_if_anim_ended(gb);
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_inc(gb, rd8(gb, wTimer)));
	gb->cpu.a = rd8(gb, wTimer);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4XCoord), 0x02);
		wr8(gb, wMenuObj4XCoord, gb->cpu.a);
		gb->cpu.a = rd8(gb, wTimer);
		alu_cp(gb, gb->cpu.a, 0x03);
		if (gb->cpu.f & FLAG_C)
			return;
		SET_HL(gb, wMenuObj4YCoord);
		alu_cp(gb, gb->cpu.a, 0x06);
		if (gb->cpu.f & FLAG_C) {
			inc_n(gb, 1);
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_C) {
			inc_n(gb, 2);
			return;
		}
		gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), 0x03);
		wr8(gb, wMenuObj4YCoord, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 0x40);
		if (gb->cpu.f & FLAG_C)
			return;
		wr8(gb, wMenuObj4YCoord, 0x40);
		ld_n(gb, wTimer, 0x10);
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4XCoord), 0x01);
	wr8(gb, wMenuObj4XCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTimer);
	SET_HL(gb, wMenuObj4YCoord);
	static const uint8_t lim[3] = {0x12, 0x14, 0x15};
	for (int i = 0; i < 3; i++) {
		alu_cp(gb, gb->cpu.a, lim[i]);
		if (gb->cpu.f & FLAG_C) {
			dec_n(gb, 3 - i);
			return;
		}
	}
	alu_cp(gb, gb->cpu.a, 0x16);
	if (gb->cpu.f & FLAG_C) {
		ld_n(gb, wTimer, 0x20);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x27);
	if (gb->cpu.f & FLAG_C) {
		inc_n(gb, 1);
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), 0x02);
	wr8(gb, wMenuObj4YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	wr8(gb, wMenuObj4YCoord, 0x40);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ShowCountRichtertoffStun);
	ld_n(gb, wTimer, 0x20);
	next_state(gb);
}

PORT(Func_1fb2f2) { wait_then_walk(gb); }
PORT(Func_1fb43e) { wait_then_walk(gb); }

PORT(Func_1fb376)
{
	if (!dec_timer(gb))
		return;
	wr8(gb, wTimer, 0x20);
	AH(ActionHelp_ShowDUpInput);
	next_state(gb);
}

PORT(Func_1fb385)
{
	if (!dec_timer(gb))
		return;
	wr8(gb, wTimer, 0x30);
	AH(ActionHelp_ShowWarioBeginThrow);
	AH(ActionHelp_ShowBInput);
	next_state(gb);
}

// The arc of a high throw.
PORT(Func_1fb3aa)
{
	idle_if_anim_ended(gb);
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_inc(gb, rd8(gb, wTimer)));
	gb->cpu.a = rd8(gb, wTimer);
	alu_cp(gb, gb->cpu.a, 0x30);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4XCoord), 0x01);
		wr8(gb, wMenuObj4XCoord, gb->cpu.a);
		gb->cpu.a = rd8(gb, wTimer);
		SET_HL(gb, wMenuObj4YCoord);
		static const uint8_t up[4] = {0x07, 0x0d, 0x0f, 0x11};
		for (int i = 0; i < 4; i++) {
			alu_cp(gb, gb->cpu.a, up[i]);
			if (gb->cpu.f & FLAG_C) {
				dec_n(gb, 4 - i);
				return;
			}
		}
		alu_cp(gb, gb->cpu.a, 0x14);
		if (gb->cpu.f & FLAG_C)
			return;
		static const uint8_t down[3] = {0x17, 0x1a, 0x1d};
		for (int i = 0; i < 3; i++) {
			alu_cp(gb, gb->cpu.a, down[i]);
			if (gb->cpu.f & FLAG_C) {
				inc_n(gb, i + 1);
				return;
			}
		}
		gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), 0x04);
		wr8(gb, wMenuObj4YCoord, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 0x40);
		if (gb->cpu.f & FLAG_C)
			return;
		wr8(gb, wMenuObj4YCoord, 0x40);
		ld_n(gb, wTimer, 0x30);
		return;
	}
	gb->cpu.a = rd8(gb, wTimer);
	SET_HL(gb, wMenuObj4YCoord);
	static const uint8_t lim[3] = {0x32, 0x34, 0x35};
	for (int i = 0; i < 3; i++) {
		alu_cp(gb, gb->cpu.a, lim[i]);
		if (gb->cpu.f & FLAG_C) {
			dec_n(gb, 3 - i);
			return;
		}
	}
	alu_cp(gb, gb->cpu.a, 0x36);
	if (gb->cpu.f & FLAG_C) {
		ld_n(gb, wTimer, 0x40);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x47);
	if (gb->cpu.f & FLAG_C) {
		inc_n(gb, 1);
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), 0x02);
	wr8(gb, wMenuObj4YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	wr8(gb, wMenuObj4YCoord, 0x40);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ShowCountRichtertoffStun);
	timer_30_next(gb);
}

PORT(Func_1fb4c2)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioGrabWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT(Func_1fb4d2)
{
	if (!dec_cp(gb, wMenuObj6XCoord, 0x80))
		return;
	AH(ActionHelp_ShowWarioBeginThrow);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ShowBInput);
	ld_n(gb, wTimer, 0x50);
	next_state(gb);
}

PORT(Func_1fb4ed)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioChargeThrow);
	next_state(gb);
}

PORT(Func_1fb4fa)
{
	play_sfx_rept(gb, 30, SFX_FULLY_CHARGED_THROW);
	if (!ld_and(gb, wActionHelpWarioAnimationEnded))
		return;
	AH(ActionHelp_ShowWarioFullyChargedGrab);
	gb->cpu.a = alu_sub(gb, rd8(gb, wMenuObj4XCoord), 0x08);
	wr8(gb, wMenuObj4XCoord, gb->cpu.a);
	timer_30_next(gb);
}

PORT(Func_1fb52b)
{
	play_sfx_rept(gb, 30, SFX_FULLY_CHARGED_THROW);
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioFullyChargedThrow);
	AH(ActionHelp_ShowCountRichtertoffThrown);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

PORT(Func_1fb555)
{
	idle_if_anim_ended(gb);
	gb->cpu.a = alu_sub(gb, rd8(gb, wMenuObj6XCoord), 0x10);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4XCoord), 0x04);
	wr8(gb, wMenuObj4XCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		return;
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ShowObj4EnemyVanish);
	next_state(gb);
}

PORT(Func_1fb579)
{
	if (!ld_and(gb, wMenuObj4AnimationEnded))
		return;
	ld_n(gb, wMenuObj4XCoord, 0xe8);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	next_state(gb);
}

PORT_JUMP(Func_1fb58e)
{
	if (dec_twice_odd_cp(gb, wMenuObj6XCoord, 0xe8)) {
		RET();
		return;
	}
	end_action_help(gb, func_1fb1b1);
}
