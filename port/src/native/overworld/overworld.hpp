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

} // namespace wl3::overworld
