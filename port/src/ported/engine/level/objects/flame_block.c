// Port of src/engine/level/objects/flame_block.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

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

// res OBJSUBFLAG_UNINITIALISED_F of hl's object
static inline uint16_t set_initialised(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	return hl;
}

// Sets hl's object's collision box (right, left, bottom), a = bottom.
static inline uint16_t set_collbox(GB *gb, uint16_t hl, uint8_t right, uint8_t left, uint8_t bottom)
{
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, right);
	wr8(gb, hl--, left);
	gb->cpu.a = bottom;
	wr8(gb, hl, bottom);
	return hl;
}

// ld hl, wCurObjFlags / res OBJFLAG_INVISIBLE_F, [hl] / set OBJFLAG_NO_COLLISION_F, [hl]
static inline uint16_t show_no_collision(GB *gb)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	return hl;
}

PORT(FlameBlockFunc)
{
	gb->cpu.a = rd8(gb, wFlameBlockCleared);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFlags, 0);
		return;
	}
	// .init
	uint16_t hl = show_no_collision(gb);
	SET_HL(gb, set_update(gb, hl, ADDR_FlameBlockFunc + 0x30)); // .Update
	SET_DE(gb, ADDR_Frameset_695de);
	CALL(SetObjectFramesetPtr);
	hl = set_initialised(gb, REG_HL(gb));
	SET_HL(gb, set_collbox(gb, hl, 13, (uint8_t)-14, (uint8_t)-1));
}

#define FLAME_BLOCK_DETROY (ADDR_FlameBlockFunc + 0x4b) // .Detroy

// Waits for every torch to be lit, then starts breaking up.
PORT_LOCAL(FlameBlockFunc, Update)
{
	gb->cpu.a = rd8(gb, wNumLitTorches);
	alu_cp(gb, gb->cpu.a, NUM_FLAME_BLOCK_TORCHES);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, wFlameBlockCleared, gb->cpu.a);
	SET_DE(gb, ADDR_Frameset_695e7);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 159;
	wr8(gb, hl++, 159); // OBJ_STATE_DURATION
	SET_HL(gb, set_update(gb, hl, FLAME_BLOCK_DETROY));
}

PORT_LOCAL(FlameBlockFunc, Detroy)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 25);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
