// Port of src/engine/level/vblank.asm
#include "ported.h"

// Writes c copies of
//   ld a, [hli] / ld d, a / ld a, [hli] / ld e, a / ld a, [bc] / ld [de], a / inc c
// at hl, which the VBlank function runs to copy the queued tiles.
static uint16_t write_tile_copies(GB *gb, uint16_t hl)
{
	static const uint8_t code[] = {0x2a, 0x57, 0x2a, 0x5f, 0x0a, 0x12, 0x0c};
	do {
		for (size_t i = 0; i < sizeof(code); i++)
			wr8(gb, hl++, code[i]);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	gb->cpu.a = code[sizeof(code) - 1];
	return hl;
}

// Builds the code in wVBlankFunc that copies the BG map tile queue to VRAM,
// for both VRAM banks, followed by jp ClearLevelTileQueue.
PORT(SetTileQueueProcessingFunctions)
{
	uint16_t hl = wVBlankFunc + 0x10;
	gb->cpu.a = rd8(gb, wBGMapTileQueueSize);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.c = gb->cpu.a;
		hl = write_tile_copies(gb, hl);
		wr8(gb, hl++, 0xc3); // jp
		wr8(gb, hl++, (uint8_t)ADDR_ProcessLevelTileQueue_VRAM1);
		wr8(gb, hl, (uint8_t)(ADDR_ProcessLevelTileQueue_VRAM1 >> 8));

		gb->cpu.c = rd8(gb, wBGMapTileQueueSize);
		hl = write_tile_copies(gb, wVBlankFuncExtended);
	}
	// .no_queue
	wr8(gb, hl++, 0xc3); // jp
	wr8(gb, hl++, (uint8_t)ADDR_ClearLevelTileQueue);
	gb->cpu.a = (uint8_t)(ADDR_ClearLevelTileQueue >> 8);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}
