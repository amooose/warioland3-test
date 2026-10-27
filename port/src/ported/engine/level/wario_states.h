// Helpers shared by the Wario state ports (wario_states_*.c): each one is
// a short asm idiom, with the registers and flags it leaves.
#ifndef WARIO_STATES_H
#define WARIO_STATES_H
#include "ported.h"

// Note: after a farcall, a = the caller's ROM bank and f = the flags from
// just before the farcall (hCallFunc pushes and pops af), so the flags have
// to be tracked exactly even on paths that end in a farcall.

// Jumps and calls within this bank (the Wario states are in banks 6, 7, $a and $b). They do the same as JUMP/CALL
// without the bank check: fuzzed inputs can make a callee switch banks
// through a stray write to the MBC, and then the asm jumps into the wrong
// bank just the same, which the check would abort on.
#define LJUMP(name) JUMP_TO(ADDR_##name)
#define LCALL(name) gb_call(gb, ADDR_##name)

#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)

#define OAM1 BANK_OAM_14000   // BANK("Wario OAM 1")
#define OAM2 BANK_OAM_1fc000  // BANK("Wario OAM 2")
#define OAM3 BANK_OAM_1dc000  // BANK("Wario OAM 3")

// ld a, v / ld [addr], a
static inline void st(GB *gb, uint16_t addr, uint8_t v)
{
	gb->cpu.a = v;
	wr8(gb, addr, v);
}

// ld a, [addr]
static inline uint8_t ld(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	return gb->cpu.a;
}

static inline void xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

// ld a, [addr] / and a; true if nz
static inline bool ld_and(GB *gb, uint16_t addr)
{
	uint8_t v = ld(gb, addr);
	return alu_and(gb, v, v) != 0;
}

// ld a, b / and a; true if nz
static inline bool b_and(GB *gb)
{
	gb->cpu.a = gb->cpu.b;
	return alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
}

// ld a, [addr] / bit n, a
static inline bool ld_bit(GB *gb, uint16_t addr, int bit)
{
	return alu_bit(gb, ld(gb, addr), bit);
}

// ld a, [addr] / and mask; true if nz
static inline bool ld_mask(GB *gb, uint16_t addr, uint8_t mask)
{
	gb->cpu.a = alu_and(gb, rd8(gb, addr), mask);
	return gb->cpu.a != 0;
}

// ld a, [addr] / cp v
static inline void ld_cp(GB *gb, uint16_t addr, uint8_t v)
{
	alu_cp(gb, ld(gb, addr), v);
}

// wDMASourceBank/Ptr = gfx, call LoadWarioGfx
static inline void load_gfx(GB *gb, uint8_t bank, uint16_t ptr)
{
	wr8(gb, wDMASourceBank, bank);
	wr8(gb, wDMASourcePtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wDMASourcePtr + 1, (uint8_t)ptr);
	CALL(LoadWarioGfx);
}

static inline void set_oam(GB *gb, uint8_t bank, uint16_t ptr)
{
	wr8(gb, wWarioOAMBank, bank);
	wr8(gb, wWarioOAMPtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wWarioOAMPtr + 1, (uint8_t)ptr);
}

static inline void set_frameset(GB *gb, uint16_t ptr)
{
	wr8(gb, wWarioFramesetPtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wWarioFramesetPtr + 1, (uint8_t)ptr);
}

// ld a, BANK("Wario OAM n") / ldh [hCallFuncBank], a / hcall UpdateWarioAnimation
static inline void anim(GB *gb, uint8_t bank)
{
	FARCALL_BANK(bank, UpdateWarioAnimation);
}

// bottom -1, left -9, right 9, top -27 (a = -27)
static inline void coll_box_blrt(GB *gb)
{
	wr8(gb, wCollisionBoxBottom, (uint8_t)-1);
	wr8(gb, wCollisionBoxLeft, (uint8_t)-9);
	wr8(gb, wCollisionBoxRight, 9);
	st(gb, wCollisionBoxTop, (uint8_t)-27);
}

// bottom -1, top -27, left -9, right 9 (a = 9)
static inline void coll_box_btlr(GB *gb)
{
	wr8(gb, wCollisionBoxBottom, (uint8_t)-1);
	wr8(gb, wCollisionBoxTop, (uint8_t)-27);
	wr8(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
}

// update_pos_y
static inline void update_pos_y(GB *gb)
{
	wr8(gb, wWarioYPos + 0, rd8(gb, hYPosHi));
	gb->cpu.a = rd8(gb, hYPosHi + 1);
	wr8(gb, wWarioYPos + 1, gb->cpu.a);
	SET_HL(gb, hYPosHi + 1);
	SET_DE(gb, wWarioYPos + 1);
}

// get_pos
static inline void get_pos(GB *gb)
{
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wWarioPos + i);
		wr8(gb, hPos + i, gb->cpu.a);
	}
	SET_HL(gb, wWarioPos + 3);
	SET_DE(gb, hPos + 3);
}

// ld a, [wSFXLoopCounter] / sub 1 / ld [wSFXLoopCounter], a / jr nc, .skip
// ld a, reload / ld [wSFXLoopCounter], a / play_sfx sfx
static inline void sfx_loop(GB *gb, uint8_t reload, uint16_t sfx)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wSFXLoopCounter), 1);
	wr8(gb, wSFXLoopCounter, gb->cpu.a);
	if (!CF)
		return;
	st(gb, wSFXLoopCounter, reload);
	play_sfx(gb, sfx);
}

// ld a, [wDirection] / and a / jr nz: picks a frameset
static inline void frameset_by_dir(GB *gb, uint16_t left, uint16_t right)
{
	set_frameset(gb, ld_and(gb, wDirection) ? right : left);
}

// ld a, [wRoomTransitionParam] / and a; true if nz
static inline bool room_transition(GB *gb)
{
	return ld_and(gb, wRoomTransitionParam);
}

// ld a, [wWarioAnimationEnded] / and a; true if nz
static inline bool anim_ended(GB *gb)
{
	return ld_and(gb, wWarioAnimationEnded);
}

// The camera/floor check after moving down:
// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp CAM_TRANSITIONS
// jr c, .skip / call GetFloorForYPos / ld a, [wFloor] / sub c / jr z / jr c
// Returns true if it should transition down.
static inline bool down_floor_check(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	if (CF)
		return false;
	CALL(GetFloorForYPos);
	gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
	return !(gb->cpu.f & (FLAG_Z | FLAG_C));
}

// ld a, [wDirection] / and a / jr nz, .right / call MoveWarioLeft / ...
static inline void move_by_dir(GB *gb)
{
	if (ld_and(gb, wDirection))
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
}

// ld a, [wWarioXPos + 1] / and $f0 / add $08 / ld [wWarioXPos + 1], a
static inline void center_on_ladder(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
}

// ld hl, wWarioStateCounter / inc [hl] / ld a, [hl]
static inline uint8_t inc_counter(GB *gb)
{
	SET_HL(gb, wWarioStateCounter);
	gb->cpu.a = alu_inc(gb, rd8(gb, wWarioStateCounter));
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	return gb->cpu.a;
}

// ld a, [wDirection] / and a: true if facing right
static inline bool facing_right(GB *gb)
{
	return ld_and(gb, wDirection);
}

// ld a, v / ld [wCollisionBox...], a, in this order
static inline void collision_box(GB *gb, int8_t bottom, int8_t top, int8_t left, int8_t right)
{
	st(gb, wCollisionBoxBottom, (uint8_t)bottom);
	st(gb, wCollisionBoxTop, (uint8_t)top);
	st(gb, wCollisionBoxLeft, (uint8_t)left);
	st(gb, wCollisionBoxRight, (uint8_t)right);
}

// xor a / ld [addr], a for each address, up to a 0
static inline void clear(GB *gb, const uint16_t *addrs)
{
	xor_a(gb);
	for (; *addrs; addrs++)
		wr8(gb, *addrs, 0);
}

// xor a / ld [wFrameDuration], a / ld [wAnimationFrame], a
static inline void reset_anim(GB *gb)
{
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
}

// ld a, t / ld [wTouchState], a / ld a, t / ld [wStingTouchState], a /
// ld a, ca94 / ld [wca94], a
static inline void touch_states(GB *gb, uint8_t t, uint8_t ca94)
{
	st(gb, wTouchState, t);
	st(gb, wStingTouchState, t);
	st(gb, wca94, ca94);
}

// ld a, [wDirection] / xor $1 / ld [wDirection], a
static inline void flip_dir(GB *gb)
{
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), 0x1);
	wr8(gb, wDirection, gb->cpu.a);
}

// flip_dir / and a: true if now facing right
static inline bool flip_dir_and(GB *gb)
{
	flip_dir(gb);
	return alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
}

#endif
