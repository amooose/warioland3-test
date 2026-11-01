// home/load_wario_gfx (src/home/load_wario_gfx.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadWarioGfx : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Queues a DMA transfer of Wario's current graphics into v0Tiles0.
	void port_LoadWarioGfx()
	{
		wr8(wPendingDMASourceBank, rd8(wDMASourceBank));
		wr8(wPendingDMASourcePtr + 0, rd8(wDMASourcePtr + 0));
		wr8(wPendingDMASourcePtr + 1, rd8(wDMASourcePtr + 1));

		wr8(wPendingDMADestinationPtr + 0, (uint8_t)(v0Tiles0 >> 8));
		wr8(wPendingDMADestinationPtr + 1, (uint8_t)v0Tiles0);
		wr8(wPendingDMADestinationBank, BANK_v0Tiles0);
		wr8(wPendingDMALength, 127);
		wr8(wIsDMATransferPending, TRUE);
		cpu_.a = TRUE;
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadWarioGfx;

PORT_EX(LoadWarioGfx, 0, 0) { HomeLoadWarioGfx(gb).port_LoadWarioGfx(); }
