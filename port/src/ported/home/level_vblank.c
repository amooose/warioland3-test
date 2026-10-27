// Port of src/home/level_vblank.asm
#include "ported.h"

// apply scroll X and Y, including shake effect, then push OAM
static void apply_scroll_and_oam(GB *gb)
{
	SET_HL(gb, wSCYShake);
	gb->cpu.a = alu_add(gb, rd8(gb, wSCY), rd8(gb, wSCYShake));
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call(gb, hTransferVirtualOAM);
}

// this function is called during level V-Blank
PORT_JUMP(LevelVBlankFunc)
{
	gb->cpu.a = rd8(gb, wIsDMATransferPending);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, rRAMB + 0x100, 0); // SRAM0
		apply_scroll_and_oam(gb);
		// write tiles in wBGMapTileVRAM0Queue to the corresponding
		// addresses in wBGMapAddressQueue
		SET_HL(gb, wBGMapAddressQueue);
		SET_BC(gb, wBGMapTileVRAM0Queue);
		JUMP_TO(wVBlankFunc + 0x10);
		return;
	}

	uint16_t hl = wPendingDMASourceBank;
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, rROMB0 + 0x100, gb->cpu.a);
	gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
	gb->cpu.a = rd8(gb, hl++); // wPendingDMASourcePtr
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++); // wPendingDMADestinationBank
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, hl++); // wPendingDMADestinationPtr
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, hl); // wPendingDMALength
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wIsDMATransferPending, 0);
	apply_scroll_and_oam(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBGMapTileQueueSize, 0);
	wr8(gb, wBGMapAddressQueueSize, 0);
	RET();
}

// called within level VBlank
PORT_JUMP(ProcessLevelTileQueue_VRAM1)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, wBGMapAddressQueue);
	SET_BC(gb, wBGMapTileVRAM1Queue);
	JUMP_TO(wVBlankFuncExtended);
}

PORT(ClearLevelTileQueue)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBGMapTileQueueSize, 0);
	wr8(gb, wBGMapAddressQueueSize, 0);
}
