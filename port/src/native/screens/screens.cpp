// The small screens; see screens.hpp.
#include "screens.hpp"

namespace wl3::screens {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;
constexpr uint8_t GFX_RAM = 0x03; // BANK("GFX RAM")
constexpr uint16_t VBLANK_LANGUAGE_FUNC = ADDR_VBlank_LanguageSelection + 0x0c; // .Func
constexpr uint8_t VBLANK_LANGUAGE_FUNC_SIZE = 0x53;

} // namespace

void Screen::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// ---------------------------------------------------------------------------
// The GB incompatible notice

void GBIncompatible::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: break; // ExitGBIncompatible: it stays
	case 4: case 5: case 6: case 7: routines_.call(ADDR_DebugReset); break;
	default: unrealistic(); break;
	}
}

void GBIncompatible::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_ClearBGMap0);
	routines_.clear_virtual_oam();
	routines_.farcall(BANK_LoadGBIncompatibleScreenGfx, ADDR_LoadGBIncompatibleScreenGfx);
	routines_.farcall(BANK_LoadGBIncompatibleScreenMap, ADDR_LoadGBIncompatibleScreenMap);
	routines_.call(ADDR_VBlank_GBIncompatible);

	wram_.scx() = 0;
	mem_[rSCX] = 0;
	wram_.scy() = 0;
	mem_[rSCY] = 0;
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// ---------------------------------------------------------------------------
// The language selection

void LanguageSelection::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: update(); break;
	case 4: case 5: case 6: case 7: case 8: case 9: routines_.call(ADDR_DebugReset); break;
	default: unrealistic(); break;
	}
}

void LanguageSelection::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	routines_.farcall(BANK_LoadFontPals, ADDR_LoadFontPals);
	routines_.farcall(BANK_LoadFontTiles, ADDR_LoadFontTiles);
	routines_.farcall(BANK_LoadLanguageSelectionText, ADDR_LoadLanguageSelectionText);
	routines_.copy(VBLANK_LANGUAGE_FUNC, wVBlankFunc, VBLANK_LANGUAGE_FUNC_SIZE);

	wram_.scy() = 0;
	mem_[rSCY] = 0;
	wram_.scx() = 0;
	mem_[rSCX] = 0;
	wram_.language() = ENGLISH;

	MenuObject cursor = object(wMenuObj1);
	cursor.init(70, 24, ADDR_Frameset_76b5);
	cursor.animate();
	cursor.load_sprite(ADDR_OAM_75c3);
	routines_.clear_unused_virtual_oam();
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// A starts a new game; up and down move between the languages.
void LanguageSelection::update()
{
	Byte pressed = wram_.joypad_pressed();
	if (pressed.bit(B_PAD_A)) {
		routines_.call(ADDR_StartOverworldStateAndNewGameParam);
		return;
	}
	if (pressed.bit(B_PAD_DOWN)) {
		if (!wram_.language())
			return; // can't go down
		choose(0, 86);
	} else if (pressed.bit(B_PAD_UP)) {
		if (wram_.language() == 1)
			return; // can't go up
		choose(1, 70);
	}
	draw_cursor();
}

void LanguageSelection::choose(uint8_t language, uint8_t cursor_y)
{
	object(wMenuObj1).y() = cursor_y;
	wram_.language() = language;
	wram_.unk_cee4() = static_cast<uint8_t>(language + 0x80); // the V-Blank function recolours
	play_sfx(mem_, SFX_0E2);
}

void LanguageSelection::draw_cursor()
{
	MenuObject cursor = object(wMenuObj1);
	cursor.animate();
	cursor.load_sprite(ADDR_OAM_75c3);
	routines_.clear_unused_virtual_oam();
}

// ---------------------------------------------------------------------------
// Perfect

void Perfect::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_SlowFadeBGToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_DarkenBGToPal_Fast); break;
	case 3: update(); break;
	case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void Perfect::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	routines_.load_pals_to_temp_pals1(ADDR_Pals_dc173);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_dc1fb);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_PerfectGfx, v0Tiles0);
	mem_[rVBK] = VRAM1;
	routines_.decompress(ADDR_PerfectAttrmap, v0BGMap0);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_PerfectTilemap, v0BGMap0);
	routines_.call(ADDR_VBlank_Perfect);

	mem_[rSCX] = 0;
	wram_.scx() = 0;
	mem_[rSCY] = 0;
	wram_.scy() = 0;

	MenuObject text = object(wMenuObj1);
	text.init(0x23, 0x2b, ADDR_Frameset_dd848);
	text.animate();
	draw_sprite();
	play_music(mem_, MUSIC_PERFECT);

	mem_[rLCDC] = LCDC_DEFAULT;
	wram_.timer() = 0;
	wram_.game_over_wario_var() = 0;
	++wram_.sub_state();
}

void Perfect::update()
{
	object(wMenuObj1).animate();
	draw_sprite();
	routines_.clear_unused_virtual_oam();

	Byte timer = wram_.timer();
	if ((!timer || !--timer) && !(wram_.global_counter() & 0x07)) {
		routines_.call(ADDR_SlowFadeInScreen);
		if (wram_.sub_state() != 3) {
			wram_.sub_state() = 3;
			next_colours();
		}
	}

	// any button: on to the pause menu, in time attack mode
	if (wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F) || !wram_.joypad_pressed())
		return;
	wram_.fade_speed() = 0;
	wram_.pal_fade_counter() = 0;
	wram_.unused_c186() = 0;
	wram_.game_mode_flags().set_bit(MODE_TIME_ATTACK_F);
	wram_.state() = ST_PAUSE_MENU;
	wram_.sub_state() = SST_PAUSE_28;
}

void Perfect::next_colours()
{
	Byte step = wram_.game_over_wario_var();
	switch (++step) {
	case 1:
		wram_.timer() = 240;
		set_colours(ADDR_Pals_dc1b3);
		break;
	case 2: set_colours(ADDR_Pals_dc1cb); break;
	case 3: set_colours(ADDR_Pals_dc1e3); break;
	case 4: set_colours(ADDR_Pals_dc1cb); break;
	case 5: set_colours(ADDR_Pals_dc1b3); break;
	default:
		step = 0;
		set_colours(ADDR_Pals_dc173 + 5 * PAL_SIZE);
		break;
	}
}

// the last three palettes of wTempPals1
void Perfect::set_colours(uint16_t pals)
{
	routines_.copy_short(pals, wTempPals1 + 5 * PAL_SIZE, 3 * PAL_SIZE);
}

void Perfect::draw_sprite()
{
	object(wMenuObj1).load_sprite(ADDR_OAM_dd710, false);
}

} // namespace wl3::screens

// Called from MainStateTable, which returns to the main loop: it reloads
// every register.
PORT_EX(GBIncompatibleStateTable, 0, ALL_REGISTERS)
{
	wl3::screens::GBIncompatible(gb).run_state();
}

PORT_EX(LanguageSelectionStateTable, 0, ALL_REGISTERS)
{
	wl3::Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = wl3::screens::GFX_RAM;
	wl3::screens::LanguageSelection(gb).run_state();
	mem[rWBK] = wram_bank;
}

// Called from PerfectStateTable (see src/ported/engine/perfect/common.c).
PORT_EX(_PerfectStateTable, 0, ALL_REGISTERS)
{
	wl3::screens::Perfect(gb).run_state();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(GBIncompatible);
PORT_COVERED(ExitGBIncompatible);
PORT_COVERED(InitLanguageSelection);
PORT_COVERED(LanguageSelection);
PORT_COVERED(LoadLanguageSelectionSprite);
PORT_COVERED(VBlank_LanguageSelection);
PORT_COVERED(InitPerfectScreen);
PORT_COVERED(UpdatePerfectScreen);
PORT_COVERED(LoadPerfectPals);
PORT_COVERED(LoadPerfectGfx);
PORT_COVERED(Func_dc101);
PORT_COVERED(Func_dc10d);
PORT_COVERED(Func_dc119);
PORT_COVERED(Func_dc125);
PORT_COVERED(Func_dc13e);
PORT_COVERED(Func_dc158);
