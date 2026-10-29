// Port of src/engine/level/objects/nobiiru.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define NOBIIRU_FUNC_4380E (ADDR_NobiiruFunc + 0x37) // .Func_4380e

// .asm_437f7: back in the wall.
static void retract(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_COLLBOX_BOTTOM);
	wr8(gb, hl++, 0xfc);
	wr8(gb, hl++, 0xfd);
	gb->cpu.a = 0x02;
	wr8(gb, hl, 0x02);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_698ff);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 60);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
}

PORT(NobiiruFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(NOBIIRU_FUNC_4380E >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)NOBIIRU_FUNC_4380E;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, NOBIIRU_LEFT);
	if (gb->cpu.f & FLAG_Z)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR)); // right variant
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
	retract(gb, hl);
}

// rst JumpTable on OBJ_VAR_1
PORT_LOCAL_JUMP(NobiiruFunc, Func_4380e)
{
	gb->cpu.a = rd8(gb, wCurObjVar1);
	gb_push16(gb, NOBIIRU_FUNC_4380E + 4); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

// .SetFrameset: call SetObjectFramesetPtr / ld a, b / ld [hli], a / inc [hl]
static void set_frameset(GB *gb)
{
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl++, gb->cpu.a);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
}

// .SetCollisionAndFrameset: a = left col, c = right col, b = duration,
// de = frameset
static void set_collision_and_frameset(GB *gb)
{
	uint16_t hl = LO(REG_HL(gb), OBJ_COLLBOX_LEFT);
	wr8(gb, hl++, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	set_frameset(gb);
}

PORT_LOCAL(NobiiruFunc, SetCollisionAndFrameset) { set_collision_and_frameset(gb); }
PORT_LOCAL(NobiiruFunc, SetFrameset) { set_frameset(gb); }

// ld hl, wCurObjStateDuration / dec [hl]; returns whether it ran out
static bool count_down(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

// Sticks out of the wall.
PORT_LOCAL(NobiiruFunc, Func_4382e)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	if (!count_down(gb))
		return;
	play_sfx(gb, SFX_079);
	gb->cpu.a = 0x0c;
	wr8(gb, wCurObjVar2, 0x0c);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C) {
		CALL(MoveObjectRightByVar2);
		SET_DE(gb, ADDR_Frameset_698b1);
		gb->cpu.c = 0x00;
	} else {
		CALL(MoveObjectLeftByVar2);
		SET_DE(gb, ADDR_Frameset_698d8);
		gb->cpu.c = 0x02;
	}
	// .asm_43861
	uint16_t hl = LO(REG_HL(gb), OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_FULL_STING);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.b = 0x02;
	set_collision_and_frameset(gb);
}

typedef struct {
	uint16_t fs_nc; uint8_t a_nc, c_nc; // facing left
	uint16_t fs_c; uint8_t a_c, c_c; // facing right
	uint8_t b;
	bool retracting; // moves back against its direction
} Step;

// One step of stretching out (or back), with the collision box to match.
static void step(GB *gb, const Step *s)
{
	if (!count_down(gb))
		return;
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool c = gb->cpu.f & FLAG_C;
	if (c != s->retracting)
		CALL(MoveObjectRightByVar2);
	else
		CALL(MoveObjectLeftByVar2);
	SET_DE(gb, c ? s->fs_c : s->fs_nc);
	gb->cpu.a = c ? s->a_c : s->a_nc;
	gb->cpu.c = c ? s->c_c : s->c_nc;
	gb->cpu.b = s->b;
	set_collision_and_frameset(gb);
}

static const Step step_438a0 = { ADDR_Frameset_698de, 0xf8, 0x0b, ADDR_Frameset_698b7, 0xf4, 0x07, 0x02, false };
static const Step step_438c5 = { ADDR_Frameset_698e1, 0xf4, 0x0f, ADDR_Frameset_698ba, 0xf0, 0x0b, 0x02, false };
static const Step step_438ea = { ADDR_Frameset_698e4, 0xf0, 0x13, ADDR_Frameset_698bd, 0xec, 0x0f, 0x02, false };
static const Step step_43910 = { ADDR_Frameset_698e7, 0xec, 0x17, ADDR_Frameset_698c0, 0xe8, 0x13, 0x14, false };
static const Step step_4396e = { ADDR_Frameset_698f0, 0xf0, 0x13, ADDR_Frameset_698c9, 0xec, 0x0f, 0x02, true };
static const Step step_43994 = { ADDR_Frameset_698f3, 0xf4, 0x0f, ADDR_Frameset_698cc, 0xf0, 0x0b, 0x02, true };
static const Step step_439ba = { ADDR_Frameset_698f6, 0xf8, 0x0b, ADDR_Frameset_698cf, 0xf4, 0x07, 0x02, true };
static const Step step_439e0 = { ADDR_Frameset_698f9, 0xfc, 0x07, ADDR_Frameset_698d2, 0xf8, 0x03, 0x02, true };
static const Step step_43a06 = { ADDR_Frameset_698fc, 0xff, 0x02, ADDR_Frameset_698d5, 0xfd, 0x00, 0x02, true };

PORT_LOCAL(NobiiruFunc, Func_43876)
{
	static const Step s = { ADDR_Frameset_698db, 0xfc, 0x07, ADDR_Frameset_698b4, 0xf8, 0x03, 0x02, false };
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0x04;
	wr8(gb, wCurObjVar2, 0x04);
	// the rest of step() without the countdown
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool c = gb->cpu.f & FLAG_C;
	if (c)
		CALL(MoveObjectRightByVar2);
	else
		CALL(MoveObjectLeftByVar2);
	SET_DE(gb, c ? s.fs_c : s.fs_nc);
	gb->cpu.a = c ? s.a_c : s.a_nc;
	gb->cpu.c = c ? s.c_c : s.c_nc;
	gb->cpu.b = s.b;
	set_collision_and_frameset(gb);
}

PORT_LOCAL(NobiiruFunc, Func_438a0) { step(gb, &step_438a0); }
PORT_LOCAL(NobiiruFunc, Func_438c5) { step(gb, &step_438c5); }
PORT_LOCAL(NobiiruFunc, Func_438ea) { step(gb, &step_438ea); }
PORT_LOCAL(NobiiruFunc, Func_43910) { step(gb, &step_43910); }
PORT_LOCAL(NobiiruFunc, Func_4396e) { step(gb, &step_4396e); }
PORT_LOCAL(NobiiruFunc, Func_43994) { step(gb, &step_43994); }
PORT_LOCAL(NobiiruFunc, Func_439ba) { step(gb, &step_439ba); }
PORT_LOCAL(NobiiruFunc, Func_439e0) { step(gb, &step_439e0); }
PORT_LOCAL(NobiiruFunc, Func_43a06) { step(gb, &step_43a06); }

PORT_LOCAL(NobiiruFunc, Func_43936)
{
	if (!count_down(gb))
		return;
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_698c3 : ADDR_Frameset_698ea);
	uint16_t hl = LO(REG_HL(gb), OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_0B);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.b = 0x08;
	set_frameset(gb);
}

PORT_LOCAL(NobiiruFunc, Func_43956)
{
	if (!count_down(gb))
		return;
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_698c6 : ADDR_Frameset_698ed);
	gb->cpu.b = 0x14;
	set_frameset(gb);
}

PORT_LOCAL(NobiiruFunc, Func_43a2c)
{
	if (!count_down(gb))
		return;
	gb->cpu.a = 0x0c;
	wr8(gb, wCurObjVar2, 0x0c);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C)
		CALL(MoveObjectLeftByVar2);
	else
		CALL(MoveObjectRightByVar2);
	retract(gb, REG_HL(gb));
}
