// home/clear_transformation (src/home/clear_transformation.asm). The hook
// is exact: the recoveries jump to it for register-level callers. The
// file's two number routines are unreferenced in the original and only
// return registers, so they have no port.
#include "../native.hpp"
#include "../routines.hpp"

PORT_EX(PlayRecoverySFX, 0, 0)
{
	wl3::play_sfx(wl3::Memory(gb), SFX_RECOVERY);
	gb->cpu.a = static_cast<uint8_t>(SFX_RECOVERY); // the macro leaves the low byte
}
