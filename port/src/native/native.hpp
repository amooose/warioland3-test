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
	Byte(const Byte &) = default; // another view of the same byte

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
	Word(const Word &) = default;

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

// A big-endian word in memory: its high byte first (the game's positions in
// the level, pointers it keeps as hi / lo, the sound IDs)
class WordBE {
public:
	WordBE(GB *gb, uint16_t addr) : gb_(gb), addr_(addr) {}
	WordBE(const WordBE &) = default;

	operator uint16_t() const { return static_cast<uint16_t>(hi() << 8 | lo()); }
	WordBE &operator=(uint16_t v)
	{
		hi() = static_cast<uint8_t>(v >> 8);
		lo() = static_cast<uint8_t>(v);
		return *this;
	}
	WordBE &operator=(const WordBE &other) { return *this = static_cast<uint16_t>(other); }
	Byte hi() const { return Byte(gb_, addr_); }
	Byte lo() const { return Byte(gb_, static_cast<uint16_t>(addr_ + 1)); }
	uint16_t addr() const { return addr_; }

private:
	GB *gb_;
	uint16_t addr_;
};

// The Game Boy's address space, as the CPU sees it.
class Memory {
public:
	explicit Memory(GB *gb) : gb_(gb) {}

	Byte operator[](uint16_t addr) const { return Byte(gb_, addr); }
	Byte byte(uint16_t addr) const { return Byte(gb_, addr); }
	Word word(uint16_t addr) const { return Word(gb_, addr); }
	GB *gb() const { return gb_; }

	// The RAM by name (ram_fields.h, from the disassembly's labels: wFooBar
	// -> foo_bar()): the byte at the label, as operator[] gives it
#define WRAM(name, label) Byte name() const { return Byte(gb_, label); }
#define HRAM(name, label) Byte name() const { return Byte(gb_, label); }
#include "ram_fields.h"
#undef WRAM
#undef HRAM

	// The 16-bit fields, as words of their byte order
	WordBE wario_x() const { return WordBE(gb_, wWarioXPos); }
	WordBE wario_y() const { return WordBE(gb_, wWarioYPos); }
	WordBE block_y() const { return WordBE(gb_, wBlockYPos); }
	WordBE block_x() const { return WordBE(gb_, wBlockXPos); }
	WordBE block_pointer() const { return WordBE(gb_, wBlockPtr); }
	WordBE bg_pointer() const { return WordBE(gb_, wBGPtr); }
	WordBE sfx() const { return WordBE(gb_, hSFXID); }
	WordBE music() const { return WordBE(gb_, hMusicID); }
	Word cur_obj_y() const { return Word(gb_, wCurObjYPos); }
	Word cur_obj_x() const { return Word(gb_, wCurObjXPos); }

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

// hTransferVirtualOAM (a = wVirtualOAM's page), as a direct call
inline void transfer_virtual_oam(GB *gb)
{
	if (!ram_transfer_virtual_oam(gb))
		gb_call(gb, hTransferVirtualOAM);
}

// A direct call into the routine at addr (in the bank mapped now), which the
// CPU never reaches: lets the per-frame trace's PC callback see the entry as
// if it had (trace.c's sites, which hand the native code the asm's LY read
// there). Through the routine's hook, the CPU already did.
inline void trace_entry(GB *gb, uint16_t addr)
{
	if (!gb->pc_cb || gb->cpu.pc == addr)
		return;
	uint16_t pc = gb->cpu.pc;
	gb->cpu.pc = addr;
	gb->pc_cb(gb);
	gb->cpu.pc = pc;
}

// A routine the module implements inside, with no hook of its own (its
// callers are all in the module): marks it ported for the progress report.
// Use as PORT_COVERED(Label);
#define PORT_COVERED(name) extern int port_covered_##name
