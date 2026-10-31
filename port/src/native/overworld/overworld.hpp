// The overworld (src/engine/overworld), as C++, part 1: its state table,
// HandleOverworld and level selection, and setting up the map and its
// scenes (init.asm): which side of the map, the cutscene or scene to play,
// the map's graphics, bars and sprites.
//
// The map's workings (bank20.asm to bank76.asm: Wario walking, the bars,
// the scenes and cutscenes, the treasure collection, best times, the
// temple, the epilogue) are still register-level C (src/ported/engine/
// overworld), called by address; so are Func_803e6, Func_80bd9 and the day
// and night switches, which that code calls.
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::overworld {

class Overworld {
public:
	explicit Overworld(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

	void handle(); // HandleOverworld
	void after_cutscene(); // InitOverworld.after_cutscene

private:
	void unrealistic();
	// a routine still in C taking a, and its result: a, or the Z flag
	uint8_t call_a(uint16_t addr, uint8_t a);
	bool call_z(uint16_t addr, uint8_t a);
	bool treasure_collected(uint8_t treasure) { return call_a(ADDR_IsTreasureCollected, treasure); }
	void stop_music() { stop_music2(mem_); }

	void run_state(); // OverworldStateTable
	void select_level(); // SelectLevel

	// init.asm: the map
	void init(); // InitOverworld
	void load_map_without_scene(); // InitOverworld.load_map_without_scene
	void play_scene(uint8_t state); // .play_ow_scene, .cutscene_check
	void wait_for_a(); // Func_804ec
	void update_scene(); // Func_80540
	void update_map_side(); // Func_805d7
	void init_map_side(); // InitMapSide
	void init_map_side_gfx(uint16_t gfx_routine);
	void init_east_map_side(); // InitEastMapSide
	void finish_map_side(); // Func_8065e
	void show_top_bar(); // the top bar's buttons, in the window
	void set_vram1(bool on) { mem_[rVBK] = on ? 1 : 0; }

	// init.asm: a scene on the map
	void init_scene(); // InitOWScene
	void init_scene_map_side(uint16_t gfx_routine, uint16_t maps_routine);
	void init_north_scene(); // InitMapSideForScene_North
	void finish_scene_map_side(); // Func_8086f
	void copy_level_name_gfx(uint16_t dest);

	// init.asm: loading
	void decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1 = false);
	// LoadBGMapsToWRAM: the tilemap to w3d500, the attrmap to wAttrmap
	void load_bg_maps(uint8_t tilemap_bank, uint16_t tilemap, uint8_t attrmap_bank, uint16_t attrmap);
	void load_common_gfx(); // LoadOverworldCommonGfx
	void load_glow_gfx(); // LoadOverworldGlowGfx
	void clear_ow_wram(); // ClearOWWRAM
	void set_stored_transition_param_as_return_to_map(); // SetStoredTransitionParamAsReturnToMap
	void update_stored_transition_param(); // UpdateStoredTransitionParam
	void load_pals_for_scene(); // LoadPalsForOWScene
	void load_map_side_pals(bool scene); // _LoadMapSidePals, _LoadPalsForOWScene
	void load_west_maps(); // Func_80ae7
	void load_south_maps(); // Func_80af5
	void load_east_maps(); // Func_80b1b
	void write_bg_map_from_wram(); // WriteBGMapFromWRAM
	void reset_clouds(); // Func_80b54
	void clear_pal7_colours();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

// The treasure collection (bank26.asm), the overworld's states 13 and 15.
class TreasureCollection {
public:
	explicit TreasureCollection(GB *gb);

	void init_far(); // InitTreasureCollection
	void update_far(); // TreasureCollection

private:
	void init(); // _InitTreasureCollection
	void update(); // _TreasureCollection
	void unrealistic();
	void clear(uint16_t addr, uint16_t size);
	void decompress(uint8_t bank, uint16_t src, uint16_t dest);
	void set_scene_obj_state(uint16_t state, uint8_t value); // SetSceneObjState
	bool routines_treasure_collected(uint8_t treasure);
	void load_page_gfx(); // LoadCollectionPageTreasureGfx
	void apply_page_pals(); // ApplyPageTreasuresPals
	void set_row_and_column(uint16_t dest, uint8_t cell); // GetCollectionCellRowAndColumn
	void move_cursor(); // GetCollectionCellCoords
	void handle_input(); // HandleTreasureCollectionInput
	void change_page();
	void process_link_state(); // ProcessCollectionLinkState
	void advance_link_state();
	void place_link_markers(); // ProcessCollectionLinkState.Func_9a806
	void animate(); // AnimateTreasureCollection

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map's top bar (bank20.asm): UpdateTopBar and its buttons.
class TopBar {
public:
	explicit TopBar(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // UpdateTopBar
	void init_buttons(); // UpdateTopBar.InitTopBarButtons

private:
	void unrealistic();

	// the states, by wOWBarsState
	void start_close_bottom_bar();
	void handle_bottom_bar_close();
	void open();
	void handle_input();
	void close();
	bool wait_for_selection();
	void cutscene();
	void collection();
	void next_map();
	void day_night();
	void delay_day_night_transition();
	void do_day_night_transition();

	bool handle_selection(); // HandleTopBarSelection
	void press_button(uint16_t attrs);
	void advance_obj_state(uint16_t state);
	bool handle_dpad(); // HandleTopBarDPad

	void show_selected_button(); // ShowTopBarSelectedButton
	void load_button_attributes(); // LoadTopBarButtonAttributes
	void write_button_attrs(uint16_t dest, const uint8_t *attrs); // Func_8269f
	void apply_button_attributes(); // ApplyTopBarButtonAttributes
	void draw(); // DrawTopBar
	void draw_obj(uint8_t state, uint16_t duration, uint16_t obj);
	void animate(uint8_t state, uint16_t duration); // Func_82521

	void copy_far_bytes(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count);
	void set_obj_pals_to_load();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map's bottom bar (bank20.asm): HandleBottomBar and DrawBottomBar.
class BottomBar {
public:
	explicit BottomBar(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void handle(); // HandleBottomBar
	void draw(); // DrawBottomBar

private:
	bool possible(); // HandleBottomBar.CheckBottomBarPossible
	void handle_closed();
	void handle_opened();
	void start_closing(bool clear_open_flag);
	void open(); // HandleBottomBar.OpenBottomBar
	void show_next_treasure(); // HandleBottomBar.SetVisibleTreasures
	void close(); // HandleBottomBar.CloseBottomBar

	void draw_best_time(); // Func_82997
	void draw_button_prompt(); // DrawBottomBarButtonPrompt
	void draw_8_coin(); // DrawBottomBar8Coin
	void load_sprite(uint8_t y, uint8_t x, uint8_t frame, uint8_t attributes); // LoadSpriteInsideWindow
	void load_sprite_got_params(uint8_t bank, uint16_t oam);
	void idle(int cycles);
	void unrealistic();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// Wario on a side of the map (bank20.asm): Func_80e89 and its states.
class MapWalk {
public:
	explicit MapWalk(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // Func_80e89
	void place_on_level(); // Func_8212c
	void update_wario(); // Func_82041

private:
	void unrealistic();
	void next_state();

	// the states, by w2d013
	void arrive(); // Func_80ee7
	void start_music(); // Func_80f0d
	void start_on_level(); // Func_80f3c
	void show_level_name(); // Func_80fb8
	void fade_in_level_name(); // Func_80fd6
	void on_level(); // Func_80ff7
	void walk_towards(uint8_t direction); // Func_81034
	void start_walk(); // Func_810bb
	void walk(); // Func_8115c
	void walk_out(); // Func_811cf
	void next_side(); // Func_81200
	void temple(); // Func_81346
	void temple_step(); // Func_8137a
	void set_scene_obj_state(uint16_t state, uint8_t value); // SetSceneObjState

	bool exit_step(uint8_t &step, uint8_t &end, uint8_t right_end, uint8_t left_end);
	static uint8_t dpad(uint8_t pad); // Func_810a9
	bool enter_level(); // Func_81240
	bool go_to_temple(); // Func_81077
	void load_map_level_from_level_index(); // LoadMapLevelFromLevelIndex
	uint16_t map_level_coords(uint8_t level); // GetMapLevelCoords
	void get_allowed_dpad_input(); // GetOWAllowedDPadInput
	void set_next_level(); // Func_820e6
	uint8_t next_path_leg(); // Func_810fc
	void face_path(); // Func_81142
	void update_objects(); // Func_81055
	void set_wario_oam(uint8_t oam); // SetOWWarioOAM
	void draw_wario(); // Func_8208d

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// Holding B on a level (bank20.asm): arrows along its paths.
class PathPreview {
public:
	explicit PathPreview(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // Func_81477
	void start_arrows(); // Func_814ec
	void move_arrows(); // Func_8150c

private:
	void unrealistic();
	void next_step(); // Func_814bd
	void swap_arrow(uint8_t index, bool in); // Func_8150c.Func_81576, .Func_81595
	void step_arrow(); // Func_8150c.Func_81615
	uint8_t next_path_leg(); // Func_810fc
	void face_path(); // Func_8150c.Func_816f6
	void copy_trail(uint16_t dest); // Func_8150c.Func_815c1, .Func_815e5
	void hide_trail(uint16_t state); // Func_8150c.Func_81609
	void draw_arrows(); // Func_81714
	void set_wario_oam(uint8_t oam);

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map's scenes (bank20.asm): set up for the unlocked events, and played.
class Scenes {
public:
	explicit Scenes(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void init(); // Func_81dce
	bool get_params(uint8_t event); // GetOWSceneParams
	void update(); // Func_818ad
	void draw_unlock_marker(); // Func_82bb8
	void draw_highlight_marker(); // Func_82bda

private:
	void unrealistic();
	uint16_t add_offset(uint16_t hl, uint8_t a); // AddOffsetInPointerTable
	uint16_t cth_word(uint16_t table, uint8_t a, uint8_t c); // GetCthWordFromAthTable
	uint16_t map_level_coords(uint8_t level); // GetMapLevelCoords
	uint16_t coord_in_tilemap(uint16_t coords); // GetOWCoordInTilemap
	void copy_far_bytes(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count);

	bool read_params(uint16_t ptr); // GetOWSceneParams_GotPtr
	bool next_params();
	void prepare_scene(); // Func_81dce.PrepareScene
	void init_action(); // Func_81dce.DoInitFunc
	void draw_unlocked_levels(); // DrawUnlockedLevels
	void draw_connection(); // DrawUnlockedLevels.Func_81e88
	void draw_arrow();
	void place_tile(uint16_t tile);
	bool left_exit_or_junction(); // IsLeftExitOrJunction
	void get_unlocked_level_data(); // GetUnlockedOWLevelData
	void get_unlocked_level_arrows(); // GetUnlockedLevelArrows
	void move_to_arrow(uint8_t arrows); // GetConnectedLevel1ArrowCoord
	uint16_t connected_level_tile(); // GetConnectedLevel1CoordsInTilemap
	void show_level_arrow(uint8_t arrows); // ShowLevelArrowInOW
	void show_level_point(); // ShowLevelPointInOW
	void apply_tile_pal(); // Func_81feb
	void place_queued_tiles(); // Func_81f21

	void end(); // Func_81931
	void stop_sfx();
	void play_action(); // Func_8195d
	void next_step(); // Func_82a8d, Func_82c22
	void finish_action(); // Func_82c29
	void wait(uint8_t frames); // Func_82c1d
	bool common_step(uint8_t step);
	void unlock_level(); // UnlockLevel
	void wait_after_tiles(uint8_t frames);
	void place_arrow(uint8_t arrows);
	void place_tiles_and_next(); // Func_82ad2
	void highlight_level(); // Func_82c33
	void update_marker_animation(uint8_t bank, uint16_t frameset);
	void draw_marker();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The time attack's best time list (bank56.asm).
class BestTimes {
public:
	explicit BestTimes(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void init(); // _InitBestTimeList
	void update(); // _BestTimeList

private:
	void unrealistic();
	void decompress(uint8_t bank, uint16_t src, uint16_t dest);
	void print_times(); // PrintBestTimes
	void print_time(uint8_t level); // PrintBestTimes.PrintLevelTime
	void auto_scroll();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The temple shaking, with its falling and bouncing rocks (bank56.asm).
class TempleRocks {
public:
	explicit TempleRocks(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // _HandleTempleShakeAndRocks

private:
	void unrealistic();
	void shake(); // HandleTempleScreenShake
	void next_shake_step();
	void fall(uint16_t rock, uint16_t obj, bool can_be_large, uint8_t x1, uint8_t x2, uint8_t max_y);
	void bounce(uint16_t rock, uint16_t obj, bool fast, bool left, uint8_t x1, uint8_t x2);
	void spawn(uint16_t obj, uint8_t y, uint8_t seed, uint8_t x1, uint8_t x2); // SpawnRock
	void set_size(uint16_t rock, uint16_t obj, uint8_t seed, bool can_be_large);
	void next_action(uint16_t rock);
	uint8_t y_velocity(Byte counter); // GetRockYVel
	bool move(uint16_t obj, uint16_t data, bool left); // ApplyOWMovement(_Mirrored)

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// bank56.asm's scene object scripts: the music box notes, the prologue's
// six objects.
class TempleNotes {
public:
	explicit TempleNotes(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void update_notes(); // Func_15848a
	void update_prologue_objects(); // Func_15868e

private:
	void unrealistic();
	void set_scene_obj_state(uint16_t state, uint8_t value);
	void staff(); // Func_15848a.Func_15849a
	void note(uint16_t note, uint16_t obj, bool waits, uint8_t dy, uint8_t dx, uint16_t rise, uint8_t x_step);

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// The map's scene objects (bank2d.asm): each side's object scripts.
class SceneObjects {
public:
	explicit SceneObjects(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update_before(); // Func_b4a3d
	void update_after(); // Func_b4aa9

private:
	void unrealistic();
	void run(uint8_t side, uint16_t state); // Func_b4fa2 and the like
	void run_state(uint8_t side, uint16_t state, uint8_t s); // Func_b4fab and the like
	void after(uint8_t special);

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map's common animations (bank76.asm): clouds, sea and cliffs.
class CommonAnimations {
public:
	explicit CommonAnimations(GB *gb) : mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // UpdateCommonOWAnimations
	void load_gfx(); // _LoadCloudAndSeaGfx

private:
	bool tick(uint16_t addr);
	void copy_tiles(uint16_t src, uint16_t dest, uint8_t dest_offset, int bit);
	void update_big_clouds(uint16_t addr, uint8_t offset);
	void update_sea(uint16_t addr);
	void update_small_clouds(uint16_t addr);

	Memory mem_;
	Wram wram_;
	Routines routines_;
};

} // namespace wl3::overworld
