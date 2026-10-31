// The golf building (choosing a course), the golf lobby and the golf clear
// cutscene (src/engine/golf/lobby.asm, building/*.asm, state_table.asm), as
// C++. The holes themselves (hole.asm) are still register-level C.
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

class Golf {
public:
	explicit Golf(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	// _GolfStateTable and _GolfBuildingStateTable: one step of the current
	// substate
	void run_golf_state();
	void run_building_state();

private:
	Byte ram(uint16_t addr) const { return mem_[addr]; }
	GolfObject object(uint16_t base) const { return GolfObject(gb_, base); }
	GolfObject wario() const { return object(wGolfWario); }
	void set_oam_pointer(uint16_t oam);
	void update_object(GolfObject obj, uint8_t y, uint8_t x, uint16_t frameset); // and load its sprite
	void clear_golf_ram_from(uint16_t start);
	void reset_scroll();
	void unrealistic();

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

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

} // namespace wl3::golf
