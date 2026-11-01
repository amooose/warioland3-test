// The flags an instruction leaves, for entry points whose callers test the
// flags the original returns with (ret c after a cp, a jr z after the
// call...). Each takes the operands and gives the F register's value.
#pragma once

#include "gb.h"

#include <cstdint>

namespace wl3::flags {

constexpr uint8_t z(uint8_t r) { return r ? 0 : FLAG_Z; }

// add a, v / adc (with carry in)
constexpr uint8_t add(uint8_t a, uint8_t v, bool carry = false)
{
	unsigned r = a + v + carry;
	return static_cast<uint8_t>(z(static_cast<uint8_t>(r)) | (((a & 0xf) + (v & 0xf) + carry) > 0xf ? FLAG_H : 0)
		| (r > 0xff ? FLAG_C : 0));
}

// sub v / sbc (with carry in) / cp v
constexpr uint8_t sub(uint8_t a, uint8_t v, bool carry = false)
{
	int r = a - v - carry;
	return static_cast<uint8_t>(FLAG_N | z(static_cast<uint8_t>(r)) | ((a & 0xf) - (v & 0xf) - carry < 0 ? FLAG_H : 0)
		| (r < 0 ? FLAG_C : 0));
}
constexpr uint8_t cp(uint8_t a, uint8_t v) { return sub(a, v); }

// inc r / dec r: the carry is kept
constexpr uint8_t inc(uint8_t f, uint8_t old)
{
	uint8_t r = static_cast<uint8_t>(old + 1);
	return static_cast<uint8_t>((f & FLAG_C) | z(r) | ((old & 0xf) == 0xf ? FLAG_H : 0));
}
constexpr uint8_t dec(uint8_t f, uint8_t old)
{
	uint8_t r = static_cast<uint8_t>(old - 1);
	return static_cast<uint8_t>((f & FLAG_C) | FLAG_N | z(r) | ((old & 0xf) == 0 ? FLAG_H : 0));
}

// and / or / xor, with their result
constexpr uint8_t and_(uint8_t r) { return static_cast<uint8_t>(z(r) | FLAG_H); }
constexpr uint8_t or_(uint8_t r) { return z(r); }
constexpr uint8_t xor_(uint8_t r) { return z(r); }

// bit n, r: the carry is kept
constexpr uint8_t bit(uint8_t f, uint8_t r, int n)
{
	return static_cast<uint8_t>((f & FLAG_C) | FLAG_H | (((r >> n) & 1) ? 0 : FLAG_Z));
}

// The rotates through the carry, on the CPU's own carry and flags (z
// clear, as the A-register forms leave it)
inline uint8_t rra(CPU &cpu, uint8_t v)
{
	uint8_t r = static_cast<uint8_t>(v >> 1 | ((cpu.f & FLAG_C) ? 0x80 : 0));
	cpu.f = (v & 1) ? FLAG_C : 0;
	return r;
}
inline uint8_t rla(CPU &cpu, uint8_t v)
{
	uint8_t r = static_cast<uint8_t>(v << 1 | ((cpu.f & FLAG_C) ? 1 : 0));
	cpu.f = (v & 0x80) ? FLAG_C : 0;
	return r;
}

} // namespace wl3::flags
