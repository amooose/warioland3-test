// Wario Land 3 PC port: SDL2 frontend and command line.
#include "difftest.h"
#include "gb.h"
#include "hooks.h"
#include "trace.h"
#include "input.h"
#include "png.h"
#include "savestate.h"

#include <SDL.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WL3_DEFAULT_ROM
#define WL3_DEFAULT_ROM "warioland3.gbc"
#endif

typedef struct {
	int frame;
	const char *path;
} FrameFile;

typedef struct {
	bool headless;
	bool quit;
	int quit_wait_frames;
	bool trace; // print CPU state every frame
	uint16_t watch[16]; // RAM addresses printed every frame
	const char *watch_names[16];
	int nwatch;
	FILE *wav;
	uint32_t wav_frames;
	int stop_frame; // stop at this frame (0: run until quit)

	InputScript script;
	bool have_script;
	const char *record_path;
	InputRecorder recorder;

	FrameFile shots[64];
	int nshots;
	FrameFile state_saves[64];
	int nstate_saves;

	const char *sav_path; // battery save, or NULL to not use one
	int frames_since_sram_write;
	const char *state_path; // F5/F7 quick save state

	// Save states can only be taken or loaded between routines, so the
	// frontend asks for them and the main loop does them when it's safe.
	const char *pending_save;
	const char *pending_load;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_AudioDeviceID audio;
	SDL_GameController *pad;
	bool fast_forward;
	uint64_t next_frame_time;
} Frontend;

static uint8_t keyboard_buttons(Frontend *fe)
{
	const Uint8 *k = SDL_GetKeyboardState(NULL);
	uint8_t b = 0;
	if (k[SDL_SCANCODE_RIGHT]) b |= BTN_RIGHT;
	if (k[SDL_SCANCODE_LEFT]) b |= BTN_LEFT;
	if (k[SDL_SCANCODE_UP]) b |= BTN_UP;
	if (k[SDL_SCANCODE_DOWN]) b |= BTN_DOWN;
	if (k[SDL_SCANCODE_X]) b |= BTN_A;
	if (k[SDL_SCANCODE_Z]) b |= BTN_B;
	if (k[SDL_SCANCODE_RETURN]) b |= BTN_START;
	if (k[SDL_SCANCODE_RSHIFT] || k[SDL_SCANCODE_BACKSPACE]) b |= BTN_SELECT;
	if (fe->pad) {
		SDL_GameController *p = fe->pad;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) b |= BTN_RIGHT;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) b |= BTN_LEFT;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_UP)) b |= BTN_UP;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) b |= BTN_DOWN;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_A)) b |= BTN_A;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_B)) b |= BTN_B;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_X)) b |= BTN_B;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_START)) b |= BTN_START;
		if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_BACK)) b |= BTN_SELECT;
		int x = SDL_GameControllerGetAxis(p, SDL_CONTROLLER_AXIS_LEFTX);
		int y = SDL_GameControllerGetAxis(p, SDL_CONTROLLER_AXIS_LEFTY);
		if (x > 16000) b |= BTN_RIGHT;
		if (x < -16000) b |= BTN_LEFT;
		if (y > 16000) b |= BTN_DOWN;
		if (y < -16000) b |= BTN_UP;
	}
	// The hardware can't report opposite directions at once.
	if ((b & (BTN_LEFT | BTN_RIGHT)) == (BTN_LEFT | BTN_RIGHT))
		b &= (uint8_t)~(BTN_LEFT | BTN_RIGHT);
	if ((b & (BTN_UP | BTN_DOWN)) == (BTN_UP | BTN_DOWN))
		b &= (uint8_t)~(BTN_UP | BTN_DOWN);
	return b;
}

static void handle_events(Frontend *fe)
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			fe->quit = true;
			break;
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym) {
			case SDLK_ESCAPE:
				fe->quit = true;
				break;
			case SDLK_TAB:
				fe->fast_forward = true;
				break;
			case SDLK_F5:
				fe->pending_save = fe->state_path;
				break;
			case SDLK_F7:
				fe->pending_load = fe->state_path;
				break;
			case SDLK_F11: {
				Uint32 fs = SDL_GetWindowFlags(fe->window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
				SDL_SetWindowFullscreen(fe->window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
				break;
			}
			}
			break;
		case SDL_KEYUP:
			if (ev.key.keysym.sym == SDLK_TAB)
				fe->fast_forward = false;
			break;
		case SDL_CONTROLLERDEVICEADDED:
			if (!fe->pad)
				fe->pad = SDL_GameControllerOpen(ev.cdevice.which);
			break;
		}
	}
}

static void pace_frame(Frontend *fe, GB *gb)
{
	if (fe->audio) {
		if (!fe->fast_forward)
			SDL_QueueAudio(fe->audio, gb->apu.buffer, (Uint32)gb->apu.buffer_len * 4);
		// Pace to the audio clock: wait while more than ~3 frames are queued.
		while (!fe->fast_forward && SDL_GetQueuedAudioSize(fe->audio) > (AUDIO_SAMPLE_RATE / 60) * 4 * 3)
			SDL_Delay(1);
	}
	gb->apu.buffer_len = 0;
	if (fe->audio || fe->fast_forward)
		return;

	uint64_t freq = SDL_GetPerformanceFrequency();
	uint64_t frame = freq * GB_CYCLES_PER_FRAME / GB_CLOCK_HZ;
	uint64_t now = SDL_GetPerformanceCounter();
	if (!fe->next_frame_time || now > fe->next_frame_time + frame * 4)
		fe->next_frame_time = now;
	fe->next_frame_time += frame;
	while (SDL_GetPerformanceCounter() < fe->next_frame_time)
		SDL_Delay(1);
}

static void put_le(uint8_t *p, uint32_t v, int n)
{
	for (int i = 0; i < n; i++)
		p[i] = (uint8_t)(v >> (8 * i));
}

static void wav_header(Frontend *fe)
{
	uint8_t h[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0};
	put_le(h + 4, 36 + fe->wav_frames * 4, 4);
	put_le(h + 24, AUDIO_SAMPLE_RATE, 4);
	put_le(h + 28, AUDIO_SAMPLE_RATE * 4, 4);
	put_le(h + 32, 4, 2);
	put_le(h + 34, 16, 2);
	memcpy(h + 36, "data", 4);
	put_le(h + 40, fe->wav_frames * 4, 4);
	fseek(fe->wav, 0, SEEK_SET);
	fwrite(h, 1, sizeof(h), fe->wav);
}

// Frames are delivered from deep inside emulation (possibly nested native
// calls), so quitting exits from here rather than unwinding.
static void shutdown(Frontend *fe, GB *gb)
{
	if (fe->wav) {
		wav_header(fe);
		fclose(fe->wav);
	}
	if (fe->record_path) {
		if (recorder_save(&fe->recorder, fe->record_path))
			printf("recorded frames %d-%d to %s\n", fe->recorder.start,
				fe->recorder.start + fe->recorder.len - 1, fe->record_path);
		else
			fprintf(stderr, "can't write recording %s\n", fe->record_path);
	}
	if (fe->frames_since_sram_write >= 0 && fe->sav_path)
		gb_save_sram(gb, fe->sav_path);
	if (!fe->headless)
		SDL_Quit();
	exit(0);
}

static void start_recording(Frontend *fe, GB *gb)
{
	// Recordings always start from a full snapshot (including save RAM), so
	// they replay identically whatever the replaying machine's save file is.
	size_t len;
	uint8_t *state = savestate_save(gb, &len);
	recorder_start(&fe->recorder, (int)gb->frame_count, state, len);
}

// Runs between instructions when no native routine is in progress.
static void do_pending(Frontend *fe, GB *gb)
{
	if (fe->pending_save) {
		if (savestate_save_file(gb, fe->pending_save))
			printf("frame %llu: saved state to %s\n", (unsigned long long)gb->frame_count, fe->pending_save);
		else
			fprintf(stderr, "can't write state %s\n", fe->pending_save);
		fe->pending_save = NULL;
	}
	if (fe->pending_load) {
		if (savestate_load_file(gb, fe->pending_load)) {
			printf("frame %llu: loaded state from %s\n", (unsigned long long)gb->frame_count, fe->pending_load);
			if (fe->record_path)
				start_recording(fe, gb);
		}
		fe->pending_load = NULL;
	}
}

static void on_frame(GB *gb)
{
	Frontend *fe = gb->user;
	int frame = (int)gb->frame_count;

	for (int i = 0; i < fe->nshots; i++) {
		if (fe->shots[i].frame == frame) {
			if (png_write(fe->shots[i].path, gb->ppu.framebuffer, GB_SCREEN_W, GB_SCREEN_H))
				printf("frame %d: wrote %s\n", frame, fe->shots[i].path);
		}
	}
	for (int i = 0; i < fe->nstate_saves; i++)
		if (fe->state_saves[i].frame == frame)
			fe->pending_save = fe->state_saves[i].path;

	if (gb->sram_dirty)
		fe->frames_since_sram_write = 0;
	else if (fe->frames_since_sram_write >= 0)
		fe->frames_since_sram_write++;
	// Flush saves shortly after the game stops writing.
	if (fe->frames_since_sram_write == 30 && fe->sav_path) {
		gb_save_sram(gb, fe->sav_path);
		fe->frames_since_sram_write = -1;
	}
	gb->sram_dirty = false;

	if (fe->trace) {
		uint16_t pc = gb->cpu.pc;
		const char *name = sym_name_at(pc < 0x4000 ? 0 : gb->rom_bank, pc);
		printf("frame %d: pc=%02x:%04x (%s) sp=%04x ime=%d ie=%02x if=%02x lcdc=%02x halt=%d\n", frame,
			gb->rom_bank, pc, name ? name : "?", gb->cpu.sp, gb->cpu.ime, gb->ie, gb->io[0x0f],
			gb->io[0x40], gb->cpu.halted);
	}

	if (fe->nwatch) {
		printf("frame %d:", frame);
		for (int i = 0; i < fe->nwatch; i++)
			printf(" %s=%02x", fe->watch_names[i], gb_read(gb, fe->watch[i]));
		printf("\n");
	}

	if (fe->wav) {
		fwrite(gb->apu.buffer, 4, (size_t)gb->apu.buffer_len, fe->wav);
		fe->wav_frames += (uint32_t)gb->apu.buffer_len;
	}

	uint8_t scripted = fe->have_script ? input_buttons(&fe->script, frame) : 0;
	if (fe->headless) {
		gb->apu.buffer_len = 0;
		gb->joypad = scripted;
	} else {
		SDL_UpdateTexture(fe->texture, NULL, gb->ppu.framebuffer, GB_SCREEN_W * 4);
		SDL_RenderClear(fe->renderer);
		SDL_RenderCopy(fe->renderer, fe->texture, NULL, NULL);
		SDL_RenderPresent(fe->renderer);

		handle_events(fe);
		gb->joypad = keyboard_buttons(fe) | scripted;
		pace_frame(fe, gb);
	}
	if (fe->record_path)
		recorder_frame(&fe->recorder, frame, gb->joypad);

	if (fe->stop_frame && frame >= fe->stop_frame)
		fe->quit = true;
	// Normally the main loop quits, between instructions. If native code is
	// mid-routine and stays there, quit from here instead.
	if (fe->quit && ++fe->quit_wait_frames > 2)
		shutdown(fe, gb);
}

static bool frontend_init(Frontend *fe, int scale)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return false;
	}
	fe->window = SDL_CreateWindow("Wario Land 3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		GB_SCREEN_W * scale, GB_SCREEN_H * scale, SDL_WINDOW_RESIZABLE);
	if (!fe->window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		return false;
	}
	fe->renderer = SDL_CreateRenderer(fe->window, -1, SDL_RENDERER_ACCELERATED);
	if (!fe->renderer)
		fe->renderer = SDL_CreateRenderer(fe->window, -1, 0);
	SDL_RenderSetLogicalSize(fe->renderer, GB_SCREEN_W, GB_SCREEN_H);
	SDL_RenderSetIntegerScale(fe->renderer, SDL_TRUE);
	fe->texture = SDL_CreateTexture(fe->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, GB_SCREEN_W, GB_SCREEN_H);

	SDL_AudioSpec want = {0}, have;
	want.freq = AUDIO_SAMPLE_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	fe->audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (fe->audio)
		SDL_PauseAudioDevice(fe->audio, 0);
	else
		fprintf(stderr, "no audio: %s\n", SDL_GetError());
	return true;
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s [options]\n"
		"  --rom PATH            ROM built from the disassembly (default: " WL3_DEFAULT_ROM ")\n"
		"  --sym PATH            symbol file (default: ROM path with .sym)\n"
		"  --sav PATH            battery save file (default: ROM path with .sav;\n"
		"                        headless runs only use one if this is given)\n"
		"  --scale N             window scale (default 4)\n"
		"  --no-hooks            run everything in the interpreter\n"
		"  --hooks               use the ported routines even when recording or\n"
		"                        replaying (which otherwise use the original's timing)\n"
		"\n"
		"  --record PATH         record your input to an input script (plus PATH.state)\n"
		"  --input PATH          play an input script (see src/input.h for the format)\n"
		"  --load-state PATH     start from a save state\n"
		"  --save-state N:PATH   save a state at frame N (repeatable)\n"
		"\n"
		"  --headless            no window or audio\n"
		"  --frames N            stop after running N frames\n"
		"  --screenshot N:PATH   write a PNG of frame N (repeatable)\n"
		"  --wav PATH            record the audio output\n"
		"  --difftest            check every ported routine against the original asm\n"
		"  --coverage PATH       write how often each routine runs (with --input)\n"
		"  --fuzz                fuzz every ported routine against the original asm,\n"
		"                        seeded from an --input replay\n"
		"  --fuzz-variants N     variations per seed (default 200)\n"
		"  --fuzz-routine LABELS fuzz only these routines (comma-separated)\n"
		"  --fuzz-shard I/N      fuzz only every Nth ported routine, from the Ith\n"
		"  --fuzz-verbose        list the asm instructions the fuzzing never executed\n"
		"  --trace-write PATH    replay --input in the asm and write its per-frame trace\n"
		"  --trace-check PATH    replay --input with the ported routines and compare\n"
		"                        every game frame against the trace\n"
		"  --asm-profile PATH    with --trace-check: write what still ran as asm\n"
		"  --trace               print CPU state every frame\n"
		"  --break LABEL         print recent instructions whenever LABEL is reached\n"
		"  --watch LABEL         print the byte at LABEL every frame (repeatable)\n"
		"\n"
		"Frame numbers count from power-on, and carry on from a loaded state.\n"
		"\n"
		"keys: arrows, X = A, Z = B, Enter = Start, Backspace = Select,\n"
		"      Tab = fast forward, F5 = save state, F7 = load state,\n"
		"      F11 = fullscreen, Esc = quit\n",
		argv0);
}

static char *replace_ext(const char *path, const char *ext)
{
	size_t n = strlen(path);
	const char *dot = strrchr(path, '.');
	if (dot && !strchr(dot, '/') && !strchr(dot, '\\'))
		n = (size_t)(dot - path);
	char *out = malloc(n + strlen(ext) + 1);
	memcpy(out, path, n);
	strcpy(out + n, ext);
	return out;
}

static bool parse_frame_file(char *spec, FrameFile *out)
{
	char *colon = strchr(spec, ':');
	// Allow Windows drive letters in the path: only split at the first colon.
	if (!colon || colon == spec)
		return false;
	*colon = 0;
	out->frame = atoi(spec);
	out->path = colon + 1;
	return true;
}

int main(int argc, char **argv)
{
	const char *rom_path = WL3_DEFAULT_ROM;
	const char *sym_path = NULL, *sav_path = NULL, *input_path = NULL, *load_state = NULL;
	const char *break_sym = NULL;
	int scale = 4, run_frames = 0;
	bool no_hooks = false, force_hooks = false, difftest = false;
	const char *coverage_path = NULL, *fuzz_routine = NULL;
	const char *trace_write_path = NULL, *trace_check_path = NULL, *asm_profile_path = NULL;
	bool fuzz = false, fuzz_verbose = false;
	int fuzz_variants = 0, fuzz_shard_i = 0, fuzz_shard_n = 1;
	static Frontend fe;

	for (int i = 1; i < argc; i++) {
		const char *a = argv[i];
		const char *next = i + 1 < argc ? argv[i + 1] : NULL;
		if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
			usage(argv[0]);
			return 0;
		}
		else if (!strcmp(a, "--rom") && next) rom_path = argv[++i];
		else if (!strcmp(a, "--sym") && next) sym_path = argv[++i];
		else if (!strcmp(a, "--sav") && next) sav_path = argv[++i];
		else if (!strcmp(a, "--scale") && next) scale = atoi(argv[++i]);
		else if (!strcmp(a, "--input") && next) input_path = argv[++i];
		else if (!strcmp(a, "--record") && next) fe.record_path = argv[++i];
		else if (!strcmp(a, "--load-state") && next) load_state = argv[++i];
		else if (!strcmp(a, "--frames") && next) run_frames = atoi(argv[++i]);
		else if (!strcmp(a, "--no-hooks")) no_hooks = true;
		else if (!strcmp(a, "--hooks")) force_hooks = true;
		else if (!strcmp(a, "--headless")) fe.headless = true;
		else if (!strcmp(a, "--difftest")) difftest = true;
		else if (!strcmp(a, "--trace-write") && next) trace_write_path = argv[++i];
		else if (!strcmp(a, "--trace-check") && next) trace_check_path = argv[++i];
		else if (!strcmp(a, "--asm-profile") && next) asm_profile_path = argv[++i];
		else if (!strcmp(a, "--coverage") && next) coverage_path = argv[++i];
		else if (!strcmp(a, "--fuzz")) fuzz = true;
		else if (!strcmp(a, "--fuzz-variants") && next) fuzz_variants = atoi(argv[++i]);
		else if (!strcmp(a, "--fuzz-routine") && next) fuzz_routine = argv[++i];
		else if (!strcmp(a, "--fuzz-shard") && next) {
			if (sscanf(argv[++i], "%d/%d", &fuzz_shard_i, &fuzz_shard_n) != 2 || fuzz_shard_n < 1
				|| fuzz_shard_i < 0 || fuzz_shard_i >= fuzz_shard_n) {
				fprintf(stderr, "--fuzz-shard wants I/N with 0 <= I < N\n");
				return 1;
			}
		}
		else if (!strcmp(a, "--fuzz-verbose")) fuzz_verbose = true;
		else if (!strcmp(a, "--trace")) fe.trace = true;
		else if (!strcmp(a, "--break") && next) break_sym = argv[++i];
		else if (!strcmp(a, "--watch") && next && fe.nwatch < 16) fe.watch_names[fe.nwatch++] = argv[++i];
		else if (!strcmp(a, "--wav") && next) {
			fe.wav = fopen(argv[++i], "wb");
			if (fe.wav)
				wav_header(&fe);
		} else if (!strcmp(a, "--screenshot") && next && fe.nshots < 64) {
			if (!parse_frame_file(argv[++i], &fe.shots[fe.nshots++])) {
				usage(argv[0]);
				return 1;
			}
		} else if (!strcmp(a, "--save-state") && next && fe.nstate_saves < 64) {
			if (!parse_frame_file(argv[++i], &fe.state_saves[fe.nstate_saves++])) {
				usage(argv[0]);
				return 1;
			}
		} else {
			usage(argv[0]);
			return 1;
		}
	}

	static GB gb;
	if (!gb_init(&gb, rom_path))
		return 1;
	if (!sym_path)
		sym_path = replace_ext(rom_path, ".sym");
	if (!sym_load(sym_path)) {
		fprintf(stderr, "can't read symbols from %s (build the ROM with make first)\n", sym_path);
		return 1;
	}
	if (break_sym) {
		int bank;
		uint16_t addr;
		if (!sym_lookup(break_sym, &bank, &addr)) {
			fprintf(stderr, "unknown label %s\n", break_sym);
			return 1;
		}
		cpu_set_breakpoint(bank, addr);
	}
	for (int i = 0; i < fe.nwatch; i++) {
		int bank;
		if (!sym_lookup(fe.watch_names[i], &bank, &fe.watch[i])) {
			fprintf(stderr, "unknown label %s\n", fe.watch_names[i]);
			return 1;
		}
	}
	ported_register_all();
	hooks_init(&gb);
	// Ported routines take no emulated time, so with them the game spends
	// less time on slow frames and loading than the original does, and
	// button presses at fixed frame numbers land at different moments.
	// Recordings and replays therefore use the original's timing: then a
	// recording stays valid however much of the game gets ported, and
	// replays exactly in the difftest (which always uses that timing).
	if (no_hooks || ((fe.record_path || input_path) && !force_hooks))
		hooks_set_enabled(false);

	if (input_path) {
		if (!input_load(&fe.script, input_path)) {
			fprintf(stderr, "can't read input script %s\n", input_path);
			return 1;
		}
		fe.have_script = true;
		if (!load_state)
			load_state = fe.script.state_path;
	}

	if (difftest || coverage_path || fuzz || trace_write_path || trace_check_path) {
		if (load_state && !savestate_load_file(&gb, load_state))
			return 1;
		int end = run_frames ? (int)gb.frame_count + run_frames :
			(fe.have_script ? input_end(&fe.script) : 3000);
		const InputScript *script = fe.have_script ? &fe.script : NULL;
		if (coverage_path)
			return coverage_run(&gb, script, end, coverage_path) ? 0 : 1;
		if (trace_write_path)
			return trace_write(&gb, script, end, trace_write_path) ? 0 : 1;
		if (trace_check_path)
			return trace_check(&gb, script, trace_check_path, asm_profile_path) ? 0 : 1;
		if (fuzz) {
			fuzz_configure(fuzz_variants, fuzz_routine, fuzz_shard_i, fuzz_shard_n, fuzz_verbose);
			return fuzz_run(&gb, script, end) ? 0 : 1;
		}
		return difftest_run(&gb, script, end) ? 0 : 1;
	}

	// Headless runs are tests and tools: they leave the player's save alone
	// unless asked otherwise.
	fe.sav_path = sav_path ? sav_path : (fe.headless ? NULL : replace_ext(rom_path, ".sav"));
	if (fe.sav_path)
		gb_load_sram(&gb, fe.sav_path);
	fe.frames_since_sram_write = -1;
	fe.state_path = replace_ext(rom_path, ".state");

	if (load_state && !savestate_load_file(&gb, load_state))
		return 1;
	if (run_frames)
		fe.stop_frame = (int)gb.frame_count + run_frames;
	else if (fe.headless && fe.have_script)
		fe.stop_frame = input_end(&fe.script);
	if (fe.record_path)
		start_recording(&fe, &gb);

	if (!fe.headless && !frontend_init(&fe, scale))
		return 1;
	gb.frame_cb = on_frame;
	gb.user = &fe;

	// A reset reached from native code comes back here (see reset_unwind).
	static jmp_buf reset;
	gb.reset_unwind = &reset;
	setjmp(reset);
	for (;;) {
		if ((fe.pending_save || fe.pending_load || fe.quit) && gb.call_depth == 0) {
			do_pending(&fe, &gb);
			if (fe.quit)
				shutdown(&fe, &gb);
		}
		gb_tick(&gb, cpu_step(&gb));
	}
}
