// overworld/bank2b_1 (src/engine/overworld/bank2b_1.asm): the V-Blank
// functions copied to wVBlankFunc, plain over memory. The rest is C++
// (src/native/overworld: SceneCommon, SceneWario, SceneSprites, the scenes),
// with its own copies of the small helpers below.
#include "../home/ow.hpp"
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::overworld {

namespace {

// The V-Blank functions, as copied to wVBlankFunc (see ram_code.c),
// plain over memory. They end in hTransferVirtualOAM, which takes a
// (wVirtualOAM's page).
void transfer_oam(GB *gb)
{
	gb->cpu.a = static_cast<uint8_t>(wVirtualOAM >> 8);
	gb_tail_jump(gb, hTransferVirtualOAM);
}

// the HDMA in wHDMA, from wHDMABank (cleared): to the VRAM bank given
// (or the one mapped)
void hdma(GB *gb, int vram_bank)
{
	Memory mem(gb);
	uint8_t bank = mem.hdma_bank();
	if (!bank)
		return;
	mem.hdma_bank() = 0;
	uint8_t rom = mem.rom_bank();
	Routines(gb).switch_bank(bank);
	if (vram_bank >= 0)
		mem[rVBK] = mem[static_cast<uint16_t>(vram_bank)];
	for (int i = 0; i < 5; i++)
		mem[static_cast<uint16_t>(rVDMA_SRC_HIGH + i)] = mem[static_cast<uint16_t>(wHDMA + i)];
	Routines(gb).switch_bank(rom);
}

// VBlank_PrologueEpilogue.Func: the scroll, the HDMA, a pending
// character (tile and attribute), the palette configs, the LCDC flags
// to flip
void prologue_epilogue_vblank(GB *gb)
{
	Memory mem(gb);
	mem[rWBK] = 0x02; // BANK("WRAM2")
	mem[rSCY] = mem.scy();
	mem[rSCX] = mem.scx();
	hdma(gb, wHDMADestVRAMBank);
	if (uint8_t lo = mem.pending_char_dest()) {
		mem.pending_char_dest() = 0;
		uint16_t dest = static_cast<uint16_t>(mem[wPendingCharDest + 1] << 8 | lo);
		mem[dest] = mem[wPendingCharDest + 2]; // wPendingCharTile
		mem[rVBK] = 0x01; // BANK("VRAM1")
		mem[dest] = mem[wPendingCharDest + 3]; // wPendingCharAttr
	}
	mem[rVBK] = 0;
	if (mem.pal_config1())
		home::apply_pal_config(gb, wPalConfig1);
	if (mem.pal_config2())
		home::apply_pal_config(gb, wPalConfig2);
	if (uint8_t flip = mem.lcdc_flags_to_flip()) {
		mem[rLCDC] = static_cast<uint8_t>(mem[rLCDC] ^ flip);
		mem.lcdc_flags_to_flip() = 0;
	}
	transfer_oam(gb);
}

// _GolfBuilding.func: the scroll and the HDMA
void golf_building_vblank(GB *gb)
{
	Memory mem(gb);
	mem[rWBK] = 2; // BANK("WRAM2")
	mem[rSCY] = mem.scy();
	mem[rSCX] = mem.scx();
	hdma(gb, -1);
	mem[rVBK] = 0;
	transfer_oam(gb);
}

} // namespace

} // namespace wl3::overworld

using namespace wl3::overworld;

PORT_COVERED(Func_ad330);
PORT_COVERED(Func_ad335);
PORT_COVERED(Func_ad011);
PORT_COVERED(Func_ad016);
PORT_COVERED(Func_aca63);
PORT_COVERED(Func_ad329);
PORT_COVERED(Func_ad33c);
PORT_LOCAL_EX(VBlank_PrologueEpilogue, Func, 0, ALL_REGISTERS & ~R_A) { prologue_epilogue_vblank(gb); }
PORT_LOCAL_EX(_GolfBuilding, func, 0, ALL_REGISTERS & ~R_A) { golf_building_vblank(gb); }

// Unreferenced in the original, so unreachable: nothing to run. Marked for
// the progress report.
PORT_COVERED(DebugSceneWario);
PORT_COVERED(Func_aca68);
