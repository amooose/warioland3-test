// The Owl demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void OwlDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen()) {
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_object(wario(), wActionHelpWarioOAMBank, 0);
	}
	load_wario_sprite_from_bank();
	load_object_sprite(enemy());
}

void OwlDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: init_demo(); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: fly_left(); break;
	case 3: fly_up(); break;
	case 4: fly_down(); break;
	case 5: fly_left_again(); break;
	case 6: fly_right(); break;
	case 7: glide_left(); break;
	case 8: glide_right(); break;
	case 9: flash_b_far(); break;
	case 10: drop(); break;
	case 11:
		enemy().y() -= 2;
		if (!timer_done())
			break;
		show_dright();
		show_wario_walk2();
		next_state();
		break;
	case 12:
		if (++wario().x() >= 0xb0)
			finish_special([this] { start(); });
		break;
	default: unrealistic(); break;
	}
}

// Wario flies in from the right, hanging from the owl.
void OwlDemo::start()
{
	queue_wario_gfx(BANK_WarioHangGfx, ADDR_WarioHangGfx);
	MenuObject w = wario();
	w.init(0x38, 0xc0, ADDR_Frameset_1fed7c);
	w.var() = 0;
	wram_.sfx_timer() = 0;
	mem_[wActionHelpWarioOAMPtr + 0] = static_cast<uint8_t>(ADDR_OAM_1fddb4 >> 8);
	mem_[wActionHelpWarioOAMPtr + 1] = static_cast<uint8_t>(ADDR_OAM_1fddb4);
	wram_.action_help_wario_oam_bank() = BANK_OAM_1fddb4;
	hide_block_debris_far(enemy());
	call(ADDR_LoadActionHelpPalsWithoutWater);
	next_state();
}

bool OwlDemo::flap(uint8_t frames, bool every_other)
{
	play_sfx_rept(frames, SFX_FLAP);
	return !every_other || !(wram_.action_help_counter() & 0x01);
}

void OwlDemo::fly_left()
{
	if (!flap(32, true))
		return;
	if (--wario().x() >= 0x60)
		return;
	show_dup();
	show_wario_hang_vertical();
	next_state();
}

void OwlDemo::fly_up()
{
	if (!flap(32, true))
		return;
	uint8_t y = --wario().y();
	if (y == 0x34) {
		clear_dpad();
		return;
	}
	if (y < 0x20)
		next_state();
}

void OwlDemo::fly_down()
{
	if (!flap(32, true))
		return;
	if (++wario().y() < 0x38)
		return;
	show_dleft();
	show_wario_hang_horizontal();
	next_state();
}

void OwlDemo::fly_left_again()
{
	if (!flap(32, true))
		return;
	uint8_t x = --wario().x();
	if (x == 0x5c) {
		clear_dpad();
		return;
	}
	if (x >= 0x10)
		return;
	show_wario_hang_owl_right();
	next_state();
}

void OwlDemo::fly_right()
{
	if (!flap(32, true))
		return;
	if (++wario().x() < 0x60)
		return;
	wram_.sfx_timer() = 0;
	show_dleft();
	show_wario_owl_move_hang();
	next_state();
}

void OwlDemo::glide_left()
{
	flap(16, false);
	if (--wario().x() >= 0x20)
		return;
	show_dright();
	show_wario_owl_move_hang_right();
	next_state();
}

void OwlDemo::glide_right()
{
	flap(16, false);
	if (++wario().x() < 0x80)
		return;
	clear_dpad();
	show_wario_hang_drop();
	show_owl_fly_up();
	next_state();
}

// B: Wario lets go and drops; the owl flies off.
void OwlDemo::drop()
{
	enemy().y() -= 2;
	if (++wario().y() < 0x40)
		return;
	clear_buttons();
	show_wario_idle2();
	timer() = 0x30;
	next_state();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(OwlActionHelp);
PORT_COVERED(Func_1e407e);
PORT_COVERED(Func_1e409a);
PORT_COVERED(Func_1e4095);
PORT_COVERED(Func_1e4104);
PORT_COVERED(Func_1e4134);
PORT_COVERED(Func_1e4165);
PORT_COVERED(Func_1e4195);
PORT_COVERED(Func_1e41c9);
PORT_COVERED(Func_1e41fd);
PORT_COVERED(Func_1e4227);
PORT_COVERED(Func_1e4254);
PORT_COVERED(Func_1e4271);
PORT_COVERED(Func_1e4286);
