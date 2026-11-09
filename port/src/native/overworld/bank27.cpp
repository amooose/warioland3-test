// overworld/bank27 (src/engine/overworld/bank27.asm): what stays
// register-level; the cutscenes are C++ (src/native/cutscenes), their
// loaders in cutscenes/loaders.cpp. Converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "../home/ow.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank27 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// the Prologue, still register-level (bank2b_1/2)
	void port_PrologueFunc()
	{
		FARCALL(Prologue);
	}

	// called by register-level code too (cutscene/init.cpp)
	void port_VBlank_Cutscene()
	{
		home::copy(gb_, ADDR_VBlank_Cutscene + 0x0c, wVBlankFunc, 0x51); // .Func, .end - .Func
	}

	// VBlank_Cutscene.Func, as copied to wVBlankFunc (see ram_code.c): the
	// scroll and window, the LCDC flags to flip, the palette configs, then
	// hTransferVirtualOAM, which takes a (wVirtualOAM's page)
	void vblank_func()
	{
		Memory mem(gb_);
		mem[rWBK] = 0x02; // BANK("WRAM2")
		mem[rSCY] = mem[wSCY];
		mem[rSCX] = mem[wSCX];
		mem[rWY] = mem[wCutsceneWY];
		mem[rWX] = mem[wCutsceneWX];
		if (uint8_t flip = mem[wLCDCFlagsToFlip]) {
			mem[rLCDC] = static_cast<uint8_t>(mem[rLCDC] ^ flip);
			mem[wLCDCFlagsToFlip] = 0;
		}
		uint8_t rom = mem[wROMBank];
		bankswitch(BANK_ApplyPalConfig1);
		if (mem[wPalConfig1Register])
			home::apply_pal_config(gb_, wPalConfig1);
		if (mem[wPalConfig2])
			home::apply_pal_config(gb_, wPalConfig2);
		bankswitch(rom);
		mem[rVBK] = 0;
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank27;

PORT_EX(PrologueFunc, 0, ALL_REGISTERS) { OverworldBank27(gb).port_PrologueFunc(); }
PORT_EX(VBlank_Cutscene, 0, ALL_REGISTERS) { OverworldBank27(gb).port_VBlank_Cutscene(); }
PORT_LOCAL_EX(VBlank_Cutscene, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { OverworldBank27(gb).vblank_func(); }
