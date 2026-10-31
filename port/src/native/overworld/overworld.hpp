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
#include <initializer_list>

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
	void set_compass(); // SetCompassSprite
	void load_compass(); // LoadCompassSprite
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
	void set_wario_oam(uint8_t oam); // SetOWWarioOAM
	uint8_t next_path_leg(); // Func_810fc
	void path_distances(); // Func_821a1
	void path_step(); // Func_82208
	uint16_t level_paths(); // Func_82111

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
	void face_path(); // Func_81142
	void update_objects(); // Func_81055
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

// The map's functions (bank2d.asm, _DoOWFunction's NOWFunc_* and the
// like): the events' scenes on each side, step by step (w2d062, its frames
// in wOWFuncCounter). Each is hooked at its own address, so they convert
// one at a time.
class MapFunctions {
public:
	explicit MapFunctions(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void run(); // _DoOWFunction

	// north
	void cut_tree(); // NOWFunc_CutTree
	void open_gate(); // NOWFunc_OpenGate
	void north_magic_seed(); // NOWFunc_MagicSeed
	void north_summon_snake(); // NOWFunc_SummonSnake
	void fall_leaves(); // NOWFunc_FallLeaves
	void play_music_box(); // NOWFunc_PlayMusicBox
	void crayon(); // NOWFunc_Crayon
	void north_bottom_bar(); // NOWFunc_BottomBar
	void day_night_spell(); // NOWFunc_DayNightSpell
	void prologue(); // NOWFunc_Prologue
	// west
	void rain(); // WOWFunc_Rain
	void clear_tornado(); // WOWFunc_ClearTornado
	void elevator_working(); // WOWFunc_ElevatorWorking
	void west_magic_seed(); // WOWFunc_MagicSeed
	void west_summon_snake(); // WOWFunc_SummonSnake
	void west_earthquake(); // WOWFunc_Earthquake
	void west_volcano(); // WOWFunc_VulcanoEruption
	void red_music_box(); // WOWFunc_RedMusicBox
	// south
	void south_magic_seed(); // SOWFunc_MagicSeed
	void yellow_music_box(); // SOWFunc_YellowMusicBox
	void south_summon_snake(); // SOWFunc_SummonSnake
	void canyon_thunder(); // SOWFunc_CanyonThunder
	void raise_tower(); // SOWFunc_RaiseTower
	void fan(); // SOWFunc_Fan
	void south_earthquake(); // SOWFunc_Earthquake
	void green_music_box(); // SOWFunc_GreenMusicBox
	void explosives(); // SOWFunc_Explosives, EOWFunc_Explosives
	void full_moon(); // SOWFunc_FullMoon
	void send_seeds(); // SOWFunc_SendSeeds
	// east
	void freeze_sea(); // EOWFunc_FreezeSea
	void east_earthquake(); // EOWFunc_Earthquake
	void burn_vines(); // EOWFunc_BurnVines
	void show_warped_void(); // EOWFunc_ShowWarpedVoid
	void raise_pipe(); // EOWFunc_RaisePipe
	void daytime(); // EOWFunc_Daytime
	void form_castle(); // EOWFunc_FormCastle
	void east_volcano(); // EOWFunc_VulcanoEruption
	void finish(); // EndOWFunc
	void shake(); // Func_b71fc
	void rumble(); // Func_b6055
	void update_tornado(); // UpdateOWTornado

private:
	void unrealistic();
	uint8_t step() { return wram_.unk_2d062(); }
	void next_step(); // AdvanceOWFunc
	void wait(uint8_t frames); // WaitOWFunc
	void wait_state(uint16_t state, uint8_t value); // WaitOWObjState
	void load_level_name(); // LoadOWSceneLevelName
	void wait_level_name(); // WaitOWSceneLevelNameFadeIn
	uint8_t falling_offset(uint16_t counter); // GetSceneFallingYOffset
	bool move(uint16_t obj, uint16_t data); // ApplyOWMovement; whether still moving
	void set_state(uint16_t state, uint8_t value); // SetSceneObjState

	struct Seed {
		uint8_t land_y, land, landed, grow, grown; // y, then scene object 4's states
	};
	void magic_seed(const Seed &seed);
	struct Snake {
		uint8_t wait; // before the level name
		uint16_t flag; // the notes moving
		uint8_t notes_x; // where the snake comes out
		uint16_t obj; // the snake
		uint8_t state, out_frame;
		int in_frame; // the frame it's back in at (north), or -1
	};
	void summon_snake(const Snake &snake);
	void tile_update(uint16_t data); // Func_b5868
	void rain_pals(); // WOWFunc_Rain.UpdatePals
	void pal_config(uint16_t config, uint8_t reg, uint16_t src);
	void flash(uint16_t colours, uint8_t count); // Func_b69a6
	void load_one_pal(uint8_t bank, uint16_t pals, uint8_t pal);
	void show_button(uint16_t block);
	void shake_steps();
	void start_shaking(uint8_t script);
	void stop_shaking();
	void quake(bool on);
	uint8_t seed_x_offset(uint16_t counter);
	void castle_pals();
	struct Fountain {
		uint16_t flag, state; // the phase (and its frames after), the object's state
		uint8_t start;
		int done; // the object's state when it's up, or -1
	};
	void lava_fountain(const Fountain &fountain);
	void freeze_tiles(uint16_t base, uint16_t offset, uint8_t bank, uint16_t update);
	void crumble_cliffs();
	void load_pals(uint8_t bank, uint16_t src, uint16_t dest);
	void canyon_flash();
	void bolt_flash();
	void canyon_pals(uint16_t pals);
	void canyon_white();
	void canyon_bolts();
	struct Bob {
		bool zero;
		uint8_t a;
	};
	Bob bob_tower(uint8_t period, uint8_t step_mask);
	bool blow_gusts(uint16_t flag, uint16_t obj);
	struct Place {
		uint8_t y, x, state;
	};
	struct Rock {
		uint16_t flag, obj; // the phase (and a byte after it), the object
		int8_t dx;
		uint8_t phases; // 8, or 2 for a single place
		Place places[4];
	};
	void fly_rock(const Rock &rock);

	void move_leaf(uint16_t flag, uint16_t leaf, uint16_t twin, uint16_t twin_flag, bool sfx);
	void move_twin(uint16_t flag, uint16_t leaf);

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map scenes' objects at their start (bank2d.asm, _InitOWObjects): each
// side's events place their scene objects (and tiles, palettes, graphics)
// by w2d065: 0 before the event, 0x80 to play it, 1 after.
class SceneInit {
public:
	explicit SceneInit(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void run(); // _InitOWObjects

private:
	struct Place {
		uint8_t y, x, state;
	};
	void unrealistic();
	void place(uint16_t obj, Place place); // InitSceneObjParams
	void tiles(uint16_t data); // Func_b586d
	void priority(uint16_t attrs, uint8_t count); // SetBGMapBytesPriority
	void decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1);
	void gfx(uint8_t bank, uint16_t src, uint8_t tile); // the LoadOverworld*Gfx
	bool unlockable() const;

	void north(uint8_t param, uint8_t c); // InitOWObjects_North
	void west(uint8_t param, uint8_t c); // InitOWObjects_West
	void south(uint8_t param, uint8_t c); // InitOWObjects_South
	void east(uint8_t param, uint8_t c); // InitOWObjects_East
	void bottom_bar(); // Func_b417a
	void top_bar(); // Func_b4247

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map sides' scene objects between scenes (bank2d.asm,
// UpdateMapSideOWAnimations): each side's objects by state, peeking out,
// switching day and night, the leaves falling.
class SideAnimations {
public:
	explicit SideAnimations(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // UpdateMapSideOWAnimations
	void fall_leaves(); // Func_b542e

private:
	struct Peek {
		uint16_t flag, timer; // out (and how long it's waited)
		uint8_t level_a, level_b; // the map's levels it shows at
		uint16_t state; // the object's
		uint8_t hide, show;
	};
	void unrealistic();
	void set_state(uint16_t state, uint8_t value); // SetSceneObjState
	bool by_day() const { return !mem_[w2d011]; }
	void north(uint8_t state); // Func_b7984
	void west(uint8_t state); // Func_b7a6d
	void south(uint8_t state); // Func_b7c18
	void east(uint8_t state); // Func_b7b61
	void peek(const Peek &peek); // Func_b7a32, Func_b7b26, Func_b7c5c
	void leaf(uint16_t phase, uint16_t obj); // Func_b542e.Func_b5435 / .Func_b54ba
	bool east_busy(); // Func_b7be9
	void moon(); // Func_b7cb3

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The map's events and their treasures (bank20.asm): which treasures
// unlock which event (EventTreasures), the next treasure to look for and
// the marks on levels with treasures left.
class Events {
public:
	explicit Events(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	struct Unlocked {
		bool all; // the event's treasures are all collected
		bool not_last; // none of them was the last one collected (c TRUE)
	};
	struct SideLevel {
		uint8_t side, level;
	};
	Unlocked event_unlocked(uint8_t event); // CheckIfEventIsUnlocked
	uint8_t unlocked_before(uint8_t event); // Func_819c6
	bool treasure_unlocks_event(); // CheckIfTreasureUnlocksEvent
	void next_treasure_to_collect(); // GetNextTreasureToCollect
	SideLevel map_level_id(uint8_t level); // GetMapLevelID
	SideLevel next_treasure_side_level(); // GetMapSideAndLevelForNextTreasure
	void show_missing_treasure_indicators(); // ShowMissingTreasureIndicators
	void set_top_bar_buttons(); // SetDayNightSpellSelectable to UpdateNextPrevMapButtonsSelectable
	uint8_t accessible_map_flags(); // GetAccessibleMapFlags
	void load_level_name_if_valid(); // LoadLevelNameIfValid

private:
	struct Found {
		uint8_t event;
		uint16_t next; // where the search stopped
		bool found;
	};
	void unrealistic();
	bool collected(uint8_t treasure); // IsTreasureCollected
	Found event_with_treasure(uint8_t treasure); // GetEventWithTreasure
	void load_event_treasures(uint8_t event); // LoadEventTreasures

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The golf building, outside (bank2b.asm): Wario walks in from the side
// he came from, to the door, and goes in or off to a side.
class GolfBuilding {
public:
	explicit GolfBuilding(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void init(); // _GolfBuilding
	void update(); // Func_adbfe

private:
	void unrealistic();
	void wario_state(uint8_t state); // SetSceneWarioState
	uint8_t walk(bool right); // ApplySceneWarioMovementLeft / Right
	void decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1);
	void place_wario(); // _GolfBuilding.Func_ada43
	void fill_bg_pals(); // _GolfBuilding.FillBGPalettes
	void leave(); // Func_adbfe.exit
	void step(); // Func_adbfe.Func_adc2f

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The epilogue (bank2b.asm): the old man's thanks, the music boxes, the
// transformation, the crowd, the gift, and Wario thrown out of the
// castle with its treasure. Steps in w2d013 (frames in w2d014); most
// have sub-steps in w2d890 (frames in w2d891).
class Epilogue {
public:
	explicit Epilogue(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void init(); // _InitEpilogue
	void update(); // Func_af13c

private:
	void unrealistic();
	void wario_state(uint8_t state); // SetSceneWarioState
	void set_state(uint16_t state, uint8_t value); // SetSceneObjState
	void far_pals(uint8_t bank, uint16_t src, uint16_t dest);
	void decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1);
	void next_step(); // Func_af135
	void wait(uint8_t frames); // Func_af130
	void next_sub(); // .asm_af40c
	void end_sub(); // .asm_af413
	void wait_wario(uint8_t state); // .asm_af400
	void wait_fade(); // .Func_af41c
	void hdma(uint8_t bank, uint8_t vram_bank, uint16_t src, uint8_t dest_hi, uint8_t mode);
	void load_pals(); // .Func_af1bd
	void load_gift_pals(); // .Func_af1db
	void step(); // .Func_af14c
	void crowd(); // .Func_af8bc
	void crowd_start(); // .Func_af23e
	void old_man_talks(); // .Func_af376
	void music_boxes(); // .Func_af7af
	void transformation(); // .Func_af741
	void leave_on_gift(); // .Func_af670
	void thrown_out(); // .Func_af436
	void bounce_wario(); // .Func_af57b
	void bounce_treasure(); // .Func_af60d

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The prologue (bank2b.asm, PrologueFunc's cutscene): Wario falls from
// the plane, finds the cave and the music box, is sucked in and wakes in
// the temple. Steps in w2d013 (frames in w2d014); the long sub-scenes
// (Prologue_Wario*) still run by address.
class Prologue {
public:
	explicit Prologue(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // Prologue
	void music_box_close_up(); // Func_ae1de
	void enter_cave(); // Prologue_WarioEnterCave
	void sucked_into_music_box(); // Prologue_WarioSuckedIntoMusicBox
	void wario_flicker(); // Func_ae523
	void wake_up(); // Func_ae55c
	void bow(); // Func_ae641
	void walk_off(); // Func_ae668
	void fall_from_plane(); // Prologue_WarioFallFromPlane
	void walk_to_cave(); // Prologue_WarioWalkToCaveEntrance
	void plane_parts(); // Func_ae8bb

private:
	void unrealistic();
	void next_sub(); // Func_ae1ce
	void wait_sub(uint8_t frames); // Func_ae1c9
	void end_sub(); // Func_ae1d5
	uint8_t walk_right(); // ApplySceneWarioMovementRight
	uint8_t walk_left(); // ApplySceneWarioMovementLeft
	void wait_wario(uint8_t state); // Func_ae1c2
	void bounce(); // Prologue_WarioFallFromPlane.DoBounce
	void next_action(uint16_t counter); // Func_ae915
	void plane_part(int index); // Func_ae924 and the like
	void music_box_glow(); // Func_ae2dd
	void wario_state(uint8_t state); // SetSceneWarioState
	void set_state(uint16_t state, uint8_t value); // SetSceneObjState
	void far_copy(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count);
	void next_step(); // Prologue_Next
	void wait(uint8_t frames); // Prologue_Delay
	void set_fade(uint8_t fade); // .Func_ae009
	void place_music_box(); // .Func_ae067
	void load_pals(uint16_t pals); // .Func_ae0ab
	void hdma(uint8_t vram_bank, uint16_t src, uint8_t dest_hi, uint8_t mode);
	void step(); // .PlayCutscene

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// The temple, inside (bank2b.asm): Wario walks in, the hidden figure
// tells him what's next (or, with the gold music box, reveals himself:
// HiddenFigureRevealScene, still by address) and he walks out.
class TempleScene {
public:
	explicit TempleScene(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void init(); // _InitTempleScene
	void update(); // _UpdateTempleScene
	void reveal(); // HiddenFigureRevealScene
	void enter_with_music_boxes(); // HandleEnterTempleWithAllMusicBoxesScene
	void show_music_boxes(); // Func_ad445
	void figure_appears(); // HandleHiddenFigureReveal
	void music_boxes(); // HandleTempleMusicBoxes
	void shake_scene(); // HandleTempleShakeScene
	void wario_panic(); // HandleTempleWarioPanic
	void left_hand(); // DoHiddenFigureLeftHandMovement
	void right_hand(); // DoHiddenFigureRightHandMovement

private:
	void next_sub(); // Func_ad335
	void wait_sub(uint8_t frames); // Func_ad330
	void wait_wario(uint8_t state); // Func_ad329
	void end_sub(); // Func_ad33c
	void music_box_hover(int index); // HandleTempleMusicBox1Hover and the like
	void next_step(); // Func_ad016
	void wait(uint8_t frames); // Func_ad011
	void fight(); // SetFightAgainstAHiddenFigure
	void hdma(uint8_t vram_bank, uint16_t src, uint8_t dest_hi);
	void panic_then_next(); // .Func_ad16a
	void close_mouth(); // .HandleHiddenFigureMouthClosing
	void set_state(uint16_t state, uint8_t value); // SetSceneObjState
	void unrealistic();
	void wario_state(uint8_t state); // SetSceneWarioState
	void decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1);
	void load_far(uint16_t routine, uint8_t bank, uint16_t src);
	void next_state(); // .NextState
	void step(); // .UpdateScene
	void leave(); // .exit
	void temple_music(); // .DelayAndPlayTempleMusic

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// Wario in the overworld's scenes (bank2b.asm): his state's animation
// (a table of framesets and the states they end in), his sprite, and the
// sprites and tiles of each group of states.
class SceneWario {
public:
	explicit SceneWario(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void update(); // UpdateSceneWario
	void set_state(uint8_t state); // SetSceneWarioState

private:
	void unrealistic();
	void animate(); // UpdateSceneWario.DoAnimation
	void footsteps(); // Func_ac4fa
	void panic_steps(); // Func_ac515
	void look_around_sounds(); // UpdateSceneWario.asm_ac65f
	void look_up_sounds();

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// The map's palettes and tile updates (bank21.asm): the day and night
// transitions (each colour stepped in a ColourFadeStruct), the PalConfig
// fades (FadePalConfig), the level name's, and the scenes' tile blocks.
class Palettes {
public:
	explicit Palettes(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void day_night(); // DayNightTransition
	void to_daytime(); // Func_84b97
	void spell_transition(); // Func_84c0d
	void grey_transition(); // OWGreyTransition
	void load_grey(); // LoadGreyOWPals
	bool fade_in_level_name(); // FadeInLevelName; whether still fading
	void fade_out_level_name(); // Func_852e5
	void func_84e43();
	void func_84fff();
	void func_85046();
	void func_850b9();
	void func_850ff();
	void func_85145();
	void func_8518c();
	void prologue_fade(); // _PrologueFade
	void star_indicator_pals(); // LoadOWStarIndicatorPals
	void update_tiles(uint16_t data); // Func_854ee
	void freeze_column(uint16_t base); // Func_85a68, Func_85aa2, Func_85acb

private:
	void unrealistic();
	bool fade_pal_config(uint16_t steps); // FadePalConfig; whether short of 0x20
	bool fade_bg(); // FadeBGPalConfig
	bool fade_bg_slow(); // Func_851bc
	bool fade_ob_slow(); // Func_851d1
	void config(uint16_t config, uint16_t src, uint8_t index, uint8_t count);
	bool fade_all(bool ob);
	void next_w2d013();
	void to_fade_structs(uint16_t src, uint16_t dest); // CopyPalToColourFadeStructs
	void init_fade_structs(); // InitColourFadeStructs
	void apply_fade_structs(); // ApplyPalFadeColours
	void next_transition(); // DayNightTransition.AdvanceTable
	uint16_t ow_pals(uint8_t which); // GetOWPals
	void load_base_and_target(uint8_t which); // LoadBaseAndTargetOWPals
	void transition_vblank(); // VBlank_OWPalTransition
	void start_transition(); // DayNightTransition.LoadBaseAndTargetPals
	void fade_colours(); // DayNightTransition.FadeColours
	void skip_transition(); // SkipDayNightTransition
	void switch_day_night(); // DayNightTransition.SwitchDayNight

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
