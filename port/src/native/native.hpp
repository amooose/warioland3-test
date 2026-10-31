// Base for the C++ modules in src/native: the game's code rewritten as
// ordinary C++ (classes, locals, return values) rather than the
// register-exact C of src/ported, running on the same machine state.
//
// A module keeps the original's memory and I/O behavior exactly: its state
// stays where the game keeps it (WRAM, HRAM), read and written through the
// memory map like the original code does, so the rest of the game, save
// states and the tests see the same bytes. What it drops is the CPU-level
// detail: registers, flags, and the return addresses and temporaries the asm
// pushes. Its entry points are registered like other ported routines, with
// a register contract for their callers (see PORT_EX), and the difftest
// checks them against the original: memory, I/O and the contract registers.
#pragma once

#include "constants.h"
#include "gb.h"
#include "hooks.h"
#include "syms.h"

#include <cstdint>

namespace wl3 {

// A byte of the Game Boy's address space. Reads and writes go through the
// memory map (WRAM banking, I/O side effects), as the original's did.
class Byte {
public:
	Byte(GB *gb, uint16_t addr) : gb_(gb), addr_(addr) {}

	operator uint8_t() const { return gb_read(gb_, addr_); }
	Byte &operator=(uint8_t v)
	{
		gb_write(gb_, addr_, v);
		return *this;
	}
	Byte &operator=(const Byte &other) { return *this = static_cast<uint8_t>(other); }
	Byte &operator+=(unsigned v) { return *this = static_cast<uint8_t>(*this + v); }
	Byte &operator-=(unsigned v) { return *this = static_cast<uint8_t>(*this - v); }
	Byte &operator|=(unsigned v) { return *this = static_cast<uint8_t>(*this | v); }
	Byte &operator&=(unsigned v) { return *this = static_cast<uint8_t>(*this & v); }
	Byte &operator^=(unsigned v) { return *this = static_cast<uint8_t>(*this ^ v); }
	Byte &operator++() { return *this += 1; }
	Byte &operator--() { return *this -= 1; }

	bool bit(int n) const { return (*this >> n) & 1; }
	void set_bit(int n) { *this |= 1u << n; }
	void reset_bit(int n) { *this &= ~(1u << n); }
	uint16_t addr() const { return addr_; }
	// the dw that starts here
	class Word word() const;

private:
	GB *gb_;
	uint16_t addr_;
};

// A 16-bit value stored little-endian (dw): low byte first.
class Word {
public:
	Word(GB *gb, uint16_t addr) : gb_(gb), addr_(addr) {}

	operator uint16_t() const
	{
		return static_cast<uint16_t>(gb_read(gb_, addr_) | gb_read(gb_, addr_ + 1) << 8);
	}
	Word &operator=(uint16_t v)
	{
		gb_write(gb_, addr_, static_cast<uint8_t>(v));
		gb_write(gb_, static_cast<uint16_t>(addr_ + 1), static_cast<uint8_t>(v >> 8));
		return *this;
	}
	Word &operator=(const Word &other) { return *this = static_cast<uint16_t>(other); }
	Byte lo() const { return Byte(gb_, addr_); }
	Byte hi() const { return Byte(gb_, static_cast<uint16_t>(addr_ + 1)); }
	uint16_t addr() const { return addr_; }

private:
	GB *gb_;
	uint16_t addr_;
};

inline Word Byte::word() const { return Word(gb_, addr_); }

// The Game Boy's address space, as the CPU sees it.
class Memory {
public:
	explicit Memory(GB *gb) : gb_(gb) {}

	Byte operator[](uint16_t addr) const { return Byte(gb_, addr); }
	Byte byte(uint16_t addr) const { return Byte(gb_, addr); }
	Word word(uint16_t addr) const { return Word(gb_, addr); }
	GB *gb() const { return gb_; }

private:
	GB *gb_;
};

} // namespace wl3

// An entry point of a C++ module, registered as a hook on the label `name`
// like PORT_EX in ported.h (tools/gen.c finds these too). `clobbers` lists
// the registers (R_* in hooks.h) that its callers don't use after it, whose
// exit values needn't match the original's; the others must.
#define PORT_EX(name, flags, clobbers) \
	static void port_##name(GB *gb); \
	extern "C" const PortInfo portinfo_##name; \
	const PortInfo portinfo_##name = {#name, port_##name, (flags), (clobbers)}; \
	static void port_##name(GB *gb)
#define PORT(name) PORT_EX(name, 0, 0)

// A local label (Parent.local), as PORT_LOCAL_EX in ported.h.
#define PORT_LOCAL_EX(parent, local, flags, clobbers) \
	static void port_##parent##__##local(GB *gb); \
	extern "C" const PortInfo portinfo_##parent##__##local; \
	const PortInfo portinfo_##parent##__##local = {#parent "." #local, port_##parent##__##local, (flags), (clobbers)}; \
	static void port_##parent##__##local(GB *gb)

// A HOOK_MANUAL_RET entry point's jump onwards (as in ported.h).
#define JUMP_TO(addr) (gb->cpu.pc = static_cast<uint16_t>(addr))

// Every register: for an entry point whose callers use none after it.
#define ALL_REGISTERS (R_A | R_F | R_B | R_C | R_D | R_E | R_H | R_L)

// A routine the module implements inside, with no hook of its own (its
// callers are all in the module): marks it ported for the progress report.
// Use as PORT_COVERED(Label);
#define PORT_COVERED(name) extern int port_covered_##name
