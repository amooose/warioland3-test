// The Rail demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void RailDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen()) {
		animate_object(enemy(), wMenuObj4FramesetBank, 0);
		animate_object(wario(), wActionHelpWarioOAMBank, wActionHelpWarioAnimationEnded);
	}
	load_wario_sprite_from_bank();
	load_object_sprite_near(enemy());
}

void RailDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp2_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: ride_left(); break;
	case 3:
		if (!ride_right())
			break;
		wario_var() = 0x18;
		next_state();
		break;
	case 4: call(ADDR_ActionHelp2_FlashBInput); break;
	case 5:
		++enemy().x();
		if (!jump_above(0x40))
			land();
		break;
	case 6: rail_leaves(); break;
	case 7: rail_back(); break;
	case 8: grab_rail(true); break;
	case 9:
		if (!wario().animation_ended())
			break;
		call(ADDR_ActionHelp_ShowWarioHangRailRight);
		next_state();
		break;
	case 10: ride_left(); break;
	case 11:
		if (ride_right())
			next_state();
		break;
	case 12: call(ADDR_ActionHelp2_FlashAInput); break;
	case 13: fall(); break;
	case 14: rail_leaves(); break;
	case 15: rail_back(); break;
	case 16: grab_rail(false); break;
	case 17:
		if (wario().animation_ended())
			finish_special([this] { start(); });
		break;
	default: unrealistic(); break;
	}
}

// Wario hangs from the rail (wMenuObj4).
void RailDemo::start()
{
	queue_wario_gfx(BANK_WarioHangGfx, ADDR_WarioHangGfx);
	MenuObject w = wario();
	w.init(0x38, 0x50, ADDR_Frameset_1feeb4);
	w.var() = 0;
	wram_.sfx_timer() = 0;
	mem_[wActionHelpWarioOAMPtr + 0] = static_cast<uint8_t>(ADDR_OAM_1fee05 >> 8);
	mem_[wActionHelpWarioOAMPtr + 1] = static_cast<uint8_t>(ADDR_OAM_1fee05);
	wram_.action_help_wario_oam_bank() = BANK_OAM_1fee05;
	init_object(enemy(), 0x20, 0x4c, ADDR_Frameset_1e6fc2, ADDR_OAM_1e6f86, BANK_OAM_1e6f86);
	call(ADDR_LoadActionHelpPalsWithoutWater);
	next_state();
}

void RailDemo::ride_left()
{
	play_sfx_rept(30, SFX_08C);
	--wario().x();
	if (--enemy().x() >= 0x1c)
		return;
	call(ADDR_ActionHelp_ShowRailRotateRight);
	next_state();
}

bool RailDemo::ride_right()
{
	play_sfx_rept(30, SFX_08C);
	++wario().x();
	if (++enemy().x() < 0x4c)
		return false;
	mem_[hSFXID + 0] = 0xff; // stop_sfx
	mem_[hSFXID + 1] = 0x00;
	call(ADDR_ActionHelp2_ShowWarioJump);
	return true;
}

void RailDemo::land()
{
	wario().y() = 0x40;
	wario_var() = 0;
	clear_buttons2();
	call(ADDR_ActionHelp2_ShowWarioIdle);
	next_state();
}

void RailDemo::rail_leaves()
{
	if (++enemy().x() < 0x7c)
		return;
	call(ADDR_ActionHelp_ShowRailRotateLeft);
	next_state();
}

void RailDemo::rail_back()
{
	if (--enemy().x() >= 0x4c)
		return;
	call(ADDR_ActionHelp2_ShowAInput);
	call(ADDR_ActionHelp2_ShowWarioJump);
	next_state();
}

// Wario jumps up and grabs the rail.
void RailDemo::grab_rail(bool reset_sfx_timer)
{
	if (!jump_above(0x38))
		return;
	wario().y() = 0x38;
	wario_var() = 0;
	if (reset_sfx_timer)
		wram_.sfx_timer() = 0;
	clear_buttons2();
	call(ADDR_ActionHelp_ShowWarioHangStopped);
	next_state();
}

// A: Wario lets go and falls to the ground.
void RailDemo::fall()
{
	++enemy().x();
	Byte step = wario_var();
	if (step != 0x27)
		++step;
	wario().y() += mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + step)];
	if (step < 0x18 || wario().y() < 0x40)
		return;
	land();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(RailActionHelp);
PORT_COVERED(Func_1e430d);
PORT_COVERED(Func_1e4329);
PORT_COVERED(Func_1e4324);
PORT_COVERED(Func_1e43a0);
PORT_COVERED(Func_1e43cb);
PORT_COVERED(Func_1e4403);
PORT_COVERED(Func_1e4432);
PORT_COVERED(Func_1e4442);
PORT_COVERED(Func_1e4455);
PORT_COVERED(Func_1e4483);
PORT_COVERED(Func_1e4490);
PORT_COVERED(Func_1e44bb);
PORT_COVERED(Func_1e44ee);
PORT_COVERED(Func_1e452a);
PORT_COVERED(Func_1e453a);
PORT_COVERED(Func_1e454d);
PORT_COVERED(Func_1e4578);
