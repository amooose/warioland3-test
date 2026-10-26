// Port of src/engine/level/collisions.asm
#include "ported.h"

// Jumps and calls within this bank use the address rather than JUMP / CALL:
// like the asm, they run whatever bank is mapped.

// A call of ProcessBlock is done as the asm does it, pushing the return
// address and jumping, rather than with gb_call: a block function can be
// anything the level's block function table points at, and must see the
// same stack. The rest of the caller (ld b, a / ret) then runs as asm.
static void call_process_block(GB *gb, uint16_t ret)
{
	gb_push16(gb, ret);
	JUMP_TO(ADDR_ProcessBlock);
}

// ld hl, hPos / call GetBlockPtr / ld a, points / ld [wCollisionPoints], a
static void get_block_at_pos(GB *gb, uint8_t points)
{
	SET_HL(gb, hPos);
	CALL(GetBlockPtr);
	gb->cpu.a = points;
	wr8(gb, wCollisionPoints, points);
}

// The routines that process the block at hPos with a set of collision
// points: the call of ProcessBlock is $0b bytes in.
static void process_block_at_pos(GB *gb, uint16_t routine, uint8_t points)
{
	get_block_at_pos(gb, points);
	call_process_block(gb, (uint16_t)(routine + 0x0b + 3));
}

PORT_JUMP(Func_19b3a)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wWaterInteraction, 0);
	wr8(gb, wLadderInteraction, 0);
	wr8(gb, wRoomTransitionParam, 0);
	wr8(gb, wIsInSand, 0);
	wr8(gb, wIsNearFence, 0);
	wr8(gb, wIsInAirCurrent, 0);
	// call Func_19c0b / ret
	gb_push16(gb, (uint16_t)(ADDR_Func_19b3a + 0x13 + 3));
	JUMP_TO(ADDR_Func_19c0b);
}

PORT_JUMP(Func_19b51)
{
	get_block_at_pos(gb, COLLISION_DOWN);
	call_process_block(gb, (uint16_t)(ADDR_Func_19b5c + 3)); // fallthrough
}

PORT(DecrementBlockPtr)
{
	SET_HL(gb, wBlockPtr + 1);
	wr8(gb, wBlockPtr + 1, alu_dec(gb, rd8(gb, wBlockPtr + 1)));
}

PORT(IncrementBlockPtr)
{
	SET_HL(gb, wBlockPtr + 1);
	wr8(gb, wBlockPtr + 1, alu_inc(gb, rd8(gb, wBlockPtr + 1)));
}

// processes the block to the left, unless it's $ff
PORT_JUMP(_ProcessBlockOnLeft)
{
	gb_call(gb, ADDR_DecrementBlockPtr);
	gb->cpu.a = alu_inc(gb, rd8(gb, REG_HL(gb)));
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_19b5c);
}

// processes the block to the right, unless it's $00
PORT_JUMP(_ProcessBlockOnRight)
{
	gb_call(gb, ADDR_IncrementBlockPtr);
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_19b5c);
}

PORT_JUMP(Func_19b7b) { process_block_at_pos(gb, ADDR_Func_19b7b, COLLISION_UNK1); }
PORT_JUMP(Func_19b8b) { process_block_at_pos(gb, ADDR_Func_19b8b, COLLISION_UNK2); }
PORT_JUMP(Func_19b9b) { process_block_at_pos(gb, ADDR_Func_19b9b, COLLISION_UNK3); }

PORT_JUMP(Func_19bc3)
{
	get_block_at_pos(gb, COLLISION_UNK4);
	call_process_block(gb, (uint16_t)(ADDR_Func_19bce + 3)); // fallthrough
}

// processes the block one row up
PORT_JUMP(Func_19bd3)
{
	uint8_t hi = alu_dec(gb, rd8(gb, wBlockPtr));
	wr8(gb, wBlockPtr, hi);
	SET_HL(gb, wBlockPtr);
	gb->cpu.a = hi;
	alu_cp(gb, hi, 0xa0 - 1);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_dec(gb, rd8(gb, wBlockPtrBank));
		wr8(gb, wBlockPtrBank, gb->cpu.a);
		gb->cpu.a = 0xc0 - 1;
		wr8(gb, wBlockPtr, gb->cpu.a);
	}
	gb->cpu.a = rd8(gb, wBlockPtr);
	gb->cpu.l = rd8(gb, wBlockPtr + 1);
	gb->cpu.h = gb->cpu.a;
	JUMP_TO(ADDR_Func_19bce);
}

PORT_JUMP(Func_19beb) { process_block_at_pos(gb, ADDR_Func_19beb, COLLISION_UNK5); }
PORT_JUMP(Func_19bfb) { process_block_at_pos(gb, ADDR_Func_19bfb, COLLISION_UNK6); }
PORT_JUMP(Func_19c0b) { process_block_at_pos(gb, ADDR_Func_19c0b, COLLISION_UNK7); }

// Copies wWarioPos to hPos, with y - offset, as the asm does it: from
// wWarioXPos + 1 down.
static void get_pos_with_y_offset(GB *gb, uint8_t offset)
{
	wr8(gb, hXPosLo, rd8(gb, wWarioXPos + 1));
	wr8(gb, hXPosLo - 1, rd8(gb, wWarioXPos));
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), offset);
	wr8(gb, hXPosLo - 2, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos), 0x00);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioYPos);
	SET_DE(gb, hXPosLo - 3);
}

PORT_JUMP(Func_19acd) { get_pos_with_y_offset(gb, 0x0c); JUMP_TO(ADDR_Func_19b3a); }
PORT_JUMP(Func_19ae4) { get_pos_with_y_offset(gb, 0x1a); JUMP_TO(ADDR_Func_19b3a); }
PORT_JUMP(Func_19afb) { get_pos_with_y_offset(gb, 0x18); JUMP_TO(ADDR_Func_19b3a); }

// collision on centre
PORT_JUMP(Func_19b12)
{
	// get_pos
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wWarioPos + i);
		wr8(gb, hPos + i, gb->cpu.a);
	}
	SET_HL(gb, wWarioPos + 3);
	SET_DE(gb, hPos + 3);
	JUMP_TO(ADDR_Func_19b3a);
}

// collision on top
PORT_JUMP(Func_19b25) { get_pos_with_y_offset(gb, 4); JUMP_TO(ADDR_Func_19b3a); } // fallthrough

// Copies wWarioPos to hPos with x moved to the left or right edge of the
// collision box, 3 pixels in, as the asm does it: from wWarioXPos + 1 down.
static void get_pos_at_box_edge(GB *gb, bool left)
{
	if (left) {
		gb->cpu.a = (uint8_t)(~rd8(gb, wCollisionBoxLeft) + 1);
		gb->cpu.c = alu_sub(gb, gb->cpu.a, 3);
		gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos + 1), gb->cpu.c);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos), 0x00);
	} else {
		gb->cpu.c = alu_sub(gb, rd8(gb, wCollisionBoxRight), 3);
		gb->cpu.a = alu_add(gb, rd8(gb, wWarioXPos + 1), gb->cpu.c);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, wWarioXPos), 0x00);
	}
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	wr8(gb, hXPosLo - 2, rd8(gb, wWarioYPos + 1));
	gb->cpu.a = rd8(gb, wWarioYPos);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioYPos);
	SET_DE(gb, hXPosLo - 3);
}

// get_pos
static void get_pos(GB *gb)
{
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wWarioPos + i);
		wr8(gb, hPos + i, gb->cpu.a);
	}
	SET_HL(gb, wWarioPos + 3);
	SET_DE(gb, hPos + 3);
}

// resets wWarioSlopeInteraction
PORT(Func_198c7)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wWarioSlopeInteraction, 0);
	get_pos(gb);
	gb_call_ret(gb, ADDR_Func_19b51, (uint16_t)(ADDR_Func_198c7 + 0x18));
}

// ld a, [wIsSmashAttacking] / and a / jr z / ld a, TRUE / ld [wBlockFuncBreakFlag], a
static void break_if_smash_attacking(GB *gb)
{
	gb->cpu.a = rd8(gb, wIsSmashAttacking);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncBreakFlag, TRUE);
}

// The calls in CheckAirborneCollision, by where they return to
#define AIRBORNE_RET_FUNC_19A53 (ADDR_CheckAirborneCollision + 0x03)
#define AIRBORNE_RET_LEFT       (ADDR_CheckAirborneCollision + 0x2f)
#define AIRBORNE_RET_RIGHT      (ADDR_CheckAirborneCollision + 0x61)
#define AIRBORNE_RET_CENTRE     (ADDR_CheckAirborneCollision + 0x8d)

PORT(CheckAirborneCollision)
{
	// call .Func_19a53, with its return address on the stack as in the asm
	gb_push16(gb, (uint16_t)AIRBORNE_RET_FUNC_19A53);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wWarioSlopeInteraction, 0);
	break_if_smash_attacking(gb);
	get_pos(gb);
	gb_call_ret(gb, ADDR_Func_19b7b, (uint16_t)AIRBORNE_RET_CENTRE);
	gb_pop16(gb);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;

	break_if_smash_attacking(gb);
	get_pos_at_box_edge(gb, true);
	gb_call_ret(gb, ADDR_Func_19bfb, (uint16_t)AIRBORNE_RET_LEFT);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wIsOnSteppableObject);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.b = gb->cpu.a;
		return;
	}

	break_if_smash_attacking(gb);
	get_pos_at_box_edge(gb, false);
	gb_call_ret(gb, ADDR_Func_19bfb, (uint16_t)AIRBORNE_RET_RIGHT);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wIsOnSteppableObject);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.b = gb->cpu.a;
}

PORT(Func_19b5c)
{
	gb_call_ret(gb, ADDR_ProcessBlock, (uint16_t)(ADDR_Func_19b5c + 3));
	gb->cpu.b = gb->cpu.a;
}

PORT(CheckCentreCollision)
{
	// collision in centre
	gb->cpu.a = 0x01;
	wr8(gb, wUnused_ced3, 0x01);
	gb_call_ret(gb, ADDR_Func_198c7, (uint16_t)(ADDR_CheckCentreCollision + 0x08));
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wUnused_ced3, 0);

	// collision on left
	get_pos_at_box_edge(gb, true);
	gb_call_ret(gb, ADDR_Func_19b8b, (uint16_t)(ADDR_CheckCentreCollision + 0x2d));
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;

	// collision on right
	get_pos_at_box_edge(gb, false);
	gb_call_ret(gb, ADDR_Func_19b8b, (uint16_t)(ADDR_CheckCentreCollision + 0x4c));
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;

	gb->cpu.a = rd8(gb, wIsOnSteppableObject);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.b = gb->cpu.a;
	// get_pos_y
	gb->cpu.a = rd8(gb, wWarioYPos);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, wWarioYPos + 1);
	wr8(gb, hYPosHi + 1, gb->cpu.a);
	SET_HL(gb, wWarioYPos + 1);
	SET_DE(gb, hYPosHi + 1);
	gb->cpu.a = 0x01;
}

PORT(Func_19bce)
{
	gb_call_ret(gb, ADDR_ProcessBlock, (uint16_t)(ADDR_Func_19bce + 3));
	gb->cpu.b = gb->cpu.a;
}
