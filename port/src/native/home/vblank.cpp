// home/vblank (src/home/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// wait_not_hblank / wait_hblank: poll rSTAT until the mode is (or isn't)
	// HBlank, letting emulated time pass.
	void wait_not_hblank()
	{
		while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
	}

	void wait_hblank()
	{
		while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
	}

	// Waits for the start of the next HBlank.
	void port_WaitNextHBlank()
	{
		wait_not_hblank();
		wait_hblank();
	}

	// ld hl, .Func / ld de, wVBlankFunc / ld b, .end - .Func / call CopyHLToDE
	void set_vblank_func(uint16_t func, uint8_t size)
	{
		SET_HL(gb_, func);
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = size;
		CALL(CopyHLToDE);
	}

	void port_VBlank_Ret()
	{
		set_vblank_func(ADDR_VBlank_Ret + 0x0c, 0x01);
	}

	void port_InitLCD()
	{
		cpu_.a = 0xd9;
		wr8(wLCDFunc, 0xd9);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rIF, 0);
		wr8(rIE, rd8(rIE) & ~(1 << B_IE_STAT));
		SET_HL(gb_, rSTAT);
		wr8(rSTAT, rd8(rSTAT) & ~(1 << B_STAT_LYC));
	}

	void port_VBlank_TimeAttackDescription()
	{
		set_vblank_func(ADDR_VBlank_TimeAttackDescription + 0x0c, 0x10);
	}

	// VBlank_TimeAttackDescription.Func, as copied to wVBlankFunc (see
	// ram_code.c). Its call returns to base + n, the block's start wherever it
	// runs from.
	void port_VBlank_TimeAttackDescription__Func()
	{
		uint16_t base = cpu_.pc;
		wr8(rSCY, rd8(wSCY));
		wr8(rSCX, rd8(wSCX));
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x0f));
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeVblank;

PORT_EX(WaitNextHBlank, 0, 0) { HomeVblank(gb).port_WaitNextHBlank(); }
PORT_EX(VBlank_Ret, 0, 0) { HomeVblank(gb).port_VBlank_Ret(); }
PORT_EX(InitLCD, 0, 0) { HomeVblank(gb).port_InitLCD(); }
PORT_EX(VBlank_TimeAttackDescription, 0, 0) { HomeVblank(gb).port_VBlank_TimeAttackDescription(); }
PORT_LOCAL_EX(VBlank_TimeAttackDescription, Func, 0, 0) { HomeVblank(gb).port_VBlank_TimeAttackDescription__Func(); }
