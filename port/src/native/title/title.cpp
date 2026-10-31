// The title screen; see title.hpp.
#include "title.hpp"

#include <initializer_list>

namespace wl3::title {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;
constexpr uint8_t GFX_RAM = 0x03; // BANK("GFX RAM")
constexpr uint16_t VBLANK_TITLE_FUNC = ADDR_VBlank_Title + 0x0c; // VBlank_Title.Func
constexpr uint8_t VBLANK_TITLE_FUNC_SIZE = 0x4c;

static_assert(wPlaneWarioVar == wPlaneWario + 8);
static_assert(wPlaneWarioAnimationEnded == wPlaneWario + 11);

} // namespace

void Screen::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// ---------------------------------------------------------------------------
// The state table

void Title::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init_intro(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: intro_phase1(); break;
	case 4: intro_phase2(); break;
	case 5: intro_phase3(); break;
	case 6: end_intro(); break;
	case SST_START_MENU: start_menu(); break;
	case SST_START_MENU + 1: routines_.call(ADDR_SlowFadeBGToWhite); break;
	case SST_START_MENU + 2: init_time_attack_description(); break;
	case SST_START_MENU + 3: time_attack_description(); break;
	case 11: case 12: case 13: case 14: case 15: case 16:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void Title::start_overworld()
{
	wram_.state() = ST_OVERWORLD;
	wram_.sub_state() = 0;
}

void Title::start_overworld_new_game()
{
	wram_.transition_param() = TRANSITION_NEW_GAME;
	start_overworld();
}

void Title::return_from_action_help()
{
	wram_.animated_tiles_frame_duration() = 0;
	wram_.animated_tiles_gfx() = 0;
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.unk_3d50d() = 0;
	wram_.day_night().set_bit(7);
	start_overworld();
}

// ---------------------------------------------------------------------------
// The intro

void Title::init_intro()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	load_title_pals();
	load_title_tiles();
	load_title_tilemap();
	set_vblank();

	wram_.scy() = 0x04;
	ram(rSCY) = 0x04;
	wram_.scx() = 0;
	ram(rSCX) = 0;

	if (!wram_.unk_ceef()) {
		init_plane(64, 208, ADDR_Frameset_712c);
		plane().animate();
		MenuObject obj1 = object(wMenuObj1);
		obj1.init(128, 80, ADDR_Frameset_6ce6);
		obj1.animate();
		MenuObject obj4 = object(wMenuObj4);
		obj4.init(16, 0, ADDR_Frameset_71a1);
		obj4.animate();
		MenuObject obj5 = object(wMenuObj5);
		obj5.init(8, 32, ADDR_Frameset_71a4);
		obj5.animate();
		load_intro_sprite(obj4);
		load_intro_sprite(obj5);
		wram_.start_menu_selection() = STARTMENU_NEW_GAME;
	} else {
		// the intro was seen: straight to the menu, the plane crashed
		init_plane(108, 128, ADDR_Frameset_7198);
		set_menu_priority();

		bool time_attack = wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F);
		MenuObject obj1 = object(wMenuObj1);
		obj1.init(0x82, 0x50, time_attack ? ADDR_Frameset_6d1e : ADDR_Frameset_6ce9);
		obj1.animate();
		MenuObject obj2 = object(wMenuObj2);
		obj2.init(0x86, time_attack ? 0x40 : 0x44, ADDR_Frameset_6d1b);
		obj2.animate();
		wram_.start_menu_selection() = STARTMENU_CONTINUE;
	}

	MenuObject obj3 = object(wMenuObj3);
	obj3.init(36, 104, ADDR_Frameset_6d18);
	obj3.animate();

	ram(rLCDC) = LCDC_DEFAULT;
	wram_.timer() = 0;
	wram_.global_counter() = 0;
	++wram_.sub_state();
}

void Title::init_plane(uint8_t y, uint8_t x, uint16_t frameset)
{
	plane().init(y, x, frameset);
	plane().var() = 0;
	wram_.sfx_timer() = 0;
}

// Sets the priority flag on the menu options' rows, to hide them behind the
// scenery, and the darker tone behind them.
void Title::set_menu_priority()
{
	ram(rVBK) = VRAM1;
	for (int row : {12, 13, 16, 17}) {
		uint16_t addr = static_cast<uint16_t>(v0BGMap0 + row * TILEMAP_WIDTH + 4);
		for (int i = 0; i < 11; i++)
			ram(static_cast<uint16_t>(addr + i)) |= 0x80;
	}
	ram(rVBK) = 0;
	uint16_t box = static_cast<uint16_t>(v0BGMap0 + 14 * TILEMAP_WIDTH + 4);
	routines_.copy(ADDR_TitleScreenBoxTilemap, box, 12);
	routines_.copy(ADDR_TitleScreenBoxTilemap + 12, static_cast<uint16_t>(box + TILEMAP_WIDTH), 12);
}

void Title::intro_phase1()
{
	if (wram_.unk_ceef()) {
		start_title_screen();
		return;
	}
	play_plane_sfx_close();
	move_clouds();

	MenuObject p = plane();
	Byte timer = wram_.timer();
	switch (p.var()) {
	case 1: // flies in, slowing down
		if (p.x() >= 72) {
			p.x() -= 2;
		} else if (p.x() >= 56) {
			--p.x();
		} else if (p.x() == 48) {
			timer = 48;
			++p.var();
		} else if (!(wram_.global_counter() & 0x03)) {
			--p.x();
		}
		break;
	case 2:
		if (--timer)
			break;
		timer = 3;
		p.set_frameset(ADDR_Frameset_71a7);
		++p.var();
		break;
	case 3: // dips
		if (wram_.global_counter() & 0x07)
			break;
		if (++p.y() == 70)
			++p.var();
		break;
	case 4: // and comes back up, three times
		if (wram_.global_counter() & 0x07)
			break;
		if (--p.y() != 64)
			break;
		if (--timer) {
			--p.var();
			break;
		}
		p.set_frameset(ADDR_Frameset_712c);
		++p.var();
		break;
	case 5: // flies off to the right
		if (p.x() != 192) {
			if (p.x() < 56 && (wram_.global_counter() & 0x03))
				break;
			++p.x();
			break;
		}
		p.y() = 16;
		p.x() = 192;
		p.frame() = 0;
		p.attributes() = 0;
		p.set_frameset(ADDR_Frameset_713d);
		p.var() = 0;
		wram_.sfx_timer() = 0;
		timer = 28;
		++wram_.sub_state();
		break;
	default: // the intro music, after 3 seconds
		if (++timer < 180)
			break;
		timer = 0;
		play_music(mem_, MUSIC_INTRO);
		++p.var();
		break;
	}

	scroll_layers();
	p.animate();
	draw_intro(p, object(wMenuObj4), object(wMenuObj5));
}

void Title::intro_phase2()
{
	move_clouds();
	play_plane_sfx_far();

	MenuObject p = plane();
	Byte timer = wram_.timer();
	switch (p.var()) {
	case 1:
		if (--timer)
			break;
		p.set_frameset(ADDR_Frameset_7144);
		wram_.sfx_timer() = 0;
		timer = 224;
		++p.var();
		break;
	case 2: // sputters, then explodes
		if (p.frameset_offset() == 0x18 && p.duration() == 0)
			play_sfx(mem_, SFX_023);
		if (--timer)
			break;
		play_sfx(mem_, SFX_PLANE_EXPLOSION);
		p.set_frameset(ADDR_Frameset_7165);
		p.var() = 0;
		wram_.sfx_timer() = 0;
		timer = 48;
		++wram_.sub_state();
		break;
	default: // waits, then moves to the centre
		if (timer) {
			--timer;
			break;
		}
		if (p.x() == 64) {
			timer = 48;
			++p.var();
			break;
		}
		if (wram_.global_counter() & (p.x() < 72 ? 0x03 : 0x01))
			break;
		--p.x();
		break;
	}

	scroll_layers();
	p.animate();
	draw_intro(object(wMenuObj4), p, object(wMenuObj5));
}

void Title::intro_phase3()
{
	move_clouds();

	MenuObject p = plane();
	bool landed = false;
	switch (p.var()) {
	case 1:
		if (!phase3_fall(16))
			break;
		p.set_frameset(ADDR_Frameset_717b);
		++p.var();
		break;
	case 2:
		if (!phase3_fall(48))
			break;
		p.set_frameset(ADDR_Frameset_7184);
		++p.var();
		break;
	case 3:
		if (!phase3_fall(112))
			break;
		play_sfx(mem_, SFX_061);
		p.set_frameset(ADDR_Frameset_718d);
		++p.var();
		break;
	case 4: // crashed: the layers line up, and the title screen comes
		if (wram_.intro_bgx_offset_near() != 224)
			p.x() += 2;
		landed = true;
		if (!p.animation_ended())
			break;
		p.set_frameset(ADDR_Frameset_7198);
		p.var() = 0;
		wram_.timer() = 0;
		routines_.load_pals_to_temp_pals1(ADDR_Pals_5002);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_5042);
		play_music(mem_, MUSIC_TITLE_SCREEN);
		++wram_.sub_state();
		break;
	default: // after the explosion, the nosedive
		if (!p.animation_ended())
			break;
		play_sfx(mem_, SFX_119);
		p.set_frameset(ADDR_Frameset_7172);
		++p.var();
		break;
	}

	if (landed)
		finish_scroll();
	else
		scroll_layers();
	p.animation_ended() = p.animate();
	draw_intro(object(wMenuObj4), p, object(wMenuObj5));
}

bool Title::phase3_fall(uint8_t y)
{
	MenuObject p = plane();
	if (!(wram_.global_counter() & 0x03))
		++p.x();
	++p.y();
	return p.y() >= y;
}

void Title::end_intro()
{
	plane().animate();
	load_intro_sprite(plane());
	routines_.clear_unused_virtual_oam();

	if (wram_.unk_ceef()) {
		start_title_screen();
		return;
	}
	Byte timer = wram_.timer();
	if (timer < 120) {
		++timer;
		return;
	}
	if (!(wram_.global_counter() & 0x03))
		routines_.call(ADDR_SlowFadeInScreen);
	if (wram_.sub_state() == SST_START_MENU)
		start_title_screen();
}

void Title::start_title_screen()
{
	play_music(mem_, MUSIC_TITLE_SCREEN);
	wram_.timer().word() = ACTION_HELP_TIMER;
	wram_.sub_state() = SST_START_MENU;
}

void Title::draw_intro(MenuObject first, MenuObject second, MenuObject third)
{
	load_intro_sprite(first);
	load_intro_sprite(second);
	load_intro_sprite(third);
	routines_.clear_unused_virtual_oam();
	routines_.call(ADDR_PanIntroBackgroundLayers);
}

// ---------------------------------------------------------------------------
// Graphics

void Title::load_title_pals()
{
	if (!wram_.unk_ceef()) {
		routines_.load_pals_to_temp_pals1(ADDR_Pals_4f82);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_4fc2);
	} else {
		routines_.load_pals_to_temp_pals1(ADDR_Pals_5002);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_5042);
	}
}

void Title::load_title_tiles()
{
	ram(rVBK) = VRAM1;
	routines_.decompress(ADDR_PlaneGfx, v1Tiles0);
	ram(rVBK) = 0;
	routines_.decompress(ADDR_TitleScreenGfx, v0Tiles0);
}

void Title::load_title_tilemap()
{
	ram(rVBK) = VRAM1;
	routines_.decompress(ADDR_TitleScreenAttrmap, v1BGMap0);
	ram(rVBK) = 0;
	routines_.decompress(ADDR_TitleScreenTilemap, v0BGMap0);
}

void Title::load_time_attack_tiles()
{
	ram(rVBK) = VRAM1;
	routines_.farcall(BANK_LoadFontTiles, ADDR_LoadFontTiles);
	ram(rVBK) = 0;
	routines_.copy_bc(ADDR_PrologueBackgroundGfx, v0Tiles2, 0x80 * TILE_SIZE, BANK_PrologueBackgroundGfx);
}

void Title::load_time_attack_text()
{
	bool english = wram_.language();
	ram(rVBK) = VRAM1;
	routines_.decompress(english ? ADDR_TimeAttack2ENTextMap : ADDR_TimeAttack2JPTextMap, v1BGMap0);
	ram(rVBK) = 0;
	routines_.decompress(english ? ADDR_TimeAttack1ENTextMap : ADDR_TimeAttack1JPTextMap, v0BGMap0);
}

void Title::set_vblank()
{
	routines_.copy(VBLANK_TITLE_FUNC, wVBlankFunc, VBLANK_TITLE_FUNC_SIZE);
}

void Title::load_intro_sprite(MenuObject obj)
{
	obj.load_sprite(ADDR_OAM_6d21);
}

void Title::load_start_menu_sprite(MenuObject obj)
{
	obj.load_sprite(ADDR_OAM_6b5f);
}

// ---------------------------------------------------------------------------
// The background layers, the clouds and the plane's sound

// The far layer every 4 frames, the centre one by 1, the near one by 2.
void Title::scroll_layers()
{
	if (!(wram_.global_counter() & 0x03))
		--wram_.intro_bgx_offset_far();
	--wram_.intro_bgx_offset_centre();
	wram_.intro_bgx_offset_near() -= 2;
}

// Scrolls the layers on until they line up with the centre.
void Title::finish_scroll()
{
	if (!(wram_.global_counter() & 0x03))
		align_layer(wIntroBGXOffsetFar);
	align_layer(wIntroBGXOffsetCentre);
	if (wram_.intro_bgx_offset_near() != 224)
		wram_.intro_bgx_offset_near() -= 2;
}

// by 1 until it's aligned to 32 pixels, then to wSCX
void Title::align_layer(uint16_t layer)
{
	if (ram(layer) & 0x1f)
		--ram(layer);
	else
		ram(layer) = wram_.scx();
}

void Title::move_clouds()
{
	if (wram_.global_counter() & 0x01)
		move_cloud(object(wMenuObj5).x());
	move_cloud(object(wMenuObj4).x());
}

// Moves a cloud right, until it's off screen once the plane has gone.
void Title::move_cloud(Byte x)
{
	if (x >= 176) {
		uint8_t state = wram_.sub_state();
		if (state >= 5)
			return;
		if (state >= 4 && plane().var())
			return;
	}
	++x;
}

void Title::play_plane_sfx_close()
{
	uint8_t x = plane().x();
	if (x >= SCREEN_WIDTH_PX)
		play_plane_sfx(SFX_PLANE3);
	else
		play_plane_sfx(x < 112 ? SFX_PLANE1 : SFX_PLANE2);
}

void Title::play_plane_sfx_far()
{
	play_plane_sfx(plane().x() < SCREEN_WIDTH_PX ? SFX_PLANE2 : SFX_PLANE3);
}

// play_sfx_rept 12, sfx
void Title::play_plane_sfx(uint16_t sfx)
{
	Byte timer = wram_.sfx_timer();
	bool due = timer == 0;
	--timer;
	if (!due)
		return;
	timer = 12;
	play_sfx(mem_, sfx);
}

// ---------------------------------------------------------------------------
// The start menu

void Title::start_menu()
{
	plane().animate();
	load_intro_sprite(plane());
	load_start_menu_sprite(object(wMenuObj3));
	handle_selection();
	load_start_menu_sprite(object(wMenuObj1));
	if (wram_.unk_ceef()) {
		MenuObject obj2 = object(wMenuObj2);
		if (!(wram_.global_counter() & 0x07))
			obj2.attributes() ^= 0x03;
		load_start_menu_sprite(obj2);
	}
	routines_.clear_unused_virtual_oam();

	uint8_t selection = wram_.start_menu_selection();
	if (selection & STARTMENUF_SELECT) {
		select_menu_option();
		return;
	}
	Word timer = wram_.timer().word();
	if (selection != STARTMENU_NEW_GAME && selection != STARTMENU_CONTINUE) {
		timer = ACTION_HELP_TIMER;
		return;
	}
	// left alone long enough, the Action Help demo starts
	timer = static_cast<uint16_t>(timer - 1);
	if (timer)
		return;
	wram_.power_up_level() |= ACTION_HELP_TITLE_SCREEN;
	routines_.call(ADDR_OpenActionHelp);
}

void Title::select_menu_option()
{
	MenuObject obj1 = object(wMenuObj1);
	MenuObject obj2 = object(wMenuObj2);
	uint16_t frameset;
	uint8_t selection;
	switch (wram_.start_menu_selection() & 0x0f) {
	case STARTMENU_NEW_GAME:
		wram_.state() = ST_LANGUAGE_SELECTION;
		wram_.sub_state() = 0;
		return;
	case STARTMENU_CONTINUE:
		if (wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F)) {
			routines_.disable_lcd();
			routines_.call(ADDR_FillWhiteBGPal);
			routines_.call(ADDR_FillWhiteOBPal);
			ram(rLCDC) = LCDC_DEFAULT;
			++wram_.sub_state();
		} else if (!(wram_.unk_ceef() & 0x3c)) {
			start_overworld();
		} else {
			wram_.state() = ST_LEVEL;
			wram_.sub_state() = 0;
		}
		return;
	case STARTMENU_CLEAR_DATA: // asks to confirm
		wram_.unk_cee4() = 0x81;
		obj2.y() = SCREEN_HEIGHT_PX;
		obj1.y() = 120;
		frameset = ADDR_Frameset_6cec;
		selection = STARTMENU_CLEAR_DATA_NO;
		break;
	case STARTMENU_CLEAR_DATA_YES:
		routines_.call(ADDR_ClearSaveData); // and resets
		return;
	default: // no: back to the menu
		obj2.y() = 134;
		obj1.y() = 130;
		if (wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F)) {
			frameset = ADDR_Frameset_6d1e;
		} else {
			wram_.unk_cee4() = 0x80;
			frameset = ADDR_Frameset_6ce9;
		}
		selection = STARTMENU_CONTINUE;
		break;
	}
	set_selection(selection, frameset);
}

void Title::set_selection(uint8_t selection, uint16_t frameset)
{
	wram_.start_menu_selection() = selection;
	MenuObject obj1 = object(wMenuObj1);
	obj1.set_frameset(frameset);
	obj1.animate();
}

void Title::handle_selection()
{
	uint8_t selection = wram_.start_menu_selection();
	if (selection & STARTMENUF_SCROLLING)
		scroll_selection();
	else if (selection & STARTMENUF_ANIM1) {
		if (object(wMenuObj1).animate())
			set_select_flag();
	} else if (selection & STARTMENUF_ANIM2)
		blink_selection();
	else
		handle_selection_input();
}

void Title::handle_selection_input()
{
	uint8_t pressed = wram_.joypad_pressed();
	Byte selection = wram_.start_menu_selection();
	if (pressed & PAD_A) {
		play_sfx(mem_, SFX_SELECTION);
		switch (selection & 0x0f) {
		case STARTMENU_CLEAR_DATA_NO:
			set_selection(0x23, ADDR_Frameset_6cf2);
			break;
		case STARTMENU_CLEAR_DATA_YES:
			set_selection(0x24, ADDR_Frameset_6d05);
			break;
		default:
			selection |= STARTMENUF_ANIM2;
			object(wMenuObj1).frameset_offset() = 0;
			break;
		}
	} else if (pressed & PAD_DOWN) {
		if (selection != STARTMENU_CONTINUE)
			return;
		selection = STARTMENU_CLEAR_DATA | STARTMENUF_SCROLLING;
		play_sfx(mem_, SFX_0E2);
	} else if (pressed & PAD_UP) {
		if (selection != STARTMENU_CLEAR_DATA)
			return;
		selection = STARTMENU_CONTINUE | STARTMENUF_SCROLLING;
		play_sfx(mem_, SFX_0E2);
	} else if (pressed & PAD_RIGHT) {
		if (selection != STARTMENU_CLEAR_DATA_YES)
			return;
		play_sfx(mem_, SFX_0E2);
		set_selection(STARTMENU_CLEAR_DATA_NO, ADDR_Frameset_6cec);
	} else if (pressed & PAD_LEFT) {
		if (selection != STARTMENU_CLEAR_DATA_NO)
			return;
		play_sfx(mem_, SFX_0E2);
		set_selection(STARTMENU_CLEAR_DATA_YES, ADDR_Frameset_6cef);
	}
}

// Moves the cursor between Continue and Clear Data.
void Title::scroll_selection()
{
	Byte y = object(wMenuObj1).y();
	uint8_t obj2_y;
	if (wram_.start_menu_selection() != (STARTMENU_CLEAR_DATA | STARTMENUF_SCROLLING)) {
		if (++y != 130)
			return;
		obj2_y = 134;
	} else {
		if (--y != 116)
			return;
		obj2_y = 118;
	}
	object(wMenuObj2).y() = obj2_y;
	wram_.start_menu_selection() &= ~STARTMENUF_SCROLLING;
}

// The chosen option blinks through palettes, every other frame.
void Title::blink_selection()
{
	if (wram_.global_counter() & 0x01)
		return;
	MenuObject obj1 = object(wMenuObj1);
	uint8_t step = obj1.frameset_offset();
	if (step == 0x10) {
		set_select_flag();
		return;
	}
	obj1.frameset_offset() = static_cast<uint8_t>(step + 1);
	static constexpr uint8_t ATTRIBUTES[4] = {0x03, 0x00, 0x01, 0x00};
	obj1.attributes() = ATTRIBUTES[step & 0x03];
}

void Title::set_select_flag()
{
	wram_.start_menu_selection() |= STARTMENUF_SELECT;
	object(wMenuObj1).frameset_offset() = 0;
}

// ---------------------------------------------------------------------------
// The time attack description

void Title::init_time_attack_description()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	routines_.load_pals_to_temp_pals1(ADDR_Pals_71b0);
	routines_.call(ADDR_ApplyTempPals1ToBGPals);
	load_time_attack_tiles();
	load_time_attack_text();
	routines_.call(ADDR_VBlank_TimeAttackDescription);

	wram_.scy() = 0;
	ram(rSCY) = 0;
	wram_.scx() = 0;
	ram(rSCX) = 0;
	ram(rLCDC) = LCDC_DEFAULT;
	wram_.timer() = 0;
	++wram_.sub_state();
}

void Title::time_attack_description()
{
	routines_.call(ADDR_AdjustJapaneseTimeAttackDescriptionWindow);
	if (!(wram_.joypad_pressed() & PAD_A))
		return;
	routines_.disable_lcd();
	routines_.call(ADDR_FillWhiteBGPal);
	ram(rLCDC) = LCDC_DEFAULT;
	start_overworld();
}

} // namespace wl3::title

using wl3::title::Title;

// Its states leave bc, de and hl as they end, and MainStateTable returns to
// the main loop, where none is live. a and f are push_wram's, as pop_wram
// leaves them.
PORT_EX(TitleStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	wl3::Memory mem(gb);
	uint8_t flags = gb->cpu.f;
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = wl3::title::GFX_RAM;
	Title(gb).run_state();
	mem[rWBK] = wram_bank;
	gb->cpu.a = wram_bank;
	gb->cpu.f = flags;
}

// These end in xor a (a = 0, only Z set); Func_4628 leaves hl at wDayNight.
PORT(StartOverworldState)
{
	Title(gb).start_overworld();
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

PORT(StartOverworldStateAndNewGameParam)
{
	Title(gb).start_overworld_new_game();
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

PORT(Func_4628)
{
	Title(gb).return_from_action_help();
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	SET_HL(gb, wDayNight);
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(StartTitleScreen);
PORT_COVERED(IntroSequencePhase1);
PORT_COVERED(IntroSequencePhase2);
PORT_COVERED(IntroSequencePhase3);
PORT_COVERED(EndIntroSequence);
PORT_COVERED(InitIntroSequence);
PORT_COVERED(ScrollIntroBackgroundLayers);
PORT_COVERED(FinishIntroBackgroundScroll);
PORT_COVERED(Func_4d45);
PORT_COVERED(LoadTitleScreenPals);
PORT_COVERED(LoadTitleScreenTiles);
PORT_COVERED(LoadTitleScreenTileMap);
PORT_COVERED(LoadTimeAttackDescriptionPals);
PORT_COVERED(LoadTimeAttackDescriptionTiles);
PORT_COVERED(LoadTimeAttackText);
PORT_COVERED(LoadIntroSprite);
PORT_COVERED(LoadStartMenuSprite);
PORT_COVERED(PlayIntroSFXPlane2);
PORT_COVERED(PlayIntroSFXPlane3);
PORT_COVERED(PlayIntroSFXPlane_Close);
PORT_COVERED(PlayIntroSFXPlane_Far);
PORT_COVERED(StartMenu);
PORT_COVERED(HandleStartMenuSelection);
PORT_COVERED(InitTimeAttackDescription);
PORT_COVERED(TimeAttackDescription);
PORT_COVERED(VBlank_Title);
