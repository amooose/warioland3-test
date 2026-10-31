// The golf building, lobby and clear cutscene; see golf.hpp.
#include "golf.hpp"

#include <initializer_list>

namespace wl3::golf {

namespace {

// tables in the routines' own code (local labels, not in syms.h)
constexpr uint16_t COURSE_DATA = ADDR_Func_1c87db + 0x2d;            // Func_1c87db.data
constexpr uint16_t COURSE_SCROLL_X = ADDR_GetGolfCourseTargetSCX + 0x0f; // .scroll_x
constexpr uint16_t FIRST_GOLF_HOLES = ADDR_Func_1c8911 + 0x42;        // .first_golf_holes
constexpr uint16_t COURSE_WARIO_FRAMESETS = ADDR_Func_1c86f8 + 0xdb;  // .framesets

constexpr uint16_t GOLF_RAM_START = 0xd800; // STARTOF("Golf RAM")
constexpr uint16_t GOLF_RAM_END = 0xe000;
constexpr uint8_t VRAM1 = BANK_v1Tiles0;

} // namespace

// ---------------------------------------------------------------------------
// The state tables

void Golf::run_golf_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init_lobby(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: handle_lobby(); break;
	case SST_GOLF_HOLE: routines_.call(ADDR_FastFadeToWhite); break;
	case SST_GOLF_HOLE + 1: GolfHole(gb_).init(); break;
	case SST_GOLF_HOLE + 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case SST_GOLF_HOLE + 3: GolfHole(gb_).handle(); break;
	case SST_GOLF_EXIT: routines_.call(ADDR_FastFadeToWhite); break;
	case SST_GOLF_EXIT + 1: routines_.call(ADDR_ReturnToLevelFromGolf); break;
	case SST_GOLF_CLEAR: routines_.call(ADDR_FastFadeToWhite); break;
	case SST_GOLF_CLEAR + 1: init_clear_cutscene(); break;
	case SST_GOLF_CLEAR + 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case SST_GOLF_CLEAR + 3: update_clear_cutscene(); break;
	default: unrealistic(); break;
	}
}

void Golf::run_building_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init_building(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: handle_building(); break;
	case 4: routines_.call(ADDR_FastFadeToWhite); break;
	case 5: GolfHole(gb_).init(); break;
	case 6: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 7: GolfHole(gb_).handle(); break;
	case 8: routines_.call(ADDR_ReturnToMap); break;
	default: unrealistic(); break;
	}
}

// A state outside a jump table: the original jumps to whatever follows it.
void GolfScreen::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// ---------------------------------------------------------------------------
// Shared

// wGolfOAMPtr is stored high byte first
void GolfScreen::set_oam_pointer(uint16_t oam)
{
	ram(wGolfOAMPtr + 0) = static_cast<uint8_t>(oam >> 8);
	ram(wGolfOAMPtr + 1) = static_cast<uint8_t>(oam);
}

void GolfScreen::update_object(GolfObject obj, uint8_t y, uint8_t x, uint16_t frameset)
{
	obj.y() = y;
	obj.x() = x;
	routines_.update_golf_object_animation(frameset, obj.duration().addr());
	load_object_sprite(obj);
}

void GolfScreen::load_object_sprite(GolfObject obj)
{
	obj.frame() = obj.current_frame();
	routines_.load_golf_sprite(obj.sprite());
}

void GolfScreen::clear_golf_ram_from(uint16_t start)
{
	for (uint16_t addr = start; addr < GOLF_RAM_END; addr++)
		ram(addr) = 0;
}

void GolfScreen::reset_scroll()
{
	ram(rSCY) = 0;
	ram(rSCX) = 0;
	wram_.scy() = 0;
	wram_.scx() = 0;
}

// ---------------------------------------------------------------------------
// The lobby

void Golf::init_lobby()
{
	routines_.disable_lcd();
	routines_.clear_virtual_oam();
	if (!wram_.unk_1d800())
		routines_.save_backup_vram();
	wram_.unk_1d800() = 0x02;
	wram_.room_animated_tiles_enabled() = 0;
	clear_golf_ram_from(wPredeterminedGolfHole);
	routines_.load_pals_to_temp_pals1(ADDR_Pals_1ca1cf);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_1ca20f);
	load_lobby_gfx();

	if (!wram_.golf_menu_option()) {
		wram_.golf_wario_state() = GOLF_WARIO_WAITING;
		wram_.golf_wario_y_pos() = SCREEN_HEIGHT_PX;
		wram_.golf_wario_x_pos() = 0x38;
	} else {
		// back from a hole: coming out of the pipe
		wram_.golf_wario_state() = GOLF_WARIO_GOING_IN_PIPE;
		wram_.golf_wario_y_pos() = 0xa0;
		wram_.golf_wario_x_pos() = 0x80;
		wram_.golf_lobby_state() = GOLFLOBBYSTATE_EXIT_PIPE;
	}
	handle_lobby_wario_sprite();

	wram_.golf_menu_option() = 0;
	ram(wGolfNumCoins + 0) = ram(wNumCoins + 0);
	ram(wGolfNumCoins + 1) = ram(wNumCoins + 1);
	if (!ram(wNumCoins + 0) && ram(wNumCoins + 1) < wram_.golf_price())
		object(wGolfObj4).state() = COININFO_NOT_ENOUGH_COINS;

	wram_.golf_vblank_mode() = 0x01;
	GolfHole(gb_).set_lobby_vblank();
	play_music2(mem_, MUSIC_GOLF_LOBBY);
	reset_scroll();
	ram(rLCDC) = LCDC_DEFAULT;
	++wram_.sub_state();
}

// Loads the lobby's gfx, and sets the price of a round from the number of
// treasures collected (shown on the sign).
void Golf::load_lobby_gfx()
{
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfLobbyGfx, ADDR_GolfLobbyGfx, v0Tiles1);
	ram(rVBK) = VRAM1;
	routines_.far_decompress(BANK_GolfLobbyGfx, ADDR_GolfLobbyAttrmap, v1BGMap0);
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfLobbyGfx, ADDR_GolfLobbyTilemap, v0BGMap0);

	uint8_t treasures = ram(wNumCollectedTreasures + 1); // BCD
	uint8_t price_tiles; // the first of the price's 4 tiles
	if (ram(wNumCollectedTreasures + 0) || treasures >= GOLF_TREASURES_TIER_3) {
		wram_.golf_price() = GOLF_PRICE_TIER_3;
		price_tiles = 0xfc;
	} else if (treasures >= GOLF_TREASURES_TIER_2) {
		wram_.golf_price() = GOLF_PRICE_TIER_2;
		price_tiles = 0xf8;
	} else {
		wram_.golf_price() = GOLF_PRICE_TIER_1; // already on the sign
		return;
	}
	ram(v0BGMap0 + 8 * TILEMAP_WIDTH + 11) = price_tiles;
	ram(v0BGMap0 + 9 * TILEMAP_WIDTH + 11) = static_cast<uint8_t>(price_tiles + 1);
	ram(v0BGMap0 + 8 * TILEMAP_WIDTH + 12) = static_cast<uint8_t>(price_tiles + 2);
	ram(v0BGMap0 + 9 * TILEMAP_WIDTH + 12) = static_cast<uint8_t>(price_tiles + 3);
}

void Golf::handle_lobby()
{
	handle_lobby_states();
	update_lobby_animations();
	handle_lobby_wario_sprite();
	routines_.clear_unused_virtual_oam();
	GolfHole(gb_).set_lobby_vblank();
}

void Golf::handle_lobby_states()
{
	switch (wram_.golf_lobby_state()) {
	case GOLFLOBBYSTATE_WAIT_INPUT: lobby_waiting_input(); break;
	case GOLFLOBBYSTATE_WALK_RIGHT: lobby_walking_right(); break;
	case GOLFLOBBYSTATE_WALK_LEFT: lobby_walking_left(); break;
	case GOLFLOBBYSTATE_ENTER_PIPE: lobby_entering_pipe(); break;
	case GOLFLOBBYSTATE_ENTER_DOOR: lobby_entering_door(); break;
	case GOLFLOBBYSTATE_EXIT_PIPE: lobby_exiting_pipe(); break;
	default: unrealistic(); break;
	}
}

void Golf::set_wario_state(uint8_t state)
{
	wram_.golf_wario_state() = state;
	wram_.golf_vblank_mode() = 0x01;
}

void Golf::lobby_wait_input()
{
	wram_.golf_counter() = 0;
	wram_.golf_wario_duration() = 0;
	wram_.golf_wario_frameset_offset() = 0;
	wram_.golf_lobby_state() = GOLFLOBBYSTATE_WAIT_INPUT;
}

// Down at the pipe pays for a round, up at the door leaves, left and right
// walk between them.
void Golf::lobby_waiting_input()
{
	uint8_t pressed = wram_.joypad_pressed();
	uint8_t x = wram_.golf_wario_x_coord();
	if (pressed & PAD_DOWN) {
		if (x != 0x78)
			return;
		GolfObject coin_info = object(wGolfObj4);
		if (coin_info.state()) {
			// not enough coins
			play_sfx(mem_, SFX_0E5);
			coin_info.state() = COININFO_SHOW_MESSAGE;
			coin_info.duration() = 0;
			coin_info.frameset_offset() = 0;
			return;
		}
		play_sfx(mem_, SFX_PIPE_ENTER);
		bool borrow;
		ram(wNumCoins + 1) = bcd_sub(ram(wNumCoins + 1), wram_.golf_price(), borrow);
		ram(wNumCoins + 0) = static_cast<uint8_t>(ram(wNumCoins + 0) - borrow);
		set_wario_state(GOLF_WARIO_GOING_IN_PIPE);
		wram_.golf_wario_x_pos() = 0x80;
		wram_.golf_counter() = 0;
		wram_.golf_wario_duration() = 0;
		wram_.golf_wario_frameset_offset() = 0;
		wram_.golf_lobby_state() = GOLFLOBBYSTATE_ENTER_PIPE;
	} else if (pressed & PAD_UP) {
		if (x != 0x38)
			return;
		play_sfx(mem_, SFX_0E6);
		wram_.golf_wario_dir() = 0;
		wram_.golf_wario_duration() = 0;
		wram_.golf_wario_frameset_offset() = 0;
		set_wario_state(GOLF_WARIO_TURNING);
		wram_.golf_lobby_state() = GOLFLOBBYSTATE_ENTER_DOOR;
	} else if (pressed & PAD_RIGHT) {
		if (x >= 0x78)
			return;
		wram_.golf_wario_dir() = 0;
		wram_.golf_wario_duration() = 0;
		wram_.golf_wario_frameset_offset() = 0;
		wram_.golf_wario_state() = GOLF_WARIO_TURNING;
		wram_.golf_wario_y_vel() = 4;
		wram_.golf_vblank_mode() = 0x01;
		wram_.golf_lobby_state() = GOLFLOBBYSTATE_WALK_RIGHT;
	} else if (pressed & PAD_LEFT) {
		if (x < 0x78)
			return;
		wram_.golf_wario_duration() = 0;
		wram_.golf_wario_frameset_offset() = 0;
		set_wario_state(GOLF_WARIO_TURNING);
		wram_.golf_wario_dir() = 0x01;
		wram_.golf_lobby_state() = GOLFLOBBYSTATE_WALK_LEFT;
	}
}

// Wario turns around before walking (wGolfCounter counts the turn).
bool Golf::lobby_turning(bool clear_dir)
{
	uint8_t counter = wram_.golf_counter();
	if (!counter)
		return false;
	if (counter >= 2)
		return true;
	// done turning, start walking
	wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
	if (clear_dir)
		wram_.golf_wario_dir() = 0;
	wram_.golf_wario_duration() = 0;
	wram_.golf_wario_frameset_offset() = 0;
	set_wario_state(GOLF_WARIO_WALKING);
	return false;
}

// to the pipe, with a jump onto it
void Golf::lobby_walking_right()
{
	if (!lobby_turning(true))
		return;
	if (wram_.golf_wario_x_pos() >= 0x58) {
		uint8_t y_vel = wram_.golf_wario_y_vel();
		uint8_t counter = static_cast<uint8_t>(wram_.golf_counter() + 1);
		wram_.golf_counter() = counter;
		if (!(counter & 1))
			--wram_.golf_wario_y_vel();
		wram_.golf_wario_y_pos() = static_cast<uint8_t>(wram_.golf_wario_y_pos() - y_vel);
		if (y_vel >= static_cast<uint8_t>(-16) && wram_.golf_wario_y_pos() >= 0x80)
			wram_.golf_wario_y_pos() = 0x80; // landed
	}
	if (wram_.golf_wario_x_pos() < 0x78) {
		++wram_.golf_wario_x_pos();
		return;
	}
	// reached the pipe
	wram_.golf_wario_y_pos() = 0x80;
	wram_.golf_wario_x_pos() = 0x78;
	set_wario_state(GOLF_WARIO_WAITING);
	lobby_wait_input();
}

// to the door, dropping off the pipe
void Golf::lobby_walking_left()
{
	if (!lobby_turning(false))
		return;
	if (wram_.golf_wario_y_pos() < SCREEN_HEIGHT_PX && wram_.golf_wario_x_pos() < 0x64)
		wram_.golf_wario_y_pos() = static_cast<uint8_t>(wram_.golf_wario_y_pos() + 2);
	if (wram_.golf_wario_x_pos() != 0x38) {
		--wram_.golf_wario_x_pos();
		return;
	}
	// reached the door
	wram_.golf_wario_y_pos() = SCREEN_HEIGHT_PX;
	wram_.golf_wario_x_pos() = 0x38;
	set_wario_state(GOLF_WARIO_WAITING);
	lobby_wait_input();
}

void Golf::lobby_entering_pipe()
{
	if (!wram_.golf_counter()) {
		++wram_.golf_counter();
		ram(wGolfNumCoins + 1) = ram(wNumCoins + 1);
		ram(wGolfNumCoins + 0) = ram(wNumCoins + 0);
	}
	if (wram_.golf_wario_y_pos() < 0xb0) {
		++wram_.golf_wario_y_pos();
		return;
	}
	wram_.sub_state() = SST_GOLF_HOLE;
}

void Golf::lobby_entering_door()
{
	uint8_t counter = wram_.golf_counter();
	if (!counter)
		return;
	if (counter == 1) {
		wram_.golf_counter() = 2;
		wram_.golf_wario_duration() = 0;
		wram_.golf_wario_frameset_offset() = 0;
		set_wario_state(GOLF_WARIO_ENTERING_DOOR);
		return;
	}
	if (counter < 28) {
		wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
		return;
	}
	wram_.unk_1d800() = 0;
	wram_.golf_menu_option() = 0;
	stop_music2(mem_);
	wram_.sub_state() = SST_GOLF_EXIT;
}

void Golf::lobby_exiting_pipe()
{
	uint8_t y = wram_.golf_wario_y_pos();
	if (y != 0x80) {
		if (y == 0xa0)
			play_sfx(mem_, SFX_PIPE_ENTER);
		wram_.golf_wario_y_pos() = static_cast<uint8_t>(y - 1);
		return;
	}
	wram_.golf_wario_state() = GOLF_WARIO_WAITING;
	wram_.golf_wario_x_pos() = 0x78;
	wram_.golf_vblank_mode() = 0x01;
	lobby_wait_input();
}

void Golf::update_lobby_animations()
{
	set_oam_pointer(ADDR_OAM_1cb2d3);
	update_object(object(wGolfObj1), 0x28, 0x24, ADDR_Frameset_1cb3b3);
	update_object(object(wGolfObj3), 0x2d, 0x8c, ADDR_Frameset_1cb3c4);
	update_object(object(wGolfObj5), 0x60, 0x28, ADDR_Frameset_1cb3d7);

	// the "not enough coins" message
	GolfObject coin_info = object(wGolfObj4);
	if (coin_info.state() != COININFO_SHOW_MESSAGE)
		return;
	coin_info.y() = 0x50;
	coin_info.x() = 0x38;
	uint16_t frameset = wram_.language() ? ADDR_Frameset_1cb3d1 : ADDR_Frameset_1cb3d4;
	if (routines_.update_golf_object_animation(frameset, coin_info.duration().addr())) {
		coin_info.state() = COININFO_NOT_ENOUGH_COINS;
		return;
	}
	coin_info.frame() = coin_info.current_frame();
	routines_.load_golf_sprite(coin_info.sprite());
}

void Golf::handle_lobby_wario_sprite()
{
	wram_.golf_wario_y_coord() = wram_.golf_wario_y_pos();
	wram_.golf_wario_x_coord() = wram_.golf_wario_x_pos();
	routines_.update_golf_wario_animation();
	GolfHole(gb_).load_wario_state_gfx();
	// a step sound at the start of each walk cycle
	if (wram_.golf_wario_state() == GOLF_WARIO_WALKING && wram_.golf_wario_frameset_offset() == 0x2 &&
		!wram_.golf_wario_duration())
		play_sfx(mem_, SFX_WALK);
}

// ---------------------------------------------------------------------------
// The clear cutscene

void Golf::init_clear_cutscene()
{
	routines_.disable_lcd();
	routines_.clear_virtual_oam();
	clear_golf_ram_from(GOLF_RAM_START);
	routines_.load_pals_to_temp_pals1(ADDR_Pals_1ca24f);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_1ca34f);

	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfMenuGfx, ADDR_GolfMenuGfx, v0Tiles0);
	ram(rVBK) = VRAM1;
	routines_.far_decompress(BANK_GolfClearTilemap, ADDR_GolfClearAttrmap, v0BGMap0);
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfClearTilemap, ADDR_GolfClearTilemap, v0BGMap0);

	object(wGolfObj1).y() = 0x70;
	update_clear_cutscene_animation();
	// VBlank_1c84e0: its V-Blank function
	routines_.copy(ADDR_VBlank_1c84e0 + 0x0b, wVBlankFunc, 0x0e);
	reset_scroll();
	ram(rLCDC) = LCDC_DEFAULT;
	++wram_.sub_state();
}

// The trophy rises, then holds.
void Golf::update_clear_cutscene()
{
	GolfObject trophy = object(wGolfObj1);
	if (trophy.state()) {
		if (wram_.golf_counter() >= 0x40) {
			wram_.sub_state() = SST_GOLF_EXIT;
			return;
		}
		++wram_.golf_counter();
	} else if (trophy.y() == 0x50) {
		wram_.golf_counter() = 0;
		trophy.frameset_offset() = 0;
		trophy.state() = 1;
	} else {
		if (!(wram_.golf_counter() & 0x0f))
			play_sfx(mem_, SFX_01F);
		++wram_.golf_counter();
		if (!(wram_.golf_counter() & 0x03))
			--trophy.y();
	}
	update_clear_cutscene_animation();
}

void Golf::update_clear_cutscene_animation()
{
	set_oam_pointer(ADDR_OAM_1cb3e0);
	GolfObject trophy = object(wGolfObj1);
	uint16_t frameset = trophy.state() ? ADDR_Frameset_1cb5cc : ADDR_Frameset_1cb5cf;
	routines_.update_golf_object_animation(frameset, trophy.duration().addr());
	trophy.frame() = trophy.current_frame();
	trophy.x() = 0x58;
	routines_.load_golf_sprite(trophy.sprite());
	routines_.clear_unused_virtual_oam();
}

// ---------------------------------------------------------------------------
// The golf building: choosing a course

void Golf::init_building()
{
	routines_.disable_lcd();
	routines_.clear_virtual_oam();
	wram_.animated_tiles_frame_duration() = 0x08;
	wram_.animated_tiles_gfx() = 0x0f;
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.room_animated_tiles_enabled() = 1;
	if (!wram_.unk_1d800())
		routines_.save_backup_vram();
	if (!wram_.golf_menu_option())
		wram_.unk_1d800() = 0x01;
	clear_golf_ram_from(wPredeterminedGolfHole);
	wram_.has_all_musical_coin_flags() = routines_.check_has_all_musical_coin_flags();

	draw_course_menu();
	routines_.load_pals_to_temp_pals1(w1db00);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_1ca34f);
	ram(wGolfNumCoins + 0) = ram(wNumCoins + 0);
	ram(wGolfNumCoins + 1) = ram(wNumCoins + 1);
	update_building_animations();
	GolfHole(gb_).set_building_vblank();
	routines_.play_new_music(MUSIC_GOLF_LOBBY);

	ram(rSCY) = 0;
	wram_.scy() = 0;
	uint8_t scx = wram_.golf_menu_target_scx();
	ram(rSCX) = scx;
	wram_.scx() = scx;
	ram(rLCDC) = LCDC_DEFAULT;
	++wram_.sub_state();
}

// The course menu, with the best score of each course.
void Golf::draw_course_menu()
{
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfMenuGfx, ADDR_GolfMenuGfx, v0Tiles0);
	ram(rVBK) = VRAM1;
	routines_.far_decompress(BANK_GolfBuildingCoursesAttrmap, ADDR_GolfBuildingCoursesAttrmap, v1BGMap0);
	wram_.golf_course() = wram_.golf_menu_option();
	load_course();
	set_course_target_scx();
	routines_.copy(wGolfCourseTileMap, v0BGMap0 + 6 * TILEMAP_WIDTH, 7 * TILEMAP_WIDTH);
	ram(rVBK) = 0;
	// (in the ROM bank load_course left in wTempBank)
	uint16_t tilemap = wram_.has_all_musical_coin_flags() ? ADDR_GolfBuildingCoursesUnlockedTilemap :
		ADDR_GolfBuildingCoursesLockedTilemap;
	SET_HL(gb_, tilemap);
	SET_BC(gb_, v0BGMap0);
	routines_.call(ADDR_FarDecompress);

	// Each score is a sign (none for 0) and two big digits, 2 tiles high,
	// in a row starting at (2, 13). Only the low byte of the position
	// changes, as in the original.
	constexpr uint16_t start = v0BGMap0 + 13 * TILEMAP_WIDTH + 2;
	uint8_t pos = static_cast<uint8_t>(start);
	auto put = [&](uint8_t tile) { ram(static_cast<uint16_t>((start & 0xff00) | pos)) = tile; };
	for (int course = 0; course < NUM_GOLF_COURSES; course++) {
		uint8_t score = ram(static_cast<uint16_t>(wGolfBestScores + course));
		uint8_t tens, ones;
		if (score >= GOLF_BASE_SCORE + 100) { // no score yet
			tens = ones = 10;
			put(0);
			pos = static_cast<uint8_t>(pos + TILEMAP_WIDTH);
			put(0);
		} else {
			uint8_t value;
			uint8_t sign;
			if (score > GOLF_BASE_SCORE) {
				sign = 0xd2; // +
				value = static_cast<uint8_t>(score - GOLF_BASE_SCORE);
			} else if (score == GOLF_BASE_SCORE) {
				sign = 0xbe;
				value = 0;
			} else {
				sign = 0xd4; // -
				value = static_cast<uint8_t>(GOLF_BASE_SCORE - score);
			}
			put(sign);
			pos = static_cast<uint8_t>(pos + TILEMAP_WIDTH);
			put(static_cast<uint8_t>(sign + 1));
			tens = value / 10;
			ones = value % 10;
		}
		for (uint8_t digit : {tens, ones}) {
			uint8_t tile = static_cast<uint8_t>(digit * 2 + 0xa0);
			pos = static_cast<uint8_t>(pos - (TILEMAP_WIDTH - 1));
			put(tile);
			pos = static_cast<uint8_t>(pos + TILEMAP_WIDTH);
			put(static_cast<uint8_t>(tile + 1));
		}
		pos = static_cast<uint8_t>(pos - 0x1a);
	}
}

// The course's tilemap (to wGolfCourseTileMap) and palettes (to w1db00)
void Golf::load_course()
{
	wram_.temp_bank() = BANK_GolfBuildingCourse1Tilemap; // BANK("Golf Gfx 2")
	uint16_t entry = static_cast<uint16_t>(COURSE_DATA + static_cast<uint8_t>(wram_.golf_course() * 4));
	routines_.far_copy(mem_.word(entry), wGolfCourseTileMap, 7 * TILEMAP_WIDTH);
	routines_.copy(mem_.word(static_cast<uint16_t>(entry + 2)), w1db00, 8 * 8);
}

void Golf::set_course_target_scx()
{
	wram_.golf_menu_target_scx() = ram(static_cast<uint16_t>(COURSE_SCROLL_X + wram_.golf_course()));
}

void Golf::handle_building()
{
	switch (wram_.unk_1dc00()) {
	case 0: handle_course_menu(); break;
	case 1: scroll_to_course(); break;
	case 2: stop_scrolling(); break;
	case 3: pay_for_course(); break;
	default: unrealistic(); break;
	}
	update_building_animations();
	routines_.clear_unused_virtual_oam();
	if (wram_.sub_state() == 0x08) {
		// leaving
		wram_.unk_1d800() = 0;
		wram_.golf_menu_option() = 0;
	}
	GolfHole(gb_).set_building_vblank();
}

// A picks the course (if Wario can pay), B leaves, left and right scroll
// through the courses (the fourth only with all Musical Coins).
void Golf::handle_course_menu()
{
	uint8_t pressed = wram_.joypad_pressed();
	if (pressed & PAD_A) {
		if (!ram(wNumCoins + 0) && ram(wNumCoins + 1) < GOLF_BUILDING_PRICE) {
			play_sfx(mem_, SFX_0E5);
			return;
		}
		bool borrow;
		ram(wNumCoins + 1) = bcd_sub(ram(wNumCoins + 1), GOLF_BUILDING_PRICE, borrow);
		ram(wNumCoins + 0) = static_cast<uint8_t>(ram(wNumCoins + 0) - borrow);
		wram_.unk_1dc00() = 0x03;
		return;
	}
	if (pressed & PAD_B) {
		wram_.sub_state() = 0x08;
		return;
	}
	Byte option = wram_.golf_menu_option();
	bool all_courses = wram_.has_all_musical_coin_flags();
	if (pressed & PAD_RIGHT) {
		wram_.golf_menu_scrolling_dir() = 0;
		uint8_t course = option;
		if (course == GOLF_COURSE_4 || (course == GOLF_COURSE_3 && !all_courses))
			option = GOLF_COURSE_1;
		else
			option = static_cast<uint8_t>(course + 1);
	} else if (pressed & PAD_LEFT) {
		wram_.golf_menu_scrolling_dir() = 0x1;
		uint8_t course = option;
		if (course)
			option = static_cast<uint8_t>(course - 1);
		else
			option = all_courses ? GOLF_COURSE_4 : GOLF_COURSE_3;
	} else {
		return;
	}

	play_sfx(mem_, SFX_0E2);
	wram_.unk_1dc00() = 0x01;
	wram_.golf_vblank_mode() = 0x01;
	wram_.golf_course() = option;
	load_course();
	routines_.far_copy(ADDR_GolfBuildingCourseScrollingAttrmap, wGoldBuildingCourseAttrMap, 7 * TILEMAP_WIDTH);
	set_course_target_scx();
}

void Golf::scroll_to_course()
{
	uint8_t target = wram_.golf_menu_target_scx();
	bool left = wram_.golf_menu_scrolling_dir();
	uint8_t scx = wram_.scx();
	if (scx == target) {
		wram_.unk_1dc00() = 0x02;
		wram_.golf_vblank_mode() = 0x02;
		return;
	}
	wram_.scx() = static_cast<uint8_t>(left ? scx - 4 : scx + 4);
}

void Golf::stop_scrolling()
{
	wram_.unk_1dc00() = 0;
	wram_.golf_vblank_mode() = 0x03;
}

// Counts the price off the coins, then starts the course's first hole.
void Golf::pay_for_course()
{
	uint8_t counter = wram_.golf_counter();
	if (counter < 0x32) { // GOLF_BUILDING_PRICE, in decimal
		wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
		if (!(counter & 0x03))
			play_sfx(mem_, SFX_12B);
		bool borrow;
		ram(wGolfNumCoins + 1) = bcd_sub(ram(wGolfNumCoins + 1), 1, borrow);
		ram(wGolfNumCoins + 0) = static_cast<uint8_t>(ram(wGolfNumCoins + 0) - borrow);
		return;
	}
	play_sfx(mem_, SFX_SELECTION);
	wram_.predetermined_golf_hole() = ram(static_cast<uint16_t>(FIRST_GOLF_HOLES + wram_.golf_menu_option()));
	wram_.sub_state() = 0x04;
}

void Golf::update_building_animations()
{
	set_oam_pointer(ADDR_OAM_1cb3e0);
	update_object(object(wGolfObj1), 0x42, 0x58, ADDR_Frameset_1cb59b);
	update_object(object(wGolfObj3), 0x2a, 0x58, ADDR_Frameset_1cb598);
	update_object(object(wGolfObj4), SCREEN_WIDTH_PX, 0x58, ADDR_Frameset_1cb59e);
	if (wram_.unk_1dc00())
		return;
	update_object(object(wGolfObj5), 0x76, 0x6c, ADDR_Frameset_1cb5a1);

	// Wario, posing for the course in the menu
	GolfObject golfer = wario();
	golfer.duration() = 0;
	golfer.frameset_offset() = 0;
	golfer.y() = 0x76;
	golfer.x() = 0x6c;
	uint16_t frameset =
		mem_.word(static_cast<uint16_t>(COURSE_WARIO_FRAMESETS + static_cast<uint8_t>(wram_.golf_menu_option() * 2)));
	routines_.update_golf_object_animation(frameset, golfer.duration().addr());
	golfer.frame() = golfer.current_frame();
	routines_.load_golf_sprite(golfer.sprite());

	update_object(object(wGolfParaGoom), 0x50, 0x55, ADDR_Frameset_1cb5c3);
}

} // namespace wl3::golf

using wl3::golf::Golf;

// The state tables, reached by farcall from GolfStateTable and
// GolfBuildingStateTable. hCallFunc restores af after them and the main loop
// follows, so their callers use no register they leave.
PORT_EX(_GolfStateTable, 0, ALL_REGISTERS)
{
	Golf(gb).run_golf_state();
}

PORT_EX(_GolfBuildingStateTable, 0, ALL_REGISTERS)
{
	Golf(gb).run_building_state();
}

// VBlank_1c84e0.Func, run from wVBlankFunc (see ram_code.c): sets the golf
// RAM bank and the scroll, then the OAM DMA.
PORT_LOCAL_EX(VBlank_1c84e0, Func, HOOK_MANUAL_RET, 0)
{
	wl3::Memory mem(gb);
	gb->cpu.a = 0x01; // BANK("Golf RAM")
	mem[rWBK] = gb->cpu.a;
	gb->cpu.a = mem[wSCX];
	mem[rSCX] = gb->cpu.a;
	gb->cpu.a = static_cast<uint8_t>(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitGolfLobby);
PORT_COVERED(HandleGolfLobby);
PORT_COVERED(HandleGolfLobbyStates);
PORT_COVERED(UpdateGolfLobbyAnimations);
PORT_COVERED(HandleGolfLobbyWarioSprite);
PORT_COVERED(InitGolfClearCutscene);
PORT_COVERED(UpdateGolfClearCutscene);
PORT_COVERED(UpdateGolfClearCutsceneAnimation);
PORT_COVERED(VBlank_1c84e0);
PORT_COVERED(Func_1c8586);
PORT_COVERED(Func_1c8604);
PORT_COVERED(Func_1c87db);
PORT_COVERED(GetGolfCourseTargetSCX);
PORT_COVERED(Func_1c86dd);
PORT_COVERED(Func_1c882b);
PORT_COVERED(Func_1c8837);
PORT_COVERED(Func_1c88dc);
PORT_COVERED(Func_1c8907);
PORT_COVERED(Func_1c8911);
PORT_COVERED(Func_1c86f8);
