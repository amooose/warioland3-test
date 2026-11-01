// action_help/pals (src/engine/action_help/pals.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::action_help {

namespace {

class ActionHelpPals : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Action Help's register-level parts (see src/native/action_help): the
	// palette loaders, which copy one palette per HBlank, waiting on rSTAT as
	// the original does; the V-Blank function copied to wVBlankFunc; and the
	// unreferenced Func_1e4b6d.

	// hl = y, x, frame and attributes; y is relative to the background scroll
	uint16_t set_cur_sprite()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.b = rd8(wSCY);
		wr8(wCurSpriteYCoord, alu_.add(alu_.sub(rd8(hl++), cpu_.b), OAM_Y_OFS));
		wr8(wCurSpriteXCoord, alu_.add(rd8(hl++), OAM_X_OFS));
		wr8(wCurSpriteFrame, rd8(hl++));
		cpu_.a = rd8(hl);
		wr8(wCurSpriteAttributes, cpu_.a);
		return hl;
	}

	// Copies b palettes from hl to the palette data register c, one palette
	// per HBlank (wait_not_hblank / wait_hblank).
	void copy_pals_hblank()
	{
		uint16_t hl = REG_HL(gb_);
		do {
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

	// push hl / ld de, dst / ld b, n / call CopyHLToDE_Short / pop hl
	void copy_pals_keep_hl(uint16_t dst, uint8_t n)
	{
		uint16_t hl = REG_HL(gb_);
		gb_push16(gb_, hl);
		SET_DE(gb_, dst);
		cpu_.b = n;
		CALL(CopyHLToDE_Short);
		SET_HL(gb_, gb_pop16(gb_));
	}

	void write_a_to_hl(uint16_t hl, uint8_t n)
	{
		SET_HL(gb_, hl);
		cpu_.b = n;
		CALL(WriteAToHL_BTimes);
	}

	// Returns whether the power-up being demonstrated is a main one; if so,
	// leaves the carry of comparing it with Wario's power-up level.
	bool cp_power_up_level()
	{
		cpu_.a = rd8(wActionHelpPowerUp);
		alu_.cp(cpu_.a, NUM_MAIN_POWER_UPS);
		if (!(cpu_.f & FLAG_C))
			return false;
		cpu_.b = cpu_.a;
		cpu_.a = alu_.and_(rd8(wPowerUpLevel), POWER_UP_MASK);
		alu_.cp(cpu_.a, cpu_.b);
		return true;
	}

	void port_LoadActionHelpOBPals()
	{
		SET_HL(gb_, ADDR_Pals_1e4f2f);
		if (!cp_power_up_level())
			return; // special Power-Up
		if (cpu_.f & FLAG_C) {
			// doesn't have Power-Up, so black out some OB pals
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			write_a_to_hl(wTempPals2, 2 * PAL_SIZE);
			write_a_to_hl(wTempPals2 + 3 * PAL_SIZE, 5 * PAL_SIZE);
			SET_HL(gb_, wTempPals2);
		} else {
			copy_pals_keep_hl(wTempPals2, 8 * PAL_SIZE);
		}
		cpu_.a = OBPI_AUTOINC | 0;
		wr8(rOBPI, cpu_.a);
		cpu_.b = 8;
		cpu_.c = (uint8_t)rOBPD;
		copy_pals_hblank();
	}

	void port_Func_1e4a47()
	{
		SET_HL(gb_, ADDR_Pals_1e4f2f + 4 * PAL_SIZE);
		if (!cp_power_up_level())
			return;
		if (cpu_.f & FLAG_C) {
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			write_a_to_hl(wTempPals2 + 4 * PAL_SIZE, PAL_SIZE);
			write_a_to_hl(wTempPals2 + 7 * PAL_SIZE, PAL_SIZE);
			SET_HL(gb_, wTempPals2 + 4 * PAL_SIZE);
		} else {
			uint16_t hl = REG_HL(gb_);
			gb_push16(gb_, hl);
			SET_DE(gb_, wTempPals2 + 4 * PAL_SIZE);
			cpu_.b = PAL_SIZE;
			CALL(CopyHLToDE_Short);
			SET_HL(gb_, ADDR_Pals_1e4f2f + 7 * PAL_SIZE);
			SET_DE(gb_, wTempPals2 + 7 * PAL_SIZE);
			cpu_.b = PAL_SIZE;
			CALL(CopyHLToDE_Short);
			SET_HL(gb_, gb_pop16(gb_));
		}
		cpu_.a = OBPI_AUTOINC | (4 * PAL_SIZE);
		wr8(rOBPI, cpu_.a);
		cpu_.c = (uint8_t)rOBPD;
		cpu_.b = 4;
		copy_pals_hblank();
	}

	// hl = 8 BG palettes: copies them to wTempPals1 and to the BG palette RAM
	void port_LoadActionHelpBGPals()
	{
		copy_pals_keep_hl(wTempPals1, 8 * PAL_SIZE);
		cpu_.a = BGPI_AUTOINC | 0;
		wr8(rBGPI, cpu_.a);
		cpu_.b = 8;
		cpu_.c = (uint8_t)rBGPD;
		copy_pals_hblank();
	}

	void port_LoadActionHelpPalsWithWater()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wActionHelpBlackedOut, 0);
		SET_HL(gb_, ADDR_Pals_1e4e6f);
		gb_call(gb_, ADDR_LoadActionHelpBGPals);
		gb_call(gb_, ADDR_LoadActionHelpOBPals);
	}

	void port_LoadActionHelpPalsWithoutWater()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wActionHelpBlackedOut, 0);
		SET_HL(gb_, ADDR_Pals_1e4eaf);
		gb_call(gb_, ADDR_LoadActionHelpBGPals);
		gb_call(gb_, ADDR_LoadActionHelpOBPals);
	}

	void port_Func_1e48bc()
	{
		cpu_.a = TRUE;
		wr8(wActionHelpBlackedOut, TRUE);
		SET_HL(gb_, ADDR_Pals_1e4eef);
		gb_call(gb_, ADDR_LoadActionHelpBGPals);
	}

	void port_Func_1e4b6d()
	{
		set_cur_sprite();
		SET_HL(gb_, ADDR_OAM_1e6bab);
		CALL(LoadSprite);
	}

	// VBlank_1fb6b7.Func, as copied to wVBlankFunc (see ram_code.c). Its call
	// returns to base + n, the block's start wherever it runs from.
	void port_VBlank_1fb6b7__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wIsDMATransferPending);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			uint16_t hl = wPendingDMASourceBank;
			cpu_.a = rd8(hl++);
			wr8(rROMB0 + 0x100, cpu_.a);
			cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			cpu_.c = alu_.inc(cpu_.c);
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			cpu_.a = rd8(hl++);
			wr8(rVBK, cpu_.a);
			cpu_.c = alu_.inc(cpu_.c);
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			cpu_.c = alu_.inc(cpu_.c);
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			cpu_.c = alu_.inc(cpu_.c);
			cpu_.a = rd8(hl);
			wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			SET_HL(gb_, hl);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wIsDMATransferPending, cpu_.a);
		}
		// .not_pending
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x33));
		RET();
	}
};

} // namespace

} // namespace wl3::action_help

using wl3::action_help::ActionHelpPals;

PORT_EX(LoadActionHelpOBPals, 0, 0) { ActionHelpPals(gb).port_LoadActionHelpOBPals(); }
PORT_EX(Func_1e4a47, 0, 0) { ActionHelpPals(gb).port_Func_1e4a47(); }
PORT_EX(LoadActionHelpBGPals, 0, 0) { ActionHelpPals(gb).port_LoadActionHelpBGPals(); }
PORT_EX(LoadActionHelpPalsWithWater, 0, 0) { ActionHelpPals(gb).port_LoadActionHelpPalsWithWater(); }
PORT_EX(LoadActionHelpPalsWithoutWater, 0, 0) { ActionHelpPals(gb).port_LoadActionHelpPalsWithoutWater(); }
PORT_EX(Func_1e48bc, 0, 0) { ActionHelpPals(gb).port_Func_1e48bc(); }
PORT_EX(Func_1e4b6d, 0, 0) { ActionHelpPals(gb).port_Func_1e4b6d(); }
PORT_LOCAL_EX(VBlank_1fb6b7, Func, HOOK_MANUAL_RET, 0) { ActionHelpPals(gb).port_VBlank_1fb6b7__Func(); }
