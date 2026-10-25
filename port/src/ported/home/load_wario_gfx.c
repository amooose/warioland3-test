// Port of src/home/load_wario_gfx.asm
#include "ported.h"

// Queues a DMA transfer of Wario's current graphics into v0Tiles0.
PORT(LoadWarioGfx)
{
	wr8(gb, wPendingDMASourceBank, rd8(gb, wDMASourceBank));
	wr8(gb, wPendingDMASourcePtr + 0, rd8(gb, wDMASourcePtr + 0));
	wr8(gb, wPendingDMASourcePtr + 1, rd8(gb, wDMASourcePtr + 1));

	wr8(gb, wPendingDMADestinationPtr + 0, (uint8_t)(v0Tiles0 >> 8));
	wr8(gb, wPendingDMADestinationPtr + 1, (uint8_t)v0Tiles0);
	wr8(gb, wPendingDMADestinationBank, BANK_v0Tiles0);
	wr8(gb, wPendingDMALength, 127);
	wr8(gb, wIsDMATransferPending, TRUE);
	gb->cpu.a = TRUE;
}
