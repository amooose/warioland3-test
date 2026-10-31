// The map sides' scene objects between scenes (bank2d.asm). See
// overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

void SideAnimations::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void SideAnimations::set_state(uint16_t state, uint8_t value)
{
	gb_->cpu.a = value;
	SET_HL(gb_, state);
	routines_.call(ADDR_SetSceneObjState);
}

// UpdateMapSideOWAnimations: each of the side's objects, by its state
void SideAnimations::update()
{
	switch (wram_.cur_map_side()) {
	case NORTH:
		for (uint16_t state : {wSceneObj2State, wSceneObj3State, wSceneObj6State,
			wSceneObj7State, wSceneObj14State, wSceneObj15State})
			north(mem_[state]);
		return;
	case WEST:
		for (uint16_t state : {wSceneObj1State, wSceneObj7State, wSceneObj8State, wSceneObj5State})
			west(mem_[state]);
		return;
	case SOUTH:
		for (uint16_t state : {wSceneObj7State, wSceneObj14State, wSceneObj15State})
			south(mem_[state]);
		return;
	case EAST:
		for (uint16_t state : {wSceneObj5State, wSceneObj7State})
			east(mem_[state]);
		return;
	default:
		unrealistic();
		return;
	}
}

// Func_b7a32 / Func_b7b26 / Func_b7c5c: something peeking out at its
// levels (.Func_b7a5a and the like) every 0x78 frames, while the map's idle
void SideAnimations::peek(const Peek &peek)
{
	auto idle = [&] {
		if (wram_.cur_event() || mem_[peek.flag])
			return false;
		uint8_t level = wram_.map_side_level_id();
		return level == peek.level_a || level == peek.level_b;
	};
	Byte timer = mem_[peek.timer];
	if (!idle()) {
		++timer;
		if (timer < 0x78)
			return;
	}
	timer = 0;
	mem_[peek.flag] = 0;
	uint8_t state = peek.hide;
	if (idle()) {
		mem_[peek.flag] = 0x01;
		state = peek.show;
	}
	set_state(peek.state, state);
}

// ---------------------------------------------------------------------------
// The north

// Func_b7984
void SideAnimations::north(uint8_t state)
{
	switch (state) {
	case 0x05: // .Func_b79de: the block breaks, from level 6
		if (wram_.cur_event() || mem_[wLevel2] != 0x06)
			return;
		set_state(wSceneObj3State, 0x06);
		play_sfx(mem_, SFX_BLOCK_BREAK);
		return;
	case 0x0e: // .Func_b7a32: the snake
		peek({w2d124, w2d125, 0x02, 0x07, wSceneObj2State, 0x1d, 0x0d});
		return;
	case 0x10: case 0x11: case 0x12: case 0x13:
		fall_leaves();
		return;
	case 0x18: // .Func_b79fa: day and night
		if (by_day())
			set_state(wSceneObj15State, 0x17);
		return;
	case 0x19: // .Func_b7a08
		if (!by_day())
			set_state(wSceneObj15State, 0x16);
		return;
	case 0x1a: // .Func_b7a16
		if (by_day())
			set_state(wSceneObj14State, 0x1b);
		return;
	case 0x1b: // .Func_b7a24
		if (!by_day())
			set_state(wSceneObj14State, 0x1a);
		return;
	default:
		if (state >= 44)
			unrealistic();
		return; // Func_b791d
	}
}

// Func_b542e: the two leaves, each (counting frames after its phase) to
// a branch, waiting, falling, lying, drifting down, then back up once
// they're off it
void SideAnimations::fall_leaves()
{
	leaf(w2d12c, wSceneObj6); // .Func_b5435
	leaf(w2d12e, wSceneObj7); // .Func_b54ba
}

void SideAnimations::leaf(uint16_t phase_addr, uint16_t obj)
{
	uint16_t state = static_cast<uint16_t>(obj + 6);
	Byte phase = mem_[phase_addr];
	if (!phase)
		return;
	Byte frames = mem_[phase_addr + 1];
	++frames;
	auto next = [&] { // .asm_b5458
		++phase;
		frames = 0;
	};
	switch (phase) {
	case 1: // .Func_b544a
		if (frames < 0x78)
			return;
		set_state(state, 0x12);
		next();
		return;
	case 2: // .Func_b5461
		if (frames < 0x5a)
			return;
		set_state(state, 0x13);
		next();
		return;
	case 3: // .Func_b5471
		if (frames < 0x28)
			return;
		set_state(state, 0x10);
		mem_[obj] = 0x38;
		next();
		return;
	case 4: // .Func_b5486
		if (frames >= 0x5a)
			next();
		return;
	case 5: { // .Func_b548e: down a pixel every 16 frames, to 0x48
		if (frames & 0x0f)
			return;
		Byte y = mem_[obj];
		++y;
		if (y == 0x48)
			next();
		return;
	}
	case 6: // .Func_b549e
		if (mem_[obj + 2] != 0x46 || !mem_[obj])
			return;
		set_state(state, 0x11);
		phase = 0x01;
		frames = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// The west

// Func_b7a6d
void SideAnimations::west(uint8_t state)
{
	switch (state) {
	case 0x02: // Func_b7ab7: the tornado, but in event 4
		if (wram_.cur_event() == 0x04)
			return;
		MapFunctions(gb_).update_tornado();
		return;
	case 0x06: { // Func_b7ac0: the elevator, by night
		if (wram_.cur_event())
			return;
		Byte waiting = mem_[wSceneObj8Unk7];
		if (!waiting) {
			if (mem_[wSceneObj7State] || mem_[wOWPalTransitionState])
				return;
			++waiting;
		}
		uint8_t value = 0x07;
		if (!by_day()) { // .asm_b7af0
			mem_[wSceneObj8State] = 0x08;
			mem_[wSceneObj8Duration] = 0;
			value = 0;
		} else if (mem_[wOWPalTransitionState] != 0x02) {
			return;
		}
		set_state(wSceneObj7State, value); // .asm_b7ae5
		waiting = 0;
		return;
	}
	case 0x08: { // Func_b7afb: and by day
		if (wram_.cur_event())
			return;
		Byte waiting = mem_[wSceneObj8Unk7];
		if (!waiting) {
			if (mem_[wOWPalTransitionState])
				return;
			++waiting;
		}
		if (!by_day() && mem_[wOWPalTransitionState] != 0x02)
			return;
		mem_[wSceneObj8State] = 0x06;
		mem_[wSceneObj8Duration] = 0;
		waiting = 0;
		return;
	}
	case 0x11: // Func_b7b26
		peek({w2d12a, w2d12b, 0x01, 0x02, wSceneObj5State, 0x13, 0x10});
		return;
	default:
		if (state >= 36)
			unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// The south

// Func_b7c18
void SideAnimations::south(uint8_t state)
{
	switch (state) {
	case 0x15: // Func_b7c5c
		peek({w2d12e, w2d12f, 0x00, 0x01, wSceneObj7State, 0x1d, 0x14});
		return;
	case 0x1e: // Func_b7c97
		if (by_day())
			set_state(wSceneObj15State, 0x1f);
		return;
	case 0x1f: // Func_b7ca5
		if (!by_day())
			set_state(wSceneObj15State, 0x1e);
		return;
	case 0x20: // Func_b7cb3
		moon();
		return;
	default:
		if (state >= 33)
			unrealistic();
		return;
	}
}

// Func_b7cb3: the moon (scene object 14) rising up-right, a pixel every
// 4 frames: once to 0x80 (w2d07c), or again and again to 0x88
void SideAnimations::moon()
{
	if (wram_.cur_event())
		return;
	uint8_t x = mem_[wSceneObj14XCoord];
	Byte started = mem_[w2d07d];
	if (mem_[w2d07c]) { // .asm_b7cc0
		if (x == 0x80)
			return;
		if (!started) { // .Func_b7cda
			++started;
			mem_[wSceneObj14YCoord] = 0x25;
			mem_[wSceneObj14XCoord] = 0x70;
		}
		if (wram_.global_counter() & 0x03)
			return;
		--mem_[wSceneObj14YCoord];
		++mem_[wSceneObj14XCoord];
		return;
	}
	if (x == 0x70) // .asm_b7ce7
		return;
	if (!started) // .Func_b7d0d
		++started;
	if (wram_.global_counter() & 0x03)
		return;
	--mem_[wSceneObj14YCoord];
	Byte moved = mem_[wSceneObj14XCoord];
	++moved;
	if (moved != 0x88)
		return;
	mem_[wSceneObj14YCoord] = 0x25;
	mem_[wSceneObj14XCoord] = 0x70;
}

// ---------------------------------------------------------------------------
// The east

// Func_b7be9: in a scene unlocking or highlighting a level (it's hidden),
// or in an event
bool SideAnimations::east_busy()
{
	uint8_t action = wram_.ow_scene_action();
	if (action == UNLOCK_LEVEL || action == HIGHLIGHT_LEVEL) {
		set_state(wSceneObj7State, 0x00);
		return true;
	}
	return wram_.cur_event();
}

// Func_b7b61
void SideAnimations::east(uint8_t state)
{
	switch (state) {
	case 0x12: case 0x13: { // Func_b7ba9 / Func_b7bc9: to the other state,
		// once the palettes reach the other time of day (2) or straight
		// away (0)
		if (east_busy())
			return;
		bool other = state == 0x12 ? !by_day() : by_day();
		if (mem_[wOWPalTransitionState] != (other ? 0x02 : 0x00))
			return;
		set_state(wSceneObj7State, state == 0x12 ? 0x13 : 0x12);
		return;
	}
	case 0x19: // Func_b7c05: every 256 frames
		if (wram_.cur_event())
			return;
		if (++mem_[w2d12b])
			return;
		set_state(wSceneObj5State, 0x1a);
		return;
	default:
		if (state >= 35)
			unrealistic();
		return;
	}
}

} // namespace wl3::overworld

// Farcalled by the map's walking and scenes (C++), which read no registers
// after.
PORT_EX(UpdateMapSideOWAnimations, 0, ALL_REGISTERS) { wl3::overworld::SideAnimations(gb).update(); }
PORT_EX(Func_b542e, 0, ALL_REGISTERS) { wl3::overworld::SideAnimations(gb).fall_leaves(); }

// Only UpdateMapSideOWAnimations runs these.
PORT_COVERED(Func_b7984);
PORT_COVERED(Func_b7a6d);
PORT_COVERED(Func_b7ab7);
PORT_COVERED(Func_b7ac0);
PORT_COVERED(Func_b7afb);
PORT_COVERED(Func_b7b26);
PORT_COVERED(Func_b7b4e);
PORT_COVERED(Func_b7b61);
PORT_COVERED(Func_b7ba9);
PORT_COVERED(Func_b7bc9);
PORT_COVERED(Func_b7be9);
PORT_COVERED(Func_b7c05);
PORT_COVERED(Func_b7c18);
PORT_COVERED(Func_b7c5c);
PORT_COVERED(Func_b7c84);
PORT_COVERED(Func_b7c97);
PORT_COVERED(Func_b7ca5);
PORT_COVERED(Func_b7cb3);
PORT_COVERED(Func_b791d);
PORT_COVERED(UpdateOWTornado);
