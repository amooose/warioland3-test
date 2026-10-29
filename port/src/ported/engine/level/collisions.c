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

PORT_JUMP(Func_19b5c)
{
	uint16_t ret = (uint16_t)(ADDR_Func_19b5c + 3);
	gb_call_ret(gb, ADDR_ProcessBlock, ret);
	// A block function can switch the ROM bank; the asm then returns into
	// whatever bank is mapped, so carry on there.
	if (gb->rom_bank != BANK_Func_19b5c) {
		JUMP_TO(ret);
		return;
	}
	gb->cpu.b = gb->cpu.a;
	RET();
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

PORT(Func_19823)
{
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncBreakFlag, TRUE);
	gb_call_ret(gb, ADDR_Func_19bd3, ADDR_Func_19823 + 0x08);
	gb->cpu.a = 0x01;
	wr8(gb, wUnused_c18d, 0x01);
	gb->cpu.b = 0x01;
}

// --- more collision checks ---

#define ZF (gb->cpu.f & FLAG_Z)

// ld a, [addr] / and a; true if nz
static bool co_ld_and(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	return alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
}

// ld a, [addr] / cpl / inc a
static uint8_t neg(GB *gb, uint16_t addr)
{
	return alu_inc(gb, alu_cpl(gb, rd8(gb, addr)));
}

// hPos = Wario's position with x += cx (or -= cx) and y -= cy(), as the asm
// does it: from wWarioXPos + 1 down, with cy computed between the halves
static void hpos_from_wario(GB *gb, bool add_x, uint8_t cx, uint8_t (*cy)(GB *))
{
	gb->cpu.c = cx;
	if (add_x) {
		gb->cpu.a = alu_add(gb, rd8(gb, wWarioXPos + 1), gb->cpu.c);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, wWarioXPos), 0x00);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos + 1), gb->cpu.c);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos), 0x00);
	}
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	gb->cpu.c = cy(gb);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), gb->cpu.c);
	wr8(gb, hXPosLo - 2, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos), 0x00);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioYPos);
	SET_DE(gb, hXPosLo - 3);
}

static uint8_t cy_bottom_7(GB *gb) { return alu_add(gb, neg(gb, wCollisionBoxBottom), 7); }
static uint8_t cy_bottom_1(GB *gb) { return alu_add(gb, neg(gb, wCollisionBoxBottom), 1); }
static uint8_t cy_top_2(GB *gb) { return alu_add(gb, neg(gb, wCollisionBoxTop), 2); }

// CheckBottomRightCollision / CheckBottomLeftCollision
static void check_bottom_collision(GB *gb, bool right)
{
	uint16_t routine = right ? ADDR_CheckBottomRightCollision : ADDR_CheckBottomLeftCollision;
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FLAT_WARIO);
	bool flat = ZF;
	uint8_t cx = right ? alu_sub(gb, rd8(gb, wCollisionBoxRight), 1)
		: alu_sub(gb, neg(gb, wCollisionBoxLeft), 1);
	hpos_from_wario(gb, right, cx, flat ? cy_bottom_1 : cy_bottom_7);
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	wr8(gb, wc0dc, gb->cpu.a);
	uint16_t ret = (uint16_t)(routine + (right ? 0x5d : 0x61));
	gb_call_ret(gb, ADDR_Func_19bc3, ret);
	// A block function can switch the ROM bank (a fuzzed block function
	// table can send ProcessBlock anywhere). The asm then returns into
	// whatever bank is mapped, so carry on there.
	if (gb->rom_bank != BANK_CheckBottomRightCollision) {
		JUMP_TO(ret);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (ZF) {
		JUMP_TO(ADDR_Func_19832);
		return;
	}
	gb->cpu.a = rd8(gb, wAttackCounter);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(ADDR_Func_19823);
		return;
	}
	gb->cpu.a = gb->cpu.b;
	RET();
}

PORT_JUMP(CheckBottomRightCollision) { check_bottom_collision(gb, true); }
PORT_JUMP(CheckBottomLeftCollision) { check_bottom_collision(gb, false); }

PORT_JUMP(Func_19741)
{
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncBreakFlag, TRUE);
	check_bottom_collision(gb, true);
}

PORT_JUMP(Func_197b1)
{
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncBreakFlag, TRUE);
	check_bottom_collision(gb, false);
}

// checks collision in side that Wario is facing
PORT_JUMP(CheckFrontCollision)
{
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncBreakFlag, TRUE);
	gb->cpu.a = rd8(gb, wDirection);
	check_bottom_collision(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) != 0);
}

// ld a, TRUE / ld [wBlockFuncWarioFlag], a / call routine / xor a /
// ld [wBlockFuncWarioFlag], a
static void as_wario(GB *gb, uint16_t routine, uint16_t ret)
{
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncWarioFlag, TRUE);
	gb_call_ret(gb, routine, ret);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBlockFuncWarioFlag, FALSE);
}

PORT(Func_19706) { as_wario(gb, ADDR_Func_19741, ADDR_Func_19706 + 0x08); }
PORT(Func_19713) { as_wario(gb, ADDR_Func_197b1, ADDR_Func_19713 + 0x08); }

PORT_JUMP(Func_19720)
{
	as_wario(gb, ADDR_CheckAirborneCollision, ADDR_Func_19720 + 0x08);
	gb->cpu.a = gb->cpu.b;
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		JUMP(TriggerFloorTransition);
	else
		JUMP(TriggerDownwardsFloorTransition);
}

PORT(Func_19832)
{
	bool skip = co_ld_and(gb, wIsCrouching);
	if (!skip) {
		gb->cpu.b = rd8(gb, wCollisionBoxTop);
		gb->cpu.a = alu_add(gb, rd8(gb, wWarioYPos + 1), gb->cpu.b);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 2);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xf0);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = rd8(gb, wc0dc);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		skip = ZF;
	}
	if (!skip) {
		gb->cpu.a = TRUE;
		wr8(gb, wBlockFuncBreakFlag, TRUE);
		gb_call_ret(gb, ADDR_Func_19bd3, ADDR_Func_19832 + 0x23);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
		if (ZF)
			return;
		static const uint16_t checks[] = {wJumpVelTable, wAttackCounter, wWaterInteraction,
			wIsCrouching, wIsOnSteppableObject, wTransformation, wIsInSand, 0};
		for (const uint16_t *p = checks; *p; p++)
			if (co_ld_and(gb, *p))
				return;
		gb->cpu.a = rd8(gb, wJoypadDown);
		bool right;
		if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
			right = true;
		else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
			right = false;
		else
			right = co_ld_and(gb, wDirection);
		gb->cpu.b = 2;
		if (right) {
			CALL(MoveWarioRight);
			FARCALL_RET(CrouchOrSlide, ADDR_Func_19832 + 0x8c);
		} else {
			CALL(MoveWarioLeft);
			FARCALL_RET(CrouchOrSlide, ADDR_Func_19832 + 0x76);
		}
	}
	// .asm_198c1
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wUnused_c18d, 0);
	gb->cpu.b = 0;
}

static void break_unless_puffy(GB *gb)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
	if (ZF)
		return;
	// can destroy blocks above
	gb->cpu.a = TRUE;
	wr8(gb, wBlockFuncBreakFlag, TRUE);
}

PORT(Func_19942)
{
	break_unless_puffy(gb);
	// 2 pixels above collision box
	gb->cpu.c = cy_top_2(gb);
	gb->cpu.a = rd8(gb, wWarioXPos + 1);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, wWarioXPos);
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), gb->cpu.c);
	wr8(gb, hXPosLo - 2, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos), 0x00);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioYPos);
	SET_DE(gb, hXPosLo - 3);
	gb_call_ret(gb, ADDR_Func_19beb, ADDR_Func_19942 + 0x2b);
}

PORT(CheckUpCollision)
{
	gb->cpu.a = TRUE;
	wr8(gb, wJumpCollision, TRUE);
	gb_call_ret(gb, ADDR_Func_19942, ADDR_CheckUpCollision + 0x08);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wJumpCollision, 0);
	break_unless_puffy(gb);
	// left collision boundary - 3 pixels
	hpos_from_wario(gb, false, alu_sub(gb, neg(gb, wCollisionBoxLeft), 3), cy_top_2);
	gb_call_ret(gb, ADDR_Func_19b9b, ADDR_CheckUpCollision + 0x44);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	break_unless_puffy(gb);
	hpos_from_wario(gb, true, alu_sub(gb, rd8(gb, wCollisionBoxRight), 3), cy_top_2);
	gb_call_ret(gb, ADDR_Func_19b9b, ADDR_CheckUpCollision + 0x7a);
}

PORT(Func_19a77)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wc1ca, FALSE);
	get_pos_at_box_edge(gb, true);
	gb_call_ret(gb, ADDR_Func_19b3a, ADDR_Func_19a77 + 0x23);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (!co_ld_and(gb, wc1ca))
			return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wc1ca, FALSE);
	get_pos_at_box_edge(gb, false);
	gb_call_ret(gb, ADDR_Func_19b3a, ADDR_Func_19a77 + 0x4c);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = TRUE;
	wr8(gb, wc1ca, TRUE);
}

// processes the block before (or after) wBlockPtr
static void process_adjacent_block(GB *gb, bool next, uint16_t ret)
{
	uint16_t hl = wBlockPtr + 1;
	wr8(gb, hl, next ? alu_inc(gb, rd8(gb, hl)) : alu_dec(gb, rd8(gb, hl)));
	gb->cpu.a = rd8(gb, hl--);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	gb_call_ret(gb, ADDR_ProcessBlock, ret);
	gb->cpu.b = gb->cpu.a;
}

PORT(Func_19bab) { process_adjacent_block(gb, false, ADDR_Func_19bab + 0x0a); }
PORT(Func_19bb7) { process_adjacent_block(gb, true, ADDR_Func_19bb7 + 0x0a); }
