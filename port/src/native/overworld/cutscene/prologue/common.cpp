// overworld/cutscene/prologue/common (src/engine/overworld/cutscene/prologue/common.asm):
// InitPrologue, the prologue's entry in the cutscene init table (C++,
// which reads no register after it).
#include "../../../native.hpp"
#include "../../../routines.hpp"

PORT_EX(InitPrologue, 0, ALL_REGISTERS) { wl3::Routines(gb).farcall(BANK__InitPrologue, ADDR__InitPrologue); }
