// The cutscenes' loaders; see loaders.hpp.
#include "loaders.hpp"

#include "../home/memory.hpp"
#include "../home/ow.hpp"

namespace wl3::cutscenes {

namespace {

// decompress_tiles0: from gfx's bank (wTempBank, then hCallFuncBank) with
// hcall Decompress
void decompress_far(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	Memory mem(gb);
	mem.temp_bank() = bank;
	mem.call_func_bank() = bank;
	home::hcall(gb, ADDR_Decompress, [&] { home::decompress(gb, RomPtr(gb, bank, gfx), dest); });
}

// decompress_tiles1: into VRAM bank 1, then back to bank 0
void decompress_far_vram1(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	Memory mem(gb);
	mem[rVBK] = BANK_v1Tiles0;
	decompress_far(gb, bank, gfx, dest);
	mem[rVBK] = 0;
}

void cutscenes1_gfx(GB *gb, bool vram1)
{
	if (vram1)
		decompress_far_vram1(gb, BANK_Cutscenes1Gfx, ADDR_Cutscenes1Gfx, v1Tiles0);
	else
		decompress_far(gb, BANK_Cutscenes1Gfx, ADDR_Cutscenes1Gfx, v0Tiles0);
}

void cutscenes2_gfx(GB *gb, bool vram1)
{
	(void)vram1;
	decompress_far(gb, BANK_Cutscenes2Gfx, ADDR_Cutscenes2Gfx, v0Tiles0);
}

void cutscenes3_gfx(GB *gb, bool vram1)
{
	(void)vram1;
	decompress_far(gb, BANK_Cutscenes3Gfx, ADDR_Cutscenes3Gfx, v0Tiles0);
}

void cutscenes4_gfx(GB *gb, bool vram1)
{
	(void)vram1;
	decompress_far(gb, BANK_Cutscenes4Gfx, ADDR_Cutscenes4Gfx, v0Tiles0);
}

void cutscenes5_gfx(GB *gb, bool vram1)
{
	if (vram1)
		decompress_far_vram1(gb, BANK_Cutscenes5Gfx, ADDR_Cutscenes5Gfx, v1Tiles2);
	else
		decompress_far(gb, BANK_Cutscenes5Gfx, ADDR_Cutscenes5Gfx, v0Tiles2);
}

void cutscenes7_gfx(GB *gb, bool vram1)
{
	if (vram1)
		decompress_far_vram1(gb, BANK_Cutscenes7Gfx, ADDR_Cutscenes7Gfx, v1Tiles0);
	else
		decompress_far(gb, BANK_Cutscenes7Gfx, ADDR_Cutscenes7Gfx, v0Tiles0);
}

void cutscenes8_gfx(GB *gb, bool vram1)
{
	if (vram1)
		decompress_far_vram1(gb, BANK_Cutscenes8Gfx, ADDR_Cutscenes8Gfx, v1Tiles0);
	else
		decompress_far(gb, BANK_Cutscenes8Gfx, ADDR_Cutscenes8Gfx, v0Tiles0);
}

void cutscenes9_gfx(GB *gb, bool vram1)
{
	if (vram1)
		decompress_far_vram1(gb, BANK_Cutscenes9Gfx, ADDR_Cutscenes9Gfx, v1Tiles0);
	else
		decompress_far(gb, BANK_Cutscenes9Gfx, ADDR_Cutscenes9Gfx, v0Tiles0);
}

void cutscenes10_gfx(GB *gb, bool vram1)
{
	if (vram1)
		decompress_far_vram1(gb, BANK_Cutscenes10Gfx, ADDR_Cutscenes10Gfx, v1Tiles2);
	else
		decompress_far(gb, BANK_Cutscenes10Gfx, ADDR_Cutscenes10Gfx, v0Tiles2);
}

// LoadCutsceneBGMap: a compressed map (in "Cutscene BG Maps") to dest,
// then VRAM bank 0
void load_bg_map(GB *gb, uint16_t map, uint16_t dest)
{
	Memory mem(gb);
	decompress_far(gb, BANK_Cutscene01Tilemap, map, dest);
	mem[rVBK] = 0;
}

void set_top_rows_attributes(GB *gb, uint16_t bg_map, uint8_t attributes)
{
	Memory mem(gb);
	mem[rVBK] = BANK_v1Tiles0;
	home::fill(gb, bg_map, attributes, 5 * TILEMAP_WIDTH);
	mem[rVBK] = 0;
}

} // namespace

void load_cutscenes1_gfx_vram0(GB *gb)
{
	cutscenes1_gfx(gb, false);
	cutscenes5_gfx(gb, false);
}

void load_cutscenes1_gfx_vram1(GB *gb)
{
	cutscenes1_gfx(gb, true);
	cutscenes5_gfx(gb, true);
}

void load_cutscenes2_gfx(GB *gb)
{
	cutscenes2_gfx(gb, false);
	cutscenes5_gfx(gb, false);
}

void load_cutscenes3_gfx(GB *gb)
{
	cutscenes3_gfx(gb, false);
	cutscenes5_gfx(gb, false);
}

void load_cutscenes4_gfx(GB *gb)
{
	cutscenes4_gfx(gb, false);
	cutscenes5_gfx(gb, false);
}

void load_cutscenes5_gfx_vram0(GB *gb)
{
	cutscenes5_gfx(gb, false);
}

void load_cutscenes5_gfx_vram1(GB *gb)
{
	cutscenes5_gfx(gb, true);
}

void load_cutscenes7_gfx_vram1(GB *gb)
{
	cutscenes7_gfx(gb, true);
	cutscenes5_gfx(gb, true);
}

void load_cutscenes7_5_gfx(GB *gb)
{
	cutscenes7_gfx(gb, false);
	cutscenes5_gfx(gb, false);
}

void load_cutscenes7_10_gfx(GB *gb)
{
	cutscenes7_gfx(gb, false);
	cutscenes10_gfx(gb, false);
}

void load_cutscenes7_gfx_vram0(GB *gb)
{
	cutscenes7_gfx(gb, false);
}

void load_cutscenes8_gfx_vram0(GB *gb)
{
	cutscenes8_gfx(gb, false);
	cutscenes5_gfx(gb, false);
}

void load_cutscenes8_5_gfx_vram1(GB *gb)
{
	cutscenes8_gfx(gb, true);
	cutscenes5_gfx(gb, true);
}

void load_cutscenes8_10_gfx_vram1(GB *gb)
{
	cutscenes8_gfx(gb, true);
	cutscenes10_gfx(gb, true);
}

void load_cutscenes8_gfx_vram1(GB *gb)
{
	cutscenes8_gfx(gb, true);
}

void load_cutscenes9_gfx_vram1(GB *gb)
{
	cutscenes9_gfx(gb, true);
	cutscenes10_gfx(gb, true);
}

void load_cutscenes9_gfx_vram0(GB *gb)
{
	cutscenes9_gfx(gb, false);
	cutscenes10_gfx(gb, false);
}

void load_cutscenes10_gfx_vram1(GB *gb)
{
	cutscenes10_gfx(gb, true);
}

void load_cutscenes10_gfx_vram0(GB *gb)
{
	cutscenes10_gfx(gb, false);
}

void load_level_main_tiles29_vram1(GB *gb)
{
	Memory mem(gb);
	mem[rVBK] = 1;
	home::load_far_tiles(gb, BANK_LevelMainTiles29, ADDR_LevelMainTiles29);
	mem[rVBK] = 0;
}

void load_level_main_tiles29_vram0(GB *gb)
{
	home::load_far_tiles(gb, BANK_LevelMainTiles29, ADDR_LevelMainTiles29);
}

void clear_temp_bg_ob_pals(GB *gb)
{
	home::fill(gb, wTempBGPals, 0, 8 * PAL_SIZE);
	home::fill(gb, wTempOBPals, 0, 8 * PAL_SIZE);
}

void clear_temp_pals(GB *gb)
{
	home::fill(gb, wTempPals1, 0, 8 * PAL_SIZE);
	home::fill(gb, wTempPals2, 0, 8 * PAL_SIZE);
}

void copy_temp_pals_to_bg_ob_pals(GB *gb)
{
	home::copy(gb, wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	home::copy(gb, wTempPals1, wTempBGPals, 8 * PAL_SIZE);
}

void set_bg_priority_on_top_rows_bg_map1(GB *gb)
{
	set_top_rows_attributes(gb, v1BGMap1, BG_BANK0 | BG_PRIO);
}

void set_bg_priority_on_top_rows_vram1(GB *gb)
{
	set_top_rows_attributes(gb, v1BGMap0, BG_BANK1 | BG_PRIO);
}

void set_bg_priority_on_top_rows_vram0(GB *gb)
{
	set_top_rows_attributes(gb, v1BGMap0, BG_BANK0 | BG_PRIO);
}

// the 6x7 block at (0, 5) of v1BGMap0
void set_bg_priority_on_block(GB *gb)
{
	Memory mem(gb);
	mem[rVBK] = BANK_v1Tiles0;
	for (int row = 0; row < 7; row++)
		home::fill(gb, static_cast<uint16_t>(v1BGMap0 + (5 + row) * TILEMAP_WIDTH), BG_BANK0 | BG_PRIO, 6);
	mem[rVBK] = 0;
}

// its .Func (0x51 bytes) to wVBlankFunc
void set_vblank_cutscene(GB *gb)
{
	home::copy(gb, ROM(gb, VBlank_Cutscene) + 0x0c, wVBlankFunc, 0x51);
}

void load_cutscene_bg_maps(GB *gb, int cutscene)
{
	static constexpr struct {
		int cutscene;
		uint16_t attrmap, tilemap;
		bool bg_map1;
	} MAPS[] = {
		{1, ADDR_Cutscene01Attrmap, ADDR_Cutscene01Tilemap, false},
		{2, ADDR_Cutscene02Attrmap, ADDR_Cutscene02Tilemap, true},
		{7, ADDR_Cutscene07Attrmap, ADDR_Cutscene07Tilemap, false},
		{3, ADDR_Cutscene03Attrmap, ADDR_Cutscene03Tilemap, false},
		{12, ADDR_Cutscene12Attrmap, ADDR_Cutscene12Tilemap, false},
		{13, ADDR_Cutscene13Attrmap, ADDR_Cutscene13Tilemap, false},
		{15, ADDR_Cutscene15Attrmap, ADDR_Cutscene15Tilemap, false},
		{16, ADDR_Cutscene16Attrmap, ADDR_Cutscene16Tilemap, false},
		{17, ADDR_Cutscene17Attrmap, ADDR_Cutscene17Tilemap, false},
		{18, ADDR_Cutscene18Attrmap, ADDR_Cutscene18Tilemap, false},
		{20, ADDR_Cutscene20Attrmap, ADDR_Cutscene20Tilemap, false},
		{22, ADDR_Cutscene22Attrmap, ADDR_Cutscene22Tilemap, false},
		{23, ADDR_Cutscene23Attrmap, ADDR_Cutscene23Tilemap, false},
		{24, ADDR_Cutscene24Attrmap, ADDR_Cutscene24Tilemap, false},
		{26, ADDR_Cutscene26Attrmap, ADDR_Cutscene26Tilemap, false},
		{27, ADDR_Cutscene27Attrmap, ADDR_Cutscene27Tilemap, false},
		{28, ADDR_Cutscene28Attrmap, ADDR_Cutscene28Tilemap, false},
		{29, ADDR_Cutscene29Attrmap, ADDR_Cutscene29Tilemap, false},
		{30, ADDR_Cutscene30Attrmap, ADDR_Cutscene30Tilemap, false},
		{31, ADDR_Cutscene31Attrmap, ADDR_Cutscene31Tilemap, false},
		{32, ADDR_Cutscene32Attrmap, ADDR_Cutscene32Tilemap, false},
		{34, ADDR_Cutscene34Attrmap, ADDR_Cutscene34Tilemap, false},
		{36, ADDR_Cutscene36Attrmap, ADDR_Cutscene36Tilemap, false},
		{37, ADDR_Cutscene37Attrmap, ADDR_Cutscene37Tilemap, false},
		{39, ADDR_Cutscene39Attrmap, ADDR_Cutscene39Tilemap, false},
		{4, ADDR_Cutscene04Attrmap, ADDR_Cutscene04Tilemap, true},
		{5, ADDR_Cutscene05Attrmap, ADDR_Cutscene05Tilemap, true},
		{14, ADDR_Cutscene14Attrmap, ADDR_Cutscene14Tilemap, true},
		{19, ADDR_Cutscene19Attrmap, ADDR_Cutscene19Tilemap, true},
		{21, ADDR_Cutscene21Attrmap, ADDR_Cutscene21Tilemap, true},
		{25, ADDR_Cutscene25Attrmap, ADDR_Cutscene25Tilemap, true},
		{35, ADDR_Cutscene35Attrmap, ADDR_Cutscene35Tilemap, true},
		{38, ADDR_Cutscene38Attrmap, ADDR_Cutscene38Tilemap, true},
		{40, ADDR_Cutscene40Attrmap, ADDR_Cutscene40Tilemap, true},
	};
	for (const auto &maps : MAPS) {
		if (maps.cutscene != cutscene)
			continue;
		uint16_t dest = maps.bg_map1 ? v0BGMap1 : v0BGMap0;
		Memory(gb).byte(rVBK) = 1; // the attrmap to VRAM bank 1
		load_bg_map(gb, maps.attrmap, dest);
		load_bg_map(gb, maps.tilemap, dest);
		return;
	}
}

} // namespace wl3::cutscenes

// Only the cutscenes (C++) call these.
PORT_COVERED(LoadCutscenes1Gfx_Vram0);
PORT_COVERED(LoadCutscenes1Gfx_Vram1);
PORT_COVERED(LoadCutscenes2Gfx);
PORT_COVERED(LoadCutscenes3Gfx);
PORT_COVERED(LoadCutscenes4Gfx);
PORT_COVERED(LoadCutscenes5Gfx_Vram0);
PORT_COVERED(LoadCutscenes5Gfx_Vram1);
PORT_COVERED(LoadCutscenes7Gfx_Vram1);
PORT_COVERED(Func_9c931);
PORT_COVERED(Func_9c937);
PORT_COVERED(LoadCutscenes7Gfx_Vram0);
PORT_COVERED(LoadCutscenes8Gfx_Vram0);
PORT_COVERED(Func_9c977);
PORT_COVERED(Func_9c97d);
PORT_COVERED(LoadCutscenes8Gfx_Vram1);
PORT_COVERED(LoadCutscenes9Gfx_Vram1);
PORT_COVERED(LoadCutscenes9Gfx_Vram0);
PORT_COVERED(LoadCutscenes10Gfx_Vram1);
PORT_COVERED(LoadCutscenes10Gfx_Vram0);
PORT_COVERED(LoadLevelMainTiles29_Vram1);
PORT_COVERED(LoadLevelMainTiles29_Vram0);
PORT_COVERED(Func_9cc87);
PORT_COVERED(ClearTempPals_Bank27);
PORT_COVERED(Func_9cd74);
PORT_COVERED(SetBGPriorityOnTopRows_BGMap1);
PORT_COVERED(SetBGPriorityOnTopRows_Vram1);
PORT_COVERED(SetBGPriorityOnTopRows_Vram0);
PORT_COVERED(Func_9cc4f);
PORT_COVERED(LoadCutsceneBGMaps01);
PORT_COVERED(LoadCutsceneBGMaps02);
PORT_COVERED(LoadCutsceneBGMaps07);
PORT_COVERED(LoadCutsceneBGMaps03);
PORT_COVERED(LoadCutsceneBGMaps12);
PORT_COVERED(LoadCutsceneBGMaps13);
PORT_COVERED(LoadCutsceneBGMaps15);
PORT_COVERED(LoadCutsceneBGMaps16);
PORT_COVERED(LoadCutsceneBGMaps17);
PORT_COVERED(LoadCutsceneBGMaps18);
PORT_COVERED(LoadCutsceneBGMaps20);
PORT_COVERED(LoadCutsceneBGMaps22);
PORT_COVERED(LoadCutsceneBGMaps23);
PORT_COVERED(LoadCutsceneBGMaps24);
PORT_COVERED(LoadCutsceneBGMaps26);
PORT_COVERED(LoadCutsceneBGMaps27);
PORT_COVERED(LoadCutsceneBGMaps28);
PORT_COVERED(LoadCutsceneBGMaps29);
PORT_COVERED(LoadCutsceneBGMaps30);
PORT_COVERED(LoadCutsceneBGMaps31);
PORT_COVERED(LoadCutsceneBGMaps32);
PORT_COVERED(LoadCutsceneBGMaps34);
PORT_COVERED(LoadCutsceneBGMaps36);
PORT_COVERED(LoadCutsceneBGMaps37);
PORT_COVERED(LoadCutsceneBGMaps39);
PORT_COVERED(LoadCutsceneBGMaps04);
PORT_COVERED(LoadCutsceneBGMaps05);
PORT_COVERED(LoadCutsceneBGMaps14);
PORT_COVERED(LoadCutsceneBGMaps19);
PORT_COVERED(LoadCutsceneBGMaps21);
PORT_COVERED(LoadCutsceneBGMaps25);
PORT_COVERED(LoadCutsceneBGMaps35);
PORT_COVERED(LoadCutsceneBGMaps38);
PORT_COVERED(LoadCutsceneBGMaps040);
PORT_COVERED(SetBGPriorityOnTopRows);
PORT_COVERED(LoadCutsceneBGMap);
PORT_COVERED(LoadCutsceneTilemap0);
PORT_COVERED(LoadCutsceneTilemap1);
PORT_COVERED(LoadCutsceneAttrmap0);
PORT_COVERED(LoadCutsceneAttrmap1);
