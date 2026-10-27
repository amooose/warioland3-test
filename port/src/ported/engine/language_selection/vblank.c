// Port of src/engine/language_selection/vblank.asm
#include "ported.h"

PORT(VBlank_LanguageSelection)
{
	SET_HL(gb, ADDR_VBlank_LanguageSelection + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x53; // .end - .Func
	CALL(CopyHLToDE);
}
