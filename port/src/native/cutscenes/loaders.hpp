// The cutscenes' loaders (src/engine/overworld/bank27.asm): their
// graphics, BG maps and palettes, as plain functions the cutscenes call
// (in bank $27, where they are).
#pragma once

#include "../native.hpp"

namespace wl3::cutscenes {

void load_cutscenes1_gfx_vram0(GB *gb); // LoadCutscenes1Gfx_Vram0
void load_cutscenes1_gfx_vram1(GB *gb); // LoadCutscenes1Gfx_Vram1
void load_cutscenes2_gfx(GB *gb); // LoadCutscenes2Gfx
void load_cutscenes3_gfx(GB *gb); // LoadCutscenes3Gfx
void load_cutscenes4_gfx(GB *gb); // LoadCutscenes4Gfx
void load_cutscenes5_gfx_vram0(GB *gb); // LoadCutscenes5Gfx_Vram0
void load_cutscenes5_gfx_vram1(GB *gb); // LoadCutscenes5Gfx_Vram1
void load_cutscenes7_gfx_vram1(GB *gb); // LoadCutscenes7Gfx_Vram1
void load_cutscenes7_5_gfx(GB *gb); // Func_9c931
void load_cutscenes7_10_gfx(GB *gb); // Func_9c937
void load_cutscenes7_gfx_vram0(GB *gb); // LoadCutscenes7Gfx_Vram0
void load_cutscenes8_gfx_vram0(GB *gb); // LoadCutscenes8Gfx_Vram0
void load_cutscenes8_5_gfx_vram1(GB *gb); // Func_9c977
void load_cutscenes8_10_gfx_vram1(GB *gb); // Func_9c97d
void load_cutscenes8_gfx_vram1(GB *gb); // LoadCutscenes8Gfx_Vram1
void load_cutscenes9_gfx_vram1(GB *gb); // LoadCutscenes9Gfx_Vram1
void load_cutscenes9_gfx_vram0(GB *gb); // LoadCutscenes9Gfx_Vram0
void load_cutscenes10_gfx_vram1(GB *gb); // LoadCutscenes10Gfx_Vram1
void load_cutscenes10_gfx_vram0(GB *gb); // LoadCutscenes10Gfx_Vram0

void load_level_main_tiles29_vram1(GB *gb); // LoadLevelMainTiles29_Vram1
void load_level_main_tiles29_vram0(GB *gb); // LoadLevelMainTiles29_Vram0
void clear_temp_bg_ob_pals(GB *gb); // Func_9cc87
void clear_temp_pals(GB *gb); // ClearTempPals_Bank27
void copy_temp_pals_to_bg_ob_pals(GB *gb); // Func_9cd74
void set_bg_priority_on_top_rows_bg_map1(GB *gb); // SetBGPriorityOnTopRows_BGMap1
void set_bg_priority_on_top_rows_vram1(GB *gb); // SetBGPriorityOnTopRows_Vram1
void set_bg_priority_on_top_rows_vram0(GB *gb); // SetBGPriorityOnTopRows_Vram0
void set_bg_priority_on_block(GB *gb); // Func_9cc4f

// LoadCutsceneBGMapsNN: cutscene NN's attrmap and tilemap, to the BG map
// it uses
void load_cutscene_bg_maps(GB *gb, int cutscene);

} // namespace wl3::cutscenes
