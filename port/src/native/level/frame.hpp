// The level's per-frame routines, for direct calls (each was a hook the
// game reached by address; the hooks stay, for the fuzzer and difftests).
#pragma once

#include "../native.hpp"

namespace wl3::level {

void update_wario_states(GB *gb); // UpdateWarioStates
void update_objects(GB *gb); // UpdateObjects
void draw_objects_no_priority(GB *gb); // DrawObjects_NoPriority
void draw_objects_with_priority(GB *gb); // DrawObjects_WithPriority
void set_wario_screen_pos(GB *gb); // SetWarioScreenPos
void level_scroll_vertical(GB *gb); // LevelScroll_Vertical
void level_scroll_horizontal(GB *gb); // LevelScroll_Horizontal
void update_particles(GB *gb); // UpdateParticles
void process_interactions(GB *gb); // ProcessInteractions
void tick_level_time(GB *gb); // TickLevelTime
void set_tile_queue_processing_functions(GB *gb); // SetTileQueueProcessingFunctions
void queue_scrolled_edge(GB *gb); // Func_8ed9
// Func_bb85: queues the block that changed, if any (b = SRAM1 when the
// queue had nothing, else 0, as the asm leaves it)
void queue_changed_block(GB *gb);

} // namespace wl3::level
