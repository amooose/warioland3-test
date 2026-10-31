// The demonstration without a power-up; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void NoPowerUpDemo::run()
{
	run_state();
	if (wram_.action_help_blacked_out())
		return;
	if (!wram_.action_help_frozen()) {
		MenuObject debris = object(wMenuObj5);
		animate_object(debris, wMenuObj5FramesetBank, wMenuObj5AnimationEnded);
		load_object_sprite(debris);
		animate_object(enemy(), wMenuObj4FramesetBank, wMenuObj4AnimationEnded);
		animate_wario(0x05);
	}
	load_object_sprite(enemy());
	load_wario_sprite();
}

void NoPowerUpDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (!--timer())
			start();
		break;
	case 2: approach(); break;
	case 3: stomp(); break;
	case 4: land_on_paragoom(); break;
	case 5: wait_then_attack(); break;
	case 6: call(ADDR_ActionHelp_FlashBInput); break;
	case 7: attack_until_gone(); break;
	case 8: paragoom_falls(); break;
	case 9: approach_block(); break;
	case 10: break_block(); break;
	case 11: bounce_back(); break;
	case 12: wait_then_walk(); break;
	case 13: walk_to_block(); break;
	case 14: bump_solid_block(); break;
	case 15: bounce_back(); break;
	case 16: wait_then_walk(); break;
	case 17: approach_slide(); break;
	case 18: jump_over(); break;
	case 19: slide_comes(); break;
	case 20: approach_slide_again(); break;
	case 21: jump_onto_slide(); break;
	case 22: wait_then_walk(); break;
	case 23: ride_slide(); break;
	case 24: call(ADDR_ActionHelp_FlashDDownInput); break;
	case 25: roll(); break;
	case 26: roll_to_block(); break;
	case 27: bump_while_rolling(); break;
	case 28: bounce_off(); break;
	case 29: wait_then_walk(); break;
	case 30: approach_last_slide(); break;
	case 31: jump_over(); break;
	case 32: slide_leaves(); break;
	default: unrealistic(); break;
	}
}

// Wario walks in towards a Para-Goom.
void NoPowerUpDemo::start()
{
	call(ADDR_ActionHelp_ShowInitialWarioWalk);
	show_dright();
	call(ADDR_ActionHelp_ShowParaGoomWalk);
	hide_obj5_debris();
	load_pals_without_water();
	next_state();
}

void NoPowerUpDemo::approach()
{
	dec_twice_odd(enemy().x());
	if (enemy().x() >= 0x60)
		return;
	show_a();
	call(ADDR_ActionHelp_ShowWarioJump);
	next_state();
}

// The Para-Goom walks under Wario as he jumps, and he lands on it.
void NoPowerUpDemo::stomp()
{
	jump_wario();
	if (wario_var() >= 0x18 && wario().y() >= 0x34) {
		play_sfx(mem_, SFX_014);
		wario_var() = 0x0a;
		call(ADDR_ActionHelp_ShowParaGoomStun);
		next_state();
		return;
	}
	MenuObject paragoom = enemy();
	if (wario().x() == paragoom.x())
		return;
	dec_twice_odd(paragoom.x());
	if (wario().x() < paragoom.x())
		return;
	paragoom.x() = wario().x();
	clear_buttons();
	clear_dpad();
}

// Wario bounces off; the stunned Para-Goom flies up with him, then both
// land.
void NoPowerUpDemo::land_on_paragoom()
{
	MenuObject paragoom = enemy();
	paragoom.y() += jump_wario();
	if (paragoom.y() < 0x40) {
		++paragoom.x();
		if (wram_.action_help_counter() & 0x01)
			++paragoom.x();
		return;
	}
	paragoom.y() = 0x40;
	if (wario().y() < 0x40)
		return;
	wario().y() = 0x40;
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioIdle);
	timer() = 0x30;
	next_state();
}

void NoPowerUpDemo::wait_then_attack()
{
	if (--timer())
		return;
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x30;
	next_state();
}

// Wario's shoulder charge pushes the Para-Goom left; true once it's past
// x_limit.
bool NoPowerUpDemo::attack_paragoom(uint8_t x_limit)
{
	play_sfx_rept(6, SFX_ATTACK);
	if (--wario_var() >= 0x2b)
		return false;
	dec_twice_odd(enemy().x());
	return enemy().x() < x_limit;
}

void NoPowerUpDemo::attack_until_gone()
{
	if (!attack_paragoom(0x4c))
		return;
	play_sfx(mem_, SFX_017);
	clear_buttons();
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioIdle);
	timer() = 0x30;
	next_state();
}

// The Para-Goom is knocked off screen; a cracked and a solid block come in.
void NoPowerUpDemo::paragoom_falls()
{
	MenuObject obj = enemy();
	obj.x() += 2;
	Byte step = wario_var();
	++step;
	if (step >= 0x28)
		step = 0x27;
	obj.y() += mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + step)];
	if (obj.y() < 0x80)
		return;

	obj.init(0x40, 0xe8, ADDR_Frameset_1e6ba5);
	mem_[static_cast<uint16_t>(obj.addr() + 8)] = static_cast<uint8_t>(ADDR_OAM_1e697c >> 8);
	mem_[static_cast<uint16_t>(obj.addr() + 9)] = static_cast<uint8_t>(ADDR_OAM_1e697c);
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioWalk);
	show_dright();
	next_state();
}

void NoPowerUpDemo::approach_block()
{
	if (--enemy().x() >= 0x78)
		return;
	show_b();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x30;
	next_state();
}

void NoPowerUpDemo::break_block()
{
	if (!attack_paragoom(0x60))
		return;
	show_block_debris(object(wMenuObj5), 0x00, 0x10);
	call(ADDR_ActionHelp_ShowSingleSolidBlock);
	clear_dpad();
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x0e;
	next_state();
}

// Wario bounces back from the block and lands.
void NoPowerUpDemo::bounce_back()
{
	dec_twice_odd(wario().x());
	jump_wario();
	if (wario().y() < 0x40)
		return;
	wario_var() = 0;
	wario().y() = 0x40;
	hide_obj5_debris();
	call(ADDR_ActionHelp_ShowWarioIdle);
	timer() = 0x30;
	next_state();
}

void NoPowerUpDemo::walk_to_block()
{
	if (++wario().x() < 0x40)
		return;
	show_b();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x30;
	next_state();
}

void NoPowerUpDemo::bump_solid_block()
{
	if (!attack_paragoom(0x50))
		return;
	clear_dpad();
	clear_buttons();
	call(ADDR_ActionHelp_ShowWarioAttack);
	play_sfx(mem_, SFX_BUMP);
	wario_var() = 0x0e;
	next_state();
}

void NoPowerUpDemo::approach_slide()
{
	if (!walk_wario_to_40())
		return;
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x58)
		return;
	--x;
	if (static_cast<uint8_t>(x - 0x40) >= 0x1c)
		return;
	show_a();
	call(ADDR_ActionHelp_ShowWarioJump);
	next_state();
}

void NoPowerUpDemo::slide_comes()
{
	if (--enemy().x() < 0xe8)
		return;
	call(ADDR_ActionHelp_ShowSlide);
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioWalk);
	show_dright();
	next_state();
}

void NoPowerUpDemo::approach_slide_again()
{
	if (!walk_wario_to_40())
		return;
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x60)
		return;
	show_a();
	call(ADDR_ActionHelp_ShowWarioJump);
	next_state();
}

// Wario jumps onto the slide and stays on top as it moves.
void NoPowerUpDemo::jump_onto_slide()
{
	jump_wario();
	MenuObject slide = enemy();
	dec_twice_odd(slide.x());
	if (wario().x() < slide.x())
		return;
	wario().x() = slide.x();
	clear_dpad();
	if (wario().y() < 0x20)
		return;
	wario().y() = 0x20;
	wario_var() = 0;
	call(ADDR_ActionHelp_ShowWarioIdle);
	clear_buttons();
	timer() = 0x30;
	next_state();
}

void NoPowerUpDemo::roll_to_block()
{
	slide_down();
	if (enemy().x() != 0xd8)
		return;
	call(ADDR_ActionHelp_ShowSingleSolidBlock);
	next_state();
}

void NoPowerUpDemo::bump_while_rolling()
{
	Byte x = enemy().x();
	dec_twice_odd(x);
	if (x >= 0x50)
		return;
	call(ADDR_ActionHelp_ShowWarioBump);
	play_sfx(mem_, SFX_BUMP);
	wario_var() = 0x0e;
	next_state();
}

void NoPowerUpDemo::slide_leaves()
{
	if (--enemy().x() < 0xe8)
		return;
	finish([this] { start(); });
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(NoPowerUpActionHelp);
PORT_COVERED(Func_1f8362);
PORT_COVERED(Func_1f839a);
PORT_COVERED(Func_1f8395);
PORT_COVERED(Func_1f83ba);
PORT_COVERED(Func_1f83d5);
PORT_COVERED(Func_1f842e);
PORT_COVERED(Func_1f847a);
PORT_COVERED(Func_1f848c);
PORT_COVERED(Func_1f84d7);
PORT_COVERED(Func_1f8522);
PORT_COVERED(Func_1f853a);
PORT_COVERED(Func_1f8588);
PORT_COVERED(Func_1f85c3);
PORT_COVERED(Func_1f85d3);
PORT_COVERED(Func_1f85eb);
PORT_COVERED(Func_1f8638);
PORT_COVERED(Func_1f8673);
PORT_COVERED(Func_1f8683);
PORT_COVERED(Func_1f86b5);
PORT_COVERED(Func_1f86ee);
PORT_COVERED(Func_1f8708);
PORT_COVERED(Func_1f8730);
PORT_COVERED(Func_1f8778);
PORT_COVERED(Func_1f8788);
PORT_COVERED(Func_1f87a1);
PORT_COVERED(Func_1f87bd);
PORT_COVERED(Func_1f87d6);
PORT_COVERED(Func_1f87fb);
PORT_COVERED(Func_1f883c);
PORT_COVERED(Func_1f884c);
PORT_COVERED(Func_1f886b);
PORT_COVERED(Func_1f88a4);
