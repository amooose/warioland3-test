// Port of src/home/start.asm
#include "ported.h"

// unreferenced
PORT(UnreferencedMuteSound)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSoundMuted), rd8(gb, wSoundMuted));
	if (gb->cpu.a)
		return;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hSFXID + 0, 0);
	wr8(gb, hSFXID + 1, 0);
	CALL(TurnMusicOff);
	SET_BC(gb, SOUND_OFF);
	CALL(TurnOffSFXID);
	CALL(UpdateAudio);
	gb->cpu.a = TRUE;
	wr8(gb, wSoundMuted, TRUE);
}

// unreferenced
PORT(UnreferencedUnmuteSound)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSoundMuted), rd8(gb, wSoundMuted));
	if (!gb->cpu.a)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSoundMuted, 0);
	wr8(gb, hSFXID + 0, 0);
	wr8(gb, hSFXID + 1, 0);
	gb->cpu.b = rd8(gb, hMusicIDBackup + 0);
	gb->cpu.c = gb->cpu.a = rd8(gb, hMusicIDBackup + 1);
	CALL(PlayNewMusic_SetNoise);
}
