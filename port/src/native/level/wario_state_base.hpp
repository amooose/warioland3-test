// A base for Wario's states (wario_states_1-4.cpp) as plain C++: the
// state helpers read and write memory and return values, leaving no
// registers or flags. Their hooks clobber every register (the asm callers
// read none after them, checked with the liveness script).
//
// Register traffic stays only where a register-level callee takes or
// returns one (CreateParticle's b, the floor transitions' c), in the
// helpers below that wrap it. The asm-style AsmModule helpers that leave
// a and f for a flag test are deleted here, so a leftover one won't build.
#pragma once

#include "../asm_module.hpp"
#include "../home/floor_transition.hpp"
#include "../home/memory.hpp"
#include "../home/pals.hpp"
#include "../home/sprites.hpp"
#include "../home/system.hpp"
#include "../home/wario.hpp"
#include "../routines.hpp"
#include "object.hpp"

namespace wl3::level {

class WarioStateBase : public AsmModule {
public:
	using AsmModule::AsmModule; // (gb) or (gb, code_bank)

protected:
	void st(uint16_t addr, uint8_t v) { wr8(addr, v); }
	bool ld_and(uint16_t addr) const { return rd8(addr) != 0; }
	bool ld_bit(uint16_t addr, int n) const { return rd8(addr) >> n & 1; }
	bool ld_mask(uint16_t addr, uint8_t mask) const { return (rd8(addr) & mask) != 0; }
	void ld(uint16_t) = delete;
	void xor_a() = delete;
	void ld_cp(uint16_t, uint8_t) = delete;
	void b_and() = delete;
	void collision_check(uint8_t, uint16_t, void (*)(GB *)) = delete;
	void collision(uint8_t, uint16_t, void (*)(GB *)) = delete;
	void run_centre_collision() = delete;
	void run_front_collision() = delete;
	void run_up_collision() = delete;
	void run_airborne_collision() = delete;
	void flip_dir_and() = delete;
	void push_af() = delete;
	void pop_af() = delete;

	void play_sfx(uint16_t sfx)
	{
		wr8(hSFXID + 0, static_cast<uint8_t>(sfx >> 8));
		mem_.sfx().lo() = static_cast<uint8_t>(sfx);
	}

	// Wario's collision checks (farcalled): the result the asm leaves in b
	uint8_t centre_collision() { return collision(BANK_CheckCentreCollision, ADDR_CheckCentreCollision, check_centre_collision); }
	uint8_t front_collision() { return collision(BANK_CheckFrontCollision, ADDR_CheckFrontCollision, check_front_collision); }
	uint8_t up_collision() { return collision(BANK_CheckUpCollision, ADDR_CheckUpCollision, check_up_collision); }
	uint8_t airborne_collision()
	{
		return collision(BANK_CheckAirborneCollision, ADDR_CheckAirborneCollision, check_airborne_collision);
	}
	uint8_t collision(uint8_t bank, uint16_t addr, uint8_t (*check)(GB *))
	{
		uint8_t result = 0;
		routines_.in_bank(bank, addr, [&] { result = check(gb_); });
		return result;
	}

	// Func_19741 / Func_197b1: the bottom collision at the box's right or
	// left edge, breaking blocks (farcalled)
	uint8_t side_collision(bool right)
	{
		if (right)
			return collision(BANK_Func_19741, ADDR_Func_19741, check_func_19741);
		return collision(BANK_Func_197b1, ADDR_Func_197b1, check_func_197b1);
	}

	// wDMASourceBank/Ptr = gfx, call LoadWarioGfx
	void load_gfx(uint8_t bank, uint16_t ptr)
	{
		mem_.dma_source_bank() = bank;
		wr8(wDMASourcePtr + 0, static_cast<uint8_t>(ptr >> 8));
		wr8(wDMASourcePtr + 1, static_cast<uint8_t>(ptr));
		home::load_wario_gfx(gb_);
	}
	void set_oam(uint8_t bank, uint16_t ptr)
	{
		mem_.wario_oam_bank() = bank;
		wr8(wWarioOAMPtr + 0, static_cast<uint8_t>(ptr >> 8));
		wr8(wWarioOAMPtr + 1, static_cast<uint8_t>(ptr));
	}
	void set_frameset(uint16_t ptr)
	{
		wr8(wWarioFramesetPtr + 0, static_cast<uint8_t>(ptr >> 8));
		wr8(wWarioFramesetPtr + 1, static_cast<uint8_t>(ptr));
	}
	void frameset_by_dir(uint16_t left, uint16_t right) { set_frameset(facing_right() ? right : left); }
	// ld a, BANK("Wario OAM n") / ldh [hCallFuncBank], a / hcall UpdateWarioAnimation
	void anim(uint8_t bank)
	{
		home::farcall(gb_, bank, ADDR_UpdateWarioAnimation, [&] { home::update_wario_animation(gb_, bank); });
	}
	void set_pal(uint16_t pals) { home::set_wario_pal(gb_, pals); } // SetWarioPal

	void coll_box_blrt() { collision_box(-1, -27, -9, 9); }
	void coll_box_btlr() { collision_box(-1, -27, -9, 9); }
	void collision_box(int8_t bottom, int8_t top, int8_t left, int8_t right)
	{
		mem_.collision_box_bottom() = static_cast<uint8_t>(bottom);
		mem_.collision_box_top() = static_cast<uint8_t>(top);
		mem_.collision_box_left() = static_cast<uint8_t>(left);
		mem_.collision_box_right() = static_cast<uint8_t>(right);
	}
	// Wario's y from hYPos (where the last check left the point)
	void update_pos_y()
	{
		wr8(wWarioYPos + 0, mem_.y_pos_hi());
		mem_.wario_y().lo() = rd8(hYPosHi + 1);
	}
	// hPos = Wario's position
	void get_pos()
	{
		for (int i = 0; i < 4; i++)
			wr8(hPos + i, rd8(wWarioPos + i));
	}
	// a particle at hPos (CreateParticle takes it in b)
	void particle(uint8_t id)
	{
		Routines(gb_).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, id); });
	}
	void particle_at_wario(uint8_t id)
	{
		get_pos();
		particle(id);
	}
	// the sfx every reload frames
	void sfx_loop(uint8_t reload, uint16_t sfx)
	{
		uint8_t v = mem_.sfx_loop_counter();
		mem_.sfx_loop_counter() = static_cast<uint8_t>(v - 1);
		if (v >= 1)
			return;
		mem_.sfx_loop_counter() = reload;
		play_sfx(sfx);
	}
	bool room_transition() const { return ld_and(wRoomTransitionParam); }
	bool still_in(uint8_t state) const { return mem_.wario_state() == state; }
	// farcall Func_19b25 / the room transition check; true if it started one
	bool room_transition_check()
	{
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (room_transition()) {
			home::trigger_room_transition(gb_);
			return true;
		}
		return false;
	}
	// the room transition check, then back to normal Wario in water; true
	// if either happened
	bool transition_or_water()
	{
		if (room_transition_check())
			return true;
		if (ld_and(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return true;
		}
		return false;
	}
	// UpdateWarioAnimation in OAM bank `bank`: true once it has ended
	bool anim_and_ended(uint8_t bank)
	{
		anim(bank);
		return anim_ended();
	}
	// cap wWalkVelIndex at max (to v)
	void cap_walk(uint8_t max, uint8_t v)
	{
		if (mem_.walk_vel_index() >= max)
			mem_.walk_vel_index() = v;
	}
	// The transformations' d-pad walk: false if neither way is held; else
	// *turn when it points away from wDirection (which then takes it), or
	// WalkIfPossible_Right / Left.
	bool walk_or_turn(bool *turn)
	{
		uint8_t pad = mem_.joypad_down();
		if (pad & PAD_RIGHT) {
			*turn = !facing_right();
			if (*turn)
				mem_.direction() = DIRECTION_RIGHT;
			else
				FARCALL(WalkIfPossible_Right);
		} else if (pad & PAD_LEFT) {
			*turn = mem_.direction() != DIRECTION_LEFT;
			if (*turn)
				mem_.direction() = DIRECTION_LEFT;
			else
				FARCALL(WalkIfPossible_Left);
		} else {
			return false;
		}
		return true;
	}
	// the jump velocity table's entry for wJumpVelIndex
	uint8_t jump_vel(uint16_t table) const { return rd8(static_cast<uint16_t>(table + mem_.jump_vel_index())); }
	// Wario moves by the jump velocity table (JumpVelTable_Normal) at wJumpVelIndex, up or down
	// (MoveWarioUp / Down); rising, the index just goes up, else it stops
	// at MAX_JUMP_VEL_INDEX. True if he rose.
	bool jump_vel_move(uint16_t table = ADDR_JumpVelTable_Normal)
	{
		uint8_t v = jump_vel(table);
		uint8_t index = static_cast<uint8_t>(mem_.jump_vel_index() + 1);
		if (v & 0x80) {
			move_y(-static_cast<uint8_t>(-v)); // cpl / inc a
			mem_.jump_vel_index() = index;
			return true;
		}
		move_y(v);
		mem_.jump_vel_index() = index >= MAX_JUMP_VEL_INDEX ? MAX_JUMP_VEL_INDEX : index;
		return false;
	}
	bool anim_ended() const { return ld_and(wWarioAnimationEnded); }
	bool facing_right() const { return ld_and(wDirection); }

	uint8_t floor_for_y_pos() const { return home::floor_for_y_pos(gb_); } // GetFloorForYPos
	// after moving down, the floor transition down if the camera does
	// transitions and Wario went past his floor
	void down_floor_transition() { home::handle_downwards_floor_transition(gb_); }
	// after moving up, the floor transition up (true if it started)
	bool up_floor_transition() { return home::handle_upwards_floor_transition(gb_); }
	// ApplyWalkVelocity_Right / Left, then MoveWarioRight / Left by it: how
	// far Wario moved
	uint8_t walk_with_velocity(bool right)
	{
		uint8_t vel = home::apply_walk_velocity(gb_, right);
		move_x(right ? vel : -vel);
		return vel;
	}
	// IsOnSlipperyGround: true if either foot is on slippery ground
	bool on_slippery_ground() { return home::is_on_slippery_ground(gb_); }

	void move_x(int delta) { home::move_wario_x(gb_, delta); }
	void move_y(int delta) { home::move_wario_y(gb_, delta); }
	// MoveWarioRight / Left by the direction
	void move_by_dir(int n) { move_x(facing_right() ? n : -n); }
	void center_on_ladder() { mem_.wario_x().lo() = static_cast<uint8_t>((mem_.wario_x().lo() & 0xf0) + 0x08); }
	// ld hl, wWarioStateCounter / inc [hl]: the new count
	uint8_t inc_counter()
	{
		uint8_t v = static_cast<uint8_t>(mem_.wario_state_counter() + 1);
		mem_.wario_state_counter() = v;
		return v;
	}
	void clear(std::initializer_list<uint16_t> addrs)
	{
		for (uint16_t a : addrs)
			wr8(a, 0);
	}
	void clear(const uint16_t *addrs)
	{
		for (; *addrs; addrs++)
			wr8(*addrs, 0);
	}
	void reset_anim() { clear({wFrameDuration, wAnimationFrame}); }
	void touch_states(uint8_t t, uint8_t ca94)
	{
		mem_.touch_state() = t;
		mem_.sting_touch_state() = t;
		mem_.unk_ca94() = ca94;
	}
	// the new direction
	uint8_t flip_dir()
	{
		uint8_t dir = mem_.direction() ^ 1;
		mem_.direction() = dir;
		return dir;
	}
};

} // namespace wl3::level
