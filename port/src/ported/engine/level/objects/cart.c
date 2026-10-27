// Port of src/engine/level/objects/cart.asm
#include "ported.h"

PORT_JUMP(CartStoppedFunc)
{
	JUMP(SetOneWayPlatformAction);
}

#define CART_WAITING (ADDR_CartFunc + 0x25) // .Waiting
#define CART_MOVING (ADDR_CartFunc + 0x3f) // .Moving
#define CARTV_WAITING (ADDR_CartVariableFunc + 0x3d) // .Waiting
#define CARTV_MOVING (ADDR_CartVariableFunc + 0x63) // .Moving

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

static void set_bits(GB *gb, uint16_t hl, uint8_t set, uint8_t res)
{
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~res) | set));
}

static void store_update(GB *gb, uint16_t hl, uint16_t update)
{
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// the rest of the init: the substate and the collision box
static void init_rest(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x0e);
	wr8(gb, hl--, 0xf1);
	gb->cpu.a = 0xf4;
	wr8(gb, hl, 0xf4);
	SET_HL(gb, hl);
}

PORT(CartFunc)
{
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, OBJFLAG_NO_COLLISION, OBJFLAG_INVISIBLE);
	store_update(gb, (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1)), CART_WAITING);
	SET_DE(gb, ADDR_Frameset_69199);
	CALL(SetObjectFramesetPtr);
	init_rest(gb);
}

// Waits for Wario to step on it. Returns false if he hasn't.
static bool waiting(GB *gb, uint16_t moving)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F))
		return false;
	set_bits(gb, hl, 0, OBJFLAG_STEPPED);
	store_update(gb, (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1)), moving);
	return true;
}

PORT_LOCAL(CartFunc, Waiting)
{
	if (!waiting(gb, CART_MOVING))
		return;
	SET_DE(gb, ADDR_Frameset_6919e);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(CartVariableFunc, Waiting)
{
	if (!waiting(gb, CARTV_MOVING)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, CART_VARIABLE_RIGHT);
	SET_DE(gb, (gb->cpu.f & FLAG_Z) ? ADDR_Frameset_691aa : ADDR_Frameset_691b3);
	JUMP(SetObjectFramesetPtr);
}

// Moves every other frame, carrying Wario, until it hits a wall.
// routine + ret_left / ret_right: where the asm's calls of Func_3513 return
static void moving(GB *gb, uint8_t right_id, uint16_t ret_left, uint16_t ret_right, uint16_t stopped)
{
	CALL(SetOneWayPlatformAction);
	gb->cpu.a = rd8(gb, wGlobalCounter);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x3f);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_06D);
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, right_id);
	bool right = gb->cpu.f & FLAG_Z;
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x04);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	if (right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x0e);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0x00);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x0f);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, right ? ret_right : ret_left);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// stops
		SET_DE(gb, stopped);
		CALL(SetObjectFramesetPtr);
		store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), ADDR_CartStoppedFunc);
		RET();
		return;
	}
	if (right)
		CALL(MoveObjectRight);
	else
		CALL(MoveObjectLeft);
	hl = lo_hl(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		RET();
		return;
	}
	set_bits(gb, hl, 0, OBJFLAG_STEPPED);
	if (right)
		FARCALL(Func_19706);
	else
		FARCALL(Func_19713);
	gb->cpu.a = gb->cpu.b;
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	if (right)
		JUMP(MoveWarioRight);
	else
		JUMP(MoveWarioLeft);
}

PORT_LOCAL_JUMP(CartFunc, Moving)
{
	moving(gb, CART_RIGHT, ADDR_CartFunc + 0x76, ADDR_CartFunc + 0xb4, ADDR_Frameset_69199);
}

PORT_LOCAL_JUMP(CartVariableFunc, Moving)
{
	moving(gb, CART_VARIABLE_RIGHT, ADDR_CartVariableFunc + 0x9a, ADDR_CartVariableFunc + 0xd8,
		ADDR_Frameset_691bf);
}

// The cart is only there once the truck wheel is collected.
PORT_JUMP(CartVariableFunc)
{
	// push_wram $02
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x02;
	wr8(gb, rWBK, 0x02);
	gb->cpu.a = TRUCK_WHEEL;
	CALL(IsTreasureCollected);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_DE(gb, ADDR_Frameset_691bf);
		SET_BC(gb, CARTV_WAITING);
	} else {
		SET_DE(gb, ADDR_Frameset_691a7);
		SET_BC(gb, ADDR_CartStoppedFunc);
	}
	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, OBJFLAG_NO_COLLISION, OBJFLAG_INVISIBLE);
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	init_rest(gb);
	JUMP(SetObjectFramesetPtr);
}
