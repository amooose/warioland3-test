// The main state table (src/engine/main_state_table.asm), for C++ callers.
#pragma once

#include "../native.hpp"

namespace wl3::engine {

// MainStateTable: runs the handler for wState from the table, with the
// table's bank mapped (the game loop's, from boot)
void run_main_state(GB *gb);

} // namespace wl3::engine
