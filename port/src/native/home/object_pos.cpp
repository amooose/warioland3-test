// home/object_pos (src/home/object_pos.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeObjectPos : public AsmModule {
public:
	using AsmModule::AsmModule;

	#define SRAM_HI 0xa0 // HIGH(STARTOF(SRAM))

	// and $0f / swap a
	uint8_t lo_nybble_up(uint8_t v)
	{
		return alu_.swap(alu_.and_(v, 0x0f));
	}

	// swap a / and $0f
	uint8_t hi_nybble_down(uint8_t v)
	{
		return alu_.and_(alu_.swap(v), 0x0f);
	}

	// and $f0 / swap a
	uint8_t and_f0_swap(uint8_t v)
	{
		return alu_.swap(alu_.and_(v, 0xf0));
	}

	// input:
	// - hl = hPos
	// output:
	// - l = x block
	// - h = y block
	// - b = 1 if even object, 0 if odd object
	void port_Func_cc0()
	{
		uint16_t hl = REG_HL(gb_);
		// [hl] = y pos
		cpu_.d = rd8(hl++);
		cpu_.c = hi_nybble_down(rd8(hl++));
		cpu_.a = lo_nybble_up(cpu_.d);
		cpu_.a = alu_.add(cpu_.a, cpu_.c);
		cpu_.a = alu_.add(cpu_.a, SRAM_HI);
		cpu_.c = cpu_.a;
		wr8(wObjectSpawnBlockY, cpu_.a);

		// [hl] = x pos
		cpu_.d = rd8(hl++);
		uint8_t l = hi_nybble_down(rd8(hl));
		cpu_.a = lo_nybble_up(cpu_.d);
		cpu_.a = alu_.add(cpu_.a, l);
		l = alu_.srl(cpu_.a);
		// ld a, 0 / adc 0 / xor $1: a is 1 if x was even, 0 if odd
		cpu_.a = alu_.adc(0, 0);
		cpu_.a = alu_.xor_(cpu_.a, 1);
		wr8(wUnused_ObjectParity, cpu_.a);
		cpu_.b = cpu_.a; // object parity

		cpu_.a = alu_.add(l, 0xb0);
		cpu_.l = cpu_.a;
		wr8(wObjectSpawnBlockX, cpu_.a);
		cpu_.h = cpu_.c;
	}

	// ldh a, [hXXXPosHi] / adc 0 / ldh [hXXXPosHi], a
	void adc_hi(uint16_t addr)
	{
		cpu_.a = alu_.adc(rd8(addr), 0);
		wr8(addr, cpu_.a);
	}

	void port_Func_cf8()
	{
		gb_push16(gb_, REG_HL(gb_));
		cpu_.a = alu_.sub(cpu_.h, SRAM_HI);
		cpu_.h = cpu_.a;
		cpu_.d = and_f0_swap(cpu_.a);
		cpu_.a = alu_.dec(rd8(wccec));
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.d);
		wr8(hYPosHi, cpu_.a);
		cpu_.a = lo_nybble_up(cpu_.h);
		cpu_.a = alu_.add(cpu_.a, 0x10);
		wr8(hYPosLo, cpu_.a);
		adc_hi(hYPosHi);

		cpu_.a = alu_.xor_(cpu_.b, 0x01);
		for (int i = 0; i < 4; i++)
			cpu_.a = alu_.add(cpu_.a, cpu_.a); // *16
		cpu_.a = alu_.add(cpu_.a, 0x08);
		cpu_.d = cpu_.a;
		cpu_.a = alu_.sub(cpu_.l, 0xb0);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.l = cpu_.a;
		cpu_.a = and_f0_swap(cpu_.a);
		wr8(hXPosHi, cpu_.a);
		cpu_.a = lo_nybble_up(cpu_.l);
		cpu_.a = alu_.add(cpu_.a, cpu_.d);
		wr8(hXPosLo, cpu_.a);
		adc_hi(hXPosHi);
		SET_HL(gb_, gb_pop16(gb_));
	}

	void port_Func_d3e()
	{
		cpu_.a = alu_.sub(cpu_.h, SRAM_HI);
		cpu_.d = cpu_.a;
		cpu_.b = and_f0_swap(cpu_.a);
		cpu_.a = alu_.dec(rd8(wBlockPtrBank));
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.b);
		wr8(hYPosHi, cpu_.a);
		cpu_.a = lo_nybble_up(cpu_.d);
		cpu_.a = alu_.add(cpu_.a, 0x10);
		wr8(hYPosLo, cpu_.a);
		adc_hi(hYPosHi);
		cpu_.a = alu_.sub(cpu_.l, 0x00);
		cpu_.e = cpu_.a;
		cpu_.a = and_f0_swap(cpu_.a);
		wr8(hXPosHi, cpu_.a);
		cpu_.a = lo_nybble_up(cpu_.e);
		cpu_.a = alu_.add(cpu_.a, 0x08);
		wr8(hXPosLo, cpu_.a);
		adc_hi(hXPosHi);
		cpu_.a = alu_.and_(cpu_.l, 1);
		cpu_.a = alu_.xor_(cpu_.a, 1);
		cpu_.b = cpu_.a;
		wr8(wUnused_ObjectParity, cpu_.a);
	}

	void port_Func_d81()
	{
		cpu_.a = alu_.sub(cpu_.l, 0xb0);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.l = cpu_.a;
		cpu_.a = alu_.xor_(cpu_.b, 0x1);
		cpu_.a = alu_.add(cpu_.a, cpu_.l);
		cpu_.l = cpu_.a;
	}

	void port_Func_d8c()
	{
		cpu_.a = rd8(wBlockPtrBank);
		wr8(wccec, cpu_.a);
		cpu_.b = 0x01;
		cpu_.a = alu_.srl(cpu_.l);
		if (cpu_.f & FLAG_C)
			cpu_.b = alu_.dec(cpu_.b);
		cpu_.a = alu_.add(cpu_.a, 0xb0);
		cpu_.l = cpu_.a;
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeObjectPos;

PORT_EX(Func_cc0, 0, 0) { HomeObjectPos(gb).port_Func_cc0(); }
PORT_EX(Func_cf8, 0, 0) { HomeObjectPos(gb).port_Func_cf8(); }
PORT_EX(Func_d3e, 0, 0) { HomeObjectPos(gb).port_Func_d3e(); }
PORT_EX(Func_d81, 0, 0) { HomeObjectPos(gb).port_Func_d81(); }
PORT_EX(Func_d8c, 0, 0) { HomeObjectPos(gb).port_Func_d8c(); }
