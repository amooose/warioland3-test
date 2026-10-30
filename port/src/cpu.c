// SM83 interpreter.
//
// Runs whatever hasn't been ported to C yet. Timing is instruction-granular:
// each instruction executes, then the rest of the hardware is advanced by its
// cycle count. That's enough for this game, which syncs to VBlank.
#include "gb.h"
#include "hooks.h"
#include "syms.h"

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

static inline uint8_t fetch8(GB *gb)
{
	return gb_read(gb, PC++);
}

static inline uint16_t fetch16(GB *gb)
{
	uint16_t v = gb_read16(gb, PC);
	PC += 2;
	return v;
}

static inline void set_flags(GB *gb, bool z, bool n, bool h, bool c)
{
	F = (uint8_t)((z ? FLAG_Z : 0) | (n ? FLAG_N : 0) | (h ? FLAG_H : 0) | (c ? FLAG_C : 0));
}

#define FZ (F & FLAG_Z)
#define FC (F & FLAG_C)

// r8 operand encoding: B C D E H L (HL) A
static uint8_t get_r8(GB *gb, int r)
{
	switch (r) {
	case 0: return B;
	case 1: return C;
	case 2: return D;
	case 3: return E;
	case 4: return H;
	case 5: return L;
	case 6: return gb_read(gb, REG_HL(gb));
	default: return A;
	}
}

static void set_r8(GB *gb, int r, uint8_t v)
{
	switch (r) {
	case 0: B = v; break;
	case 1: C = v; break;
	case 2: D = v; break;
	case 3: E = v; break;
	case 4: H = v; break;
	case 5: L = v; break;
	case 6: gb_write(gb, REG_HL(gb), v); break;
	default: A = v; break;
	}
}

static uint16_t get_r16(GB *gb, int r)
{
	switch (r) {
	case 0: return REG_BC(gb);
	case 1: return REG_DE(gb);
	case 2: return REG_HL(gb);
	default: return SP;
	}
}

static void set_r16(GB *gb, int r, uint16_t v)
{
	switch (r) {
	case 0: SET_BC(gb, v); break;
	case 1: SET_DE(gb, v); break;
	case 2: SET_HL(gb, v); break;
	default: SP = v; break;
	}
}

static bool cond(GB *gb, int cc)
{
	switch (cc) {
	case 0: return !FZ;
	case 1: return FZ;
	case 2: return !FC;
	default: return FC;
	}
}

static void alu(GB *gb, int op, uint8_t v)
{
	int r;
	int carry = FC ? 1 : 0;
	switch (op) {
	case 0: // add
		r = A + v;
		set_flags(gb, (r & 0xff) == 0, false, ((A & 0xf) + (v & 0xf)) > 0xf, r > 0xff);
		A = (uint8_t)r;
		break;
	case 1: // adc
		r = A + v + carry;
		set_flags(gb, (r & 0xff) == 0, false, ((A & 0xf) + (v & 0xf) + carry) > 0xf, r > 0xff);
		A = (uint8_t)r;
		break;
	case 2: // sub
		r = A - v;
		set_flags(gb, (r & 0xff) == 0, true, (A & 0xf) < (v & 0xf), r < 0);
		A = (uint8_t)r;
		break;
	case 3: // sbc
		r = A - v - carry;
		set_flags(gb, (r & 0xff) == 0, true, (A & 0xf) < (v & 0xf) + carry, r < 0);
		A = (uint8_t)r;
		break;
	case 4: // and
		A &= v;
		set_flags(gb, A == 0, false, true, false);
		break;
	case 5: // xor
		A ^= v;
		set_flags(gb, A == 0, false, false, false);
		break;
	case 6: // or
		A |= v;
		set_flags(gb, A == 0, false, false, false);
		break;
	default: // cp
		r = A - v;
		set_flags(gb, (r & 0xff) == 0, true, (A & 0xf) < (v & 0xf), r < 0);
		break;
	}
}

// CB-prefixed instructions. Returns T-cycles.
static int exec_cb(GB *gb)
{
	uint8_t op = fetch8(gb);
	int r = op & 7;
	int bit = (op >> 3) & 7;
	uint8_t v = get_r8(gb, r);
	int cycles = r == 6 ? 16 : 8;

	switch (op >> 6) {
	case 0: {
		uint8_t res;
		bool c;
		switch (bit) {
		case 0: c = v >> 7; res = (uint8_t)(v << 1 | v >> 7); break;         // rlc
		case 1: c = v & 1; res = (uint8_t)(v >> 1 | v << 7); break;          // rrc
		case 2: c = v >> 7; res = (uint8_t)(v << 1 | (FC ? 1 : 0)); break;  // rl
		case 3: c = v & 1; res = (uint8_t)(v >> 1 | (FC ? 0x80 : 0)); break; // rr
		case 4: c = v >> 7; res = (uint8_t)(v << 1); break;                  // sla
		case 5: c = v & 1; res = (uint8_t)((v >> 1) | (v & 0x80)); break;    // sra
		case 6: c = false; res = (uint8_t)(v << 4 | v >> 4); break;          // swap
		default: c = v & 1; res = v >> 1; break;                             // srl
		}
		set_flags(gb, res == 0, false, false, c);
		set_r8(gb, r, res);
		break;
	}
	case 1: // bit
		F = (uint8_t)((F & FLAG_C) | FLAG_H | ((v >> bit) & 1 ? 0 : FLAG_Z));
		if (r == 6)
			cycles = 12;
		break;
	case 2: // res
		set_r8(gb, r, (uint8_t)(v & ~(1 << bit)));
		break;
	default: // set
		set_r8(gb, r, (uint8_t)(v | (1 << bit)));
		break;
	}
	return cycles;
}

// Execution history, for debugging crashes and --break.
#define HISTORY_LEN 256
static struct {
	uint16_t pc, sp;
	uint8_t bank, a;
} history[HISTORY_LEN];
static unsigned history_pos;

static int break_bank = -1;
static uint16_t break_addr;
static int break_count;

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

static void unknown_opcode(GB *gb, uint8_t op)
{
	if (gb->test.active) {
		// Fuzzed inputs can send code anywhere; just stop this run.
		gb->test.crashed = true;
		PC--;
		return;
	}
	uint16_t at = (uint16_t)(PC - 1);
	const char *name = sym_name_at(at < 0x4000 ? 0 : gb->rom_bank, at);
	fprintf(stderr, "illegal opcode %02x at %02x:%04x (%s)\n", op, gb->rom_bank, at, name ? name : "?");
	cpu_dump_history(gb, 64);
	exit(1);
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

static int exec(GB *gb)
{
	uint8_t op = fetch8(gb);
	gb->test.op = op;
	uint8_t v8;
	uint16_t v16;
	int r;

	// ld r8, r8 (0x40-0x7f, except halt)
	if (op >= 0x40 && op < 0x80) {
		if (op == 0x76) {
			gb->cpu.halted = true;
			return 4;
		}
		int dst = (op >> 3) & 7, src = op & 7;
		set_r8(gb, dst, get_r8(gb, src));
		return (dst == 6 || src == 6) ? 8 : 4;
	}
	// alu a, r8
	if (op >= 0x80 && op < 0xc0) {
		alu(gb, (op >> 3) & 7, get_r8(gb, op & 7));
		return (op & 7) == 6 ? 8 : 4;
	}

	switch (op) {
	case 0x00: return 4;
	case 0x10: // stop
		fetch8(gb);
		if (gb->cpu.speed_switch_armed) {
			gb->cpu.double_speed = !gb->cpu.double_speed;
			gb->cpu.speed_switch_armed = false;
		}
		gb->div_counter = 0;
		return 4;

	// ld r16, d16
	case 0x01: case 0x11: case 0x21: case 0x31:
		set_r16(gb, op >> 4, fetch16(gb));
		return 12;
	// ld [r16], a / ld a, [r16]
	case 0x02: gb_write(gb, REG_BC(gb), A); return 8;
	case 0x12: gb_write(gb, REG_DE(gb), A); return 8;
	case 0x22: gb_write(gb, REG_HL(gb), A); SET_HL(gb, REG_HL(gb) + 1); return 8;
	case 0x32: gb_write(gb, REG_HL(gb), A); SET_HL(gb, REG_HL(gb) - 1); return 8;
	case 0x0a: A = gb_read(gb, REG_BC(gb)); return 8;
	case 0x1a: A = gb_read(gb, REG_DE(gb)); return 8;
	case 0x2a: A = gb_read(gb, REG_HL(gb)); SET_HL(gb, REG_HL(gb) + 1); return 8;
	case 0x3a: A = gb_read(gb, REG_HL(gb)); SET_HL(gb, REG_HL(gb) - 1); return 8;
	// inc/dec r16
	case 0x03: case 0x13: case 0x23: case 0x33:
		set_r16(gb, op >> 4, (uint16_t)(get_r16(gb, op >> 4) + 1));
		return 8;
	case 0x0b: case 0x1b: case 0x2b: case 0x3b:
		set_r16(gb, op >> 4, (uint16_t)(get_r16(gb, op >> 4) - 1));
		return 8;
	// inc r8
	case 0x04: case 0x0c: case 0x14: case 0x1c: case 0x24: case 0x2c: case 0x34: case 0x3c:
		r = (op >> 3) & 7;
		v8 = (uint8_t)(get_r8(gb, r) + 1);
		set_r8(gb, r, v8);
		F = (uint8_t)((F & FLAG_C) | (v8 == 0 ? FLAG_Z : 0) | ((v8 & 0xf) == 0 ? FLAG_H : 0));
		return r == 6 ? 12 : 4;
	// dec r8
	case 0x05: case 0x0d: case 0x15: case 0x1d: case 0x25: case 0x2d: case 0x35: case 0x3d:
		r = (op >> 3) & 7;
		v8 = (uint8_t)(get_r8(gb, r) - 1);
		set_r8(gb, r, v8);
		F = (uint8_t)((F & FLAG_C) | FLAG_N | (v8 == 0 ? FLAG_Z : 0) | ((v8 & 0xf) == 0xf ? FLAG_H : 0));
		return r == 6 ? 12 : 4;
	// ld r8, d8
	case 0x06: case 0x0e: case 0x16: case 0x1e: case 0x26: case 0x2e: case 0x36: case 0x3e:
		r = (op >> 3) & 7;
		set_r8(gb, r, fetch8(gb));
		return r == 6 ? 12 : 8;
	// rotates on a
	case 0x07:
		A = (uint8_t)(A << 1 | A >> 7);
		set_flags(gb, false, false, false, A & 1);
		return 4;
	case 0x0f:
		set_flags(gb, false, false, false, A & 1);
		A = (uint8_t)(A >> 1 | A << 7);
		return 4;
	case 0x17:
		v8 = A >> 7;
		A = (uint8_t)(A << 1 | (FC ? 1 : 0));
		set_flags(gb, false, false, false, v8);
		return 4;
	case 0x1f:
		v8 = A & 1;
		A = (uint8_t)(A >> 1 | (FC ? 0x80 : 0));
		set_flags(gb, false, false, false, v8);
		return 4;
	case 0x08: // ld [a16], sp
		gb_write16(gb, fetch16(gb), SP);
		return 20;
	// add hl, r16
	case 0x09: case 0x19: case 0x29: case 0x39: {
		uint16_t hl = REG_HL(gb);
		v16 = get_r16(gb, op >> 4);
		uint32_t res = (uint32_t)hl + v16;
		F = (uint8_t)((F & FLAG_Z) | (((hl & 0xfff) + (v16 & 0xfff)) > 0xfff ? FLAG_H : 0) | (res > 0xffff ? FLAG_C : 0));
		SET_HL(gb, (uint16_t)res);
		return 8;
	}
	// jr
	case 0x18:
		v8 = fetch8(gb);
		PC = (uint16_t)(PC + (int8_t)v8);
		return 12;
	case 0x20: case 0x28: case 0x30: case 0x38:
		v8 = fetch8(gb);
		if (cond(gb, (op >> 3) & 3)) {
			PC = (uint16_t)(PC + (int8_t)v8);
			return 12;
		}
		return 8;
	case 0x27: { // daa
		int a = A;
		bool c = FC;
		if (!(F & FLAG_N)) {
			if (c || a > 0x99) {
				a += 0x60;
				c = true;
			}
			if ((F & FLAG_H) || (a & 0xf) > 9)
				a += 0x06;
		} else {
			if (c)
				a -= 0x60;
			if (F & FLAG_H)
				a -= 0x06;
		}
		A = (uint8_t)a;
		F = (uint8_t)((F & FLAG_N) | (A == 0 ? FLAG_Z : 0) | (c ? FLAG_C : 0));
		return 4;
	}
	case 0x2f: // cpl
		A = (uint8_t)~A;
		F |= FLAG_N | FLAG_H;
		return 4;
	case 0x37: // scf
		F = (uint8_t)((F & FLAG_Z) | FLAG_C);
		return 4;
	case 0x3f: // ccf
		F = (uint8_t)((F & FLAG_Z) | (FC ? 0 : FLAG_C));
		return 4;

	// ret cc
	case 0xc0: case 0xc8: case 0xd0: case 0xd8:
		if (cond(gb, (op >> 3) & 3)) {
			PC = gb_pop16(gb);
			return 20;
		}
		return 8;
	// pop
	case 0xc1: SET_BC(gb, gb_pop16(gb)); return 12;
	case 0xd1: SET_DE(gb, gb_pop16(gb)); return 12;
	case 0xe1: SET_HL(gb, gb_pop16(gb)); return 12;
	case 0xf1:
		v16 = gb_pop16(gb);
		A = (uint8_t)(v16 >> 8);
		F = (uint8_t)(v16 & 0xf0);
		return 12;
	// push
	case 0xc5: gb_push16(gb, REG_BC(gb)); return 16;
	case 0xd5: gb_push16(gb, REG_DE(gb)); return 16;
	case 0xe5: gb_push16(gb, REG_HL(gb)); return 16;
	case 0xf5: gb_push16(gb, REG_AF(gb)); return 16;
	// jp
	case 0xc2: case 0xca: case 0xd2: case 0xda:
		v16 = fetch16(gb);
		if (cond(gb, (op >> 3) & 3)) {
			PC = v16;
			return 16;
		}
		return 12;
	case 0xc3:
		PC = fetch16(gb);
		return 16;
	case 0xe9:
		PC = REG_HL(gb);
		return 4;
	// call
	case 0xc4: case 0xcc: case 0xd4: case 0xdc:
		v16 = fetch16(gb);
		if (cond(gb, (op >> 3) & 3)) {
			gb_push16(gb, PC);
			PC = v16;
			return 24;
		}
		return 12;
	case 0xcd:
		v16 = fetch16(gb);
		gb_push16(gb, PC);
		PC = v16;
		return 24;
	// alu a, d8
	case 0xc6: case 0xce: case 0xd6: case 0xde: case 0xe6: case 0xee: case 0xf6: case 0xfe:
		alu(gb, (op >> 3) & 7, fetch8(gb));
		return 8;
	// rst
	case 0xc7: case 0xcf: case 0xd7: case 0xdf: case 0xe7: case 0xef: case 0xf7: case 0xff:
		gb_push16(gb, PC);
		PC = op & 0x38;
		return 16;
	case 0xc9:
		PC = gb_pop16(gb);
		return 16;
	case 0xd9: // reti
		PC = gb_pop16(gb);
		gb->cpu.ime = true;
		return 16;
	case 0xcb:
		return exec_cb(gb);
	case 0xe0:
		gb_write(gb, (uint16_t)(0xff00 + fetch8(gb)), A);
		return 12;
	case 0xf0:
		A = gb_read(gb, (uint16_t)(0xff00 + fetch8(gb)));
		return 12;
	case 0xe2:
		gb_write(gb, (uint16_t)(0xff00 + C), A);
		return 8;
	case 0xf2:
		A = gb_read(gb, (uint16_t)(0xff00 + C));
		return 8;
	case 0xea:
		gb_write(gb, fetch16(gb), A);
		return 16;
	case 0xfa:
		A = gb_read(gb, fetch16(gb));
		return 16;
	case 0xe8: case 0xf8: { // add sp, e8 / ld hl, sp+e8
		int8_t e8 = (int8_t)fetch8(gb);
		uint16_t res = (uint16_t)(SP + e8);
		set_flags(gb, false, false, ((SP & 0xf) + (e8 & 0xf)) > 0xf, ((SP & 0xff) + (e8 & 0xff)) > 0xff);
		if (op == 0xe8) {
			SP = res;
			return 16;
		}
		SET_HL(gb, res);
		return 12;
	}
	case 0xf9:
		SP = REG_HL(gb);
		return 8;
	case 0xf3:
		gb->cpu.ime = false;
		gb->cpu.ime_delay = 0;
		return 4;
	case 0xfb:
		gb->cpu.ime_delay = 2;
		return 4;
	default:
		unknown_opcode(gb, op);
		return 4;
	}
}

int cpu_step(GB *gb)
{
	int cycles = service_interrupts(gb);
	if (cycles)
		return cycles;
	if (gb->cpu.halted)
		return 4;

	Hook *hook = hook_find(gb, PC);
	if (hook) {
		hook->calls++;
		hook->fn(gb);
		if (!(hook->flags & HOOK_MANUAL_RET))
			PC = gb_pop16(gb);
		came_from_wram = false;
		prev_pc = PREV_NATIVE;
		// Native code takes no emulated time of its own; charge a token
		// amount so the hardware keeps moving.
		cycles = 4;
	} else {
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
		if (break_bank >= 0 && PC == break_addr && (PC >= 0x4000 ? gb->rom_bank : 0) == break_bank) {
			fprintf(stderr, "break #%d at frame %llu\n", ++break_count, (unsigned long long)gb->frame_count);
			cpu_dump_history(gb, HISTORY_LEN);
		}
		cycles = exec(gb);
		// a call (or rst) from WRAM code: remember where it returns to
		uint8_t op = gb->test.op;
		if (came_from_wram && SP == (uint16_t)(sp_before - 2) &&
			(op == 0xcd || op == 0xc4 || op == 0xcc || op == 0xd4 || op == 0xdc || (op & 0xc7) == 0xc7)) {
			uint8_t *lo = gb_ram_byte(gb, SP), *hi = gb_ram_byte(gb, (uint16_t)(SP + 1));
			if (lo && hi)
				wram_rets[wram_ret_pos++ % 8] = (uint16_t)(*lo | *hi << 8);
		}
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

void gb_call_ret(GB *gb, uint16_t addr, uint16_t ret)
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
	// frames until the host stack overflows.
	if (gb->test.active && gb->test.stack_lo && SP < gb->test.stack_lo) {
		gb->test.unrealistic = true;
		gb->test.crashed = true;
	}
	gb->call_depth++;
	while (!(PC == ret && SP == saved_sp) && !gb->test.crashed) {
		// Under test, steps count against the budget too: a halted CPU
		// makes no memory accesses.
		if (gb->test.budget && --gb->test.budget == 0)
			gb->test.out_of_budget(gb);
		int cycles = cpu_step(gb);
		if (!gb->no_tick)
			gb_tick(gb, cycles);
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
}
