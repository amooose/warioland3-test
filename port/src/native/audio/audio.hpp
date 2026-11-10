// The audio entry points (src/home/audio.asm), for C++ callers.
//
// Each maps the Audio RAM bank and runs the engine as the original's
// trampolines do (a call while the engine is busy does nothing, and
// UpdateAudio is then deferred). The hooks in audio.cpp take the original's
// register arguments; these take plain ones.
#pragma once

#include "../native.hpp"

namespace wl3::audio {

void play_sfx(GB *gb, uint16_t id);                 // PlaySFX
void play_new_music(GB *gb, uint16_t id);           // PlayNewMusic
void play_new_music_set_noise(GB *gb, uint16_t id); // PlayNewMusic_SetNoise
void turn_off_sfx(GB *gb, uint16_t id);             // TurnOffSFXID
void turn_music_off(GB *gb);                        // TurnMusicOff

// UpdateAudio. The original passes its caller's leftover c to
// wSoundEngineParam1 when a music fade-out step lowers the volume. Every
// reader of that byte (and of wCurSoundID, wTempTrackPriority at the same
// address) writes it first, so this passes 0.
void update(GB *gb);

// ExecuteAudioMod, for the mods with one 8-bit value (tempo, volume, pan,
// vibrato amplitude): wSoundEngineParam1 gets 0 in place of the caller's
// leftover c, as in update.
void execute_audio_mod(GB *gb, uint8_t mod, uint8_t value, uint8_t sfx_channels, uint8_t music_channels);

} // namespace wl3::audio
