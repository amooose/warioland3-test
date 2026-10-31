// The CPU loop: runs the native routine at PC (every routine of the game has
// one), services interrupts and halt, and implements the calls between
// native routines (gb_call_ret). Code without a native routine goes to the
// interpreter (interp.c) in the development build, and is an error in a
// build without it.
#include "gb.h"
#include "hooks.h"
#include "syms.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#define A (gb->cpu.a)
#define F (gb->cpu.f)
#define B (gb->cpu.b)
#define C (gb->cpu.c)
#define D (gb->cpu.d)
#define E (gb->cpu.e)
#define H (gb->cpu.h)
#define L (gb->cpu.l)
#define SP (gb->cpu.sp)
#define PC (gb->cpu.pc)

// The WRAM the game copies or writes code into and runs on purpose: the
// V-Blank and LCD functions (wVBlankFunc, wLCDFunc, up to wROMBank) and the
// level's tile queue copy loop (wVBlankFuncExtended).
static inline bool wram_code(uint16_t pc)
{
	return (pc >= wVBlankFunc && pc < wROMBank) ||
		(pc >= wVBlankFuncExtended && pc < wVBlankFuncExtended + 0x200);
}

// Whether the previous instruction ran in WRAM code, where the previous
// instruction (or native routine) was, and the return addresses that calls
// made from WRAM code pushed (the code in these buffers calls ROM routines,
// which return into it).
static bool came_from_wram;
static uint16_t prev_pc;
#define PREV_NATIVE 0xffff // the previous code was native (a hook, gb_call_ret)
#if WL3_INTERPRETER
static uint16_t wram_rets[8];
static unsigned wram_ret_pos;

static bool is_wram_ret(uint16_t pc)
{
	for (int i = 0; i < 8; i++)
		if (wram_rets[i] == pc)
			return true;
	return false;
}

// WRAM code is entered only the way the game enters it: call wVBlankFunc in
// VBlank, jp wVBlankFunc + $10 and jp wVBlankFuncExtended in
// LevelVBlankFunc (or those routines' native versions), jp wLCDFunc at the
// LCD vector. After that it runs on, or is returned into from the ROM code
// it calls. A fuzzed jump or ret landing in these buffers any other way is
// as unrealistic as anywhere else in RAM.
static bool wram_code_ok(uint16_t pc)
{
	if (!wram_code(pc))
		return false;
	if (came_from_wram || is_wram_ret(pc))
		return true;
	switch (pc) {
	case wVBlankFunc:
		return prev_pc == ADDR_VBlank + 0x18 || prev_pc == PREV_NATIVE;
	case wVBlankFunc + 0x10:
		return prev_pc == ADDR_LevelVBlankFunc + 0x23 || prev_pc == PREV_NATIVE;
	case wVBlankFuncExtended:
		return prev_pc == ADDR_LevelVBlankFunc + 0x69 || prev_pc == PREV_NATIVE;
	case wLCDFunc:
		return prev_pc == 0x0048;
	default:
		return false;
	}
}
#endif

// Execution history, for debugging crashes and --break.
#define HISTORY_LEN 256
static struct {
	uint16_t pc, sp;
	uint8_t bank, a;
} history[HISTORY_LEN];
static unsigned history_pos;

static int break_bank = -1;
static uint16_t break_addr;
#if WL3_INTERPRETER
static int break_count;
#endif

void cpu_set_breakpoint(int bank, uint16_t addr)
{
	break_bank = addr < 0x4000 ? 0 : bank;
	break_addr = addr;
}

void cpu_dump_history(GB *gb, int n)
{
	if (n > HISTORY_LEN)
		n = HISTORY_LEN;
	fprintf(stderr, "last %d instructions (oldest first):\n", n);
	for (int i = n; i > 0; i--) {
		unsigned idx = (history_pos - (unsigned)i) % HISTORY_LEN;
		uint16_t pc = history[idx].pc;
		int bank = pc < 0x4000 ? 0 : history[idx].bank;
		const char *name = sym_name_at(bank, pc);
		fprintf(stderr, "  %02x:%04x a=%02x sp=%04x  %s\n", bank, pc, history[idx].a, history[idx].sp,
			name ? name : "?");
	}
	(void)gb;
}

static int service_interrupts(GB *gb)
{
	uint8_t pending = gb->ie & gb->io[0x0f] & 0x1f;
	if (!pending)
		return 0;
	gb->cpu.halted = false;
	if (!gb->cpu.ime)
		return 0;
	for (int i = 0; i < 5; i++) {
		if (pending & (1 << i)) {
			// Checking a routine without time: one that enables interrupts
			// with one pending is interrupted in the asm, at a point native
			// code (which is atomic) can't match.
			if (gb->test.active && !gb->test.timed)
				gb->test.unrealistic = true;
			// The V-Blank and LCD handlers do real work (the V-Blank one in
			// native code), at moments that differ between the native and
			// the asm run, so their results can't be compared.
			if (gb->test.active && i <= 1)
				gb->test.unrealistic = true;
			gb->io[0x0f] &= (uint8_t)~(1 << i);
			gb->cpu.ime = false;
			gb_push16(gb, PC);
			PC = (uint16_t)(0x40 + i * 8);
			return 20;
		}
	}
	return 0;
}

#if WL3_INTERPRETER
// No native routine at PC: run the original instruction.
static int no_native_routine(GB *gb)
{
	int cycles;
	unsigned idx = history_pos++ % HISTORY_LEN;
	history[idx].pc = PC;
	history[idx].sp = SP;
	history[idx].bank = (uint8_t)gb->rom_bank;
	history[idx].a = A;
	// Under test: game code runs from ROM, HRAM (hCallFunc and the OAM
	// DMA routine) and the WRAM buffers the game builds code in;
	// anywhere else means a fuzzed input sent it astray.
	if (gb->test.active && PC >= 0x8000 && PC < 0xff80 && !wram_code_ok(PC))
		gb->test.unrealistic = true;
	came_from_wram = wram_code(PC);
	prev_pc = PC;
	uint16_t sp_before = SP;
	if (gb->test.exec_map && PC < 0x8000) {
		size_t off = PC < 0x4000 ? PC : (size_t)(gb->rom_bank % gb->rom_banks) * 0x4000 + (PC - 0x4000);
		gb->test.exec_map[off] = 1;
	}
	if (gb->asm_profile)
		gb->asm_profile[PC < 0x8000 ? (PC < 0x4000 ? PC : (size_t)(gb->rom_bank % gb->rom_banks) * 0x4000 +
			(PC - 0x4000)) : gb->rom_size + PC]++;
	if (break_bank >= 0 && PC == break_addr && (PC >= 0x4000 ? gb->rom_bank : 0) == break_bank) {
		fprintf(stderr, "break #%d at frame %llu\n", ++break_count, (unsigned long long)gb->frame_count);
		cpu_dump_history(gb, HISTORY_LEN);
	}
	cycles = interp_exec(gb);
	// a call (or rst) from WRAM code: remember where it returns to
	uint8_t op = gb->test.op;
	if (came_from_wram && SP == (uint16_t)(sp_before - 2) &&
		(op == 0xcd || op == 0xc4 || op == 0xcc || op == 0xd4 || op == 0xdc || (op & 0xc7) == 0xc7)) {
		uint8_t *lo = gb_ram_byte(gb, SP), *hi = gb_ram_byte(gb, (uint16_t)(SP + 1));
		if (lo && hi)
			wram_rets[wram_ret_pos++ % 8] = (uint16_t)(*lo | *hi << 8);
	}
	return cycles;
}
#else
// The debug_nop macro, nop / jr :-, an endless loop the game reaches only
// in debug situations: it stays there as time (and interrupts) go on.
static bool debug_nop_at(GB *gb, uint16_t pc)
{
	return gb_read(gb, pc) == 0x00 && gb_read(gb, (uint16_t)(pc + 1)) == 0x18 &&
		gb_read(gb, (uint16_t)(pc + 2)) == 0xfd;
}

// No native routine at PC, and no interpreter in this build.
static int no_native_routine(GB *gb)
{
	if (debug_nop_at(gb, PC))
		return 16; // nop / jr, PC stays
	int bank = PC < 0x4000 ? 0 : PC < 0x8000 ? gb->rom_bank : 0;
	const char *name = sym_name_at(bank, PC);
	fprintf(stderr, "no native code at %02x:%04x (%s), and this build has no interpreter\n", bank, PC,
		name ? name : "?");
	cpu_dump_history(gb, 32);
	exit(1);
}
#endif

int cpu_step(GB *gb)
{
	int cycles = service_interrupts(gb);
	if (cycles)
		return cycles;
	if (gb->cpu.halted)
		return 4;
	if (gb->pc_cb)
		gb->pc_cb(gb);

	Hook *hook = hook_find(gb, PC);
	RamHookFn ram_hook = hook ? NULL : ram_hook_find(PC);
	if (hook) {
#if !WL3_INTERPRETER
		// Without the interpreter, the history is of native routines.
		unsigned idx = history_pos++ % HISTORY_LEN;
		history[idx].pc = PC;
		history[idx].sp = SP;
		history[idx].bank = (uint8_t)gb->rom_bank;
		history[idx].a = A;
#endif
		hook->calls++;
		hook->fn(gb);
		if (!(hook->flags & HOOK_MANUAL_RET))
			PC = gb_pop16(gb);
		came_from_wram = false;
		prev_pc = PREV_NATIVE;
		// Native code takes no emulated time of its own; charge a token
		// amount so the hardware keeps moving.
		cycles = 4;
	} else if (ram_hook && ram_hook(gb)) {
		came_from_wram = false;
		prev_pc = PREV_NATIVE;
		cycles = 4;
	} else {
		cycles = no_native_routine(gb);
	}

	// EI takes effect after the instruction following it.
	if (gb->cpu.ime_delay && --gb->cpu.ime_delay == 0)
		gb->cpu.ime = true;
	return cycles;
}

// Return address used by gb_call; it lies in the unusable FEA0-FEFF area, so
// no real code ever runs there.
#define CALL_SENTINEL 0xfea0

void gb_call(GB *gb, uint16_t addr)
{
	gb_call_ret(gb, addr, CALL_SENTINEL);
}

// The call loop of gb_call_ret. With may_unwind, code that pops past the
// call's frame (returning to the caller's caller) ends it: false, with PC
// and SP where that code left them.
static bool call_ret(GB *gb, uint16_t addr, uint16_t ret, bool may_unwind)
{
	uint16_t saved_sp = SP;
	uint16_t saved_pc = PC;
	gb_push16(gb, ret);
	PC = addr;
	prev_pc = PREV_NATIVE;
	came_from_wram = false;
	// Under test, a push below the stack's bottom is unrealistic (see
	// test_access). Stop there: native code that recurses (a fuzzed jump
	// table leading back into its caller) would otherwise nest host stack
	// frames until the host stack overflows. Recursion that keeps SP up
	// (fuzzed code in wVBlankFunc that re-enables interrupts, so V-Blank
	// re-enters itself) is caught by depth instead: the game's own nesting
	// stays around ten.
	if (gb->test.active && ((gb->test.stack_lo && SP < gb->test.stack_lo) || gb->call_depth >= 256)) {
		gb->test.unrealistic = true;
		gb->test.crashed = true;
	}
	gb->call_depth++;
	while (!(PC == ret && SP == saved_sp) && !gb->test.crashed) {
		// The game resets: it sets SP back to the top, so no caller of ours
		// will ever be returned to. Drop the native frames as it does.
		if (PC == ADDR_InitWithoutDemoPowerUpReset && gb->reset_unwind && !gb->test.active) {
			gb->call_depth = 0;
			longjmp(*(jmp_buf *)gb->reset_unwind, 1);
		}
		// Under test, steps count against the budget too: a halted CPU
		// makes no memory accesses.
		if (gb->test.budget && --gb->test.budget == 0)
			gb->test.out_of_budget(gb);
		int cycles = cpu_step(gb);
		if (!gb->no_tick)
			gb_tick(gb, cycles);
		if (may_unwind && SP > saved_sp) {
			gb->call_depth--;
			return false;
		}
		// Code that pops past the native caller's frame (a fuzzed jump
		// table sending a call somewhere that unwinds the stack) can never
		// come back here; the asm caller would carry on, so it's not a
		// fair comparison.
		if (gb->test.active && SP > saved_sp) {
			gb->test.unrealistic = true;
			gb->test.crashed = true;
		}
	}
	gb->call_depth--;
	PC = saved_pc;
	return true;
}

void gb_call_ret(GB *gb, uint16_t addr, uint16_t ret)
{
	call_ret(gb, addr, ret, false);
}

bool gb_call_ret_unwind(GB *gb, uint16_t addr, uint16_t ret)
{
	return call_ret(gb, addr, ret, true);
}
