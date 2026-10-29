// Port of src/engine/level/objects/shoot.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Sets up a goal counter, with update as its update function.
static void goal_counter(GB *gb, uint16_t update)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
	wr8(gb, wCurObjFrame, 0x3c);
	// move up $58 pixels
	gb->cpu.a = 0x58;
	wr8(gb, wCurObjVar2, 0x58);
	JUMP(MoveObjectUpByVar2);
}

PORT_JUMP(ShootGoalCounterFunc) { goal_counter(gb, ADDR_ShootGoalCounterFunc + 0x20); }
PORT_JUMP(WarioGoalCounterFunc) { goal_counter(gb, ADDR_WarioGoalCounterFunc + 0x20); }

// a = [bc + var3], the next entry of a $80-terminated table of y offsets
// (stays on the last one); b = a
static void kick_y_offset(GB *gb)
{
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	gb->cpu.b = gb->cpu.a;
}

// moves Wario up by b: xor a / sub b / ld b, a / call MoveWarioUp /
// jp HandleUpwardsFloorTransition
static void kick_wario_up(GB *gb, uint16_t ret)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.a = alu_sub(gb, 0, gb->cpu.b);
	gb->cpu.b = gb->cpu.a;
	gb_call_ret(gb, ADDR_MoveWarioUp, ret);
	JUMP(HandleUpwardsFloorTransition);
}

// bc = the y offsets: moves the object and Wario along them
PORT_JUMP(DoPreKickingJumpYMovement)
{
	kick_y_offset(gb);
	gb->cpu.l = OBJ_Y_POS;
	uint16_t hl = REG_HL(gb);
	alu_cp(gb, gb->cpu.a, (uint8_t)-128);
	gb->cpu.c = rd8(gb, hl);
	if (!(gb->cpu.f & FLAG_C)) {
		// .moving_up
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (!(gb->cpu.f & FLAG_C))
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		kick_wario_up(gb, ADDR_DoPreKickingJumpYMovement + 0x2e);
		return;
	}
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	gb_call_ret(gb, ADDR_MoveWarioDown, ADDR_DoPreKickingJumpYMovement + 0x20);
	JUMP(HandleDownwardsFloorTransition);
}

PORT_JUMP(ApplyYOffsetOnKickedWarioBall)
{
	kick_y_offset(gb);
	gb_call_ret(gb, ADDR_MoveWarioDown, ADDR_ApplyYOffsetOnKickedWarioBall + 0x14);
	JUMP(HandleDownwardsFloorTransition);
}

PORT_JUMP(Func_5725b) { kick_wario_up(gb, ADDR_Func_5725b + 0x06); }

// --- GK Tortoise platform ---

#define GKTP_MOVE_LEFT (ADDR_GKTortoisePlatformFunc + 0x35)
#define GKTP_MOVE_UP_AND_DOWN (ADDR_GKTortoisePlatformFunc + 0x58)

static inline void res_bit(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << bit)));
}

// ld l, OBJ_UPDATE_FUNCTION + 1 / store f, big-endian downwards
static uint16_t set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

PORT_JUMP(GKTortoisePlatformFunc)
{
	uint16_t hl = wCurObjFlags;
	res_bit(gb, hl, OBJFLAG_INVISIBLE_F);
	hl = set_update(gb, hl, GKTP_MOVE_LEFT);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~(1 << OBJSUBFLAG_UNINITIALISED_F)) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	gb->cpu.a = 5;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)-6;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69fdc);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 32;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = 60;
	wr8(gb, hl++, gb->cpu.a); // OBJ_VAR_1
	// move 64 pixels right and 16 pixels down
	gb->cpu.a = 64;
	wr8(gb, hl, gb->cpu.a); // OBJ_VAR_2
	SET_HL(gb, hl);
	CALL(MoveObjectRightByVar2);
	gb->cpu.a = 16;
	wr8(gb, wCurObjVar2, gb->cpu.a);
	JUMP(MoveObjectDownByVar2);
}

PORT_LOCAL_JUMP(GKTortoisePlatformFunc, MoveLeft)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
		wr8(gb, wCurObjAction, gb->cpu.a);
		JUMP(MoveObjectLeft);
		return;
	}
	hl = LO(hl, alu_inc(gb, (uint8_t)hl));
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl))); // OBJ_VAR_1
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	hl = LO(hl, alu_dec(gb, (uint8_t)hl));
	gb->cpu.a = 176;
	wr8(gb, hl, gb->cpu.a); // OBJ_STATE_DURATION
	set_update(gb, hl, GKTP_MOVE_UP_AND_DOWN);
	RET();
}

PORT_LOCAL_JUMP(GKTortoisePlatformFunc, MoveUpAndDown)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjVar1;
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl))); // OBJ_STATE_DURATION
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, 176);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0); // OBJ_VAR_1
			wr8(gb, wCurObjState, 0);
			RET();
			return;
		}
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
			res_bit(gb, hl, OBJFLAG_STEPPED_F);
			gb->cpu.b = 1;
			CALL(MoveWarioDown);
			CALL(HandleDownwardsFloorTransition);
		}
		JUMP(MoveObjectDown);
		return;
	}
	// moving up
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl))); // OBJ_STATE_DURATION
	if (gb->cpu.f & FLAG_Z) {
		hl = LO(hl, alu_inc(gb, (uint8_t)hl));
		SET_HL(gb, hl);
		gb->cpu.a = 0x01;
		wr8(gb, hl, gb->cpu.a); // OBJ_VAR_1
		RET();
		return;
	}
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		res_bit(gb, hl, OBJFLAG_STEPPED_F);
		gb->cpu.b = 1;
		CALL(MoveWarioUp);
		CALL(HandleUpwardsFloorTransition);
	}
	JUMP(MoveObjectUp);
}
