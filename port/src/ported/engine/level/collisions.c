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
