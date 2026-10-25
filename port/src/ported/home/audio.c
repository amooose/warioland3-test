// Port of src/home/audio.asm
//
// Each entry point maps the audio WRAM bank in (push_wram), calls its
// AudioFunc_* trampoline and restores the WRAM bank (pop_wram).
#include "ported.h"

#define AUDIO_RAM_BANK BANK_wAudioEngineFlags

// push_wram BANK("Audio RAM")
static void push_wram(GB *gb)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = AUDIO_RAM_BANK;
	wr8(gb, rWBK, gb->cpu.a);
}

// pop_wram
static void pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

#define PORT_AUDIO_WRAPPER(name, func) \
	PORT(name) \
	{ \
		push_wram(gb); \
		CALL(func); \
		pop_wram(gb); \
	}

// Same, for the entry points that take an argument in a (through hffac).
#define PORT_AUDIO_WRAPPER_A(name, func) \
	PORT(name) \
	{ \
		wr8(gb, hffac, gb->cpu.a); \
		push_wram(gb); \
		gb->cpu.a = rd8(gb, hffac); \
		CALL(func); \
		pop_wram(gb); \
	}

PORT_AUDIO_WRAPPER(InitAudio, AudioFunc_InitAudio)
PORT_AUDIO_WRAPPER(UpdateAudio, AudioFunc_UpdateAudio)
PORT_AUDIO_WRAPPER(PlaySFX, AudioFunc_PlaySFX)
PORT_AUDIO_WRAPPER(Func_fd8, Func_3f12)
PORT_AUDIO_WRAPPER(PlayNewMusic, AudioFunc_PlayNewMusic)
PORT_AUDIO_WRAPPER(TurnOffSFXID, AudioFunc_TurnOffSFXID)
PORT_AUDIO_WRAPPER(TurnMusicOff, AudioFunc_TurnMusicOff)
PORT_AUDIO_WRAPPER(Func_1010, Func_3f2a)
PORT_AUDIO_WRAPPER(Func_101e, Func_3f30)
PORT_AUDIO_WRAPPER_A(ExecuteAudioMod, AudioFunc_ExecuteAudioMod)
PORT_AUDIO_WRAPPER_A(UnreferencedMusicFadeOut, AudioFunc_MusicFadeOut)
PORT_AUDIO_WRAPPER_A(UnreferencedGetPlayingSoundID, AudioFunc_GetPlayingSoundID)
PORT_AUDIO_WRAPPER(PlayNewMusic_SetNoise, AudioFunc_PlayNewMusic_SetNoise)
