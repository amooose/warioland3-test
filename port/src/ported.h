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

// Memory access. Always go through these (not raw arrays) so banking and I/O
// side effects behave exactly like the original code.
static inline uint8_t rd8(GB *gb, uint16_t addr) { return gb_read(gb, addr); }
static inline void wr8(GB *gb, uint16_t addr, uint8_t v) { gb_write(gb, addr, v); }
static inline uint16_t rd16(GB *gb, uint16_t addr) { return gb_read16(gb, addr); }
static inline void wr16(GB *gb, uint16_t addr, uint16_t v) { gb_write16(gb, addr, v); }
// ldh [rXXX] style access to I/O registers, by offset from $ff00.
static inline uint8_t rdio(GB *gb, uint8_t reg) { return gb_read(gb, (uint16_t)(0xff00 + reg)); }
static inline void wrio(GB *gb, uint8_t reg, uint8_t v) { gb_write(gb, (uint16_t)(0xff00 + reg), v); }

// Call another routine by label. If it has been ported it runs natively,
// otherwise the interpreter runs the original. Registers are passed and
// returned through gb->cpu exactly as in the asm.
#define CALL(name) port_call(gb, BANK_##name, ADDR_##name, #name)
// The farcall macro: switch to the routine's bank via hCallFunc, call it,
// switch back. Runs the real hCallFunc so all side effects match.
#define FARCALL(name) port_farcall(gb, BANK_##name, ADDR_##name)

void port_call(GB *gb, int bank, uint16_t addr, const char *name);
void port_farcall(GB *gb, int bank, uint16_t addr);

// The bankswitch macro: ld [wROMBank], a / ld [rROMB0 + $100], a
static inline void bankswitch(GB *gb, uint8_t bank)
{
	wr8(gb, wROMBank, bank);
	wr8(gb, 0x2100, bank);
}

// Flag helpers for routines whose callers test the flags.
static inline void set_z(GB *gb, bool z)
{
	gb->cpu.f = (uint8_t)(z ? (gb->cpu.f | FLAG_Z) : (gb->cpu.f & ~FLAG_Z));
}
static inline void set_c(GB *gb, bool c)
{
	gb->cpu.f = (uint8_t)(c ? (gb->cpu.f | FLAG_C) : (gb->cpu.f & ~FLAG_C));
}

// Hardware registers (offsets from $ff00) used by ported code.
enum {
	rJOYP = 0x00,
	rIF = 0x0f,
	rLCDC = 0x40,
	rSTAT = 0x41,
	rSCY = 0x42,
	rSCX = 0x43,
	rLY = 0x44,
	rVBK = 0x4f,
	rWBK = 0x70,
	rIE = 0xff,
};

// Values of the joypad select register, as in hardware.inc.
enum {
	JOYP_GET_BUTTONS = 0x10,
	JOYP_GET_CTRL_PAD = 0x20,
	JOYP_GET_NONE = 0x30,
};

#define OAM_SIZE 0xa0

#endif
