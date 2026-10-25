// Port of src/audio/engine.asm
//
// The sound engine. Every routine is written as a static function with
// "call" semantics (it runs until the asm routine's final ret), mirroring the
// asm register by register so callers see exactly the same machine state.
// The PORT() entries at the bottom hook each global label.
//
// The audio command interpreter is a loop: DoAudioCommand reads a command and
// jumps to its handler through a pointer table, and most handlers jump back
// to DoAudioCommand. Here the handlers return NEXT_CMD for that, and the loop
// lives in do_audio_command(), so a long command chain doesn't recurse.
//
// The top-level entry points end in `jp ReturnFromAudioJump` (or
// `jp SwitchBackFromAudioBank`); their bodies return at that jump and the
// PORT_JUMP entries perform it.
#include "ported.h"


#define R (gb->cpu)
#define HL REG_HL(gb)
#define BC REG_BC(gb)
#define DE REG_DE(gb)
#define ZF (R.f & FLAG_Z)
#define CF (R.f & FLAG_C)

// Q.6 fixed point values used by the engine
#define Q6_1_0 0x40   // 1.0
#define Q6_0_063 0x04 // 0.063
#define Q6_256_0_HI 0x40 // HIGH(256.0)
#define Q6_255_99 0x3fff // 255.99

enum { DONE = 0, NEXT_CMD = 1 };

// ---------------------------------------------------------------------------
// Instruction helpers

static inline uint8_t ld_hli(GB *gb)
{
	uint16_t hl = HL;
	SET_HL(gb, hl + 1);
	return rd8(gb, hl);
}
static inline uint8_t ld_hld(GB *gb)
{
	uint16_t hl = HL;
	SET_HL(gb, hl - 1);
	return rd8(gb, hl);
}
static inline void st_hli(GB *gb, uint8_t v)
{
	uint16_t hl = HL;
	wr8(gb, hl, v);
	SET_HL(gb, hl + 1);
}
static inline void st_hld(GB *gb, uint8_t v)
{
	uint16_t hl = HL;
	wr8(gb, hl, v);
	SET_HL(gb, hl - 1);
}
static inline void inc_hl(GB *gb) { SET_HL(gb, HL + 1); }
static inline void dec_hl(GB *gb) { SET_HL(gb, HL - 1); }
static inline void inc_de(GB *gb) { SET_DE(gb, DE + 1); }
static inline void dec_de(GB *gb) { SET_DE(gb, DE - 1); }
static inline void inc_bc(GB *gb) { SET_BC(gb, BC + 1); }
// add hl, rr
static inline void add_hl(GB *gb, uint16_t v) { SET_HL(gb, alu_add16(gb, HL, v)); }
// ld a, [addr] / ld l, a / ld a, [addr + 1] / ld h, a
static inline void ld_hl_from(GB *gb, uint16_t addr)
{
	R.a = rd8(gb, addr);
	R.l = R.a;
	R.a = rd8(gb, addr + 1);
	R.h = R.a;
}
// ld a, [$ff00+c] / ld [$ff00+c], a
static inline uint8_t ldh_c_rd(GB *gb) { return rd8(gb, 0xff00 | R.c); }
static inline void ldh_c_wr(GB *gb, uint8_t v) { wr8(gb, 0xff00 | R.c, v); }

// rotates and shifts
static inline uint8_t f_rra(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | (CF ? 0x80 : 0));
	R.f = (v & 1) ? FLAG_C : 0;
	return r;
}
static inline uint8_t f_rrca(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | v << 7);
	R.f = (v & 1) ? FLAG_C : 0;
	return r;
}
static inline uint8_t f_rlca(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v << 1 | v >> 7);
	R.f = (v & 0x80) ? FLAG_C : 0;
	return r;
}
static inline uint8_t f_sla(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v << 1);
	R.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
	return r;
}
static inline uint8_t f_rr(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | (CF ? 0x80 : 0));
	R.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
	return r;
}
static inline uint8_t f_rl(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v << 1 | (CF ? 1 : 0));
	R.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
	return r;
}
static inline uint8_t f_rrc(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | v << 7);
	R.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
	return r;
}
static inline void f_cpl(GB *gb)
{
	R.a = (uint8_t)~R.a;
	R.f |= FLAG_N | FLAG_H;
}
static inline void f_ccf(GB *gb)
{
	R.f = (uint8_t)((R.f & FLAG_Z) | ((R.f & FLAG_C) ^ FLAG_C));
}
// xor a
static inline void xor_a(GB *gb)
{
	R.a = 0;
	R.f = FLAG_Z;
}
// and a
static inline void and_a(GB *gb) { R.a = alu_and(gb, R.a, R.a); }
// dec [hl] / inc [hl]
static inline void dec_mhl(GB *gb) { wr8(gb, HL, alu_dec(gb, rd8(gb, HL))); }
static inline void inc_mhl(GB *gb) { wr8(gb, HL, alu_inc(gb, rd8(gb, HL))); }

// ROM0 helpers
static void read_cmds_1byte(GB *gb) { CALL(ReadAudioCommands_1Byte); }
static void read_cmds_2bytes(GB *gb) { CALL(ReadAudioCommands_2Bytes); }

// ---------------------------------------------------------------------------
// Forward declarations

static void e_UpdateMusicFadeOut(GB *gb);
static void e_SetSFXChannels(GB *gb);
static void e_SetMusicChannels(GB *gb);
static void e_SetAudioChannels(GB *gb);
static void e_NextChannel(GB *gb);
static void e_SetTrack(GB *gb);
static void e_SetAllTracks(GB *gb);
static void e_NextTrack(GB *gb);
static bool e_GetSound(GB *gb);
static void e_LoadChannelWithSound(GB *gb);
static void e_SkipLoadingChannel(GB *gb);
static void e_InitChannel(GB *gb);
static void e_TurnSFXOff(GB *gb);
static void e__TurnMusicOff(GB *gb);
static void e_DeactivateMusicChannels(GB *gb);
static void e_DeactivateMusicChannels_FromFadeOut(GB *gb);
static void e_PlayMusic(GB *gb);
static void e_Func_302e4(GB *gb);
static void e_ExecuteAudioMod_Internal(GB *gb);
static void e_PrepareChannelBitmasks(GB *gb);
static void e_AddChannelStructLengthToHL(GB *gb);
static void e_Func_30547(GB *gb);
static void e_UpdateChannel(GB *gb);
static void e_DoAudioCommand(GB *gb);
static int jump_to_pointer_in_table(GB *gb);
static void e_UpdateVibrato(GB *gb);
static void e_Func_30651(GB *gb);
static void e_CalculateTempo(GB *gb);
static void e_GetWave(GB *gb);
static void e_Func_30bb0(GB *gb);
static void e_Func_30bc2(GB *gb);
static void e_Func_30bd2(GB *gb);
static void e_Func_30be3(GB *gb);
static void e_Func_30bf2(GB *gb);
static void e_Func_30bf6(GB *gb);
static void e_Func_30bfb(GB *gb);
static void e_SetTimbre(GB *gb);
static void e_SetFrequency(GB *gb);
static void e_SetSoundOutput(GB *gb);
static void e_SetVolumeEnvelope(GB *gb);
static void e_TurnSoundRegisterOff(GB *gb);
static void e_GetNoteFrequency(GB *gb);
static void e_MultiplyBByC(GB *gb);
static void e_MultiplyBByC_Short(GB *gb);

// ---------------------------------------------------------------------------
// Init / update

// ends at jp SwitchBackFromAudioBank
static void e__InitAudio(GB *gb)
{
	R.a = 0xff;
	wr8(gb, wAudioEngineFlags, R.a);

	R.a = rd8(gb, wAudioBankBackup);
	push_af(gb);
	SET_HL(gb, wAudioBankBackup);
	R.b = (uint8_t)(wStartChannel + 1 - wAudioBankBackup);
	xor_a(gb);
	do {
		st_hli(gb, R.a);
		R.b = alu_dec(gb, R.b);
	} while (!ZF);
	pop_af(gb);
	wr8(gb, wAudioBankBackup, R.a);

	R.a = 74;
	wr8(gb, wSFXBaseTempo, R.a);
	wr8(gb, wSFXTempo, R.a);
	wr8(gb, wMusicBaseTempo, R.a);
	wr8(gb, wMusicTempo, R.a);
	R.a = Q6_1_0;
	wr8(gb, wSFXTempoMult, R.a);
	wr8(gb, wMusicTempoMult, R.a);

	R.a = 0xff;
	wr8(gb, wCurWaveSample, R.a);

	xor_a(gb);
	R.b = NUM_CHANNELS;
	SET_DE(gb, CHANNEL_STRUCT_LENGTH);
	SET_HL(gb, wChannels);
	do {
		wr8(gb, HL, R.a);
		add_hl(gb, DE);
		R.b = alu_dec(gb, R.b);
	} while (!ZF);

	xor_a(gb);
	R.b = NUM_TRACKS;
	SET_DE(gb, TRACK_STRUCT_LENGTH);
	SET_HL(gb, wTracks);
	do {
		wr8(gb, HL, R.a);
		add_hl(gb, DE);
		R.b = alu_dec(gb, R.b);
	} while (!ZF);

	R.a = AUDENA_ON;
	wr8(gb, rAUDENA, R.a);
	R.a = 0x00;
	wr8(gb, rAUDTERM, R.a);
	R.a = AUD1ENV_UP;
	wr8(gb, rAUD1ENV, R.a);
	wr8(gb, rAUD2ENV, R.a);
	wr8(gb, rAUD4ENV, R.a);
	R.a = AUD1HIGH_RESTART;
	wr8(gb, rAUD1HIGH, R.a);
	wr8(gb, rAUD2HIGH, R.a);
	wr8(gb, rAUD4GO, R.a);
	R.a = AUD3ENA_OFF;
	wr8(gb, rAUD3ENA, R.a);
	R.a = 0xff; // all channels in both speakers
	wr8(gb, rAUDTERM, R.a);
	R.a = MAX_VOLUME;
	wr8(gb, rAUDVOL, R.a);
	R.a = 0x00;
	wr8(gb, wAudioEngineFlags, R.a);
}

// Adds wSFXTempo / wMusicTempo to its accumulator (at hl) and checks
// whether it reached $4a: returns with carry set if not, and with a, b and hl
// as the asm leaves them.
static void tempo_accumulate(GB *gb, uint16_t tempo)
{
	SET_HL(gb, tempo);
	R.a = ld_hli(gb);
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	st_hli(gb, R.a);
	R.a = 0;
	R.a = alu_adc(gb, R.a, rd8(gb, HL));
	st_hld(gb, R.a);
	R.a = ld_hli(gb);
	R.a = alu_sub(gb, R.a, 0x4a);
	R.b = R.a;
	R.a = rd8(gb, HL);
	R.a = alu_sbc(gb, R.a, 0);
}

static void tempo_recheck(GB *gb, uint16_t acc)
{
	SET_HL(gb, acc);
	R.a = ld_hli(gb);
	R.a = alu_sub(gb, R.a, 0x4a);
	R.b = R.a;
	R.a = rd8(gb, HL);
	R.a = alu_sbc(gb, R.a, 0);
}

// ends at jp ReturnFromAudioJump
static void e__UpdateAudio(GB *gb)
{
	gb_push16(gb, BC);
	gb_push16(gb, DE);
	e_UpdateMusicFadeOut(gb);

	SET_HL(gb, wAudioEngineFlags);
	wr8(gb, HL, rd8(gb, HL) | AUDIOENG_HANDLING_SFX);

	tempo_accumulate(gb, wSFXTempo);
	if (CF) {
		e_SetSFXChannels(gb);
		do {
			e_Func_30547(gb);
			e_NextChannel(gb);
		} while (!ZF);
	} else {
		do {
			st_hld(gb, R.a);
			wr8(gb, HL, R.b);

			e_SetAllTracks(gb);
			do {
				e_Func_30bb0(gb);
				e_NextTrack(gb);
			} while (!ZF);

			e_SetSFXChannels(gb);
			do {
				e_UpdateChannel(gb);
				e_NextChannel(gb);
			} while (!ZF);

			tempo_recheck(gb, w3d008);
		} while (!CF);
	}

	SET_HL(gb, wAudioEngineFlags);
	wr8(gb, HL, rd8(gb, HL) & ~AUDIOENG_HANDLING_SFX);

	tempo_accumulate(gb, wMusicTempo);
	if (CF) {
		e_SetMusicChannels(gb);
		do {
			e_Func_30547(gb);
			e_NextChannel(gb);
		} while (!ZF);
	} else {
		do {
			st_hld(gb, R.a);
			wr8(gb, HL, R.b);

			e_SetAllTracks(gb);
			do {
				e_Func_30bc2(gb);
				e_NextTrack(gb);
			} while (!ZF);

			e_SetMusicChannels(gb);
			do {
				e_UpdateChannel(gb);
				e_NextChannel(gb);
			} while (!ZF);

			tempo_recheck(gb, w3d00d);
		} while (!CF);
	}

	e_SetAllTracks(gb);
	do {
		e_Func_30bfb(gb);
		e_NextTrack(gb);
	} while (!ZF);

	R.a = rd8(gb, w3d01a);
	and_a(gb);
	if (ZF)
		R.a = 0x0f;
	R.a = alu_dec(gb, R.a);
	wr8(gb, w3d01a, R.a);

	// get active channels
	R.c = NUM_CHANNELS;
	SET_DE(gb, CHANNEL_STRUCT_LENGTH);
	SET_HL(gb, wChannels);
	do {
		R.a = rd8(gb, HL);
		R.a = alu_and(gb, R.a, 0xff ^ (CHANFLAGS_UPDATE_SO | CHANFLAGS_UPDATE_VOLUME | CHANFLAGS_UPDATE_FREQUENCY));
		wr8(gb, HL, R.a);
		R.a = alu_rla(gb, R.a);
		R.b = f_rr(gb, R.b);
		add_hl(gb, DE);
		R.c = alu_dec(gb, R.c);
	} while (!ZF);
	R.a = R.b;
	wr8(gb, wActiveChannels, R.a);
	SET_DE(gb, gb_pop16(gb));
	SET_BC(gb, gb_pop16(gb));
}

// ---------------------------------------------------------------------------
// Channel and track iteration

static void e_SetSFXChannels(GB *gb)
{
	SET_DE(gb, wChannels);
	R.a = NUM_SFX_CHANNELS;
	e_SetAudioChannels(gb);
}

static void e_SetMusicChannels(GB *gb)
{
	SET_DE(gb, wMusicChannels);
	R.a = NUM_MUSIC_CHANNELS;
	e_SetAudioChannels(gb);
}

static void e_SetAudioChannels(GB *gb)
{
	SET_HL(gb, wNumAudioChannels);
	st_hli(gb, R.a);
	R.a = R.e;
	st_hli(gb, R.a);
	wr8(gb, HL, R.d);
}

static void e_NextChannel(GB *gb)
{
	SET_HL(gb, wNumAudioChannels);
	dec_mhl(gb);
	if (ZF)
		return;
	inc_hl(gb);
	R.a = CHANNEL_STRUCT_LENGTH;
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	st_hli(gb, R.a);
	R.a = 0;
	R.a = alu_adc(gb, R.a, rd8(gb, HL));
	wr8(gb, HL, R.a);
	R.a = f_rra(gb, R.a);
}

static void set_track_n(GB *gb, uint16_t track, uint8_t reg)
{
	SET_DE(gb, track);
	R.a = 1;
	R.c = reg;
	e_SetTrack(gb);
}
static void e_SetTrack1(GB *gb) { set_track_n(gb, wTrack1, (uint8_t)rAUD1ENV); }
static void e_SetTrack2(GB *gb) { set_track_n(gb, wTrack2, (uint8_t)rAUD2ENV); }
static void e_SetTrack3(GB *gb) { set_track_n(gb, wTrack3, (uint8_t)rAUD3LEVEL); }
static void e_SetTrack4(GB *gb) { set_track_n(gb, wTrack4, (uint8_t)rAUD4ENV); }

static void e_SetAllTracks(GB *gb)
{
	SET_DE(gb, wTrack1);
	R.a = NUM_TRACKS;
	R.c = (uint8_t)rAUD1ENV;
	e_SetTrack(gb);
}

static void e_SetTrack(GB *gb)
{
	SET_HL(gb, wNumTracks);
	st_hli(gb, R.a);
	R.a = R.e;
	st_hli(gb, R.a);
	R.a = R.d;
	st_hli(gb, R.a);
	wr8(gb, HL, R.c);
}

static void e_NextTrack(GB *gb)
{
	SET_HL(gb, wNumTracks);
	dec_mhl(gb);
	if (ZF)
		return;
	inc_hl(gb);
	R.a = TRACK_STRUCT_LENGTH;
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	st_hli(gb, R.a);
	R.a = 0;
	R.a = alu_adc(gb, R.a, rd8(gb, HL));
	st_hli(gb, R.a);
	R.a = rd8(gb, HL);
	R.a = alu_add(gb, R.a, 5);
	wr8(gb, HL, R.a);
	R.a = f_rra(gb, R.a);
}

// ---------------------------------------------------------------------------
// Starting and stopping sounds

// The return address `call GetSound` pushes in each caller: GetSound pops
// it into hl when the sound ID is invalid.
#define PLAYSFX_GETSOUND_RET (ADDR__PlaySFX + 8)
#define PLAYMUSIC_GETSOUND_RET (ADDR_PlayMusic + 0x18)

// ends at jp ReturnFromAudioJump
static void e__PlaySFX(GB *gb)
{
	R.a = R.b;
	R.a = alu_or(gb, R.a, R.c);
	if (ZF) {
		e_TurnSFXOff(gb);
		return;
	}
	if (!e_GetSound(gb)) {
		SET_HL(gb, PLAYSFX_GETSOUND_RET);
		return;
	}

	R.a = rd8(gb, wStartChannel);
	if (alu_bit(gb, R.a, 7)) {
		// .asm_30211
		e_SetSFXChannels(gb);
		R.a = 1;
		wr8(gb, wSoundEngineParam4, R.a);
		xor_a(gb);
		wr8(gb, wSoundEngineParam3, R.a);
		do {
			ld_hl_from(gb, wCurChannelPtr);
			if (!alu_bit(gb, rd8(gb, HL), CHANFLAGS_ACTIVE_F)) {
				e_LoadChannelWithSound(gb);
				if (ZF)
					return; // .done_2
				R.a = rd8(gb, wSoundEngineParam4);
				R.b = R.a;
				R.a = rd8(gb, wSoundEngineParam3);
				R.a = alu_or(gb, R.a, R.b);
				wr8(gb, wSoundEngineParam3, R.a);
			}
			// .active_ch
			R.a = rd8(gb, wSoundEngineParam4);
			R.a = f_sla(gb, R.a);
			wr8(gb, wSoundEngineParam4, R.a);
			e_NextChannel(gb);
		} while (!ZF);

		e_SetSFXChannels(gb);
		R.a = 1;
		wr8(gb, wSoundEngineParam4, R.a);
		do {
			R.a = rd8(gb, wSoundEngineParam4);
			R.b = R.a;
			R.a = rd8(gb, wSoundEngineParam3);
			R.a = alu_and(gb, R.a, R.b);
			if (ZF) {
				ld_hl_from(gb, wCurChannelPtr);
				SET_BC(gb, CHANNEL_PRIORITY);
				add_hl(gb, BC);
				R.a = rd8(gb, wSoundPriority);
				alu_cp(gb, R.a, rd8(gb, HL));
				if (!CF) {
					e_LoadChannelWithSound(gb);
					if (ZF)
						return; // .done_2
				}
			}
			// .asm_3026f
			R.a = rd8(gb, wSoundEngineParam4);
			R.a = f_sla(gb, R.a);
			wr8(gb, wSoundEngineParam4, R.a);
			e_NextChannel(gb);
		} while (!ZF);
		return;
	}

	R.b = R.a;
	R.a = alu_sub(gb, R.a, CHAN5_F);
	if (!CF)
		return; // .done_1
	f_cpl(gb);
	R.a = alu_inc(gb, R.a);
	wr8(gb, wNumAudioChannels, R.a);

	SET_HL(gb, wChannels);
	R.a = R.b;
	and_a(gb);
	if (!ZF) {
		SET_DE(gb, CHANNEL_STRUCT_LENGTH);
		do {
			add_hl(gb, DE);
			R.b = alu_dec(gb, R.b);
		} while (!ZF);
	}
	// .got_ch_ptr
	R.a = R.l;
	wr8(gb, wCurChannelPtr + 0, R.a);
	R.a = R.h;
	wr8(gb, wCurChannelPtr + 1, R.a);
	for (bool first = true;; first = false) {
		if (!first) // .loop_1
			ld_hl_from(gb, wCurChannelPtr);
		// .asm_301f1
		bool load = true;
		if (alu_bit(gb, rd8(gb, HL), CHANFLAGS_ACTIVE_F)) {
			SET_BC(gb, CHANNEL_PRIORITY);
			add_hl(gb, BC);
			R.a = rd8(gb, wSoundPriority);
			alu_cp(gb, R.a, rd8(gb, HL));
			load = !CF;
		}
		if (load)
			e_LoadChannelWithSound(gb);
		else
			e_SkipLoadingChannel(gb);
		if (ZF)
			return; // .done_1
		e_NextChannel(gb);
		if (ZF)
			return;
	}
}

// ends at jp ReturnFromAudioJump
static void e_PlayMusic(GB *gb)
{
	R.a = R.b;
	R.a = alu_or(gb, R.a, R.c);
	if (ZF) {
		e__TurnMusicOff(gb);
		return;
	}
	R.a = R.c;
	wr8(gb, wLoadedMusic + 0, R.a);
	R.a = R.b;
	wr8(gb, wLoadedMusic + 1, R.a);
	xor_a(gb);
	wr8(gb, wMusicFadeOutPeriod, R.a);
	xor_a(gb);
	wr8(gb, w3d025, R.a);
	if (!e_GetSound(gb)) {
		SET_HL(gb, PLAYMUSIC_GETSOUND_RET);
		return;
	}

	e_SetMusicChannels(gb);
	for (;;) {
		e_LoadChannelWithSound(gb);
		if (ZF)
			break; // .next_music_ch
		e_NextChannel(gb);
		if (ZF)
			return;
	}
	for (;;) {
		// .next_music_ch
		e_NextChannel(gb);
		if (ZF)
			return;
		// .loop_music_chs
		ld_hl_from(gb, wCurChannelPtr);
		xor_a(gb);
		wr8(gb, HL, R.a);
	}
}

// ends at jp ReturnFromAudioJump
static void e__PlayNewMusic(GB *gb)
{
	R.a = rd8(gb, wLoadedMusic + 0);
	alu_cp(gb, R.a, R.c);
	if (!ZF) {
		e_PlayMusic(gb);
		return;
	}
	R.a = rd8(gb, wLoadedMusic + 1);
	alu_cp(gb, R.a, R.b);
	if (!ZF) {
		e_PlayMusic(gb);
		return;
	}
	R.a = rd8(gb, wActiveChannels);
	R.a = alu_and(gb, R.a, MUSIC_CHANNELS);
	if (ZF)
		e_PlayMusic(gb);
}

// ends at jp ReturnFromAudioJump
static void e__PlayNewMusic_SetNoise(GB *gb)
{
	R.a = rd8(gb, wLoadedMusic + 0);
	alu_cp(gb, R.a, R.c);
	if (!ZF) {
		e_PlayMusic(gb);
		return;
	}
	R.a = rd8(gb, wLoadedMusic + 1);
	alu_cp(gb, R.a, R.b);
	if (!ZF) {
		e_PlayMusic(gb);
		return;
	}
	R.a = rd8(gb, wActiveChannels);
	R.a = alu_and(gb, R.a, MUSIC_CHANNELS);
	if (ZF)
		e_Func_302e4(gb);
}

// ends at jp ReturnFromAudioJump
static void e_Func_302e4(GB *gb)
{
	xor_a(gb);
	wr8(gb, w3d025, R.a);
	e_SetMusicChannels(gb);
	do {
		ld_hl_from(gb, wCurChannelPtr);
		R.a = rd8(gb, HL);
		R.a = alu_and(gb, R.a, CHANFLAGS_5 | CHANFLAGS_6);
		if (!ZF)
			wr8(gb, HL, rd8(gb, HL) | CHANFLAGS_ACTIVE);
		e_NextChannel(gb);
	} while (!ZF);
}

// Returns false where the asm pops its return address and jumps to
// ReturnFromAudioJump (invalid sound ID); hl is then left to the caller.
static bool e_GetSound(GB *gb)
{
	R.a = R.b;
	wr8(gb, wCurSoundID + 1, R.a);
	alu_cp(gb, R.a, NUM_SOUNDS >> 8);
	R.a = R.c;
	if (ZF) {
		alu_cp(gb, R.a, NUM_SOUNDS & 0xff);
		if (!CF)
			return false;
	} else if (!CF) {
		return false;
	}

	wr8(gb, wCurSoundID + 0, R.a);
	R.l = R.c;
	R.h = R.b;
	add_hl(gb, HL);
	add_hl(gb, HL);
	add_hl(gb, HL);
	SET_BC(gb, ADDR_SoundHeaders - 8);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	R.e = R.a;
	R.a = ld_hli(gb);
	R.d = R.a;

	R.a = ld_hli(gb);
	wr8(gb, wSoundBank, R.a);
	inc_hl(gb);
	R.a = ld_hli(gb);
	wr8(gb, wSoundPriority, R.a);
	R.a = ld_hli(gb);
	wr8(gb, wStartChannel, R.a);
	R.a = rd8(gb, HL);
	wr8(gb, wNumChannels, R.a);
	inc_de(gb);
	inc_de(gb);
	R.a = R.e;
	wr8(gb, wAudioCmdPtr + 0, R.a);
	R.a = R.d;
	wr8(gb, wAudioCmdPtr + 1, R.a);
	return true;
}

static void e_LoadChannelWithSound(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	R.a = CHANFLAGS_5 | CHANFLAGS_ACTIVE;
	st_hli(gb, R.a);
	inc_hl(gb);
	R.a = rd8(gb, wAudioCmdPtr + 0);
	st_hli(gb, R.a);
	R.a = rd8(gb, wAudioCmdPtr + 1);
	st_hli(gb, R.a);
	R.a = rd8(gb, wSoundBank);
	st_hli(gb, R.a);
	inc_hl(gb);
	R.a = rd8(gb, wCurSoundID + 0);
	st_hli(gb, R.a);
	R.a = rd8(gb, wCurSoundID + 1);
	st_hli(gb, R.a);
	R.a = rd8(gb, wSoundPriority);
	st_hli(gb, R.a);
	e_SkipLoadingChannel(gb);
}

static void e_SkipLoadingChannel(GB *gb)
{
	SET_HL(gb, wNumChannels);
	dec_mhl(gb);
	if (ZF)
		return;
	SET_HL(gb, wAudioCmdPtr);
	R.a = 2;
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	st_hli(gb, R.a);
	R.a = 0;
	R.a = alu_adc(gb, R.a, rd8(gb, HL));
	wr8(gb, HL, R.a);
	R.a = f_rra(gb, R.a);
}

// hl = channel
static void e_InitChannel(GB *gb)
{
	// values written from CHANNEL_PITCH to CHANNEL_UNK_30
	static const uint8_t init[] = {
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
	R.a = CHANFLAGS_6 | CHANFLAGS_ACTIVE;
	st_hli(gb, R.a);
	xor_a(gb);
	st_hli(gb, R.a);
	R.a = ld_hli(gb);
	R.e = R.a;
	R.a = ld_hli(gb);
	R.d = R.a;
	R.a = ld_hld(gb);
	wr8(gb, wSoundBank, R.a);
	read_cmds_2bytes(gb);
	R.a = R.b;
	st_hld(gb, R.a);
	wr8(gb, HL, R.c);
	SET_BC(gb, CHANNEL_PITCH - CHANNEL_CMD_PTR);
	add_hl(gb, BC);
	for (unsigned i = 0; i < sizeof(init); i++)
		st_hli(gb, init[i]);
	R.a = 0xff;
	R.f = FLAG_Z; // last xor a
}

// ends at jp ReturnFromAudioJump
static void e__TurnOffSFXID(GB *gb)
{
	R.a = R.b;
	R.a = alu_or(gb, R.a, R.c);
	if (ZF) {
		e_TurnSFXOff(gb);
		return;
	}
	R.a = R.c;
	wr8(gb, wCurSoundID + 0, R.a);
	R.a = R.b;
	wr8(gb, wCurSoundID + 1, R.a);
	e_SetSFXChannels(gb);
	do {
		R.a = rd8(gb, wCurChannelPtr + 0);
		R.c = R.a;
		R.a = rd8(gb, wCurChannelPtr + 1);
		R.b = R.a;
		R.a = rd8(gb, BC);
		if (alu_bit(gb, R.a, CHANFLAGS_ACTIVE_F)) {
			SET_HL(gb, CHANNEL_SOUND_ID - CHANNEL_FLAGS);
			add_hl(gb, BC);
			R.a = rd8(gb, wCurSoundID + 0);
			alu_cp(gb, R.a, rd8(gb, HL));
			if (ZF) {
				inc_hl(gb);
				R.a = rd8(gb, wCurSoundID + 1);
				alu_cp(gb, R.a, rd8(gb, HL));
				if (ZF) {
					xor_a(gb);
					wr8(gb, BC, R.a);
				}
			}
		}
		e_NextChannel(gb);
	} while (!ZF);
}

// ends at jp ReturnFromAudioJump
static void e_TurnSFXOff(GB *gb)
{
	e_SetSFXChannels(gb);
	do {
		ld_hl_from(gb, wCurChannelPtr);
		xor_a(gb);
		wr8(gb, HL, R.a);
		e_NextChannel(gb);
	} while (!ZF);
}

// ends at jp ReturnFromAudioJump
static void e__TurnMusicOff(GB *gb)
{
	e_DeactivateMusicChannels(gb);
}

static void e_DeactivateMusicChannels_FromFadeOut(GB *gb)
{
	xor_a(gb);
	wr8(gb, wMusicFadeOutPeriod, R.a);
	e_DeactivateMusicChannels(gb);
}

static void e_DeactivateMusicChannels(GB *gb)
{
	R.a = 0xff;
	wr8(gb, w3d025, R.a);
	e_SetMusicChannels(gb);
	do {
		ld_hl_from(gb, wCurChannelPtr);
		wr8(gb, HL, rd8(gb, HL) & ~CHANFLAGS_ACTIVE);
		e_NextChannel(gb);
	} while (!ZF);
}

// ends at jp ReturnFromAudioJump
static void e_Func_30438(GB *gb)
{
	R.a = rd8(gb, wActiveChannels);
	R.a = alu_and(gb, R.a, SFX_CHANNELS);
	R.d = R.a;
	R.a = rd8(gb, wActiveChannels);
	R.a = alu_and(gb, R.a, MUSIC_CHANNELS);
	R.a = alu_swap(gb, R.a);
	R.e = R.a;
}

#define GETPLAYINGSOUNDID_CHANNEL_POINTERS (ADDR_AudioModFunctionTable - 2 * NUM_MUSIC_CHANNELS)

// ends at jp ReturnFromAudioJump
static void e__GetPlayingSoundID(GB *gb)
{
	and_a(gb);
	if (ZF) {
		R.a = rd8(gb, wActiveChannels);
		R.a = alu_and(gb, R.a, MUSIC_CHANNELS);
		if (!ZF) {
			R.a = rd8(gb, wLoadedMusic + 0);
			R.c = R.a;
			R.a = rd8(gb, wLoadedMusic + 1);
			R.b = R.a;
			return;
		}
		SET_BC(gb, 0);
		return;
	}
	alu_cp(gb, R.a, NUM_SFX_CHANNELS + 1);
	if (!CF) {
		SET_BC(gb, 0);
		return;
	}
	SET_BC(gb, GETPLAYINGSOUNDID_CHANNEL_POINTERS - 2);
	R.a = f_sla(gb, R.a);
	R.a = alu_add(gb, R.a, R.c);
	R.c = R.a;
	R.a = 0;
	R.a = alu_adc(gb, R.a, R.b);
	R.b = R.a;
	R.a = rd8(gb, BC);
	R.l = R.a;
	inc_bc(gb);
	R.a = rd8(gb, BC);
	R.h = R.a;
	SET_BC(gb, 0);
	if (!alu_bit(gb, rd8(gb, HL), CHANFLAGS_ACTIVE_F))
		return;
	SET_BC(gb, CHANNEL_SOUND_ID);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	R.c = R.a;
	R.a = ld_hli(gb);
	R.b = R.a;
}

// ---------------------------------------------------------------------------
// Audio mods

// ends at jp ReturnFromAudioJump
static void e__ExecuteAudioMod(GB *gb)
{
	e_ExecuteAudioMod_Internal(gb);
}

static void e_ExecuteAudioMod_Internal(GB *gb)
{
	alu_cp(gb, R.a, NUM_AUDIOMOD_FUNCTIONS);
	if (!CF)
		return;
	push_af(gb);
	SET_HL(gb, wSoundEngineParams);
	R.a = R.c;
	st_hli(gb, R.a);
	R.a = R.b;
	st_hli(gb, R.a);
	R.a = R.e;
	st_hli(gb, R.a);
	R.a = R.d;
	st_hli(gb, R.a);
	xor_a(gb);
	wr8(gb, HL, R.a);
	pop_af(gb);
	SET_BC(gb, ADDR_AudioModFunctionTable);
	jump_to_pointer_in_table(gb);
}

// bc = channel struct field
static void e_AudioModSetValue_Short(GB *gb)
{
	e_PrepareChannelBitmasks(gb);
	do {
		R.d = f_rrc(gb, R.d);
		if (CF && alu_bit(gb, rd8(gb, HL), CHANFLAGS_ACTIVE_F)) {
			R.a = rd8(gb, wNumChannels);
			R.a = alu_or(gb, R.a, rd8(gb, HL));
			wr8(gb, HL, R.a);
			uint16_t hl = HL;
			add_hl(gb, BC);
			R.a = rd8(gb, wSoundEngineParam2);
			wr8(gb, HL, R.a);
			SET_HL(gb, hl);
		}
		e_AddChannelStructLengthToHL(gb);
	} while (!ZF);
}

static void e_AudioModSetValue_Long(GB *gb)
{
	e_PrepareChannelBitmasks(gb);
	do {
		R.d = f_rrc(gb, R.d);
		if (CF && alu_bit(gb, rd8(gb, HL), CHANFLAGS_ACTIVE_F)) {
			R.a = rd8(gb, wNumChannels);
			R.a = alu_or(gb, R.a, rd8(gb, HL));
			wr8(gb, HL, R.a);
			uint16_t hl = HL;
			add_hl(gb, BC);
			R.a = rd8(gb, wSoundEngineParam1);
			st_hli(gb, R.a);
			R.a = rd8(gb, wSoundEngineParam2);
			st_hld(gb, R.a);
			SET_HL(gb, hl);
		}
		e_AddChannelStructLengthToHL(gb);
	} while (!ZF);
}

static void e_PrepareChannelBitmasks(GB *gb)
{
	R.a = rd8(gb, wSoundEngineParam4);
	R.a = alu_and(gb, R.a, 0x0f);
	R.d = R.a;
	R.a = rd8(gb, wSoundEngineParam3);
	R.a = alu_swap(gb, R.a);
	R.a = alu_and(gb, R.a, 0xf0);
	R.a = alu_or(gb, R.a, R.d);
	R.d = R.a;
	R.e = NUM_CHANNELS;
	SET_HL(gb, wChannels);
}

static void e_AddChannelStructLengthToHL(GB *gb)
{
	R.a = CHANNEL_STRUCT_LENGTH;
	R.a = alu_add(gb, R.a, R.l);
	R.l = R.a;
	R.a = 0;
	R.a = alu_adc(gb, R.a, R.h);
	R.h = R.a;
	R.e = alu_dec(gb, R.e);
}

// ends at jp ReturnFromAudioJump
static void e__SetMusicFadeOut(GB *gb)
{
	wr8(gb, wMusicFadeOutPeriod, R.a);
	wr8(gb, wMusicFadeOutTimer, R.a);
	R.a = Q6_1_0;
	wr8(gb, wMusicFadeOutVolumeMult, R.a);
}

static void e_UpdateMusicFadeOut(GB *gb)
{
	R.a = rd8(gb, w3d025);
	and_a(gb);
	if (!ZF)
		return;
	R.a = rd8(gb, wMusicFadeOutPeriod);
	and_a(gb);
	if (ZF)
		return;
	SET_HL(gb, wMusicFadeOutTimer);
	dec_mhl(gb);
	if (!ZF)
		return;
	st_hli(gb, R.a);
	R.a = rd8(gb, HL);
	R.a = alu_sub(gb, R.a, Q6_0_063);
	if (CF) {
		e_DeactivateMusicChannels_FromFadeOut(gb);
		return;
	}
	wr8(gb, HL, R.a);
	R.b = R.a;
	R.a = AUDIOMOD_VOLUME;
	R.d = 0x00;
	R.e = 0xff;
	e_ExecuteAudioMod_Internal(gb);
}

static void e_AudioMod_Tempo(GB *gb)
{
	R.a = rd8(gb, wSoundEngineParam4);
	and_a(gb);
	if (!ZF) {
		SET_HL(gb, wSFXTempoMult);
		R.a = rd8(gb, wSoundEngineParam2);
		st_hld(gb, R.a);
		e_CalculateTempo(gb);
	}
	R.a = rd8(gb, wSoundEngineParam3);
	and_a(gb);
	if (ZF)
		return;
	SET_HL(gb, wMusicTempoMult);
	R.a = rd8(gb, wSoundEngineParam2);
	st_hld(gb, R.a);
	e_CalculateTempo(gb);
}

static void e_AudioMod_Pitch(GB *gb)
{
	R.a = rd8(gb, wNumChannels);
	R.a |= CHANFLAGS_UPDATE_FREQUENCY;
	wr8(gb, wNumChannels, R.a);
	SET_BC(gb, CHANNEL_PITCH_OFFSET_MOD);
	e_AudioModSetValue_Long(gb);
}

static void e_AudioMod_VibratoSpeed(GB *gb)
{
	R.a = rd8(gb, wSoundEngineParam2);
	R.a = f_sla(gb, R.a);
	if (!ZF) {
		SET_BC(gb, CHANNEL_VIBRATO_SPEED);
		e_AudioModSetValue_Short(gb);
		return;
	}
	R.a = f_rra(gb, R.a);
	wr8(gb, wSoundEngineParam1, R.a);
	R.a = f_rra(gb, R.a);
	R.a = alu_xor(gb, R.a, 0x40);
	wr8(gb, wSoundEngineParam2, R.a);
	SET_BC(gb, CHANNEL_VIBRATO_SPEED);
	e_AudioModSetValue_Long(gb);
}

static void e_AudioMod_Volume(GB *gb)
{
	R.a = rd8(gb, wNumChannels);
	R.a |= CHANFLAGS_UPDATE_VOLUME;
	wr8(gb, wNumChannels, R.a);
	SET_BC(gb, CHANNEL_VOLUME_MOD);
	e_AudioModSetValue_Short(gb);
}

static void e_AudioMod_Pan(GB *gb)
{
	R.a = rd8(gb, wNumChannels);
	R.a |= CHANFLAGS_UPDATE_SO;
	wr8(gb, wNumChannels, R.a);
	SET_BC(gb, CHANNEL_GLOBAL_PAN);
	e_AudioModSetValue_Short(gb);
}

static void e_AudioMod_VibratoAmplitude(GB *gb)
{
	SET_BC(gb, CHANNEL_VIBRATO_AMPLITUDE_MOD);
	e_AudioModSetValue_Short(gb);
}

static void e_Func_309b6(GB *gb)
{
	SET_BC(gb, CHANNEL_UNK_27);
	e_AudioModSetValue_Long(gb);
}

// ---------------------------------------------------------------------------
// Channel update and audio commands

static void e_Func_30547(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	R.a = rd8(gb, HL);
	alu_cp(gb, R.a, CHANFLAGS_6 | CHANFLAGS_ACTIVE);
	if (CF)
		return;
	wr8(gb, wCurChannelFlags, R.a);
	e_Func_30651(gb);
}

static void e_UpdateChannel(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	R.a = rd8(gb, HL);
	if (!alu_bit(gb, R.a, CHANFLAGS_ACTIVE_F))
		return;
	if (!alu_bit(gb, R.a, CHANFLAGS_6_F)) {
		uint16_t hl = HL;
		e_InitChannel(gb);
		SET_HL(gb, hl);
		R.a = rd8(gb, HL);
	}
	wr8(gb, wCurChannelFlags, R.a);
	inc_hl(gb);
	R.a = rd8(gb, HL);
	and_a(gb);
	if (!ZF) {
		dec_mhl(gb);
		inc_hl(gb);
		e_UpdateVibrato(gb);
		return;
	}
	inc_hl(gb);
	R.a = ld_hli(gb);
	R.e = R.a;
	R.a = ld_hli(gb);
	R.d = R.a;
	R.a = ld_hli(gb);
	wr8(gb, wSoundBank, R.a);
	e_DoAudioCommand(gb);
}

static int h_Func_30a14(GB *gb);
static int h_Func_3073f(GB *gb);

// One pass of DoAudioCommand, up to the jump to the command's handler;
// returns what the handler returns.
static int do_audio_command_once(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_ACTIVE_COMMAND);
	add_hl(gb, BC);
	read_cmds_2bytes(gb);
	if (!alu_bit(gb, R.c, 7)) {
		// previous cmd
		R.a = R.c;
		wr8(gb, wAudioCmdArg, R.a);
		dec_de(gb);
		R.a = rd8(gb, HL);
	} else {
		R.a = R.b;
		wr8(gb, wAudioCmdArg, R.a);
		R.a = R.c;
		alu_cp(gb, R.a, 0xbe);
		if (!CF)
			wr8(gb, HL, R.a);
	}
	// .got_cmd
	alu_cp(gb, R.a, 0xd0);
	if (!CF)
		return h_Func_30a14(gb);
	R.a = alu_sub(gb, R.a, 0xb1);
	if (CF)
		return h_Func_3073f(gb);
	SET_BC(gb, ADDR_PointerTable_306d1);
	return jump_to_pointer_in_table(gb);
}

static void e_DoAudioCommand(GB *gb)
{
	while (do_audio_command_once(gb) == NEXT_CMD) {
	}
}

static int h_AudioCmd_ClearFlags(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	xor_a(gb);
	st_hli(gb, R.a);
	return DONE;
}

static void e_UpdateVibrato(GB *gb)
{
	SET_BC(gb, CHANNEL_VIBRATO_COUNTER - CHANNEL_CMD_PTR);
	add_hl(gb, BC);
	R.a = rd8(gb, HL);
	and_a(gb);
	if (!ZF) {
		// still counting down vibrato delay
		dec_mhl(gb);
		SET_BC(gb, (uint16_t)(CHANNEL_UNK_VIBRATO_VALUE - CHANNEL_VIBRATO_COUNTER));
		add_hl(gb, BC);
		R.a = 0x40;
	} else {
		// .counter_done
		SET_BC(gb, (uint16_t)(CHANNEL_VIBRATO_SPEED - CHANNEL_VIBRATO_COUNTER));
		add_hl(gb, BC);
		R.a = ld_hli(gb);
		R.a = alu_add(gb, R.a, rd8(gb, HL));
	}
	// .write_vibrato_value
	wr8(gb, HL, R.a);
	R.a = f_sla(gb, R.a);
	if (CF)
		f_cpl(gb);
	wr8(gb, wVibratoValue, R.a);

	SET_BC(gb, CHANNEL_VIBRATO_AMPLITUDE - CHANNEL_UNK_VIBRATO_VALUE);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	bool vibrato;
	if (!ZF) {
		R.c = R.a;
		R.a = rd8(gb, wVibratoValue);
		R.b = R.a;
		uint16_t hl = HL, bc = BC;
		e_MultiplyBByC(gb);
		SET_BC(gb, bc);
		R.b = R.h;
		SET_HL(gb, hl);
		inc_hl(gb);
		R.a = ld_hli(gb); // CHANNEL_VIBRATO_DISABLED
		alu_cp(gb, R.a, FALSE);
		vibrato = ZF;
		if (vibrato) {
			// .calculate_vibrato: bc = (b - c/2) * 8
			R.a = R.b;
			R.c = alu_srl(gb, R.c);
			R.a = alu_sub(gb, R.a, R.c);
			R.c = R.a;
			R.a = 0;
			R.a = alu_sbc(gb, R.a, R.a);
			R.c = f_sla(gb, R.c);
			R.a = alu_rla(gb, R.a);
			R.c = f_sla(gb, R.c);
			R.a = alu_rla(gb, R.a);
			R.c = f_sla(gb, R.c);
			R.a = alu_rla(gb, R.a);
			R.b = R.a;
		}
	} else {
		// .zero_vibrato_pitch
		SET_BC(gb, 0);
		inc_hl(gb);
		R.a = ld_hli(gb); // CHANNEL_VIBRATO_DISABLED
		alu_cp(gb, R.a, FALSE);
		vibrato = ZF;
	}
	if (vibrato) {
		// .check_change_in_freq_lo
		R.a = R.c;
		alu_cp(gb, R.a, rd8(gb, HL));
		if (!ZF) {
			R.a = rd8(gb, wCurChannelFlags);
			R.a |= CHANFLAGS_UPDATE_FREQUENCY;
			wr8(gb, wCurChannelFlags, R.a);
			wr8(gb, HL, R.c);
		}
		// .check_change_in_freq_hi
		inc_hl(gb);
		R.a = R.b;
		alu_cp(gb, R.a, rd8(gb, HL));
		if (!ZF) {
			R.a = rd8(gb, wCurChannelFlags);
			R.a |= CHANFLAGS_UPDATE_FREQUENCY;
			wr8(gb, wCurChannelFlags, R.a);
			wr8(gb, HL, R.b);
		}
	}
	// .skip_vibrato
	ld_hl_from(gb, wCurChannelPtr);
	R.a = rd8(gb, wCurChannelFlags);
	e_Func_30651(gb);
}

// a = channel flags, hl = channel
static void e_Func_30651(GB *gb)
{
	if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_FREQUENCY_F)) {
		SET_BC(gb, CHANNEL_UNK_1D);
		add_hl(gb, BC);
		R.e = rd8(gb, HL);
		R.a = 0;
		R.e = f_sla(gb, R.e);
		R.a = alu_sbc(gb, R.a, R.a);
		R.d = R.a;

		SET_BC(gb, (uint16_t)(CHANNEL_PITCH_PRODUCT - CHANNEL_UNK_1D));
		add_hl(gb, BC);
		R.a = ld_hli(gb);
		R.a = alu_add(gb, R.a, R.e);
		R.e = R.a;
		R.a = ld_hld(gb);
		R.a = alu_adc(gb, R.a, R.d);
		R.d = R.a;

		SET_BC(gb, (uint16_t)(CHANNEL_PITCH_OFFSET_MOD - CHANNEL_PITCH_PRODUCT));
		add_hl(gb, BC);
		R.a = ld_hli(gb);
		R.a = alu_add(gb, R.a, R.e);
		R.e = R.a;
		R.a = ld_hld(gb);
		R.a = alu_adc(gb, R.a, R.d);
		R.d = R.a;

		SET_BC(gb, CHANNEL_VIBRATO_DISABLED - CHANNEL_PITCH_OFFSET_MOD);
		add_hl(gb, BC);
		R.a = ld_hli(gb);
		alu_cp(gb, R.a, FALSE);
		if (ZF) {
			R.a = ld_hli(gb);
			R.a = alu_add(gb, R.a, R.e);
			R.e = R.a;
			R.a = ld_hld(gb);
			R.a = alu_adc(gb, R.a, R.d);
			R.d = R.a;
		}
		// .no_vibrato
		SET_BC(gb, CHANNEL_FREQUENCY - CHANNEL_UNK_24);
		add_hl(gb, BC);
		R.a = R.e;
		st_hli(gb, R.a);
		wr8(gb, HL, R.d);

		SET_BC(gb, (uint16_t)-(CHANNEL_FREQUENCY + 1));
		add_hl(gb, BC);
		R.a = rd8(gb, wCurChannelFlags);
	}
	// .skip_frequency
	if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_SO_F)) {
		SET_BC(gb, CHANNEL_SINGLE_PAN);
		add_hl(gb, BC);
		R.a = ld_hli(gb);
		R.a = alu_add(gb, R.a, rd8(gb, HL));
		R.e = R.a;
		SET_BC(gb, CHANNEL_SO_FLAGS - CHANNEL_GLOBAL_PAN);
		add_hl(gb, BC);
		wr8(gb, HL, R.e);
		SET_BC(gb, (uint16_t)-CHANNEL_SO_FLAGS);
		add_hl(gb, BC);
		R.a = rd8(gb, wCurChannelFlags);
	}
	// .asm_306a8
	if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_VOLUME_F)) {
		SET_BC(gb, CHANNEL_VOLUME);
		add_hl(gb, BC);
		R.a = ld_hli(gb);
		R.b = R.a;
		R.c = rd8(gb, HL);
		e_MultiplyBByC_Short(gb);
		R.a = alu_add(gb, R.a, 0x0f);
		R.a = alu_and(gb, R.a, 0xf0);
		alu_cp(gb, R.a, 0x40);
		if (!CF)
			R.a = 0xff;
		R.a = f_rlca(gb, R.a);
		R.a = f_rlca(gb, R.a);
		R.e = R.a;
		SET_BC(gb, CHANNEL_UNK_2F - CHANNEL_VOLUME_MOD);
		add_hl(gb, BC);
		wr8(gb, HL, R.e);
		SET_BC(gb, (uint16_t)-CHANNEL_UNK_2F);
		add_hl(gb, BC);
		R.a = rd8(gb, wCurChannelFlags);
	}
	// .done
	wr8(gb, HL, R.a);
}

// Command handlers. They run with de = command pointer, and return NEXT_CMD
// where the asm jumps back to DoAudioCommand.

static int h_Func_30727(GB *gb)
{
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	alu_cp(gb, R.a, 0x0c);
	if (!CF)
		return h_AudioCmd_ClearFlags(gb);
	R.b = R.a;
	read_cmds_1byte(gb);
	R.a = R.c;
	wr8(gb, wAudioCmdArg, R.a);
	R.a = R.b;
	SET_BC(gb, ADDR_PointerTable_3070f);
	return jump_to_pointer_in_table(gb);
}

static int h_Func_3073f(GB *gb)
{
	R.a = alu_add(gb, R.a, 0x31);
	if (ZF)
		return NEXT_CMD;
	R.a = alu_add(gb, R.a, ADDR_Data_3102b & 0xff);
	R.l = R.a;
	R.a = 0;
	R.a = alu_adc(gb, R.a, ADDR_Data_3102b >> 8);
	R.h = R.a;
	R.b = rd8(gb, HL);
	R.b = alu_dec(gb, R.b);

	ld_hl_from(gb, wCurChannelPtr);
	inc_hl(gb);
	R.a = R.b;
	st_hli(gb, R.a);
	R.a = R.e;
	st_hli(gb, R.a);
	R.a = R.d;
	st_hld(gb, R.a);
	e_UpdateVibrato(gb);
	return DONE;
}

static int h_AudioCmd_Jump(GB *gb)
{
	read_cmds_2bytes(gb);
	R.e = R.c;
	R.d = R.b;
	return NEXT_CMD;
}

static int h_AudioCmd_Call(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_UNK_26);
	add_hl(gb, BC);
	R.a = rd8(gb, HL);
	alu_cp(gb, R.a, 0x0a);
	if (!CF)
		return h_AudioCmd_ClearFlags(gb);
	R.a = alu_inc(gb, R.a);
	R.a = alu_inc(gb, R.a);
	wr8(gb, HL, R.a);
	R.c = R.a;
	R.b = 0x00;
	add_hl(gb, BC);
	SET_BC(gb, 0x0a);
	add_hl(gb, BC);
	inc_de(gb);
	inc_de(gb);
	R.a = R.e;
	st_hli(gb, R.a);
	wr8(gb, HL, R.d);
	dec_de(gb);
	dec_de(gb);
	return h_AudioCmd_Jump(gb);
}

static int h_AudioCmd_Ret(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_UNK_26);
	add_hl(gb, BC);
	R.a = rd8(gb, HL);
	and_a(gb);
	if (ZF)
		return NEXT_CMD;
	R.a = alu_dec(gb, R.a);
	R.a = alu_dec(gb, R.a);
	wr8(gb, HL, R.a);
	R.c = R.a;
	R.b = 0x00;
	add_hl(gb, BC);
	SET_BC(gb, 0x0c);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	R.e = R.a;
	R.d = rd8(gb, HL);
	return NEXT_CMD;
}

static int h_AudioCmd_Loop(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_UNK_29);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	and_a(gb);
	if (ZF)
		return h_AudioCmd_Jump(gb);
	inc_mhl(gb);
	alu_cp(gb, R.a, rd8(gb, HL));
	if (!ZF)
		return h_AudioCmd_Jump(gb);
	xor_a(gb);
	wr8(gb, HL, R.a);
	inc_de(gb);
	inc_de(gb);
	return NEXT_CMD;
}

static int h_AudioCmd_SetTempo(GB *gb)
{
	SET_HL(gb, wAudioEngineFlags);
	bool sfx = alu_bit(gb, rd8(gb, HL), AUDIOENG_HANDLING_SFX_F);
	SET_HL(gb, sfx ? wSFXBaseTempo : wMusicBaseTempo);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	wr8(gb, HL, R.a);
	e_CalculateTempo(gb);
	return NEXT_CMD;
}

// hl = wSFXBaseTempo or wMusicBaseTempo
static void e_CalculateTempo(GB *gb)
{
	R.a = ld_hli(gb);
	R.b = R.a;
	R.a = ld_hli(gb);
	R.c = R.a;
	uint16_t hl = HL;
	e_MultiplyBByC(gb);
	R.a = R.h;
	alu_cp(gb, R.a, Q6_256_0_HI);
	if (!CF) {
		R.a = Q6_255_99 >> 8;
		R.l = Q6_255_99 & 0xff;
	}
	R.l = f_sla(gb, R.l);
	R.a = alu_rla(gb, R.a);
	R.l = f_sla(gb, R.l);
	R.a = alu_rla(gb, R.a);
	and_a(gb);
	if (ZF)
		R.a = alu_inc(gb, R.a);
	SET_HL(gb, hl);
	wr8(gb, HL, R.a);
}

// SetChannelProperty with bc = field
static int h_SetChannelProperty(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	wr8(gb, HL, R.a);
	return NEXT_CMD;
}

static int set_channel_property(GB *gb, uint16_t field)
{
	SET_BC(gb, field);
	return h_SetChannelProperty(gb);
}

static int h_AudioCmd_SetSemitoneOffset(GB *gb) { return set_channel_property(gb, CHANNEL_SEMITONE_OFFSET); }
static int h_AudioCmd_SetVibratoDelay(GB *gb) { return set_channel_property(gb, CHANNEL_VIBRATO_DELAY); }
static int h_AudioCmd_SetVibratoAmplitude(GB *gb) { return set_channel_property(gb, CHANNEL_VIBRATO_AMPLITUDE); }
static int h_AudioCmd_SetChannelUnk27(GB *gb) { return set_channel_property(gb, CHANNEL_UNK_27); }
static int h_AudioCmd_SetChannelUnk28(GB *gb) { return set_channel_property(gb, CHANNEL_UNK_28); }
static int h_AudioCmd_SetChannelTimbre(GB *gb) { return set_channel_property(gb, CHANNEL_TIMBRE); }
static int h_AudioCmd_SetChannelDuration(GB *gb) { return set_channel_property(gb, CHANNEL_LENGTH); }
static int h_AudioCmd_SetChannelSweep(GB *gb) { return set_channel_property(gb, CHANNEL_SWEEP); }

static int h_AudioCmd_SetChannelVibratoDisabled(GB *gb)
{
	R.a = rd8(gb, wCurChannelFlags);
	R.a = alu_or(gb, R.a, CHANFLAGS_UPDATE_SO | CHANFLAGS_UPDATE_VOLUME | CHANFLAGS_UPDATE_FREQUENCY);
	wr8(gb, wCurChannelFlags, R.a);
	return set_channel_property(gb, CHANNEL_VIBRATO_DISABLED);
}

static int h_AudioCmd_SetChannelUnk1e(GB *gb)
{
	R.a = rd8(gb, wCurChannelFlags);
	R.a |= CHANFLAGS_UPDATE_FREQUENCY;
	wr8(gb, wCurChannelFlags, R.a);
	return set_channel_property(gb, CHANNEL_UNK_1E);
}

static int h_AudioCmd_SetWave(GB *gb)
{
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	alu_cp(gb, R.a, WAVE_28);
	if (ZF) {
		// .percussion
		R.a = rd8(gb, wCurChannelFlags);
		R.a |= CHANFLAGS_UPDATE_PERCUSSION;
		wr8(gb, wCurChannelFlags, R.a);
		return NEXT_CMD;
	}
	e_GetWave(gb);
	R.a = rd8(gb, wCurChannelPtr + 0);
	R.a = alu_add(gb, R.a, CHANNEL_TIMBRE);
	R.c = R.a;
	R.a = rd8(gb, wCurChannelPtr + 1);
	R.a = alu_adc(gb, R.a, 0);
	R.b = R.a;
	for (int i = 0; i < 5; i++) {
		if (i)
			inc_bc(gb);
		R.a = ld_hli(gb);
		wr8(gb, BC, R.a);
	}
	R.a = rd8(gb, wCurChannelFlags);
	R.a &= ~CHANFLAGS_UPDATE_PERCUSSION;
	wr8(gb, wCurChannelFlags, R.a);
	return NEXT_CMD;
}

static void e_GetWave(GB *gb)
{
	R.l = R.a;
	R.h = 0x00;
	add_hl(gb, HL);
	R.c = R.l;
	R.b = R.h;
	add_hl(gb, HL);
	add_hl(gb, BC);
	SET_BC(gb, ADDR_Waves);
	add_hl(gb, BC);
}

// b = pitch offset, c = pitch offset multiplier, hl = CHANNEL_PITCH_PRODUCT
static int h_CalculatePitchProduct(GB *gb)
{
	uint16_t hl = HL;
	R.b = f_sla(gb, R.b);
	if (!CF) {
		e_MultiplyBByC(gb);
		R.c = R.l;
		R.b = R.h;
	} else {
		R.a = R.b;
		f_cpl(gb);
		R.b = R.a;
		e_MultiplyBByC(gb);
		dec_hl(gb);
		R.a = R.l;
		f_cpl(gb);
		R.c = R.a;
		R.a = R.h;
		f_cpl(gb);
		R.b = R.a;
	}
	SET_HL(gb, hl);
	R.a = R.c;
	st_hli(gb, R.a);
	wr8(gb, HL, R.b);
	R.a = rd8(gb, wCurChannelFlags);
	R.a |= CHANFLAGS_UPDATE_FREQUENCY;
	wr8(gb, wCurChannelFlags, R.a);
	return NEXT_CMD;
}

static int h_AudioCmd_SetPitchOffset(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_PITCH_OFFSET);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	R.a = f_rlca(gb, R.a);
	R.a = alu_sub(gb, R.a, 0x80);
	st_hli(gb, R.a);
	R.b = R.a;
	R.a = ld_hli(gb);
	R.c = R.a;
	return h_CalculatePitchProduct(gb);
}

static int h_AudioCmd_SetPitchOffsetMultiplier(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_PITCH_OFFSET_MULT);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	st_hld(gb, R.a);
	R.c = R.a;
	R.a = ld_hli(gb);
	R.b = R.a;
	inc_hl(gb);
	return h_CalculatePitchProduct(gb);
}

static int h_AudioCmd_SetVibratoSpeed(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_VIBRATO_SPEED);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	st_hli(gb, R.a);
	R.a = f_sla(gb, R.a);
	if (ZF) {
		f_ccf(gb);
		R.a = f_rra(gb, R.a);
		R.a = f_rra(gb, R.a);
		wr8(gb, HL, R.a);
	}
	return NEXT_CMD;
}

static int h_AudioCmd_SetChannelVolume(GB *gb)
{
	R.a = rd8(gb, wCurChannelFlags);
	R.a |= CHANFLAGS_UPDATE_VOLUME;
	wr8(gb, wCurChannelFlags, R.a);
	SET_BC(gb, CHANNEL_VOLUME);
	ld_hl_from(gb, wCurChannelPtr);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	R.a = f_rlca(gb, R.a);
	wr8(gb, HL, R.a);
	return NEXT_CMD;
}

// bc = field: stores (arg rlca) - $80
static int h_Func_3098b(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	R.a = f_rlca(gb, R.a);
	R.a = alu_sub(gb, R.a, 0x80);
	wr8(gb, HL, R.a);
	return NEXT_CMD;
}

static int h_AudioCmd_Pan(GB *gb)
{
	R.a = rd8(gb, wCurChannelFlags);
	R.a |= CHANFLAGS_UPDATE_SO;
	wr8(gb, wCurChannelFlags, R.a);
	SET_BC(gb, CHANNEL_SINGLE_PAN);
	return h_Func_3098b(gb);
}

static int h_AudioCmd_SetChannelUnk1d(GB *gb)
{
	R.a = rd8(gb, wCurChannelFlags);
	R.a |= CHANFLAGS_UPDATE_FREQUENCY;
	wr8(gb, wCurChannelFlags, R.a);
	SET_BC(gb, CHANNEL_UNK_1D);
	return h_Func_3098b(gb);
}

// bc = field: sets its high nybble
static int h_Func_309ce(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	R.a = alu_swap(gb, R.a);
	R.a = alu_and(gb, R.a, 0xf0);
	R.b = R.a;
	R.a = rd8(gb, HL);
	R.a = alu_and(gb, R.a, 0x0f);
	R.a = alu_or(gb, R.a, R.b);
	wr8(gb, HL, R.a);
	return NEXT_CMD;
}

// bc = field: sets its low nybble
static int h_Func_309eb(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	inc_de(gb);
	R.a = alu_and(gb, R.a, 0x0f);
	R.b = R.a;
	R.a = rd8(gb, HL);
	R.a = alu_and(gb, R.a, 0xf0);
	R.a = alu_or(gb, R.a, R.b);
	wr8(gb, HL, R.a);
	return NEXT_CMD;
}

static int h_AudioCmd_SetChannelFadeInEnvelope(GB *gb)
{
	SET_BC(gb, CHANNEL_FADE_IN_ENVELOPE);
	return h_Func_309ce(gb);
}
static int h_Func_309e8(GB *gb)
{
	SET_BC(gb, CHANNEL_FADE_IN_ENVELOPE);
	return h_Func_309eb(gb);
}
static int h_Func_30a03(GB *gb)
{
	SET_BC(gb, CHANNEL_FADE_OUT_ENVELOPE);
	return h_Func_309ce(gb);
}
static int h_AudioCmd_SetChannelFadeOutEnvelope(GB *gb)
{
	SET_BC(gb, CHANNEL_FADE_OUT_ENVELOPE);
	return h_Func_309eb(gb);
}

static int h_InvalidAudioCmd(GB *gb)
{
	return h_AudioCmd_ClearFlags(gb);
}

static int h_Func_30a1f(GB *gb);

static int h_Func_30a10(GB *gb)
{
	R.b = 0x00;
	return h_Func_30a1f(gb);
}

static int h_Func_30a14(GB *gb)
{
	R.a = alu_sub(gb, R.a, 0xcf);
	R.a = alu_add(gb, R.a, ADDR_Data_3102b & 0xff);
	R.l = R.a;
	R.a = 0;
	R.a = alu_adc(gb, R.a, ADDR_Data_3102b >> 8);
	R.h = R.a;
	R.b = rd8(gb, HL);
	return h_Func_30a1f(gb);
}

// b = note duration
static int h_Func_30a1f(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	R.a = R.b;
	SET_BC(gb, CHANNEL_UNK_0B);
	add_hl(gb, BC);
	st_hld(gb, R.a);
	R.b = 0x00;
	R.a = rd8(gb, wAudioCmdArg);
	for (bool first = true;; first = false) {
		if (!first) {
			// .loop_args
			inc_de(gb);
			read_cmds_1byte(gb);
			R.a = R.c;
		}
		// .got_arg
		if (alu_bit(gb, R.a, 7))
			break;
		alu_cp(gb, R.a, C_0);
		if (!CF) {
			// .pitch
			if (alu_bit(gb, R.b, 7))
				break;
			R.b |= 1 << 7;
			dec_hl(gb);
			st_hli(gb, R.a);
			continue;
		}
		alu_cp(gb, R.a, 0x20);
		if (CF) {
			// .asm_30a5a
			if (alu_bit(gb, R.b, 6))
				break;
			R.b |= 1 << 6;
			R.a = f_rlca(gb, R.a);
			R.a = f_rlca(gb, R.a);
			R.a = f_rlca(gb, R.a);
			R.a = alu_or(gb, R.a, 0x07);
			wr8(gb, HL, R.a);
			continue;
		}
		if (alu_bit(gb, R.b, 5))
			break;
		R.b |= 1 << 5;
		inc_hl(gb);
		R.a = alu_add(gb, R.a, rd8(gb, HL));
		st_hld(gb, R.a);
	}

	// .done_args
	uint16_t saved_de = DE;
	dec_hl(gb);
	R.a = rd8(gb, HL);
	SET_BC(gb, CHANNEL_SEMITONE_OFFSET - CHANNEL_PITCH);
	add_hl(gb, BC);
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	wr8(gb, wSoundEngineParam3, R.a);
	wr8(gb, wSoundEngineParam4, R.a);
	SET_BC(gb, CHANNEL_VIBRATO_DELAY - CHANNEL_SEMITONE_OFFSET);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	st_hld(gb, R.a);
	SET_BC(gb, (uint16_t)(CHANNEL_TIMBRE - CHANNEL_VIBRATO_DELAY));
	add_hl(gb, BC);

	R.a = rd8(gb, wCurChannelFlags);
	if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_PERCUSSION_F)) {
		uint16_t hl = HL;
		R.e = R.l;
		R.d = R.h;
		R.a = rd8(gb, wSoundEngineParam3);
		R.a = alu_add(gb, R.a, 4);
		e_GetWave(gb);
		for (int i = 0; i < 5; i++) {
			if (i)
				inc_de(gb);
			R.a = ld_hli(gb);
			wr8(gb, DE, R.a);
		}
		R.a = rd8(gb, HL);
		wr8(gb, wSoundEngineParam4, R.a);
		SET_HL(gb, hl);
	}
	// .asm_30aa5
	R.a = rd8(gb, HL);
	alu_cp(gb, R.a, 0x10);
	if (!CF) {
		alu_cp(gb, R.a, 0x40);
		if (!CF)
			e_SetTrack4(gb);
		else
			e_SetTrack3(gb);
	} else {
		alu_cp(gb, R.a, 0x08);
		if (!CF)
			e_SetTrack2(gb);
		else
			e_SetTrack1(gb);
	}

	// .got_track
	R.a = rd8(gb, wNumAudioChannels);
	SET_HL(gb, wAudioEngineFlags);
	if (alu_bit(gb, rd8(gb, HL), AUDIOENG_HANDLING_SFX_F))
		R.a |= 1 << 7;
	wr8(gb, wTempTrackUnk02, R.a);
	R.e = R.a;
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_PRIORITY);
	add_hl(gb, BC);
	R.a = rd8(gb, HL);
	wr8(gb, wTempTrackPriority, R.a);
	R.d = R.a;

	ld_hl_from(gb, wCurTrackPtr);
	R.a = ld_hli(gb);
	and_a(gb);
	bool update = true;
	if (!ZF && alu_bit(gb, R.a, TRACKFLAGS_5_F)) {
		// .asm_30af7
		R.a = ld_hli(gb);
		alu_cp(gb, R.a, R.d);
		if (!CF) {
			if (!ZF) {
				update = false;
			} else {
				R.a = R.e;
				alu_cp(gb, R.a, rd8(gb, HL));
				if (CF)
					update = false;
			}
		}
	}
	if (update) {
		// .UpdateTrack
		e_TurnSoundRegisterOff(gb);
		R.a = rd8(gb, wCurChannelPtr + 0);
		R.e = R.a;
		R.a = rd8(gb, wCurChannelPtr + 1);
		R.d = R.a;
		ld_hl_from(gb, wCurTrackPtr);

		R.a = TRACKFLAGS_4 | TRACKFLAGS_5 | TRACKFLAGS_6 | TRACKFLAGS_7;
		st_hli(gb, R.a);
		R.a = rd8(gb, wTempTrackPriority);
		st_hli(gb, R.a);
		R.a = rd8(gb, wTempTrackUnk02);
		st_hli(gb, R.a);
		R.a = rd8(gb, wSoundEngineParam3);
		st_hli(gb, R.a);
		R.a = R.e;
		st_hli(gb, R.a);
		R.a = R.d;
		st_hli(gb, R.a);
		R.a = CHANNEL_UNK_0A;
		R.a = alu_add(gb, R.a, R.e);
		R.e = R.a;
		R.a = 0;
		R.a = alu_adc(gb, R.a, R.d);
		R.d = R.a;
		R.a = rd8(gb, DE);
		st_hli(gb, R.a);
		for (int i = 0; i < 6; i++) {
			inc_de(gb);
			R.a = rd8(gb, DE);
			st_hli(gb, R.a);
		}
		xor_a(gb);
		st_hli(gb, R.a);
		R.a = rd8(gb, wSoundEngineParam4);
		wr8(gb, HL, R.a);
	}
	// .skip
	SET_DE(gb, saved_de);
	return NEXT_CMD;
}

static int h_Func_30b4d(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_PITCH);
	add_hl(gb, BC);
	R.a = rd8(gb, wAudioCmdArg);
	if (!alu_bit(gb, R.a, 7)) {
		alu_cp(gb, R.a, 0x24);
		if (!CF) {
			inc_de(gb);
			wr8(gb, HL, R.a);
		}
	}
	// .asm_30b66
	R.a = rd8(gb, HL);
	SET_BC(gb, CHANNEL_SEMITONE_OFFSET - CHANNEL_PITCH);
	add_hl(gb, BC);
	R.a = alu_add(gb, R.a, rd8(gb, HL));
	wr8(gb, wSoundEngineParam3, R.a);

	uint16_t saved_de = DE;
	e_SetAllTracks(gb);
	R.a = rd8(gb, wNumAudioChannels);
	SET_HL(gb, wAudioEngineFlags);
	if (alu_bit(gb, rd8(gb, HL), AUDIOENG_HANDLING_SFX_F))
		R.a |= 1 << 7;
	R.e = R.a;
	R.a = rd8(gb, wSoundEngineParam3);
	R.d = R.a;

	for (;;) {
		ld_hl_from(gb, wCurTrackPtr);
		R.a = ld_hli(gb);
		if (alu_bit(gb, R.a, TRACKFLAGS_5_F)) {
			inc_hl(gb);
			R.a = ld_hli(gb);
			alu_cp(gb, R.a, R.e);
			if (ZF) {
				R.a = rd8(gb, HL);
				alu_cp(gb, R.a, R.d);
				if (ZF) {
					SET_BC(gb, TRACK_DURATION - TRACK_UNK03);
					add_hl(gb, BC);
					R.a = rd8(gb, HL);
					and_a(gb);
					if (ZF) {
						e_Func_30be3(gb);
						break;
					}
				}
			}
		}
		// .next_track
		e_NextTrack(gb);
		if (ZF)
			break;
	}
	// .done
	SET_DE(gb, saved_de);
	return NEXT_CMD;
}

// JumpToPointerInTable: a = index, bc = table. Runs the routine it jumps to.
static int jump_to_pointer_in_table(GB *gb)
{
	R.a = f_sla(gb, R.a);
	R.a = alu_add(gb, R.a, R.c);
	R.c = R.a;
	R.a = 0;
	R.a = alu_adc(gb, R.a, R.b);
	R.b = R.a;
	R.a = rd8(gb, BC);
	R.l = R.a;
	inc_bc(gb);
	R.a = rd8(gb, BC);
	R.h = R.a;
	switch (HL) {
	case ADDR_AudioCmd_ClearFlags: return h_AudioCmd_ClearFlags(gb);
	case ADDR_AudioCmd_Jump: return h_AudioCmd_Jump(gb);
	case ADDR_AudioCmd_Call: return h_AudioCmd_Call(gb);
	case ADDR_AudioCmd_Ret: return h_AudioCmd_Ret(gb);
	case ADDR_AudioCmd_Loop: return h_AudioCmd_Loop(gb);
	case ADDR_InvalidAudioCmd: return h_InvalidAudioCmd(gb);
	case ADDR_AudioCmd_SetTempo: return h_AudioCmd_SetTempo(gb);
	case ADDR_AudioCmd_SetSemitoneOffset: return h_AudioCmd_SetSemitoneOffset(gb);
	case ADDR_AudioCmd_SetWave: return h_AudioCmd_SetWave(gb);
	case ADDR_AudioCmd_SetChannelVolume: return h_AudioCmd_SetChannelVolume(gb);
	case ADDR_AudioCmd_Pan: return h_AudioCmd_Pan(gb);
	case ADDR_AudioCmd_SetPitchOffset: return h_AudioCmd_SetPitchOffset(gb);
	case ADDR_AudioCmd_SetPitchOffsetMultiplier: return h_AudioCmd_SetPitchOffsetMultiplier(gb);
	case ADDR_AudioCmd_SetVibratoSpeed: return h_AudioCmd_SetVibratoSpeed(gb);
	case ADDR_AudioCmd_SetVibratoDelay: return h_AudioCmd_SetVibratoDelay(gb);
	case ADDR_AudioCmd_SetVibratoAmplitude: return h_AudioCmd_SetVibratoAmplitude(gb);
	case ADDR_AudioCmd_SetChannelVibratoDisabled: return h_AudioCmd_SetChannelVibratoDisabled(gb);
	case ADDR_AudioCmd_SetChannelUnk1d: return h_AudioCmd_SetChannelUnk1d(gb);
	case ADDR_AudioCmd_SetChannelUnk1e: return h_AudioCmd_SetChannelUnk1e(gb);
	case ADDR_Func_30727: return h_Func_30727(gb);
	case ADDR_Func_30a10: return h_Func_30a10(gb);
	case ADDR_Func_30b4d: return h_Func_30b4d(gb);
	case ADDR_AudioCmd_SetChannelTimbre: return h_AudioCmd_SetChannelTimbre(gb);
	case ADDR_AudioCmd_SetChannelFadeInEnvelope: return h_AudioCmd_SetChannelFadeInEnvelope(gb);
	case ADDR_Func_309e8: return h_Func_309e8(gb);
	case ADDR_Func_30a03: return h_Func_30a03(gb);
	case ADDR_AudioCmd_SetChannelFadeOutEnvelope: return h_AudioCmd_SetChannelFadeOutEnvelope(gb);
	case ADDR_AudioCmd_SetChannelUnk27: return h_AudioCmd_SetChannelUnk27(gb);
	case ADDR_AudioCmd_SetChannelUnk28: return h_AudioCmd_SetChannelUnk28(gb);
	case ADDR_AudioCmd_SetChannelDuration: return h_AudioCmd_SetChannelDuration(gb);
	case ADDR_AudioCmd_SetChannelSweep: return h_AudioCmd_SetChannelSweep(gb);
	case ADDR_AudioMod_Tempo: e_AudioMod_Tempo(gb); return DONE;
	case ADDR_AudioMod_Pitch: e_AudioMod_Pitch(gb); return DONE;
	case ADDR_AudioMod_Volume: e_AudioMod_Volume(gb); return DONE;
	case ADDR_AudioMod_Pan: e_AudioMod_Pan(gb); return DONE;
	case ADDR_AudioMod_VibratoAmplitude: e_AudioMod_VibratoAmplitude(gb); return DONE;
	case ADDR_AudioMod_VibratoSpeed: e_AudioMod_VibratoSpeed(gb); return DONE;
	case ADDR_Func_309b6: e_Func_309b6(gb); return DONE;
	}
	// Every index is range checked, so this only happens when a write
	// through a garbage pointer switched ROM banks and the table was read
	// from another bank. The asm jumps there; run whatever is there.
	gb_call(gb, HL);
	return DONE;
}

// ---------------------------------------------------------------------------
// Tracks

static void e_Func_30bb0(GB *gb)
{
	ld_hl_from(gb, wCurTrackPtr);
	if (!alu_bit(gb, rd8(gb, HL), TRACKFLAGS_7_F))
		return;
	inc_hl(gb);
	inc_hl(gb);
	if (!alu_bit(gb, rd8(gb, HL), 7))
		return;
	e_Func_30bd2(gb);
}

static void e_Func_30bc2(GB *gb)
{
	ld_hl_from(gb, wCurTrackPtr);
	if (!alu_bit(gb, rd8(gb, HL), TRACKFLAGS_7_F))
		return;
	inc_hl(gb);
	inc_hl(gb);
	if (alu_bit(gb, rd8(gb, HL), 7))
		return;
	e_Func_30bd2(gb);
}

// hl = TRACK_UNK02
static void e_Func_30bd2(GB *gb)
{
	inc_hl(gb);
	inc_hl(gb);
	R.a = ld_hli(gb);
	R.e = R.a;
	R.a = ld_hli(gb);
	R.d = R.a;
	R.a = rd8(gb, DE);
	alu_cp(gb, R.a, TRACKFLAGS_6 | TRACKFLAGS_7);
	if (CF) {
		e_Func_30bf2(gb);
		return;
	}
	inc_hl(gb);
	R.a = rd8(gb, HL);
	and_a(gb);
	if (ZF)
		return;
	dec_mhl(gb);
	if (!ZF)
		return;
	e_Func_30be3(gb);
}

// hl = TRACK_DURATION
static void e_Func_30be3(GB *gb)
{
	SET_BC(gb, (uint16_t)(TRACK_FLAGS - TRACK_DURATION));
	add_hl(gb, BC);
	R.a = rd8(gb, HL);
	if (alu_bit(gb, R.a, TRACKFLAGS_6_F)) {
		e_Func_30bf6(gb);
		return;
	}
	R.a = alu_or(gb, R.a, TRACKFLAGS_4 | TRACKFLAGS_6);
	R.a = alu_and(gb, R.a, 0xff ^ TRACKFLAGS_5);
	wr8(gb, HL, R.a);
}

// hl = TRACK_CHANNEL_PTR + 2
static void e_Func_30bf2(GB *gb)
{
	SET_BC(gb, (uint16_t)(TRACK_FLAGS - (TRACK_CHANNEL_PTR + 2)));
	add_hl(gb, BC);
	e_Func_30bf6(gb);
}

// hl = track
static void e_Func_30bf6(GB *gb)
{
	xor_a(gb);
	wr8(gb, HL, R.a);
	e_TurnSoundRegisterOff(gb);
}

static void e_CheckEnvelopeValue(GB *gb)
{
	SET_BC(gb, (uint16_t)(TRACK_FADE_OUT_ENVELOPE - TRACK_VOLUME_ENVELOPE));
	add_hl(gb, BC);
	R.a = rd8(gb, HL);
	SET_BC(gb, TRACK_VOLUME_ENVELOPE - TRACK_FADE_OUT_ENVELOPE);
	add_hl(gb, BC);
	R.a = alu_and(gb, R.a, 0xf0);
}

// hl = track
static void e_Func_30dc6(GB *gb)
{
	uint16_t hl = HL;
	SET_BC(gb, TRACK_UNK06);
	add_hl(gb, BC);
	R.c = rd8(gb, HL);
	ld_hl_from(gb, wCurChannelPtr);
	SET_DE(gb, CHANNEL_UNK_2F);
	add_hl(gb, DE);
	R.b = rd8(gb, HL);
	e_MultiplyBByC_Short(gb);
	R.a = alu_add(gb, R.a, 0x0f);
	R.a = alu_and(gb, R.a, 0xf0);
	SET_HL(gb, hl);
	SET_DE(gb, TRACK_VOLUME);
	add_hl(gb, DE);
	wr8(gb, HL, R.a);
	SET_DE(gb, (uint16_t)(TRACK_FLAGS - TRACK_VOLUME));
	add_hl(gb, DE);
}

// hl = track
static void e_Func_30deb(GB *gb)
{
	uint16_t hl = HL;
	SET_BC(gb, TRACK_FADE_OUT_ENVELOPE);
	add_hl(gb, BC);
	R.b = rd8(gb, HL);
	SET_DE(gb, (uint16_t)(TRACK_UNK06 - TRACK_FADE_OUT_ENVELOPE));
	add_hl(gb, DE);
	R.c = rd8(gb, HL);
	e_MultiplyBByC_Short(gb);
	R.a = alu_add(gb, R.a, 0x0f);
	R.c = R.a;
	ld_hl_from(gb, wCurChannelPtr);
	SET_DE(gb, CHANNEL_UNK_2F);
	add_hl(gb, DE);
	R.b = rd8(gb, HL);
	e_MultiplyBByC_Short(gb);
	R.a = alu_add(gb, R.a, 0x0f);
	R.a = alu_and(gb, R.a, 0xf0);
	SET_HL(gb, hl);
	SET_DE(gb, TRACK_VOLUME);
	add_hl(gb, DE);
	wr8(gb, HL, R.a);
	SET_DE(gb, (uint16_t)(TRACK_FLAGS - TRACK_VOLUME));
	add_hl(gb, DE);
}

// a = volume envelope, hl = TRACK_VOLUME_ENVELOPE
static void e_Func_30e1b(GB *gb)
{
	R.b = R.a;
	R.a = rd8(gb, wCurSoundRegister);
	alu_cp(gb, R.a, (uint8_t)rAUD3LEVEL);
	R.a = R.b;
	if (!ZF)
		return; // not sound3
	R.a = alu_xor(gb, R.a, rd8(gb, HL));
	R.a = alu_and(gb, R.a, 0xc0);
	if (!ZF) {
		R.a = rd8(gb, wCurChannelFlags);
		R.a |= CHANFLAGS_UPDATE_VOLUME;
		wr8(gb, wCurChannelFlags, R.a);
	}
	R.a = R.b;
}

// Labels in Func_30bfb, to translate its gotos
enum {
	L_30c95, L_30ce9, L_30d22, L_30d70, L_30cba, L_30d3b, L_30d77, L_30d84, L_30d95, L_30d9d,
};

static void e_Func_30bfb(GB *gb)
{
	int next;
	ld_hl_from(gb, wCurTrackPtr);
	R.a = rd8(gb, HL);
	if (!alu_bit(gb, R.a, TRACKFLAGS_7_F))
		return;

	R.b = R.a;
	uint16_t track = HL;
	SET_DE(gb, TRACK_CHANNEL_PTR);
	add_hl(gb, DE);
	R.a = ld_hli(gb);
	wr8(gb, wCurChannelPtr + 0, R.a);
	R.e = R.a;
	R.a = ld_hli(gb);
	wr8(gb, wCurChannelPtr + 1, R.a);
	R.d = R.a;
	R.a = rd8(gb, DE);
	alu_cp(gb, R.a, CHANFLAGS_6 | CHANFLAGS_ACTIVE);
	if (CF) {
		SET_HL(gb, track);
		e_Func_30bf6(gb);
		return;
	}

	// .asm_30c1f
	wr8(gb, wCurChannelFlags, R.a);
	if (!alu_bit(gb, R.b, TRACKFLAGS_6_F)) {
		SET_DE(gb, TRACK_FREQUENCY - (TRACK_CHANNEL_PTR + 2));
		add_hl(gb, DE);
		R.a = rd8(gb, w3d01a);
		and_a(gb);
		if (ZF)
			inc_mhl(gb);
		inc_mhl(gb);
		SET_HL(gb, track);
		if (alu_bit(gb, R.b, TRACKFLAGS_5_F))
			next = alu_bit(gb, R.b, TRACKFLAGS_4_F) ? L_30c95 : L_30ce9;
		else
			next = alu_bit(gb, R.b, TRACKFLAGS_4_F) ? L_30d22 : L_30d70;
	} else {
		// .asm_30c46
		SET_HL(gb, track);
		wr8(gb, HL, rd8(gb, HL) & ~TRACKFLAGS_6);
		if (alu_bit(gb, rd8(gb, HL), TRACKFLAGS_5_F)) {
			uint16_t hl = HL;
			e_SetTimbre(gb);
			SET_HL(gb, hl);

			R.a = rd8(gb, wCurChannelFlags);
			R.a = alu_or(gb, R.a, CHANFLAGS_UPDATE_SO | CHANFLAGS_UPDATE_VOLUME | CHANFLAGS_UPDATE_FREQUENCY);
			wr8(gb, wCurChannelFlags, R.a);
			e_Func_30dc6(gb);

			// fade in
			SET_BC(gb, TRACK_FADE_IN_ENVELOPE);
			add_hl(gb, BC);
			R.a = rd8(gb, HL);
			SET_BC(gb, TRACK_VOLUME_ENVELOPE - TRACK_FADE_IN_ENVELOPE);
			add_hl(gb, BC);
			R.a = alu_swap(gb, R.a);
			f_cpl(gb);
			R.a = f_rrca(gb, R.a);
			R.a = alu_and(gb, R.a, 0x07);
			if (ZF) {
				next = L_30cba;
			} else {
				R.a = alu_or(gb, R.a, 0x08);
				st_hli(gb, R.a);
				xor_a(gb);
				st_hli(gb, R.a);
				next = L_30d9d;
			}
		} else {
			// .fade_out
			SET_BC(gb, TRACK_FADE_OUT_ENVELOPE);
			add_hl(gb, BC);
			R.b = rd8(gb, HL);
			SET_DE(gb, TRACK_VOLUME_ENVELOPE - TRACK_FADE_OUT_ENVELOPE);
			add_hl(gb, DE);
			R.a = rd8(gb, HL);
			R.a = alu_and(gb, R.a, 0xf0);
			if (ZF) {
				next = L_30d3b;
			} else {
				R.c = R.a;
				R.a = R.b;
				f_cpl(gb);
				R.a = f_rrca(gb, R.a);
				R.a = alu_and(gb, R.a, 0x07);
				if (ZF) {
					next = L_30d3b;
				} else {
					R.a = alu_or(gb, R.a, R.c);
					st_hli(gb, R.a);
					xor_a(gb);
					st_hli(gb, R.a);
					next = L_30d95;
				}
			}
		}
	}

	for (;;) {
		switch (next) {
		case L_30c95:
			R.a = rd8(gb, wCurChannelFlags);
			if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_VOLUME_F))
				e_Func_30dc6(gb);
			SET_BC(gb, TRACK_VOLUME_ENVELOPE);
			add_hl(gb, BC);
			R.a = ld_hli(gb);
			R.a = alu_and(gb, R.a, 0x07);
			R.a = alu_dec(gb, R.a);
			alu_cp(gb, R.a, rd8(gb, HL));
			if (CF) {
				xor_a(gb);
				st_hld(gb, R.a);
				R.a = rd8(gb, HL);
				R.a = alu_add(gb, R.a, 0x10);
				if (CF) {
					next = L_30cba;
					continue;
				}
				e_Func_30e1b(gb);
				st_hli(gb, R.a);
			}
			// .asm_30cb3
			dec_hl(gb);
			R.a = ld_hld(gb);
			alu_cp(gb, R.a, rd8(gb, HL));
			if (CF) {
				next = L_30d84;
				continue;
			}
			inc_hl(gb);
			next = L_30cba;
			continue;

		case L_30cba: {
			SET_BC(gb, (uint16_t)(TRACK_FADE_IN_ENVELOPE - TRACK_VOLUME_ENVELOPE));
			add_hl(gb, BC);
			R.b = rd8(gb, HL);
			SET_DE(gb, TRACK_VOLUME - TRACK_FADE_IN_ENVELOPE);
			add_hl(gb, DE);
			R.c = rd8(gb, HL);
			uint16_t bc = BC;
			SET_BC(gb, (uint16_t)(TRACK_FLAGS - TRACK_VOLUME));
			add_hl(gb, BC);
			wr8(gb, HL, rd8(gb, HL) & ~TRACKFLAGS_4);
			e_Func_30deb(gb);
			R.d = R.a;
			SET_BC(gb, TRACK_VOLUME_ENVELOPE);
			add_hl(gb, BC);
			SET_BC(gb, bc);
			R.a = R.b;
			f_cpl(gb);
			R.a = f_rrca(gb, R.a);
			R.a = alu_and(gb, R.a, 0x07);
			if (ZF) {
				e_CheckEnvelopeValue(gb);
				if (ZF) {
					next = L_30d3b;
					continue;
				}
				R.a = R.d;
				R.c = R.d;
			}
			// .asm_30ce2
			R.a = alu_or(gb, R.a, R.c);
			st_hli(gb, R.a);
			xor_a(gb);
			st_hli(gb, R.a);
			next = L_30d95;
			continue;
		}

		case L_30ce9:
			R.a = rd8(gb, wCurChannelFlags);
			if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_VOLUME_F))
				e_Func_30deb(gb);
			SET_BC(gb, TRACK_VOLUME_ENVELOPE);
			add_hl(gb, BC);
			R.a = ld_hli(gb);
			R.a = alu_and(gb, R.a, 0x07);
			if (ZF) {
				// .asm_30d1b
				dec_hl(gb);
				R.a = ld_hld(gb);
				R.a = alu_xor(gb, R.a, rd8(gb, HL));
				if (ZF) {
					next = L_30d9d;
					continue;
				}
				// .asm_30d17
				R.a = ld_hli(gb);
				wr8(gb, HL, R.a);
				next = L_30d95;
				continue;
			}
			R.a = alu_dec(gb, R.a);
			alu_cp(gb, R.a, rd8(gb, HL));
			bool to_30d11 = false;
			if (CF) {
				xor_a(gb);
				st_hld(gb, R.a);
				R.a = rd8(gb, HL);
				R.a = alu_sub(gb, R.a, 0x10);
				if (CF) {
					to_30d11 = true;
				} else {
					e_Func_30e1b(gb);
					st_hli(gb, R.a);
				}
			}
			if (!to_30d11) {
				// .asm_30d09
				dec_hl(gb);
				dec_hl(gb);
				R.a = ld_hli(gb);
				R.a = alu_or(gb, R.a, 0x0f);
				alu_cp(gb, R.a, rd8(gb, HL));
				if (CF) {
					next = L_30d84;
					continue;
				}
			}
			// .asm_30d11
			e_CheckEnvelopeValue(gb);
			if (ZF) {
				next = L_30d3b;
				continue;
			}
			dec_hl(gb);
			// .asm_30d17
			R.a = ld_hli(gb);
			wr8(gb, HL, R.a);
			next = L_30d95;
			continue;

		case L_30d22:
			SET_BC(gb, TRACK_VOLUME_ENVELOPE);
			add_hl(gb, BC);
			R.a = ld_hli(gb);
			R.a = alu_and(gb, R.a, 0x07);
			R.a = alu_dec(gb, R.a);
			alu_cp(gb, R.a, rd8(gb, HL));
			if (!CF) {
				next = L_30d84;
				continue;
			}
			xor_a(gb);
			st_hld(gb, R.a);
			R.a = rd8(gb, HL);
			R.a = alu_sub(gb, R.a, 0x10);
			if (CF) {
				next = L_30d3b;
				continue;
			}
			e_Func_30e1b(gb);
			wr8(gb, HL, R.a);
			next = L_30d84;
			continue;

		case L_30d3b: {
			uint16_t hl = HL;
			ld_hl_from(gb, wCurChannelPtr);
			SET_BC(gb, CHANNEL_UNK_2F);
			add_hl(gb, BC);
			R.b = rd8(gb, HL);
			SET_DE(gb, (uint16_t)(CHANNEL_UNK_27 - CHANNEL_UNK_2F));
			add_hl(gb, DE);
			R.a = ld_hli(gb);
			R.c = R.a;
			R.d = rd8(gb, HL);
			SET_HL(gb, hl);

			R.a = R.b;
			and_a(gb);
			if (!ZF) {
				R.a = R.c;
				and_a(gb);
			}
			if (!ZF) {
				R.a = R.d;
				and_a(gb);
			}
			if (ZF) {
				next = L_30d77;
				continue;
			}
			e_MultiplyBByC_Short(gb);
			R.a = alu_add(gb, R.a, 0x0f);
			R.a = alu_and(gb, R.a, 0xf0);
			st_hli(gb, R.a);
			wr8(gb, HL, R.d);
			SET_BC(gb, (uint16_t)(TRACK_FLAGS - TRACK_FREQUENCY));
			add_hl(gb, BC);
			R.a = rd8(gb, HL);
			R.a = alu_and(gb, R.a, 0xff ^ (TRACKFLAGS_4 | TRACKFLAGS_5 | TRACKFLAGS_6));
			wr8(gb, HL, R.a);
			next = L_30d95;
			continue;
		}

		case L_30d70:
			SET_BC(gb, TRACK_FREQUENCY);
			add_hl(gb, BC);
			dec_mhl(gb);
			if (!ZF) {
				next = L_30d9d;
				continue;
			}
			next = L_30d77;
			continue;

		case L_30d77:
			ld_hl_from(gb, wCurTrackPtr);
			xor_a(gb);
			wr8(gb, HL, R.a);
			e_TurnSoundRegisterOff(gb);
			return;

		case L_30d84:
			R.a = rd8(gb, wCurSoundRegister);
			alu_cp(gb, R.a, (uint8_t)rAUD3LEVEL);
			if (!ZF) {
				R.a = rd8(gb, wCurChannelFlags);
				R.a &= ~CHANFLAGS_UPDATE_VOLUME;
				wr8(gb, wCurChannelFlags, R.a);
			}
			next = L_30d9d;
			continue;

		case L_30d95:
			R.a = rd8(gb, wCurChannelFlags);
			R.a |= CHANFLAGS_UPDATE_VOLUME;
			wr8(gb, wCurChannelFlags, R.a);
			next = L_30d9d;
			continue;

		case L_30d9d:
			R.a = rd8(gb, wCurChannelFlags);
			if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_FREQUENCY_F)) {
				e_SetFrequency(gb);
				R.a = rd8(gb, wCurChannelFlags);
			}
			if (alu_bit(gb, R.a, CHANFLAGS_UPDATE_SO_F)) {
				e_SetSoundOutput(gb);
				R.a = rd8(gb, wCurChannelFlags);
			}
			if (!alu_bit(gb, R.a, CHANFLAGS_UPDATE_VOLUME_F))
				return;
			e_SetVolumeEnvelope(gb);
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// Sound hardware

static void e_SetTimbre(GB *gb)
{
	ld_hl_from(gb, wCurTrackPtr);
	SET_BC(gb, TRACK_TIMBRE);
	add_hl(gb, BC);
	R.a = rd8(gb, wCurSoundRegister);
	R.c = R.a;
	alu_cp(gb, R.a, (uint8_t)rAUD3LEVEL);
	if (CF) {
		// .rect_sound
		R.a = ld_hli(gb);
		R.a = f_rrca(gb, R.a);
		R.a = f_rrca(gb, R.a);
		R.a = alu_and(gb, R.a, AUD1LEN_DUTY);
		R.b = R.a;
		R.a = ld_hli(gb);
		and_a(gb);
		if (!ZF) {
			f_cpl(gb);
			R.a = alu_inc(gb, R.a);
			R.a = alu_and(gb, R.a, AUD1LEN_TIMER);
			R.a = alu_or(gb, R.a, R.b);
			R.b = R.a;
			R.a = AUD1HIGH_LENGTH_ON;
		}
		R.c = alu_inc(gb, R.c);
		R.c = alu_inc(gb, R.c);
		ldh_c_wr(gb, R.a); // freq hi
		R.c = alu_dec(gb, R.c);
		R.c = alu_dec(gb, R.c);
		R.c = alu_dec(gb, R.c);
		R.a = R.b;
		ldh_c_wr(gb, R.a); // length / duty cycle
		R.c = alu_dec(gb, R.c);
		R.a = rd8(gb, HL);
		ldh_c_wr(gb, R.a); // sweep
		return;
	}
	if (ZF) {
		// .sound3
		R.a = ld_hli(gb);
		R.a = alu_sub(gb, R.a, WAVEFORM_SINE);
		R.b = R.a;
		R.a = rd8(gb, HL);
		and_a(gb);
		if (!ZF) {
			f_cpl(gb);
			R.a = alu_inc(gb, R.a);
			wr8(gb, rAUD3LEN, R.a);
			R.a = 0x40;
		}
		wr8(gb, rAUD3HIGH, R.a);

		R.a = rd8(gb, wCurWaveSample);
		alu_cp(gb, R.a, R.b);
		if (ZF)
			return;
		R.a = R.b;
		wr8(gb, wCurWaveSample, R.a);
		R.l = R.b;
		R.h = 0x00;
		add_hl(gb, HL);
		add_hl(gb, HL);
		add_hl(gb, HL);
		add_hl(gb, HL);
		SET_BC(gb, ADDR_WaveSamples);
		add_hl(gb, BC);
		R.c = (uint8_t)_AUD3WAVERAM;
		R.b = AUD3WAVE_SIZE;
		do {
			R.a = ld_hli(gb);
			ldh_c_wr(gb, R.a);
			R.c = alu_inc(gb, R.c);
			R.b = alu_dec(gb, R.b);
		} while (!ZF);
		return;
	}
	// noise
	R.a = ld_hli(gb);
	R.a = f_rlca(gb, R.a);
	R.a = f_rlca(gb, R.a);
	R.a = f_rlca(gb, R.a);
	R.a = alu_and(gb, R.a, 0x08);
	R.b = R.a;
	R.c = alu_inc(gb, R.c);
	R.a = ldh_c_rd(gb);
	R.a = alu_and(gb, R.a, 0xff ^ AUD4POLY_7STEP);
	R.a = alu_or(gb, R.a, R.b);
	ldh_c_wr(gb, R.a);
	R.a = rd8(gb, HL);
	and_a(gb);
	if (!ZF) {
		f_cpl(gb);
		R.a = alu_inc(gb, R.a);
		wr8(gb, rAUD4LEN, R.a);
		R.a = AUD4GO_LENGTH_ON;
	}
	R.a = alu_or(gb, R.a, AUD4GO_RESTART);
	wr8(gb, rAUD4GO, R.a);
}

static void e_SetFrequency(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_FREQUENCY);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	R.c = R.a;
	R.b = rd8(gb, HL);

	ld_hl_from(gb, wCurTrackPtr);
	SET_DE(gb, TRACK_UNK0E);
	add_hl(gb, DE);
	R.a = rd8(gb, HL);
	R.a = alu_add(gb, R.a, R.b);
	R.b = R.a;

	R.a = rd8(gb, wCurSoundRegister);
	alu_cp(gb, R.a, (uint8_t)rAUD3LEVEL);
	if (!CF && !ZF) {
		// .noise
		R.c = R.a;
		R.c = alu_inc(gb, R.c);
		R.a = R.b;
		R.a = alu_sub(gb, R.a, 0x0d);
		f_cpl(gb);
		R.b = R.a;
		R.a = alu_and(gb, R.a, 0x03);
		R.d = R.a;
		R.a = R.b;
		alu_cp(gb, R.a, 0xc0);
		if (!CF) {
			R.a = alu_and(gb, R.a, AUD4POLY_SHIFT >> 2);
			R.a = f_rlca(gb, R.a);
			R.a = f_rlca(gb, R.a);
			R.a = alu_or(gb, R.a, R.d);
			R.a = alu_or(gb, R.a, 4 & AUD4POLY_DIV);
			R.d = R.a;
		}
		// .asm_30f2b
		R.a = ldh_c_rd(gb);
		R.a = alu_and(gb, R.a, AUD4POLY_7STEP);
		R.a = alu_or(gb, R.a, R.d);
		ldh_c_wr(gb, R.a);
		R.c = alu_inc(gb, R.c);
		R.a = ldh_c_rd(gb);
		R.a = alu_or(gb, R.a, AUD4GO_RESTART);
		ldh_c_wr(gb, R.a);
		return;
	}
	if (ZF) {
		// sound3: channel 3 plays an octave higher
		R.a = R.b;
		R.a = alu_add(gb, R.a, C_1 - C_0);
		R.b = R.a;
	}
	// .rect_sound
	uint16_t hl = HL;
	R.a = R.b;
	e_GetNoteFrequency(gb);
	R.b = R.a;
	e_MultiplyBByC(gb);
	SET_BC(gb, 0x00ff);
	add_hl(gb, BC);
	R.l = R.h;
	R.h = 0x00;
	add_hl(gb, DE);

	R.a = rd8(gb, wCurSoundRegister);
	R.c = R.a;
	R.c = alu_inc(gb, R.c);
	R.a = R.l;
	ldh_c_wr(gb, R.a);
	R.c = alu_inc(gb, R.c);
	R.a = ldh_c_rd(gb);
	R.a = alu_and(gb, R.a, AUD1HIGH_LENGTH_ON);
	if (!ZF) {
		R.a = alu_or(gb, R.a, R.h);
		ldh_c_wr(gb, R.a);
	} else {
		// .length_counter
		R.a = alu_or(gb, R.a, R.h);
		ldh_c_wr(gb, R.a);
		R.b = R.a;
		R.a = rd8(gb, wCurSoundRegister);
		R.c = R.a;
		R.c = alu_dec(gb, R.c);
		R.a = ldh_c_rd(gb);
		R.a = alu_and(gb, R.a, 0xc0);
		ldh_c_wr(gb, R.a);
		R.a = R.b;
	}
	// .skip_length
	SET_HL(gb, hl);
	SET_BC(gb, TRACK_FREQUENCY + 1 - TRACK_UNK0E);
	add_hl(gb, BC);
	R.a = alu_or(gb, R.a, AUD1HIGH_RESTART);
	wr8(gb, HL, R.a);
}

static void e_SetSoundOutput(GB *gb)
{
	ld_hl_from(gb, wCurChannelPtr);
	SET_BC(gb, CHANNEL_SO_FLAGS);
	add_hl(gb, BC);
	R.a = rd8(gb, wCurSoundRegister);
	uint8_t mask;
	alu_cp(gb, R.a, (uint8_t)rAUD2ENV);
	if (CF) {
		mask = AUDTERM_1_RIGHT | AUDTERM_1_LEFT;
	} else if (ZF) {
		mask = AUDTERM_2_RIGHT | AUDTERM_2_LEFT;
	} else {
		alu_cp(gb, R.a, (uint8_t)rAUD4ENV);
		mask = CF ? (AUDTERM_3_RIGHT | AUDTERM_3_LEFT) : (AUDTERM_4_RIGHT | AUDTERM_4_LEFT);
	}
	R.d = (uint8_t)(0xff ^ mask);
	R.e = mask;

	// .got_masks
	bool pan = false;
	if (!alu_bit(gb, rd8(gb, HL), 7)) {
		if (alu_bit(gb, rd8(gb, HL), 6)) {
			R.a = SO_RIGHT; // only bit 6 set
			pan = true;
		}
	} else {
		if (!alu_bit(gb, rd8(gb, HL), 6)) {
			R.a = SO_LEFT; // only bit 7 set
			pan = true;
		}
	}
	if (pan) {
		R.a = alu_and(gb, R.a, R.e);
		R.e = R.a;
	}
	// .no_panning
	R.c = (uint8_t)rAUDTERM;
	R.a = ldh_c_rd(gb);
	R.a = alu_and(gb, R.a, R.d);
	R.a = alu_or(gb, R.a, R.e);
	ldh_c_wr(gb, R.a);
}

static void e_SetVolumeEnvelope(GB *gb)
{
	ld_hl_from(gb, wCurTrackPtr);
	SET_BC(gb, TRACK_VOLUME_ENVELOPE);
	add_hl(gb, BC);
	R.a = ld_hli(gb);
	R.b = R.a;
	R.a = rd8(gb, wCurSoundRegister);
	R.c = R.a;
	alu_cp(gb, R.a, (uint8_t)rAUD3LEVEL);
	if (CF) {
		// .rect
		R.a = R.b;
		ldh_c_wr(gb, R.a);
		R.c = alu_inc(gb, R.c);
		R.c = alu_inc(gb, R.c);
		inc_hl(gb);
		R.a = rd8(gb, HL);
		ldh_c_wr(gb, R.a);
		return;
	}
	if (ZF) {
		// .sound3
		R.a = R.b;
		R.a = alu_sub(gb, R.a, 0x40);
		R.a = alu_xor(gb, R.a, 0xc0);
		R.a = f_rrca(gb, R.a);
		ldh_c_wr(gb, R.a);

		R.a = rd8(gb, rAUD3ENA);
		R.a = alu_rla(gb, R.a);
		if (CF)
			return; // sound is on
		R.a = AUD3ENA_ON;
		wr8(gb, rAUD3ENA, R.a);
		inc_hl(gb);
		R.a = rd8(gb, HL);
		wr8(gb, rAUD3HIGH, R.a);
		return;
	}
	// noise
	R.a = R.b;
	ldh_c_wr(gb, R.a);
	R.a = rd8(gb, rAUD4GO);
	R.a = alu_or(gb, R.a, AUD4GO_RESTART);
	wr8(gb, rAUD4GO, R.a);
}

static void e_TurnSoundRegisterOff(GB *gb)
{
	R.a = rd8(gb, wCurSoundRegister);
	R.c = R.a;
	alu_cp(gb, R.a, (uint8_t)rAUD3LEVEL);
	if (ZF) {
		R.a = AUD3ENA_OFF;
		wr8(gb, rAUD3ENA, R.a);
		return;
	}
	R.a = 0x08; // envelope up
	ldh_c_wr(gb, R.a);
	R.c = alu_inc(gb, R.c);
	R.c = alu_inc(gb, R.c);
	R.a = 0x80; // initialise
	ldh_c_wr(gb, R.a);
}

// ---------------------------------------------------------------------------
// Math

// a = note; returns de = frequency, a = third byte of the entry
static void e_GetNoteFrequency(GB *gb)
{
	R.a = alu_sub(gb, R.a, C_0);
	if (CF)
		xor_a(gb);
	alu_cp(gb, R.a, (B_9 + 1) - C_0);
	if (!CF)
		R.a = B_9 - C_0;
	R.d = 0x00;
	R.e = R.a;
	R.a = alu_add(gb, R.a, R.e);
	R.a = alu_add(gb, R.a, R.e);
	R.d = f_rl(gb, R.d);
	R.e = R.a;
	SET_HL(gb, ADDR_NoteFrequencies);
	add_hl(gb, DE);
	R.a = ld_hli(gb);
	R.e = R.a;
	R.a = ld_hli(gb);
	R.d = R.a;
	R.a = rd8(gb, HL);
}

// hl = b * c
static void e_MultiplyBByC(GB *gb)
{
	R.h = R.b;
	R.l = 0x00;
	R.b = R.l;
	for (int i = 0; i < 7; i++) {
		add_hl(gb, HL);
		if (CF)
			add_hl(gb, BC);
	}
	add_hl(gb, HL);
	if (!CF)
		return;
	add_hl(gb, BC);
}

// a = high nybble of b * high nybble of c (Q4)
static void e_MultiplyBByC_Short(GB *gb)
{
	R.a = R.c;
	R.a = alu_and(gb, R.a, 0xf0);
	R.a = alu_swap(gb, R.a);
	R.c = R.a;
	R.a = R.b;
	R.a = alu_and(gb, R.a, 0xf0);
	for (int i = 0; i < 3; i++) {
		R.a = alu_add(gb, R.a, R.a);
		if (CF)
			R.a = alu_add(gb, R.a, R.c);
	}
	R.a = alu_add(gb, R.a, R.a);
	if (!CF)
		return;
	R.a = alu_add(gb, R.a, R.c);
}

// ---------------------------------------------------------------------------
// Entry points

// Routines whose asm ends in jp ReturnFromAudioJump.
#define PORT_TO_RETURN_FROM_AUDIO_JUMP(name) \
	PORT_JUMP(name) \
	{ \
		e_##name(gb); \
		JUMP(ReturnFromAudioJump); \
	}

PORT_JUMP(_InitAudio)
{
	e__InitAudio(gb);
	JUMP(SwitchBackFromAudioBank);
}

PORT_TO_RETURN_FROM_AUDIO_JUMP(_UpdateAudio)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_PlaySFX)
PORT_TO_RETURN_FROM_AUDIO_JUMP(PlayMusic)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_PlayNewMusic)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_PlayNewMusic_SetNoise)
PORT_TO_RETURN_FROM_AUDIO_JUMP(Func_302e4)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_TurnOffSFXID)
PORT_TO_RETURN_FROM_AUDIO_JUMP(TurnSFXOff)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_TurnMusicOff)
PORT_TO_RETURN_FROM_AUDIO_JUMP(Func_30438)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_GetPlayingSoundID)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_ExecuteAudioMod)
PORT_TO_RETURN_FROM_AUDIO_JUMP(_SetMusicFadeOut)

PORT_JUMP(GetSound)
{
	if (e_GetSound(gb)) {
		RET();
		return;
	}
	// invalid sound: pop hl / jp ReturnFromAudioJump
	SET_HL(gb, gb_pop16(gb));
	JUMP(ReturnFromAudioJump);
}

PORT(SetSFXChannels) { e_SetSFXChannels(gb); }
PORT(SetMusicChannels) { e_SetMusicChannels(gb); }
PORT(SetAudioChannels) { e_SetAudioChannels(gb); }
PORT(NextChannel) { e_NextChannel(gb); }
PORT(SetTrack1) { e_SetTrack1(gb); }
PORT(SetTrack2) { e_SetTrack2(gb); }
PORT(SetTrack3) { e_SetTrack3(gb); }
PORT(SetTrack4) { e_SetTrack4(gb); }
PORT(SetAllTracks) { e_SetAllTracks(gb); }
PORT(SetTrack) { e_SetTrack(gb); }
PORT(NextTrack) { e_NextTrack(gb); }
PORT(LoadChannelWithSound) { e_LoadChannelWithSound(gb); }
PORT(SkipLoadingChannel) { e_SkipLoadingChannel(gb); }
PORT(InitChannel) { e_InitChannel(gb); }
PORT(DeactivateMusicChannels_FromFadeOut) { e_DeactivateMusicChannels_FromFadeOut(gb); }
PORT(DeactivateMusicChannels) { e_DeactivateMusicChannels(gb); }
PORT(ExecuteAudioMod_Internal) { e_ExecuteAudioMod_Internal(gb); }
PORT(AudioModSetValue_Short) { e_AudioModSetValue_Short(gb); }
PORT(AudioModSetValue_Long) { e_AudioModSetValue_Long(gb); }
PORT(PrepareChannelBitmasks) { e_PrepareChannelBitmasks(gb); }
PORT(AddChannelStructLengthToHL) { e_AddChannelStructLengthToHL(gb); }
PORT(UpdateMusicFadeOut) { e_UpdateMusicFadeOut(gb); }
PORT(Func_30547) { e_Func_30547(gb); }
PORT(UpdateChannel) { e_UpdateChannel(gb); }
PORT(DoAudioCommand) { e_DoAudioCommand(gb); }
PORT(UpdateVibrato) { e_UpdateVibrato(gb); }
PORT(Func_30651) { e_Func_30651(gb); }
PORT(CalculateTempo) { e_CalculateTempo(gb); }
PORT(AudioMod_Tempo) { e_AudioMod_Tempo(gb); }
PORT(AudioMod_Pitch) { e_AudioMod_Pitch(gb); }
PORT(AudioMod_VibratoSpeed) { e_AudioMod_VibratoSpeed(gb); }
PORT(AudioMod_Volume) { e_AudioMod_Volume(gb); }
PORT(AudioMod_Pan) { e_AudioMod_Pan(gb); }
PORT(AudioMod_VibratoAmplitude) { e_AudioMod_VibratoAmplitude(gb); }
PORT(Func_309b6) { e_Func_309b6(gb); }
PORT(GetWave) { e_GetWave(gb); }
PORT(Func_30bb0) { e_Func_30bb0(gb); }
PORT(Func_30bc2) { e_Func_30bc2(gb); }
PORT(Func_30bd2) { e_Func_30bd2(gb); }
PORT(Func_30be3) { e_Func_30be3(gb); }
PORT(Func_30bf2) { e_Func_30bf2(gb); }
PORT(Func_30bf6) { e_Func_30bf6(gb); }
PORT(Func_30bfb) { e_Func_30bfb(gb); }
PORT(SetTimbre) { e_SetTimbre(gb); }
PORT(SetFrequency) { e_SetFrequency(gb); }
PORT(SetSoundOutput) { e_SetSoundOutput(gb); }
PORT(SetVolumeEnvelope) { e_SetVolumeEnvelope(gb); }
PORT(TurnSoundRegisterOff) { e_TurnSoundRegisterOff(gb); }
PORT(GetNoteFrequency) { e_GetNoteFrequency(gb); }
PORT(MultiplyBByC) { e_MultiplyBByC(gb); }
PORT(MultiplyBByC_Short) { e_MultiplyBByC_Short(gb); }

PORT_JUMP(JumpToPointerInTable)
{
	R.a = f_sla(gb, R.a);
	R.a = alu_add(gb, R.a, R.c);
	R.c = R.a;
	R.a = 0;
	R.a = alu_adc(gb, R.a, R.b);
	R.b = R.a;
	R.a = rd8(gb, BC);
	R.l = R.a;
	inc_bc(gb);
	R.a = rd8(gb, BC);
	R.h = R.a;
	JUMP_TO(HL);
}

// Command handlers: those that jump back to DoAudioCommand end with that
// jump, the others return.
static void handler_exit(GB *gb, int r)
{
	// JUMP_TO rather than JUMP: a write through a garbage channel pointer
	// can switch ROM banks, and the asm then jumps into the other bank.
	if (r == NEXT_CMD)
		JUMP_TO(ADDR_DoAudioCommand);
	else
		RET();
}

#define PORT_HANDLER(name) \
	PORT_JUMP(name) { handler_exit(gb, h_##name(gb)); }

PORT_HANDLER(AudioCmd_ClearFlags)
PORT_HANDLER(Func_30727)
PORT_HANDLER(Func_3073f)
PORT_HANDLER(AudioCmd_Call)
PORT_HANDLER(AudioCmd_Jump)
PORT_HANDLER(AudioCmd_Ret)
PORT_HANDLER(AudioCmd_Loop)
PORT_HANDLER(AudioCmd_SetTempo)
PORT_HANDLER(AudioCmd_SetSemitoneOffset)
PORT_HANDLER(AudioCmd_SetWave)
PORT_HANDLER(AudioCmd_SetPitchOffset)
PORT_HANDLER(CalculatePitchProduct)
PORT_HANDLER(AudioCmd_SetPitchOffsetMultiplier)
PORT_HANDLER(AudioCmd_SetVibratoSpeed)
PORT_HANDLER(AudioCmd_SetVibratoDelay)
PORT_HANDLER(AudioCmd_SetChannelVolume)
PORT_HANDLER(AudioCmd_Pan)
PORT_HANDLER(AudioCmd_SetVibratoAmplitude)
PORT_HANDLER(SetChannelProperty)
PORT_HANDLER(AudioCmd_SetChannelVibratoDisabled)
PORT_HANDLER(AudioCmd_SetChannelUnk1d)
PORT_HANDLER(Func_3098b)
PORT_HANDLER(AudioCmd_SetChannelUnk1e)
PORT_HANDLER(AudioCmd_SetChannelUnk27)
PORT_HANDLER(AudioCmd_SetChannelUnk28)
PORT_HANDLER(AudioCmd_SetChannelTimbre)
PORT_HANDLER(AudioCmd_SetChannelDuration)
PORT_HANDLER(AudioCmd_SetChannelSweep)
PORT_HANDLER(AudioCmd_SetChannelFadeInEnvelope)
PORT_HANDLER(Func_309ce)
PORT_HANDLER(Func_309e8)
PORT_HANDLER(Func_309eb)
PORT_HANDLER(Func_30a03)
PORT_HANDLER(AudioCmd_SetChannelFadeOutEnvelope)
PORT_HANDLER(InvalidAudioCmd)
PORT_HANDLER(Func_30a10)
PORT_HANDLER(Func_30a14)
PORT_HANDLER(Func_30a1f)
PORT_HANDLER(Func_30b4d)
