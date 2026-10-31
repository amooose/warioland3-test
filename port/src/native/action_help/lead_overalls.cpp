// The Lead Overalls demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void LeadOverallsDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen()) {
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_wario(0x05);
	}
	load_object_sprite(enemy());
	load_wario_sprite();
	MenuObject debris = object(wMenuObj5);
	animate_object(debris, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
	load_object_sprite(debris);
}

void LeadOverallsDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: approach(); break;
	case 3: jump_at_enemy(); break;
	case 4: call(ADDR_ActionHelp_FlashDDownInput); break;
	case 5: pound_enemy(); break;
	case 6: land(); break;
	case 7: blocks_come(); break;
	case 8: wait_then_walk(); break;
	case 9: approach(); break;
	case 10: jump_onto_blocks(); break;
	case 11:
		if (timer_done())
			start_jump();
		break;
	case 12: high_jump(); break;
	case 13: pound_top_block(); break;
	case 14: pound_again(); break;
	case 15: pound_lower_block(); break;
	case 16: stand(); break;
	case 17: jump_off(); break;
	case 18: fall_off(); break;
	case 19: futamogu_comes(); break;
	case 20: jump_at_futamogu(); break;
	case 21: smash_down(); break;
	case 22: pound_futamogu(0x40); break;
	case 23: smash_again(); break;
	case 24: pound_futamogu(0x44); break;
	case 25: bounce(); break;
	case 26: land_on_futamogu(); break;
	case 27: futamogu_climbs(); break;
	case 28: climb_out(); break;
	case 29: walk_on(); break;
	case 30: futamogu_leaves(); break;
	default: unrealistic(); break;
	}
}

// Wario walks in towards a Para-Goom.
void LeadOverallsDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	call(ADDR_ActionHelp_ShowParaGoomWalk);
	hide_obj5_debris();
	load_pals_without_water();
	next_state();
}

void LeadOverallsDemo::approach()
{
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x60)
		return;
	start_jump();
}

void LeadOverallsDemo::jump_at_enemy()
{
	if (jump_towards_enemy())
		return;
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	clear_buttons();
	next_state();
}

void LeadOverallsDemo::pound_enemy()
{
	if (jump_above(0x34))
		return;
	wario().y() = 0x34;
	wario_var() = 0x0a;
	call(ADDR_ActionHelp_ShowObj4EnemyVanish);
	next_state();
}

void LeadOverallsDemo::land()
{
	if (jump_above(0x40))
		return;
	wario().y() = 0x40;
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioGroundPound);
	next_state();
}

void LeadOverallsDemo::blocks_come()
{
	if (!wram_.menu_obj4_animation_ended())
		return;
	call(ADDR_ActionHelp_ShowVerticalBlockPair);
	call(ADDR_ActionHelp_ShowWarioIdle);
	clear_dpad();
	timer() = 0x30;
	next_state();
}

void LeadOverallsDemo::pound_top_block()
{
	if (jump_above(0x20))
		return;
	wario().y() = 0x20;
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioGroundPound);
	call(ADDR_ActionHelp_ShowSingleSolidBlock);
	gb_->cpu.b = 0x10; // the debris's y offset
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	timer() = 0x10;
	next_state();
}

void LeadOverallsDemo::pound_again()
{
	if (!timer_done())
		return;
	wario_var() = 0x18;
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	hide_obj5_debris();
	next_state();
}

void LeadOverallsDemo::pound_lower_block()
{
	if (jump_above(0x30))
		return;
	wario().y() = 0x30;
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioGroundPound);
	timer() = 0x10;
	next_state();
}

void LeadOverallsDemo::stand()
{
	if (!timer_done())
		return;
	call(ADDR_ActionHelp_ShowWarioIdle);
	clear_dpad();
	timer() = 0x30;
	next_state();
}

void LeadOverallsDemo::jump_off()
{
	if (!timer_done())
		return;
	show_dright();
	start_jump();
}

// Wario falls off the block; a Futamogu and the debris come in.
void LeadOverallsDemo::fall_off()
{
	Byte step = wario_var();
	if (step != 0x27)
		++step;
	wario().y() += mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + step)];
	dec_twice_odd(enemy().x());
	if (wario().y() < 0x40)
		return;
	wario().y() = 0x40;
	wario_var() = 0;
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioWalk);
	show_dright();
	init_object(enemy(), 0x50, 0xc0, ADDR_Frameset_1e6f66, ADDR_OAM_1e6bab, BANK_OAM_1e6bab);
	init_object(object(wMenuObj5), 0x40, 0xb8, ADDR_Frameset_1e6b8b, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	next_state();
}

bool LeadOverallsDemo::move_futamogu(uint8_t x)
{
	dec_twice_odd(enemy().x());
	Byte debris_x = object(wMenuObj5).x();
	debris_x = static_cast<uint8_t>(enemy().x() - 8);
	return debris_x < x;
}

void LeadOverallsDemo::futamogu_comes()
{
	if (!move_futamogu(0x40))
		return;
	call(ADDR_ActionHelp_ShowWarioIdle);
	call(ADDR_ActionHelp_ShowFutamoguAngry);
	clear_dpad();
	timer() = 0x30;
	next_state();
}

void LeadOverallsDemo::jump_at_futamogu()
{
	if (!timer_done())
		return;
	show_a();
	call(ADDR_ActionHelp_ShowWarioJump);
	call(ADDR_ActionHelp_ShowFutamoguHappy);
	next_state();
}

void LeadOverallsDemo::smash_down()
{
	jump_wario();
	if (wario_var() < 0x18)
		return;
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	call(ADDR_ActionHelp_ShowDDownInput);
	clear_buttons();
	next_state();
}

// Each ground pound drives the Futamogu further into the ground.
void LeadOverallsDemo::pound_futamogu(uint8_t y)
{
	if (jump_above(y))
		return;
	wario().y() = y;
	call(ADDR_ActionHelp_ShowWarioGroundPound);
	enemy().y() += 4;
	if (y == 0x40)
		enemy().set_frameset(ADDR_Frameset_1e6f7d);
	timer() = 0x10;
	next_state();
}

void LeadOverallsDemo::smash_again()
{
	if (!timer_done())
		return;
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	next_state();
}

void LeadOverallsDemo::bounce()
{
	if (!timer_done())
		return;
	wario_var() = 0x18;
	call(ADDR_ActionHelp_ShowWarioIdle);
	clear_dpad();
	next_state();
}

void LeadOverallsDemo::land_on_futamogu()
{
	if (jump_above(0x48))
		return;
	wario().y() = 0x48;
	wario_var() = 0;
	timer() = 0x20;
	next_state();
}

void LeadOverallsDemo::futamogu_climbs()
{
	if (!timer_done())
		return;
	enemy().set_frameset(ADDR_Frameset_1e6f6b);
	next_state();
}

// The Futamogu climbs out, lifting Wario with it.
void LeadOverallsDemo::climb_out()
{
	--enemy().y();
	if (--wario().y() != 0x40)
		return;
	call(ADDR_ActionHelp_ShowWarioIdle);
	call(ADDR_ActionHelp_ShowFutamoguAngry);
	timer() = 0x30;
	next_state();
}

void LeadOverallsDemo::walk_on()
{
	if (!timer_done())
		return;
	show_dright();
	call(ADDR_ActionHelp_ShowWarioWalk);
	call(ADDR_ActionHelp_ShowFutamoguHappy);
	next_state();
}

void LeadOverallsDemo::futamogu_leaves()
{
	if (move_futamogu(0xf0))
		return;
	finish([this] { start(); });
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(LeadOverallsActionHelp);
PORT_COVERED(Func_1f8988);
PORT_COVERED(Func_1f89c0);
PORT_COVERED(Func_1f89bb);
PORT_COVERED(Func_1f89e0);
PORT_COVERED(Func_1f89fb);
PORT_COVERED(Func_1f8a2a);
PORT_COVERED(Func_1f8a52);
PORT_COVERED(Func_1f8a79);
PORT_COVERED(Func_1f8a91);
PORT_COVERED(Func_1f8aa1);
PORT_COVERED(Func_1f8abc);
PORT_COVERED(Func_1f8b04);
PORT_COVERED(Func_1f8b14);
PORT_COVERED(Func_1f8b3d);
PORT_COVERED(Func_1f8b71);
PORT_COVERED(Func_1f8b86);
PORT_COVERED(Func_1f8bb2);
PORT_COVERED(Func_1f8bc7);
PORT_COVERED(Func_1f8bda);
PORT_COVERED(Func_1f8c59);
PORT_COVERED(Func_1f8c81);
PORT_COVERED(Func_1f8c94);
PORT_COVERED(Func_1f8cbb);
PORT_COVERED(Func_1f8cf7);
PORT_COVERED(Func_1f8d04);
PORT_COVERED(Func_1f8d34);
PORT_COVERED(Func_1f8d49);
PORT_COVERED(Func_1f8d72);
PORT_COVERED(Func_1f8d88);
PORT_COVERED(Func_1f8da4);
PORT_COVERED(Func_1f8db7);
