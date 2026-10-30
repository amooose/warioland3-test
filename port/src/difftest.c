// Differential testing of ported routines against the original asm.
//
// The core (check) runs a ported routine and the original asm from the same
// machine state, then compares the results. Two drivers use it:
// - difftest_run replays an input script and checks real calls as they
//   happen;
// - fuzz_run also replays one, but from each sampled call it generates many
//   variations (random registers, mutated memory the routine reads) and
//   checks those, and it measures how much of each routine's asm the
//   checks executed.
#include "difftest.h"
#include "hooks.h"
#include "input.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Calls tested per hook by the replay difftest: every call up to this many,
// then exponentially spaced ones, so hot routines don't dominate.
#define DENSE_SAMPLES 64
// A routine that runs longer than this is assumed to be waiting on hardware
// (VBlank, LY) that doesn't advance during an isolated run.
#define MAX_STEPS 4000000
// Memory accesses native code may make in one check before it's considered
// stuck in a loop (fuzzed inputs can make loops run forever).
#define NATIVE_BUDGET 8000000
#define MAX_REPORTED_DIFFS 12

typedef struct {
	int diffs;
	char buf[2048];
	size_t len;
} Report;

#ifdef __GNUC__
static void report(Report *r, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#endif
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

// Checks normally run with emulated time frozen, so results don't depend on
// how long the native code takes. A routine that waits on the hardware
// (polls rLY or rSTAT) can't finish that way; its checks are rerun with
// time flowing on both sides, and the registers that only reflect the
// passage of time aren't compared.
static bool timed;
static bool fuzzed;

// Stack memory below SP is dead once a routine returns. Native code pushes
// different return addresses there than the asm does, so it's not compared.
static uint16_t stack_bottom, stack_top, dead_stack_end, wrom_bank;

static void cmp_mem(Report *r, const char *region, const uint8_t *asm_, const uint8_t *nat,
	size_t len, uint16_t base, int bank)
{
	if (!memcmp(asm_, nat, len))
		return;
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
	for (int b = 0; b < GB_SRAM_BANKS; b++)
		cmp_mem(r, "sram", a->sram[b], n->sram[b], 0x2000, 0xa000, b);
	cmp_mem(r, "oam", a->oam, n->oam, sizeof(a->oam), 0xfe00, 0);
	if (timed) {
		// DIV, TIMA, IF, LY and the HDMA countdown only reflect how much time
		// passed, which differs between native code and the asm.
		static const uint8_t timing_regs[] = {0x04, 0x05, 0x0f, 0x44, 0x55};
		uint8_t ia[sizeof(a->io)], in[sizeof(n->io)];
		memcpy(ia, a->io, sizeof(ia));
		memcpy(in, n->io, sizeof(in));
		for (size_t i = 0; i < sizeof(timing_regs); i++)
			ia[timing_regs[i]] = in[timing_regs[i]] = 0;
		cmp_mem(r, "io", ia, in, sizeof(ia), 0xff00, 0);
	} else {
		cmp_mem(r, "io", a->io, n->io, sizeof(a->io), 0xff00, 0);
	}
	cmp_mem(r, "hram", a->hram, n->hram, sizeof(a->hram), 0xff80, 0);
	cmp_mem(r, "bgpal", a->bg_pal, n->bg_pal, sizeof(a->bg_pal), 0, 0);
	cmp_mem(r, "objpal", a->obj_pal, n->obj_pal, sizeof(a->obj_pal), 0, 0);
	if (a->ie != n->ie && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    ie: asm=%02x native=%02x\n", a->ie, n->ie);
	// (The sound channels' internal state runs on time too; the sound
	// registers the code writes are compared as I/O above.)
	if (!timed && memcmp(&a->apu.ch, &n->apu.ch, sizeof(a->apu.ch)) && r->diffs++ < MAX_REPORTED_DIFFS)
		report(r, "    apu channel state differs\n");
	return r->diffs;
}

typedef enum {
	CHECK_PASS,
	CHECK_FAIL,
	// The asm never finishes without hardware time passing (it waits for
	// VBlank, LY or an interrupt), or a fuzzed input sent it somewhere it
	// can't run: nothing to compare.
	CHECK_INCONCLUSIVE,
} CheckResult;

static GB *ref, *nat;
static jmp_buf native_escape;
static uint64_t native_budget = NATIVE_BUDGET, native_accesses;

static void native_out_of_budget(GB *gb)
{
	(void)gb;
	longjmp(native_escape, 1);
}

// Run the asm until the CPU arrives at (pc, sp), without advancing hardware.
// *left is set if the asm unwound past entry_sp on the way, i.e. it returned
// from the routine (somewhere other than (pc, sp)).
static long asm_step_limit = MAX_STEPS, asm_steps_taken;

// The routine being checked, in its bank: its own code running with another
// bank mapped means the asm is executing garbage (a fuzzed bank number
// restored before a ret, say). Other banks' code at the same addresses,
// reached the normal way (a farcall that switched the bank in ROM0 and
// called it), is fine.
static int body_bank;
static uint16_t body_start, body_end;

// The ROM bank mapped when each return address on the WRAM stack was pushed
// by the asm run, valid when push_gen matches asm_gen.
static uint16_t push_bank[0x2000];
static uint32_t push_gen[0x2000], asm_gen;

static uint8_t rom_opcode(const GB *gb, uint16_t at)
{
	if (at < 0x4000)
		return gb->rom[at];
	if (at < 0x8000)
		return gb->rom[(size_t)(gb->rom_bank % gb->rom_banks) * 0x4000 + (at - 0x4000)];
	return 0x00; // code in RAM: not tracked
}

static bool is_call(uint8_t op)
{
	return op == 0xcd || op == 0xc4 || op == 0xcc || op == 0xd4 || op == 0xdc || (op & 0xc7) == 0xc7;
}

static bool is_ret(uint8_t op)
{
	return op == 0xc9 || op == 0xd9 || op == 0xc0 || op == 0xc8 || op == 0xd0 || op == 0xd8;
}

static bool stop_if_unrealistic;

static bool run_asm_until(GB *gb, uint16_t pc, uint16_t sp, uint16_t entry_sp, bool *left)
{
	*left = false;
	asm_gen++;
	for (asm_steps_taken = 0; asm_steps_taken < asm_step_limit && !gb->test.crashed; asm_steps_taken++) {
		if (stop_if_unrealistic && gb->test.unrealistic)
			break;
		uint16_t at = gb->cpu.pc;
		if (at == pc && gb->cpu.sp == sp)
			return true;
		if (gb->cpu.sp > entry_sp)
			*left = true;
		uint8_t op = rom_opcode(gb, at);
		uint16_t sp_before = gb->cpu.sp;
		int bank_before = gb->rom_bank;
		int cycles = cpu_step(gb);
		if (timed)
			gb_tick(gb, cycles);
		uint16_t now = gb->cpu.pc;
		if (is_call(op) && gb->cpu.sp == (uint16_t)(sp_before - 2) && gb->cpu.sp >= 0xc000 && gb->cpu.sp < 0xe000) {
			push_bank[gb->cpu.sp & 0x1fff] = (uint16_t)gb->rom_bank;
			push_gen[gb->cpu.sp & 0x1fff] = asm_gen;
		}
		if (now < 0x4000 || now >= 0x8000 || now < body_start || now >= body_end || gb->rom_bank == body_bank)
			continue;
		// In the routine's range with another bank mapped. Unrealistic if
		// ROMX code switched the bank under itself, or a ret came back with
		// another bank than the one its call was made in.
		if (at >= 0x4000 && at < 0x8000 && gb->rom_bank != bank_before)
			gb->test.unrealistic = true;
		if (is_ret(op) && gb->cpu.sp == (uint16_t)(sp_before + 2)) {
			unsigned i = sp_before & 0x1fff;
			if (sp_before < 0xc000 || sp_before >= 0xe000 || push_gen[i] != asm_gen || push_bank[i] != gb->rom_bank)
				gb->test.unrealistic = true;
		}
	}
	return false;
}

static void prepare(GB *dst, const GB *src)
{
	*dst = *src;
	dst->frame_cb = NULL;
	dst->no_tick = !timed;
	// Interrupts are held off so both runs see the same events; native
	// routines are atomic anyway.
	dst->cpu.ime = false;
	dst->cpu.ime_delay = 0;
	memset(&dst->test, 0, sizeof(dst->test));
	dst->test.active = true;
	dst->test.stack_lo = stack_bottom;
	dst->test.stack_hi = stack_top;
	dst->test.timed = timed;
	dst->test.fuzzed = fuzzed;
}

// The end of the routine at h: the next global label in the same region.
static uint16_t routine_end(Hook *h)
{
	uint16_t end = h->addr < 0x4000 ? 0x4000 : 0x8000;
	for (int i = 0; i < sym_count(); i++) {
		const char *name;
		int bank;
		uint16_t addr;
		sym_get(i, &name, &bank, &addr);
		if (strchr(name, '.') || addr <= h->addr || addr >= end)
			continue;
		if (h->addr >= 0x4000 && bank != h->bank)
			continue;
		end = addr;
	}
	return end;
}

static uint16_t *routine_ends; // per hook, filled in on first use

static bool jumps_into_itself(const GB *gb, Hook *h)
{
	if (!routine_ends) {
		routine_ends = malloc((size_t)hook_count() * sizeof(uint16_t));
		for (int i = 0; i < hook_count(); i++)
			routine_ends[i] = routine_end(hook_get(i));
	}
	uint16_t pc = gb->cpu.pc;
	bool same_bank = h->addr < 0x4000 || gb->rom_bank == h->bank;
	return same_bank && pc >= h->addr && pc < routine_ends[h - hook_get(0)];
}

// Runs hook h natively and the asm from base, and compares. Optionally logs
// what the asm run reads (struct offsets, see GB.test.read_log) and marks
// the instructions it executes in exec_map.
static CheckResult check_once(const GB *base, Hook *h, Report *r, const char **why,
	uint32_t *read_log, int read_log_cap, int *read_log_len, uint8_t *exec_map, bool *waits)
{
	*why = NULL;
	*waits = false;
	jumps_into_itself(base, h); // makes sure routine_ends is filled in
	body_bank = h->bank;
	body_start = h->addr;
	body_end = routine_ends[h - hook_get(0)];

	// Native run.
	prepare(nat, base);
	nat->test.fuzzed = false;
	uint16_t entry_sp = nat->cpu.sp;
	nat->test.budget = native_budget;
	nat->test.out_of_budget = native_out_of_budget;
	volatile bool native_stuck = false; // set across the longjmp
	hooks_set_enabled(true);
	if (setjmp(native_escape)) {
		native_stuck = true;
	} else {
		h->fn(nat);
		if (!(h->flags & HOOK_MANUAL_RET))
			nat->cpu.pc = gb_pop16(nat);
	}
	hooks_set_enabled(false);
	native_accesses = native_budget - nat->test.budget;

	// Reference run: the original asm, from the same state. It runs until it
	// gets where the native code went. A routine that ends in a plain ret
	// goes back to the return address on the stack; one that was jumped to
	// (like HomeJumpRet, which pops a saved bank first) or ends in a jump
	// goes somewhere else, and the native exit says where.
	prepare(ref, base);
	ref->test.read_log = read_log;
	ref->test.read_log_cap = read_log_cap;
	ref->test.exec_map = exec_map;
	// Either run turning out unrealistic makes the check inconclusive, so the
	// asm needn't run (on) then.
	bool left = false;
	asm_steps_taken = 0;
	stop_if_unrealistic = true;
	bool reached = !native_stuck && !nat->test.crashed && !nat->test.unrealistic &&
		run_asm_until(ref, nat->cpu.pc, nat->cpu.sp, entry_sp, &left);
	stop_if_unrealistic = false;
	if (read_log_len)
		*read_log_len = ref->test.read_log_len;

	if (ref->test.crashed || nat->test.crashed || ref->test.unrealistic || nat->test.unrealistic)
		return CHECK_INCONCLUSIVE;
	// A jump back into the routine's own code (a fuzzed jump table entry,
	// say) has no clean end to compare at: the asm passes through that
	// address on its way.
	if ((h->flags & HOOK_MANUAL_RET) && !native_stuck && jumps_into_itself(nat, h))
		return CHECK_INCONCLUSIVE;
	// Nor does a jump into the rst and interrupt vectors, which only a
	// fuzzed table entry makes: one into JumpTable's code ($28) is passed
	// through by the asm's own rst JumpTable.
	if ((h->flags & HOOK_MANUAL_RET) && !native_stuck && nat->cpu.pc < 0x0100)
		return CHECK_INCONCLUSIVE;
	if (native_stuck) {
		// Fine if the asm loops forever too; otherwise the native code is
		// stuck where the original isn't. An asm run that turned
		// unrealistic on its way out proves nothing either way.
		bool asm_left;
		prepare(ref, base);
		ref->test.exec_map = exec_map;
		bool asm_ends = run_asm_until(ref, 0xffff, 0, entry_sp, &asm_left) || asm_left;
		if (asm_ends && ref->test.unrealistic)
			return CHECK_INCONCLUSIVE;
		if (asm_ends) {
			*why = "the native code never finishes, the asm does";
			return CHECK_FAIL;
		}
		*waits = !ref->test.crashed && !ref->test.unrealistic;
		return CHECK_INCONCLUSIVE;
	}
	if (!reached) {
		if (!left) {
			*waits = true;
			return CHECK_INCONCLUSIVE;
		}
		*why = "the native code exits somewhere other than the asm";
		return CHECK_FAIL;
	}
	return compare(ref, nat, h->clobbers, r) ? CHECK_FAIL : CHECK_PASS;
}

static bool *needs_time; // per hook: known to wait on the hardware

// Runs hook h natively and the asm from base, and compares: with time
// frozen, or, for routines that wait on the hardware, with time flowing.
// Only real calls (real = true: the replay, and the fuzzer's unmodified
// seeds) can show that a routine waits on the hardware; a fuzzed input that
// makes it loop forever just isn't comparable.
static CheckResult check(const GB *base, Hook *h, Report *r, const char **why,
	uint32_t *read_log, int read_log_cap, int *read_log_len, uint8_t *exec_map, bool real)
{
	if (!needs_time)
		needs_time = calloc((size_t)hook_count(), sizeof(bool));
	bool *known = &needs_time[h - hook_get(0)];
	bool waits = false;
	CheckResult res = CHECK_INCONCLUSIVE;
	fuzzed = !real;
	if (!*known) {
		timed = false;
		res = check_once(base, h, r, why, read_log, read_log_cap, read_log_len, exec_map, &waits);
		if (!waits || !real) {
			fuzzed = false;
			return res;
		}
		*known = true;
	}
	timed = true;
	memset(r, 0, sizeof(*r));
	res = check_once(base, h, r, why, read_log, read_log_cap, read_log_len, exec_map, &waits);
	timed = false;
	fuzzed = false;
	return res;
}

static void print_failure(const GB *base, Hook *h, const char *context, Report *r, const char *why)
{
	printf("FAIL %s (%s)", h->name, context);
	if (why)
		printf(": %s, to %04x (sp %04x)\n", why, nat->cpu.pc, nat->cpu.sp);
	else
		printf(": %d difference%s\n", r->diffs, r->diffs == 1 ? "" : "s");
	printf("  entry: a=%02x f=%02x bc=%04x de=%04x hl=%04x sp=%04x bank=%d\n",
		base->cpu.a, base->cpu.f, REG_BC(base), REG_DE(base), REG_HL(base), base->cpu.sp, base->rom_bank);
	fwrite(r->buf, 1, r->len, stdout);
	if (r->diffs > MAX_REPORTED_DIFFS)
		printf("    ... and %d more\n", r->diffs - MAX_REPORTED_DIFFS);
}

static void setup_common(void)
{
	int bank;
	if (!sym_lookup("wStackBottom", &bank, &stack_bottom))
		stack_bottom = 0xcf00;
	if (!sym_lookup("wStackTop", &bank, &stack_top))
		stack_top = 0xd000;
	if (!sym_lookup("wROMBank", &bank, &wrom_bank))
		wrom_bank = 0xc5ff;
	if (!ref) {
		ref = malloc(sizeof(GB));
		nat = malloc(sizeof(GB));
	}
}

static const InputScript *script;

static void replay_frame(GB *gb)
{
	gb->joypad = script ? input_buttons(script, (int)gb->frame_count) : 0;
	gb->apu.buffer_len = 0;
}

// ---------------------------------------------------------------------------
// Replay difftest

typedef struct {
	uint64_t seen, tested, passed, failed, skipped;
	bool reported;
} HookStats;

static bool should_sample(uint64_t n)
{
	return n <= DENSE_SAMPLES || (n & (n - 1)) == 0;
}

bool difftest_run(GB *gb, const InputScript *input, int end_frame)
{
	int n = hook_count();
	if (!n) {
		printf("difftest: no ported routines registered\n");
		return true;
	}
	HookStats *stats = calloc((size_t)n, sizeof(HookStats));
	setup_common();
	script = input;

	hooks_set_enabled(false);
	gb->frame_cb = replay_frame;
	printf("difftest: %d ported routine%s, frames %llu-%d\n", n, n == 1 ? "" : "s",
		(unsigned long long)gb->frame_count, end_frame);

	while ((int)gb->frame_count < end_frame) {
		Hook *h = hook_find_any(gb, gb->cpu.pc);
		if (h && !gb->cpu.halted) {
			HookStats *st = &stats[h - hook_get(0)];
			if (should_sample(++st->seen)) {
				st->tested++;
				Report r = {0};
				const char *why;
				switch (check(gb, h, &r, &why, NULL, 0, NULL, NULL, true)) {
				case CHECK_PASS:
					st->passed++;
					break;
				case CHECK_INCONCLUSIVE:
					st->skipped++;
					break;
				case CHECK_FAIL:
					st->failed++;
					if (!st->reported) {
						st->reported = true;
						char context[64];
						snprintf(context, sizeof(context), "call %llu, frame %llu",
							(unsigned long long)st->seen, (unsigned long long)gb->frame_count);
						print_failure(gb, h, context, &r, why);
					}
					break;
				}
			}
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
	free(stats);
	return ok;
}

// ---------------------------------------------------------------------------
// Fuzzer

typedef struct {
	int seeds;
	uint64_t seen;
	uint64_t samples, passed, failed, inconclusive;
	bool reported;
	// The routine's own asm: every instruction reachable from its entry
	// before the next global label, as ROM offsets.
	size_t *insns;
	int ninsns;
	// Constants the routine compares against or masks with (cp n, and n,
	// ...): inputs equal to them, or next to them, tend to take new branches.
	uint8_t dict[64];
	int ndict;
} FuzzStats;

static uint64_t rng_state;

static uint64_t rng(void)
{
	rng_state ^= rng_state << 13;
	rng_state ^= rng_state >> 7;
	rng_state ^= rng_state << 17;
	return rng_state;
}

static const FuzzStats *cur_stats; // the routine being fuzzed

static uint8_t interesting_byte(uint8_t original)
{
	static const uint8_t values[] = {0x00, 0x01, 0x02, 0x0f, 0x10, 0x7f, 0x80, 0x81, 0xfe, 0xff};
	switch (rng() % 6) {
	case 0: return values[rng() % sizeof(values)];
	case 1: return (uint8_t)(original + 1);
	case 2: return (uint8_t)(original - 1);
	case 3:
	case 4:
		if (cur_stats && cur_stats->ndict) {
			uint8_t v = cur_stats->dict[rng() % (uint64_t)cur_stats->ndict];
			return (uint8_t)(v + (int)(rng() % 3) - 1);
		}
		return (uint8_t)rng();
	default: return (uint8_t)rng();
	}
}

static size_t rom_offset(int bank, uint16_t addr)
{
	return addr < 0x4000 ? addr : (size_t)bank * 0x4000 + (addr - 0x4000);
}

static int insn_length(uint8_t op)
{
	switch (op) {
	case 0x01: case 0x11: case 0x21: case 0x31: case 0x08:
	case 0xc2: case 0xc3: case 0xca: case 0xd2: case 0xda:
	case 0xc4: case 0xcc: case 0xcd: case 0xd4: case 0xdc:
	case 0xea: case 0xfa:
		return 3;
	case 0x06: case 0x0e: case 0x16: case 0x1e: case 0x26: case 0x2e: case 0x36: case 0x3e:
	case 0x18: case 0x20: case 0x28: case 0x30: case 0x38:
	case 0xc6: case 0xce: case 0xd6: case 0xde: case 0xe6: case 0xee: case 0xf6: case 0xfe:
	case 0xe0: case 0xf0: case 0xe8: case 0xf8: case 0xcb: case 0x10:
		return 2;
	default:
		return 1;
	}
}

// Finds the instructions of the routine starting at (bank, start): follows
// its control flow (jumps and branches within it, fallthrough after calls)
// until the next global label.
static void find_routine_insns(GB *gb, Hook *h, FuzzStats *st)
{
	uint16_t end = routine_end(h);

	int len = end - h->addr;
	uint8_t *seen = calloc((size_t)len, 1);
	uint16_t *todo = malloc((size_t)len * sizeof(uint16_t));
	int ntodo = 0;
	todo[ntodo++] = h->addr;
	st->insns = malloc((size_t)len * sizeof(size_t));
	while (ntodo) {
		uint16_t pc = todo[--ntodo];
		while (pc >= h->addr && pc < end && !seen[pc - h->addr]) {
			seen[pc - h->addr] = 1;
			size_t off = rom_offset(h->bank, pc);
			if (off >= gb->rom_size)
				break;
			st->insns[st->ninsns++] = off;
			uint8_t op = gb->rom[off];
			int n = insn_length(op);
			// immediate operands of cp, and, or, xor, add, sub, adc, sbc
			if ((op & 0xc7) == 0xc6 && st->ndict < (int)sizeof(st->dict))
				st->dict[st->ndict++] = gb->rom[off + 1];
			uint16_t next = (uint16_t)(pc + n);
			uint16_t target = 0;
			bool has_target = false, falls_through = true;
			if (op == 0x18 || op == 0x20 || op == 0x28 || op == 0x30 || op == 0x38) {
				target = (uint16_t)(next + (int8_t)gb->rom[off + 1]);
				has_target = true;
				falls_through = op != 0x18;
			} else if (op == 0xc3 || op == 0xc2 || op == 0xca || op == 0xd2 || op == 0xda) {
				target = (uint16_t)(gb->rom[off + 1] | gb->rom[off + 2] << 8);
				has_target = true;
				falls_through = op != 0xc3;
			} else if (op == 0xc9 || op == 0xd9 || op == 0xe9 || op == 0xef) {
				// ret, reti, jp hl, rst JumpTable (data follows)
				falls_through = false;
			} else if (op == 0xd3 || op == 0xdb || op == 0xdd || op == 0xe3 || op == 0xe4 ||
				op == 0xeb || op == 0xec || op == 0xed || op == 0xf4 || op == 0xfc || op == 0xfd) {
				falls_through = false; // not code
			}
			if (has_target && target >= h->addr && target < end && !seen[target - h->addr])
				todo[ntodo++] = target;
			if (!falls_through)
				break;
			pc = next;
		}
	}
	free(seen);
	free(todo);
}

// Mutating these would make the routine return to, or execute, garbage.
static bool protected_offset(uint32_t off, uint16_t entry_sp)
{
	size_t wram0 = offsetof(GB, wram), hram = offsetof(GB, hram);
	if (off >= wram0 && off < wram0 + 0x1000) {
		uint16_t addr = (uint16_t)(0xc000 + (off - wram0));
		// wROMBank always holds the mapped bank; the game restores it from
		// there after far calls.
		if (addr == wrom_bank)
			return true;
		return addr >= stack_bottom && addr < stack_top && addr >= entry_sp;
	}
	if (off >= hram && off < hram + 0x7f) {
		// hCallFunc and hTransferVirtualOAM: code the game copies to HRAM
		// at startup, sized the way it copies them
		static int bank;
		static uint16_t call_func, call_func_size, transfer_oam, transfer_oam_size;
		if (!call_func) {
			uint16_t src, src_end;
			call_func = 0xff80;
			call_func_size = 0x20;
			if (sym_lookup("hCallFunc", &bank, &call_func) &&
				sym_lookup("InitHRAMCallFunc.Func", &bank, &src) &&
				sym_lookup("InitHRAMCallFunc.end", &bank, &src_end))
				call_func_size = (uint16_t)(src_end - src);
			transfer_oam = 0xffe8;
			transfer_oam_size = 0x10;
			// start.asm copies (WriteAToHL_BCTimes - PushOAM) + 1 bytes
			if (sym_lookup("hTransferVirtualOAM", &bank, &transfer_oam) &&
				sym_lookup("PushOAM", &bank, &src) &&
				sym_lookup("WriteAToHL_BCTimes", &bank, &src_end))
				transfer_oam_size = (uint16_t)(src_end - src + 1);
		}
		uint16_t addr = (uint16_t)(0xff80 + (off - hram));
		return (addr >= call_func && addr < call_func + call_func_size) ||
			(addr >= transfer_oam && addr < transfer_oam + transfer_oam_size);
	}
	return false;
}

#define MAX_READS 4096

typedef struct {
	uint32_t offsets[MAX_READS];
	int n;
	uint8_t *marked; // per struct byte
} ReadSet;

static void readset_add(ReadSet *rs, const uint32_t *log, int n)
{
	for (int i = 0; i < n && rs->n < MAX_READS; i++) {
		if (!rs->marked[log[i]]) {
			rs->marked[log[i]] = 1;
			rs->offsets[rs->n++] = log[i];
		}
	}
}

static void readset_clear(ReadSet *rs)
{
	for (int i = 0; i < rs->n; i++)
		rs->marked[rs->offsets[i]] = 0;
	rs->n = 0;
}

static FuzzStats *fstats;
static int fuzz_variants = 200;
static const char *fuzz_only; // comma-separated labels, or NULL for all
static int fuzz_shard_i, fuzz_shard_n = 1;
static bool fuzz_verbose;
static GB *variant;
static ReadSet readset;
static uint32_t read_log[1 << 16];
static uint8_t *exec_map;

#define MAX_CORPUS 16
static GB *corpus[MAX_CORPUS];

static int routine_coverage(const FuzzStats *st)
{
	int covered = 0;
	for (int k = 0; k < st->ninsns; k++)
		covered += exec_map[st->insns[k]] != 0;
	return covered;
}

// Fuzz hook h from the state base (at its entry).
static void fuzz_seed(const GB *base, Hook *h, int index, const char *seed_name)
{
	FuzzStats *st = &fstats[index];
	st->seeds++;
	// Seeded by the routine's own address, not its index among the hooks, so
	// that porting other routines doesn't change the inputs it gets.
	rng_state = 0x9e3779b97f4a7c15ull ^ ((uint64_t)h->bank << 48) ^ ((uint64_t)h->addr << 32)
		^ (uint64_t)st->seeds;

	// First the seed itself, which also shows what memory the routine reads
	// and how long it normally runs. Fuzzed inputs can make a routine loop
	// for a very long time (or forever); variants get 20 times as long as
	// the seed took, which is plenty for longer loops and keeps runaway ones
	// cheap.
	readset_clear(&readset);
	asm_step_limit = MAX_STEPS;
	native_budget = NATIVE_BUDGET;
	cur_stats = st;
	int ncorpus = 0;
	int seed_reads = 0; // the bytes the real call read come first in readset
	for (int v = 0; v <= fuzz_variants; v++) {
		if (v == 1) {
			asm_step_limit = asm_steps_taken * 20 > 300000 ? asm_steps_taken * 20 : 300000;
			native_budget = native_accesses * 20 > 1000000 ? native_accesses * 20 : 1000000;
		}
		// Coverage-guided: variants that reached new code become starting
		// points for further variants.
		*variant = (ncorpus && rng() % 2) ? *corpus[rng() % (uint64_t)ncorpus] : *base;
		char context[96];
		if (v == 0) {
			snprintf(context, sizeof(context), "%s", seed_name);
		} else {
			snprintf(context, sizeof(context), "%s, variant %d", seed_name, v);
			// random registers (keeping sp and pc)
			if (rng() % 4) {
				uint8_t *regs[] = {&variant->cpu.a, &variant->cpu.b, &variant->cpu.c, &variant->cpu.d,
					&variant->cpu.e, &variant->cpu.h, &variant->cpu.l};
				for (int i = 0; i < 7; i++)
					if (rng() % 2)
						*regs[i] = interesting_byte(*regs[i]);
				variant->cpu.f = (uint8_t)(rng() & 0xf0);
			}
			// and some of the memory it reads
			int mutations = readset.n ? (int)(rng() % 5) : 0;
			for (int m = 0; m < mutations; m++) {
				// Mostly the routine's genuine inputs: variants that wander
				// off through garbage pointers read lots of unrelated bytes.
				int pool = seed_reads && rng() % 10 < 7 ? seed_reads : readset.n;
				uint32_t off = readset.offsets[rng() % (uint64_t)pool];
				if (protected_offset(off, variant->cpu.sp))
					continue;
				uint8_t *p = (uint8_t *)variant + off;
				*p = interesting_byte(*p);
			}
		}

		Report r = {0};
		const char *why;
		int nreads = 0;
		int covered_before = routine_coverage(st);
		CheckResult res = check(variant, h, &r, &why, read_log, (int)(sizeof(read_log) / sizeof(*read_log)),
			&nreads, exec_map, v == 0);
		readset_add(&readset, read_log, nreads);
		if (v == 0) {
			seed_reads = readset.n;
			// The seed itself never finished (it waits for hardware that
			// doesn't run here, e.g. H-Blank with the LCD off): its variants
			// would each spin 20 times as long for nothing.
			if (res == CHECK_INCONCLUSIVE && asm_steps_taken >= asm_step_limit) {
				st->samples++;
				st->inconclusive++;
				break;
			}
		}
		if (v > 0 && routine_coverage(st) > covered_before && ncorpus < MAX_CORPUS) {
			if (!corpus[ncorpus])
				corpus[ncorpus] = malloc(sizeof(GB));
			*corpus[ncorpus++] = *variant;
		}
		st->samples++;
		switch (res) {
		case CHECK_PASS:
			st->passed++;
			break;
		case CHECK_INCONCLUSIVE:
			st->inconclusive++;
			break;
		case CHECK_FAIL:
			st->failed++;
			if (!st->reported) {
				st->reported = true;
				print_failure(variant, h, context, &r, why);
			}
			break;
		}
	}
}

static void fuzz_seed_done(void)
{
	asm_step_limit = MAX_STEPS;
	native_budget = NATIVE_BUDGET;
}

static bool fuzz_this(Hook *h)
{
	if ((int)(h - hook_get(0)) % fuzz_shard_n != fuzz_shard_i)
		return false;
	if (!fuzz_only)
		return true;
	size_t len = strlen(h->name);
	for (const char *p = fuzz_only; *p;) {
		const char *end = strchr(p, ',');
		size_t n = end ? (size_t)(end - p) : strlen(p);
		if (n == len && !strncmp(p, h->name, n))
			return true;
		if (!end)
			break;
		p = end + 1;
	}
	return false;
}

// Seeds per routine from the replay, at these call numbers.
static bool fuzz_seed_call(uint64_t n)
{
	return n == 1 || n == 2 || n == 8 || n == 64 || n == 512 || n == 4096;
}

void fuzz_configure(int variants, const char *only, int shard_i, int shard_n, bool verbose)
{
	if (variants > 0)
		fuzz_variants = variants;
	fuzz_only = only;
	if (shard_n > 0 && shard_i >= 0 && shard_i < shard_n) {
		fuzz_shard_i = shard_i;
		fuzz_shard_n = shard_n;
	}
	fuzz_verbose = verbose;
}

bool fuzz_run(GB *gb, const InputScript *input, int end_frame)
{
	int n = hook_count();
	if (!n) {
		printf("fuzz: no ported routines registered\n");
		return true;
	}
	setup_common();
	fstats = calloc((size_t)n, sizeof(FuzzStats));
	variant = malloc(sizeof(GB));
	readset.marked = calloc(sizeof(GB), 1);
	exec_map = calloc(gb->rom_size, 1);
	for (int i = 0; i < n; i++)
		find_routine_insns(gb, hook_get(i), &fstats[i]);
	script = input;

	hooks_set_enabled(false);
	gb->frame_cb = replay_frame;
	printf("fuzz: %d ported routine%s, %d variants per seed, seeds from frames %llu-%d\n", n,
		n == 1 ? "" : "s", fuzz_variants, (unsigned long long)gb->frame_count, end_frame);

	// Seeds from real calls during the replay.
	while ((int)gb->frame_count < end_frame) {
		Hook *h = hook_find_any(gb, gb->cpu.pc);
		if (h && !gb->cpu.halted && fuzz_this(h)) {
			int index = (int)(h - hook_get(0));
			uint64_t seen = ++fstats[index].seen;
			if (fuzz_seed_call(seen)) {
				char name[64];
				snprintf(name, sizeof(name), "seed from call %llu, frame %llu",
					(unsigned long long)seen, (unsigned long long)gb->frame_count);
				fuzz_seed(gb, h, index, name);
				fuzz_seed_done();
			}
		}
		gb_tick(gb, cpu_step(gb));
	}

	// Synthetic seeds for routines the replay never reached: the final game
	// state, with the routine called on top of it.
	for (int i = 0; i < n; i++) {
		Hook *h = hook_get(i);
		if (fstats[i].seeds || !fuzz_this(h))
			continue;
		GB *base = malloc(sizeof(GB));
		*base = *gb;
		if (h->addr >= 0x4000) {
			base->rom_bank = h->bank;
			*gb_ram_byte(base, wrom_bank) = (uint8_t)h->bank;
		}
		gb_push16(base, 0xfea0); // returns to the gb_call sentinel area
		base->cpu.pc = h->addr;
		base->cpu.halted = false;
		for (int s = 0; s < 3; s++)
			fuzz_seed(base, h, i, "synthetic seed");
		fuzz_seed_done();
		free(base);
	}

	bool ok = true;
	int full = 0, total_insns = 0, covered_insns = 0;
	printf("\n%-40s %6s %8s %8s %7s %7s %9s\n", "routine", "seeds", "samples", "passed", "failed",
		"inconc.", "asm cover");
	for (int i = 0; i < n; i++) {
		FuzzStats *st = &fstats[i];
		Hook *h = hook_get(i);
		if (!fuzz_this(h))
			continue;
		int covered = 0;
		for (int k = 0; k < st->ninsns; k++)
			covered += exec_map[st->insns[k]] != 0;
		total_insns += st->ninsns;
		covered_insns += covered;
		full += covered == st->ninsns;
		printf("%-40s %6d %8llu %8llu %7llu %7llu %4d/%-4d%s\n", h->name, st->seeds,
			(unsigned long long)st->samples, (unsigned long long)st->passed,
			(unsigned long long)st->failed, (unsigned long long)st->inconclusive, covered, st->ninsns,
			covered == st->ninsns ? "" : " *");
		if (fuzz_verbose && covered != st->ninsns) {
			printf("    dictionary:");
			for (int k = 0; k < st->ndict; k++)
				printf(" %02x", st->dict[k]);
			printf("\n");
			for (int k = 0; k < st->ninsns; k++) {
				if (exec_map[st->insns[k]])
					continue;
				size_t off = st->insns[k];
				uint16_t addr = off < 0x4000 ? (uint16_t)off : (uint16_t)(0x4000 + off % 0x4000);
				const char *name = sym_name_at(h->bank, addr);
				printf("    not executed: %02x:%04x (%s)\n", h->bank, addr, name ? name : "?");
			}
		}
		if (st->failed)
			ok = false;
	}
	printf("\n%d of %d routines have every instruction of their asm executed and checked "
		"(%d of %d instructions); * marks the rest%s.\n", full, n, covered_insns, total_insns,
		fuzz_verbose ? "" : ", --fuzz-verbose lists what's missing");
	printf("\nfuzz: %s\n", ok ? "PASS" : "FAIL");
	return ok;
}

// ---------------------------------------------------------------------------
// Coverage report

typedef struct {
	size_t offset;
	int sym;
} LabelAt;

static int cmp_label_at(const void *a, const void *b)
{
	size_t x = ((const LabelAt *)a)->offset, y = ((const LabelAt *)b)->offset;
	return x < y ? -1 : x > y;
}

bool coverage_run(GB *gb, const InputScript *input, int end_frame, const char *out_path)
{
	// Global ROM labels sorted by ROM offset; a bitmap filters the lookups.
	size_t bitmap_len = gb->rom_size / 8 + 1;
	uint8_t *bitmap = calloc(bitmap_len, 1);
	int nsyms = sym_count(), nlabels = 0;
	LabelAt *labels = malloc((size_t)nsyms * sizeof(LabelAt));
	uint64_t *counts = calloc((size_t)nsyms, sizeof(uint64_t));
	for (int i = 0; i < nsyms; i++) {
		const char *name;
		int bank;
		uint16_t addr;
		sym_get(i, &name, &bank, &addr);
		if (addr >= 0x8000 || strchr(name, '.'))
			continue;
		size_t off = rom_offset(bank, addr);
		if (off >= gb->rom_size)
			continue;
		labels[nlabels++] = (LabelAt){off, i};
		bitmap[off / 8] |= (uint8_t)(1 << (off % 8));
	}
	qsort(labels, (size_t)nlabels, sizeof(LabelAt), cmp_label_at);

	script = input;
	hooks_set_enabled(false);
	gb->frame_cb = replay_frame;
	while ((int)gb->frame_count < end_frame) {
		uint16_t pc = gb->cpu.pc;
		if (pc < 0x8000 && !gb->cpu.halted) {
			size_t off = rom_offset(gb->rom_bank % gb->rom_banks, pc);
			if (bitmap[off / 8] & (1 << (off % 8))) {
				LabelAt key = {off, 0};
				LabelAt *hit = bsearch(&key, labels, (size_t)nlabels, sizeof(LabelAt), cmp_label_at);
				// Several labels can share an address; count them all.
				for (LabelAt *l = hit; l && l >= labels && l->offset == off; l--)
					counts[l->sym]++;
				for (LabelAt *l = hit ? hit + 1 : NULL; l && l < labels + nlabels && l->offset == off; l++)
					counts[l->sym]++;
			}
		}
		gb_tick(gb, cpu_step(gb));
	}

	FILE *f = fopen(out_path, "w");
	if (!f) {
		fprintf(stderr, "coverage: can't write %s\n", out_path);
		return false;
	}
	for (int i = 0; i < nsyms; i++) {
		if (!counts[i])
			continue;
		const char *name;
		int bank;
		uint16_t addr;
		sym_get(i, &name, &bank, &addr);
		fprintf(f, "%s %llu\n", name, (unsigned long long)counts[i]);
	}
	free(bitmap);
	free(labels);
	free(counts);
	return fclose(f) == 0;
}
