// Port of src/engine/title/vblank.asm
#include "ported.h"

PORT(VBlank_Title)
{
	SET_HL(gb, ADDR_VBlank_Title + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x4c; // .end - .Func
	CALL(CopyHLToDE);
}
