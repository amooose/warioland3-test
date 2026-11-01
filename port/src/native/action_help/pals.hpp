// Action Help's palettes (src/engine/action_help/pals.asm): loaded to the
// temp buffers and applied to palette RAM, a palette per H-Blank as the
// original does. The OB palettes black out the power-up's colours while
// Wario hasn't got it.
#pragma once

#include "../native.hpp"

namespace wl3::action_help {

void load_ob_pals(GB *gb); // LoadActionHelpOBPals
void load_ob_pals_4_to_7(GB *gb); // Func_1e4a47
void load_bg_pals(GB *gb, uint16_t pals); // LoadActionHelpBGPals
void load_pals_with_water(GB *gb); // LoadActionHelpPalsWithWater
void load_pals_without_water(GB *gb); // LoadActionHelpPalsWithoutWater
void load_blacked_out_bg_pals(GB *gb); // Func_1e48bc

} // namespace wl3::action_help
