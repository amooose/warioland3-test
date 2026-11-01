// overworld/bank56 (src/engine/overworld/bank56.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank56 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// function, run from wVBlankFunc; the rest is C++ (src/native/overworld).

	// _InitBestTimeList.func, as copied to wVBlankFunc (see ram_code.c)
	void port__InitBestTimeList__func()
	{
		cpu_.a = 0x02;
		wr8(rWBK, cpu_.a);
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank56;

PORT_LOCAL_EX(_InitBestTimeList, func, HOOK_MANUAL_RET, 0) { OverworldBank56(gb).port__InitBestTimeList__func(); }
