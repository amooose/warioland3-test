// What the power-up demonstrations (power_ups/*.asm) share: a state machine
// on wActionHelpState that moves Wario, an enemy or object (wMenuObj4) and
// block debris (wMenuObj5), and shows the inputs that do it.
//
// The prompts, Wario's animations and the other helpers they call
// (common1.asm, LoadActionHelpSprite_Far...) are still register-level C,
// called by address.
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::action_help {

class Demo {
public:
	explicit Demo(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

protected:
	MenuObject object(uint16_t base) const { return MenuObject(gb_, base); }
	MenuObject wario() const { return object(wActionHelpWario); }
	MenuObject enemy() const { return object(wMenuObj4); }
	Byte wario_var() const { return wram_.action_help_wario_var(); }
	Byte timer() const { return wram_.timer(); }
	void unrealistic();

	void next_state() { ++wram_.action_help_state(); }
	// by 1, or by 2 on odd frames
	void dec_twice_odd(Byte value);
	// one step of a jump: the next JumpVelTable_Normal entry
	uint8_t jump_velocity(Byte step);
	// Wario's jump: his y by the next velocity; returns it
	uint8_t jump_wario();
	void play_sfx_rept(uint8_t frames, uint16_t sfx); // play_sfx_rept

	bool timer_done() { return !--timer(); } // dec wTimer: whether it hit 0
	bool walk_wario_to_40(); // whether he's there
	void wait_then_walk(); // then Wario walks right
	void start_jump(); // the A prompt and Wario's jump; next state
	void start_attack(); // the B prompt and Wario's charge; next state
	// Wario's jump; whether he's still above y
	bool jump_above(uint8_t y)
	{
		jump_wario();
		return wario().y() < y;
	}
	// an object with its OAM data (and bank) set, and its animation not
	// ended
	void init_object(MenuObject obj, uint8_t y, uint8_t x, uint16_t frameset, uint16_t oam, uint8_t bank,
		bool reset_ended = true);

	// waits out wTimer, then goes on every (mask + 1) frames
	bool wait_then_every(uint8_t mask);
	// queues a DMA of $80 tiles of Wario's graphics into v0Tiles0
	void queue_wario_gfx(uint8_t bank, uint16_t gfx);

	// The demonstration's end: back to the level after a clear, or to the
	// title screen; from the pause menu, again from the start (restart).
	template <class F>
	void finish(F restart)
	{
		Byte level = wram_.power_up_level();
		if (level.bit(ACTION_HELP_LEVEL_CLEAR_F)) {
			level = level & POWER_UP_MASK;
			routines_.farcall(BANK_Func_4628, ADDR_Func_4628);
			call(ADDR_ResumeNormalVolume);
			return;
		}
		if (level.bit(ACTION_HELP_TITLE_SCREEN_F)) {
			call(ADDR_InitWithoutDemoPowerUpReset);
			return;
		}
		clear_dpad();
		clear_buttons();
		wram_.action_help_state() = 1;
		restart();
	}

	// animation and sprites
	// an object whose framesets are in the bank at bank_field
	void animate_object(MenuObject obj, uint16_t bank_field, uint16_t ended_field);
	void load_object_sprite(MenuObject obj); // LoadActionHelpSprite_Far
	void animate_wario(uint8_t bank); // UpdateObjAnim with bank mapped
	void load_wario_sprite(); // LoadActionHelpWarioSprite

	// common1.asm's states and helpers (still C)
	void call(uint16_t addr) { routines_.call(addr); }
	void clear_dpad() { call(ADDR_ActionHelp_ClearDPadInput); }
	void clear_buttons() { call(ADDR_ActionHelp_ClearButtonsInput); }
	void show_dright() { call(ADDR_ActionHelp_ShowDRightInput); }
	void show_a() { call(ADDR_ActionHelp_ShowAInput); }
	void show_b() { call(ADDR_ActionHelp_ShowBInput); }
	void hide_obj5_debris() { call(ADDR_ActionHelp_HideObj5BlockDebris); }
	// debris at obj's position, offset by (y, x)
	void show_block_debris(MenuObject obj, uint8_t y, uint8_t x);
	void load_pals_without_water();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

// power_ups/none.asm: no power-up. Wario jumps on a Para-Goom, attacks it,
// breaks a block, bumps a solid one and slides under a pillar.
class NoPowerUpDemo : public Demo {
public:
	using Demo::Demo;

	void run(); // NoPowerUpActionHelp

private:
	void run_state();
	void start(); // Func_1f839a

	void approach(); // Func_1f83ba
	void stomp(); // Func_1f83d5
	void land_on_paragoom(); // Func_1f842e
	void wait_then_attack(); // Func_1f847a
	bool attack_paragoom(uint8_t x_limit);
	void attack_until_gone(); // Func_1f848c
	void paragoom_falls(); // Func_1f84d7: and the blocks come in
	void approach_block(); // Func_1f8522
	void break_block(); // Func_1f853a
	void bounce_back(); // Func_1f8588, Func_1f8638
	void walk_to_block(); // Func_1f85d3
	void bump_solid_block(); // Func_1f85eb
	void approach_slide(); // Func_1f8683
	void jump_over(); // Func_1f86b5, Func_1f886b
	void slide_comes(); // Func_1f86ee
	void approach_slide_again(); // Func_1f8708
	void jump_onto_slide(); // Func_1f8730
	void ride_slide(); // Func_1f8788
	void slide_down();
	void roll(); // Func_1f87a1
	void roll_to_block(); // Func_1f87bd
	void bump_while_rolling(); // Func_1f87d6
	void bounce_off(); // Func_1f87fb
	void approach_last_slide(); // Func_1f884c
	void slide_leaves(); // Func_1f88a4
};

// power_ups/lead_overalls.asm: Wario's ground pound breaks blocks and
// squashes a Futamogu into the ground.
class LeadOverallsDemo : public Demo {
public:
	using Demo::Demo;

	void run(); // LeadOverallsActionHelp

private:
	void run_state();
	void start(); // Func_1f89c0
	void approach(); // Func_1f89e0, Func_1f8aa1
	bool jump_towards_enemy(); // whether Wario is still left of it
	void jump_at_enemy(); // Func_1f89fb
	void pound_enemy(); // Func_1f8a2a
	void land(); // Func_1f8a52
	void blocks_come(); // Func_1f8a79
	void jump_onto_blocks(); // Func_1f8abc
	void high_jump(); // Func_1f8b14
	void pound_top_block(); // Func_1f8b3d
	void pound_again(); // Func_1f8b71
	void pound_lower_block(); // Func_1f8b86
	void stand(); // Func_1f8bb2
	void jump_off(); // Func_1f8bc7
	void fall_off(); // Func_1f8bda: and a Futamogu comes
	bool move_futamogu(uint8_t x); // whether the debris is left of x
	void futamogu_comes(); // Func_1f8c59
	void jump_at_futamogu(); // Func_1f8c81
	void smash_down(); // Func_1f8c94
	void pound_futamogu(uint8_t y); // Func_1f8cbb, Func_1f8d04
	void smash_again(); // Func_1f8cf7
	void bounce(); // Func_1f8d34
	void land_on_futamogu(); // Func_1f8d49
	void futamogu_climbs(); // Func_1f8d72
	void climb_out(); // Func_1f8d88
	void walk_on(); // Func_1f8da4
	void futamogu_leaves(); // Func_1f8db7
};

// What the swimming demonstrations share (swimming_flippers.asm and
// prince_frogs_gloves.asm): Wario walks into the water, swims about, and
// jumps out.
class WaterDemo : public Demo {
public:
	using Demo::Demo;

protected:
	void draw(); // Wario's animation and sprite, after the state
	void start(); // Func_1f8e69, Func_1fad7a
	void swim(); // play_sfx_rept: the swimming sound
	void swim_fast(); // the underwater thrust's sound
	void walk_to_water(); // Func_1f8ee9, Func_1fadfa
	void jump_in(); // Func_1f8efe, Func_1fae0f
	void dive(); // Func_1f8f39, Func_1fae4d
	void swim_right(); // Func_1f8f6b, Func_1fae7f
	void swim_down(uint8_t mask, bool dright); // Func_1f8fa6, Func_1f9080, Func_1faeba
	void swim_left_up(); // Func_1f8fdc, Func_1f910e, Func_1faff5
	void surface(); // Func_1f913c, Func_1fb023
	void jump_out(); // Func_1f917a, Func_1fb061
	void land(); // Func_1f9191, Func_1fb078
};

// power_ups/swimming_flippers.asm: Wario swims, dives and thrusts
// underwater.
class SwimmingFlippersDemo : public WaterDemo {
public:
	using WaterDemo::WaterDemo;

	void run(); // SwimmingFlippersActionHelp

private:
	void run_state();
	void swim_up(); // Func_1f900a
	void thrust(); // Func_1f9043
	void swim_back(); // Func_1f90b6
	void thrust_again(); // Func_1f90e2
};

// power_ups/prince_frogs_gloves.asm: underwater, Wario's thrust keeps going
// with B held.
class PrinceFrogsGlovesDemo : public WaterDemo {
public:
	using WaterDemo::WaterDemo;

	void run(); // PrinceFrogsGlovesActionHelp

private:
	void run_state();
	void stop_at(uint8_t x, bool b_prompt); // Func_1faef0, Func_1faf4e
	bool thrust_right(uint8_t slow_from, uint8_t to); // whether he got there
	void thrust(); // Func_1faf19
	void short_thrust(); // Func_1faf7a
	void thrust_again(); // Func_1fafb1
};

// power_ups/head_smash_helmet.asm: Wario jumps into blocks above him,
// breaking them with his head.
class HeadSmashHelmetDemo : public Demo {
public:
	using Demo::Demo;

	void run(); // HeadSmashHelmetActionHelp

private:
	void run_state();
	void start(); // Func_1f92b2
	void block_above(); // Func_1f92ee, Func_1f93a3
	void head_smash(uint16_t next_block); // Func_1f9313, Func_1f93cb
	void land(); // Func_1f935b, Func_1f9413
	void hide_debris_when_done();
};

// power_ups/high_jump_boots.asm: Wario's high jump onto a tall block,
// and onto a Para-Goom.
class HighJumpBootsDemo : public Demo {
public:
	using Demo::Demo;

	void run(); // HighJumpBootsActionHelp

private:
	// how a step takes the next velocity from wActionHelpWarioVar
	enum class Step { Next, Current, NextCapped };
	// Wario's y by the next velocity in table; returns his y
	uint8_t jump_by(uint16_t table, Step step);

	void run_state();
	void start(); // Func_1fa94a
	void rise(); // Func_1fa9a9: up to the jump's peak
	void high_jump_up(); // Func_1fa9d3, Func_1fac71
	void land_on_top(bool dright);
	void jump_down(); // Func_1faa1c
	void jump_on_paragoom(); // Func_1faa64
	void bounce_back(); // Func_1faa99
	void jump_on_block(); // Func_1faacc
	void jump_smash(); // Func_1faafb
	void smash_paragoom(); // Func_1fab19
	void bounce_up(); // Func_1fab55
	void bounce_on(); // Func_1fab83
	void fall(); // Func_1fabb2
	void land_after_jump(); // Func_1fac08
	void jump_off(); // Func_1facb8
};

} // namespace wl3::action_help
