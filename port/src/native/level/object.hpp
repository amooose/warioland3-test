// The levels' objects (src/engine/level/objects), as C++.
//
// The object being updated is copied to wCurObj (an obj_struct) by
// UpdateObject, which calls its update function (wCurObjUpdateFunction,
// in its Object Functions bank) through hCallFunc and copies it back. Each
// object is a class over `Object`, in its own file; `Object` reaches
// wCurObj's fields and the home routines objects use (src/home/
// object_mechanics.asm and the like, still C, called by address). An object's states stay addresses in the
// ROM (ADDR_FooFunc + offset, the asm's local labels), written to
// wCurObjUpdateFunction as the asm does, so save states and the rest of
// the game see the same bytes; each is a hook calling the class.
//
// Update functions are only called by UpdateObject's hCallFunc, after
// which UpdateObjectAnimation (or UpdateObjectFrame) and
// CheckObjectOnScreen load every register before reading it: the hooks
// clobber every register.
#pragma once

#include "../native.hpp"
#include "../ram.hpp"
#include "../routines.hpp"
#include "../home/object_mechanics.hpp"
#include <initializer_list>

namespace wl3::level {

class Object {
public:
	explicit Object(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

protected:
	// an input the game never gives (a state out of a jumptable's range)
	void unrealistic()
	{
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	// wCurObj's fields
	Byte flags() const { return mem_[wCurObjFlags]; }
	Byte substate() const { return mem_[wCurObjSubState]; }
	Byte state() const { return mem_[wCurObjState]; }
	Byte state_duration() const { return mem_[wCurObjStateDuration]; }
	Byte frame() const { return mem_[wCurObjFrame]; }
	Byte frame_duration() const { return mem_[wCurObjFrameDuration]; }
	Byte interaction() const { return mem_[wCurObjInteractionType]; }
	Byte screen_x() const { return mem_[wCurObjScreenXPos]; }
	Byte screen_y() const { return mem_[wCurObjScreenYPos]; }
	Byte var1() const { return mem_[wCurObjVar1]; }
	Byte var2() const { return mem_[wCurObjVar2]; }
	Byte var3() const { return mem_[wCurObjVar3]; }
	Word y() const { return mem_.word(wCurObjYPos); }
	Word x() const { return mem_.word(wCurObjXPos); }

	bool facing_right() const { return substate() & OBJSUBFLAG_HDIR; }
	void set_update(uint16_t f) // high byte first, as the asm
	{
		mem_[wCurObjUpdateFunction + 1] = f >> 8;
		mem_[wCurObjUpdateFunction] = f & 0xff;
	}
	// the collision box's top, bottom, left and right
	void set_coll_box(uint8_t top, uint8_t bottom, uint8_t left, uint8_t right);
	// keeps HEAVY_OBJ
	void set_interaction(uint8_t type) { interaction() = (interaction() & HEAVY_OBJ) | type; }

	// dec [hl] at any address: the value it computed (a write to the MBC
	// doesn't read back)
	uint8_t dec_at(uint16_t addr)
	{
		uint8_t v = static_cast<uint8_t>(mem_[addr] - 1);
		mem_[addr] = v;
		return v;
	}

	// no action for a frame (no_actions_for 1)
	void no_actions() { mem_[wCurObjAction] = (1 << NO_ACTIONS_F) | 1; }

	// home routines
	void set_frameset(uint16_t frameset); // SetObjectFramesetPtr (plain: leaves no registers)
	// home's MoveObject* (1 pixel) and MoveObject*ByVar2, plain: they leave
	// no registers (the asm leaves hl in wCurObj and b = var2; callers that
	// want those use the addresses and var2())
	void move_left() { home::move_object_x(gb_, -1); }
	void move_right() { home::move_object_x(gb_, 1); }
	void move_up() { home::move_object_y(gb_, -1); }
	void move_down() { home::move_object_y(gb_, 1); }
	void move_left_by_var2() { home::move_object_x(gb_, -var2()); }
	void move_right_by_var2() { home::move_object_x(gb_, var2()); }
	void move_up_by_var2() { home::move_object_y(gb_, -var2()); }
	void move_down_by_var2() { home::move_object_y(gb_, var2()); }
	// home's MoveObject* routines by address (1 pixel, _Fast 2, _Slow 1
	// every other frame: right and down on even frames, left and up on odd
	// ones; ByVar2 by var2), plain; anything else is called
	void move_by(uint16_t routine);
	// The block at hPos through ProcessBlock, by the collision points it's
	// processed with (Func_3513 and the like: homecalls of collisions.asm's
	// routines): the block function's result, which they leave in a. The
	// CPU keeps the rest of the block function's registers, which some
	// objects read on.
	uint8_t hpos_block_unk4() { return call_a(ADDR_Func_3513); } // COLLISION_UNK4
	uint8_t hpos_block_unk1() { return call_a(ADDR_Func_352b); } // COLLISION_UNK1
	uint8_t hpos_block_down() { return call_a(ADDR_Func_3543); } // COLLISION_DOWN
	uint8_t hpos_block_unk3() { return call_a(ADDR_Func_358b); } // COLLISION_UNK3
	uint8_t hpos_block_unk2() { return call_a(ADDR_Func_35a3); } // COLLISION_UNK2
	// ProcessBlockOnRight / Left: the block beside the one at hl
	uint8_t block_beside(bool right) { return call_a(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft); }
	// The movement tables (bc): the next velocity at var3 (y) or var2 (x),
	// staying on the last entry, or the _Loop ones starting over
	void apply_y_movement(uint16_t table) { call_bc(ADDR_ApplyObjYMovement, table); }
	void apply_y_movement_loop(uint16_t table) { call_bc(ADDR_ApplyObjYMovement_Loop, table); }
	void apply_x_movement(uint16_t table) { call_bc(ADDR_ApplyObjXMovement, table); }
	void apply_x_movement_loop(uint16_t table) { call_bc(ADDR_ApplyObjXMovement_Loop, table); }
	// a new object from its creation data (bc), at the data's offset from
	// this one, or where this one is
	void create_object_at_offset(uint16_t params) { call_bc(ADDR_CreateObjectAtRelativePos, params); }
	void create_object_here(uint16_t params) { call_bc(ADDR_CreateObjectFromCurObjPos, params); }
	// DoGroundShake: the ground shakes for `frames` (b), unless it already does
	void ground_shake(uint8_t frames)
	{
		gb_->cpu.b = frames;
		routines_.call(ADDR_DoGroundShake);
	}
	// a home routine that takes bc
	void call_bc(uint16_t routine, uint16_t bc)
	{
		SET_BC(gb_, bc);
		routines_.call(routine);
	}
	// a home routine that returns its result in a
	uint8_t call_a(uint16_t routine)
	{
		routines_.call(routine);
		return gb_->cpu.a;
	}
	void vanish() { routines_.call(ADDR_Func_3173); } // puff, then Func_4c860 despawns it
	void one_way_platform() { routines_.call(ADDR_SetOneWayPlatformAction); } // SetOneWayPlatformAction
	void despawn(); // DespawnObject, with hl = wCurObjUnk02
	void collect(); // Func_bb2d, with hl = wCurObjUnk02
	// the usual start: visible, no collision, initialised; its update
	// function and frameset
	void start(uint16_t update, uint16_t frameset);
	void play_sfx(uint16_t sfx) { wl3::play_sfx(mem_, sfx); }
	// on the last tick of the given frame of the animation
	bool frame_ending(uint8_t f) const { return frame() == f && frame_duration() == 1; }

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

} // namespace wl3::level
