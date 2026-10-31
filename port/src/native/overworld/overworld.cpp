// The overworld, part 1; see overworld.hpp.
#include "overworld.hpp"

#include <initializer_list>

namespace wl3::overworld {

namespace {

constexpr uint8_t WRAM2 = 2;
constexpr uint8_t OW_LCDC_WIN = LCDC_ON | LCDC_WIN_9C00 | LCDC_WIN_ON | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
constexpr uint16_t OW_WRAM = 0xd060; // STARTOF("OW WRAM")
constexpr uint16_t OW_WRAM_SIZE = 0x07a0;

static_assert(SST_OVERWORLD_OW_SCENE == 5);
static_assert(SST_OVERWORLD_09 == 9);
static_assert(SST_OVERWORLD_CUTSCENE == 17);
static_assert(SST_OVERWORLD_GOLF_BUILDING == 32);
static_assert(NORTH == 0 && WEST == 1 && SOUTH == 2 && EAST == 3);
static_assert(TRANSITION_NEW_GAME == 0xff);

// a map side's graphics, tilemap and attrmap
struct MapSide {
	uint8_t gfx_bank;
	uint16_t gfx;
};

constexpr MapSide SIDE_GFX[4] = {
	{BANK_Overworld1Gfx, ADDR_Overworld1Gfx},
	{BANK_Overworld2Gfx, ADDR_Overworld2Gfx},
	{BANK_Overworld5Gfx, ADDR_Overworld5Gfx},
	{BANK_Overworld6Gfx, ADDR_Overworld6Gfx},
};

} // namespace

void Overworld::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

uint8_t Overworld::call_a(uint16_t addr, uint8_t a)
{
	gb_->cpu.a = a;
	routines_.call(addr);
	return gb_->cpu.a;
}

bool Overworld::call_z(uint16_t addr, uint8_t a)
{
	gb_->cpu.a = a;
	routines_.call(addr);
	return gb_->cpu.f & FLAG_Z;
}

// ---------------------------------------------------------------------------
// common.asm, state_table.asm, level_select.asm

void Overworld::handle()
{
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = WRAM2;
	routines_.in_bank(BANK_OverworldStateTable, ADDR_OverworldStateTable, [&] { run_state(); });
	mem_[rWBK] = wram_bank;

	// a level was chosen
	if (wram_.state() != ST_LEVEL)
		return;
	play_sfx(mem_, SFX_SELECTION);
	select_level();
	uint8_t level = wram_.level();
	if (level == GOLF_BUILDING) {
		wram_.state() = ST_GOLF_BUILDING;
	} else if (level == THE_TEMPLE) {
		mem_[hSFXID + 0] = 0xff; // stop_sfx
		mem_[hSFXID + 1] = 0x00;
	}
}

void Overworld::run_state()
{
	// the states still in C, by address (0: a C++ one, below)
	static constexpr uint16_t STATES[40] = {
		ADDR_FastFadeToWhite, 0, ADDR_SlowFadeFromWhite, 0, ADDR_FastFadeToWhite,
		0, ADDR_SlowFadeFromWhite, 0,
		ADDR_FadeBGToWhite_Normal, 0, ADDR_DarkenBGToPal_Normal, 0,
		ADDR_FadeBGToWhite_Normal, ADDR_InitTreasureCollection, ADDR_DarkenBGToPal_Normal,
		ADDR_TreasureCollection, ADDR_FadeBGToWhite_Normal,
		ADDR_InitCutscene, ADDR_DarkenBGToPal_Normal, ADDR_UpdateCutscene, ADDR_FadeBGToWhite_Normal,
		ADDR_Func_80db1,
		ADDR_FadeBGToWhite_Normal, ADDR_InitBestTimeList, ADDR_DarkenBGToPal_Normal, ADDR_BestTimeList,
		ADDR_FadeBGToWhite_Normal, ADDR_InitTempleScene, ADDR_DarkenBGToPal_Normal, ADDR_UpdateTempleScene,
		ADDR_Func_80e33,
		ADDR_FadeBGToWhite_Normal, ADDR_GolfBuilding, ADDR_DarkenBGToPal_Normal, ADDR_Func_80e55,
		ADDR_FastFadeToWhite, ADDR_InitEpilogue, ADDR_SlowFadeFromWhite, ADDR_Func_80e75, ADDR_Func_80e85,
	};
	uint8_t state = wram_.sub_state();
	switch (state) {
	case 1: init(); return;
	case 3: wait_for_a(); return;
	case SST_OVERWORLD_OW_SCENE: init_scene(); return;
	case 7: update_scene(); return;
	case SST_OVERWORLD_09: init_map_side(); return;
	case 11: update_map_side(); return;
	}
	if (state >= 40) {
		unrealistic();
		return;
	}
	routines_.call(STATES[state]);
}

// The level for wOWLevel: its variant (by the treasures that change it)
// and day or night.
void Overworld::select_level()
{
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = WRAM2;
	uint8_t ow_level = wram_.ow_level();
	if (!ow_level || ow_level == LEVEL_GOLF_BUILDING) {
		wram_.level() = ow_level ? GOLF_BUILDING : THE_TEMPLE;
		mem_[rWBK] = wram_bank;
		return;
	}
	uint8_t level = static_cast<uint8_t>((ow_level - 1) * 8);
	wram_.level() = level;
	uint16_t requirements = static_cast<uint16_t>(ADDR_LevelTreasureRequirements + level);
	uint8_t variant = 0;
	for (int pair = 0; pair < 3; pair++) {
		uint8_t first = treasure_collected(mem_[static_cast<uint16_t>(requirements + 2 * pair)]);
		uint8_t second = treasure_collected(mem_[static_cast<uint16_t>(requirements + 2 * pair + 1)]);
		if (first & second) {
			variant = static_cast<uint8_t>(3 - pair);
			break;
		}
	}
	// four variants each by day, then by night
	wram_.level() = static_cast<uint8_t>((level + variant) | ((wram_.day_night() & 0x01) * 4));
	mem_[rWBK] = wram_bank;
}

// ---------------------------------------------------------------------------
// init.asm: the map

void Overworld::init()
{
	mem_[rWBK] = WRAM2;
	stop_music();
	uint8_t transition = wram_.transition_param();
	wram_.last_transition_param() = transition;
	wram_.room_animated_tiles_enabled() = 0;
	// a new game has the 0th treasure
	if (transition == TRANSITION_NEW_GAME)
		wram_.treasures_collected().set_bit(0);
	routines_.call(ADDR_GetCrayonFlags);

	if (wram_.ow_bars_state() != OWBARS_INPUT_TOP) {
		uint8_t ow_level = wram_.ow_level();
		if (ow_level >= LEVEL_GOLF_BUILDING) {
			wram_.next_map_side() = 0;
			wram_.map_side_level_id() = 0;
			wram_.sub_state() = SST_OVERWORLD_GOLF_BUILDING;
			routines_.call(ADDR_Func_803e6);
			return;
		}
		gb_->cpu.b = ow_level;
		routines_.call(ADDR_GetMapLevelID);
		wram_.next_map_side() = gb_->cpu.b;
		wram_.map_side_level_id() = gb_->cpu.d;
	}
	clear_ow_wram();

	// by the transition to the map
	switch (transition) {
	case 0:
	case TRANSITION_RETURN_TO_MAP:
		load_map_without_scene();
		return;
	case TRANSITION_EPILOGUE_NOT_PERFECT:
	case TRANSITION_EPILOGUE_PERFECT:
		wram_.stored_transition_param() = TRANSITION_RETURN_TO_MAP;
		wram_.cur_event() = EVENT_EPILOGUE;
		after_cutscene();
		return;
	case TRANSITION_GAME_OVER:
		routines_.call(ADDR_Func_803e6);
		return;
	case TRANSITION_NEW_GAME:
		wram_.cur_event() = EVENT_PROLOGUE;
		break;
	default: // a treasure: does it unlock an event?
		if (call_z(ADDR_CheckIfTreasureUnlocksEvent, static_cast<uint8_t>(transition + 1))) {
			load_map_without_scene();
			return;
		}
		break;
	}
	set_stored_transition_param_as_return_to_map();
	routines_.farcall(BANK_CheckIfEventHasCutscene, ADDR_CheckIfEventHasCutscene);
	if (wram_.event_with_cutscene()) {
		play_scene(SST_OVERWORLD_CUTSCENE);
		return;
	}
	after_cutscene();
}

// whether the event has a scene on the map to play
void Overworld::after_cutscene()
{
	if (call_z(ADDR_GetOWSceneParams, wram_.cur_event())) {
		load_map_without_scene();
		return;
	}
	play_scene(SST_OVERWORLD_OW_SCENE);
}

void Overworld::play_scene(uint8_t state)
{
	update_stored_transition_param();
	wram_.sub_state() = state;
}

void Overworld::load_map_without_scene()
{
	wram_.cur_event() = EVENT_00;
	wram_.sub_state() = SST_OVERWORLD_09;
	routines_.call(ADDR_Func_803e6);
}

void Overworld::wait_for_a()
{
	if (wram_.joypad_pressed().bit(B_PAD_A))
		++wram_.sub_state();
}

void Overworld::update_scene()
{
	routines_.call(ADDR_Func_818ad);
	routines_.call(ADDR_Func_82bb8);
	routines_.call(ADDR_Func_82bda);
	routines_.farcall(BANK_Func_b4a37, ADDR_Func_b4a37);
	routines_.call(ADDR_LoadCompassSprite);
	routines_.clear_unused_virtual_oam();
}

void Overworld::update_map_side()
{
	routines_.call(ADDR_Func_80e89);
	routines_.call(ADDR_LoadCompassSprite);
	routines_.clear_unused_virtual_oam();
}

// A side of the map, by day or night.
void Overworld::init_map_side()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	stop_music();
	mem_[rWBK] = WRAM2;
	routines_.clear_virtual_oam();
	routines_.call(ADDR_VBlank_Overworld);
	clear_ow_wram();
	routines_.call(ADDR_GetNextTreasureToCollect);

	// day or night (bit 0 of wDayNight)
	uint8_t day_night = wram_.day_night();
	Byte time = wram_.unk_2d011();
	if (!(day_night & 0x80)) {
		time = (day_night >> 4) & 0x0f;
		wram_.day_night() = static_cast<uint8_t>((wram_.day_night() & 0xfe) | (time & 0x01));
	} else {
		routines_.call(ADDR_SwitchDayNight);
		time = day_night & ~((1 << 7) | (1 << 4));
		if (!wram_.transition_param()) {
			time ^= 0x01;
			wram_.day_night() = static_cast<uint8_t>((wram_.day_night() & 0xfe) | (time & 0x01));
			wram_.day_night().reset_bit(7);
		}
	}
	wram_.unk_2d07c() = time;
	wram_.unk_2d07d() = 0;

	uint8_t side = wram_.next_map_side();
	wram_.cur_map_side() = side;
	switch (side) {
	case NORTH:
		init_map_side_gfx(0);
		load_bg_maps(BANK_OverworldNorthTilemap, ADDR_OverworldNorthTilemap,
			BANK_OverworldNorthAttrmap, ADDR_OverworldNorthAttrmap);
		break;
	case WEST:
		init_map_side_gfx(1);
		load_west_maps();
		break;
	case SOUTH:
		init_map_side_gfx(2);
		load_south_maps();
		break;
	case EAST:
		init_east_map_side();
		return;
	default:
		unrealistic();
		return;
	}
	finish_map_side();
}

void Overworld::init_map_side_gfx(uint16_t side)
{
	routines_.farcall(BANK__LoadMapSidePals, ADDR__LoadMapSidePals);
	decompress(SIDE_GFX[side].gfx_bank, SIDE_GFX[side].gfx, v0Tiles0);
	load_common_gfx();
	decompress(BANK_OverworldArrowsGfx, ADDR_OverworldArrowsGfx, v0Tiles1);
}

// The east, dark until the sun medallion is found.
void Overworld::init_east_map_side()
{
	uint8_t medallion = call_a(ADDR_Func_819c6, EVENT_SUMMON_SUN);
	wram_.got_sun_medallion() = medallion;
	if (!medallion) {
		wram_.unk_2d011() = 0x01;
		wram_.day_night() = static_cast<uint8_t>((wram_.day_night() | 0x01) & ~0x80);
	}
	if (wram_.transition_param() == KEY_CARD_BLUE && call_z(ADDR_IsTreasureCollected, KEY_CARD_RED))
		set_stored_transition_param_as_return_to_map();
	init_map_side_gfx(3);
	load_east_maps();
	finish_map_side();
}

// The map side's bars, Wario, the clouds and the level name.
void Overworld::finish_map_side()
{
	Byte flags = wram_.game_mode_flags();
	wram_.missing_treasure_indicators_enabled() =
		flags.bit(MODE_TIME_ATTACK_F) ? 0 : static_cast<uint8_t>(flags & MODE_GAME_CLEARED);
	reset_clouds();
	decompress(BANK_OverworldTopBarTilemap, ADDR_OverworldTopBarTilemap, v0BGMap0 + 29 * TILEMAP_WIDTH);
	decompress(BANK_OverworldTopBarAttrmap, ADDR_OverworldTopBarAttrmap, v1BGMap0 + 29 * TILEMAP_WIDTH, true);
	uint8_t scy = wram_.ow_bars_state() ? 0xec : 0x04;
	mem_[rSCY] = scy;
	wram_.scy() = scy;
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	wram_.virtual_oam_byte_size() = 0;
	call_a(ADDR_Func_81dce, wram_.cur_map_side());
	routines_.call(ADDR_ShowMissingTreasureIndicators);
	routines_.call(ADDR_Func_80bd9);
	write_bg_map_from_wram();
	routines_.call(ADDR_Func_8212c);
	uint8_t level = wram_.map_side_level_id();
	wram_.level2() = level;
	wram_.unk_2d068() = level;
	wram_.level_arrow_directions1() = 0;
	call_a(ADDR_SetOWWarioOAM, OWWARIO_DOWN);
	routines_.farcall(BANK_Func_b4a3d, ADDR_Func_b4a3d);
	routines_.call(ADDR_Func_82041);
	routines_.farcall(BANK_Func_b4aa9, ADDR_Func_b4aa9);
	routines_.farcall(BANK__LoadCloudAndSeaGfx, ADDR__LoadCloudAndSeaGfx);
	if (!wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F)) {
		decompress(BANK_OverworldBottomBarTreasuresTilemap, ADDR_OverworldBottomBarTreasuresTilemap, v0BGMap1);
		decompress(BANK_OverworldBottomBarTreasuresAttrmap, ADDR_OverworldBottomBarTreasuresAttrmap,
			v1BGMap1, true);
	} else {
		decompress(BANK_OverworldBottomBarTimeAttackTilemap, ADDR_OverworldBottomBarTimeAttackTilemap, v0BGMap1);
		decompress(BANK_OverworldBottomBarTimeAttackAttrmap, ADDR_OverworldBottomBarTimeAttackAttrmap,
			v1BGMap1, true);
	}

	wram_.wx() = 0;
	mem_[rWX] = 0;
	wram_.wy() = SCREEN_HEIGHT_PX;
	mem_[rWY] = SCREEN_HEIGHT_PX;
	wram_.bottom_bar_action() = BOTBAR_CLOSED;
	wram_.has_magnifying_glass() = treasure_collected(MAGNIFYING_GLASS);
	wram_.top_bar_selectable_buttons() = 1 << TOPBAR_COLLECTION_F; // always selectable
	routines_.call(ADDR_SetDayNightSpellSelectable);
	routines_.call(ADDR_SetCutsceneButtonSelectable);
	routines_.call(ADDR_SetNextPrevMapButtonsSelectable);
	routines_.call(ADDR_UpdateNextPrevMapButtonsSelectable);
	show_top_bar();

	routines_.call(ADDR_LoadLevelNameIfValid);
	bool at_exit = wram_.map_side_level_id() >= OW_EXITS;
	if (at_exit)
		clear_pal7_colours();
	wram_.unk_2d0e0() = at_exit ? 0 : 1;
	routines_.farcall(BANK_DrawCoinCount, ADDR_DrawCoinCount);
	routines_.call(ADDR_SetCompassSprite);
	if (!wram_.ow_bars_state())
		routines_.call(ADDR_DrawBottomBar);
	wram_.level_arrow_directions1() = 0;
	wram_.ow_pal_transition_state() = 0;
	wram_.unk_2d013() = 0;
	mem_[rLCDC] = OW_LCDC_WIN;
	++wram_.sub_state();
}

// The window's rows (the top bar) kept in wAttrmap; with the top bar open,
// its buttons drawn there and put back.
void Overworld::show_top_bar()
{
	constexpr uint16_t WINDOW_ROWS = v0BGMap0 + 30 * TILEMAP_WIDTH;
	constexpr uint16_t TILES = wAttrmap + 21 * TILEMAP_WIDTH;
	constexpr uint16_t ATTRS = wAttrmap + 19 * TILEMAP_WIDTH;
	routines_.copy(WINDOW_ROWS, TILES, 2 * TILEMAP_WIDTH);
	set_vram1(true);
	routines_.copy(WINDOW_ROWS, ATTRS, 2 * TILEMAP_WIDTH);
	set_vram1(false);
	if (wram_.ow_bars_state() != OWBARS_INPUT_TOP)
		return;
	routines_.call(ADDR_UpdateTopBar + 0x72); // UpdateTopBar.InitTopBarButtons
	routines_.copy(TILES, WINDOW_ROWS, 2 * TILEMAP_WIDTH);
	set_vram1(true);
	routines_.copy(ATTRS, WINDOW_ROWS, 2 * TILEMAP_WIDTH);
	set_vram1(false);
	wram_.transition_param() = wram_.unk_2d02c();
	routines_.call(ADDR_PlayOverworldMusic);
}

// ---------------------------------------------------------------------------
// init.asm: a scene on the map

void Overworld::init_scene()
{
	routines_.disable_lcd();
	stop_music();
	clear_ow_wram();
	routines_.call(ADDR_VBlank_OWScene);
	routines_.clear_virtual_oam();
	wram_.unk_2d011() = 0;
	wram_.unk_2d018() = wram_.unk_2d015();
	wram_.temp_unlockable_connection_id() = wram_.ow_scene_action_param();
	wram_.temp_ow_scene_action() = wram_.ow_scene_action();
	mem_[wTempOWSceneParamsPtr + 0] = mem_[wOWSceneParamsPtr + 0];
	mem_[wTempOWSceneParamsPtr + 1] = mem_[wOWSceneParamsPtr + 1];

	uint8_t side = wram_.ow_scene_map_side();
	wram_.cur_map_side() = side;
	switch (side) {
	case NORTH:
		init_north_scene();
		break;
	case WEST:
		init_scene_map_side(1, ADDR_Func_80ae7);
		break;
	case SOUTH:
		if (wram_.cur_event() == EVENT_SUMMON_MOON)
			wram_.unk_2d011() = 0x01;
		init_scene_map_side(2, ADDR_Func_80af5);
		break;
	case EAST: {
		uint8_t medallion = call_a(ADDR_Func_819c6, EVENT_SUMMON_SUN);
		wram_.got_sun_medallion() = medallion;
		if (!medallion)
			wram_.unk_2d011() = 0x01;
		init_scene_map_side(3, ADDR_Func_80b1b);
		break;
	}
	default:
		unrealistic();
		break;
	}
}

void Overworld::init_scene_map_side(uint16_t side, uint16_t maps_routine)
{
	load_pals_for_scene();
	decompress(SIDE_GFX[side].gfx_bank, SIDE_GFX[side].gfx, v0Tiles0);
	load_common_gfx();
	load_glow_gfx();
	switch (maps_routine) {
	case ADDR_Func_80ae7: load_west_maps(); break;
	case ADDR_Func_80af5: load_south_maps(); break;
	default: load_east_maps(); break;
	}
	finish_scene_map_side();
}

void Overworld::init_north_scene()
{
	load_pals_for_scene();
	load_common_gfx();
	decompress(SIDE_GFX[0].gfx_bank, SIDE_GFX[0].gfx, v0Tiles0);
	if (wram_.cur_event() != EVENT_DAY_OR_NIGHT_SPELL)
		load_glow_gfx();
	load_bg_maps(BANK_OverworldNorthTilemap, ADDR_OverworldNorthTilemap,
		BANK_OverworldNorthAttrmap, ADDR_OverworldNorthAttrmap);
	finish_scene_map_side();
}

void Overworld::finish_scene_map_side()
{
	reset_clouds();
	call_a(ADDR_Func_81dce, wram_.cur_map_side());
	write_bg_map_from_wram();
	mem_[rSCY] = 0x04;
	wram_.scy() = 0x04;
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	wram_.unk_2d015() = wram_.unk_2d018();
	wram_.ow_scene_action_param() = wram_.temp_unlockable_connection_id();
	wram_.ow_scene_action() = wram_.temp_ow_scene_action();
	wram_.ow_scene_map_side() = wram_.cur_map_side();
	mem_[wOWSceneParamsPtr + 0] = mem_[wTempOWSceneParamsPtr + 0];
	mem_[wOWSceneParamsPtr + 1] = mem_[wTempOWSceneParamsPtr + 1];
	routines_.farcall(BANK_Func_b4a37, ADDR_Func_b4a37);
	routines_.farcall(BANK__LoadCloudAndSeaGfx, ADDR__LoadCloudAndSeaGfx);
	clear_pal7_colours();
	set_vram1(true);
	copy_level_name_gfx(v1Tiles2 + TILE_SIZE * 0x50);
	copy_level_name_gfx(v1Tiles2 + TILE_SIZE * 0x60);
	set_vram1(false);
	wram_.unk_2d0e0() = 0;
	routines_.call(ADDR_SetCompassSprite);
	wram_.level_arrow_directions1() = 0;
	wram_.ow_pal_transition_state() = 0;
	wram_.unk_2d013() = 0;
	bool window = !wram_.cur_map_side() && wram_.cur_event() == EVENT_MAGNIFYING_GLASS;
	mem_[rLCDC] = window ? OW_LCDC_WIN : LCDC_DEFAULT;
	++wram_.sub_state();
}

void Overworld::copy_level_name_gfx(uint16_t dest)
{
	gb_->cpu.b = static_cast<uint8_t>(wram_.language() + 0x24); // the bank
	gb_->cpu.c = 0x00; // $10 tiles
	SET_HL(gb_, ADDR_LevelNamesNorthJPGfx + (7 * 0x20 * TILE_SIZE));
	SET_DE(gb_, dest);
	routines_.call(ADDR_CopyFarBytes);
}

// ---------------------------------------------------------------------------
// init.asm: loading

void Overworld::decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1)
{
	if (vram1)
		set_vram1(true);
	wram_.temp_bank() = bank;
	SET_HL(gb_, src);
	SET_BC(gb_, dest);
	routines_.farcall(bank, ADDR_Decompress);
	if (vram1)
		set_vram1(false);
}

void Overworld::load_bg_maps(uint8_t tilemap_bank, uint16_t tilemap, uint8_t attrmap_bank, uint16_t attrmap)
{
	wram_.unk_2dfff() = tilemap_bank;
	decompress(attrmap_bank, attrmap, wAttrmap, true);
	decompress(tilemap_bank, tilemap, w3d500);
}

void Overworld::load_common_gfx()
{
	decompress(BANK_OverworldCommonGfx, ADDR_OverworldCommonGfx, v1Tiles0, true);
	decompress(BANK_OverworldNumbersGfx, ADDR_OverworldNumbersGfx, v1Tiles0 + 0x60 * TILE_SIZE, true);
}

void Overworld::load_glow_gfx()
{
	decompress(BANK_OverworldGlowGfx, ADDR_OverworldGlowGfx, v1Tiles0 + 0x20 * TILE_SIZE, true);
}

void Overworld::clear_ow_wram()
{
	SET_HL(gb_, OW_WRAM);
	SET_BC(gb_, OW_WRAM_SIZE);
	gb_->cpu.a = 0;
	routines_.call(ADDR_WriteAToHL_BCTimes);
}

void Overworld::set_stored_transition_param_as_return_to_map()
{
	if (wram_.ow_bars_state())
		return; // a manual cutscene
	wram_.stored_transition_param() = TRANSITION_RETURN_TO_MAP;
}

void Overworld::update_stored_transition_param()
{
	if (wram_.ow_bars_state() || wram_.ow_scene_action() == SPECIAL_ACTION)
		return;
	uint8_t last = wram_.last_transition_param();
	if (last == TRANSITION_EPILOGUE_NOT_PERFECT || last == TRANSITION_EPILOGUE_PERFECT)
		return;
	wram_.stored_transition_param() = last;
}

// an unlocked level's exit (to the next side of the map) lights up
void Overworld::load_pals_for_scene()
{
	wram_.ow_exit_unlocked() = 0;
	if (wram_.ow_scene_action() == UNLOCK_LEVEL) {
		gb_->cpu.a = wram_.ow_scene_map_side();
		gb_->cpu.c = mem_[wOWSceneMapSide + 1]; // wOWSceneActionParam
		SET_HL(gb_, ADDR_UnlockableConnections);
		routines_.call(ADDR_GetCthWordFromAthTable);
		if (mem_[REG_HL(gb_)] >= OW_EXITS)
			wram_.ow_exit_unlocked() = TRUE;
	}
	routines_.farcall(BANK__LoadPalsForOWScene, ADDR__LoadPalsForOWScene);
}

void Overworld::load_west_maps()
{
	load_bg_maps(BANK_OverworldWestTilemap, ADDR_OverworldWestTilemap,
		BANK_OverworldWestAttrmap, ADDR_OverworldWestAttrmap);
}

// with the castle, once revealed
void Overworld::load_south_maps()
{
	load_bg_maps(BANK_OverworldSouthTilemap, ADDR_OverworldSouthTilemap,
		BANK_OverworldSouthAttrmap, ADDR_OverworldSouthAttrmap);
	if (call_z(ADDR_Func_819c6, EVENT_REVEAL_CASTLE))
		return;
	SET_HL(gb_, ADDR_Data_8561f);
	routines_.farcall(BANK_Func_854ee, ADDR_Func_854ee);
}

void Overworld::load_east_maps()
{
	load_bg_maps(BANK_OverworldEastTilemap, ADDR_OverworldEastTilemap,
		BANK_OverworldEastAttrmap, ADDR_OverworldEastAttrmap);
}

void Overworld::write_bg_map_from_wram()
{
	routines_.copy_bc(wTilemap, v0BGMap0, 19 * TILEMAP_WIDTH);
	set_vram1(true);
	routines_.copy_bc(wAttrmap, v1BGMap0, 19 * TILEMAP_WIDTH);
	set_vram1(false);
}

// the clouds' starting state
void Overworld::reset_clouds()
{
	wram_.unk_2d032() = 0x1e;
	wram_.unk_2d035() = 0x04;
	wram_.unk_2d038() = 0x24;
	wram_.unk_2d03b() = 0x24;
	wram_.unk_2d03e() = 0x0e;
	wram_.unk_2d041() = 0x14;
	for (uint16_t addr : {w2d031, wCloudType, w2d049, w2d033, w2d036, w2d039, w2d03c, w2d03f,
		 w2d042, w2d034, w2d037, w2d03a, w2d03d, w2d040, w2d043})
		mem_[addr] = 0;
	wram_.unk_2d046() = 0x10;
	wram_.unk_2d044() = 0;
	wram_.unk_2d047() = 0;
	wram_.unk_2d048() = 0;
	switch (wram_.cur_map_side()) {
	case WEST:
	case EAST:
		wram_.cloud_type() = 0x01;
		break;
	case SOUTH:
		wram_.unk_2d03b() = 0x04;
		break;
	}
}

void Overworld::clear_pal7_colours()
{
	for (int i = 0; i < 6; i++)
		mem_[static_cast<uint16_t>(wTempPals1 + PAL_SIZE * 7 + COLOR_SIZE * 1 + i)] = 0;
}

} // namespace wl3::overworld

// Called from MainStateTable, which returns to the main loop: it reloads
// every register.
PORT_EX(HandleOverworld, 0, ALL_REGISTERS)
{
	wl3::overworld::Overworld(gb).handle();
}

// Cutscenes jump back here when they end; their callers (the state table)
// use no register after it.
PORT_LOCAL_EX(InitOverworld, after_cutscene, 0, ALL_REGISTERS)
{
	wl3::overworld::Overworld(gb).after_cutscene();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(OverworldStateTable);
PORT_COVERED(SelectLevel);
PORT_COVERED(InitOverworld);
PORT_COVERED(Func_804ec);
PORT_COVERED(InitOWScene);
PORT_COVERED(Func_80540);
PORT_COVERED(InitMapSide);
PORT_COVERED(Func_805d7);
PORT_COVERED(InitNorthMapSide);
PORT_COVERED(InitWestMapSide);
PORT_COVERED(InitSouthMapSide);
PORT_COVERED(InitEastMapSide);
PORT_COVERED(Func_8065e);
PORT_COVERED(InitMapSideForScene_North);
PORT_COVERED(InitMapSideForScene_West);
PORT_COVERED(InitMapSideForScene_South);
PORT_COVERED(InitMapSideForScene_East);
PORT_COVERED(Func_8086f);
PORT_COVERED(LoadBGMapsToWRAM);
PORT_COVERED(LoadOverworldCommonGfx);
PORT_COVERED(LoadOverworld1Gfx);
PORT_COVERED(LoadOverworld2Gfx);
PORT_COVERED(LoadOverworld5Gfx);
PORT_COVERED(LoadOverworld6Gfx);
PORT_COVERED(LoadOverworldArrowsGfx);
PORT_COVERED(LoadOverworldGlowGfx);
PORT_COVERED(LoadMapSidePals);
PORT_COVERED(LoadPalsForOWScene);
PORT_COVERED(Func_80ae7);
PORT_COVERED(Func_80af5);
PORT_COVERED(Func_80b1b);
PORT_COVERED(ClearOWWRAM);
PORT_COVERED(WriteBGMapFromWRAM);
PORT_COVERED(Func_80b54);
PORT_COVERED(LoadCloudAndSeaGfx);
PORT_COVERED(SetStoredTransitionParamAsReturnToMap);
PORT_COVERED(UpdateStoredTransitionParam);
