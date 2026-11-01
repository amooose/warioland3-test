// pause/save/vblank (src/engine/pause/save/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::pause {

namespace {

class PauseSaveVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// VBlank_Saving.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
	// return to cpu_.pc + n, the block's start wherever it runs from.
	void port_VBlank_Saving__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x05));
		RET();
	}

	// VBlank_SaveScreen.Func, as copied to wVBlankFunc
	void port_VBlank_SaveScreen__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wcee4);
		if (!alu_.bit(cpu_.a, 7))
			goto transfer;
		cpu_.a = rd8(wBGMapTileVRAM0Queue);
		cpu_.l = rd8(wBGMapTileVRAM0Queue + 1);
		cpu_.h = cpu_.a;
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		wr8(REG_HL(gb_), 0x05); // pal
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, cpu_.a);
		wr8(REG_HL(gb_), 0x02);
		SET_HL(gb_, wcee4);
		wr8(wcee4, (uint8_t)(rd8(wcee4) & 0x7f));
	transfer:
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x22));
		RET();
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PauseSaveVblank;

PORT_LOCAL_EX(VBlank_Saving, Func, HOOK_MANUAL_RET, 0) { PauseSaveVblank(gb).port_VBlank_Saving__Func(); }
PORT_LOCAL_EX(VBlank_SaveScreen, Func, HOOK_MANUAL_RET, 0) { PauseSaveVblank(gb).port_VBlank_SaveScreen__Func(); }
