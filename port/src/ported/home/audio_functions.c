// Port of src/home/audio_functions.asm
//
// The AudioFunc_* trampolines call PrepareAudioJump_*, which either returns
// to them (they then jump into the audio engine) or pops their return
// address and returns straight to their caller. Here they push the same
// return address the asm call does, run the PrepareAudioJump logic on the
// real stack, and then go wherever its ret goes.
#include "ported.h"

#define AUDIO_ENGINE_BANK BANK__InitAudio

// SwitchToAudioBank.switch_bank: ld [wROMBank], a / ld [rROMB0], a
static void switch_bank(GB *gb)
{
	wr8(gb, wROMBank, gb->cpu.a);
	wr8(gb, rROMB0, gb->cpu.a);
}

static void switch_to_audio_bank(GB *gb)
{
	gb->cpu.a = AUDIO_ENGINE_BANK;
	switch_bank(gb);
}

static void backup_bank_and_switch_to_audio_bank(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	wr8(gb, wAudioBankBackup, gb->cpu.a);
	switch_to_audio_bank(gb);
}

static void switch_back_from_audio_bank(GB *gb)
{
	gb->cpu.a = rd8(gb, wAudioBankBackup);
	switch_bank(gb);
}

// PrepareAudioJump_NoCallback.backup, up to its ret
static void prepare_backup(GB *gb)
{
	// hl = wAudioEngineFlags
	wr8(gb, wAudioEngineFlags, rd8(gb, wAudioEngineFlags) | AUDIOENG_HAS_BACKUP_BANK);
	push_af(gb);
	backup_bank_and_switch_to_audio_bank(gb);
	pop_af(gb);
}

// PrepareAudioJump_NoCallback.pop_stack, up to its ret
static void prepare_pop_stack(GB *gb)
{
	SET_HL(gb, gb_pop16(gb));
	gb->cpu.a = 0xff;
}

// PrepareAudioJump_NoCallback, up to its ret (not performed)
static void prepare_no_callback(GB *gb)
{
	SET_HL(gb, wAudioEngineFlags);
	if (alu_bit(gb, rd8(gb, wAudioEngineFlags), AUDIOENG_HAS_BACKUP_BANK_F))
		prepare_pop_stack(gb);
	else
		prepare_backup(gb);
}

// PrepareAudioJump_WithCallback, up to its ret (not performed)
static void prepare_with_callback(GB *gb)
{
	SET_HL(gb, wAudioEngineFlags);
	if (!alu_bit(gb, rd8(gb, wAudioEngineFlags), AUDIOENG_HAS_BACKUP_BANK_F)) {
		prepare_backup(gb);
		return;
	}
	if (alu_bit(gb, rd8(gb, wAudioEngineFlags), AUDIOENG_HAS_CALLBACK_F)) {
		prepare_pop_stack(gb);
		return;
	}
	wr8(gb, wAudioEngineFlags, rd8(gb, wAudioEngineFlags) | AUDIOENG_HAS_CALLBACK);
	// pop the calling function from the stack and store it in WRAM
	SET_HL(gb, gb_pop16(gb));
	gb->cpu.a = gb->cpu.h;
	wr8(gb, wAudioBankCallback + 1, gb->cpu.a);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, wAudioBankCallback + 0, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// call PrepareAudioJump_* (at the start of an AudioFunc_* routine at `self`)
// / jp target
static void audio_func(GB *gb, uint16_t self, uint16_t target, bool with_callback)
{
	uint16_t ret = (uint16_t)(self + 3);
	gb_push16(gb, ret);
	if (with_callback)
		prepare_with_callback(gb);
	else
		prepare_no_callback(gb);
	RET();
	if (gb->cpu.pc == ret)
		JUMP_TO(target); // jp target (whatever bank is mapped, as in the asm)
}

PORT_JUMP(AudioFunc_InitAudio)
{
	backup_bank_and_switch_to_audio_bank(gb);
	JUMP_TO(ADDR__InitAudio);
}

PORT_JUMP(AudioFunc_UpdateAudio)
{
	audio_func(gb, ADDR_AudioFunc_UpdateAudio, ADDR__UpdateAudio, true);
}

PORT_JUMP(AudioFunc_PlaySFX)
{
	audio_func(gb, ADDR_AudioFunc_PlaySFX, ADDR__PlaySFX, false);
}

PORT_JUMP(Func_3f12)
{
	audio_func(gb, ADDR_Func_3f12, ADDR_PlayMusic, false);
}

PORT_JUMP(AudioFunc_PlayNewMusic)
{
	audio_func(gb, ADDR_AudioFunc_PlayNewMusic, ADDR__PlayNewMusic, false);
}

PORT_JUMP(AudioFunc_TurnOffSFXID)
{
	audio_func(gb, ADDR_AudioFunc_TurnOffSFXID, ADDR__TurnOffSFXID, false);
}

PORT_JUMP(AudioFunc_TurnMusicOff)
{
	audio_func(gb, ADDR_AudioFunc_TurnMusicOff, ADDR__TurnMusicOff, false);
}

PORT_JUMP(Func_3f2a)
{
	audio_func(gb, ADDR_Func_3f2a, ADDR_Func_302e4, false);
}

PORT_JUMP(Func_3f30)
{
	audio_func(gb, ADDR_Func_3f30, ADDR_Func_30438, false);
}

PORT_JUMP(AudioFunc_ExecuteAudioMod)
{
	audio_func(gb, ADDR_AudioFunc_ExecuteAudioMod, ADDR__ExecuteAudioMod, false);
}

PORT_JUMP(AudioFunc_MusicFadeOut)
{
	audio_func(gb, ADDR_AudioFunc_MusicFadeOut, ADDR__SetMusicFadeOut, false);
}

PORT_JUMP(AudioFunc_GetPlayingSoundID)
{
	audio_func(gb, ADDR_AudioFunc_GetPlayingSoundID, ADDR__GetPlayingSoundID, false);
}

PORT_JUMP(AudioFunc_PlayNewMusic_SetNoise)
{
	audio_func(gb, ADDR_AudioFunc_PlayNewMusic_SetNoise, ADDR__PlayNewMusic_SetNoise, false);
}

PORT(BackupBankAndSwitchToAudioBank)
{
	backup_bank_and_switch_to_audio_bank(gb);
}

PORT(SwitchToAudioBank)
{
	switch_to_audio_bank(gb);
}

PORT(SwitchBackFromAudioBank)
{
	switch_back_from_audio_bank(gb);
}

PORT_JUMP(PrepareAudioJump_NoCallback)
{
	prepare_no_callback(gb);
	RET();
}

PORT_JUMP(PrepareAudioJump_WithCallback)
{
	prepare_with_callback(gb);
	RET();
}

PORT_JUMP(ReturnFromAudioJump)
{
	SET_HL(gb, wAudioEngineFlags);
	if (!alu_bit(gb, rd8(gb, wAudioEngineFlags), AUDIOENG_HAS_CALLBACK_F)) {
		switch_back_from_audio_bank(gb);
		wr8(gb, wAudioEngineFlags, rd8(gb, wAudioEngineFlags) & ~AUDIOENG_HAS_BACKUP_BANK);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		RET();
		return;
	}
	// .push_callback: push the callback to the stack and return to it
	gb->cpu.a = rd8(gb, wAudioBankCallback + 1);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = rd8(gb, wAudioBankCallback + 0);
	gb->cpu.l = gb->cpu.a;
	gb_push16(gb, REG_HL(gb));
	SET_HL(gb, wAudioEngineFlags);
	wr8(gb, wAudioEngineFlags, rd8(gb, wAudioEngineFlags) & ~AUDIOENG_HAS_CALLBACK);
	RET();
}

PORT(ReadAudioCommands_1Byte)
{
	gb->cpu.a = rd8(gb, wSoundBank);
	switch_bank(gb);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.c = gb->cpu.a;
	switch_to_audio_bank(gb);
}

PORT(ReadAudioCommands_2Bytes)
{
	gb->cpu.a = rd8(gb, wSoundBank);
	switch_bank(gb);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.c = gb->cpu.a;
	SET_DE(gb, REG_DE(gb) + 1);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.b = gb->cpu.a;
	switch_to_audio_bank(gb);
}
