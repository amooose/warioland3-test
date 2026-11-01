// home/clear_transformation (src/home/clear_transformation.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeClearTransformation : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_PlayRecoverySFX()
	{
		play_sfx(SFX_RECOVERY);
	}

	// unreferenced
	void port_Func_10fc()
	{
		cpu_.a = cpu_.c;
		cpu_.b = 0;
		uint16_t hl = 0;
		for (;;) {
			alu_.cp(cpu_.a, 100);
			if (cpu_.f & FLAG_C)
				break;
			hl = alu_.add16(hl, 0x9c);
			cpu_.a = alu_.sub(cpu_.a, 100);
		}
		cpu_.b = 0;
		for (;;) {
			alu_.cp(cpu_.a, 10);
			if (cpu_.f & FLAG_C)
				break;
			hl = alu_.add16(hl, 0x6);
			cpu_.a = alu_.sub(cpu_.a, 10);
		}
		SET_DE(gb_, 0x6);
		SET_HL(gb_, alu_.add16(hl, REG_BC(gb_)));
	}

	// unreferenced
	void port_Func_111e()
	{
		gb_push16(gb_, REG_HL(gb_));
		cpu_.a = cpu_.h;
		uint16_t hl = 0;
		for (;;) {
			cpu_.a = alu_.and_(cpu_.a, cpu_.a);
			if (!cpu_.a)
				break;
			hl = alu_.add16(hl, (uint16_t)-0x9c);
			cpu_.a = alu_.dec(cpu_.a);
		}
		SET_BC(gb_, hl);
		uint16_t entry_hl = gb_pop16(gb_);
		gb_push16(gb_, entry_hl);
		cpu_.a = (uint8_t)entry_hl;
		hl = 0;
		for (;;) {
			alu_.cp(cpu_.a, 16);
			if (cpu_.f & FLAG_C)
				break;
			hl = alu_.add16(hl, (uint16_t)-0x6);
			cpu_.a = alu_.sub(cpu_.a, 16);
		}
		hl = alu_.add16(hl, REG_BC(gb_));
		SET_DE(gb_, gb_pop16(gb_));
		hl = alu_.add16(hl, REG_DE(gb_));
		SET_HL(gb_, hl);
		cpu_.c = cpu_.l;
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeClearTransformation;

PORT_EX(PlayRecoverySFX, 0, 0) { HomeClearTransformation(gb).port_PlayRecoverySFX(); }
PORT_EX(Func_10fc, 0, 0) { HomeClearTransformation(gb).port_Func_10fc(); }
PORT_EX(Func_111e, 0, 0) { HomeClearTransformation(gb).port_Func_111e(); }
