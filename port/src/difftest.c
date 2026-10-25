// Differential testing of ported routines against the original asm.
#include "difftest.h"
#include "hooks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Calls tested per hook: every call up to this many, then exponentially
// spaced ones, so hot routines don't dominate the run time.
#define DENSE_SAMPLES 64
// A routine that runs longer than this is assumed to be waiting on hardware
// (VBlank, LY) that doesn't advance during an isolated run.
#define MAX_STEPS 4000000
#define MAX_REPORTED_DIFFS 12

typedef struct {
	uint64_t seen, tested, passed, failed, skipped;
	bool reported;
} HookStats;

static HookStats *stats;

static bool should_sample(uint64_t n)
{
	return n <= DENSE_SAMPLES || (n & (n - 1)) == 0;
}

// Run the asm until the CPU arrives at (pc, sp), without advancing hardware.
static bool run_asm_until(GB *gb, uint16_t pc, uint16_t sp)
{
	for (long i = 0; i < MAX_STEPS; i++) {
		if (gb->cpu.pc == pc && gb->cpu.sp == sp)
			return true;
		cpu_step(gb);
	}
	return false;
}

typedef struct {
	const char *what;
	int diffs;
	char buf[2048];
	size_t len;
} Report;

static void report(Report *r, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#include <stdarg.h>
static void report(Report *r, const char *fmt, ...)
{
	if (r->len >= sizeof(r->buf) - 1)
		return;
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(r->buf + r->len, sizeof(r->buf) - r->len, fmt, ap);
	va_end(ap);
	if (n > 0)
		r->len += (size_t)n;
	if (r->len > sizeof(r->buf) - 1)
		r->len = sizeof(r->buf) - 1;
}

// Stack memory below SP is dead once a routine returns. Native code pushes
// different return addresses there than the asm does, so it's not compared.
static uint16_t stack_bottom, dead_stack_end;

static void cmp_mem(Report *r, const char *region, const uint8_t *asm_, const uint8_t *nat,
	size_t len, uint16_t base, int bank)
{
	for (size_t i = 0; i < len; i++) {
		if (asm_[i] == nat[i])
			continue;
		if (base == 0xc000 && base + i >= stack_bottom && base + i < dead_stack_end)
			continue;
		if (r->diffs++ < MAX_REPORTED_DIFFS) {
			uint16_t addr = (uint16_t)(base + i);
			const char *name = base ? sym_name_at(bank, addr) : NULL;
			report(r, "    %s %d:%04x%s%s: asm=%02x native=%02x\n", region, bank, addr,
				name ? " " : "", name ? name : "", asm_[i], nat[i]);
		}
	}
}

#define CMP_REG(name, field) \
	if (a->cpu.field != n->cpu.field && r->diffs++ < MAX_REPORTED_DIFFS) \
		report(r, "    reg %s: asm=%x native=%x\n", name, (unsigned)a->cpu.field, (unsigned)n->cpu.field)
// Same bit order as R_* in ported.h.
#define CMP_GPR(bit, name, field) \
	if (!(clobbers & (1u << (bit)))) \
		CMP_REG(name, field)

static int compare(const GB *a, const GB *n, unsigned clobbers, Report *r)
{
	CMP_GPR(0, "a", a);
	CMP_GPR(1, "f", f);
	CMP_GPR(2, "b", b);
	CMP_GPR(3, "c", c);
	CMP_GPR(4, "d", d);
	CMP_GPR(5, "e", e);
	CMP_GPR(6, "h", h);
	CMP_GPR(7, "l", l);
	CMP_REG("sp", sp);
	CMP_REG("pc", pc);
	CMP_REG("ime", ime);
	dead_stack_end = a->cpu.sp < n->cpu.sp ? a->cpu.sp : n->cpu.sp;
	if (a->rom_bank != n->rom_bank && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    rom bank: asm=%d native=%d\n", a->rom_bank, n->rom_bank);
	if (a->sram_bank != n->sram_bank && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    sram bank: asm=%d native=%d\n", a->sram_bank, n->sram_bank);
	if (a->sram_enabled != n->sram_enabled && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    sram enable: asm=%d native=%d\n", a->sram_enabled, n->sram_enabled);
	cmp_mem(r, "wram", a->wram[0], n->wram[0], 0x1000, 0xc000, 0);
	for (int b = 1; b < 8; b++)
		cmp_mem(r, "wram", a->wram[b], n->wram[b], 0x1000, 0xd000, b);
	for (int b = 0; b < 2; b++)
		cmp_mem(r, "vram", a->vram[b], n->vram[b], 0x2000, 0x8000, b);
	for (int b = 0; b < 4; b++)
		cmp_mem(r, "sram", a->sram[b], n->sram[b], 0x2000, 0xa000, b);
	cmp_mem(r, "oam", a->oam, n->oam, sizeof(a->oam), 0xfe00, 0);
	cmp_mem(r, "io", a->io, n->io, sizeof(a->io), 0xff00, 0);
	cmp_mem(r, "hram", a->hram, n->hram, sizeof(a->hram), 0xff80, 0);
	cmp_mem(r, "bgpal", a->bg_pal, n->bg_pal, sizeof(a->bg_pal), 0, 0);
	cmp_mem(r, "objpal", a->obj_pal, n->obj_pal, sizeof(a->obj_pal), 0, 0);
	if (a->ie != n->ie && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    ie: asm=%02x native=%02x\n", a->ie, n->ie);
	if (memcmp(&a->apu.ch, &n->apu.ch, sizeof(a->apu.ch)) && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    apu channel state differs\n");
	return r->diffs;
}

static GB *ref, *nat;

static void test_call(GB *gb, Hook *h, int index)
{
	HookStats *st = &stats[index];
	st->tested++;

	// Native run. Interrupts are held off for both runs so they see the same
	// sequence of events; hooks are atomic anyway.
	*nat = *gb;
	nat->frame_cb = NULL;
	nat->no_tick = true;
	nat->cpu.ime = false;
	nat->cpu.ime_delay = 0;
	uint16_t entry_sp = nat->cpu.sp;
	uint16_t ret_addr = gb_read16(nat, entry_sp);
	hooks_set_enabled(true);
	h->fn(nat);
	if (!(h->flags & HOOK_MANUAL_RET))
		nat->cpu.pc = gb_pop16(nat);
	hooks_set_enabled(false);

	// Reference run: the original asm, from the same state.
	*ref = *gb;
	ref->frame_cb = NULL;
	ref->no_tick = true;
	ref->cpu.ime = false;
	ref->cpu.ime_delay = 0;
	bool done;
	if (h->flags & HOOK_MANUAL_RET)
		done = run_asm_until(ref, nat->cpu.pc, nat->cpu.sp);
	else
		done = run_asm_until(ref, ret_addr, (uint16_t)(entry_sp + 2));
	if (!done) {
		if (h->flags & HOOK_MANUAL_RET) {
			// Either the asm waits on hardware, or the native code exited
			// somewhere the asm never goes. Both need a look.
			st->failed++;
			if (!st->reported) {
				st->reported = true;
				printf("FAIL %s (call %llu): asm never reached the native exit %04x (sp %04x)\n",
					h->name, (unsigned long long)st->seen, nat->cpu.pc, nat->cpu.sp);
			}
		} else {
			st->skipped++;
		}
		return;
	}

	Report r = {0};
	if (compare(ref, nat, h->clobbers, &r)) {
		st->failed++;
		if (!st->reported) {
			st->reported = true;
			printf("FAIL %s (call %llu, frame %llu): %d difference%s\n", h->name,
				(unsigned long long)st->seen, (unsigned long long)gb->frame_count, r.diffs,
				r.diffs == 1 ? "" : "s");
			printf("  entry: a=%02x f=%02x bc=%04x de=%04x hl=%04x sp=%04x bank=%d\n",
				gb->cpu.a, gb->cpu.f, REG_BC(gb), REG_DE(gb), REG_HL(gb), gb->cpu.sp, gb->rom_bank);
			fwrite(r.buf, 1, r.len, stdout);
			if (r.diffs > MAX_REPORTED_DIFFS)
				printf("    ... and %d more\n", r.diffs - MAX_REPORTED_DIFFS);
		}
	} else {
		st->passed++;
	}
}

static uint8_t *script;
static int script_frames;

static void load_script(const char *path, int frames)
{
	script = calloc((size_t)frames + 1, 1);
	script_frames = frames;
	if (!path)
		return;
	FILE *f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "can't read input script %s\n", path);
		return;
	}
	char line[256];
	while (fgets(line, sizeof(line), f)) {
		int frame, duration, n;
		if (line[0] == '#' || sscanf(line, "%d %d %n", &frame, &duration, &n) != 2)
			continue;
		uint8_t b = 0;
		for (char *t = strtok(line + n, " ,+\t\r\n"); t; t = strtok(NULL, " ,+\t\r\n")) {
			static const char *names[8] = {"right", "left", "up", "down", "a", "b", "select", "start"};
			for (int i = 0; i < 8; i++)
				if (!strcmp(t, names[i]))
					b |= (uint8_t)(1 << i);
		}
		for (int i = frame; i < frame + duration && i <= frames; i++)
			if (i >= 0)
				script[i] |= b;
	}
	fclose(f);
}

static void difftest_frame(GB *gb)
{
	int f = (int)gb->frame_count;
	gb->joypad = f <= script_frames ? script[f] : 0;
	gb->apu.buffer_len = 0;
}

bool difftest_run(GB *gb, const char *input_path, int frames)
{
	int n = hook_count();
	if (!n) {
		printf("difftest: no ported routines registered\n");
		return true;
	}
	stats = calloc((size_t)n, sizeof(HookStats));
	int bank;
	if (!sym_lookup("wStackBottom", &bank, &stack_bottom))
		stack_bottom = 0xcf00;
	ref = malloc(sizeof(GB));
	nat = malloc(sizeof(GB));
	load_script(input_path, frames);

	hooks_set_enabled(false);
	gb->frame_cb = difftest_frame;
	printf("difftest: %d ported routine%s, %d frames\n", n, n == 1 ? "" : "s", frames);

	while ((int)gb->frame_count < frames) {
		Hook *h = hook_find_any(gb, gb->cpu.pc);
		if (h && !gb->cpu.halted) {
			int index = (int)(h - hook_get(0));
			if (should_sample(++stats[index].seen))
				test_call(gb, h, index);
		}
		gb_tick(gb, cpu_step(gb));
	}

	bool ok = true;
	printf("\n%-40s %10s %8s %8s %8s %8s\n", "routine", "calls", "tested", "passed", "failed", "skipped");
	for (int i = 0; i < n; i++) {
		HookStats *st = &stats[i];
		printf("%-40s %10llu %8llu %8llu %8llu %8llu\n", hook_get(i)->name,
			(unsigned long long)st->seen, (unsigned long long)st->tested,
			(unsigned long long)st->passed, (unsigned long long)st->failed,
			(unsigned long long)st->skipped);
		if (st->failed)
			ok = false;
	}
	int unexercised = 0;
	for (int i = 0; i < n; i++)
		if (!stats[i].seen)
			unexercised++;
	if (unexercised)
		printf("\n%d routine%s never called in this run; extend the input script to cover %s.\n",
			unexercised, unexercised == 1 ? " was" : "s were", unexercised == 1 ? "it" : "them");
	printf("\ndifftest: %s\n", ok ? "PASS" : "FAIL");
	return ok;
}
