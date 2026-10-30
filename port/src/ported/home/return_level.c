// Port of src/home/return_level.asm
#include "ported.h"

PORT(ReturnToPendingLevelState)
{
	SET_HL(gb, wState);
	wr8(gb, wState, ST_LEVEL);
	gb->cpu.a = rd8(gb, wPendingSubState);
	wr8(gb, wSubState, gb->cpu.a);
}

PORT(ReturnToMap)
{
	gb->cpu.a = TRANSITION_RETURN_TO_MAP;
	wr8(gb, wTransitionParam, TRANSITION_RETURN_TO_MAP);
	FARCALL(StartOverworldState);
}

PORT_JUMP(ReturnToMapFromLevel)
{
	SET_HL(gb, wDayNight);
	wr8(gb, wDayNight, (uint8_t)(rd8(gb, wDayNight) | 0x80));
	JUMP(ReturnToMap); // fallthrough
}

PORT(ReturnToPauseMenuFromActionHelp)
{
	// push_wram $03
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(_ReturnToPauseMenuFromActionHelp);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

PORT_JUMP(ReturnToLevelFromGolf)
{
	CALL(DisableLCD);
	// push_wram $03
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempAnimatedTilesFrameDuration);
	wr8(gb, wAnimatedTilesFrameDuration, gb->cpu.a);
	gb->cpu.a = rd8(gb, wTempAnimatedTilesGroup);
	wr8(gb, wAnimatedTilesGfx, gb->cpu.a);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	gb->cpu.a = TRUE;
	wr8(gb, wRoomAnimatedTilesEnabled, TRUE);
	CALL(LoadBackupVRAM);
	gb->cpu.a = alu_and(gb, rd8(gb, wRoomTransitionParam), ROOMTRANSITION_FLAGS_MASK);
	gb->cpu.a = alu_or(gb, gb->cpu.a, ROOMTRANSITION_DOOR);
	wr8(gb, wRoomTransitionParam, gb->cpu.a);
	FARCALL(DrawLevelObjectsAfterLevelReturn);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	JUMP(ReturnToPendingLevelState); // fallthrough
}
