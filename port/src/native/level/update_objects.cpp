// The object engine (src/engine/level/update_objects.asm): drawing the
// objects, loading and unloading them off screen, the shared object states
// (falling, thrown, bumped, knocked, grabbed, vanishing), object movement
// tables and their actions.
//
// Objects (C++) and register-level C call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
#include "asm_module.hpp"

namespace wl3::level {

namespace {

class ObjectEngine : public AsmModule {
public:
	using AsmModule::AsmModule;

	#define OBJ_STRUCT_SIZE (wObj2Flags - wObj1Flags)

	// Draws every active, on-screen, visible object whose flags (masked as
	// below) match.
	void draw_objects(uint8_t match)
	{
		for (int n = 0; n < NUM_OBJECTS; n++) {
			uint16_t obj = (uint16_t)(wObj1Flags + n * OBJ_STRUCT_SIZE);
			SET_HL(gb_, obj);
			cpu_.a = alu_.and_(rd8(obj),
				OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_INVISIBLE | OBJFLAG_PRIORITY);
			alu_.cp(cpu_.a, match);
			if (!(cpu_.f & FLAG_Z))
				continue;
			cpu_.e = (uint8_t)(wObj1ScreenYPos + n * OBJ_STRUCT_SIZE);
			cpu_.l = (uint8_t)(wObj1Id + n * OBJ_STRUCT_SIZE);
			CALL(UpdateObjSprite);
		}
	}

	void port_DrawObjects_NoPriority()
	{
		draw_objects(OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_PRIORITY);
	}

	void port_DrawObjects_WithPriority()
	{
		draw_objects(OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	}



	// One of CheckObjectOnScreen's range checks of the position hi:lo against
	// the camera position in hl: lo += add, then if the position is past hl,
	// hl += span and lo -= sub, and the position must be below hl. A carry out
	// of hi on the first step counts as in range if wrap_in, otherwise it goes
	// on to the second step. Returns whether the position is in range.
	bool in_range(uint8_t *hi, uint8_t *lo, uint8_t add, uint8_t span, uint8_t sub,
		bool wrap_in)
	{
		cpu_.a = *lo = alu_.add(*lo, add);
		if (cpu_.f & FLAG_C) {
			*hi = alu_.inc(*hi);
			if (cpu_.f & FLAG_Z) {
				if (wrap_in)
					return true;
				goto upper;
			}
		}
		cpu_.a = *hi;
		alu_.cp(*hi, cpu_.h);
		if (cpu_.f & FLAG_C)
			return false;
		if (cpu_.f & FLAG_Z) {
			cpu_.a = *lo;
			alu_.cp(*lo, cpu_.l);
			if (cpu_.f & FLAG_C)
				return false;
			if (cpu_.f & FLAG_Z)
				return true;
		}
	upper:
		cpu_.a = cpu_.l = alu_.add(cpu_.l, span);
		if (cpu_.f & FLAG_C)
			cpu_.h = alu_.inc(cpu_.h);
		cpu_.a = *lo = alu_.sub(*lo, sub);
		if (cpu_.f & FLAG_C) {
			*hi = alu_.dec(*hi);
			cpu_.a = alu_.inc(*hi);
			if (cpu_.f & FLAG_Z)
				return true;
		}
		cpu_.a = *hi;
		alu_.cp(*hi, cpu_.h);
		if (cpu_.f & FLAG_C)
			return true;
		if (!(cpu_.f & FLAG_Z))
			return false;
		cpu_.a = *lo;
		alu_.cp(*lo, cpu_.l);
		return cpu_.f & FLAG_C;
	}

	// ld hl, addr / ld a, [hli] / ld l, [hl] / ld h, a (a big-endian pointer)
	void load_hl_be(uint16_t addr)
	{
		cpu_.a = rd8(addr);
		cpu_.l = rd8(addr + 1);
		cpu_.h = cpu_.a;
	}

	// seems to be a function to load/unload objects
	// that are inside and outside the screen
	void port_CheckObjectOnScreen()
	{
		cpu_.c = rd8(wCurObjYPos + 0); // y lo
		cpu_.b = rd8(wCurObjYPos + 1); // y hi
		cpu_.a = cpu_.e = rd8(wCurObjYPos + 2); // x lo
		cpu_.d = rd8(wCurObjYPos + 3); // x hi

		load_hl_be(wCameraSCY);
		gb_push16(gb_, REG_BC(gb_));
		gb_push16(gb_, REG_DE(gb_));
		gb_push16(gb_, REG_HL(gb_));
		bool on_screen = false;
		if (in_range(&cpu_.b, &cpu_.c, 24, SCREEN_HEIGHT_PX, 56, false)) {
			// .skip_y2
			load_hl_be(wCameraSCX);
			on_screen = in_range(&cpu_.d, &cpu_.e, 24, SCREEN_WIDTH_PX, 0x30, true);
		} else {
			// .asm_630fd
			cpu_.a = alu_.and_(rd8(wCurObjState), 0xfe);
			alu_.cp(cpu_.a, OBJSTATE_48);
			if (cpu_.f & FLAG_Z)
				goto pop;
		}
		SET_HL(gb_, wCurObjFlags);
		if (on_screen)
			wr8(wCurObjFlags, rd8(wCurObjFlags) | OBJFLAG_ON_SCREEN);
		else
			wr8(wCurObjFlags, rd8(wCurObjFlags) & ~OBJFLAG_ON_SCREEN);
	pop:
		SET_HL(gb_, gb_pop16(gb_));
		SET_DE(gb_, gb_pop16(gb_));
		SET_BC(gb_, gb_pop16(gb_));

		// whether the object is far enough outside the screen to unload
		bool keep = false;
		cpu_.a = alu_.rla(cpu_.d);
		if (!(cpu_.f & FLAG_C)) {
			cpu_.a = alu_.rla(cpu_.b);
			if (!(cpu_.f & FLAG_C) && in_range(&cpu_.b, &cpu_.c, 92, 0x90, 0xcb, true)) {
				// .asm_63147
				load_hl_be(wCameraSCX);
				keep = in_range(&cpu_.d, &cpu_.e, 0x6f, SCREEN_WIDTH_PX, 0xde, true);
			}
		}
		if (keep)
			return;

		// .asm_6317e
		cpu_.a = alu_.and_(rd8(wCurObjSubState), 0xf | OBJSUBFLAG_UNK_4);
		alu_.cp(cpu_.a, 0x0 | OBJSUBFLAG_UNK_4);
		if (!(cpu_.f & FLAG_C))
			return;
		// OBJSUBFLAG_UNK_4 is not set
		SET_HL(gb_, wCurObjUnk02);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			FARCALL(DespawnObject);
			return;
		}
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wCurObjFlags, 0);
	}

	// a = value to fill wObjects with, c = a bytes (0: 256)
	void port_ClearObjects()
	{
		uint16_t hl = wObjects;
		cpu_.c = cpu_.a;
		do {
			wr8(hl++, cpu_.a);
			cpu_.c = alu_.dec(cpu_.c);
		} while (cpu_.c);
		SET_HL(gb_, hl);
	}

	// a = value to fill wObjDataPointers with
	void port_ClearObjDataPointers()
	{
		uint16_t hl = wObjDataPointers;
		cpu_.c = wObjDataPointersEnd - wObjDataPointers;
		do {
			wr8(hl++, cpu_.a);
			cpu_.c = alu_.dec(cpu_.c);
		} while (cpu_.c);
		SET_HL(gb_, hl);
	}

	// Sets the obj state at hl by the y velocity in hffa0: states[0] below 2,
	// states[1] below 3, states[2] below 4, else states[3]; then HomeJumpRet.
	void set_state_by_y_vel(const uint8_t states[4])
	{
		cpu_.a = rd8(hffa0);
		int i;
		for (i = 0; i < 3; i++) {
			alu_.cp(cpu_.a, (uint8_t)(2 + i));
			if (cpu_.f & FLAG_C)
				break;
		}
		cpu_.a = states[i];
		wr8(REG_HL(gb_), cpu_.a);
		JUMP(HomeJumpRet);
	}

	void port_Func_62d57()
	{
		static const uint8_t states[4] = {OBJSTATE_STUN_LEFT_START, OBJSTATE_20, OBJSTATE_22, OBJSTATE_24};
		set_state_by_y_vel(states);
	}

	void port_Func_62e31()
	{
		static const uint8_t states[4] = {OBJSTATE_STUN_RIGHT_START, OBJSTATE_21, OBJSTATE_23, OBJSTATE_25};
		set_state_by_y_vel(states);
	}

	// Stores hPos's y in wCurObjYPos, and updates OBJ_VAR_2 by the slope the
	// object is on: set to `set` on the slope `slope_set`, kept on the slope
	// `slope_keep`, else counted down to 0. Then hl = OBJ_STATE, and on to
	// `next`.
	void store_y_pos_by_slope(uint8_t slope_keep, uint8_t slope_set, bool dec, uint16_t next)
	{
		wr8(wCurObjYPos + 0, rd8(hYPosLo));
		cpu_.a = rd8(hYPosHi);
		wr8(wCurObjYPos + 1, cpu_.a);
		SET_HL(gb_, wCurObjYPos + 1);
		cpu_.a = alu_.and_(rd8(wObjSlopeInteraction), 0x0f);
		alu_.cp(cpu_.a, slope_keep);
		if (!(cpu_.f & FLAG_Z)) {
			cpu_.l = OBJ_VAR_2;
			uint16_t hl = REG_HL(gb_);
			alu_.cp(cpu_.a, slope_set);
			if (cpu_.f & FLAG_Z) {
				if (dec)
					cpu_.a = alu_.dec(cpu_.a);
				wr8(hl, cpu_.a);
			} else {
				cpu_.a = rd8(hl);
				if (alu_.and_(cpu_.a, cpu_.a))
					wr8(hl, alu_.dec(rd8(hl)));
			}
		}
		cpu_.l = OBJ_STATE;
		JUMP_TO(next); // fallthrough
	}

	void port_Func_62d34() { store_y_pos_by_slope(RIGHT_SLOPE, 0x02, true, ADDR_Func_62d57); }
	void port_Func_62e0f() { store_y_pos_by_slope(LEFT_SLOPE, 0x01, false, ADDR_Func_62e31); }

	// hPos = wCurObjYPos + (-dy, -dx), with the asm's sub / sbc
	void set_hpos_sub(uint16_t hl, uint8_t dy)
	{
		cpu_.a = alu_.sub(rd8(hl++), dy);
		wr8(hYPosLo, cpu_.a);
		cpu_.a = alu_.sbc(rd8(hl++), 0);
		wr8(hYPosHi, cpu_.a);
		SET_HL(gb_, hl);
	}

	// ld a, [addr] / cpl / inc a / ld b, a
	uint8_t neg_into_b(uint16_t addr)
	{
		cpu_.a = alu_.inc(alu_.cpl(rd8(addr)));
		cpu_.b = cpu_.a;
		return cpu_.b;
	}

	// An object thrown to the left (Func_62ec3) or right (Func_62faa): bc =
	// table of y movements, indexed by OBJ_VAR_3 (+$10 if heavy).
	void thrown_object_movement(bool left)
	{
		uint16_t routine = left ? ADDR_Func_62ec3 : ADDR_Func_62faa;
		uint16_t hl = wCurObjVar3;
		SET_HL(gb_, hl);
		cpu_.a = alu_.add(rd8(hl), cpu_.c);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		if (cpu_.f & FLAG_C) { // heavy
			cpu_.a = alu_.add(cpu_.c, 0x10);
			cpu_.c = cpu_.a;
		}
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (cpu_.f & FLAG_Z) {
			cpu_.c = alu_.dec(cpu_.c);
			cpu_.a = rd8(REG_BC(gb_));
		} else {
			wr8(hl, alu_.inc(rd8(hl)));
		}
		if (alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.l = OBJ_Y_POS;
			hl = REG_HL(gb_);
			cpu_.c = rd8(hl);
			cpu_.a = alu_.add(cpu_.a, cpu_.c);
			wr8(hl++, cpu_.a);
			SET_HL(gb_, hl);
			if (!(cpu_.f & FLAG_C))
				wr8(hl, alu_.dec(rd8(hl)));
		}
		if (left)
			CALL(MoveObjectLeftByVar2);
		else
			CALL(MoveObjectRightByVar2);

		cpu_.a = alu_.add(rd8(wCurObjScreenXPos), 0x28);
		alu_.cp(cpu_.a, left ? 0xf0 : 0x10);
		if (left ? !(cpu_.f & FLAG_C) : (cpu_.f & FLAG_C)) {
			cpu_.l = OBJ_FLAGS;
			hl = REG_HL(gb_);
			wr8(hl, (uint8_t)(rd8(hl) & ~(1 << OBJFLAG_GRABBED_F)));
		}

		uint8_t b = neg_into_b(wCurObjCollBoxTop);
		set_hpos_sub(wCurObjYPos, b);
		hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
		gb_call_ret(gb_, ADDR_Func_358b, (uint16_t)(routine + 0x50));
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		bool collided = cpu_.a != 0;
		if (!collided) {
			set_hpos_sub(wCurObjYPos, 8);
			hl = REG_HL(gb_);
			if (left) {
				b = neg_into_b(wCurObjCollBoxLeft);
				cpu_.a = alu_.sub(rd8(hl++), b);
				wr8(hXPosLo, cpu_.a);
				cpu_.a = alu_.sbc(rd8(hl++), 0);
				wr8(hXPosHi, cpu_.a);
			} else {
				cpu_.a = alu_.add(rd8(wCurObjCollBoxRight), rd8(hl));
				wr8(hXPosLo, cpu_.a);
				hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
				cpu_.a = alu_.adc(rd8(hl), 0);
				wr8(hXPosHi, cpu_.a);
			}
			SET_HL(gb_, hl);
			gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + (left ? 0x73 : 0x70)));
			cpu_.a = alu_.and_(cpu_.a, 0x0f);
			collided = cpu_.a != 0;
		}
		if (!collided) {
			SET_HL(gb_, wCurObjStateDuration);
			uint8_t d = alu_.dec(rd8(wCurObjStateDuration));
			wr8(wCurObjStateDuration, d);
			if (d) {
				JUMP(HomeJumpRet);
				return;
			}
		}
		// stops when it lands or hits something, the other way after a hit
		cpu_.a = (collided == left) ? OBJSTATE_27 : OBJSTATE_26;
		wr8(wCurObjState, cpu_.a);
		SET_HL(gb_, wCurObjFlags);
		wr8(wCurObjFlags, (uint8_t)(rd8(wCurObjFlags) & ~(1 << OBJFLAG_NO_COLLISION_F)));
		JUMP(HomeJumpRet);
	}

	void port_Func_62ec3() { thrown_object_movement(true); }
	void port_Func_62faa() { thrown_object_movement(false); }

	// output: e = number of objects with OBJFLAG_ACTIVE and OBJFLAG_TRANSIENT
	void port__CountActiveTransientObjects()
	{
		cpu_.e = 0;
		cpu_.d = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
		for (int n = 0; n < NUM_OBJECTS; n++) {
			cpu_.a = alu_.and_(rd8(wObj1Flags + n * OBJ_STRUCT_SIZE), cpu_.d);
			alu_.cp(cpu_.a, cpu_.d);
			if (cpu_.f & FLAG_Z)
				cpu_.e = alu_.inc(cpu_.e);
		}
	}

	// Moves the current object b pixels right (or left), and along the arc in
	// Data_60180 (Data_601a0 if heavy), indexed by OBJ_VAR_3.
	void move_obj_in_arc(bool right)
	{
		uint16_t hl = wCurObjXPos;
		uint8_t x = rd8(hl);
		cpu_.a = right ? alu_.add(x, cpu_.b) : alu_.sub(x, cpu_.b);
		wr8(hl++, cpu_.a);
		if (cpu_.f & FLAG_C)
			wr8(hl, right ? alu_.inc(rd8(hl)) : alu_.dec(rd8(hl)));

		hl = wCurObjVar3;
		cpu_.b = (uint8_t)(ADDR_Data_60180 >> 8);
		cpu_.a = alu_.add(rd8(hl), (uint8_t)ADDR_Data_60180);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		if (cpu_.f & FLAG_C) { // heavy
			cpu_.a = alu_.add(cpu_.c, (uint8_t)(ADDR_Data_601a0 - ADDR_Data_60180));
			cpu_.c = cpu_.a;
		}
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (cpu_.f & FLAG_Z) {
			cpu_.c = alu_.dec(cpu_.c);
			cpu_.a = rd8(REG_BC(gb_));
		} else {
			wr8(hl, alu_.inc(rd8(hl)));
		}

		wr8(hffa0, cpu_.a);
		hl = (uint16_t)((hl & 0xff00) | OBJ_Y_POS);
		alu_.cp(cpu_.a, 0x80);
		bool negative = !(cpu_.f & FLAG_C);
		cpu_.c = rd8(hl);
		cpu_.a = alu_.add(cpu_.a, cpu_.c);
		wr8(hl++, cpu_.a);
		if (negative && !(cpu_.f & FLAG_C))
			wr8(hl, alu_.dec(rd8(hl)));
		else if (!negative && (cpu_.f & FLAG_C))
			wr8(hl, alu_.inc(rd8(hl)));
		SET_HL(gb_, hl);

		cpu_.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(wCurObjAction, cpu_.a);
		JUMP(HomeJumpRet);
	}

	void port_Func_6284f() { move_obj_in_arc(true); }
	void port_Func_628a7() { move_obj_in_arc(false); }

	void port_Func_628ea()
	{
		uint16_t hl = wCurObjYPos;
		wr8(hYPosLo, rd8(hl++));
		wr8(hYPosHi, rd8(hl++));
		wr8(hXPosLo, rd8(hl++));
		cpu_.a = rd8(hl);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
		gb_call_ret(gb_, ADDR_Func_3543, ADDR_Func_628ea + 0x12);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.a = OBJSTATE_12;
			wr8(wCurObjState, OBJSTATE_12);
		}
		JUMP(HomeJumpRet);
	}

	// --- thrown object state entries ---

	// ld hl, wCurObjState / ld a, state / ld [hld], a / dec l / xor a /
	// ld [hld], a (OBJ_VAR_3) / ld a, [wCurObjInteractionType] / rla /
	// ld a, light / jr nc / ld a, heavy / ld [wCurObjStateDuration], a
	void set_thrown_state(uint8_t state, uint8_t light, uint8_t heavy)
	{
		uint16_t hl = wCurObjState;
		wr8(hl--, state);
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
		xor_a();
		wr8(hl--, 0);
		SET_HL(gb_, hl);
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		cpu_.a = CF ? heavy : light;
		wr8(wCurObjStateDuration, cpu_.a);
	}

	void thrown_left(uint16_t data)
	{
		SET_BC(gb_, data);
		JUMP_TO(ADDR_Func_62ec3);
	}

	void thrown_right(uint16_t data)
	{
		SET_BC(gb_, data);
		JUMP_TO(ADDR_Func_62faa);
	}

	void port_Func_62e74() { set_thrown_state(OBJSTATE_54, 0x06, 0x02); thrown_left(ADDR_Data_60300); }
	void port_Func_62e8a() { thrown_left(ADDR_Data_60300); }
	void port_Func_62e8f() { set_thrown_state(OBJSTATE_50, 0x04, 0x01); thrown_left(ADDR_Data_602c0); }
	void port_Func_62ea5() { thrown_left(ADDR_Data_602c0); }
	void port_Func_62eaa() { set_thrown_state(OBJSTATE_52, 0x05, 0x02); thrown_left(ADDR_Data_602e0); }
	void port_Func_62ec0() { thrown_left(ADDR_Data_602e0); }
	void port_Func_62f5b() { set_thrown_state(OBJSTATE_55, 0x06, 0x02); thrown_right(ADDR_Data_60300); }
	void port_Func_62f71() { thrown_right(ADDR_Data_60300); }
	void port_Func_62f76() { set_thrown_state(OBJSTATE_51, 0x04, 0x01); thrown_right(ADDR_Data_602c0); }
	void port_Func_62f8c() { thrown_right(ADDR_Data_602c0); }
	void port_Func_62f91() { set_thrown_state(OBJSTATE_53, 0x05, 0x02); thrown_right(ADDR_Data_602e0); }
	void port_Func_62fa7() { thrown_right(ADDR_Data_602e0); }

	// --- arcs ---

	// ld a, [wCurObjVar2] / ld b, a; *4 if the power up is at least GARLIC
	void var2_into_b(bool garlic)
	{
		cpu_.a = rd8(wCurObjVar2);
		cpu_.b = cpu_.a;
		if (!garlic)
			return;
		ld_cp(wEffectivePowerUp, GARLIC);
		if (CF)
			return;
		cpu_.b = alu_.sla(cpu_.b);
		cpu_.b = alu_.sla(cpu_.b);
	}

	void port_Func_6283a() { var2_into_b(false); JUMP_TO(ADDR_Func_6284f); }
	void port_Func_62840() { var2_into_b(true); JUMP_TO(ADDR_Func_6284f); }
	void port_Func_62892() { var2_into_b(false); JUMP_TO(ADDR_Func_628a7); }
	void port_Func_62898() { var2_into_b(true); JUMP_TO(ADDR_Func_628a7); }

	void port_Func_62908()
	{
		uint16_t hl = wCurObjYPos;
		wr8(hYPosLo, rd8(hl++));
		wr8(hYPosHi, rd8(hl++));
		wr8(hXPosLo, rd8(hl++));
		cpu_.a = rd8(hl);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
		gb_call_ret(gb_, ADDR_Func_3543, ADDR_Func_62908 + 0x12);
		if (!alu_.and_(cpu_.a, cpu_.a))
			st(wCurObjState, OBJSTATE_STANDING_FALL_START);
		JUMP(HomeJumpRet);
	}

	// --- y velocity tables ---

	// hl = wCurObjVar3, bc = light + OBJ_VAR_3 (or the heavy table), a = [bc],
	// the last entry ($80 ends the table) repeats, else OBJ_VAR_3 goes up.
	uint8_t obj_y_vel(uint16_t light, uint16_t heavy)
	{
		uint16_t hl = wCurObjVar3;
		SET_HL(gb_, hl);
		cpu_.b = (uint8_t)(light >> 8);
		cpu_.a = alu_.add(rd8(hl), (uint8_t)light);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		if (CF) {
			cpu_.a = alu_.add(cpu_.c, (uint8_t)(heavy - light));
			cpu_.c = cpu_.a;
		}
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (ZF) {
			cpu_.c = alu_.dec(cpu_.c);
			cpu_.a = rd8(REG_BC(gb_));
		} else {
			wr8(hl, alu_.inc(rd8(hl)));
		}
		return cpu_.a;
	}

	enum { ADD_SIGNED, ADD_POSITIVE, ADD_NEGATIVE };

	// ld l, OBJ_Y_POS / (cp $80) / ld c, [hl] / add c / ld [hli], a and the
	// carry into the high byte for a positive / negative a
	void add_obj_y(int mode)
	{
		uint16_t hl = (uint16_t)((REG_HL(gb_) & 0xff00) | OBJ_Y_POS);
		bool negative = mode == ADD_NEGATIVE;
		if (mode == ADD_SIGNED) {
			alu_.cp(cpu_.a, 0x80);
			negative = !CF;
		}
		cpu_.c = rd8(hl);
		cpu_.a = alu_.add(cpu_.a, cpu_.c);
		wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		if (negative && !CF)
			wr8(hl, alu_.dec(rd8(hl)));
		else if (!negative && CF)
			wr8(hl, alu_.inc(rd8(hl)));
	}

	// hPos = wCurObjYPos..; hl = wCurObjYPos + 3
	void obj_pos_to_hpos()
	{
		uint16_t hl = wCurObjYPos;
		cpu_.a = rd8(hl++);
		wr8(hYPosLo, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(hYPosHi, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = rd8(hl);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
	}

	// The end of a fall: checks the floor (Func_352b, returning to ret) and
	// lands in the state land_state, or on a slope, or vanishes in water/sand.
	void obj_fall_and_land(uint16_t ret, uint8_t land_state)
	{
		obj_pos_to_hpos();
		xor_a();
		wr8(wObjSlopeInteraction, 0);
		gb_call_ret(gb_, ADDR_Func_352b, ret);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			if (ld_and(wIsInWaterOrSand))
				st(wCurObjState, OBJSTATE_VANISH_TOUCH);
			JUMP(HomeJumpRet);
			return;
		}
		uint16_t hl = wCurObjYPos;
		cpu_.a = rd8(hYPosLo);
		wr8(hl++, cpu_.a);
		cpu_.a = rd8(hYPosHi);
		wr8(hl, cpu_.a);
		hl = (uint16_t)((hl & 0xff00) | OBJ_STATE);
		SET_HL(gb_, hl);
		if (!ld_and(wObjSlopeInteraction)) {
			cpu_.a = land_state;
			wr8(hl, land_state);
			JUMP(HomeJumpRet);
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		alu_.cp(cpu_.a, 0x01);
		st(wCurObjVar2, 0x02);
		JUMP_TO(ZF ? ADDR_Func_62d57 : ADDR_Func_62e31);
	}

	void port_Func_62926()
	{
		cpu_.a = obj_y_vel(ADDR_Data_60040, ADDR_Data_60060);
		wr8(hffa0, cpu_.a);
		add_obj_y(ADD_SIGNED);
		obj_fall_and_land(ADDR_Func_62926 + 0x48, OBJSTATE_0F);
	}

	void port_Func_629a6()
	{
		cpu_.a = obj_y_vel(ADDR_Data_602a0, ADDR_Data_602b0);
		wr8(hffa0, cpu_.a);
		add_obj_y(ADD_NEGATIVE);
		JUMP(HomeJumpRet);
	}

	void port__ObjState_StandingFall()
	{
		// speed up animation by 3x
		SET_HL(gb_, wCurObjFrameDuration);
		cpu_.a = rd8(wCurObjFrameDuration);
		alu_.cp(cpu_.a, 3);
		if (!CF) {
			cpu_.a = alu_.sub(cpu_.a, 3);
			wr8(wCurObjFrameDuration, cpu_.a);
		}
		cpu_.a = obj_y_vel(ADDR_StandingFallYVel_Light, ADDR_StandingFallYVel_Heavy);
		wr8(hffa0, cpu_.a);
		add_obj_y(ADD_SIGNED);
		obj_fall_and_land(ADDR__ObjState_StandingFall + 0x53, OBJSTATE_TURN_AROUND_START);
	}

	void port__ObjState_Fall()
	{
		cpu_.a = obj_y_vel(ADDR_FallingYVel_Light, ADDR_FallingYVel_Heavy);
		wr8(hffa0, cpu_.a);
		add_obj_y(ADD_POSITIVE);
		obj_fall_and_land(ADDR__ObjState_Fall + 0x3d, OBJSTATE_LAND);
	}

	// Func_627f0 / Func_62690: moves the object up or down by the table entry
	// at OBJ_VAR_3 (table_z if b was 1, else table_nz; +$20 if heavy) and
	// advances OBJ_VAR_3 until the table's $80 end marker.
	void obj_y_by_table(uint16_t table_z, uint16_t table_nz)
	{
		uint16_t hl = (uint16_t)((REG_HL(gb_) & 0xff00) | OBJ_VAR_3);
		SET_HL(gb_, hl);
		cpu_.a = rd8(hl);
		cpu_.b = alu_.dec(cpu_.b);
		uint16_t table = ZF ? table_z : table_nz;
		cpu_.b = (uint8_t)(table >> 8);
		cpu_.a = alu_.add(cpu_.a, (uint8_t)table);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		if (CF) {
			cpu_.a = alu_.add(cpu_.c, 0x20);
			cpu_.c = cpu_.a;
		}
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (ZF) {
			cpu_.c = alu_.dec(cpu_.c);
			cpu_.a = rd8(REG_BC(gb_));
		} else {
			wr8(hl, alu_.inc(rd8(hl)));
		}
		wr8(hffa0, cpu_.a);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			JUMP(HomeJumpRet);
			return;
		}
		hl = (uint16_t)((hl & 0xff00) | OBJ_Y_POS);
		alu_.cp(cpu_.a, 0x80);
		bool negative = !CF;
		cpu_.c = rd8(hl);
		cpu_.a = alu_.add(cpu_.a, cpu_.c);
		wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		bool carry = CF;
		cpu_.a = negative ? 0x00 : 0x01;
		wr8(wCurObjStateDuration, cpu_.a);
		if (negative && !carry)
			wr8(hl, alu_.dec(rd8(hl)));
		else if (!negative && carry)
			wr8(hl, alu_.inc(rd8(hl)));
		JUMP(HomeJumpRet);
	}

	void port_Func_627f0() { obj_y_by_table(ADDR_Data_60140, ADDR_Data_600c0); }
	void port_Func_62690() { obj_y_by_table(ADDR_Data_60100, ADDR_Data_60080); }

	// --- misc states ---

	void port_Func_6303f()
	{
		uint16_t hl = wCurObjState;
		wr8(hl--, OBJSTATE_1B);
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
		xor_a();
		wr8(hl--, 0); // OBJ_VAR_3
		SET_HL(gb_, hl);
		cpu_.a = 0x07;
		wr8(wCurObjStateDuration, 0x07);
		JUMP(HomeJumpRet);
	}

	void port_Func_63050()
	{
		SET_HL(gb_, wCurObjStateDuration);
		uint8_t d = alu_.dec(rd8(wCurObjStateDuration));
		wr8(wCurObjStateDuration, d);
		if (d) {
			JUMP_TO(ADDR_Func_629a6);
			return;
		}
		st(wCurObjState, OBJSTATE_STANDING_FALL_START);
		JUMP(HomeJumpRet);
	}

	void port_Func_6305f()
	{
		cpu_.a = alu_.rra(rd8(rDIV));
		cpu_.a = CF ? OBJSTATE_WOBBLE_LEFT : OBJSTATE_WOBBLE_RIGHT;
		uint16_t hl = wCurObjState;
		wr8(hl--, cpu_.a);
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
		xor_a();
		wr8(hl--, 0); // OBJ_VAR_3
		cpu_.a = 0x01;
		wr8(hl--, 0x01); // OBJ_VAR_2
		SET_HL(gb_, hl);
		cpu_.l = alu_.dec(cpu_.l); // dec l (sets the exit flags)
		hl = REG_HL(gb_);
		cpu_.a = 0x04;
		wr8(hl++, 0x04); // OBJ_STATE_DURATION
		SET_HL(gb_, hl);
		JUMP(HomeJumpRet);
	}

	// --- bumping and knocked-back objects ---

	// res OBJFLAG_GRABBED_F of the flags at the current page (ld l, OBJ_FLAGS)
	void res_grabbed()
	{
		cpu_.l = OBJ_FLAGS;
		uint16_t hl = REG_HL(gb_);
		wr8(hl, (uint8_t)(rd8(hl) & ~(1 << OBJFLAG_GRABBED_F)));
	}

	// ld a, [wCurObjScreenXPos] / add $28 / cp $f0 (left) or $10 (right), and
	// the object is let go when it goes off screen
	void check_offscreen_grabbed(bool left)
	{
		cpu_.a = alu_.add(rd8(wCurObjScreenXPos), 0x28);
		alu_.cp(cpu_.a, left ? 0xf0 : 0x10);
		if (left ? !CF : CF)
			res_grabbed();
	}

	// hXPos = the object's left edge (wCurObjXPos - -box left), from hl = wCurObjXPos
	void hxpos_left_edge(uint16_t hl)
	{
		uint8_t b = neg_into_b(wCurObjCollBoxLeft);
		cpu_.a = alu_.sub(rd8(hl++), b);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = alu_.sbc(rd8(hl++), 0);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
	}

	// hXPos = the object's right edge (box right + wCurObjXPos), from hl = wCurObjXPos
	void hxpos_right_edge(uint16_t hl)
	{
		cpu_.a = alu_.add(rd8(wCurObjCollBoxRight), rd8(hl));
		wr8(hXPosLo, cpu_.a);
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
		cpu_.a = alu_.adc(rd8(hl), 0);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
	}

	// hl = YPos / [hli] = hYPosLo / [hl] = hYPosHi
	void hypos_to_obj()
	{
		uint16_t hl = wCurObjYPos;
		cpu_.a = rd8(hYPosLo);
		wr8(hl++, cpu_.a);
		cpu_.a = rd8(hYPosHi);
		wr8(hl, cpu_.a);
		SET_HL(gb_, hl);
	}

	// Func_62ae5 (moving left) / Func_62bce (moving right): an object that
	// bumped into something slides back, slowing down
	void obj_bump_slide(bool left)
	{
		uint16_t routine = left ? ADDR_Func_62ae5 : ADDR_Func_62bce;
		set_hpos_sub(wCurObjYPos, 4);
		if (left)
			hxpos_left_edge(REG_HL(gb_));
		else
			hxpos_right_edge(REG_HL(gb_));
		gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + (left ? 0x1f : 0x1c)));
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		if (cpu_.a) {
			play_sfx(SFX_01A);
			st(wCurObjState, left ? OBJSTATE_STUN_RIGHT_START : OBJSTATE_STUN_LEFT_START);
		} else {
			if (left)
				CALL(MoveObjectLeftByVar2);
			else
				CALL(MoveObjectRightByVar2);
			check_offscreen_grabbed(left);
			obj_pos_to_hpos();
			xor_a();
			wr8(wObjSlopeInteraction, 0);
			gb_call_ret(gb_, ADDR_Func_3543, (uint16_t)(routine + (left ? 0x4a : 0x47)));
			alu_.cp(cpu_.a, 0x10);
			uint8_t f = cpu_.f;
			hypos_to_obj();
			cpu_.f = f;
			if (ZF) {
				res_grabbed();
				cpu_.l = OBJ_STATE;
				uint16_t hl = REG_HL(gb_);
				cpu_.a = alu_.rra(rd8(wObjSlopeInteraction));
				if (left ? !CF : CF) {
					cpu_.a = left ? OBJSTATE_STUN_RIGHT_START : OBJSTATE_STUN_LEFT_START;
					wr8(hl, cpu_.a);
				} else {
					cpu_.a = rd8(hl);
					alu_.cp(cpu_.a, left ? OBJSTATE_STUN_LEFT : OBJSTATE_STUN_RIGHT);
					if (!ZF) {
						cpu_.a = left ? OBJSTATE_14 : OBJSTATE_15;
						wr8(hl, cpu_.a);
					}
				}
				JUMP(HomeJumpRet);
				return;
			}
			bool hit = false;
			for (int side = 0; side < 2 && !hit; side++) {
				// left: the left edge then the right edge; right: the other way
				if ((side == 0) == left)
					hxpos_left_edge(wCurObjXPos);
				else
					hxpos_right_edge(wCurObjXPos);
				gb_call_ret(gb_, ADDR_Func_35a3, (uint16_t)(routine +
					(left ? (side ? 0x81 : 0x6c) : (side ? 0x7e : 0x66))));
				hit = alu_.and_(cpu_.a, cpu_.a) != 0;
			}
			if (!hit)
				st(wCurObjState, left ? OBJSTATE_14 : OBJSTATE_15);
		}
		SET_HL(gb_, wCurObjVar2);
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		bool heavy = CF;
		cpu_.a = alu_.and_(rd8(wGlobalCounter), heavy ? 0x3 : 0x7);
		if (!cpu_.a)
			wr8(wCurObjVar2, alu_.dec(rd8(wCurObjVar2)));
		JUMP(HomeJumpRet);
	}

	void port_Func_62ae5() { obj_bump_slide(true); }
	void port_Func_62bce() { obj_bump_slide(false); }

	void obj_bump(uint16_t slide)
	{
		uint16_t hl = wCurObjVar2;
		SET_HL(gb_, hl);
		cpu_.a = rd8(hl);
		if (alu_.and_(cpu_.a, cpu_.a)) {
			JUMP_TO(slide);
			return;
		}
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 2)); // dec l / dec l
		SET_HL(gb_, hl);
		uint8_t d = alu_.dec(rd8(hl)); // OBJ_STATE_DURATION
		wr8(hl, d);
		if (!d) {
			cpu_.a = OBJSTATE_TURN_AROUND_START;
			wr8(wCurObjState, cpu_.a);
		}
		JUMP(HomeJumpRet);
	}

	void port__ObjState_BumpRight() { obj_bump(ADDR_Func_62ae5); }
	void port__ObjState_BumpLeft() { obj_bump(ADDR_Func_62bce); }

	// Func_62ca8 (to the left) / Func_62d86 (to the right): an object knocked
	// back in the air
	void obj_knocked_fall(bool left)
	{
		uint16_t routine = left ? ADDR_Func_62ca8 : ADDR_Func_62d86;
		if (left)
			CALL(MoveObjectLeftByVar2);
		else
			CALL(MoveObjectRightByVar2);
		check_offscreen_grabbed(left);
		cpu_.l = OBJ_VAR_3;
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl);
		cpu_.b = (uint8_t)(ADDR_FallingYVel_Light >> 8);
		cpu_.a = alu_.add(cpu_.a, (uint8_t)ADDR_FallingYVel_Light);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.rla(rd8(wCurObjInteractionType));
		if (CF) {
			cpu_.a = alu_.add(cpu_.c, (uint8_t)(ADDR_FallingYVel_Heavy - ADDR_FallingYVel_Light));
			cpu_.c = cpu_.a;
		}
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (ZF) {
			cpu_.c = alu_.dec(cpu_.c);
			cpu_.a = rd8(REG_BC(gb_));
		} else {
			wr8(hl, alu_.inc(rd8(hl)));
		}
		wr8(hffa0, cpu_.a);
		add_obj_y(ADD_POSITIVE);
		obj_pos_to_hpos();
		xor_a();
		wr8(wObjSlopeInteraction, 0);
		wr8(wc1ca, 0);
		gb_call_ret(gb_, ADDR_Func_352b, (uint16_t)(routine + 0x4f));
		if (alu_.and_(cpu_.a, cpu_.a)) {
			JUMP_TO(left ? ADDR_Func_62d34 : ADDR_Func_62e0f);
			return;
		}
		if (ld_and(wIsInWaterOrSand)) {
			st(wCurObjState, OBJSTATE_VANISH_TOUCH);
			JUMP(HomeJumpRet);
			return;
		}
		set_hpos_sub(wCurObjYPos, 4);
		if (left)
			hxpos_left_edge(REG_HL(gb_));
		else
			hxpos_right_edge(REG_HL(gb_));
		gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + (left ? 0x77 : 0x74)));
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		if (cpu_.a)
			st(wCurObjState, left ? OBJSTATE_15 : OBJSTATE_14);
		JUMP(HomeJumpRet);
	}

	void port_Func_62ca8() { obj_knocked_fall(true); }
	void port_Func_62d86() { obj_knocked_fall(false); }

	// ld hl, wCurObjState / ld a, state / ld [hld], a / dec l / xor a / ld [hld], a
	void set_state_clear_var3(uint8_t state)
	{
		uint16_t hl = wCurObjState;
		cpu_.a = state;
		wr8(hl--, state);
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
		xor_a();
		wr8(hl--, 0);
		SET_HL(gb_, hl);
	}

	void port_Func_62c9f() { set_state_clear_var3(OBJSTATE_56); obj_knocked_fall(true); }
	void port_Func_62d7d() { set_state_clear_var3(OBJSTATE_57); obj_knocked_fall(false); }

	// --- vanishing ---

	void vanish_object()
	{
		uint16_t hl = wCurObjUpdateFunction + 1;
		wr8(hl--, (uint8_t)(ADDR_StarsFunc >> 8));
		cpu_.a = (uint8_t)ADDR_StarsFunc;
		wr8(hl--, cpu_.a);
		hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
		cpu_.a = alu_.and_(rd8(hl), OBJSUBFLAGS_MASK);
		wr8(hl--, cpu_.a);
		SET_HL(gb_, hl);
	}

	// The object turns into stars (StarsFunc)
	void port_Func_618b4()
	{
		// (BANK(StarsFunc) - BANK("Object Functions 1")) << 4
		st(wCurObjId, 0x30);
		uint16_t hl = wCurObjOAMPtr;
		wr8(hl++, (uint8_t)ADDR_OAM_18c000);
		wr8(hl++, (uint8_t)(ADDR_OAM_18c000 >> 8));
		wr8(hl++, (uint8_t)ADDR_Frameset_680c5);
		cpu_.a = (uint8_t)(ADDR_Frameset_680c5 >> 8);
		wr8(hl++, cpu_.a);
		xor_a();
		wr8(hl++, 0); // OBJ_FRAME_DURATION
		wr8(hl++, 0); // OBJ_FRAMESET_OFFSET
		cpu_.a = 33;
		wr8(hl++, 33); // OBJ_STATE_DURATION

		hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
		uint8_t flags = rd8(hl);
		flags &= (uint8_t)~(1 << OBJFLAG_GRABBED_F);
		wr8(hl, flags);
		wr8(hl, (uint8_t)(rd8(hl) | (1 << OBJFLAG_NO_COLLISION_F)));
		st(wCurObjAction, (1 << NO_ACTIONS_F) | 1); // no_actions_for 1

		hl = (uint16_t)((hl & 0xff00) | OBJ_Y_POS);
		cpu_.a = alu_.sub(rd8(hl), 8);
		wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		if (CF)
			wr8(hl, alu_.dec(rd8(hl)));
	}

	void port_VanishObject_NoSFX()
	{
		vanish_object();
		JUMP_TO(ADDR_Func_618b4);
	}

	void port__VanishObject()
	{
		play_sfx(SFX_VANISH);
		vanish_object();
		JUMP_TO(ADDR_Func_618b4);
	}

	// --- init ---

	void init_object_vars(bool level_start)
	{
		xor_a();
		gb_call(gb_, ADDR_ClearObjects);
		gb_call(gb_, ADDR_ClearObjDataPointers);
		if (level_start)
			wr8(wFlameBlockCleared, cpu_.a);
		wr8(wDollBoyNoFlashingBarrel, cpu_.a);
		wr8(wDollBoyRemainingParts, cpu_.a);
		wr8(wNumLitTorches, cpu_.a);
		wr8(wShootGoals, cpu_.a);
		wr8(wWarioGoals, cpu_.a);
	}

	void port_InitObjectVars_FromLevelStart() { init_object_vars(true); }
	void port_InitObjectVars_FromTransition() { init_object_vars(false); }

	// --- movement tables ---

	// The signed add of a to the 16-bit position at hl (lo, hi): cp $80 /
	// ld c, [hl] / add c / ld [hli], a / then inc or dec the high byte on a
	// carry / no carry. hl ends at the high byte.
	void add_signed_at(uint16_t hl)
	{
		alu_.cp(cpu_.a, 0x80);
		bool negative = !CF;
		cpu_.c = rd8(hl);
		cpu_.a = alu_.add(cpu_.a, cpu_.c);
		wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		if (negative && !CF)
			wr8(hl, alu_.dec(rd8(hl)));
		else if (!negative && CF)
			wr8(hl, alu_.inc(rd8(hl)));
	}

	// hl = index, a = [bc + [hl]]; at the $80 end marker the last entry
	// repeats (loop: the table restarts), and the index goes up
	void movement_lookup(uint16_t index, bool loop)
	{
		uint8_t start = cpu_.c;
		if (loop)
			cpu_.e = start;
		SET_HL(gb_, index);
		cpu_.a = alu_.add(rd8(index), cpu_.c);
		cpu_.c = cpu_.a;
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (ZF) {
			if (loop) {
				xor_a();
				wr8(index, 0);
				cpu_.c = cpu_.e;
			} else {
				cpu_.c = alu_.dec(cpu_.c);
			}
			cpu_.a = rd8(REG_BC(gb_));
			if (!loop)
				return;
		}
		wr8(index, alu_.inc(rd8(index)));
	}

	void apply_obj_movement(uint16_t index, uint8_t pos, bool loop)
	{
		movement_lookup(index, loop);
		add_signed_at((uint16_t)((index & 0xff00) | pos));
	}

	void port__ApplyObjYMovement() { apply_obj_movement(wCurObjVar3, OBJ_Y_POS, false); }
	void port__ApplyObjXMovement() { apply_obj_movement(wCurObjVar2, OBJ_X_POS, false); }
	void port__ApplyObjYMovement_Loop() { apply_obj_movement(wCurObjVar3, OBJ_Y_POS, true); }
	void port__ApplyObjXMovement_Loop() { apply_obj_movement(wCurObjVar2, OBJ_X_POS, true); }

	void port_Func_63209()
	{
		movement_lookup(wCurObjVar3, false);
		cpu_.b = cpu_.a;
		add_signed_at((uint16_t)((wCurObjVar3 & 0xff00) | OBJ_Y_POS));
		cpu_.l = OBJ_FLAGS;
		uint16_t hl = REG_HL(gb_);
		if (!alu_.bit(rd8(hl), OBJFLAG_STEPPED_F))
			return;
		wr8(hl, (uint8_t)(rd8(hl) & ~(1 << OBJFLAG_STEPPED_F)));
		CALL(MoveWarioDown);
		FARCALL(Func_19720);
	}

	void port__ApplyObjSetMovement()
	{
		uint16_t hl = wCurObjVar1;
		SET_HL(gb_, hl);
		uint8_t d = alu_.dec(rd8(hl));
		wr8(hl, d);
		if (d)
			return;
		uint8_t l = alu_.inc((uint8_t)hl); // inc l
		hl = (uint16_t)((hl & 0xff00) | l);
		cpu_.a = rd8(hl++); // var2
		hl = (uint16_t)((rd8(hl) << 8) | cpu_.a); // var3
		cpu_.a = rd8(hl++);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			// loop
			hl = REG_DE(gb_);
			cpu_.a = rd8(hl++);
		}
		wr8(wCurObjVar1, cpu_.a); // duration
		cpu_.c = rd8(hl++);
		cpu_.b = rd8(hl++);
		cpu_.a = (uint8_t)hl;
		wr8(wCurObjVar2, cpu_.a);
		cpu_.a = (uint8_t)(hl >> 8);
		wr8(wCurObjVar3, cpu_.a);

		// y += c, x += b (signed)
		hl = wCurObjYPos;
		for (int i = 0; i < 2; i++) {
			uint8_t v = i ? cpu_.b : cpu_.c;
			if (i) {
				l = alu_.inc((uint8_t)hl); // inc l
				hl = (uint16_t)((hl & 0xff00) | l);
			}
			alu_.cp(v, 0x80);
			bool negative = !CF;
			cpu_.a = alu_.add(rd8(hl), v);
			wr8(hl++, cpu_.a);
			if (negative && !CF)
				wr8(hl, alu_.dec(rd8(hl)));
			else if (!negative && CF)
				wr8(hl, alu_.inc(rd8(hl)));
		}
		SET_HL(gb_, hl);
	}

	// e = the number of active transient objects with the same OBJ_UNK_01/02
	// as the current object
	void port_Func_632ac()
	{
		cpu_.c = rd8(wCurObjUnk01);
		cpu_.b = rd8(wCurObjUnk01 + 1);
		cpu_.e = 0x00;
		cpu_.d = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
		uint16_t hl = 0;
		for (int n = 0; n < NUM_OBJECTS; n++) {
			hl = (uint16_t)((wObj1 & 0xff00) | (n * 0x20));
			cpu_.a = alu_.and_(rd8(hl++), cpu_.d);
			alu_.cp(cpu_.a, cpu_.d);
			if (!ZF)
				continue;
			cpu_.a = rd8(hl++);
			alu_.cp(cpu_.a, cpu_.c);
			if (!ZF)
				continue;
			cpu_.a = rd8(hl);
			alu_.cp(cpu_.a, cpu_.b);
			if (!ZF)
				continue;
			cpu_.e = alu_.inc(cpu_.e);
		}
		SET_HL(gb_, hl);
	}

	// --- thrown objects hitting other objects ---

	#define OBJ_N(addr, n) ((uint16_t)((addr) + (n) * OBJ_STRUCT_SIZE))

	// [hl] = a for a in states, with hli
	void write_states(uint16_t hl, const uint8_t *states, int n)
	{
		for (int i = 0; i < n; i++) {
			cpu_.a = states[i];
			wr8(hl++, states[i]);
		}
		SET_HL(gb_, hl);
	}

	// a = [hl] & $fe / cp OBJSTATE_PICKED_UP_LEFT / jr z / cp OBJSTATE_48 /
	// jr nz, .skip / xor a / ld [wGrabState], a
	void reset_grab_if_held(uint16_t state)
	{
		cpu_.a = alu_.and_(rd8(state), 0xfe);
		alu_.cp(cpu_.a, OBJSTATE_PICKED_UP_LEFT);
		if (!ZF) {
			alu_.cp(cpu_.a, OBJSTATE_48);
			if (!ZF)
				return;
		}
		xor_a();
		wr8(wGrabState, 0);
	}

	void port_HandleGrabbedObjectCollisions()
	{
		uint16_t hl = wObj1Flags;
		cpu_.e = OBJFLAG_ACTIVE | OBJFLAG_GRABBED;
		int n;
		for (n = 0; n < NUM_OBJECTS; n++) {
			hl = OBJ_N(wObj1Flags, n);
			cpu_.a = alu_.and_(rd8(hl), cpu_.e);
			alu_.cp(cpu_.a, cpu_.e);
			if (ZF)
				break;
		}
		SET_HL(gb_, hl);
		if (n == NUM_OBJECTS)
			return;

		gb_push16(gb_, hl);
		hl = (uint16_t)((hl & 0xff00) | alu_.add((uint8_t)hl, OBJ_INTERACTION_TYPE - OBJ_FLAGS));
		cpu_.a = rd8(hl++);
		wr8(hThrownObjInteractionType, cpu_.a);
		cpu_.b = rd8(hl++);
		cpu_.c = rd8(hl++);
		cpu_.d = rd8(hl++);
		cpu_.e = rd8(hl++);
		cpu_.a = alu_.add(alu_.add(rd8(hl), 0x2a), cpu_.b);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.add(alu_.add(rd8(hl++), 0x2a), cpu_.c);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.add(alu_.add(rd8(hl), 0x2a), cpu_.d);
		cpu_.d = cpu_.a;
		cpu_.a = alu_.add(alu_.add(rd8(hl++), 0x2a), cpu_.e);
		cpu_.e = cpu_.a;

		hl = (uint16_t)((hl & 0xff00) | alu_.add((uint8_t)hl, OBJ_STATE - (OBJ_SCREEN_X_POS + 1)));
		cpu_.a = rd8(hl); // OBJ_STATE
		bool full_throw = true;
		alu_.cp(cpu_.a, OBJSTATE_FULL_THROW_LEFT_START);
		if (CF) {
			alu_.cp(cpu_.a, OBJSTATE_48);
			if (!CF)
				full_throw = false;
		}
		cpu_.a = alu_.rra(cpu_.a);
		bool right = CF;
		if (full_throw) {
			cpu_.a = rd8(hThrownObjInteractionType);
			cpu_.a = alu_.rla(cpu_.a);
			bool heavy = CF;
			static const uint8_t states[4][4] = {
				{OBJSTATE_28, OBJSTATE_29, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_29},
				{OBJSTATE_28, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_28, OBJSTATE_29},
				{OBJSTATE_29, OBJSTATE_28, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_28},
				{OBJSTATE_29, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_29, OBJSTATE_28},
			};
			write_states(hThrownObjInteractionType + 1, states[right * 2 + heavy], 4);
		} else {
			// OBJSTATE_48 <= state <= OBJSTATE_4B
			static const uint8_t states[2][2] = {
				{OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_WOBBLE_LEFT_START},
				{OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_WOBBLE_RIGHT_START},
			};
			write_states(hCollidedObjState_Light, states[right], 2);
		}

		for (n = 0; n < NUM_OBJECTS; n++) {
			cpu_.a = alu_.and_(rd8(OBJ_N(wObj1Flags, n)),
				OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_GRABBED | OBJFLAG_NO_COLLISION | OBJFLAG_INVISIBLE);
			alu_.cp(cpu_.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
			if (!ZF)
				continue;
			cpu_.a = alu_.add(rd8(OBJ_N(wObj1ScreenYPos, n)), 0x2a);
			cpu_.h = cpu_.a;
			cpu_.a = alu_.add(rd8(OBJ_N(wObj1ScreenXPos, n)), 0x2a);
			cpu_.l = cpu_.a;
			cpu_.a = alu_.sub(alu_.add(rd8(OBJ_N(wObj1CollBoxBottom, n)), cpu_.h), cpu_.b);
			if (CF)
				continue;
			cpu_.a = alu_.sub(alu_.add(rd8(OBJ_N(wObj1CollBoxTop, n)), cpu_.h), cpu_.c);
			if (!CF)
				continue;
			cpu_.a = alu_.sub(alu_.add(rd8(OBJ_N(wObj1CollBoxRight, n)), cpu_.l), cpu_.d);
			if (CF)
				continue;
			cpu_.a = alu_.sub(alu_.add(rd8(OBJ_N(wObj1CollBoxLeft, n)), cpu_.l), cpu_.e);
			if (!CF)
				continue;

			// do collision
			hl = gb_pop16(gb_);
			wr8(hl, (uint8_t)(rd8(hl) & ~(1 << OBJFLAG_GRABBED_F)));
			hl = (uint16_t)((hl & 0xff00) | alu_.add((uint8_t)hl, OBJ_STATE - OBJ_FLAGS));
			SET_HL(gb_, hl);
			cpu_.a = alu_.and_(rd8(hl), 0xfe);
			alu_.cp(cpu_.a, OBJSTATE_4A);
			if (!ZF)
				alu_.cp(cpu_.a, OBJSTATE_48);
			uint16_t other_state = (uint16_t)((hl & 0xff00) | (uint8_t)OBJ_N(wObj1State, n));
			uint16_t collided;
			if (ZF) {
				// .CollideWithObject
				xor_a();
				wr8(wGrabState, 0);
				cpu_.a = rd8(hThrownObjState_Light);
				wr8(hl, cpu_.a);
				collided = hCollidedObjState_Light;
			} else {
				cpu_.a = alu_.rla(rd8(OBJ_N(wObj1InteractionType, n)));
				bool heavy = CF;
				cpu_.a = rd8(heavy ? hThrownObjState_Heavy : hThrownObjState_Light);
				wr8(hl, cpu_.a);
				reset_grab_if_held(other_state);
				collided = heavy ? hCollidedObjState_Heavy : hCollidedObjState_Light;
			}
			cpu_.a = rd8(collided);
			wr8(other_state, cpu_.a);
			hl = (uint16_t)((hl & 0xff00) | (uint8_t)OBJ_N(wObj1Flags, n));
			SET_HL(gb_, hl);
			wr8(hl, (uint8_t)(rd8(hl) | (1 << OBJFLAG_NO_COLLISION_F)));
			play_sfx(SFX_016);
			return;
		}
		hl = gb_pop16(gb_);
		SET_HL(gb_, hl);
	}

	// --- DoObjectAction ---

	// ld [hld], a (OBJ_ACTION = action) / ld a, state / ld [hl], a (OBJ_STATE)
	void set_action_state(uint8_t action, uint8_t state)
	{
		cpu_.a = action;
		wr8(wCurObjAction, action);
		cpu_.a = state;
		wr8(wCurObjState, state);
		SET_HL(gb_, wCurObjState);
	}

	// ld hl, wCurObjAction / ld a, action / ld [hld], a / ld a, state / ld [hl], a
	void action_hl_state(uint8_t action, uint8_t state)
	{
		set_action_state(action, state);
	}

	void obj_flags_res_grabbed(bool set_no_collision)
	{
		uint8_t v = (uint8_t)(rd8(wCurObjFlags) & ~(1 << OBJFLAG_GRABBED_F));
		wr8(wCurObjFlags, v);
		if (set_no_collision)
			wr8(wCurObjFlags, (uint8_t)(rd8(wCurObjFlags) | (1 << OBJFLAG_NO_COLLISION_F)));
	}

	// ld a, [hld] (OBJ_UNK_1D) / and INTERACTION_RIGHT / ld a, no_actions_for 15 /
	// ld [hld], a / jr z, .right / left or right state
	void action_by_interaction(uint8_t left, uint8_t right)
	{
		alu_.and_(rd8(wCurObjUnk1d), INTERACTION_RIGHT);
		set_action_state(15 | (1 << 7), ZF ? right : left);
	}

	void action_wobble()
	{
		obj_flags_res_grabbed(false);
		action_by_interaction(OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_WOBBLE_RIGHT_START);
	}

	// .Action06 / .Action12
	void action_06(uint8_t b)
	{
		cpu_.b = b;
		cpu_.a = rd8(wCurObjUnk1d);
		wr8(wCurObjAction, cpu_.b);
		cpu_.l = alu_.dec((uint8_t)wCurObjAction);
		cpu_.h = (uint8_t)(wCurObjAction >> 8);
		static const uint8_t states[4] = {OBJSTATE_0E, OBJSTATE_0D, OBJSTATE_0B, OBJSTATE_0C};
		int i;
		for (i = 0; i < 3; i++) {
			cpu_.a = alu_.rla(cpu_.a);
			if (CF)
				break;
		}
		cpu_.a = states[i];
		wr8(REG_HL(gb_), cpu_.a);
	}

	void port_DoObjectAction()
	{
		cpu_.a = rd8(wCurObjAction);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (ZF) {
			RET();
			return;
		}
		if (alu_.bit(cpu_.a, NO_ACTIONS_F)) {
			cpu_.a = alu_.dec(cpu_.a);
			alu_.cp(cpu_.a, 1 << NO_ACTIONS_F); // no_actions_for 0
			if (ZF)
				xor_a();
			wr8(wCurObjAction, cpu_.a);
			RET();
			return;
		}

		// .jump: rst JumpTable
		uint8_t idx = cpu_.a;
		uint16_t table = ADDR_DoObjectAction + 0x14;
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(table, cpu_.e);
		cpu_.a = rd8(hl);
		cpu_.h = rd8((uint16_t)(hl + 1));
		cpu_.l = cpu_.a;
		uint16_t target = REG_HL(gb_);
		if (idx >= NUM_OBJ_ACTIONS) {
			JUMP_TO(target);
			return;
		}

		switch (idx) {
		case OBJACTION_NONE:
			break;
		case OBJACTION_BUMP:
			cpu_.a = alu_.and_(rd8(wCurObjInteractionType), INTERACTION_MASK);
			if (ZF) {
				action_wobble();
				break;
			}
			action_by_interaction(OBJSTATE_BUMP_LEFT_START, OBJSTATE_BUMP_RIGHT_START);
			break;
		case OBJACTION_ATTACK:
			obj_flags_res_grabbed(true);
			action_by_interaction(OBJSTATE_ATTACKED_LEFT_START, OBJSTATE_ATTACKED_RIGHT_START);
			break;
		case OBJACTION_PICK_UP:
			obj_flags_res_grabbed(false);
			cpu_.a = rd8(wDirection);
			cpu_.a = alu_.and_(cpu_.a, cpu_.a);
			set_action_state(15 | (1 << 7),
				ZF ? OBJSTATE_PICKED_UP_RIGHT_START : OBJSTATE_PICKED_UP_LEFT_START);
			break;
		case OBJACTION_WOBBLE:
			action_wobble();
			break;
		case OBJACTION_VANISH_TOUCH:
			obj_flags_res_grabbed(true);
			set_action_state(15 | (1 << 7), OBJSTATE_VANISH_TOUCH);
			break;
		case OBJACTION_06:
			action_06(15 | (1 << 7));
			break;
		case OBJACTION_07:
			xor_a();
			set_action_state(0, OBJSTATE_18);
			break;
		case OBJACTION_VANISH_TREASURE:
			JUMP_TO(ADDR_VanishObject_NoSFX);
			return;
		case OBJACTION_STEP_ON:
			xor_a();
			wr8(wCurObjAction, 0);
			SET_HL(gb_, wCurObjFlags);
			wr8(wCurObjFlags, (uint8_t)(rd8(wCurObjFlags) | (1 << OBJFLAG_STEPPED_F)));
			break;
		case OBJACTION_0A:
			cpu_.a = 15 | (1 << 7);
			wr8(wCurObjAction, cpu_.a);
			break;
		case OBJACTION_FLATTEN:
		case OBJACTION_0C:
		case OBJACTION_0E:
		case OBJACTION_TELEPORT:
		case OBJACTION_11:
			action_hl_state(15 | (1 << 7), OBJSTATE_ACTIVATE);
			break;
		case OBJACTION_0D:
			action_hl_state(15 | (1 << 7), OBJSTATE_19);
			break;
		case OBJACTION_SMASH_ATTACK_WALKABLE:
			action_hl_state(2 | (1 << 7), OBJSTATE_VANISH_TOUCH);
			break;
		case OBJACTION_12:
			action_06(7 | (1 << 7));
			break;
		case OBJACTION_13:
			action_hl_state(15 | (1 << 7), OBJSTATE_SPECIAL_3);
			break;
		}
		RET();
	}

	void port_Func_62e57()
	{
		play_sfx(SFX_VANISH);
		wr8(wCurObjUpdateFunction + 1, (uint8_t)(ADDR_Func_4c860 >> 8));
		cpu_.a = (uint8_t)ADDR_Func_4c860;
		wr8(wCurObjUpdateFunction + 0, cpu_.a);
		SET_HL(gb_, wCurObjUpdateFunction - 1);
		gb_call(gb_, ADDR_Func_618b4);
		JUMP(HomeJumpRet);
	}

	void port__VanishObject2()
	{
		gb_call(gb_, ADDR__VanishObject);
		JUMP(HomeJumpRet);
	}

	// --- throwing a grabbed object ---

	// ThrowObject_Left / ThrowObject_Right: b bit 6 set for a full throw.
	// OBJ_VAR_2 = the throw strength (1 when holding up), state = the throw
	// state, action = 31 | (1 << 7).
	void throw_object(uint8_t state, uint8_t full_state)
	{
		play_sfx(SFX_THROW);
		uint16_t hl = wCurObjInteractionType;
		cpu_.a = alu_.or_(alu_.and_(rd8(hl), HEAVY_OBJ), OBJ_INTERACTION_00);
		wr8(hl, cpu_.a);
		cpu_.a = alu_.rla(cpu_.a);
		hl = (uint16_t)((hl & 0xff00) | OBJ_STATE_DURATION);
		cpu_.a = 0;
		wr8(hl++, 0);
		hl = (uint16_t)((hl & 0xff00) | alu_.inc((uint8_t)hl)); // inc l
		bool full = alu_.bit(cpu_.b, 6);
		if (full) {
			cpu_.b = 0x04;
		} else {
			cpu_.b = CF ? 0x02 : 0x03;
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_UP))
			cpu_.b = 0x01;
		cpu_.a = cpu_.b;
		wr8(hl++, cpu_.a); // OBJ_VAR_2
		xor_a();
		wr8(hl++, 0); // OBJ_VAR_3
		hl = (uint16_t)((hl & 0xff00) | alu_.inc((uint8_t)hl)); // inc l
		cpu_.a = full ? full_state : state;
		wr8(hl++, cpu_.a); // OBJ_STATE
		cpu_.a = 31 | (1 << 7);
		wr8(hl--, cpu_.a); // OBJ_ACTION
		SET_HL(gb_, hl);
		JUMP(HomeJumpRet);
	}

	void port_ThrowObject_Left() { throw_object(OBJSTATE_4E, OBJSTATE_FULL_THROW_LEFT_START); }
	void port_ThrowObject_Right() { throw_object(OBJSTATE_4F, OBJSTATE_FULL_THROW_RIGHT_START); }

	// Func_626da (moving left) / Func_62768 (moving right): a thrown object
	// moving sideways. While it's going down (wCurObjStateDuration set) it
	// checks the floor, and lands on a slope; it vanishes in water or sand, or
	// when it hits a ceiling or a wall. Otherwise it moves by OBJ_VAR_2 and
	// goes on to Func_627f0.
	// Func_62574 / Func_62605 do the same with other states for a ceiling
	// (ceil_state) or a wall (wall_state) and go on to Func_62690 without the
	// offscreen check.
	void thrown_sideways(bool left, uint16_t routine, uint8_t ceil_state,
		uint8_t wall_state, uint16_t next, uint8_t wall_ret)
	{
		uint16_t hl = wCurObjXPos + 1;
		cpu_.a = rd8(hl--);
		wr8(hXPosHi, cpu_.a);
		cpu_.a = rd8(hl--);
		wr8(hXPosLo, cpu_.a);
		SET_HL(gb_, hl);
		if (ld_and(wCurObjStateDuration)) {
			cpu_.a = rd8(hl--);
			wr8(hYPosHi, cpu_.a);
			cpu_.a = rd8(hl);
			wr8(hYPosLo, cpu_.a);
			SET_HL(gb_, hl);
			xor_a();
			wr8(wc1ca, FALSE);
			cpu_.a = alu_.inc(cpu_.a);
			wr8(wBlockFuncSpecialCollision, cpu_.a); // COLLISION_THROWN_OBJ
			gb_call_ret(gb_, ADDR_Func_352b, (uint16_t)(routine + 0x20));
			alu_.and_(cpu_.a, cpu_.a);
			cpu_.a = 0x03;
			wr8(hffa0, cpu_.a);
			if (!ZF) {
				JUMP_TO(left ? ADDR_Func_62d34 : ADDR_Func_62e0f);
				return;
			}
			if (ld_and(wIsInWaterOrSand))
				goto vanish;
		}

		neg_into_b(wCurObjCollBoxTop);
		set_hpos_sub(wCurObjYPos, cpu_.b);
		cpu_.a = COLLISION_THROWN_OBJ;
		wr8(wBlockFuncSpecialCollision, cpu_.a);
		gb_call_ret(gb_, ADDR_Func_358b, (uint16_t)(routine + 0x50));
		if (alu_.and_(cpu_.a, cpu_.a)) {
			st(wCurObjState, ceil_state);
			JUMP(HomeJumpRet);
			return;
		}
		set_hpos_sub(wCurObjYPos, 8);
		if (left)
			hxpos_left_edge(REG_HL(gb_));
		else
			hxpos_right_edge(REG_HL(gb_));
		cpu_.a = COLLISION_THROWN_OBJ;
		wr8(wBlockFuncSpecialCollision, cpu_.a);
		gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + wall_ret));
		if (alu_.and_(cpu_.a, 0x0f)) {
			st(wCurObjState, wall_state);
			JUMP(HomeJumpRet);
			return;
		}
		if (left)
			CALL(MoveObjectLeftByVar2);
		else
			CALL(MoveObjectRightByVar2);
		if (next == ADDR_Func_627f0)
			check_offscreen_grabbed(left);
		JUMP_TO(next);
		return;

	vanish:
		cpu_.a = OBJSTATE_VANISH_TOUCH;
		wr8(wCurObjState, cpu_.a);
		JUMP(HomeJumpRet);
	}

	void port_Func_626da()
	{
		thrown_sideways(true, ADDR_Func_626da, OBJSTATE_VANISH_TOUCH, OBJSTATE_VANISH_TOUCH,
			ADDR_Func_627f0, 0x77);
	}
	void port_Func_62768()
	{
		thrown_sideways(false, ADDR_Func_62768, OBJSTATE_VANISH_TOUCH, OBJSTATE_VANISH_TOUCH,
			ADDR_Func_627f0, 0x74);
	}
	void port_Func_62574()
	{
		thrown_sideways(true, ADDR_Func_62574, OBJSTATE_14, OBJSTATE_15,
			ADDR_Func_62690, 0x77);
	}
	void port_Func_62605()
	{
		thrown_sideways(false, ADDR_Func_62605, OBJSTATE_15, OBJSTATE_14,
			ADDR_Func_62690, 0x74);
	}

	// --- an object held by Wario ---

	// a grabbed object let go: it falls off sideways
	void drop_grabbed(uint8_t state)
	{
		xor_a();
		wr8(wGrabState, 0);
		play_sfx(SFX_01A);
		st(wCurObjVar2, 0x02);
		uint16_t hl = wCurObjAction;
		cpu_.a = 31 | (1 << 7);
		wr8(hl--, cpu_.a);
		cpu_.a = state;
		wr8(hl--, cpu_.a); // OBJ_STATE
		SET_HL(gb_, hl);
		JUMP(HomeJumpRet);
	}

	// hl = the Wario position byte pair after hl's; adds the signed offset v to
	// it into [hl] / [hl + 1], and returns hl + 2 (inc l).
	uint16_t picked_up_pos(uint16_t hl, uint8_t v, uint16_t wario)
	{
		cpu_.a = v;
		alu_.cp(cpu_.a, 0x80);
		bool negative = !CF;
		cpu_.a = alu_.add(rd8(wario + 1), v);
		wr8(hl++, cpu_.a);
		bool carry = CF;
		cpu_.a = rd8(wario + 0);
		if (negative && !carry)
			cpu_.a = alu_.dec(cpu_.a);
		else if (!negative && carry)
			cpu_.a = alu_.inc(cpu_.a);
		wr8(hl, cpu_.a);
		return (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1));
	}

	// Func_61f54 (held to the right) / Func_620a6 (held to the left)
	void grabbed_object(bool right)
	{
		uint16_t routine = right ? ADDR_Func_61f54 : ADDR_Func_620a6;
		uint8_t drop_state = right ? OBJSTATE_14 : OBJSTATE_15;
		uint8_t idle_state = right ? OBJSTATE_19 : OBJSTATE_18;
		st(wCurObjAction, (1 << NO_ACTIONS_F) | 1);

		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_STATE_MASK);
		alu_.cp(cpu_.a, GRAB_PICK_UP);
		if (ZF)
			goto pick_up;
		alu_.cp(cpu_.a, GRAB_IDLE);
		if (!ZF) {
			st(wCurObjState, OBJSTATE_12);
			JUMP(HomeJumpRet);
			return;
		}

		// GrabIdle: hold the object above Wario
		cpu_.a = rd8(wIsSmashAttacking);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *4
		cpu_.e = cpu_.a;
		cpu_.a = alu_.sub(30, cpu_.e);
		cpu_.e = cpu_.a;
		{
			uint16_t hl = wCurObjYPos;
			cpu_.a = alu_.sub(rd8(wWarioYPos + 1), cpu_.e);
			wr8(hl++, cpu_.a);
			cpu_.a = alu_.sbc(rd8(wWarioYPos + 0), 0);
			wr8(hl++, cpu_.a);
			if (right) {
				cpu_.a = alu_.add(rd8(wWarioXPos + 1), 7);
				wr8(hl++, cpu_.a);
				cpu_.a = alu_.adc(rd8(wWarioXPos + 0), 0);
			} else {
				cpu_.a = alu_.sub(rd8(wWarioXPos + 1), 7);
				wr8(hl++, cpu_.a);
				cpu_.a = alu_.sbc(rd8(wWarioXPos + 0), 0);
			}
			wr8(hl, cpu_.a);

			hl = wCurObjYPos;
			cpu_.a = rd8(hl++);
			wr8(hYPosLo, cpu_.a);
			cpu_.a = rd8(hl++);
			wr8(hYPosHi, cpu_.a);
			cpu_.a = rd8(hl++);
			wr8(hXPosLo, cpu_.a);
			cpu_.a = rd8(hl);
			wr8(hXPosHi, cpu_.a);
			SET_HL(gb_, hl);
		}
		xor_a();
		wr8(wc1ca, FALSE);
		gb_call_ret(gb_, ADDR_Func_352b, (uint16_t)(routine + 0x55));
		if (!alu_.and_(cpu_.a, cpu_.a) || ld_and(wc1ca)) {
			st(wCurObjState, idle_state);
			JUMP(HomeJumpRet);
			return;
		}
		{
			uint16_t hl = wCurObjYPos;
			cpu_.a = alu_.add(rd8(hl), 16);
			wr8(hl++, cpu_.a);
			SET_HL(gb_, hl);
			if (CF)
				wr8(hl, alu_.inc(rd8(hl)));
		}
		drop_grabbed(drop_state);
		return;

	pick_up:
		{
			uint16_t hl = wCurObjVar1;
			SET_HL(gb_, hl);
			wr8(hl, alu_.dec(rd8(hl)));
			if (!ZF) {
				JUMP(HomeJumpRet);
				return;
			}
			hl = (uint16_t)((hl & 0xff00) | alu_.inc((uint8_t)hl)); // inc l
			cpu_.a = rd8(hl++); // OBJ_VAR_2
			hl = (uint16_t)(rd8(hl) << 8 | cpu_.a); // OBJ_VAR_3
			cpu_.a = rd8(hl++);
			if (!alu_.and_(cpu_.a, cpu_.a)) {
				uint8_t l = (uint8_t)hl;
				for (int i = 0; i < 4; i++)
					l = alu_.dec(l);
				hl = (uint16_t)((hl & 0xff00) | l);
				cpu_.a = rd8(hl++);
			}
			wr8(wCurObjVar1, cpu_.a);
			cpu_.c = cpu_.a = rd8(hl++);
			cpu_.b = cpu_.a = rd8(hl++);
			cpu_.a = (uint8_t)hl;
			wr8(wCurObjVar2, cpu_.a);
			cpu_.a = (uint8_t)(hl >> 8);
			wr8(wCurObjVar3, cpu_.a);

			hl = picked_up_pos(hPickedUpObjPos, cpu_.c, wWarioYPos);
			hl = picked_up_pos(hl, cpu_.b, wWarioXPos);
			SET_HL(gb_, (uint16_t)(hl - 1));

			set_hpos_sub(hPickedUpObjPos, 4);
			if (right)
				hxpos_right_edge(REG_HL(gb_));
			else
				hxpos_left_edge(REG_HL(gb_));
			gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + (right ? 0x106 : 0x109)));
			if (!alu_.and_(cpu_.a, 0x0f)) {
				neg_into_b(wCurObjCollBoxTop);
				set_hpos_sub(hPickedUpObjPos, cpu_.b);
				hl = REG_HL(gb_);
				cpu_.a = rd8(hl++);
				wr8(hXPosLo, cpu_.a);
				cpu_.a = rd8(hl++);
				wr8(hXPosHi, cpu_.a);
				SET_HL(gb_, hl);
				gb_call_ret(gb_, ADDR_Func_358b, (uint16_t)(routine + (right ? 0x125 : 0x128)));
				if (!alu_.and_(cpu_.a, 0x0f)) {
					hl = hPickedUpObjPos;
					uint16_t de = wCurObjYPos;
					for (int i = 0; i < 4; i++) {
						cpu_.a = rd8(hl++);
						wr8(de, cpu_.a);
						if (i < 3)
							de = (uint16_t)((de & 0xff00) | alu_.inc((uint8_t)de)); // inc e
					}
					SET_HL(gb_, hl);
					SET_DE(gb_, de);
					JUMP(HomeJumpRet);
					return;
				}
			}
			// hit a wall or a ceiling
			cpu_.a = alu_.and_(rd8(wCurObjInteractionType), HEAVY_OBJ);
			if (!ZF) {
				drop_grabbed(drop_state);
				return;
			}
			cpu_.a = alu_.or_(cpu_.a, GRAB_UNK_02);
			wr8(wGrabState, cpu_.a);
			st(wCurObjState, idle_state);
			JUMP(HomeJumpRet);
		}
	}

	void port_Func_61f54() { grabbed_object(true); }
	void port_Func_620a6() { grabbed_object(false); }

	// wCurObjYPos/XPos = Wario's position, 30 (26 when smash attacking) up and
	// 7 to the right or left
	void hold_above_wario(bool right)
	{
		cpu_.a = rd8(wIsSmashAttacking);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.a = alu_.sub(30, cpu_.e);
		cpu_.e = cpu_.a;
		uint16_t hl = wCurObjYPos;
		cpu_.a = alu_.sub(rd8(wWarioYPos + 1), cpu_.e);
		wr8(hl++, cpu_.a);
		cpu_.a = alu_.sbc(rd8(wWarioYPos + 0), 0);
		wr8(hl++, cpu_.a);
		if (right) {
			cpu_.a = alu_.add(rd8(wWarioXPos + 1), 7);
			wr8(hl++, cpu_.a);
			cpu_.a = alu_.adc(rd8(wWarioXPos + 0), 0);
		} else {
			cpu_.a = alu_.sub(rd8(wWarioXPos + 1), 7);
			wr8(hl++, cpu_.a);
			cpu_.a = alu_.sbc(rd8(wWarioXPos + 0), 0);
		}
		wr8(hl, cpu_.a);
		SET_HL(gb_, hl);
	}

	void res_grabbed_at(uint16_t hl)
	{
		wr8(hl, (uint8_t)(rd8(hl) & ~(1 << OBJFLAG_GRABBED_F)));
	}

	// Func_621fb (held to the right) / Func_622bd (held to the left)
	void held_object(bool right)
	{
		uint16_t routine = right ? ADDR_Func_621fb : ADDR_Func_622bd;
		st(wCurObjAction, (1 << NO_ACTIONS_F) | 1);
		cpu_.a = rd8(wGrabState);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		alu_.cp(cpu_.a, GRAB_IDLE);
		if (!ZF) {
			alu_.cp(cpu_.a, GRAB_CHARGE_THROW);
			if (ZF) {
				st(wCurObjState, right ? OBJSTATE_4B : OBJSTATE_4A);
				JUMP(HomeJumpRet);
				return;
			}
			alu_.cp(cpu_.a, GRAB_THROW);
			if (ZF) {
				JUMP_TO(right ? ADDR_ThrowObject_Right : ADDR_ThrowObject_Left);
				return;
			}
			uint16_t hl = wCurObjFlags;
			res_grabbed_at(hl);
			hl = (uint16_t)((hl & 0xff00) | OBJ_STATE);
			SET_HL(gb_, hl);
			if (ld_and(wIsCrouching)) {
				st(wCurObjVar2, 0x01);
				cpu_.a = right ? OBJSTATE_15 : OBJSTATE_14;
			} else {
				cpu_.a = OBJSTATE_12;
			}
			wr8(hl, cpu_.a);
			JUMP(HomeJumpRet);
			return;
		}

		// GrabIdle
		hold_above_wario(right);
		neg_into_b(wCurObjCollBoxTop);
		set_hpos_sub(wCurObjYPos, cpu_.b);
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(hXPosHi, cpu_.a);
		SET_HL(gb_, hl);
		gb_call_ret(gb_, ADDR_Func_358b, (uint16_t)(routine + 0x7d));
		if (!alu_.and_(cpu_.a, right ? 0x0f : 0xff)) {
			set_hpos_sub(wCurObjYPos, 8);
			if (right)
				hxpos_right_edge(REG_HL(gb_));
			else
				hxpos_left_edge(REG_HL(gb_));
			gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + (right ? 0x9d : 0x9f)));
			if (!alu_.and_(cpu_.a, right ? 0xff : 0x0f)) {
				JUMP(HomeJumpRet);
				return;
			}
		}
		xor_a();
		wr8(wGrabState, 0);
		play_sfx(SFX_01A);
		st(wCurObjVar2, 0x02);
		hl = wCurObjAction;
		cpu_.a = 31 | (1 << 7);
		wr8(hl--, cpu_.a);
		cpu_.a = right ? OBJSTATE_14 : OBJSTATE_15;
		wr8(hl, cpu_.a); // OBJ_STATE
		hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
		SET_HL(gb_, hl);
		res_grabbed_at(hl);
		JUMP(HomeJumpRet);
	}

	void port_Func_621fb() { held_object(true); }
	void port_Func_622bd() { held_object(false); }

	// Func_6247b (charging a throw to the right) / Func_62382 (to the left)
	void charging_throw(bool right)
	{
		uint16_t routine = right ? ADDR_Func_6247b : ADDR_Func_62382;
		st(wCurObjAction, (1 << NO_ACTIONS_F) | 1);
		cpu_.a = rd8(wGrabState);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		alu_.cp(cpu_.a, GRAB_CHARGE_THROW);
		bool charge = ZF;
		if (!charge) {
			alu_.cp(cpu_.a, GRAB_THROW_FULLY_CHARGED);
			charge = ZF;
		}
		if (charge) {
			hold_above_wario(right);
			JUMP(HomeJumpRet);
			return;
		}
		alu_.cp(cpu_.a, GRAB_HOLD_FULL_CHARGE);
		if (ZF) {
			uint16_t hl = hPickedUpObjPos;
			cpu_.a = alu_.sub(rd8(wWarioYPos + 1), 30);
			wr8(hl++, cpu_.a);
			cpu_.a = alu_.sbc(rd8(wWarioYPos + 0), 0);
			wr8(hl++, cpu_.a);
			if (right) {
				cpu_.a = alu_.sub(rd8(wWarioXPos + 1), 5);
				wr8(hl++, cpu_.a);
				cpu_.a = alu_.sbc(rd8(wWarioXPos + 0), 0);
			} else {
				cpu_.a = alu_.add(rd8(wWarioXPos + 1), 5);
				wr8(hl++, cpu_.a);
				cpu_.a = alu_.adc(rd8(wWarioXPos + 0), 0);
			}
			wr8(hl++, cpu_.a);
			set_hpos_sub(hPickedUpObjPos, 4);
			hl = REG_HL(gb_);
			cpu_.a = rd8(hl++);
			wr8(hXPosLo, cpu_.a);
			cpu_.a = rd8(hl++);
			wr8(hXPosHi, cpu_.a);
			SET_HL(gb_, hl);
			gb_call_ret(gb_, ADDR_Func_3513, (uint16_t)(routine + 0x81));
			if (!alu_.and_(cpu_.a, 0x0f)) {
				hl = hPickedUpObjPos;
				uint16_t de = wCurObjYPos;
				for (int i = 0; i < 4; i++) {
					cpu_.a = rd8(hl++);
					wr8(de, cpu_.a);
					if (i < 3)
						de = (uint16_t)((de & 0xff00) | alu_.inc((uint8_t)de)); // inc e
				}
				SET_HL(gb_, hl);
				SET_DE(gb_, de);
				JUMP(HomeJumpRet);
				return;
			}
			xor_a();
			wr8(wGrabState, 0);
			play_sfx(SFX_01A);
		} else {
			alu_.cp(cpu_.a, GRAB_THROW);
			if (ZF) {
				JUMP_TO(right ? ADDR_ThrowObject_Right : ADDR_ThrowObject_Left);
				return;
			}
		}
		SET_HL(gb_, wCurObjFlags);
		res_grabbed_at(wCurObjFlags);
		st(wCurObjState, OBJSTATE_12);
		JUMP(HomeJumpRet);
	}

	void port_Func_6247b() { charging_throw(true); }
	void port_Func_62382() { charging_throw(false); }


	// The object update loop (UpdateObject and friends)

	enum { UPD_NORMAL, UPD_TEMPLE, UPD_FLOOR };

	// Return addresses (the bytes after each call, checked against the ROM).
	#define RET_TEMPLE_ACTION 0x5250 // UpdateObject_Temple: call DoObjectAction
	#define RET_TEMPLE_HCALL 0x5261  // UpdateObject_Temple: call hCallFunc
	#define RET_TEMPLE_ANIM 0x5264
	#define RET_TEMPLE_SCREEN 0x5267
	#define RET_NORMAL_ACTION 0x5461 // UpdateObject: call DoObjectAction
	#define RET_NORMAL_HCALL 0x5479  // UpdateObject: call hCallFunc
	#define RET_NORMAL_ANIM 0x547c
	#define RET_NORMAL_SCREEN 0x547f
	#define RET_FLOOR_HCALL 0x55b4  // UpdateObjectDuringFloorTransition: call hCallFunc
	#define RET_FLOOR_FRAME 0x55b7
	#define RET_FLOOR_SCREEN 0x55ba
	#define RET_TEMPLE_GRAB 0x530e  // UpdateObjects_Temple: call HandleGrabbedObjectCollisions
	#define RET_TEMPLE_LOOP 0x5316  // first call c, UpdateObject_Temple (then +7 each)
	#define RET_UPDATE_GRAB 0x5368  // UpdateObjects: call HandleGrabbedObjectCollisions
	#define RET_UPDATE_LOOP 0x5370  // first call c, UpdateObject
	#define RET_FLOOR_LOOP 0x53aa   // first call c, UpdateObjectDuringFloorTransition
	#define OBJFUNCS1_BANK 0x10     // BANK("Object Functions 1")
	#define OBJFUNCS4_BANK 0x13     // BANK("Object Functions 4")

	// ld a, [wCurObjId] / swap a / and $0f / or BANK("Object Functions 1") /
	// ldh [hCallFuncBank], a / (pointer from wCurObjUpdateFunction) / call hCallFunc
	void call_update_function(uint8_t bank, bool by_id, uint16_t ret)
	{
		if (by_id) {
			cpu_.a = alu_.swap(rd8(wCurObjId));
			cpu_.a = alu_.and_(cpu_.a, 0x0f);
			cpu_.a = alu_.or_(cpu_.a, OBJFUNCS1_BANK);
			bank = cpu_.a;
		} else {
			cpu_.a = bank;
		}
		wr8(hCallFuncBank, bank);
		cpu_.a = rd8(wCurObjUpdateFunction + 0);
		wr8(hCallFuncPointer + 0, cpu_.a);
		cpu_.a = rd8(wCurObjUpdateFunction + 1);
		wr8(hCallFuncPointer + 1, cpu_.a);
		uint8_t entry_bank = rd8(wROMBank);
		gb_call_ret(gb_, hCallFunc, ret);
		if (gb_->test.active && gb_->rom_bank != entry_bank)
			gb_->test.unrealistic = true;
	}

	void update_object(int kind)
	{
		gb_push16(gb_, REG_HL(gb_));
		cpu_.l = alu_.inc(cpu_.l);
		cpu_.a = alu_.rla(cpu_.a);
		wr8(wCurObjFlags, cpu_.a);
		uint16_t hl = REG_HL(gb_);
		for (int n = 1; n < OBJ_STRUCT_LENGTH - 1; n++) {
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(wCurObj + n), cpu_.a);
		}
		cpu_.a = rd8(hl);
		wr8((uint16_t)(wCurObj + OBJ_STRUCT_LENGTH - 1), cpu_.a);
		SET_HL(gb_, hl);

		switch (kind) {
		case UPD_NORMAL:
			gb_call_ret(gb_, ADDR_DoObjectAction, RET_NORMAL_ACTION);
			call_update_function(0, true, RET_NORMAL_HCALL);
			gb_call_ret(gb_, ADDR_UpdateObjectAnimation, RET_NORMAL_ANIM);
			gb_call_ret(gb_, ADDR_CheckObjectOnScreen, RET_NORMAL_SCREEN);
			break;
		case UPD_TEMPLE:
			gb_call_ret(gb_, ADDR_DoObjectAction, RET_TEMPLE_ACTION);
			call_update_function(OBJFUNCS4_BANK, false, RET_TEMPLE_HCALL);
			gb_call_ret(gb_, ADDR_UpdateObjectAnimation, RET_TEMPLE_ANIM);
			gb_call_ret(gb_, ADDR_CheckObjectOnScreen, RET_TEMPLE_SCREEN);
			break;
		case UPD_FLOOR:
			SET_HL(gb_, wCurObjSubState);
			if (alu_.bit(rd8(wCurObjSubState), OBJSUBFLAG_UNINITIALISED_F))
				call_update_function(0, true, RET_FLOOR_HCALL);
			gb_call_ret(gb_, ADDR_UpdateObjectFrame, RET_FLOOR_FRAME);
			gb_call_ret(gb_, ADDR_CheckObjectOnScreen, RET_FLOOR_SCREEN);
			break;
		}

		cpu_.b = rd8(wSCY);
		cpu_.a = alu_.add(rd8(wCurObjYPos + 0), OAM_Y_OFS);
		cpu_.a = alu_.sub(cpu_.a, cpu_.b);
		wr8(wCurObjScreenYPos, cpu_.a);
		cpu_.b = rd8(wSCX);
		cpu_.a = alu_.add(rd8(wCurObjXPos), OAM_X_OFS);
		cpu_.a = alu_.sub(cpu_.a, cpu_.b);
		wr8(wCurObjScreenYPos + 1, cpu_.a);
		SET_HL(gb_, wCurObjScreenYPos + 2);

		if (kind == UPD_TEMPLE) {
			SET_HL(gb_, wSCYShake);
			cpu_.a = alu_.add(rd8(wHiddenFigureSCYShake), rd8(wSCYShake));
			wr8(wSCY, cpu_.a);
			cpu_.a = rd8(wc08b);
			wr8(wSCX, cpu_.a);
		}
		SET_HL(gb_, gb_pop16(gb_));

		hl = REG_HL(gb_);
		cpu_.a = alu_.and_(rd8(wCurObjFlags), 0xff ^ OBJFLAG_STEPPED);
		wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		cpu_.l = alu_.inc(cpu_.l);
		cpu_.l = alu_.inc(cpu_.l);
		hl = REG_HL(gb_);
		static const uint16_t fields1[] = {
			wCurObjYPos + 0, wCurObjYPos + 1, wCurObjXPos + 0, wCurObjXPos + 1,
			wCurObjId, wCurObjInteractionType,
			wCurObjCollBoxTop, wCurObjCollBoxBottom, wCurObjCollBoxLeft, wCurObjCollBoxRight,
			wCurObjScreenYPos, wCurObjScreenXPos, wCurObjFrame,
			wCurObjOAMPtr + 0, wCurObjOAMPtr + 1, wCurObjFramesetPtr + 0, wCurObjFramesetPtr + 1,
			wCurObjFrameDuration + 0, wCurObjFrameDuration + 1, wCurObjStateDuration,
			wCurObjVar1, wCurObjVar2, wCurObjVar3, wCurObjSubState, wCurObjState, wCurObjAction,
		};
		for (size_t i = 0; i < sizeof(fields1) / sizeof(fields1[0]); i++) {
			cpu_.a = rd8(fields1[i]);
			wr8(hl++, cpu_.a);
		}
		SET_HL(gb_, hl);
		cpu_.l = alu_.inc(cpu_.l);
		hl = REG_HL(gb_);
		cpu_.a = rd8(wCurObjUpdateFunction + 0);
		wr8(hl++, cpu_.a);
		cpu_.a = rd8(wCurObjUpdateFunction + 1);
		wr8(hl, cpu_.a);
		SET_HL(gb_, hl);
	}

	void port_UpdateObject() { update_object(UPD_NORMAL); }
	void port_UpdateObject_Temple() { update_object(UPD_TEMPLE); }
	void port_UpdateObjectDuringFloorTransition() { update_object(UPD_FLOOR); }

	// ld hl, wObj1Flags / ld a, [hl] / rra / call c, fn, then ld l, LOW(wObjNFlags)
	// for the others
	void update_all(uint16_t fn, uint16_t ret)
	{
		SET_HL(gb_, wObj1Flags);
		for (int n = 0; n < NUM_OBJECTS; n++) {
			if (n)
				cpu_.l = (uint8_t)(wObj1Flags + n * OBJ_STRUCT_SIZE);
			cpu_.a = alu_.rra(rd8(REG_HL(gb_)));
			if (cpu_.f & FLAG_C)
				gb_call_ret(gb_, fn, (uint16_t)(ret + n * 7));
		}
	}

	void update_objects_temple()
	{
		gb_call_ret(gb_, ADDR_HandleGrabbedObjectCollisions, RET_TEMPLE_GRAB);
		update_all(ADDR_UpdateObject_Temple, RET_TEMPLE_LOOP);
	}

	void port_UpdateObjects_Temple() { update_objects_temple(); }

	void port_UpdateObjects()
	{
		cpu_.e = rd8(wPowerUpLevel);
		cpu_.a = alu_.rla(rd8(wTransformation));
		if (cpu_.f & FLAG_C)
			cpu_.e = SUPER_JUMP_SLAM_OVERALLS;
		cpu_.a = cpu_.e;
		wr8(wEffectivePowerUp, cpu_.a);

		cpu_.a = rd8(wLevel);
		alu_.cp(cpu_.a, THE_TEMPLE);
		if (cpu_.f & FLAG_Z) {
			update_objects_temple(); // jr z, UpdateObjects_Temple
			return;
		}
		cpu_.a = rd8(wIsFloorTransition);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			update_all(ADDR_UpdateObjectDuringFloorTransition, RET_FLOOR_LOOP);
			return;
		}
		gb_call_ret(gb_, ADDR_HandleGrabbedObjectCollisions, RET_UPDATE_GRAB);
		update_all(ADDR_UpdateObject, RET_UPDATE_LOOP);
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::ObjectEngine;

PORT_EX(DrawObjects_NoPriority, 0, 0) { ObjectEngine(gb).port_DrawObjects_NoPriority(); }
PORT_EX(DrawObjects_WithPriority, 0, 0) { ObjectEngine(gb).port_DrawObjects_WithPriority(); }
PORT_EX(CheckObjectOnScreen, 0, 0) { ObjectEngine(gb).port_CheckObjectOnScreen(); }
PORT_EX(ClearObjects, 0, 0) { ObjectEngine(gb).port_ClearObjects(); }
PORT_EX(ClearObjDataPointers, 0, 0) { ObjectEngine(gb).port_ClearObjDataPointers(); }
PORT_EX(Func_62d57, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62d57(); }
PORT_EX(Func_62e31, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62e31(); }
PORT_EX(Func_62d34, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62d34(); }
PORT_EX(Func_62e0f, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62e0f(); }
PORT_EX(Func_62ec3, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62ec3(); }
PORT_EX(Func_62faa, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62faa(); }
PORT_EX(_CountActiveTransientObjects, 0, 0) { ObjectEngine(gb).port__CountActiveTransientObjects(); }
PORT_EX(Func_6284f, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_6284f(); }
PORT_EX(Func_628a7, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_628a7(); }
PORT_EX(Func_628ea, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_628ea(); }
PORT_EX(Func_62e74, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62e74(); }
PORT_EX(Func_62e8a, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62e8a(); }
PORT_EX(Func_62e8f, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62e8f(); }
PORT_EX(Func_62ea5, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62ea5(); }
PORT_EX(Func_62eaa, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62eaa(); }
PORT_EX(Func_62ec0, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62ec0(); }
PORT_EX(Func_62f5b, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62f5b(); }
PORT_EX(Func_62f71, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62f71(); }
PORT_EX(Func_62f76, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62f76(); }
PORT_EX(Func_62f8c, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62f8c(); }
PORT_EX(Func_62f91, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62f91(); }
PORT_EX(Func_62fa7, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62fa7(); }
PORT_EX(Func_6283a, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_6283a(); }
PORT_EX(Func_62840, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62840(); }
PORT_EX(Func_62892, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62892(); }
PORT_EX(Func_62898, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62898(); }
PORT_EX(Func_62908, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62908(); }
PORT_EX(Func_62926, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62926(); }
PORT_EX(Func_629a6, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_629a6(); }
PORT_EX(_ObjState_StandingFall, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port__ObjState_StandingFall(); }
PORT_EX(_ObjState_Fall, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port__ObjState_Fall(); }
PORT_EX(Func_627f0, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_627f0(); }
PORT_EX(Func_62690, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62690(); }
PORT_EX(Func_6303f, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_6303f(); }
PORT_EX(Func_63050, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_63050(); }
PORT_EX(Func_6305f, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_6305f(); }
PORT_EX(Func_62ae5, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62ae5(); }
PORT_EX(Func_62bce, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62bce(); }
PORT_EX(_ObjState_BumpRight, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port__ObjState_BumpRight(); }
PORT_EX(_ObjState_BumpLeft, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port__ObjState_BumpLeft(); }
PORT_EX(Func_62ca8, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62ca8(); }
PORT_EX(Func_62d86, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62d86(); }
PORT_EX(Func_62c9f, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62c9f(); }
PORT_EX(Func_62d7d, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62d7d(); }
PORT_EX(Func_618b4, 0, 0) { ObjectEngine(gb).port_Func_618b4(); }
PORT_EX(VanishObject_NoSFX, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_VanishObject_NoSFX(); }
PORT_EX(_VanishObject, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port__VanishObject(); }
PORT_EX(InitObjectVars_FromLevelStart, 0, 0) { ObjectEngine(gb).port_InitObjectVars_FromLevelStart(); }
PORT_EX(InitObjectVars_FromTransition, 0, 0) { ObjectEngine(gb).port_InitObjectVars_FromTransition(); }
PORT_EX(_ApplyObjYMovement, 0, 0) { ObjectEngine(gb).port__ApplyObjYMovement(); }
PORT_EX(_ApplyObjXMovement, 0, 0) { ObjectEngine(gb).port__ApplyObjXMovement(); }
PORT_EX(_ApplyObjYMovement_Loop, 0, 0) { ObjectEngine(gb).port__ApplyObjYMovement_Loop(); }
PORT_EX(_ApplyObjXMovement_Loop, 0, 0) { ObjectEngine(gb).port__ApplyObjXMovement_Loop(); }
PORT_EX(Func_63209, 0, 0) { ObjectEngine(gb).port_Func_63209(); }
PORT_EX(_ApplyObjSetMovement, 0, 0) { ObjectEngine(gb).port__ApplyObjSetMovement(); }
PORT_EX(Func_632ac, 0, 0) { ObjectEngine(gb).port_Func_632ac(); }
PORT_EX(HandleGrabbedObjectCollisions, 0, 0) { ObjectEngine(gb).port_HandleGrabbedObjectCollisions(); }
PORT_EX(DoObjectAction, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_DoObjectAction(); }
PORT_EX(Func_62e57, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62e57(); }
PORT_EX(_VanishObject2, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port__VanishObject2(); }
PORT_EX(ThrowObject_Left, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_ThrowObject_Left(); }
PORT_EX(ThrowObject_Right, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_ThrowObject_Right(); }
PORT_EX(Func_626da, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_626da(); }
PORT_EX(Func_62768, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62768(); }
PORT_EX(Func_62574, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62574(); }
PORT_EX(Func_62605, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62605(); }
PORT_EX(Func_61f54, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_61f54(); }
PORT_EX(Func_620a6, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_620a6(); }
PORT_EX(Func_621fb, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_621fb(); }
PORT_EX(Func_622bd, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_622bd(); }
PORT_EX(Func_6247b, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_6247b(); }
PORT_EX(Func_62382, HOOK_MANUAL_RET, 0) { ObjectEngine(gb).port_Func_62382(); }
PORT_EX(UpdateObject, 0, 0) { ObjectEngine(gb).port_UpdateObject(); }
PORT_EX(UpdateObject_Temple, 0, 0) { ObjectEngine(gb).port_UpdateObject_Temple(); }
PORT_EX(UpdateObjectDuringFloorTransition, 0, 0) { ObjectEngine(gb).port_UpdateObjectDuringFloorTransition(); }
PORT_EX(UpdateObjects_Temple, 0, 0) { ObjectEngine(gb).port_UpdateObjects_Temple(); }
PORT_EX(UpdateObjects, 0, 0) { ObjectEngine(gb).port_UpdateObjects(); }
