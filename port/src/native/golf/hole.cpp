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
	ram(wRoomAnimatedTilesEnabled) = 0;

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

	uint16_t pals = mem_.word(static_cast<uint16_t>(COURSE_PALS + static_cast<uint8_t>(ram(wGolfMenuOption) * 2)));
	routines_.load_pals_to_temp_pals1(pals);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_1ca18f);

	ram(rVBK) = VRAM1;
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfFlagGfx, v1Tiles0);
	ram(rVBK) = 0;
	routines_.far_decompress(BANK_GolfGfx, ADDR_GolfGfx, v0Tiles1);
	ram(wTempBank) = BANK_WarioIdleGfx;
	routines_.far_copy(ADDR_WarioIdleGfx, v0Tiles0, 0x80 * TILE_SIZE);

	reset_scroll();
	ram(rWY) = 0;
	ram(rWX) = 159 + WX_OFS;
	ram(wGolfWX) = 159 + WX_OFS;

	set_up_hole();
	set_hole_vblank();
	play_music2(mem_, MUSIC_GOLF);
	ram(rLCDC) = LCDC_DEFAULT; // (set_hole_vblank already chose it)
	++ram(wSubState);
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
	ram(wTempBank) = BANK_GolfGfx;
	uint8_t hole;
	if (uint8_t predetermined = ram(wPredeterminedGolfHole)) {
		hole = static_cast<uint8_t>(predetermined - 1);
	} else {
		ram(wGolfHoleTier) = GOLF_HOLE_TIER_3;
		uint8_t treasures = ram(wNumCollectedTreasures + 1); // BCD
		if (!ram(wNumCollectedTreasures + 0) && treasures < GOLF_TREASURES_TIER_3)
			ram(wGolfHoleTier) = treasures < GOLF_TREASURES_TIER_2 ? GOLF_HOLE_TIER_1 : GOLF_HOLE_TIER_2;
		uint8_t tier = ram(wGolfHoleTier);
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
	if (ram(w1d800) != 0x02) {
		ram(wGolfMaxStrokes) = static_cast<uint8_t>(ram(wGolfPar) + MAX_GOLF_COURSE_STROKES);
		number = ram(wGolfCourseHole);
	} else {
		// in the Golf Building, the strokes can't exceed the par
		ram(wGolfMaxStrokes) = ram(wGolfPar);
		number = static_cast<uint8_t>(ram(wGolfPar) - 1);
	}
	auto tiles = [](uint8_t tile) { return static_cast<uint16_t>(v1Tiles0 + tile * 0x10); };
	routines_.copy(static_cast<uint16_t>(ADDR_GolfFlagNumbersGfx + static_cast<uint8_t>(number << 4 | number >> 4)),
		tiles(0x01), 0x10);

	uint8_t par = static_cast<uint8_t>(ram(wGolfPar) - 1);
	uint8_t par_gfx = static_cast<uint8_t>((par << 4 | par >> 4) - 2 * 0x10);
	for (uint8_t tile : {0x2b, 0x4b, 0x6b, 0x8b})
		routines_.copy(static_cast<uint16_t>(ADDR_GolfParNumbersGfx + par_gfx), tiles(tile), 0x10);

	if (ram(w1d800) != 0x02) {
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
	ram(wShotPowerGaugeCursor) = BASE_SHOT_POWER;
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
	uint8_t stroke = ram(wGolfStroke);
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
	switch (ram(wGolfHoleState)) {
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
	switch (ram(wSubState)) {
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
	uint16_t scroll = ram_word(wGolfXScroll);
	uint8_t pressed = ram(wJoypadPressed);
	if (pressed & PAD_SELECT) {
		play_sfx(mem_, SFX_0E4);
		object(wGolfObj3).duration() = 0;
		object(wGolfObj3).frameset_offset() = 0;
		ram(wGolfHoleState) = GOLFHOLESTATE_PAUSE;
		return;
	}
	if (pressed & PAD_A) {
		play_sfx(mem_, SFX_0E9);
		for (uint16_t i = 0; i < 8; i++) // wGolfOverlayText
			ram(static_cast<uint16_t>(wGolfOverlayText + i)) = 0;
		ram(wGolfResult) = 0;
		ram(wGolfHoleState) = GOLFHOLESTATE_SELECT_SHOT_POWER;
		ram(wStrokeGolfTerrain) = ram(wGolfTerrain);
		ram(w1dc53) = ram(w1dc47);
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
	ram(wGolfHoleScrollingDir) = dir;
	ram(wTempGolfXScroll + 0) = ram(wGolfXScroll + 0);
	ram(wTempGolfXScroll + 1) = ram(wGolfXScroll + 1);
	ram(wGolfHoleState) = GOLFHOLESTATE_SCROLL;
}

// Looking along the hole, with a ghost Para-Goom where the shot would land
// at the default power.
void GolfHole::state_scroll()
{
	scroll_by_input();

	uint16_t x = static_cast<uint16_t>(ram_word(wGolfParaGoomXPos) >> 2);
	uint8_t terrain = ram(wGolfTerrain);
	uint8_t distance = terrain == GOLF_TERRAIN_ROUGH ? 0x80 : terrain == GOLF_TERRAIN_BUNKER ? 0x50 : 0xa0;
	x = static_cast<uint16_t>(ram(wGolfHoleDirection) ? x - distance : x + distance);
	if (!in_view(x))
		return;
	GolfObject ghost = object(wGolfParaGoomGhost);
	ghost.x() = static_cast<uint8_t>(static_cast<uint8_t>(x) + OAM_X_OFS - ram(wSCX));
	ghost.y() = 0x72;
	routines_.update_golf_object_animation(ADDR_Frameset_1cb0a4, ghost.duration().addr());
	load_object_sprite(ghost);
}

// A or B goes back; the d-pad scrolls further (while held), else it scrolls
// back to where it started.
void GolfHole::scroll_by_input()
{
	if (ram(wJoypadPressed) & (PAD_A | PAD_B)) {
		ram(wGolfXScroll + 0) = ram(wTempGolfXScroll + 0);
		ram(wGolfXScroll + 1) = ram(wTempGolfXScroll + 1);
		ram(wGolfHoleState) = GOLFHOLESTATE_WAIT_INPUT;
		update_display_mode();
		return;
	}
	uint16_t scroll = ram_word(wGolfXScroll);
	bool left = ram(wGolfHoleScrollingDir);
	uint8_t down = ram(wJoypadDown);
	if (!left && (down & PAD_RIGHT)) {
		if ((scroll >> 8) && static_cast<uint8_t>(scroll) >= static_cast<uint8_t>(MAX_GOLF_X_SCROLL))
			return; // can't scroll further right
		ram_word(wGolfXScroll) = static_cast<uint16_t>(scroll + 1);
		update_display_mode();
		return;
	}
	if (left && (down & PAD_LEFT)) {
		if (!scroll)
			return; // can't scroll further left
		ram_word(wGolfXScroll) = static_cast<uint16_t>(scroll - 1);
		update_display_mode();
		return;
	}
	// scroll back
	scroll = static_cast<uint16_t>(left ? scroll + 1 : scroll - 1);
	ram_word(wGolfXScroll) = scroll;
	if (scroll == ram_word(wTempGolfXScroll))
		ram(wGolfHoleState) = GOLFHOLESTATE_WAIT_INPUT;
	update_display_mode();
}

// The power gauge rises (faster in its upper half) and falls back; A picks
// the power. Letting it fall to 0 is a miss.
void GolfHole::state_select_shot_power()
{
	Byte cursor = ram(wShotPowerGaugeCursor);
	if (!ram(wIsPowerGaugeFalling)) {
		uint8_t power = static_cast<uint8_t>(cursor + (cursor >= 16 ? 2 : 1));
		cursor = power;
		if (power >= MAX_SHOT_POWER) {
			play_sfx(mem_, SFX_0EA);
			ram(wIsPowerGaugeFalling) = TRUE;
		}
	} else {
		uint8_t power = cursor;
		cursor = static_cast<uint8_t>(power - 2);
		if (power < 2) {
			ram(wGolfCounter) = 0;
			cursor = 0;
			ram(wGolfHoleState) = GOLFHOLESTATE_START_SHOT;
			return;
		}
	}
	if (!(ram(wJoypadPressed) & PAD_A))
		return;

	wario().duration() = 0;
	wario().frameset_offset() = 0;
	object(wGolfObj5).state() = 1;
	ram(wGolfHoleState) = GOLFHOLESTATE_SELECT_SHOT_SPIN;
	uint8_t power = cursor;
	ram(wSelectedShotPowerGauge) = power;
	static const uint8_t limits[] = {16, 24, 32, 40, 48, 56, 64, 70, 74};
	static const uint8_t levels[] = {
		SHOT_POWER_0, SHOT_POWER_1, SHOT_POWER_2, SHOT_POWER_3, SHOT_POWER_4,
		SHOT_POWER_5, SHOT_POWER_6, SHOT_POWER_7, SHOT_POWER_8, SHOT_POWER_9,
	};
	unsigned level = 0;
	while (level < sizeof(limits) && power >= limits[level])
		level++;
	ram(wShotPowerLevel) = levels[level];

	uint16_t vels = static_cast<uint16_t>(SHOT_POWER_VELS + static_cast<uint8_t>(ram(wShotPowerLevel) << 1));
	ram(wShotYVel + 0) = mem_[vels];
	ram(wShotYVel + 1) = 0;
	ram(wShotXVel) = mem_[static_cast<uint16_t>(vels + 1)];

	// a shot from the rough or a bunker goes less high
	uint8_t terrain = ram(wGolfTerrain);
	uint8_t y_vel = ram(wShotYVel);
	if (terrain == GOLF_TERRAIN_ROUGH) {
		ram(wShotYVel) = static_cast<uint8_t>(static_cast<uint8_t>(y_vel * 3 + 3) >> 2);
		ram(wShotTerrainModifier) = SHOT_MODIFIER_ROUGH;
	} else if (terrain == GOLF_TERRAIN_BUNKER) {
		ram(wShotYVel) = static_cast<uint8_t>((static_cast<uint8_t>(y_vel + 1) >> 1) - 1);
		ram(wShotTerrainModifier) = SHOT_MODIFIER_BUNKER;
	} else {
		ram(wShotTerrainModifier) = SHOT_MODIFIER_NONE;
	}
}

// The gauge again, for the spin: A picks it by how far it went.
void GolfHole::state_select_shot_spin()
{
	Byte gauge = ram(wSelectedShotPowerGauge);
	bool check_a = true;
	if (!ram(wIsPowerGaugeFalling)) {
		uint8_t value = static_cast<uint8_t>(gauge + (gauge >= 16 ? 2 : 1));
		gauge = value;
		if (value >= MAX_SHOT_POWER) {
			play_sfx(mem_, SFX_0EA);
			ram(wIsPowerGaugeFalling) = TRUE;
		}
	} else {
		uint8_t value = gauge;
		gauge = static_cast<uint8_t>(value - 2);
		if (value < 2) {
			gauge = 0;
			check_a = false;
		}
	}
	if (check_a && !(ram(wJoypadPressed) & PAD_A))
		return;

	ram(wGolfCounter) = 0;
	ram(wGolfHoleState) = GOLFHOLESTATE_START_SHOT;
	uint8_t value = gauge;
	uint8_t spin = SHOT_MISS;
	if (value >= 2 && value < 5)
		spin = SHOT_BACKSPIN;
	else if (value >= 5 && value < 11)
		spin = SHOT_NORMAL;
	else if (value >= 11 && value < 14)
		spin = SHOT_TOPSPIN;
	ram(wShotSpin) = spin;
}

// Wario's swing: wGolfCounter steps it (the animation advances it too).
void GolfHole::state_start_shot()
{
	uint8_t counter = ram(wGolfCounter);
	switch (counter) {
	case 0:
		ram(wGolfCounter) = 1;
		++ram(wGolfStroke);
		ram(wGolfVBlankMode) = 0x03;
		print_stroke_number();
		return;
	case 1:
		ram(wGolfCounter) = 2;
		play_sfx(mem_, SFX_0ED);
		wario().duration() = 0;
		wario().frameset_offset() = 0;
		ram(wGolfWarioState) = GOLF_WARIO_CHARGING;
		ram(wGolfVBlankMode) = 0x02;
		return;
	case 3:
		ram(wGolfCounter) = 4;
		wario().duration() = 0;
		wario().frameset_offset() = 0;
		if (ram(wShotPowerGaugeCursor) && ram(wShotSpin)) {
			play_sfx(mem_, SFX_017);
			ram(wGolfWarioState) = GOLF_WARIO_ATTACKING_1;
		} else {
			play_sfx(mem_, SFX_0E8);
			ram(wGolfWarioState) = GOLF_WARIO_MISSING;
			ram(wGolfVBlankMode) = 0x02;
		}
		return;
	}
	if (counter < 5)
		return;

	ram(wGolfCounter) = 0;
	wario().duration() = 0;
	wario().frameset_offset() = 0;
	if (ram(wGolfWarioState) == GOLF_WARIO_MISSING) {
		ram(wGolfVBlankMode) = 0x02;
		ram(wGolfWarioState) = GOLF_WARIO_IDLING;
		ram(wGolfHoleState) = GOLFHOLESTATE_BETWEEN_SHOTS;
		return;
	}
	ram(wShotDirection) = ram(wGolfHoleDirection);
	ram(wGolfParaGoomFramesetOffset) = 0;
	ram(wGolfParaGoomState) = GOLF_PARAGOOM_1;
	ram(wGolfWarioState) = GOLF_WARIO_ATTACKING_2;
	ram(w1dc01) = GOLFHOLESTATE_BOUNCE;
	ram(wGolfHoleState) = GOLFHOLESTATE_SHOT;
}

// The Para-Goom flies; when it lands (and bounces), the bounce gets its
// velocities by the shot's power and spin.
void GolfHole::state_shot()
{
	update_paragoom_x();
	update_paragoom_y();
	align_x_scroll();
	update_display_mode();
	if (ram(wGolfHoleState) == GOLFHOLESTATE_SHOT)
		return;

	ram(w1dc01) = GOLFHOLESTATE_WALK;
	uint8_t spin = ram(wShotSpin);
	uint16_t table = NORMAL_BOUNCE_VELS;
	if (spin == SHOT_TOPSPIN)
		table = TOPSPIN_BOUNCE_VELS;
	else if (spin != SHOT_NORMAL)
		ram(wShotDirection) ^= 0x1; // backspin bounces back
	uint16_t vels = static_cast<uint16_t>(table + static_cast<uint8_t>(ram(wShotPowerLevel) << 1));
	ram(wShotYVel + 0) = mem_[vels];
	ram(wShotYVel + 1) = 0;
	ram(wShotXVel) = mem_[static_cast<uint16_t>(vels + 1)];

	uint8_t modifier = ram(wShotTerrainModifier);
	uint8_t y_vel = ram(wShotYVel);
	if (modifier == SHOT_MODIFIER_ROUGH)
		ram(wShotYVel) = static_cast<uint8_t>(static_cast<uint8_t>(y_vel * 3 + 3) >> 2);
	else if (modifier == SHOT_MODIFIER_BUNKER)
		ram(wShotYVel) = static_cast<uint8_t>(static_cast<uint8_t>(y_vel + 1) >> 1);
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
	ram(w1dc01) = GOLFHOLESTATE_BETWEEN_SHOTS;

	bool rise = false; // .asm_1c9743: back up from under the ground
	uint8_t terrain = ram(wGolfTerrain) & GOLF_TERRAIN_MASK;
	switch (terrain) {
	case GOLF_TERRAIN_HOLE: {
		uint8_t counter = ram(wGolfCounter);
		if (counter < 0x30) {
			ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
			if (counter >= 0x18)
				return; // a delay
			// rolling into the hole
			Word x = ram_word(wGolfParaGoomXPos);
			uint8_t offset = mem_[static_cast<uint16_t>(HOLE_X_OFFSETS + counter)];
			x = static_cast<uint16_t>(mem_[static_cast<uint16_t>(HOLE_X_OFFSET_SIGNS + counter)] ? x - offset : x + offset);
			ram(wGolfParaGoomYPos) =
				static_cast<uint8_t>(ram(wGolfParaGoomYPos) + mem_[static_cast<uint16_t>(HOLE_Y_OFFSETS + counter)]);
			return;
		}
		uint8_t stroke = ram(wGolfStroke), par = ram(wGolfPar);
		uint8_t next = GOLFHOLESTATE_CLEARED;
		if (stroke == par) {
			play_music2(mem_, MUSIC_GOLF_CLEAR);
			ram(wGolfResult) = GOLFRESULT_PAR;
		} else if (stroke < par) {
			play_music2(mem_, MUSIC_GOLF_CLEAR);
		} else {
			ram(wGolfResult) = GOLFRESULT_OVER_PAR;
			if (ram(w1d800) != 0x01)
				next = GOLFHOLESTATE_GAME_OVER;
		}
		ram(w1dc01) = next;
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
		uint8_t state = ram(wGolfParaGoomState);
		if (state == GOLF_PARAGOOM_4) {
			rise = true;
			break;
		}
		if (state == GOLF_PARAGOOM_6) {
			uint8_t y = ram(wGolfParaGoomYPos);
			if (y < (water ? 0xc0 : 0xa0)) {
				ram(wGolfParaGoomYPos) = static_cast<uint8_t>(y + 4); // sinking
				return;
			}
			if (water) {
				ram(wGolfParaGoomState) = GOLF_PARAGOOM_2;
				ram(wStrokeGolfTerrain) = GOLF_TERRAIN_PLAIN;
				return;
			}
			ram(wGolfParaGoomState) = GOLF_PARAGOOM_3;
		} else if (water) {
			return;
		}
		if (!animate_overlay_text_1cb016())
			return;
		ram(wGolfParaGoomState) = GOLF_PARAGOOM_4;
		align_x_scroll();
		update_display_mode();
		update_hole_direction();
		return;
	}

	default:
		if (ram(wGolfParaGoomState) == GOLF_PARAGOOM_4) {
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
		ram(wGolfParaGoomFramesetOffset) = 0;
		ram(wGolfParaGoomState) = GOLF_PARAGOOM_4;
		set_respawn_y_if_enough_strokes();
		return;
	}

	if (rise) {
		if (!ram_word(wGolfParaGoomYPos))
			play_sfx(mem_, SFX_0EE);
		uint16_t last = ram_word(wLastGolfParaGoomYPos);
		Word y = ram_word(wGolfParaGoomYPos);
		y = static_cast<uint16_t>(y + 2);
		if (compare16(last, y) == 2)
			return;
		ram(wGolfTerrain) = ram(wStrokeGolfTerrain);
	}
	walk_to_paragoom();
}

// .asm_1c976f: Wario walks over to the Para-Goom
void GolfHole::walk_to_paragoom()
{
	if (ram(wGolfParaGoomState) != GOLF_PARAGOOM_5)
		ram(wGolfParaGoomState) = GOLF_PARAGOOM_0;
	ram(wGolfCounter) = 0;
	ram(wGolfParaGoomDuration) = 0;
	wario().duration() = 0;
	ram(wGolfParaGoomFramesetOffset) = 0;
	wario().frameset_offset() = 0;
	object(wGolfOverlayText).frameset_offset() = 0;
	object(wGolfOverlayText).state() = 0;
	update_hole_direction();
	ram(wGolfHoleState) = GOLFHOLESTATE_WALK;
}

// Wario walks to the Para-Goom (down from the tee box first), then the next
// state is w1dc01's.
void GolfHole::state_walk()
{
	uint8_t counter = ram(wGolfCounter);
	GolfObject golfer = wario();
	if (counter < 0x19) {
		if (counter < 0x18) {
			ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
			if (counter)
				return;
			ram(wGolfVBlankMode) = 0x03;
			ram(w1dc21) = 0x03;
			print_stroke_number();
			return;
		}
		if (ram(w1dc47) && ram(wGolfWarioYPos) < 0x60) {
			uint8_t ground = static_cast<uint8_t>(ram_word(wGolfParaGoomYPos) >> 2);
			if (ram(wGolfWarioYPos) < ground) {
				// down from the tee box
				uint8_t y = static_cast<uint8_t>(ram(wGolfWarioYPos) + 2);
				ram(wGolfWarioYPos) = y;
				if (y >= 0x60)
					ram(wGolfVBlankMode) = 0x01;
				return;
			}
		}
		if (compare16(ram_word(wGolfWarioXPos), ram_word(wGolfWarioTargetXPos)) == 1) {
			ram(wGolfCounter) = 0;
			ram(wGolfHoleState) = GOLFHOLESTATE_BETWEEN_SHOTS;
			return;
		}
		ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
		golfer.duration() = 0;
		golfer.frameset_offset() = 0;
		ram(wGolfWarioState) = GOLF_WARIO_WALKING;
		ram(wGolfVBlankMode) = 0x02;
		return;
	}

	// walking
	Word x = ram_word(wGolfWarioXPos);
	uint16_t target = ram_word(wGolfWarioTargetXPos);
	bool reached;
	uint16_t next;
	if (!ram(wGolfWarioDir)) {
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
	ram(wGolfWarioDir) = ram(wGolfHoleDirection);
	ram(wGolfCounter) = 0;
	golfer.duration() = 0;
	golfer.frameset_offset() = 0;

	uint8_t next_state = ram(w1dc01);
	uint8_t wario_state;
	if (next_state == GOLFHOLESTATE_CLEARED) {
		if (ram(wGolfResult) < GOLFRESULT_OVER_PAR) {
			wario_state = GOLF_WARIO_CLEARED;
		} else {
			play_music2(mem_, MUSIC_GOLF_GAME_OVER);
			wario_state = GOLF_WARIO_LOST;
		}
	} else {
		wario_state = next_state == GOLFHOLESTATE_GAME_OVER ? GOLF_WARIO_LOST : GOLF_WARIO_IDLING;
	}
	ram(wGolfWarioState) = wario_state;
	ram(wGolfVBlankMode) = 0x02;
	ram(wGolfHoleState) = ram(w1dc01);
}

// A pause before the next shot, or game over with too many strokes.
void GolfHole::state_between_shots()
{
	uint8_t counter = static_cast<uint8_t>(ram(wGolfCounter) + 1);
	ram(wGolfCounter) = counter;
	if (counter == 0x08) {
		if (ram(wGolfStroke) < ram(wGolfMaxStrokes))
			return;
		play_music2(mem_, MUSIC_GOLF_GAME_OVER);
		ram(wGolfCounter) = 0;
		wario().duration() = 0;
		wario().frameset_offset() = 0;
		object(wGolfOverlayText).duration() = 0;
		object(wGolfOverlayText).frameset_offset() = 0;
		object(wGolfObj4).state() = 0x01;
		ram(wGolfWarioState) = GOLF_WARIO_LOST;
		ram(wGolfVBlankMode) = 0x02;
		ram(wGolfHoleState) = GOLFHOLESTATE_GAME_OVER;
		return;
	}
	if (counter < 0x20)
		return;
	ram(wShotPowerGaugeCursor) = BASE_SHOT_POWER;
	ram(wGolfCounter) = 0;
	ram(w1dc51) = 0;
	ram(wIsPowerGaugeFalling) = 0;
	object(wGolfObj4).state() = 0;
	object(wGolfObj5).state() = 0;
	ram(wGolfHoleState) = GOLFHOLESTATE_WAIT_INPUT;
}

// The hole is done: a single hole ends here, a course moves on to its next
// hole (or the result).
void GolfHole::state_cleared()
{
	GolfObject text = object(wGolfOverlayText);
	if (ram(w1d800) != 0x01) {
		update_object(text, 0x50, 0x58, ADDR_Frameset_1caf8d);
		uint8_t counter = ram(wGolfCounter);
		if (counter < 0x50) {
			ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
			return;
		}
		if (!(ram(wJoypadPressed) & PAD_A))
			return;
		stop_music2(mem_);
		ram(w1d800) = 0;
		ram(wGolfMenuOption) = 0;
		if (ram(wIsMinigameCleared)) {
			ram(wSubState) = 0x08;
			return;
		}
		ram(wIsMinigameCleared) = TRUE;
		ram(wSubState) = SST_GOLF_CLEAR;
		return;
	}

	// on a course
	show_result_overlay_text();
	show_score();
	uint8_t wait = ram(wGolfWarioState) == 0x06 ? 0x50 : 0x70;
	uint8_t counter = ram(wGolfCounter);
	if (counter < wait) {
		ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
		return;
	}
	if (!(ram(wJoypadPressed) & PAD_A))
		return;
	ram(wGolfCourseScore) = static_cast<uint8_t>(ram(wGolfCourseScore) + ram(wGolfStroke));
	uint8_t hole = ram(wGolfCourseHole);
	if (hole != NUM_GOLF_COURSE_HOLES) {
		ram(wGolfCourseHole) = static_cast<uint8_t>(hole + 1);
		++ram(wPredeterminedGolfHole);
		ram(wSubState) = 0x04;
		return;
	}
	// the course is finished
	ram(wGolfCounter) = 0;
	wario().duration() = 0;
	wario().frameset_offset() = 0;
	text.duration() = 0;
	text.frameset_offset() = 0;
	object(wGolfParaGoomGhost).duration() = 0;
	object(wGolfParaGoomGhost).frameset_offset() = 0;
	uint8_t score = ram(wGolfCourseScore);
	ram(wGolfResult) = score < GOLF_BASE_SCORE + 1 ? GOLFRESULT_COURSE_UNDER_PAR : GOLFRESULT_COURSE_OVER_PAR;
	if (score < ram(static_cast<uint16_t>(wGolfBestScores + ram(wGolfCourse)))) {
		play_sfx(mem_, SFX_0FA);
		ram(wGolfWarioState) = GOLF_WARIO_CLEARED;
	} else {
		ram(wGolfWarioState) = GOLF_WARIO_LOST;
	}
	ram(wGolfHoleState) = GOLFHOLESTATE_RESULT;
}

void GolfHole::state_game_over()
{
	update_object(object(wGolfOverlayText), 0x50, 0x58, ADDR_Frameset_1cafa0);
	uint8_t counter = ram(wGolfCounter);
	if (counter < 0x50) {
		ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
		return;
	}
	if (!(ram(wJoypadPressed) & PAD_A))
		return;
	if (ram(w1d800) == 0x02)
		ram(wGolfMenuOption) = 0x01;
	ram(wSubState) = 0;
}

// A quits, B goes back.
void GolfHole::state_pause()
{
	GolfObject menu = object(wGolfObj3);
	update_object(menu, 0x9c, 0x58, ADDR_Frameset_1cb0da);
	uint8_t pressed = ram(wJoypadPressed);
	if (pressed & PAD_A) {
		if (ram(w1d800) != 0x01)
			ram(wGolfMenuOption) = 0x01;
		play_sfx(mem_, SFX_SELECTION);
		ram(wSubState) = 0;
	} else if (pressed & PAD_B) {
		play_sfx(mem_, SFX_0E7);
		menu.duration() = 0;
		menu.frameset_offset() = 0;
		ram(wGolfHoleState) = GOLFHOLESTATE_WAIT_INPUT;
	}
}

// The course's total score, and a new best score.
void GolfHole::state_result()
{
	show_result_overlay_text();

	// the score's sign and digits in the overlay text's sprites
	--ram(wVirtualOAMSprite00YCoord);
	uint8_t score = ram(wGolfCourseScore);
	uint8_t value;
	if (score > GOLF_BASE_SCORE) {
		value = static_cast<uint8_t>(score - GOLF_BASE_SCORE);
	} else {
		if (score == GOLF_BASE_SCORE) {
			--ram(wVirtualOAMSprite00YCoord);
			ram(wVirtualOAMSprite00Frame) = 0xfe;
		}
		value = static_cast<uint8_t>(GOLF_BASE_SCORE - score);
	}
	uint8_t tens = static_cast<uint8_t>(value / 10), ones = static_cast<uint8_t>(value % 10);
	if (tens)
		ram(wVirtualOAMSprite01Frame) = static_cast<uint8_t>(tens * 2 + 0x80);
	ram(wVirtualOAMSprite02Frame) = static_cast<uint8_t>(ones * 2 + 0x80);

	update_object(object(wGolfParaGoomGhost), 0x3c, 0x50, ADDR_Frameset_1cb0d7); // "TOTAL"

	uint8_t counter = ram(wGolfCounter);
	if (counter < 0x20) {
		ram(wGolfCounter) = static_cast<uint8_t>(counter + 1);
		return;
	}
	if (!(ram(wJoypadPressed) & PAD_A))
		return;
	Byte best = ram(static_cast<uint16_t>(wGolfBestScores + ram(wGolfCourse)));
	if (score < best)
		best = score;
	ram(wSubState) = 0;
}

// ---------------------------------------------------------------------------
// The Para-Goom

// Moves it along by the shot's x velocity (which drops every 8 frames), and
// finds the terrain under it: 32 pixels a block.
void GolfHole::update_paragoom_x()
{
	uint8_t frames = static_cast<uint8_t>(ram(w1dc51) + 1);
	ram(w1dc51) = frames;
	if (!(frames & 0x07))
		--ram(wShotXVel);

	Word x = ram_word(wGolfParaGoomXPos);
	bool out_of_bounds = false;
	if (!ram(wShotDirection)) {
		x = static_cast<uint16_t>(x + ram(wShotXVel));
	} else {
		x = static_cast<uint16_t>(x - ram(wShotXVel));
		if ((x >> 8) == 0xff) {
			// off the left edge
			ram(wGolfParaGoomXPos + 1) = 0x00;
			ram(wGolfTerrain) = GOLF_TERRAIN_END;
			out_of_bounds = true;
		}
	}
	if (!out_of_bounds) {
		uint16_t block = static_cast<uint16_t>(x >> 5);
		ram_word(w1dc3e) = block;
		uint8_t terrain = ram(static_cast<uint16_t>(wGolfTerrainBlocks + block));
		ram(wGolfTerrain) = terrain;
		uint8_t kind = terrain & GOLF_TERRAIN_MASK;
		if (kind == GOLF_TERRAIN_TEE_BOX) {
			if (!ram(w1dc47)) {
				ram(wGroundLevelYPos) = static_cast<uint8_t>(PARAGOOM_TEE_BOX_LEVEL);
				return;
			}
			if (terrain & GOLF_TERRAIN_FLAGS_MASK)
				ram(wGolfTerrain) = GOLF_TERRAIN_TEE_OB;
			ram(wGroundLevelYPos) = GROUND_LO;
			return;
		}
		if (kind != GOLF_TERRAIN_END) {
			ram(wGroundLevelYPos) = GROUND_LO;
			return;
		}
	}

	// out of bounds
	ram(wGolfParaGoomDuration) = 0;
	wario().duration() = 0;
	ram(wGolfParaGoomFramesetOffset) = 0;
	wario().frameset_offset() = 0;
	ram(wGolfParaGoomState) = GOLF_PARAGOOM_8;
	ram(wGolfWarioState) = GOLF_WARIO_IDLING;
	ram(wGolfVBlankMode) = 0x02;
	ram(wGolfHoleState) = GOLFHOLESTATE_SPECIAL_TERRAIN;
	++ram(wGolfStroke);
}

// Gravity, and the landing: a bounce on the fairway or tee box, else the
// terrain's effect (penalty strokes for water and out of bounds).
void GolfHole::update_paragoom_y()
{
	Word y_vel = ram_word(wShotYVel);
	y_vel = static_cast<uint16_t>(y_vel - 1);
	Word y = ram_word(wGolfParaGoomYPos);
	y = static_cast<uint16_t>(y - y_vel); // the y velocity points up
	if ((y >> 8) != GROUND_HI)
		return;
	uint8_t ground = ram(wGroundLevelYPos);
	if (ground >= static_cast<uint8_t>(y))
		return;

	// landed: on the ground
	y = static_cast<uint16_t>(GROUND_HI << 8 | ground);
	uint8_t terrain = ram(wGolfTerrain);
	uint8_t kind = terrain & GOLF_TERRAIN_MASK;
	if (kind && kind < GOLF_TERRAIN_OB)
		ram(w1dc47) = kind;
	ram(w1dc51) = 0;
	if (kind < SPECIAL_GOLF_TERRAIN) {
		// the tee box or the fairway
		play_sfx(mem_, SFX_BUMP);
		uint8_t next = ram(w1dc01);
		ram(wGolfHoleState) = next;
		if (next == GOLFHOLESTATE_BOUNCE)
			return;
		ram(w1dc01) = GOLFHOLESTATE_BETWEEN_SHOTS;
		update_hole_direction();
		become_idle();
		return;
	}

	ram(wGolfHoleState) = GOLFHOLESTATE_SPECIAL_TERRAIN;
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
		ram(w1dc47) = ram(w1dc53);
		penalty = true;
		break;
	case GOLF_TERRAIN_TEE_OB:
		++ram(wGolfStroke);
		return;
	case GOLF_TERRAIN_BUNKER:
		play_sfx(mem_, SFX_065);
		ram(wGolfParaGoomState) = GOLF_PARAGOOM_5;
		return;
	default: // rough
		play_sfx(mem_, SFX_0A1);
		return;
	}
	if (penalty) {
		ram(wGolfParaGoomState) = GOLF_PARAGOOM_6;
		++ram(wGolfStroke);
	}

	// at the edge of a block: snaps to it
	uint8_t block_lo;
	if (terrain & (1 << GOLF_TERRAIN_EDGE_RIGHT_F))
		block_lo = ram(w1dc3e);
	else if (terrain & (1 << GOLF_TERRAIN_EDGE_LEFT_F))
		block_lo = static_cast<uint8_t>(ram(w1dc3e) + 1);
	else
		return;
	uint16_t block = static_cast<uint16_t>(ram(w1dc3e + 1) << 8 | block_lo);
	ram_word(wGolfParaGoomXPos) = static_cast<uint16_t>(block << 5);
}

void GolfHole::become_idle()
{
	ram(wGolfParaGoomDuration) = 0;
	wario().duration() = 0;
	ram(wGolfParaGoomFramesetOffset) = 0;
	wario().frameset_offset() = 0;
	ram(wGolfParaGoomState) = GOLF_PARAGOOM_0;
	ram(wGolfWarioState) = GOLF_WARIO_IDLING;
	ram(wGolfVBlankMode) = 0x02;
}

// After a splash or out of bounds, the Para-Goom comes back: at the water's
// respawn block, or where it was shot from.
bool GolfHole::respawn_paragoom()
{
	uint8_t state = ram(wGolfParaGoomState);
	if (state == GOLF_PARAGOOM_2) {
		ram_word(wLastGolfParaGoomYPos) = PARAGOOM_GROUND_LEVEL;
		ram_word(wGolfParaGoomYPos) = 0;
		uint8_t block = ram(wGolfTerrain) & GOLF_TERRAIN_2ND_WATER ? ram(wGolfWaterSpawn2) : ram(wGolfWaterSpawn1);
		ram_word(wGolfParaGoomXPos) = static_cast<uint16_t>(block * 32);
		align_x_scroll();
		update_display_mode();
		ram(wGolfParaGoomFramesetOffset) = 0;
		ram(wGolfParaGoomState) = GOLF_PARAGOOM_4;
		set_respawn_y_if_enough_strokes();
		return true;
	}
	if (state != GOLF_PARAGOOM_3)
		return false;
	ram_word(wGolfParaGoomYPos) = 0;
	ram(wGolfParaGoomXPos + 0) = ram(wLastGolfParaGoomXPos + 0);
	ram(wGolfParaGoomXPos + 1) = ram(wLastGolfParaGoomXPos + 1);
	ram(wGolfParaGoomState) = GOLF_PARAGOOM_8;
	set_respawn_y_if_enough_strokes();
	return true;
}

// With no strokes left, the Para-Goom stays on the ground.
void GolfHole::set_respawn_y_if_enough_strokes()
{
	if (ram(wGolfStroke) < ram(wGolfMaxStrokes))
		return;
	ram_word(wGolfParaGoomYPos) = PARAGOOM_GROUND_LEVEL;
	ram(w1dc20) = TRUE;
}

// Which way the hole is from the Para-Goom, and where Wario stands to hit
// it (64 to its other side).
void GolfHole::update_hole_direction()
{
	uint16_t x = ram_word(wGolfParaGoomXPos);
	bool past_hole = compare16(x, ram_word(static_cast<uint16_t>(wGolfParaGoomXPos + 2))) >= 2;
	ram(wGolfHoleDirection) = past_hole ? 1 : 0;
	ram_word(wGolfWarioTargetXPos) = static_cast<uint16_t>(past_hole ? x + 0x40 : x - 0x40);
}

// ---------------------------------------------------------------------------
// The screen

// The view follows the Para-Goom: 80 pixels from the left, within the hole.
void GolfHole::align_x_scroll()
{
	int x = ram_word(wGolfParaGoomXPos) >> 2;
	int scroll = x - 0x50;
	if (scroll < 0)
		scroll = 0;
	if (scroll > 0x160)
		scroll = 0x160;
	ram_word(wGolfXScroll) = static_cast<uint16_t>(scroll);
}

// The hole is 2 BG maps wide: the first, both (split with the window), or
// the second.
void GolfHole::update_display_mode()
{
	uint16_t scroll = ram_word(wGolfXScroll);
	uint8_t lo = static_cast<uint8_t>(scroll);
	ram(wSCX) = lo;
	if (scroll >> 8) {
		ram(wGolfDisplayMode) = GOLF_DISPLAY_BGMAP1;
	} else if (lo < static_cast<uint8_t>(MAX_GOLF_X_SCROLL)) {
		ram(wGolfDisplayMode) = GOLF_DISPLAY_BGMAP0;
	} else {
		ram(wGolfWX) = static_cast<uint8_t>(167 - lo + 89 + WX_OFS);
		ram(wGolfDisplayMode) = GOLF_DISPLAY_SPLIT;
	}
}

// Func_1c8f56: whether x (a quarter of a position) is on screen.
bool GolfHole::in_view(uint16_t x)
{
	uint16_t relative = static_cast<uint16_t>(x - ram_word(wGolfXScroll));
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
	if (ram(wGolfHoleState) == GOLFHOLESTATE_PAUSE)
		return;
	update_object(object(wGolfObj3), SCREEN_HEIGHT_PX, 0x58, ADDR_Frameset_1caf8a);
	if (object(wGolfObj4).state())
		return;
	update_object(object(wGolfObj4), SCREEN_HEIGHT_PX, static_cast<uint8_t>(ram(wShotPowerGaugeCursor) + 0x32),
		ADDR_Frameset_1caf84);
	if (!object(wGolfObj5).state())
		return;
	update_object(object(wGolfObj5), SCREEN_HEIGHT_PX, static_cast<uint8_t>(ram(wSelectedShotPowerGauge) + 0x32),
		ADDR_Frameset_1caf87);
}

void GolfHole::update_paragoom()
{
	if (ram(w1dc20))
		return;
	if (ram(wGolfParaGoomState) >= GOLF_PARAGOOM_8)
		return;
	uint16_t x = static_cast<uint16_t>(ram_word(wGolfParaGoomXPos) >> 2);
	if (!in_view(x))
		return;
	GolfObject paragoom = object(wGolfParaGoom);
	paragoom.x() = static_cast<uint8_t>(static_cast<uint8_t>(x) + OAM_X_OFS - ram(wSCX));
	uint8_t state = ram(wGolfParaGoomState);
	if (state >= GOLF_PARAGOOM_8)
		return;
	uint8_t y_offset = state == GOLF_PARAGOOM_5 ? 0x13 : 0x12;
	paragoom.y() = static_cast<uint8_t>(static_cast<uint8_t>(ram_word(wGolfParaGoomYPos) >> 2) + y_offset);

	uint16_t frameset = mem_.word(static_cast<uint16_t>(PARAGOOM_FRAMESETS + static_cast<uint8_t>(state << 1)));
	if (routines_.update_golf_object_animation(frameset, paragoom.duration().addr()) && respawn_paragoom())
		return;
	if (ram(wGolfParaGoomState) == 0x02 && !paragoom.duration()) {
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
	uint16_t x = static_cast<uint16_t>(ram_word(wGolfWarioXPos) >> 2);
	if (!in_view(x)) {
		if (!ram(w1dc21))
			return;
		// off screen after the view jumped: just outside it, on his side
		ram(w1dc21) = 0;
		uint16_t scroll = static_cast<uint16_t>(ram_word(wGolfXScroll) << 2);
		ram_word(wGolfWarioXPos) = static_cast<uint16_t>(ram(wGolfWarioDir) ? scroll + 680 : scroll - 40);
		return;
	}
	ram(w1dc21) = 0;
	ram(wGolfWarioXCoord) = static_cast<uint8_t>(static_cast<uint8_t>(x) + OAM_X_OFS - ram(wSCX));
	ram(wGolfWarioYCoord) = static_cast<uint8_t>(ram(wGolfWarioYPos) + 0x11);
	routines_.update_golf_wario_animation();
	if (ram(wGolfWarioState) == GOLF_WARIO_WALKING && wario().frameset_offset() == 0x2 && !wario().duration())
		play_sfx(mem_, SFX_WALK);
}

void GolfHole::show_result_overlay_text()
{
	GolfObject text = object(wGolfOverlayText);
	text.y() = 0x50;
	text.x() = 0x58;
	uint16_t frameset = mem_.word(static_cast<uint16_t>(RESULT_FRAMESETS + static_cast<uint8_t>(ram(wGolfResult) * 2)));
	routines_.update_golf_object_animation(frameset, text.duration().addr());
	load_object_sprite(text);
}

// The strokes over or under par, in the overlay text's sprites
void GolfHole::show_score()
{
	uint8_t result = ram(wGolfResult);
	if (result == GOLFRESULT_PAR)
		return;
	uint8_t value;
	if (result == GOLFRESULT_OVER_PAR) {
		value = static_cast<uint8_t>(ram(wGolfStroke) - ram(wGolfPar));
		if (value >= 10) {
			ram(wVirtualOAMSprite01Frame) = 0x82;
			value = static_cast<uint8_t>(value - 10);
		}
	} else {
		value = static_cast<uint8_t>(ram(wGolfPar) - ram(wGolfStroke));
	}
	ram(wVirtualOAMSprite02Frame) = static_cast<uint8_t>(value * 2 + 0x80);
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
	uint8_t state = ram(wGolfWarioState);
	ram(wGolfWarioTilesBank) = mem_[static_cast<uint16_t>(WARIO_GFX_BANKS + state)];
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
	ram(w1dc13) = 0x0d; // 7 dma_rows
}

// Func_1c9cf9
void GolfHole::set_hole_vblank()
{
	uint8_t mode = ram(wGolfDisplayMode);
	if (mode == GOLF_DISPLAY_SPLIT)
		ram(wGolfLCDConfig) = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_ON | LCDC_WIN_9C00 | LCDC_ON;
	else if (mode == GOLF_DISPLAY_BGMAP1)
		ram(wGolfLCDConfig) = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_BG_9C00 | LCDC_ON;
	else
		ram(wGolfLCDConfig) = LCDC_DEFAULT;

	switch (ram(wGolfVBlankMode)) {
	case 0: {
		// the waving pin flag, 4 frames of 8
		uint8_t counter = ram(w1dc0b);
		uint8_t frame = counter >= 0x18 ? 0x0c : counter >= 0x10 ? 0x08 : counter >= 0x08 ? 0x04 : 0x00;
		routines_.copy(static_cast<uint16_t>(ADDR_GolfPinFlagGfx + frame * 16), wGolfPinFlagTiles, 4 * 16);
		ram(w1dc0b) = static_cast<uint8_t>((ram(w1dc0b) + 1) & 0x1f);
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
	switch (ram(wGolfVBlankMode)) {
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
	switch (ram(wGolfVBlankMode)) {
	case 0: copy_vblank_func(ADDR_VBlank_1c9f36 + 0x0b, 0x39); break;
	case 1: copy_vblank_func(ADDR_VBlank_1c9f7a + 0x0b, 0x29); break;
	default: unrealistic(); break;
	}
}

// Func_1c9fbd: the coin count's digits (BCD) in the first 3 sprites
void GolfHole::set_coin_sprites()
{
	uint8_t hundreds = ram(wGolfNumCoins + 0), tens_ones = ram(wGolfNumCoins + 1);
	ram(wVirtualOAMSprite00Frame) = static_cast<uint8_t>(hundreds * 2 + 0xa0);
	ram(wVirtualOAMSprite01Frame) = static_cast<uint8_t>((tens_ones >> 4) * 2 + 0xa0);
	ram(wVirtualOAMSprite02Frame) = static_cast<uint8_t>((tens_ones & 0x0f) * 2 + 0xa0);
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
