// The Head Smash Helmet demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void HeadSmashHelmetDemo::run()
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
	}
	load_object_sprite(enemy());
	load_wario_sprite();
}

void HeadSmashHelmetDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: block_above(); break;
	case 3:
		if (!timer_done())
			break;
		call(ADDR_ActionHelp_ShowWarioJump);
		next_state();
		break;
	case 4: call(ADDR_ActionHelp_FlashAInput); break;
	case 5: head_smash(ADDR_Frameset_1e6b67); break;
	case 6: land(); break;
	case 7:
		hide_debris_when_done();
		wait_then_walk();
		break;
	case 8: block_above(); break;
	case 9:
		if (!timer_done())
			break;
		call(ADDR_ActionHelp_ShowWarioJump);
		show_a();
		next_state();
		break;
	case 10: head_smash(ADDR_Frameset_1e6b6a); break;
	case 11: land(); break;
	case 12:
		hide_debris_when_done();
		if (timer_done())
			finish([this] { start(); });
		break;
	default: unrealistic(); break;
	}
}

// Wario walks in; a block (wMenuObj4) comes in above.
void HeadSmashHelmetDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	init_object(enemy(), 0x20, 0xe8, ADDR_Frameset_1e6b64, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	hide_obj5_debris();
	load_pals_without_water();
	next_state();
}

// The block moves left until it's above Wario.
void HeadSmashHelmetDemo::block_above()
{
	if (--enemy().x() >= 0x40)
		return;
	call(ADDR_ActionHelp_ShowWarioIdle);
	clear_dpad();
	timer() = 0x10;
	next_state();
}

// Wario jumps into the block and breaks it; next_block takes its place.
void HeadSmashHelmetDemo::head_smash(uint16_t next_block)
{
	jump_wario();
	if (static_cast<uint8_t>(wario().y() - 0x18) >= enemy().y())
		return;
	wario_var() = 0x18;
	gb_->cpu.b = 0x00; // the debris's y offset
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	init_object(enemy(), 0x20, 0xe8, next_block, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	next_state();
}

void HeadSmashHelmetDemo::land()
{
	if (jump_above(0x40))
		return;
	wario().y() = 0x40;
	wario_var() = 0;
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioIdle);
	timer() = 0x20;
	next_state();
}

void HeadSmashHelmetDemo::hide_debris_when_done()
{
	if (wram_.menu_obj5_animation_ended())
		hide_obj5_debris();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(HeadSmashHelmetActionHelp);
PORT_COVERED(Func_1f927a);
PORT_COVERED(Func_1f92b2);
PORT_COVERED(Func_1f92ad);
PORT_COVERED(Func_1f92ee);
PORT_COVERED(Func_1f9306);
PORT_COVERED(Func_1f9313);
PORT_COVERED(Func_1f935b);
PORT_COVERED(Func_1f938a);
PORT_COVERED(Func_1f93a3);
PORT_COVERED(Func_1f93bb);
PORT_COVERED(Func_1f93cb);
PORT_COVERED(Func_1f9413);
PORT_COVERED(Func_1f9442);
