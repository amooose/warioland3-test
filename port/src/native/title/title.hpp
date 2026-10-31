// The title screen (src/engine/title) and the game over screen
// (src/engine/game_over), as C++: the intro with Wario's plane, the start
// menu, the time attack description, and Wario asleep under "Game Over".
//
// The raster effects (PanIntroBackgroundLayers and the Japanese time attack
// window, which wait on rLY) and the V-Blank function copied to wVBlankFunc
// are still register-level C (src/ported/engine/title/raster.c).
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::title {

// What the screens share: the machine, and their objects.
class Screen {
public:
	explicit Screen(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

protected:
	Byte ram(uint16_t addr) const { return mem_[addr]; }
	MenuObject object(uint16_t base) const { return MenuObject(gb_, base); }
	// a jump table index out of range: the original jumps to whatever
	// follows the table
	void unrealistic();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

class Title : public Screen {
public:
	using Screen::Screen;

	void run_state(); // TitleStateTable.jump

	// wState = ST_OVERWORLD, from its start (StartOverworldState); for a
	// new game (StartOverworldStateAndNewGameParam); and coming back from
	// Action Help, with the animated tiles reset (Func_4628)
	void start_overworld();
	void start_overworld_new_game();
	void return_from_action_help();

private:
	MenuObject plane() const { return object(wPlaneWario); }

	// intro.asm
	void init_intro(); // InitIntroSequence
	void init_plane(uint8_t y, uint8_t x, uint16_t frameset);
	void set_menu_priority(); // .no_intro: the menu behind the scenery
	void intro_phase1(); // IntroSequencePhase1: the plane flies close by
	void intro_phase2(); // IntroSequencePhase2: it comes back far away
	void intro_phase3(); // IntroSequencePhase3: it explodes and nosedives
	bool phase3_fall(uint8_t y); // whether the plane reached y
	void end_intro(); // EndIntroSequence
	void start_title_screen(); // StartTitleScreen
	// the intro objects' sprites in this order, then the layers' raster
	// scroll
	void draw_intro(MenuObject first, MenuObject second, MenuObject third);

	// load_gfx.asm, load_oam.asm, vblank.asm
	void load_title_pals();
	void load_title_tiles();
	void load_title_tilemap();
	void load_time_attack_tiles();
	void load_time_attack_text();
	void set_vblank(); // VBlank_Title
	void load_intro_sprite(MenuObject obj);
	void load_start_menu_sprite(MenuObject obj);

	// intro_animation.asm, plane_sfx.asm
	void scroll_layers(); // ScrollIntroBackgroundLayers
	void finish_scroll(); // FinishIntroBackgroundScroll
	void align_layer(uint16_t layer);
	void move_clouds(); // Func_4d45
	void move_cloud(Byte x);
	void play_plane_sfx_close(); // PlayIntroSFXPlane_Close
	void play_plane_sfx_far(); // PlayIntroSFXPlane_Far
	void play_plane_sfx(uint16_t sfx); // every 13 frames

	// start_menu.asm, start_menu_selection.asm
	void start_menu(); // StartMenu
	void select_menu_option(); // StartMenu.select
	void handle_selection(); // HandleStartMenuSelection
	void handle_selection_input();
	void scroll_selection();
	void blink_selection(); // .UpdateAnim2
	void set_select_flag();
	void set_selection(uint8_t selection, uint16_t frameset);

	// time_attack_description.asm
	void init_time_attack_description(); // InitTimeAttackDescription
	void time_attack_description(); // TimeAttackDescription
};

class GameOver : public Screen {
public:
	using Screen::Screen;

	void run_state(); // _GameOverStateTable
	void load_hidden_figure_gfx(); // LoadHiddenFigureGfx (for the final battle)

private:
	MenuObject wario() const { return object(wGameOverWario); }

	void init(); // InitGameOverScreen
	void update(); // UpdateGameOverScreen
	void wake_up(); // Func_db3e4: until Wario's animation ends
	// Wario's animation (its frameset in bank), the sprites; whether
	// Wario's animation restarted
	bool update_sprites(uint8_t bank, bool snore);
	void load_pals(); // LoadGameOverPals
	void load_gfx(); // LoadGameOverGfx
	void load_tilemap(); // Func_db4cf
};

} // namespace wl3::title
