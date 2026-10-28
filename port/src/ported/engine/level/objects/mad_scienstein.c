// Port of src/engine/level/objects/mad_scienstein.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define MS(off) (ADDR_MadSciensteinFunc + (off))
#define MS_INIT MS(0xa) // .Init
#define MS_UPDATE MS(0x1e) // .Update
#define MS_FUNC_44FD9 MS(0x29d)
#define MS_FUNC_452F7 MS(0x5bb)
#define MS_FUNC_45301 MS(0x5c5)
#define MS_FUNC_453CC MS(0x690)
#define MS_FUNC_453F8 MS(0x6bc)
#define INVISIBILITY_POTION_DROP_UPDATE (ADDR_InvisibilityPotionFunc_Drop + 0x11) // .Update

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

// call SetObjectFramesetPtr / ld a, v / ld [hli], a
static void set_frameset_duration(GB *gb, uint16_t frameset, uint8_t v)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = v;
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
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

// ld hl, wCurObjYPos / copies the position to hYPos / hXPos
static void obj_pos_to_h(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
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

// Walks one step if there's ground below and no wall ahead (ret: the return
// addresses of the 5 calls). Returns 0 when it moved (or fell), else 1
// (blocked on the left) or 2 (on the right).
static int walk(GB *gb, const uint16_t ret[5], uint16_t move_left, uint16_t move_right)
{
	obj_pos_to_h(gb);
	gb_call_ret(gb, ADDR_Func_3543, ret[0]);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0x11;
		wr8(gb, wCurObjState, 0x11);
		RET();
		return 0;
	}
	uint16_t hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = alu_sub(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x07);
		if (gb->cpu.f & FLAG_C) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, ret[1]);
			if (!(gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f)))
				return 1;
		}
		ahead_pos_to_h(gb, -7);
		gb_call_ret(gb, ADDR_Func_3513, ret[2]);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP_TO(move_left);
			return 0;
		}
		return 1;
	}
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x06);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		gb_call_ret(gb, ADDR_ProcessBlockOnRight, ret[3]);
		if (!(gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f)))
			return 2;
	}
	ahead_pos_to_h(gb, 6);
	gb_call_ret(gb, ADDR_Func_3513, ret[4]);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(move_right);
		return 0;
	}
	return 2;
}

// ld hl, wCurObjStateDuration / ld a, [hl] / cp turn / jr z, .turn /
// dec [hl] / jp nz, Func_3308; returns true when the duration ran out
// (hl = OBJ_STATE_DURATION), false when it's done.
static bool turn_or_count(GB *gb, uint8_t turn)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, turn);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		else
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		RET();
		return false;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_3308);
		return false;
	}
	return true;
}

PORT(MadSciensteinFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(MS_INIT >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)MS_INIT;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, MS_UPDATE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	JUMP_TO(MS_FUNC_44FD9);
}

// rst JumpTable on the object's state
PORT_LOCAL_JUMP(MadSciensteinFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	gb_push16(gb, MS_UPDATE + 4); // rst JumpTable
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
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_44e18)
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
	JUMP_TO((gb->cpu.f & FLAG_C) ? MS_FUNC_45301 : MS_FUNC_452F7);
}

// Throws an invisibility potion.
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_44e50)
{
	if (ground_shake(gb, 0x1c)) {
		RET();
		return;
	}
	// .asm_44e5d
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_44e86
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = LO(hl, OBJ_FLAGS);
		if (alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
			play_sfx(gb, SFX_067);
		hl = LO(hl, OBJ_STATE);
		gb->cpu.a = 0x5a;
		wr8(gb, hl--, 0x5a);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
		SET_HL(gb, hl);
		SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_InvisibilityPotionRight : ADDR_ObjParams_InvisibilityPotionLeft);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_3308);
		return;
	}
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x2b;
	wr8(gb, hl--, 0x2b);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	set_frameset_duration(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68c2c : ADDR_Frameset_68c25, 28);
	RET();
}

// .Func_44eaa / .Func_44f89
static void turn_then(GB *gb, uint8_t state, uint16_t left, uint16_t right, int duration)
{
	if (!turn_or_count(gb, 0x08))
		return;
	uint16_t hl = LO(REG_HL(gb), OBJ_STATE);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	uint16_t fs = (gb->cpu.f & FLAG_C) ? right : left;
	if (duration < 0) {
		SET_DE(gb, fs);
		CALL(SetObjectFramesetPtr);
	} else {
		set_frameset_duration(gb, fs, (uint8_t)duration);
	}
	RET();
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_44eaa) { turn_then(gb, 0x5b, ADDR_Frameset_68bd1, ADDR_Frameset_68bda, -1); }
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_44f89) { turn_then(gb, 0x5c, ADDR_Frameset_68bff, ADDR_Frameset_68c08, 63); }

static const uint16_t walk_44eda[5] = { MS(0x1bd), MS(0x1df), MS(0x1fd), MS(0x21c), MS(0x23a) };
static const uint16_t walk_45054[5] = { MS(0x386), MS(0x3a8), MS(0x3c6), MS(0x3dc), MS(0x3fa) };

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_44eda)
{
	if (ground_shake(gb, 0x1c)) {
		RET();
		return;
	}
	int side = walk(gb, walk_44eda, ADDR_MoveObjectLeft, ADDR_MoveObjectRight);
	if (!side)
		return;
	// .asm_44f3d / .asm_44f7a
	gb->cpu.a = 0x2c;
	wr8(gb, wCurObjState, 0x2c);
	set_frameset_duration(gb, side == 2 ? ADDR_Frameset_68c2c : ADDR_Frameset_68c25, 28);
	RET();
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_44fbf)
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
		JUMP(Func_3308);
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, wCurObjState, 0x01);
	RET();
}

// Stands still, facing Wario.
PORT_LOCAL(MadSciensteinFunc, Func_44fd9)
{
	uint16_t hl = set_interaction(gb, wCurObjInteractionType, OBJ_INTERACTION_01);
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x06);
	wr8(gb, hl--, 0xf9);
	wr8(gb, hl, 0xff);
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
	SET_DE(gb, right ? ADDR_Frameset_68bbe : ADDR_Frameset_68bb5);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(MadSciensteinFunc, Func_45018)
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
	SET_DE(gb, right ? ADDR_Frameset_68bbe : ADDR_Frameset_68bb5);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(MadSciensteinFunc, Func_45034)
{
	uint16_t hl = set_interaction(gb, wCurObjInteractionType, OBJ_INTERACTION_01);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68bbe : ADDR_Frameset_68bb5);
	CALL(SetObjectFramesetPtr);
}

// Walks slowly; when Wario is close in front, starts throwing.
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45054)
{
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	// .asm_45061
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x20);
	bool close = gb->cpu.f & FLAG_C;
	if (!close) {
		alu_cp(gb, gb->cpu.a, 0xe0);
		close = !(gb->cpu.f & FLAG_C);
	}
	if (close) {
		// .asm_45075
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0xc0);
		int attack = -1;
		uint16_t fs = 0;
		if (!(gb->cpu.f & FLAG_C)) {
			// .asm_4509f
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
			attack = (gb->cpu.f & FLAG_C) ? 1 : 0;
			fs = ADDR_Frameset_68bf6;
		} else {
			alu_cp(gb, gb->cpu.a, 0x40);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
				attack = (gb->cpu.f & FLAG_C) ? 0 : 1;
				fs = ADDR_Frameset_68bed;
			}
		}
		if (attack >= 0) {
			SET_HL(gb, hl);
			if (attack) {
				gb->cpu.a = 0x5a;
				wr8(gb, hl, 0x5a);
				set_frameset_duration(gb, fs, 63);
			} else {
				// .asm_4509b
				gb->cpu.a = 0x13;
				wr8(gb, hl, 0x13);
			}
			RET();
			return;
		}
	}
	// .asm_450b0
	if (!walk(gb, walk_45054, ADDR_MoveObjectLeft_Slow, ADDR_MoveObjectRight_Slow))
		return;
	gb->cpu.a = 0x13;
	wr8(gb, wCurObjState, 0x13);
	RET();
}

// .Func_45140 / .Func_45186: drops a potion on some frames.
static void drop_potion(GB *gb, uint8_t state, uint16_t params)
{
	gb->cpu.a = rd8(gb, wCurObjFrame);
	alu_cp(gb, gb->cpu.a, 0x19);
	bool drop = false;
	if (gb->cpu.f & FLAG_C) {
		alu_cp(gb, gb->cpu.a, 0x14);
		if (!(gb->cpu.f & FLAG_C)) {
			drop = true;
		} else {
			alu_cp(gb, gb->cpu.a, 0x06);
			drop = gb->cpu.f & FLAG_C;
		}
	}
	if (drop) {
		SET_BC(gb, params);
		CALL(CreateObjectAtRelativePos);
	}
	set_state_frameset(gb, state, ADDR_Frameset_68bc7, ADDR_Frameset_68bcc);
	// .asm_451b3
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0x0c;
	wr8(gb, hl--, 0x0c);
	SET_HL(gb, hl);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
}

PORT_LOCAL(MadSciensteinFunc, Func_45140) { drop_potion(gb, 0x32, ADDR_ObjParams_InvisibilityPotionDropLeft); }
PORT_LOCAL(MadSciensteinFunc, Func_45186) { drop_potion(gb, 0x33, ADDR_ObjParams_InvisibilityPotionDropRight); }

// .Func_4515d / .Func_451bc
static void bumped(GB *gb, uint16_t bump, uint16_t fs_left, uint8_t st_left, uint16_t fs_right, uint8_t st_right)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(bump);
		return;
	}
	hl = DEC_L(DEC_L(hl));
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
	SET_HL(gb, hl);
	bool right = gb->cpu.f & FLAG_C;
	uint8_t st = right ? st_right : st_left;
	gb->cpu.a = st;
	wr8(gb, hl, st);
	if (st == 0x2b) {
		set_frameset_duration(gb, right ? fs_right : fs_left, 28);
	} else {
		SET_DE(gb, right ? fs_right : fs_left);
		CALL(SetObjectFramesetPtr);
	}
	RET();
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_4515d)
{
	bumped(gb, ADDR_ObjState_BumpLeft, ADDR_Frameset_68c25, 0x2b, ADDR_Frameset_68bda, 0x5b);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_451bc)
{
	bumped(gb, ADDR_ObjState_BumpRight, ADDR_Frameset_68bd1, 0x5b, ADDR_Frameset_68c2c, 0x2b);
}

// .Func_45202 / .Func_4522d
static void func_45202(GB *gb, uint16_t func)
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

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_451e5)
{
	set_state_frameset(gb, 0x46, ADDR_Frameset_68be3, ADDR_Frameset_68be8);
	SET_HL(gb, set_interaction(gb, REG_HL(gb), OBJ_INTERACTION_00));
	func_45202(gb, ADDR_Func_32ae);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45202) { func_45202(gb, ADDR_Func_32ae); }
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45210)
{
	set_state_frameset(gb, 0x47, ADDR_Frameset_68be3, ADDR_Frameset_68be8);
	SET_HL(gb, set_interaction(gb, REG_HL(gb), OBJ_INTERACTION_00));
	func_45202(gb, ADDR_Func_32bd);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_4522d) { func_45202(gb, ADDR_Func_32bd); }

// .Func_4523b / .Func_45243
static void func_45249(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68be3, ADDR_Frameset_68be8);
	uint16_t hl = INC_L(INC_L(REG_HL(gb)));
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL(MadSciensteinFunc, Func_4523b) { func_45249(gb, 0x44); }
PORT_LOCAL(MadSciensteinFunc, Func_45243) { func_45249(gb, 0x45); }

// .Func_4527a
static void func_4527a(GB *gb)
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

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_4526b)
{
	gb->cpu.a = 0x3f;
	wr8(gb, wCurObjState, 0x3f);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) & ~OBJFLAG_GRABBED));
	func_4527a(gb);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_4527a) { func_4527a(gb); }

// .Func_452a2
static void func_452a2(GB *gb)
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

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45294)
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
	func_452a2(gb);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_452a2) { func_452a2(gb); }

PORT_LOCAL(MadSciensteinFunc, Func_452af)
{
	set_state_frameset(gb, 0x43, ADDR_Frameset_68c17, ADDR_Frameset_68c1e);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0x1c;
	wr8(gb, hl++, 0x1c);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_452cb)
{
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	if (!turn_or_count(gb, 0x08))
		return;
	gb->cpu.a = 0x01;
	wr8(gb, wCurObjState, 0x01);
	RET();
}

// .Func_452f7 / .Func_45301
static void func_45309(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x8f;
	wr8(gb, hl--, 0x8f);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68be8 : ADDR_Frameset_68be3);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x04);
	hl = INC_L(hl);
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL(MadSciensteinFunc, Func_452f7) { func_45309(gb, 0x38); }
PORT_LOCAL(MadSciensteinFunc, Func_45301) { func_45309(gb, 0x39); }

// .asm_45388
static void func_45388(GB *gb, uint16_t hl)
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
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68c14 : ADDR_Frameset_68c11);
	JUMP(SetObjectFramesetPtr);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a / ld b, b / jr .asm_45388
static void stunned(GB *gb, uint8_t state, uint8_t b)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.b = b;
	func_45388(gb, hl);
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_4532e) { stunned(gb, 0x59, 0x02); }
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45338) { stunned(gb, 0x58, 0x02); }

// .Func_45342 / .Func_45366: bumped unless Wario can attack it.
static void func_45342(GB *gb, uint8_t attacked, uint8_t state)
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

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45342) { func_45342(gb, 0x08, 0x34); }
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45366) { func_45342(gb, 0x09, 0x35); }

// .Func_453a6 / .Func_453b9
static void func_453a6(GB *gb, uint8_t state, uint8_t v)
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

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_453a6)
{
	func_453a6(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_453b9)
{
	func_453a6(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// .Func_453cc / .Func_453f8 up to .Func_453e8 / .Func_45414
static void grabbed(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68be3, ADDR_Frameset_68be8);
	uint16_t hl = LO(REG_HL(gb), OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_GRABBED));
}

// .Func_453e8 / .Func_45414: follows Wario's direction while carried.
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
	JUMP_TO(left ? MS_FUNC_453F8 : MS_FUNC_453CC);
}

PORT_LOCAL_JUMP(MadSciensteinFunc, Func_453cc)
{
	grabbed(gb, 0x49);
	carried(gb, true);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_453e8) { carried(gb, true); }
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_453f8)
{
	grabbed(gb, 0x48);
	carried(gb, false);
}
PORT_LOCAL_JUMP(MadSciensteinFunc, Func_45414) { carried(gb, false); }

PORT_LOCAL(MadSciensteinFunc, Func_45425)
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

PORT_JUMP(InvisibilityPotionFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_InvisibilityPotionFunc);
}

PORT_JUMP(InvisibilityPotionFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_InvisibilityPotionFunc); // fallthrough
}

PORT_JUMP(InvisibilityPotionFunc)
{
	gb->cpu.l = OBJ_FLAGS;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DestroyInvisibilityPotion);
		return;
	}
	SET_BC(gb, ADDR_Data_60510);
	JUMP(ApplyObjYMovement);
}

PORT(DestroyInvisibilityPotion)
{
	FARCALL(VanishObject_NoSFX);
}

// Breaks the potion once it lands.
PORT(Func_45453)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, ADDR_Func_45453 + 0x12);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	FARCALL(VanishObject_NoSFX); // DestroyInvisibilityPotion
}

// .Update
static void potion_drop_update(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DestroyInvisibilityPotion);
		return;
	}
	SET_BC(gb, ADDR_Data_60540);
	CALL(ApplyObjYMovement);
	JUMP_TO(ADDR_Func_45453);
}

PORT_JUMP(InvisibilityPotionFunc_Drop)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(INVISIBILITY_POTION_DROP_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)INVISIBILITY_POTION_DROP_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	hl = DEC_L(hl);
	gb->cpu.a = 0x8f;
	wr8(gb, hl, 0x8f);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	potion_drop_update(gb);
}

PORT_LOCAL_JUMP(InvisibilityPotionFunc_Drop, Update) { potion_drop_update(gb); }
