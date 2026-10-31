// The Vampire demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void VampireDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen()) {
		animate_wario(0x7f);
		wario().animation_ended() = wram_.obj_anim_was_reset();
	}
	load_wario_sprite_from_bank();
}

void VampireDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: init_demo(); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2:
		if (++wario().x() < 0x80)
			break;
		clear_dpad();
		show_wario_bat_transform();
		next_state();
		break;
	case 3: flash_b_far(); break;
	case 4:
		if (!timer()) {
			play_sfx(mem_, SFX_046);
			timer() = 0x01;
			break;
		}
		if (!wario().animation_ended())
			break;
		show_wario_bat(ADDR_Frameset_1fd46c);
		timer() = 0x30;
		next_state();
		break;
	case 5:
		play_sfx_rept(32, SFX_FLAP);
		if (!timer_done())
			break;
		show_dleft();
		show_wario_bat(ADDR_Frameset_1fd42c);
		next_state();
		break;
	case 6: flash_a_far(); break;
	case 7: flap_up(0x30, 0x20); break;
	case 8: glide(); break;
	case 9: flap_up(0x2c, 0x1c); break;
	case 10: glide(); break;
	case 11: flap_up(0x28, 0x18); break;
	case 12: glide(); break;
	case 13: fly_home(); break;
	case 14: land(); break;
	case 15:
		play_sfx_rept(32, SFX_FLAP);
		if (!timer_done())
			break;
		show_b();
		show_wario_vampire_transform();
		next_state();
		break;
	case 16:
		if (wario().animation_ended())
			finish_special([this] { start(); });
		break;
	default: unrealistic(); break;
	}
}

// Vampire Wario walks in.
void VampireDemo::start()
{
	show_dright();
	queue_wario_gfx(BANK_WarioVampireGfx, ADDR_WarioVampireGfx);
	MenuObject w = wario();
	w.init(0x40, 0x30, ADDR_Frameset_1fd82e);
	w.var() = 0;
	mem_[wActionHelpWarioOAMPtr + 0] = static_cast<uint8_t>(ADDR_OAM_1fd498 >> 8);
	mem_[wActionHelpWarioOAMPtr + 1] = static_cast<uint8_t>(ADDR_OAM_1fd498);
	wram_.action_help_wario_oam_bank() = BANK_OAM_1fd498;
	call(ADDR_LoadActionHelpPalsWithoutWater);
	next_state();
}

void VampireDemo::drift_left()
{
	if (wram_.action_help_counter() & 0x01)
		--wario().x();
}

void VampireDemo::rise(uint8_t fast_from)
{
	Byte y = wario().y();
	if (y >= fast_from)
		--y;
	--y;
}

// A: the bat flaps up, drifting left.
void VampireDemo::flap_up(uint8_t fast_from, uint8_t to)
{
	play_sfx_rept(16, SFX_FLAP);
	drift_left();
	rise(fast_from);
	if (wario().y() >= to)
		return;
	clear_buttons();
	wario_var() = 0x18;
	next_state();
}

// then glides down
void VampireDemo::glide()
{
	drift_left();
	wario().y() += mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + wario_var())];
	if (++wario_var() < 0x27)
		return;
	wario_var() = 0;
	wram_.sfx_timer() = 0;
	show_a();
	next_state();
}

void VampireDemo::fly_home()
{
	play_sfx_rept(16, SFX_FLAP);
	rise(0x20);
	if (!(wram_.action_help_counter() & 0x01))
		return;
	if (--wario().x() != 0x30)
		return;
	clear_buttons();
	clear_dpad();
	wario_var() = 0x18;
	next_state();
}

void VampireDemo::land()
{
	Byte step = wario_var();
	if (step != 0x27)
		++step;
	if ((wario().y() += mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + step)]) < 0x40)
		return;
	wario().y() = 0x40;
	wario_var() = 0;
	show_wario_bat(ADDR_Frameset_1fd46c);
	timer() = 0x30;
	next_state();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(VampireActionHelp);
PORT_COVERED(Func_1e45d7);
PORT_COVERED(Func_1e45f3);
PORT_COVERED(Func_1e45ee);
PORT_COVERED(Func_1e464b);
PORT_COVERED(Func_1e465e);
PORT_COVERED(Func_1e4684);
PORT_COVERED(ApplyWaterCurrentMovementb);
PORT_COVERED(Func_1e46e8);
PORT_COVERED(Func_1e471c);
PORT_COVERED(Func_1e4759);
PORT_COVERED(Func_1e478d);
PORT_COVERED(Func_1e47ca);
PORT_COVERED(Func_1e47fe);
PORT_COVERED(Func_1e483d);
PORT_COVERED(Func_1e4871);
PORT_COVERED(Func_1e4898);
