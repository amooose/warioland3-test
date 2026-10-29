// Port of src/engine/level/objects/omodonmeka.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define OMODONMEKA_FUNC_42535 (ADDR_OmodonmekaWithOmodonFunc + 0x4b) // .Func_42535
#define OMODONMEKA_FUNC_42559 (ADDR_OmodonmekaWithOmodonFunc + 0x6f) // .Func_42559

// Spawns the Omodon riding the Omodonmeka (unless Func_3444 says otherwise).
static void omodonmeka_with_omodon(GB *gb, uint16_t omodon)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(ADDR_OmodonmekaWithOmodonFunc >> 8));
	wr8(gb, hl--, (uint8_t)ADDR_OmodonmekaWithOmodonFunc);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0xfd);
	wr8(gb, hl--, 0xf3); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 0xf9;
	wr8(gb, hl--, 0xf9); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	CALL(Func_3444);
	gb->cpu.a = gb->cpu.e;
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(OMODONMEKA_FUNC_42559);
		return;
	}
	SET_BC(gb, omodon);
	CALL(CreateObjectAtRelativePos);
	SET_DE(gb, ADDR_Frameset_68f14);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x00;
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x00);
	SET_HL(gb, hl);
	wr8(gb, wCurObjVar3, 0x00);
	JUMP_TO(OMODONMEKA_FUNC_42535);
}

PORT_JUMP(OmodonmekaWithOmodon1Func) { omodonmeka_with_omodon(gb, ADDR_ObjParams_Omodon1); }
PORT_JUMP(OmodonmekaWithOmodon2Func) { omodonmeka_with_omodon(gb, ADDR_ObjParams_Omodon2); }

PORT(Func_42725)
{
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.b = (uint8_t)(ADDR_Data_40000 >> 8);
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)ADDR_Data_40000);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_sub(gb, gb->cpu.c, 0x3f);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_BC(gb));
		gb->cpu.c = 0x01;
		wr8(gb, hl, 0x01);
		hl = LO(hl, OBJ_STATE_DURATION);
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	SET_HL(gb, hl);
	// .apply_y_offset
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	hl = LO(hl, OBJ_Y_POS);
	alu_cp(gb, gb->cpu.a, 0x80);
	gb->cpu.c = rd8(gb, hl);
	bool negative = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	bool carry = gb->cpu.f & FLAG_C;
	if (!negative) {
		if (!carry)
			return;
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	} else {
		if (carry)
			return;
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
}

PORT(Func_42754)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << OBJFLAG_STEPPED_F)));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	uint8_t d = alu_dec(gb, gb->cpu.a);
	wr8(gb, hl, d);
	if (d)
		return;
	SET_DE(gb, ADDR_Frameset_68f50);
	CALL(SetObjectFramesetPtr);
}

#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define OM(off) (ADDR_OmodonmekaFunc + (off))
#define OM_FUNC_42199 OM(0x29)
#define OM_FUNC_42218 OM(0xa8)
#define OM_JUMPTABLE OM(0xd9 + 6) // after .asm_42249's rst JumpTable
#define OM_FUNC_42432 OM(0x2c2)

// ld a, no_actions_for 1 / ld [wCurObjAction], a
static inline void no_actions_for_1(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// ld l, OBJ_INTERACTION_TYPE / ld a, [hl] / and HEAVY_OBJ / or type / ld [hld], a
static inline uint16_t set_interaction(GB *gb, uint16_t hl, uint8_t type)
{
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld l, OBJ_SUBSTATE / ld a, [hl] / and mask / or v / ld [hld], a
static inline uint16_t set_substate(GB *gb, uint16_t hl, uint8_t mask, uint8_t v)
{
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), mask), v);
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

PORT_JUMP(OmodonmekaFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(OM_FUNC_42218 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)OM_FUNC_42218;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0xfd);
	wr8(gb, hl--, 0xf3);
	gb->cpu.a = 0xf9;
	wr8(gb, hl--, 0xf9);
	hl = LO(hl, OBJ_VAR_1);
	wr8(gb, hl++, rd8(gb, wCurObjYPos + 0));
	gb->cpu.a = rd8(gb, wCurObjYPos + 1);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	JUMP_TO(OM_FUNC_42432);
}

// .asm_42249: rst JumpTable on the low nibble of OBJ_SUBSTATE
static void om_dispatch(GB *gb)
{
	uint16_t hl = LO(REG_HL(gb), OBJ_SUBSTATE);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
	gb_push16(gb, OM_JUMPTABLE); // rst JumpTable
	hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

// .Func_42199 (hl = wCurObjState): Wario got on.
PORT_LOCAL_JUMP(OmodonmekaFunc, Func_42199)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_DE(gb, ADDR_Frameset_68f14);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FLAT_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x18;
		wr8(gb, hl++, 0x18);
		gb->cpu.e = 0x07;
		gb->cpu.d = 0x07;
	} else {
		gb->cpu.e = 0x05;
		gb->cpu.d = 0x1a;
	}
	hl = LO(hl, OBJ_Y_POS);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), gb->cpu.d);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl), 0);
	wr8(gb, wWarioXPos + 0, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), 0xf0), gb->cpu.e);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0xf9;
	wr8(gb, wCurObjCollBoxBottom, 0xf9);
	om_dispatch(gb);
}

PORT_LOCAL_JUMP(OmodonmekaFunc, Func_42218)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (!alu_and(gb, s, s)) {
		om_dispatch(gb);
		return;
	}
	alu_cp(gb, s, 0x2a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(OM_FUNC_42199);
		return;
	}
	static const struct { uint8_t state; uint16_t func; } knocked[] = {
		{ 0x04, ADDR_Func_3326 }, // .asm_421ec
		{ 0x05, ADDR_Func_3317 }, // .asm_421f4
		{ 0x28, ADDR_Func_33f8 }, // .asm_421dc
		{ 0x29, ADDR_Func_3407 }, // .asm_421e4
	};
	for (int i = 0; i < 4; i++) {
		alu_cp(gb, s, knocked[i].state);
		if (!(gb->cpu.f & FLAG_Z))
			continue;
		gb->cpu.a = (uint8_t)(knocked[i].func >> 8);
		gb->cpu.c = (uint8_t)knocked[i].func;
		gb->cpu.b = 0x02;
		// .asm_421fa
		hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
		wr8(gb, hl--, gb->cpu.a);
		wr8(gb, hl, gb->cpu.c);
		hl = LO(hl, OBJ_VAR_3);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl--, 0);
		gb->cpu.a = gb->cpu.b;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
		SET_DE(gb, ADDR_Frameset_68f3d);
		CALL(SetObjectFramesetPtr);
		no_actions_for_1(gb);
		gb->cpu.a = 0x3a;
		wr8(gb, wCurObjState, 0x3a);
		RET();
		return;
	}
	alu_cp(gb, s, 0x0a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(Func_3173);
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), 0xf0), 0x04);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f34);
	CALL(SetObjectFramesetPtr);
	om_dispatch(gb);
}

// The check at the start of the carrying states: once the auto-move is
// over, back to state 1. Returns true if so.
static bool auto_move_over(GB *gb)
{
	no_actions_for_1(gb);
	gb->cpu.a = rd8(gb, wAutoMoveState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return false;
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wCurObjState, gb->cpu.a);
	return true;
}

PORT_LOCAL(OmodonmekaFunc, Func_42261)
{
	if (auto_move_over(gb))
		return;
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	gb->cpu.a = d;
	alu_cp(gb, d, 0x10);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.e = 0x0e;
	} else {
		alu_cp(gb, d, 0x07);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.e = 0x16;
	}
	// .asm_42283
	hl = LO(hl, OBJ_Y_POS);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), gb->cpu.e);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
}

// Rises with Wario on it until it hits the ceiling or its top position.
PORT_LOCAL_JUMP(OmodonmekaFunc, Func_42291)
{
	if (auto_move_over(gb)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x1d);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_358b, OM(0x147));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	bool stop = gb->cpu.a != 0;
	if (!stop) {
		hl = wCurObjVar1;
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x40);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl), 0);
		gb->cpu.d = gb->cpu.a;
		hl = LO(hl, OBJ_Y_POS + 1);
		gb->cpu.a = rd8(gb, hl--);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.d);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, hl);
			alu_cp(gb, gb->cpu.a, gb->cpu.e);
			stop = gb->cpu.f & FLAG_Z;
		}
	}
	if (stop) {
		// .asm_422d0
		hl = set_substate(gb, wCurObjSubState, 0xf0, 0x06);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		SET_HL(gb, hl);
		SET_DE(gb, ADDR_Frameset_68f14);
		CALL(SetObjectFramesetPtr);
		RET();
		return;
	}
	// .asm_422e2
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (!gb->cpu.a)
		play_sfx(gb, SFX_087);
	gb->cpu.b = 0x01;
	CALL(MoveWarioUp);
	CALL(HandleUpwardsFloorTransition);
	JUMP(MoveObjectUp);
}

PORT_LOCAL(OmodonmekaFunc, Func_422fc)
{
	if (auto_move_over(gb))
		return;
	SET_BC(gb, ADDR_Data_604c0);
	CALL(ApplyObjYMovement_Loop);
	uint16_t hl = LO(REG_HL(gb), OBJ_Y_POS);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x1a);
	wr8(gb, wWarioYPos + 1, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	wr8(gb, wWarioYPos + 0, gb->cpu.a);
	SET_HL(gb, hl);
}

// Hovers; opens its mouth when Wario's close and above.
PORT_LOCAL(OmodonmekaFunc, Func_42321)
{
	SET_BC(gb, ADDR_Data_604c0);
	CALL(ApplyObjYMovement_Loop);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
		return;
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x22);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x28);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_cp(gb, gb->cpu.a, 0xd8);
		if (gb->cpu.f & FLAG_C)
			return;
	}
	// .asm_42347
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_C))
		return;
	SET_DE(gb, ADDR_Frameset_68f1d);
	CALL(SetObjectFramesetPtr);
	SET_HL(gb, set_substate(gb, REG_HL(gb), 0xf0, 0x08));
	play_sfx(gb, SFX_08C);
}

// Mouth open: closes it again when Wario moves away, or swallows him.
PORT_LOCAL(OmodonmekaFunc, Func_42369)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1f);
	if (!gb->cpu.a)
		play_sfx(gb, SFX_08C);
	SET_BC(gb, ADDR_Data_604c0);
	CALL(ApplyObjYMovement_Loop);
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x22);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	int path; // 0: .asm_4239b, 1: .asm_423ab, 2: .asm_423b8
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C) {
		path = 2;
	} else if (alu_cp(gb, gb->cpu.a, 0xf0), !(gb->cpu.f & FLAG_C)) {
		path = 2;
	} else if (alu_cp(gb, gb->cpu.a, 0x28), gb->cpu.f & FLAG_C) {
		path = 1;
	} else if (alu_cp(gb, gb->cpu.a, 0xd8), !(gb->cpu.f & FLAG_C)) {
		path = 1;
	} else {
		path = 0;
	}
	if (path) {
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		bool c = gb->cpu.f & FLAG_C;
		if (path == 1) {
			if (c)
				return;
		} else {
			if (!c)
				return;
			SET_DE(gb, ADDR_Frameset_68f2b);
			CALL(SetObjectFramesetPtr);
			uint16_t hl = set_substate(gb, REG_HL(gb), 0xf0, 0x02);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			SET_HL(gb, hl);
			gb->cpu.a = 0x00;
			wr8(gb, wCurObjCollBoxBottom, 0x00);
			play_sfx(gb, SFX_088);
			return;
		}
	}
	// .asm_4239b
	SET_HL(gb, set_substate(gb, wCurObjSubState, 0xf0, 0x01));
	SET_DE(gb, ADDR_Frameset_68f0b);
	CALL(SetObjectFramesetPtr);
}

// Falls back to the ground.
PORT_LOCAL(OmodonmekaFunc, Func_423e1)
{
	SET_BC(gb, ADDR_Data_60500);
	CALL(ApplyObjYMovement);
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, OM(0x289));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_01);
	SET_HL(gb, hl);
	gb->cpu.a = 0xf9;
	wr8(gb, wCurObjCollBoxBottom, 0xf9);
	SET_DE(gb, ADDR_Frameset_68f14);
	CALL(SetObjectFramesetPtr);
	SET_HL(gb, set_substate(gb, REG_HL(gb), 0xf0, 0x03));
}

// .Func_42432: hovering, waiting for Wario.
static void hover(GB *gb)
{
	uint16_t hl = set_substate(gb, wCurObjSubState, OBJSUBFLAGS_MASK, 0x1);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f0b);
	CALL(SetObjectFramesetPtr);
	SET_HL(gb, set_interaction(gb, REG_HL(gb), OBJ_INTERACTION_OMODONMEKA));
}

PORT_LOCAL(OmodonmekaFunc, Func_42432) { hover(gb); }

// Compares OBJ_Y_POS with the starting position in OBJ_VAR_1 / OBJ_VAR_2;
// returns whether it's back there.
static bool at_start(GB *gb)
{
	uint16_t hl = wCurObjVar1;
	gb->cpu.e = rd8(gb, hl);
	hl = INC_L(hl);
	gb->cpu.d = rd8(gb, hl);
	hl = LO(hl, OBJ_Y_POS + 1);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (!(gb->cpu.f & FLAG_Z))
		return false;
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	return gb->cpu.f & FLAG_Z;
}

// Rises back to its starting position.
PORT_LOCAL_JUMP(OmodonmekaFunc, Func_42420)
{
	if (!at_start(gb)) {
		JUMP(MoveObjectUp);
		return;
	}
	hover(gb);
	RET();
}

PORT_LOCAL(OmodonmekaFunc, Func_4244c)
{
	no_actions_for_1(gb);
	SET_BC(gb, ADDR_Data_604e0);
	CALL(ApplyObjYMovement);
	SET_BC(gb, ADDR_Data_60650);
	CALL(ApplyObjXMovement_Loop);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FLAT_WARIO);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	if (!at_start(gb))
		return;
	gb->cpu.a = 0xf9;
	wr8(gb, wCurObjCollBoxBottom, 0xf9);
	hover(gb);
}

#define OWO(off) (ADDR_OmodonmekaWithOmodonFunc + (off))
#define OWO_JUMPTABLE OWO(0x3b + 6) // after .asm_42525's rst JumpTable
#define OWO_FUNC_42714 OWO(0x22a)
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)

// With its Omodon: handles hits like OmodonmekaFunc, else runs the state
// in the low nibble of OBJ_SUBSTATE.
PORT_JUMP(OmodonmekaWithOmodonFunc)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (!alu_and(gb, s, s)) {
		// .asm_42525
		hl = LO(hl, OBJ_SUBSTATE);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
		gb_push16(gb, OWO_JUMPTABLE); // rst JumpTable
		hl = gb_pop16(gb); // pop hl
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.d = 0;
		hl = alu_add16(gb, hl, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl);
		gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
		gb->cpu.l = gb->cpu.a;
		JUMP_TO(REG_HL(gb));
		return;
	}
	static const struct { uint8_t state; uint16_t target; } hits[] = {
		{ 0x04, OM(0x7c) }, // OmodonmekaFunc.asm_421ec
		{ 0x05, OM(0x84) }, // OmodonmekaFunc.asm_421f4
		{ 0x28, OM(0x6c) }, // OmodonmekaFunc.asm_421dc
		{ 0x29, OM(0x74) }, // OmodonmekaFunc.asm_421e4
		{ 0x0a, OM(0xa5) }, // OmodonmekaFunc.asm_42215
	};
	for (int i = 0; i < 5; i++) {
		alu_cp(gb, s, hits[i].state);
		if (gb->cpu.f & FLAG_Z) {
			JUMP_TO(hits[i].target);
			return;
		}
	}
	gb->cpu.a = 0x3a;
	wr8(gb, hl--, 0x3a);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f34);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x02);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(OWO_FUNC_42714 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)OWO_FUNC_42714;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	JUMP_TO(OWO_FUNC_42714);
}

// Waits for Wario to step on it.
PORT_LOCAL_JUMP(OmodonmekaWithOmodonFunc, Func_42535)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FLAT_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_42725);
		return;
	}
	uint16_t hl = set_substate(gb, wCurObjSubState, OBJSUBFLAGS_MASK, 0x2);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f26);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x3c;
	wr8(gb, hl++, 0x3c);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
	RET();
}

// .Func_42559
static void func_42559(GB *gb)
{
	uint16_t hl = set_substate(gb, wCurObjSubState, OBJSUBFLAGS_MASK, 0x3);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f2b);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x28;
	wr8(gb, hl++, 0x28);
	SET_HL(gb, hl);
}

PORT_LOCAL(OmodonmekaWithOmodonFunc, Func_42554)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	func_42559(gb);
}

PORT_LOCAL(OmodonmekaWithOmodonFunc, Func_42559) { func_42559(gb); }

// Rises, carrying Wario if he's on it.
PORT_LOCAL_JUMP(OmodonmekaWithOmodonFunc, Func_426ee)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		gb->cpu.b = 0x01;
		CALL(MoveWarioUp);
		CALL(HandleUpwardsFloorTransition);
	}
	// .asm_42702
	hl = LO(REG_HL(gb), OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(MoveObjectUp);
		return;
	}
	hl = set_substate(gb, wCurObjSubState, 0xf0, 0x01);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	RET();
}

PORT_LOCAL_JUMP(OmodonmekaWithOmodonFunc, Func_42714)
{
	no_actions_for_1(gb);
	SET_BC(gb, ADDR_Data_604e0);
	CALL(ApplyObjYMovement);
	SET_BC(gb, ADDR_Data_60650);
	JUMP(ApplyObjXMovement_Loop);
}

// The Omodon riding it.
PORT(Func_42767)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FLAT_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gb_call_ret(gb, ADDR_Func_42725, ADDR_Func_42767 + 0x0f);
		gb_call_ret(gb, ADDR_Func_4279c + 0xf8, ADDR_Func_42767 + 0x12); // Func_4279c.Func_42894
		hl = wCurObjFlags;
		SET_HL(gb, hl);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
		return;
	}
	// .asm_4277f
	hl = wCurObjState;
	gb->cpu.a = 0x2e;
	wr8(gb, hl--, 0x2e);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f42);
	CALL(SetObjectFramesetPtr);
	hl = set_interaction(gb, REG_HL(gb), OBJ_INTERACTION_OMODON);
	hl = LO(hl, OBJ_COLLBOX_LEFT);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	hl = INC_L(hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
}

// Looks for the active object (flags $41) whose OBJ_UNK_01/02 is c:b (or
// e:d), starting at hl = wObj1. Returns the slot's low byte, or -1; hl ends
// after the compared bytes, like the asm.
static int find_obj(GB *gb, uint8_t mask, uint8_t lo, uint8_t hi, bool inc_last)
{
	for (int base = 0; base < 0x100; base += 0x20) {
		uint16_t hl = (uint16_t)((wObj1 & 0xff00) | base);
		gb->cpu.a = alu_and(gb, rd8(gb, hl++), mask);
		alu_cp(gb, gb->cpu.a, mask);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, hl++);
			alu_cp(gb, gb->cpu.a, lo);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = rd8(gb, hl);
				if (inc_last)
					hl++;
				alu_cp(gb, gb->cpu.a, hi);
				if (gb->cpu.f & FLAG_Z) {
					SET_HL(gb, hl);
					return base;
				}
			}
		}
		SET_HL(gb, hl);
	}
	return -1;
}

// Swallowing Wario: lines up with the object riding it (Wario) and then
// sets that object's state.
PORT_LOCAL_JUMP(OmodonmekaWithOmodonFunc, Func_4256c)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x28);
	if (gb->cpu.f & FLAG_Z) {
		hl = LO(hl, OBJ_UNK_01);
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.b = rd8(gb, hl);
		gb->cpu.d = 0x41;
		int base = find_obj(gb, gb->cpu.d, gb->cpu.c, gb->cpu.b, false);
		if (base < 0) {
			RET();
			return;
		}
		hl = (uint16_t)((wObj1 & 0xff00) | (base + OBJ_SCREEN_Y_POS));
		SET_HL(gb, hl);
		// .Func_4261f
		gb->cpu.a = rd8(gb, wCurObjScreenYPos);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, rd8(gb, hl), 0x10);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = alu_sub(gb, rd8(gb, hl), 0x0f);
			alu_cp(gb, gb->cpu.a, gb->cpu.c);
			if (!(gb->cpu.f & FLAG_Z)) {
				JUMP(MoveObjectDown);
				return;
			}
		}
		hl = wCurObjStateDuration;
		SET_HL(gb, hl);
	}
	// .asm_42635
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	hl = set_substate(gb, wCurObjSubState, 0xf0, 0x04);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68f14);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x32;
	wr8(gb, hl++, 0x32);
	hl = wCurObjUnk01;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	gb->cpu.b = 0x41;
	int base = find_obj(gb, gb->cpu.b, gb->cpu.e, gb->cpu.d, true);
	if (base < 0) {
		RET();
		return;
	}
	// .asm_426cc
	hl = REG_HL(gb);
	gb->cpu.a = alu_add(gb, (uint8_t)hl, 0x18);
	hl = LO(hl, gb->cpu.a);
	wr8(gb, hl--, 0x2d);
	gb->cpu.a = alu_sub(gb, (uint8_t)hl, 0x04);
	hl = LO(hl, gb->cpu.a);
	wr8(gb, hl--, 0x33);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl--, 0);
	wr8(gb, hl--, 0x4f);
	wr8(gb, hl--, 0x42);
	gb->cpu.a = alu_sub(gb, (uint8_t)hl, 0x07);
	hl = LO(hl, gb->cpu.a);
	gb->cpu.a = 0x10;
	wr8(gb, hl++, 0x10);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	hl = INC_L(hl);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
	RET();
}

#define OMODON(off) (ADDR_Func_4279c + (off))
#define OMODON_FUNC_427A9 OMODON(0xd)
#define OMODON_FUNC_42894 OMODON(0xf8)
#define OMODON_FUNC_42935 OMODON(0x199)

// The scan of .Func_42894 / .Func_42935: looks for the object whose flags
// & d are e and whose OBJ_UNK_01/02 is c:b. Returns its low byte or -1; hl
// is left like the asm leaves it (on OBJ_UNK_01 of the match).
static int find_owner(GB *gb)
{
	for (int base = 0; base < 0x100; base += 0x20) {
		uint16_t hl = (uint16_t)((wObj1 & 0xff00) | base);
		gb->cpu.a = alu_and(gb, rd8(gb, hl++), gb->cpu.d);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, hl++);
			alu_cp(gb, gb->cpu.a, gb->cpu.c);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = rd8(gb, hl--);
				alu_cp(gb, gb->cpu.a, gb->cpu.b);
				if (gb->cpu.f & FLAG_Z) {
					SET_HL(gb, hl);
					return base;
				}
			}
		}
		SET_HL(gb, hl);
	}
	return -1;
}

// ld hl, wCurObjUnk01 / ld a, [hli] / ld c, a / ld b, [hl] / ld d, d /
// ld e, e / ld h, HIGH(wObj1)
static void owner_key(GB *gb, uint8_t d, uint8_t e)
{
	uint16_t hl = wCurObjUnk01;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = rd8(gb, hl);
	gb->cpu.d = d;
	gb->cpu.e = e;
}

// Copies its own direction to the Omodonmeka it rides.
PORT_LOCAL_JUMP(Func_4279c, Func_42894)
{
	owner_key(gb, 0x43, 0x03);
	if (find_owner(gb) < 0) {
		RET();
		return;
	}
	// .asm_42919
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjFlags));
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	uint16_t hl = DEC_L(REG_HL(gb));
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	uint8_t a = gb->cpu.a;
	gb->cpu.e = alu_sla(gb, gb->cpu.e);
	gb->cpu.a = alu_rra(gb, a);
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = alu_add(gb, (uint8_t)hl, 0x1b);
	hl = LO(hl, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x3a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_42767 + 0x18); // Func_42767.asm_4277f
}

// Sets OBJ_SUBSTATE to $10 while its Omodonmeka is around, else 0.
PORT_LOCAL(Func_4279c, Func_42935)
{
	owner_key(gb, 0x41, 0x01);
	gb->cpu.a = find_owner(gb) < 0 ? 0 : 0x10;
	if (!gb->cpu.a)
		gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjSubState, gb->cpu.a);
}

// .asm_4283c: becomes solid ground for a while.
static void omodon_land(GB *gb, uint16_t hl)
{
	gb->cpu.a = 0x1c;
	wr8(gb, hl++, 0x1c);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_SOLID);
	SET_HL(gb, hl);
	gb->cpu.a = 0x2f;
	wr8(gb, wCurObjState, 0x2f);
}

// ld hl, wCurObjYPos / puts the Omodon $18 pixels above Wario
static void above_wario(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), 0x18);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
}

// .Func_427a9
static void omodon_update(GB *gb)
{
	gb_call_ret(gb, OMODON_FUNC_42935, OMODON(0x10));
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (!alu_and(gb, s, s)) {
		JUMP_TO(ADDR_Func_42767);
		return;
	}
	alu_cp(gb, s, 0x2d);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_427cd
		gb_call_ret(gb, OMODON_FUNC_42894, OMODON(0x34));
		hl = wCurObjStateDuration;
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x33);
		if (gb->cpu.f & FLAG_Z) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			RET();
			return;
		}
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
			gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
			if (gb->cpu.f & FLAG_C) {
				RET();
				return;
			}
		}
		// .asm_427e5
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			JUMP(MoveObjectUp);
			return;
		}
		SET_DE(gb, ADDR_Frameset_68f50);
		CALL(SetObjectFramesetPtr);
		hl = LO(REG_HL(gb), OBJ_STATE);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl--, 0);
		hl = DEC_L(hl);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl--, 0);
		SET_HL(gb, hl);
		gb->cpu.a = 0x0b;
		wr8(gb, wCurObjCollBoxBottom, 0x0b);
		RET();
		return;
	}
	alu_cp(gb, s, 0x2e);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_427fe: falls, and shakes the ground when it lands.
		SET_BC(gb, ADDR_Data_605e0);
		CALL(ApplyObjYMovement);
		hl = wCurObjYPos;
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x0f);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
		wr8(gb, hYPosHi, gb->cpu.a);
		wr8(gb, hXPosLo, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_352b, OMODON(0x7e));
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		hl = wCurObjYPos;
		gb->cpu.a = alu_sub(gb, rd8(gb, hYPosLo), 0x0f);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hYPosHi), 0);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
		play_sfx(gb, SFX_063);
		gb->cpu.b = 0x18;
		CALL(DoGroundShake);
		SET_DE(gb, ADDR_Frameset_68f45);
		CALL(SetObjectFramesetPtr);
		omodon_land(gb, REG_HL(gb));
		RET();
		return;
	}
	alu_cp(gb, s, 0x2f);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_42754);
		return;
	}
	alu_cp(gb, s, 0x2a);
	if (gb->cpu.f & FLAG_Z) {
		// .Func_4284d
		SET_DE(gb, ADDR_Frameset_68f45);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		wr8(gb, hl++, 0x14);
		SET_HL(gb, hl);
		gb->cpu.a = 0x5a;
		wr8(gb, wCurObjState, 0x5a);
		above_wario(gb);
		RET();
		return;
	}
	alu_cp(gb, s, 0x5a);
	if (gb->cpu.f & FLAG_Z) {
		// .Func_4286b
		hl = wCurObjStateDuration;
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x14);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, wAutoMoveState);
			if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
				JUMP(MoveObjectDown_Fast);
				return;
			}
			above_wario(gb);
		}
		// .asm_42889
		hl = LO(REG_HL(gb), OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		gb->cpu.a = (1 << NO_ACTIONS_F) | 31; // no_actions_for 31
		wr8(gb, wCurObjAction, gb->cpu.a);
		omodon_land(gb, hl);
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	JUMP_TO(ADDR_Func_42767);
}

PORT_JUMP(Func_4279c)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(OMODON_FUNC_427A9 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)OMODON_FUNC_427A9;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	omodon_update(gb);
}

PORT_LOCAL_JUMP(Func_4279c, Func_427a9) { omodon_update(gb); }
