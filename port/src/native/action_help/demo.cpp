// What the power-up demonstrations share; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

static_assert(wActionHelpWarioXCoord == wActionHelpWario + 1);

void Demo::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void Demo::dec_twice_odd(Byte value)
{
	--value;
	if (wram_.action_help_counter() & 0x01)
		--value;
}

uint8_t Demo::jump_velocity(Byte step)
{
	++step;
	return mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + step)];
}

uint8_t Demo::jump_wario()
{
	uint8_t velocity = jump_velocity(wario_var());
	wario().y() += velocity;
	return velocity;
}

void Demo::play_sfx_rept(uint8_t frames, uint16_t sfx)
{
	Byte timer = wram_.sfx_timer();
	bool due = timer == 0;
	--timer;
	if (!due)
		return;
	timer = frames;
	play_sfx(mem_, sfx);
}

bool Demo::walk_wario_to_40()
{
	Byte x = wario().x();
	if (x == 0x40)
		return true;
	return ++x >= 0x40;
}

void Demo::wait_then_walk()
{
	if (!timer_done())
		return;
	call(ADDR_ActionHelp_ShowWarioWalk);
	show_dright();
	next_state();
}

void Demo::start_jump()
{
	show_a();
	call(ADDR_ActionHelp_ShowWarioJump);
	next_state();
}

void Demo::start_attack()
{
	show_b();
	call(ADDR_ActionHelp_ShowWarioAttack);
	wario_var() = 0x30;
	next_state();
}

bool Demo::wait_then_every(uint8_t mask)
{
	if (timer()) {
		--timer();
		return false;
	}
	return !(wram_.action_help_counter() & mask);
}

void Demo::queue_wario_gfx(uint8_t bank, uint16_t gfx)
{
	wram_.pending_dma_source_bank() = bank;
	mem_[wPendingDMASourcePtr + 0] = static_cast<uint8_t>(gfx >> 8);
	mem_[wPendingDMASourcePtr + 1] = static_cast<uint8_t>(gfx);
	mem_[wPendingDMADestinationPtr + 0] = static_cast<uint8_t>(v0Tiles0 >> 8);
	mem_[wPendingDMADestinationPtr + 1] = static_cast<uint8_t>(v0Tiles0);
	wram_.pending_dma_destination_bank() = BANK_v0Tiles0;
	wram_.pending_dma_length() = 0x80 - 1;
	wram_.is_dma_transfer_pending() = TRUE;
}

void Demo::init_object(MenuObject obj, uint8_t y, uint8_t x, uint16_t frameset, uint16_t oam, uint8_t bank,
	bool reset_ended)
{
	obj.init(y, x, frameset);
	mem_[static_cast<uint16_t>(obj.addr() + 8)] = static_cast<uint8_t>(oam >> 8);
	mem_[static_cast<uint16_t>(obj.addr() + 9)] = static_cast<uint8_t>(oam);
	mem_[static_cast<uint16_t>(obj.addr() + 10)] = bank;
	if (reset_ended)
		mem_[static_cast<uint16_t>(obj.addr() + 11)] = 0;
}

bool Demo::jump_towards_enemy()
{
	jump_wario();
	dec_twice_odd(enemy().x());
	return wario().x() < enemy().x();
}

void Demo::jump_onto_blocks()
{
	if (jump_towards_enemy())
		return;
	wario().x() = enemy().x();
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

void Demo::high_jump()
{
	if (!jump_above(0x0c))
		return;
	wario_var() = 0x18;
	call(ADDR_ActionHelp_ShowWarioSmashAttack);
	call(ADDR_ActionHelp_ShowDDownInput);
	clear_buttons();
	next_state();
}

uint8_t Demo::capped_velocity()
{
	Byte step = wario_var();
	++step;
	if (step >= 0x28)
		step = 0x27;
	return mem_[static_cast<uint16_t>(ADDR_JumpVelTable_Normal + step)];
}

uint8_t Demo::jump_wario_capped()
{
	return wario().y() += capped_velocity();
}

bool Demo::ground_shake(uint16_t at_5)
{
	if (timer_done())
		return true;
	if (timer() == 0x05) {
		call(at_5);
		clear_dpad();
	}
	Byte scy = wram_.scy();
	switch (timer() & 0x03) {
	case 0x03: scy += 2; break;
	case 0x01: scy -= 2; break;
	}
	return false;
}

// Wario jumps over the slide as it moves left.
void Demo::jump_over()
{
	if (wario().x() != 0x40)
		++wario().x();
	jump_wario();
	if (wario().y() < 0x40) {
		--enemy().x();
		return;
	}
	wario_var() = 0;
	wario().y() = 0x40;
	call(ADDR_ActionHelp_ShowWarioWalk);
	clear_buttons();
	next_state();
}

void Demo::ride_slide()
{
	uint8_t x = --enemy().x();
	if (x >= 0x39)
		return;
	if (x >= 0x30) {
		++wario().y();
		return;
	}
	call(ADDR_ActionHelp_ShowWarioSlide);
	next_state();
}

// Wario slides down to the ground as the slide moves left.
void Demo::slide_down()
{
	if (wario().y() < 0x40)
		++wario().y();
	--enemy().x();
}

void Demo::roll()
{
	slide_down();
	if (enemy().x() >= 0x18)
		return;
	clear_dpad();
	call(ADDR_ActionHelp_ShowWarioRoll);
	next_state();
}

void Demo::bounce_off()
{
	jump_wario();
	dec_twice_odd(wario().x());
	if (wario_var() < 0x18 || wario().y() < 0x40)
		return;
	wario().y() = 0x40;
	call(ADDR_ActionHelp_ShowWarioIdle);
	wario_var() = 0;
	timer() = 0x30;
	next_state();
}

void Demo::approach_last_slide()
{
	Byte x = wario().x();
	if (x != 0x40)
		++x;
	uint8_t wario_x = x;
	uint8_t slide_x = --enemy().x();
	if (static_cast<uint8_t>(slide_x - wario_x) >= 0x1c)
		return;
	show_a();
	call(ADDR_ActionHelp_ShowWarioJump);
	next_state();
}

void Demo::animate_object(MenuObject obj, uint16_t bank_field, uint16_t ended_field)
{
	uint8_t bank = mem_[bank_field];
	wram_.temp_bank() = bank;
	obj.animate_far(bank);
	if (ended_field)
		mem_[ended_field] = wram_.obj_anim_was_reset();
}

void Demo::load_object_sprite(MenuObject obj)
{
	SET_HL(gb_, obj.addr());
	routines_.farcall(BANK_LoadActionHelpSprite_Far, ADDR_LoadActionHelpSprite_Far);
}

void Demo::animate_wario(uint8_t bank)
{
	wario().animate_far(bank);
}

void Demo::load_wario_sprite()
{
	SET_HL(gb_, wActionHelpWario);
	routines_.farcall(BANK_LoadActionHelpWarioSprite, ADDR_LoadActionHelpWarioSprite);
}

void Demo::show_block_debris(MenuObject obj, uint8_t y, uint8_t x)
{
	SET_HL(gb_, obj.addr());
	gb_->cpu.b = y;
	gb_->cpu.c = x;
	call(ADDR_ActionHelp_ShowBlockDebris);
}

void Demo::load_pals_without_water()
{
	routines_.farcall(BANK_LoadActionHelpPalsWithoutWater, ADDR_LoadActionHelpPalsWithoutWater);
}

} // namespace wl3::action_help
