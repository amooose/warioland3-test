// Port of src/engine/level/objects/spearhead.asm
#include "ported.h"

// Asm-mirroring helpers: each is one instruction on the real registers.
#define A (gb->cpu.a)
#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)
#define RETURN do { RET(); return; } while (0)
static inline void ld_hl(GB *gb, uint16_t v) { SET_HL(gb, v); }
static inline void ld_l(GB *gb, uint8_t v) { gb->cpu.l = v; }
static inline uint8_t rd_hl(GB *gb) { return rd8(gb, REG_HL(gb)); }
static inline void wr_hl(GB *gb, uint8_t v) { wr8(gb, REG_HL(gb), v); }
static inline uint8_t rd_hli(GB *gb) { uint16_t hl = REG_HL(gb); SET_HL(gb, hl + 1); return rd8(gb, hl); }
static inline uint8_t rd_hld(GB *gb) { uint16_t hl = REG_HL(gb); SET_HL(gb, hl - 1); return rd8(gb, hl); }
static inline void wr_hli(GB *gb, uint8_t v) { uint16_t hl = REG_HL(gb); wr8(gb, hl, v); SET_HL(gb, hl + 1); }
static inline void wr_hld(GB *gb, uint8_t v) { uint16_t hl = REG_HL(gb); wr8(gb, hl, v); SET_HL(gb, hl - 1); }
static inline void xor_a(GB *gb) { A = 0; gb->cpu.f = FLAG_Z; }
static inline void inc_l(GB *gb) { gb->cpu.l = alu_inc(gb, gb->cpu.l); }
static inline void dec_l(GB *gb) { gb->cpu.l = alu_dec(gb, gb->cpu.l); }
static inline uint8_t inc_mhl(GB *gb) { uint8_t v = alu_inc(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline uint8_t dec_mhl(GB *gb) { uint8_t v = alu_dec(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline void set_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) | (1 << bit))); }
static inline void res_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) & ~(1 << bit))); }
// ld a, [hl] / and HEAVY_OBJ / or x
static inline void heavy_or(GB *gb, uint8_t x) { A = alu_or(gb, alu_and(gb, rd_hl(gb), HEAVY_OBJ), x); }
// ld a, [hld] / rlca / ld_obj_frameset left, right
static inline void frameset_by_dir_hld(GB *gb, uint16_t nc, uint16_t c)
{
	A = alu_rlca(gb, rd_hld(gb));
	SET_DE(gb, CF ? c : nc);
}
// ld hl, wCurObjYPos / copy the position to hYPos / hXPos
static inline void obj_pos_to_hpos(GB *gb)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); wr8(gb, hYPosHi, A);
	A = rd_hli(gb); wr8(gb, hXPosLo, A);
	A = rd_hl(gb); wr8(gb, hXPosHi, A);
}

// rst JumpTable, with the table at `table`.
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// ld a, [wGroundShakeCounter] / cp $10 / jr c, .no: with the ground shaking,
// the state becomes 2 * half + (DIV & 1) and it returns.
static bool shaken(GB *gb, uint8_t half)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (CF)
		return false;
	A = alu_rra(gb, rd8(gb, rDIV));
	A = alu_rla(gb, half);
	wr8(gb, wCurObjState, A);
	RET();
	return true;
}

// ld l, OBJ_COLLBOX_RIGHT / ld a, right / ld [hld], a / ld a, left / ld [hld], a
static void collbox_rl(GB *gb, uint8_t right, uint8_t left)
{
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = right; wr_hld(gb, A);
	A = left; wr_hld(gb, A);
}

// ld l, OBJ_INTERACTION_TYPE / interaction, keeping HEAVY_OBJ / ld [hld], a
static void interaction_hld(GB *gb, uint8_t x)
{
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, x);
	wr_hld(gb, A);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a
static void set_state_hld(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
}

#define SH(off) (ADDR_SpearheadFunc + (off))
#define SH_INIT SH(0x0e)
#define SH_UPDATE SH(0x29)

PORT(SpearheadFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(SH_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)SH_INIT; wr_hld(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
}

PORT_LOCAL_JUMP(SpearheadFunc, Update)
{
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, SH_UPDATE + 4));
}

static void sh_full_throw_start(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_68292, ADDR_Frameset_6829b);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SpearheadFunc, FullThrowLeftStart)
{
	sh_full_throw_start(gb, OBJSTATE_FULL_THROW_LEFT);
	JUMP(ObjState_FullThrowLeft);
}

PORT_LOCAL_JUMP(SpearheadFunc, FullThrowRightStart)
{
	sh_full_throw_start(gb, OBJSTATE_FULL_THROW_RIGHT);
	JUMP(ObjState_FullThrowRight);
}

static void sh_sleep_start(GB *gb)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	ld_l(gb, OBJ_STATE);
	A = OBJSTATE_SPECIAL_1; wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_68234);
	CALL(SetObjectFramesetPtr);
	collbox_rl(gb, 5, (uint8_t)-6);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, SleepStart) { sh_sleep_start(gb); }

static void sh_default(GB *gb)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	ld_l(gb, OBJ_STATE);
	A = OBJSTATE_FALL; wr_hld(gb, A);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (!CF) {
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
		dec_l(gb);
		xor_a(gb);
		wr_hld(gb, A);
		collbox_rl(gb, 6, (uint8_t)-10);
		SET_DE(gb, ADDR_Frameset_681d2);
	} else {
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
		dec_l(gb);
		xor_a(gb);
		wr_hld(gb, A);
		collbox_rl(gb, 9, (uint8_t)-7);
		SET_DE(gb, ADDR_Frameset_681db);
	}
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SpearheadFunc, Default) { sh_default(gb); }

PORT_LOCAL_JUMP(SpearheadFunc, Init)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(SH_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)SH_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = alu_rra(gb, rd8(gb, wDayNight));
	if (CF)
		sh_sleep_start(gb);
	else
		sh_default(gb);
}

static void sh_turn_around_start(GB *gb)
{
	set_state_hld(gb, OBJSTATE_TURN_AROUND);
	frameset_by_dir_hld(gb, ADDR_Frameset_6824f, ADDR_Frameset_6825a);
	CALL(SetObjectFramesetPtr);
	A = 48; wr_hli(gb, A);
	A = 0x1c; wr_hl(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, TurnAroundStart) { sh_turn_around_start(gb); }

PORT_LOCAL_JUMP(SpearheadFunc, Sleep)
{
	ld_hl(gb, wCurObjSubState);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	// .asm_40199
	gb->cpu.c = 0x2a;
	A = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.b = A;
	A = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	A = alu_sub(gb, A, gb->cpu.b);
	alu_cp(gb, A, 32);
	bool wake = false;
	bool close_y = CF;
	if (!close_y) {
		alu_cp(gb, A, (uint8_t)-32);
		close_y = !CF;
	}
	if (close_y) {
		// .asm_401ad
		A = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
		gb->cpu.b = A;
		A = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
		A = alu_sub(gb, A, gb->cpu.b);
		alu_cp(gb, A, (uint8_t)-32);
		if (!CF) {
			wake = true;
		} else {
			alu_cp(gb, A, 32);
			wake = CF;
		}
	}
	if (wake) {
		// .wake_up
		A = alu_rlca(gb, rd_hli(gb));
		A = OBJSTATE_SPECIAL_3;
		wr_hl(gb, A);
		if (CF) {
			collbox_rl(gb, 9, (uint8_t)-7);
			SET_DE(gb, ADDR_Frameset_68287);
		} else {
			SET_DE(gb, ADDR_Frameset_6827c);
		}
		CALL(SetObjectFramesetPtr);
		A = 32; wr_hli(gb, A);
		RETURN;
	}
	// .check_shake
	if (shaken(gb, OBJSTATE_WOBBLE_LEFT_START >> 1))
		return;
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_3543, SH(0x1a2));
	if (!alu_and(gb, A, A)) {
		A = OBJSTATE_STANDING_FALL_START;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, WakeUp)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	sh_turn_around_start(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, StandingFallStart)
{
	set_state_hld(gb, OBJSTATE_STANDING_FALL);
	A = alu_rlca(gb, rd_hld(gb));
	bool c = CF;
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, c ? ADDR_Frameset_681db : ADDR_Frameset_681d2);
	CALL(SetObjectFramesetPtr);
	RETURN;
}

// .LandWithSting / .Land: interaction, state, collision box and frameset by
// direction, then the state duration
static void sh_land(GB *gb, uint8_t inter, uint8_t state, uint16_t fs_r, uint16_t fs_l, uint8_t dur)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, inter);
	wr_hld(gb, A);
	ld_l(gb, OBJ_STATE);
	A = state; wr_hld(gb, A);
	A = alu_rlca(gb, rd_hld(gb));
	if (!CF) {
		collbox_rl(gb, 6, (uint8_t)-10);
		SET_DE(gb, fs_r);
	} else {
		collbox_rl(gb, 9, (uint8_t)-7);
		SET_DE(gb, fs_l);
	}
	CALL(SetObjectFramesetPtr);
	A = dur; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, LandWithSting)
{
	sh_land(gb, OBJ_INTERACTION_FRONT_STING, OBJSTATE_SPECIAL_2, ADDR_Frameset_68208, ADDR_Frameset_68213, 32);
}

PORT_LOCAL_JUMP(SpearheadFunc, Land)
{
	A = alu_inc(gb, rd8(gb, wCurObjVar1));
	if (ZF)
		sh_land(gb, OBJ_INTERACTION_FRONT_STING, OBJSTATE_SPECIAL_2, ADDR_Frameset_68208, ADDR_Frameset_68213, 32);
	else
		sh_land(gb, OBJ_INTERACTION_01, OBJSTATE_WALK, ADDR_Frameset_681d2, ADDR_Frameset_681db, 10);
}

PORT_LOCAL_JUMP(SpearheadFunc, Angry)
{
	if (shaken(gb, OBJSTATE_WOBBLE_LEFT_START >> 1))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	inc_l(gb);
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_STATE);
	A = OBJSTATE_WALK; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_681d2, ADDR_Frameset_681db);
	CALL(SetObjectFramesetPtr);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, Walk)
{
	if (shaken(gb, OBJSTATE_WOBBLE_LEFT_START >> 1))
		return;
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		ld_l(gb, OBJ_INTERACTION_TYPE);
		heavy_or(gb, OBJ_INTERACTION_FRONT_STING);
		wr_hld(gb, A);
		RETURN;
	}
	// .has_sting
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_3543, SH(0x304));
	if (!alu_and(gb, A, A)) {
		A = OBJSTATE_STANDING_FALL_START;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hli(gb, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_sub(gb, A, 10);
		bool blocked = false;
		if (CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, SH(0x326));
			A = alu_and(gb, A, 0x0f);
			blocked = ZF;
		}
		if (!blocked) {
			ld_hl(gb, wCurObjYPos);
			A = alu_sub(gb, rd_hli(gb), 4);
			wr8(gb, hYPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0);
			wr8(gb, hYPosHi, A);
			A = alu_sub(gb, rd_hli(gb), 10);
			wr8(gb, hXPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, SH(0x344));
			if (!alu_and(gb, A, A)) {
				JUMP(MoveObjectLeft_Slow);
				return;
			}
		}
	} else {
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_add(gb, A, 9);
		alu_cp(gb, A, 16);
		bool blocked = false;
		if (!CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnRight, SH(0x35a));
			A = alu_and(gb, A, 0x0f);
			blocked = ZF;
		}
		if (!blocked) {
			ld_hl(gb, wCurObjYPos);
			A = alu_sub(gb, rd_hli(gb), 4);
			wr8(gb, hYPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0);
			wr8(gb, hYPosHi, A);
			A = alu_add(gb, rd_hli(gb), 9);
			wr8(gb, hXPosLo, A);
			A = alu_adc(gb, rd_hli(gb), 0);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, SH(0x378));
			if (!alu_and(gb, A, A)) {
				JUMP(MoveObjectRight_Slow);
				return;
			}
		}
	}
	A = OBJSTATE_TURN_AROUND_START;
	wr8(gb, wCurObjState, A);
	RETURN;
}

static void sh_bump_start(GB *gb, uint8_t state, uint16_t fs_r, uint16_t fs_l)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, fs_r, fs_l);
	CALL(SetObjectFramesetPtr);
	A = 12; wr_hld(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
	collbox_rl(gb, 5, (uint8_t)-6);
	interaction_hld(gb, OBJ_INTERACTION_01);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, BumpLeftStart)
{
	sh_bump_start(gb, OBJSTATE_BUMP_LEFT, ADDR_Frameset_6826e, ADDR_Frameset_681f6);
}

PORT_LOCAL_JUMP(SpearheadFunc, BumpRightStart)
{
	sh_bump_start(gb, OBJSTATE_BUMP_RIGHT, ADDR_Frameset_681ff, ADDR_Frameset_68265);
}

// ld a, [hl] / and a / jp nz, move / ld a, state / ld [wCurObjState], a / ret
static void sh_var2_move(GB *gb, bool left, uint8_t state)
{
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (left)
			JUMP(Func_32ae);
		else
			JUMP(Func_32bd);
		return;
	}
	A = state;
	wr8(gb, wCurObjState, A);
	RETURN;
}

static void sh_stun_start(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_681e4, ADDR_Frameset_681ed);
	CALL(SetObjectFramesetPtr);
	collbox_rl(gb, 5, (uint8_t)-6);
	interaction_hld(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL_JUMP(SpearheadFunc, StunLeftStart)
{
	sh_stun_start(gb, OBJSTATE_STUN_LEFT);
	ld_hl(gb, wCurObjVar2);
	sh_var2_move(gb, true, OBJSTATE_0F);
}

PORT_LOCAL_JUMP(SpearheadFunc, StunLeft)
{
	ld_hl(gb, wCurObjVar2);
	sh_var2_move(gb, true, OBJSTATE_0F);
}

PORT_LOCAL_JUMP(SpearheadFunc, StunRightStart)
{
	sh_stun_start(gb, OBJSTATE_STUN_RIGHT);
	ld_hl(gb, wCurObjVar2);
	sh_var2_move(gb, false, OBJSTATE_0F);
}

PORT_LOCAL_JUMP(SpearheadFunc, StunRight)
{
	ld_hl(gb, wCurObjVar2);
	sh_var2_move(gb, false, OBJSTATE_0F);
}

static void sh_40478(GB *gb)
{
	frameset_by_dir_hld(gb, ADDR_Frameset_681e4, ADDR_Frameset_681ed);
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	collbox_rl(gb, 5, (uint8_t)-6);
	interaction_hld(gb, OBJ_INTERACTION_00);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, State14)
{
	set_state_hld(gb, OBJSTATE_44);
	sh_40478(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, State15)
{
	set_state_hld(gb, OBJSTATE_45);
	sh_40478(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, Func_40478) { sh_40478(gb); }

static void sh_state3f(GB *gb)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		A = OBJSTATE_1C;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_32ea);
		return;
	}
	A = OBJSTATE_10;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, State0f)
{
	A = OBJSTATE_3F;
	wr8(gb, wCurObjState, A);
	A = 100;
	wr8(gb, wCurObjStateDuration, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	sh_state3f(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, State3f) { sh_state3f(gb); }

static void sh_state40(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_32f9);
		return;
	}
	A = OBJSTATE_00;
	wr8(gb, wCurObjState, A);
	A = 0xff;
	wr8(gb, wCurObjVar1, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, State10)
{
	set_state_hld(gb, OBJSTATE_40);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	A = 7;
	wr8(gb, wCurObjStateDuration, A);
	sh_state40(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, State40) { sh_state40(gb); }

PORT_LOCAL_JUMP(SpearheadFunc, TurnAround)
{
	if (shaken(gb, OBJSTATE_WOBBLE_LEFT_START >> 1))
		return;
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb)) {
			JUMP(Func_3308);
			return;
		}
		collbox_rl(gb, 5, (uint8_t)-6);
		interaction_hld(gb, OBJ_INTERACTION_01);
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, CF ? ADDR_Frameset_68229 : ADDR_Frameset_6821e);
		CALL(SetObjectFramesetPtr);
		JUMP(Func_3308);
		return;
	}
	// .asm_4054e
	inc_l(gb);
	if (dec_mhl(gb)) {
		JUMP(Func_3308);
		return;
	}
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	set_state_hld(gb, OBJSTATE_LAND);
	if (CF)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

static void sh_4057d(GB *gb)
{
	wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_681e4, ADDR_Frameset_681ed);
	CALL(SetObjectFramesetPtr);
	A = 4; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	collbox_rl(gb, 5, (uint8_t)-6);
	interaction_hld(gb, OBJ_INTERACTION_00);
	RETURN;
}

static void sh_wobble_start(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjAction);
	A = 15 | (1 << 7); wr_hld(gb, A);
	A = state;
	sh_4057d(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, WobbleLeftStart) { sh_wobble_start(gb, OBJSTATE_WOBBLE_LEFT); }
PORT_LOCAL_JUMP(SpearheadFunc, WobbleRightStart) { sh_wobble_start(gb, OBJSTATE_WOBBLE_RIGHT); }

static void sh_405b6(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearheadFunc, State0b) { sh_405b6(gb, OBJSTATE_3B); }
PORT_LOCAL_JUMP(SpearheadFunc, State0c) { sh_405b6(gb, OBJSTATE_3C); }

static void sh_state3b(GB *gb, bool left)
{
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	ld_l(gb, OBJ_VAR_2);
	sh_var2_move(gb, left, OBJSTATE_TURN_AROUND_START);
}

PORT_LOCAL_JUMP(SpearheadFunc, State3b) { sh_state3b(gb, false); }
PORT_LOCAL_JUMP(SpearheadFunc, State3c) { sh_state3b(gb, true); }

static void sh_attacked(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	gb->cpu.b = 0x02;
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.b);
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	SET_DE(gb, ADDR_Frameset_68277);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SpearheadFunc, State29) { sh_attacked(gb, OBJSTATE_59); }
PORT_LOCAL_JUMP(SpearheadFunc, State28) { sh_attacked(gb, OBJSTATE_58); }
PORT_LOCAL_JUMP(SpearheadFunc, AttackedLeftStart) { sh_attacked(gb, OBJSTATE_ATTACKED_LEFT); }
PORT_LOCAL_JUMP(SpearheadFunc, AttackedRightStart) { sh_attacked(gb, OBJSTATE_ATTACKED_RIGHT); }

static void sh_picked_up(GB *gb, uint8_t state, uint16_t data)
{
	set_state_hld(gb, state);
	dec_l(gb);
	A = (uint8_t)(data >> 8); wr_hld(gb, A);
	A = (uint8_t)data; wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(SpearheadFunc, PickedUpLeftStart)
{
	sh_picked_up(gb, OBJSTATE_PICKED_UP_LEFT, ADDR_Data_601c0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(SpearheadFunc, PickedUpRightStart)
{
	sh_picked_up(gb, OBJSTATE_PICKED_UP_RIGHT, ADDR_Data_601e0);
	JUMP(Func_3344);
}

static void sh_grabbed(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_681e4, ADDR_Frameset_681ed);
	CALL(SetObjectFramesetPtr);
	collbox_rl(gb, 5, (uint8_t)-6);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_GRABBED_F);
}

static void sh_flip(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
}

static void sh_state48(GB *gb);

static void sh_state49(GB *gb)
{
	A = rd8(gb, wDirection);
	if (alu_and(gb, A, A)) {
		JUMP(Func_3353);
		return;
	}
	sh_flip(gb);
	sh_grabbed(gb, OBJSTATE_48);
	sh_state48(gb);
}

static void sh_state48(GB *gb)
{
	A = rd8(gb, wDirection);
	if (!alu_and(gb, A, A)) {
		JUMP(Func_3362);
		return;
	}
	sh_flip(gb);
	sh_grabbed(gb, OBJSTATE_49);
	sh_state49(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, State19)
{
	sh_grabbed(gb, OBJSTATE_49);
	sh_state49(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, State49) { sh_state49(gb); }

PORT_LOCAL_JUMP(SpearheadFunc, State18)
{
	sh_grabbed(gb, OBJSTATE_48);
	sh_state48(gb);
}

PORT_LOCAL_JUMP(SpearheadFunc, State48) { sh_state48(gb); }

PORT_LOCAL_JUMP(SpearheadFunc, State12)
{
	ld_hl(gb, wCurObjAction);
	A = 31 | (1 << 7); wr_hld(gb, A);
	A = OBJSTATE_42; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	interaction_hld(gb, OBJ_INTERACTION_00);
	RETURN;
}
