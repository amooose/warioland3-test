// Port of src/engine/action_help/power_ups/high_jump_boots.asm
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

PORT(HighJumpBootsActionHelp)
{
	run_state(gb, ADDR_HighJumpBootsActionHelp + 3, ADDR_HighJumpBootsActionHelp + 0x9d);

	if (ld_and(gb, wActionHelpBlackedOut))
		return;
	if (!ld_and(gb, wActionHelpFrozen)) {
		update_menu_obj_anim_ended(gb, wMenuObj5FramesetPtr, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
		update_menu_obj_anim_ended(gb, wMenuObj4FramesetPtr, wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		SET_HL(gb, wActionHelpWarioFramesetPtr + 1);
		FARCALL_BANK(0x05, UpdateObjAnim);
		gb->cpu.a = rd8(gb, wObjAnimWasReset);
		wr8(gb, wActionHelpWarioAnimationEnded, gb->cpu.a);
	}
	SET_HL(gb, wMenuObj5);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wMenuObj4);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wActionHelpWario);
	FARCALL(LoadActionHelpWarioSprite);
}

static void func_1fa94a(GB *gb)
{
	AH(ActionHelp_ShowInitialWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowParaGoomWalk);
	init_obj(gb, wMenuObj5, 0x40, 0xe8, ADDR_Frameset_1e6b99, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	FARCALL(LoadActionHelpPalsWithoutWater);
	next_state(gb);
}

PORT_JUMP(Func_1fa912) { end_action_help(gb, func_1fa94a); }
PORT(Func_1fa94a) { func_1fa94a(gb); }

PORT(Func_1fa945)
{
	if (!dec_timer(gb))
		return;
	func_1fa94a(gb);
}

PORT(Func_1fa984)
{
	if (!dec_cp(gb, wMenuObj5XCoord, 0x58))
		return;
	AH(ActionHelp_ShowWarioIdle);
	next_state(gb);
}

PORT(Func_1fa994)
{
	if (inc_cp(gb, wTimer, 0x30))
		return;
	wr8(gb, wTimer, 0x00);
	start_jump(gb);
}

enum { VEL_INC, VEL_POST_INC, VEL_CAPPED };

// ld hl, wActionHelpWarioVar / (inc [hl] / ld a, [hl]) or (ld a, [hl] /
// inc [hl]) or (inc [hl] / ld a, [hl] / cp $28 / jr c, :+ / ld [hl], $27 /
// ld a, [hl] / :) / ld e, a / ld d, $00 / ld hl, table / add hl, de /
// ld a, [wActionHelpWarioYCoord] / add [hl] / ld [wActionHelpWarioYCoord], a
static void jump_vel(GB *gb, uint16_t table, int mode)
{
	uint16_t var = wActionHelpWarioVar;
	if (mode == VEL_POST_INC) {
		gb->cpu.a = rd8(gb, var);
		wr8(gb, var, alu_inc(gb, rd8(gb, var)));
	} else {
		wr8(gb, var, alu_inc(gb, rd8(gb, var)));
		gb->cpu.a = rd8(gb, var);
		if (mode == VEL_CAPPED) {
			alu_cp(gb, gb->cpu.a, 0x28);
			if (!(gb->cpu.f & FLAG_C)) {
				wr8(gb, var, 0x27);
				gb->cpu.a = 0x27;
			}
		}
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, table, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wActionHelpWarioYCoord), rd8(gb, hl));
	wr8(gb, wActionHelpWarioYCoord, gb->cpu.a);
}

// ld hl, addr / dec [hl] (n times)
static void dec_x(GB *gb, uint16_t addr, int n)
{
	SET_HL(gb, addr);
	while (n--)
		wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
}

static void inc_x(GB *gb, uint16_t addr, int n)
{
	SET_HL(gb, addr);
	while (n--)
		wr8(gb, addr, alu_inc(gb, rd8(gb, addr)));
}

// ld a, [wActionHelpWarioVar] / cp $18: returns whether it's below
static bool var_below_18(GB *gb)
{
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x18);
	return gb->cpu.f & FLAG_C;
}

// ClearButtonsInput / var = $18 / next state
static void clear_buttons_var_18(GB *gb)
{
	AH(ActionHelp_ClearButtonsInput);
	ld_n(gb, wActionHelpWarioVar, 0x18);
	next_state(gb);
}

PORT(Func_1fa9a9)
{
	dec_x(gb, wMenuObj5XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_HighJump, VEL_INC);
	if (var_below_18(gb))
		return;
	clear_buttons_var_18(gb);
}

// lands on top at y $10: (ClearDPadInput or ShowDRightInput) / ShowWarioIdle /
// var = 0 / timer = $30 / next state
static void land_on_top(GB *gb, bool d_right)
{
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x10);
	if (d_right)
		AH(ActionHelp_ShowDRightInput);
	else
		AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ShowWarioIdle);
	clear(gb, wActionHelpWarioVar);
	timer_30_next(gb);
}

// the high jump up onto the block
static void high_jump_up(GB *gb)
{
	dec_x(gb, wMenuObj5XCoord, 2);
	jump_vel(gb, ADDR_JumpVelTable_HighJump, VEL_POST_INC);
	alu_cp(gb, gb->cpu.a, 0x10);
	land_on_top(gb, false);
}

PORT(Func_1fa9d3) { high_jump_up(gb); }
PORT(Func_1fac71) { high_jump_up(gb); }

PORT(Func_1faa07)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowWarioJump);
	ld_n(gb, wActionHelpWarioVar, 0x18);
	next_state(gb);
}

PORT(Func_1faa1c)
{
	dec_x(gb, wMenuObj5XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_HighJump, VEL_CAPPED);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ShowWarioIdle);
	clear(gb, wActionHelpWarioVar);
	next_state(gb);
}

PORT(Func_1faa51)
{
	if (!dec_cp(gb, wMenuObj4XCoord, 0x68))
		return;
	start_jump(gb);
}

PORT(Func_1faa64)
{
	dec_x(gb, wMenuObj4XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_INC);
	if (var_below_18(gb))
		return;
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x34);
	if (gb->cpu.f & FLAG_C)
		return;
	AH(ActionHelp_ShowDLeftInput);
	AH(ActionHelp_ShowParaGoomStun);
	AH(ActionHelp_ShowWarioJumpLeft);
	clear(gb, wActionHelpWarioVar);
	next_state(gb);
}

PORT(Func_1faa99)
{
	SET_HL(gb, wMenuObj5XCoord);
	gb->cpu.a = rd8(gb, wMenuObj5XCoord);
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wMenuObj5XCoord, alu_inc(gb, rd8(gb, wMenuObj5XCoord)));
		inc_x(gb, wMenuObj4XCoord, 1);
	}
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_INC);
	if (var_below_18(gb))
		return;
	clear_buttons_var_18(gb);
}

PORT(Func_1faacc)
{
	jump_wario(gb);
	alu_cp(gb, gb->cpu.a, 0x10);
	land_on_top(gb, true);
}

PORT(Func_1faafb)
{
	AH(ActionHelp_ClearDPadInput);
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowDRightAndDDownInput);
	AH(ActionHelp_ShowWarioJump);
	AH(ActionHelp_ShowWarioSmashAttack);
	ld_n(gb, wActionHelpWarioVar, 0x18);
	next_state(gb);
}

PORT(Func_1fab19)
{
	dec_x(gb, wMenuObj4XCoord, 1);
	dec_x(gb, wMenuObj5XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_CAPPED);
	alu_cp(gb, gb->cpu.a, 0x34);
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x30);
	AH(ActionHelp_ShowDLeftInput);
	AH(ActionHelp_ShowObj4EnemyVanish);
	AH(ActionHelp_ShowWarioJumpLeft);
	clear(gb, wActionHelpWarioVar);
	next_state(gb);
}

PORT(Func_1fab55)
{
	inc_x(gb, wMenuObj4XCoord, 1);
	inc_x(gb, wMenuObj5XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_INC);
	if (var_below_18(gb))
		return;
	clear_buttons_var_18(gb);
}

PORT(Func_1fab83)
{
	inc_x(gb, wMenuObj4XCoord, 2);
	inc_x(gb, wMenuObj5XCoord, 2);
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_INC);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x10);
	var_0e_next(gb);
}

PORT(Func_1fabb2)
{
	if (ld_and(gb, wMenuObj4AnimationEnded)) {
		SET_HL(gb, wMenuObj4);
		AH(ActionHelp_HideBlockDebris);
	}
	inc_x(gb, wMenuObj5XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_CAPPED);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ClearDPadInput);
	AH(ActionHelp_ShowWarioIdle);
	clear(gb, wActionHelpWarioVar);
	timer_30_next(gb);
}

PORT(Func_1fabf8)
{
	if (!dec_timer(gb))
		return;
	start_jump(gb);
}

PORT(Func_1fac08)
{
	jump_wario(gb);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		AH(ActionHelp_ClearButtonsInput);
		return;
	}
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	AH(ActionHelp_ShowAInput);
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ShowWarioLand);
	next_state(gb);
}

PORT(Func_1fac3c)
{
	if (!ld_and(gb, wActionHelpWarioAnimationEnded))
		return;
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

PORT(Func_1fac4c)
{
	dec_x(gb, wMenuObj5XCoord, 1);
	jump_vel(gb, ADDR_JumpVelTable_HighJump, VEL_INC);
	if (var_below_18(gb))
		return;
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

PORT(Func_1faca5)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowWarioJump);
	next_state(gb);
}

PORT_JUMP(Func_1facb8)
{
	dec_x(gb, wMenuObj5XCoord, 2);
	jump_vel(gb, ADDR_JumpVelTable_Normal, VEL_CAPPED);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x20);
	alu_cp(gb, gb->cpu.a, 0x60);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	end_action_help(gb, func_1fa94a);
}
