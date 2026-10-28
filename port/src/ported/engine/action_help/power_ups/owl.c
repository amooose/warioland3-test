// Port of src/engine/action_help/power_ups/owl.asm
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

PORT(OwlActionHelp)
{
	run_state(gb, ADDR_OwlActionHelp + 3, ADDR_OwlActionHelp + 0x54);

	if (ld_and(gb, wActionHelpBlackedOut))
		return;
	if (!ld_and(gb, wActionHelpFrozen)) {
		update_menu_obj_anim_ended(gb, wMenuObj4FramesetPtr, wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		update_menu_obj_anim(gb, wActionHelpWarioFramesetPtr, wActionHelpWarioOAMBank);
	}
	SET_HL(gb, wActionHelpWario);
	AH(LoadActionHelpWarioSprite_FromBank);
	SET_HL(gb, wMenuObj4);
	AH(LoadActionHelpSprite_Far);
}

static void func_1e409a(GB *gb)
{
	wr8(gb, wPendingDMASourceBank, BANK_WarioHangGfx);
	wr8(gb, wPendingDMASourcePtr + 0, (uint8_t)(ADDR_WarioHangGfx >> 8));
	wr8(gb, wPendingDMASourcePtr + 1, (uint8_t)ADDR_WarioHangGfx);
	wr8(gb, wPendingDMADestinationPtr + 0, (uint8_t)(v0Tiles0 >> 8));
	wr8(gb, wPendingDMADestinationPtr + 1, (uint8_t)v0Tiles0);
	wr8(gb, wPendingDMADestinationBank, BANK_v0Tiles0);
	wr8(gb, wPendingDMALength, 0x80 - 1);
	wr8(gb, wIsDMATransferPending, TRUE);

	uint16_t hl = wActionHelpWario;
	wr8(gb, hl++, 0x38);
	wr8(gb, hl++, 0xc0);
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(ADDR_Frameset_1fed7c >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_Frameset_1fed7c);
	clear(gb, hl);
	wr8(gb, wSFXTimer, 0);
	wr8(gb, wActionHelpWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_1fddb4 >> 8));
	wr8(gb, wActionHelpWarioOAMPtr + 1, (uint8_t)ADDR_OAM_1fddb4);
	wr8(gb, wActionHelpWarioOAMBank, BANK_OAM_1fddb4);

	SET_HL(gb, wMenuObj4);
	FARCALL(ActionHelp_HideBlockDebris);
	AH(LoadActionHelpPalsWithoutWater);
	next_state(gb);
}

// Ends the demonstration: back to the title screen, or restarts it.
static void end_owl(GB *gb)
{
	gb->cpu.a = rd8(gb, wPowerUpLevel);
	if (alu_bit(gb, gb->cpu.a, ACTION_HELP_TITLE_SCREEN_F)) {
		JUMP(InitWithoutDemoPowerUpReset);
		return;
	}
	AH(ActionHelp2_ClearDPadInput);
	AH(ActionHelp2_ClearButtonsInput);
	ld_n(gb, wActionHelpState, 0x01);
	func_1e409a(gb);
	RET();
}

PORT_JUMP(Func_1e407e) { end_owl(gb); }
PORT(Func_1e409a) { func_1e409a(gb); }

PORT(Func_1e4095)
{
	if (!dec_timer(gb))
		return;
	func_1e409a(gb);
}

// play_sfx_rept n, SFX_FLAP / ld a, [wActionHelpCounter] / and %1 / ret nz:
// returns false on the ret
static bool flap(GB *gb, uint8_t n, bool every_other)
{
	play_sfx_rept(gb, n, SFX_FLAP);
	if (!every_other)
		return true;
	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpCounter), 0x01);
	return !gb->cpu.a;
}

PORT(Func_1e4104)
{
	if (!flap(gb, 32, true))
		return;
	if (!dec_cp(gb, wActionHelpWarioXCoord, 0x60))
		return;
	AH(ActionHelp2_ShowDUpInput);
	AH(ActionHelp_ShowWarioHangVertical);
	next_state(gb);
}

PORT(Func_1e4134)
{
	if (!flap(gb, 32, true))
		return;
	dec_cp(gb, wActionHelpWario, 0x34);
	if (gb->cpu.f & FLAG_Z) {
		AH(ActionHelp2_ClearDPadInput);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x20);
	if (!(gb->cpu.f & FLAG_C))
		return;
	next_state(gb);
}

PORT(Func_1e4165)
{
	if (!flap(gb, 32, true))
		return;
	if (inc_cp(gb, wActionHelpWario, 0x38))
		return;
	AH(ActionHelp2_ShowDLeftInput);
	AH(ActionHelp_ShowWarioHangHorizontal);
	next_state(gb);
}

PORT(Func_1e4195)
{
	if (!flap(gb, 32, true))
		return;
	dec_cp(gb, wActionHelpWarioXCoord, 0x5c);
	if (gb->cpu.f & FLAG_Z) {
		AH(ActionHelp2_ClearDPadInput);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C))
		return;
	AH(ActionHelp_ShowWarioHangOwlRight);
	next_state(gb);
}

PORT(Func_1e41c9)
{
	if (!flap(gb, 32, true))
		return;
	if (inc_cp(gb, wActionHelpWarioXCoord, 0x60))
		return;
	clear(gb, wSFXTimer);
	AH(ActionHelp2_ShowDLeftInput);
	AH(ActionHelp_ShowWarioOwlMoveHang);
	next_state(gb);
}

PORT(Func_1e41fd)
{
	flap(gb, 16, false);
	if (!dec_cp(gb, wActionHelpWarioXCoord, 0x20))
		return;
	AH(ActionHelp2_ShowDRightInput);
	AH(ActionHelp_ShowWarioOwlMoveHangRight);
	next_state(gb);
}

PORT(Func_1e4227)
{
	flap(gb, 16, false);
	if (inc_cp(gb, wActionHelpWarioXCoord, 0x80))
		return;
	AH(ActionHelp2_ClearDPadInput);
	AH(ActionHelp_ShowWarioHangDrop);
	AH(ActionHelp_ShowOwlFlyUp);
	next_state(gb);
}

// ld hl, wMenuObj4YCoord / dec [hl] / dec [hl]
static void owl_fly_up(GB *gb)
{
	SET_HL(gb, wMenuObj4YCoord);
	wr8(gb, wMenuObj4YCoord, alu_dec(gb, rd8(gb, wMenuObj4YCoord)));
	wr8(gb, wMenuObj4YCoord, alu_dec(gb, rd8(gb, wMenuObj4YCoord)));
}

PORT(Func_1e4254)
{
	owl_fly_up(gb);
	if (inc_cp(gb, wActionHelpWario, 0x40))
		return;
	AH(ActionHelp2_ClearButtonsInput);
	AH(ActionHelp2_ShowWarioIdle);
	timer_30_next(gb);
}

PORT(Func_1e4271)
{
	owl_fly_up(gb);
	if (!dec_timer(gb))
		return;
	AH(ActionHelp2_ShowDRightInput);
	AH(ActionHelp2_ShowWarioWalk);
	next_state(gb);
}

PORT_JUMP(Func_1e4286)
{
	if (inc_cp(gb, wActionHelpWarioXCoord, 0xb0)) {
		RET();
		return;
	}
	end_owl(gb);
}
