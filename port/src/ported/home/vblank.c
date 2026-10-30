// Port of src/home/vblank.asm
#include "ported.h"

// wait_not_hblank / wait_hblank: poll rSTAT until the mode is (or isn't)
// HBlank, letting emulated time pass.
static void wait_not_hblank(GB *gb)
{
	while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
		idle(gb, 32);
}

static void wait_hblank(GB *gb)
{
	while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
		idle(gb, 32);
}

// Waits for the start of the next HBlank.
PORT(WaitNextHBlank)
{
	wait_not_hblank(gb);
	wait_hblank(gb);
}

// ld hl, .Func / ld de, wVBlankFunc / ld b, .end - .Func / call CopyHLToDE
static void set_vblank_func(GB *gb, uint16_t func, uint8_t size)
{
	SET_HL(gb, func);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = size;
	CALL(CopyHLToDE);
}

PORT(VBlank_Ret)
{
	set_vblank_func(gb, ADDR_VBlank_Ret + 0x0c, 0x01);
}

PORT(InitLCD)
{
	gb->cpu.a = 0xd9;
	wr8(gb, wLCDFunc, 0xd9);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rIF, 0);
	wr8(gb, rIE, rd8(gb, rIE) & ~(1 << B_IE_STAT));
	SET_HL(gb, rSTAT);
	wr8(gb, rSTAT, rd8(gb, rSTAT) & ~(1 << B_STAT_LYC));
}

PORT(VBlank_TimeAttackDescription)
{
	set_vblank_func(gb, ADDR_VBlank_TimeAttackDescription + 0x0c, 0x10);
}

// VBlank_TimeAttackDescription.Func, as copied to wVBlankFunc (see
// ram_code.c). Its call returns to base + n, the block's start wherever it
// runs from.
PORT_LOCAL(VBlank_TimeAttackDescription, Func)
{
	uint16_t base = gb->cpu.pc;
	wr8(gb, rSCY, rd8(gb, wSCY));
	wr8(gb, rSCX, rd8(gb, wSCX));
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x0f));
}
