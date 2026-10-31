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

	// no action for a frame (no_actions_for 1)
	void no_actions() { mem_[wCurObjAction] = (1 << NO_ACTIONS_F) | 1; }

	// home routines
	void set_frameset(uint16_t frameset); // SetObjectFramesetPtr
	void move_left() { routines_.call(ADDR_MoveObjectLeft); }
	void move_right() { routines_.call(ADDR_MoveObjectRight); }
	void move_up() { routines_.call(ADDR_MoveObjectUp); }
	void move_down() { routines_.call(ADDR_MoveObjectDown); }
	// by var2
	void move_left_by_var2() { routines_.call(ADDR_MoveObjectLeftByVar2); }
	void move_right_by_var2() { routines_.call(ADDR_MoveObjectRightByVar2); }
	void move_up_by_var2() { routines_.call(ADDR_MoveObjectUpByVar2); }
	void move_down_by_var2() { routines_.call(ADDR_MoveObjectDownByVar2); }
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
