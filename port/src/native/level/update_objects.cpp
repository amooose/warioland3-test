// The object engine (src/engine/level/update_objects.asm): drawing the
// objects, loading and unloading them off screen, the shared object states
// (falling, thrown, bumped, knocked, grabbed, vanishing), object movement
// tables and their actions.
//
// Objects (C++) and register-level C call these. It's plain C++
// (docs/HANDOFF.md); registers are left only where a hook takes an input in
// one or the objects read what it leaves.
#include "../asm_module.hpp"
#include "../home/object_mechanics.hpp"
#include "../home/memory.hpp"
#include "object.hpp"

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
			uint16_t obj = static_cast<uint16_t>(wObj1Flags + n * OBJ_STRUCT_SIZE);
			uint8_t flags = rd8(obj) & (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_INVISIBLE | OBJFLAG_PRIORITY);
			if (flags == match)
				home::update_obj_sprite(gb_, static_cast<uint16_t>(wObj1Id + n * OBJ_STRUCT_SIZE),
					static_cast<uint16_t>(wObj1ScreenYPos + n * OBJ_STRUCT_SIZE));
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

	// One of CheckObjectOnScreen's range checks of a position against the
	// camera's: pos + add must be past the camera (at it counts as in
	// range; a carry out of 16 bits is in range if wrap_in, else it goes on
	// with the wrapped value), then pos + add - sub (in range if it goes
	// below 0) must be below the camera + span.
	static bool in_range(uint16_t pos, uint16_t cam, uint8_t add, uint8_t span, uint8_t sub, bool wrap_in)
	{
		uint32_t p = pos + add;
		if (p > 0xffff) {
			if (wrap_in)
				return true;
			p &= 0xffff;
		} else if (p < cam) {
			return false;
		} else if (p == cam) {
			return true;
		}
		if (p < sub)
			return true;
		return static_cast<uint16_t>(p - sub) < static_cast<uint16_t>(cam + span);
	}

	// the camera positions are big-endian
	uint16_t camera(uint16_t addr) const { return static_cast<uint16_t>(rd8(addr) << 8 | rd8(addr + 1)); }

	// seems to be a function to load/unload objects
	// that are inside and outside the screen
	void check_object_on_screen()
	{
		uint16_t y = mem_.word(wCurObjYPos);
		uint16_t x = mem_.word(wCurObjXPos);
		uint16_t cam_y = camera(wCameraSCY);
		if (in_range(y, cam_y, 24, SCREEN_HEIGHT_PX, 56, false)) {
			if (in_range(x, camera(wCameraSCX), 24, SCREEN_WIDTH_PX, 0x30, true))
				wr8(wCurObjFlags, rd8(wCurObjFlags) | OBJFLAG_ON_SCREEN);
			else
				wr8(wCurObjFlags, rd8(wCurObjFlags) & ~OBJFLAG_ON_SCREEN);
		} else if ((rd8(wCurObjState) & 0xfe) != OBJSTATE_48) {
			wr8(wCurObjFlags, rd8(wCurObjFlags) & ~OBJFLAG_ON_SCREEN);
		}

		// whether the object is far enough outside the screen to unload
		// (a negative position always is)
		if (!(x & 0x8000) && !(y & 0x8000) && in_range(y, cam_y, 92, 0x90, 0xcb, true)
			&& in_range(x, camera(wCameraSCX), 0x6f, SCREEN_WIDTH_PX, 0xde, true))
			return;

		uint8_t sub = rd8(wCurObjSubState) & (0xf | OBJSUBFLAG_UNK_4);
		if (sub >= (0x0 | OBJSUBFLAG_UNK_4))
			return;
		// OBJSUBFLAG_UNK_4 is not set
		if (sub) {
			routines_.in_bank(BANK_DespawnObject, ADDR_DespawnObject, [&] { despawn_object(gb_, wCurObjUnk02); });
			return;
		}
		wr8(wCurObjFlags, 0);
	}

	// fills wObjects' first n bytes (0: 256) with value
	void clear_objects(uint8_t value, uint8_t n)
	{
		uint16_t addr = wObjects;
		do
			wr8(addr++, value);
		while (--n);
	}

	void clear_obj_data_pointers(uint8_t value)
	{
		for (uint16_t addr = wObjDataPointers; addr < wObjDataPointersEnd; addr++)
			wr8(addr, value);
	}

	// --- the shared states, plain ---
	//
	// The objects reach these through home's homejp wrappers (Func_3191 and
	// the like), which push the caller's bank for HomeJumpRet to restore;
	// they read no register on entry unless one is passed below, and their
	// callers none after them (live.py).

	bool heavy() const { return rd8(wCurObjInteractionType) & HEAVY_OBJ; }
	// the object's position (little-endian)
	uint16_t obj_y() const { return mem_.word(wCurObjYPos); }
	uint16_t obj_x() const { return mem_.word(wCurObjXPos); }
	void set_obj_y(uint16_t v) { mem_.word(wCurObjYPos) = v; }
	void set_obj_x(uint16_t v) { mem_.word(wCurObjXPos) = v; }
	void set_hpos_y(uint16_t v) { wr8(hYPosLo, static_cast<uint8_t>(v)); wr8(hYPosHi, static_cast<uint8_t>(v >> 8)); }
	void set_hpos_x(uint16_t v) { wr8(hXPosLo, static_cast<uint8_t>(v)); wr8(hXPosHi, static_cast<uint8_t>(v >> 8)); }
	uint16_t hpos_y() const { return static_cast<uint16_t>(rd8(hYPosHi) << 8 | rd8(hYPosLo)); }
	// hPos = the object's position
	void hpos_at_obj() { set_hpos_y(obj_y()); set_hpos_x(obj_x()); }
	// the x of its left edge (the box's left is negative) and right edge
	uint16_t left_edge() const { return static_cast<uint16_t>(obj_x() - static_cast<uint8_t>(-rd8(wCurObjCollBoxLeft))); }
	uint16_t right_edge() const { return static_cast<uint16_t>(obj_x() + rd8(wCurObjCollBoxRight)); }
	uint16_t edge(bool left) const { return left ? left_edge() : right_edge(); }
	// its top (the box's top is negative)
	uint16_t top() const { return static_cast<uint16_t>(obj_y() - static_cast<uint8_t>(-rd8(wCurObjCollBoxTop))); }
	void res_flag(uint8_t bit) { wr8(wCurObjFlags, rd8(wCurObjFlags) & ~(1 << bit)); }

	// A block check through home's wrapper (Func_3513 and the like),
	// returning to ret as the asm's call does: the block function's result
	uint8_t block(uint16_t wrapper, uint16_t ret)
	{
		gb_call_ret(gb_, wrapper, ret);
		return cpu_.a;
	}

	// A velocity table's next entry: the one at index's offset (an 8-bit
	// add, within the table's page; heavy objects' entries `heavy_ofs` on).
	// $80 ends a table, which then repeats its last entry; otherwise the
	// index goes on.
	uint8_t table_step(uint16_t index, uint16_t table, uint8_t heavy_ofs)
	{
		uint8_t c = static_cast<uint8_t>(rd8(index) + static_cast<uint8_t>(table));
		if (heavy())
			c = static_cast<uint8_t>(c + heavy_ofs);
		uint16_t page = table & 0xff00;
		uint8_t v = rd8(static_cast<uint16_t>(page | c));
		if (v == 0x80)
			v = rd8(static_cast<uint16_t>(page | static_cast<uint8_t>(c - 1)));
		else
			wr8(index, static_cast<uint8_t>(rd8(index) + 1));
		return v;
	}

	// Called plain (object_state, below), with the bank switched around it,
	// rather than entered by its homejp wrapper
	bool plain_ = false;

	// The end of a state entered by homejp; a plain call just returns
	void home_jump_ret()
	{
		if (!plain_)
			JUMP(HomeJumpRet);
	}

	// The state at `state` by the y velocity in hffa0: below 2, 3, 4, or
	// more; then HomeJumpRet.
	void set_state_by_y_vel(uint16_t state, bool left)
	{
		static const uint8_t states[2][4] = {
			{OBJSTATE_STUN_RIGHT_START, OBJSTATE_21, OBJSTATE_23, OBJSTATE_25},
			{OBJSTATE_STUN_LEFT_START, OBJSTATE_20, OBJSTATE_22, OBJSTATE_24},
		};
		uint8_t v = rd8(hffa0);
		int i = v < 2 ? 0 : v < 3 ? 1 : v < 4 ? 2 : 3;
		wr8(state, states[left][i]);
		home_jump_ret();
	}


	// Lands at hPos's y, with OBJ_VAR_2 by the slope it's on: set (less one
	// for the right slope, which the left fall lands on) on the slope it
	// falls along, kept on the other, else counted down to 0; then the
	// state by its y velocity.
	void land_by_slope(bool left)
	{
		wr8(wCurObjYPos + 0, rd8(hYPosLo));
		wr8(wCurObjYPos + 1, rd8(hYPosHi));
		uint8_t slope = rd8(wObjSlopeInteraction) & 0x0f;
		if (slope != (left ? RIGHT_SLOPE : LEFT_SLOPE)) {
			if (slope == (left ? 0x02 : 0x01))
				wr8(wCurObjVar2, left ? static_cast<uint8_t>(slope - 1) : slope);
			else if (rd8(wCurObjVar2))
				wr8(wCurObjVar2, static_cast<uint8_t>(rd8(wCurObjVar2) - 1));
		}
		set_state_by_y_vel(wCurObjState, left);
	}


	// moves the object var2 to the side (home's MoveObject*ByVar2)
	void move_x_by_var2(bool left)
	{
		uint16_t x = obj_x();
		set_obj_x(static_cast<uint16_t>(left ? x - rd8(wCurObjVar2) : x + rd8(wCurObjVar2)));
	}

	// let go once it's far enough off screen on the side it moves to
	void let_go_off_screen(bool left)
	{
		uint8_t x = static_cast<uint8_t>(rd8(wCurObjScreenXPos) + 0x28);
		if (left ? x >= 0xf0 : x < 0x10)
			res_flag(OBJFLAG_GRABBED_F);
	}

	// An object thrown to the left (Func_62ec3) or right (Func_62faa), up by
	// the table of y movements at table (indexed by OBJ_VAR_3, +$10 if
	// heavy; its entries are negative) and sideways by OBJ_VAR_2, until it
	// hits something or OBJ_STATE_DURATION runs out.
	void thrown_object_movement(bool left, uint16_t table)
	{
		uint16_t routine = left ? ADDR_Func_62ec3 : ADDR_Func_62faa;
		uint8_t v = table_step(wCurObjVar3, table, 0x10);
		if (v) {
			// an add with the carry taken as a negative's
			uint8_t lo = rd8(wCurObjYPos);
			wr8(wCurObjYPos, static_cast<uint8_t>(lo + v));
			if (lo + v < 0x100)
				wr8(wCurObjYPos + 1, static_cast<uint8_t>(rd8(wCurObjYPos + 1) - 1));
		}
		move_x_by_var2(left);
		let_go_off_screen(left);

		// its top, then 8 above its feet at the side it moves to
		set_hpos_y(top());
		set_hpos_x(obj_x());
		bool collided = block(ADDR_Func_358b, static_cast<uint16_t>(routine + 0x50)) & 0x0f;
		if (!collided) {
			set_hpos_y(static_cast<uint16_t>(obj_y() - 8));
			set_hpos_x(edge(left));
			collided = block(ADDR_Func_3513, static_cast<uint16_t>(routine + (left ? 0x73 : 0x70))) & 0x0f;
		}
		if (!collided) {
			uint8_t d = static_cast<uint8_t>(rd8(wCurObjStateDuration) - 1);
			wr8(wCurObjStateDuration, d);
			if (d) {
				home_jump_ret();
				return;
			}
		}
		// stops when it lands or hits something, the other way after a hit
		wr8(wCurObjState, (collided == left) ? OBJSTATE_27 : OBJSTATE_26);
		res_flag(OBJFLAG_NO_COLLISION_F);
		home_jump_ret();
	}

	// The thrown states' entries jump on into these with bc set, as the
	// asm's jr does (a hook each, so the fuzzer checks each part alone)
	void thrown(bool left, uint16_t table)
	{
		if (plain_) {
			thrown_object_movement(left, table);
			return;
		}
		SET_BC(gb_, table);
		JUMP_TO(left ? ADDR_Func_62ec3 : ADDR_Func_62faa);
	}

	// bc = the y movement table
	void port_Func_62ec3() { thrown_object_movement(true, REG_BC(gb_)); }
	void port_Func_62faa() { thrown_object_movement(false, REG_BC(gb_)); }

	// the number of objects with OBJFLAG_ACTIVE and OBJFLAG_TRANSIENT
	uint8_t count_active_transient_objects()
	{
		uint8_t n = 0;
		for (int i = 0; i < NUM_OBJECTS; i++)
			if ((rd8(static_cast<uint16_t>(wObj1Flags + i * OBJ_STRUCT_SIZE)) & (OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT))
				== (OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT))
				n++;
		return n;
	}

	// output: e (and d the mask, hl untouched: Anonster writes through it on)
	void port__CountActiveTransientObjects()
	{
		cpu_.e = count_active_transient_objects();
		cpu_.d = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	}

	// y += the signed v
	void add_signed_y(uint8_t v) { set_obj_y(static_cast<uint16_t>(obj_y() + static_cast<int8_t>(v))); }

	// Moves the current object dx pixels right (or left), and along the arc
	// in Data_60180 (Data_601a0 if heavy), indexed by OBJ_VAR_3.
	void move_obj_in_arc(bool right, uint8_t dx)
	{
		uint16_t x = obj_x();
		set_obj_x(static_cast<uint16_t>(right ? x + dx : x - dx));
		uint8_t v = table_step(wCurObjVar3, ADDR_Data_60180,
			static_cast<uint8_t>(ADDR_Data_601a0 - ADDR_Data_60180));
		wr8(hffa0, v);
		add_signed_y(v);
		no_actions();
		home_jump_ret();
	}

	void no_actions() { wr8(wCurObjAction, (1 << NO_ACTIONS_F) | 1); } // no_actions_for 1


	// When there's no floor under it (Func_3543): `state`
	void fall_if_no_floor(uint16_t routine, uint8_t state)
	{
		hpos_at_obj();
		if (!block(ADDR_Func_3543, static_cast<uint16_t>(routine + 0x12)))
			wr8(wCurObjState, state);
		home_jump_ret();
	}

	void port_Func_628ea() { fall_if_no_floor(ADDR_Func_628ea, OBJSTATE_12); }
	void port_Func_62908() { fall_if_no_floor(ADDR_Func_62908, OBJSTATE_STANDING_FALL_START); }

	// --- thrown object state entries ---

	// the state, OBJ_VAR_3 cleared and OBJ_STATE_DURATION by weight
	void set_thrown_state(uint8_t state, uint8_t light, uint8_t heavy_duration)
	{
		wr8(wCurObjState, state);
		wr8(wCurObjVar3, 0);
		wr8(wCurObjStateDuration, heavy() ? heavy_duration : light);
	}

	void port_Func_62e74() { set_thrown_state(OBJSTATE_54, 0x06, 0x02); thrown(true, ADDR_Data_60300); }
	void port_Func_62e8a() { thrown(true, ADDR_Data_60300); }
	void port_Func_62e8f() { set_thrown_state(OBJSTATE_50, 0x04, 0x01); thrown(true, ADDR_Data_602c0); }
	void port_Func_62ea5() { thrown(true, ADDR_Data_602c0); }
	void port_Func_62eaa() { set_thrown_state(OBJSTATE_52, 0x05, 0x02); thrown(true, ADDR_Data_602e0); }
	void port_Func_62ec0() { thrown(true, ADDR_Data_602e0); }
	void port_Func_62f5b() { set_thrown_state(OBJSTATE_55, 0x06, 0x02); thrown(false, ADDR_Data_60300); }
	void port_Func_62f71() { thrown(false, ADDR_Data_60300); }
	void port_Func_62f76() { set_thrown_state(OBJSTATE_51, 0x04, 0x01); thrown(false, ADDR_Data_602c0); }
	void port_Func_62f8c() { thrown(false, ADDR_Data_602c0); }
	void port_Func_62f91() { set_thrown_state(OBJSTATE_53, 0x05, 0x02); thrown(false, ADDR_Data_602e0); }
	void port_Func_62fa7() { thrown(false, ADDR_Data_602e0); }

	// --- arcs ---

	// OBJ_VAR_2, times 4 if `garlic` and the power up is at least GARLIC
	uint8_t arc_dx(bool garlic)
	{
		uint8_t v = rd8(wCurObjVar2);
		if (garlic && rd8(wEffectivePowerUp) >= GARLIC)
			v = static_cast<uint8_t>(v << 2);
		return v;
	}

	void port_Func_6283a() { move_obj_in_arc(true, arc_dx(false)); }
	void port_Func_62840() { move_obj_in_arc(true, arc_dx(true)); }
	void port_Func_62892() { move_obj_in_arc(false, arc_dx(false)); }
	void port_Func_62898() { move_obj_in_arc(false, arc_dx(true)); }

	// --- falling ---

	// the next y velocity from the light or heavy table, into hffa0
	uint8_t obj_y_vel(uint16_t light, uint16_t heavy_table)
	{
		uint8_t v = table_step(wCurObjVar3, light, static_cast<uint8_t>(heavy_table - light));
		wr8(hffa0, v);
		return v;
	}

	// The end of a fall: checks the floor (Func_352b, returning to ret) and
	// lands in the state land_state, or on a slope, or vanishes in water/sand.
	void obj_fall_and_land(uint16_t ret, uint8_t land_state)
	{
		hpos_at_obj();
		wr8(wObjSlopeInteraction, 0);
		if (!block(ADDR_Func_352b, ret)) {
			if (rd8(wIsInWaterOrSand))
				wr8(wCurObjState, OBJSTATE_VANISH_TOUCH);
			home_jump_ret();
			return;
		}
		wr8(wCurObjYPos + 0, rd8(hYPosLo));
		wr8(wCurObjYPos + 1, rd8(hYPosHi));
		uint8_t slope = rd8(wObjSlopeInteraction);
		if (!slope) {
			wr8(wCurObjState, land_state);
			home_jump_ret();
			return;
		}
		wr8(wCurObjVar2, 0x02);
		set_state_by_y_vel(wCurObjState, (slope & 0x0f) == 0x01);
	}

	void port_Func_62926()
	{
		add_signed_y(obj_y_vel(ADDR_Data_60040, ADDR_Data_60060));
		obj_fall_and_land(ADDR_Func_62926 + 0x48, OBJSTATE_0F);
	}

	// up, by a table of negative entries
	void port_Func_629a6()
	{
		set_obj_y(static_cast<uint16_t>(obj_y() + obj_y_vel(ADDR_Data_602a0, ADDR_Data_602b0) - 0x100));
		home_jump_ret();
	}

	void port__ObjState_StandingFall()
	{
		// speed up animation by 3x
		if (rd8(wCurObjFrameDuration) >= 3)
			wr8(wCurObjFrameDuration, static_cast<uint8_t>(rd8(wCurObjFrameDuration) - 3));
		add_signed_y(obj_y_vel(ADDR_StandingFallYVel_Light, ADDR_StandingFallYVel_Heavy));
		obj_fall_and_land(ADDR__ObjState_StandingFall + 0x53, OBJSTATE_TURN_AROUND_START);
	}

	// down, by a table of positive entries
	void port__ObjState_Fall()
	{
		set_obj_y(static_cast<uint16_t>(obj_y() + obj_y_vel(ADDR_FallingYVel_Light, ADDR_FallingYVel_Heavy)));
		obj_fall_and_land(ADDR__ObjState_Fall + 0x3d, OBJSTATE_LAND);
	}

	// Func_627f0 / Func_62690: moves the object up or down by the table entry
	// at OBJ_VAR_3 (table_z if b was 1, else table_nz; +$20 if heavy) and
	// advances OBJ_VAR_3 until the table's $80 end marker. OBJ_VAR_3 and
	// OBJ_Y_POS are at `page` (h on entry, the object's page from its
	// caller's MoveObject*ByVar2 / res).
	void obj_y_by_table(uint16_t table_z, uint16_t table_nz, uint8_t b, uint16_t page)
	{
		page &= 0xff00;
		uint8_t v = table_step(static_cast<uint16_t>(page | OBJ_VAR_3), b == 1 ? table_z : table_nz, 0x20);
		wr8(hffa0, v);
		if (v) {
			uint16_t ypos = static_cast<uint16_t>(page | OBJ_Y_POS);
			bool negative = v >= 0x80;
			uint8_t lo = rd8(ypos);
			wr8(ypos, static_cast<uint8_t>(lo + v));
			bool carry = lo + v >= 0x100;
			wr8(wCurObjStateDuration, negative ? 0x00 : 0x01);
			uint16_t hi = static_cast<uint16_t>(ypos + 1);
			if (negative && !carry)
				wr8(hi, static_cast<uint8_t>(rd8(hi) - 1));
			else if (!negative && carry)
				wr8(hi, static_cast<uint8_t>(rd8(hi) + 1));
		}
		home_jump_ret();
	}


	// --- misc states ---

	void port_Func_6303f()
	{
		wr8(wCurObjState, OBJSTATE_1B);
		wr8(wCurObjVar3, 0);
		wr8(wCurObjStateDuration, 0x07);
		home_jump_ret();
	}

	void port_Func_63050()
	{
		uint8_t d = static_cast<uint8_t>(rd8(wCurObjStateDuration) - 1);
		wr8(wCurObjStateDuration, d);
		if (d) {
			port_Func_629a6();
			return;
		}
		wr8(wCurObjState, OBJSTATE_STANDING_FALL_START);
		home_jump_ret();
	}

	void port_Func_6305f()
	{
		wr8(wCurObjState, (rd8(rDIV) & 1) ? OBJSTATE_WOBBLE_LEFT : OBJSTATE_WOBBLE_RIGHT);
		wr8(wCurObjVar3, 0);
		wr8(wCurObjVar2, 0x01);
		wr8(wCurObjStateDuration, 0x04);
		home_jump_ret();
	}

	// --- bumping and knocked-back objects ---

	// Func_62ae5 (moving left) / Func_62bce (moving right): an object that
	// bumped into something slides back, slowing down
	void obj_bump_slide(bool left)
	{
		uint16_t routine = left ? ADDR_Func_62ae5 : ADDR_Func_62bce;
		set_hpos_y(static_cast<uint16_t>(obj_y() - 4));
		set_hpos_x(edge(left));
		if (block(ADDR_Func_3513, static_cast<uint16_t>(routine + (left ? 0x1f : 0x1c))) & 0x0f) {
			play_sfx(SFX_01A);
			wr8(wCurObjState, left ? OBJSTATE_STUN_RIGHT_START : OBJSTATE_STUN_LEFT_START);
		} else {
			move_x_by_var2(left);
			let_go_off_screen(left);
			hpos_at_obj();
			wr8(wObjSlopeInteraction, 0);
			uint8_t floor = block(ADDR_Func_3543, static_cast<uint16_t>(routine + (left ? 0x4a : 0x47)));
			wr8(wCurObjYPos + 0, rd8(hYPosLo));
			wr8(wCurObjYPos + 1, rd8(hYPosHi));
			if (floor == 0x10) {
				// on a slope: down it, or stunned against it
				res_flag(OBJFLAG_GRABBED_F);
				bool slope_bit = rd8(wObjSlopeInteraction) & 1;
				if (left ? !slope_bit : slope_bit)
					wr8(wCurObjState, left ? OBJSTATE_STUN_RIGHT_START : OBJSTATE_STUN_LEFT_START);
				else if (rd8(wCurObjState) != (left ? OBJSTATE_STUN_LEFT : OBJSTATE_STUN_RIGHT))
					wr8(wCurObjState, left ? OBJSTATE_14 : OBJSTATE_15);
				home_jump_ret();
				return;
			}
			// the edge it moves to first, then the other
			bool hit = false;
			for (int side = 0; side < 2 && !hit; side++) {
				set_hpos_x(edge((side == 0) == left));
				hit = block(ADDR_Func_35a3, static_cast<uint16_t>(routine +
					(left ? (side ? 0x81 : 0x6c) : (side ? 0x7e : 0x66)))) != 0;
			}
			if (!hit)
				wr8(wCurObjState, left ? OBJSTATE_14 : OBJSTATE_15);
		}
		// slows down every 8 frames (4 if heavy)
		if (!(rd8(wGlobalCounter) & (heavy() ? 0x3 : 0x7)))
			wr8(wCurObjVar2, static_cast<uint8_t>(rd8(wCurObjVar2) - 1));
		home_jump_ret();
	}

	void port_Func_62ae5() { obj_bump_slide(true); }
	void port_Func_62bce() { obj_bump_slide(false); }

	// slides while OBJ_VAR_2 lasts, then turns around when
	// OBJ_STATE_DURATION runs out
	void obj_bump(bool left)
	{
		if (rd8(wCurObjVar2)) {
			obj_bump_slide(left);
			return;
		}
		uint8_t d = static_cast<uint8_t>(rd8(wCurObjStateDuration) - 1);
		wr8(wCurObjStateDuration, d);
		if (!d)
			wr8(wCurObjState, OBJSTATE_TURN_AROUND_START);
		home_jump_ret();
	}

	void port__ObjState_BumpRight() { obj_bump(true); }
	void port__ObjState_BumpLeft() { obj_bump(false); }

	// knocked to the side, falling, until it lands (on a slope as
	// land_by_slope), vanishes in water or sand, or hits a wall
	void obj_knocked_fall(bool left)
	{
		uint16_t routine = left ? ADDR_Func_62ca8 : ADDR_Func_62d86;
		move_x_by_var2(left);
		let_go_off_screen(left);
		set_obj_y(static_cast<uint16_t>(obj_y() + obj_y_vel(ADDR_FallingYVel_Light, ADDR_FallingYVel_Heavy)));
		hpos_at_obj();
		wr8(wObjSlopeInteraction, 0);
		wr8(wc1ca, 0);
		if (block(ADDR_Func_352b, static_cast<uint16_t>(routine + 0x4f))) {
			land_by_slope(left);
			return;
		}
		if (rd8(wIsInWaterOrSand)) {
			wr8(wCurObjState, OBJSTATE_VANISH_TOUCH);
			home_jump_ret();
			return;
		}
		set_hpos_y(static_cast<uint16_t>(obj_y() - 4));
		set_hpos_x(edge(left));
		if (block(ADDR_Func_3513, static_cast<uint16_t>(routine + (left ? 0x77 : 0x74))) & 0x0f)
			wr8(wCurObjState, left ? OBJSTATE_15 : OBJSTATE_14);
		home_jump_ret();
	}

	void port_Func_62ca8() { obj_knocked_fall(true); }
	void port_Func_62d86() { obj_knocked_fall(false); }

	void port_Func_62c9f()
	{
		wr8(wCurObjState, OBJSTATE_56);
		wr8(wCurObjVar3, 0);
		obj_knocked_fall(true);
	}

	void port_Func_62d7d()
	{
		wr8(wCurObjState, OBJSTATE_57);
		wr8(wCurObjVar3, 0);
		obj_knocked_fall(false);
	}

	// --- vanishing ---

	// The object turns into stars (StarsFunc)
	void stars()
	{
		// (BANK(StarsFunc) - BANK("Object Functions 1")) << 4
		wr8(wCurObjId, 0x30);
		mem_.word(wCurObjOAMPtr) = ADDR_OAM_18c000;
		mem_.word(wCurObjFramesetPtr) = ADDR_Frameset_680c5;
		wr8(wCurObjFrameDuration, 0);
		wr8(wCurObjFramesetOffset, 0);
		wr8(wCurObjStateDuration, 33);
		wr8(wCurObjFlags, (rd8(wCurObjFlags) & ~(1 << OBJFLAG_GRABBED_F)) | (1 << OBJFLAG_NO_COLLISION_F));
		no_actions();
		set_obj_y(static_cast<uint16_t>(obj_y() - 8));
	}

	// StarsFunc as its update function, then the stars
	void vanish(bool sfx)
	{
		if (sfx)
			play_sfx(SFX_VANISH);
		wr8(wCurObjUpdateFunction + 1, static_cast<uint8_t>(ADDR_StarsFunc >> 8));
		wr8(wCurObjUpdateFunction + 0, static_cast<uint8_t>(ADDR_StarsFunc));
		wr8(wCurObjSubState, rd8(wCurObjSubState) & OBJSUBFLAGS_MASK);
		stars();
	}

	void port_VanishObject_NoSFX() { vanish(false); }
	void port__VanishObject() { vanish(true); }

	// --- init ---

	void init_object_vars(bool level_start)
	{
		clear_objects(0, 0);
		clear_obj_data_pointers(0);
		if (level_start)
			wr8(wFlameBlockCleared, 0);
		wr8(wDollBoyNoFlashingBarrel, 0);
		wr8(wDollBoyRemainingParts, 0);
		wr8(wNumLitTorches, 0);
		wr8(wShootGoals, 0);
		wr8(wWarioGoals, 0);
	}

	void port_InitObjectVars_FromLevelStart() { init_object_vars(true); }
	void port_InitObjectVars_FromTransition() { init_object_vars(false); }

	// --- movement tables ---

	// A movement table's next entry for the index (an 8-bit add, within
	// the table's page). At the $80 end the last entry repeats, or with
	// loop the table starts over; otherwise the index goes on.
	uint8_t movement_step(uint16_t index, uint16_t table, bool loop)
	{
		uint16_t page = table & 0xff00;
		uint8_t c = static_cast<uint8_t>(rd8(index) + static_cast<uint8_t>(table));
		uint8_t v = rd8(static_cast<uint16_t>(page | c));
		if (v == 0x80) {
			if (!loop)
				return rd8(static_cast<uint16_t>(page | static_cast<uint8_t>(c - 1)));
			wr8(index, 0);
			v = rd8(table);
		}
		wr8(index, static_cast<uint8_t>(rd8(index) + 1));
		return v;
	}

	// The signed add of v to the position at `at`. Returns where its high
	// byte is (hl, which the objects read the page of).
	uint16_t add_signed_at(uint16_t at, uint8_t v)
	{
		uint8_t lo = rd8(at);
		wr8(at, static_cast<uint8_t>(lo + v));
		bool carry = lo + v > 0xff;
		bool negative = v >= 0x80;
		uint16_t hi = static_cast<uint16_t>(at + 1);
		if (negative && !carry)
			wr8(hi, static_cast<uint8_t>(rd8(hi) - 1));
		else if (!negative && carry)
			wr8(hi, static_cast<uint8_t>(rd8(hi) + 1));
		return hi;
	}

	// The next entry of the table moves the object along y (index
	// OBJ_VAR_3) or x (OBJ_VAR_2).
	uint16_t apply_obj_movement(uint16_t index, uint8_t pos, uint16_t table, bool loop)
	{
		uint8_t v = movement_step(index, table, loop);
		return add_signed_at(static_cast<uint16_t>((index & 0xff00) | pos), v);
	}

	// bc = the table; the objects read on the h (and the e of the _Loop
	// ones) these leave
	void apply_obj_movement_hook(uint16_t index, uint8_t pos, bool loop)
	{
		uint16_t table = REG_BC(gb_);
		SET_HL(gb_, apply_obj_movement(index, pos, table, loop));
		if (loop)
			cpu_.e = static_cast<uint8_t>(table);
	}

	void port__ApplyObjYMovement() { apply_obj_movement_hook(wCurObjVar3, OBJ_Y_POS, false); }
	void port__ApplyObjXMovement() { apply_obj_movement_hook(wCurObjVar2, OBJ_X_POS, false); }
	void port__ApplyObjYMovement_Loop() { apply_obj_movement_hook(wCurObjVar3, OBJ_Y_POS, true); }
	void port__ApplyObjXMovement_Loop() { apply_obj_movement_hook(wCurObjVar2, OBJ_X_POS, true); }

	// A platform's y movement (the table at bc), which carries Wario along
	// when he stands on it
	void move_platform_y(uint16_t table)
	{
		uint8_t v = movement_step(wCurObjVar3, table, false);
		add_signed_at(wCurObjYPos, v);
		if (!(rd8(wCurObjFlags) & (1 << OBJFLAG_STEPPED_F)))
			return;
		wr8(wCurObjFlags, rd8(wCurObjFlags) & ~(1 << OBJFLAG_STEPPED_F));
		home::move_wario_y(gb_, static_cast<uint8_t>(v));
		FARCALL(Func_19720);
	}

	void port_Func_63209() { move_platform_y(REG_BC(gb_)); }

	// A set movement: OBJ_VAR_1 counts down the current step's duration;
	// then the next step (duration, y and x deltas) from the pointer in
	// OBJ_VAR_2/3, the steps starting over at `start` at the 0 end.
	void apply_obj_set_movement(uint16_t start)
	{
		uint8_t d = static_cast<uint8_t>(rd8(wCurObjVar1) - 1);
		wr8(wCurObjVar1, d);
		if (d)
			return;
		uint16_t p = static_cast<uint16_t>(rd8(wCurObjVar3) << 8 | rd8(wCurObjVar2));
		uint8_t duration = rd8(p++);
		if (!duration) {
			p = start;
			duration = rd8(p++);
		}
		wr8(wCurObjVar1, duration);
		uint8_t dy = rd8(p++);
		uint8_t dx = rd8(p++);
		wr8(wCurObjVar2, static_cast<uint8_t>(p));
		wr8(wCurObjVar3, static_cast<uint8_t>(p >> 8));
		add_signed_at(wCurObjYPos, dy);
		add_signed_at(wCurObjXPos, dx);
	}

	// de = the first step
	void port__ApplyObjSetMovement() { apply_obj_set_movement(REG_DE(gb_)); }

	// the number of active transient objects with the same OBJ_UNK_01/02 as
	// the current object
	uint8_t count_same_transient_objects()
	{
		uint8_t n = 0;
		for (int i = 0; i < NUM_OBJECTS; i++) {
			uint16_t obj = static_cast<uint16_t>((wObj1 & 0xff00) | (i * 0x20));
			if ((rd8(obj) & (OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT)) == (OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT)
				&& rd8(obj + 1) == rd8(wCurObjUnk01) && rd8(obj + 2) == rd8(wCurObjUnk01 + 1))
				n++;
		}
		return n;
	}

	// output: e
	void port_Func_632ac() { cpu_.e = count_same_transient_objects(); }

	// --- thrown objects hitting other objects ---

	#define OBJ_N(addr, n) ((uint16_t)((addr) + (n) * OBJ_STRUCT_SIZE))

	// A grabbed (thrown) object that touches another active, on-screen,
	// visible object with collision: the states each goes to, by their
	// weights and the throw's direction, the grab let go of if either was
	// held, and SFX_016.
	void handle_grabbed_object_collisions()
	{
		int g;
		for (g = 0; g < NUM_OBJECTS; g++)
			if ((rd8(OBJ_N(wObj1Flags, g)) & (OBJFLAG_ACTIVE | OBJFLAG_GRABBED)) == (OBJFLAG_ACTIVE | OBJFLAG_GRABBED))
				break;
		if (g == NUM_OBJECTS)
			return;

		// the thrown object's box on screen (+$2a, so that it stays positive)
		uint8_t type = rd8(OBJ_N(wObj1InteractionType, g));
		wr8(hThrownObjInteractionType, type);
		uint8_t sy = static_cast<uint8_t>(rd8(OBJ_N(wObj1ScreenYPos, g)) + 0x2a);
		uint8_t sx = static_cast<uint8_t>(rd8(OBJ_N(wObj1ScreenXPos, g)) + 0x2a);
		uint8_t top = static_cast<uint8_t>(sy + rd8(OBJ_N(wObj1CollBoxTop, g)));
		uint8_t bottom = static_cast<uint8_t>(sy + rd8(OBJ_N(wObj1CollBoxBottom, g)));
		uint8_t left = static_cast<uint8_t>(sx + rd8(OBJ_N(wObj1CollBoxLeft, g)));
		uint8_t right = static_cast<uint8_t>(sx + rd8(OBJ_N(wObj1CollBoxRight, g)));

		// the states for the thrown object and the one it hits (light and
		// heavy, then the same for the other)
		uint8_t state = rd8(OBJ_N(wObj1State, g));
		bool to_right = state & 1;
		if (state >= OBJSTATE_FULL_THROW_LEFT_START || state < OBJSTATE_48) {
			static const uint8_t states[4][4] = {
				{OBJSTATE_28, OBJSTATE_29, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_29},
				{OBJSTATE_28, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_28, OBJSTATE_29},
				{OBJSTATE_29, OBJSTATE_28, OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_28},
				{OBJSTATE_29, OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_29, OBJSTATE_28},
			};
			const uint8_t *s = states[to_right * 2 + ((type & HEAVY_OBJ) ? 1 : 0)];
			for (int i = 0; i < 4; i++)
				wr8(static_cast<uint16_t>(hThrownObjInteractionType + 1 + i), s[i]);
		} else {
			// OBJSTATE_48 <= state <= OBJSTATE_4B
			static const uint8_t states[2][2] = {
				{OBJSTATE_WOBBLE_RIGHT_START, OBJSTATE_WOBBLE_LEFT_START},
				{OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_WOBBLE_RIGHT_START},
			};
			wr8(hCollidedObjState_Light, states[to_right][0]);
			wr8(static_cast<uint16_t>(hCollidedObjState_Light + 1), states[to_right][1]);
		}

		for (int n = 0; n < NUM_OBJECTS; n++) {
			if ((rd8(OBJ_N(wObj1Flags, n)) & (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_GRABBED
				| OBJFLAG_NO_COLLISION | OBJFLAG_INVISIBLE)) != (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN))
				continue;
			// the boxes overlap (8-bit, as the asm)
			uint8_t oy = static_cast<uint8_t>(rd8(OBJ_N(wObj1ScreenYPos, n)) + 0x2a);
			uint8_t ox = static_cast<uint8_t>(rd8(OBJ_N(wObj1ScreenXPos, n)) + 0x2a);
			if (static_cast<uint8_t>(rd8(OBJ_N(wObj1CollBoxBottom, n)) + oy) < top
				|| static_cast<uint8_t>(rd8(OBJ_N(wObj1CollBoxTop, n)) + oy) >= bottom
				|| static_cast<uint8_t>(rd8(OBJ_N(wObj1CollBoxRight, n)) + ox) < left
				|| static_cast<uint8_t>(rd8(OBJ_N(wObj1CollBoxLeft, n)) + ox) >= right)
				continue;

			uint16_t thrown = OBJ_N(wObj1Flags, g);
			wr8(thrown, rd8(thrown) & ~(1 << OBJFLAG_GRABBED_F));
			uint16_t thrown_state = OBJ_N(wObj1State, g);
			uint16_t other_state = OBJ_N(wObj1State, n);
			uint8_t s = rd8(thrown_state) & 0xfe;
			uint16_t collided;
			if (s == OBJSTATE_4A || s == OBJSTATE_48) {
				// .CollideWithObject
				wr8(wGrabState, 0);
				wr8(thrown_state, rd8(hThrownObjState_Light));
				collided = hCollidedObjState_Light;
			} else {
				bool heavy_other = rd8(OBJ_N(wObj1InteractionType, n)) & HEAVY_OBJ;
				wr8(thrown_state, rd8(heavy_other ? hThrownObjState_Heavy : hThrownObjState_Light));
				uint8_t o = rd8(other_state) & 0xfe;
				if (o == OBJSTATE_PICKED_UP_LEFT || o == OBJSTATE_48)
					wr8(wGrabState, 0);
				collided = heavy_other ? hCollidedObjState_Heavy : hCollidedObjState_Light;
			}
			wr8(other_state, rd8(collided));
			wr8(OBJ_N(wObj1Flags, n), rd8(OBJ_N(wObj1Flags, n)) | (1 << OBJFLAG_NO_COLLISION_F));
			play_sfx(SFX_016);
			return;
		}
	}


	// --- DoObjectAction ---

	// the action and state, by the side Wario touched it from (OBJ_UNK_1D)
	void action_by_interaction(uint8_t left, uint8_t right)
	{
		wr8(wCurObjAction, 15 | (1 << 7));
		wr8(wCurObjState, (rd8(wCurObjUnk1d) & INTERACTION_RIGHT) ? left : right);
	}

	void set_action_state(uint8_t action, uint8_t state)
	{
		wr8(wCurObjAction, action);
		wr8(wCurObjState, state);
	}

	void action_wobble()
	{
		res_flag(OBJFLAG_GRABBED_F);
		action_by_interaction(OBJSTATE_WOBBLE_LEFT_START, OBJSTATE_WOBBLE_RIGHT_START);
	}

	// .Action06 / .Action12: the state by the direction it was hit from
	// (OBJ_UNK_1D's top bits: down, up, right, else left)
	void action_06(uint8_t action)
	{
		uint8_t from = rd8(wCurObjUnk1d);
		wr8(wCurObjAction, action);
		wr8(wCurObjState, (from & 0x80) ? OBJSTATE_0E : (from & 0x40) ? OBJSTATE_0D
			: (from & 0x20) ? OBJSTATE_0B : OBJSTATE_0C);
	}

	// What Wario (or another object) did to the object last frame
	// (OBJ_ACTION, from the interactions), or a count of frames with none
	void do_object_action()
	{
		uint8_t action = rd8(wCurObjAction);
		if (!action)
			return;
		if (action & (1 << NO_ACTIONS_F)) {
			action--;
			wr8(wCurObjAction, action == (1 << NO_ACTIONS_F) ? 0 : action); // no_actions_for 0
			return;
		}
		switch (action) {
		case OBJACTION_NONE:
			break;
		case OBJACTION_BUMP:
			if (!(rd8(wCurObjInteractionType) & INTERACTION_MASK))
				action_wobble();
			else
				action_by_interaction(OBJSTATE_BUMP_LEFT_START, OBJSTATE_BUMP_RIGHT_START);
			break;
		case OBJACTION_ATTACK:
			wr8(wCurObjFlags, (rd8(wCurObjFlags) & ~(1 << OBJFLAG_GRABBED_F)) | (1 << OBJFLAG_NO_COLLISION_F));
			action_by_interaction(OBJSTATE_ATTACKED_LEFT_START, OBJSTATE_ATTACKED_RIGHT_START);
			break;
		case OBJACTION_PICK_UP:
			res_flag(OBJFLAG_GRABBED_F);
			set_action_state(15 | (1 << 7),
				rd8(wDirection) ? OBJSTATE_PICKED_UP_LEFT_START : OBJSTATE_PICKED_UP_RIGHT_START);
			break;
		case OBJACTION_WOBBLE:
			action_wobble();
			break;
		case OBJACTION_VANISH_TOUCH:
			wr8(wCurObjFlags, (rd8(wCurObjFlags) & ~(1 << OBJFLAG_GRABBED_F)) | (1 << OBJFLAG_NO_COLLISION_F));
			set_action_state(15 | (1 << 7), OBJSTATE_VANISH_TOUCH);
			break;
		case OBJACTION_06:
			action_06(15 | (1 << 7));
			break;
		case OBJACTION_07:
			set_action_state(0, OBJSTATE_18);
			break;
		case OBJACTION_VANISH_TREASURE:
			vanish(false);
			break;
		case OBJACTION_STEP_ON:
			wr8(wCurObjAction, 0);
			wr8(wCurObjFlags, rd8(wCurObjFlags) | (1 << OBJFLAG_STEPPED_F));
			break;
		case OBJACTION_0A:
			wr8(wCurObjAction, 15 | (1 << 7));
			break;
		case OBJACTION_FLATTEN:
		case OBJACTION_0C:
		case OBJACTION_0E:
		case OBJACTION_TELEPORT:
		case OBJACTION_11:
			set_action_state(15 | (1 << 7), OBJSTATE_ACTIVATE);
			break;
		case OBJACTION_0D:
			set_action_state(15 | (1 << 7), OBJSTATE_19);
			break;
		case OBJACTION_SMASH_ATTACK_WALKABLE:
			set_action_state(2 | (1 << 7), OBJSTATE_VANISH_TOUCH);
			break;
		case OBJACTION_12:
			action_06(7 | (1 << 7));
			break;
		case OBJACTION_13:
			set_action_state(15 | (1 << 7), OBJSTATE_SPECIAL_3);
			break;
		default:
			// past the table's end: no action does that
			if (gb_->test.active)
				gb_->test.unrealistic = true;
			break;
		}
	}

	void port_Func_62e57()
	{
		play_sfx(SFX_VANISH);
		wr8(wCurObjUpdateFunction + 1, static_cast<uint8_t>(ADDR_Func_4c860 >> 8));
		wr8(wCurObjUpdateFunction + 0, static_cast<uint8_t>(ADDR_Func_4c860));
		stars();
		home_jump_ret();
	}

	void port__VanishObject2()
	{
		vanish(true);
		home_jump_ret();
	}

	// --- throwing a grabbed object ---

	// ThrowObject_Left / ThrowObject_Right: grab (wGrabState) bit 6 set for
	// a full throw. OBJ_VAR_2 = the throw strength (1 when holding up),
	// state = the throw state, action = 31 | (1 << 7).
	void throw_object(bool right, uint8_t grab)
	{
		play_sfx(SFX_THROW);
		uint8_t type = static_cast<uint8_t>((rd8(wCurObjInteractionType) & HEAVY_OBJ) | OBJ_INTERACTION_00);
		wr8(wCurObjInteractionType, type);
		wr8(wCurObjStateDuration, 0);
		bool full = grab & (1 << 6);
		uint8_t strength = full ? 0x04 : (type & HEAVY_OBJ) ? 0x02 : 0x03;
		if (rd8(wJoypadDown) & (1 << B_PAD_UP))
			strength = 0x01;
		wr8(wCurObjVar2, strength);
		wr8(wCurObjVar3, 0);
		wr8(wCurObjState, full ? (right ? OBJSTATE_FULL_THROW_RIGHT_START : OBJSTATE_FULL_THROW_LEFT_START)
			: (right ? OBJSTATE_4F : OBJSTATE_4E));
		wr8(wCurObjAction, 31 | (1 << 7));
		home_jump_ret();
	}

	// Func_626da (moving left) / Func_62768 (moving right): a thrown object
	// moving sideways. While it's going down (wCurObjStateDuration set) it
	// checks the floor, and lands on a slope; it vanishes in water or sand, or
	// when it hits a ceiling or a wall. Otherwise it moves by OBJ_VAR_2 and
	// goes on along the y table of Func_627f0.
	// Func_62574 / Func_62605 do the same with other states for a ceiling
	// (ceil_state) or a wall (wall_state) and go on to Func_62690's table
	// without the offscreen check.
	void thrown_sideways(bool left, uint16_t routine, uint8_t ceil_state,
		uint8_t wall_state, bool tables_627f0, uint8_t wall_ret)
	{
		set_hpos_x(obj_x());
		if (rd8(wCurObjStateDuration)) {
			set_hpos_y(obj_y());
			wr8(wc1ca, FALSE);
			wr8(wBlockFuncSpecialCollision, COLLISION_THROWN_OBJ);
			uint8_t floor = block(ADDR_Func_352b, static_cast<uint16_t>(routine + 0x20));
			wr8(hffa0, 0x03);
			if (floor) {
				land_by_slope(left);
				return;
			}
			if (rd8(wIsInWaterOrSand)) {
				wr8(wCurObjState, OBJSTATE_VANISH_TOUCH);
				home_jump_ret();
				return;
			}
		}

		set_hpos_y(top());
		wr8(wBlockFuncSpecialCollision, COLLISION_THROWN_OBJ);
		if (block(ADDR_Func_358b, static_cast<uint16_t>(routine + 0x50))) {
			wr8(wCurObjState, ceil_state);
			home_jump_ret();
			return;
		}
		set_hpos_y(static_cast<uint16_t>(obj_y() - 8));
		set_hpos_x(edge(left));
		wr8(wBlockFuncSpecialCollision, COLLISION_THROWN_OBJ);
		if (block(ADDR_Func_3513, static_cast<uint16_t>(routine + wall_ret)) & 0x0f) {
			wr8(wCurObjState, wall_state);
			home_jump_ret();
			return;
		}
		move_x_by_var2(left);
		if (tables_627f0) {
			let_go_off_screen(left);
			obj_y_by_table(ADDR_Data_60140, ADDR_Data_600c0, rd8(wCurObjVar2), wCurObjVar3);
		} else {
			obj_y_by_table(ADDR_Data_60100, ADDR_Data_60080, rd8(wCurObjVar2), wCurObjVar3);
		}
	}

	void port_Func_626da() { thrown_sideways(true, ADDR_Func_626da, OBJSTATE_VANISH_TOUCH, OBJSTATE_VANISH_TOUCH, true, 0x77); }
	void port_Func_62768() { thrown_sideways(false, ADDR_Func_62768, OBJSTATE_VANISH_TOUCH, OBJSTATE_VANISH_TOUCH, true, 0x74); }
	void port_Func_62574() { thrown_sideways(true, ADDR_Func_62574, OBJSTATE_14, OBJSTATE_15, false, 0x77); }
	void port_Func_62605() { thrown_sideways(false, ADDR_Func_62605, OBJSTATE_15, OBJSTATE_14, false, 0x74); }

	// --- an object held by Wario ---

	// a grabbed object let go: it falls off sideways
	void drop_grabbed(uint8_t state)
	{
		wr8(wGrabState, 0);
		play_sfx(SFX_01A);
		wr8(wCurObjVar2, 0x02);
		wr8(wCurObjAction, 31 | (1 << 7));
		wr8(wCurObjState, state);
		home_jump_ret();
	}

	// Wario's position (big-endian)
	uint16_t wario_y() const { return static_cast<uint16_t>(rd8(wWarioYPos + 0) << 8 | rd8(wWarioYPos + 1)); }
	uint16_t wario_x() const { return static_cast<uint16_t>(rd8(wWarioXPos + 0) << 8 | rd8(wWarioXPos + 1)); }

	// hPickedUpObjPos (y, x, little-endian)
	uint16_t picked_y() const { return mem_.word(hPickedUpObjPos); }
	uint16_t picked_x() const { return mem_.word(hPickedUpObjPos + 2); }
	void set_picked(uint16_t y, uint16_t x)
	{
		mem_.word(hPickedUpObjPos) = y;
		mem_.word(hPickedUpObjPos + 2) = x;
	}
	// the object moves to hPickedUpObjPos
	void obj_to_picked()
	{
		for (int i = 0; i < 4; i++)
			wr8(static_cast<uint16_t>(wCurObjYPos + i), rd8(static_cast<uint16_t>(hPickedUpObjPos + i)));
	}

	// the object at Wario's position, 30 (26 when smash attacking) up and 7
	// to the right or left
	void hold_above_wario(bool right)
	{
		uint8_t up = static_cast<uint8_t>(30 - rd8(wIsSmashAttacking) * 4);
		set_obj_y(static_cast<uint16_t>(wario_y() - up));
		set_obj_x(static_cast<uint16_t>(right ? wario_x() + 7 : wario_x() - 7));
	}

	// Func_61f54 (held to the right) / Func_620a6 (held to the left): being
	// picked up (along the steps in OBJ_VAR_2/3, which start over 4 bytes
	// back at their 0 end; stopped by a wall or ceiling), then held
	void grabbed_object(bool right)
	{
		uint16_t routine = right ? ADDR_Func_61f54 : ADDR_Func_620a6;
		uint8_t drop_state = right ? OBJSTATE_14 : OBJSTATE_15;
		uint8_t idle_state = right ? OBJSTATE_19 : OBJSTATE_18;
		no_actions();

		uint8_t grab = rd8(wGrabState) & GRAB_STATE_MASK;
		if (grab != GRAB_PICK_UP) {
			if (grab != GRAB_IDLE) {
				wr8(wCurObjState, OBJSTATE_12);
				home_jump_ret();
				return;
			}
			// GrabIdle: held above Wario, until a floor's above him
			hold_above_wario(right);
			hpos_at_obj();
			wr8(wc1ca, FALSE);
			if (!block(ADDR_Func_352b, static_cast<uint16_t>(routine + 0x55)) || rd8(wc1ca)) {
				wr8(wCurObjState, idle_state);
				home_jump_ret();
				return;
			}
			set_obj_y(static_cast<uint16_t>(obj_y() + 16));
			drop_grabbed(drop_state);
			return;
		}

		uint8_t d = static_cast<uint8_t>(rd8(wCurObjVar1) - 1);
		wr8(wCurObjVar1, d);
		if (d) {
			home_jump_ret();
			return;
		}
		uint16_t p = static_cast<uint16_t>(rd8(wCurObjVar3) << 8 | rd8(wCurObjVar2));
		uint8_t duration = rd8(p++);
		if (!duration) {
			p = static_cast<uint16_t>((p & 0xff00) | static_cast<uint8_t>(p - 4)); // dec l x4
			duration = rd8(p++);
		}
		wr8(wCurObjVar1, duration);
		uint8_t dy = rd8(p++);
		uint8_t dx = rd8(p++);
		wr8(wCurObjVar2, static_cast<uint8_t>(p));
		wr8(wCurObjVar3, static_cast<uint8_t>(p >> 8));
		set_picked(static_cast<uint16_t>(wario_y() + static_cast<int8_t>(dy)),
			static_cast<uint16_t>(wario_x() + static_cast<int8_t>(dx)));

		// the side it's held on, then its top
		set_hpos_y(static_cast<uint16_t>(picked_y() - 4));
		set_hpos_x(right ? static_cast<uint16_t>(picked_x() + rd8(wCurObjCollBoxRight))
			: static_cast<uint16_t>(picked_x() - static_cast<uint8_t>(-rd8(wCurObjCollBoxLeft))));
		if (!(block(ADDR_Func_3513, static_cast<uint16_t>(routine + (right ? 0x106 : 0x109))) & 0x0f)) {
			set_hpos_y(static_cast<uint16_t>(picked_y() - static_cast<uint8_t>(-rd8(wCurObjCollBoxTop))));
			set_hpos_x(picked_x());
			if (!(block(ADDR_Func_358b, static_cast<uint16_t>(routine + (right ? 0x125 : 0x128))) & 0x0f)) {
				obj_to_picked();
				home_jump_ret();
				return;
			}
		}
		// hit a wall or a ceiling
		if (rd8(wCurObjInteractionType) & HEAVY_OBJ) {
			drop_grabbed(drop_state);
			return;
		}
		wr8(wGrabState, GRAB_UNK_02);
		wr8(wCurObjState, idle_state);
		home_jump_ret();
	}

	void port_Func_61f54() { grabbed_object(true); }
	void port_Func_620a6() { grabbed_object(false); }

	// Func_621fb (held to the right) / Func_622bd (held to the left)
	void held_object(bool right)
	{
		uint16_t routine = right ? ADDR_Func_621fb : ADDR_Func_622bd;
		no_actions();
		uint8_t grab = rd8(wGrabState);
		uint8_t g = grab & 0x0f;
		if (g != GRAB_IDLE) {
			if (g == GRAB_CHARGE_THROW) {
				wr8(wCurObjState, right ? OBJSTATE_4B : OBJSTATE_4A);
				home_jump_ret();
				return;
			}
			if (g == GRAB_THROW) {
				throw_object(right, grab);
				return;
			}
			res_flag(OBJFLAG_GRABBED_F);
			if (rd8(wIsCrouching)) {
				wr8(wCurObjVar2, 0x01);
				wr8(wCurObjState, right ? OBJSTATE_15 : OBJSTATE_14);
			} else {
				wr8(wCurObjState, OBJSTATE_12);
			}
			home_jump_ret();
			return;
		}

		// GrabIdle: let go when it hits a ceiling, or a wall at its side
		hold_above_wario(right);
		set_hpos_y(top());
		set_hpos_x(obj_x());
		if (!(block(ADDR_Func_358b, static_cast<uint16_t>(routine + 0x7d)) & (right ? 0x0f : 0xff))) {
			set_hpos_y(static_cast<uint16_t>(obj_y() - 8));
			set_hpos_x(edge(!right));
			if (!(block(ADDR_Func_3513, static_cast<uint16_t>(routine + (right ? 0x9d : 0x9f))) & (right ? 0xff : 0x0f))) {
				home_jump_ret();
				return;
			}
		}
		wr8(wGrabState, 0);
		play_sfx(SFX_01A);
		wr8(wCurObjVar2, 0x02);
		wr8(wCurObjAction, 31 | (1 << 7));
		wr8(wCurObjState, right ? OBJSTATE_14 : OBJSTATE_15);
		res_flag(OBJFLAG_GRABBED_F);
		home_jump_ret();
	}

	void port_Func_621fb() { held_object(true); }
	void port_Func_622bd() { held_object(false); }

	// Func_6247b (charging a throw to the right) / Func_62382 (to the left)
	void charging_throw(bool right)
	{
		uint16_t routine = right ? ADDR_Func_6247b : ADDR_Func_62382;
		no_actions();
		uint8_t grab = rd8(wGrabState);
		uint8_t g = grab & 0x0f;
		if (g == GRAB_CHARGE_THROW || g == GRAB_THROW_FULLY_CHARGED) {
			hold_above_wario(right);
			home_jump_ret();
			return;
		}
		if (g == GRAB_HOLD_FULL_CHARGE) {
			// held behind Wario, unless there's a wall there
			set_picked(static_cast<uint16_t>(wario_y() - 30),
				static_cast<uint16_t>(right ? wario_x() - 5 : wario_x() + 5));
			set_hpos_y(static_cast<uint16_t>(picked_y() - 4));
			set_hpos_x(picked_x());
			if (!(block(ADDR_Func_3513, static_cast<uint16_t>(routine + 0x81)) & 0x0f)) {
				obj_to_picked();
				home_jump_ret();
				return;
			}
			wr8(wGrabState, 0);
			play_sfx(SFX_01A);
		} else if (g == GRAB_THROW) {
			throw_object(right, grab);
			return;
		}
		res_flag(OBJFLAG_GRABBED_F);
		wr8(wCurObjState, OBJSTATE_12);
		home_jump_ret();
	}

	void port_Func_6247b() { charging_throw(true); }
	void port_Func_62382() { charging_throw(false); }


	// The object update loop (UpdateObject and friends)

	enum { UPD_NORMAL, UPD_TEMPLE, UPD_FLOOR };

	// Return addresses (the bytes after each call, checked against the ROM).
	#define RET_TEMPLE_HCALL 0x5261  // UpdateObject_Temple: call hCallFunc
	#define RET_TEMPLE_ANIM 0x5264
	#define RET_NORMAL_HCALL 0x5479  // UpdateObject: call hCallFunc
	#define RET_NORMAL_ANIM 0x547c
	#define RET_FLOOR_HCALL 0x55b4  // UpdateObjectDuringFloorTransition: call hCallFunc
	#define RET_FLOOR_FRAME 0x55b7
	#define RET_TEMPLE_LOOP 0x5316  // first call c, UpdateObject_Temple (then +7 each)
	#define RET_UPDATE_LOOP 0x5370  // first call c, UpdateObject
	#define RET_FLOOR_LOOP 0x53aa   // first call c, UpdateObjectDuringFloorTransition
	#define OBJFUNCS1_BANK 0x10     // BANK("Object Functions 1")
	#define OBJFUNCS4_BANK 0x13     // BANK("Object Functions 4")

	// The update function's pointer and bank into hCallFunc (the bank by the
	// object's id, or the temple's), and the call
	void call_update_function(uint8_t bank, bool by_id, uint16_t ret)
	{
		if (by_id) {
			uint8_t id = rd8(wCurObjId);
			bank = static_cast<uint8_t>((((id << 4) | (id >> 4)) & 0x0f) | OBJFUNCS1_BANK);
			cpu_.f = 0; // or BANK("Object Functions 1")
		}
		wr8(hCallFuncBank, bank);
		wr8(hCallFuncPointer + 0, rd8(wCurObjUpdateFunction + 0));
		wr8(hCallFuncPointer + 1, rd8(wCurObjUpdateFunction + 1));
		// Update functions read the a and f this leaves (and, as the
		// asm's, any of the registers before)
		cpu_.a = rd8(wCurObjUpdateFunction + 1);
		// Update functions all live in the Object Functions banks: a fuzzed
		// pointer into home code (MoveWarioUp, reading the b the asm loop
		// leaves) isn't comparable.
		if (gb_->test.active && rd8(wCurObjUpdateFunction + 1) < 0x40)
			gb_->test.unrealistic = true;
		uint8_t entry_bank = rd8(wROMBank);
		gb_call_ret(gb_, hCallFunc, ret);
		if (gb_->test.active && gb_->rom_bank != entry_bank)
			gb_->test.unrealistic = true;
	}

	// inc l: the next address in its page
	static uint16_t next_in_page(uint16_t addr)
	{
		return static_cast<uint16_t>((addr & 0xff00) | ((addr + 1) & 0xff));
	}

	// UpdateObject and friends: the object at obj (its flags, which the
	// loop passes rotated in a) copied to wCurObj, its action, update
	// function and animation run, its screen position set, and copied back
	// (all but OBJFLAG_STEPPED, unk01, unk02 and unk1d). The object's slot
	// stays pushed while it runs, as in the asm.
	void update_object(int kind, uint16_t obj, uint8_t flags)
	{
		gb_push16(gb_, obj);
		wr8(wCurObjFlags, flags);
		uint16_t src = next_in_page(obj);
		for (int n = 1; n < OBJ_STRUCT_LENGTH; n++)
			wr8(static_cast<uint16_t>(wCurObj + n), rd8(static_cast<uint16_t>(src + n - 1)));
		SET_HL(gb_, src + OBJ_STRUCT_LENGTH - 2); // where the copy ends

		switch (kind) {
		case UPD_NORMAL:
			do_object_action();
			call_update_function(0, true, RET_NORMAL_HCALL);
			gb_call_ret(gb_, ADDR_UpdateObjectAnimation, RET_NORMAL_ANIM);
			check_object_on_screen();
			break;
		case UPD_TEMPLE:
			do_object_action();
			call_update_function(OBJFUNCS4_BANK, false, RET_TEMPLE_HCALL);
			gb_call_ret(gb_, ADDR_UpdateObjectAnimation, RET_TEMPLE_ANIM);
			check_object_on_screen();
			break;
		case UPD_FLOOR:
			// during a floor transition only an object's first update runs
			SET_HL(gb_, wCurObjSubState);
			if (rd8(wCurObjSubState) & (1 << OBJSUBFLAG_UNINITIALISED_F))
				call_update_function(0, true, RET_FLOOR_HCALL);
			gb_call_ret(gb_, ADDR_UpdateObjectFrame, RET_FLOOR_FRAME);
			check_object_on_screen();
			break;
		}

		wr8(wCurObjScreenYPos, static_cast<uint8_t>(rd8(wCurObjYPos) + OAM_Y_OFS - rd8(wSCY)));
		wr8(wCurObjScreenXPos, static_cast<uint8_t>(rd8(wCurObjXPos) + OAM_X_OFS - rd8(wSCX)));
		if (kind == UPD_TEMPLE) {
			wr8(wSCY, static_cast<uint8_t>(rd8(wHiddenFigureSCYShake) + rd8(wSCYShake)));
			wr8(wSCX, rd8(wc08b));
		}
		obj = gb_pop16(gb_);

		wr8(obj, rd8(wCurObjFlags) & (0xff ^ OBJFLAG_STEPPED));
		uint16_t dst = next_in_page(next_in_page(static_cast<uint16_t>(obj + 1)));
		for (int n = OBJ_Y_POS; n <= OBJ_ACTION; n++)
			wr8(dst++, rd8(static_cast<uint16_t>(wCurObj + n)));
		dst = next_in_page(dst);
		wr8(dst, rd8(wCurObjUpdateFunction + 0));
		wr8(static_cast<uint16_t>(dst + 1), rd8(wCurObjUpdateFunction + 1));
	}

	// Every active object (OBJFLAG_ACTIVE, bit 0) updated by kind, with the
	// return address its call in the asm pushes
	void update_all(int kind, uint16_t ret)
	{
		for (int n = 0; n < NUM_OBJECTS; n++) {
			uint16_t obj = static_cast<uint16_t>(wObj1Flags + n * OBJ_STRUCT_SIZE);
			uint8_t flags = rd8(obj);
			if (!(flags & OBJFLAG_ACTIVE))
				continue;
			gb_push16(gb_, static_cast<uint16_t>(ret + n * 7));
			update_object(kind, obj, flags);
			// the asm returns to what's there: something that changed it
			// (a fuzzed table popping past its frame) isn't comparable
			if (gb_pop16(gb_) != static_cast<uint16_t>(ret + n * 7))
				if (gb_->test.active)
					gb_->test.unrealistic = true;
		}
	}

	void update_objects_temple()
	{
		handle_grabbed_object_collisions();
		update_all(UPD_TEMPLE, RET_TEMPLE_LOOP);
	}


	void port_UpdateObjects()
	{
		wr8(wEffectivePowerUp, (rd8(wTransformation) & 0x80) ? SUPER_JUMP_SLAM_OVERALLS : rd8(wPowerUpLevel));
		if (rd8(wLevel) == THE_TEMPLE)
			update_objects_temple();
		else if (rd8(wIsFloorTransition))
			update_all(UPD_FLOOR, RET_FLOOR_LOOP);
		else {
			handle_grabbed_object_collisions();
			update_all(UPD_NORMAL, RET_UPDATE_LOOP);
		}
	}
};

} // namespace

// The homejp state wrappers (Func_3173 to Func_3407 in home): the state they
// jump to, with its bank mapped for it as homejp and HomeJumpRet do. No
// caller reads the registers these leave.
bool object_state(GB *gb, uint16_t wrapper)
{
	struct Entry { uint16_t wrapper; uint8_t bank; void (ObjectEngine::*fn)(); };
	static const Entry entries[] = {
		{ADDR_Func_3173, BANK_Func_62e57, &ObjectEngine::port_Func_62e57},
		{ADDR_VanishObject2, BANK__VanishObject2, &ObjectEngine::port__VanishObject2},
		{ADDR_Func_3191, BANK_Func_62e8f, &ObjectEngine::port_Func_62e8f},
		{ADDR_Func_31a0, BANK_Func_62ea5, &ObjectEngine::port_Func_62ea5},
		{ADDR_Func_31af, BANK_Func_62f76, &ObjectEngine::port_Func_62f76},
		{ADDR_Func_31be, BANK_Func_62f8c, &ObjectEngine::port_Func_62f8c},
		{ADDR_Func_31cd, BANK_Func_62eaa, &ObjectEngine::port_Func_62eaa},
		{ADDR_Func_31dc, BANK_Func_62ec0, &ObjectEngine::port_Func_62ec0},
		{ADDR_Func_31eb, BANK_Func_62f91, &ObjectEngine::port_Func_62f91},
		{ADDR_Func_31fa, BANK_Func_62fa7, &ObjectEngine::port_Func_62fa7},
		{ADDR_Func_3209, BANK_Func_62e74, &ObjectEngine::port_Func_62e74},
		{ADDR_Func_3218, BANK_Func_62e8a, &ObjectEngine::port_Func_62e8a},
		{ADDR_Func_3227, BANK_Func_62f5b, &ObjectEngine::port_Func_62f5b},
		{ADDR_Func_3236, BANK_Func_62f71, &ObjectEngine::port_Func_62f71},
		{ADDR_Func_3245, BANK_Func_62c9f, &ObjectEngine::port_Func_62c9f},
		{ADDR_Func_3254, BANK_Func_62d7d, &ObjectEngine::port_Func_62d7d},
		{ADDR_Func_3263, BANK_Func_6303f, &ObjectEngine::port_Func_6303f},
		{ADDR_Func_3272, BANK_Func_63050, &ObjectEngine::port_Func_63050},
		{ADDR_Func_3281, BANK_Func_6305f, &ObjectEngine::port_Func_6305f},
		{ADDR_ObjState_Fall, BANK__ObjState_Fall, &ObjectEngine::port__ObjState_Fall},
		{ADDR_ObjState_StandingFall, BANK__ObjState_StandingFall, &ObjectEngine::port__ObjState_StandingFall},
		{ADDR_Func_32ae, BANK_Func_62ae5, &ObjectEngine::port_Func_62ae5},
		{ADDR_Func_32bd, BANK_Func_62bce, &ObjectEngine::port_Func_62bce},
		{ADDR_Func_32cc, BANK_Func_62ca8, &ObjectEngine::port_Func_62ca8},
		{ADDR_Func_32db, BANK_Func_62d86, &ObjectEngine::port_Func_62d86},
		{ADDR_Func_32ea, BANK_Func_628ea, &ObjectEngine::port_Func_628ea},
		{ADDR_Func_32f9, BANK_Func_629a6, &ObjectEngine::port_Func_629a6},
		{ADDR_Func_3308, BANK_Func_62908, &ObjectEngine::port_Func_62908},
		{ADDR_Func_3317, BANK_Func_62898, &ObjectEngine::port_Func_62898},
		{ADDR_Func_3326, BANK_Func_62840, &ObjectEngine::port_Func_62840},
		{ADDR_Func_3335, BANK_Func_61f54, &ObjectEngine::port_Func_61f54},
		{ADDR_Func_3344, BANK_Func_620a6, &ObjectEngine::port_Func_620a6},
		{ADDR_Func_3353, BANK_Func_621fb, &ObjectEngine::port_Func_621fb},
		{ADDR_Func_3362, BANK_Func_622bd, &ObjectEngine::port_Func_622bd},
		{ADDR_Func_3371, BANK_Func_62926, &ObjectEngine::port_Func_62926},
		{ADDR_Func_3380, BANK_Func_62382, &ObjectEngine::port_Func_62382},
		{ADDR_Func_338f, BANK_Func_6247b, &ObjectEngine::port_Func_6247b},
		{ADDR_Func_339e, BANK_Func_62574, &ObjectEngine::port_Func_62574},
		{ADDR_Func_33ad, BANK_Func_62605, &ObjectEngine::port_Func_62605},
		{ADDR_ObjState_FullThrowLeft, BANK_Func_626da, &ObjectEngine::port_Func_626da},
		{ADDR_ObjState_FullThrowRight, BANK_Func_62768, &ObjectEngine::port_Func_62768},
		{ADDR_ObjState_BumpLeft, BANK__ObjState_BumpLeft, &ObjectEngine::port__ObjState_BumpLeft},
		{ADDR_ObjState_BumpRight, BANK__ObjState_BumpRight, &ObjectEngine::port__ObjState_BumpRight},
		{ADDR_Func_33f8, BANK_Func_62892, &ObjectEngine::port_Func_62892},
		{ADDR_Func_3407, BANK_Func_6283a, &ObjectEngine::port_Func_6283a},
	};
	for (const Entry &e : entries) {
		if (e.wrapper != wrapper)
			continue;
		uint8_t bank = Memory(gb)[wROMBank];
		home::bankswitch(gb, e.bank, wrapper);
		ObjectEngine engine(gb);
		engine.plain_ = true;
		(engine.*e.fn)();
		home::bankswitch(gb, bank, ADDR_HomeJumpRet);
		return true;
	}
	return false;
}

// The movement tables and counts, plain, for the objects (object.hpp)
void apply_obj_movement(GB *gb, bool x, uint16_t table, bool loop)
{
	ObjectEngine(gb).apply_obj_movement(x ? wCurObjVar2 : wCurObjVar3, x ? OBJ_X_POS : OBJ_Y_POS, table, loop);
}

void apply_obj_set_movement(GB *gb, uint16_t start) { ObjectEngine(gb).apply_obj_set_movement(start); }
void move_platform_y(GB *gb, uint16_t table) { ObjectEngine(gb).move_platform_y(table); }
uint8_t count_active_transient_objects(GB *gb) { return ObjectEngine(gb).count_active_transient_objects(); }
uint8_t count_same_transient_objects(GB *gb) { return ObjectEngine(gb).count_same_transient_objects(); }

} // namespace wl3::level

using wl3::level::ObjectEngine;

// The entries farcalled from the level loop and start_room, and the update
// loop's own: their callers read no register after them (live.py).
// internal (the update loop calls them directly)
PORT_COVERED(CheckObjectOnScreen);
PORT_COVERED(Func_62d57);
PORT_COVERED(Func_62e31);
PORT_COVERED(Func_6284f);
PORT_COVERED(Func_628a7);
PORT_COVERED(Func_618b4);
PORT_COVERED(HandleGrabbedObjectCollisions);
PORT_COVERED(UpdateObject);
PORT_COVERED(UpdateObject_Temple);
PORT_COVERED(UpdateObjectDuringFloorTransition);
PORT_COVERED(UpdateObjects_Temple);
PORT_COVERED(Func_627f0);
PORT_COVERED(Func_62690);
PORT_COVERED(Func_62d34);
PORT_COVERED(Func_62e0f);
PORT_COVERED(ThrowObject_Left);
PORT_COVERED(ThrowObject_Right);
PORT_COVERED(DoObjectAction);
PORT_COVERED(ClearObjects);
PORT_COVERED(ClearObjDataPointers);
PORT_EX(DrawObjects_NoPriority, 0, ALL_REGISTERS) { ObjectEngine(gb).port_DrawObjects_NoPriority(); }
PORT_EX(DrawObjects_WithPriority, 0, ALL_REGISTERS) { ObjectEngine(gb).port_DrawObjects_WithPriority(); }
PORT_EX(Func_62ec3, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62ec3(); }
PORT_EX(Func_62faa, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62faa(); }
PORT_EX(_CountActiveTransientObjects, 0, R_A | R_F | R_B | R_C) { ObjectEngine(gb).port__CountActiveTransientObjects(); }
PORT_EX(Func_628ea, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_628ea(); }
PORT_EX(Func_62e74, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62e74(); }
PORT_EX(Func_62e8a, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62e8a(); }
PORT_EX(Func_62e8f, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62e8f(); }
PORT_EX(Func_62ea5, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62ea5(); }
PORT_EX(Func_62eaa, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62eaa(); }
PORT_EX(Func_62ec0, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62ec0(); }
PORT_EX(Func_62f5b, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62f5b(); }
PORT_EX(Func_62f71, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62f71(); }
PORT_EX(Func_62f76, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62f76(); }
PORT_EX(Func_62f8c, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62f8c(); }
PORT_EX(Func_62f91, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62f91(); }
PORT_EX(Func_62fa7, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62fa7(); }
PORT_EX(Func_6283a, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_6283a(); }
PORT_EX(Func_62840, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62840(); }
PORT_EX(Func_62892, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62892(); }
PORT_EX(Func_62898, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62898(); }
PORT_EX(Func_62908, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62908(); }
PORT_EX(Func_62926, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62926(); }
PORT_EX(Func_629a6, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_629a6(); }
PORT_EX(_ObjState_StandingFall, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port__ObjState_StandingFall(); }
PORT_EX(_ObjState_Fall, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port__ObjState_Fall(); }
PORT_EX(Func_6303f, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_6303f(); }
PORT_EX(Func_63050, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_63050(); }
PORT_EX(Func_6305f, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_6305f(); }
PORT_EX(Func_62ae5, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62ae5(); }
PORT_EX(Func_62bce, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62bce(); }
PORT_EX(_ObjState_BumpRight, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port__ObjState_BumpRight(); }
PORT_EX(_ObjState_BumpLeft, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port__ObjState_BumpLeft(); }
PORT_EX(Func_62ca8, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62ca8(); }
PORT_EX(Func_62d86, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62d86(); }
PORT_EX(Func_62c9f, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62c9f(); }
PORT_EX(Func_62d7d, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62d7d(); }
PORT_EX(VanishObject_NoSFX, 0, ALL_REGISTERS) { ObjectEngine(gb).port_VanishObject_NoSFX(); }
PORT_EX(_VanishObject, 0, ALL_REGISTERS) { ObjectEngine(gb).port__VanishObject(); }
PORT_EX(InitObjectVars_FromLevelStart, 0, ALL_REGISTERS) { ObjectEngine(gb).port_InitObjectVars_FromLevelStart(); }
PORT_EX(InitObjectVars_FromTransition, 0, ALL_REGISTERS) { ObjectEngine(gb).port_InitObjectVars_FromTransition(); }
PORT_EX(_ApplyObjYMovement, 0, R_A | R_F | R_B | R_C) { ObjectEngine(gb).port__ApplyObjYMovement(); }
PORT_EX(_ApplyObjXMovement, 0, R_A | R_F | R_B | R_C) { ObjectEngine(gb).port__ApplyObjXMovement(); }
PORT_EX(_ApplyObjYMovement_Loop, 0, R_A | R_F | R_B | R_C) { ObjectEngine(gb).port__ApplyObjYMovement_Loop(); }
PORT_EX(_ApplyObjXMovement_Loop, 0, R_A | R_F | R_B | R_C) { ObjectEngine(gb).port__ApplyObjXMovement_Loop(); }
PORT_EX(Func_63209, 0, ALL_REGISTERS) { ObjectEngine(gb).port_Func_63209(); }
PORT_EX(_ApplyObjSetMovement, 0, ALL_REGISTERS) { ObjectEngine(gb).port__ApplyObjSetMovement(); }
PORT_EX(Func_632ac, 0, ALL_REGISTERS & ~R_E) { ObjectEngine(gb).port_Func_632ac(); }
PORT_EX(Func_62e57, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62e57(); }
PORT_EX(_VanishObject2, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port__VanishObject2(); }
PORT_EX(Func_626da, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_626da(); }
PORT_EX(Func_62768, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62768(); }
PORT_EX(Func_62574, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62574(); }
PORT_EX(Func_62605, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62605(); }
PORT_EX(Func_61f54, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_61f54(); }
PORT_EX(Func_620a6, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_620a6(); }
PORT_EX(Func_621fb, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_621fb(); }
PORT_EX(Func_622bd, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_622bd(); }
PORT_EX(Func_6247b, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_6247b(); }
PORT_EX(Func_62382, HOOK_MANUAL_RET, ALL_REGISTERS) { ObjectEngine(gb).port_Func_62382(); }
PORT_EX(UpdateObjects, 0, ALL_REGISTERS) { ObjectEngine(gb).port_UpdateObjects(); }
