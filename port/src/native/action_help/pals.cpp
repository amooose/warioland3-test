// Action Help's palettes (see pals.hpp), and the V-Blank function it
// copies to wVBlankFunc.
#include "pals.hpp"

#include "../asm_module.hpp"
#include "../home/memory.hpp"
#include "../home/pals.hpp"
#include "../home/sprites.hpp"

namespace wl3::action_help {

namespace {

// count palettes from src to the palette data register, a palette per
// H-Blank (wait_not_hblank / wait_hblank)
void apply_pals_in_hblank(GB *gb, uint16_t src, uint16_t data_reg, int count)
{
	Memory mem(gb);
	while (count--) {
		while (!(mem[rSTAT] & STAT_MODE))
			home::idle(gb, 32);
		while (mem[rSTAT] & STAT_MODE)
			home::idle(gb, 32);
		for (int i = 0; i < PAL_SIZE; i++)
			mem[data_reg] = mem[src++];
	}
}

// For a main power-up (the special ones show as they are): whether Wario
// hasn't got it yet, so its colours are blacked out.
enum class PowerUp { SPECIAL, MISSING, OWNED };

PowerUp power_up(Memory mem)
{
	uint8_t shown = mem[wActionHelpPowerUp];
	if (shown >= NUM_MAIN_POWER_UPS)
		return PowerUp::SPECIAL;
	return (mem[wPowerUpLevel] & POWER_UP_MASK) < shown ? PowerUp::MISSING : PowerUp::OWNED;
}

} // namespace

void load_ob_pals(GB *gb)
{
	Memory mem(gb);
	uint16_t src = ADDR_Pals_1e4f2f;
	switch (power_up(mem)) {
	case PowerUp::SPECIAL:
		return;
	case PowerUp::MISSING:
		home::fill(gb, wTempPals2, 0, 2 * PAL_SIZE);
		home::fill(gb, wTempPals2 + 3 * PAL_SIZE, 0, 5 * PAL_SIZE);
		src = wTempPals2;
		break;
	case PowerUp::OWNED:
		home::copy_short(gb, src, wTempPals2, 8 * PAL_SIZE);
		break;
	}
	mem[rOBPI] = OBPI_AUTOINC | 0;
	apply_pals_in_hblank(gb, src, rOBPD, 8);
}

void load_ob_pals_4_to_7(GB *gb)
{
	Memory mem(gb);
	uint16_t src = ADDR_Pals_1e4f2f + 4 * PAL_SIZE;
	switch (power_up(mem)) {
	case PowerUp::SPECIAL:
		return;
	case PowerUp::MISSING:
		home::fill(gb, wTempPals2 + 4 * PAL_SIZE, 0, PAL_SIZE);
		home::fill(gb, wTempPals2 + 7 * PAL_SIZE, 0, PAL_SIZE);
		src = wTempPals2 + 4 * PAL_SIZE;
		break;
	case PowerUp::OWNED:
		home::copy_short(gb, src, wTempPals2 + 4 * PAL_SIZE, PAL_SIZE);
		home::copy_short(gb, ADDR_Pals_1e4f2f + 7 * PAL_SIZE, wTempPals2 + 7 * PAL_SIZE, PAL_SIZE);
		break;
	}
	mem[rOBPI] = OBPI_AUTOINC | (4 * PAL_SIZE);
	apply_pals_in_hblank(gb, src, rOBPD, 4);
}

void load_bg_pals(GB *gb, uint16_t pals)
{
	home::copy_short(gb, pals, wTempPals1, 8 * PAL_SIZE);
	Memory(gb).byte(rBGPI) = BGPI_AUTOINC | 0;
	apply_pals_in_hblank(gb, pals, rBGPD, 8);
}

void load_pals_with_water(GB *gb)
{
	Memory(gb).byte(wActionHelpBlackedOut) = 0;
	load_bg_pals(gb, ADDR_Pals_1e4e6f);
	load_ob_pals(gb);
}

void load_pals_without_water(GB *gb)
{
	Memory(gb).byte(wActionHelpBlackedOut) = 0;
	load_bg_pals(gb, ADDR_Pals_1e4eaf);
	load_ob_pals(gb);
}

void load_blacked_out_bg_pals(GB *gb)
{
	Memory(gb).byte(wActionHelpBlackedOut) = TRUE;
	load_bg_pals(gb, ADDR_Pals_1e4eef);
}

namespace {

class ActionHelpPals : public AsmModule {
public:
	using AsmModule::AsmModule;

	// VBlank_1fb6b7.Func, as copied to wVBlankFunc, plain over memory: the
	// V-Blank handler pushes and pops every register around it. The pending
	// DMA (wPendingDMASourceBank, then the source, destination bank and
	// destination, and the length), the scroll, then hTransferVirtualOAM
	// (a = wVirtualOAM's page).
	void port_VBlank_1fb6b7__Func()
	{
		if (mem_[wIsDMATransferPending]) {
			mem_[rROMB0 + 0x100] = mem_[wPendingDMASourceBank];
			mem_[rVDMA_SRC_HIGH] = mem_[wPendingDMASourceBank + 1];
			mem_[rVDMA_SRC_LOW] = mem_[wPendingDMASourceBank + 2];
			mem_[rVBK] = mem_[wPendingDMASourceBank + 3];
			mem_[rVDMA_DEST_HIGH] = mem_[wPendingDMASourceBank + 4];
			mem_[rVDMA_DEST_LOW] = mem_[wPendingDMASourceBank + 5];
			mem_[rVDMA_LEN] = mem_[wPendingDMASourceBank + 6];
			mem_[wIsDMATransferPending] = 0;
		}
		mem_[rSCY] = mem_[wSCY];
		mem_[rSCX] = mem_[wSCX];
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}
};

} // namespace

} // namespace wl3::action_help

using wl3::action_help::ActionHelpPals;

PORT_LOCAL_EX(VBlank_1fb6b7, Func, 0, ALL_REGISTERS) { ActionHelpPals(gb).port_VBlank_1fb6b7__Func(); }

// unreferenced: hl = a sprite's y (on the BG map), x, frame and attributes,
// drawn from OAM_1e6bab
PORT_EX(Func_1e4b6d, 0, 0)
{
	wl3::Memory mem(gb);
	uint16_t obj = REG_HL(gb);
	mem[wCurSpriteYCoord] = static_cast<uint8_t>(mem[obj] - mem[wSCY] + OAM_Y_OFS);
	mem[wCurSpriteXCoord] = static_cast<uint8_t>(mem[static_cast<uint16_t>(obj + 1)] + OAM_X_OFS);
	mem[wCurSpriteFrame] = mem[static_cast<uint16_t>(obj + 2)];
	mem[wCurSpriteAttributes] = mem[static_cast<uint16_t>(obj + 3)];
	wl3::home::set_sprite_regs(gb, wl3::home::load_sprite(gb, ADDR_OAM_1e6bab));
}

// Only C++ modules call these now.
PORT_COVERED(LoadActionHelpOBPals);
PORT_COVERED(Func_1e4a47);
PORT_COVERED(LoadActionHelpBGPals);
PORT_COVERED(LoadActionHelpPalsWithWater);
PORT_COVERED(LoadActionHelpPalsWithoutWater);
PORT_COVERED(Func_1e48bc);
