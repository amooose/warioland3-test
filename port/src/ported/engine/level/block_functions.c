// Port of src/engine/level/block_functions.asm
#include "ported.h"

// Jumps between block functions use JUMP_TO: like the asm, they run whatever
// bank is mapped.

// processes the block function of wBlockPtrBank:wBlockPtr
PORT_JUMP(ProcessBlock)
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
	JUMP_TO(func);
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

PORT_JUMP(BlockFunc_Water)
{
	gb->cpu.a = TRUE;
	wr8(gb, wIsInWaterOrSand, TRUE);
	gb->cpu.a = rd8(gb, wBlockFuncWarioFlag);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_BlockFunc_Free);
		return;
	}
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (!alu_bit(gb, gb->cpu.a, COLLISION_UNK7_F)) {
		JUMP_TO(ADDR_BlockFunc_Free);
		return;
	}
	gb->cpu.a = SUBMERSIBLE_WATER;
	wr8(gb, wWaterInteraction, SUBMERSIBLE_WATER);
	JUMP_TO(ADDR_BlockFunc_Free);
}

// Sets wWaterCurrent if Wario's in the current, then it's water.
static void block_func_current(GB *gb, uint8_t current)
{
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (alu_bit(gb, gb->cpu.a, COLLISION_UNK7_F)) {
		gb->cpu.a = current;
		wr8(gb, wWaterCurrent, current);
	}
	JUMP_TO(ADDR_BlockFunc_Water);
}

PORT_JUMP(BlockFunc_RightCurrent) { block_func_current(gb, CURRENT_RIGHT); }
PORT_JUMP(BlockFunc_UpCurrent) { block_func_current(gb, CURRENT_UP); }
PORT_JUMP(BlockFunc_LeftCurrent) { block_func_current(gb, CURRENT_LEFT); }
PORT_JUMP(BlockFunc_DownCurrent) { block_func_current(gb, CURRENT_DOWN); }

PORT_JUMP(BlockFunc_Floor)
{
	gb->cpu.a = TRUE;
	wr8(gb, wc1ca, TRUE);
	gb->cpu.a = rd8(gb, wCollisionPoints);
	bool solid = alu_bit(gb, gb->cpu.a, COLLISION_DOWN_F) || alu_bit(gb, gb->cpu.a, COLLISION_UNK2_F);
	if (!solid) {
		gb->cpu.a = alu_and(gb, gb->cpu.a,
			COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7);
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP_TO(ADDR_BlockFunc_Free);
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0x0f);
		alu_cp(gb, gb->cpu.a, 0x05);
		if (!(gb->cpu.f & FLAG_C)) {
			JUMP_TO(ADDR_BlockFunc_Free);
			return;
		}
	}
	// .asm_18228
	gb->cpu.a = rd8(gb, wBlockFuncWarioFlag);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_BlockFunc_Solid);
		return;
	}
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, WST_ZOMBIE_LANDING);
	if (!(gb->cpu.f & FLAG_Z))
		alu_cp(gb, gb->cpu.a, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_BlockFunc_Free);
		return;
	}
	JUMP_TO(ADDR_BlockFunc_Solid);
}

// output:
// - a = $00
PORT(Func_1808c)
{
	sramswitch(gb, 0); // SRAM0
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);
	wr8(gb, wUnused_c18d, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT_JUMP(Func_182ec)
{
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (!alu_bit(gb, gb->cpu.a, COLLISION_UNK7_F)) {
		JUMP_TO(ADDR_BlockFunc_Floor);
		return;
	}
	gb->cpu.a = LADDER_UNDERNEATH;
	wr8(gb, wLadderInteraction, gb->cpu.a);
	JUMP_TO(ADDR_Func_1808c);
}

// ld a, HIGH(ptr) / ld [addr + 0], a / ld a, LOW(ptr) / ld [addr + 1], a
static void st_ptr_be(GB *gb, uint16_t addr, uint16_t ptr)
{
	wr8(gb, addr + 0, (uint8_t)(ptr >> 8));
	gb->cpu.a = (uint8_t)ptr;
	wr8(gb, addr + 1, gb->cpu.a);
}

PORT_JUMP(EnterDoor)
{
	// centre on the door
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0), 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
	play_sfx(gb, SFX_0E6);

	gb->cpu.a = WST_ENTERING_DOOR;
	wr8(gb, wWarioState, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wGrabState, wAttackCounter,
		wJumpVelIndex, wJumpVelTable, wIsCrouching, wFrameDuration, wAnimationFrame};
	for (unsigned i = 0; i < sizeof(zero) / sizeof(zero[0]); i++)
		wr8(gb, zero[i], 0);

	wr8(gb, wDMASourceBank, BANK_WarioIdleGfx);
	st_ptr_be(gb, wDMASourcePtr, ADDR_WarioIdleGfx);
	CALL(LoadWarioGfx);
	wr8(gb, wWarioOAMBank, BANK_OAM_14a82);
	st_ptr_be(gb, wWarioOAMPtr, ADDR_OAM_14a82);
	gb->cpu.a = rd8(gb, wDirection);
	bool right = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	st_ptr_be(gb, wWarioFramesetPtr, right ? ADDR_Frameset_14d10 : ADDR_Frameset_14d0b);
	FARCALL_BANK(BANK_OAM_14000, UpdateWarioAnimation); // BANK("Wario OAM 1")
	JUMP_TO(ADDR_BlockFunc_Free);
}
