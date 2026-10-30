// Port of src/engine/title/vblank.asm
#include "ported.h"

PORT(VBlank_Title)
{
	SET_HL(gb, ADDR_VBlank_Title + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x4c; // .end - .Func
	CALL(CopyHLToDE);
}

// VBlank_Title.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_Title, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_TitleScreenBoxTilemap;
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wcee4);
	if (alu_bit(gb, gb->cpu.a, 7)) {
		SET_HL(gb, ADDR_TitleScreenBoxTilemap);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a (VRAM0)
		wr8(gb, rVBK, gb->cpu.a);
		SET_DE(gb, v0BGMap0 + 14 * TILEMAP_WIDTH + 4); // debgcoord 4, 14
		gb_push16(gb, REG_DE(gb));
		gb->cpu.b = 12;
		gb_call_ret(gb, ADDR_CopyHLToDE, (uint16_t)(base + 0x22));
		SET_DE(gb, gb_pop16(gb));
		gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
		gb->cpu.d = gb->cpu.a;
		gb->cpu.b = 12;
		gb_call_ret(gb, ADDR_CopyHLToDE, (uint16_t)(base + 0x30));
		SET_HL(gb, wcee4);
		wr8(gb, wcee4, (uint8_t)(rd8(gb, wcee4) & 0x7f));
	}
	// .skip
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x4b));
	RET();
}
