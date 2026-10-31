// Holding B on a level (bank20.asm, Func_81477): Wario turns into arrows,
// and an arrow runs along each path out of the level, up to the next one.
// w2d0e3 is the step, w2d0e2 counts its frames. Each of the four arrows
// (right, left, up, down) keeps its state in a $20-byte slot from w2d800,
// swapped in and out of the path walking's variables (w2d880, w2d0a0,
// wOWCoords1) to move along its path as Wario does. See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint16_t SLOTS = w2d800;
constexpr uint16_t SLOT_SIZE = 0x20;
constexpr uint16_t SLOTS_SIZE = 0x8e;

// the arrow being moved: w2d880 (9 bytes), w2d0a0 (7) and wOWCoords1 (8)
constexpr uint16_t ARROW = w2d880;
constexpr uint16_t ARROW_Y = ARROW + 0;
constexpr uint16_t ARROW_X = ARROW + 1;
constexpr uint16_t ARROW_DURATION = ARROW + 4;
constexpr uint16_t ARROW_FRAMESET_OFFSET = ARROW + 5;
constexpr uint16_t ARROW_STATE = ARROW + 6; // its animation (direction + 1)
constexpr uint16_t ARROW_STEP = ARROW + 7;
constexpr uint16_t ARROW_COUNTER = ARROW + 8;
constexpr uint16_t ARROW_INDEX = w2d88b;
constexpr uint16_t LEVEL_PATHS = w2d889; // this level's paths (Func_82111)
constexpr uint16_t SAVED_COORDS = w2d88e; // Wario's, while he's arrows

// .Func_81615's steps
enum Step : uint8_t {
	START,
	FIND_PATH,
	START_PATH,
	MOVE,
	WAIT,
	DONE,
};

constexpr uint8_t OW_ANIMATION_BANK = 0x2a;

// Func_81714: the arrows' animation states, drawn in turn: the four
// slots, then the copies of them at wOWUIObj1 and w2d160 that trail behind
constexpr uint16_t ARROW_STATES[12] = {
	wDayNightTransistionSteps, wPalFade1Colour1BlueUnk3, wPalFade1Colour3RedUnk3, wPalFade1Colour4GreenUnk3,
	wOWUIObj1State, wOWUIObj2State, wd156, w2d15e,
	w2d166, w2d16e, w2d176, w2d17e,
};
static_assert(wDayNightTransistionSteps == SLOTS + 6 && wPalFade1Colour4GreenUnk3 == SLOTS + 3 * SLOT_SIZE + 6);
constexpr uint16_t ARROW_FRAMESETS = ADDR_Func_81714 + 0x6f; // Func_81714.framesets

} // namespace

void PathPreview::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// Func_81477
void PathPreview::update()
{
	if (!wram_.joypad_down().bit(B_PAD_B)) {
		// let go: Wario again, where he was
		if (!wram_.unk_2d0e3())
			return;
		if (wram_.unk_2d106() != OWWARIO_DOWN)
			set_wario_oam(OWWARIO_DOWN);
		uint8_t y = mem_[SAVED_COORDS + 0];
		wram_.owy_coord1() = y;
		wram_.unk_2d100() = y;
		uint8_t x = mem_[SAVED_COORDS + 1];
		wram_.owx_coord1() = x;
		wram_.unk_2d101() = x;
		wram_.unk_2d0e3() = 0;
		return;
	}
	++wram_.unk_2d0e2();
	switch (wram_.unk_2d0e3()) {
	case 0: // Func_814b2
		mem_[SAVED_COORDS + 0] = wram_.unk_2d100();
		mem_[SAVED_COORDS + 1] = wram_.unk_2d101();
		break;
	case 1: // Func_814c4: after 10 frames, the arrows on the level
		if (wram_.unk_2d0e2() < 10)
			return;
		play_sfx(mem_, SFX_113);
		wram_.owy_coord1() = wram_.unk_2d069();
		wram_.unk_2d100() = wram_.unk_2d069();
		wram_.owx_coord1() = wram_.unk_2d06a();
		wram_.unk_2d101() = wram_.unk_2d06a();
		set_wario_oam(OWWARIO_ARROWS);
		break;
	case 2:
		start_arrows();
		return;
	case 3:
		move_arrows();
		return;
	default:
		unrealistic();
		return;
	}
	next_step();
}

// Func_814bd
void PathPreview::next_step()
{
	wram_.unk_2d0e2() = 0;
	++wram_.unk_2d0e3();
}

// Func_814ec: the arrows' slots and objects cleared, and the level's paths
void PathPreview::start_arrows()
{
	for (uint16_t i = 0; i < SLOTS_SIZE; i++)
		mem_[SLOTS + i] = 0;
	for (uint16_t i = 0; i < 0x40; i++)
		mem_[wOWUIObj1 + i] = 0;
	routines_.call(ADDR_Func_82111);
	mem_[LEVEL_PATHS + 0] = gb_->cpu.l;
	mem_[LEVEL_PATHS + 1] = gb_->cpu.h;
	next_step();
}

// Func_8150c
void PathPreview::move_arrows()
{
	for (uint8_t i = 0; i < 4; i++) {
		swap_arrow(i, true); // .Func_81576
		step_arrow(); // .Func_81615
		swap_arrow(mem_[ARROW_INDEX], false); // .Func_81595
	}
	// the trails: copies of the arrows' first 8 bytes, at 20 and 40
	// frames, each shown from 4 frames later
	uint8_t frames = wram_.unk_2d0e2();
	if (frames == 0x14)
		copy_trail(wOWUIObj1); // .Func_815c1
	if (wram_.unk_2d0e2() == 0x28)
		copy_trail(w2d160); // .Func_815e5
	if (wram_.unk_2d0e2() == 0x2c)
		hide_trail(wOWUIObj1State); // .Func_81609
	if (wram_.unk_2d0e2() == 0x40)
		hide_trail(w2d160 + 6);
	draw_arrows(); // Func_81714

	for (uint16_t slot = 0; slot < 4; slot++) {
		if (mem_[static_cast<uint16_t>(SLOTS + slot * SLOT_SIZE + 7)] != DONE)
			return;
	}
	--wram_.unk_2d0e3(); // all done: again from the level
}

// .Func_81576 (in) and .Func_81595 (out): an arrow's slot to the path
// walking's variables, or back
void PathPreview::swap_arrow(uint8_t index, bool in)
{
	if (in)
		mem_[ARROW_INDEX] = index;
	// rlca / swap a: index * 32 for the four slots
	uint8_t rotated = static_cast<uint8_t>(index << 1 | index >> 7);
	uint8_t offset = static_cast<uint8_t>(rotated << 4 | rotated >> 4);
	uint16_t slot = static_cast<uint16_t>(SLOTS + offset);
	static constexpr struct {
		uint16_t addr;
		uint8_t size;
	} PARTS[3] = {{ARROW, 9}, {w2d0a0, 7}, {wOWCoords1, 8}};
	for (auto part : PARTS) {
		for (uint8_t i = 0; i < part.size; i++) {
			if (in)
				mem_[static_cast<uint16_t>(part.addr + i)] = mem_[slot];
			else
				mem_[slot] = mem_[static_cast<uint16_t>(part.addr + i)];
			slot = static_cast<uint16_t>(slot + 1);
		}
	}
}

// .Func_81615: the arrow's step
void PathPreview::step_arrow()
{
	Byte step = mem_[ARROW_STEP];
	switch (step) {
	case START: { // .Func_81625: a tile off the level, if there's a path
		uint8_t index = mem_[ARROW_INDEX];
		uint8_t y = wram_.unk_2d100();
		uint8_t x = wram_.unk_2d101();
		uint8_t n = static_cast<uint8_t>(index + 1);
		uint8_t delta = (n & 0x02) ? 0xf8 : 0x08;
		if (n < 3)
			x = static_cast<uint8_t>(x + delta);
		else
			y = static_cast<uint8_t>(y + delta);
		mem_[ARROW_Y] = y;
		wram_.owy_coord1() = y;
		wram_.owy_coord2() = y;
		wram_.unk_2d06f() = y;
		mem_[ARROW_X] = x;
		wram_.owx_coord1() = x;
		wram_.owx_coord2() = x;
		wram_.unk_2d070() = x;
		mem_[ARROW_DURATION] = 0;
		mem_[ARROW_FRAMESET_OFFSET] = 0;
		// rrca n times from the swapped d-pad bits: the direction's bit
		uint8_t allowed = wram_.ow_allowed_d_pad_input();
		allowed = static_cast<uint8_t>(allowed << 4 | allowed >> 4);
		bool carry = false;
		for (uint8_t b = n; b; --b) {
			carry = allowed & 1;
			allowed = static_cast<uint8_t>(allowed >> 1 | allowed << 7);
		}
		mem_[ARROW_STATE] = carry ? n : 0;
		++step;
		return;
	}
	case FIND_PATH: { // .Func_8167f: the level the path leads to
		if (!mem_[ARROW_STATE]) {
			step = DONE;
			return;
		}
		uint16_t entry = static_cast<uint16_t>(mem_.word(LEVEL_PATHS) + mem_[ARROW_INDEX]);
		uint8_t level = mem_[entry];
		wram_.ow_path_shape() = mem_[static_cast<uint16_t>(entry + 4)];
		if (level == 0x80) {
			step = DONE;
			return;
		}
		gb_->cpu.a = level;
		routines_.call(ADDR_GetMapLevelCoords);
		uint16_t coords = REG_HL(gb_);
		wram_.unk_2d06f() = mem_[coords];
		wram_.unk_2d070() = mem_[static_cast<uint16_t>(coords + 1)];
		++step;
		return;
	}
	case START_PATH: // .Func_816b3
		next_path_leg();
		routines_.call(ADDR_Func_821a1);
		++step;
		return;
	case MOVE: // .Func_816be: along the path, as Wario walks
		routines_.call(ADDR_Func_82208);
		mem_[ARROW_Y] = wram_.owy_coord1();
		mem_[ARROW_X] = wram_.owx_coord1();
		if (wram_.unk_2d0a1())
			return;
		if (next_path_leg() == 0xff) {
			mem_[ARROW_COUNTER] = 0;
			++step;
			return;
		}
		routines_.call(ADDR_Func_821a1);
		face_path(); // .Func_816f6
		return;
	case WAIT: { // .Func_816ea: 8 frames at the next level
		Byte counter = mem_[ARROW_COUNTER];
		++counter;
		if (counter < 8)
			return;
		++step;
		return;
	}
	case DONE: // .Func_816f5
		return;
	default:
		unrealistic();
		return;
	}
}

// Func_810fc: the path's next leg; its new shape, or $ff at its end
uint8_t PathPreview::next_path_leg()
{
	SET_BC(gb_, wOWPathShape);
	routines_.call(ADDR_Func_810fc);
	return gb_->cpu.a;
}

// .Func_816f6: the arrow pointing along the leg (1 right, 2 left, 3 up,
// 4 down), its animation from the start
void PathPreview::face_path()
{
	Byte flags = wram_.unk_2d0a0();
	uint8_t state;
	if (flags.bit(0))
		state = flags.bit(2) ? 2 : 1;
	else
		state = flags.bit(1) ? 3 : 4;
	mem_[ARROW_STATE] = state;
	mem_[ARROW_FRAMESET_OFFSET] = 0;
	mem_[ARROW_DURATION] = 0;
}

// .Func_815c1, .Func_815e5: the arrows' first 8 bytes (an OW object each)
void PathPreview::copy_trail(uint16_t dest)
{
	for (uint16_t slot = 0; slot < 4; slot++) {
		for (uint16_t i = 0; i < 8; i++)
			mem_[static_cast<uint16_t>(dest + slot * 8 + i)] = mem_[static_cast<uint16_t>(SLOTS + slot * SLOT_SIZE + i)];
	}
}

// .Func_81609: the four objects' states from hl, cleared
void PathPreview::hide_trail(uint16_t state)
{
	for (uint16_t i = 0; i < 4; i++)
		mem_[static_cast<uint16_t>(state + i * 8)] = 0;
}

// Func_81714
void PathPreview::draw_arrows()
{
	for (uint16_t state : ARROW_STATES) {
		// .Func_8175d
		uint8_t s = mem_[state];
		if (!s)
			continue;
		uint8_t offset = static_cast<uint8_t>((s - 1) * 2);
		uint16_t frameset = mem_.word(static_cast<uint16_t>(ARROW_FRAMESETS + offset));
		// ld a, [hld] / dec l: the duration, two bytes before the state
		uint16_t duration = static_cast<uint16_t>(state - 1);
		duration = static_cast<uint16_t>((duration & 0xff00) | static_cast<uint8_t>(duration - 1));
		SET_DE(gb_, frameset);
		SET_HL(gb_, duration);
		gb_->cpu.b = OW_ANIMATION_BANK;
		routines_.call(ADDR_UpdateOWAnimation);
		gb_->cpu.l &= 0xf8; // the object's start
		SET_DE(gb_, ADDR_OAM_aa0cf);
		gb_->cpu.b = BANK_OAM_aa0cf;
		routines_.call(ADDR_LoadOWSpriteWithScroll);
	}
}

void PathPreview::set_wario_oam(uint8_t oam)
{
	gb_->cpu.a = oam;
	routines_.call(ADDR_SetOWWarioOAM);
}

} // namespace wl3::overworld

// Func_80ff7 (map_walk.cpp) calls it and reloads what it uses after.
PORT_EX(Func_81477, 0, ALL_REGISTERS)
{
	wl3::overworld::PathPreview(gb).update();
}

// The unreferenced Func_8143d also runs these two, through a jumptable, and
// reloads every register after them.
PORT_EX(Func_814ec, 0, ALL_REGISTERS)
{
	wl3::overworld::PathPreview(gb).start_arrows();
}

PORT_EX(Func_8150c, 0, ALL_REGISTERS)
{
	wl3::overworld::PathPreview(gb).move_arrows();
}

PORT_COVERED(Func_814b2);
PORT_COVERED(Func_814bd);
PORT_COVERED(Func_814c4);
PORT_COVERED(Func_81714);
