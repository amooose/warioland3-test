// Port of src/engine/level/objects/hammer_bot.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define HB(off) (ADDR_HammerBotFunc + (off))
#define HB_INIT HB(0xa) // .Init
#define HB_UPDATE HB(0x1e) // .Update
#define HB_FUNC_4434F HB(0x114)
#define HB_FUNC_4472F HB(0x4f4)
#define HB_FUNC_44739 HB(0x4fe)
#define HB_FUNC_4485F HB(0x624)
#define HB_FUNC_44890 HB(0x655)

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static inline uint16_t set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld l, OBJ_INTERACTION_TYPE / ld a, [hl] / and HEAVY_OBJ / or type / ld [hld], a
static inline uint16_t set_interaction(GB *gb, uint16_t hl, uint8_t type)
{
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld l, OBJ_COLLBOX_RIGHT / ld a, right / ld [hld], a / ld a, left / ld [hld], a
static inline uint16_t set_collbox_x(GB *gb, uint16_t hl, uint8_t right, uint8_t left)
{
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, right);
	gb->cpu.a = left;
	wr8(gb, hl--, left);
	return hl;
}

// ld hl, wCurObjState / ld a, state / ld [hld], a / ld a, [hld] / rlca /
// ld_obj_frameset left, right / call SetObjectFramesetPtr
static void set_state_frameset(GB *gb, uint8_t state, uint16_t left, uint16_t right)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? right : left);
	CALL(SetObjectFramesetPtr);
}

// When the ground shakes, the object falls over into the given state.
// Returns true if so.
static bool ground_shake(GB *gb, uint8_t state)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return false;
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
	return true;
}

// Checks for ground below; when there's none, falls (state $11) and returns
// false. Else snaps to it and returns true with hl = wCurObjXPos.
static bool on_ground(GB *gb, uint16_t ret)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3543, ret);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0x11;
		wr8(gb, wCurObjState, 0x11);
		RET();
		return false;
	}
	hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	return true;
}

// The position 4 pixels up and dx pixels to the side into hYPos / hXPos
// (dx < 0: sub / sbc, else add / adc).
static void ahead_pos_to_h(GB *gb, int dx)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 4);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	if (dx < 0) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// Steps left (hl = wCurObjXPos) unless there's a wall d pixels ahead.
// Returns false when it moved, true when blocked.
static bool step_left(GB *gb, uint8_t d, uint16_t ret_block, uint16_t ret_3513, uint16_t move)
{
	gb->cpu.a = alu_sub(gb, alu_and(gb, rd8(gb, REG_HL(gb)), 0x0f), d);
	if (gb->cpu.f & FLAG_C) {
		gb_call_ret(gb, ADDR_ProcessBlockOnLeft, ret_block);
		if (!(gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f)))
			return true;
	}
	ahead_pos_to_h(gb, -(int)d);
	gb_call_ret(gb, ADDR_Func_3513, ret_3513);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(move);
		return false;
	}
	return true;
}

// Steps right (hl = wCurObjXPos) unless there's a wall d pixels ahead.
static bool step_right(GB *gb, uint8_t d, uint16_t ret_block, uint16_t ret_3513, uint16_t move)
{
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, REG_HL(gb)), 0x0f), d);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		gb_call_ret(gb, ADDR_ProcessBlockOnRight, ret_block);
		if (!(gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f)))
			return true;
	}
	ahead_pos_to_h(gb, d);
	gb_call_ret(gb, ADDR_Func_3513, ret_3513);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(move);
		return false;
	}
	return true;
}

// ld a, [wGlobalCounter] / ld b, a / and mask / play_sfx z, SFX_107
static void hammer_sfx(GB *gb, uint8_t mask)
{
	gb->cpu.a = rd8(gb, wGlobalCounter);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, mask);
	if (!gb->cpu.a)
		play_sfx(gb, SFX_107);
}

PORT(HammerBotFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HB_INIT >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HB_INIT;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(HammerBotFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, HB_UPDATE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	JUMP_TO(HB_FUNC_4434F);
}

// rst JumpTable on the object's state
PORT_LOCAL_JUMP(HammerBotFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	gb_push16(gb, HB_UPDATE + 4); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

// Hit: vanishes unless Wario can only bump it, in which case it's pushed
// away from him.
PORT_LOCAL_JUMP(HammerBotFunc, Func_44317)
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
	JUMP_TO((gb->cpu.f & FLAG_C) ? HB_FUNC_44739 : HB_FUNC_4472F);
}

// Stands still, facing Wario.
PORT_LOCAL(HammerBotFunc, Func_4434f)
{
	uint16_t hl = set_interaction(gb, wCurObjInteractionType, OBJ_INTERACTION_01);
	hl = set_collbox_x(gb, hl, 0x07, 0xf8);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x30;
	wr8(gb, hl--, 0x30);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool right = gb->cpu.f & FLAG_C;
	if (right)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, right ? ADDR_Frameset_68b17 : ADDR_Frameset_68b12);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(HammerBotFunc, Func_4438b)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0x41;
	wr8(gb, hl--, 0x41);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	bool right = gb->cpu.f & FLAG_C;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, right ? ADDR_Frameset_68b17 : ADDR_Frameset_68b12);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(HammerBotFunc, Func_443a7)
{
	if (ground_shake(gb, 0x1a))
		return;
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0x2e;
	wr8(gb, wCurObjState, 0x2e);
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_443bf)
{
	hammer_sfx(gb, 0x0f);
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	// .asm_443dc
	if (!on_ground(gb, HB(0x1b3)))
		return;
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool blocked;
	if (gb->cpu.f & FLAG_C)
		blocked = step_right(gb, 0x0f, HB(0x209), HB(0x227), ADDR_MoveObjectRight);
	else
		blocked = step_left(gb, 0x10, HB(0x1d5), HB(0x1f3), ADDR_MoveObjectLeft);
	if (!blocked)
		return;
	gb->cpu.a = 0x13;
	wr8(gb, wCurObjState, 0x13);
	RET();
}

PORT_LOCAL(HammerBotFunc, Func_4446c)
{
	uint16_t hl = set_interaction(gb, wCurObjInteractionType, OBJ_INTERACTION_01);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68b17 : ADDR_Frameset_68b12);
	CALL(SetObjectFramesetPtr);
}

// Starts swinging the hammer.
static void swing(GB *gb, uint16_t frameset, uint8_t right, uint8_t left)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 20);
	hl = set_collbox_x(gb, hl, right, left);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_BOUNCY);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x2d;
	wr8(gb, hl--, 0x2d);
	SET_HL(gb, hl);
}

// Walks slowly; when Wario is close in front, swings its hammer.
PORT_LOCAL_JUMP(HammerBotFunc, Func_4448c)
{
	if (alu_bit(gb, rd8(gb, wCurObjFlags), OBJFLAG_ON_SCREEN_F))
		hammer_sfx(gb, 0x1f);
	SET_HL(gb, wCurObjFlags);
	// .asm_444a3
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	if (!on_ground(gb, HB(0x287)))
		return;
	gb->cpu.c = 0x2a;
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = gb->cpu.f & FLAG_C;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x18);
	bool close = gb->cpu.f & FLAG_C;
	if (!close) {
		alu_cp(gb, gb->cpu.a, 0xe8);
		close = !(gb->cpu.f & FLAG_C);
	}
	if (close) {
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		if (!right) {
			alu_cp(gb, gb->cpu.a, 0x40);
			if (gb->cpu.f & FLAG_C) {
				swing(gb, ADDR_Frameset_68b1c, 0x07, 0xf0);
				RET();
				return;
			}
		} else {
			alu_cp(gb, gb->cpu.a, 0xc0);
			if (!(gb->cpu.f & FLAG_C)) {
				swing(gb, ADDR_Frameset_68b25, 0x0f, 0xf8);
				RET();
				return;
			}
		}
	}
	bool blocked;
	if (right)
		blocked = step_right(gb, 0x07, HB(0x35e), HB(0x37c), ADDR_MoveObjectRight_Slow);
	else
		blocked = step_left(gb, 0x08, HB(0x2eb), HB(0x309), ADDR_MoveObjectLeft_Slow);
	if (!blocked)
		return;
	gb->cpu.a = 0x13;
	wr8(gb, wCurObjState, 0x13);
	RET();
}

// .Func_445c1 / .Func_445c9
static void func_445cf(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68b58, ADDR_Frameset_68b5b);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 12;
	wr8(gb, hl--, 12);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
	hl = set_collbox_x(gb, hl, 0x07, 0xf8);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_01));
}

PORT_LOCAL(HammerBotFunc, Func_445c1) { func_445cf(gb, 0x32); }
PORT_LOCAL(HammerBotFunc, Func_445c9) { func_445cf(gb, 0x33); }

// .Func_4461c / .Func_4464f
static void func_4461c(GB *gb, uint16_t func)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(func);
		return;
	}
	gb->cpu.a = 0x0f;
	wr8(gb, wCurObjState, 0x0f);
	RET();
}

// .Func_445f7 / .Func_4462a
static void func_445f7(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68b2e, ADDR_Frameset_68b37);
	uint16_t hl = set_collbox_x(gb, REG_HL(gb), 0x07, 0xf8);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_445f7)
{
	func_445f7(gb, 0x46);
	func_4461c(gb, ADDR_Func_32ae);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_4461c) { func_4461c(gb, ADDR_Func_32ae); }
PORT_LOCAL_JUMP(HammerBotFunc, Func_4462a)
{
	func_445f7(gb, 0x47);
	func_4461c(gb, ADDR_Func_32bd);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_4464f) { func_4461c(gb, ADDR_Func_32bd); }

// .Func_4465d / .Func_44665
static void func_4466b(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68b2e, ADDR_Frameset_68b37);
	uint16_t hl = INC_L(INC_L(REG_HL(gb)));
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	hl = set_collbox_x(gb, hl, 0x07, 0xf8);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL(HammerBotFunc, Func_4465d) { func_4466b(gb, 0x44); }
PORT_LOCAL(HammerBotFunc, Func_44665) { func_4466b(gb, 0x45); }

// .Func_446a1
static void func_446a1(GB *gb)
{
	if (ground_shake(gb, 0x1c)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_32ea);
		return;
	}
	gb->cpu.a = 0x10;
	wr8(gb, wCurObjState, 0x10);
	RET();
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_44692)
{
	gb->cpu.a = 0x3f;
	wr8(gb, wCurObjState, 0x3f);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) & ~OBJFLAG_GRABBED));
	func_446a1(gb);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_446a1) { func_446a1(gb); }

// .Func_446c9
static void func_446c9(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_32f9);
		return;
	}
	gb->cpu.a = 0x00;
	wr8(gb, wCurObjState, 0x00);
	RET();
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_446bb)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0x40;
	wr8(gb, hl--, 0x40);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = 0x07;
	wr8(gb, wCurObjStateDuration, 0x07);
	func_446c9(gb);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_446c9) { func_446c9(gb); }

PORT_LOCAL(HammerBotFunc, Func_446d6)
{
	set_state_frameset(gb, 0x43, ADDR_Frameset_68b4a, ADDR_Frameset_68b51);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 26;
	wr8(gb, hl++, 26);
	hl = set_collbox_x(gb, hl, 0x07, 0xf8);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_01);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_PRIORITY));
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_44703)
{
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	// .asm_44710
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x09);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_44722
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		else
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_3308);
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, wCurObjState, 0x01);
	RET();
}

// .Func_4472f / .Func_44739
static void func_44741(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x8f;
	wr8(gb, hl--, 0x8f);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68b37 : ADDR_Frameset_68b2e);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 4);
	hl = INC_L(hl);
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	hl = set_collbox_x(gb, hl, 0x07, 0xf8);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL(HammerBotFunc, Func_4472f) { func_44741(gb, 0x38); }
PORT_LOCAL(HammerBotFunc, Func_44739) { func_44741(gb, 0x39); }

// .Func_4476b / .Func_44778: knocked over, seeing stars.
static void func_44783(GB *gb, uint8_t state, uint16_t params, uint16_t frameset)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
	SET_BC(gb, params);
	SET_DE(gb, frameset);
	// .asm_44783
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 20;
	wr8(gb, hl++, 20);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_PRIORITY));
	play_sfx(gb, SFX_049);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_4476b) { func_44783(gb, 0x3b, ADDR_ObjParams_StarsLeft, ADDR_Frameset_68b5e); }
PORT_LOCAL_JUMP(HammerBotFunc, Func_44778) { func_44783(gb, 0x3c, ADDR_ObjParams_StarsRight, ADDR_Frameset_68b61); }

// .Func_4479d / .Func_447af
static void func_4479d(GB *gb, uint16_t func)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(func);
		return;
	}
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		gb->cpu.a = 0x13;
		wr8(gb, wCurObjState, 0x13);
	}
	RET();
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_4479d) { func_4479d(gb, ADDR_Func_32bd); }
PORT_LOCAL_JUMP(HammerBotFunc, Func_447af) { func_4479d(gb, ADDR_Func_32ae); }

// .asm_4481b
static void func_4481b(GB *gb, uint16_t hl)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xf0);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, gb->cpu.c);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68b45 : ADDR_Frameset_68b40);
	JUMP(SetObjectFramesetPtr);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a / ld b, b / jr .asm_4481b
static void stunned(GB *gb, uint8_t state, uint8_t b)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.b = b;
	func_4481b(gb, hl);
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_447c1) { stunned(gb, 0x59, 0x02); }
PORT_LOCAL_JUMP(HammerBotFunc, Func_447cb) { stunned(gb, 0x58, 0x02); }

// .Func_447d5 / .Func_447f9: bumped unless Wario can attack it.
static void func_447d5(GB *gb, uint8_t attacked, uint8_t state)
{
	gb->cpu.a = rd8(gb, wTransformation);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wEffectivePowerUp);
		alu_cp(gb, gb->cpu.a, GARLIC);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), INTERACTION_MASK);
			if (gb->cpu.a) {
				gb->cpu.a = attacked;
				wr8(gb, wCurObjState, attacked);
				RET();
				return;
			}
		}
	}
	stunned(gb, state, 0x01);
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_447d5) { func_447d5(gb, 0x08, 0x34); }
PORT_LOCAL_JUMP(HammerBotFunc, Func_447f9) { func_447d5(gb, 0x09, 0x35); }

// .Func_44839 / .Func_4484c
static void func_44839(GB *gb, uint8_t state, uint8_t v)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = DEC_L(hl);
	wr8(gb, hl--, 0x41);
	wr8(gb, hl--, v);
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_44839)
{
	func_44839(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_4484c)
{
	func_44839(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// .Func_4485f / .Func_44890 up to .Func_44880 / .Func_448b1
static void grabbed(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68b2e, ADDR_Frameset_68b37);
	uint16_t hl = set_collbox_x(gb, REG_HL(gb), 0x07, 0xf8);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_GRABBED));
}

// .Func_44880 / .Func_448b1: follows Wario's direction while carried.
static void carried(GB *gb, bool left)
{
	gb->cpu.a = rd8(gb, wDirection);
	bool nz = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (left ? nz : !nz) {
		if (left)
			JUMP(Func_3353);
		else
			JUMP(Func_3362);
		return;
	}
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	JUMP_TO(left ? HB_FUNC_44890 : HB_FUNC_4485F);
}

PORT_LOCAL_JUMP(HammerBotFunc, Func_4485f)
{
	grabbed(gb, 0x49);
	carried(gb, true);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_44880) { carried(gb, true); }
PORT_LOCAL_JUMP(HammerBotFunc, Func_44890)
{
	grabbed(gb, 0x48);
	carried(gb, false);
}
PORT_LOCAL_JUMP(HammerBotFunc, Func_448b1) { carried(gb, false); }

PORT_LOCAL(HammerBotFunc, Func_448c2)
{
	uint16_t hl = wCurObjAction;
	wr8(gb, hl--, 0x9f);
	wr8(gb, hl--, 0x42);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}
