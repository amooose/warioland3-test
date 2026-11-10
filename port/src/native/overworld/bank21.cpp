// overworld/bank21 (src/engine/overworld/bank21.asm): the palette configs
// and the overworld palette transition's V-Blank function, plain over
// memory; the pal config hooks leave the registers the asm leaves.
#include "../asm_module.hpp"
#include "../home/memory.hpp"
#include "../home/ow.hpp"
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank21 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// unreferenced: the temp palettes cleared
	void port_Func_84fd5()
	{
		home::fill(gb_, wTempBGPals, 0, 8 * PAL_SIZE);
		home::fill(gb_, wTempOBPals, 0, 8 * PAL_SIZE);
	}

	// VBlank_OWPalTransition.Func, as copied to wVBlankFunc, plain over
	// memory: the V-Blank handler pushes and pops every register around
	// it. The scroll and window, Func_1d8c2c (in its bank), the
	// 8 BG palettes from wTempBGPals, then a jump to hTransferVirtualOAM
	// (a = wVirtualOAM's page).
	void port_VBlank_OWPalTransition__Func()
	{
		mem_[rWBK] = 0x02; // BANK("WRAM2")
		mem_[rSCY] = mem_[wSCY];
		mem_[rSCX] = mem_[wSCX];
		mem_[rWY] = mem_[wWY];
		mem_[rWX] = static_cast<uint8_t>(mem_[wWX] + WX_OFS);

		uint8_t bank = mem_[wROMBank];
		bankswitch(BANK_Func_1d8c2c);
		update_ow_tilemap_vdma(gb_);
		bankswitch(bank);

		mem_[rBGPI] = BGPI_AUTOINC | 0; // palette 0
		for (int i = 0; i < 8 * PAL_SIZE; i++)
			mem_[rBGPD] = mem_[static_cast<uint16_t>(wTempBGPals + i)];
		mem_[rVBK] = 0;
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		gb_tail_jump(gb_, hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank21;

PORT_EX(Func_84fd5, 0, ALL_REGISTERS) { OverworldBank21(gb).port_Func_84fd5(); }
// hl = palette config: applies its palettes and clears its register
// (home::apply_pal_config), leaving the asm's registers: a = the last
// byte, b = 0, c = the data register, hl after the colours, f as the last
// dec b's (carry clear from the xor a)
static void apply_pal_config(GB *gb, uint16_t config)
{
	wl3::home::PalConfigEnd end = wl3::home::apply_pal_config(gb, config);
	SET_HL(gb, end.src);
	gb->cpu.b = 0;
	gb->cpu.c = end.data_reg;
	gb->cpu.a = end.last;
	gb->cpu.f = FLAG_Z | FLAG_N;
}

PORT(ApplyPalConfig_Bank21) { apply_pal_config(gb, REG_HL(gb)); }
PORT(ApplyPalConfig1) { apply_pal_config(gb, wPalConfig1); }
PORT(ApplyPalConfig2) { apply_pal_config(gb, wPalConfig2); }
PORT_LOCAL_EX(VBlank_OWPalTransition, Func, 0, ALL_REGISTERS & ~R_A) { OverworldBank21(gb).port_VBlank_OWPalTransition__Func(); }
