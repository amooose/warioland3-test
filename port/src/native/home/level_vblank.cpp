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

	// apply scroll X and Y, including shake effect, then push OAM
	void apply_scroll_and_oam()
	{
		SET_HL(gb_, wSCYShake);
		cpu_.a = alu_.add(rd8(wSCY), rd8(wSCYShake));
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call(gb_, hTransferVirtualOAM);
	}

	// this function is called during level V-Blank
	void port_LevelVBlankFunc()
	{
		cpu_.a = rd8(wIsDMATransferPending);
		alu_.and_(cpu_.a, cpu_.a);
		if (!cpu_.a) {
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(rRAMB + 0x100, 0); // SRAM0
			apply_scroll_and_oam();
			// write tiles in wBGMapTileVRAM0Queue to the corresponding
			// addresses in wBGMapAddressQueue
			SET_HL(gb_, wBGMapAddressQueue);
			SET_BC(gb_, wBGMapTileVRAM0Queue);
			JUMP_TO(wVBlankFunc + 0x10);
			return;
		}

		uint16_t hl = wPendingDMASourceBank;
		cpu_.a = rd8(hl++);
		wr8(rROMB0 + 0x100, cpu_.a);
		cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
		cpu_.a = rd8(hl++); // wPendingDMASourcePtr
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(hl++);
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		cpu_.a = rd8(hl++); // wPendingDMADestinationBank
		wr8(rVBK, cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(hl++); // wPendingDMADestinationPtr
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(hl++);
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(hl); // wPendingDMALength
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wIsDMATransferPending, 0);
		apply_scroll_and_oam();
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wBGMapTileQueueSize, 0);
		wr8(wBGMapAddressQueueSize, 0);
		RET();
	}

	// called within level VBlank
	void port_ProcessLevelTileQueue_VRAM1()
	{
		cpu_.a = BANK_v1Tiles0; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, wBGMapAddressQueue);
		SET_BC(gb_, wBGMapTileVRAM1Queue);
		JUMP_TO(wVBlankFuncExtended);
	}

	void port_ClearLevelTileQueue()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wBGMapTileQueueSize, 0);
		wr8(wBGMapAddressQueueSize, 0);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLevelVblank;

PORT_EX(LevelVBlankFunc, HOOK_MANUAL_RET, 0) { HomeLevelVblank(gb).port_LevelVBlankFunc(); }
PORT_EX(ProcessLevelTileQueue_VRAM1, HOOK_MANUAL_RET, 0) { HomeLevelVblank(gb).port_ProcessLevelTileQueue_VRAM1(); }
PORT_EX(ClearLevelTileQueue, 0, 0) { HomeLevelVblank(gb).port_ClearLevelTileQueue(); }
