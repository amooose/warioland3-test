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
