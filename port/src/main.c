// Wario Land 3 PC port: SDL2 frontend and command line.
#include "difftest.h"
#include "gb.h"
#include "hooks.h"
#include "png.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WL3_DEFAULT_ROM
#define WL3_DEFAULT_ROM "warioland3.gbc"
#endif

typedef struct {
	int frame, duration;
	uint8_t buttons;
} InputEvent;

typedef struct {
	int frame;
	const char *path;
} Screenshot;

typedef struct {
	bool headless;
	bool quit;
	bool trace; // print CPU state every frame
	FILE *wav;
	uint32_t wav_frames;
	int max_frames; // headless: stop after this many frames
	InputEvent *events;
	int nevents;
	Screenshot shots[64];
	int nshots;
	const char *sav_path;
	int frames_since_sram_write;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_AudioDeviceID audio;
	SDL_GameController *pad;
	bool fast_forward;
	uint64_t next_frame_time;
} Frontend;

static uint8_t parse_buttons(char *s)
{
	uint8_t b = 0;
	for (char *tok = strtok(s, " ,+\t\r\n"); tok; tok = strtok(NULL, " ,+\t\r\n")) {
		if (!strcmp(tok, "a")) b |= BTN_A;
		else if (!strcmp(tok, "b")) b |= BTN_B;
		else if (!strcmp(tok, "start")) b |= BTN_START;
		else if (!strcmp(tok, "select")) b |= BTN_SELECT;
		else if (!strcmp(tok, "up")) b |= BTN_UP;
		else if (!strcmp(tok, "down")) b |= BTN_DOWN;
		else if (!strcmp(tok, "left")) b |= BTN_LEFT;
		else if (!strcmp(tok, "right")) b |= BTN_RIGHT;
		else fprintf(stderr, "input: unknown button '%s'\n", tok);
	}
	return b;
}

// Input script: one event per line, "<frame> <duration> <buttons...>".
// Lines starting with # are comments.
static bool load_input_script(Frontend *fe, const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return false;
	char line[256];
	int cap = 0;
	while (fgets(line, sizeof(line), f)) {
		int frame, duration, n;
		if (line[0] == '#' || sscanf(line, "%d %d %n", &frame, &duration, &n) != 2)
			continue;
		if (fe->nevents == cap) {
			cap = cap ? cap * 2 : 64;
			fe->events = realloc(fe->events, (size_t)cap * sizeof(InputEvent));
		}
		fe->events[fe->nevents++] = (InputEvent){frame, duration, parse_buttons(line + n)};
	}
	fclose(f);
	return true;
}

static uint8_t scripted_buttons(Frontend *fe, int frame)
{
	uint8_t b = 0;
	for (int i = 0; i < fe->nevents; i++) {
		InputEvent *e = &fe->events[i];
		if (frame >= e->frame && frame < e->frame + e->duration)
			b |= e->buttons;
	}
	return b;
}

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
			if (ev.key.keysym.sym == SDLK_ESCAPE)
				fe->quit = true;
			if (ev.key.keysym.sym == SDLK_TAB)
				fe->fast_forward = true;
			if (ev.key.keysym.sym == SDLK_F11) {
				Uint32 fs = SDL_GetWindowFlags(fe->window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
				SDL_SetWindowFullscreen(fe->window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
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

// Frames are delivered from deep inside emulation (possibly nested native
// calls), so quitting exits from here rather than unwinding.
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

static void shutdown(Frontend *fe, GB *gb)
{
	if (fe->wav) {
		wav_header(fe);
		fclose(fe->wav);
	}
	if (fe->frames_since_sram_write >= 0 && fe->sav_path)
		gb_save_sram(gb, fe->sav_path);
	if (!fe->headless)
		SDL_Quit();
	exit(0);
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

	if (fe->wav) {
		fwrite(gb->apu.buffer, 4, (size_t)gb->apu.buffer_len, fe->wav);
		fe->wav_frames += (uint32_t)gb->apu.buffer_len;
	}

	if (fe->headless) {
		gb->apu.buffer_len = 0;
		gb->joypad = scripted_buttons(fe, frame);
		if (fe->max_frames && frame >= fe->max_frames)
			shutdown(fe, gb);
		return;
	}

	SDL_UpdateTexture(fe->texture, NULL, gb->ppu.framebuffer, GB_SCREEN_W * 4);
	SDL_RenderClear(fe->renderer);
	SDL_RenderCopy(fe->renderer, fe->texture, NULL, NULL);
	SDL_RenderPresent(fe->renderer);

	handle_events(fe);
	gb->joypad = keyboard_buttons(fe) | scripted_buttons(fe, frame);
	pace_frame(fe, gb);
	if (fe->quit || (fe->max_frames && frame >= fe->max_frames))
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
		"  --rom PATH          ROM built from the disassembly (default: " WL3_DEFAULT_ROM ")\n"
		"  --sym PATH          symbol file (default: ROM path with .sym)\n"
		"  --sav PATH          save file (default: ROM path with .sav)\n"
		"  --scale N           window scale (default 4)\n"
		"  --no-hooks          run everything in the interpreter\n"
		"  --input PATH        scripted input: lines of '<frame> <duration> <buttons>'\n"
		"  --headless          no window or audio\n"
		"  --frames N          stop after N frames\n"
		"  --screenshot N:PATH write a PNG of frame N (repeatable)\n"
		"  --difftest          check every ported routine against the original asm\n"
		"  --trace             print CPU state every frame\n"
		"  --wav PATH          record the audio output\n"
		"  --break LABEL       print recent instructions whenever LABEL is reached\n"
		"\n"
		"keys: arrows, X = A, Z = B, Enter = Start, Backspace = Select,\n"
		"      Tab = fast forward, F11 = fullscreen, Esc = quit\n",
		argv0);
}

static char *replace_ext(const char *path, const char *ext)
{
	size_t n = strlen(path);
	const char *dot = strrchr(path, '.');
	if (dot && !strchr(dot, '/'))
		n = (size_t)(dot - path);
	char *out = malloc(n + strlen(ext) + 1);
	memcpy(out, path, n);
	strcpy(out + n, ext);
	return out;
}

int main(int argc, char **argv)
{
	const char *rom_path = WL3_DEFAULT_ROM;
	const char *sym_path = NULL, *sav_path = NULL, *input_path = NULL;
	int scale = 4;
	bool no_hooks = false, difftest = false;
	const char *break_sym = NULL;
	static Frontend fe;

	for (int i = 1; i < argc; i++) {
		const char *a = argv[i];
		const char *next = i + 1 < argc ? argv[i + 1] : NULL;
		if (!strcmp(a, "--rom") && next) rom_path = argv[++i];
		else if (!strcmp(a, "--sym") && next) sym_path = argv[++i];
		else if (!strcmp(a, "--sav") && next) sav_path = argv[++i];
		else if (!strcmp(a, "--scale") && next) scale = atoi(argv[++i]);
		else if (!strcmp(a, "--input") && next) input_path = argv[++i];
		else if (!strcmp(a, "--frames") && next) fe.max_frames = atoi(argv[++i]);
		else if (!strcmp(a, "--no-hooks")) no_hooks = true;
		else if (!strcmp(a, "--headless")) fe.headless = true;
		else if (!strcmp(a, "--difftest")) difftest = true;
		else if (!strcmp(a, "--trace")) fe.trace = true;
		else if (!strcmp(a, "--wav") && next) {
			fe.wav = fopen(argv[++i], "wb");
			if (fe.wav)
				wav_header(&fe);
		}
		else if (!strcmp(a, "--break") && next) break_sym = argv[++i];
		else if (!strcmp(a, "--screenshot") && next && fe.nshots < 64) {
			char *spec = argv[++i];
			char *colon = strchr(spec, ':');
			if (!colon) {
				usage(argv[0]);
				return 1;
			}
			*colon = 0;
			fe.shots[fe.nshots++] = (Screenshot){atoi(spec), colon + 1};
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
	ported_register_all();
	hooks_init(&gb);
	if (no_hooks)
		hooks_set_enabled(false);

	if (difftest)
		return difftest_run(&gb, input_path, fe.max_frames ? fe.max_frames : 3000) ? 0 : 1;

	if (input_path && !load_input_script(&fe, input_path)) {
		fprintf(stderr, "can't read input script %s\n", input_path);
		return 1;
	}
	fe.sav_path = sav_path ? sav_path : replace_ext(rom_path, ".sav");
	gb_load_sram(&gb, fe.sav_path);
	fe.frames_since_sram_write = -1;

	if (!fe.headless && !frontend_init(&fe, scale))
		return 1;
	gb.frame_cb = on_frame;
	gb.user = &fe;

	for (;;)
		gb_tick(&gb, cpu_step(&gb));
}
