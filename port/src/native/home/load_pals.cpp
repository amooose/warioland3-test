// home/load_pals (src/home/load_pals.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadPals : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Writes b bytes from hl to the palette data register c, after selecting
	// palette 0 with auto-increment through the index register.
	void load_pals_from_wram(uint16_t src, uint16_t index_reg, uint8_t index, uint16_t data_reg)
	{
		wr8(index_reg, index);
		cpu_.a = index;
		for (int i = 0; i < 8 * PAL_SIZE; i++) {
			cpu_.a = rd8(src++);
			wr8(data_reg, cpu_.a);
		}
		SET_HL(gb_, src);
		cpu_.b = 0;
		cpu_.c = (uint8_t)data_reg;
		cpu_.f = (uint8_t)((cpu_.f & FLAG_C) | FLAG_Z | FLAG_N); // dec b
	}

	void port_LoadBGPalettesFromWRAM()
	{
		load_pals_from_wram(wTempBGPals, rBGPI, BGPI_AUTOINC | 0, rBGPD);
	}

	void port_LoadOBPalettesFromWRAM()
	{
		load_pals_from_wram(wTempOBPals, rOBPI, OBPI_AUTOINC | 0, rOBPD);
	}

	// Copies b bytes from hl to de, incrementing only e (the destination never
	// crosses a page boundary).
	void copy_short(uint16_t de, int count)
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.d = (uint8_t)(de >> 8);
		cpu_.e = (uint8_t)de;
		for (int i = 0; i < count; i++) {
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(cpu_.d << 8 | cpu_.e), cpu_.a);
			cpu_.e++;
		}
		SET_HL(gb_, hl);
		cpu_.b = 0;
		cpu_.f = (uint8_t)((cpu_.f & FLAG_C) | FLAG_Z | FLAG_N); // dec b
	}

	// copies hl to wTempPals1
	void port_LoadPalsToTempPals1()
	{
		copy_short(wTempPals1, 8 * PAL_SIZE);
	}

	// copies hl to wTempPals2
	void port_LoadPalsToTempPals2()
	{
		copy_short(wTempPals2, 8 * PAL_SIZE);
	}

	// copies b bytes from hl to de
	// given that e doesn't overflow
	void port_CopyHLToDE_Short()
	{
		int n = cpu_.b ? cpu_.b : 256;
		copy_short(REG_DE(gb_), n);
	}

	// hl = 4 OB palettes, copied to wTempPals2 palettes 4-7
	void port_LoadObjPals()
	{
		copy_short(wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadPals;

PORT_EX(LoadBGPalettesFromWRAM, 0, 0) { HomeLoadPals(gb).port_LoadBGPalettesFromWRAM(); }
PORT_EX(LoadOBPalettesFromWRAM, 0, 0) { HomeLoadPals(gb).port_LoadOBPalettesFromWRAM(); }
PORT_EX(LoadPalsToTempPals1, 0, 0) { HomeLoadPals(gb).port_LoadPalsToTempPals1(); }
PORT_EX(LoadPalsToTempPals2, 0, 0) { HomeLoadPals(gb).port_LoadPalsToTempPals2(); }
PORT_EX(CopyHLToDE_Short, 0, 0) { HomeLoadPals(gb).port_CopyHLToDE_Short(); }
PORT_EX(LoadObjPals, 0, 0) { HomeLoadPals(gb).port_LoadObjPals(); }
