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
	// ram_code.c)
	void port_VBlank_TimeAttackDescription__Func()
	{
		wr8(rSCY, rd8(wSCY));
		wr8(rSCX, rd8(wSCX));
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeVblank;

PORT_LOCAL_EX(VBlank_TimeAttackDescription, Func, 0, 0) { HomeVblank(gb).port_VBlank_TimeAttackDescription__Func(); }
