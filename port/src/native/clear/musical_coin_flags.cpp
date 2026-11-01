// clear/musical_coin_flags (src/engine/clear/musical_coin_flags.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::clear {

namespace {

class ClearMusicalCoinFlags : public AsmModule {
public:
	using AsmModule::AsmModule;

	// which home code calls with registers (the rest is C++, src/native/clear)

	// rrc r
	uint8_t alu_rrc(uint8_t v)
	{
		uint8_t r = (uint8_t)(v >> 1 | v << 7);
		cpu_.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
		return r;
	}

	// rlc r
	uint8_t alu_rlc(uint8_t v)
	{
		uint8_t r = (uint8_t)(v << 1 | v >> 7);
		cpu_.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 0x80) ? FLAG_C : 0));
		return r;
	}

	// rotates the flag byte at hl right n times, then back
	void rotate_back(uint16_t hl)
	{
		alu_.and_(cpu_.a, cpu_.a);
		if (!cpu_.a)
			return; // no need to rotate back
		do {
			wr8(hl, alu_.rlc(rd8(hl)));
			cpu_.a = alu_.dec(cpu_.a);
		} while (cpu_.a);
	}

	// hl = flag byte, a = c = bit: rrc [hl] c times
	void rotate_to_bit(uint16_t hl)
	{
		cpu_.a = alu_.and_(cpu_.c, 0x07);
		cpu_.c = cpu_.a;
		if (!cpu_.a)
			return;
		do {
			wr8(hl, alu_.rrc(rd8(hl)));
			cpu_.c = alu_.dec(cpu_.c);
		} while (cpu_.c);
	}

	// check if a bit is set in wMusicalCoinFlags
	// for the level in wOWLevel
	// returns b = TRUE if set
	void port__CheckLevelMusicalCoinFlag()
	{
		cpu_.a = alu_.dec(rd8(wOWLevel));
		cpu_.c = cpu_.a;
		cpu_.a = alu_.srl(cpu_.a);
		cpu_.a = alu_.srl(cpu_.a);
		cpu_.a = alu_.srl(cpu_.a); // /8
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(wMusicalCoinFlags, REG_DE(gb_));
		SET_HL(gb_, hl);
		rotate_to_bit(hl);
		cpu_.c = cpu_.a;
		cpu_.b = alu_.bit(rd8(hl), 0) ? TRUE : FALSE;
		cpu_.a = cpu_.c;
		rotate_back(hl);
	}
};

} // namespace

} // namespace wl3::clear

using wl3::clear::ClearMusicalCoinFlags;

PORT_EX(_CheckLevelMusicalCoinFlag, 0, 0) { ClearMusicalCoinFlags(gb).port__CheckLevelMusicalCoinFlag(); }
