// Port of src/home/object_mechanics.asm
#include "ported.h"

// Object positions are 16-bit little-endian: the low byte at the label, the
// high byte after it.

// ld a, [hl] / add or sub n / ld [hli], a / ret nc / inc or dec [hl]
static void move_pos_by(GB *gb, uint16_t pos, uint8_t n, bool subtract)
{
	uint8_t lo = rd8(gb, pos);
	gb->cpu.a = subtract ? alu_sub(gb, lo, n) : alu_add(gb, lo, n);
	wr8(gb, pos, gb->cpu.a);
	SET_HL(gb, pos + 1);
	if (!(gb->cpu.f & FLAG_C))
		return;
	uint8_t hi = rd8(gb, pos + 1);
	wr8(gb, pos + 1, subtract ? alu_dec(gb, hi) : alu_inc(gb, hi));
}

// ld hl, pos / inc [hl] / ret nz / inc l / inc [hl]
static void inc_pos(GB *gb, uint16_t pos)
{
	SET_HL(gb, pos);
	uint8_t lo = alu_inc(gb, rd8(gb, pos));
	wr8(gb, pos, lo);
	if (lo)
		return;
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

// ld a, [wGlobalCounter] / rra: the slow movers only move every other frame.
// Returns the carry (bit 0 of the counter).
static bool rra_global_counter(GB *gb)
{
	uint8_t v = rd8(gb, wGlobalCounter);
	bool carry = v & 1;
	gb->cpu.a = (uint8_t)((v >> 1) | ((gb->cpu.f & FLAG_C) ? 0x80 : 0));
	gb->cpu.f = carry ? FLAG_C : 0;
	return carry;
}

PORT(MoveObjectLeftByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjXPos, gb->cpu.b, true);
}

PORT(MoveObjectRightByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjXPos, gb->cpu.b, false);
}

PORT(MoveObjectUpByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjYPos, gb->cpu.b, true);
}

PORT(MoveObjectDownByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjYPos, gb->cpu.b, false);
}

PORT(MoveObjectLeft_Fast)
{
	move_pos_by(gb, wCurObjXPos, 2, true);
}

PORT(MoveObjectRight_Fast)
{
	move_pos_by(gb, wCurObjXPos, 2, false);
}

PORT(MoveObjectDown_Fast)
{
	move_pos_by(gb, wCurObjYPos, 2, false);
}

PORT(MoveObjectUp_Fast)
{
	move_pos_by(gb, wCurObjYPos, 2, true);
}

// moves current object right
// by 0.5 pixel per frame
PORT(MoveObjectRight_Slow)
{
	if (!rra_global_counter(gb))
		inc_pos(gb, wCurObjXPos);
}

// moves current object right
// at 1 pixel per frame
PORT(MoveObjectRight)
{
	inc_pos(gb, wCurObjXPos);
}

// moves current object left
// by 0.5 pixel per frame
PORT(MoveObjectLeft_Slow)
{
	if (rra_global_counter(gb))
		move_pos_by(gb, wCurObjXPos, 1, true);
}

// moves current object left
// at 1 pixel per frame
PORT(MoveObjectLeft)
{
	move_pos_by(gb, wCurObjXPos, 1, true);
}

// moves current object down
// at 0.5 pixel per frame
PORT(MoveObjectDown_Slow)
{
	if (!rra_global_counter(gb))
		inc_pos(gb, wCurObjYPos);
}

// moves current object down
// at 1 pixel per frame
PORT(MoveObjectDown)
{
	inc_pos(gb, wCurObjYPos);
}

PORT(MoveObjectUp_Slow)
{
	if (rra_global_counter(gb))
		move_pos_by(gb, wCurObjYPos, 1, true);
}

// moves current object up
// at 1 pixel per frame
PORT(MoveObjectUp)
{
	move_pos_by(gb, wCurObjYPos, 1, true);
}

// input:
// - de = frameset pointer
PORT(SetObjectFramesetPtr)
{
	wr8(gb, wCurObjFramesetPtr + 0, gb->cpu.e);
	wr8(gb, wCurObjFramesetPtr + 1, gb->cpu.d);
	wr8(gb, wCurObjFramesetPtr + 2, 0); // OBJ_FRAME_DURATION
	wr8(gb, wCurObjFramesetPtr + 3, 0); // OBJ_FRAMESET_OFFSET
	SET_HL(gb, wCurObjFramesetPtr + 4);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// The tail of routines entered with homejp, which pushed the caller's bank.
PORT(HomeJumpRet)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}
