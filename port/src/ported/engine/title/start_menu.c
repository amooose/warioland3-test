// Port of src/engine/title/start_menu.asm
#include "ported.h"

PORT(StartOverworldState)
{
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

PORT(StartOverworldStateAndNewGameParam)
{
	gb->cpu.a = TRANSITION_NEW_GAME;
	wr8(gb, wTransitionParam, TRANSITION_NEW_GAME);
	// fallthrough into StartOverworldState
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

PORT(Func_4628)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameDuration, 0);
	wr8(gb, wAnimatedTilesGfx, 0);
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	wr8(gb, w3d50d, 0);
	SET_HL(gb, wDayNight);
	wr8(gb, wDayNight, rd8(gb, wDayNight) | 0x80); // set 7, [hl]
	// jr StartOverworldState
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}
