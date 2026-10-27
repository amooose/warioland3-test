// Port of src/engine/level/objects/owl.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define OWL_SLEEP (ADDR_OwlFunc + 0x3c) // .Sleep
#define OWL_WAKE_UP (ADDR_OwlFunc + 0x66) // .WakeUp
#define OWL_FLY_UPWARDS (ADDR_OwlFunc + 0x92) // .FlyUpwards
#define OWL_FUNC_492B4 (ADDR_OwlFunc + 0xe1) // .Func_492b4

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

// ld l, OBJ_INTERACTION_TYPE / ld a, [hl] / and HEAVY_OBJ / or type / ld [hld], a
static inline uint16_t set_interaction(GB *gb, uint16_t hl, uint8_t type)
{
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// Plays the flap sound on the last tick of the given frame.
static void flap(GB *gb, uint8_t frame)
{
	gb->cpu.a = rd8(gb, wCurObjFrame);
	alu_cp(gb, gb->cpu.a, frame);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_dec(gb, rd8(gb, wCurObjFrameDuration));
	if (!gb->cpu.a)
		play_sfx(gb, SFX_FLAP);
}

// .set_fly_upwards
static void set_fly_upwards(GB *gb, uint16_t hl)
{
	SET_HL(gb, set_update(gb, hl, OWL_FLY_UPWARDS));
	SET_DE(gb, ADDR_Frameset_69335);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 54;
	wr8(gb, hl++, 54);
	SET_HL(gb, hl);
}

// Sleeps by day; at night it flies off straight away.
PORT_JUMP(OwlFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x06);
	wr8(gb, hl--, 0xf9); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0xfe); // OBJ_COLLBOX_BOTTOM
	gb->cpu.a = alu_rra(gb, rd8(gb, wDayNight));
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, set_update(gb, hl, OWL_SLEEP));
		SET_DE(gb, ADDR_Frameset_692e7);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	// .night
	play_sfx(gb, SFX_OWL_HOOT);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_0B);
	set_fly_upwards(gb, hl);
	RET();
}

PORT_LOCAL(OwlFunc, Sleep)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, set_update(gb, hl, OWL_WAKE_UP));
	SET_DE(gb, ADDR_Frameset_69318);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x28);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_0B);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_OWL_HOOT);
}

PORT_LOCAL(OwlFunc, WakeUp)
{
	flap(gb, 0x0c);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	set_fly_upwards(gb, hl);
}

// Flies up, then turns towards Wario.
PORT_LOCAL_JUMP(OwlFunc, FlyUpwards)
{
	flap(gb, 0x0c);
	CALL(MoveObjectUp);
	uint16_t hl = LO(REG_HL(gb), OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = 30;
	wr8(gb, hl++, 30);
	hl = set_update(gb, hl, OWL_FUNC_492B4);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_OWL);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		SET_DE(gb, ADDR_Frameset_69377);
	} else {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		SET_DE(gb, ADDR_Frameset_6936c);
	}
	JUMP(SetObjectFramesetPtr);
}

// Flies back and forth, turning around every 60 frames, and despawns once
// it's been grabbed.
PORT_LOCAL_JUMP(OwlFunc, Func_492b4)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		// .despawn
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFlags, 0);
		SET_HL(gb, wCurObjUnk02);
		FARCALL(DespawnObject);
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		// .asm_492f7
		gb->cpu.a = 60;
		wr8(gb, hl, 60);
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
		wr8(gb, hl, gb->cpu.a);
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69377 : ADDR_Frameset_6936c);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C) {
		flap(gb, 0x23);
		JUMP(MoveObjectRight);
	} else {
		flap(gb, 0x1e);
		JUMP(MoveObjectLeft);
	}
}
