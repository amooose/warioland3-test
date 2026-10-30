// Port of src/engine/pause/save/vblank.asm
#include "ported.h"

// ld hl, .Func / ld de, wVBlankFunc / ld b, .end - .Func / call CopyHLToDE
static void set_vblank_func(GB *gb, uint16_t func, uint8_t size)
{
	SET_HL(gb, func);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = size;
	CALL(CopyHLToDE);
}

// used for when game is actually writing data to SRAM
// only handles OAM and nothing else
PORT(VBlank_Saving)
{
	set_vblank_func(gb, ADDR_VBlank_Saving + 0x0c, 0x06);
}

// used for when game is showing the save progress bar
// updates the bar progress by overwriting tile and attribute
PORT(VBlank_SaveScreen)
{
	set_vblank_func(gb, ADDR_VBlank_SaveScreen + 0x0c, 0x23);
}

// VBlank_Saving.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to gb->cpu.pc + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_Saving, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x05));
	RET();
}
