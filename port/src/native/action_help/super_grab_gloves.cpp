// The Super Grab Gloves demonstration, and what the grabbing demonstrations
// share; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void SuperGrabGlovesDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen()) {
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_wario(0x05);
		wario().animation_ended() = wram_.obj_anim_was_reset();
	}
	load_object_sprite(enemy());
	load_wario_sprite();
	MenuObject wall = object(wMenuObj6);
	animate_object(wall, wMenuObj6FramesetBank, 0);
	load_object_sprite(wall);
}

void SuperGrabGlovesDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: init_demo(); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: grab(); break;
	case 3: lift(); break;
	case 4:
		if (!timer_done())
			break;
		show_wario_begin_throw();
		show_b();
		timer() = 0x20;
		next_state();
		break;
	case 5: throw_count(); break;
	case 6: throw_arc(ADDR_ActionHelp_ShowCountRichtertoffStun); break;
	case 7: wait_then_walk(); break;
	case 8: grab(); break;
	case 9: lift(); break;
	case 10:
		if (!timer_done())
			break;
		timer() = 0x20;
		show_dup();
		next_state();
		break;
	case 11:
		if (!timer_done())
			break;
		timer() = 0x30;
		show_wario_begin_throw();
		show_b();
		next_state();
		break;
	case 12: throw_count(); break;
	case 13: high_throw_arc(ADDR_ActionHelp_ShowCountRichtertoffStun); break;
	case 14: wait_then_walk(); break;
	case 15: grab(); break;
	case 16: lift(); break;
	case 17:
		if (!timer_done())
			break;
		show_wario_grab_walk();
		show_dright();
		next_state();
		break;
	case 18: approach_wall(); break;
	case 19:
		if (!timer_done())
			break;
		show_wario_charge_throw();
		next_state();
		break;
	case 20: charged(); break;
	case 21: charged_throw(ADDR_ActionHelp_ShowCountRichtertoffThrown); break;
	case 22: hit_wall(); break;
	case 23: enemy_gone(); break;
	case 24: walk_off([this] { start(); }); break;
	default: unrealistic(); break;
	}
}

// Wario walks in; Count Richtertoff stands stunned under a pillar.
void SuperGrabGlovesDemo::start()
{
	show_initial_wario_walk();
	show_dright();
	show_count_richtertoff_idle();
	show_count_richtertoff_stun();
	show_overhead_pillar();
	load_pals_without_water();
	next_state();
}

void GrabDemo::hold(uint8_t above, uint8_t ahead)
{
	enemy().y() = static_cast<uint8_t>(wario().y() - above);
	enemy().x() = static_cast<uint8_t>(wario().x() + ahead);
}

void SuperGrabGlovesDemo::grab()
{
	if (--enemy().x() >= 0x48)
		return;
	hold(0x05, 0x10);
	show_wario_slow_grab();
	clear_dpad();
	next_state();
}

// Wario lifts the Count over his head.
void SuperGrabGlovesDemo::lift()
{
	switch (++timer()) {
	case 0x07: hold(0x04, 0x10); return;
	case 0x0d: hold(0x14, 0x10); return;
	case 0x0f: hold(0x1c, 0x06); return;
	}
	if (!wario().animation_ended())
		return;
	timer() = 0x30;
	show_wario_begin_grab();
	next_state();
}

void SuperGrabGlovesDemo::throw_count()
{
	if (!timer_done())
		return;
	show_wario_throw();
	show_count_richtertoff_thrown();
	clear_buttons();
	next_state();
}

void GrabDemo::idle_when_done()
{
	if (wario().animation_ended())
		show_wario_idle();
}

// The enemy's flight: forwards to the ground, then a bounce.
void GrabDemo::throw_arc(uint16_t stun, bool pillar)
{
	idle_when_done();
	uint8_t t = ++timer();
	Byte y = enemy().y();
	if (t < 0x10) {
		enemy().x() += 2;
		if (pillar && enemy().x() >= object(wMenuObj6).x()) {
			// the Para-Goom breaks the pillar (.Func_1f9a25)
			show_block_debris_with_y_offset(0x00);
			MenuObject debris = object(wMenuObj5);
			debris.y() = object(wMenuObj6).y();
			debris.x() = object(wMenuObj6).x();
			show_overhead_pillar();
		}
		if (t < 0x03)
			return;
		if (t < 0x06)
			++y;
		else if (t < 0x0a)
			y += 2;
		else if ((y += 3) >= 0x40) {
			y = 0x40;
			timer() = 0x10;
		}
		return;
	}
	enemy().x() += 1;
	if (t < 0x12)
		y -= 3;
	else if (t < 0x14)
		y -= 2;
	else if (t < 0x15)
		--y;
	else if (t < 0x16)
		timer() = 0x20;
	else if (t < 0x27)
		++y;
	else if ((y += 2) >= 0x40) {
		y = 0x40;
		show_wario_idle();
		call(stun);
		timer() = 0x20;
		next_state();
	}
}

// The high throw: up, over and down, then a bounce in place.
void GrabDemo::high_throw_arc(uint16_t stun)
{
	idle_when_done();
	uint8_t t = ++timer();
	Byte y = enemy().y();
	if (t < 0x30) {
		enemy().x() += 1;
		if (t < 0x07)
			y -= 4;
		else if (t < 0x0d)
			y -= 3;
		else if (t < 0x0f)
			y -= 2;
		else if (t < 0x11)
			--y;
		else if (t < 0x14) {
			// at the top
		} else if (t < 0x17)
			++y;
		else if (t < 0x1a)
			y += 2;
		else if (t < 0x1d)
			y += 3;
		else if ((y += 4) >= 0x40) {
			y = 0x40;
			timer() = 0x30;
		}
		return;
	}
	if (t < 0x32)
		y -= 3;
	else if (t < 0x34)
		y -= 2;
	else if (t < 0x35)
		--y;
	else if (t < 0x36)
		timer() = 0x40;
	else if (t < 0x47)
		++y;
	else if ((y += 2) >= 0x40) {
		y = 0x40;
		show_wario_idle();
		call(stun);
		timer() = 0x30;
		next_state();
	}
}

void SuperGrabGlovesDemo::approach_wall()
{
	if (--object(wMenuObj6).x() >= 0x80)
		return;
	show_wario_begin_throw();
	clear_dpad();
	show_b();
	timer() = 0x50;
	next_state();
}

void GrabDemo::charged()
{
	play_sfx_rept(30, SFX_FULLY_CHARGED_THROW);
	if (!wario().animation_ended())
		return;
	show_wario_fully_charged_grab();
	enemy().x() -= 8;
	timer() = 0x30;
	next_state();
}

void GrabDemo::charged_throw(uint16_t thrown)
{
	play_sfx_rept(30, SFX_FULLY_CHARGED_THROW);
	if (!timer_done())
		return;
	show_wario_fully_charged_throw();
	call(thrown);
	clear_buttons();
	next_state();
}

// The enemy flies into the wall and vanishes.
void GrabDemo::hit_wall()
{
	idle_when_done();
	uint8_t wall = static_cast<uint8_t>(object(wMenuObj6).x() - 0x10);
	if ((enemy().x() += 4) < wall)
		return;
	show_wario_idle();
	show_obj4_enemy_vanish();
	next_state();
}

void GrabDemo::enemy_gone()
{
	if (!wram_.menu_obj4_animation_ended())
		return;
	enemy().x() = 0xe8;
	show_wario_walk();
	show_dright();
	next_state();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(SuperGrabGloveActionHelp);
PORT_COVERED(Func_1fb179);
PORT_COVERED(Func_1fb1b1);
PORT_COVERED(Func_1fb1ac);
PORT_COVERED(Func_1fb1d4);
PORT_COVERED(Func_1fb1f6);
PORT_COVERED(Func_1fb248);
PORT_COVERED(Func_1fb25d);
PORT_COVERED(Func_1fb270);
PORT_COVERED(Func_1fb2f2);
PORT_COVERED(Func_1fb302);
PORT_COVERED(Func_1fb324);
PORT_COVERED(Func_1fb376);
PORT_COVERED(Func_1fb385);
PORT_COVERED(Func_1fb397);
PORT_COVERED(Func_1fb3aa);
PORT_COVERED(Func_1fb43e);
PORT_COVERED(Func_1fb44e);
PORT_COVERED(Func_1fb470);
PORT_COVERED(Func_1fb4c2);
PORT_COVERED(Func_1fb4d2);
PORT_COVERED(Func_1fb4ed);
PORT_COVERED(Func_1fb4fa);
PORT_COVERED(Func_1fb52b);
PORT_COVERED(Func_1fb555);
PORT_COVERED(Func_1fb579);
PORT_COVERED(Func_1fb58e);
