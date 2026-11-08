// home/level_vblank (src/home/level_vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLevelVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// apply scroll X and Y, including shake effect, then push OAM (a and f
	// as hTransferVirtualOAM leaves them: its dec loop keeps the carry of
	// the shake's add)
	void apply_scroll_and_oam()
	{
		uint8_t scy = mem_[wSCY], shake = mem_[wSCYShake];
		alu_.add(scy, shake); // the add's flags
		mem_[rSCY] = static_cast<uint8_t>(scy + shake);
		mem_[rSCX] = mem_[wSCX];
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		gb_call(gb_, hTransferVirtualOAM);
	}

	// called during level V-Blank: the pending DMA, or the tile queue (the
	// RAM code at wVBlankFunc + $10 takes hl and bc)
	void port_LevelVBlankFunc()
	{
		if (!mem_[wIsDMATransferPending]) {
			mem_[rRAMB + 0x100] = 0; // SRAM0
			apply_scroll_and_oam();
			// the tiles in wBGMapTileVRAM0Queue to the addresses in
			// wBGMapAddressQueue
			SET_HL(gb_, wBGMapAddressQueue);
			SET_BC(gb_, wBGMapTileVRAM0Queue);
			JUMP_TO(wVBlankFunc + 0x10);
			return;
		}
		// wPendingDMASourceBank, then the source, destination bank and
		// destination (big-endian) and length
		mem_[rROMB0 + 0x100] = mem_[wPendingDMASourceBank];
		mem_[rVDMA_SRC_HIGH] = mem_[wPendingDMASourceBank + 1];
		mem_[rVDMA_SRC_LOW] = mem_[wPendingDMASourceBank + 2];
		mem_[rVBK] = mem_[wPendingDMASourceBank + 3];
		mem_[rVDMA_DEST_HIGH] = mem_[wPendingDMASourceBank + 4];
		mem_[rVDMA_DEST_LOW] = mem_[wPendingDMASourceBank + 5];
		mem_[rVDMA_LEN] = mem_[wPendingDMASourceBank + 6];
		mem_[wIsDMATransferPending] = 0;
		apply_scroll_and_oam();
		mem_[wBGMapTileQueueSize] = 0;
		mem_[wBGMapAddressQueueSize] = 0;
		// the asm's c, hl (the shake's, ld hl, wSCYShake / add [hl]) and
		// xor a
		cpu_.c = static_cast<uint8_t>(rVDMA_LEN);
		SET_HL(gb_, wSCYShake);
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		RET();
	}

	// called within level V-Blank: the RAM code at wVBlankFuncExtended
	// takes hl and bc
	void port_ProcessLevelTileQueue_VRAM1()
	{
		cpu_.a = BANK_v1Tiles0; // BANK("VRAM1")
		mem_[rVBK] = cpu_.a;
		SET_HL(gb_, wBGMapAddressQueue);
		SET_BC(gb_, wBGMapTileVRAM1Queue);
		JUMP_TO(wVBlankFuncExtended);
	}

	void port_ClearLevelTileQueue()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		mem_[wBGMapTileQueueSize] = 0;
		mem_[wBGMapAddressQueueSize] = 0;
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLevelVblank;

PORT_EX(LevelVBlankFunc, HOOK_MANUAL_RET, 0) { HomeLevelVblank(gb).port_LevelVBlankFunc(); }
PORT_EX(ProcessLevelTileQueue_VRAM1, HOOK_MANUAL_RET, 0) { HomeLevelVblank(gb).port_ProcessLevelTileQueue_VRAM1(); }
PORT_EX(ClearLevelTileQueue, 0, 0) { HomeLevelVblank(gb).port_ClearLevelTileQueue(); }
