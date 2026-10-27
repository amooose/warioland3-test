// Port of src/engine/level/objects/count_richtertoffen.asm
#include "ported.h"

#define CR(off) ((uint16_t)(ADDR_CountRichtertoffenFunc + (off)))
#define CR_UPDATE CR(0x1f) // .Update

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

static void set_bits(GB *gb, uint16_t hl, uint8_t set, uint8_t res)
{
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~res) | set));
}

static void set_frameset(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
}

// ld [hli], a with the hl SetObjectFramesetPtr left
static void ld_hli(GB *gb, uint8_t v)
{
	gb->cpu.a = v;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
}

static void set_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a
static uint16_t state_hld(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	return hl;
}

// dec l / xor a / ld [hld], a (OBJ_VAR_3, with hl = OBJ_SUBSTATE)
static void clear_var3(GB *gb)
{
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
}

// ld l, OBJ_INTERACTION_TYPE / ld a, [hl] / and HEAVY_OBJ / or type / ld [hld], a
static void set_interaction(GB *gb, uint8_t type)
{
	uint16_t hl = lo_hl(gb, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// rst JumpTable: a = index, table = the address after the rst
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

// dec [hl] at hl: true if it reached 0
static bool dec_hl(GB *gb, uint16_t hl)
{
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

// [wCurObjPos] into hYPos / hXPos (hl ends at the x position's high byte)
static void copy_pos(GB *gb, uint16_t hl)
{
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// ld a, [hli] / add or sub delta / ldh [lo], a / ld a, [hli] / adc or sbc 0 / ldh [hi], a
static uint16_t offset16(GB *gb, uint16_t hl, int delta, uint16_t lo, uint16_t hi)
{
	if (delta < 0) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-delta);
		wr8(gb, lo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)delta);
		wr8(gb, lo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hi, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

static void cr_default(GB *gb)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_RICHTERTOFFEN);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = OBJSTATE_FALL;
	wr8(gb, hl--, OBJSTATE_FALL);
	SET_HL(gb, hl);
	clear_var3(gb);
	set_frameset(gb, ADDR_Frameset_683e1);
}

PORT_JUMP(CountRichtertoffenFunc)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	hl = lo_hl(gb, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(CR_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)CR_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 7);
	wr8(gb, hl--, (uint8_t)-8); // OBJ_COLLBOX_LEFT
	gb->cpu.a = (uint8_t)-4;
	wr8(gb, hl, gb->cpu.a); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	cr_default(gb);
	RET();
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, CR(0x23)));
}

PORT_LOCAL(CountRichtertoffenFunc, Default) { cr_default(gb); }

// .asm_41255: wobbles
static void wobble(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 15 | (1 << 7);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = state;
	wr8(gb, hl--, state); // OBJ_STATE
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_683d9);
	ld_hli(gb, 4); // OBJ_STATE_DURATION
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02); // OBJ_VAR_2
	ld_hli(gb, 0x00); // OBJ_VAR_3
	gb->cpu.f = FLAG_Z; // xor a
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(CountRichtertoffenFunc, WobbleLeftStart) { wobble(gb, OBJSTATE_WOBBLE_LEFT); }
PORT_LOCAL(CountRichtertoffenFunc, WobbleRightStart) { wobble(gb, OBJSTATE_WOBBLE_RIGHT); }

PORT_LOCAL_JUMP(CountRichtertoffenFunc, VanishTouch)
{
	gb->cpu.a = rd8(gb, wCurObjAction);
	bool vanish = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!vanish) {
		gb->cpu.a = rd8(gb, wTransformation);
		vanish = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!vanish) {
		gb->cpu.a = rd8(gb, wEffectivePowerUp);
		alu_cp(gb, gb->cpu.a, SUPER_JUMP_SLAM_OVERALLS);
		vanish = !(gb->cpu.f & FLAG_C);
	}
	if (!vanish) {
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), INTERACTION_MASK);
		vanish = !gb->cpu.a;
	}
	if (vanish) {
		JUMP(VanishObject2);
		return;
	}
	play_sfx(gb, SFX_014);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	wobble(gb, (gb->cpu.f & FLAG_C) ? OBJSTATE_WOBBLE_RIGHT : OBJSTATE_WOBBLE_LEFT);
	RET();
}

// .asm_41044: falls from the jump
static void jump_fall_start(GB *gb)
{
	set_frameset(gb, ADDR_Frameset_683c7);
	uint16_t hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = OBJSTATE_SPECIAL_2;
	wr8(gb, hl--, OBJSTATE_SPECIAL_2);
	SET_HL(gb, hl);
	clear_var3(gb);
}

PORT_LOCAL(CountRichtertoffenFunc, State0b)
{
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
	jump_fall_start(gb);
}

PORT_LOCAL(CountRichtertoffenFunc, State0c)
{
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
	jump_fall_start(gb);
}

// wWarioYPos - 8 into the y position at hl
static void y_at_wario(GB *gb, uint16_t hl)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), 8);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL(CountRichtertoffenFunc, Flatten)
{
	set_frameset(gb, ADDR_Frameset_683d4);
	ld_hli(gb, 20); // OBJ_STATE_DURATION
	uint16_t hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = OBJSTATE_5A;
	wr8(gb, hl--, OBJSTATE_5A);
	SET_HL(gb, hl);
	y_at_wario(gb, lo_hl(gb, OBJ_Y_POS));
}

// .asm_40fe9 (h = the object's): jumps, away from Wario (towards him when
// he's flat)
static void jump(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = OBJSTATE_SPECIAL_1;
	wr8(gb, hl--, OBJSTATE_SPECIAL_1);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FLAT_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		gb->cpu.f ^= FLAG_C; // ccf
	} else {
		gb->cpu.c = 0x2a;
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	}
}

// .asm_41006: carry = face right
static void jump_dir(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	if (gb->cpu.f & FLAG_C) {
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
		set_frameset(gb, ADDR_Frameset_683c4);
	} else {
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
		set_frameset(gb, ADDR_Frameset_683c1);
	}
	ld_hli(gb, 22); // OBJ_STATE_DURATION
	hl = lo_hl(gb, OBJ_VAR_3);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_066);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State5a)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 20);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wAutoMoveState);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP(MoveObjectDown_Fast);
			return;
		}
		y_at_wario(gb, wCurObjYPos);
	}
	// .asm_40f72
	if (!dec_hl(gb, lo_hl(gb, OBJ_STATE_DURATION))) {
		RET();
		return;
	}
	gb->cpu.a = 31 | (1 << 7); // no_actions_for 31
	wr8(gb, wCurObjAction, gb->cpu.a);
	jump(gb);
	jump_dir(gb);
	RET();
}

PORT_LOCAL(CountRichtertoffenFunc, StandingFallStart)
{
	state_hld(gb, OBJSTATE_STANDING_FALL);
	clear_var3(gb);
	set_frameset(gb, ADDR_Frameset_683de);
}

PORT_LOCAL(CountRichtertoffenFunc, Land)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjFlags), OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	alu_cp(gb, gb->cpu.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	state_hld(gb, OBJSTATE_WALK);
	set_frameset(gb, ADDR_Frameset_683bc);
	ld_hli(gb, 44); // OBJ_STATE_DURATION
}

PORT_LOCAL(CountRichtertoffenFunc, Walk)
{
	if (dec_hl(gb, wCurObjStateDuration)) {
		jump(gb);
		jump_dir(gb);
		return;
	}
	copy_pos(gb, wCurObjYPos);
	gb_call_ret(gb, ADDR_Func_3543, CR(0x1c4));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		set_state(gb, OBJSTATE_STANDING_FALL_START);
		return;
	}
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
}

// Bumped: jumps away from Wario.
PORT_LOCAL(CountRichtertoffenFunc, Bump)
{
	state_hld(gb, OBJSTATE_SPECIAL_1);
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	jump_dir(gb);
}

// Moves sideways in the air, turning around at walls. rets: where the asm's
// calls of Func_3513 return (left, right).
static void air_move(GB *gb, uint16_t movement, uint16_t ret_left, uint16_t ret_right)
{
	SET_BC(gb, movement);
	CALL(ApplyObjYMovement);
	uint16_t hl = offset16(gb, wCurObjYPos, -10, hYPosLo, hYPosHi);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = gb->cpu.f & FLAG_C;
	offset16(gb, hl, right ? 7 : -8, hXPosLo, hXPosHi);
	gb_call_ret(gb, ADDR_Func_3513, right ? ret_right : ret_left);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (!gb->cpu.a) {
		if (right)
			JUMP(MoveObjectRight);
		else
			JUMP(MoveObjectLeft);
		return;
	}
	set_frameset(gb, right ? ADDR_Frameset_683c1 : ADDR_Frameset_683c4);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, right ? 0 : OBJSUBFLAG_HDIR, right ? OBJSUBFLAG_HDIR : 0);
	RET();
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, JumpRise)
{
	uint16_t hl = wCurObjStateDuration;
	if (dec_hl(gb, hl)) {
		jump_fall_start(gb);
		RET();
		return;
	}
	// .asm_41053
	hl = offset16(gb, lo_hl(gb, OBJ_Y_POS), -18, hYPosLo, hYPosHi);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_358b, CR(0x256));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.a) {
		// hit a ceiling
		hl = wCurObjVar2;
		gb->cpu.a = 0x01;
		wr8(gb, hl, 0x01);
		SET_HL(gb, hl);
		hl = lo_hl(gb, OBJ_SUBSTATE);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
		jump_fall_start(gb);
		RET();
		return;
	}
	air_move(gb, ADDR_Data_603d0, CR(0x28e), CR(0x2ab));
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, JumpFall)
{
	copy_pos(gb, wCurObjYPos);
	gb_call_ret(gb, ADDR_Func_352b, CR(0x2cd));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// fall bounce
		uint16_t hl = wCurObjYPos;
		gb->cpu.a = rd8(gb, hYPosLo);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
		hl = lo_hl(gb, OBJ_STATE);
		gb->cpu.a = OBJSTATE_SPECIAL_3;
		wr8(gb, hl--, OBJSTATE_SPECIAL_3);
		SET_HL(gb, hl);
		clear_var3(gb);
		set_frameset(gb, ADDR_Frameset_683d4);
		ld_hli(gb, 24); // OBJ_STATE_DURATION
		play_sfx(gb, SFX_075);
		gb->cpu.b = 24;
		JUMP(DoGroundShake);
		return;
	}
	// .continue_fall
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(VanishObject2);
		return;
	}
	air_move(gb, ADDR_Data_603e0, CR(0x324), CR(0x341));
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, FallBounce)
{
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, 15);
	if (gb->cpu.f & FLAG_Z) {
		// .check_bounce_end
		if (dec_hl(gb, wCurObjStateDuration))
			set_state(gb, OBJSTATE_LAND);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 14);
	if (gb->cpu.f & FLAG_Z)
		set_frameset(gb, ADDR_Frameset_683e1);
	SET_BC(gb, ADDR_Data_604b0);
	JUMP(ApplyObjYMovement);
}

// Stunned while var2 isn't 0.
static void stun(GB *gb, bool right)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			JUMP(Func_32bd);
		else
			JUMP(Func_32ae);
		return;
	}
	set_state(gb, OBJ_INTERACTION_RICHTERTOFFEN);
	RET();
}

static void stun_start(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	set_frameset(gb, ADDR_Frameset_683d9);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, StunLeftStart)
{
	stun_start(gb, OBJSTATE_STUN_LEFT);
	stun(gb, false);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, StunLeft) { stun(gb, false); }

PORT_LOCAL_JUMP(CountRichtertoffenFunc, StunRightStart)
{
	stun_start(gb, OBJSTATE_STUN_RIGHT);
	stun(gb, true);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, StunRight) { stun(gb, true); }

static void func_411d9(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	set_frameset(gb, ADDR_Frameset_683d9);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02); // OBJ_VAR_2
	ld_hli(gb, 0x00); // OBJ_VAR_3
	gb->cpu.f = FLAG_Z; // xor a
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(CountRichtertoffenFunc, State14) { func_411d9(gb, OBJSTATE_44); }
PORT_LOCAL(CountRichtertoffenFunc, State15) { func_411d9(gb, OBJSTATE_45); }

static void state3f(GB *gb)
{
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_32ea);
		return;
	}
	set_state(gb, OBJSTATE_10);
	RET();
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State0f)
{
	set_state(gb, OBJSTATE_3F);
	gb->cpu.a = 100;
	wr8(gb, wCurObjStateDuration, 100);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_GRABBED);
	state3f(gb);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State3f) { state3f(gb); }

static void state40(GB *gb)
{
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_32f9);
		return;
	}
	set_state(gb, OBJSTATE_00);
	RET();
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State10)
{
	state_hld(gb, OBJSTATE_40);
	clear_var3(gb);
	gb->cpu.a = 7;
	wr8(gb, wCurObjStateDuration, 7);
	state40(gb);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State40) { state40(gb); }

PORT_LOCAL(CountRichtertoffenFunc, TurnAroundStart)
{
	state_hld(gb, OBJSTATE_TURN_AROUND);
	set_frameset(gb, ADDR_Frameset_683e1);
	ld_hli(gb, 48); // OBJ_STATE_DURATION
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, TurnAround)
{
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_3308);
		return;
	}
	set_state(gb, OBJ_INTERACTION_01);
	RET();
}

// .asm_412a4: knocked away, b = speed
static void knocked(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0); // OBJ_VAR_3
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a); // OBJ_VAR_2
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_683ca);
	JUMP(SetObjectFramesetPtr);
}

static void knock(GB *gb, uint8_t state, uint8_t speed)
{
	state_hld(gb, state);
	gb->cpu.b = speed;
	knocked(gb);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State29) { knock(gb, OBJSTATE_59, 0x02); }
PORT_LOCAL_JUMP(CountRichtertoffenFunc, State28) { knock(gb, OBJSTATE_58, 0x02); }

// Attacked: wobbles if Wario is normal and weak enough, else knocked away.
static void attacked(GB *gb, uint8_t wobble_state, uint8_t knock_state)
{
	gb->cpu.a = rd8(gb, wTransformation);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wEffectivePowerUp);
		alu_cp(gb, gb->cpu.a, GARLIC);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), INTERACTION_MASK);
			if (gb->cpu.a) {
				set_state(gb, wobble_state);
				RET();
				return;
			}
		}
	}
	knock(gb, knock_state, 0x01);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, AttackedLeftStart)
{
	attacked(gb, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_ATTACKED_LEFT);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, AttackedRightStart)
{
	attacked(gb, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_ATTACKED_RIGHT);
}

// Picked up.
static void picked_up(GB *gb, uint8_t state, uint8_t var2)
{
	state_hld(gb, state);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, 0x41); // OBJ_VAR_3
	wr8(gb, hl--, var2); // OBJ_VAR_2
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01); // OBJ_VAR_1
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, PickedUpLeftStart)
{
	picked_up(gb, OBJSTATE_PICKED_UP_LEFT, 0xd0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, PickedUpRightStart)
{
	picked_up(gb, OBJSTATE_PICKED_UP_RIGHT, 0xf0);
	JUMP(Func_3344);
}

// Grabbed: .State19 ($49) and .State18 ($48) turn with Wario.
static void grab(GB *gb, bool right)
{
	for (;;) {
		state_hld(gb, right ? OBJSTATE_49 : OBJSTATE_48);
		set_frameset(gb, ADDR_Frameset_683d9);
		set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_GRABBED, 0);
		gb->cpu.a = rd8(gb, wDirection);
		bool dir = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
		if (right ? dir : !dir) {
			if (right)
				JUMP(Func_3353);
			else
				JUMP(Func_3362);
			return;
		}
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
		right = !right;
	}
}

static void hold(GB *gb, bool right)
{
	gb->cpu.a = rd8(gb, wDirection);
	bool dir = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (right ? dir : !dir) {
		if (right)
			JUMP(Func_3353);
		else
			JUMP(Func_3362);
		return;
	}
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	grab(gb, !right);
}

PORT_LOCAL_JUMP(CountRichtertoffenFunc, State19) { grab(gb, true); }
PORT_LOCAL_JUMP(CountRichtertoffenFunc, State49) { hold(gb, true); }
PORT_LOCAL_JUMP(CountRichtertoffenFunc, State18) { grab(gb, false); }
PORT_LOCAL_JUMP(CountRichtertoffenFunc, State48) { hold(gb, false); }

PORT_LOCAL(CountRichtertoffenFunc, State12)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 31 | (1 << 7);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = OBJSTATE_42;
	wr8(gb, hl--, OBJSTATE_42); // OBJ_STATE
	SET_HL(gb, hl);
	clear_var3(gb);
	set_interaction(gb, OBJ_INTERACTION_00); // unnecessary
}
