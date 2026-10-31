// Port of src/engine/credits/credits.asm: the V-Blank function (the rest is
// C++, src/native/screens/credits.cpp)
#include "ported.h"

// VBlank_160439.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_160439, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = rd8(gb, wcee4);
	if (!alu_bit(gb, gb->cpu.a, 7))
		goto dma_transfer;
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x7f);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .write_text
		gb->cpu.a = rd8(gb, wCreditsBGMapPtr);
		gb->cpu.e = rd8(gb, wCreditsBGMapPtr + 1);
		gb->cpu.d = gb->cpu.a;
		uint16_t hl = wCreditsTextBuffer;
		for (;;) {
			gb->cpu.a = rd8(gb, hl++);
			alu_cp(gb, gb->cpu.a, 0x7f); // '\n'
			if (gb->cpu.f & FLAG_Z)
				break;
			wr8(gb, REG_DE(gb), gb->cpu.a);
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
		}
		SET_HL(gb, hl);
		goto clear_pending;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_160499
		gb_call_ret(gb, ADDR_ApplyTempPals1ToBGPals, (uint16_t)(base + 0x57));
		gb_call_ret(gb, ADDR_ApplyTempPals2ToOBPals, (uint16_t)(base + 0x5a));
		SET_HL(gb, rLCDC);
		gb->cpu.a = alu_or(gb, rd8(gb, rLCDC), LCDC_WIN_ON | LCDC_WIN_9C00);
		wr8(gb, rLCDC, gb->cpu.a);
		goto clear_pending;
	}
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCreditsBGMapPtr);
	gb->cpu.e = rd8(gb, wCreditsBGMapPtr + 1);
	gb->cpu.d = gb->cpu.a;
	SET_HL(gb, wCreditsTextPals);
	gb->cpu.b = CREDITS_TEXT_LENGTH;
	gb_call_ret(gb, ADDR_CopyHLToDE, (uint16_t)(base + 0x2b));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCreditsBGMapPtr);
	gb->cpu.l = rd8(gb, wCreditsBGMapPtr + 1);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = 0x7e;
	gb->cpu.b = CREDITS_TEXT_LENGTH;
	gb_call_ret(gb, ADDR_WriteAToHL_BTimes, (uint16_t)(base + 0x3b));
	gb->cpu.a = 0x01 | (1 << 7);
	wr8(gb, wcee4, gb->cpu.a);
	goto dma_transfer;
clear_pending:
	// .asm_1604a6
	SET_HL(gb, wcee4);
	wr8(gb, wcee4, (uint8_t)(rd8(gb, wcee4) & 0x7f));
dma_transfer:
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x6b));
	RET();
}
