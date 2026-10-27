// Port of src/engine/overworld/vblank.asm
#include "ported.h"

// Both copy their .Func, the VBlank handler that runs from WRAM, to
// wVBlankFunc. The handlers themselves stay asm.

PORT(VBlank_Overworld)
{
	SET_HL(gb, ADDR_VBlank_Overworld + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = (uint8_t)(ADDR_VBlank_OWScene - (ADDR_VBlank_Overworld + 0x0c)); // .end - .Func
	CALL(CopyHLToDE);
}

PORT(VBlank_OWScene)
{
	SET_HL(gb, ADDR_VBlank_OWScene + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = (uint8_t)(ADDR_UnreferencedOpenTreasureCollection - (ADDR_VBlank_OWScene + 0x0c)); // .end - .Func
	CALL(CopyHLToDE);
}
