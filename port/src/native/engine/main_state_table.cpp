// The main state table (src/engine/main_state_table.asm): the handler for
// each game state, by wState. The handlers are called by address (they take
// no register arguments). InvalidState restarts the game.
#include "main_state_table.hpp"
#include "../asm_module.hpp"

namespace wl3::engine {

void run_main_state(GB *gb)
{
	Memory mem(gb);
	// rst JumpTable over the table after it (add a, a: the index wraps at $80)
	uint16_t target = (ROM(gb, MainStateTable) + 4).word(static_cast<uint8_t>(mem.state() * 2));
	Routines(gb).call(target);
}

namespace {

class EngineMainStateTable : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_InvalidState() { JUMP(Init); }
};

} // namespace

} // namespace wl3::engine

using wl3::engine::EngineMainStateTable;

// The game loop reads no register after it.
PORT_EX(MainStateTable, 0, ALL_REGISTERS) { wl3::engine::run_main_state(gb); }
PORT_EX(InvalidState, HOOK_MANUAL_RET, 0) { EngineMainStateTable(gb).port_InvalidState(); }
