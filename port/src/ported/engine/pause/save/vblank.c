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

// VBlank_SaveScreen.Func, as copied to wVBlankFunc
PORT_LOCAL_JUMP(VBlank_SaveScreen, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wcee4);
	if (!alu_bit(gb, gb->cpu.a, 7))
		goto transfer;
	gb->cpu.a = rd8(gb, wBGMapTileVRAM0Queue);
	gb->cpu.l = rd8(gb, wBGMapTileVRAM0Queue + 1);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	wr8(gb, REG_HL(gb), 0x05); // pal
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, gb->cpu.a);
	wr8(gb, REG_HL(gb), 0x02);
	SET_HL(gb, wcee4);
	wr8(gb, wcee4, (uint8_t)(rd8(gb, wcee4) & 0x7f));
transfer:
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x22));
	RET();
}
