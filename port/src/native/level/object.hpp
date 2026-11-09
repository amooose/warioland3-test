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
#include "../home/pals.hpp"
#include "../home/memory.hpp"
#include "../home/floor_transition.hpp"
#include "../home/sprites.hpp"
#include "../home/system.hpp"
#include "../home/wario.hpp"
#include <initializer_list>

namespace wl3::level {

// A homejp state wrapper (Func_3173 to Func_3407, ObjState_*) run plain:
// false if `wrapper` isn't one (update_objects.cpp)
bool object_state(GB *gb, uint16_t wrapper);
// CreateParticle: particle `id` at hYPos / hXPos, in the next of the 4
// slots; returns the address past what it wrote (particles.cpp)
uint16_t create_particle(GB *gb, uint8_t id);
// CreateParticleInBlock: a particle at the block's position in the level map
void create_particle_in_block(GB *gb, uint16_t block, uint8_t id);
void block_to_hpos(GB *gb, uint16_t block); // Func_ce3e
// _CreateObjectFromCurObjPos: an object from its creation data, at wCurObj's position
void create_object_from_cur_obj_pos(GB *gb, uint16_t params);
// SpawnObject: index = its wObjDataPointers entry, at = its object map entry
void spawn_object(GB *gb, uint8_t index, uint16_t at);
// The object's place in the level map, from its OBJ_UNK_02 at `at`:
void despawn_object(GB *gb, uint16_t at); // DespawnObject
void set_object_block(GB *gb, uint16_t at, uint8_t nybble); // Func_ba42
void clear_object_block(GB *gb, uint16_t at); // Func_bb2d
// The object engine's movement tables and counts (update_objects.cpp):
// _ApplyObjX/YMovement(_Loop) (by OBJ_VAR_2 / OBJ_VAR_3), _ApplyObjSet-
// Movement, Func_63209 (a platform's y movement, carrying Wario),
// _CountActiveTransientObjects and Func_632ac (the transient objects
// with the current one's id)
void apply_obj_movement(GB *gb, bool x, uint16_t table, bool loop);
void apply_obj_set_movement(GB *gb, uint16_t start);
void move_platform_y(GB *gb, uint16_t table);
uint8_t count_active_transient_objects(GB *gb);
uint8_t count_same_transient_objects(GB *gb);

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
	// A home routine by address, plain where the objects call it plain:
	// MoveObject* (1 pixel, _Fast 2, _Slow 1 every other frame: right and
	// down on even frames, left and up on odd ones; ByVar2 by var2), the
	// homejp state wrappers (object_state), DecrementRemainingBossHits and
	// the floor transitions (which no object reads a or c of).
	// Anything else is called.
	void call_home(uint16_t routine);
	// MoveWarioRight / Left / Down / Up (by address): Wario and the camera
	// moved n pixels (the asm's b, which no object reads after)
	void move_wario(uint16_t routine, uint8_t n);
	// DecrementRemainingBossHits: a boss's hits left (the substate's low
	// nibble) down one, unless it's the last
	void decrement_boss_hits()
	{
		if ((substate() & 0x0f) != 1)
			substate() = static_cast<uint8_t>(substate() - 1);
	}
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
	// The homecall macro around a plain fn: bank mapped, then the caller's
	// again (the wrapper's address standing for home code's)
	template <class F>
	void homecall(uint8_t bank, uint16_t wrapper, F fn)
	{
		uint8_t saved = mem_[wROMBank];
		home::bankswitch(gb_, bank, wrapper);
		fn();
		home::bankswitch(gb_, saved, wrapper);
	}
	// ApplyObjY/XMovement(_Loop): the y or x position by the next step of
	// the movement table (the _Loop ones start over at its end)
	void apply_movement(uint16_t wrapper, uint8_t bank, bool x, uint16_t table, bool loop)
	{
		homecall(bank, wrapper, [&] { level::apply_obj_movement(gb_, x, table, loop); });
	}
	void apply_y_movement(uint16_t table) { apply_movement(ADDR_ApplyObjYMovement, BANK__ApplyObjYMovement, false, table, false); }
	void apply_y_movement_loop(uint16_t table) { apply_movement(ADDR_ApplyObjYMovement_Loop, BANK__ApplyObjYMovement_Loop, false, table, true); }
	void apply_x_movement(uint16_t table) { apply_movement(ADDR_ApplyObjXMovement, BANK__ApplyObjXMovement, true, table, false); }
	void apply_x_movement_loop(uint16_t table) { apply_movement(ADDR_ApplyObjXMovement_Loop, BANK__ApplyObjXMovement_Loop, true, table, true); }
	// ApplyObjSetMovement: the set movement starting at `start`
	void apply_set_movement(uint16_t start)
	{
		homecall(BANK__ApplyObjSetMovement, ADDR_ApplyObjSetMovement, [&] { level::apply_obj_set_movement(gb_, start); });
	}
	// Func_34fc: a platform's y movement by the table, carrying Wario
	void platform_y_movement(uint16_t table)
	{
		homecall(BANK_Func_63209, ADDR_Func_34fc, [&] { level::move_platform_y(gb_, table); });
	}
	// CountActiveTransientObjects: how many active transient objects
	uint8_t active_transient_objects()
	{
		uint8_t n = 0;
		homecall(BANK__CountActiveTransientObjects, ADDR_CountActiveTransientObjects,
			[&] { n = level::count_active_transient_objects(gb_); });
		return n;
	}
	// Func_3444: how many transient objects share this one's id
	uint8_t same_transient_objects()
	{
		uint8_t n = 0;
		homecall(BANK_Func_632ac, ADDR_Func_3444, [&] { n = level::count_same_transient_objects(gb_); });
		return n;
	}
	// a new object from its creation data (bc), at the data's offset from
	// this one, or where this one is
	void create_object_at_offset(uint16_t params) { call_bc(ADDR_CreateObjectAtRelativePos, params); }
	void create_object_here(uint16_t params) { call_bc(ADDR_CreateObjectFromCurObjPos, params); }
	// DoGroundShake: the ground shakes for `frames` (b), unless it already does
	void ground_shake(uint8_t frames)
	{
		home::do_ground_shake(gb_, frames);
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
	// CopyAndApplyOBPals: count OB palettes from src to dest (wTempPals2's)
	// and applied from OB palette `first`, one per H-Blank
	void copy_and_apply_ob_pals(uint16_t src, uint16_t dest, uint8_t count, uint8_t first)
	{
		home::copy_and_apply_ob_pals(gb_, src, dest, count, first);
	}
	// SetBGPals / SetOBPals, BlackOutBGPals / BlackOutOBPals: count
	// palettes from palette `first`, from src or black, one per H-Blank
	void set_bg_pals(uint16_t src, uint8_t count, uint8_t first) { home::set_pals(gb_, true, src, count, first); }
	void set_ob_pals(uint16_t src, uint8_t count, uint8_t first) { home::set_pals(gb_, false, src, count, first); }
	void black_out_bg_pals(uint8_t count, uint8_t first) { home::black_out_pals(gb_, true, count, first); }
	void black_out_ob_pals(uint8_t count, uint8_t first) { home::black_out_pals(gb_, false, count, first); }
	// CreateParticle (farcalled): particle `id` at hYPos / hXPos
	// Func_ba42: this object's entry in the level map becomes `nybble`
	void set_block_object(uint8_t nybble);
	void particle(uint8_t id)
	{
		routines_.in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, id); });
	}
	void vanish() { call_home(ADDR_Func_3173); } // puff, then Func_4c860 despawns it
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
