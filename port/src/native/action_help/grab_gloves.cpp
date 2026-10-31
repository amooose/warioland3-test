// The Grab Gloves demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void GrabGlovesDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	MenuObject debris = object(wMenuObj5);
	animate_object(debris, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
	load_object_sprite(debris);
	if (!wram_.action_help_frozen()) {
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_wario(0x05);
		wario().animation_ended() = wram_.obj_anim_was_reset();
	}
	load_object_sprite(enemy());
	load_wario_sprite();
	MenuObject pillar = object(wMenuObj6);
	animate_object(pillar, wMenuObj6FramesetBank, 0);
	load_object_sprite(pillar);
}

void GrabGlovesDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: {
		Byte x = enemy().x();
		dec_twice_odd(x);
		if (x < 0x70)
			start_jump();
		break;
	}
	case 3: stomp(); break;
	case 4: bounce_off(); break;
	case 5: wait_then_walk(); break;
	case 6: grab(); break;
	case 7: lift(); break;
	case 8:
		if (!timer_done())
			break;
		call(ADDR_ActionHelp_ShowWarioBeginThrow);
		next_state();
		break;
	case 9: call(ADDR_ActionHelp_FlashBInput); break;
	case 10: throw_after_wait(); break;
	case 11: throw_arc(ADDR_ActionHelp_ShowParaGoomStun); break;
	case 12: wait_then_walk(); break;
	case 13: grab(); break;
	case 14: lift(); break;
	case 15:
		if (timer_done())
			next_state();
		break;
	case 16: call(ADDR_ActionHelp_FlashDUpInput); break;
	case 17: throw_up(); break;
	case 18: high_throw_arc(ADDR_ActionHelp_ShowParaGoomStun); break;
	case 19: wait_then_walk(); break;
	case 20: grab(); break;
	case 21: lift(); break;
	case 22: grab_walk(); break;
	case 23:
		if (walk_to_pillar(0x5c))
			next_state();
		break;
	case 24: throw_after_wait(); break;
	case 25: throw_arc(ADDR_ActionHelp_ShowParaGoomStun, true); break;
	case 26:
		if (wram_.menu_obj5_animation_ended())
			hide_obj5_debris();
		wait_then_walk();
		break;
	case 27: grab(); break;
	case 28: lift(); break;
	case 29: grab_walk(); break;
	case 30:
		if (!walk_to_pillar(0x80))
			break;
		timer() = 0x50;
		next_state();
		break;
	case 31:
		if (!timer_done())
			break;
		call(ADDR_ActionHelp_ShowWarioChargeThrow);
		next_state();
		break;
	case 32: charged(); break;
	case 33: charged_throw(ADDR_ActionHelp_ShowParaGoomThrown); break;
	case 34: hit_wall(); break;
	case 35: enemy_gone(); break;
	case 36: walk_off([this] { start(); }); break;
	default: unrealistic(); break;
	}
}

// Wario walks in towards a Para-Goom; a pillar (wMenuObj6) comes later.
void GrabGlovesDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	call(ADDR_ActionHelp_ShowParaGoomWalk);
	hide_obj5_debris();
	init_object(object(wMenuObj6), 0x40, 0xe8, ADDR_Frameset_1e6b6a, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	load_pals_without_water();
	next_state();
}

// Wario lands on the Para-Goom, stunning it.
void GrabGlovesDemo::stomp()
{
	jump_wario();
	dec_twice_odd(enemy().x());
	if (wario_var() < 0x18 || wario().y() < 0x36)
		return;
	clear_dpad();
	clear_buttons();
	play_sfx(mem_, SFX_014);
	wario_var() = 0x0a;
	call(ADDR_ActionHelp_ShowParaGoomStun);
	next_state();
}

// Wario bounces off; the Para-Goom flies up with him and both land.
void GrabGlovesDemo::bounce_off()
{
	MenuObject paragoom = enemy();
	++paragoom.x();
	if (wram_.action_help_counter() & 0x01)
		++paragoom.x();
	uint8_t velocity = jump_velocity(wario_var());
	uint8_t paragoom_y = static_cast<uint8_t>(paragoom.y() + velocity);
	if (paragoom_y < 0x40)
		paragoom.y() = paragoom_y;
	if ((wario().y() += velocity) < 0x40)
		return;
	wario().y() = 0x40;
	paragoom.y() = 0x40;
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioIdle);
	timer() = 0x20;
	next_state();
}

void GrabGlovesDemo::grab()
{
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x48)
		return;
	hold(0x05, 0x10);
	call(ADDR_ActionHelp_ShowWarioGrab);
	clear_dpad();
	next_state();
}

// Wario lifts the Para-Goom over his head.
void GrabGlovesDemo::lift()
{
	switch (++timer()) {
	case 0x03: hold(0x0c, 0x10); return;
	case 0x06: hold(0x1c, 0x06); return;
	}
	if (!wario().animation_ended())
		return;
	timer() = 0x30;
	call(ADDR_ActionHelp_ShowWarioBeginGrab);
	next_state();
}

void GrabGlovesDemo::throw_paragoom()
{
	call(ADDR_ActionHelp_ShowWarioThrow);
	call(ADDR_ActionHelp_ShowParaGoomThrown);
	clear_buttons();
	next_state();
}

void GrabGlovesDemo::throw_after_wait()
{
	if (++timer() < 0x30)
		return;
	timer() = 0;
	throw_paragoom();
}

// Up and B: the high throw.
void GrabGlovesDemo::throw_up()
{
	uint8_t t = ++timer();
	if (t == 0x01) {
		call(ADDR_ActionHelp_ShowWarioBeginThrow);
		show_b();
		return;
	}
	if (t < 0x30)
		return;
	timer() = 0;
	throw_paragoom();
}

void GrabGlovesDemo::grab_walk()
{
	if (!timer_done())
		return;
	call(ADDR_ActionHelp_ShowWarioGrabWalk);
	show_dright();
	next_state();
}

bool GrabGlovesDemo::walk_to_pillar(uint8_t x)
{
	Byte pillar_x = object(wMenuObj6).x();
	dec_twice_odd(pillar_x);
	if (pillar_x >= x)
		return false;
	call(ADDR_ActionHelp_ShowWarioBeginThrow);
	clear_dpad();
	show_b();
	return true;
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(GrabGloveActionHelp);
PORT_COVERED(Func_1f9569);
PORT_COVERED(Func_1f95a1);
PORT_COVERED(Func_1f959c);
PORT_COVERED(Func_1f95de);
PORT_COVERED(Func_1f95f9);
PORT_COVERED(Func_1f963f);
PORT_COVERED(Func_1f9685);
PORT_COVERED(Func_1f9695);
PORT_COVERED(Func_1f96bf);
PORT_COVERED(Func_1f96fb);
PORT_COVERED(Func_1f9708);
PORT_COVERED(Func_1f9720);
PORT_COVERED(Func_1f97a2);
PORT_COVERED(Func_1f97b2);
PORT_COVERED(Func_1f97dc);
PORT_COVERED(Func_1f981a);
PORT_COVERED(Func_1f9824);
PORT_COVERED(Func_1f9847);
PORT_COVERED(Func_1f98db);
PORT_COVERED(Func_1f98eb);
PORT_COVERED(Func_1f9915);
PORT_COVERED(Func_1f9953);
PORT_COVERED(Func_1f9963);
PORT_COVERED(Func_1f9981);
PORT_COVERED(Func_1f9999);
PORT_COVERED(Func_1f9a3a);
PORT_COVERED(Func_1f9a53);
PORT_COVERED(Func_1f9a7d);
PORT_COVERED(Func_1f9abb);
PORT_COVERED(Func_1f9acb);
PORT_COVERED(Func_1f9aee);
PORT_COVERED(Func_1f9afb);
PORT_COVERED(Func_1f9b2c);
PORT_COVERED(Func_1f9b56);
PORT_COVERED(Func_1f9b7a);
PORT_COVERED(Func_1f9b8f);
