// Port of src/engine/clear/vblank.asm
#include "ported.h"

PORT(VBlank_ClearScreen)
{
	SET_HL(gb, ADDR_VBlank_ClearScreen + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x37; // .end - .Func
	CALL(CopyHLToDE);
}

PORT(VBlank_MusicalCoinsScreen)
{
	SET_HL(gb, ADDR_VBlank_MusicalCoinsScreen + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x3e; // .end - .Func
	CALL(CopyHLToDE);
}
