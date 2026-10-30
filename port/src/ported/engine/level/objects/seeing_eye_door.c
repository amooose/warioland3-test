// Port of src/engine/level/objects/seeing_eye_door.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SEEING_EYE_DOOR_CHECK_DISTANCE (ADDR_SeeingEyeDoorFunc + 0x2b) // .CheckWarioDistance

PORT(SeeingEyeDoorFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(SEEING_EYE_DOOR_CHECK_DISTANCE >> 8));
	wr8(gb, hl--, (uint8_t)SEEING_EYE_DOOR_CHECK_DISTANCE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	gb->cpu.a = 24;
	wr8(gb, wCurObjVar2, 24);
	CALL(MoveObjectDownByVar2);
	SET_DE(gb, ADDR_Frameset_68c47);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x02);
	wr8(gb, hl--, 0xfd); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 0x17;
	wr8(gb, hl, 0x17); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}

#define SEEING_EYE_DOOR_LOOK_AROUND (ADDR_SeeingEyeDoorFunc + 0xa0) // .LookAround
#define SEEING_EYE_DOOR_DEFAULT (ADDR_SeeingEyeDoorFunc + 0xb4) // .Default
#define SEEING_EYE_DOOR_STATE_54F6 (ADDR_SeeingEyeDoorFunc + 0x56) // .WaitForWarioInRange

// ld a, [wWarioScreenXPos] / add $2a / ld b, a / ld a, [wCurObjScreenXPos] /
// add $2a / cp b: carry when the door is left of Wario
static bool wario_is_right(GB *gb)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return (gb->cpu.f & FLAG_C) != 0;
}

// ld [hl], a / play_sfx sfx / ld de, frameset / jp SetObjectFramesetPtr
static void set_var1(GB *gb, uint8_t var1, uint16_t sfx, uint16_t frameset)
{
	wr8(gb, REG_HL(gb), var1);
	play_sfx(gb, sfx);
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
}

// .asm_45545
static void look_done(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_68c36);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(SEEING_EYE_DOOR_STATE_54F6 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SEEING_EYE_DOOR_STATE_54F6;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// .set_look_around
static void set_look_around(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurObjVar1);
	alu_cp(gb, gb->cpu.a, 0x2e);
	if (gb->cpu.f & FLAG_Z) {
		look_done(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x5a);
	if (gb->cpu.f & FLAG_Z) {
		look_done(gb);
		return;
	}
	SET_DE(gb, wario_is_right(gb) ? ADDR_Frameset_68c56 : ADDR_Frameset_68c4a); // ld_obj_frameset
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 8;
	wr8(gb, hl++, 8);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(SEEING_EYE_DOOR_LOOK_AROUND >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SEEING_EYE_DOOR_LOOK_AROUND;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// .Default
static void seeing_eye_door_default(GB *gb)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_INVISIBLE_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		set_look_around(gb);
		return;
	}
	gb->cpu.b = alu_sub(gb, rd8(gb, wCurObjScreenYPos), 0x19);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioScreenYPos), gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x36);
	if (!(gb->cpu.f & FLAG_C)) {
		set_look_around(gb);
		return;
	}
	uint8_t d = gb->cpu.d = rd8(gb, wWarioScreenXPos);
	uint8_t e = gb->cpu.e = rd8(gb, wCurObjScreenXPos);
	uint16_t hl = wCurObjState;
	gb->cpu.a = alu_and(gb, rd8(gb, hl), rd8(gb, hl));
	if (gb->cpu.a) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, hl, 0);
		hl = LO(hl, OBJ_FLAGS);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_NO_COLLISION));
	}
	hl = LO(hl, OBJ_VAR_1);
	SET_HL(gb, hl);
	uint8_t v = gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, v, 0x5c);
	if (v == 0x5c) {
		// .asm_45597
		gb->cpu.a = alu_add(gb, e, 0x17);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C) {
			set_var1(gb, 0x2d, SFX_07A, ADDR_Frameset_68c56);
			return;
		}
		gb->cpu.a = alu_sub(gb, e, 0x18);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C)
			return;
		set_var1(gb, 0x5b, SFX_07A, ADDR_Frameset_68c4a);
		return;
	}
	alu_cp(gb, v, 0x2d);
	if (v == 0x2d) {
		// .asm_455ba
		gb->cpu.a = alu_add(gb, e, 0x46);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C) {
			// .asm_455d6
			play_sfx(gb, SFX_07A);
			gb->cpu.a = 0x2e;
			wr8(gb, hl, 0x2e);
			SET_DE(gb, ADDR_Frameset_68c59);
			CALL(SetObjectFramesetPtr);
			return;
		}
		gb->cpu.a = alu_add(gb, e, 0x17);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C)
			return;
		play_sfx(gb, SFX_07B);
		gb->cpu.a = 0x5c;
		wr8(gb, hl, 0x5c);
		SET_DE(gb, ADDR_Frameset_68c50);
		CALL(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, v, 0x2e);
	if (v == 0x2e) {
		// .asm_455e7
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
		gb->cpu.a = alu_add(gb, e, 0x46);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C)
			return;
		play_sfx(gb, SFX_07A);
		gb->cpu.a = 0x2d;
		wr8(gb, hl, 0x2d);
		SET_DE(gb, ADDR_Frameset_68c56);
		CALL(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, v, 0x5b);
	if (v == 0x5b) {
		// .asm_45602
		gb->cpu.a = alu_sub(gb, e, 0x18);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C) {
			// .asm_4561e
			play_sfx(gb, SFX_07B);
			gb->cpu.a = 0x5c;
			wr8(gb, hl, 0x5c);
			SET_DE(gb, ADDR_Frameset_68c50);
			CALL(SetObjectFramesetPtr);
			return;
		}
		gb->cpu.a = alu_sub(gb, e, 0x47);
		alu_cp(gb, gb->cpu.a, d);
		if (gb->cpu.f & FLAG_C)
			return;
		play_sfx(gb, SFX_07A);
		gb->cpu.a = 0x5a;
		wr8(gb, hl, 0x5a);
		SET_DE(gb, ADDR_Frameset_68c47);
		CALL(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, v, 0x5a);
	if (v == 0x5a) {
		// .asm_4562f
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
		gb->cpu.a = alu_sub(gb, e, 0x47);
		alu_cp(gb, gb->cpu.a, d);
		if (!(gb->cpu.f & FLAG_C))
			return;
		play_sfx(gb, SFX_07A);
		gb->cpu.a = 0x5b;
		wr8(gb, hl, 0x5b);
		SET_DE(gb, ADDR_Frameset_68c4a);
		CALL(SetObjectFramesetPtr);
		return;
	}
}

PORT_LOCAL(SeeingEyeDoorFunc, Default)
{
	seeing_eye_door_default(gb);
}

PORT_LOCAL(SeeingEyeDoorFunc, CheckWarioDistance)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SEEING_EYE_DOOR_DEFAULT >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SEEING_EYE_DOOR_DEFAULT;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	if (wario_is_right(gb)) {
		gb->cpu.a = 0x2e;
		SET_DE(gb, ADDR_Frameset_68c59);
	} else {
		gb->cpu.a = 0x5a;
		SET_DE(gb, ADDR_Frameset_68c47);
	}
	wr8(gb, wCurObjVar1, gb->cpu.a);
	CALL(SetObjectFramesetPtr);
	seeing_eye_door_default(gb);
}

PORT_LOCAL(SeeingEyeDoorFunc, LookAround)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t v = (uint8_t)(rd8(gb, hl) - 1);
	wr8(gb, hl, v);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_N | (v ? 0 : FLAG_Z) | ((v & 0xf) == 0xf ? FLAG_H : 0));
	if (v)
		return;
	look_done(gb);
}

// Waits for Wario (unless invisible) to come within $36 pixels below the
// door's top, then checks his distance again.
PORT_LOCAL(SeeingEyeDoorFunc, WaitForWarioInRange)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_INVISIBLE_WARIO);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_sub(gb, rd8(gb, wCurObjScreenYPos), 0x19);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioScreenYPos), gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x36);
	if (!(gb->cpu.f & FLAG_C))
		return;
	port_SeeingEyeDoorFunc__CheckWarioDistance(gb); // jr .CheckWarioDistance
}
