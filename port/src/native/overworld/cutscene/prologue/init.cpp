// overworld/cutscene/prologue/init (src/engine/overworld/cutscene/prologue/init.asm)
// as plain C++. InitPrologue farcalls it from the cutscene's init table
// (C++), which reads no register after.
#include "../../overworld.hpp"
#include "../../../home/memory.hpp"
#include "../../../home/ow.hpp"
#include "../../../native.hpp"
#include "../../../routines.hpp"

namespace wl3::overworld {

namespace {

// decompress_tilemap / decompress_tiles0: src (in bank) to dest
void decompress(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	Memory mem(gb);
	mem.temp_bank() = bank;
	home::farcall(gb, bank, ADDR_Decompress, [&] { home::decompress(gb, RomPtr(gb, bank, src), dest); });
}

// decompress_attrmap / decompress_tiles1: the same, into VRAM bank 1
void decompress_vram1(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	Memory mem(gb);
	mem[rVBK] = 1; // BANK("VRAM1")
	decompress(gb, bank, src, dest);
	mem[rVBK] = 0;
}

void init_prologue(GB *gb)
{
	Memory mem(gb);
	Routines routines(gb);
	routines.call(ADDR_ClearGeneralSceneRAM);
	routines.call(ADDR_ClearSceneObjsRAM);
	routines.call(ADDR_VBlank_PrologueEpilogue);

	// the prologue's BG maps and gfx
	decompress_vram1(gb, BANK_PrologueAttrmap, ADDR_PrologueAttrmap, v1BGMap1);
	decompress(gb, BANK_PrologueTilemap, ADDR_PrologueTilemap, v0BGMap1);
	home::load_far_tiles(gb, BANK_PrologueBackgroundGfx, ADDR_PrologueBackgroundGfx);
	decompress_vram1(gb, BANK_FontGfx, ADDR_FontGfx, v1Tiles0);
	decompress_vram1(gb, BANK_PrologueGfx, ADDR_PrologueGfx, v1Tiles0);

	// the Music Box scene's
	decompress_vram1(gb, BANK_MusicBoxAttrmap, ADDR_MusicBoxAttrmap, v1BGMap0);
	decompress(gb, BANK_MusicBoxTilemap, ADDR_MusicBoxTilemap, v0BGMap0);
	home::copy_far_bytes_long(gb, BANK_MusicBoxGfx, ADDR_MusicBoxGfx, v0Tiles1, (0x30 * TILE_SIZE) >> 8);

	// the prologue's text to the buffer
	if (mem.language())
		decompress(gb, BANK_TextEN_HiddenFigureAreYouAware, ADDR_TextEN_HiddenFigureAreYouAware, wTextBuffer);
	else
		decompress(gb, BANK_TextJP_HiddenFigureAreYouAware, ADDR_TextJP_HiddenFigureAreYouAware, wTextBuffer);
	routines.call(ADDR_FillClearedTextBuffer);

	home::fill(gb, wTempPals1, 0, 8 * PAL_SIZE);
	home::fill(gb, wTempPals2, 0, 8 * PAL_SIZE);

	// the scene's Wario
	mem[wSceneWarioYCoord + 0] = 96; // y
	mem[wSceneWarioYCoord + 1] = 96; // x
	mem.scene_wario_state_group() = SCENEWARIO_GROUP_NONE;
	overworld::SceneWario(gb).set_state(SCENEWARIO_NONE);

	mem.lcdc_flags_to_flip() = LCDC_BG_MAP;
}

} // namespace

} // namespace wl3::overworld

PORT_EX(_InitPrologue, 0, ALL_REGISTERS) { wl3::overworld::init_prologue(gb); }
