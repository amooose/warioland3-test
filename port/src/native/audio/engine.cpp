// The sound engine; see engine.hpp. Each function names the asm routine it
// replaces where the name says more than the routine's own label.
#include "engine.hpp"

namespace wl3::audio {

namespace {

// Q.6 fixed point values used by the engine
constexpr uint8_t Q6_1_0 = 0x40;     // 1.0
constexpr uint8_t Q6_0_063 = 0x04;   // 0.063
constexpr uint8_t Q6_256_0_HI = 0x40; // HIGH(256.0)

constexpr uint8_t AUDIO_ENGINE_BANK = BANK__InitAudio; // BANK("Audio Engine")
constexpr uint8_t CHANNEL_PLAYING = CHANFLAGS_6 | CHANFLAGS_ACTIVE;
constexpr uint8_t CHANNEL_UPDATES = CHANFLAGS_UPDATE_SO | CHANFLAGS_UPDATE_VOLUME | CHANFLAGS_UPDATE_FREQUENCY;
constexpr uint8_t SOUND3 = static_cast<uint8_t>(rAUD3LEVEL); // wCurSoundRegister of each track
constexpr uint8_t SOUND2 = static_cast<uint8_t>(rAUD2ENV);
constexpr uint8_t SOUND4 = static_cast<uint8_t>(rAUD4ENV);

uint8_t swap(uint8_t v) { return static_cast<uint8_t>(v << 4 | v >> 4); }
uint8_t rlca(uint8_t v) { return static_cast<uint8_t>(v << 1 | v >> 7); }
uint8_t rrca(uint8_t v) { return static_cast<uint8_t>(v >> 1 | v << 7); }
uint16_t io(uint8_t reg) { return static_cast<uint16_t>(0xff00 | reg); }

// MultiplyBByC_Short: the high nybble of b times the high nybble of c, in
// the engine's 4.4 fixed point (8-bit, wrapping as the asm does).
uint8_t multiply_short(uint8_t b, uint8_t c)
{
	uint8_t mult = static_cast<uint8_t>(c >> 4);
	uint8_t a = b & 0xf0;
	for (int i = 0; i < 4; i++) {
		bool carry = a & 0x80;
		a = static_cast<uint8_t>(a << 1);
		if (carry)
			a = static_cast<uint8_t>(a + mult);
	}
	return a;
}

// MultiplyBByC: b * c as 8.8 fixed point (16-bit, wrapping as the asm does).
uint16_t multiply(uint8_t b, uint8_t c)
{
	uint16_t product = static_cast<uint16_t>(b << 8);
	for (int i = 0; i < 8; i++) {
		bool carry = product & 0x8000;
		product = static_cast<uint16_t>(product << 1);
		if (carry)
			product = static_cast<uint16_t>(product + c);
	}
	return product;
}

} // namespace

// ---------------------------------------------------------------------------
// Channels, tracks and the sound data

void Engine::set_channels(uint16_t first, uint8_t count)
{
	wram_.num_audio_channels() = count;
	wram_.cur_channel_ptr().word() = first;
}

bool Engine::next_channel()
{
	uint8_t left = static_cast<uint8_t>(wram_.num_audio_channels() - 1);
	wram_.num_audio_channels() = left;
	if (!left)
		return false;
	wram_.cur_channel_ptr().word() = static_cast<uint16_t>(wram_.cur_channel_ptr().word() + CHANNEL_STRUCT_LENGTH);
	return true;
}

void Engine::set_tracks(uint16_t first, uint8_t count, uint8_t sound_register)
{
	wram_.num_tracks() = count;
	wram_.cur_track_ptr().word() = first;
	wram_.cur_sound_register() = sound_register;
}

bool Engine::next_track()
{
	uint8_t left = static_cast<uint8_t>(wram_.num_tracks() - 1);
	wram_.num_tracks() = left;
	if (!left)
		return false;
	wram_.cur_track_ptr().word() = static_cast<uint16_t>(wram_.cur_track_ptr().word() + TRACK_STRUCT_LENGTH);
	wram_.cur_sound_register() += 5; // the next sound generator's registers
	return true;
}

void Engine::map_rom_bank(uint8_t bank)
{
	wram_.rom_bank() = bank;
	ram(rROMB0) = bank;
}

uint8_t Engine::read_command_byte(uint16_t addr)
{
	map_rom_bank(wram_.sound_bank());
	uint8_t value = mem_[addr];
	map_rom_bank(AUDIO_ENGINE_BANK);
	return value;
}

uint16_t Engine::read_command_word(uint16_t addr)
{
	map_rom_bank(wram_.sound_bank());
	uint8_t lo = mem_[addr];
	uint8_t hi = mem_[static_cast<uint16_t>(addr + 1)];
	map_rom_bank(AUDIO_ENGINE_BANK);
	return static_cast<uint16_t>(hi << 8 | lo);
}

// ---------------------------------------------------------------------------
// Init and update

void Engine::init()
{
	wram_.audio_engine_flags() = 0xff;
	uint8_t bank_backup = wram_.audio_bank_backup();
	for (uint16_t addr = wAudioBankBackup; addr <= wStartChannel; addr++)
		ram(addr) = 0;
	wram_.audio_bank_backup() = bank_backup;

	wram_.sfx_base_tempo() = 74;
	wram_.sfx_tempo() = 74;
	wram_.music_base_tempo() = 74;
	wram_.music_tempo() = 74;
	wram_.sfx_tempo_mult() = Q6_1_0;
	wram_.music_tempo_mult() = Q6_1_0;
	wram_.cur_wave_sample() = 0xff;

	for (int i = 0; i < NUM_CHANNELS; i++)
		Channel(gb_, static_cast<uint16_t>(wChannels + i * CHANNEL_STRUCT_LENGTH)).flags() = 0;
	for (int i = 0; i < NUM_TRACKS; i++)
		Track(gb_, static_cast<uint16_t>(wTracks + i * TRACK_STRUCT_LENGTH)).flags() = 0;

	mem_[rAUDENA] = AUDENA_ON;
	mem_[rAUDTERM] = 0x00;
	mem_[rAUD1ENV] = AUD1ENV_UP;
	mem_[rAUD2ENV] = AUD1ENV_UP;
	mem_[rAUD4ENV] = AUD1ENV_UP;
	mem_[rAUD1HIGH] = AUD1HIGH_RESTART;
	mem_[rAUD2HIGH] = AUD1HIGH_RESTART;
	mem_[rAUD4GO] = AUD1HIGH_RESTART;
	mem_[rAUD3ENA] = AUD3ENA_OFF;
	mem_[rAUDTERM] = 0xff; // all channels in both speakers
	mem_[rAUDVOL] = MAX_VOLUME;
	wram_.audio_engine_flags() = 0x00;
}

void Engine::update(uint8_t caller_c)
{
	update_music_fade_out(caller_c);

	// The SFX, then the music: each steps its channels once per $4a of
	// tempo accumulated (with no step due, only the channels' outputs are
	// updated).
	struct Part {
		bool sfx;
		uint16_t tempo, accumulator;
	};
	static const Part parts[] = {{true, wSFXTempo, w3d008}, {false, wMusicTempo, w3d00d}};
	for (const Part &part : parts) {
		if (part.sfx)
			wram_.audio_engine_flags().set_bit(AUDIOENG_HANDLING_SFX_F);
		else
			wram_.audio_engine_flags().reset_bit(AUDIOENG_HANDLING_SFX_F);
		Word accumulator = ram_word(part.accumulator);
		accumulator = static_cast<uint16_t>(accumulator + ram(part.tempo));
		auto set_part_channels = [&] { part.sfx ? set_sfx_channels() : set_music_channels(); };
		if (accumulator < 0x4a) {
			set_part_channels();
			do
				update_channel_without_commands();
			while (next_channel());
			continue;
		}
		do {
			accumulator = static_cast<uint16_t>(accumulator - 0x4a);
			set_all_tracks();
			do
				part.sfx ? update_sfx_track() : update_music_track();
			while (next_track());
			set_part_channels();
			do
				update_channel();
			while (next_channel());
		} while (accumulator >= 0x4a);
	}

	set_all_tracks();
	do
		update_track_output();
	while (next_track());

	uint8_t counter = wram_.unk_3d01a();
	wram_.unk_3d01a() = static_cast<uint8_t>((counter ? counter : 0x0f) - 1);

	// clear the channels' update flags, and collect which are active
	uint8_t active = 0;
	for (int i = 0; i < NUM_CHANNELS; i++) {
		Channel ch(gb_, static_cast<uint16_t>(wChannels + i * CHANNEL_STRUCT_LENGTH));
		uint8_t flags = ch.flags() & ~CHANNEL_UPDATES;
		ch.flags() = flags;
		active = static_cast<uint8_t>(active >> 1 | (flags & CHANFLAGS_ACTIVE));
	}
	wram_.active_channels() = active;
}

// ---------------------------------------------------------------------------
// Starting and stopping sounds

void Engine::play_sfx(uint16_t id)
{
	if (!id) {
		turn_sfx_off();
		return;
	}
	if (!load_sound(id))
		return;

	uint8_t start = wram_.start_channel();
	if (start & 0x80) {
		// Any free SFX channels first (wSoundEngineParam4 is the bit of the
		// channel, wSoundEngineParam3 collects the ones loaded), then the
		// others whose sound has no higher priority.
		set_sfx_channels();
		wram_.sound_engine_param4() = 1;
		wram_.sound_engine_param3() = 0;
		do {
			if (!cur_channel().active()) {
				if (load_channel_with_sound())
					return;
				wram_.sound_engine_param3() |= wram_.sound_engine_param4();
			}
			wram_.sound_engine_param4() = static_cast<uint8_t>(wram_.sound_engine_param4() << 1);
		} while (next_channel());

		set_sfx_channels();
		wram_.sound_engine_param4() = 1;
		do {
			if (!(wram_.sound_engine_param3() & wram_.sound_engine_param4()) &&
				wram_.sound_priority() >= cur_channel()[CHANNEL_PRIORITY]) {
				if (load_channel_with_sound())
					return;
			}
			wram_.sound_engine_param4() = static_cast<uint8_t>(wram_.sound_engine_param4() << 1);
		} while (next_channel());
		return;
	}

	// from the given channel on, over sounds of no higher priority
	if (start >= CHAN5_F)
		return;
	wram_.num_audio_channels() = static_cast<uint8_t>(CHAN5_F - start);
	wram_.cur_channel_ptr().word() = static_cast<uint16_t>(wChannels + start * CHANNEL_STRUCT_LENGTH);
	for (;;) {
		Channel ch = cur_channel();
		bool load = !ch.active() || wram_.sound_priority() >= ch[CHANNEL_PRIORITY];
		if (load ? load_channel_with_sound() : skip_loading_channel())
			return;
		if (!next_channel())
			return;
	}
}

void Engine::play_music(uint16_t id)
{
	if (!id) {
		deactivate_music_channels();
		return;
	}
	wram_.loaded_music().word() = id;
	wram_.music_fade_out_period() = 0;
	wram_.unk_3d025() = 0;
	if (!load_sound(id))
		return;

	set_music_channels();
	for (;;) {
		if (load_channel_with_sound())
			break;
		if (!next_channel())
			return;
	}
	// the music channels it doesn't use
	while (next_channel())
		cur_channel().flags() = 0;
}

void Engine::play_new_music(uint16_t id)
{
	if (wram_.loaded_music().word() != id || !(wram_.active_channels() & MUSIC_CHANNELS))
		play_music(id);
}

void Engine::play_new_music_set_noise(uint16_t id)
{
	if (wram_.loaded_music().word() != id)
		play_music(id);
	else if (!(wram_.active_channels() & MUSIC_CHANNELS))
		resume_music();
}

void Engine::resume_music()
{
	wram_.unk_3d025() = 0;
	set_music_channels();
	do {
		Channel ch = cur_channel();
		if (ch.flags() & (CHANFLAGS_5 | CHANFLAGS_6))
			ch.flags().set_bit(CHANFLAGS_ACTIVE_F);
	} while (next_channel());
}

// GetSound: loads the sound's header. False for an invalid ID.
bool Engine::load_sound(uint16_t id)
{
	ram(wCurSoundID + 1) = static_cast<uint8_t>(id >> 8);
	if (id >= NUM_SOUNDS)
		return false;
	ram(wCurSoundID + 0) = static_cast<uint8_t>(id);

	uint16_t header = static_cast<uint16_t>(ADDR_SoundHeaders - 8 + id * 8);
	uint16_t commands = mem_.word(header);
	wram_.sound_bank() = mem_[static_cast<uint16_t>(header + 2)];
	wram_.sound_priority() = mem_[static_cast<uint16_t>(header + 4)];
	wram_.start_channel() = mem_[static_cast<uint16_t>(header + 5)];
	wram_.num_channels() = mem_[static_cast<uint16_t>(header + 6)];
	wram_.audio_cmd_ptr().word() = static_cast<uint16_t>(commands + 2);
	return true;
}

// Gives the current channel the sound's next part. True when that was its
// last part.
bool Engine::load_channel_with_sound()
{
	Channel ch = cur_channel();
	ch.flags() = CHANFLAGS_5 | CHANFLAGS_ACTIVE;
	ch.word(CHANNEL_CMD_PTR) = wram_.audio_cmd_ptr().word();
	ch[CHANNEL_SOUND_BANK] = wram_.sound_bank();
	ch.word(CHANNEL_SOUND_ID) = wram_.cur_sound_id().word();
	ch[CHANNEL_PRIORITY] = wram_.sound_priority();
	return skip_loading_channel();
}

// Moves on to the sound's next part. True when there's none left.
bool Engine::skip_loading_channel()
{
	uint8_t left = static_cast<uint8_t>(wram_.num_channels() - 1);
	wram_.num_channels() = left;
	if (!left)
		return true;
	wram_.audio_cmd_ptr().word() = static_cast<uint16_t>(wram_.audio_cmd_ptr().word() + 2);
	return false;
}

// Starts a newly loaded channel: its command pointer pointed at the pointer
// to its commands.
void Engine::init_channel(Channel ch)
{
	// the fields from CHANNEL_PITCH to CHANNEL_UNK_30
	static const uint8_t initial[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // PITCH .. SWEEP
		0xff, 0xff,                         // FADE_IN/OUT_ENVELOPE
		0x00, 0x00, 0x00, 0x00,             // ACTIVE_COMMAND .. PITCH_OFFSET_MOD
		0xff, Q6_1_0,                       // VOLUME, VOLUME_MOD
		0x00, 0x00, 0x00, 0x02,             // pans, PITCH_OFFSET(_MULT)
		0x00, 0x00, 0x00, 0x00,             // PITCH_PRODUCT, UNK_1D, UNK_1E
		0x17,                               // VIBRATO_SPEED
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // UNK_VIBRATO_VALUE ..
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // .. UNK_2F
		0xff,                               // UNK_30
	};
	ch.flags() = CHANFLAGS_6 | CHANFLAGS_ACTIVE;
	ch[CHANNEL_DURATION] = 0;
	uint16_t pointer = ch.word(CHANNEL_CMD_PTR);
	wram_.sound_bank() = ch[CHANNEL_SOUND_BANK];
	ch.word(CHANNEL_CMD_PTR) = read_command_word(pointer);
	for (unsigned i = 0; i < sizeof(initial); i++)
		ch[CHANNEL_PITCH + static_cast<int>(i)] = initial[i];
}

void Engine::turn_off_sfx(uint16_t id)
{
	if (!id) {
		turn_sfx_off();
		return;
	}
	wram_.cur_sound_id().word() = id;
	set_sfx_channels();
	do {
		Channel ch = cur_channel();
		if (ch.active() && ch[CHANNEL_SOUND_ID] == wram_.cur_sound_id() &&
			ch[CHANNEL_SOUND_ID + 1] == ram(wCurSoundID + 1))
			ch.flags() = 0;
	} while (next_channel());
}

void Engine::turn_sfx_off()
{
	set_sfx_channels();
	do
		cur_channel().flags() = 0;
	while (next_channel());
}

void Engine::turn_music_off()
{
	deactivate_music_channels();
}

void Engine::deactivate_music_channels()
{
	wram_.unk_3d025() = 0xff;
	set_music_channels();
	do
		cur_channel().flags().reset_bit(CHANFLAGS_ACTIVE_F);
	while (next_channel());
}

uint16_t Engine::playing_sound_id(uint8_t which)
{
	if (!which)
		return (wram_.active_channels() & MUSIC_CHANNELS) ? static_cast<uint16_t>(wram_.loaded_music().word()) : 0;
	if (which >= NUM_SFX_CHANNELS + 1)
		return 0;
	// a table of the SFX channels, just before AudioModFunctionTable
	uint16_t table = static_cast<uint16_t>(ADDR_AudioModFunctionTable - 2 * NUM_MUSIC_CHANNELS - 2);
	Channel ch(gb_, mem_.word(static_cast<uint16_t>(table + static_cast<uint8_t>(which << 1))));
	if (!ch.active())
		return 0;
	return ch.word(CHANNEL_SOUND_ID);
}

// ---------------------------------------------------------------------------
// Audio mods: changes to the playing channels' tempo, pitch, volume, pan or
// vibrato. wSoundEngineParam4 and 3 select the SFX and music channels (a
// bit each), 2 and 1 are the value, and wNumChannels holds the channel
// update flags to set.

void Engine::execute_audio_mod(uint8_t mod, uint8_t b, uint8_t c, uint8_t d, uint8_t e)
{
	execute_audio_mod_internal(mod, b, c, d, e);
}

void Engine::execute_audio_mod_internal(uint8_t mod, uint8_t b, uint8_t c, uint8_t d, uint8_t e)
{
	if (mod >= NUM_AUDIOMOD_FUNCTIONS)
		return;
	wram_.sound_engine_param1() = c;
	wram_.sound_engine_param2() = b;
	wram_.sound_engine_param3() = e;
	wram_.sound_engine_param4() = d;
	wram_.num_channels() = 0;

	switch (mem_.word(static_cast<uint16_t>(ADDR_AudioModFunctionTable + mod * 2))) {
	case ADDR_AudioMod_Tempo:
		if (wram_.sound_engine_param4()) {
			wram_.sfx_tempo_mult() = wram_.sound_engine_param2();
			calculate_tempo(wSFXBaseTempo);
		}
		if (wram_.sound_engine_param3()) {
			wram_.music_tempo_mult() = wram_.sound_engine_param2();
			calculate_tempo(wMusicBaseTempo);
		}
		return;
	case ADDR_AudioMod_Pitch:
		wram_.num_channels() |= CHANFLAGS_UPDATE_FREQUENCY;
		audio_mod_set_long(CHANNEL_PITCH_OFFSET_MOD);
		return;
	case ADDR_AudioMod_Volume:
		wram_.num_channels() |= CHANFLAGS_UPDATE_VOLUME;
		audio_mod_set_short(CHANNEL_VOLUME_MOD);
		return;
	case ADDR_AudioMod_Pan:
		wram_.num_channels() |= CHANFLAGS_UPDATE_SO;
		audio_mod_set_short(CHANNEL_GLOBAL_PAN);
		return;
	case ADDR_AudioMod_VibratoAmplitude:
		audio_mod_set_short(CHANNEL_VIBRATO_AMPLITUDE_MOD);
		return;
	case ADDR_AudioMod_VibratoSpeed: {
		uint8_t speed = wram_.sound_engine_param2();
		if (static_cast<uint8_t>(speed << 1)) {
			audio_mod_set_short(CHANNEL_VIBRATO_SPEED);
			return;
		}
		// speed $00 or $80: also sets the vibrato value
		bool half = speed & 0x80;
		wram_.sound_engine_param1() = half ? 0x80 : 0x00;
		wram_.sound_engine_param2() = half ? 0x00 : 0x40;
		audio_mod_set_long(CHANNEL_VIBRATO_SPEED);
		return;
	}
	case ADDR_Func_309b6:
		audio_mod_set_long(CHANNEL_UNK_27);
		return;
	}
	// The table is in ROM, so only a garbage bank can get here.
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// PrepareChannelBitmasks: bit i selects channel i (SFX 1-4, then music).
uint8_t Engine::channel_bitmask() const
{
	return static_cast<uint8_t>((wram_.sound_engine_param4() & 0x0f) | (swap(wram_.sound_engine_param3()) & 0xf0));
}

void Engine::audio_mod_set_short(int field)
{
	uint8_t mask = channel_bitmask();
	for (int i = 0; i < NUM_CHANNELS; i++, mask = rrca(mask)) {
		Channel ch(gb_, static_cast<uint16_t>(wChannels + i * CHANNEL_STRUCT_LENGTH));
		if ((mask & 1) && ch.active()) {
			ch.flags() |= wram_.num_channels();
			ch[field] = wram_.sound_engine_param2();
		}
	}
}

void Engine::audio_mod_set_long(int field)
{
	uint8_t mask = channel_bitmask();
	for (int i = 0; i < NUM_CHANNELS; i++, mask = rrca(mask)) {
		Channel ch(gb_, static_cast<uint16_t>(wChannels + i * CHANNEL_STRUCT_LENGTH));
		if ((mask & 1) && ch.active()) {
			ch.flags() |= wram_.num_channels();
			ch[field] = wram_.sound_engine_param1();
			ch[field + 1] = wram_.sound_engine_param2();
		}
	}
}

void Engine::set_music_fade_out(uint8_t period)
{
	wram_.music_fade_out_period() = period;
	wram_.music_fade_out_timer() = period;
	wram_.music_fade_out_volume_mult() = Q6_1_0;
}

void Engine::update_music_fade_out(uint8_t caller_c)
{
	if (wram_.unk_3d025())
		return;
	uint8_t period = wram_.music_fade_out_period();
	if (!period)
		return;
	uint8_t timer = static_cast<uint8_t>(wram_.music_fade_out_timer() - 1);
	wram_.music_fade_out_timer() = timer;
	if (timer)
		return;
	wram_.music_fade_out_timer() = period;
	uint8_t volume = wram_.music_fade_out_volume_mult();
	if (volume < Q6_0_063) {
		wram_.music_fade_out_period() = 0;
		deactivate_music_channels();
		return;
	}
	volume = static_cast<uint8_t>(volume - Q6_0_063);
	wram_.music_fade_out_volume_mult() = volume;
	execute_audio_mod_internal(AUDIOMOD_VOLUME, volume, caller_c, 0x00, 0xff);
}

// base_tempo: wSFXBaseTempo or wMusicBaseTempo, followed by its multiplier
// and the resulting tempo
void Engine::calculate_tempo(uint16_t base_tempo)
{
	uint16_t product = multiply(ram(base_tempo), ram(static_cast<uint16_t>(base_tempo + 1)));
	if ((product >> 8) >= Q6_256_0_HI)
		product = 0x3fff; // 255.99
	uint8_t tempo = static_cast<uint8_t>(static_cast<uint16_t>(product << 2) >> 8);
	ram(static_cast<uint16_t>(base_tempo + 2)) = tempo ? tempo : 1;
}

// ---------------------------------------------------------------------------
// Updating channels

void Engine::update_channel_without_commands()
{
	Channel ch = cur_channel();
	uint8_t flags = ch.flags();
	if (flags < CHANNEL_PLAYING)
		return;
	wram_.cur_channel_flags() = flags;
	apply_channel_updates(ch, flags);
}

void Engine::update_channel()
{
	Channel ch = cur_channel();
	uint8_t flags = ch.flags();
	if (!(flags & CHANFLAGS_ACTIVE))
		return;
	if (!(flags & CHANFLAGS_6)) {
		init_channel(ch);
		flags = ch.flags();
	}
	wram_.cur_channel_flags() = flags;

	uint8_t duration = ch[CHANNEL_DURATION];
	if (duration) {
		ch[CHANNEL_DURATION] = static_cast<uint8_t>(duration - 1);
		update_vibrato(ch);
		return;
	}
	uint16_t cmd = ch.word(CHANNEL_CMD_PTR);
	wram_.sound_bank() = ch[CHANNEL_SOUND_BANK];
	do_audio_commands(cmd);
}

void Engine::update_vibrato(Channel ch)
{
	uint8_t value;
	uint8_t delay = ch[CHANNEL_VIBRATO_COUNTER];
	if (delay) {
		// still counting down the vibrato delay
		ch[CHANNEL_VIBRATO_COUNTER] = static_cast<uint8_t>(delay - 1);
		value = 0x40;
	} else {
		value = static_cast<uint8_t>(ch[CHANNEL_VIBRATO_SPEED] + ch[CHANNEL_UNK_VIBRATO_VALUE]);
	}
	ch[CHANNEL_UNK_VIBRATO_VALUE] = value;
	uint8_t wave = static_cast<uint8_t>(value << 1); // a triangle wave
	if (value & 0x80)
		wave = static_cast<uint8_t>(~wave);
	wram_.vibrato_value() = wave;

	uint8_t amplitude = static_cast<uint8_t>(ch[CHANNEL_VIBRATO_AMPLITUDE] + ch[CHANNEL_VIBRATO_AMPLITUDE_MOD]);
	bool enabled = ch[CHANNEL_VIBRATO_DISABLED] == FALSE;
	if (enabled) {
		// the pitch offset: (amplitude * wave - amplitude / 2) * 8
		uint16_t offset = 0;
		if (amplitude) {
			uint8_t scaled = static_cast<uint8_t>(multiply(wave, amplitude) >> 8);
			uint8_t half = amplitude >> 1;
			uint16_t diff = static_cast<uint16_t>((scaled < half ? 0xff00 : 0) | static_cast<uint8_t>(scaled - half));
			offset = static_cast<uint16_t>(diff << 3);
		}
		if (static_cast<uint8_t>(offset) != ch[CHANNEL_UNK_24]) {
			wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_FREQUENCY;
			ch[CHANNEL_UNK_24] = static_cast<uint8_t>(offset);
		}
		if (static_cast<uint8_t>(offset >> 8) != ch[CHANNEL_UNK_24 + 1]) {
			wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_FREQUENCY;
			ch[CHANNEL_UNK_24 + 1] = static_cast<uint8_t>(offset >> 8);
		}
	}
	apply_channel_updates(ch, wram_.cur_channel_flags());
}

// Func_30651: recalculates the channel's frequency, output and volume as
// its update flags ask.
void Engine::apply_channel_updates(Channel ch, uint8_t flags)
{
	if (flags & CHANFLAGS_UPDATE_FREQUENCY) {
		int8_t fine = static_cast<int8_t>(static_cast<uint8_t>(ch[CHANNEL_UNK_1D]));
		uint16_t frequency = static_cast<uint16_t>(fine * 2 + ch.word(CHANNEL_PITCH_PRODUCT) +
			ch.word(CHANNEL_PITCH_OFFSET_MOD));
		if (ch[CHANNEL_VIBRATO_DISABLED] == FALSE)
			frequency = static_cast<uint16_t>(frequency + ch.word(CHANNEL_UNK_24));
		ch.word(CHANNEL_FREQUENCY) = frequency;
		flags = wram_.cur_channel_flags();
	}
	if (flags & CHANFLAGS_UPDATE_SO) {
		ch[CHANNEL_SO_FLAGS] = static_cast<uint8_t>(ch[CHANNEL_SINGLE_PAN] + ch[CHANNEL_GLOBAL_PAN]);
		flags = wram_.cur_channel_flags();
	}
	if (flags & CHANFLAGS_UPDATE_VOLUME) {
		uint8_t volume = static_cast<uint8_t>((multiply_short(ch[CHANNEL_VOLUME], ch[CHANNEL_VOLUME_MOD]) + 0x0f) & 0xf0);
		if (volume >= 0x40)
			volume = 0xff;
		ch[CHANNEL_UNK_2F] = rlca(rlca(volume));
		flags = wram_.cur_channel_flags();
	}
	ch.flags() = flags;
}

// DoAudioCommand: runs the channel's commands up to the next note or rest.
// A command byte with bit 7 clear repeats the channel's previous command
// with that byte as its argument.
void Engine::do_audio_commands(uint16_t cmd)
{
	for (;;) {
		Channel ch = cur_channel();
		uint16_t bytes = read_command_word(cmd);
		uint8_t first = static_cast<uint8_t>(bytes), second = static_cast<uint8_t>(bytes >> 8);
		uint8_t command;
		if (!(first & 0x80)) {
			wram_.audio_cmd_arg() = first;
			command = ch[CHANNEL_ACTIVE_COMMAND];
		} else {
			cmd++;
			wram_.audio_cmd_arg() = second;
			command = first;
			if (command >= 0xbe)
				ch[CHANNEL_ACTIVE_COMMAND] = command;
		}
		// cmd points at the argument now
		bool next;
		if (command >= 0xd0)
			next = cmd_note(cmd, mem_[static_cast<uint16_t>(ADDR_Data_3102b + static_cast<uint8_t>(command - 0xcf))]);
		else if (command < 0xb1)
			next = cmd_rest(cmd, command);
		else
			next = run_command(ADDR_PointerTable_306d1, static_cast<uint8_t>(command - 0xb1), cmd);
		if (!next)
			return;
	}
}

// JumpToPointerInTable into one of the command tables
bool Engine::run_command(uint16_t table, uint8_t index, uint16_t &cmd)
{
	return command_by_address(mem_.word(static_cast<uint16_t>(table + static_cast<uint8_t>(index << 1))), cmd);
}

bool Engine::command_by_address(uint16_t handler, uint16_t &cmd)
{
	switch (handler) {
	case ADDR_AudioCmd_ClearFlags: return cmd_clear_flags(cmd);
	case ADDR_InvalidAudioCmd: return cmd_clear_flags(cmd);
	case ADDR_AudioCmd_Jump: return cmd_jump(cmd);
	case ADDR_AudioCmd_Call: return cmd_call(cmd);
	case ADDR_AudioCmd_Ret: return cmd_ret(cmd);
	case ADDR_AudioCmd_Loop: return cmd_loop(cmd);
	case ADDR_AudioCmd_SetTempo: return cmd_set_tempo(cmd);
	case ADDR_AudioCmd_SetWave: return cmd_set_wave(cmd);
	case ADDR_AudioCmd_SetChannelVolume: return cmd_set_volume(cmd);
	case ADDR_AudioCmd_SetPitchOffset: return cmd_set_pitch_offset(cmd);
	case ADDR_AudioCmd_SetPitchOffsetMultiplier: return cmd_set_pitch_offset_multiplier(cmd);
	case ADDR_AudioCmd_SetVibratoSpeed: return cmd_set_vibrato_speed(cmd);
	case ADDR_AudioCmd_SetSemitoneOffset: return cmd_set_property(cmd, CHANNEL_SEMITONE_OFFSET);
	case ADDR_AudioCmd_SetVibratoDelay: return cmd_set_property(cmd, CHANNEL_VIBRATO_DELAY);
	case ADDR_AudioCmd_SetVibratoAmplitude: return cmd_set_property(cmd, CHANNEL_VIBRATO_AMPLITUDE);
	case ADDR_AudioCmd_SetChannelUnk27: return cmd_set_property(cmd, CHANNEL_UNK_27);
	case ADDR_AudioCmd_SetChannelUnk28: return cmd_set_property(cmd, CHANNEL_UNK_28);
	case ADDR_AudioCmd_SetChannelTimbre: return cmd_set_property(cmd, CHANNEL_TIMBRE);
	case ADDR_AudioCmd_SetChannelDuration: return cmd_set_property(cmd, CHANNEL_LENGTH);
	case ADDR_AudioCmd_SetChannelSweep: return cmd_set_property(cmd, CHANNEL_SWEEP);
	case ADDR_AudioCmd_SetChannelVibratoDisabled:
		wram_.cur_channel_flags() |= CHANNEL_UPDATES;
		return cmd_set_property(cmd, CHANNEL_VIBRATO_DISABLED);
	case ADDR_AudioCmd_SetChannelUnk1e:
		wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_FREQUENCY;
		return cmd_set_property(cmd, CHANNEL_UNK_1E);
	case ADDR_AudioCmd_Pan:
		wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_SO;
		return cmd_set_signed(cmd, CHANNEL_SINGLE_PAN);
	case ADDR_AudioCmd_SetChannelUnk1d:
		wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_FREQUENCY;
		return cmd_set_signed(cmd, CHANNEL_UNK_1D);
	case ADDR_AudioCmd_SetChannelFadeInEnvelope: return cmd_set_high_nybble(cmd, CHANNEL_FADE_IN_ENVELOPE);
	case ADDR_Func_309e8: return cmd_set_low_nybble(cmd, CHANNEL_FADE_IN_ENVELOPE);
	case ADDR_Func_30a03: return cmd_set_high_nybble(cmd, CHANNEL_FADE_OUT_ENVELOPE);
	case ADDR_AudioCmd_SetChannelFadeOutEnvelope: return cmd_set_low_nybble(cmd, CHANNEL_FADE_OUT_ENVELOPE);
	case ADDR_Func_30727: return cmd_extended(cmd);
	case ADDR_Func_30a10: return cmd_note(cmd, 0);
	case ADDR_Func_30b4d: return cmd_stop_note(cmd);
	}
	// The tables are in ROM, so only a garbage bank can get here.
	if (gb_->test.active)
		gb_->test.unrealistic = true;
	return false;
}

bool Engine::cmd_clear_flags(uint16_t &)
{
	cur_channel().flags() = 0;
	return false;
}

// Func_30727: a command from a second table, with its own argument byte
bool Engine::cmd_extended(uint16_t &cmd)
{
	uint8_t index = wram_.audio_cmd_arg();
	cmd++;
	if (index >= 0x0c)
		return cmd_clear_flags(cmd);
	wram_.audio_cmd_arg() = read_command_byte(cmd);
	return run_command(ADDR_PointerTable_3070f, index, cmd);
}

// Func_3073f: commands below $b1 rest (or hold) for a duration from
// Data_3102b, $80 does nothing
bool Engine::cmd_rest(uint16_t &cmd, uint8_t command)
{
	uint8_t index = static_cast<uint8_t>(command - 0x80);
	if (!index)
		return true;
	uint8_t duration = static_cast<uint8_t>(mem_[static_cast<uint16_t>(ADDR_Data_3102b + index)] - 1);
	Channel ch = cur_channel();
	ch[CHANNEL_DURATION] = duration;
	ch.word(CHANNEL_CMD_PTR) = cmd;
	update_vibrato(ch);
	return false;
}

bool Engine::cmd_jump(uint16_t &cmd)
{
	cmd = read_command_word(cmd);
	return true;
}

// Calls nest up to 5 deep; the return addresses go after CHANNEL_UNK_26.
bool Engine::cmd_call(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t depth = ch[CHANNEL_UNK_26];
	if (depth >= 0x0a)
		return cmd_clear_flags(cmd);
	depth = static_cast<uint8_t>(depth + 2);
	ch[CHANNEL_UNK_26] = depth;
	ch.word(CHANNEL_UNK_26 + depth + 0x0a) = static_cast<uint16_t>(cmd + 2);
	return cmd_jump(cmd);
}

bool Engine::cmd_ret(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t depth = ch[CHANNEL_UNK_26];
	if (!depth)
		return true;
	depth = static_cast<uint8_t>(depth - 2);
	ch[CHANNEL_UNK_26] = depth;
	cmd = ch.word(CHANNEL_UNK_26 + depth + 0x0c);
	return true;
}

// Jumps back the given number of times, then goes on.
bool Engine::cmd_loop(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t count = wram_.audio_cmd_arg();
	cmd++;
	if (!count)
		return cmd_jump(cmd);
	uint8_t done = static_cast<uint8_t>(ch[CHANNEL_UNK_29] + 1);
	ch[CHANNEL_UNK_29] = done;
	if (count != done)
		return cmd_jump(cmd);
	ch[CHANNEL_UNK_29] = 0;
	cmd = static_cast<uint16_t>(cmd + 2);
	return true;
}

bool Engine::cmd_set_tempo(uint16_t &cmd)
{
	uint16_t base = wram_.audio_engine_flags().bit(AUDIOENG_HANDLING_SFX_F) ? wSFXBaseTempo : wMusicBaseTempo;
	ram(base) = wram_.audio_cmd_arg();
	cmd++;
	calculate_tempo(base);
	return true;
}

// SetChannelProperty
bool Engine::cmd_set_property(uint16_t &cmd, int field)
{
	cur_channel()[field] = wram_.audio_cmd_arg();
	cmd++;
	return true;
}

// The timbre, length, sweep and fade envelopes of a wave (WAVE_*), or the
// note's own percussion sound for WAVE_28.
bool Engine::cmd_set_wave(uint16_t &cmd)
{
	uint8_t wave = wram_.audio_cmd_arg();
	cmd++;
	if (wave == WAVE_28) {
		wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_PERCUSSION;
		return true;
	}
	uint16_t data = static_cast<uint16_t>(ADDR_Waves + wave * 6);
	Channel ch = cur_channel();
	for (int i = 0; i < 5; i++)
		ch[CHANNEL_TIMBRE + i] = mem_[static_cast<uint16_t>(data + i)];
	wram_.cur_channel_flags() &= ~CHANFLAGS_UPDATE_PERCUSSION;
	return true;
}

bool Engine::cmd_set_pitch_offset(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t offset = static_cast<uint8_t>(rlca(wram_.audio_cmd_arg()) - 0x80);
	cmd++;
	ch[CHANNEL_PITCH_OFFSET] = offset;
	return set_pitch_product(ch, offset, ch[CHANNEL_PITCH_OFFSET_MULT]);
}

bool Engine::cmd_set_pitch_offset_multiplier(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t mult = wram_.audio_cmd_arg();
	cmd++;
	ch[CHANNEL_PITCH_OFFSET_MULT] = mult;
	return set_pitch_product(ch, ch[CHANNEL_PITCH_OFFSET], mult);
}

// CalculatePitchProduct: the signed pitch offset times its multiplier
bool Engine::set_pitch_product(Channel ch, uint8_t offset, uint8_t mult)
{
	uint8_t magnitude = static_cast<uint8_t>(offset << 1);
	uint16_t product;
	if (!(offset & 0x80))
		product = multiply(magnitude, mult);
	else
		product = static_cast<uint16_t>(~(multiply(static_cast<uint8_t>(~magnitude), mult) - 1));
	ch.word(CHANNEL_PITCH_PRODUCT) = product;
	wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_FREQUENCY;
	return true;
}

bool Engine::cmd_set_vibrato_speed(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t speed = wram_.audio_cmd_arg();
	cmd++;
	ch[CHANNEL_VIBRATO_SPEED] = speed;
	if (!static_cast<uint8_t>(speed << 1))
		ch[CHANNEL_UNK_VIBRATO_VALUE] = (speed & 0x80) ? 0x00 : 0x40;
	return true;
}

bool Engine::cmd_set_volume(uint16_t &cmd)
{
	wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_VOLUME;
	cur_channel()[CHANNEL_VOLUME] = rlca(wram_.audio_cmd_arg());
	cmd++;
	return true;
}

// Func_3098b: a signed value, centred on $80
bool Engine::cmd_set_signed(uint16_t &cmd, int field)
{
	cur_channel()[field] = static_cast<uint8_t>(rlca(wram_.audio_cmd_arg()) - 0x80);
	cmd++;
	return true;
}

bool Engine::cmd_set_high_nybble(uint16_t &cmd, int field)
{
	Byte value = cur_channel()[field];
	value = static_cast<uint8_t>((swap(wram_.audio_cmd_arg()) & 0xf0) | (value & 0x0f));
	cmd++;
	return true;
}

bool Engine::cmd_set_low_nybble(uint16_t &cmd, int field)
{
	Byte value = cur_channel()[field];
	value = static_cast<uint8_t>((wram_.audio_cmd_arg() & 0x0f) | (value & 0xf0));
	cmd++;
	return true;
}

// Func_30a1f: a note. Its arguments (each optional, in any order, up to the
// next command byte) set the pitch, CHANNEL_UNK_0A, and add to the duration.
// Then the channel takes over its note's track unless a sound of higher
// priority has it.
bool Engine::cmd_note(uint16_t &cmd, uint8_t duration)
{
	Channel ch = cur_channel();
	ch[CHANNEL_UNK_0B] = duration;
	uint8_t seen = 0;
	uint8_t arg = wram_.audio_cmd_arg();
	for (bool first = true;; first = false) {
		if (!first) {
			cmd++;
			arg = read_command_byte(cmd);
		}
		if (arg & 0x80)
			break;
		if (arg >= C_0) { // pitch
			if (seen & 0x80)
				break;
			seen |= 0x80;
			ch[CHANNEL_PITCH] = arg;
		} else if (arg < 0x20) {
			if (seen & 0x40)
				break;
			seen |= 0x40;
			ch[CHANNEL_UNK_0A] = static_cast<uint8_t>(rlca(rlca(rlca(arg))) | 0x07);
		} else {
			if (seen & 0x20)
				break;
			seen |= 0x20;
			ch[CHANNEL_UNK_0B] = static_cast<uint8_t>(ch[CHANNEL_UNK_0B] + arg);
		}
	}

	uint8_t note = static_cast<uint8_t>(ch[CHANNEL_PITCH] + ch[CHANNEL_SEMITONE_OFFSET]);
	wram_.sound_engine_param3() = note;
	wram_.sound_engine_param4() = note;
	ch[CHANNEL_VIBRATO_COUNTER] = ch[CHANNEL_VIBRATO_DELAY];

	if (wram_.cur_channel_flags() & CHANFLAGS_UPDATE_PERCUSSION) {
		uint16_t data = static_cast<uint16_t>(ADDR_Waves + static_cast<uint8_t>(note + 4) * 6);
		for (int i = 0; i < 5; i++)
			ch[CHANNEL_TIMBRE + i] = mem_[static_cast<uint16_t>(data + i)];
		wram_.sound_engine_param4() = mem_[static_cast<uint16_t>(data + 5)];
	}

	// the timbre selects the track: duty cycles for the two square waves,
	// waveforms for the wave channel, the rest is noise
	uint8_t timbre = ch[CHANNEL_TIMBRE];
	if (timbre >= 0x10) {
		if (timbre >= 0x40)
			set_tracks(wTrack4, 1, SOUND4);
		else
			set_tracks(wTrack3, 1, SOUND3);
	} else {
		if (timbre >= 0x08)
			set_tracks(wTrack2, 1, SOUND2);
		else
			set_tracks(wTrack1, 1, static_cast<uint8_t>(rAUD1ENV));
	}

	uint8_t owner = wram_.num_audio_channels();
	if (wram_.audio_engine_flags().bit(AUDIOENG_HANDLING_SFX_F))
		owner |= 0x80;
	wram_.temp_track_unk02() = owner;
	uint8_t priority = ch[CHANNEL_PRIORITY];
	wram_.temp_track_priority() = priority;

	Track tr = cur_track();
	uint8_t track_flags = tr.flags();
	bool take = true;
	if (track_flags && (track_flags & TRACKFLAGS_5)) {
		uint8_t track_priority = tr[TRACK_PRIORITY];
		if (track_priority > priority || (track_priority == priority && owner < tr[TRACK_UNK02]))
			take = false;
	}
	if (take) {
		turn_sound_register_off();
		uint16_t channel = wram_.cur_channel_ptr().word();
		tr = cur_track();
		tr.flags() = TRACKFLAGS_4 | TRACKFLAGS_5 | TRACKFLAGS_6 | TRACKFLAGS_7;
		tr[TRACK_PRIORITY] = wram_.temp_track_priority();
		tr[TRACK_UNK02] = wram_.temp_track_unk02();
		tr[TRACK_UNK03] = wram_.sound_engine_param3();
		tr.word(TRACK_CHANNEL_PTR) = channel;
		for (int i = 0; i < 7; i++) // CHANNEL_UNK_0A .. CHANNEL_FADE_OUT_ENVELOPE
			tr[TRACK_UNK06 + i] = mem_[static_cast<uint16_t>(channel + CHANNEL_UNK_0A + i)];
		tr[TRACK_UNK0D] = 0;
		tr[TRACK_UNK0E] = wram_.sound_engine_param4();
	}
	// the note's arguments are all read: cmd stays where it was
	return true;
}

// Func_30b4d: ends the channel's note (the pitch given, else its own) on the
// track that plays it
bool Engine::cmd_stop_note(uint16_t &cmd)
{
	Channel ch = cur_channel();
	uint8_t arg = wram_.audio_cmd_arg();
	if (!(arg & 0x80) && arg >= 0x24) {
		cmd++;
		ch[CHANNEL_PITCH] = arg;
	}
	uint8_t note = static_cast<uint8_t>(ch[CHANNEL_PITCH] + ch[CHANNEL_SEMITONE_OFFSET]);
	wram_.sound_engine_param3() = note;

	set_all_tracks();
	uint8_t owner = wram_.num_audio_channels();
	if (wram_.audio_engine_flags().bit(AUDIOENG_HANDLING_SFX_F))
		owner |= 0x80;
	for (;;) {
		Track tr = cur_track();
		if ((tr.flags() & TRACKFLAGS_5) && tr[TRACK_UNK02] == owner && tr[TRACK_UNK03] == note &&
			!tr[TRACK_DURATION]) {
			end_track_note(tr);
			break;
		}
		if (!next_track())
			break;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Tracks

// Func_30bb0 / Func_30bc2: counts down the note duration of the tracks the
// SFX (or the music) own, and releases those whose channel stopped.
void Engine::update_track_duration(bool sfx)
{
	Track tr = cur_track();
	if (!(tr.flags() & TRACKFLAGS_7))
		return;
	if (static_cast<bool>(tr[TRACK_UNK02] & 0x80) != sfx)
		return;
	if (mem_[static_cast<uint16_t>(tr.word(TRACK_CHANNEL_PTR))] < CHANNEL_PLAYING) {
		release_track(tr);
		return;
	}
	uint8_t duration = tr[TRACK_DURATION];
	if (!duration)
		return;
	tr[TRACK_DURATION] = static_cast<uint8_t>(duration - 1);
	if (duration == 1)
		end_track_note(tr);
}

void Engine::end_track_note(Track tr)
{
	uint8_t flags = tr.flags();
	if (flags & TRACKFLAGS_6) {
		release_track(tr);
		return;
	}
	tr.flags() = static_cast<uint8_t>((flags | TRACKFLAGS_4 | TRACKFLAGS_6) & ~TRACKFLAGS_5);
}

void Engine::release_track(Track tr)
{
	tr.flags() = 0;
	turn_sound_register_off();
}

// Func_30dc6
void Engine::set_track_volume(Track tr)
{
	uint8_t volume = multiply_short(cur_channel()[CHANNEL_UNK_2F], tr[TRACK_UNK06]);
	tr[TRACK_VOLUME] = static_cast<uint8_t>((volume + 0x0f) & 0xf0);
}

// Func_30deb
uint8_t Engine::set_track_fade_out_volume(Track tr)
{
	uint8_t fade = static_cast<uint8_t>(multiply_short(tr[TRACK_FADE_OUT_ENVELOPE], tr[TRACK_UNK06]) + 0x0f);
	uint8_t volume = static_cast<uint8_t>((multiply_short(cur_channel()[CHANNEL_UNK_2F], fade) + 0x0f) & 0xf0);
	tr[TRACK_VOLUME] = volume;
	return volume;
}

// Func_30e1b: the wave channel's output level is the envelope's top two
// bits: when they change, its volume needs updating.
uint8_t Engine::check_sound3_envelope(uint8_t envelope, uint8_t old)
{
	if (wram_.cur_sound_register() == SOUND3 && ((envelope ^ old) & 0xc0))
		wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_VOLUME;
	return envelope;
}

// Func_30bfb: steps the track's volume envelope (fading in, holding, fading
// out, by the TRACKFLAGS_4..6 state) and sends the frequency, output and
// volume to the sound hardware as needed. The steps are named after the
// asm's labels.
void Engine::update_track_output()
{
	Track tr = cur_track();
	uint8_t flags = tr.flags();
	if (!(flags & TRACKFLAGS_7))
		return;
	uint16_t channel = tr.word(TRACK_CHANNEL_PTR);
	wram_.cur_channel_ptr().word() = channel;
	uint8_t channel_flags = mem_[channel];
	if (channel_flags < CHANNEL_PLAYING) {
		release_track(tr);
		return;
	}
	wram_.cur_channel_flags() = channel_flags;

	constexpr int ENVELOPE = TRACK_VOLUME_ENVELOPE;
	constexpr int STEP = TRACK_VOLUME_ENVELOPE + 1; // the envelope's step counter
	enum class Step { L30c95, L30ce9, L30d22, L30d70, L30cba, L30d3b, L30d77, L30d84, L30d95, L30d9d } step;

	if (!(flags & TRACKFLAGS_6)) {
		if (!wram_.unk_3d01a())
			++tr[TRACK_FREQUENCY];
		++tr[TRACK_FREQUENCY];
		if (flags & TRACKFLAGS_5)
			step = (flags & TRACKFLAGS_4) ? Step::L30c95 : Step::L30ce9;
		else
			step = (flags & TRACKFLAGS_4) ? Step::L30d22 : Step::L30d70;
	} else {
		tr.flags().reset_bit(TRACKFLAGS_6_F);
		if (tr.flags() & TRACKFLAGS_5) {
			// a new note: fade in
			set_timbre();
			wram_.cur_channel_flags() |= CHANNEL_UPDATES;
			set_track_volume(tr);
			uint8_t rate = rrca(static_cast<uint8_t>(~swap(tr[TRACK_FADE_IN_ENVELOPE]))) & 0x07;
			if (!rate) {
				step = Step::L30cba;
			} else {
				tr[ENVELOPE] = rate | 0x08;
				tr[STEP] = 0;
				step = Step::L30d9d;
			}
		} else {
			// fade out
			uint8_t fade_out = tr[TRACK_FADE_OUT_ENVELOPE];
			uint8_t level = tr[ENVELOPE] & 0xf0;
			uint8_t rate = rrca(static_cast<uint8_t>(~fade_out)) & 0x07;
			if (!level || !rate) {
				step = Step::L30d3b;
			} else {
				tr[ENVELOPE] = rate | level;
				tr[STEP] = 0;
				step = Step::L30d95;
			}
		}
	}

	for (;;) {
		switch (step) {
		case Step::L30c95: {
			if (wram_.cur_channel_flags() & CHANFLAGS_UPDATE_VOLUME)
				set_track_volume(tr);
			if (static_cast<uint8_t>((tr[ENVELOPE] & 0x07) - 1) < tr[STEP]) {
				tr[STEP] = 0;
				unsigned level = tr[ENVELOPE] + 0x10u;
				if (level > 0xff) {
					step = Step::L30cba;
					break;
				}
				tr[ENVELOPE] = check_sound3_envelope(static_cast<uint8_t>(level), tr[ENVELOPE]);
			}
			step = tr[ENVELOPE] < tr[TRACK_VOLUME] ? Step::L30d84 : Step::L30cba;
			break;
		}

		case Step::L30cba: {
			uint8_t fade_in = tr[TRACK_FADE_IN_ENVELOPE];
			uint8_t volume = tr[TRACK_VOLUME];
			tr.flags().reset_bit(TRACKFLAGS_4_F);
			uint8_t faded = set_track_fade_out_volume(tr);
			uint8_t rate = rrca(static_cast<uint8_t>(~fade_in)) & 0x07;
			uint8_t envelope;
			if (rate) {
				envelope = static_cast<uint8_t>(rate | volume);
			} else {
				if (!(tr[TRACK_FADE_OUT_ENVELOPE] & 0xf0)) {
					step = Step::L30d3b;
					break;
				}
				envelope = faded;
			}
			tr[ENVELOPE] = envelope;
			tr[STEP] = 0;
			step = Step::L30d95;
			break;
		}

		case Step::L30ce9: {
			if (wram_.cur_channel_flags() & CHANFLAGS_UPDATE_VOLUME)
				set_track_fade_out_volume(tr);
			uint8_t rate = tr[ENVELOPE] & 0x07;
			if (!rate) {
				if (tr[ENVELOPE] == tr[TRACK_VOLUME]) {
					step = Step::L30d9d;
					break;
				}
				tr[ENVELOPE] = tr[TRACK_VOLUME];
				step = Step::L30d95;
				break;
			}
			bool underflow = false;
			if (static_cast<uint8_t>(rate - 1) < tr[STEP]) {
				tr[STEP] = 0;
				int level = tr[ENVELOPE] - 0x10;
				if (level < 0)
					underflow = true;
				else
					tr[ENVELOPE] = check_sound3_envelope(static_cast<uint8_t>(level), tr[ENVELOPE]);
			}
			if (!underflow && static_cast<uint8_t>(tr[TRACK_VOLUME] | 0x0f) < tr[ENVELOPE]) {
				step = Step::L30d84;
				break;
			}
			if (!(tr[TRACK_FADE_OUT_ENVELOPE] & 0xf0)) {
				step = Step::L30d3b;
				break;
			}
			tr[ENVELOPE] = tr[TRACK_VOLUME];
			step = Step::L30d95;
			break;
		}

		case Step::L30d22: {
			if (!(static_cast<uint8_t>((tr[ENVELOPE] & 0x07) - 1) < tr[STEP])) {
				step = Step::L30d84;
				break;
			}
			tr[STEP] = 0;
			int level = tr[ENVELOPE] - 0x10;
			if (level < 0) {
				step = Step::L30d3b;
				break;
			}
			tr[ENVELOPE] = check_sound3_envelope(static_cast<uint8_t>(level), tr[ENVELOPE]);
			step = Step::L30d84;
			break;
		}

		case Step::L30d3b: {
			Channel ch = cur_channel();
			uint8_t volume = ch[CHANNEL_UNK_2F], level = ch[CHANNEL_UNK_27], rate = ch[CHANNEL_UNK_28];
			if (!volume || !level || !rate) {
				step = Step::L30d77;
				break;
			}
			tr[ENVELOPE] = static_cast<uint8_t>((multiply_short(volume, level) + 0x0f) & 0xf0);
			tr[STEP] = rate;
			tr.flags() &= ~(TRACKFLAGS_4 | TRACKFLAGS_5 | TRACKFLAGS_6);
			step = Step::L30d95;
			break;
		}

		case Step::L30d70: {
			uint8_t left = static_cast<uint8_t>(tr[TRACK_FREQUENCY] - 1);
			tr[TRACK_FREQUENCY] = left;
			step = left ? Step::L30d9d : Step::L30d77;
			break;
		}

		case Step::L30d77:
			cur_track().flags() = 0;
			turn_sound_register_off();
			return;

		case Step::L30d84:
			if (wram_.cur_sound_register() != SOUND3)
				wram_.cur_channel_flags() &= ~CHANFLAGS_UPDATE_VOLUME;
			step = Step::L30d9d;
			break;

		case Step::L30d95:
			wram_.cur_channel_flags() |= CHANFLAGS_UPDATE_VOLUME;
			step = Step::L30d9d;
			break;

		case Step::L30d9d:
			if (wram_.cur_channel_flags() & CHANFLAGS_UPDATE_FREQUENCY)
				set_frequency();
			if (wram_.cur_channel_flags() & CHANFLAGS_UPDATE_SO)
				set_sound_output();
			if (wram_.cur_channel_flags() & CHANFLAGS_UPDATE_VOLUME)
				set_volume_envelope();
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// The sound hardware

void Engine::set_timbre()
{
	Track tr = cur_track();
	uint8_t reg = wram_.cur_sound_register();
	uint8_t timbre = tr[TRACK_TIMBRE];
	uint8_t length = tr[TRACK_LENGTH];

	if (reg < SOUND3) {
		// a square wave: duty cycle, length and sweep
		uint8_t duty = rrca(rrca(timbre)) & AUD1LEN_DUTY;
		uint8_t length_duty = duty, high = 0;
		if (length) {
			length_duty = static_cast<uint8_t>((static_cast<uint8_t>(-length) & AUD1LEN_TIMER) | duty);
			high = AUD1HIGH_LENGTH_ON;
		}
		mem_[io(static_cast<uint8_t>(reg + 2))] = high;
		mem_[io(static_cast<uint8_t>(reg - 1))] = length_duty;
		mem_[io(static_cast<uint8_t>(reg - 2))] = tr[TRACK_SWEEP];
		return;
	}
	if (reg == SOUND3) {
		// the wave channel: length, and its wave samples if they changed
		uint8_t sample = static_cast<uint8_t>(timbre - WAVEFORM_SINE);
		uint8_t high = 0;
		if (length) {
			mem_[rAUD3LEN] = static_cast<uint8_t>(-length);
			high = 0x40;
		}
		mem_[rAUD3HIGH] = high;
		if (wram_.cur_wave_sample() == sample)
			return;
		wram_.cur_wave_sample() = sample;
		uint16_t data = static_cast<uint16_t>(ADDR_WaveSamples + sample * 16);
		for (int i = 0; i < AUD3WAVE_SIZE; i++)
			mem_[static_cast<uint16_t>(_AUD3WAVERAM + i)] = mem_[static_cast<uint16_t>(data + i)];
		return;
	}
	// noise: the LFSR width, and length
	uint8_t width = rlca(rlca(rlca(timbre))) & 0x08;
	uint16_t poly = io(static_cast<uint8_t>(reg + 1));
	mem_[poly] = static_cast<uint8_t>((mem_[poly] & ~AUD4POLY_7STEP) | width);
	uint8_t go = 0;
	if (length) {
		mem_[rAUD4LEN] = static_cast<uint8_t>(-length);
		go = AUD4GO_LENGTH_ON;
	}
	mem_[rAUD4GO] = static_cast<uint8_t>(go | AUD4GO_RESTART);
}

void Engine::set_frequency()
{
	uint16_t frequency = cur_channel().word(CHANNEL_FREQUENCY);
	Track tr = cur_track();
	uint8_t note = static_cast<uint8_t>(tr[TRACK_UNK0E] + (frequency >> 8));
	uint8_t reg = wram_.cur_sound_register();

	if (reg > SOUND3) {
		// noise: the note sets the clock shift and divider
		uint16_t poly = io(static_cast<uint8_t>(reg + 1));
		uint8_t value = static_cast<uint8_t>(~static_cast<uint8_t>(note - 0x0d));
		uint8_t clock = value & 0x03;
		if (value >= 0xc0)
			clock = static_cast<uint8_t>(rlca(rlca(value & (AUD4POLY_SHIFT >> 2))) | clock | (4 & AUD4POLY_DIV));
		mem_[poly] = static_cast<uint8_t>((mem_[poly] & AUD4POLY_7STEP) | clock);
		uint16_t go = io(static_cast<uint8_t>(reg + 2));
		mem_[go] = static_cast<uint8_t>(mem_[go] | AUD4GO_RESTART);
		return;
	}
	if (reg == SOUND3)
		note = static_cast<uint8_t>(note + C_1 - C_0); // the wave channel plays an octave higher

	// GetNoteFrequency: the note's frequency, interpolated towards the next
	// one by the fine part of the channel's frequency
	uint8_t index = note < C_0 ? 0 : static_cast<uint8_t>(note - C_0);
	if (index >= (B_9 + 1) - C_0)
		index = B_9 - C_0;
	uint16_t entry = static_cast<uint16_t>(ADDR_NoteFrequencies + index * 3);
	uint16_t base = mem_.word(entry);
	uint8_t step = mem_[static_cast<uint16_t>(entry + 2)];
	uint16_t value = static_cast<uint16_t>(((multiply(step, static_cast<uint8_t>(frequency)) + 0x00ff) >> 8) + base);

	mem_[io(static_cast<uint8_t>(reg + 1))] = static_cast<uint8_t>(value);
	uint16_t high_reg = io(static_cast<uint8_t>(reg + 2));
	bool length_on = mem_[high_reg] & AUD1HIGH_LENGTH_ON;
	uint8_t high = static_cast<uint8_t>((length_on ? AUD1HIGH_LENGTH_ON : 0) | value >> 8);
	mem_[high_reg] = high;
	if (!length_on) {
		uint16_t length_reg = io(static_cast<uint8_t>(reg - 1));
		mem_[length_reg] = static_cast<uint8_t>(mem_[length_reg] & 0xc0);
	}
	tr[TRACK_FREQUENCY + 1] = static_cast<uint8_t>(high | AUD1HIGH_RESTART);
}

void Engine::set_sound_output()
{
	uint8_t so_flags = cur_channel()[CHANNEL_SO_FLAGS];
	uint8_t reg = wram_.cur_sound_register();
	uint8_t mask;
	if (reg < SOUND2)
		mask = AUDTERM_1_RIGHT | AUDTERM_1_LEFT;
	else if (reg == SOUND2)
		mask = AUDTERM_2_RIGHT | AUDTERM_2_LEFT;
	else if (reg < SOUND4)
		mask = AUDTERM_3_RIGHT | AUDTERM_3_LEFT;
	else
		mask = AUDTERM_4_RIGHT | AUDTERM_4_LEFT;
	uint8_t output = mask;
	if ((so_flags & 0xc0) == 0x40)
		output &= SO_RIGHT;
	else if ((so_flags & 0xc0) == 0x80)
		output &= SO_LEFT;
	mem_[rAUDTERM] = static_cast<uint8_t>((mem_[rAUDTERM] & ~mask) | output);
}

void Engine::set_volume_envelope()
{
	Track tr = cur_track();
	uint8_t envelope = tr[TRACK_VOLUME_ENVELOPE];
	uint8_t reg = wram_.cur_sound_register();
	if (reg < SOUND3) {
		mem_[io(reg)] = envelope;
		mem_[io(static_cast<uint8_t>(reg + 2))] = tr[TRACK_FREQUENCY + 1];
		return;
	}
	if (reg == SOUND3) {
		mem_[io(reg)] = rrca(static_cast<uint8_t>((envelope - 0x40) ^ 0xc0)); // output level
		if (mem_[rAUD3ENA] & 0x80)
			return; // already on
		mem_[rAUD3ENA] = AUD3ENA_ON;
		mem_[rAUD3HIGH] = tr[TRACK_FREQUENCY + 1];
		return;
	}
	mem_[io(reg)] = envelope;
	mem_[rAUD4GO] = static_cast<uint8_t>(mem_[rAUD4GO] | AUD4GO_RESTART);
}

void Engine::turn_sound_register_off()
{
	uint8_t reg = wram_.cur_sound_register();
	if (reg == SOUND3) {
		mem_[rAUD3ENA] = AUD3ENA_OFF;
		return;
	}
	mem_[io(reg)] = 0x08; // envelope up
	mem_[io(static_cast<uint8_t>(reg + 2))] = 0x80; // restart
}

} // namespace wl3::audio
