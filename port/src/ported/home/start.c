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

// handle playing SFX and music
PORT(HandleSound)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSoundMuted), rd8(gb, wSoundMuted));
	if (gb->cpu.a)
		return;
	gb->cpu.a = rd8(gb, hSFXID);
	SET_HL(gb, hSFXID + 1);
	alu_cp(gb, gb->cpu.a, 0xff);
	bool play = true;
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, SOUND_OFF);
	} else {
		// .valid_sfx
		gb->cpu.c = rd8(gb, hSFXID + 1);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
		play = gb->cpu.a != 0;
	}
	if (play) {
		// .got_sfx
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hSFXID + 1, 0);
		wr8(gb, hSFXID + 0, 0);
		SET_HL(gb, hSFXID);
		CALL(PlaySFX);
	}

	// .music
	gb->cpu.a = rd8(gb, hMusicID);
	SET_HL(gb, hMusicID + 1);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, SOUND_OFF);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hMusicID + 1, 0);
		wr8(gb, hMusicID + 0, 0);
		SET_HL(gb, hMusicID);
		CALL(PlayNewMusic);
	} else {
		// .valid_music
		gb->cpu.c = rd8(gb, hMusicID + 1);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.a) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hMusicID + 1, 0);
			wr8(gb, hMusicID + 0, 0);
			SET_HL(gb, hMusicID);
			CALL(PlayNewMusic_SetNoise);
		}
	}
	// .update
	CALL(UpdateAudio);
}
