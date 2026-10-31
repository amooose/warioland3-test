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
	// Wario's jump, its velocity index held at the table's last entry
	uint8_t jump_wario_capped();
	bool jump_towards_enemy(); // Wario's jump; whether he's still left of it
	void jump_onto_blocks(); // onto the enemy's blocks, at y $20
	void high_jump(); // up to y $0c, then the smash attack
	// the velocity of Wario's next jump step, its index held at the
	// table's last entry
	uint8_t capped_velocity();
	// the slide (wMenuObj4)
	void jump_over(); // over it as it moves left
	void ride_slide(); // on top of it, until it goes down
	void slide_down(); // Wario down to the ground as it moves left
	void roll(); // off its end
	void bounce_off(); // bounced back by a block, and lands
	void approach_last_slide();

	// the ground shaking after a ground pound: the screen bobs, at_5 is
	// called 5 frames from the end; whether it's over
	bool ground_shake(uint16_t at_5);

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
	// an object whose framesets are in the bank at bank_field; whether the
	// animation restarted goes to ended_field (unless 0)
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
	void slide_comes(); // Func_1f86ee
	void approach_slide_again(); // Func_1f8708
	void jump_onto_slide(); // Func_1f8730
	void roll_to_block(); // Func_1f87bd
	void bump_while_rolling(); // Func_1f87d6
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
	void jump_at_enemy(); // Func_1f89fb
	void pound_enemy(); // Func_1f8a2a
	void land(); // Func_1f8a52
	void blocks_come(); // Func_1f8a79
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

// power_ups/super_jump_slam_overalls.asm: Wario's super ground pound
// shakes the ground, stunning enemies and breaking blocks below.
class SuperJumpSlamOverallsDemo : public Demo {
public:
	using Demo::Demo;

	void run(); // SuperJumpSlamOverallsActionHelp

private:
	void run_state();
	void start(); // Func_1fa4ea
	void approach(); // Func_1fa526
	void jump_up(); // Func_1fa54c
	void slam(); // Func_1fa574
	void paragoom_bounces(); // Func_1fa5a8
	void slam_paragoom(); // Func_1fa602
	void slam_again(); // Func_1fa653
	void land(uint8_t wait); // Func_1fa698, Func_1fa80b
	void slam_blocks(); // Func_1fa76e
	void break_lower_block(); // Func_1fa796
	void shake_then_jump(); // Func_1fa7d0
};

// What the grabbing demonstrations share (grab_gloves.asm and
// super_grab_gloves.asm): Wario carries an enemy (wMenuObj4) over his head
// and throws it, forwards, high, and fully charged at a wall (wMenuObj6).
class GrabDemo : public Demo {
public:
	using Demo::Demo;

protected:
	void hold(uint8_t above, uint8_t ahead); // the enemy, relative to Wario
	void idle_when_done();
	// the throw's arc: forwards to the ground, a bounce, then stun (the
	// enemy's stunned look); with pillar, the enemy breaks it on the way
	void throw_arc(uint16_t stun, bool pillar = false);
	void high_throw_arc(uint16_t stun); // up, over, down and a bounce
	void charged(); // Func_1f9afb, Func_1fb4fa
	void charged_throw(uint16_t thrown); // Func_1f9b2c, Func_1fb52b
	void hit_wall(); // Func_1f9b56, Func_1fb555
	void enemy_gone(); // Func_1f9b7a, Func_1fb579
	template <class F>
	void walk_off(F restart) // Func_1f9b8f, Func_1fb58e
	{
		Byte x = object(wMenuObj6).x();
		dec_twice_odd(x);
		if (x >= 0xe8)
			finish(restart);
	}
};

// power_ups/super_grab_gloves.asm: Wario picks up Count Richtertoff and
// throws him, high, and fully charged at a wall.
class SuperGrabGlovesDemo : public GrabDemo {
public:
	using GrabDemo::GrabDemo;

	void run(); // SuperGrabGloveActionHelp

private:
	void run_state();
	void start(); // Func_1fb1b1
	void grab(); // Func_1fb1d4, Func_1fb302, Func_1fb44e
	void lift(); // Func_1fb1f6, Func_1fb324, Func_1fb470
	void throw_count(); // Func_1fb25d, Func_1fb397
	void approach_wall(); // Func_1fb4d2
};

// power_ups/grab_gloves.asm: Wario stuns a Para-Goom, carries it and
// throws it, high, into a pillar, and fully charged at a wall.
class GrabGlovesDemo : public GrabDemo {
public:
	using GrabDemo::GrabDemo;

	void run(); // GrabGloveActionHelp

private:
	void run_state();
	void start(); // Func_1f95a1
	void stomp(); // Func_1f95f9
	void bounce_off(); // Func_1f963f
	void grab(); // Func_1f9695 and others
	void lift(); // Func_1f96bf and others
	void throw_paragoom();
	void throw_after_wait(); // Func_1f9708, Func_1f9981
	void throw_up(); // Func_1f9824
	void grab_walk(); // Func_1f9953, Func_1f9abb
	bool walk_to_pillar(uint8_t x); // whether he's there
};

// power_ups/garlic.asm: with garlic, Wario's charge breaks blocks and
// knocks out Count Richtertoff.
class GarlicDemo : public Demo {
public:
	using Demo::Demo;

	void run(); // GarlicActionHelp

private:
	void run_state();
	void start(); // Func_1f9d1f
	bool move_blocks(); // whether they're close enough
	void push_blocks(int times); // both blocks (wMenuObj4, wMenuObj6) left
	bool attack_countdown(); // whether the charge is on
	bool attack_blocks(); // whether they're pushed far enough
	bool attack_enemy();
	void break_blocks();
	void hide_debris();
	void break_first_block(); // Func_1f9d97
	void break_second_block(); // Func_1f9e07
	void charge_ends(); // Func_1f9e62
	void break_blocks_again(); // Func_1f9eb7
	void bounce_back(); // Func_1f9f35, Func_1f9ff4
	void break_block_at_count(); // Func_1f9f9e
	void knock_out_count(); // Func_1fa072
	void count_flies(); // Func_1fa0c2
	void bump_count(); // Func_1fa223
	void count_falls(); // Func_1fa248
	void blocks_come(); // Func_1fa298
	void blocks_break(); // Func_1fa2d4
};

// What the transformations' demonstrations share (owl.asm, rail.asm,
// vampire.asm): they run in the bank of common2.asm, so they call its
// sprite routines and the ActionHelp2_* prompts directly, and end without
// a level-clear case.
class SpecialDemo : public Demo {
public:
	using Demo::Demo;

protected:
	void load_object_sprite_near(MenuObject obj); // call LoadActionHelpSprite_Far
	void load_wario_sprite_from_bank(); // LoadActionHelpWarioSprite_FromBank
	void hide_block_debris(MenuObject obj); // farcall ActionHelp_HideBlockDebris
	void clear_dpad2() { call(ADDR_ActionHelp2_ClearDPadInput); }
	void clear_buttons2() { call(ADDR_ActionHelp2_ClearButtonsInput); }

	// The end: back to the title screen, or again from the start.
	template <class F>
	void finish_special(F restart)
	{
		if (wram_.power_up_level().bit(ACTION_HELP_TITLE_SCREEN_F)) {
			call(ADDR_InitWithoutDemoPowerUpReset);
			return;
		}
		clear_dpad2();
		clear_buttons2();
		wram_.action_help_state() = 1;
		restart();
	}
};

// power_ups/owl.asm: Owl Wario hangs from the owl and flies about.
class OwlDemo : public SpecialDemo {
public:
	using SpecialDemo::SpecialDemo;

	void run(); // OwlActionHelp

private:
	void run_state();
	void start(); // Func_1e409a
	bool flap(uint8_t frames, bool every_other); // whether to move this frame
	void fly_left(); // Func_1e4104
	void fly_up(); // Func_1e4134
	void fly_down(); // Func_1e4165
	void fly_left_again(); // Func_1e4195
	void fly_right(); // Func_1e41c9
	void glide_left(); // Func_1e41fd
	void glide_right(); // Func_1e4227
	void drop(); // Func_1e4254
};

// power_ups/rail.asm: Wario rides a rail, jumps off and back on.
class RailDemo : public SpecialDemo {
public:
	using SpecialDemo::SpecialDemo;

	void run(); // RailActionHelp

private:
	void run_state();
	void start(); // Func_1e4329
	void ride_left(); // Func_1e43a0, Func_1e4490
	bool ride_right(); // Func_1e43cb, Func_1e44bb: whether at the end
	void land();
	void rail_leaves(); // Func_1e4432, Func_1e452a
	void rail_back(); // Func_1e4442, Func_1e453a
	void grab_rail(bool reset_sfx_timer); // Func_1e4455, Func_1e454d
	void fall(); // Func_1e44ee
};

// power_ups/vampire.asm: Vampire Wario turns into a bat and flies.
class VampireDemo : public SpecialDemo {
public:
	using SpecialDemo::SpecialDemo;

	void run(); // VampireActionHelp

private:
	void run_state();
	void start(); // Func_1e45f3
	void drift_left(); // every other frame
	void rise(uint8_t fast_from); // y up 1, or 2 from y fast_from on
	void flap_up(uint8_t fast_from, uint8_t to); // ApplyWaterCurrentMovementb and others
	void glide(); // Func_1e46e8, Func_1e4759, Func_1e47ca
	void fly_home(); // Func_1e47fe
	void land(); // Func_1e483d
};

} // namespace wl3::action_help
