// The Super Jump Slam Overalls demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void SuperJumpSlamOverallsDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	MenuObject block = object(wMenuObj5);
	if (!wram_.action_help_frozen()) {
		animate_object(block, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_wario(0x05);
	}
	load_object_sprite(block);
	load_object_sprite(enemy());
	load_wario_sprite();
}

void SuperJumpSlamOverallsDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: approach(); break;
	case 3: jump_up(); break;
	case 4: call(ADDR_ActionHelp_FlashDDownInput); break;
	case 5: slam(); break;
	case 6: paragoom_bounces(); break;
	case 7: slam_paragoom(); break;
	case 8: slam_again(); break;
	case 9: land(0); break;
	case 10:
		if (!wram_.menu_obj4_animation_ended())
			break;
		call(ADDR_ActionHelp_ShowVerticalBlockPair);
		call(ADDR_ActionHelp_ShowWarioWalk);
		show_dright();
		next_state();
		break;
	case 11: {
		Byte x = enemy().x();
		dec_twice_odd(x);
		if (x < 0x60)
			start_jump();
		break;
	}
	case 12: jump_onto_blocks(); break;
	case 13:
		if (timer_done())
			start_jump();
		break;
	case 14: high_jump(); break;
	case 15: slam_blocks(); break;
	case 16: break_lower_block(); break;
	case 17: shake_then_jump(); break;
	case 18: land(0x30); break;
	case 19:
		if (timer_done())
			finish([this] { start(); });
		break;
	default: unrealistic(); break;
	}
}

// Wario walks in; a Para-Goom and a Futamogu (wMenuObj5) come in.
void SuperJumpSlamOverallsDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	call(ADDR_ActionHelp_ShowParaGoomWalk);
	init_object(object(wMenuObj5), 0x40, 0xe8, ADDR_Frameset_1e6f4d, ADDR_OAM_1e6bab, BANK_OAM_1e6bab);
	load_pals_without_water();
	next_state();
}

void SuperJumpSlamOverallsDemo::approach()
{
	if (--object(wMenuObj5).x() >= 0x90)
		return;
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0xa0)
		return;
	show_a();
	clear_dpad();
	call(ADDR_ActionHelp_ShowWarioJump);
	next_state();
}

void SuperJumpSlamOverallsDemo::jump_up()
{
	--enemy().x();
	jump_wario();
	if (wario_var() < 0x18)
		return;
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	clear_buttons();
	next_state();
}

// The slam: the ground shakes and the Para-Goom is stunned.
void SuperJumpSlamOverallsDemo::slam()
{
	--enemy().x();
	if (jump_above(0x40))
		return;
	wario().y() = 0x40;
	call(ADDR_ActionHelp_ShowWarioGroundShake);
	call(ADDR_ActionHelp_ShowParaGoomStun);
	wario_var() = 0x0a;
	timer() = 0x20;
	next_state();
}

// The stunned Para-Goom bounces while the ground shakes.
void SuperJumpSlamOverallsDemo::paragoom_bounces()
{
	MenuObject paragoom = enemy();
	paragoom.y() += jump_velocity(wario_var());
	if (paragoom.y() >= 0x40) {
		wario_var() = 0x26;
		paragoom.y() = 0x40;
	}
	if (!ground_shake(ADDR_ActionHelp_ShowWarioIdle))
		return;
	call(ADDR_ActionHelp_ShowWarioJump);
	show_a();
	wario_var() = 0;
	next_state();
}

void SuperJumpSlamOverallsDemo::slam_paragoom()
{
	MenuObject futamogu = object(wMenuObj5);
	if (futamogu.x() != 0x40) {
		--enemy().x();
		if (--futamogu.x() == 0x40) {
			clear_buttons();
			call(ADDR_ActionHelp_ShowDDownInput);
			call(ADDR_ActionHelp_ShowWarioSmashAttack);
			wario_var() = 0x18;
		}
	}
	jump_wario();
	if (wario_var() < 0x18 || wario().y() < 0x30)
		return;
	wario_var() = 0x0a;
	SET_HL(gb_, futamogu.frameset_offset().addr());
	call(ADDR_ActionHelp_ShowEnemyVanish);
	next_state();
}

void SuperJumpSlamOverallsDemo::slam_again()
{
	Byte x = enemy().x();
	if (x != 0x40 && --x == 0x40)
		wario_var() = 0x18;
	jump_wario();
	if (wario_var() < 0x18 || wario().y() < 0x34)
		return;
	wario_var() = 0x0a;
	clear_dpad();
	hide_obj5_debris();
	call(ADDR_ActionHelp_ShowObj4EnemyVanish);
	next_state();
}

// lands; wait: the timer for the next state (0: none)
void SuperJumpSlamOverallsDemo::land(uint8_t wait)
{
	if (jump_above(0x40))
		return;
	wario().y() = 0x40;
	if (wait) {
		call(ADDR_ActionHelp_ShowWarioIdle);
		wario_var() = 0;
		timer() = wait;
	} else {
		wario_var() = 0;
		call(ADDR_ActionHelp_ShowWarioIdle);
	}
	next_state();
}

void SuperJumpSlamOverallsDemo::slam_blocks()
{
	if (jump_above(0x20))
		return;
	wario().y() = 0x20;
	call(ADDR_ActionHelp_ShowSingleSolidBlock);
	gb_->cpu.b = 0x10; // the debris's y offset
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	next_state();
}

void SuperJumpSlamOverallsDemo::break_lower_block()
{
	if (jump_wario_capped() < 0x30)
		return;
	wario().y() = 0x30;
	gb_->cpu.b = 0x00;
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	SET_HL(gb_, wMenuObj4);
	call(ADDR_ActionHelp_HideBlockDebris);
	call(ADDR_ActionHelp_ShowWarioGroundShake);
	timer() = 0x20;
	next_state();
}

void SuperJumpSlamOverallsDemo::shake_then_jump()
{
	if (wram_.menu_obj5_animation_ended())
		hide_obj5_debris();
	if (!ground_shake(ADDR_ActionHelp_ShowWarioJump))
		return;
	wario_var() = 0x18;
	next_state();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(SuperJumpSlamOverallsActionHelp);
PORT_COVERED(Func_1fa4b2);
PORT_COVERED(Func_1fa4ea);
PORT_COVERED(Func_1fa4e5);
PORT_COVERED(Func_1fa526);
PORT_COVERED(Func_1fa54c);
PORT_COVERED(Func_1fa574);
PORT_COVERED(Func_1fa5a8);
PORT_COVERED(Func_1fa602);
PORT_COVERED(Func_1fa653);
PORT_COVERED(Func_1fa698);
PORT_COVERED(Func_1fa6bf);
PORT_COVERED(Func_1fa6d2);
PORT_COVERED(Func_1fa6ed);
PORT_COVERED(Func_1fa735);
PORT_COVERED(Func_1fa745);
PORT_COVERED(Func_1fa76e);
PORT_COVERED(Func_1fa796);
PORT_COVERED(Func_1fa7d0);
PORT_COVERED(Func_1fa80b);
PORT_COVERED(Func_1fa837);
