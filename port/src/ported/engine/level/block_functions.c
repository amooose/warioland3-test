// Port of src/engine/level/block_functions.asm
#include "ported.h"

// processes the block function of wBlockPtrBank:wBlockPtr
PORT(ProcessBlock)
{
	wr8(gb, wIsInWaterOrSand, FALSE);
	gb->cpu.a = rd8(gb, wBlockPtrBank);
	sramswitch(gb, gb->cpu.a);

	// the block at wBlockPtr (stored big-endian) indexes the table of block
	// functions, which the level loads into wBlockFunctionTable
	uint16_t block_ptr = (uint16_t)(rd8(gb, wBlockPtr + 0) << 8 | rd8(gb, wBlockPtr + 1));
	uint8_t offset = alu_add(gb, rd8(gb, block_ptr), rd8(gb, block_ptr));
	gb->cpu.d = 0;
	gb->cpu.e = offset;
	uint16_t entry = alu_add16(gb, wBlockFunctionTable, offset);
	uint16_t func = rd16(gb, entry);
	gb->cpu.a = (uint8_t)func;
	SET_HL(gb, func);

	// jp hl: the block function returns to our caller
	gb_call(gb, func);
}

// output:
// - a = $00
PORT(BlockFunc_Free)
{
	sramswitch(gb, 0); // SRAM0
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);
	wr8(gb, wUnused_c18d, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// output:
// - a = $00 or $01
PORT(BlockFunc_Solid)
{
	sramswitch(gb, 0); // SRAM0
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);

	// Wario standing on a slope keeps his y position; everything else is
	// snapped to the top of the block.
	if (alu_and(gb, rd8(gb, wBlockFuncWarioFlag), 0xff) &&
		alu_and(gb, rd8(gb, wWarioSlopeInteraction), 0xff) &&
		alu_bit(gb, rd8(gb, wCollisionPoints), COLLISION_UNK4_F)) {
		wr8(gb, wUnused_c18d, 0);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}

	// .snap_y_pos
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = 1;
	wr8(gb, wUnused_c18d, 1);
}
