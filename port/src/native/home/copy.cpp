// home/copy (src/home/copy.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeCopy : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Shared loop of the BC-count copy/fill routines. In the asm, the count is
	// handled as "c, then 256 per remaining b", with b pre-incremented when c is
	// nonzero; a count of 0 means 65536. They all leave b = c = 0.
	uint32_t bc_count()
	{
		uint16_t bc = REG_BC(gb_);
		return bc ? bc : 0x10000;
	}

	// writes a to address in hl, b times
	void port_WriteAToHL_BTimes()
	{
		uint16_t hl = REG_HL(gb_);
		int n = cpu_.b ? cpu_.b : 256;
		for (int i = 0; i < n; i++)
			wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		cpu_.b = 0;
		// flags from the final dec b
		cpu_.f = (uint8_t)((cpu_.f & FLAG_C) | FLAG_Z | FLAG_N);
	}

	// write a to hl, bc times
	void port_WriteAToHL_BCTimes()
	{
		uint16_t hl = REG_HL(gb_);
		uint32_t n = bc_count();
		for (uint32_t i = 0; i < n; i++)
			wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		SET_BC(gb_, 0);
		// push af / pop af keeps the entry carry; the final dec b sets z and n
		cpu_.f = (uint8_t)((cpu_.f & FLAG_C) | FLAG_Z | FLAG_N);
	}

	void copy_bc()
	{
		uint16_t hl = REG_HL(gb_), de = REG_DE(gb_);
		uint32_t n = bc_count();
		for (uint32_t i = 0; i < n; i++) {
			cpu_.a = rd8(hl++);
			wr8(de++, cpu_.a);
		}
		SET_HL(gb_, hl);
		SET_DE(gb_, de);
		SET_BC(gb_, 0);
	}

	// copies bc bytes from hl to de
	void port_CopyHLToDE_BC()
	{
		copy_bc();
		// and a clears carry, the final dec b sets z and n
		cpu_.f = FLAG_Z | FLAG_N;
	}

	// switches bank to wTempBank
	// then copies bc bytes from hl to de
	void port_FarCopyHLToDE_BC()
	{
		uint8_t old_bank = rd8(wROMBank);
		uint8_t entry_f = cpu_.f;
		bankswitch(rd8(wTempBank));
		copy_bc();
		// pop af restores a and f from the push at entry
		cpu_.a = old_bank;
		cpu_.f = entry_f;
		bankswitch(old_bank);
	}

	// copies b bytes from hl to de
	void port_CopyHLToDE()
	{
		uint16_t hl = REG_HL(gb_), de = REG_DE(gb_);
		int n = cpu_.b ? cpu_.b : 256;
		for (int i = 0; i < n; i++) {
			cpu_.a = rd8(hl++);
			wr8(de++, cpu_.a);
		}
		SET_HL(gb_, hl);
		SET_DE(gb_, de);
		cpu_.b = 0;
		cpu_.f = (uint8_t)((cpu_.f & FLAG_C) | FLAG_Z | FLAG_N);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeCopy;

PORT_EX(WriteAToHL_BTimes, 0, 0) { HomeCopy(gb).port_WriteAToHL_BTimes(); }
PORT_EX(WriteAToHL_BCTimes, 0, 0) { HomeCopy(gb).port_WriteAToHL_BCTimes(); }
PORT_EX(CopyHLToDE_BC, 0, 0) { HomeCopy(gb).port_CopyHLToDE_BC(); }
PORT_EX(FarCopyHLToDE_BC, 0, 0) { HomeCopy(gb).port_FarCopyHLToDE_BC(); }
PORT_EX(CopyHLToDE, 0, 0) { HomeCopy(gb).port_CopyHLToDE(); }
