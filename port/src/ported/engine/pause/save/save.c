// Port of src/engine/pause/save/save.asm
#include "ported.h"

PORT(HandleSaveCompleteBox)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		SET_HL(gb, wTimer);
		wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
		if (!(gb->cpu.f & FLAG_Z))
			return;
		SET_HL(gb, wTimer + 1);
		wr8(gb, wTimer + 1, alu_dec(gb, rd8(gb, wTimer + 1)));
		if (!(gb->cpu.f & FLAG_Z))
			return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer + 0, 0);
	wr8(gb, wTimer + 1, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT(FillSaveScreenBar)
{
	gb->cpu.a = alu_inc(gb, rd8(gb, wTimer));
	wr8(gb, wTimer, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
	gb->cpu.a = alu_inc(gb, rd8(gb, wcee4));
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = alu_or(gb, gb->cpu.a, 1 << 7);
		wr8(gb, wcee4, gb->cpu.a);
		SET_HL(gb, wBGMapTileVRAM0Queue + 1);
		wr8(gb, wBGMapTileVRAM0Queue + 1, alu_inc(gb, rd8(gb, wBGMapTileVRAM0Queue + 1)));
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wcee4, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT(TransitionAfterSave)
{
	gb->cpu.a = rd8(gb, wTransitionParam);
	uint8_t state;
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_NOT_PERFECT);
	if (gb->cpu.f & FLAG_Z) {
		state = ST_CREDITS;
	} else {
		alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_PERFECT);
		if (gb->cpu.f & FLAG_Z) {
			state = ST_CREDITS;
		} else {
			alu_cp(gb, gb->cpu.a, TRANSITION_GAME_OVER);
			if (gb->cpu.f & FLAG_Z) {
				state = ST_GAME_OVER;
			} else {
				CALL(DisableLCD);
				CALL(LoadBackupVRAM);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wResetDisabled, 0);
				gb->cpu.a = LCDC_ON | LCDC_WIN_9C00 | LCDC_WIN_ON | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
				wr8(gb, rLCDC, gb->cpu.a);
				gb->cpu.a = ST_OVERWORLD;
				wr8(gb, wState, ST_OVERWORLD);
				gb->cpu.a = rd8(gb, wPendingSubState);
				wr8(gb, wSubState, gb->cpu.a);
				return;
			}
		}
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wResetDisabled, 0);
	SET_HL(gb, wState);
	wr8(gb, wState, state);
	wr8(gb, wSubState, 0);
}

// the farcalls to draw the "save in progress" box, VBlank_SaveScreen
static void save_screen_gfx(GB *gb)
{
	FARCALL(LoadSaveScreenPals);
	FARCALL(LoadSaveScreenGfx);
	FARCALL(DrawSaveInProgressBox);
	gb_call(gb, ADDR_VBlank_SaveScreen);
}

// the BG map queue for the progress bar, LCDC_DEFAULT, next substate
static void save_screen_finish(GB *gb)
{
	wr8(gb, wBGMapTileVRAM0Queue + 0, (uint8_t)((v0BGMap0 + 0x164) >> 8));
	wr8(gb, wBGMapTileVRAM0Queue + 1, (uint8_t)(v0BGMap0 + 0x164));
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT(InitSaveScreen)
{
	gb->cpu.a = TRUE;
	wr8(gb, wResetDisabled, gb->cpu.a);
	CALL(DisableLCD);
	CALL(ClearVirtualOAM);
	save_screen_gfx(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCY, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wPalFadeCounter, 0);
	wr8(gb, wcee4, 0);
	wr8(gb, wTimer, 0);
	save_screen_finish(gb);
}

PORT(InitSaveScreenAndBackupVRAM)
{
	gb->cpu.a = TRUE;
	wr8(gb, wResetDisabled, gb->cpu.a);
	CALL(DisableLCD);
	CALL(SaveBackupVRAM);
	CALL(ClearBGMap1);
	CALL(ClearVirtualOAM);
	save_screen_gfx(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCY, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wPalFadeCounter, 0);
	wr8(gb, wTimer, 0);
	wr8(gb, wcee4, 0);
	save_screen_finish(gb);
}

// ld a, val / ld [s0a790], a / ld [s0a7e0], a / ld [s0afa0], a
static void save_marker(GB *gb, uint8_t val)
{
	gb->cpu.a = val;
	wr8(gb, s0a790, val);
	wr8(gb, s0a7e0, val);
	wr8(gb, s0afa0, val);
}

// the check values at de, then the progress WRAM and the WRAM2 treasures
// and level data copied after them
static void save_game_data(GB *gb, uint16_t checkvals)
{
	static const uint8_t vals[4] = {CHECKVAL1_1, CHECKVAL1_2, CHECKVAL1_3, CHECKVAL1_4};
	SET_DE(gb, checkvals);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = vals[i];
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
	}
	SET_HL(gb, 0xca00); // STARTOF("Progress WRAM")
	gb->cpu.b = 0x5b;   // SIZEOF("Progress WRAM")
	CALL(CopyHLToDE);
	// push_wram BANK("WRAM2")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wTreasuresCollected;
	wr8(gb, rWBK, gb->cpu.a);
	SET_HL(gb, wTreasuresCollected);
	gb->cpu.b = (uint8_t)((wOWLevel - wTreasuresCollected) + 2);
	CALL(CopyHLToDE);
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// the checksum in de to its two echoes, with the markers before each
static void save_echoes(GB *gb, uint8_t marker, uint16_t echo1, uint16_t echo2)
{
	save_marker(gb, marker);
	wr8(gb, echo1 + 0, gb->cpu.d);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, echo1 + 1, gb->cpu.e);
	save_marker(gb, (uint8_t)(marker + 1));
	wr8(gb, echo2 + 0, gb->cpu.d);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, echo2 + 1, gb->cpu.e);
}

PORT(Save)
{
	CALL(DisableLCD);
	gb_call(gb, ADDR_IncrementSaveCounter);

	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = 0; // BANK("SRAM0")
	sramswitch(gb, 0);

	save_marker(gb, 0x10);
	save_game_data(gb, sCheckVals);
	save_marker(gb, 0x11);

	gb_call(gb, ADDR_CalculateGameDataChecksum);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, sGameDataChecksum + 0, gb->cpu.a);
	wr8(gb, wChecksum + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, sGameDataChecksum + 1, gb->cpu.a);
	wr8(gb, wChecksum + 1, gb->cpu.a);
	save_echoes(gb, 0x12, sGameDataChecksumEcho1, sGameDataChecksumEcho2);

	save_marker(gb, 0x20);
	save_game_data(gb, sBackupCheckVals);
	save_marker(gb, 0x21);

	gb->cpu.a = rd8(gb, wChecksum + 0);
	gb->cpu.d = gb->cpu.a;
	wr8(gb, sBackupGameDataChecksum + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, wChecksum + 1);
	gb->cpu.e = gb->cpu.a;
	wr8(gb, sBackupGameDataChecksum + 1, gb->cpu.a);
	save_echoes(gb, 0x22, sBackupGameDataChecksumEcho1, sBackupGameDataChecksumEcho2);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	save_marker(gb, 0);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);

	gb_call(gb, ADDR_VBlank_Saving);
	FARCALL(LoadSaveScreenPals);
	FARCALL(DrawSaveCompleteBox);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer + 0, 0);
	gb->cpu.a = (uint8_t)(512 >> 8);
	wr8(gb, wTimer + 1, gb->cpu.a);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
