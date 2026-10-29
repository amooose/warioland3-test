// Port of src/engine/level/objects/hebarii.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)

#define HB(off) (ADDR_HebariiFunc + (off))
#define HB_INIT HB(0xa) // .Init
#define HB_ATTACHED_UPDATE HB(0x73)
#define HB_DETACHED_UPDATE HB(0x157)
#define HB_STATE18 HB(0x36c)
#define HB_STATE19 HB(0x34d)
#define HP(off) (ADDR_HebariiProjectileFunc + (off))
#define HP_MOVE_WITH_LATERAL HP(0x3f)
#define HP_MOVE_WITHOUT_LATERAL HP(0x4d)
#define HP_DESTROY HP(0x8c)

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

// ld a, no_actions_for 1 / ld [wCurObjAction], a
static inline void no_actions_for_1(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// ld de, frameset / call SetObjectFramesetPtr
static inline void set_frameset(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
}

PORT(HebariiFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HB_INIT >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HB_INIT;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL(HebariiFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 5);
	wr8(gb, hl--, (uint8_t)-7);
	wr8(gb, hl, (uint8_t)-12);
	SET_HL(gb, set_update(gb, hl, HB_ATTACHED_UPDATE));
	set_frameset(gb, ADDR_Frameset_683e4);
	hl = REG_HL(gb);
	wr8(gb, hl++, 20); // OBJ_STATE_DURATION
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0); // OBJ_VAR_1
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
}

// .asm_4a3af: hands the object to the update function a:c, with
// OBJ_VAR_2 = b.
static void knocked(GB *gb, uint16_t func)
{
	gb->cpu.a = (uint8_t)(func >> 8);
	gb->cpu.c = (uint8_t)func;
	gb->cpu.b = 0x02;
	uint16_t hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0); // OBJ_VAR_3
	wr8(gb, hl, gb->cpu.b); // OBJ_VAR_2
	SET_HL(gb, hl);
	no_actions_for_1(gb);
}

static void state28(GB *gb) { set_frameset(gb, ADDR_Frameset_6842d); knocked(gb, ADDR_Func_33f8); }
static void state29(GB *gb) { set_frameset(gb, ADDR_Frameset_6842a); knocked(gb, ADDR_Func_3407); }
static void attacked_right_start(GB *gb) { set_frameset(gb, ADDR_Frameset_6842d); knocked(gb, ADDR_Func_3317); }
static void attacked_left_start(GB *gb) { set_frameset(gb, ADDR_Frameset_6842a); knocked(gb, ADDR_Func_3326); }

PORT_LOCAL(HebariiFunc, State28) { state28(gb); }
PORT_LOCAL(HebariiFunc, State29) { state29(gb); }
PORT_LOCAL(HebariiFunc, AttackedRightStart) { attacked_right_start(gb); }
PORT_LOCAL(HebariiFunc, AttackedLeftStart) { attacked_left_start(gb); }

// For when Hebarii is still attached to the ceiling: walks along it and
// drips projectiles.
PORT_LOCAL_JUMP(HebariiFunc, AttachedUpdate)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		uint16_t hl = wCurObjUpdateFunction + 1;
		gb->cpu.a = (uint8_t)(HB_DETACHED_UPDATE >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)HB_DETACHED_UPDATE;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
		set_frameset(gb, ADDR_Frameset_683f8);
		hl = LO(REG_HL(gb), OBJ_COLLBOX_RIGHT);
		wr8(gb, hl--, 5);
		wr8(gb, hl--, (uint8_t)-6);
		wr8(gb, hl--, (uint8_t)-1);
		wr8(gb, hl--, (uint8_t)-14);
		gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
		wr8(gb, hl--, gb->cpu.a); // OBJ_INTERACTION_TYPE
		SET_HL(gb, hl);
		gb->cpu.a = OBJSTATE_SPECIAL_1;
		wr8(gb, wCurObjState, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjVar3, 0);
		RET();
		return;
	}
	// .no_shake1
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (alu_and(gb, s, s)) {
		static const struct { uint8_t state; void (*f)(GB *); } starts[] = {
			{ OBJSTATE_ATTACKED_LEFT_START, attacked_left_start },
			{ OBJSTATE_ATTACKED_RIGHT_START, attacked_right_start },
			{ OBJSTATE_28, state28 },
			{ OBJSTATE_29, state29 },
		};
		for (int i = 0; i < 4; i++) {
			alu_cp(gb, s, starts[i].state);
			if (gb->cpu.f & FLAG_Z) {
				// .StartAttackedLeftStart etc.
				SET_HL(gb, set_update(gb, hl, HB_DETACHED_UPDATE));
				starts[i].f(gb);
				RET();
				return;
			}
		}
		alu_cp(gb, s, OBJSTATE_VANISH_TOUCH);
		if (gb->cpu.f & FLAG_Z) {
			JUMP(VanishObject2);
			return;
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		hl = LO(hl, OBJ_FLAGS);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_NO_COLLISION));
	}
	// .asm_4a08d
	hl = LO(hl, OBJ_VAR_1);
	gb->cpu.a = alu_dec(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	if (!gb->cpu.a) {
		// .asm_4a0fd
		gb->cpu.a = rd8(gb, hl); // OBJ_STATE_DURATION
		alu_cp(gb, gb->cpu.a, 30);
		if (gb->cpu.f & FLAG_Z) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			hl = LO(hl, OBJ_FLAGS);
			SET_HL(gb, hl);
			if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
				RET();
				return;
			}
			SET_BC(gb, ADDR_ObjParams_HebariiProjectile);
			JUMP(CreateObjectAtRelativePos);
			return;
		}
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		gb->cpu.a = 126;
		wr8(gb, hl++, 126);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0); // OBJ_VAR_1
		SET_HL(gb, hl);
		set_frameset(gb, ADDR_Frameset_683e4);
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		set_frameset(gb, ADDR_Frameset_683ed);
		hl = REG_HL(gb);
		wr8(gb, hl++, 41); // OBJ_STATE_DURATION
		gb->cpu.a = 0x01;
		wr8(gb, hl, 0x01); // OBJ_VAR_1
		SET_HL(gb, hl);
		RET();
		return;
	}
	// .asm_4a0a3
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (gb->cpu.a) {
		RET();
		return;
	}
	hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 18);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 9);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 11);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	// .asm_4a0d2
	gb_call_ret(gb, ADDR_Func_3513, HB(0x111));
	bool turn = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!turn) {
		hl = wCurObjYPos;
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 8);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		wr8(gb, hYPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_3513, HB(0x124));
		turn = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!turn) {
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		JUMP_TO((gb->cpu.f & FLAG_C) ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
		return;
	}
	// .switch_direction
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	RET();
}

// rst JumpTable on the object's state
PORT_LOCAL_JUMP(HebariiFunc, DetachedUpdate)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	gb_push16(gb, HB_DETACHED_UPDATE + 4); // rst JumpTable
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

PORT_LOCAL_JUMP(HebariiFunc, Vanish) { JUMP(VanishObject2); }

PORT_LOCAL_JUMP(HebariiFunc, FallFromCeiling)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, HB(0x230));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wIsInWaterOrSand);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP(VanishObject2);
			return;
		}
		SET_BC(gb, ADDR_FallingYVel_Light);
		JUMP(ApplyObjYMovement);
		return;
	}
	// .asm_4a204
	hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = OBJSTATE_WALK;
	wr8(gb, wCurObjState, gb->cpu.a);
	set_frameset(gb, ADDR_Frameset_683fb);
	hl = REG_HL(gb);
	wr8(gb, hl++, 160);
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_00);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	RET();
}

// .State3f: waits, then despawns.
static void state3f(GB *gb)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.l = OBJ_UNK_02;
	FARCALL(DespawnObject);
}

// .State0f
static void state0f(GB *gb)
{
	no_actions_for_1(gb);
	gb->cpu.a = OBJSTATE_3F;
	wr8(gb, wCurObjState, gb->cpu.a);
	set_frameset(gb, ADDR_Frameset_68430);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 35;
	wr8(gb, hl++, 35);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_GRABBED));
	state3f(gb);
}

PORT_LOCAL(HebariiFunc, State0f) { state0f(gb); }
PORT_LOCAL(HebariiFunc, State3f) { state3f(gb); }

PORT_LOCAL(HebariiFunc, Func_4a224)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_rra(gb, rd8(gb, rDIV));
		// either OBJSTATE_WOBBLE_LEFT_START or OBJSTATE_WOBBLE_RIGHT_START
		gb->cpu.a = alu_rla(gb, OBJSTATE_WOBBLE_LEFT_START >> 1);
		wr8(gb, wCurObjState, gb->cpu.a);
		return;
	}
	// .no_shake2
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	state0f(gb);
}

// .Func_4a26d / .Func_4a277
static void wobble(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	wr8(gb, hl--, 15 | (1 << 7));
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_683fb);
	hl = REG_HL(gb);
	wr8(gb, hl++, 4); // OBJ_STATE_DURATION
	hl = INC_L(hl);
	wr8(gb, hl++, 0x02); // OBJ_VAR_2
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0); // OBJ_VAR_3
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_00);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL(HebariiFunc, Func_4a26d) { wobble(gb, OBJSTATE_WOBBLE_LEFT); }
PORT_LOCAL(HebariiFunc, Func_4a277) { wobble(gb, OBJSTATE_WOBBLE_RIGHT); }

// .StunLeft / .StunRight
static void stun(GB *gb, uint16_t func)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(func);
		return;
	}
	gb->cpu.a = OBJSTATE_0F;
	wr8(gb, wCurObjState, gb->cpu.a);
	RET();
}

// .StunLeftStart / .StunRightStart
static void stun_start(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_683fb);
}

PORT_LOCAL_JUMP(HebariiFunc, StunLeftStart)
{
	stun_start(gb, OBJSTATE_STUN_LEFT);
	stun(gb, ADDR_Func_32ae);
}
PORT_LOCAL_JUMP(HebariiFunc, StunLeft) { stun(gb, ADDR_Func_32ae); }
PORT_LOCAL_JUMP(HebariiFunc, StunRightStart)
{
	stun_start(gb, OBJSTATE_STUN_RIGHT);
	stun(gb, ADDR_Func_32bd);
}
PORT_LOCAL_JUMP(HebariiFunc, StunRight) { stun(gb, ADDR_Func_32bd); }

// .State14 / .State15
static void state14(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
}

PORT_LOCAL(HebariiFunc, State14) { state14(gb, OBJSTATE_44); }
PORT_LOCAL(HebariiFunc, State15) { state14(gb, OBJSTATE_45); }

// .PickedUpLeftStart / .PickedUpRightStart
static void picked_up(GB *gb, uint8_t state, uint8_t var2)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = DEC_L(hl);
	wr8(gb, hl--, 65); // OBJ_VAR_3
	wr8(gb, hl--, var2); // OBJ_VAR_2
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01); // OBJ_VAR_1
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_683fb);
}

PORT_LOCAL_JUMP(HebariiFunc, PickedUpLeftStart)
{
	picked_up(gb, OBJSTATE_PICKED_UP_LEFT, 0xc0);
	JUMP(Func_3335);
}
PORT_LOCAL_JUMP(HebariiFunc, PickedUpRightStart)
{
	picked_up(gb, OBJSTATE_PICKED_UP_RIGHT, 0xe0);
	JUMP(Func_3344);
}

// .State18 / .State19 up to .State48 / .State49
static void grabbed(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
	set_frameset(gb, ADDR_Frameset_683fb);
	uint16_t hl = LO(REG_HL(gb), OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_GRABBED));
}

// .State49 / .State48: follows Wario's direction while carried.
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
	JUMP_TO(left ? HB_STATE18 : HB_STATE19);
}

PORT_LOCAL_JUMP(HebariiFunc, State19)
{
	grabbed(gb, OBJSTATE_49);
	carried(gb, true);
}
PORT_LOCAL_JUMP(HebariiFunc, State49) { carried(gb, true); }
PORT_LOCAL_JUMP(HebariiFunc, State18)
{
	grabbed(gb, OBJSTATE_48);
	carried(gb, false);
}
PORT_LOCAL_JUMP(HebariiFunc, State48) { carried(gb, false); }

PORT_LOCAL(HebariiFunc, State12)
{
	uint16_t hl = wCurObjAction;
	wr8(gb, hl--, 31 | (1 << 7));
	wr8(gb, hl--, OBJSTATE_42); // OBJ_STATE
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0); // OBJ_VAR_3
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(HebariiFunc, FullThrowLeftStart)
{
	gb->cpu.a = OBJSTATE_FULL_THROW_LEFT;
	wr8(gb, wCurObjState, gb->cpu.a);
	set_frameset(gb, ADDR_Frameset_68421);
	JUMP(ObjState_FullThrowLeft);
}

PORT_LOCAL_JUMP(HebariiFunc, FullThrowRightStart)
{
	gb->cpu.a = OBJSTATE_FULL_THROW_RIGHT;
	wr8(gb, wCurObjState, gb->cpu.a);
	set_frameset(gb, ADDR_Frameset_68418);
	JUMP(ObjState_FullThrowRight);
}

// Falls towards Wario's side if he's not right below.
PORT(HebariiProjectileFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 100;
	wr8(gb, hl, 100);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x18);
	bool straight = gb->cpu.f & FLAG_C;
	if (!straight) {
		alu_cp(gb, gb->cpu.a, 0xe8);
		straight = !(gb->cpu.f & FLAG_C);
	}
	hl = wCurObjUpdateFunction + 1;
	if (straight) {
		gb->cpu.a = (uint8_t)(HP_MOVE_WITHOUT_LATERAL >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)HP_MOVE_WITHOUT_LATERAL;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x80);
	bool right = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = (uint8_t)(HP_MOVE_WITH_LATERAL >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HP_MOVE_WITH_LATERAL;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	if (right)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
}

// .MoveWithoutLateral: falls, and bursts when it lands or is hit.
static void projectile_fall(GB *gb)
{
	CALL(MoveObjectDown_Slow);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	bool destroy = !d;
	if (!destroy) {
		gb->cpu.a = rd8(gb, wCurObjState);
		destroy = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!destroy) {
		hl = wCurObjYPos;
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
		wr8(gb, hXPosLo, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_352b, HP(0x6e));
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
			return;
		hl = wCurObjYPos;
		wr8(gb, hl++, rd8(gb, hYPosLo));
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, hl, gb->cpu.a);
	}
	// .set_destroy
	hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HP_DESTROY >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HP_DESTROY;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_6840d);
	hl = REG_HL(gb);
	gb->cpu.a = 0x18;
	wr8(gb, hl++, 0x18);
	SET_HL(gb, hl);
}

PORT_LOCAL(HebariiProjectileFunc, MoveWithLateral)
{
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C)
		CALL(MoveObjectRight_Slow);
	else
		CALL(MoveObjectLeft_Slow);
	projectile_fall(gb);
}

PORT_LOCAL(HebariiProjectileFunc, MoveWithoutLateral) { projectile_fall(gb); }

PORT_LOCAL(HebariiProjectileFunc, Destroy)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
