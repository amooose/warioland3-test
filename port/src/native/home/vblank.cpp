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

PORT_LOCAL_EX(VBlank_TimeAttackDescription, Func, 0, 0) { HomeVblank(gb).port_VBlank_TimeAttackDescription__Func(); }
