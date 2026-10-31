// The Swimming Flippers demonstration, and what the swimming demonstrations
// share; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void SwimmingFlippersDemo::run()
{
	run_state();
	draw();
}

void WaterDemo::draw()
{
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen())
		animate_wario(0x05);
	load_wario_sprite();
}

void SwimmingFlippersDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: init_demo(); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: wait_then_walk(); break;
	case 3: walk_to_water(); break;
	case 4: jump_in(); break;
	case 5:
		if (!timer_done())
			break;
		show_wario_submerge();
		next_state();
		break;
	case 6: flash_ddown(); break;
	case 7: dive(); break;
	case 8: swim_right(); break;
	case 9: swim_down(0x03, false); break;
	case 10: swim_left_up(); break;
	case 11: swim_up(); break;
	case 12: flash_b(); break;
	case 13: thrust(); break;
	case 14: swim_down(0x03, true); break;
	case 15: swim_back(); break;
	case 16: thrust_again(); break;
	case 17: swim_left_up(); break;
	case 18: surface(); break;
	case 19: jump_out(); break;
	case 20: land(); break;
	default: unrealistic(); break;
	}
}

// Wario stands by the water, in the water palettes, the water animating.
void WaterDemo::start()
{
	queue_wario_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	MenuObject w = wario();
	w.init(0x40, 0x18, ADDR_Frameset_1425f);
	w.var() = 0;
	mem_[wActionHelpWarioOAMPtr + 0] = static_cast<uint8_t>(ADDR_OAM_14000 >> 8);
	mem_[wActionHelpWarioOAMPtr + 1] = static_cast<uint8_t>(ADDR_OAM_14000);
	wram_.animated_tiles_frame_duration() = 6;
	wram_.animated_tiles_gfx() = ANIMATED_TILES_GFX_02;
	wram_.room_animated_tiles_enabled() = TRUE;
	timer() = 0x30;
	routines_.farcall(BANK_LoadActionHelpPalsWithWater, ADDR_LoadActionHelpPalsWithWater);
	next_state();
}

void WaterDemo::swim()
{
	play_sfx_rept(37, SFX_00E);
}

void WaterDemo::swim_fast()
{
	play_sfx_rept(15, SFX_00F);
}

void WaterDemo::walk_to_water()
{
	if (++wario().x() < 0x2c)
		return;
	show_wario_jump();
	wario_var() = 0x18;
	next_state();
}

void WaterDemo::jump_in()
{
	++wario().x();
	if (jump_above(0x50))
		return;
	wario_var() = 0;
	show_wario_swim();
	clear_dpad();
	timer() = 0x30;
	next_state();
}

void WaterDemo::dive()
{
	swim();
	if (wram_.action_help_counter() & 0x03)
		return;
	if (++wario().y() < 0x58)
		return;
	show_dright();
	timer() = 0x10;
	next_state();
}

void WaterDemo::swim_right()
{
	swim();
	if (!wait_then_every(0x01))
		return;
	if (++wario().x() < 0x6c)
		return;
	show_ddown();
	timer() = 0x10;
	next_state();
}

void WaterDemo::swim_down(uint8_t mask, bool dright)
{
	swim();
	if (!wait_then_every(mask))
		return;
	if (++wario().y() < 0x60)
		return;
	if (dright)
		show_dright();
	else
		clear_dpad();
	next_state();
}

void WaterDemo::swim_left_up()
{
	swim();
	if (--wario().x() >= 0x34)
		return;
	wario().x() = 0x34;
	show_dup();
	timer() = 0x10;
	next_state();
}

void SwimmingFlippersDemo::swim_up()
{
	swim();
	if (!wait_then_every(0x03))
		return;
	if (--wario().y() >= 0x58)
		return;
	show_wario_underwater_thrust();
	clear_dpad();
	next_state();
}

// The underwater thrust: faster, until past x $58 every other frame.
void SwimmingFlippersDemo::thrust()
{
	swim_fast();
	if (wario().x() >= 0x58 && (wram_.action_help_counter() & 0x01))
		return;
	if (++wario().x() < 0x6c)
		return;
	show_wario_submerge();
	show_ddown();
	clear_buttons();
	timer() = 0x10;
	next_state();
}

void SwimmingFlippersDemo::swim_back()
{
	swim();
	if (--wario().x() >= 0x50)
		return;
	wario().x() = 0x50;
	show_wario_underwater_thrust();
	show_b();
	next_state();
}

void SwimmingFlippersDemo::thrust_again()
{
	swim_fast();
	if (++timer() < 0x30)
		return;
	timer() = 0;
	show_wario_submerge();
	clear_buttons();
	next_state();
}

void WaterDemo::surface()
{
	swim();
	if (!wait_then_every(0x03))
		return;
	if (--wario().y() >= 0x50)
		return;
	show_wario_swim_left();
	clear_dpad();
	timer() = 0x30;
	next_state();
}

void WaterDemo::jump_out()
{
	if (!timer_done())
		return;
	show_wario_jump_left();
	show_dleft();
	show_a();
	wario_var() = 0;
	next_state();
}

void WaterDemo::land()
{
	if (wario().x() != 0x18)
		--wario().x();
	jump_wario();
	if (wario_var() < 0x18 || wario().y() < 0x40)
		return;
	wario().y() = 0x40;
	wario_var() = 0;
	finish([this] { start(); });
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(SwimmingFlippersActionHelp);
PORT_COVERED(Func_1f8e31);
PORT_COVERED(Func_1f8e69);
PORT_COVERED(Func_1f8e64);
PORT_COVERED(Func_1f8ed9);
PORT_COVERED(Func_1f8ee9);
PORT_COVERED(Func_1f8efe);
PORT_COVERED(Func_1f8f2c);
PORT_COVERED(Func_1f8f39);
PORT_COVERED(Func_1f8f6b);
PORT_COVERED(Func_1f8fa6);
PORT_COVERED(Func_1f8fdc);
PORT_COVERED(Func_1f900a);
PORT_COVERED(Func_1f9043);
PORT_COVERED(Func_1f9080);
PORT_COVERED(Func_1f90b6);
PORT_COVERED(Func_1f90e2);
PORT_COVERED(Func_1f910e);
PORT_COVERED(Func_1f913c);
PORT_COVERED(Func_1f917a);
PORT_COVERED(Func_1f9191);
