// The save screen and saving (src/engine/pause/save/save.asm); see
// pause.hpp.
#include "pause.hpp"

namespace wl3::pause {

namespace {

constexpr uint16_t PROGRESS_WRAM = 0xca00; // STARTOF("Progress WRAM")
constexpr uint8_t PROGRESS_WRAM_SIZE = 0x5b;
constexpr uint8_t PROGRESS_AND_LEVEL_WRAM_SIZE = 0xca; // with "Level WRAM"
constexpr uint16_t LEVEL_OBJECTS_WRAM = 0xd000; // STARTOF("Level Objects WRAM")
constexpr uint16_t LEVEL_OBJECTS_WRAM_SIZE = 0x14a;
constexpr uint8_t LEVEL_OBJECTS_WRAM_BANK = 0x01;
constexpr uint8_t TREASURES_SIZE = static_cast<uint8_t>((wOWLevel - wTreasuresCollected) + 2);

} // namespace

void SaveScreen::handle_complete_box()
{
	if (!wram_.joypad_pressed().bit(B_PAD_A)) {
		if (--mem_[wTimer + 0])
			return;
		if (--mem_[wTimer + 1])
			return;
	}
	mem_[wTimer + 0] = 0;
	mem_[wTimer + 1] = 0;
	++wram_.sub_state();
}

// The progress bar fills, a tile every 4 frames (the V-Blank function
// draws it).
void SaveScreen::fill_bar()
{
	if (++wram_.timer() < 0x04)
		return;
	wram_.timer() = 0;
	uint8_t step = static_cast<uint8_t>(wram_.unk_cee4() + 1);
	if (step != 0x0a) {
		wram_.unk_cee4() = static_cast<uint8_t>(step | (1 << 7));
		++mem_[wBGMapTileVRAM0Queue + 1];
		return;
	}
	wram_.unk_cee4() = 0;
	++wram_.sub_state();
}

void SaveScreen::transition_after_save()
{
	uint8_t state;
	switch (wram_.transition_param()) {
	case TRANSITION_EPILOGUE_NOT_PERFECT:
	case TRANSITION_EPILOGUE_PERFECT:
		state = ST_CREDITS;
		break;
	case TRANSITION_GAME_OVER:
		state = ST_GAME_OVER;
		break;
	default: // back to the map
		routines_.disable_lcd();
		routines_.call(ADDR_LoadBackupVRAM);
		wram_.reset_disabled() = 0;
		mem_[rLCDC] = LCDC_ON | LCDC_WIN_9C00 | LCDC_WIN_ON | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
		wram_.state() = ST_OVERWORLD;
		wram_.sub_state() = wram_.pending_sub_state();
		return;
	}
	wram_.reset_disabled() = 0;
	wram_.state() = state;
	wram_.sub_state() = 0;
}

// The "save in progress" box; with backup, the level's VRAM is kept first.
void SaveScreen::init(bool backup)
{
	wram_.reset_disabled() = TRUE;
	routines_.disable_lcd();
	if (backup) {
		routines_.save_backup_vram();
		routines_.call(ADDR_ClearBGMap1);
	}
	routines_.clear_virtual_oam();
	routines_.farcall(BANK_LoadSaveScreenPals, ADDR_LoadSaveScreenPals);
	routines_.farcall(BANK_LoadSaveScreenGfx, ADDR_LoadSaveScreenGfx);
	routines_.farcall(BANK_DrawSaveInProgressBox, ADDR_DrawSaveInProgressBox);
	routines_.call(ADDR_VBlank_SaveScreen);
	wram_.scy() = 0;
	mem_[rSCY] = 0;
	wram_.scx() = 0;
	mem_[rSCX] = 0;
	wram_.pal_fade_counter() = 0;
	wram_.unk_cee4() = 0;
	wram_.timer() = 0;
	// the progress bar's first tile
	mem_[wBGMapTileVRAM0Queue + 0] = static_cast<uint8_t>((v0BGMap0 + 0x164) >> 8);
	mem_[wBGMapTileVRAM0Queue + 1] = static_cast<uint8_t>(v0BGMap0 + 0x164);
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// The save step markers, the same in three places of SRAM.
void SaveScreen::mark(uint8_t step)
{
	mem_[s0a790] = step;
	mem_[s0a7e0] = step;
	mem_[s0afa0] = step;
}

// The four check values at dest (stepping within its 256 bytes); returns
// where the data goes after them.
uint16_t SaveScreen::write_check_values(uint16_t dest, const uint8_t (&values)[4])
{
	for (uint8_t value : values) {
		mem_[dest] = value;
		dest = static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest + 1));
	}
	return dest;
}

uint16_t SaveScreen::copy(uint16_t src, uint16_t dest, uint8_t count)
{
	routines_.copy(src, dest, count);
	return static_cast<uint16_t>(dest + count);
}

// the treasures and level data (WRAM2), after the rest
uint16_t SaveScreen::copy_treasures(uint16_t dest)
{
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = BANK_wTreasuresCollected;
	dest = copy(wTreasuresCollected, dest, TREASURES_SIZE);
	mem_[rWBK] = wram_bank;
	return dest;
}

void SaveScreen::save_game_data(uint16_t dest)
{
	static constexpr uint8_t CHECK_VALUES[4] = {CHECKVAL1_1, CHECKVAL1_2, CHECKVAL1_3, CHECKVAL1_4};
	dest = write_check_values(dest, CHECK_VALUES);
	dest = copy(PROGRESS_WRAM, dest, PROGRESS_WRAM_SIZE);
	copy_treasures(dest);
}

void SaveScreen::save_level_data(uint16_t dest)
{
	static constexpr uint8_t CHECK_VALUES[4] = {CHECKVAL2_1, CHECKVAL2_2, CHECKVAL2_3, CHECKVAL2_4};
	dest = write_check_values(dest, CHECK_VALUES);
	dest = copy(PROGRESS_WRAM, dest, PROGRESS_AND_LEVEL_WRAM_SIZE);
	dest = copy_treasures(dest);
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = LEVEL_OBJECTS_WRAM_BANK;
	SET_HL(gb_, LEVEL_OBJECTS_WRAM);
	SET_DE(gb_, dest);
	SET_BC(gb_, LEVEL_OBJECTS_WRAM_SIZE);
	routines_.call(ADDR_CopyHLToDE_BC);
	mem_[rWBK] = wram_bank;
}

// a checksum routine's result (de); also kept in wChecksum
uint16_t SaveScreen::checksum(uint16_t routine, uint16_t dest)
{
	routines_.call(routine);
	uint16_t sum = REG_DE(gb_);
	write_sum(dest, sum);
	mem_[wChecksum + 0] = static_cast<uint8_t>(sum >> 8);
	mem_[wChecksum + 1] = static_cast<uint8_t>(sum);
	return sum;
}

void SaveScreen::write_sum(uint16_t dest, uint16_t sum)
{
	mem_[dest] = static_cast<uint8_t>(sum >> 8);
	mem_[static_cast<uint16_t>(dest + 1)] = static_cast<uint8_t>(sum);
}

// the checksum's two echoes, each after its step marker
void SaveScreen::write_echoes(uint8_t step, uint16_t echo1, uint16_t echo2, uint16_t sum)
{
	mark(step);
	write_sum(echo1, sum);
	mark(static_cast<uint8_t>(step + 1));
	write_sum(echo2, sum);
}

void SaveScreen::switch_sram(uint8_t bank)
{
	wram_.sram_bank() = bank;
	mem_[rRAMB + 0x100] = bank;
}

// the "save complete" box, LCD on
void SaveScreen::show_complete_box()
{
	routines_.call(ADDR_VBlank_Saving);
	routines_.farcall(BANK_LoadSaveScreenPals, ADDR_LoadSaveScreenPals);
	routines_.farcall(BANK_DrawSaveCompleteBox, ADDR_DrawSaveCompleteBox);
}

// The game's progress, in SRAM and its backup, each with its checksum.
void SaveScreen::save()
{
	routines_.disable_lcd();
	routines_.call(ADDR_IncrementSaveCounter);
	uint8_t sram_bank = wram_.sram_bank();
	switch_sram(0);
	mark(0x10);
	save_game_data(sCheckVals);
	mark(0x11);
	uint16_t sum = checksum(ADDR_CalculateGameDataChecksum, sGameDataChecksum);
	write_echoes(0x12, sGameDataChecksumEcho1, sGameDataChecksumEcho2, sum);
	mark(0x20);
	save_game_data(sBackupCheckVals);
	mark(0x21);
	sum = static_cast<uint16_t>(mem_[wChecksum + 0] << 8 | mem_[wChecksum + 1]);
	write_sum(sBackupGameDataChecksum, sum);
	write_echoes(0x22, sBackupGameDataChecksumEcho1, sBackupGameDataChecksumEcho2, sum);
	mark(0);
	switch_sram(sram_bank);

	show_complete_box();
	mem_[wTimer + 0] = 0;
	mem_[wTimer + 1] = static_cast<uint8_t>(512 >> 8);
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// The level in progress, in SRAM and three backups, each with its
// checksum.
void SaveScreen::save_level()
{
	routines_.disable_lcd();
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = BANK_w3d280;
	routines_.copy(w3d280 + 12 * PAL_SIZE, wcaa1, 4 * PAL_SIZE); // palettes 12 to 15
	mem_[rWBK] = wram_bank;
	routines_.call(ADDR_IncrementSaveCounter);
	uint8_t sram_bank = wram_.sram_bank();
	switch_sram(0);

	mark(0x30);
	save_level_data(s0a000);
	mark(0x31);
	uint16_t sum = checksum(ADDR_CalculateWRAMDataChecksum, sWRAMChecksum);
	write_echoes(0x32, sWRAMChecksumEcho1, sWRAMChecksumEcho2, sum);
	mark(0x41);
	sum = checksum(ADDR_CalculateBackupSRAMChecksum1, s0a79d);
	write_echoes(0x42, s0a7ed, s0afad, sum);

	static constexpr struct { uint8_t step; uint16_t data, sum, echo1, echo2; } BACKUPS[3] = {
		{0x50, s0a800, sBackup1WRAMChecksum, sBackup1WRAMChecksumEcho1, sBackup1WRAMChecksumEcho2},
		{0x60, s0a400, sBackup2WRAMChecksum, sBackup2WRAMChecksumEcho1, sBackup2WRAMChecksumEcho2},
		{0x80, s0ac00, sBackup3WRAMChecksum, sBackup3WRAMChecksumEcho1, sBackup3WRAMChecksumEcho2},
	};
	for (const auto &b : BACKUPS) {
		if (b.step == 0x80)
			routines_.call(ADDR_Func_1f14c6);
		mark(b.step);
		save_level_data(b.data);
		mark(static_cast<uint8_t>(b.step + 1));
		sum = checksum(ADDR_CalculateWRAMDataChecksum, b.sum);
		write_echoes(static_cast<uint8_t>(b.step + 2), b.echo1, b.echo2, sum);
	}
	mark(0);
	wram_.reset_disabled() = 0;
	switch_sram(sram_bank);

	show_complete_box();
	mem_[rLCDC] = LCDC_DEFAULT;
	mem_[wTimer + 0] = 0;
	mem_[wTimer + 1] = 2;
	++wram_.sub_state();
}

} // namespace wl3::pause

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(HandleSaveCompleteBox);
PORT_COVERED(FillSaveScreenBar);
PORT_COVERED(TransitionAfterSave);
PORT_COVERED(InitSaveScreen);
PORT_COVERED(InitSaveScreenAndBackupVRAM);
PORT_COVERED(Save);
PORT_COVERED(SaveLevel);
PORT_COVERED(ResetAfterPauseMenuSave);
PORT_COVERED(ResetAfterSave);
