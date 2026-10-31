// Action Help (src/engine/action_help), as C++: the screen that shows what
// each power-up lets Wario do, opened from the pause menu, after a level
// clear or as the title screen's demo.
//
// This is the screen itself: its states, the power-up selection, the
// graphics it loads and the input prompts' animation. The demonstrations
// (power_ups/*.asm) and the helpers they call (common1.asm, most of
// common2.asm) are still register-level C (src/ported/engine/action_help).
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::action_help {

class ActionHelp {
public:
	explicit ActionHelp(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

	void run_state(); // _ActionHelpStateTable

private:
	Byte ram(uint16_t addr) const { return mem_[addr]; }
	MenuObject object(uint16_t base) const { return MenuObject(gb_, base); }
	bool main_power_up() const; // not a transformation's
	void unrealistic();

	// core.asm
	void init(); // InitActionHelp
	uint8_t pick_power_up(); // the power-up to show
	void update(); // UpdateActionHelp
	void close(); // .CloseActionHelp
	void update_menu_object(MenuObject obj); // animation and sprite
	void run_main_power_up(); // ExecuteMainPowerUpActionHelp
	void run_special_power_up(); // ExecuteSpecialPowerUpActionHelp

	// common1.asm: the power-up bar
	void set_vblank(); // VBlank_1fb6b7
	void select_power_up(); // Func_1fb6f7: by the d-pad
	void colour_slot(uint8_t slot, uint8_t attributes);

	// common2.asm
	void load_common_pals(); // LoadActionHelpCommonPals
	void load_power_up_pals(); // LoadActionHelpPowerUpPals
	void load_gfx(); // LoadActionHelpGfx
	void load_bg_maps(); // LoadActionHelpBGMaps
	void draw_empty_slot(uint16_t addr); // .Func_1e499c
	void load_sprite(MenuObject obj); // LoadActionHelpSprite (farcall)

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

} // namespace wl3::action_help
