// Port of src/engine/action_help/power_ups/lead_overalls.asm
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

PORT(LeadOverallsActionHelp)
{
	run_state(gb, ADDR_LeadOverallsActionHelp + 3, ADDR_LeadOverallsActionHelp + 0x97);

	if (ld_and(gb, wActionHelpBlackedOut))
		return;
	if (!ld_and(gb, wActionHelpFrozen)) {
		update_menu_obj_anim_ended(gb, wMenuObj4FramesetPtr, wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		SET_HL(gb, wActionHelpWarioFramesetPtr + 1);
		FARCALL_BANK(0x05, UpdateObjAnim);
	}
	SET_HL(gb, wMenuObj4);
	FARCALL(LoadActionHelpSprite_Far);
	SET_HL(gb, wActionHelpWario);
	FARCALL(LoadActionHelpWarioSprite);
	update_menu_obj_anim_ended(gb, wMenuObj5FramesetPtr, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
	SET_HL(gb, wMenuObj5);
	FARCALL(LoadActionHelpSprite_Far);
}

static void func_1f89c0(GB *gb)
{
	AH(ActionHelp_ShowInitialWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowParaGoomWalk);
	AH(ActionHelp_HideObj5BlockDebris);
	FARCALL(LoadActionHelpPalsWithoutWater);
	next_state(gb);
}

PORT_JUMP(Func_1f8988) { end_action_help(gb, func_1f89c0); }
PORT(Func_1f89c0) { func_1f89c0(gb); }

PORT(Func_1f89bb)
{
	if (!dec_timer(gb))
		return;
	func_1f89c0(gb);
}

PORT(Func_1f89e0)
{
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x60))
		return;
	start_jump(gb);
}

PORT(Func_1f8aa1)
{
	if (!dec_twice_odd_cp(gb, wMenuObj4XCoord, 0x60))
		return;
	start_jump(gb);
}

// jump_wario / dec_twice_odd obj4 x / ld a, [wActionHelpWarioXCoord] /
// cp [hl]: returns whether Wario is left of obj4
static bool jump_towards_obj4(GB *gb)
{
	jump_wario(gb);
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioXCoord);
	alu_cp(gb, gb->cpu.a, rd8(gb, wMenuObj4XCoord));
	return gb->cpu.f & FLAG_C;
}

PORT(Func_1f89fb)
{
	if (jump_towards_obj4(gb))
		return;
	AH(ActionHelp_ShowWarioSmashAttack);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

// jump_wario / cp n: returns whether Wario's y is below n (carry)
static bool jump_cp(GB *gb, uint8_t n)
{
	jump_wario(gb);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

PORT(Func_1f8a2a)
{
	if (jump_cp(gb, 0x34))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x34);
	ld_n(gb, wActionHelpWarioVar, 0x0a);
	AH(ActionHelp_ShowObj4EnemyVanish);
	next_state(gb);
}

PORT(Func_1f8a52)
{
	if (jump_cp(gb, 0x40))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ShowWarioGroundPound);
	next_state(gb);
}

PORT(Func_1f8a79)
{
	if (!ld_and(gb, wMenuObj4AnimationEnded))
		return;
	AH(ActionHelp_ShowVerticalBlockPair);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ClearDPadInput);
	timer_30_next(gb);
}

PORT(Func_1f8a91) { wait_then_walk(gb); }

PORT(Func_1f8abc)
{
	if (jump_towards_obj4(gb))
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
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ClearButtonsInput);
	timer_30_next(gb);
}

PORT(Func_1f8b04)
{
	if (!dec_timer(gb))
		return;
	start_jump(gb);
}

PORT(Func_1f8b14)
{
	if (!jump_cp(gb, 0x0c))
		return;
	ld_n(gb, wActionHelpWarioVar, 0x18);
	AH(ActionHelp_ShowWarioSmashAttack);
	AH(ActionHelp_ShowDDownInput);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

PORT(Func_1f8b3d)
{
	if (jump_cp(gb, 0x20))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x20);
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ShowWarioGroundPound);
	AH(ActionHelp_ShowSingleSolidBlock);
	gb->cpu.b = 0x10;
	AH(ActionHelp_ShowBlockDebrisWithYOffset);
	ld_n(gb, wTimer, 0x10);
	next_state(gb);
}

PORT(Func_1f8b71)
{
	if (!dec_timer(gb))
		return;
	ld_n(gb, wActionHelpWarioVar, 0x18);
	AH(ActionHelp_ShowWarioSmashAttack);
	AH(ActionHelp_HideObj5BlockDebris);
	next_state(gb);
}

PORT(Func_1f8b86)
{
	if (jump_cp(gb, 0x30))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x30);
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ShowWarioGroundPound);
	ld_n(gb, wTimer, 0x10);
	next_state(gb);
}

PORT(Func_1f8bb2)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ClearDPadInput);
	timer_30_next(gb);
}

PORT(Func_1f8bc7)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowDRightInput);
	start_jump(gb);
}

PORT(Func_1f8bda)
{
	SET_HL(gb, wActionHelpWarioVar);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x27);
	if (!(gb->cpu.f & FLAG_Z))
		wr8(gb, wActionHelpWarioVar, alu_inc(gb, rd8(gb, wActionHelpWarioVar)));
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	gb->cpu.a = alu_add(gb, rd8(gb, wActionHelpWarioYCoord), rd8(gb, hl));
	wr8(gb, wActionHelpWarioYCoord, gb->cpu.a);
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = rd8(gb, wActionHelpWarioYCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C)
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	clear(gb, wActionHelpWarioVar);
	AH(ActionHelp_ClearButtonsInput);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowDRightInput);
	// Futamogu sprite
	hl = init_obj(gb, wMenuObj4, 0x50, 0xc0, ADDR_Frameset_1e6f66, ADDR_OAM_1e6bab, BANK_OAM_1e6bab);
	clear(gb, hl);
	hl = init_obj(gb, wMenuObj5, 0x40, 0xb8, ADDR_Frameset_1e6b8b, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	clear(gb, hl);
	next_state(gb);
}

// dec_twice_odd obj4 x / ld a, [hl] / add -$8 / ld [wMenuObj5XCoord], a /
// cp n: returns the carry
static bool move_futamogu(GB *gb, uint8_t n)
{
	dec_twice_odd(gb, wMenuObj4XCoord);
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4XCoord), (uint8_t)-8);
	wr8(gb, wMenuObj5XCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

PORT(Func_1f8c59)
{
	if (!move_futamogu(gb, 0x40))
		return;
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ShowFutamoguAngry);
	AH(ActionHelp_ClearDPadInput);
	timer_30_next(gb);
}

PORT(Func_1f8c81)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowAInput);
	AH(ActionHelp_ShowWarioJump);
	AH(ActionHelp_ShowFutamoguHappy);
	next_state(gb);
}

PORT(Func_1f8c94)
{
	jump_wario(gb);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_C)
		return;
	AH(ActionHelp_ShowWarioSmashAttack);
	AH(ActionHelp_ShowDDownInput);
	AH(ActionHelp_ClearButtonsInput);
	next_state(gb);
}

// ld a, [wMenuObj4YCoord] / add $04 / ld [wMenuObj4YCoord], a
static void drop_futamogu(GB *gb)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj4YCoord), 0x04);
	wr8(gb, wMenuObj4YCoord, gb->cpu.a);
}

// ld hl, wMenuObj4FramesetOffset / xor a / ld [hli], a x2 / frameset with
// ld [hli], a
static void obj4_frameset(GB *gb, uint16_t fs)
{
	uint16_t hl = wMenuObj4FramesetOffset;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(fs >> 8));
	gb->cpu.a = (uint8_t)fs;
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(Func_1f8cbb)
{
	if (jump_cp(gb, 0x40))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x40);
	AH(ActionHelp_ShowWarioGroundPound);
	drop_futamogu(gb);
	obj4_frameset(gb, ADDR_Frameset_1e6f7d);
	ld_n(gb, wTimer, 0x10);
	next_state(gb);
}

PORT(Func_1f8cf7)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowWarioSmashAttack);
	next_state(gb);
}

PORT(Func_1f8d04)
{
	if (jump_cp(gb, 0x44))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x44);
	AH(ActionHelp_ShowWarioGroundPound);
	drop_futamogu(gb);
	ld_n(gb, wTimer, 0x10);
	next_state(gb);
}

PORT(Func_1f8d34)
{
	if (!dec_timer(gb))
		return;
	ld_n(gb, wActionHelpWarioVar, 0x18);
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ClearDPadInput);
	next_state(gb);
}

PORT(Func_1f8d49)
{
	if (jump_cp(gb, 0x48))
		return;
	ld_n(gb, wActionHelpWarioYCoord, 0x48);
	clear(gb, wActionHelpWarioVar);
	ld_n(gb, wTimer, 0x20);
	next_state(gb);
}

PORT(Func_1f8d72)
{
	if (!dec_timer(gb))
		return;
	// show Futamogu climbing
	obj4_frameset(gb, ADDR_Frameset_1e6f6b);
	next_state(gb);
}

PORT(Func_1f8d88)
{
	SET_HL(gb, wMenuObj4YCoord);
	wr8(gb, wMenuObj4YCoord, alu_dec(gb, rd8(gb, wMenuObj4YCoord)));
	dec_cp(gb, wActionHelpWarioYCoord, 0x40);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	AH(ActionHelp_ShowWarioIdle);
	AH(ActionHelp_ShowFutamoguAngry);
	timer_30_next(gb);
}

PORT(Func_1f8da4)
{
	if (!dec_timer(gb))
		return;
	AH(ActionHelp_ShowDRightInput);
	AH(ActionHelp_ShowWarioWalk);
	AH(ActionHelp_ShowFutamoguHappy);
	next_state(gb);
}

PORT_JUMP(Func_1f8db7)
{
	if (move_futamogu(gb, 0xf0)) {
		RET();
		return;
	}
	end_action_help(gb, func_1f89c0);
}
