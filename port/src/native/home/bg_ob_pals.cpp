// home/bg_ob_pals (src/home/bg_ob_pals.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeBgObPals : public AsmModule {
public:
	using AsmModule::AsmModule;

	// SetWarioPal, LoadCollectedTreasurePal_Level and CopyAndApplyOBPals wait
	// for H-Blank with idle().

	// Reads all 8 palettes from the palette data register through the index
	// register into hl.
	void store_pals(uint16_t dst, uint16_t index_reg, uint16_t data_reg)
	{
		cpu_.e = 0;
		for (int i = 0; i < 8 * PAL_SIZE; i++) {
			wr8(index_reg, cpu_.e);
			cpu_.a = rd8(data_reg);
			wr8(dst++, cpu_.a);
			cpu_.e++;
		}
		SET_HL(gb_, dst);
		cpu_.b = 0;
		cpu_.c = (uint8_t)data_reg;
		cpu_.f = FLAG_Z | FLAG_N; // xor a cleared carry; final dec b
	}

	// Writes all 8 palettes from hl to the palette data register, after selecting
	// palette 0 with auto-increment.
	void apply_pals(uint16_t src, uint16_t index_reg, uint8_t index, uint16_t data_reg)
	{
		cpu_.a = index;
		wr8(index_reg, index);
		for (int i = 0; i < 8 * PAL_SIZE; i++) {
			cpu_.a = rd8(src++);
			wr8(data_reg, cpu_.a);
		}
		SET_HL(gb_, src);
		cpu_.b = 0;
		cpu_.c = (uint8_t)data_reg;
		cpu_.f = (uint8_t)((cpu_.f & FLAG_C) | FLAG_Z | FLAG_N); // dec b
	}

	// copies all BG pals to wTempBGPals
	void port_StoreBGPals()
	{
		store_pals(wTempBGPals, rBGPI, rBGPD);
	}

	// copies all OB pals to wTempOBPals
	void port_StoreOBPals()
	{
		store_pals(wTempOBPals, rOBPI, rOBPD);
	}

	// fills BG palette with just white
	void port_FillWhiteBGPal()
	{
		apply_pals(ADDR_PalsWhite, rBGPI, BGPI_AUTOINC | 0, rBGPD);
	}

	// fills OB palette with just white
	void port_FillWhiteOBPal()
	{
		apply_pals(ADDR_PalsWhite, rOBPI, OBPI_AUTOINC | 0, rOBPD);
	}

	// unreferenced
	void port_Func_1ae2()
	{
		SET_HL(gb_, ADDR_PalsWhite);
		SET_DE(gb_, wTempBGPals);
		cpu_.b = 8 * PAL_SIZE;
		CALL(CopyHLToDE);
		SET_HL(gb_, ADDR_PalsWhite);
		cpu_.b = 8 * PAL_SIZE;
		CALL(CopyHLToDE);
	}

	void port_LoadCollectedTreasurePal_ClearScreen()
	{
		// push_wram BANK(wLevelTreasurePals)
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = BANK_wLevelTreasurePals;
		wr8(rWBK, cpu_.a);

		cpu_.a = alu_.and_(rd8(wLevelEndScreen), 0x7f);
		cpu_.a = alu_.dec(cpu_.a);
		cpu_.c = cpu_.a;
		cpu_.b = 0;
		uint16_t hl = alu_.add16(wLevelTreasurePals, REG_BC(gb_));
		cpu_.a = alu_.sub(rd8(hl), OBPAL_TREASURE_YELLOW);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *8
		cpu_.e = cpu_.a;
		cpu_.d = cpu_.b;
		SET_HL(gb_, alu_.add16(ADDR_TreasurePals, REG_DE(gb_)));

		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_TreasurePals;
		bankswitch(cpu_.a);
		SET_DE(gb_, wTempPals2 + 7 * PAL_SIZE);
		cpu_.b = PAL_SIZE;
		FARCALL_BANK(BANK_TreasurePals, CopyHLToDE_Short); // unneeded farcall
		pop_af();
		bankswitch(cpu_.a);

		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	void port_ApplyTempPals1ToBGPals()
	{
		apply_pals(wTempPals1, rBGPI, BGPI_AUTOINC | 0, rBGPD);
	}

	void port_ApplyTempPals2ToOBPals()
	{
		apply_pals(wTempPals2, rOBPI, OBPI_AUTOINC | 0, rOBPD);
	}

	// hl = Wario's palettes (in the "Wario Palettes" bank)
	// Applies them to the first two OB palettes, one palette per HBlank.
	void port_SetWarioPal()
	{
		wr8(wWarioPalsPtr + 0, cpu_.h);
		cpu_.a = cpu_.l;
		wr8(wWarioPalsPtr + 1, cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_WarioDefaultPal; // BANK("Wario Palettes")
		bankswitch(cpu_.a);
		gb_push16(gb_, REG_HL(gb_));
		SET_DE(gb_, wTempPals2);
		cpu_.b = 2 * PAL_SIZE;
		cpu_.a = BANK_WarioDefaultPal;
		wr8(hCallFuncBank, cpu_.a);
		HCALL(CopyHLToDE_Short);
		SET_HL(gb_, gb_pop16(gb_));

		cpu_.a = OBPI_AUTOINC | 0;
		wr8(rOBPI, cpu_.a);
		cpu_.b = 2;
		cpu_.c = (uint8_t)rOBPD;
		uint16_t hl = REG_HL(gb_);
		do {
			// wait_not_hblank / wait_hblank
			while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
				idle(32);
			while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
				idle(32);
			for (int i = 0; i < PAL_SIZE; i++) {
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);

		pop_af();
		bankswitch(cpu_.a);
	}

	void port_LoadCollectedTreasurePal_Level()
	{
		// push_wram BANK(wLevelTreasurePals)
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = BANK_wLevelTreasurePals;
		wr8(rWBK, cpu_.a);

		cpu_.a = alu_.and_(rd8(wLevelEndScreen), 0x7f);
		cpu_.a = alu_.dec(cpu_.a);
		cpu_.c = cpu_.a;
		cpu_.b = 0;
		uint16_t hl = alu_.add16(wLevelTreasurePals, REG_BC(gb_));
		cpu_.a = alu_.sub(rd8(hl), OBPAL_TREASURE_YELLOW);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *8
		cpu_.e = cpu_.a;
		cpu_.d = cpu_.b;
		SET_HL(gb_, alu_.add16(ADDR_TreasurePals, REG_DE(gb_)));

		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_TreasurePals;
		bankswitch(cpu_.a);
		gb_push16(gb_, REG_HL(gb_));
		SET_DE(gb_, wTempPals2 + 4 * PAL_SIZE);
		cpu_.a = cpu_.c;
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *8
		cpu_.c = cpu_.a;
		cpu_.a = alu_.add(cpu_.a, cpu_.e);
		cpu_.e = cpu_.a;
		cpu_.b = PAL_SIZE;
		FARCALL_BANK(BANK_TreasurePals, CopyHLToDE_Short);
		hl = gb_pop16(gb_);

		cpu_.a = alu_.or_(OBPI_AUTOINC | (4 * PAL_SIZE), cpu_.c);
		wr8(rOBPI, cpu_.a);
		cpu_.c = (uint8_t)rOBPD;
		// wait_not_hblank / wait_hblank
		while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
		while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
		for (int i = 0; i < PAL_SIZE; i++) {
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		}
		SET_HL(gb_, hl);
		pop_af();
		bankswitch(cpu_.a);

		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	// b = number of pals
	// c = OB palette number
	// hl = palette source
	// de = OB pals dest
	void port_CopyAndApplyOBPals()
	{
		gb_push16(gb_, REG_BC(gb_));
		gb_push16(gb_, REG_HL(gb_));
		cpu_.b = alu_.sla(cpu_.b);
		cpu_.b = alu_.sla(cpu_.b);
		cpu_.b = alu_.sla(cpu_.b); // *8
		CALL(CopyHLToDE_Short);
		uint16_t hl = gb_pop16(gb_);
		SET_BC(gb_, gb_pop16(gb_));
		cpu_.a = cpu_.c;
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *8
		cpu_.a = alu_.or_(cpu_.a, OBPI_AUTOINC);
		wr8(rOBPI, cpu_.a);
		cpu_.c = (uint8_t)rOBPD;
		do {
			// wait_not_hblank / wait_hblank
			while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
				idle(32);
			while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
				idle(32);
			for (int i = 0; i < PAL_SIZE; i++) {
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeBgObPals;

PORT_EX(StoreBGPals, 0, 0) { HomeBgObPals(gb).port_StoreBGPals(); }
PORT_EX(StoreOBPals, 0, 0) { HomeBgObPals(gb).port_StoreOBPals(); }
PORT_EX(FillWhiteBGPal, 0, 0) { HomeBgObPals(gb).port_FillWhiteBGPal(); }
PORT_EX(FillWhiteOBPal, 0, 0) { HomeBgObPals(gb).port_FillWhiteOBPal(); }
PORT_EX(Func_1ae2, 0, 0) { HomeBgObPals(gb).port_Func_1ae2(); }
PORT_EX(LoadCollectedTreasurePal_ClearScreen, 0, 0) { HomeBgObPals(gb).port_LoadCollectedTreasurePal_ClearScreen(); }
PORT_EX(ApplyTempPals1ToBGPals, 0, 0) { HomeBgObPals(gb).port_ApplyTempPals1ToBGPals(); }
PORT_EX(ApplyTempPals2ToOBPals, 0, 0) { HomeBgObPals(gb).port_ApplyTempPals2ToOBPals(); }
PORT_EX(SetWarioPal, 0, 0) { HomeBgObPals(gb).port_SetWarioPal(); }
PORT_EX(LoadCollectedTreasurePal_Level, 0, 0) { HomeBgObPals(gb).port_LoadCollectedTreasurePal_Level(); }
PORT_EX(CopyAndApplyOBPals, 0, 0) { HomeBgObPals(gb).port_CopyAndApplyOBPals(); }
