// The map's scenes (bank20.asm): what an event changes on the map, set up
// for each unlocked event when a side loads (Func_81dce), and played when
// it's just been unlocked (Func_818ad): a level unlocked (its path's
// arrows appear), a level highlighted, or a side's own function (bank2d).
// Each event's scene is a list of actions in OWSceneParams. See
// overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

static_assert(wTileToPlaceInOW + 1 == wAttrToPlaceInOW && static_cast<uint8_t>(wTileToPlaceInOW) != 0xff);
static_assert(w2d062 == wOWFuncCounter + 1);

constexpr uint8_t SCENE_END = 0x80;
constexpr uint8_t SEEN = 0x80; // w2d065: unlocked before, already played

// The steps UnlockLevel and Func_82c33 share
enum Shared : uint8_t {
	NOT_SHARED,
	GREY, // Func_82a0a
	GREY_DONE, // Func_82a26
	NAME, // Func_82a2f
	NAME_FADE, // Func_82a4e
	STAR_PALS, // Func_82a62
};

// UnlockLevel's and Func_82c33's steps that wait, by frames
constexpr uint8_t GREY_WAIT = 20;
constexpr uint8_t STAR_PALS_WAIT = 16;
constexpr uint8_t ARROW_WAIT = 4;
constexpr uint8_t NEW_LEVEL_WAIT = 30;
constexpr uint8_t MARKER_WAIT = 0x40;

constexpr uint8_t VERTICAL_ARROW = 0x01;
constexpr uint8_t HORIZONTAL_ARROW = 0x02;
constexpr uint8_t X_FLIP = 1 << 5;
constexpr uint8_t Y_FLIP = 1 << 6;

// hl, dec h x3: the attribute under a tile of wTilemap (wAttrmap)
uint16_t attr_of(uint16_t tile)
{
	return static_cast<uint16_t>((static_cast<uint8_t>((tile >> 8) - 3) << 8) | (tile & 0xff));
}

} // namespace

void Scenes::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// AddOffsetInPointerTable: hl + a * 2 (the carry of a * 2 into d)
uint16_t Scenes::add_offset(uint16_t hl, uint8_t a)
{
	return static_cast<uint16_t>(hl + a * 2);
}

// GetCthWordFromAthTable: the a-th pointer of table, plus c words
uint16_t Scenes::cth_word(uint16_t table, uint8_t a, uint8_t c)
{
	return add_offset(mem_.word(add_offset(table, a)), c);
}

// GetMapLevelCoords
uint16_t Scenes::map_level_coords(uint8_t level)
{
	gb_->cpu.a = level;
	routines_.call(ADDR_GetMapLevelCoords);
	return REG_HL(gb_);
}

// GetOWCoordInTilemap: the tile at the coords in hl (also left in
// wOWTilemapPtr)
uint16_t Scenes::coord_in_tilemap(uint16_t coords)
{
	SET_HL(gb_, coords);
	routines_.call(ADDR_GetOWCoordInTilemap);
	return REG_HL(gb_);
}

void Scenes::copy_far_bytes(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count)
{
	SET_HL(gb_, src);
	SET_DE(gb_, dest);
	gb_->cpu.c = count;
	gb_->cpu.b = bank;
	routines_.call(ADDR_CopyFarBytes);
}

// ---------------------------------------------------------------------------
// Scene params

// GetOWSceneParams: the first action of event's scene; false if it has
// none (the asm's z)
bool Scenes::get_params(uint8_t event)
{
	return read_params(mem_.word(add_offset(ADDR_OWSceneParams, event)));
}

// GetOWSceneParams_GotPtr: the action at ptr; false at the scene's end
bool Scenes::read_params(uint16_t ptr)
{
	uint8_t action = mem_[ptr];
	wram_.ow_scene_action() = action;
	if (action == SCENE_END)
		return false;
	wram_.ow_scene_map_side() = mem_[static_cast<uint16_t>(ptr + 1)];
	wram_.ow_scene_action_param() = mem_[static_cast<uint16_t>(ptr + 2)];
	wram_.ow_scene_params_ptr().word() = static_cast<uint16_t>(ptr + 3);
	return true;
}

// the next action (GetByteFromPointerInHL, GetOWSceneParams_GotPtr)
bool Scenes::next_params()
{
	return read_params(wram_.ow_scene_params_ptr().word());
}

// ---------------------------------------------------------------------------
// Func_81dce: every event's scene, set up on this side

void Scenes::init()
{
	wram_.temp_event() = EVENT_PROLOGUE;
	uint8_t last = wram_.last_transition_param();
	if (!last)
		wram_.last_transition_param() = 0xff;
	else if (last == TRANSITION_NEW_GAME)
		wram_.last_transition_param() = 0;

	for (;;) {
		wram_.unk_2d078() = 0;
		gb_->cpu.a = wram_.temp_event();
		routines_.call(ADDR_CheckIfEventIsUnlocked);
		uint8_t unlocked = gb_->cpu.a;
		wram_.unk_2d065() = unlocked;
		// its c, TRUE unless the last treasure unlocked it: seen before
		if (unlocked && static_cast<uint8_t>(gb_->cpu.c - 1))
			wram_.unk_2d065() = SEEN;
		prepare_scene();
		uint8_t next = static_cast<uint8_t>(wram_.temp_event() + 1);
		if (next == NUM_EVENTS)
			break;
		wram_.temp_event() = next;
	}
	if (static_cast<uint8_t>(wram_.last_transition_param() + 1) == 0)
		wram_.last_transition_param() = 0; // TRANSITION_NEW_GAME
}

// Func_81dce.PrepareScene: the scene's actions on this side, set up
void Scenes::prepare_scene()
{
	if (!get_params(wram_.temp_event()))
		return;
	do {
		if (wram_.ow_scene_map_side() == wram_.cur_map_side())
			init_action(); // .DoInitFunc
	} while (next_params());
}

void Scenes::init_action()
{
	switch (wram_.ow_scene_action()) {
	case UNLOCK_LEVEL:
		draw_unlocked_levels();
		return;
	case DO_OW_FUNC:
	case SPECIAL_ACTION: // InitOWObjects
		routines_.farcall(BANK__InitOWObjects, ADDR__InitOWObjects);
		return;
	case HIGHLIGHT_LEVEL: // InitHighlightLevelScene: its star's palettes
		if (wram_.unk_2d065() == SEEN)
			copy_far_bytes(BANK_Pals_848e0, ADDR_Pals_848e0, wTempPals2 + 4 * PAL_SIZE, 2 * PAL_SIZE);
		return;
	default: // Func_81e44: debug_nop, or past the table
		unrealistic();
		return;
	}
}

// DrawUnlockedLevels: an unlocked level's path's arrows, already there
void Scenes::draw_unlocked_levels()
{
	if (!static_cast<uint8_t>(wram_.unk_2d065() << 1))
		return; // not unlocked
	wram_.unk_2d072() = 0;
	get_unlocked_level_data();
	get_unlocked_level_arrows();
	do {
		draw_connection(); // .Func_81e88
		++wram_.unk_2d072();
	} while (wram_.unk_2d074() != wram_.unk_2d072());
	wram_.unk_2d072() = 0;
	wram_.ow_tilemap_ptr().word() = 0;
	wram_.attr_to_place_in_ow() = 0;
	wram_.tile_to_place_in_ow() = 0;
	wram_.ow_path_shape() = 0;
}

// DrawUnlockedLevels.Func_81e88: the arrows by the two levels a path
// connects (none by a left exit or the junction), and the point on the
// second
void Scenes::draw_connection()
{
	if (!left_exit_or_junction()) {
		draw_arrow();
		if (wram_.level2() == OW_EXIT_RIGHT)
			return;
	}
	wram_.owx_coord1() = wram_.owx_coord2();
	wram_.owy_coord1() = wram_.owy_coord2();
	wram_.level_arrow_directions1() = wram_.level_arrow_directions2();
	draw_arrow();

	show_level_point();
	place_tile(coord_in_tilemap(wOWCoords2));
}

void Scenes::draw_arrow()
{
	uint8_t arrows = wram_.level_arrow_directions1();
	show_level_arrow(arrows);
	move_to_arrow(arrows);
	place_tile(connected_level_tile());
}

// The tile and attribute to place, at tile (as Func_81feb sets the
// attribute)
void Scenes::place_tile(uint16_t tile)
{
	wram_.unk_2d07f() = TRUE;
	apply_tile_pal();
	mem_[tile] = wram_.tile_to_place_in_ow();
	mem_[attr_of(tile)] = wram_.attr_to_place_in_ow();
	wram_.unk_2d07f() = FALSE;
}

// IsLeftExitOrJunction
bool Scenes::left_exit_or_junction()
{
	uint8_t level = wram_.level1();
	if (level == OW_EXIT_LEFT)
		return true;
	if (wram_.ow_scene_map_side() != NORTH)
		return false;
	return level == OWNORTH_JUNCTION;
}

// GetUnlockedOWLevelData: the path's two levels (wLevel1 and wLevel2),
// their coords and the path's arrows
void Scenes::get_unlocked_level_data()
{
	uint8_t side = wram_.ow_scene_map_side();
	uint8_t param = wram_.ow_scene_action_param();
	uint16_t connection = cth_word(ADDR_UnlockableConnections, side, param);
	wram_.level1() = mem_[connection];
	wram_.level2() = mem_[static_cast<uint16_t>(connection + 1)];
	uint16_t arrows = cth_word(ADDR_UnlockableLevelArrows, side, param);
	wram_.unlocked_level_arrows_ptr().word() = mem_.word(arrows);

	uint16_t coords = map_level_coords(wram_.level1());
	wram_.unk_2d069() = mem_[coords];
	wram_.owy_coord1() = mem_[coords];
	wram_.unk_2d06a() = mem_[static_cast<uint16_t>(coords + 1)];
	wram_.owx_coord1() = mem_[static_cast<uint16_t>(coords + 1)];
	coords = map_level_coords(wram_.level2());
	wram_.owy_coord2() = mem_[coords];
	wram_.owx_coord2() = mem_[static_cast<uint16_t>(coords + 1)];
}

// GetUnlockedLevelArrows: the path's arrows, by wOWPathShape
void Scenes::get_unlocked_level_arrows()
{
	uint16_t arrows = add_offset(wram_.unlocked_level_arrows_ptr().word(), wram_.ow_path_shape());
	wram_.level_arrow_directions1() = mem_[arrows];
	wram_.unk_2d074() = mem_[static_cast<uint16_t>(arrows + 1)];
	uint8_t second = mem_[static_cast<uint16_t>(arrows + 2)];
	wram_.level_arrow_directions2() = second;
	wram_.unk_2d076() = second == 0xff ? 0 : mem_[static_cast<uint16_t>(arrows + 3)];
}

// GetConnectedLevel1ArrowCoord: wOWCoords1 a tile towards the arrow
void Scenes::move_to_arrow(uint8_t arrows)
{
	if (arrows & PAD_RIGHT)
		wram_.owx_coord1() += 8;
	else if (arrows & PAD_LEFT)
		wram_.owx_coord1() -= 8;
	else if (arrows & PAD_UP)
		wram_.owy_coord1() -= 8;
	else if (arrows & PAD_DOWN)
		wram_.owy_coord1() += 8;
}

// GetConnectedLevel1CoordsInTilemap
uint16_t Scenes::connected_level_tile()
{
	wram_.temp_ow_tilemap_ptr().word() = wram_.ow_tilemap_ptr().word();
	return coord_in_tilemap(wOWCoords1);
}

// ShowLevelArrowInOW: the arrow's tile and flips
void Scenes::show_level_arrow(uint8_t arrows)
{
	Byte attr = wram_.attr_to_place_in_ow();
	attr = 0;
	if (arrows & PAD_RIGHT) {
		wram_.tile_to_place_in_ow() = HORIZONTAL_ARROW;
	} else if (arrows & PAD_LEFT) {
		attr.set_bit(5);
		wram_.tile_to_place_in_ow() = HORIZONTAL_ARROW;
	} else if (arrows & PAD_UP) {
		wram_.tile_to_place_in_ow() = VERTICAL_ARROW;
	} else if (arrows & PAD_DOWN) {
		attr.set_bit(6);
		wram_.tile_to_place_in_ow() = VERTICAL_ARROW;
	}
	static_assert(X_FLIP == 1 << 5 && Y_FLIP == 1 << 6);
}

// ShowLevelPointInOW
void Scenes::show_level_point()
{
	wram_.attr_to_place_in_ow() = 0;
	wram_.tile_to_place_in_ow() = 0;
}

// Func_81feb: palette 7 and VRAM bank 1 for the tile to place
void Scenes::apply_tile_pal()
{
	if (!wram_.unk_2d07f())
		return;
	uint8_t unused = mem_[wram_.ow_tilemap_ptr().word()]; // read, and never used
	(void)unused;
	Byte attr = wram_.attr_to_place_in_ow();
	attr = static_cast<uint8_t>((attr & ~BG_PALMASK) | 7 | BG_BANK1);
	if (wram_.unk_2d07f() != 1)
		unrealistic(); // debug_nop
}

// Func_81f21: the tiles queued from wTileToPlaceInOW (tile, attribute,
// address), w2d07f of them, to wTilemap and wAttrmap
void Scenes::place_queued_tiles()
{
	if (!wram_.unk_2d07f())
		return;
	uint16_t entry = wTileToPlaceInOW;
	do {
		auto at = [&](uint8_t i) {
			return static_cast<uint16_t>((entry & 0xff00) | static_cast<uint8_t>(entry + i));
		};
		uint8_t tile = mem_[at(0)];
		uint8_t attr = mem_[at(1)];
		uint16_t addr = static_cast<uint16_t>(mem_[at(2)] | mem_[at(3)] << 8);
		mem_[addr] = tile;
		mem_[attr_of(addr)] = attr;
		entry = at(4);
	} while (--wram_.unk_2d07f());
	wram_.ow_pending_tile_update() = TRUE;
}

// ---------------------------------------------------------------------------
// Func_818ad: playing the scene

void Scenes::update()
{
	routines_.farcall(BANK_UpdateCommonOWAnimations, ADDR_UpdateCommonOWAnimations);
	routines_.farcall(BANK_UpdateMapSideOWAnimations, ADDR_UpdateMapSideOWAnimations);
	if (wram_.ow_bars_state() && wram_.joypad_pressed().bit(B_PAD_B)) {
		end(); // skipped
		return;
	}
	++wram_.unk_2d014();
	switch (wram_.unk_2d013()) {
	case 0: // Func_818e7
		if (wram_.unk_2d014() < 4)
			return;
		break;
	case 1: // Func_818f6: the action, until it clears its param
		play_action();
		if (wram_.ow_scene_action_param())
			return;
		break;
	case 2: // Func_81900: the next action, here or on another side
		if (wram_.unk_2d014() < 4)
			return;
		if (!next_params()) {
			end();
			return;
		}
		if (wram_.ow_scene_map_side() == wram_.cur_map_side()) {
			wram_.unk_2d014() = 0;
			wram_.unk_2d013() = 0;
			return;
		}
		stop_sfx();
		wram_.sub_state() = 0x04;
		return;
	default:
		unrealistic();
		return;
	}
	// Func_818ed
	wram_.unk_2d014() = 0;
	++wram_.unk_2d013();
}

// Func_81931: the scene is over
void Scenes::end()
{
	stop_sfx();
	uint8_t event = wram_.cur_event();
	wram_.last_transition_param() = 0;
	wram_.cur_event() = 0;
	wram_.ow_scene_action() = 0;
	wram_.temp_ow_scene_action() = 0;
	if (event == EVENT_EPILOGUE) {
		wram_.sub_state() = SST_OVERWORLD_EPILOGUE;
		return;
	}
	++wram_.sub_state();
	routines_.call(ADDR_Func_803e6);
}

void Scenes::stop_sfx()
{
	mem_[hSFXID + 0] = 0xff;
	mem_[hSFXID + 1] = 0x00;
}

// Func_8195d
void Scenes::play_action()
{
	switch (wram_.ow_scene_action()) {
	case UNLOCK_LEVEL:
		unlock_level();
		return;
	case DO_OW_FUNC:
	case SPECIAL_ACTION: // DoOWFunction
		routines_.farcall(BANK__DoOWFunction, ADDR__DoOWFunction);
		return;
	case HIGHLIGHT_LEVEL:
		highlight_level();
		return;
	default: // .InvalidAction: debug_nop
		unrealistic();
		return;
	}
}

// Func_82a8d, Func_82c22: the next step, from its first frame
void Scenes::next_step()
{
	wram_.ow_func_counter() = 0;
	++wram_.unk_2d062();
}

// Func_82c29: the action is over
void Scenes::finish_action()
{
	wram_.ow_scene_action_param() = 0;
	wram_.ow_func_counter() = 0;
	wram_.unk_2d062() = 0;
}

// Func_82c1d: wait until the counter passes frames
void Scenes::wait(uint8_t frames)
{
	if (frames >= wram_.ow_func_counter())
		return;
	next_step();
}

// The steps UnlockLevel and Func_82c33 share: the map grey, the level's
// name, the star palettes. False if the step isn't one of them.
bool Scenes::common_step(uint8_t step)
{
	switch (step) {
	case GREY: // fade to grey
		routines_.farcall(BANK_OWGreyTransition, ADDR_OWGreyTransition);
		if (wram_.ow_pal_transition_state())
			return true;
		gb_->cpu.ime = false; // di
		routines_.call(ADDR_VBlank_OWScene);
		gb_->cpu.ime = true; // ei
		gb_->cpu.ime_delay = 0;
		next_step();
		return true;
	case GREY_DONE:
		wait(GREY_WAIT);
		return true;
	case NAME: { // the level's name, if it isn't an exit
		uint8_t level = wram_.level2();
		if (level >= OW_EXITS) {
			wram_.unk_2d062() += 2; // no name to fade in
			wram_.ow_func_counter() = 0;
			return true;
		}
		gb_->cpu.b = wram_.ow_scene_map_side();
		gb_->cpu.d = level;
		routines_.call(ADDR_LoadLevelName);
		wram_.pal_config1_total_steps() = 0;
		next_step();
		return true;
	}
	case NAME_FADE:
		routines_.farcall(BANK_FadeInLevelName, ADDR_FadeInLevelName);
		if (gb_->cpu.c & 1) // still fading in
			return true;
		next_step();
		return true;
	case STAR_PALS:
		if (wram_.ow_func_counter() < STAR_PALS_WAIT)
			return true;
		routines_.farcall(BANK_LoadOWStarIndicatorPals, ADDR_LoadOWStarIndicatorPals);
		next_step();
		return true;
	default:
		return false;
	}
}

// UnlockLevel: the path to a new level appears, arrow by arrow
void Scenes::unlock_level()
{
	++wram_.ow_func_counter();
	static constexpr Shared SHARED[16] = {
		NOT_SHARED, GREY, GREY_DONE, NAME, NAME_FADE, STAR_PALS};
	uint8_t step = wram_.unk_2d062();
	if (step < 16 && SHARED[step]) {
		common_step(SHARED[step]);
		return;
	}
	switch (step) {
	case 0: // Func_82a79
		get_unlocked_level_data();
		wram_.ow_path_shape() = 0;
		wram_.unk_2d072() = 0;
		for (uint16_t i = 0; i < 8; i++)
			mem_[wCurSceneObj + i] = 0;
		next_step();
		return;
	case 6: // Func_82a96
		get_unlocked_level_arrows();
		next_step();
		return;
	case 7: // Func_82a9b: a few frames after each arrow
	case 9:
	case 11:
		wait_after_tiles(ARROW_WAIT);
		return;
	case 8: // Func_82aae: the first level's arrow
		if (left_exit_or_junction()) {
			next_step();
			return;
		}
		place_arrow(wram_.level_arrow_directions1());
		return;
	case 10: // Func_82aea: the second level's
		if (wram_.level2() == OW_EXIT_RIGHT) {
			next_step();
			return;
		}
		wram_.owx_coord1() = wram_.owx_coord2();
		wram_.owy_coord1() = wram_.owy_coord2();
		wram_.level_arrow_directions1() = wram_.level_arrow_directions2();
		place_arrow(wram_.level_arrow_directions1());
		return;
	case 12: // Func_82b22: the second level's point
		if (wram_.level2() == OW_EXIT_RIGHT) {
			next_step();
			return;
		}
		coord_in_tilemap(wOWCoords2);
		show_level_point();
		wram_.unk_2d07f() = 1;
		place_tiles_and_next();
		return;
	case 13: // Func_82b67
		wait_after_tiles(NEW_LEVEL_WAIT);
		return;
	case 14: { // Func_82b7b: the new level's marker
		uint8_t frames = wram_.ow_func_counter();
		if (frames == 1) {
			if (wram_.level2() >= OW_EXITS) {
				next_step();
				return;
			}
			wram_.cur_scene_obj_y_coord() = wram_.owy_coord2();
			wram_.cur_scene_obj_x_coord() = wram_.owx_coord2();
			wram_.cur_scene_obj_state() = 1;
			play_sfx(mem_, SFX_104);
			return;
		}
		if (frames < MARKER_WAIT)
			return;
		next_step();
		return;
	}
	case 15: // Func_82baa
		if (wram_.ow_func_counter() < NEW_LEVEL_WAIT)
			return;
		wram_.ow_scene_action_param() = 0;
		wram_.unk_2d062() = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// Func_82a9b, Func_82b67: once the tiles are placed, frames more
void Scenes::wait_after_tiles(uint8_t frames)
{
	if (wram_.unk_2d07f()) {
		wram_.ow_func_counter() = 0;
		return;
	}
	if (wram_.ow_func_counter() < frames)
		return;
	next_step();
}

// Func_82aae, Func_82aea: an arrow, a tile from wOWCoords1
void Scenes::place_arrow(uint8_t arrows)
{
	move_to_arrow(arrows);
	connected_level_tile();
	show_level_arrow(wram_.level_arrow_directions1());
	wram_.unk_2d07f() = TRUE;
	play_sfx(mem_, SFX_103);
	place_tiles_and_next();
}

// Func_82ad2
void Scenes::place_tiles_and_next()
{
	apply_tile_pal();
	place_queued_tiles();
	next_step();
}

// Func_82c33: a level highlighted, with its star
void Scenes::highlight_level()
{
	++wram_.ow_func_counter();
	static constexpr Shared SHARED[11] = {
		NOT_SHARED, NOT_SHARED, GREY, GREY_DONE, NAME, NAME_FADE, STAR_PALS};
	uint8_t step = wram_.unk_2d062();
	if (step < 11 && SHARED[step]) {
		common_step(SHARED[step]);
		return;
	}
	switch (step) {
	case 0: { // .Func_82c55: the level (params count the north's from 0, the
		// others' from 1)
		uint8_t side = wram_.cur_map_side();
		uint8_t param = wram_.ow_scene_action_param();
		wram_.level2() = static_cast<uint8_t>(param - (side != NORTH ? 1 : 0));
		uint8_t event = wram_.cur_event();
		if (event == EVENT_GOLD_MUSIC_BOX) {
			wram_.ow_scene_action_param() = 0x07;
		} else if (event == EVENT_SUPER_GRAB_GLOVES && side == EAST && param == 0x07) {
			gb_->cpu.a = MYSTERY_HANDLE;
			routines_.call(ADDR_IsTreasureCollected);
			if (gb_->cpu.f & FLAG_Z) { // not collected: nothing to show
				finish_action();
				return;
			}
		}
		next_step();
		return;
	}
	case 1: // .Func_82c8f
		wait(12);
		return;
	case 7: // .Func_82c93: the star on the level
		gb_->cpu.c = wram_.level2();
		SET_DE(gb_, wCurSceneObj);
		routines_.call(ADDR_SetObjPositionToLevelCoords);
		SET_HL(gb_, wCurSceneObjState);
		gb_->cpu.a = 0x02;
		routines_.call(ADDR_SetSceneObjState);
		play_sfx(mem_, SFX_104);
		next_step();
		return;
	case 8: // .Func_82cb0: until its animation ends
		if (wram_.cur_scene_obj_state())
			return;
		next_step();
		return;
	case 9: // .Func_82c51
		wait(30);
		return;
	case 10: // Func_82c29
		finish_action();
		return;
	default:
		unrealistic();
		return;
	}
}

// Func_82bb8: the unlocked level's marker
void Scenes::draw_unlock_marker()
{
	if (wram_.ow_scene_action() != UNLOCK_LEVEL || !wram_.cur_scene_obj_state())
		return;
	update_marker_animation(BANK_Frameset_aa544, ADDR_Frameset_aa544);
	draw_marker();
}

// Func_82bda: the highlighted level's star, until its animation ends
void Scenes::draw_highlight_marker()
{
	if (wram_.ow_scene_action() != HIGHLIGHT_LEVEL || !wram_.cur_scene_obj_state())
		return;
	update_marker_animation(BANK_Frameset_aa555, ADDR_Frameset_aa555);
	if (wram_.ow_animation_finished() == 0xff) {
		wram_.cur_scene_obj_state() = 0;
		return;
	}
	draw_marker();
}

void Scenes::update_marker_animation(uint8_t bank, uint16_t frameset)
{
	SET_HL(gb_, wCurSceneObjDuration);
	SET_DE(gb_, frameset);
	gb_->cpu.b = bank;
	routines_.call(ADDR_UpdateOWAnimation);
}

void Scenes::draw_marker()
{
	SET_HL(gb_, wCurSceneObj);
	SET_DE(gb_, ADDR_OAM_aa445);
	gb_->cpu.b = BANK_OAM_aa445;
	routines_.call(ADDR_LoadOWSpriteWithScroll);
}

} // namespace wl3::overworld

// The entry points init.asm calls (overworld.cpp, C++ too); what runs after
// each loads its own registers.
PORT_EX(Func_81dce, 0, ALL_REGISTERS)
{
	wl3::overworld::Scenes(gb).init();
}

PORT_EX(Func_818ad, 0, ALL_REGISTERS)
{
	wl3::overworld::Scenes(gb).update();
}

PORT_EX(Func_82bb8, 0, ALL_REGISTERS)
{
	wl3::overworld::Scenes(gb).draw_unlock_marker();
}

PORT_EX(Func_82bda, 0, ALL_REGISTERS)
{
	wl3::overworld::Scenes(gb).draw_highlight_marker();
}

PORT_COVERED(Func_81e44);
PORT_COVERED(InitOWObjects);
PORT_COVERED(DrawUnlockedLevels);
PORT_COVERED(InitHighlightLevelScene);
PORT_COVERED(GetOWSceneParams);
PORT_COVERED(GetOWSceneParams_GotPtr);
PORT_COVERED(GetUnlockedOWLevelData);
PORT_COVERED(GetUnlockedLevelArrows);
PORT_COVERED(GetConnectedLevel1ArrowCoord);
PORT_COVERED(GetConnectedLevel1CoordsInTilemap);
PORT_COVERED(IsLeftExitOrJunction);
PORT_COVERED(ShowLevelPointInOW);
PORT_COVERED(ShowLevelArrowInOW);
PORT_COVERED(Func_81feb);
PORT_COVERED(Func_81f21);
PORT_COVERED(Func_818e7);
PORT_COVERED(Func_818ed);
PORT_COVERED(Func_818f6);
PORT_COVERED(Func_81900);
PORT_COVERED(Func_81931);
PORT_COVERED(Func_8195d);
PORT_COVERED(DoOWFunction);
PORT_COVERED(UnlockLevel);
PORT_COVERED(Func_82a0a);
PORT_COVERED(Func_82a26);
PORT_COVERED(Func_82a2f);
PORT_COVERED(Func_82a4e);
PORT_COVERED(Func_82a62);
PORT_COVERED(Func_82a79);
PORT_COVERED(Func_82a8d);
PORT_COVERED(Func_82a96);
PORT_COVERED(Func_82a9b);
PORT_COVERED(Func_82aae);
PORT_COVERED(Func_82ad2);
PORT_COVERED(Func_82aea);
PORT_COVERED(Func_82b22);
PORT_COVERED(Func_82b67);
PORT_COVERED(Func_82b7b);
PORT_COVERED(Func_82baa);
PORT_COVERED(Func_82c1d);
PORT_COVERED(Func_82c22);
PORT_COVERED(Func_82c29);
PORT_COVERED(Func_82c33);
