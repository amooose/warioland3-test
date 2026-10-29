// Port of src/engine/golf/lobby.asm
#include "ported.h"

PORT_JUMP(VBlank_1c84e0)
{
	SET_HL(gb, ADDR_VBlank_1c84e0 + 0x0b); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x0e; // .end - .Func
	JUMP(CopyHLToDE);
}
