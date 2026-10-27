// Port of src/engine/level/update_objects.asm
#include "ported.h"

#define OBJ_STRUCT_SIZE (wObj2Flags - wObj1Flags)

// Draws every active, on-screen, visible object whose flags (masked as
// below) match.
static void draw_objects(GB *gb, uint8_t match)
{
	for (int n = 0; n < NUM_OBJECTS; n++) {
		uint16_t obj = (uint16_t)(wObj1Flags + n * OBJ_STRUCT_SIZE);
		SET_HL(gb, obj);
		gb->cpu.a = alu_and(gb, rd8(gb, obj),
			OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_INVISIBLE | OBJFLAG_PRIORITY);
		alu_cp(gb, gb->cpu.a, match);
		if (!(gb->cpu.f & FLAG_Z))
			continue;
		gb->cpu.e = (uint8_t)(wObj1ScreenYPos + n * OBJ_STRUCT_SIZE);
		gb->cpu.l = (uint8_t)(wObj1Id + n * OBJ_STRUCT_SIZE);
		CALL(UpdateObjSprite);
	}
}

PORT(DrawObjects_NoPriority)
{
	draw_objects(gb, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_PRIORITY);
}

PORT(DrawObjects_WithPriority)
{
	draw_objects(gb, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
}


#define CPU (&gb->cpu)

// One of CheckObjectOnScreen's range checks of the position hi:lo against
// the camera position in hl: lo += add, then if the position is past hl,
// hl += span and lo -= sub, and the position must be below hl. A carry out
// of hi on the first step counts as in range if wrap_in, otherwise it goes
// on to the second step. Returns whether the position is in range.
static bool in_range(GB *gb, uint8_t *hi, uint8_t *lo, uint8_t add, uint8_t span, uint8_t sub,
	bool wrap_in)
{
	CPU->a = *lo = alu_add(gb, *lo, add);
	if (CPU->f & FLAG_C) {
		*hi = alu_inc(gb, *hi);
		if (CPU->f & FLAG_Z) {
			if (wrap_in)
				return true;
			goto upper;
		}
	}
	CPU->a = *hi;
	alu_cp(gb, *hi, CPU->h);
	if (CPU->f & FLAG_C)
		return false;
	if (CPU->f & FLAG_Z) {
		CPU->a = *lo;
		alu_cp(gb, *lo, CPU->l);
		if (CPU->f & FLAG_C)
			return false;
		if (CPU->f & FLAG_Z)
			return true;
	}
upper:
	CPU->a = CPU->l = alu_add(gb, CPU->l, span);
	if (CPU->f & FLAG_C)
		CPU->h = alu_inc(gb, CPU->h);
	CPU->a = *lo = alu_sub(gb, *lo, sub);
	if (CPU->f & FLAG_C) {
		*hi = alu_dec(gb, *hi);
		CPU->a = alu_inc(gb, *hi);
		if (CPU->f & FLAG_Z)
			return true;
	}
	CPU->a = *hi;
	alu_cp(gb, *hi, CPU->h);
	if (CPU->f & FLAG_C)
		return true;
	if (!(CPU->f & FLAG_Z))
		return false;
	CPU->a = *lo;
	alu_cp(gb, *lo, CPU->l);
	return CPU->f & FLAG_C;
}

// ld hl, addr / ld a, [hli] / ld l, [hl] / ld h, a (a big-endian pointer)
static void load_hl_be(GB *gb, uint16_t addr)
{
	CPU->a = rd8(gb, addr);
	CPU->l = rd8(gb, addr + 1);
	CPU->h = CPU->a;
}

// seems to be a function to load/unload objects
// that are inside and outside the screen
PORT(CheckObjectOnScreen)
{
	CPU->c = rd8(gb, wCurObjYPos + 0); // y lo
	CPU->b = rd8(gb, wCurObjYPos + 1); // y hi
	CPU->a = CPU->e = rd8(gb, wCurObjYPos + 2); // x lo
	CPU->d = rd8(gb, wCurObjYPos + 3); // x hi

	load_hl_be(gb, wCameraSCY);
	gb_push16(gb, REG_BC(gb));
	gb_push16(gb, REG_DE(gb));
	gb_push16(gb, REG_HL(gb));
	bool on_screen = false;
	if (in_range(gb, &CPU->b, &CPU->c, 24, SCREEN_HEIGHT_PX, 56, false)) {
		// .skip_y2
		load_hl_be(gb, wCameraSCX);
		on_screen = in_range(gb, &CPU->d, &CPU->e, 24, SCREEN_WIDTH_PX, 0x30, true);
	} else {
		// .asm_630fd
		CPU->a = alu_and(gb, rd8(gb, wCurObjState), 0xfe);
		alu_cp(gb, CPU->a, OBJSTATE_48);
		if (CPU->f & FLAG_Z)
			goto pop;
	}
	SET_HL(gb, wCurObjFlags);
	if (on_screen)
		wr8(gb, wCurObjFlags, rd8(gb, wCurObjFlags) | OBJFLAG_ON_SCREEN);
	else
		wr8(gb, wCurObjFlags, rd8(gb, wCurObjFlags) & ~OBJFLAG_ON_SCREEN);
pop:
	SET_HL(gb, gb_pop16(gb));
	SET_DE(gb, gb_pop16(gb));
	SET_BC(gb, gb_pop16(gb));

	// whether the object is far enough outside the screen to unload
	bool keep = false;
	CPU->a = alu_rla(gb, CPU->d);
	if (!(CPU->f & FLAG_C)) {
		CPU->a = alu_rla(gb, CPU->b);
		if (!(CPU->f & FLAG_C) && in_range(gb, &CPU->b, &CPU->c, 92, 0x90, 0xcb, true)) {
			// .asm_63147
			load_hl_be(gb, wCameraSCX);
			keep = in_range(gb, &CPU->d, &CPU->e, 0x6f, SCREEN_WIDTH_PX, 0xde, true);
		}
	}
	if (keep)
		return;

	// .asm_6317e
	CPU->a = alu_and(gb, rd8(gb, wCurObjSubState), 0xf | OBJSUBFLAG_UNK_4);
	alu_cp(gb, CPU->a, 0x0 | OBJSUBFLAG_UNK_4);
	if (!(CPU->f & FLAG_C))
		return;
	// OBJSUBFLAG_UNK_4 is not set
	SET_HL(gb, wCurObjUnk02);
	alu_and(gb, CPU->a, CPU->a);
	if (!(CPU->f & FLAG_Z)) {
		FARCALL(DespawnObject);
		return;
	}
	CPU->a = 0;
	CPU->f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// a = value to fill wObjects with, c = a bytes (0: 256)
PORT(ClearObjects)
{
	uint16_t hl = wObjects;
	gb->cpu.c = gb->cpu.a;
	do {
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	SET_HL(gb, hl);
}

// a = value to fill wObjDataPointers with
PORT(ClearObjDataPointers)
{
	uint16_t hl = wObjDataPointers;
	gb->cpu.c = wObjDataPointersEnd - wObjDataPointers;
	do {
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	SET_HL(gb, hl);
}

// Sets the obj state at hl by the y velocity in hffa0: states[0] below 2,
// states[1] below 3, states[2] below 4, else states[3]; then HomeJumpRet.
static void set_state_by_y_vel(GB *gb, const uint8_t states[4])
{
	gb->cpu.a = rd8(gb, hffa0);
	int i;
	for (i = 0; i < 3; i++) {
		alu_cp(gb, gb->cpu.a, (uint8_t)(2 + i));
		if (gb->cpu.f & FLAG_C)
			break;
	}
	gb->cpu.a = states[i];
	wr8(gb, REG_HL(gb), gb->cpu.a);
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_62d57)
{
	static const uint8_t states[4] = {OBJSTATE_STUN_LEFT_START, OBJSTATE_20, OBJSTATE_22, OBJSTATE_24};
	set_state_by_y_vel(gb, states);
}

PORT_JUMP(Func_62e31)
{
	static const uint8_t states[4] = {OBJSTATE_STUN_RIGHT_START, OBJSTATE_21, OBJSTATE_23, OBJSTATE_25};
	set_state_by_y_vel(gb, states);
}

// Stores hPos's y in wCurObjYPos, and updates OBJ_VAR_2 by the slope the
// object is on: set to `set` on the slope `slope_set`, kept on the slope
// `slope_keep`, else counted down to 0. Then hl = OBJ_STATE, and on to
// `next`.
static void store_y_pos_by_slope(GB *gb, uint8_t slope_keep, uint8_t slope_set, bool dec, uint16_t next)
{
	wr8(gb, wCurObjYPos + 0, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, wCurObjYPos + 1, gb->cpu.a);
	SET_HL(gb, wCurObjYPos + 1);
	gb->cpu.a = alu_and(gb, rd8(gb, wObjSlopeInteraction), 0x0f);
	alu_cp(gb, gb->cpu.a, slope_keep);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.l = OBJ_VAR_2;
		uint16_t hl = REG_HL(gb);
		alu_cp(gb, gb->cpu.a, slope_set);
		if (gb->cpu.f & FLAG_Z) {
			if (dec)
				gb->cpu.a = alu_dec(gb, gb->cpu.a);
			wr8(gb, hl, gb->cpu.a);
		} else {
			gb->cpu.a = rd8(gb, hl);
			if (alu_and(gb, gb->cpu.a, gb->cpu.a))
				wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		}
	}
	gb->cpu.l = OBJ_STATE;
	JUMP_TO(next); // fallthrough
}

PORT_JUMP(Func_62d34) { store_y_pos_by_slope(gb, RIGHT_SLOPE, 0x02, true, ADDR_Func_62d57); }
PORT_JUMP(Func_62e0f) { store_y_pos_by_slope(gb, LEFT_SLOPE, 0x01, false, ADDR_Func_62e31); }

// hPos = wCurObjYPos + (-dy, -dx), with the asm's sub / sbc
static void set_hpos_sub(GB *gb, uint16_t hl, uint8_t dy)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), dy);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// ld a, [addr] / cpl / inc a / ld b, a
static uint8_t neg_into_b(GB *gb, uint16_t addr)
{
	gb->cpu.a = alu_inc(gb, alu_cpl(gb, rd8(gb, addr)));
	gb->cpu.b = gb->cpu.a;
	return gb->cpu.b;
}

// An object thrown to the left (Func_62ec3) or right (Func_62faa): bc =
// table of y movements, indexed by OBJ_VAR_3 (+$10 if heavy).
static void thrown_object_movement(GB *gb, bool left)
{
	uint16_t routine = left ? ADDR_Func_62ec3 : ADDR_Func_62faa;
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	if (gb->cpu.f & FLAG_C) { // heavy
		gb->cpu.a = alu_add(gb, gb->cpu.c, 0x10);
		gb->cpu.c = gb->cpu.a;
	}
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.l = OBJ_Y_POS;
		hl = REG_HL(gb);
		gb->cpu.c = rd8(gb, hl);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (!(gb->cpu.f & FLAG_C))
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
	if (left)
		CALL(MoveObjectLeftByVar2);
	else
		CALL(MoveObjectRightByVar2);

	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x28);
	alu_cp(gb, gb->cpu.a, left ? 0xf0 : 0x10);
	if (left ? !(gb->cpu.f & FLAG_C) : (gb->cpu.f & FLAG_C)) {
		gb->cpu.l = OBJ_FLAGS;
		hl = REG_HL(gb);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << OBJFLAG_GRABBED_F)));
	}

	uint8_t b = neg_into_b(gb, wCurObjCollBoxTop);
	set_hpos_sub(gb, wCurObjYPos, b);
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_358b, (uint16_t)(routine + 0x50));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	bool collided = gb->cpu.a != 0;
	if (!collided) {
		set_hpos_sub(gb, wCurObjYPos, 8);
		hl = REG_HL(gb);
		if (left) {
			b = neg_into_b(gb, wCurObjCollBoxLeft);
			gb->cpu.a = alu_sub(gb, rd8(gb, hl++), b);
			wr8(gb, hXPosLo, gb->cpu.a);
			gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
			wr8(gb, hXPosHi, gb->cpu.a);
		} else {
			gb->cpu.a = alu_add(gb, rd8(gb, wCurObjCollBoxRight), rd8(gb, hl));
			wr8(gb, hXPosLo, gb->cpu.a);
			hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
			gb->cpu.a = alu_adc(gb, rd8(gb, hl), 0);
			wr8(gb, hXPosHi, gb->cpu.a);
		}
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + (left ? 0x73 : 0x70)));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
		collided = gb->cpu.a != 0;
	}
	if (!collided) {
		SET_HL(gb, wCurObjStateDuration);
		uint8_t d = alu_dec(gb, rd8(gb, wCurObjStateDuration));
		wr8(gb, wCurObjStateDuration, d);
		if (d) {
			JUMP(HomeJumpRet);
			return;
		}
	}
	// stops when it lands or hits something, the other way after a hit
	gb->cpu.a = (collided == left) ? OBJSTATE_27 : OBJSTATE_26;
	wr8(gb, wCurObjState, gb->cpu.a);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) & ~(1 << OBJFLAG_NO_COLLISION_F)));
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_62ec3) { thrown_object_movement(gb, true); }
PORT_JUMP(Func_62faa) { thrown_object_movement(gb, false); }

// output: e = number of objects with OBJFLAG_ACTIVE and OBJFLAG_TRANSIENT
PORT(_CountActiveTransientObjects)
{
	gb->cpu.e = 0;
	gb->cpu.d = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	for (int n = 0; n < NUM_OBJECTS; n++) {
		gb->cpu.a = alu_and(gb, rd8(gb, wObj1Flags + n * OBJ_STRUCT_SIZE), gb->cpu.d);
		alu_cp(gb, gb->cpu.a, gb->cpu.d);
		if (gb->cpu.f & FLAG_Z)
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
	}
}

// Moves the current object b pixels right (or left), and along the arc in
// Data_60180 (Data_601a0 if heavy), indexed by OBJ_VAR_3.
static void move_obj_in_arc(GB *gb, bool right)
{
	uint16_t hl = wCurObjXPos;
	uint8_t x = rd8(gb, hl);
	gb->cpu.a = right ? alu_add(gb, x, gb->cpu.b) : alu_sub(gb, x, gb->cpu.b);
	wr8(gb, hl++, gb->cpu.a);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, right ? alu_inc(gb, rd8(gb, hl)) : alu_dec(gb, rd8(gb, hl)));

	hl = wCurObjVar3;
	gb->cpu.b = (uint8_t)(ADDR_Data_60180 >> 8);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), (uint8_t)ADDR_Data_60180);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	if (gb->cpu.f & FLAG_C) { // heavy
		gb->cpu.a = alu_add(gb, gb->cpu.c, (uint8_t)(ADDR_Data_601a0 - ADDR_Data_60180));
		gb->cpu.c = gb->cpu.a;
	}
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}

	wr8(gb, hffa0, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_Y_POS);
	alu_cp(gb, gb->cpu.a, 0x80);
	bool negative = !(gb->cpu.f & FLAG_C);
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	if (negative && !(gb->cpu.f & FLAG_C))
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	else if (!negative && (gb->cpu.f & FLAG_C))
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	SET_HL(gb, hl);

	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_6284f) { move_obj_in_arc(gb, true); }
PORT_JUMP(Func_628a7) { move_obj_in_arc(gb, false); }

PORT_JUMP(Func_628ea)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3543, ADDR_Func_628ea + 0x12);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = OBJSTATE_12;
		wr8(gb, wCurObjState, OBJSTATE_12);
	}
	JUMP(HomeJumpRet);
}
