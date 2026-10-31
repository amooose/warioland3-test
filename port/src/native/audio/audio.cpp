// The audio entry points (src/home/audio.asm), and the way the original
// calls into the engine around them (src/home/audio_functions.asm).
#include "engine.hpp"

using wl3::Byte;
using wl3::Memory;
using wl3::audio::Engine;

namespace {

constexpr uint8_t AUDIO_RAM_BANK = BANK_wAudioEngineFlags;  // BANK("Audio RAM")
constexpr uint8_t AUDIO_ENGINE_BANK = BANK__InitAudio;      // BANK("Audio Engine")
// What PrepareAudioJump_WithCallback stores to resume a deferred update: the
// return address of its call in AudioFunc_UpdateAudio (then jp _UpdateAudio).
constexpr uint16_t UPDATE_AUDIO_CALLBACK = ADDR_AudioFunc_UpdateAudio + 3;

// One call into the engine, as the original makes it: the entry point maps
// the Audio RAM bank (push_wram / pop_wram), and its AudioFunc_* trampoline
// maps the engine's ROM bank, remembering the caller's (PrepareAudioJump_*,
// ReturnFromAudioJump).
//
// A call made while the engine is running (from an interrupt, in the
// original) does nothing, except UpdateAudio, which is deferred until the
// running call returns. Native code never interrupts the engine, but a save
// state or the fuzzer can start with the engine's flags saying it's busy.
class AudioCall {
public:
	// trampoline: the AudioFunc_* label the original goes through
	AudioCall(GB *gb, uint16_t trampoline) : gb_(gb), mem_(gb), wram_(gb), hram_(gb), trampoline_(trampoline)
	{
		saved_wram_bank_ = mem_[rWBK];
		mem_[rWBK] = AUDIO_RAM_BANK;
	}

	// PrepareAudioJump_NoCallback: whether the engine is free to run
	bool prepare()
	{
		Byte flags = wram_.audio_engine_flags();
		if (flags.bit(AUDIOENG_HAS_BACKUP_BANK_F)) {
			busy();
			return false;
		}
		flags.set_bit(AUDIOENG_HAS_BACKUP_BANK_F);
		backup_bank();
		return true;
	}

	// PrepareAudioJump_WithCallback
	bool prepare_with_callback()
	{
		Byte flags = wram_.audio_engine_flags();
		if (!flags.bit(AUDIOENG_HAS_BACKUP_BANK_F)) {
			flags.set_bit(AUDIOENG_HAS_BACKUP_BANK_F);
			backup_bank();
			return true;
		}
		if (!flags.bit(AUDIOENG_HAS_CALLBACK_F)) {
			flags.set_bit(AUDIOENG_HAS_CALLBACK_F);
			wram_.audio_bank_callback().word() = return_address();
		}
		busy();
		return false;
	}

	// BackupBankAndSwitchToAudioBank
	void backup_bank()
	{
		wram_.audio_bank_backup() = wram_.rom_bank();
		map_rom_bank(AUDIO_ENGINE_BANK);
	}

	// SwitchBackFromAudioBank
	void switch_back() { map_rom_bank(wram_.audio_bank_backup()); }

	// ReturnFromAudioJump: runs a deferred update, then maps the caller's
	// bank back.
	void finish(Engine &engine)
	{
		Byte flags = wram_.audio_engine_flags();
		while (flags.bit(AUDIOENG_HAS_CALLBACK_F)) {
			uint16_t callback = wram_.audio_bank_callback().word();
			flags.reset_bit(AUDIOENG_HAS_CALLBACK_F);
			if (callback != UPDATE_AUDIO_CALLBACK) {
				// the original returns to whatever the garbage address holds
				if (gb_->test.active)
					gb_->test.unrealistic = true;
				break;
			}
			engine.update(gb_->cpu.c);
		}
		switch_back();
		flags.reset_bit(AUDIOENG_HAS_BACKUP_BANK_F);
		SET_HL(gb_, wAudioEngineFlags);
	}

	// pop_wram, which also restores a (to the bank) and f
	void leave()
	{
		mem_[rWBK] = saved_wram_bank_;
		gb_->cpu.a = saved_wram_bank_;
	}

private:
	void map_rom_bank(uint8_t bank)
	{
		wram_.rom_bank() = bank;
		mem_[rROMB0] = bank;
	}

	// where the trampoline's call returns: PrepareAudioJump_* pops it into
	// hl when the engine is busy
	uint16_t return_address() const { return static_cast<uint16_t>(trampoline_ + 3); }
	void busy() { SET_HL(gb_, return_address()); }

	GB *gb_;
	Memory mem_;
	wl3::Wram wram_;
	wl3::Hram hram_;
	uint16_t trampoline_;
	uint8_t saved_wram_bank_;
};

// An entry point through PrepareAudioJump_NoCallback; returns whether the
// engine ran.
template <class Body> bool engine_call(GB *gb, uint16_t trampoline, Body body)
{
	AudioCall call(gb, trampoline);
	bool ran = call.prepare();
	if (ran) {
		Engine engine(gb);
		body(engine);
		call.finish(engine);
	}
	call.leave();
	return ran;
}

uint16_t sound_id(GB *gb)
{
	return REG_BC(gb);
}

} // namespace

// The registers each entry point leaves are the original's, except where its
// clobber mask says otherwise: bc and de, which the engine leaves with
// whatever its last loop used. No caller uses them after the call.

PORT(InitAudio)
{
	AudioCall call(gb, ADDR_AudioFunc_InitAudio);
	call.backup_bank();
	Engine(gb).init();
	call.switch_back();
	gb->cpu.b = 0;
	SET_DE(gb, TRACK_STRUCT_LENGTH);
	SET_HL(gb, wTracks + NUM_TRACKS * TRACK_STRUCT_LENGTH);
	call.leave();
}

PORT(UpdateAudio)
{
	AudioCall call(gb, ADDR_AudioFunc_UpdateAudio);
	if (call.prepare_with_callback()) {
		Engine engine(gb);
		engine.update(gb->cpu.c);
		call.finish(engine);
	}
	call.leave();
}

PORT_EX(PlaySFX, 0, R_B | R_C | R_D | R_E)
{
	uint16_t id = sound_id(gb);
	engine_call(gb, ADDR_AudioFunc_PlaySFX, [&](Engine &e) { e.play_sfx(id); });
}

// unreferenced
PORT_EX(Func_fd8, 0, R_B | R_C | R_D | R_E)
{
	uint16_t id = sound_id(gb);
	engine_call(gb, ADDR_Func_3f12, [&](Engine &e) { e.play_music(id); });
}

PORT_EX(PlayNewMusic, 0, R_B | R_C | R_D | R_E)
{
	uint16_t id = sound_id(gb);
	engine_call(gb, ADDR_AudioFunc_PlayNewMusic, [&](Engine &e) { e.play_new_music(id); });
}

PORT_EX(PlayNewMusic_SetNoise, 0, R_B | R_C | R_D | R_E)
{
	uint16_t id = sound_id(gb);
	engine_call(gb, ADDR_AudioFunc_PlayNewMusic_SetNoise, [&](Engine &e) { e.play_new_music_set_noise(id); });
}

PORT(TurnOffSFXID)
{
	uint16_t id = sound_id(gb);
	if (engine_call(gb, ADDR_AudioFunc_TurnOffSFXID, [&](Engine &e) { e.turn_off_sfx(id); })) {
		SET_DE(gb, wChannels);
		if (id) // the last SFX channel's pointer
			SET_BC(gb, wChannels + (NUM_SFX_CHANNELS - 1) * CHANNEL_STRUCT_LENGTH);
	}
}

PORT(TurnMusicOff)
{
	if (engine_call(gb, ADDR_AudioFunc_TurnMusicOff, [](Engine &e) { e.turn_music_off(); }))
		SET_DE(gb, wMusicChannels);
}

// unreferenced
PORT(Func_1010)
{
	if (engine_call(gb, ADDR_Func_3f2a, [](Engine &e) { e.resume_music(); }))
		SET_DE(gb, wMusicChannels);
}

// unreferenced: d = the active SFX channels, e = the active music channels
PORT(Func_101e)
{
	uint8_t active = 0;
	// (read in the call, with the Audio RAM bank mapped)
	if (engine_call(gb, ADDR_Func_3f30, [&](Engine &) { active = Memory(gb).byte(wActiveChannels); })) {
		gb->cpu.d = active & SFX_CHANNELS;
		gb->cpu.e = static_cast<uint8_t>((active & MUSIC_CHANNELS) >> 4);
	}
}

// The entry points that take an argument in a pass it through hffac.

// a = AUDIOMOD_* function, b = value, d / e = the SFX / music channels.
// It leaves bc and de as the original's mod function does (action help
// code after it reads them): the channel field it set in bc, the channel
// mask in d (rotated full circle) and e = 0; for the tempo, bc as
// CalculateTempo leaves it.
PORT(ExecuteAudioMod)
{
	uint8_t mod = gb->cpu.a, b = gb->cpu.b, c = gb->cpu.c, d = gb->cpu.d, e = gb->cpu.e;
	Memory mem(gb);
	mem[hffac] = mod;
	if (!engine_call(gb, ADDR_AudioFunc_ExecuteAudioMod, [&](Engine &en) { en.execute_audio_mod(mod, b, c, d, e); }) ||
		mod >= NUM_AUDIOMOD_FUNCTIONS)
		return;
	if (mod == AUDIOMOD_TEMPO) {
		if (d || e)
			SET_BC(gb, b); // b = 0, c = the tempo multiplier
		else // JumpToPointerInTable's pointer into the table
			SET_BC(gb, ADDR_AudioModFunctionTable + mod * 2 + 1);
		return;
	}
	static const uint8_t fields[NUM_AUDIOMOD_FUNCTIONS] = {
		0, CHANNEL_PITCH_OFFSET_MOD, CHANNEL_VOLUME_MOD, CHANNEL_GLOBAL_PAN,
		CHANNEL_VIBRATO_AMPLITUDE_MOD, CHANNEL_VIBRATO_SPEED, CHANNEL_UNK_27,
	};
	SET_BC(gb, fields[mod]);
	gb->cpu.d = static_cast<uint8_t>((d & 0x0f) | ((e << 4) & 0xf0));
	gb->cpu.e = 0;
}

// unreferenced: a = fade-out period
PORT(UnreferencedMusicFadeOut)
{
	uint8_t period = gb->cpu.a;
	Memory mem(gb);
	mem[hffac] = period;
	engine_call(gb, ADDR_AudioFunc_MusicFadeOut, [&](Engine &e) { e.set_music_fade_out(period); });
}

// unreferenced: a = 0 for the music or an SFX channel (1-4); returns the
// sound ID it's playing in bc (0 if none)
PORT(UnreferencedGetPlayingSoundID)
{
	uint8_t which = gb->cpu.a;
	Memory mem(gb);
	mem[hffac] = which;
	uint16_t id = 0;
	if (engine_call(gb, ADDR_AudioFunc_GetPlayingSoundID, [&](Engine &e) { id = e.playing_sound_id(which); }))
		SET_BC(gb, id);
}

// The engine's routines and the trampolines, implemented inside this module
// (for the progress report; they have no hooks of their own).
PORT_COVERED(AddChannelStructLengthToHL);
PORT_COVERED(AudioCmd_Call);
PORT_COVERED(AudioCmd_ClearFlags);
PORT_COVERED(AudioCmd_Jump);
PORT_COVERED(AudioCmd_Loop);
PORT_COVERED(AudioCmd_Pan);
PORT_COVERED(AudioCmd_Ret);
PORT_COVERED(AudioCmd_SetChannelDuration);
PORT_COVERED(AudioCmd_SetChannelFadeInEnvelope);
PORT_COVERED(AudioCmd_SetChannelFadeOutEnvelope);
PORT_COVERED(AudioCmd_SetChannelSweep);
PORT_COVERED(AudioCmd_SetChannelTimbre);
PORT_COVERED(AudioCmd_SetChannelUnk1d);
PORT_COVERED(AudioCmd_SetChannelUnk1e);
PORT_COVERED(AudioCmd_SetChannelUnk27);
PORT_COVERED(AudioCmd_SetChannelUnk28);
PORT_COVERED(AudioCmd_SetChannelVibratoDisabled);
PORT_COVERED(AudioCmd_SetChannelVolume);
PORT_COVERED(AudioCmd_SetPitchOffset);
PORT_COVERED(AudioCmd_SetPitchOffsetMultiplier);
PORT_COVERED(AudioCmd_SetSemitoneOffset);
PORT_COVERED(AudioCmd_SetTempo);
PORT_COVERED(AudioCmd_SetVibratoAmplitude);
PORT_COVERED(AudioCmd_SetVibratoDelay);
PORT_COVERED(AudioCmd_SetVibratoSpeed);
PORT_COVERED(AudioCmd_SetWave);
PORT_COVERED(AudioFunc_ExecuteAudioMod);
PORT_COVERED(AudioFunc_GetPlayingSoundID);
PORT_COVERED(AudioFunc_InitAudio);
PORT_COVERED(AudioFunc_MusicFadeOut);
PORT_COVERED(AudioFunc_PlayNewMusic);
PORT_COVERED(AudioFunc_PlayNewMusic_SetNoise);
PORT_COVERED(AudioFunc_PlaySFX);
PORT_COVERED(AudioFunc_TurnMusicOff);
PORT_COVERED(AudioFunc_TurnOffSFXID);
PORT_COVERED(AudioFunc_UpdateAudio);
PORT_COVERED(AudioModSetValue_Long);
PORT_COVERED(AudioModSetValue_Short);
PORT_COVERED(AudioMod_Pan);
PORT_COVERED(AudioMod_Pitch);
PORT_COVERED(AudioMod_Tempo);
PORT_COVERED(AudioMod_VibratoAmplitude);
PORT_COVERED(AudioMod_VibratoSpeed);
PORT_COVERED(AudioMod_Volume);
PORT_COVERED(BackupBankAndSwitchToAudioBank);
PORT_COVERED(CalculatePitchProduct);
PORT_COVERED(CalculateTempo);
PORT_COVERED(DeactivateMusicChannels);
PORT_COVERED(DeactivateMusicChannels_FromFadeOut);
PORT_COVERED(DoAudioCommand);
PORT_COVERED(ExecuteAudioMod_Internal);
PORT_COVERED(Func_302e4);
PORT_COVERED(Func_30438);
PORT_COVERED(Func_30547);
PORT_COVERED(Func_30651);
PORT_COVERED(Func_30727);
PORT_COVERED(Func_3073f);
PORT_COVERED(Func_3098b);
PORT_COVERED(Func_309b6);
PORT_COVERED(Func_309ce);
PORT_COVERED(Func_309e8);
PORT_COVERED(Func_309eb);
PORT_COVERED(Func_30a03);
PORT_COVERED(Func_30a10);
PORT_COVERED(Func_30a14);
PORT_COVERED(Func_30a1f);
PORT_COVERED(Func_30b4d);
PORT_COVERED(Func_30bb0);
PORT_COVERED(Func_30bc2);
PORT_COVERED(Func_30bd2);
PORT_COVERED(Func_30be3);
PORT_COVERED(Func_30bf2);
PORT_COVERED(Func_30bf6);
PORT_COVERED(Func_30bfb);
PORT_COVERED(Func_3f12);
PORT_COVERED(Func_3f2a);
PORT_COVERED(Func_3f30);
PORT_COVERED(GetNoteFrequency);
PORT_COVERED(GetSound);
PORT_COVERED(GetWave);
PORT_COVERED(InitChannel);
PORT_COVERED(InvalidAudioCmd);
PORT_COVERED(JumpToPointerInTable);
PORT_COVERED(LoadChannelWithSound);
PORT_COVERED(MultiplyBByC);
PORT_COVERED(MultiplyBByC_Short);
PORT_COVERED(NextChannel);
PORT_COVERED(NextTrack);
PORT_COVERED(PlayMusic);
PORT_COVERED(PrepareAudioJump_NoCallback);
PORT_COVERED(PrepareAudioJump_WithCallback);
PORT_COVERED(PrepareChannelBitmasks);
PORT_COVERED(ReadAudioCommands_1Byte);
PORT_COVERED(ReadAudioCommands_2Bytes);
PORT_COVERED(ReturnFromAudioJump);
PORT_COVERED(SetAllTracks);
PORT_COVERED(SetAudioChannels);
PORT_COVERED(SetChannelProperty);
PORT_COVERED(SetFrequency);
PORT_COVERED(SetMusicChannels);
PORT_COVERED(SetSFXChannels);
PORT_COVERED(SetSoundOutput);
PORT_COVERED(SetTimbre);
PORT_COVERED(SetTrack);
PORT_COVERED(SetTrack1);
PORT_COVERED(SetTrack2);
PORT_COVERED(SetTrack3);
PORT_COVERED(SetTrack4);
PORT_COVERED(SetVolumeEnvelope);
PORT_COVERED(SkipLoadingChannel);
PORT_COVERED(SwitchBackFromAudioBank);
PORT_COVERED(SwitchToAudioBank);
PORT_COVERED(TurnSFXOff);
PORT_COVERED(TurnSoundRegisterOff);
PORT_COVERED(UpdateChannel);
PORT_COVERED(UpdateMusicFadeOut);
PORT_COVERED(UpdateVibrato);
PORT_COVERED(_ExecuteAudioMod);
PORT_COVERED(_GetPlayingSoundID);
PORT_COVERED(_InitAudio);
PORT_COVERED(_PlayNewMusic);
PORT_COVERED(_PlayNewMusic_SetNoise);
PORT_COVERED(_PlaySFX);
PORT_COVERED(_SetMusicFadeOut);
PORT_COVERED(_TurnMusicOff);
PORT_COVERED(_TurnOffSFXID);
PORT_COVERED(_UpdateAudio);
