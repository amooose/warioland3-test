// Port of src/engine/level/objects/sun_moon.asm
#include "ported.h"

#define SUN_FLAME_UPDATE (ADDR_SunFlameFunc + 0x20) // .Update
#define MOON_STAR_UPDATE (ADDR_MoonStarFunc + 0x20) // .Update

// hYPos = [wCurObjYPos] - y (16-bit), hl = wCurObjYPos + 2
static void set_hypos_sub(GB *gb, uint8_t y)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), y);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// Moves the sun/moon horizontally by var2 unless a wall is in the way.
static void sun_moon_move_x(GB *gb, bool left, uint16_t routine)
{
	set_hypos_sub(gb, 6);
	uint16_t hl = REG_HL(gb);
	if (left) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 12);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 12);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + 0x1a));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	set_hypos_sub(gb, 20);
	gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + 0x2c));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	if (left)
		JUMP(MoveObjectLeftByVar2);
	else
		JUMP(MoveObjectRightByVar2);
}

PORT_JUMP(SunMoon_MoveRight) { sun_moon_move_x(gb, false, ADDR_SunMoon_MoveRight); }
PORT_JUMP(SunMoon_MoveLeft) { sun_moon_move_x(gb, true, ADDR_SunMoon_MoveLeft); }

PORT_JUMP(SunMoon_MoveDown)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	wr8(gb, hXPosHi, rd8(gb, hl));
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wc1ca, 0);
	gb_call_ret(gb, ADDR_Func_352b, ADDR_SunMoon_MoveDown + 0x16);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wc1ca);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
	}
	JUMP(MoveObjectDownByVar2);
}

PORT_JUMP(SunMoon_MoveUp)
{
	set_hypos_sub(gb, 24);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_358b, ADDR_SunMoon_MoveUp + 0x16);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	JUMP(MoveObjectUpByVar2);
}

// Moves with bc's movement; once its state changes, it bursts.
static void flame_star_func(GB *gb, uint16_t update, uint16_t frameset)
{
	CALL(ApplyObjXMovement);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 14;
	hl = REG_HL(gb);
	wr8(gb, hl++, 14); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
}

PORT_JUMP(SunFlameFunc_Left)
{
	SET_BC(gb, ADDR_Data_606e0);
	JUMP_TO(ADDR_SunFlameFunc);
}

PORT_JUMP(SunFlameFunc_Right)
{
	SET_BC(gb, ADDR_Data_606f0);
	JUMP_TO(ADDR_SunFlameFunc); // fallthrough
}

PORT(SunFlameFunc) { flame_star_func(gb, SUN_FLAME_UPDATE, ADDR_Frameset_68fc9); }

PORT_JUMP(MoonStarFunc_Left)
{
	SET_BC(gb, ADDR_Data_606e0);
	JUMP_TO(ADDR_MoonStarFunc);
}

PORT_JUMP(MoonStarFunc_Right)
{
	SET_BC(gb, ADDR_Data_606f0);
	JUMP_TO(ADDR_MoonStarFunc); // fallthrough
}

PORT(MoonStarFunc) { flame_star_func(gb, MOON_STAR_UPDATE, ADDR_Frameset_69024); }

// a = [hl] / e = a, d = [hl + 1] (hl + 1 left in hl); b, c = the 16-bit
// big-endian value at `wario`
static void sm_load_pos(GB *gb, uint8_t l, uint16_t wario)
{
	gb->cpu.l = l;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	SET_HL(gb, hl);
	gb->cpu.b = gb->cpu.a = rd8(gb, wario + 0);
	gb->cpu.c = gb->cpu.a = rd8(gb, wario + 1);
	gb->cpu.a = gb->cpu.b;
}

// Moves towards Wario (keeping 32 pixels away horizontally), turning to face
// him, then follows him vertically.
PORT_JUMP(SunMoon_GeneralMovement)
{
	SET_HL(gb, wCurObjFlags);
	if (!alu_bit(gb, rd8(gb, wCurObjFlags), OBJFLAG_ON_SCREEN_F)) {
		gb->cpu.a = 2;
		wr8(gb, wCurObjVar2, 2);
	}
	sm_load_pos(gb, OBJ_X_POS, wWarioXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	bool left = gb->cpu.f & FLAG_C;
	if (!left && (gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = gb->cpu.c;
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		left = gb->cpu.f & FLAG_C;
	}
	uint16_t hl;
	if (!left) {
		hl = alu_add16(gb, 32, REG_DE(gb));
		SET_HL(gb, hl);
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, gb->cpu.h);
		bool near = gb->cpu.f & FLAG_C;
		if (!near && (gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = gb->cpu.c;
			alu_cp(gb, gb->cpu.a, gb->cpu.l);
			near = gb->cpu.f & FLAG_C;
		}
		if (near) {
			gb->cpu.a = 1;
			wr8(gb, wCurObjVar2, 1);
		} else {
			gb_call_ret(gb, ADDR_SunMoon_MoveRight, ADDR_SunMoon_GeneralMovement + 0x3b);
		}
		// .asm_473fb
		SET_HL(gb, wCurObjSubState);
		if (!alu_bit(gb, rd8(gb, wCurObjSubState), OBJSUBFLAG_HDIR_F)) {
			wr8(gb, wCurObjSubState, (uint8_t)(rd8(gb, wCurObjSubState) | OBJSUBFLAG_HDIR));
			gb->cpu.a = 0x01;
			wr8(gb, wCurObjVar1, 0x01);
		}
	} else {
		// .asm_4740b
		hl = alu_add16(gb, (uint16_t)-32, REG_DE(gb));
		SET_HL(gb, hl);
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, gb->cpu.h);
		bool far = gb->cpu.f & FLAG_C;
		if (!far && (gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = gb->cpu.c;
			alu_cp(gb, gb->cpu.a, gb->cpu.l);
			far = gb->cpu.f & FLAG_C;
		}
		if (!far) {
			gb->cpu.a = 1;
			wr8(gb, wCurObjVar2, 1);
		} else {
			gb_call_ret(gb, ADDR_SunMoon_MoveLeft, ADDR_SunMoon_GeneralMovement + 0x63);
		}
		// .asm_47423
		SET_HL(gb, wCurObjSubState);
		if (alu_bit(gb, rd8(gb, wCurObjSubState), OBJSUBFLAG_HDIR_F)) {
			wr8(gb, wCurObjSubState, (uint8_t)(rd8(gb, wCurObjSubState) & ~OBJSUBFLAG_HDIR));
			gb->cpu.a = 0x02;
			wr8(gb, wCurObjVar1, 0x02);
		}
	}
	// .asm_47431
	gb->cpu.l = OBJ_Y_POS;
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	hl = alu_add16(gb, 0x2a, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.b = gb->cpu.a = rd8(gb, wWarioYPos + 0);
	gb->cpu.c = gb->cpu.a = rd8(gb, wWarioYPos + 1);
	gb->cpu.a = gb->cpu.b;
	alu_cp(gb, gb->cpu.a, gb->cpu.h);
	if (gb->cpu.f & FLAG_C) {
		JUMP_TO(ADDR_SunMoon_MoveUp);
		return;
	}
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP_TO(ADDR_SunMoon_MoveDown);
		return;
	}
	gb->cpu.a = gb->cpu.c;
	alu_cp(gb, gb->cpu.a, gb->cpu.l);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	if (gb->cpu.f & FLAG_C)
		JUMP_TO(ADDR_SunMoon_MoveUp);
	else
		JUMP_TO(ADDR_SunMoon_MoveDown);
}

// Moves towards Wario horizontally (faster off-screen), keeping 32 pixels
// away.
PORT_JUMP(SunMoon_XMovement)
{
	SET_HL(gb, wCurObjFlags);
	gb->cpu.a = alu_bit(gb, rd8(gb, wCurObjFlags), OBJFLAG_ON_SCREEN_F) ? 1 : 2;
	wr8(gb, wCurObjVar2, gb->cpu.a);
	sm_load_pos(gb, OBJ_X_POS, wWarioXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	bool left = gb->cpu.f & FLAG_C;
	if (!left && (gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = gb->cpu.c;
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		left = gb->cpu.f & FLAG_C;
	}
	if (!left) {
		// .going_right
		SET_HL(gb, alu_add16(gb, 32, REG_DE(gb)));
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, gb->cpu.h);
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP_TO(ADDR_SunMoon_MoveRight);
			return;
		}
		gb->cpu.a = gb->cpu.c;
		alu_cp(gb, gb->cpu.a, gb->cpu.l);
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		JUMP_TO(ADDR_SunMoon_MoveRight);
		return;
	}
	// .going_left
	SET_HL(gb, alu_add16(gb, (uint16_t)-32, REG_DE(gb)));
	gb->cpu.a = gb->cpu.b;
	alu_cp(gb, gb->cpu.a, gb->cpu.h);
	if (gb->cpu.f & FLAG_C) {
		JUMP_TO(ADDR_SunMoon_MoveLeft);
		return;
	}
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = gb->cpu.c;
	alu_cp(gb, gb->cpu.a, gb->cpu.l);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_SunMoon_MoveLeft);
}

// SunFunc and MoonFunc share their code, with different framesets, sounds
// and projectiles.
typedef struct {
	uint16_t base; // SunFunc / MoonFunc
	uint16_t fs_right, fs_left; // facing left (no HDIR) / right
	uint16_t fs_idle_r, fs_idle_l;
	uint16_t fs_3, fs_4;
	uint8_t dur_34;
	uint16_t sfx;
	uint16_t proj_left, proj_right;
} SunMoon;

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

static const SunMoon sun = {
	ADDR_SunFunc, ADDR_Frameset_68f74, ADDR_Frameset_68f6b, ADDR_Frameset_68f9e, ADDR_Frameset_68f8f,
	ADDR_Frameset_68fc0, ADDR_Frameset_68fb7, 35, SFX_068, ADDR_ObjParams_SunFlameLeft, ADDR_ObjParams_SunFlameRight,
};
static const SunMoon moon = {
	ADDR_MoonFunc, ADDR_Frameset_68fd9, ADDR_Frameset_68fd0, ADDR_Frameset_69003, ADDR_Frameset_68ff4,
	ADDR_Frameset_69036, ADDR_Frameset_6902b, 40, SFX_06E, ADDR_ObjParams_MoonStarLeft, ADDR_ObjParams_MoonStarRight,
};

// ld [hli], a / ... with hl = OBJ_STATE_DURATION: duration 50, var1 0
static void sm_reset_duration(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 50;
	wr8(gb, hl++, 50);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
}

static void sm_func(GB *gb, const SunMoon *s)
{
	SET_DE(gb, s->fs_left);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	uint16_t f = s->base + 0x24; // .CheckDirection
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 7);
	wr8(gb, hl--, (uint8_t)-8);
	gb->cpu.a = (uint8_t)-4;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
}

PORT(SunFunc) { sm_func(gb, &sun); }
PORT(MoonFunc) { sm_func(gb, &moon); }

static void sm_check_direction(GB *gb, const SunMoon *s)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	uint16_t f = s->base + 0x58;
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		SET_DE(gb, s->fs_left);
	} else {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		SET_DE(gb, s->fs_right);
	}
	CALL(SetObjectFramesetPtr);
	sm_reset_duration(gb);
	hl = REG_HL(gb) + 1;
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a); // OBJ_VAR_2
	SET_HL(gb, hl);
}

PORT_LOCAL(SunFunc, CheckDirection) { sm_check_direction(gb, &sun); }
PORT_LOCAL(MoonFunc, CheckDirection) { sm_check_direction(gb, &moon); }

// the update: a jump table on var1
static void sm_update(GB *gb, const SunMoon *s)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCurObjVar1);
	uint16_t table = s->base + 0x58 + 9;
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

PORT_LOCAL_JUMP(SunFunc, Func_472a6) { sm_update(gb, &sun); }
PORT_LOCAL_JUMP(MoonFunc, Func_475c7) { sm_update(gb, &moon); }

// shoots a flame / star every 50 frames
static void sm_shoot(GB *gb, const SunMoon *s)
{
	gb_call_ret(gb, ADDR_SunMoon_XMovement, s->base + 0x70);
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wc1ca, 0);
	gb_call_ret(gb, ADDR_Func_352b, s->base + 0x86);
	bool fall = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!fall) {
		gb->cpu.a = rd8(gb, wc1ca);
		fall = alu_and(gb, gb->cpu.a, gb->cpu.a);
	}
	if (fall) {
		SET_BC(gb, ADDR_Data_60690);
		CALL(ApplyObjYMovement);
	}
	// .asm_472e3
	hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		sm_reset_duration(gb);
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? s->fs_left : s->fs_right);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	gb->cpu.a = d;
	alu_cp(gb, d, 52);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, s->sfx);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? s->proj_right : s->proj_left);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL_JUMP(SunFunc, Func_472bb) { sm_shoot(gb, &sun); }
PORT_LOCAL_JUMP(MoonFunc, Func_475dc) { sm_shoot(gb, &moon); }

// follows Wario; when he's close (and doesn't press anything for a while),
// gets ready to shoot
static void sm_idle(GB *gb, const SunMoon *s)
{
	gb_call_ret(gb, ADDR_SunMoon_GeneralMovement, s->base + 0xd3);
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadPressed), 0xff ^ (PAD_SELECT | PAD_START));
	if (gb->cpu.a) {
		gb->cpu.a = 50;
		wr8(gb, wCurObjStateDuration, 50);
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, gb->cpu.a));
		RET();
		return;
	}
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
		RET();
		return;
	}
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 64);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_cp(gb, gb->cpu.a, (uint8_t)-64);
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
	}
	gb->cpu.b = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 43);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	hl = LO(hl, OBJ_STATE_DURATION);
	gb->cpu.a = 82;
	wr8(gb, hl++, 82);
	gb->cpu.a = 0x05;
	wr8(gb, hl++, 0x05); // OBJ_VAR_1
	hl++;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0); // OBJ_VAR_3
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--)); // OBJ_SUBSTATE
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? s->fs_idle_l : s->fs_idle_r);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SunFunc, Func_4731e) { sm_idle(gb, &sun); }
PORT_LOCAL_JUMP(MoonFunc, Func_4763f) { sm_idle(gb, &moon); }

// .Func_47378 / .Func_4739c: frameset, duration, next var1
static void sm_start(GB *gb, const SunMoon *s, uint16_t off, uint16_t fs, uint8_t var1)
{
	gb_call_ret(gb, ADDR_SunMoon_GeneralMovement, s->base + off + 3);
	SET_DE(gb, fs);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = s->dur_34;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = var1;
	wr8(gb, hl, var1);
	SET_HL(gb, hl);
}

// .Func_47388 / .Func_473ac: back to var1 0 when the duration runs out
static void sm_end(GB *gb, const SunMoon *s, uint16_t off, uint16_t fs)
{
	gb_call_ret(gb, ADDR_SunMoon_GeneralMovement, s->base + off + 3);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	SET_DE(gb, fs);
	CALL(SetObjectFramesetPtr);
	sm_reset_duration(gb);
}

PORT_LOCAL(SunFunc, Func_47378) { sm_start(gb, &sun, 0x12a, sun.fs_3, 3); }
PORT_LOCAL(SunFunc, Func_47388) { sm_end(gb, &sun, 0x13a, sun.fs_left); }
PORT_LOCAL(SunFunc, Func_4739c) { sm_start(gb, &sun, 0x14e, sun.fs_4, 4); }
PORT_LOCAL(SunFunc, Func_473ac) { sm_end(gb, &sun, 0x15e, sun.fs_right); }
PORT_LOCAL(MoonFunc, Func_47699) { sm_start(gb, &moon, 0x12a, moon.fs_3, 3); }
PORT_LOCAL(MoonFunc, Func_476a9) { sm_end(gb, &moon, 0x13a, moon.fs_left); }
PORT_LOCAL(MoonFunc, Func_476bd) { sm_start(gb, &moon, 0x14e, moon.fs_4, 4); }
PORT_LOCAL(MoonFunc, Func_476cd) { sm_end(gb, &moon, 0x15e, moon.fs_right); }
