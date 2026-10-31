// The sound engine (src/audio/engine.asm and src/home/audio*.asm), as C++.
//
// The engine plays up to 4 sound effects and 4 music parts at once. Each one
// is a Channel: a stream of audio commands (notes, tempo, envelopes,
// vibrato, calls and loops) that the engine steps through at the SFX or
// music tempo. The 4 hardware sound generators are Tracks: a channel that
// plays a note takes over the track for its note's sound register, if its
// priority allows, and the tracks then drive the sound hardware.
//
// Its state stays in the Audio RAM WRAM bank, byte for byte where the
// original keeps it, including the cursors the original iterates with
// (wCurChannelPtr, wNumAudioChannels, wCurTrackPtr...). See native.hpp.
#pragma once

#include "../native.hpp"
#include "../ram.hpp"

namespace wl3::audio {

// A channel_struct in the Audio RAM, by its CHANNEL_* fields.
class Channel {
public:
	Channel(GB *gb, uint16_t base) : gb_(gb), base_(base) {}

	uint16_t base() const { return base_; }
	Byte operator[](int field) const { return Byte(gb_, static_cast<uint16_t>(base_ + field)); }
	Word word(int field) const { return Word(gb_, static_cast<uint16_t>(base_ + field)); }
	Byte flags() const { return (*this)[CHANNEL_FLAGS]; }
	bool active() const { return flags().bit(CHANFLAGS_ACTIVE_F); }

private:
	GB *gb_;
	uint16_t base_;
};

// A track_struct in the Audio RAM, by its TRACK_* fields.
class Track {
public:
	Track(GB *gb, uint16_t base) : gb_(gb), base_(base) {}

	uint16_t base() const { return base_; }
	Byte operator[](int field) const { return Byte(gb_, static_cast<uint16_t>(base_ + field)); }
	Word word(int field) const { return Word(gb_, static_cast<uint16_t>(base_ + field)); }
	Byte flags() const { return (*this)[TRACK_FLAGS]; }

private:
	GB *gb_;
	uint16_t base_;
};

class Engine {
public:
	explicit Engine(GB *gb) : gb_(gb), mem_(gb), wram_(gb), hram_(gb) {}

	// The entry points. Each runs with the Audio RAM bank and the audio
	// engine's ROM bank mapped, like the original's.
	void init();
	// caller_c: the caller's c register, which the original passes on to
	// wSoundEngineParam1 when a music fade-out step lowers the volume.
	void update(uint8_t caller_c);
	void play_sfx(uint16_t id);
	void play_music(uint16_t id);
	void play_new_music(uint16_t id);
	void play_new_music_set_noise(uint16_t id);
	void resume_music(); // Func_302e4
	void turn_off_sfx(uint16_t id);
	void turn_sfx_off();
	void turn_music_off();
	void execute_audio_mod(uint8_t mod, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
	void set_music_fade_out(uint8_t period);
	uint16_t playing_sound_id(uint8_t which);

private:
	using CommandFn = bool (Engine::*)(uint16_t &);

	Byte ram(uint16_t addr) const { return mem_[addr]; }
	Word ram_word(uint16_t addr) const { return mem_.word(addr); }

	// The channel and track being worked on (wCurChannelPtr, wCurTrackPtr).
	Channel cur_channel() const { return Channel(gb_, wram_.cur_channel_ptr().word()); }
	Track cur_track() const { return Track(gb_, wram_.cur_track_ptr().word()); }

	// Stepping through channels and tracks, keeping the original's cursors.
	void set_channels(uint16_t first, uint8_t count);
	void set_sfx_channels() { set_channels(wChannels, NUM_SFX_CHANNELS); }
	void set_music_channels() { set_channels(wMusicChannels, NUM_MUSIC_CHANNELS); }
	bool next_channel();
	void set_tracks(uint16_t first, uint8_t count, uint8_t sound_register);
	void set_all_tracks() { set_tracks(wTrack1, NUM_TRACKS, static_cast<uint8_t>(rAUD1ENV)); }
	bool next_track();

	// Reading a sound's commands from its ROM bank (ReadAudioCommands_*).
	uint8_t read_command_byte(uint16_t addr);
	uint16_t read_command_word(uint16_t addr);
	void map_rom_bank(uint8_t bank);

	// Starting and stopping sounds
	bool load_sound(uint16_t id); // GetSound
	bool load_channel_with_sound(); // true when there are no more to load
	bool skip_loading_channel();
	void init_channel(Channel ch);
	void deactivate_music_channels();

	// Audio mods
	void execute_audio_mod_internal(uint8_t mod, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
	uint8_t channel_bitmask() const;
	void audio_mod_set_short(int field);
	void audio_mod_set_long(int field);
	void update_music_fade_out(uint8_t caller_c);
	void calculate_tempo(uint16_t base_tempo);

	// Updating channels
	void update_channel_without_commands(); // Func_30547
	void update_channel();
	void update_vibrato(Channel ch);
	void apply_channel_updates(Channel ch, uint8_t flags); // Func_30651
	void do_audio_commands(uint16_t cmd);
	bool run_command(uint16_t table, uint8_t index, uint16_t &cmd);
	bool command_by_address(uint16_t handler, uint16_t &cmd);

	// Audio commands: each takes the command pointer, and returns true where
	// the original jumps back to DoAudioCommand for the next command.
	bool cmd_clear_flags(uint16_t &cmd);
	bool cmd_extended(uint16_t &cmd); // Func_30727
	bool cmd_rest(uint16_t &cmd, uint8_t command); // Func_3073f
	bool cmd_jump(uint16_t &cmd);
	bool cmd_call(uint16_t &cmd);
	bool cmd_ret(uint16_t &cmd);
	bool cmd_loop(uint16_t &cmd);
	bool cmd_set_tempo(uint16_t &cmd);
	bool cmd_set_property(uint16_t &cmd, int field);
	bool cmd_set_wave(uint16_t &cmd);
	bool cmd_set_pitch_offset(uint16_t &cmd);
	bool cmd_set_pitch_offset_multiplier(uint16_t &cmd);
	bool set_pitch_product(Channel ch, uint8_t offset, uint8_t mult);
	bool cmd_set_vibrato_speed(uint16_t &cmd);
	bool cmd_set_volume(uint16_t &cmd);
	bool cmd_set_signed(uint16_t &cmd, int field); // Func_3098b
	bool cmd_set_high_nybble(uint16_t &cmd, int field); // Func_309ce
	bool cmd_set_low_nybble(uint16_t &cmd, int field); // Func_309eb
	bool cmd_note(uint16_t &cmd, uint8_t duration); // Func_30a1f
	bool cmd_stop_note(uint16_t &cmd); // Func_30b4d

	// Tracks and the sound hardware
	void update_sfx_track() { update_track_duration(true); } // Func_30bb0
	void update_music_track() { update_track_duration(false); } // Func_30bc2
	void update_track_duration(bool sfx);
	void end_track_note(Track tr); // Func_30be3
	void release_track(Track tr); // Func_30bf6
	void update_track_output(); // Func_30bfb
	void set_track_volume(Track tr); // Func_30dc6
	uint8_t set_track_fade_out_volume(Track tr); // Func_30deb
	uint8_t check_sound3_envelope(uint8_t envelope, uint8_t old); // Func_30e1b
	void set_timbre();
	void set_frequency();
	void set_sound_output();
	void set_volume_envelope();
	void turn_sound_register_off();

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Hram hram_;
};

} // namespace wl3::audio
