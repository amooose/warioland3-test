// Port of src/engine/title/intro.asm
#include "ported.h"

PORT(StartTitleScreen)
{
	// play_music MUSIC_TITLE_SCREEN
	wr8(gb, hMusicID + 0, (uint8_t)(MUSIC_TITLE_SCREEN >> 8));
	wr8(gb, hMusicID + 1, (uint8_t)MUSIC_TITLE_SCREEN);
	gb->cpu.a = (uint8_t)(ACTION_HELP_TIMER >> 8);
	wr8(gb, wTimer + 1, gb->cpu.a);
	gb->cpu.a = (uint8_t)ACTION_HELP_TIMER;
	wr8(gb, wTimer, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, SST_START_MENU);
}
