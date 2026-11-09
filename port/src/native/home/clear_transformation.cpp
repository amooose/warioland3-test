// home/clear_transformation (src/home/clear_transformation.asm). The
// file's two number routines are unreferenced in the original and only
// return registers, so they have no port.
#include "../native.hpp"
#include "../routines.hpp"

PORT_EX(PlayRecoverySFX, 0, ALL_REGISTERS)
{
	wl3::play_sfx(wl3::Memory(gb), SFX_RECOVERY);
}

// Unreferenced in the original, so unreachable: nothing to run. Marked for
// the progress report.
PORT_COVERED(Func_10fc);
PORT_COVERED(Func_111e);
