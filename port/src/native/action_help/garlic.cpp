// The Garlic demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void GarlicDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	MenuObject debris = object(wMenuObj5);
	animate_object(debris, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
	load_object_sprite(debris);
	MenuObject debris2 = object(wMenuObj7);
	animate_object(debris2, wMenuObj7FramesetBank, wMenuObj7AnimationEnded);
	load_object_sprite(debris2);
	MenuObject block2 = object(wMenuObj6);
	if (!wram_.action_help_frozen()) {
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_object(block2, wMenuObj6FramesetBank, 0);
		animate_wario(0x05);
		wario().animation_ended() = wram_.obj_anim_was_reset();
	}
	load_object_sprite(enemy());
	load_object_sprite(block2);
	load_wario_sprite();
}

void GarlicDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2:
		if (!move_blocks())
			break;
		call(ADDR_ActionHelp_ShowWarioAttack);
		wario_var() = 0x30;
		next_state();
		break;
	case 3: call(ADDR_ActionHelp_FlashBInput); break;
	case 4: break_first_block(); break;
	case 5: break_second_block(); break;
	case 6: charge_ends(); break;
	case 7:
		if (move_blocks())
			start_attack();
		break;
	case 8: break_blocks_again(); break;
	case 9: bounce_back(); break;
	case 10: wait_then_walk(); break;
	case 11:
		if (++wario().x() >= 0x40)
			start_attack();
		break;
	case 12: break_block_at_count(); break;
	case 13: bounce_back(); break;
	case 14: wait_then_walk(); break;
	case 15: {
		if (!walk_wario_to_40())
			break;
		Byte x = enemy().x();
		dec_twice_odd(x);
		if (x < 0x58)
			start_attack();
		break;
	}
	case 16: knock_out_count(); break;
	case 17: count_flies(); break;
	case 18: wait_then_walk(); break;
	case 19: {
		if (!walk_wario_to_40())
			break;
		Byte x = enemy().x();
		dec_twice_odd(x);
		if (x < 0x60)
			start_jump();
		break;
	}
	case 20: jump_onto_blocks(); break;
	case 21: wait_then_walk(); break;
	case 22: ride_slide(); break;
	case 23: call(ADDR_ActionHelp_FlashDDownInput); break;
	case 24: roll(); break;
	case 25:
		slide_down();
		if (enemy().x() != 0xd8)
			break;
		call(ADDR_ActionHelp_ShowCountRichtertoffIdle);
		next_state();
		break;
	case 26: bump_count(); break;
	case 27: count_falls(); break;
	case 28: blocks_come(); break;
	case 29: blocks_break(); break;
	case 30: {
		if (wram_.menu_obj5_animation_ended())
			hide_obj5_debris();
		Byte x = enemy().x();
		dec_twice_odd(x);
		if (x >= 0x50)
			break;
		call(ADDR_ActionHelp_ShowWarioBump);
		wario_var() = 0x0e;
		next_state();
		break;
	}
	case 31: bounce_off(); break;
	case 32: wait_then_walk(); break;
	case 33: approach_last_slide(); break;
	case 34: jump_over(); break;
	case 35:
		if (--enemy().x() >= 0xe8)
			finish([this] { start(); });
		break;
	default: unrealistic(); break;
	}
}

// Wario walks in; two cracked blocks (wMenuObj4, wMenuObj6) come in.
void GarlicDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	init_object(enemy(), 0x40, 0xe8, ADDR_Frameset_1e6b9c, ADDR_OAM_1e697c, BANK_OAM_1e697c);
	hide_obj5_debris();
	init_object(object(wMenuObj6), 0x40, 0xf8, ADDR_Frameset_1e6b9c, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	SET_HL(gb_, wMenuObj7);
	call(ADDR_ActionHelp_HideBlockDebris);
	load_pals_without_water();
	next_state();
}

void GarlicDemo::push_blocks(int times)
{
	for (int i = 0; i < times; i++) {
		--object(wMenuObj6).x();
		--enemy().x();
	}
}

bool GarlicDemo::move_blocks()
{
	push_blocks(1);
	return enemy().x() < 0x60;
}

bool GarlicDemo::attack_countdown()
{
	play_sfx_rept(6, SFX_ATTACK_POWERED_UP);
	return --wario_var() < 0x2b;
}

bool GarlicDemo::attack_blocks()
{
	if (!attack_countdown())
		return false;
	push_blocks((wram_.action_help_counter() & 0x01) ? 2 : 1);
	return enemy().x() < 0x50;
}

bool GarlicDemo::attack_enemy()
{
	if (!attack_countdown())
		return false;
	Byte x = enemy().x();
	dec_twice_odd(x);
	return x < 0x50;
}

// the front block's debris, and wMenuObj7's
void GarlicDemo::break_blocks()
{
	gb_->cpu.b = 0x00; // the debris's y offset
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	show_block_debris(object(wMenuObj7), 0x10, 0x00);
}

void GarlicDemo::hide_debris()
{
	hide_obj5_debris();
	SET_HL(gb_, wMenuObj7);
	call(ADDR_ActionHelp_HideBlockDebris);
}

void GarlicDemo::break_first_block()
{
	if (!attack_blocks())
		return;
	break_blocks();
	enemy().x() = object(wMenuObj6).x();
	init_object(object(wMenuObj6), 0x40, 0xf8, ADDR_Frameset_1e6b96, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	next_state();
}

void GarlicDemo::break_second_block()
{
	play_sfx_rept(6, SFX_ATTACK_POWERED_UP);
	--wario_var();
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x50)
		return;
	break_blocks();
	init_object(enemy(), 0x40, 0xe8, ADDR_Frameset_1e6b96, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	next_state();
}

void GarlicDemo::charge_ends()
{
	play_sfx_rept(6, SFX_ATTACK_POWERED_UP);
	if (wram_.menu_obj5_animation_ended())
		hide_debris();
	if (--wario_var())
		return;
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioWalk);
	show_dright();
	next_state();
}

void GarlicDemo::break_blocks_again()
{
	if (!attack_blocks())
		return;
	break_blocks();
	enemy().x() = object(wMenuObj6).x();
	init_object(object(wMenuObj6), 0xa0, 0xf8, ADDR_Frameset_1e6ba2, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	clear_dpad();
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x0e;
	next_state();
}

void GarlicDemo::bounce_back()
{
	dec_twice_odd(wario().x());
	if (jump_above(0x40))
		return;
	wario_var() = 0;
	wario().y() = 0x40;
	hide_debris();
	call(ADDR_ActionHelp_ShowWarioIdle);
	timer() = 0x30;
	next_state();
}

void GarlicDemo::break_block_at_count()
{
	if (!attack_enemy())
		return;
	break_blocks();
	call(ADDR_ActionHelp_ShowCountRichtertoffIdle);
	clear_dpad();
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x0e;
	next_state();
}

void GarlicDemo::knock_out_count()
{
	if (!attack_enemy())
		return;
	play_sfx(mem_, SFX_017);
	call(ADDR_ActionHelp_ShowCountRichtertoffStun);
	clear_dpad();
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x0e;
	next_state();
}

// Wario bounces back; the Count flies off the screen, and a slide comes.
void GarlicDemo::count_flies()
{
	uint8_t velocity = capped_velocity();
	MenuObject count = enemy();
	if (wario_var() < 0x18) {
		wario().y() += velocity;
		count.y() += velocity;
		count.x() += 2;
		dec_twice_odd(wario().x());
		return;
	}
	if (wario().y() == 0x40) {
		count.y() += velocity;
		count.x() += 2;
		if (count.y() < 0x80)
			return;
		call(ADDR_ActionHelp_ShowSlide);
		wario_var() = 0;
		timer() = 0x30;
		next_state();
		return;
	}
	wario().y() += velocity;
	count.y() += velocity;
	count.x() += 2;
	if (wario().y() < 0x40)
		return;
	wario().y() = 0x40;
	call(ADDR_ActionHelp_ShowWarioIdle);
}

void GarlicDemo::bump_count()
{
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x50)
		return;
	play_sfx(mem_, SFX_BUMP);
	call(ADDR_ActionHelp_ShowCountRichtertoffStun);
	wario_var() = 0x0e;
	next_state();
}

// The Count flies off; blocks come.
void GarlicDemo::count_falls()
{
	MenuObject count = enemy();
	count.y() += capped_velocity();
	count.x() += 2;
	if (count.y() < 0x80)
		return;
	init_object(count, 0x40, 0xd8, ADDR_Frameset_1e6b64, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
	object(wMenuObj6).y() = 0x40;
	wario_var() = 0;
	next_state();
}

void GarlicDemo::blocks_come()
{
	push_blocks((wram_.action_help_counter() & 0x01) ? 2 : 1);
	if ((enemy().x() & 0xfe) != 0x48)
		return;
	enemy().x() = 0x48;
	gb_->cpu.b = 0x00;
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	routines_.copy(wMenuObj6, wMenuObj4, 0x0b);
	SET_HL(gb_, wMenuObj6);
	call(ADDR_ActionHelp_HideBlockDebris);
	next_state();
}

void GarlicDemo::blocks_break()
{
	Byte x = enemy().x();
	dec_twice_odd(x);
	uint8_t at = x & 0xfe;
	if (at == 0x58) {
		x = 0x58;
		gb_->cpu.b = 0x00;
		call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
		call(ADDR_ActionHelp_ShowSingleSolidBlock);
		return;
	}
	if (at != 0x48)
		return;
	gb_->cpu.b = 0x00;
	call(ADDR_ActionHelp_ShowBlockDebrisWithYOffset);
	enemy().init(0x50, 0xe8, ADDR_Frameset_1e6b85);
	next_state();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(GarlicActionHelp);
PORT_COVERED(Func_1f9ce7);
PORT_COVERED(Func_1f9d1f);
PORT_COVERED(Func_1f9d1a);
PORT_COVERED(Func_1f9d7e);
PORT_COVERED(Func_1f9d97);
PORT_COVERED(Func_1f9e07);
PORT_COVERED(Func_1f9e62);
PORT_COVERED(Func_1f9e9b);
PORT_COVERED(Func_1f9eb7);
PORT_COVERED(Func_1f9f35);
PORT_COVERED(Func_1f9f76);
PORT_COVERED(Func_1f9f86);
PORT_COVERED(Func_1f9f9e);
PORT_COVERED(Func_1f9ff4);
PORT_COVERED(Func_1fa035);
PORT_COVERED(Func_1fa045);
PORT_COVERED(Func_1fa072);
PORT_COVERED(Func_1fa0c2);
PORT_COVERED(Func_1fa145);
PORT_COVERED(Func_1fa155);
PORT_COVERED(Func_1fa17d);
PORT_COVERED(Func_1fa1c5);
PORT_COVERED(Func_1fa1d5);
PORT_COVERED(Func_1fa1ee);
PORT_COVERED(Func_1fa20a);
PORT_COVERED(Func_1fa223);
PORT_COVERED(Func_1fa248);
PORT_COVERED(Func_1fa298);
PORT_COVERED(Func_1fa2d4);
PORT_COVERED(Func_1fa315);
PORT_COVERED(Func_1fa33b);
PORT_COVERED(Func_1fa37c);
PORT_COVERED(Func_1fa38c);
PORT_COVERED(Func_1fa3ab);
PORT_COVERED(Func_1fa3e4);
