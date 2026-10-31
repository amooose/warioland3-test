// The golf holes; see golf.hpp.
#include "golf.hpp"

#include <initializer_list>

namespace wl3::golf {

namespace {

// tables in the routines' own code (local labels, not in syms.h)
constexpr uint16_t WARIO_GFX_BANKS = ADDR_LoadGolfWarioStateGfx + 0x1d;
constexpr uint16_t WARIO_GFX_POINTERS = ADDR_LoadGolfWarioStateGfx + 0x29;
constexpr uint16_t SHOT_POWER_VELS = ADDR_GolfHoleState_SelectShotPower + 0x10e; // .data
constexpr uint16_t NORMAL_BOUNCE_VELS = ADDR_GolfHoleState_Shot + 0x69;
constexpr uint16_t TOPSPIN_BOUNCE_VELS = ADDR_GolfHoleState_Shot + 0x7d;
constexpr uint16_t HOLE_X_OFFSET_SIGNS = ADDR_GolfHoleState_SpecialTerrain + 0x22b;
constexpr uint16_t HOLE_X_OFFSETS = ADDR_GolfHoleState_SpecialTerrain + 0x243;
constexpr uint16_t HOLE_Y_OFFSETS = ADDR_GolfHoleState_SpecialTerrain + 0x25b;
constexpr uint16_t RESULT_FRAMESETS = ADDR_ShowGolfResultOverlayText + 0x2a;
constexpr uint16_t PARAGOOM_FRAMESETS = ADDR_UpdateGolfParaGoom + 0xa1;
constexpr uint16_t COURSE_PALS = ADDR_InitGolfHole + 0x8d; // .GolfCoursePals

constexpr uint8_t VRAM1 = BANK_v1Tiles0;
constexpr uint8_t GROUND_LO = static_cast<uint8_t>(PARAGOOM_GROUND_LEVEL);
constexpr uint8_t GROUND_HI = static_cast<uint8_t>(PARAGOOM_GROUND_LEVEL >> 8);

// Compare16Bit: 0 if a < b, 1 if equal, 2 if a > b
uint8_t compare16(uint16_t a, uint16_t b)
{
	return a < b ? 0 : a == b ? 1 : 2;
}

// a % b, as the asm's subtraction loop computes it (b = 0 never ends)
uint8_t remainder(uint8_t a, uint8_t b)
{
	return static_cast<uint8_t>(a % b);
}

} // namespace

// ---------------------------------------------------------------------------
// Setting up a hole

void GolfHole::init()
{
	routines_.disable_lcd();
	routines_.clear_virtual_oam();
	wram_.room_animated_tiles_enabled() = 0;

	// clears the hole's RAM; the loop's counter (c, then b) runs out after
	// c + 256 * (b - 1) bytes, not bc
	uint16_t size = wGolfRAMEnd - wGolfHoleRAMStart;
	uint8_t b = static_cast<uint8_t>(size >> 8), c = static_cast<uint8_t>(size);
	uint16_t addr = wGolfHoleRAMStart;
	for (;;) {
		ram(addr++) = 0;
		if (--c)
			continue;
		if (!--b)
			break;
	}

	uint16_t pals = mem_.word(static_cast<uint16_t>(COURSE_PALS + static_cast<uint8_t>(wram_.golf_menu_option() * 2)));
	routines_.load_pals_to_temp_pals1(pals);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_1ca18f);

	ram(rVBK) = VRAM1;
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfFlagGfx, v1Tiles0);
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfGfx, v0Tiles1);
	wram_.temp_bank() = BANK_WarioIdleGfx;
	routines_.far_copy(ADDR_WarioIdleGfx, v0Tiles0, 0x80 * TILE_SIZE);

	reset_scroll();
	ram(rWY) = 0;
	ram(rWX) = 159 + WX_OFS;
	wram_.golf_wx() = 159 + WX_OFS;

	set_up_hole();
	set_hole_vblank();
	play_music2(mem_, MUSIC_GOLF);
	ram(rLCDC) = LCDC_DEFAULT; // (set_hole_vblank already chose it)
	++wram_.sub_state();
}

void GolfHole::set_up_hole()
{
	pick_and_load_hole();
	show_flag_and_par();
	set_up_objects();
}

// The hole is wPredeterminedGolfHole (on a course), else a random one of
// the first 8, 16 or all 24, by the number of treasures collected.
void GolfHole::pick_and_load_hole()
{
	wram_.temp_bank() = BANK_GolfGfx;
	uint8_t hole;
	if (uint8_t predetermined = wram_.predetermined_golf_hole()) {
		hole = static_cast<uint8_t>(predetermined - 1);
	} else {
		wram_.golf_hole_tier() = GOLF_HOLE_TIER_3;
		uint8_t treasures = ram(wNumCollectedTreasures + 1); // BCD
		if (!ram(wNumCollectedTreasures + 0) && treasures < GOLF_TREASURES_TIER_3)
			wram_.golf_hole_tier() = treasures < GOLF_TREASURES_TIER_2 ? GOLF_HOLE_TIER_1 : GOLF_HOLE_TIER_2;
		uint8_t tier = wram_.golf_hole_tier();
		uint8_t div = ram(rDIV);
		if (!tier) {
			unrealistic(); // the asm's loop never ends
			return;
		}
		hole = remainder(div, tier);
	}

	// a hole: 4 pointers to its tilemaps and attribute maps (in the order
	// they're used, from the last), then one to its data
	uint16_t entry = static_cast<uint16_t>(ADDR_GolfHoles + hole * 10);
	auto pointer = [&](int i) -> uint16_t { return mem_.word(static_cast<uint16_t>(entry + i * 2)); };
	ram(rVBK) = VRAM1;
	routines_.far_decompress(BANK_GolfGfx, pointer(4), 7 * TILEMAP_WIDTH + v0BGMap0);
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfBackground1Attrmap, v1BGMap0);
	routines_.far_decompress(BANK_GolfGfx, pointer(3), 7 * TILEMAP_WIDTH + v1BGMap1);
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfBackground2Attrmap, v1BGMap1);
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfGfx, pointer(2), 7 * TILEMAP_WIDTH + v0BGMap0);
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfBackground1Tilemap, v0BGMap0);
	routines_.far_decompress(BANK_GolfGfx, pointer(1), 7 * TILEMAP_WIDTH + v0BGMap1);
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfBackground2Tilemap, v0BGMap1);
	routines_.far_copy(pointer(0), wGolfHoleData, wGolfHoleDataEnd - wGolfHoleData);
	ram(wGolfHoleHoleXPos + 0) = ram(w1db82 + 0);
	ram(wGolfHoleHoleXPos + 1) = ram(w1db82 + 1);
}

// The flag's hole number, the par, the flag (outside the golf building)
// and the stroke number, in VRAM bank 1.
void GolfHole::show_flag_and_par()
{
	ram(rVBK) = VRAM1;
	uint8_t number;
	if (wram_.unk_1d800() != 0x02) {
		wram_.golf_max_strokes() = static_cast<uint8_t>(wram_.golf_par() + MAX_GOLF_COURSE_STROKES);
		number = wram_.golf_course_hole();
	} else {
		// in the Golf Building, the strokes can't exceed the par
		wram_.golf_max_strokes() = wram_.golf_par();
		number = static_cast<uint8_t>(wram_.golf_par() - 1);
	}
	auto tiles = [](uint8_t tile) { return static_cast<uint16_t>(v1Tiles0 + tile * 0x10); };
	routines_.copy(static_cast<uint16_t>(ADDR_GolfFlagNumbersGfx + static_cast<uint8_t>(number << 4 | number >> 4)),
		tiles(0x01), 0x10);

	uint8_t par = static_cast<uint8_t>(wram_.golf_par() - 1);
	uint8_t par_gfx = static_cast<uint8_t>((par << 4 | par >> 4) - 2 * 0x10);
	for (uint8_t tile : {0x2b, 0x4b, 0x6b, 0x8b})
		routines_.copy(static_cast<uint16_t>(ADDR_GolfParNumbersGfx + par_gfx), tiles(tile), 0x10);

	if (wram_.unk_1d800() != 0x02) {
		uint16_t flag = ADDR_GolfHoleFlagGfx;
		for (uint16_t dest : {tiles(0x20), tiles(0x40), tiles(0x60), static_cast<uint16_t>(v0Tiles1)}) {
			routines_.copy(flag, dest, 6 * 0x10);
			flag = static_cast<uint16_t>(flag + 6 * 0x10);
		}
	}

	print_stroke_number();
	routines_.copy(w1db90, tiles(0xf0), 4 * 0x10);
	ram(rVBK) = 0;
}

void GolfHole::set_up_objects()
{
	set_oam_pointer(ADDR_OAM_1ca6cf);
	wram_.shot_power_gauge_cursor() = BASE_SHOT_POWER;
	static const uint8_t positions[] = {
		0x58, 0x00, 0x40, 0x00, // Wario's y and x
		static_cast<uint8_t>(PARAGOOM_TEE_BOX_LEVEL), static_cast<uint8_t>(PARAGOOM_TEE_BOX_LEVEL >> 8),
		0x80, 0x00, // the Para-Goom's y and x
	};
	for (unsigned i = 0; i < sizeof(positions); i++)
		ram(static_cast<uint16_t>(wGolfWarioYPos + i)) = positions[i];
	update_ui();
	update_paragoom();
	update_wario();
	routines_.clear_unused_virtual_oam();
}

// The stroke number's two big digits, to w1db90 (the V-Blank function
// copies them to VRAM).
void GolfHole::print_stroke_number()
{
	uint8_t stroke = wram_.golf_stroke();
	uint8_t tens = static_cast<uint8_t>(stroke / 10), ones = static_cast<uint8_t>(stroke % 10);
	routines_.copy(static_cast<uint16_t>(ADDR_GolfStrokeNumbersGfx + tens * 2 * 16), w1db90, 2 * 16);
	routines_.copy(static_cast<uint16_t>(ADDR_GolfStrokeNumbersGfx + ones * 2 * 16), w1db90 + 2 * 16, 2 * 16);
}

// ---------------------------------------------------------------------------
// Each frame

void GolfHole::handle()
{
	set_oam_pointer(ADDR_OAM_1ca6cf);
	run_state();
	update_ui();
	update_paragoom();
	update_wario();
	routines_.clear_unused_virtual_oam();
	load_wario_state_gfx();
	set_hole_vblank();
}

void GolfHole::run_state()
{
	switch (wram_.golf_hole_state()) {
	case GOLFHOLESTATE_WAIT_INPUT: state_wait_input(); break;
	case GOLFHOLESTATE_SCROLL: state_scroll(); break;
	case GOLFHOLESTATE_SELECT_SHOT_POWER: state_select_shot_power(); break;
	case GOLFHOLESTATE_SELECT_SHOT_SPIN: state_select_shot_spin(); break;
	case GOLFHOLESTATE_START_SHOT: state_start_shot(); break;
	case GOLFHOLESTATE_SHOT: state_shot(); break;
	case GOLFHOLESTATE_BOUNCE: state_bounce(); break;
	case GOLFHOLESTATE_SPECIAL_TERRAIN: state_special_terrain(); break;
	case GOLFHOLESTATE_WALK: state_walk(); break;
	case GOLFHOLESTATE_BETWEEN_SHOTS: state_between_shots(); break;
	case GOLFHOLESTATE_CLEARED: state_cleared(); break;
	case GOLFHOLESTATE_GAME_OVER: state_game_over(); break;
	case GOLFHOLESTATE_PAUSE: state_pause(); break;
	case GOLFHOLESTATE_RESULT: state_result(); break;
	default: unrealistic(); break;
	}
}

// Func_1c8957: an unreferenced state table for a hole on its own
void GolfHole::run_unreferenced_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: handle(); break;
	case 4: break; // Func_1c8fb7: nothing
	default: unrealistic(); break;
	}
}

// ---------------------------------------------------------------------------
// The states

// Select pauses, A starts a shot, left and right look along the hole.
void GolfHole::state_wait_input()
{
	uint16_t scroll = wram_.golf_x_scroll().word();
	uint8_t pressed = wram_.joypad_pressed();
	if (pressed & PAD_SELECT) {
		play_sfx(mem_, SFX_0E4);
		object(wGolfObj3).duration() = 0;
		object(wGolfObj3).frameset_offset() = 0;
		wram_.golf_hole_state() = GOLFHOLESTATE_PAUSE;
		return;
	}
	if (pressed & PAD_A) {
		play_sfx(mem_, SFX_0E9);
		for (uint16_t i = 0; i < 8; i++) // wGolfOverlayText
			ram(static_cast<uint16_t>(wGolfOverlayText + i)) = 0;
		wram_.golf_result() = 0;
		wram_.golf_hole_state() = GOLFHOLESTATE_SELECT_SHOT_POWER;
		wram_.stroke_golf_terrain() = wram_.golf_terrain();
		wram_.unk_1dc53() = wram_.unk_1dc47();
		// where the Para-Goom was, for a ball out of play
		routines_.copy(wGolfParaGoomYPos, wLastGolfParaGoomYPos, 4);
		return;
	}
	uint8_t dir;
	if (pressed & PAD_RIGHT) {
		if ((scroll >> 8) && static_cast<uint8_t>(scroll) >= static_cast<uint8_t>(MAX_GOLF_X_SCROLL))
			return;
		dir = 0;
	} else if (pressed & PAD_LEFT) {
		if (!scroll)
			return;
		dir = 1;
	} else {
		return;
	}
	wram_.golf_hole_scrolling_dir() = dir;
	ram(wTempGolfXScroll + 0) = ram(wGolfXScroll + 0);
	ram(wTempGolfXScroll + 1) = ram(wGolfXScroll + 1);
	wram_.golf_hole_state() = GOLFHOLESTATE_SCROLL;
}

// Looking along the hole, with a ghost Para-Goom where the shot would land
// at the default power.
void GolfHole::state_scroll()
{
	scroll_by_input();

	uint16_t x = static_cast<uint16_t>(wram_.golf_para_goom_x_pos().word() >> 2);
	uint8_t terrain = wram_.golf_terrain();
	uint8_t distance = terrain == GOLF_TERRAIN_ROUGH ? 0x80 : terrain == GOLF_TERRAIN_BUNKER ? 0x50 : 0xa0;
	x = static_cast<uint16_t>(wram_.golf_hole_direction() ? x - distance : x + distance);
	if (!in_view(x))
		return;
	GolfObject ghost = object(wGolfParaGoomGhost);
	ghost.x() = static_cast<uint8_t>(static_cast<uint8_t>(x) + OAM_X_OFS - wram_.scx());
	ghost.y() = 0x72;
	routines_.update_golf_object_animation(ADDR_Frameset_1cb0a4, ghost.duration().addr());
	load_object_sprite(ghost);
}

// A or B goes back; the d-pad scrolls further (while held), else it scrolls
// back to where it started.
void GolfHole::scroll_by_input()
{
	if (wram_.joypad_pressed() & (PAD_A | PAD_B)) {
		ram(wGolfXScroll + 0) = ram(wTempGolfXScroll + 0);
		ram(wGolfXScroll + 1) = ram(wTempGolfXScroll + 1);
		wram_.golf_hole_state() = GOLFHOLESTATE_WAIT_INPUT;
		update_display_mode();
		return;
	}
	uint16_t scroll = wram_.golf_x_scroll().word();
	bool left = wram_.golf_hole_scrolling_dir();
	uint8_t down = wram_.joypad_down();
	if (!left && (down & PAD_RIGHT)) {
		if ((scroll >> 8) && static_cast<uint8_t>(scroll) >= static_cast<uint8_t>(MAX_GOLF_X_SCROLL))
			return; // can't scroll further right
		wram_.golf_x_scroll().word() = static_cast<uint16_t>(scroll + 1);
		update_display_mode();
		return;
	}
	if (left && (down & PAD_LEFT)) {
		if (!scroll)
			return; // can't scroll further left
		wram_.golf_x_scroll().word() = static_cast<uint16_t>(scroll - 1);
		update_display_mode();
		return;
	}
	// scroll back
	scroll = static_cast<uint16_t>(left ? scroll + 1 : scroll - 1);
	wram_.golf_x_scroll().word() = scroll;
	if (scroll == wram_.temp_golf_x_scroll().word())
		wram_.golf_hole_state() = GOLFHOLESTATE_WAIT_INPUT;
	update_display_mode();
}

// The power gauge rises (faster in its upper half) and falls back; A picks
// the power. Letting it fall to 0 is a miss.
void GolfHole::state_select_shot_power()
{
	Byte cursor = wram_.shot_power_gauge_cursor();
	if (!wram_.is_power_gauge_falling()) {
		uint8_t power = static_cast<uint8_t>(cursor + (cursor >= 16 ? 2 : 1));
		cursor = power;
		if (power >= MAX_SHOT_POWER) {
			play_sfx(mem_, SFX_0EA);
			wram_.is_power_gauge_falling() = TRUE;
		}
	} else {
		uint8_t power = cursor;
		cursor = static_cast<uint8_t>(power - 2);
		if (power < 2) {
			wram_.golf_counter() = 0;
			cursor = 0;
			wram_.golf_hole_state() = GOLFHOLESTATE_START_SHOT;
			return;
		}
	}
	if (!(wram_.joypad_pressed() & PAD_A))
		return;

	wario().duration() = 0;
	wario().frameset_offset() = 0;
	object(wGolfObj5).state() = 1;
	wram_.golf_hole_state() = GOLFHOLESTATE_SELECT_SHOT_SPIN;
	uint8_t power = cursor;
	wram_.selected_shot_power_gauge() = power;
	static const uint8_t limits[] = {16, 24, 32, 40, 48, 56, 64, 70, 74};
	static const uint8_t levels[] = {
		SHOT_POWER_0, SHOT_POWER_1, SHOT_POWER_2, SHOT_POWER_3, SHOT_POWER_4,
		SHOT_POWER_5, SHOT_POWER_6, SHOT_POWER_7, SHOT_POWER_8, SHOT_POWER_9,
	};
	unsigned level = 0;
	while (level < sizeof(limits) && power >= limits[level])
		level++;
	wram_.shot_power_level() = levels[level];

	uint16_t vels = static_cast<uint16_t>(SHOT_POWER_VELS + static_cast<uint8_t>(wram_.shot_power_level() << 1));
	ram(wShotYVel + 0) = mem_[vels];
	ram(wShotYVel + 1) = 0;
	wram_.shot_x_vel() = mem_[static_cast<uint16_t>(vels + 1)];

	// a shot from the rough or a bunker goes less high
	uint8_t terrain = wram_.golf_terrain();
	uint8_t y_vel = wram_.shot_y_vel();
	if (terrain == GOLF_TERRAIN_ROUGH) {
		wram_.shot_y_vel() = static_cast<uint8_t>(static_cast<uint8_t>(y_vel * 3 + 3) >> 2);
		wram_.shot_terrain_modifier() = SHOT_MODIFIER_ROUGH;
	} else if (terrain == GOLF_TERRAIN_BUNKER) {
		wram_.shot_y_vel() = static_cast<uint8_t>((static_cast<uint8_t>(y_vel + 1) >> 1) - 1);
		wram_.shot_terrain_modifier() = SHOT_MODIFIER_BUNKER;
	} else {
		wram_.shot_terrain_modifier() = SHOT_MODIFIER_NONE;
	}
}

// The gauge again, for the spin: A picks it by how far it went.
void GolfHole::state_select_shot_spin()
{
	Byte gauge = wram_.selected_shot_power_gauge();
	bool check_a = true;
	if (!wram_.is_power_gauge_falling()) {
		uint8_t value = static_cast<uint8_t>(gauge + (gauge >= 16 ? 2 : 1));
		gauge = value;
		if (value >= MAX_SHOT_POWER) {
			play_sfx(mem_, SFX_0EA);
			wram_.is_power_gauge_falling() = TRUE;
		}
	} else {
		uint8_t value = gauge;
		gauge = static_cast<uint8_t>(value - 2);
		if (value < 2) {
			gauge = 0;
			check_a = false;
		}
	}
	if (check_a && !(wram_.joypad_pressed() & PAD_A))
		return;

	wram_.golf_counter() = 0;
	wram_.golf_hole_state() = GOLFHOLESTATE_START_SHOT;
	uint8_t value = gauge;
	uint8_t spin = SHOT_MISS;
	if (value >= 2 && value < 5)
		spin = SHOT_BACKSPIN;
	else if (value >= 5 && value < 11)
		spin = SHOT_NORMAL;
	else if (value >= 11 && value < 14)
		spin = SHOT_TOPSPIN;
	wram_.shot_spin() = spin;
}

// Wario's swing: wGolfCounter steps it (the animation advances it too).
void GolfHole::state_start_shot()
{
	uint8_t counter = wram_.golf_counter();
	switch (counter) {
	case 0:
		wram_.golf_counter() = 1;
		++wram_.golf_stroke();
		wram_.golf_vblank_mode() = 0x03;
		print_stroke_number();
		return;
	case 1:
		wram_.golf_counter() = 2;
		play_sfx(mem_, SFX_0ED);
		wario().duration() = 0;
		wario().frameset_offset() = 0;
		wram_.golf_wario_state() = GOLF_WARIO_CHARGING;
		wram_.golf_vblank_mode() = 0x02;
		return;
	case 3:
		wram_.golf_counter() = 4;
		wario().duration() = 0;
		wario().frameset_offset() = 0;
		if (wram_.shot_power_gauge_cursor() && wram_.shot_spin()) {
			play_sfx(mem_, SFX_017);
			wram_.golf_wario_state() = GOLF_WARIO_ATTACKING_1;
		} else {
			play_sfx(mem_, SFX_0E8);
			wram_.golf_wario_state() = GOLF_WARIO_MISSING;
			wram_.golf_vblank_mode() = 0x02;
		}
		return;
	}
	if (counter < 5)
		return;

	wram_.golf_counter() = 0;
	wario().duration() = 0;
	wario().frameset_offset() = 0;
	if (wram_.golf_wario_state() == GOLF_WARIO_MISSING) {
		wram_.golf_vblank_mode() = 0x02;
		wram_.golf_wario_state() = GOLF_WARIO_IDLING;
		wram_.golf_hole_state() = GOLFHOLESTATE_BETWEEN_SHOTS;
		return;
	}
	wram_.shot_direction() = wram_.golf_hole_direction();
	wram_.golf_para_goom_frameset_offset() = 0;
	wram_.golf_para_goom_state() = GOLF_PARAGOOM_1;
	wram_.golf_wario_state() = GOLF_WARIO_ATTACKING_2;
	wram_.unk_1dc01() = GOLFHOLESTATE_BOUNCE;
	wram_.golf_hole_state() = GOLFHOLESTATE_SHOT;
}

// The Para-Goom flies; when it lands (and bounces), the bounce gets its
// velocities by the shot's power and spin.
void GolfHole::state_shot()
{
	update_paragoom_x();
	update_paragoom_y();
	align_x_scroll();
	update_display_mode();
	if (wram_.golf_hole_state() == GOLFHOLESTATE_SHOT)
		return;

	wram_.unk_1dc01() = GOLFHOLESTATE_WALK;
	uint8_t spin = wram_.shot_spin();
	uint16_t table = NORMAL_BOUNCE_VELS;
	if (spin == SHOT_TOPSPIN)
		table = TOPSPIN_BOUNCE_VELS;
	else if (spin != SHOT_NORMAL)
		wram_.shot_direction() ^= 0x1; // backspin bounces back
	uint16_t vels = static_cast<uint16_t>(table + static_cast<uint8_t>(wram_.shot_power_level() << 1));
	ram(wShotYVel + 0) = mem_[vels];
	ram(wShotYVel + 1) = 0;
	wram_.shot_x_vel() = mem_[static_cast<uint16_t>(vels + 1)];

	uint8_t modifier = wram_.shot_terrain_modifier();
	uint8_t y_vel = wram_.shot_y_vel();
	if (modifier == SHOT_MODIFIER_ROUGH)
		wram_.shot_y_vel() = static_cast<uint8_t>(static_cast<uint8_t>(y_vel * 3 + 3) >> 2);
	else if (modifier == SHOT_MODIFIER_BUNKER)
		wram_.shot_y_vel() = static_cast<uint8_t>(static_cast<uint8_t>(y_vel + 1) >> 1);
}

void GolfHole::state_bounce()
{
	update_paragoom_x();
	update_paragoom_y();
	align_x_scroll();
	update_display_mode();
}

// After landing somewhere other than the fairway: the hole, the rough, a
// bunker, water, out of bounds.
void GolfHole::state_special_terrain()
{
	GolfObject text = object(wGolfOverlayText);
	text.y() = 0x50;
	text.x() = 0x58;
	object(wGolfObj4).state() = 0x58; // (sic: the x coordinate)
	wram_.unk_1dc01() = GOLFHOLESTATE_BETWEEN_SHOTS;

	bool rise = false; // .asm_1c9743: back up from under the ground
	uint8_t terrain = wram_.golf_terrain() & GOLF_TERRAIN_MASK;
	switch (terrain) {
	case GOLF_TERRAIN_HOLE: {
		uint8_t counter = wram_.golf_counter();
		if (counter < 0x30) {
			wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
			if (counter >= 0x18)
				return; // a delay
			// rolling into the hole
			Word x = wram_.golf_para_goom_x_pos().word();
			uint8_t offset = mem_[static_cast<uint16_t>(HOLE_X_OFFSETS + counter)];
			x = static_cast<uint16_t>(mem_[static_cast<uint16_t>(HOLE_X_OFFSET_SIGNS + counter)] ? x - offset : x + offset);
			wram_.golf_para_goom_y_pos() =
				static_cast<uint8_t>(wram_.golf_para_goom_y_pos() + mem_[static_cast<uint16_t>(HOLE_Y_OFFSETS + counter)]);
			return;
		}
		uint8_t stroke = wram_.golf_stroke(), par = wram_.golf_par();
		uint8_t next = GOLFHOLESTATE_CLEARED;
		if (stroke == par) {
			play_music2(mem_, MUSIC_GOLF_CLEAR);
			wram_.golf_result() = GOLFRESULT_PAR;
		} else if (stroke < par) {
			play_music2(mem_, MUSIC_GOLF_CLEAR);
		} else {
			wram_.golf_result() = GOLFRESULT_OVER_PAR;
			if (wram_.unk_1d800() != 0x01)
				next = GOLFHOLESTATE_GAME_OVER;
		}
		wram_.unk_1dc01() = next;
		break;
	}

	case GOLF_TERRAIN_ROUGH:
	case GOLF_TERRAIN_BUNKER: {
		bool rough = terrain == GOLF_TERRAIN_ROUGH;
		if (animate_overlay_text(rough ? ADDR_Frameset_1cb035 : ADDR_Frameset_1cb068))
			break;
		if (text.frameset_offset() == (rough ? 0x0a : 0x06) && !text.duration())
			play_sfx(mem_, SFX_0F0);
		load_object_sprite(text);
		return;
	}

	case GOLF_TERRAIN_WATER:
	case GOLF_TERRAIN_OB: {
		bool water = terrain == GOLF_TERRAIN_WATER;
		uint8_t state = wram_.golf_para_goom_state();
		if (state == GOLF_PARAGOOM_4) {
			rise = true;
			break;
		}
		if (state == GOLF_PARAGOOM_6) {
			uint8_t y = wram_.golf_para_goom_y_pos();
			if (y < (water ? 0xc0 : 0xa0)) {
				wram_.golf_para_goom_y_pos() = static_cast<uint8_t>(y + 4); // sinking
				return;
			}
			if (water) {
				wram_.golf_para_goom_state() = GOLF_PARAGOOM_2;
				wram_.stroke_golf_terrain() = GOLF_TERRAIN_PLAIN;
				return;
			}
			wram_.golf_para_goom_state() = GOLF_PARAGOOM_3;
		} else if (water) {
			return;
		}
		if (!animate_overlay_text_1cb016())
			return;
		wram_.golf_para_goom_state() = GOLF_PARAGOOM_4;
		align_x_scroll();
		update_display_mode();
		update_hole_direction();
		return;
	}

	default:
		if (wram_.golf_para_goom_state() == GOLF_PARAGOOM_4) {
			rise = true;
			break;
		}
		if (!animate_overlay_text_1cb016())
			return;
		// back where it was shot from
		ram(wGolfParaGoomYPos + 0) = 0;
		ram(wGolfParaGoomYPos + 1) = 0;
		ram(wGolfParaGoomXPos + 0) = ram(wLastGolfParaGoomXPos + 0);
		ram(wGolfParaGoomXPos + 1) = ram(wLastGolfParaGoomXPos + 1);
		align_x_scroll();
		update_display_mode();
		update_hole_direction();
		wram_.golf_para_goom_frameset_offset() = 0;
		wram_.golf_para_goom_state() = GOLF_PARAGOOM_4;
		set_respawn_y_if_enough_strokes();
		return;
	}

	if (rise) {
		if (!wram_.golf_para_goom_y_pos().word())
			play_sfx(mem_, SFX_0EE);
		uint16_t last = wram_.last_golf_para_goom_y_pos().word();
		Word y = wram_.golf_para_goom_y_pos().word();
		y = static_cast<uint16_t>(y + 2);
		if (compare16(last, y) == 2)
			return;
		wram_.golf_terrain() = wram_.stroke_golf_terrain();
	}
	walk_to_paragoom();
}

// .asm_1c976f: Wario walks over to the Para-Goom
void GolfHole::walk_to_paragoom()
{
	if (wram_.golf_para_goom_state() != GOLF_PARAGOOM_5)
		wram_.golf_para_goom_state() = GOLF_PARAGOOM_0;
	wram_.golf_counter() = 0;
	wram_.golf_para_goom_duration() = 0;
	wario().duration() = 0;
	wram_.golf_para_goom_frameset_offset() = 0;
	wario().frameset_offset() = 0;
	object(wGolfOverlayText).frameset_offset() = 0;
	object(wGolfOverlayText).state() = 0;
	update_hole_direction();
	wram_.golf_hole_state() = GOLFHOLESTATE_WALK;
}

// Wario walks to the Para-Goom (down from the tee box first), then the next
// state is w1dc01's.
void GolfHole::state_walk()
{
	uint8_t counter = wram_.golf_counter();
	GolfObject golfer = wario();
	if (counter < 0x19) {
		if (counter < 0x18) {
			wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
			if (counter)
				return;
			wram_.golf_vblank_mode() = 0x03;
			wram_.unk_1dc21() = 0x03;
			print_stroke_number();
			return;
		}
		if (wram_.unk_1dc47() && wram_.golf_wario_y_pos() < 0x60) {
			uint8_t ground = static_cast<uint8_t>(wram_.golf_para_goom_y_pos().word() >> 2);
			if (wram_.golf_wario_y_pos() < ground) {
				// down from the tee box
				uint8_t y = static_cast<uint8_t>(wram_.golf_wario_y_pos() + 2);
				wram_.golf_wario_y_pos() = y;
				if (y >= 0x60)
					wram_.golf_vblank_mode() = 0x01;
				return;
			}
		}
		if (compare16(wram_.golf_wario_x_pos().word(), wram_.golf_wario_target_x_pos().word()) == 1) {
			wram_.golf_counter() = 0;
			wram_.golf_hole_state() = GOLFHOLESTATE_BETWEEN_SHOTS;
			return;
		}
		wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
		golfer.duration() = 0;
		golfer.frameset_offset() = 0;
		wram_.golf_wario_state() = GOLF_WARIO_WALKING;
		wram_.golf_vblank_mode() = 0x02;
		return;
	}

	// walking
	Word x = wram_.golf_wario_x_pos().word();
	uint16_t target = wram_.golf_wario_target_x_pos().word();
	bool reached;
	uint16_t next;
	if (!wram_.golf_wario_dir()) {
		next = static_cast<uint16_t>(x + 4);
		reached = compare16(next, target) != 0;
	} else {
		next = static_cast<uint16_t>(x - 4);
		reached = compare16(next, target) != 2;
	}
	if (!reached) {
		x = next;
		return;
	}
	x = target;
	wram_.golf_wario_dir() = wram_.golf_hole_direction();
	wram_.golf_counter() = 0;
	golfer.duration() = 0;
	golfer.frameset_offset() = 0;

	uint8_t next_state = wram_.unk_1dc01();
	uint8_t wario_state;
	if (next_state == GOLFHOLESTATE_CLEARED) {
		if (wram_.golf_result() < GOLFRESULT_OVER_PAR) {
			wario_state = GOLF_WARIO_CLEARED;
		} else {
			play_music2(mem_, MUSIC_GOLF_GAME_OVER);
			wario_state = GOLF_WARIO_LOST;
		}
	} else {
		wario_state = next_state == GOLFHOLESTATE_GAME_OVER ? GOLF_WARIO_LOST : GOLF_WARIO_IDLING;
	}
	wram_.golf_wario_state() = wario_state;
	wram_.golf_vblank_mode() = 0x02;
	wram_.golf_hole_state() = wram_.unk_1dc01();
}

// A pause before the next shot, or game over with too many strokes.
void GolfHole::state_between_shots()
{
	uint8_t counter = static_cast<uint8_t>(wram_.golf_counter() + 1);
	wram_.golf_counter() = counter;
	if (counter == 0x08) {
		if (wram_.golf_stroke() < wram_.golf_max_strokes())
			return;
		play_music2(mem_, MUSIC_GOLF_GAME_OVER);
		wram_.golf_counter() = 0;
		wario().duration() = 0;
		wario().frameset_offset() = 0;
		object(wGolfOverlayText).duration() = 0;
		object(wGolfOverlayText).frameset_offset() = 0;
		object(wGolfObj4).state() = 0x01;
		wram_.golf_wario_state() = GOLF_WARIO_LOST;
		wram_.golf_vblank_mode() = 0x02;
		wram_.golf_hole_state() = GOLFHOLESTATE_GAME_OVER;
		return;
	}
	if (counter < 0x20)
		return;
	wram_.shot_power_gauge_cursor() = BASE_SHOT_POWER;
	wram_.golf_counter() = 0;
	wram_.unk_1dc51() = 0;
	wram_.is_power_gauge_falling() = 0;
	object(wGolfObj4).state() = 0;
	object(wGolfObj5).state() = 0;
	wram_.golf_hole_state() = GOLFHOLESTATE_WAIT_INPUT;
}

// The hole is done: a single hole ends here, a course moves on to its next
// hole (or the result).
void GolfHole::state_cleared()
{
	GolfObject text = object(wGolfOverlayText);
	if (wram_.unk_1d800() != 0x01) {
		update_object(text, 0x50, 0x58, ADDR_Frameset_1caf8d);
		uint8_t counter = wram_.golf_counter();
		if (counter < 0x50) {
			wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
			return;
		}
		if (!(wram_.joypad_pressed() & PAD_A))
			return;
		stop_music2(mem_);
		wram_.unk_1d800() = 0;
		wram_.golf_menu_option() = 0;
		if (wram_.is_minigame_cleared()) {
			wram_.sub_state() = 0x08;
			return;
		}
		wram_.is_minigame_cleared() = TRUE;
		wram_.sub_state() = SST_GOLF_CLEAR;
		return;
	}

	// on a course
	show_result_overlay_text();
	show_score();
	uint8_t wait = wram_.golf_wario_state() == 0x06 ? 0x50 : 0x70;
	uint8_t counter = wram_.golf_counter();
	if (counter < wait) {
		wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
		return;
	}
	if (!(wram_.joypad_pressed() & PAD_A))
		return;
	wram_.golf_course_score() = static_cast<uint8_t>(wram_.golf_course_score() + wram_.golf_stroke());
	uint8_t hole = wram_.golf_course_hole();
	if (hole != NUM_GOLF_COURSE_HOLES) {
		wram_.golf_course_hole() = static_cast<uint8_t>(hole + 1);
		++wram_.predetermined_golf_hole();
		wram_.sub_state() = 0x04;
		return;
	}
	// the course is finished
	wram_.golf_counter() = 0;
	wario().duration() = 0;
	wario().frameset_offset() = 0;
	text.duration() = 0;
	text.frameset_offset() = 0;
	object(wGolfParaGoomGhost).duration() = 0;
	object(wGolfParaGoomGhost).frameset_offset() = 0;
	uint8_t score = wram_.golf_course_score();
	wram_.golf_result() = score < GOLF_BASE_SCORE + 1 ? GOLFRESULT_COURSE_UNDER_PAR : GOLFRESULT_COURSE_OVER_PAR;
	if (score < ram(static_cast<uint16_t>(wGolfBestScores + wram_.golf_course()))) {
		play_sfx(mem_, SFX_0FA);
		wram_.golf_wario_state() = GOLF_WARIO_CLEARED;
	} else {
		wram_.golf_wario_state() = GOLF_WARIO_LOST;
	}
	wram_.golf_hole_state() = GOLFHOLESTATE_RESULT;
}

void GolfHole::state_game_over()
{
	update_object(object(wGolfOverlayText), 0x50, 0x58, ADDR_Frameset_1cafa0);
	uint8_t counter = wram_.golf_counter();
	if (counter < 0x50) {
		wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
		return;
	}
	if (!(wram_.joypad_pressed() & PAD_A))
		return;
	if (wram_.unk_1d800() == 0x02)
		wram_.golf_menu_option() = 0x01;
	wram_.sub_state() = 0;
}

// A quits, B goes back.
void GolfHole::state_pause()
{
	GolfObject menu = object(wGolfObj3);
	update_object(menu, 0x9c, 0x58, ADDR_Frameset_1cb0da);
	uint8_t pressed = wram_.joypad_pressed();
	if (pressed & PAD_A) {
		if (wram_.unk_1d800() != 0x01)
			wram_.golf_menu_option() = 0x01;
		play_sfx(mem_, SFX_SELECTION);
		wram_.sub_state() = 0;
	} else if (pressed & PAD_B) {
		play_sfx(mem_, SFX_0E7);
		menu.duration() = 0;
		menu.frameset_offset() = 0;
		wram_.golf_hole_state() = GOLFHOLESTATE_WAIT_INPUT;
	}
}

// The course's total score, and a new best score.
void GolfHole::state_result()
{
	show_result_overlay_text();

	// the score's sign and digits in the overlay text's sprites
	--wram_.virtual_oam_sprite00_y_coord();
	uint8_t score = wram_.golf_course_score();
	uint8_t value;
	if (score > GOLF_BASE_SCORE) {
		value = static_cast<uint8_t>(score - GOLF_BASE_SCORE);
	} else {
		if (score == GOLF_BASE_SCORE) {
			--wram_.virtual_oam_sprite00_y_coord();
			wram_.virtual_oam_sprite00_frame() = 0xfe;
		}
		value = static_cast<uint8_t>(GOLF_BASE_SCORE - score);
	}
	uint8_t tens = static_cast<uint8_t>(value / 10), ones = static_cast<uint8_t>(value % 10);
	if (tens)
		wram_.virtual_oam_sprite01_frame() = static_cast<uint8_t>(tens * 2 + 0x80);
	wram_.virtual_oam_sprite02_frame() = static_cast<uint8_t>(ones * 2 + 0x80);

	update_object(object(wGolfParaGoomGhost), 0x3c, 0x50, ADDR_Frameset_1cb0d7); // "TOTAL"

	uint8_t counter = wram_.golf_counter();
	if (counter < 0x20) {
		wram_.golf_counter() = static_cast<uint8_t>(counter + 1);
		return;
	}
	if (!(wram_.joypad_pressed() & PAD_A))
		return;
	Byte best = ram(static_cast<uint16_t>(wGolfBestScores + wram_.golf_course()));
	if (score < best)
		best = score;
	wram_.sub_state() = 0;
}

// ---------------------------------------------------------------------------
// The Para-Goom

// Moves it along by the shot's x velocity (which drops every 8 frames), and
// finds the terrain under it: 32 pixels a block.
void GolfHole::update_paragoom_x()
{
	uint8_t frames = static_cast<uint8_t>(wram_.unk_1dc51() + 1);
	wram_.unk_1dc51() = frames;
	if (!(frames & 0x07))
		--wram_.shot_x_vel();

	Word x = wram_.golf_para_goom_x_pos().word();
	bool out_of_bounds = false;
	if (!wram_.shot_direction()) {
		x = static_cast<uint16_t>(x + wram_.shot_x_vel());
	} else {
		x = static_cast<uint16_t>(x - wram_.shot_x_vel());
		if ((x >> 8) == 0xff) {
			// off the left edge
			ram(wGolfParaGoomXPos + 1) = 0x00;
			wram_.golf_terrain() = GOLF_TERRAIN_END;
			out_of_bounds = true;
		}
	}
	if (!out_of_bounds) {
		uint16_t block = static_cast<uint16_t>(x >> 5);
		wram_.unk_1dc3e().word() = block;
		uint8_t terrain = ram(static_cast<uint16_t>(wGolfTerrainBlocks + block));
		wram_.golf_terrain() = terrain;
		uint8_t kind = terrain & GOLF_TERRAIN_MASK;
		if (kind == GOLF_TERRAIN_TEE_BOX) {
			if (!wram_.unk_1dc47()) {
				wram_.ground_level_y_pos() = static_cast<uint8_t>(PARAGOOM_TEE_BOX_LEVEL);
				return;
			}
			if (terrain & GOLF_TERRAIN_FLAGS_MASK)
				wram_.golf_terrain() = GOLF_TERRAIN_TEE_OB;
			wram_.ground_level_y_pos() = GROUND_LO;
			return;
		}
		if (kind != GOLF_TERRAIN_END) {
			wram_.ground_level_y_pos() = GROUND_LO;
			return;
		}
	}

	// out of bounds
	wram_.golf_para_goom_duration() = 0;
	wario().duration() = 0;
	wram_.golf_para_goom_frameset_offset() = 0;
	wario().frameset_offset() = 0;
	wram_.golf_para_goom_state() = GOLF_PARAGOOM_8;
	wram_.golf_wario_state() = GOLF_WARIO_IDLING;
	wram_.golf_vblank_mode() = 0x02;
	wram_.golf_hole_state() = GOLFHOLESTATE_SPECIAL_TERRAIN;
	++wram_.golf_stroke();
}

// Gravity, and the landing: a bounce on the fairway or tee box, else the
// terrain's effect (penalty strokes for water and out of bounds).
void GolfHole::update_paragoom_y()
{
	Word y_vel = wram_.shot_y_vel().word();
	y_vel = static_cast<uint16_t>(y_vel - 1);
	Word y = wram_.golf_para_goom_y_pos().word();
	y = static_cast<uint16_t>(y - y_vel); // the y velocity points up
	if ((y >> 8) != GROUND_HI)
		return;
	uint8_t ground = wram_.ground_level_y_pos();
	if (ground >= static_cast<uint8_t>(y))
		return;

	// landed: on the ground
	y = static_cast<uint16_t>(GROUND_HI << 8 | ground);
	uint8_t terrain = wram_.golf_terrain();
	uint8_t kind = terrain & GOLF_TERRAIN_MASK;
	if (kind && kind < GOLF_TERRAIN_OB)
		wram_.unk_1dc47() = kind;
	wram_.unk_1dc51() = 0;
	if (kind < SPECIAL_GOLF_TERRAIN) {
		// the tee box or the fairway
		play_sfx(mem_, SFX_BUMP);
		uint8_t next = wram_.unk_1dc01();
		wram_.golf_hole_state() = next;
		if (next == GOLFHOLESTATE_BOUNCE)
			return;
		wram_.unk_1dc01() = GOLFHOLESTATE_BETWEEN_SHOTS;
		update_hole_direction();
		become_idle();
		return;
	}

	wram_.golf_hole_state() = GOLFHOLESTATE_SPECIAL_TERRAIN;
	become_idle();
	bool penalty = false;
	switch (kind) {
	case GOLF_TERRAIN_HOLE:
		play_sfx(mem_, SFX_0EF);
		break;
	case GOLF_TERRAIN_WATER:
		play_sfx(mem_, SFX_SPLASH);
		penalty = true;
		break;
	case GOLF_TERRAIN_OB:
		play_sfx(mem_, SFX_073);
		wram_.unk_1dc47() = wram_.unk_1dc53();
		penalty = true;
		break;
	case GOLF_TERRAIN_TEE_OB:
		++wram_.golf_stroke();
		return;
	case GOLF_TERRAIN_BUNKER:
		play_sfx(mem_, SFX_065);
		wram_.golf_para_goom_state() = GOLF_PARAGOOM_5;
		return;
	default: // rough
		play_sfx(mem_, SFX_0A1);
		return;
	}
	if (penalty) {
		wram_.golf_para_goom_state() = GOLF_PARAGOOM_6;
		++wram_.golf_stroke();
	}

	// at the edge of a block: snaps to it
	uint8_t block_lo;
	if (terrain & (1 << GOLF_TERRAIN_EDGE_RIGHT_F))
		block_lo = wram_.unk_1dc3e();
	else if (terrain & (1 << GOLF_TERRAIN_EDGE_LEFT_F))
		block_lo = static_cast<uint8_t>(wram_.unk_1dc3e() + 1);
	else
		return;
	uint16_t block = static_cast<uint16_t>(ram(w1dc3e + 1) << 8 | block_lo);
	wram_.golf_para_goom_x_pos().word() = static_cast<uint16_t>(block << 5);
}

void GolfHole::become_idle()
{
	wram_.golf_para_goom_duration() = 0;
	wario().duration() = 0;
	wram_.golf_para_goom_frameset_offset() = 0;
	wario().frameset_offset() = 0;
	wram_.golf_para_goom_state() = GOLF_PARAGOOM_0;
	wram_.golf_wario_state() = GOLF_WARIO_IDLING;
	wram_.golf_vblank_mode() = 0x02;
}

// After a splash or out of bounds, the Para-Goom comes back: at the water's
// respawn block, or where it was shot from.
bool GolfHole::respawn_paragoom()
{
	uint8_t state = wram_.golf_para_goom_state();
	if (state == GOLF_PARAGOOM_2) {
		wram_.last_golf_para_goom_y_pos().word() = PARAGOOM_GROUND_LEVEL;
		wram_.golf_para_goom_y_pos().word() = 0;
		uint8_t block = wram_.golf_terrain() & GOLF_TERRAIN_2ND_WATER ? wram_.golf_water_spawn2() : wram_.golf_water_spawn1();
		wram_.golf_para_goom_x_pos().word() = static_cast<uint16_t>(block * 32);
		align_x_scroll();
		update_display_mode();
		wram_.golf_para_goom_frameset_offset() = 0;
		wram_.golf_para_goom_state() = GOLF_PARAGOOM_4;
		set_respawn_y_if_enough_strokes();
		return true;
	}
	if (state != GOLF_PARAGOOM_3)
		return false;
	wram_.golf_para_goom_y_pos().word() = 0;
	ram(wGolfParaGoomXPos + 0) = ram(wLastGolfParaGoomXPos + 0);
	ram(wGolfParaGoomXPos + 1) = ram(wLastGolfParaGoomXPos + 1);
	wram_.golf_para_goom_state() = GOLF_PARAGOOM_8;
	set_respawn_y_if_enough_strokes();
	return true;
}

// With no strokes left, the Para-Goom stays on the ground.
void GolfHole::set_respawn_y_if_enough_strokes()
{
	if (wram_.golf_stroke() < wram_.golf_max_strokes())
		return;
	wram_.golf_para_goom_y_pos().word() = PARAGOOM_GROUND_LEVEL;
	wram_.unk_1dc20() = TRUE;
}

// Which way the hole is from the Para-Goom, and where Wario stands to hit
// it (64 to its other side).
void GolfHole::update_hole_direction()
{
	uint16_t x = wram_.golf_para_goom_x_pos().word();
	bool past_hole = compare16(x, ram_word(static_cast<uint16_t>(wGolfParaGoomXPos + 2))) >= 2;
	wram_.golf_hole_direction() = past_hole ? 1 : 0;
	wram_.golf_wario_target_x_pos().word() = static_cast<uint16_t>(past_hole ? x + 0x40 : x - 0x40);
}

// ---------------------------------------------------------------------------
// The screen

// The view follows the Para-Goom: 80 pixels from the left, within the hole.
void GolfHole::align_x_scroll()
{
	int x = wram_.golf_para_goom_x_pos().word() >> 2;
	int scroll = x - 0x50;
	if (scroll < 0)
		scroll = 0;
	if (scroll > 0x160)
		scroll = 0x160;
	wram_.golf_x_scroll().word() = static_cast<uint16_t>(scroll);
}

// The hole is 2 BG maps wide: the first, both (split with the window), or
// the second.
void GolfHole::update_display_mode()
{
	uint16_t scroll = wram_.golf_x_scroll().word();
	uint8_t lo = static_cast<uint8_t>(scroll);
	wram_.scx() = lo;
	if (scroll >> 8) {
		wram_.golf_display_mode() = GOLF_DISPLAY_BGMAP1;
	} else if (lo < static_cast<uint8_t>(MAX_GOLF_X_SCROLL)) {
		wram_.golf_display_mode() = GOLF_DISPLAY_BGMAP0;
	} else {
		wram_.golf_wx() = static_cast<uint8_t>(167 - lo + 89 + WX_OFS);
		wram_.golf_display_mode() = GOLF_DISPLAY_SPLIT;
	}
}

// Func_1c8f56: whether x (a quarter of a position) is on screen.
bool GolfHole::in_view(uint16_t x)
{
	uint16_t relative = static_cast<uint16_t>(x - wram_.golf_x_scroll().word());
	uint8_t hi = static_cast<uint8_t>(relative >> 8), lo = static_cast<uint8_t>(relative);
	if (hi == 0)
		return lo < 0xb0;
	return hi == 0xff && lo >= 0xe0;
}

// The power gauge, the shot's markers and the menu bar
void GolfHole::update_ui()
{
	update_object(object(wGolfObj1), 0x30, 0x0c, ADDR_Frameset_1caf6f);
	update_object(object(wGolfObj2), 0x30, 0x08, ADDR_Frameset_1caf81);
	if (wram_.golf_hole_state() == GOLFHOLESTATE_PAUSE)
		return;
	update_object(object(wGolfObj3), SCREEN_HEIGHT_PX, 0x58, ADDR_Frameset_1caf8a);
	if (object(wGolfObj4).state())
		return;
	update_object(object(wGolfObj4), SCREEN_HEIGHT_PX, static_cast<uint8_t>(wram_.shot_power_gauge_cursor() + 0x32),
		ADDR_Frameset_1caf84);
	if (!object(wGolfObj5).state())
		return;
	update_object(object(wGolfObj5), SCREEN_HEIGHT_PX, static_cast<uint8_t>(wram_.selected_shot_power_gauge() + 0x32),
		ADDR_Frameset_1caf87);
}

void GolfHole::update_paragoom()
{
	if (wram_.unk_1dc20())
		return;
	if (wram_.golf_para_goom_state() >= GOLF_PARAGOOM_8)
		return;
	uint16_t x = static_cast<uint16_t>(wram_.golf_para_goom_x_pos().word() >> 2);
	if (!in_view(x))
		return;
	GolfObject paragoom = object(wGolfParaGoom);
	paragoom.x() = static_cast<uint8_t>(static_cast<uint8_t>(x) + OAM_X_OFS - wram_.scx());
	uint8_t state = wram_.golf_para_goom_state();
	if (state >= GOLF_PARAGOOM_8)
		return;
	uint8_t y_offset = state == GOLF_PARAGOOM_5 ? 0x13 : 0x12;
	paragoom.y() = static_cast<uint8_t>(static_cast<uint8_t>(wram_.golf_para_goom_y_pos().word() >> 2) + y_offset);

	uint16_t frameset = mem_.word(static_cast<uint16_t>(PARAGOOM_FRAMESETS + static_cast<uint8_t>(state << 1)));
	if (routines_.update_golf_object_animation(frameset, paragoom.duration().addr()) && respawn_paragoom())
		return;
	if (wram_.golf_para_goom_state() == 0x02 && !paragoom.duration()) {
		uint8_t offset = paragoom.frameset_offset();
		if (offset == 0x1e)
			play_sfx(mem_, SFX_0E5);
		else if (offset == 0x14)
			play_sfx(mem_, SFX_00F);
	}
	load_object_sprite(paragoom);
}

void GolfHole::update_wario()
{
	uint16_t x = static_cast<uint16_t>(wram_.golf_wario_x_pos().word() >> 2);
	if (!in_view(x)) {
		if (!wram_.unk_1dc21())
			return;
		// off screen after the view jumped: just outside it, on his side
		wram_.unk_1dc21() = 0;
		uint16_t scroll = static_cast<uint16_t>(wram_.golf_x_scroll().word() << 2);
		wram_.golf_wario_x_pos().word() = static_cast<uint16_t>(wram_.golf_wario_dir() ? scroll + 680 : scroll - 40);
		return;
	}
	wram_.unk_1dc21() = 0;
	wram_.golf_wario_x_coord() = static_cast<uint8_t>(static_cast<uint8_t>(x) + OAM_X_OFS - wram_.scx());
	wram_.golf_wario_y_coord() = static_cast<uint8_t>(wram_.golf_wario_y_pos() + 0x11);
	routines_.update_golf_wario_animation();
	if (wram_.golf_wario_state() == GOLF_WARIO_WALKING && wario().frameset_offset() == 0x2 && !wario().duration())
		play_sfx(mem_, SFX_WALK);
}

void GolfHole::show_result_overlay_text()
{
	GolfObject text = object(wGolfOverlayText);
	text.y() = 0x50;
	text.x() = 0x58;
	uint16_t frameset = mem_.word(static_cast<uint16_t>(RESULT_FRAMESETS + static_cast<uint8_t>(wram_.golf_result() * 2)));
	routines_.update_golf_object_animation(frameset, text.duration().addr());
	load_object_sprite(text);
}

// The strokes over or under par, in the overlay text's sprites
void GolfHole::show_score()
{
	uint8_t result = wram_.golf_result();
	if (result == GOLFRESULT_PAR)
		return;
	uint8_t value;
	if (result == GOLFRESULT_OVER_PAR) {
		value = static_cast<uint8_t>(wram_.golf_stroke() - wram_.golf_par());
		if (value >= 10) {
			wram_.virtual_oam_sprite01_frame() = 0x82;
			value = static_cast<uint8_t>(value - 10);
		}
	} else {
		value = static_cast<uint8_t>(wram_.golf_par() - wram_.golf_stroke());
	}
	wram_.virtual_oam_sprite02_frame() = static_cast<uint8_t>(value * 2 + 0x80);
}

bool GolfHole::animate_overlay_text(uint16_t frameset)
{
	return routines_.update_golf_object_animation(frameset, object(wGolfOverlayText).duration().addr());
}

// The Frameset_1cb016 text, with its sound; true when it has finished.
bool GolfHole::animate_overlay_text_1cb016()
{
	GolfObject text = object(wGolfOverlayText);
	if (animate_overlay_text(ADDR_Frameset_1cb016))
		return true;
	if (text.frameset_offset() == 0x2 && text.duration() == 1)
		play_sfx(mem_, SFX_0E5);
	load_object_sprite(text);
	return false;
}

void GolfHole::load_wario_state_gfx()
{
	uint8_t state = wram_.golf_wario_state();
	wram_.golf_wario_tiles_bank() = mem_[static_cast<uint16_t>(WARIO_GFX_BANKS + state)];
	uint16_t pointer = static_cast<uint16_t>(WARIO_GFX_POINTERS + static_cast<uint8_t>(state << 1));
	// stored high byte first
	ram(wGolfWarioTilesPtr + 1) = mem_[pointer];
	ram(wGolfWarioTilesPtr + 0) = mem_[static_cast<uint16_t>(pointer + 1)];
}

// ---------------------------------------------------------------------------
// The V-Blank functions

void GolfHole::copy_vblank_func(uint16_t func, uint8_t length)
{
	routines_.copy(func, wVBlankFunc, length);
}

// for the V-Blank functions that DMA an attribute map
void GolfHole::set_attrmap_dma()
{
	ram(wdc11 + 0) = static_cast<uint8_t>(((v1BGMap0 + 6 * TILEMAP_WIDTH) >> 8) - 0x80);
	ram(wdc11 + 1) = static_cast<uint8_t>(v1BGMap0 + 6 * TILEMAP_WIDTH);
	wram_.unk_1dc13() = 0x0d; // 7 dma_rows
}

// Func_1c9cf9
void GolfHole::set_hole_vblank()
{
	uint8_t mode = wram_.golf_display_mode();
	if (mode == GOLF_DISPLAY_SPLIT)
		wram_.golf_lcd_config() = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_ON | LCDC_WIN_9C00 | LCDC_ON;
	else if (mode == GOLF_DISPLAY_BGMAP1)
		wram_.golf_lcd_config() = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_BG_9C00 | LCDC_ON;
	else
		wram_.golf_lcd_config() = LCDC_DEFAULT;

	switch (wram_.golf_vblank_mode()) {
	case 0: {
		// the waving pin flag, 4 frames of 8
		uint8_t counter = wram_.unk_1dc0b();
		uint8_t frame = counter >= 0x18 ? 0x0c : counter >= 0x10 ? 0x08 : counter >= 0x08 ? 0x04 : 0x00;
		routines_.copy(static_cast<uint16_t>(ADDR_GolfPinFlagGfx + frame * 16), wGolfPinFlagTiles, 4 * 16);
		wram_.unk_1dc0b() = static_cast<uint8_t>((wram_.unk_1dc0b() + 1) & 0x1f);
		copy_vblank_func(ADDR_VBlank_1c9d1d + 0x0e, 0x2f);
		break;
	}
	case 1: copy_vblank_func(ADDR_VBlank_1c9e8d + 0x0b, 0x1b); break;
	case 2: copy_vblank_func(ADDR_VBlank_1c9eb3 + 0x0b, 0x33); break;
	case 3: copy_vblank_func(ADDR_VBlank_1c9ef1 + 0x0b, 0x32); break;
	default: unrealistic(); break;
	}
}

// Func_1c9fae
void GolfHole::set_building_vblank()
{
	set_coin_sprites();
	switch (wram_.golf_vblank_mode()) {
	case 0: copy_vblank_func(ADDR_VBlank_1c9fe1 + 0x0b, 0x0e); break;
	case 1:
		set_attrmap_dma();
		copy_vblank_func(ADDR_VBlank_1c9ffa + 0x1a, 0x1f);
		break;
	case 2: copy_vblank_func(ADDR_VBlank_1ca033 + 0x0b, 0x18); break;
	case 3:
		set_attrmap_dma();
		copy_vblank_func(ADDR_VBlank_1ca056 + 0x1a, 0x1f);
		break;
	default: unrealistic(); break;
	}
}

// GolfLobbyVBlank
void GolfHole::set_lobby_vblank()
{
	switch (wram_.golf_vblank_mode()) {
	case 0: copy_vblank_func(ADDR_VBlank_1c9f36 + 0x0b, 0x39); break;
	case 1: copy_vblank_func(ADDR_VBlank_1c9f7a + 0x0b, 0x29); break;
	default: unrealistic(); break;
	}
}

// Func_1c9fbd: the coin count's digits (BCD) in the first 3 sprites
void GolfHole::set_coin_sprites()
{
	uint8_t hundreds = ram(wGolfNumCoins + 0), tens_ones = ram(wGolfNumCoins + 1);
	wram_.virtual_oam_sprite00_frame() = static_cast<uint8_t>(hundreds * 2 + 0xa0);
	wram_.virtual_oam_sprite01_frame() = static_cast<uint8_t>((tens_ones >> 4) * 2 + 0xa0);
	wram_.virtual_oam_sprite02_frame() = static_cast<uint8_t>((tens_ones & 0x0f) * 2 + 0xa0);
}

} // namespace wl3::golf

// unreferenced
PORT_EX(Func_1c8957, 0, ALL_REGISTERS)
{
	wl3::golf::GolfHole(gb).run_unreferenced_state();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitGolfHole);
PORT_COVERED(HandleGolfHole);
PORT_COVERED(PickAndLoadGolfHole);
PORT_COVERED(ShowGolfHoleFlagAndPar);
PORT_COVERED(SetUpGolfHoleObjects);
PORT_COVERED(PrintGolfStrokeNumber);
PORT_COVERED(GolfHoleStateTable);
PORT_COVERED(GolfHoleState_WaitInput);
PORT_COVERED(GolfHoleState_Scroll);
PORT_COVERED(GolfHoleState_SelectShotPower);
PORT_COVERED(GolfHoleState_SelectShotSpin);
PORT_COVERED(GolfHoleState_StartShot);
PORT_COVERED(GolfHoleState_Shot);
PORT_COVERED(GolfHoleState_Bounce);
PORT_COVERED(GolfHoleState_SpecialTerrain);
PORT_COVERED(GolfHoleState_Walk);
PORT_COVERED(GolfHoleState_BetweenShots);
PORT_COVERED(GolfHoleState_Cleared);
PORT_COVERED(GolfHoleState_GameOver);
PORT_COVERED(GolfHoleState_Pause);
PORT_COVERED(GolfHoleState_Result);
PORT_COVERED(UpdateGolfParaGoomXPos);
PORT_COVERED(UpdateGolfParaGoomYPos);
PORT_COVERED(HandleGolfParaGoomRespawn);
PORT_COVERED(SetGolfParaGoomRespawnYPosIfEnoughStrokes);
PORT_COVERED(UpdateGolfHoleDirection);
PORT_COVERED(AlignGolfXScrollWithParaGoom);
PORT_COVERED(UpdateGolfDisplayMode);
PORT_COVERED(Func_1c8f56);
PORT_COVERED(Sub16Bit);
PORT_COVERED(Compare16Bit);
PORT_COVERED(UpdateGolfHoleUI);
PORT_COVERED(UpdateGolfParaGoom);
PORT_COVERED(UpdateGolfWario);
PORT_COVERED(ShowGolfResultOverlayText);
PORT_COVERED(ShowGolfScore);
PORT_COVERED(LoadGolfWarioStateGfx);
PORT_COVERED(Func_1c8fb7);
PORT_COVERED(Func_1c9cf9);
PORT_COVERED(Func_1c9fae);
PORT_COVERED(Func_1c9fbd);
PORT_COVERED(GolfLobbyVBlank);
PORT_COVERED(VBlank_1c9d1d);
PORT_COVERED(VBlank_1c9e8d);
PORT_COVERED(VBlank_1c9eb3);
PORT_COVERED(VBlank_1c9ef1);
PORT_COVERED(VBlank_1c9f36);
PORT_COVERED(VBlank_1c9f7a);
PORT_COVERED(VBlank_1c9fe1);
PORT_COVERED(VBlank_1c9ffa);
PORT_COVERED(VBlank_1ca033);
PORT_COVERED(VBlank_1ca056);
