// The level clear screens (src/engine/clear), as C++: the treasure screen
// (Wario, the treasure found and the music boxes), the time attack result,
// and the musical coins tally, where a pencil marks the level's square when
// all its coins were found.
//
// Still C: the V-Blank functions copied to wVBlankFunc (clear/vblank.c) and
// _CheckLevelMusicalCoinFlag, which home code calls with registers
// (clear/musical_coin_flags.c).
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::clear {

class ClearScreen {
public:
	explicit ClearScreen(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

	void run_state(); // _ClearScreenStateTable

private:
	MenuObject object(uint16_t base) const { return MenuObject(gb_, base); }
	MenuObject wario() const { return object(wClearScreenWario); }
	void unrealistic();
	// animates obj and draws it (LoadClearScreenSprite)
	void update_object(MenuObject obj);
	void draw(MenuObject obj); // LoadClearScreenSprite
	void draw_wario(); // LoadClearScreenWarioSprite
	// Wario's second animation, once his first one has played once
	void animate_wario(Byte played, uint16_t next_frameset);
	void stop_sfx();
	void copy_vblank(uint16_t func, uint8_t size);

	// treasure_clear.asm
	void init(); // InitClearScreen
	void count_music_boxes();
	uint8_t music_boxes() const; // 1 to 5, or 0
	void update(); // UpdateClearScreen
	void set_treasure_transition_param(); // SetTreasureTransitionParam
	void update_num_collected_treasures(); // UpdateNumCollectedTreasures
	void load_gfx(); // LoadClearScreenGfx
	void set_up_sprites(); // Func_1e015b

	// time_attack_clear.asm
	void init_time_attack(); // InitTimeAttackClearScreen
	void time_attack_result(); // Func_1e005f
	void print_time(uint16_t coord, uint16_t time);
	void set_up_time_attack_sprites(); // Func_1e028a
	void update_time_attack(); // Func_d434c

	// musical_coins.asm, musical_coin_flags.asm, pencil.asm
	void init_musical_coins(); // InitMusicalCoinScreen
	void update_coin_box(); // the box of coins, wMenuObj3
	void count_coins(); // Func_d4472
	void finish_soon(); // Func_d45e1
	void pencil_moves(); // Func_d4507
	bool move_pencil(); // DoPencilMovement: whether it's there
	void pencil_scribbles(); // Func_d454c
	void draw_pencil(); // LoadPencilSprite
	void exit(); // ExitClearScreen
	void draw_coin_flags(); // Func_d47f1
	void set_musical_coin_flag(); // SetMusicalCoinFlag

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

} // namespace wl3::clear
