// The golf minigame (src/engine/golf), as C++: the golf building (choosing
// a course), the lobby, the holes and the clear cutscene.
//
// Their state stays in the Golf RAM WRAM bank, which the callers map.
#pragma once

#include "../routines.hpp"

namespace wl3::golf {

// A golf_obj_struct: a sprite animated from a frameset.
class GolfObject {
public:
	GolfObject(GB *gb, uint16_t base) : gb_(gb), base_(base) {}

	Byte state() const { return byte(0); }
	Byte duration() const { return byte(1); }
	Byte frameset_offset() const { return byte(2); }
	Byte current_frame() const { return byte(3); }
	uint16_t sprite() const { return static_cast<uint16_t>(base_ + 4); }
	Byte y() const { return byte(4); }
	Byte x() const { return byte(5); }
	Byte frame() const { return byte(6); }

private:
	Byte byte(int offset) const { return Byte(gb_, static_cast<uint16_t>(base_ + offset)); }

	GB *gb_;
	uint16_t base_;
};

// What the golf screens share: the machine, and the golf objects.
class GolfScreen {
public:
	explicit GolfScreen(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

protected:
	Byte ram(uint16_t addr) const { return mem_[addr]; }
	Word ram_word(uint16_t addr) const { return mem_.word(addr); }
	GolfObject object(uint16_t base) const { return GolfObject(gb_, base); }
	GolfObject wario() const { return object(wGolfWario); }
	void set_oam_pointer(uint16_t oam);
	// sets the object's position, animates it and loads its sprite
	void update_object(GolfObject obj, uint8_t y, uint8_t x, uint16_t frameset);
	void load_object_sprite(GolfObject obj); // frame = current frame, load the sprite
	void clear_golf_ram_from(uint16_t start);
	void reset_scroll();
	// a jump table index out of range: the original jumps to whatever
	// follows the table
	void unrealistic();

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// The golf building (choosing a course), the lobby and the clear cutscene
class Golf : public GolfScreen {
public:
	using GolfScreen::GolfScreen;

	// _GolfStateTable and _GolfBuildingStateTable: one step of the current
	// substate
	void run_golf_state();
	void run_building_state();

private:
	// lobby.asm
	void init_lobby();
	void load_lobby_gfx();
	void handle_lobby();
	void handle_lobby_states();
	void lobby_waiting_input();
	bool lobby_turning(bool clear_dir); // whether Wario walks
	void lobby_walking_right();
	void lobby_walking_left();
	void lobby_entering_pipe();
	void lobby_entering_door();
	void lobby_exiting_pipe();
	void lobby_wait_input();
	void set_wario_state(uint8_t state);
	void update_lobby_animations();
	void handle_lobby_wario_sprite();
	void init_clear_cutscene();
	void update_clear_cutscene();
	void update_clear_cutscene_animation();

	// building/building.asm: choosing a course
	void init_building(); // Func_1c8586
	void draw_course_menu(); // Func_1c8604
	void load_course(); // Func_1c87db
	void set_course_target_scx(); // GetGolfCourseTargetSCX
	void handle_building(); // Func_1c86dd
	void handle_course_menu(); // Func_1c8837
	void scroll_to_course(); // Func_1c88dc
	void stop_scrolling(); // Func_1c8907
	void pay_for_course(); // Func_1c8911
	void update_building_animations(); // Func_1c86f8
};

// A golf hole (hole.asm): Wario hits the Para-Goom towards the hole, over
// fairway, rough, bunkers, water and out of bounds, in as few strokes as
// he can. The V-Blank functions it copies to wVBlankFunc and
// HoldGolfWarioLastFrame (called from home code) are still register-level C
// (src/ported/engine/golf/hole_ram.c).
class GolfHole : public GolfScreen {
public:
	using GolfScreen::GolfScreen;

	void init(); // InitGolfHole
	void handle(); // HandleGolfHole
	void run_unreferenced_state(); // Func_1c8957
	// the V-Blank function for the hole, the building and the lobby, by
	// wGolfVBlankMode (Func_1c9cf9, Func_1c9fae, GolfLobbyVBlank)
	void set_hole_vblank();
	void set_building_vblank();
	void set_lobby_vblank();
	void load_wario_state_gfx(); // LoadGolfWarioStateGfx

private:
	// setting up
	void set_up_hole(); // InitGolfHole.SetUpGolfHole
	void pick_and_load_hole();
	void show_flag_and_par(); // ShowGolfHoleFlagAndPar
	void set_up_objects(); // SetUpGolfHoleObjects
	void print_stroke_number();

	// the states (GolfHoleStateTable)
	void run_state();
	void state_wait_input();
	void state_scroll();
	void scroll_by_input(); // .HandleDirectionalInputScroll
	void state_select_shot_power();
	void state_select_shot_spin();
	void state_start_shot();
	void state_shot();
	void state_bounce();
	void state_special_terrain();
	void state_walk();
	void state_between_shots();
	void state_cleared();
	void state_game_over();
	void state_pause();
	void state_result();

	// the Para-Goom
	void update_paragoom_x(); // UpdateGolfParaGoomXPos
	void update_paragoom_y(); // UpdateGolfParaGoomYPos
	bool respawn_paragoom(); // HandleGolfParaGoomRespawn; whether it did
	void set_respawn_y_if_enough_strokes();
	void become_idle(); // Wario and the Para-Goom, after a shot lands
	void update_hole_direction();
	void walk_to_paragoom(); // .asm_1c976f

	// the screen
	void align_x_scroll(); // AlignGolfXScrollWithParaGoom
	void update_display_mode();
	bool in_view(uint16_t x); // Func_1c8f56
	void update_ui(); // UpdateGolfHoleUI
	void update_paragoom(); // UpdateGolfParaGoom
	void update_wario(); // UpdateGolfWario
	void show_result_overlay_text();
	void show_score(); // ShowGolfScore
	bool animate_overlay_text(uint16_t frameset); // whether it finished
	bool animate_overlay_text_1cb016();
	void set_coin_sprites(); // Func_1c9fbd
	void copy_vblank_func(uint16_t func, uint8_t length);
	void set_attrmap_dma();
};

} // namespace wl3::golf
