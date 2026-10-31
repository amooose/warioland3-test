// The small screens, as C++: the notice on a Game Boy that can't run the
// game (src/engine/gb_incompatible), the language selection of a new game
// (src/engine/language_selection) and the "Perfect" screen after collecting
// everything (src/engine/perfect), and the credits (src/engine/credits).
//
// Their V-Blank functions, copied to wVBlankFunc, are still C
// (src/ported/engine/language_selection/vblank.c) or home code.
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::screens {

// What the screens share: the machine, and their objects.
class Screen {
public:
	explicit Screen(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

protected:
	MenuObject object(uint16_t base) const { return MenuObject(gb_, base); }
	void unrealistic();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

class GBIncompatible : public Screen {
public:
	using Screen::Screen;

	void run_state(); // GBIncompatibleStateTable

private:
	void init(); // GBIncompatible
};

class LanguageSelection : public Screen {
public:
	using Screen::Screen;

	void run_state(); // LanguageSelectionStateTable.jump

private:
	void init(); // InitLanguageSelection
	void update(); // LanguageSelection
	void choose(uint8_t language, uint8_t cursor_y);
	void draw_cursor();
};

class Perfect : public Screen {
public:
	using Screen::Screen;

	void run_state(); // _PerfectStateTable

private:
	void init(); // InitPerfectScreen
	void update(); // UpdatePerfectScreen
	void next_colours(); // the palettes cycling as the screen fades in
	void set_colours(uint16_t pals); // Func_dc101 and others
	void draw_sprite(); // Func_dc158
};

class Credits : public Screen {
public:
	using Screen::Screen;

	void run_state(); // _CreditsStateTable

private:
	MenuObject music_box() const { return object(wCreditsMusicBox); }
	MenuObject winding_key() const { return object(wMenuObj1); }

	void init(); // InitCredits
	void load_gfx(); // LoadCreditsGfx
	void set_music_box(uint8_t y, uint8_t x, uint16_t frameset, uint16_t oam, uint8_t bank);
	void animate_music_box();
	void draw_music_box(); // LoadCreditsMusicBoxSprite
	void show(); // ShowCredits
	void write_next_line();
	void music_box_plays(); // Func_160190
	void close_up(); // Func_16026c
	void music_box_stops(); // Func_1602ee
	void wind_key(bool slowly); // Func_160318 (slowly), Func_160365
	void last_state(); // Func_160365
};

} // namespace wl3::screens
