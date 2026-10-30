// Per-frame traces; see trace.h.
//
// A game frame ends where the main loop starts over
// (InitWithoutDemoPowerUpReset.GameLoop): by then the frame's logic, its
// V-Blank work and HandleSound have all run. The native routines take far
// less emulated time than the asm, so the two runs reach that point at
// different moments, sometimes in different video frames (the asm's slow
// frames take two). Comparing there, and leaving out what only reflects
// elapsed time (LY, STAT, the timers, IF, the dead stack below SP), lets
// them match anyway. For the same reason a trace check feeds the buttons per
// game frame, as the asm run saw them, not per video frame.
#include "trace.h"
#include "hooks.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	R_WRAM0, R_WRAM1, R_WRAM2, R_WRAM3, R_WRAM4, R_WRAM5, R_WRAM6, R_WRAM7,
	R_VRAM0, R_VRAM1, R_OAM, R_HRAM, R_BG_PAL, R_OBJ_PAL, R_SRAM, R_IO, R_CPU,
	R_SCREEN, NREGIONS
};

static const char *const region_names[NREGIONS] = {
	"wram 0", "wram 1", "wram 2", "wram 3", "wram 4", "wram 5", "wram 6", "wram 7",
	"vram 0", "vram 1", "oam", "hram", "bg pals", "obj pals", "sram", "io", "cpu", "screen",
};

// The I/O registers compared: the ones the game sets, not the ones that
// count time (DIV, TIMA, IF, STAT, LY, the HDMA countdown).
static const uint8_t io_regs[] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x42, 0x43, 0x45, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4f, 0x68, 0x6a, 0x70,
};

typedef struct {
	uint32_t video_frame; // the video frame the asm run reached the point in
	uint8_t joypad;       // the buttons UpdateJoypad read in this game frame
	uint8_t pad[3];
	uint64_t hash[NREGIONS];
} Record;

#define MAGIC "WL3TRC2"

// Reads whose result depends on how long the code before them took, and
// steer the game: the asm, being slower, sees later values than native code
// does. A trace records what the asm read at each of these sites, every game
// frame, and a check hands the native code the same value: the first read
// of the register after the routine is entered.
static const struct {
	const char *label;
	uint16_t reg;
} sites[] = {
	// skips animating when the frame has run late (LY >= LY_VBLANK - 20)
	{"UpdateRoomAnimatedTiles", 0xff44},
};
#define NSITES (sizeof(sites) / sizeof(sites[0]))
static int site_bank[NSITES];
static uint16_t site_addr[NSITES];

typedef struct {
	uint32_t frame; // game frame
	uint16_t site;
	uint8_t value;
	uint8_t used;   // trace_check: handed out already
} Tap;

static Tap *taps;
static size_t ntaps, taps_cap, tap_pos;
static uint32_t cur_frame; // game frames ended so far
static bool checking;
static int pending_site = -1; // trace_write: tap armed for this site

static uint16_t sync_pc, update_joypad_pc, stack_bottom;
static const InputScript *script;
static uint64_t screen_hash;
// trace_check: the buttons of the game frame being run, once the first
// frame end is reached (before that, the script's).
static bool joypad_from_trace;
// trace_check: give up on a game frame that runs this many video frames
// longer than the asm's did (native code is never slower); 0: no limit.
static uint64_t frame_deadline;
static jmp_buf stuck; // longjmp'd to with one of:
enum { CHECK_STUCK = 1, CHECK_MISMATCH, CHECK_DONE };
static uint8_t trace_joypad;

static uint64_t fnv(uint64_t h, const void *data, size_t len)
{
	const uint8_t *p = data;
	for (size_t i = 0; i < len; i++)
		h = (h ^ p[i]) * 0x100000001b3ull;
	return h;
}

#define FNV_START 0xcbf29ce484222325ull

// The live part of WRAM bank 0: everything but the stack below SP.
static size_t dead_stack(const GB *gb, size_t *start)
{
	*start = (size_t)(stack_bottom - 0xc000);
	if (gb->cpu.sp <= stack_bottom || gb->cpu.sp > 0xd000)
		return 0;
	return (size_t)(gb->cpu.sp - stack_bottom);
}

static void cpu_bytes(const GB *gb, uint8_t out[16])
{
	const CPU *c = &gb->cpu;
	uint8_t b[16] = {c->a, c->f, c->b, c->c, c->d, c->e, c->h, c->l, (uint8_t)c->sp, (uint8_t)(c->sp >> 8),
		(uint8_t)gb->rom_bank, (uint8_t)(gb->rom_bank >> 8), (uint8_t)gb->sram_bank, gb->sram_enabled,
		c->double_speed, gb->ie};
	memcpy(out, b, 16);
}

static void io_bytes(const GB *gb, uint8_t out[sizeof(io_regs)])
{
	for (size_t i = 0; i < sizeof(io_regs); i++)
		out[i] = gb->io[io_regs[i]];
}

static void hash_state(const GB *gb, uint64_t h[NREGIONS])
{
	size_t start, dead = dead_stack(gb, &start);
	h[R_WRAM0] = fnv(fnv(FNV_START, gb->wram[0], start), gb->wram[0] + start + dead, 0x1000 - start - dead);
	for (int b = 1; b < 8; b++)
		h[R_WRAM0 + b] = fnv(FNV_START, gb->wram[b], 0x1000);
	h[R_VRAM0] = fnv(FNV_START, gb->vram[0], 0x2000);
	h[R_VRAM1] = fnv(FNV_START, gb->vram[1], 0x2000);
	h[R_OAM] = fnv(FNV_START, gb->oam, sizeof(gb->oam));
	h[R_HRAM] = fnv(FNV_START, gb->hram, sizeof(gb->hram));
	h[R_BG_PAL] = fnv(FNV_START, gb->bg_pal, sizeof(gb->bg_pal));
	h[R_OBJ_PAL] = fnv(FNV_START, gb->obj_pal, sizeof(gb->obj_pal));
	h[R_SRAM] = fnv(FNV_START, gb->sram, sizeof(gb->sram));
	uint8_t io[sizeof(io_regs)], cpu[16];
	io_bytes(gb, io);
	cpu_bytes(gb, cpu);
	h[R_IO] = fnv(FNV_START, io, sizeof(io));
	h[R_CPU] = fnv(FNV_START, cpu, sizeof(cpu));
	h[R_SCREEN] = screen_hash;
}

static void trace_frame(GB *gb)
{
	gb->joypad = joypad_from_trace ? trace_joypad : (script ? input_buttons(script, (int)gb->frame_count) : 0);
	gb->apu.buffer_len = 0;
	screen_hash = fnv(FNV_START, gb->ppu.framebuffer, GB_SCREEN_W * GB_SCREEN_H * sizeof(uint32_t));
	// called from wherever the CPU is, native routines' wait loops included
	if (frame_deadline && gb->frame_count > frame_deadline)
		longjmp(stuck, CHECK_STUCK);
}

static int site_at(const GB *gb)
{
	uint16_t pc = gb->cpu.pc;
	for (size_t s = 0; s < NSITES; s++)
		if (pc == site_addr[s] && (pc < 0x4000 || gb->rom_bank == site_bank[s]))
			return (int)s;
	return -1;
}

static void record_tap(GB *gb)
{
	// the armed read happened: log what it returned
	if (pending_site >= 0 && !gb->tap_addr) {
		if (ntaps == taps_cap)
			taps = realloc(taps, (taps_cap = taps_cap ? taps_cap * 2 : 1024) * sizeof(Tap));
		taps[ntaps++] = (Tap){cur_frame, (uint16_t)pending_site, gb->tap_value, 0};
		pending_site = -1;
	}
	int s = site_at(gb);
	if (s >= 0 && pending_site < 0) {
		gb->tap_addr = sites[s].reg;
		gb->tap_override = false;
		pending_site = s;
	}
}

static void replay_tap(GB *gb)
{
	int s = site_at(gb);
	if (s < 0)
		return;
	while (tap_pos < ntaps && taps[tap_pos].frame < cur_frame)
		tap_pos++;
	for (size_t t = tap_pos; t < ntaps && taps[t].frame == cur_frame; t++) {
		if (taps[t].site == s && !taps[t].used) {
			taps[t].used = 1;
			gb->tap_addr = sites[s].reg;
			gb->tap_override = true;
			gb->tap_value = taps[t].value;
			return;
		}
	}
}

// Two kinds of read the native code makes at the same points as the asm,
// in the same order, but can't get the same value from, so a trace logs
// every one and a check hands them back in turn:
// - DIV, only ever read as a random number (never waited on);
// - the stack as data (not by push, pop, call or ret), which only the
//   original's own bugs do: the tile queue overrun in
//   QueueBlockTilesAlongRow_* reads its callers' return addresses. What's
//   there differs, since native routines don't push what the asm does.
#define SITE_DIV 0xffff
#define SITE_STACK 0xfffe
static size_t stream_pos[2]; // per kind: the next log entry to look from

static uint8_t stream_read(uint16_t kind, uint8_t value)
{
	if (!checking) {
		if (ntaps == taps_cap)
			taps = realloc(taps, (taps_cap = taps_cap ? taps_cap * 2 : 1024) * sizeof(Tap));
		taps[ntaps++] = (Tap){cur_frame, kind, value, 0};
		return value;
	}
	size_t *pos = &stream_pos[kind == SITE_STACK];
	while (*pos < ntaps && (taps[*pos].frame < cur_frame || (taps[*pos].frame == cur_frame &&
		(taps[*pos].site != kind || taps[*pos].used))))
		(*pos)++;
	if (*pos < ntaps && taps[*pos].frame == cur_frame) {
		taps[*pos].used = 1;
		return taps[*pos].value;
	}
	return value; // more reads than the asm made: the runs have diverged
}

static uint8_t trace_read(GB *gb, uint16_t addr, uint8_t value)
{
	if (addr == 0xff04)
		return stream_read(SITE_DIV, value);
	if (addr >= stack_bottom && addr < 0xd000 && !gb->test.in_stack_op)
		return stream_read(SITE_STACK, value);
	return value;
}

static void check_frame_end(GB *gb);

static void trace_pc(GB *gb)
{
	if (checking) {
		if (gb->cpu.pc == sync_pc)
			check_frame_end(gb);
		replay_tap(gb);
	} else {
		record_tap(gb);
	}
}

// A game frame ended: a tap armed in it no longer applies.
static void end_game_frame(GB *gb)
{
	gb->tap_addr = 0;
	pending_site = -1;
	cur_frame++;
}

// trace_check: the trace, and how far the check got. The frame ends are
// caught before each instruction (trace_pc), wherever the CPU is: after a
// game over the game restarts its main loop from inside a native routine's
// call, which never returns.
static const Record *chk_recs;
static size_t chk_n, chk_i;
static uint64_t chk_h[NREGIONS];
// Game frames whose last drawn video frame differs, though everything the
// screen is drawn from matches: a split screen timed by an interrupt, or a
// frame the asm drew mid-update, comes out differently at other moments. It
// doesn't fail a check.
static size_t screen_diffs, first_screen_diff;

static void check_frame_end(GB *gb)
{
	if (gb->cpu.halted)
		return;
	hash_state(gb, chk_h);
	if (memcmp(chk_h, chk_recs[chk_i].hash, R_SCREEN * sizeof(*chk_h)))
		longjmp(stuck, CHECK_MISMATCH);
	if (chk_h[R_SCREEN] != chk_recs[chk_i].hash[R_SCREEN] && !screen_diffs++)
		first_screen_diff = chk_i;
	joypad_from_trace = true;
	gb->joypad = trace_joypad = chk_recs[chk_i].joypad;
	end_game_frame(gb);
	if (++chk_i == chk_n)
		longjmp(stuck, CHECK_DONE);
	// Native code is never slower than the asm: this far behind, it's stuck
	// somewhere the asm wasn't.
	frame_deadline = chk_recs[chk_i].video_frame + 600;
}

static bool setup(void)
{
	int bank;
	uint16_t addr;
	if (!sym_lookup("InitWithoutDemoPowerUpReset.GameLoop", &bank, &sync_pc) ||
		!sym_lookup("UpdateJoypad", &bank, &update_joypad_pc)) {
		fprintf(stderr, "trace: the main loop's symbols aren't in the .sym file\n");
		return false;
	}
	stack_bottom = sym_lookup("wStackBottom", &bank, &addr) ? addr : 0xcf00;
	for (size_t s = 0; s < NSITES; s++) {
		if (!sym_lookup(sites[s].label, &site_bank[s], &site_addr[s])) {
			fprintf(stderr, "trace: %s isn't in the .sym file\n", sites[s].label);
			return false;
		}
	}
	cur_frame = 0;
	pending_site = -1;
	tap_pos = 0;
	stream_pos[0] = stream_pos[1] = 0;
	return true;
}

static bool at_frame_end(const GB *gb)
{
	return gb->cpu.pc == sync_pc && !gb->cpu.halted && gb->call_depth == 0;
}

bool trace_write(GB *gb, const InputScript *input, int end_frame, const char *path)
{
	if (!setup())
		return false;
	script = input;
	joypad_from_trace = false;
	checking = false;
	ntaps = 0;
	hooks_set_enabled(false);
	gb->frame_cb = trace_frame;
	gb->pc_cb = trace_pc;
	gb->read_cb = trace_read;

	size_t n = 0, cap = 1024;
	Record *recs = malloc(cap * sizeof(Record));
	bool want_joypad = false;
	while ((int)gb->frame_count < end_frame) {
		if (at_frame_end(gb)) {
			if (n == cap)
				recs = realloc(recs, (cap *= 2) * sizeof(Record));
			Record *r = &recs[n++];
			memset(r, 0, sizeof(*r));
			r->video_frame = (uint32_t)gb->frame_count;
			r->joypad = gb->joypad;
			hash_state(gb, r->hash);
			end_game_frame(gb);
			want_joypad = true;
		} else if (want_joypad && gb->cpu.pc == update_joypad_pc) {
			recs[n - 1].joypad = gb->joypad;
			want_joypad = false;
		}
		gb_tick(gb, cpu_step(gb));
	}

	FILE *f = fopen(path, "wb");
	if (!f) {
		fprintf(stderr, "trace: can't write %s\n", path);
		free(recs);
		return false;
	}
	uint32_t hdr[3] = {NREGIONS, (uint32_t)n, (uint32_t)ntaps};
	fwrite(MAGIC, 1, 8, f);
	fwrite(hdr, sizeof(hdr), 1, f);
	fwrite(recs, sizeof(Record), n, f);
	fwrite(taps, sizeof(Tap), ntaps, f);
	fclose(f);
	printf("trace: wrote %zu game frames (video frames to %d) and %zu timing reads to %s\n", n, end_frame,
		ntaps, path);
	free(recs);
	return true;
}

static Record *read_trace(const char *path, size_t *n)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "trace: can't read %s\n", path);
		return NULL;
	}
	char magic[8];
	uint32_t hdr[3];
	Record *recs = NULL;
	if (fread(magic, 1, 8, f) == 8 && !memcmp(magic, MAGIC, 8) && fread(hdr, sizeof(hdr), 1, f) == 1 &&
		hdr[0] == NREGIONS) {
		recs = malloc((hdr[1] ? hdr[1] : 1) * sizeof(Record));
		taps = realloc(taps, (hdr[2] ? hdr[2] : 1) * sizeof(Tap));
		if (fread(recs, sizeof(Record), hdr[1], f) == hdr[1] && fread(taps, sizeof(Tap), hdr[2], f) == hdr[2]) {
			*n = hdr[1];
			ntaps = hdr[2];
		} else {
			free(recs);
			recs = NULL;
		}
	}
	fclose(f);
	if (!recs)
		fprintf(stderr, "trace: %s isn't a trace from this build (rewrite it)\n", path);
	return recs;
}

// Print up to `max` differing bytes of a region.
static void diff_bytes(const char *what, int bank, uint16_t base, const uint8_t *ref, const uint8_t *nat, size_t len,
	size_t skip_start, size_t skip_len)
{
	int shown = 0, total = 0;
	for (size_t i = 0; i < len; i++) {
		if ((i >= skip_start && i < skip_start + skip_len) || ref[i] == nat[i])
			continue;
		if (shown++ < 12) {
			uint16_t addr = (uint16_t)(base + i);
			const char *name = sym_name_at(bank, addr);
			printf("    %s %x:%04x %s: asm=%02x native=%02x\n", what, bank, addr, name ? name : "", ref[i], nat[i]);
		}
		total++;
	}
	if (total > 12)
		printf("    ... and %d more in %s\n", total - 12, what);
}

static void diff_states(const GB *ref, const GB *nat, const uint64_t ref_hash[NREGIONS], const uint64_t nat_hash[NREGIONS])
{
	for (int r = 0; r < NREGIONS; r++) {
		if (ref_hash[r] == nat_hash[r])
			continue;
		size_t start, dead;
		switch (r) {
		case R_WRAM0:
			dead = dead_stack(ref, &start);
			diff_bytes("wram", 0, 0xc000, ref->wram[0], nat->wram[0], 0x1000, start, dead);
			break;
		case R_WRAM1: case R_WRAM2: case R_WRAM3: case R_WRAM4: case R_WRAM5: case R_WRAM6: case R_WRAM7:
			diff_bytes("wram", r - R_WRAM0, 0xd000, ref->wram[r - R_WRAM0], nat->wram[r - R_WRAM0], 0x1000, 0, 0);
			break;
		case R_VRAM0: case R_VRAM1:
			diff_bytes("vram", r - R_VRAM0, 0x8000, ref->vram[r - R_VRAM0], nat->vram[r - R_VRAM0], 0x2000, 0, 0);
			break;
		case R_OAM:
			diff_bytes("oam", 0, 0xfe00, ref->oam, nat->oam, sizeof(ref->oam), 0, 0);
			break;
		case R_HRAM:
			diff_bytes("hram", 0, 0xff80, ref->hram, nat->hram, sizeof(ref->hram), 0, 0);
			break;
		case R_BG_PAL:
			diff_bytes("bg pal", 0, 0, ref->bg_pal, nat->bg_pal, sizeof(ref->bg_pal), 0, 0);
			break;
		case R_OBJ_PAL:
			diff_bytes("obj pal", 0, 0, ref->obj_pal, nat->obj_pal, sizeof(ref->obj_pal), 0, 0);
			break;
		case R_SRAM:
			for (int b = 0; b < GB_SRAM_BANKS; b++)
				diff_bytes("sram", b, 0xa000, ref->sram[b], nat->sram[b], 0x2000, 0, 0);
			break;
		case R_IO: {
			uint8_t a[sizeof(io_regs)], b[sizeof(io_regs)];
			io_bytes(ref, a);
			io_bytes(nat, b);
			for (size_t i = 0; i < sizeof(io_regs); i++)
				if (a[i] != b[i])
					printf("    io ff%02x: asm=%02x native=%02x\n", io_regs[i], a[i], b[i]);
			break;
		}
		case R_CPU: {
			static const char *const names[16] = {"a", "f", "b", "c", "d", "e", "h", "l", "sp lo", "sp hi",
				"rom bank", "rom bank hi", "sram bank", "sram enabled", "double speed", "ie"};
			uint8_t a[16], b[16];
			cpu_bytes(ref, a);
			cpu_bytes(nat, b);
			for (int i = 0; i < 16; i++)
				if (a[i] != b[i])
					printf("    %s: asm=%02x native=%02x\n", names[i], a[i], b[i]);
			break;
		}
		case R_SCREEN:
			printf("    the last video frame drawn differs\n");
			break;
		}
	}
}

bool trace_check(GB *gb, const InputScript *input, const char *path)
{
	if (!setup())
		return false;
	size_t n;
	Record *recs = read_trace(path, &n);
	if (!recs)
		return false;
	script = input;

	// The starting state, to rerun the asm from for the details of a mismatch.
	GB *start = malloc(sizeof(GB));
	*start = *gb;

	joypad_from_trace = false;
	checking = true;
	hooks_set_enabled(true);
	gb->frame_cb = trace_frame;
	gb->pc_cb = trace_pc;
	gb->read_cb = trace_read;
	chk_recs = recs;
	chk_n = n;
	chk_i = 0;
	uint64_t *h = chk_h;
	bool ok = true;
	frame_deadline = n ? recs[0].video_frame + 600 : 0;
	switch (setjmp(stuck)) {
	case 0: {
		if (!n)
			break;
		// as in normal play, a reset from native code unwinds to here
		static jmp_buf reset;
		gb->reset_unwind = &reset;
		setjmp(reset);
		for (;;)
			gb_tick(gb, cpu_step(gb));
	}
	case CHECK_STUCK: {
		gb->reset_unwind = NULL;
		printf("trace: game frame %zu never ends (the asm ended it in video frame %u)\n", chk_i,
			recs[chk_i].video_frame);
		const char *name = sym_name_at(gb->cpu.pc < 0x4000 ? 0 : gb->rom_bank, gb->cpu.pc);
		printf("  stuck at %02x:%04x (%s), %d native routines deep\n", gb->cpu.pc < 0x4000 ? 0 : gb->rom_bank,
			gb->cpu.pc, name ? name : "?", gb->call_depth);
		hash_state(gb, h);
		ok = false;
		break;
	}
	case CHECK_MISMATCH:
		ok = false;
		break;
	case CHECK_DONE:
		break;
	}
	size_t i = chk_i;
	frame_deadline = 0;

	if (screen_diffs)
		printf("trace: the screen alone differs in %zu game frames, the first at %zu\n", screen_diffs,
			first_screen_diff);
	if (ok) {
		printf("trace: PASS, all %zu game frames match\n", n);
	} else {
		printf("trace: FAIL at game frame %zu of %zu (video frame %u in the asm, %llu here); "
			"the first %zu match. Differences:\n",
			i, n, recs[i].video_frame, (unsigned long long)gb->frame_count, i);
		// Rerun the asm from the start to the same game frame.
		GB *nat = malloc(sizeof(GB));
		*nat = *gb;
		*gb = *start; // pure asm from here: no timing reads to hand out
		joypad_from_trace = false;
		hooks_set_enabled(false);
		gb->frame_cb = trace_frame;
		size_t at = 0;
		while (!(at_frame_end(gb) && at == i)) {
			if (at_frame_end(gb))
				at++;
			gb_tick(gb, cpu_step(gb));
		}
		uint64_t ref_h[NREGIONS];
		hash_state(gb, ref_h);
		for (int r = 0; r < NREGIONS; r++)
			if (ref_h[r] != h[r])
				printf("  %s\n", region_names[r]);
		diff_states(gb, nat, ref_h, h);
		free(nat);
	}
	free(start);
	free(recs);
	return ok;
}
