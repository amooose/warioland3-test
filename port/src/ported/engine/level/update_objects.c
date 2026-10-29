// Port of src/engine/level/update_objects.asm
#include "wario_states.h"

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

// --- thrown object state entries ---

// ld hl, wCurObjState / ld a, state / ld [hld], a / dec l / xor a /
// ld [hld], a (OBJ_VAR_3) / ld a, [wCurObjInteractionType] / rla /
// ld a, light / jr nc / ld a, heavy / ld [wCurObjStateDuration], a
static void set_thrown_state(GB *gb, uint8_t state, uint8_t light, uint8_t heavy)
{
	uint16_t hl = wCurObjState;
	wr8(gb, hl--, state);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
	xor_a(gb);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	gb->cpu.a = CF ? heavy : light;
	wr8(gb, wCurObjStateDuration, gb->cpu.a);
}

static void thrown_left(GB *gb, uint16_t data)
{
	SET_BC(gb, data);
	JUMP_TO(ADDR_Func_62ec3);
}

static void thrown_right(GB *gb, uint16_t data)
{
	SET_BC(gb, data);
	JUMP_TO(ADDR_Func_62faa);
}

PORT_JUMP(Func_62e74) { set_thrown_state(gb, OBJSTATE_54, 0x06, 0x02); thrown_left(gb, ADDR_Data_60300); }
PORT_JUMP(Func_62e8a) { thrown_left(gb, ADDR_Data_60300); }
PORT_JUMP(Func_62e8f) { set_thrown_state(gb, OBJSTATE_50, 0x04, 0x01); thrown_left(gb, ADDR_Data_602c0); }
PORT_JUMP(Func_62ea5) { thrown_left(gb, ADDR_Data_602c0); }
PORT_JUMP(Func_62eaa) { set_thrown_state(gb, OBJSTATE_52, 0x05, 0x02); thrown_left(gb, ADDR_Data_602e0); }
PORT_JUMP(Func_62ec0) { thrown_left(gb, ADDR_Data_602e0); }
PORT_JUMP(Func_62f5b) { set_thrown_state(gb, OBJSTATE_55, 0x06, 0x02); thrown_right(gb, ADDR_Data_60300); }
PORT_JUMP(Func_62f71) { thrown_right(gb, ADDR_Data_60300); }
PORT_JUMP(Func_62f76) { set_thrown_state(gb, OBJSTATE_51, 0x04, 0x01); thrown_right(gb, ADDR_Data_602c0); }
PORT_JUMP(Func_62f8c) { thrown_right(gb, ADDR_Data_602c0); }
PORT_JUMP(Func_62f91) { set_thrown_state(gb, OBJSTATE_53, 0x05, 0x02); thrown_right(gb, ADDR_Data_602e0); }
PORT_JUMP(Func_62fa7) { thrown_right(gb, ADDR_Data_602e0); }

// --- arcs ---

// ld a, [wCurObjVar2] / ld b, a; *4 if the power up is at least GARLIC
static void var2_into_b(GB *gb, bool garlic)
{
	gb->cpu.a = rd8(gb, wCurObjVar2);
	gb->cpu.b = gb->cpu.a;
	if (!garlic)
		return;
	ld_cp(gb, wEffectivePowerUp, GARLIC);
	if (CF)
		return;
	gb->cpu.b = alu_sla(gb, gb->cpu.b);
	gb->cpu.b = alu_sla(gb, gb->cpu.b);
}

PORT_JUMP(Func_6283a) { var2_into_b(gb, false); JUMP_TO(ADDR_Func_6284f); }
PORT_JUMP(Func_62840) { var2_into_b(gb, true); JUMP_TO(ADDR_Func_6284f); }
PORT_JUMP(Func_62892) { var2_into_b(gb, false); JUMP_TO(ADDR_Func_628a7); }
PORT_JUMP(Func_62898) { var2_into_b(gb, true); JUMP_TO(ADDR_Func_628a7); }

PORT_JUMP(Func_62908)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3543, ADDR_Func_62908 + 0x12);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		st(gb, wCurObjState, OBJSTATE_STANDING_FALL_START);
	JUMP(HomeJumpRet);
}

// --- y velocity tables ---

// hl = wCurObjVar3, bc = light + OBJ_VAR_3 (or the heavy table), a = [bc],
// the last entry ($80 ends the table) repeats, else OBJ_VAR_3 goes up.
static uint8_t obj_y_vel(GB *gb, uint16_t light, uint16_t heavy)
{
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.b = (uint8_t)(light >> 8);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), (uint8_t)light);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	if (CF) {
		gb->cpu.a = alu_add(gb, gb->cpu.c, (uint8_t)(heavy - light));
		gb->cpu.c = gb->cpu.a;
	}
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (ZF) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	return gb->cpu.a;
}

enum { ADD_SIGNED, ADD_POSITIVE, ADD_NEGATIVE };

// ld l, OBJ_Y_POS / (cp $80) / ld c, [hl] / add c / ld [hli], a and the
// carry into the high byte for a positive / negative a
static void add_obj_y(GB *gb, int mode)
{
	uint16_t hl = (uint16_t)((REG_HL(gb) & 0xff00) | OBJ_Y_POS);
	bool negative = mode == ADD_NEGATIVE;
	if (mode == ADD_SIGNED) {
		alu_cp(gb, gb->cpu.a, 0x80);
		negative = !CF;
	}
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (negative && !CF)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	else if (!negative && CF)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

// hPos = wCurObjYPos..; hl = wCurObjYPos + 3
static void obj_pos_to_hpos(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// The end of a fall: checks the floor (Func_352b, returning to ret) and
// lands in the state land_state, or on a slope, or vanishes in water/sand.
static void obj_fall_and_land(GB *gb, uint16_t ret, uint8_t land_state)
{
	obj_pos_to_hpos(gb);
	xor_a(gb);
	wr8(gb, wObjSlopeInteraction, 0);
	gb_call_ret(gb, ADDR_Func_352b, ret);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (ld_and(gb, wIsInWaterOrSand))
			st(gb, wCurObjState, OBJSTATE_VANISH_TOUCH);
		JUMP(HomeJumpRet);
		return;
	}
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_STATE);
	SET_HL(gb, hl);
	if (!ld_and(gb, wObjSlopeInteraction)) {
		gb->cpu.a = land_state;
		wr8(gb, hl, land_state);
		JUMP(HomeJumpRet);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	alu_cp(gb, gb->cpu.a, 0x01);
	st(gb, wCurObjVar2, 0x02);
	JUMP_TO(ZF ? ADDR_Func_62d57 : ADDR_Func_62e31);
}

PORT_JUMP(Func_62926)
{
	gb->cpu.a = obj_y_vel(gb, ADDR_Data_60040, ADDR_Data_60060);
	wr8(gb, hffa0, gb->cpu.a);
	add_obj_y(gb, ADD_SIGNED);
	obj_fall_and_land(gb, ADDR_Func_62926 + 0x48, OBJSTATE_0F);
}

PORT_JUMP(Func_629a6)
{
	gb->cpu.a = obj_y_vel(gb, ADDR_Data_602a0, ADDR_Data_602b0);
	wr8(gb, hffa0, gb->cpu.a);
	add_obj_y(gb, ADD_NEGATIVE);
	JUMP(HomeJumpRet);
}

PORT_JUMP(_ObjState_StandingFall)
{
	// speed up animation by 3x
	SET_HL(gb, wCurObjFrameDuration);
	gb->cpu.a = rd8(gb, wCurObjFrameDuration);
	alu_cp(gb, gb->cpu.a, 3);
	if (!CF) {
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 3);
		wr8(gb, wCurObjFrameDuration, gb->cpu.a);
	}
	gb->cpu.a = obj_y_vel(gb, ADDR_StandingFallYVel_Light, ADDR_StandingFallYVel_Heavy);
	wr8(gb, hffa0, gb->cpu.a);
	add_obj_y(gb, ADD_SIGNED);
	obj_fall_and_land(gb, ADDR__ObjState_StandingFall + 0x53, OBJSTATE_TURN_AROUND_START);
}

PORT_JUMP(_ObjState_Fall)
{
	gb->cpu.a = obj_y_vel(gb, ADDR_FallingYVel_Light, ADDR_FallingYVel_Heavy);
	wr8(gb, hffa0, gb->cpu.a);
	add_obj_y(gb, ADD_POSITIVE);
	obj_fall_and_land(gb, ADDR__ObjState_Fall + 0x3d, OBJSTATE_LAND);
}

PORT_JUMP(Func_627f0)
{
	uint16_t hl = (uint16_t)((REG_HL(gb) & 0xff00) | OBJ_VAR_3);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.b = alu_dec(gb, gb->cpu.b);
	if (ZF) {
		gb->cpu.b = 0x41;
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0x40);
	} else {
		gb->cpu.b = 0x40;
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0xc0);
	}
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	if (CF) {
		gb->cpu.a = alu_add(gb, gb->cpu.c, 0x20);
		gb->cpu.c = gb->cpu.a;
	}
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (ZF) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	wr8(gb, hffa0, gb->cpu.a);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(HomeJumpRet);
		return;
	}
	hl = (uint16_t)((hl & 0xff00) | OBJ_Y_POS);
	alu_cp(gb, gb->cpu.a, 0x80);
	bool negative = !CF;
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	bool carry = CF;
	gb->cpu.a = negative ? 0x00 : 0x01;
	wr8(gb, wCurObjStateDuration, gb->cpu.a);
	if (negative && !carry)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	else if (!negative && carry)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	JUMP(HomeJumpRet);
}

// --- misc states ---

PORT_JUMP(Func_6303f)
{
	uint16_t hl = wCurObjState;
	wr8(gb, hl--, OBJSTATE_1B);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
	xor_a(gb);
	wr8(gb, hl--, 0); // OBJ_VAR_3
	SET_HL(gb, hl);
	gb->cpu.a = 0x07;
	wr8(gb, wCurObjStateDuration, 0x07);
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_63050)
{
	SET_HL(gb, wCurObjStateDuration);
	uint8_t d = alu_dec(gb, rd8(gb, wCurObjStateDuration));
	wr8(gb, wCurObjStateDuration, d);
	if (d) {
		JUMP_TO(ADDR_Func_629a6);
		return;
	}
	st(gb, wCurObjState, OBJSTATE_STANDING_FALL_START);
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_6305f)
{
	gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
	gb->cpu.a = CF ? OBJSTATE_WOBBLE_LEFT : OBJSTATE_WOBBLE_RIGHT;
	uint16_t hl = wCurObjState;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
	xor_a(gb);
	wr8(gb, hl--, 0); // OBJ_VAR_3
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01); // OBJ_VAR_2
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l); // dec l (sets the exit flags)
	hl = REG_HL(gb);
	gb->cpu.a = 0x04;
	wr8(gb, hl++, 0x04); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
	JUMP(HomeJumpRet);
}

// --- bumping and knocked-back objects ---

// res OBJFLAG_GRABBED_F of the flags at the current page (ld l, OBJ_FLAGS)
static void res_grabbed(GB *gb)
{
	gb->cpu.l = OBJ_FLAGS;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << OBJFLAG_GRABBED_F)));
}

// ld a, [wCurObjScreenXPos] / add $28 / cp $f0 (left) or $10 (right), and
// the object is let go when it goes off screen
static void check_offscreen_grabbed(GB *gb, bool left)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x28);
	alu_cp(gb, gb->cpu.a, left ? 0xf0 : 0x10);
	if (left ? !CF : CF)
		res_grabbed(gb);
}

// hXPos = the object's left edge (wCurObjXPos - -box left), from hl = wCurObjXPos
static void hxpos_left_edge(GB *gb, uint16_t hl)
{
	uint8_t b = neg_into_b(gb, wCurObjCollBoxLeft);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), b);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// hXPos = the object's right edge (box right + wCurObjXPos), from hl = wCurObjXPos
static void hxpos_right_edge(GB *gb, uint16_t hl)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjCollBoxRight), rd8(gb, hl));
	wr8(gb, hXPosLo, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
	gb->cpu.a = alu_adc(gb, rd8(gb, hl), 0);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// hl = YPos / [hli] = hYPosLo / [hl] = hYPosHi
static void hypos_to_obj(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

// Func_62ae5 (moving left) / Func_62bce (moving right): an object that
// bumped into something slides back, slowing down
static void obj_bump_slide(GB *gb, bool left)
{
	uint16_t routine = left ? ADDR_Func_62ae5 : ADDR_Func_62bce;
	set_hpos_sub(gb, wCurObjYPos, 4);
	if (left)
		hxpos_left_edge(gb, REG_HL(gb));
	else
		hxpos_right_edge(gb, REG_HL(gb));
	gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + (left ? 0x1f : 0x1c)));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.a) {
		play_sfx(gb, SFX_01A);
		st(gb, wCurObjState, left ? OBJSTATE_STUN_RIGHT_START : OBJSTATE_STUN_LEFT_START);
	} else {
		if (left)
			CALL(MoveObjectLeftByVar2);
		else
			CALL(MoveObjectRightByVar2);
		check_offscreen_grabbed(gb, left);
		obj_pos_to_hpos(gb);
		xor_a(gb);
		wr8(gb, wObjSlopeInteraction, 0);
		gb_call_ret(gb, ADDR_Func_3543, (uint16_t)(routine + (left ? 0x4a : 0x47)));
		alu_cp(gb, gb->cpu.a, 0x10);
		uint8_t f = gb->cpu.f;
		hypos_to_obj(gb);
		gb->cpu.f = f;
		if (ZF) {
			res_grabbed(gb);
			gb->cpu.l = OBJ_STATE;
			uint16_t hl = REG_HL(gb);
			gb->cpu.a = alu_rra(gb, rd8(gb, wObjSlopeInteraction));
			if (left ? !CF : CF) {
				gb->cpu.a = left ? OBJSTATE_STUN_RIGHT_START : OBJSTATE_STUN_LEFT_START;
				wr8(gb, hl, gb->cpu.a);
			} else {
				gb->cpu.a = rd8(gb, hl);
				alu_cp(gb, gb->cpu.a, left ? OBJSTATE_STUN_LEFT : OBJSTATE_STUN_RIGHT);
				if (!ZF) {
					gb->cpu.a = left ? OBJSTATE_14 : OBJSTATE_15;
					wr8(gb, hl, gb->cpu.a);
				}
			}
			JUMP(HomeJumpRet);
			return;
		}
		bool hit = false;
		for (int side = 0; side < 2 && !hit; side++) {
			// left: the left edge then the right edge; right: the other way
			if ((side == 0) == left)
				hxpos_left_edge(gb, wCurObjXPos);
			else
				hxpos_right_edge(gb, wCurObjXPos);
			gb_call_ret(gb, ADDR_Func_35a3, (uint16_t)(routine +
				(left ? (side ? 0x81 : 0x6c) : (side ? 0x7e : 0x66))));
			hit = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
		}
		if (!hit)
			st(gb, wCurObjState, left ? OBJSTATE_14 : OBJSTATE_15);
	}
	SET_HL(gb, wCurObjVar2);
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	bool heavy = CF;
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), heavy ? 0x3 : 0x7);
	if (!gb->cpu.a)
		wr8(gb, wCurObjVar2, alu_dec(gb, rd8(gb, wCurObjVar2)));
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_62ae5) { obj_bump_slide(gb, true); }
PORT_JUMP(Func_62bce) { obj_bump_slide(gb, false); }

static void obj_bump(GB *gb, uint16_t slide)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(slide);
		return;
	}
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 2)); // dec l / dec l
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl)); // OBJ_STATE_DURATION
	wr8(gb, hl, d);
	if (!d) {
		gb->cpu.a = OBJSTATE_TURN_AROUND_START;
		wr8(gb, wCurObjState, gb->cpu.a);
	}
	JUMP(HomeJumpRet);
}

PORT_JUMP(_ObjState_BumpRight) { obj_bump(gb, ADDR_Func_62ae5); }
PORT_JUMP(_ObjState_BumpLeft) { obj_bump(gb, ADDR_Func_62bce); }

// Func_62ca8 (to the left) / Func_62d86 (to the right): an object knocked
// back in the air
static void obj_knocked_fall(GB *gb, bool left)
{
	uint16_t routine = left ? ADDR_Func_62ca8 : ADDR_Func_62d86;
	if (left)
		CALL(MoveObjectLeftByVar2);
	else
		CALL(MoveObjectRightByVar2);
	check_offscreen_grabbed(gb, left);
	gb->cpu.l = OBJ_VAR_3;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.b = (uint8_t)(ADDR_FallingYVel_Light >> 8);
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)ADDR_FallingYVel_Light);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, rd8(gb, wCurObjInteractionType));
	if (CF) {
		gb->cpu.a = alu_add(gb, gb->cpu.c, (uint8_t)(ADDR_FallingYVel_Heavy - ADDR_FallingYVel_Light));
		gb->cpu.c = gb->cpu.a;
	}
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (ZF) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	wr8(gb, hffa0, gb->cpu.a);
	add_obj_y(gb, ADD_POSITIVE);
	obj_pos_to_hpos(gb);
	xor_a(gb);
	wr8(gb, wObjSlopeInteraction, 0);
	wr8(gb, wc1ca, 0);
	gb_call_ret(gb, ADDR_Func_352b, (uint16_t)(routine + 0x4f));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(left ? ADDR_Func_62d34 : ADDR_Func_62e0f);
		return;
	}
	if (ld_and(gb, wIsInWaterOrSand)) {
		st(gb, wCurObjState, OBJSTATE_VANISH_TOUCH);
		JUMP(HomeJumpRet);
		return;
	}
	set_hpos_sub(gb, wCurObjYPos, 4);
	if (left)
		hxpos_left_edge(gb, REG_HL(gb));
	else
		hxpos_right_edge(gb, REG_HL(gb));
	gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + (left ? 0x77 : 0x74)));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.a)
		st(gb, wCurObjState, left ? OBJSTATE_15 : OBJSTATE_14);
	JUMP(HomeJumpRet);
}

PORT_JUMP(Func_62ca8) { obj_knocked_fall(gb, true); }
PORT_JUMP(Func_62d86) { obj_knocked_fall(gb, false); }

// ld hl, wCurObjState / ld a, state / ld [hld], a / dec l / xor a / ld [hld], a
static void set_state_clear_var3(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
	xor_a(gb);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
}

PORT_JUMP(Func_62c9f) { set_state_clear_var3(gb, OBJSTATE_56); obj_knocked_fall(gb, true); }
PORT_JUMP(Func_62d7d) { set_state_clear_var3(gb, OBJSTATE_57); obj_knocked_fall(gb, false); }

// --- vanishing ---

static void vanish_object(GB *gb)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	wr8(gb, hl--, (uint8_t)(ADDR_StarsFunc >> 8));
	gb->cpu.a = (uint8_t)ADDR_StarsFunc;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// The object turns into stars (StarsFunc)
PORT(Func_618b4)
{
	// (BANK(StarsFunc) - BANK("Object Functions 1")) << 4
	st(gb, wCurObjId, 0x30);
	uint16_t hl = wCurObjOAMPtr;
	wr8(gb, hl++, (uint8_t)ADDR_OAM_18c000);
	wr8(gb, hl++, (uint8_t)(ADDR_OAM_18c000 >> 8));
	wr8(gb, hl++, (uint8_t)ADDR_Frameset_680c5);
	gb->cpu.a = (uint8_t)(ADDR_Frameset_680c5 >> 8);
	wr8(gb, hl++, gb->cpu.a);
	xor_a(gb);
	wr8(gb, hl++, 0); // OBJ_FRAME_DURATION
	wr8(gb, hl++, 0); // OBJ_FRAMESET_OFFSET
	gb->cpu.a = 33;
	wr8(gb, hl++, 33); // OBJ_STATE_DURATION

	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	uint8_t flags = rd8(gb, hl);
	flags &= (uint8_t)~(1 << OBJFLAG_GRABBED_F);
	wr8(gb, hl, flags);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << OBJFLAG_NO_COLLISION_F)));
	st(gb, wCurObjAction, (1 << NO_ACTIONS_F) | 1); // no_actions_for 1

	hl = (uint16_t)((hl & 0xff00) | OBJ_Y_POS);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), 8);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (CF)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
}

PORT_JUMP(VanishObject_NoSFX)
{
	vanish_object(gb);
	JUMP_TO(ADDR_Func_618b4);
}

PORT_JUMP(_VanishObject)
{
	play_sfx(gb, SFX_VANISH);
	vanish_object(gb);
	JUMP_TO(ADDR_Func_618b4);
}

// --- init ---

static void init_object_vars(GB *gb, bool level_start)
{
	xor_a(gb);
	gb_call(gb, ADDR_ClearObjects);
	gb_call(gb, ADDR_ClearObjDataPointers);
	if (level_start)
		wr8(gb, wFlameBlockCleared, gb->cpu.a);
	wr8(gb, wDollBoyNoFlashingBarrel, gb->cpu.a);
	wr8(gb, wDollBoyRemainingParts, gb->cpu.a);
	wr8(gb, wNumLitTorches, gb->cpu.a);
	wr8(gb, wShootGoals, gb->cpu.a);
	wr8(gb, wWarioGoals, gb->cpu.a);
}

PORT(InitObjectVars_FromLevelStart) { init_object_vars(gb, true); }
PORT(InitObjectVars_FromTransition) { init_object_vars(gb, false); }

// --- movement tables ---

// The signed add of a to the 16-bit position at hl (lo, hi): cp $80 /
// ld c, [hl] / add c / ld [hli], a / then inc or dec the high byte on a
// carry / no carry. hl ends at the high byte.
static void add_signed_at(GB *gb, uint16_t hl)
{
	alu_cp(gb, gb->cpu.a, 0x80);
	bool negative = !CF;
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (negative && !CF)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	else if (!negative && CF)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

// hl = index, a = [bc + [hl]]; at the $80 end marker the last entry
// repeats (loop: the table restarts), and the index goes up
static void movement_lookup(GB *gb, uint16_t index, bool loop)
{
	uint8_t start = gb->cpu.c;
	if (loop)
		gb->cpu.e = start;
	SET_HL(gb, index);
	gb->cpu.a = alu_add(gb, rd8(gb, index), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (ZF) {
		if (loop) {
			xor_a(gb);
			wr8(gb, index, 0);
			gb->cpu.c = gb->cpu.e;
		} else {
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
		}
		gb->cpu.a = rd8(gb, REG_BC(gb));
		if (!loop)
			return;
	}
	wr8(gb, index, alu_inc(gb, rd8(gb, index)));
}

static void apply_obj_movement(GB *gb, uint16_t index, uint8_t pos, bool loop)
{
	movement_lookup(gb, index, loop);
	add_signed_at(gb, (uint16_t)((index & 0xff00) | pos));
}

PORT(_ApplyObjYMovement) { apply_obj_movement(gb, wCurObjVar3, OBJ_Y_POS, false); }
PORT(_ApplyObjXMovement) { apply_obj_movement(gb, wCurObjVar2, OBJ_X_POS, false); }
PORT(_ApplyObjYMovement_Loop) { apply_obj_movement(gb, wCurObjVar3, OBJ_Y_POS, true); }
PORT(_ApplyObjXMovement_Loop) { apply_obj_movement(gb, wCurObjVar2, OBJ_X_POS, true); }

PORT(Func_63209)
{
	movement_lookup(gb, wCurObjVar3, false);
	gb->cpu.b = gb->cpu.a;
	add_signed_at(gb, (uint16_t)((wCurObjVar3 & 0xff00) | OBJ_Y_POS));
	gb->cpu.l = OBJ_FLAGS;
	uint16_t hl = REG_HL(gb);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F))
		return;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << OBJFLAG_STEPPED_F)));
	CALL(MoveWarioDown);
	FARCALL(Func_19720);
}

PORT(_ApplyObjSetMovement)
{
	uint16_t hl = wCurObjVar1;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	uint8_t l = alu_inc(gb, (uint8_t)hl); // inc l
	hl = (uint16_t)((hl & 0xff00) | l);
	gb->cpu.a = rd8(gb, hl++); // var2
	hl = (uint16_t)((rd8(gb, hl) << 8) | gb->cpu.a); // var3
	gb->cpu.a = rd8(gb, hl++);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// loop
		hl = REG_DE(gb);
		gb->cpu.a = rd8(gb, hl++);
	}
	wr8(gb, wCurObjVar1, gb->cpu.a); // duration
	gb->cpu.c = rd8(gb, hl++);
	gb->cpu.b = rd8(gb, hl++);
	gb->cpu.a = (uint8_t)hl;
	wr8(gb, wCurObjVar2, gb->cpu.a);
	gb->cpu.a = (uint8_t)(hl >> 8);
	wr8(gb, wCurObjVar3, gb->cpu.a);

	// y += c, x += b (signed)
	hl = wCurObjYPos;
	for (int i = 0; i < 2; i++) {
		uint8_t v = i ? gb->cpu.b : gb->cpu.c;
		if (i) {
			l = alu_inc(gb, (uint8_t)hl); // inc l
			hl = (uint16_t)((hl & 0xff00) | l);
		}
		alu_cp(gb, v, 0x80);
		bool negative = !CF;
		gb->cpu.a = alu_add(gb, rd8(gb, hl), v);
		wr8(gb, hl++, gb->cpu.a);
		if (negative && !CF)
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		else if (!negative && CF)
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	SET_HL(gb, hl);
}

// e = the number of active transient objects with the same OBJ_UNK_01/02
// as the current object
PORT(Func_632ac)
{
	gb->cpu.c = rd8(gb, wCurObjUnk01);
	gb->cpu.b = rd8(gb, wCurObjUnk01 + 1);
	gb->cpu.e = 0x00;
	gb->cpu.d = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	uint16_t hl = 0;
	for (int n = 0; n < NUM_OBJECTS; n++) {
		hl = (uint16_t)((wObj1 & 0xff00) | (n * 0x20));
		gb->cpu.a = alu_and(gb, rd8(gb, hl++), gb->cpu.d);
		alu_cp(gb, gb->cpu.a, gb->cpu.d);
		if (!ZF)
			continue;
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (!ZF)
			continue;
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (!ZF)
			continue;
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
	}
	SET_HL(gb, hl);
}

// --- thrown objects hitting other objects ---

#define OBJ_N(addr, n) ((uint16_t)((addr) + (n) * OBJ_STRUCT_SIZE))

// [hl] = a for a in states, with hli
static void write_states(GB *gb, uint16_t hl, const uint8_t *states, int n)
{
	for (int i = 0; i < n; i++) {
		gb->cpu.a = states[i];
		wr8(gb, hl++, states[i]);
	}
	SET_HL(gb, hl);
}

// a = [hl] & $fe / cp OBJSTATE_PICKED_UP_LEFT / jr z / cp OBJSTATE_48 /
// jr nz, .skip / xor a / ld [wGrabState], a
static void reset_grab_if_held(GB *gb, uint16_t state)
{
	gb->cpu.a = alu_and(gb, rd8(gb, state), 0xfe);
	alu_cp(gb, gb->cpu.a, OBJSTATE_PICKED_UP_LEFT);
	if (!ZF) {
		alu_cp(gb, gb->cpu.a, OBJSTATE_48);
		if (!ZF)
			return;
	}
	xor_a(gb);
	wr8(gb, wGrabState, 0);
}

PORT(HandleGrabbedObjectCollisions)
{
	uint16_t hl = wObj1Flags;
	gb->cpu.e = OBJFLAG_ACTIVE | OBJFLAG_GRABBED;
	int n;
	for (n = 0; n < NUM_OBJECTS; n++) {
		hl = OBJ_N(wObj1Flags, n);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), gb->cpu.e);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (ZF)
			break;
	}
	SET_HL(gb, hl);
	if (n == NUM_OBJECTS)
		return;

	gb_push16(gb, hl);
	hl = (uint16_t)((hl & 0xff00) | alu_add(gb, (uint8_t)hl, OBJ_INTERACTION_TYPE - OBJ_FLAGS));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hThrownObjInteractionType, gb->cpu.a);
	gb->cpu.b = rd8(gb, hl++);
	gb->cpu.c = rd8(gb, hl++);
	gb->cpu.d = rd8(gb, hl++);
	gb->cpu.e = rd8(gb, hl++);
	gb->cpu.a = alu_add(gb, alu_add(gb, rd8(gb, hl), 0x2a), gb->cpu.b);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, alu_add(gb, rd8(gb, hl++), 0x2a), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_add(gb, alu_add(gb, rd8(gb, hl), 0x2a), gb->cpu.d);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_add(gb, alu_add(gb, rd8(gb, hl++), 0x2a), gb->cpu.e);
	gb->cpu.e = gb->cpu.a;

	hl = (uint16_t)((hl & 0xff00) | alu_add(gb, (uint8_t)hl, OBJ_STATE - (OBJ_SCREEN_X_POS + 1)));
	gb->cpu.a = rd8(gb, hl); // OBJ_STATE
	bool full_throw = true;
	alu_cp(gb, gb->cpu.a, OBJSTATE_FULL_THROW_LEFT_START);
	if (CF) {
		alu_cp(gb, gb->cpu.a, OBJSTATE_48);
		if (!CF)
			full_throw = false;
	}
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	bool right = CF;
	if (full_throw) {
		gb->cpu.a = rd8(gb, hThrownObjInteractionType);
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		bool heavy = CF;
		static const uint8_t states[4][4] = {
			{OBJSTATE_28, OBJSTATE_29, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_29},
			{OBJSTATE_28, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_28, OBJSTATE_29},
			{OBJSTATE_29, OBJSTATE_28, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_28},
			{OBJSTATE_29, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_29, OBJSTATE_28},
		};
		write_states(gb, hThrownObjInteractionType + 1, states[right * 2 + heavy], 4);
	} else {
		// OBJSTATE_48 <= state <= OBJSTATE_4B
		static const uint8_t states[2][2] = {
			{OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_WOBBLE_LEFT_START},
			{OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_WOBBLE_RIGHT_START},
		};
		write_states(gb, hCollidedObjState_Light, states[right], 2);
	}

	for (n = 0; n < NUM_OBJECTS; n++) {
		gb->cpu.a = alu_and(gb, rd8(gb, OBJ_N(wObj1Flags, n)),
			OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_GRABBED | OBJFLAG_NO_COLLISION | OBJFLAG_INVISIBLE);
		alu_cp(gb, gb->cpu.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
		if (!ZF)
			continue;
		gb->cpu.a = alu_add(gb, rd8(gb, OBJ_N(wObj1ScreenYPos, n)), 0x2a);
		gb->cpu.h = gb->cpu.a;
		gb->cpu.a = alu_add(gb, rd8(gb, OBJ_N(wObj1ScreenXPos, n)), 0x2a);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, OBJ_N(wObj1CollBoxBottom, n)), gb->cpu.h), gb->cpu.b);
		if (CF)
			continue;
		gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, OBJ_N(wObj1CollBoxTop, n)), gb->cpu.h), gb->cpu.c);
		if (!CF)
			continue;
		gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, OBJ_N(wObj1CollBoxRight, n)), gb->cpu.l), gb->cpu.d);
		if (CF)
			continue;
		gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, OBJ_N(wObj1CollBoxLeft, n)), gb->cpu.l), gb->cpu.e);
		if (!CF)
			continue;

		// do collision
		hl = gb_pop16(gb);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << OBJFLAG_GRABBED_F)));
		hl = (uint16_t)((hl & 0xff00) | alu_add(gb, (uint8_t)hl, OBJ_STATE - OBJ_FLAGS));
		SET_HL(gb, hl);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xfe);
		alu_cp(gb, gb->cpu.a, OBJSTATE_4A);
		if (!ZF)
			alu_cp(gb, gb->cpu.a, OBJSTATE_48);
		uint16_t other_state = (uint16_t)((hl & 0xff00) | (uint8_t)OBJ_N(wObj1State, n));
		uint16_t collided;
		if (ZF) {
			// .CollideWithObject
			xor_a(gb);
			wr8(gb, wGrabState, 0);
			gb->cpu.a = rd8(gb, hThrownObjState_Light);
			wr8(gb, hl, gb->cpu.a);
			collided = hCollidedObjState_Light;
		} else {
			gb->cpu.a = alu_rla(gb, rd8(gb, OBJ_N(wObj1InteractionType, n)));
			bool heavy = CF;
			gb->cpu.a = rd8(gb, heavy ? hThrownObjState_Heavy : hThrownObjState_Light);
			wr8(gb, hl, gb->cpu.a);
			reset_grab_if_held(gb, other_state);
			collided = heavy ? hCollidedObjState_Heavy : hCollidedObjState_Light;
		}
		gb->cpu.a = rd8(gb, collided);
		wr8(gb, other_state, gb->cpu.a);
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)OBJ_N(wObj1Flags, n));
		SET_HL(gb, hl);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << OBJFLAG_NO_COLLISION_F)));
		play_sfx(gb, SFX_016);
		return;
	}
	hl = gb_pop16(gb);
	SET_HL(gb, hl);
}

// --- DoObjectAction ---

// ld [hld], a (OBJ_ACTION = action) / ld a, state / ld [hl], a (OBJ_STATE)
static void set_action_state(GB *gb, uint8_t action, uint8_t state)
{
	gb->cpu.a = action;
	wr8(gb, wCurObjAction, action);
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
	SET_HL(gb, wCurObjState);
}

// ld hl, wCurObjAction / ld a, action / ld [hld], a / ld a, state / ld [hl], a
static void action_hl_state(GB *gb, uint8_t action, uint8_t state)
{
	set_action_state(gb, action, state);
}

static void obj_flags_res_grabbed(GB *gb, bool set_no_collision)
{
	uint8_t v = (uint8_t)(rd8(gb, wCurObjFlags) & ~(1 << OBJFLAG_GRABBED_F));
	wr8(gb, wCurObjFlags, v);
	if (set_no_collision)
		wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | (1 << OBJFLAG_NO_COLLISION_F)));
}

// ld a, [hld] (OBJ_UNK_1D) / and INTERACTION_RIGHT / ld a, no_actions_for 15 /
// ld [hld], a / jr z, .right / left or right state
static void action_by_interaction(GB *gb, uint8_t left, uint8_t right)
{
	alu_and(gb, rd8(gb, wCurObjUnk1d), INTERACTION_RIGHT);
	set_action_state(gb, 15 | (1 << 7), ZF ? right : left);
}

static void action_wobble(GB *gb)
{
	obj_flags_res_grabbed(gb, false);
	action_by_interaction(gb, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_WOBBLE_RIGHT_START);
}

// .Action06 / .Action12
static void action_06(GB *gb, uint8_t b)
{
	gb->cpu.b = b;
	gb->cpu.a = rd8(gb, wCurObjUnk1d);
	wr8(gb, wCurObjAction, gb->cpu.b);
	gb->cpu.l = alu_dec(gb, (uint8_t)wCurObjAction);
	gb->cpu.h = (uint8_t)(wCurObjAction >> 8);
	static const uint8_t states[4] = {OBJSTATE_0E, OBJSTATE_0D, OBJSTATE_0B, OBJSTATE_0C};
	int i;
	for (i = 0; i < 3; i++) {
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		if (CF)
			break;
	}
	gb->cpu.a = states[i];
	wr8(gb, REG_HL(gb), gb->cpu.a);
}

PORT_JUMP(DoObjectAction)
{
	gb->cpu.a = rd8(gb, wCurObjAction);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (ZF) {
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, NO_ACTIONS_F)) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 1 << NO_ACTIONS_F); // no_actions_for 0
		if (ZF)
			xor_a(gb);
		wr8(gb, wCurObjAction, gb->cpu.a);
		RET();
		return;
	}

	// .jump: rst JumpTable
	uint8_t idx = gb->cpu.a;
	uint16_t table = ADDR_DoObjectAction + 0x14;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, table, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	uint16_t target = REG_HL(gb);
	if (idx >= NUM_OBJ_ACTIONS) {
		JUMP_TO(target);
		return;
	}

	switch (idx) {
	case OBJACTION_NONE:
		break;
	case OBJACTION_BUMP:
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), INTERACTION_MASK);
		if (ZF) {
			action_wobble(gb);
			break;
		}
		action_by_interaction(gb, OBJSTATE_BUMP_LEFT_START, OBJSTATE_BUMP_RIGHT_START);
		break;
	case OBJACTION_ATTACK:
		obj_flags_res_grabbed(gb, true);
		action_by_interaction(gb, OBJSTATE_ATTACKED_LEFT_START, OBJSTATE_ATTACKED_RIGHT_START);
		break;
	case OBJACTION_PICK_UP:
		obj_flags_res_grabbed(gb, false);
		gb->cpu.a = rd8(gb, wDirection);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		set_action_state(gb, 15 | (1 << 7),
			ZF ? OBJSTATE_PICKED_UP_RIGHT_START : OBJSTATE_PICKED_UP_LEFT_START);
		break;
	case OBJACTION_WOBBLE:
		action_wobble(gb);
		break;
	case OBJACTION_VANISH_TOUCH:
		obj_flags_res_grabbed(gb, true);
		set_action_state(gb, 15 | (1 << 7), OBJSTATE_VANISH_TOUCH);
		break;
	case OBJACTION_06:
		action_06(gb, 15 | (1 << 7));
		break;
	case OBJACTION_07:
		xor_a(gb);
		set_action_state(gb, 0, OBJSTATE_18);
		break;
	case OBJACTION_VANISH_TREASURE:
		JUMP_TO(ADDR_VanishObject_NoSFX);
		return;
	case OBJACTION_STEP_ON:
		xor_a(gb);
		wr8(gb, wCurObjAction, 0);
		SET_HL(gb, wCurObjFlags);
		wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | (1 << OBJFLAG_STEPPED_F)));
		break;
	case OBJACTION_0A:
		gb->cpu.a = 15 | (1 << 7);
		wr8(gb, wCurObjAction, gb->cpu.a);
		break;
	case OBJACTION_FLATTEN:
	case OBJACTION_0C:
	case OBJACTION_0E:
	case OBJACTION_TELEPORT:
	case OBJACTION_11:
		action_hl_state(gb, 15 | (1 << 7), OBJSTATE_ACTIVATE);
		break;
	case OBJACTION_0D:
		action_hl_state(gb, 15 | (1 << 7), OBJSTATE_19);
		break;
	case OBJACTION_SMASH_ATTACK_WALKABLE:
		action_hl_state(gb, 2 | (1 << 7), OBJSTATE_VANISH_TOUCH);
		break;
	case OBJACTION_12:
		action_06(gb, 7 | (1 << 7));
		break;
	case OBJACTION_13:
		action_hl_state(gb, 15 | (1 << 7), OBJSTATE_SPECIAL_3);
		break;
	}
	RET();
}

PORT_JUMP(Func_62e57)
{
	play_sfx(gb, SFX_VANISH);
	wr8(gb, wCurObjUpdateFunction + 1, (uint8_t)(ADDR_Func_4c860 >> 8));
	gb->cpu.a = (uint8_t)ADDR_Func_4c860;
	wr8(gb, wCurObjUpdateFunction + 0, gb->cpu.a);
	SET_HL(gb, wCurObjUpdateFunction - 1);
	gb_call(gb, ADDR_Func_618b4);
	JUMP(HomeJumpRet);
}

PORT_JUMP(_VanishObject2)
{
	gb_call(gb, ADDR__VanishObject);
	JUMP(HomeJumpRet);
}
