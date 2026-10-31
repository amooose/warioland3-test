// The map's bottom bar (bank20.asm): held B with the magnifying glass opens
// it on a level, showing which of its four treasures Wario has (they appear
// one by one, at random) and, with all of them, the 8 coin. In time attack
// it shows the level's best time instead. See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

// the window's y, open and closed
constexpr uint8_t OPEN_WY = 0x78;
constexpr uint8_t CLOSED_WY = SCREEN_HEIGHT_PX;
constexpr uint8_t WINDOW_STEP = 6;

// wBottomBarFlags: the treasures shown (bits 0-3), the bar being open (4),
// all the treasures collected (6) and the 8 coin shown (7)
constexpr int FLAG_OPEN = 4;
constexpr int FLAG_ALL_TREASURES = 6;
constexpr int FLAG_8_COIN = 7;
constexpr uint8_t ALL_SHOWN = 0x0f;

// LoadSpriteInsideWindow's y, by row
constexpr uint8_t HIGH_ROW = 0x20;
constexpr uint8_t LOW_ROW = 0x24;
constexpr uint8_t NO_TREASURE = 0x09;

// the time attack best time's digits: tile and x
constexpr uint8_t DIGIT_TILES = 0x1e;

// The asm's .SetVisibleTreasures loop spins on rDIV until it picks a
// treasure not yet shown: about this many cycles a pass, as idle() passes.
constexpr int PICK_CYCLES = 80;

} // namespace

// ---------------------------------------------------------------------------
// HandleBottomBar

void BottomBar::handle()
{
	if (wram_.unk_2d068() != wram_.map_side_level_id()) {
		// Wario left the level: close the bar
		wram_.unk_2d096() = 0;
		uint8_t action = wram_.bottom_bar_action();
		if (action == BOTBAR_CLOSED)
			return;
		if (action == BOTBAR_CLOSING) {
			close();
			return;
		}
		start_closing(false);
		wram_.magnifying_glass_counter() = 0;
		return;
	}
	switch (wram_.bottom_bar_action()) {
	case BOTBAR_CLOSING: close(); return;
	case BOTBAR_OPENING: open(); return;
	default: break;
	}
	if (!possible())
		return;
	switch (wram_.bottom_bar_action()) {
	case BOTBAR_OPENED: handle_opened(); return;
	case BOTBAR_CLOSED: handle_closed(); return;
	default: return;
	}
}

// HandleBottomBar.CheckBottomBarPossible: only on a level (not the temple or
// a junction), with Wario standing on the map
bool BottomBar::possible()
{
	if (!static_cast<uint8_t>(wram_.ow_level() << 1))
		return false;
	return wram_.unk_2d013() == 0x05;
}

// Holding B for 10 frames, with the magnifying glass, opens the bar.
void BottomBar::handle_closed()
{
	if (!wram_.has_magnifying_glass())
		return;
	Byte counter = wram_.magnifying_glass_counter();
	if (!wram_.joypad_down().bit(B_PAD_B)) {
		counter = 0;
		return;
	}
	++counter;
	if (counter < 10)
		return;
	wram_.bottom_bar_action() = BOTBAR_OPENING;
	wram_.bottom_bar_state() = 0;
}

// Letting go of B closes it.
void BottomBar::handle_opened()
{
	if (wram_.joypad_down().bit(B_PAD_B))
		return;
	start_closing(true);
}

void BottomBar::start_closing(bool clear_open_flag)
{
	if (clear_open_flag)
		wram_.bottom_bar_flags().reset_bit(FLAG_OPEN);
	wram_.bottom_bar_action() = BOTBAR_CLOSING;
	wram_.bottom_bar_state() = 0;
}

// HandleBottomBar.OpenBottomBar
void BottomBar::open()
{
	if (!wram_.joypad_down().bit(B_PAD_B)) {
		start_closing(true);
		return;
	}
	Byte state = wram_.bottom_bar_state();
	switch (state) {
	case 0: // .InitBar
		wram_.bottom_bar_flags().set_bit(FLAG_OPEN);
		wram_.hdma() = 0;
		wram_.unk_2d0b5() = 0;
		++state;
		return;
	case 1: { // .OpenWindow
		Byte wy = wram_.wy();
		if (wy > OPEN_WY) {
			wy -= WINDOW_STEP;
			return;
		}
		wy = OPEN_WY;
		wram_.magnifying_glass_counter() = 0;
		++state;
		return;
	}
	case 2: // .SetVisibleTreasures
		show_next_treasure();
		return;
	case 3: { // .SetBottomBarAsOpen
		wram_.bottom_bar_action() = BOTBAR_OPENED;
		state = 0;
		Byte flags = wram_.bottom_bar_flags();
		if (flags.bit(FLAG_ALL_TREASURES))
			flags.set_bit(FLAG_8_COIN);
		return;
	}
	default:
		unrealistic();
		return;
	}
}

// HandleBottomBar.SetVisibleTreasures: every 3 frames, one more of the
// level's treasures, picked at random (from rDIV) among those not shown
void BottomBar::show_next_treasure()
{
	Byte counter = wram_.magnifying_glass_counter();
	++counter;
	if (counter < 3)
		return;
	counter = 0;

	Byte flags = wram_.bottom_bar_flags();
	uint8_t bit;
	for (;;) {
		// rl c, (DIV & $0f) + 1 times from a set carry: the bit goes round
		// c's 8 bits and the carry (9 positions; in the carry, c is 0)
		int n = (mem_[rDIV] & 0x0f) % 9;
		bit = n < 8 ? static_cast<uint8_t>(1 << n) : 0;
		if (!(bit & 0x0f))
			bit = static_cast<uint8_t>(bit << 4 | bit >> 4); // swap c
		if (!(bit & flags))
			break;
		idle(PICK_CYCLES);
	}
	flags |= bit;
	if ((flags & ALL_SHOWN) == ALL_SHOWN)
		++wram_.bottom_bar_state();
}

// HandleBottomBar.CloseBottomBar
void BottomBar::close()
{
	Byte state = wram_.bottom_bar_state();
	switch (state) {
	case 0: { // .CloseWindow
		Byte wy = wram_.wy();
		if (wy < CLOSED_WY) {
			wy += WINDOW_STEP;
			return;
		}
		wy = CLOSED_WY;
		++state;
		return;
	}
	case 1: { // .DeinitBar
		Byte flags = wram_.bottom_bar_flags();
		flags &= 1 << FLAG_ALL_TREASURES;
		wram_.compass_obj_state() = 0;
		wram_.compass_obj_unk7() = 0;
		wram_.bottom_bar_action() = BOTBAR_CLOSED;
		state = 0;
		wram_.magnifying_glass_counter() = 0;
		return;
	}
	default:
		unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// DrawBottomBar

void BottomBar::draw()
{
	if (wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F)) {
		draw_best_time(); // Func_82997
		return;
	}
	// the four treasures, as the grey, red, green and blue ones, or as a
	// blank when not shown
	static constexpr uint16_t PALS[4] = {
		wLevelGreyTreasurePal, wLevelRedTreasurePal, wLevelGreenTreasurePal, wLevelBlueTreasurePal};
	for (int i = 0; i < 4; i++) {
		bool shown = wram_.bottom_bar_flags().bit(i);
		uint8_t x = static_cast<uint8_t>(0x20 + i * 0x1c);
		if (shown)
			load_sprite(LOW_ROW, x, static_cast<uint8_t>(i), mem_[PALS[i]]);
		else
			load_sprite(LOW_ROW, x, NO_TREASURE, 0x00);
	}
	draw_button_prompt();
	draw_8_coin();
}

// Func_82997, Func_829a1: the best time's four digits, then the B prompt
// and the 8 coin as the treasures have them
void BottomBar::draw_best_time()
{
	uint8_t hi = wram_.unk_2d0ee();
	uint8_t lo = wram_.unk_2d0ef();
	load_sprite(HIGH_ROW, 0x50, static_cast<uint8_t>((hi >> 4) + DIGIT_TILES), 0x00);
	load_sprite(HIGH_ROW, 0x58, static_cast<uint8_t>((hi & 0x0f) + DIGIT_TILES), 0x00);
	load_sprite(HIGH_ROW, 0x68, static_cast<uint8_t>((lo >> 4) + DIGIT_TILES), 0x00);
	load_sprite(HIGH_ROW, 0x70, static_cast<uint8_t>((lo & 0x0f) + DIGIT_TILES), 0x00);
	draw_button_prompt();
	draw_8_coin();
}

// DrawBottomBarButtonPrompt: the B button, with the magnifying glass; held
// while the bar is open, and crossed out off the levels
void BottomBar::draw_button_prompt()
{
	if (!wram_.has_magnifying_glass())
		return;
	uint8_t frame = 0x04;
	if (!static_cast<uint8_t>(wram_.ow_level() << 1))
		frame = 0x06;
	else if (wram_.bottom_bar_flags().bit(FLAG_OPEN))
		frame = 0x05;
	wram_.cur_sprite_frame() = frame;
	wram_.cur_sprite_y_coord() = static_cast<uint8_t>(wram_.wy() + 0x10);
	wram_.cur_sprite_x_coord() = static_cast<uint8_t>(wram_.wx() + 0x10);
	wram_.cur_sprite_attributes() = 0x00;
	load_sprite_got_params(BANK_OAM_aa580, ADDR_OAM_aa580);
}

// DrawBottomBar8Coin: the spinning 8 coin, once every treasure is shown.
// Its animation runs on wCompassObjState as the duration, and the frame it
// draws is wCompassObjDuration (the asm's own odd offsets).
void BottomBar::draw_8_coin()
{
	if (!wram_.bottom_bar_flags().bit(FLAG_8_COIN))
		return;
	SET_HL(gb_, wCompassObjState);
	SET_DE(gb_, ADDR_Frameset_aa2fb);
	gb_->cpu.b = BANK_Frameset_aa2fb;
	routines_.call(ADDR_UpdateOWAnimation);
	load_sprite(LOW_ROW, 0x90, wram_.compass_obj_duration(), 0x00);
}

// LoadSpriteInsideWindow: a sprite at (x, y) in the window
void BottomBar::load_sprite(uint8_t y, uint8_t x, uint8_t frame, uint8_t attributes)
{
	wram_.cur_sprite_y_coord() = static_cast<uint8_t>(wram_.wy() + y);
	wram_.cur_sprite_x_coord() = static_cast<uint8_t>(wram_.wx() + x);
	wram_.cur_sprite_frame() = frame;
	wram_.cur_sprite_attributes() = attributes;
	load_sprite_got_params(BANK_OAM_aa0cf, ADDR_OAM_aa0cf);
}

void BottomBar::load_sprite_got_params(uint8_t bank, uint16_t oam)
{
	SET_HL(gb_, oam);
	gb_->cpu.b = bank;
	routines_.call(ADDR_LoadOWSpriteWithScroll_GotParams);
}

void BottomBar::idle(int cycles)
{
	if (!gb_->no_tick)
		gb_tick(gb_, cycles);
}

void BottomBar::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

} // namespace wl3::overworld

// Func_80e89 calls both, and the top bar; what runs after them loads every
// register it uses (Func_852e5 and the w2d013 states read wram first).
PORT_EX(HandleBottomBar, 0, ALL_REGISTERS)
{
	wl3::overworld::BottomBar(gb).handle();
}

PORT_EX(DrawBottomBar, 0, ALL_REGISTERS)
{
	wl3::overworld::BottomBar(gb).draw();
}

PORT_COVERED(LoadSpriteInsideWindow_High);
PORT_COVERED(LoadSpriteInsideWindow_Low);
PORT_COVERED(LoadSpriteInsideWindow);
PORT_COVERED(DrawBottomBar8Coin);
PORT_COVERED(DrawBottomBarButtonPrompt);
PORT_COVERED(Func_82997);
PORT_COVERED(Func_829a1);
