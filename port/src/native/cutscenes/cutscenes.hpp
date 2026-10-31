// The overworld's event cutscenes (bank27.asm), as C++, a few at a time.
//
// DoCutsceneFunc runs the cutscene of wEventWithCutscene each frame. A
// cutscene is a list of steps (w2d013, its frames in w2d014) and an
// animation routine that draws its scene objects. The ones not converted
// yet are still register-level C (src/ported/engine/overworld/bank27.c),
// called by address with the registers the asm's jumptable leaves.
#pragma once

#include "../routines.hpp"

#include <initializer_list>

namespace wl3::cutscenes {

class Cutscenes {
public:
	explicit Cutscenes(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void update(); // DoCutsceneFunc
	void init(); // InitEventCutscene

private:
	void unrealistic();
	bool play(uint8_t event); // a converted cutscene; false if it isn't
	bool init(uint8_t event); // a converted cutscene's init; false if it isn't
	// an unconverted one, the target of table's entry for event
	void call_c(uint16_t table, uint8_t event, uint16_t addr);

	// the steps' helpers
	uint8_t frame() { return wram_.unk_2d014(); } // [bc] in the asm
	void advance(); // AdvanceCutsceneFunc
	void wait(uint8_t frames); // WaitCutsceneFunc
	void end(uint8_t frames); // EndCutscene
	void finish(); // Func_9cd16: the cutscene's over
	void place(uint16_t obj, uint8_t y, uint8_t x, uint8_t state); // Func_9ccf9
	void set_state(uint16_t obj, uint8_t state); // SetSceneObjState
	void stop_sfx();
	// obj up a pixel (and dx across): the whoosh (Func_9ce1f) at y sound_at,
	// the next step at stop_at
	void rise(uint16_t obj, uint8_t sound_at, uint8_t stop_at, uint8_t dx = 0);
	void fade_in(); // Func_9cd8d
	// the palettes shown now (Func_9cd74), and pals to fade to; the next step
	void next_fade(uint8_t bank, uint16_t pals);
	void set_bg_pals_to_load(); // Func_9cd40: wTempPals1 to the BG palettes
	void set_ob_pals_to_load(); // Func_9cd5a: wTempPals2 to the OB palettes
	void copy_far(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count); // CopyFarBytes
	void shake_screen(); // Func_9cdcd: the screen shakes up and down, less and less
	// the init functions' loading
	void decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1 = false);
	void load_pals1(uint8_t bank, uint16_t pals); // LoadFarPalsToTempPals1
	void load_pals2(uint8_t bank, uint16_t pals); // LoadFarPalsToTempPals2

	// the scene objects' animation
	uint16_t frameset(uint16_t table, uint8_t state); // Func_9f0a5
	uint16_t animate(uint16_t state, uint16_t frameset, uint8_t bank = ANIMATION_BANK); // Func_9f0ae
	bool next_state_when_done(uint16_t obj, uint8_t next); // Func_9f0d0
	void draw(uint16_t obj, uint16_t oam, uint8_t bank = ANIMATION_BANK); // Func_9f0bc
	void draw_shadow(uint16_t obj, uint16_t dest, uint8_t y_check, uint8_t frame); // Func_9fb53
	// a sound looping every period frames, counted in counter (an Unk7)
	void loop_sound(uint16_t counter, uint8_t period, uint16_t sfx);
	// the sound when counter reaches period, then from 0 again
	void periodic_sound(uint16_t counter, uint8_t period, uint16_t sfx);
	// the plain animation routines: each object by its state's frameset
	void animate_all(std::initializer_list<uint16_t> states, uint16_t framesets, uint16_t oam);
	// the same, with states that go on to the next when their animation ends
	struct Next {
		uint8_t from, to;
	};
	void animate_all(std::initializer_list<uint16_t> states, uint16_t framesets, uint16_t oam,
		std::initializer_list<Next> next);

	static constexpr uint8_t ANIMATION_BANK = 0x2f;

	// the cutscenes
	// InitCutscene02, 03, 04: palettes, graphics and BG maps
	void init_north(uint16_t pals2, uint8_t pals2_bank, uint16_t gfx, uint16_t bg_maps1, uint16_t bg_maps2);
	void cut_tree(); // Cutscene02Func: EVENT_CUT_TREE
	void draw_cut_tree(); // OWCutTree
	void open_north_gate(); // Cutscene03Func: EVENT_OPEN_NORTH_GATE
	void draw_func_9f0e0(); // Func_9f0e0
	void expel_tornado(); // Cutscene04Func: EVENT_EXPEL_TORNADO
	void init_rainstorm(); // InitCutscene06
	void rainstorm(); // Cutscene06Func: EVENT_RAINSTORM
	void draw_func_9f11e(); // Func_9f11e
	void init_fix_elevator(); // InitCutscene07
	void fix_elevator(); // Cutscene07Func: EVENT_FIX_ELEVATOR
	void draw_func_9f174(); // Func_9f174
	void init_music_box(uint16_t pals); // InitCutscene08 and the like, Func_9c25f
	void music_box(uint8_t box); // Cutscene08Func and the like, Func_9d6f0
	void draw_music_box(); // Func_9fc1d
	void init_plant_seeds(); // InitCutscene09
	void plant_seeds(); // Cutscene09Func: EVENT_PLANT_SEEDS
	void draw_func_9fbfa(); // Func_9fbfa
	void init_raise_tower(); // InitCutscene0a, Func_9d0e6
	void raise_tower(); // Cutscene0aFunc: EVENT_RAISE_TOWER
	uint8_t raise_tower_step(); // Cutscene0aFunc.Func_9d133
	void draw_func_9f146(); // Func_9f146
	void init_blow_mist(); // InitCutscene0c
	void blow_mist(); // Cutscene0cFunc: EVENT_BLOW_MIST
	void init_summon_snakes(); // InitCutscene0d
	void summon_snakes(); // Cutscene0dFunc: EVENT_SUMMON_SNAKES
	bool snake_left(); // Cutscene0dFunc.Func_9d967
	void init_freeze_sea(uint16_t pals1, uint16_t pals2); // InitCutscene0e, InitCutscene11
	void freeze_sea(); // Cutscene0eFunc, Cutscene11Func: EVENT_FREEZE_SEA, EVENT_SUMMON_LIGHTNING
	void draw_func_9f1f3(); // Func_9f1f3
	void init_purify_water(); // InitCutscene18
	void purify_water(); // Cutscene18Func: EVENT_PURIFY_WATER
	void init_reveal_castle(); // InitCutscene19
	void reveal_castle(); // Cutscene19Func: EVENT_REVEAL_CASTLE
	void init_summon_sun(); // InitCutscene1b
	void summon_sun(); // Cutscene1bFunc: EVENT_SUMMON_SUN
	void draw_owl(); // Func_9f244
	void init_explode_bombs(); // InitCutscene1e, Func_9d536
	void explode_bombs(); // Cutscene1eFunc: EVENT_EXPLODE_BOMBS
	void init_leaves_fall(); // InitCutscene1f, Func_9dc6a
	void leaves_fall(); // Cutscene1fFunc: EVENT_LEAVES_FALL
	void draw_func_9f423(); // Func_9f423
	void init_treasure_map(); // InitCutscene22
	void treasure_map(); // Cutscene22Func: EVENT_TREASURE_MAP
	void init_open_tree_mouth(); // InitCutscene24, Func_9db85
	void open_tree_mouth(); // Cutscene24Func: EVENT_OPEN_TREE_MOUTH
	void draw_func_9f3ee(); // Func_9f3ee
	void init_cart_wheels(); // InitCutscene27, Func_9d64b
	void cart_wheels(); // Cutscene27Func: EVENT_PLACE_CART_WHEELS
	void draw_cart(); // Func_9f2f3
	void init_make_wire(); // InitCutscene21
	void make_wire(); // Cutscene21Func: EVENT_MAKE_WIRE
	void draw_func_9f49c(); // Func_9f49c
	void init_raise_ice_blocks(); // InitCutscene30, Func_9ded4
	void raise_ice_blocks(); // Cutscene30Func: EVENT_RAISE_ICE_BLOCKS
	void draw_func_9f5a0(); // Func_9f5a0
	void init_torch_forest(); // InitCutscene37
	void torch_forest(); // Cutscene37Func: EVENT_TORCH_FOREST
	void init_reveal_warped_void(); // InitCutscene38
	void reveal_warped_void(); // Cutscene38Func: EVENT_REVEAL_WARPED_VOID
	void init_raise_pipe(); // InitCutscene39
	void raise_pipe(); // Cutscene39Func: EVENT_RAISE_PIPE
	void init_remove_warps(); // InitCutscene3a, Func_9e018
	void remove_warps(); // Cutscene3aFunc: EVENT_REMOVE_WARPS
	void draw_func_9f638(); // Func_9f638
	void init_demons_blood(); // InitCutscene3c
	void demons_blood(); // Cutscene3cFunc: EVENT_COLLECT_DEMONS_BLOOD
	void draw_func_9f6a2(); // Func_9f6a2
	void init_paragoom_cage(); // InitCutscene3d
	void paragoom_cage(); // Cutscene3dFunc: EVENT_OPEN_PARAGOOM_CAGE
	void init_place_propeller(); // InitCutscene43
	void place_propeller(); // Cutscene43Func: EVENT_PLACE_PROPELLER
	void init_feed_octohon(); // InitCutscene44
	void feed_octohon(); // Cutscene44Func: EVENT_FEED_OCTOHON
	uint8_t octohon_step(); // Cutscene44Func.Func_9ebc7
	void init_sky_door(); // InitCutscene4a, Func_9e438
	void sky_door(); // Cutscene4aFunc: EVENT_OPEN_SKY_DOOR
	void init_extinguish_fire(); // InitCutscene4c, Func_9e4ae
	void extinguish_fire(); // Cutscene4cFunc: EVENT_EXTINGUISH_FIRE
	void draw_extinguish_fire(); // Func_9f7dc, Func_9f821
	void init_place_brick(); // InitCutscene4e
	void place_brick(); // Cutscene4eFunc: EVENT_PLACE_BRICK
	void init_spray_iron_wall(); // InitCutscene4f, Func_9e5a1
	void spray_iron_wall(); // Cutscene4fFunc: EVENT_SPRAY_IRON_WALL
	void draw_func_9f86a(); // Func_9f86a
	void init_mix_chemicals(); // InitCutscene51, Func_9e655
	void mix_chemicals(); // Cutscene51Func: EVENT_MIX_CHEMICALS
	void draw_func_9f8da(); // Func_9f8da
	void init_cut_balloon(); // InitCutscene54, Func_9e2b8
	void cut_balloon(); // Cutscene54Func: EVENT_CUT_BALOON
	void init_reveal_dark_room(); // InitCutscene56, Func_9e6ef
	void reveal_dark_room(); // Cutscene56Func: EVENT_REVEAL_DARK_ROOM
	void draw_func_9f91f(); // Func_9f91f
	void init_summon_moon(); // InitCutscene57, Func_9e79a
	void summon_moon(); // Cutscene57Func: EVENT_SUMMON_MOON
	void move_debris(); // Func_9ea9e, Func_9eab4: wSceneObj10 and 11 along their paths
	void draw_func_9f9f5(); // Func_9f9f5
	void init_crater_hole(); // InitCutscene58, Func_9e855
	void crater_hole(); // Cutscene58Func: EVENT_OPEN_CRATER_HOLE
	void init_drill_holes(); // InitCutscene55, Func_9e91a
	void drill_holes(); // Cutscene55Func: EVENT_DRILL_HOLES
	void draw_func_9fa48(); // Func_9fa48
	void init_volcano(); // InitCutscene14
	void volcano(); // Cutscene14Func: EVENT_VOLCANO_ERUPTION
	bool bob_rocks(); // Cutscene14Func.Func_9ee5b
	void draw_volcano(); // Func_9fba9
	void init_foot_stone(); // InitCutscene13
	void foot_stone(); // Cutscene13Func: EVENT_FOOT_STONE
	void init_snake_door(uint16_t pals1, uint16_t pals2); // InitCutscene15, InitCutscene49
	void snake_door(); // Cutscene15Func, Cutscene49Func
	void draw_snake_door(); // Func_9f38d

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

} // namespace wl3::cutscenes
