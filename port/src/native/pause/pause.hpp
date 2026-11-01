// The pause menu (src/engine/pause), as C++: the menu over the level
// (return, save, Action Help, back to the map) with the level's keys and
// treasures, and its state table.
//
// The save screens and saving (save.asm) are `SaveScreen`, the checksums
// and save validation `SaveData`. Still register-level: the save screen's
// graphics loaders (bank7d.cpp), its V-Blank functions (save/vblank.cpp),
// and DrawLevelObjectsAfterLevelReturn, which home code calls.
#pragma once

#include "../menu_object.hpp"
#include "../routines.hpp"

namespace wl3::pause {

class PauseMenu {
public:
	explicit PauseMenu(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), routines_(gb) {}

	void run_state(); // _PauseMenuStateTable
	void return_from_action_help(); // _ReturnToPauseMenuFromActionHelp
	void draw(MenuObject obj); // LoadPauseMenuSprite

private:
	MenuObject object(uint16_t base) const { return MenuObject(gb_, base); }
	MenuObject cursor() const { return object(wMenuObj2); }
	void unrealistic();

	// pause_menu_1.asm
	void init(); // InitPauseMenu
	void init_menu(); // InitPauseMenu_SkipBackupVRAM
	void show(MenuObject obj, uint8_t y, uint8_t x, uint8_t attributes, uint16_t frameset);
	void update(); // UpdatePauseMenu
	void exit(); // ExitPauseMenu

	// pause_menu_2.asm
	void print_music_coins(); // PrintNumberMusicCoins
	void handle_input(); // HandlePauseMenuInput
	// the cursor on an option (or, with select, chosen)
	void highlight(uint8_t selection, uint8_t y, uint8_t x, uint16_t frameset);
	void highlight_save();
	void highlight_action_help();
	void highlight_to_map();
	void highlight_return();
	void choose(uint16_t frameset, uint16_t sfx); // the cursor's chosen animation
	// by the transformation: owl, rail, vampire, or none
	uint16_t transformation_frameset(uint16_t owl, uint16_t rail, uint16_t vampire, uint16_t other) const;
	bool time_attack() const { return wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F); }

	// bank7d.asm
	void load_gfx(); // LoadPauseMenuGfx
	void draw_maps(); // Func_1f403f
	void draw_time(uint16_t coord, uint16_t time);

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
	Routines routines_;
};

// save.asm: the save screen, and writing the game or the level in progress
// to SRAM with checksums and backups.
class SaveScreen {
public:
	explicit SaveScreen(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void init(bool backup); // InitSaveScreen, InitSaveScreenAndBackupVRAM
	void fill_bar(); // FillSaveScreenBar
	void save(); // Save
	void save_level(); // SaveLevel
	void handle_complete_box(); // HandleSaveCompleteBox
	void transition_after_save(); // TransitionAfterSave

private:
	void mark(uint8_t step);
	uint16_t write_check_values(uint16_t dest, const uint8_t (&values)[4]);
	uint16_t copy(uint16_t src, uint16_t dest, uint8_t count); // returns the end
	uint16_t copy_treasures(uint16_t dest);
	void save_game_data(uint16_t dest);
	void save_level_data(uint16_t dest);
	uint16_t checksum(uint16_t sum, uint16_t dest);
	void write_sum(uint16_t dest, uint16_t sum);
	void write_echoes(uint8_t step, uint16_t echo1, uint16_t echo2, uint16_t sum);
	void switch_sram(uint8_t bank); // sramswitch
	void show_complete_box();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

// checksum.asm and validate_save.asm: the save's checksums, and on boot
// checking the save and its backups, keeping the newest good copy.
// Checksums are 16-bit byte sums, stored high byte first.
class SaveData {
public:
	explicit SaveData(GB *gb) : mem_(gb), wram_(gb) {}

	void validate(); // ValidateSaveData
	void increment_save_counter(); // IncrementSaveCounter
	uint16_t game_data_checksum(); // CalculateGameDataChecksum
	uint16_t wram_data_checksum(); // CalculateWRAMDataChecksum
	uint16_t backup_sram_checksum1(); // CalculateBackupSRAMChecksum1
	uint16_t backup_sram_checksum2(); // CalculateBackupSRAMChecksum2
	void back_up_sram(); // Func_1f14c6

private:
	// checksum.asm
	uint16_t sum(uint16_t sum, uint16_t &addr, uint16_t count);
	uint16_t sum_long(uint16_t sum, uint16_t &addr, uint16_t count);
	uint16_t level_data_checksum(uint16_t addr); // Func_1f0b5b
	uint16_t backup_sram_checksum(uint8_t bank, uint16_t start, uint16_t sram2);

	// validate_save.asm
	bool check_vals(uint16_t addr, const uint8_t (&vals)[4]) const; // CheckSaveVals1/2
	uint16_t game_data_checksum_at(uint16_t addr); // Func_1f0d60.CalculateChecksumAtHL
	void match_checksums(uint16_t sum, uint16_t echo1, uint16_t echo2); // .GetMatchingChecksum
	bool checksum_ok(uint16_t sum) const;
	void check(); // Func_1f0d60
	void restore(); // Func_1f1246
	void restore_game_data(uint16_t src);
	void restore_wram(uint16_t src);
	void restore_backup_sram();
	void copy_sram(uint8_t from, uint8_t to); // a whole SRAM bank
	void copy(uint16_t src, uint16_t dest, uint16_t count);

	void mark(uint8_t step);
	void clear_check_vals(uint16_t addr);
	void clear_save(uint8_t step, uint16_t addr);
	uint16_t read_sum(uint16_t addr) const;
	void write_sum(uint16_t addr, uint16_t sum);
	void write_sums(uint8_t step, const uint16_t (&dest)[3], uint16_t sum);
	void switch_sram(uint8_t bank); // sramswitch
	void switch_wram(uint8_t bank) { mem_[rWBK] = bank; }

	Memory mem_;
	Wram wram_;
};

} // namespace wl3::pause
