// The CPU's 8-bit arithmetic on its own flags, for the C++ entry points that
// still have to leave the original's registers and flags (their callers
// are register-level C): each returns the result and sets cpu.f as the
// instruction does (the same as ported.h's alu_*). For a single flag
// result without the CPU, see flags.hpp.
#pragma once

#include "gb.h"

#include <cstdint>

namespace wl3 {

class Alu {
public:
	explicit Alu(CPU &cpu) : cpu_(cpu) {}

	bool zf() const { return cpu_.f & FLAG_Z; }
	bool cf() const { return cpu_.f & FLAG_C; }

	uint8_t add(uint8_t a, uint8_t v)
	{
		unsigned r = a + v;
		cpu_.f = static_cast<uint8_t>(((r & 0xff) ? 0 : FLAG_Z) | (((a & 0xf) + (v & 0xf)) > 0xf ? FLAG_H : 0)
			| (r > 0xff ? FLAG_C : 0));
		return static_cast<uint8_t>(r);
	}
	uint8_t adc(uint8_t a, uint8_t v)
	{
		unsigned carry = cf();
		unsigned r = a + v + carry;
		cpu_.f = static_cast<uint8_t>(((r & 0xff) ? 0 : FLAG_Z)
			| (((a & 0xf) + (v & 0xf) + carry) > 0xf ? FLAG_H : 0) | (r > 0xff ? FLAG_C : 0));
		return static_cast<uint8_t>(r);
	}
	uint8_t sub(uint8_t a, uint8_t v)
	{
		cpu_.f = static_cast<uint8_t>((a == v ? FLAG_Z : 0) | FLAG_N | ((a & 0xf) < (v & 0xf) ? FLAG_H : 0)
			| (a < v ? FLAG_C : 0));
		return static_cast<uint8_t>(a - v);
	}
	uint8_t sbc(uint8_t a, uint8_t v)
	{
		int carry = cf();
		int r = a - v - carry;
		cpu_.f = static_cast<uint8_t>(((r & 0xff) ? 0 : FLAG_Z) | FLAG_N
			| ((a & 0xf) - (v & 0xf) - carry < 0 ? FLAG_H : 0) | (r < 0 ? FLAG_C : 0));
		return static_cast<uint8_t>(r);
	}
	void cp(uint8_t a, uint8_t v) { sub(a, v); }
	uint8_t and_(uint8_t a, uint8_t v)
	{
		uint8_t r = a & v;
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | FLAG_H);
		return r;
	}
	uint8_t or_(uint8_t a, uint8_t v)
	{
		uint8_t r = a | v;
		cpu_.f = r ? 0 : FLAG_Z;
		return r;
	}
	uint8_t xor_(uint8_t a, uint8_t v)
	{
		uint8_t r = a ^ v;
		cpu_.f = r ? 0 : FLAG_Z;
		return r;
	}
	uint8_t inc(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v + 1);
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_C) | (r ? 0 : FLAG_Z) | ((r & 0xf) == 0 ? FLAG_H : 0));
		return r;
	}
	uint8_t dec(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v - 1);
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_C) | FLAG_N | (r ? 0 : FLAG_Z) | ((r & 0xf) == 0xf ? FLAG_H : 0));
		return r;
	}
	uint8_t swap(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v << 4 | v >> 4);
		cpu_.f = r ? 0 : FLAG_Z;
		return r;
	}
	uint8_t srl(uint8_t v)
	{
		uint8_t r = v >> 1;
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
		return r;
	}
	uint8_t sra(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v >> 1 | (v & 0x80));
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
		return r;
	}
	uint8_t sla(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v << 1);
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
		return r;
	}
	// rl r (the CB one, which sets z)
	uint8_t rl(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v << 1 | cf());
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
		return r;
	}
	// rr r (the CB one)
	uint8_t rr(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v >> 1 | (cf() ? 0x80 : 0));
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
		return r;
	}
	// rlc r / rrc r (the CB ones)
	uint8_t rlc(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v << 1 | v >> 7);
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
		return r;
	}
	uint8_t rrc(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v >> 1 | v << 7);
		cpu_.f = static_cast<uint8_t>((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
		return r;
	}
	// rla / rra / rlca / rrca: z clear
	uint8_t rla(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v << 1 | cf());
		cpu_.f = (v & 0x80) ? FLAG_C : 0;
		return r;
	}
	uint8_t rra(uint8_t v)
	{
		uint8_t r = static_cast<uint8_t>(v >> 1 | (cf() ? 0x80 : 0));
		cpu_.f = (v & 1) ? FLAG_C : 0;
		return r;
	}
	uint8_t rlca(uint8_t v)
	{
		cpu_.f = (v & 0x80) ? FLAG_C : 0;
		return static_cast<uint8_t>(v << 1 | v >> 7);
	}
	uint8_t rrca(uint8_t v)
	{
		cpu_.f = (v & 1) ? FLAG_C : 0;
		return static_cast<uint8_t>(v >> 1 | v << 7);
	}
	// cpl: n and h set, z and carry kept
	uint8_t cpl(uint8_t v)
	{
		cpu_.f |= FLAG_N | FLAG_H;
		return static_cast<uint8_t>(~v);
	}
	// bit n, r: z if the bit's clear, h set, carry kept
	bool bit(uint8_t v, int n)
	{
		bool set = (v >> n) & 1;
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_C) | FLAG_H | (set ? 0 : FLAG_Z));
		return set;
	}
	// daa: the BCD adjust after an add or sub
	uint8_t daa(uint8_t v)
	{
		int a = v;
		bool c = cf();
		if (!(cpu_.f & FLAG_N)) {
			if (c || a > 0x99) {
				a += 0x60;
				c = true;
			}
			if ((cpu_.f & FLAG_H) || (a & 0xf) > 9)
				a += 0x06;
		} else {
			if (c)
				a -= 0x60;
			if (cpu_.f & FLAG_H)
				a -= 0x06;
		}
		uint8_t r = static_cast<uint8_t>(a);
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_N) | (r ? 0 : FLAG_Z) | (c ? FLAG_C : 0));
		return r;
	}
	// add hl, rr: z kept
	uint16_t add16(uint16_t hl, uint16_t v)
	{
		uint32_t r = static_cast<uint32_t>(hl) + v;
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | (((hl & 0xfff) + (v & 0xfff)) > 0xfff ? FLAG_H : 0)
			| (r > 0xffff ? FLAG_C : 0));
		return static_cast<uint16_t>(r);
	}

private:
	CPU &cpu_;
};

} // namespace wl3
