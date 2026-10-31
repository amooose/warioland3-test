// The credits (src/engine/credits): the names scroll up past a music box,
// whose key is wound at the end; see screens.hpp.
#include "screens.hpp"

namespace wl3::screens {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;
constexpr uint16_t VBLANK_CREDITS_FUNC = ADDR_VBlank_160439 + 0x0c; // VBlank_160439.Func
constexpr uint8_t VBLANK_CREDITS_FUNC_SIZE = 0x6c;
constexpr uint8_t NEWLINE = 0x7f; // in the credits charmap

// a big-endian pointer in RAM
uint16_t read_pointer(Memory mem, uint16_t addr)
{
	return static_cast<uint16_t>(mem[addr] << 8 | mem[static_cast<uint16_t>(addr + 1)]);
}

void write_pointer(Memory mem, uint16_t addr, uint16_t value)
{
	mem[addr] = static_cast<uint8_t>(value >> 8);
	mem[static_cast<uint16_t>(addr + 1)] = static_cast<uint8_t>(value);
}

// inc e: the next byte of a buffer that stays in its 256-byte page
uint16_t next_in_page(uint16_t addr)
{
	return static_cast<uint16_t>((addr & 0xff00) | static_cast<uint8_t>(addr + 1));
}

} // namespace

void Credits::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: show(); break;
	case 4: music_box_plays(); break;
	case 5: close_up(); break;
	case 6: music_box_stops(); break;
	case 7: wind_key(true); break;
	case 8: last_state(); break;
	case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void Credits::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	routines_.load_pals_to_temp_pals1(ADDR_Pals_1604cc);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_16050c);
	load_gfx();
	mem_[rVBK] = VRAM1;
	routines_.decompress(ADDR_CreditsAttrmap, v1BGMap1);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_CreditsTilemap, v0BGMap1);
	routines_.copy(VBLANK_CREDITS_FUNC, wVBlankFunc, VBLANK_CREDITS_FUNC_SIZE);

	mem_[rSCX] = 0x04;
	wram_.scx() = 0x04;
	mem_[rSCY] = 0;
	wram_.scy() = 0;
	wram_.credits_page() = 0;
	mem_[rWY] = 0;
	wram_.unk_cee4() = 0;
	mem_[rWX] = 0 + WX_OFS;

	write_pointer(mem_, wCreditsBGMapPtr, v0BGMap0 + 0x2e7);
	write_pointer(mem_, wCreditsTextPtr, ADDR_CreditsText);
	write_pointer(mem_, wCreditsPalsMapPtr, ADDR_CreditsPalsMap);

	set_music_box(0x5b, 0x18, ADDR_Frameset_161cc4, ADDR_OAM_1617e8, BANK_OAM_1617e8);
	wram_.credits_music_box_var() = 0;
	MenuObject box = music_box();
	box.animate();
	box.load_sprite(ADDR_OAM_1617e8, false);
	MenuObject key = winding_key();
	key.init(0x5b, 0x18, ADDR_Frameset_161caf);
	key.animate();
	key.load_sprite(ADDR_OAM_1617e8, false);
	routines_.clear_unused_virtual_oam();

	stop_music(mem_);
	wram_.timer() = 120; // before the music starts
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

void Credits::load_gfx()
{
	mem_[rVBK] = VRAM1;
	SET_HL(gb_, ADDR_PrologueGfx);
	SET_BC(gb_, v0Tiles0);
	routines_.farcall(BANK_PrologueGfx, ADDR_Decompress);
	routines_.copy_bc(ADDR_MusicBoxGfx, v1Tiles1, 0x30 * TILE_SIZE, BANK_MusicBoxGfx);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_CreditsFontGfx, v0Tiles0);
	routines_.decompress(ADDR_CreditsMusicBoxGfx, v0Tiles0);
}

// the music box at (y, x), its frameset and OAM data
void Credits::set_music_box(uint8_t y, uint8_t x, uint16_t frameset, uint16_t oam, uint8_t bank)
{
	music_box().init(y, x, frameset);
	write_pointer(mem_, wCreditsMusicBoxOAMPtr, oam);
	wram_.credits_music_box_oam_bank() = bank;
}

void Credits::animate_music_box()
{
	uint8_t bank = wram_.credits_music_box_oam_bank();
	wram_.temp_bank() = bank;
	music_box().animate_far(bank);
}

void Credits::draw_music_box()
{
	SET_HL(gb_, wCreditsMusicBox);
	routines_.call(ADDR_LoadCreditsMusicBoxSprite);
}

// The names scroll up a pixel a frame (every other frame but on the first
// and last pages), a new line written every 8 pixels.
void Credits::show()
{
	Byte timer = wram_.timer();
	if (timer) {
		if (!--timer)
			play_music(mem_, MUSIC_CREDITS);
		music_box().load_sprite(ADDR_OAM_1617e8, false);
		winding_key().load_sprite(ADDR_OAM_1617e8, false);
		routines_.clear_unused_virtual_oam();
		return;
	}

	MenuObject box = music_box();
	box.animate();
	box.load_sprite(ADDR_OAM_1617e8, false);
	MenuObject key = winding_key();
	key.animate();
	key.load_sprite(ADDR_OAM_1617e8, false);
	routines_.clear_unused_virtual_oam();

	uint8_t page = wram_.credits_page();
	if (page == NUM_CREDITS_PAGES) {
		wram_.credits_music_box_var() = 0;
		++wram_.sub_state();
		return;
	}
	if (page && page != NUM_CREDITS_PAGES - 1 && (wram_.global_counter() & 0x01))
		return;

	// wSCY and wCreditsPage are one 16-bit scroll
	Word scroll = wram_.scy().word();
	scroll = static_cast<uint16_t>(scroll + 1);
	page = wram_.credits_page();
	if (page < 1 || page >= NUM_CREDITS_PAGES || (wram_.scy() & 0x07))
		return;
	write_next_line();
}

void Credits::write_next_line()
{
	uint16_t text = read_pointer(mem_, wCreditsTextPtr);
	uint16_t buffer = wCreditsTextBuffer;
	uint8_t c;
	do {
		c = mem_[text++];
		mem_[buffer] = c;
		buffer = next_in_page(buffer);
	} while (c != NEWLINE);
	write_pointer(mem_, wCreditsTextPtr, text);

	// each letter's palette
	uint16_t pals = read_pointer(mem_, wCreditsPalsMapPtr);
	uint16_t dest = wCreditsTextPals;
	for (int i = 0; i < CREDITS_TEXT_LENGTH; i++) {
		mem_[dest] = mem_[pals++];
		dest = next_in_page(dest);
	}
	write_pointer(mem_, wCreditsPalsMapPtr, pals);

	// the next row, wrapping within the BG map
	uint16_t row = static_cast<uint16_t>(read_pointer(mem_, wCreditsBGMapPtr) + TILEMAP_WIDTH);
	write_pointer(mem_, wCreditsBGMapPtr, static_cast<uint16_t>(row & 0xfbff));
	wram_.unk_cee4() = 1 << 7; // the V-Blank function writes it
}

// The music box comes in and plays, its key turning.
void Credits::music_box_plays()
{
	MenuObject box = music_box();
	Byte var = wram_.credits_music_box_var();
	bool odd_frame = wram_.global_counter() & 0x01;
	if (!var) {
		uint8_t x = box.x();
		if (x < 0x38 || (x < 0x50 && !odd_frame)) {
			++box.x();
			++winding_key().x();
			goto update_anims;
		}
		if (x < 0x50)
			goto add_sprites;
		var = 1; // stops moving
	}
	if (odd_frame)
		goto add_sprites;
	if (wram_.credits_music_box_animation_ended()) {
		wram_.credits_music_box_animation_ended() = 0;
		uint8_t v = ++var;
		if (v == 4) {
			// the close-up starts
			set_music_box(0x5b, 0x50, ADDR_Frameset_161ce8, ADDR_OAM_1617e8, BANK_OAM_1617e8);
			wram_.credits_music_box_var() = 0;
			winding_key().init(0xe0, 0x4e, ADDR_Frameset_161ce5);
			wram_.timer() = 16;
			++wram_.sub_state();
		} else if (v >= 3) {
			box.set_frameset(ADDR_Frameset_161cee);
		}
	}

update_anims:
	animate_music_box();
	wram_.credits_music_box_animation_ended() = wram_.obj_anim_was_reset();
	if (!(wram_.credits_music_box_var() == 0x03 && (wram_.global_counter() & 0x03)))
		winding_key().animate();

add_sprites:
	draw_music_box();
	winding_key().load_sprite(ADDR_OAM_1617e8, false);
	routines_.clear_unused_virtual_oam();
}

void Credits::close_up()
{
	Byte timer = wram_.timer();
	if (!--timer) {
		set_music_box(0x50, 0x50, ADDR_Frameset_15e7ed, ADDR_OAM_15e44a, BANK_OAM_15e44a);
		routines_.load_pals_to_temp_pals1(ADDR_Pals_16054c);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_16058c);
		wram_.unk_cee4() = 0x2 | (1 << 7);
		++wram_.sub_state();
		animate_music_box();
	} else if (timer == 6) {
		set_music_box(0x5b, 0x50, ADDR_Frameset_161ceb, ADDR_OAM_1617e8, BANK_OAM_1617e8);
		animate_music_box();
	}
	draw_music_box();
	routines_.clear_unused_virtual_oam();
}

void Credits::music_box_stops()
{
	animate_music_box();
	draw_music_box();
	routines_.clear_unused_virtual_oam();
	routines_.load_pals_to_temp_pals1(ADDR_Pals_1605cc);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_16060c);
	++wram_.sub_state();
}

// The key rises to y $80 (slowly: every other frame).
void Credits::wind_key(bool slowly)
{
	animate_music_box();
	draw_music_box();
	MenuObject key = winding_key();
	if (key.y() != 0x80 && !(slowly && (wram_.global_counter() & 0x01)))
		--key.y();
	key.animate();
	key.load_sprite(ADDR_OAM_1617e8, false);
	routines_.clear_unused_virtual_oam();

	if (!slowly)
		return;
	// Func_160318: the screen fades in
	if (wram_.global_counter() & 0x0f)
		return;
	routines_.call(ADDR_SlowFadeInScreen);
	wram_.timer() = 180;
}

// After a perfect game, any button or the timer goes on to the "Perfect"
// screen.
void Credits::last_state()
{
	wind_key(false);
	if (wram_.transition_param() != TRANSITION_EPILOGUE_PERFECT)
		return;
	if (wram_.joypad_pressed() || !--wram_.timer())
		routines_.call(ADDR_SetPerfectState);
}

} // namespace wl3::screens

// Called from CreditsStateTable (see src/ported/engine/credits/common.c).
PORT_EX(_CreditsStateTable, 0, ALL_REGISTERS)
{
	wl3::screens::Credits(gb).run_state();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitCredits);
PORT_COVERED(ShowCredits);
PORT_COVERED(Func_160190);
PORT_COVERED(Func_16026c);
PORT_COVERED(Func_1602ee);
PORT_COVERED(Func_160318);
PORT_COVERED(Func_160365);
PORT_COVERED(LoadCreditsPals);
PORT_COVERED(Func_1603be);
PORT_COVERED(Func_1603cb);
PORT_COVERED(LoadCreditsGfx);
PORT_COVERED(Func_16041f);
PORT_COVERED(VBlank_160439);
PORT_COVERED(LoadCreditsSprite);
