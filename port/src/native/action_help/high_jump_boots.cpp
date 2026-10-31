// The High Jump Boots demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void HighJumpBootsDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	MenuObject block = object(wMenuObj5);
	if (!wram_.action_help_frozen()) {
		animate_object(block, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_wario(0x05);
		wario().animation_ended() = wram_.obj_anim_was_reset();
	}
	load_object_sprite(block);
	load_object_sprite(enemy());
	load_wario_sprite();
}

uint8_t HighJumpBootsDemo::jump_by(uint16_t table, Step step)
{
	Byte var = wario_var();
	uint8_t index;
	if (step == Step::Current) {
		index = var;
		++var;
	} else {
		++var;
		if (step == Step::NextCapped && var >= 0x28)
			var = 0x27;
		index = var;
	}
	wario().y() += mem_[static_cast<uint16_t>(table + index)];
	return wario().y();
}

void HighJumpBootsDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2:
		if (--object(wMenuObj5).x() >= 0x58)
			break;
		call(ADDR_ActionHelp_ShowWarioIdle);
		next_state();
		break;
	case 3: call(ADDR_ActionHelp_FlashDUpInput); break;
	case 4:
		if (++timer() < 0x30)
			break;
		timer() = 0;
		start_jump();
		break;
	case 5: rise(); break;
	case 6: high_jump_up(); break;
	case 7:
		if (!timer_done())
			break;
		show_dright();
		call(ADDR_ActionHelp_ShowWarioJump);
		wario_var() = 0x18;
		next_state();
		break;
	case 8: jump_down(); break;
	case 9:
		if (--enemy().x() >= 0x68)
			break;
		start_jump();
		break;
	case 10: jump_on_paragoom(); break;
	case 11: bounce_back(); break;
	case 12: jump_on_block(); break;
	case 13: jump_smash(); break;
	case 14: smash_paragoom(); break;
	case 15: bounce_up(); break;
	case 16: bounce_on(); break;
	case 17: fall(); break;
	case 18:
		if (timer_done())
			start_jump();
		break;
	case 19: land_after_jump(); break;
	case 20:
		if (!wario().animation_ended())
			break;
		show_dright();
		call(ADDR_ActionHelp_ShowWarioJump);
		next_state();
		break;
	case 21:
		--object(wMenuObj5).x();
		jump_by(ADDR_JumpVelTable_HighJump, Step::Next);
		if (wario_var() < 0x18)
			break;
		clear_buttons();
		next_state();
		break;
	case 22: high_jump_up(); break;
	case 23:
		if (!timer_done())
			break;
		show_a();
		show_dright();
		call(ADDR_ActionHelp_ShowWarioJump);
		next_state();
		break;
	case 24: jump_off(); break;
	default: unrealistic(); break;
	}
}

// Wario walks in; a tall block comes in.
void HighJumpBootsDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	call(ADDR_ActionHelp_ShowParaGoomWalk);
	init_object(object(wMenuObj5), 0x40, 0xe8, ADDR_Frameset_1e6b99, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	load_pals_without_water();
	next_state();
}

void HighJumpBootsDemo::rise()
{
	--object(wMenuObj5).x();
	jump_by(ADDR_JumpVelTable_HighJump, Step::Next);
	if (wario_var() < 0x18)
		return;
	clear_buttons();
	wario_var() = 0x18;
	next_state();
}

void HighJumpBootsDemo::high_jump_up()
{
	object(wMenuObj5).x() -= 2;
	if (jump_by(ADDR_JumpVelTable_HighJump, Step::Current) < 0x10)
		return;
	land_on_top(false);
}

// on top of the block, at y $10
void HighJumpBootsDemo::land_on_top(bool dright)
{
	wario().y() = 0x10;
	if (dright)
		show_dright();
	else
		clear_dpad();
	call(ADDR_ActionHelp_ShowWarioIdle);
	wario_var() = 0;
	timer() = 0x30;
	next_state();
}

void HighJumpBootsDemo::jump_down()
{
	--object(wMenuObj5).x();
	if (jump_by(ADDR_JumpVelTable_HighJump, Step::NextCapped) < 0x40)
		return;
	wario().y() = 0x40;
	clear_dpad();
	call(ADDR_ActionHelp_ShowWarioIdle);
	wario_var() = 0;
	next_state();
}

void HighJumpBootsDemo::jump_on_paragoom()
{
	--enemy().x();
	jump_by(ADDR_JumpVelTable_Normal, Step::Next);
	if (wario_var() < 0x18 || wario().y() < 0x34)
		return;
	call(ADDR_ActionHelp_ShowDLeftInput);
	call(ADDR_ActionHelp_ShowParaGoomStun);
	call(ADDR_ActionHelp_ShowWarioJumpLeft);
	wario_var() = 0;
	next_state();
}

void HighJumpBootsDemo::bounce_back()
{
	Byte block_x = object(wMenuObj5).x();
	if (block_x < 0x3c) {
		++block_x;
		++enemy().x();
	}
	jump_by(ADDR_JumpVelTable_Normal, Step::Next);
	if (wario_var() < 0x18)
		return;
	clear_buttons();
	wario_var() = 0x18;
	next_state();
}

void HighJumpBootsDemo::jump_on_block()
{
	if (jump_by(ADDR_JumpVelTable_Normal, Step::Next) < 0x10)
		return;
	land_on_top(true);
}

void HighJumpBootsDemo::jump_smash()
{
	clear_dpad();
	if (!timer_done())
		return;
	show_a();
	call(ADDR_ActionHelp_ShowDRightAndDDownInput);
	call(ADDR_ActionHelp_ShowWarioJump);
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	wario_var() = 0x18;
	next_state();
}

void HighJumpBootsDemo::smash_paragoom()
{
	--enemy().x();
	--object(wMenuObj5).x();
	if (jump_by(ADDR_JumpVelTable_Normal, Step::NextCapped) < 0x34)
		return;
	wario().y() = 0x30;
	call(ADDR_ActionHelp_ShowDLeftInput);
	call(ADDR_ActionHelp_ShowObj4EnemyVanish);
	call(ADDR_ActionHelp_ShowWarioJumpLeft);
	wario_var() = 0;
	next_state();
}

void HighJumpBootsDemo::bounce_up()
{
	++enemy().x();
	++object(wMenuObj5).x();
	jump_by(ADDR_JumpVelTable_Normal, Step::Next);
	if (wario_var() < 0x18)
		return;
	clear_buttons();
	wario_var() = 0x18;
	next_state();
}

void HighJumpBootsDemo::bounce_on()
{
	enemy().x() += 2;
	object(wMenuObj5).x() += 2;
	if (jump_by(ADDR_JumpVelTable_Normal, Step::Next) < 0x10)
		return;
	wario().y() = 0x10;
	wario_var() = 0x0e;
	next_state();
}

void HighJumpBootsDemo::fall()
{
	if (wram_.menu_obj4_animation_ended()) {
		SET_HL(gb_, wMenuObj4);
		call(ADDR_ActionHelp_HideBlockDebris);
	}
	++object(wMenuObj5).x();
	if (jump_by(ADDR_JumpVelTable_Normal, Step::NextCapped) < 0x40)
		return;
	wario().y() = 0x40;
	clear_dpad();
	call(ADDR_ActionHelp_ShowWarioIdle);
	wario_var() = 0;
	timer() = 0x30;
	next_state();
}

void HighJumpBootsDemo::land_after_jump()
{
	jump_wario();
	if (wario_var() == 0x18) {
		clear_buttons();
		return;
	}
	if (wario_var() < 0x18 || wario().y() < 0x40)
		return;
	show_a();
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioLand);
	next_state();
}

void HighJumpBootsDemo::jump_off()
{
	object(wMenuObj5).x() -= 2;
	uint8_t y = jump_by(ADDR_JumpVelTable_Normal, Step::NextCapped);
	if (static_cast<uint8_t>(y + 0x20) < 0x60)
		return;
	wario().y() = 0x40;
	finish([this] { start(); });
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(HighJumpBootsActionHelp);
PORT_COVERED(Func_1fa912);
PORT_COVERED(Func_1fa94a);
PORT_COVERED(Func_1fa945);
PORT_COVERED(Func_1fa984);
PORT_COVERED(Func_1fa994);
PORT_COVERED(Func_1fa9a9);
PORT_COVERED(Func_1fa9d3);
PORT_COVERED(Func_1faa07);
PORT_COVERED(Func_1faa1c);
PORT_COVERED(Func_1faa51);
PORT_COVERED(Func_1faa64);
PORT_COVERED(Func_1faa99);
PORT_COVERED(Func_1faacc);
PORT_COVERED(Func_1faafb);
PORT_COVERED(Func_1fab19);
PORT_COVERED(Func_1fab55);
PORT_COVERED(Func_1fab83);
PORT_COVERED(Func_1fabb2);
PORT_COVERED(Func_1fabf8);
PORT_COVERED(Func_1fac08);
PORT_COVERED(Func_1fac3c);
PORT_COVERED(Func_1fac4c);
PORT_COVERED(Func_1fac71);
PORT_COVERED(Func_1faca5);
PORT_COVERED(Func_1facb8);
