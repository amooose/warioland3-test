// Port of src/engine/load_font.asm
#include "ported.h"

PORT(LoadFontTiles)
{
	SET_HL(gb, ADDR_FontGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}
