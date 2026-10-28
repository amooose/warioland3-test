// Port of src/engine/level/objects/futamogu.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)

#define FUTAMOGU_UPDATE (ADDR_FutamoguFunc + 0x7f) // .Update

PORT(FutamoguFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(FUTAMOGU_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FUTAMOGU_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68319);
	CALL(SetObjectFramesetPtr);
	// save starting position
	hl = INC_L(REG_HL(gb));
	wr8(gb, hl++, rd8(gb, wCurObjYPos + 0)); // OBJ_VAR_1
	wr8(gb, hl++, rd8(gb, wCurObjYPos + 1)); // OBJ_VAR_2
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 5);
	wr8(gb, hl--, (uint8_t)-21); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 4;
	wr8(gb, hl, 4); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}

// .set_idle
static void set_idle(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_68319);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = LO(REG_HL(gb), OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_NO_COLLISION));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	RET();
}

// .set_shake (hl = wCurObjState)
static void set_shake(GB *gb, uint16_t hl)
{
	gb->cpu.a = OBJSTATE_ATTACKED_LEFT;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68348);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 32;
	wr8(gb, hl++, 32); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
	RET();
}

// Compares the position with the starting one in OBJ_VAR_1 / OBJ_VAR_2;
// returns whether it's back at the start.
static bool at_start(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_VAR_1);
	SET_HL(gb, hl);
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = rd8(gb, wCurObjYPos + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z))
		return false;
	hl = INC_L(hl);
	SET_HL(gb, hl);
	gb->cpu.b = rd8(gb, hl);
	gb->cpu.a = rd8(gb, wCurObjYPos + 1);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return gb->cpu.f & FLAG_Z;
}

// .vanish
static void vanish(GB *gb)
{
	gb->cpu.a = 0x08;
	wr8(gb, wCurObjVar2, 0x08);
	CALL(MoveObjectLeftByVar2);
	JUMP(VanishObject);
}

// .Climb: rises back to its starting position, lifting Wario if he's on it.
static void climb(GB *gb, uint16_t hl)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		set_shake(gb, hl);
		return;
	}
	if (at_start(gb, hl)) {
		set_idle(gb);
		return;
	}
	// .check_stepped_on
	hl = LO(REG_HL(gb), OBJ_FLAGS);
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
		// move upwards at 0.5 pixels
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (!(gb->cpu.f & FLAG_C)) {
			RET();
			return;
		}
		gb->cpu.b = 1;
		CALL(MoveWarioUp);
		CALL(HandleUpwardsFloorTransition);
		JUMP(MoveObjectUp);
		return;
	}
	// .not_stepped_on: double speed animation
	hl = LO(hl, OBJ_FRAME_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	JUMP(MoveObjectUp);
}

// .Fall: knocked down by a ground pound.
static void fall(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		set_idle(gb);
		return;
	}
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	if (gb->cpu.f & FLAG_C) {
		RET(); // already as low as possible
		return;
	}
	gb->cpu.a = rd8(gb, wEffectivePowerUp);
	alu_cp(gb, gb->cpu.a, SUPER_JUMP_SLAM_OVERALLS);
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_Data_603f0 : ADDR_Data_60490);
	CALL(ApplyObjYMovement);
	hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, ADDR_FutamoguFunc + 0x13f);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wEffectivePowerUp);
	alu_cp(gb, gb->cpu.a, SUPER_JUMP_SLAM_OVERALLS);
	if (!(gb->cpu.f & FLAG_C)) {
		vanish(gb);
		return;
	}
	hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR)); // can't go lower
	RET();
}

PORT_LOCAL_JUMP(FutamoguFunc, Update)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (!alu_and(gb, s, s)) {
		// .Idle
		gb->cpu.a = rd8(gb, wGroundShakeCounter);
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!(gb->cpu.f & FLAG_C)) {
			set_shake(gb, hl);
			return;
		}
		// .no_shake
		if (!at_start(gb, hl)) {
			// .asm_4078c
			hl = LO(REG_HL(gb), OBJ_STATE);
			gb->cpu.a = OBJSTATE_SPECIAL_3;
			wr8(gb, hl--, gb->cpu.a);
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR)); // OBJ_SUBSTATE
			SET_HL(gb, hl);
			SET_DE(gb, ADDR_Frameset_68336);
			CALL(SetObjectFramesetPtr);
			RET();
			return;
		}
		// .asm_4079a
		hl = LO(REG_HL(gb), OBJ_FLAGS);
		SET_HL(gb, hl);
		if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
			RET();
			return;
		}
		gb->cpu.a = OBJSTATE_SPECIAL_2;
		wr8(gb, wCurObjState, gb->cpu.a);
		SET_DE(gb, ADDR_Frameset_68343);
		CALL(SetObjectFramesetPtr);
		RET();
		return;
	}
	alu_cp(gb, s, OBJSTATE_SPECIAL_3);
	if (gb->cpu.f & FLAG_Z) {
		climb(gb, hl);
		return;
	}
	alu_cp(gb, s, OBJSTATE_SPECIAL_2);
	if (gb->cpu.f & FLAG_Z) {
		// .Angry
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
			set_idle(gb);
			return;
		}
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
		RET();
		return;
	}
	alu_cp(gb, s, OBJSTATE_ATTACKED_LEFT);
	if (gb->cpu.f & FLAG_Z) {
		// .Shake
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		set_idle(gb);
		return;
	}
	alu_cp(gb, s, OBJSTATE_VANISH_TOUCH);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_407bf
		hl = DEC_L(hl);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl++)); // OBJ_SUBSTATE
		SET_HL(gb, hl);
		if (gb->cpu.f & FLAG_C) {
			vanish(gb);
			return;
		}
		gb->cpu.a = OBJSTATE_3A;
		wr8(gb, hl--, gb->cpu.a); // OBJ_STATE
		hl = DEC_L(hl);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0); // OBJ_VAR_3
		SET_HL(gb, hl);
		SET_DE(gb, ADDR_Frameset_68348);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		gb->cpu.a = 48;
		wr8(gb, hl++, 48); // OBJ_STATE_DURATION
		SET_HL(gb, hl);
		RET();
		return;
	}
	alu_cp(gb, s, OBJSTATE_3A);
	if (gb->cpu.f & FLAG_Z) {
		fall(gb, hl);
		return;
	}
	gb->cpu.a = alu_and(gb, s, 0xfe);
	alu_cp(gb, gb->cpu.a, OBJSTATE_ATTACKED_LEFT_START);
	if (gb->cpu.f & FLAG_Z) {
		// .Attacked
		gb->cpu.a = OBJSTATE_ATTACKED_LEFT;
		wr8(gb, hl, gb->cpu.a);
		SET_DE(gb, ADDR_Frameset_68351);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		gb->cpu.a = 16;
		wr8(gb, hl++, 16); // OBJ_STATE_DURATION
		SET_HL(gb, hl);
		RET();
		return;
	}
	set_idle(gb);
}
