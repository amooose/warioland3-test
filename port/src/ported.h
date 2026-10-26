// API for hand-ported routines.
//
// Each ported routine is written as
//
//     PORT(RoutineName)
//     {
//         ...
//     }
//
// where RoutineName is the label from the disassembly. The build finds every
// PORT() and registers it as a hook, so it replaces the asm at that label.
// See port/docs/PORTING.md for the rules ported code must follow.
#ifndef PORTED_H
#define PORTED_H

#include "gb.h"
#include "hooks.h"
#include "constants.h" // generated: every constant in src/constants etc.
#include "syms.h" // generated: every label in the disassembly

// Registers, for clobber masks.
enum {
	R_A = 1 << 0,
	R_F = 1 << 1,
	R_B = 1 << 2,
	R_C = 1 << 3,
	R_D = 1 << 4,
	R_E = 1 << 5,
	R_H = 1 << 6,
	R_L = 1 << 7,
};

typedef struct {
	const char *name;
	HookFn fn;
	unsigned flags;
	unsigned clobbers; // R_* registers whose exit value doesn't have to match
} PortInfo;

void port_register(const PortInfo *info);

#define PORT_EX(name, flags, clobbers) \
	static void port_##name(GB *gb); \
	const PortInfo portinfo_##name = {#name, port_##name, (flags), (clobbers)}; \
	static void port_##name(GB *gb)

// A routine that ends in `ret` and whose register outputs must all match the
// original exactly.
#define PORT(name) PORT_EX(name, 0, 0)
// A routine that ends by jumping to another one (jp Label, jr Label, jp hl,
// or falling through into the next label): it must end with JUMP() or
// JUMP_TO(), or RET() on paths that return.
#define PORT_JUMP(name) PORT_EX(name, HOOK_MANUAL_RET, 0)

// Memory access. Hardware registers are addresses too (rJOYP = $ff00 etc.,
// from hardware.inc), so ldh [rLCDC], a is wr8(gb, rLCDC, a).
// Always go through these (not raw arrays) so banking and I/O
// side effects behave exactly like the original code.
static inline uint8_t rd8(GB *gb, uint16_t addr) { return gb_read(gb, addr); }
static inline void wr8(GB *gb, uint16_t addr, uint8_t v) { gb_write(gb, addr, v); }
static inline uint16_t rd16(GB *gb, uint16_t addr) { return gb_read16(gb, addr); }
static inline void wr16(GB *gb, uint16_t addr, uint16_t v) { gb_write16(gb, addr, v); }

// Call another routine by label. If it has been ported it runs natively,
// otherwise the interpreter runs the original. Registers are passed and
// returned through gb->cpu exactly as in the asm.
#define CALL(name) port_call(gb, BANK_##name, ADDR_##name, #name)
// The farcall macro: switch to the routine's bank via hCallFunc, call it,
// switch back. Runs the real hCallFunc so all side effects match.
#define FARCALL(name) port_farcall(gb, BANK_##name, ADDR_##name)
// farcall bank, Label: call a routine with another bank mapped (for
// routines in ROM0 that read data from that bank).
#define FARCALL_BANK(bank, name) port_farcall(gb, (bank), ADDR_##name)
// The hcall macro: call through hCallFunc with whatever bank is already in
// hCallFuncBank.
#define HCALL(name) port_hcall(gb, ADDR_##name)
// FARCALL, pushing ret (where the asm's call hCallFunc returns to) rather
// than gb_call's sentinel: for callees that may look at the stack, such as
// block functions.
#define FARCALL_RET(name, ret) port_farcall_ret(gb, BANK_##name, ADDR_##name, (ret))

// A tail jump: execution continues at the label with the stack exactly as
// it is, as in the asm. (CALL would push a return address, which a routine
// that inspects or pops the stack would see.) Only in PORT_JUMP routines.
#define JUMP(name) port_jump(gb, BANK_##name, ADDR_##name, #name)
// jp hl and other computed jumps.
#define JUMP_TO(addr) (gb->cpu.pc = (uint16_t)(addr))
// ret, in PORT_JUMP routines.
#define RET() (gb->cpu.pc = gb_pop16(gb))

void port_call(GB *gb, int bank, uint16_t addr, const char *name);
void port_jump(GB *gb, int bank, uint16_t addr, const char *name);
void port_farcall(GB *gb, int bank, uint16_t addr);
void port_hcall(GB *gb, uint16_t addr);
void port_farcall_ret(GB *gb, int bank, uint16_t addr, uint16_t ret);

// The play_sfx macro: ld a, HIGH(sfx) / ldh [hSFXID], a / ld a, LOW(sfx) /
// ldh [hSFXID + 1], a
static inline void play_sfx(GB *gb, uint16_t sfx)
{
	wr8(gb, hSFXID + 0, (uint8_t)(sfx >> 8));
	gb->cpu.a = (uint8_t)sfx;
	wr8(gb, hSFXID + 1, gb->cpu.a);
}

// The bankswitch macro: ld [wROMBank], a / ld [rROMB0 + $100], a
static inline void bankswitch(GB *gb, uint8_t bank)
{
	wr8(gb, wROMBank, bank);
	wr8(gb, rROMB0 + 0x100, bank);
}

// The sramswitch macro: ld [wSRAMBank], a / ld [rRAMB + $100], a
static inline void sramswitch(GB *gb, uint8_t bank)
{
	wr8(gb, wSRAMBank, bank);
	wr8(gb, rRAMB + 0x100, bank);
}

// push af / pop af, on the real stack, like the asm.
static inline void push_af(GB *gb)
{
	gb_push16(gb, REG_AF(gb));
}
static inline void pop_af(GB *gb)
{
	uint16_t v = gb_pop16(gb);
	gb->cpu.a = (uint8_t)(v >> 8);
	gb->cpu.f = (uint8_t)(v & 0xf0);
}

// Waiting on the hardware: let emulated time pass, as the asm's polling loop
// would. A wait loop such as wait_hblank becomes
//     while (alu_and(gb, rd8(gb, rSTAT), STAT_MODE))
//         idle(gb, 32);
// (32 cycles: about one pass of the asm loop.) The difftest checks such
// routines with time flowing; see docs/PORTING.md.
static inline void idle(GB *gb, int cycles)
{
	if (!gb->no_tick)
		gb_tick(gb, cycles);
}

// The flag results of 8-bit arithmetic, exactly as the CPU computes them.
// Use these where the asm's exit flags have to be reproduced: each returns
// the result and sets gb->cpu.f the way the instruction would.
static inline uint8_t alu_add(GB *gb, uint8_t a, uint8_t v)
{
	unsigned r = (unsigned)a + v;
	gb->cpu.f = (uint8_t)(((r & 0xff) ? 0 : FLAG_Z) | (((a & 0xf) + (v & 0xf)) > 0xf ? FLAG_H : 0) |
		(r > 0xff ? FLAG_C : 0));
	return (uint8_t)r;
}
static inline uint8_t alu_adc(GB *gb, uint8_t a, uint8_t v)
{
	unsigned carry = (gb->cpu.f & FLAG_C) ? 1 : 0;
	unsigned r = (unsigned)a + v + carry;
	gb->cpu.f = (uint8_t)(((r & 0xff) ? 0 : FLAG_Z) | (((a & 0xf) + (v & 0xf) + carry) > 0xf ? FLAG_H : 0) |
		(r > 0xff ? FLAG_C : 0));
	return (uint8_t)r;
}
static inline uint8_t alu_sub(GB *gb, uint8_t a, uint8_t v)
{
	gb->cpu.f = (uint8_t)((a == v ? FLAG_Z : 0) | FLAG_N | ((a & 0xf) < (v & 0xf) ? FLAG_H : 0) |
		(a < v ? FLAG_C : 0));
	return (uint8_t)(a - v);
}
static inline uint8_t alu_sbc(GB *gb, uint8_t a, uint8_t v)
{
	unsigned carry = (gb->cpu.f & FLAG_C) ? 1 : 0;
	int r = (int)a - v - (int)carry;
	gb->cpu.f = (uint8_t)(((r & 0xff) ? 0 : FLAG_Z) | FLAG_N |
		((int)(a & 0xf) - (int)(v & 0xf) - (int)carry < 0 ? FLAG_H : 0) | (r < 0 ? FLAG_C : 0));
	return (uint8_t)r;
}
// cp: flags of a - v, a unchanged
static inline void alu_cp(GB *gb, uint8_t a, uint8_t v)
{
	alu_sub(gb, a, v);
}
// and / or / xor
static inline uint8_t alu_and(GB *gb, uint8_t a, uint8_t v)
{
	uint8_t r = a & v;
	gb->cpu.f = (uint8_t)((r ? 0 : FLAG_Z) | FLAG_H);
	return r;
}
static inline uint8_t alu_or(GB *gb, uint8_t a, uint8_t v)
{
	uint8_t r = a | v;
	gb->cpu.f = r ? 0 : FLAG_Z;
	return r;
}
static inline uint8_t alu_xor(GB *gb, uint8_t a, uint8_t v)
{
	uint8_t r = a ^ v;
	gb->cpu.f = r ? 0 : FLAG_Z;
	return r;
}
// inc r / dec r (carry unchanged)
static inline uint8_t alu_inc(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v + 1);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | (r ? 0 : FLAG_Z) | ((r & 0xf) == 0 ? FLAG_H : 0));
	return r;
}
static inline uint8_t alu_dec(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v - 1);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_N | (r ? 0 : FLAG_Z) | ((r & 0xf) == 0xf ? FLAG_H : 0));
	return r;
}
// swap r
static inline uint8_t alu_swap(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v << 4 | v >> 4);
	gb->cpu.f = r ? 0 : FLAG_Z;
	return r;
}
// srl r
static inline uint8_t alu_srl(GB *gb, uint8_t v)
{
	uint8_t r = v >> 1;
	gb->cpu.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
	return r;
}
// rla: rotate a left through carry (z always clear)
static inline uint8_t alu_rla(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v << 1 | ((gb->cpu.f & FLAG_C) ? 1 : 0));
	gb->cpu.f = (v & 0x80) ? FLAG_C : 0;
	return r;
}
// bit n, r: z set if the bit is clear, h set, carry unchanged
static inline bool alu_bit(GB *gb, uint8_t v, int bit)
{
	bool set = (v >> bit) & 1;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_C) | FLAG_H | (set ? 0 : FLAG_Z));
	return set;
}
// daa
static inline uint8_t alu_daa(GB *gb, uint8_t v)
{
	int a = v;
	bool c = gb->cpu.f & FLAG_C;
	if (!(gb->cpu.f & FLAG_N)) {
		if (c || a > 0x99) {
			a += 0x60;
			c = true;
		}
		if ((gb->cpu.f & FLAG_H) || (a & 0xf) > 9)
			a += 0x06;
	} else {
		if (c)
			a -= 0x60;
		if (gb->cpu.f & FLAG_H)
			a -= 0x06;
	}
	uint8_t r = (uint8_t)a;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_N) | (r ? 0 : FLAG_Z) | (c ? FLAG_C : 0));
	return r;
}
// add hl, rr (z unchanged)
static inline uint16_t alu_add16(GB *gb, uint16_t hl, uint16_t v)
{
	uint32_t r = (uint32_t)hl + v;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | (((hl & 0xfff) + (v & 0xfff)) > 0xfff ? FLAG_H : 0) |
		(r > 0xffff ? FLAG_C : 0));
	return (uint16_t)r;
}


#endif
