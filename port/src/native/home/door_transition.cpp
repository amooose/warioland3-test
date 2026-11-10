// Door transitions (src/home/door_transition.asm): the tiles around Wario
// covered step by step, up and left in turn, from the V-Blank function, as
// the level's sub-state counts the steps.
//
// Plain C++ over memory. DoorTransition is called by LevelStateTable,
// which sets a and f after, and DoTransitionEffect from wVBlankFunc's jp,
// inside the V-Blank handler's pushes: the hooks clobber all registers.
#include "../native.hpp"
#include "memory.hpp"

namespace wl3::home {

namespace {

constexpr uint8_t GFX_RAM = 0x03; // the WRAM bank both use

class DoorTransition {
public:
	explicit DoorTransition(GB *gb) : gb_(gb), mem_(gb) {}

	void set_vblank(); // VBlank_DoorTransition
	void update(); // DoorTransition
	void effect(); // VBlank_DoorTransition.DoTransitionEffect

private:
	void copy3(uint16_t src, uint16_t dest)
	{
		gb_->cpu.ime = false; // di
		copy(gb_, src, dest, 3);
		gb_->cpu.ime = true; // ei
		gb_->cpu.ime_delay = 0;
	}
	void prepare_next_step();
	void cover_tile(uint16_t at);
	uint16_t fill_upwards(uint16_t at, uint8_t n);
	uint16_t fill_left(uint16_t at, uint8_t n);

	GB *gb_;
	Memory mem_;
};

// backs up wVBlankFunc and puts .Func's jp to DoTransitionEffect there
void DoorTransition::set_vblank()
{
	gb_->cpu.ime = false; // di
	copy(gb_, wVBlankFunc, wBackupVBlankFunc, 3);
	copy(gb_, ADDR_VBlank_DoorTransition + 0x19, wVBlankFunc, 3); // .Func, .end - .Func
	gb_->cpu.ime = true; // ei
	gb_->cpu.ime_delay = 0;
}

// .PrepareNextStepInTransitionEffect: 2 more tiles each way, from one
// tile down and left (wrapping in the BG map), for 16 steps
void DoorTransition::prepare_next_step()
{
	if (mem_.door_transition_counter() >= 16)
		return;
	mem_.transition_effect_step() = static_cast<uint8_t>(mem_.transition_effect_step() + 2);
	// wDoorTransitionEffectBGPtr (low byte first): one row below
	Byte lo = mem_[wDoorTransitionEffectBGPtr + 0];
	Byte hi = mem_[wDoorTransitionEffectBGPtr + 1];
	unsigned below = lo + TILEMAP_WIDTH;
	lo = static_cast<uint8_t>(below);
	uint8_t row = static_cast<uint8_t>(hi + (below >> 8));
	if (row >= (v0BGMap0 + TILEMAP_WIDTH * TILEMAP_HEIGHT) >> 8)
		row = static_cast<uint8_t>(row - ((TILEMAP_WIDTH * TILEMAP_HEIGHT) >> 8)); // wrap up
	hi = row;
	// one column left
	lo = static_cast<uint8_t>(lo - 1);
	if ((lo & 0x1f) == 0x1f)
		lo = static_cast<uint8_t>(lo + TILEMAP_WIDTH); // wrap right
}

// a step of the transition per two frames, the orientation toggling;
// after 17, the V-Blank function back, the music and SFX stopped and the
// level's next sub-state
void DoorTransition::update()
{
	uint8_t wbk = mem_[rWBK]; // push_wram $03
	mem_[rWBK] = GFX_RAM;
	if (!(mem_.door_transition_counter() | mem_.door_transition_orientation())) {
		mem_.unused_c186() = 0x01;
		set_vblank();
		mem_.door_transition_orientation() ^= 1;
		mem_.transition_effect_step() = 1;
		// starting with Wario's tile
		mem_[wDoorTransitionEffectBGPtr + 0] = mem_[wWarioTileBGPtr + 0];
		mem_[wDoorTransitionEffectBGPtr + 1] = mem_[wWarioTileBGPtr + 1];
	} else if (mem_.door_transition_orientation() ^= 1) { // .already_started
		prepare_next_step();
	} else if (++mem_.door_transition_counter() >= 17) { // .skip_incr_step
		copy3(wBackupVBlankFunc, wVBlankFunc);
		mem_.door_transition_counter() = 0;
		mem_.unused_c186() = 0;
		mem_.door_transition_orientation() = 0;
		mem_.music().hi() = 0xff; // stop_music
		mem_.music().lo() = 0x00;
		mem_.sfx().hi() = 0xff; // stop_sfx
		mem_.sfx().lo() = 0x00;
		++mem_.sub_state();
	}
	mem_[rWBK] = wbk; // pop_wram
}

// one tile: the attribute's palette 7 (and $7f / or $0f, in VRAM1), the
// tile $7f
void DoorTransition::cover_tile(uint16_t at)
{
	mem_[rVBK] = 1; // BANK("VRAM1")
	mem_[at] = static_cast<uint8_t>((mem_[at] & 0x7f) | 0x0f);
	mem_[rVBK] = 0; // BANK("VRAM0")
	mem_[at] = 0x7f;
}

// n tiles up (n 0 being 256), wrapping down below v0BGMap0
uint16_t DoorTransition::fill_upwards(uint16_t at, uint8_t n)
{
	do {
		cover_tile(at);
		at = static_cast<uint16_t>(at - TILEMAP_WIDTH);
		if ((at >> 8) < (v0BGMap0 >> 8))
			at = static_cast<uint16_t>((((at >> 8) + 0x04) & 0xff) << 8 | (at & 0xff)); // wrap down
	} while (--n);
	return at;
}

// n tiles right along the row (ld [hli]), wrapping back at its end
uint16_t DoorTransition::fill_left(uint16_t at, uint8_t n)
{
	do {
		cover_tile(at);
		at++;
		if (!(at & 0x1f))
			at = static_cast<uint16_t>(at - TILEMAP_WIDTH); // wrap left
	} while (--n);
	return at;
}

// the scroll, then the step's tiles: up then left, or left then up
void DoorTransition::effect()
{
	mem_[rSCY] = mem_.scy();
	mem_[rSCX] = mem_.scx();
	uint8_t wbk = mem_[rWBK]; // push_wram $03
	mem_[rWBK] = GFX_RAM;
	uint8_t step = mem_.transition_effect_step();
	uint16_t at = static_cast<uint16_t>(mem_[wDoorTransitionEffectBGPtr + 0] | mem_[wDoorTransitionEffectBGPtr + 1] << 8);
	if (mem_.door_transition_orientation()) {
		at = fill_upwards(at, step);
		fill_left(at, static_cast<uint8_t>(step + 1));
	} else { // .left_then_up
		at = fill_left(at, step);
		fill_upwards(at, step);
	}
	mem_[rWBK] = wbk; // pop_wram
}

} // namespace

} // namespace wl3::home

using wl3::home::DoorTransition;

PORT_EX(VBlank_DoorTransition, 0, ALL_REGISTERS) { DoorTransition(gb).set_vblank(); }
PORT_EX(DoorTransition, 0, ALL_REGISTERS) { DoorTransition(gb).update(); }
PORT_LOCAL_EX(VBlank_DoorTransition, DoTransitionEffect, 0, ALL_REGISTERS) { DoorTransition(gb).effect(); }
