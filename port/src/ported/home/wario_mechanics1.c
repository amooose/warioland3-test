// Port of src/home/wario_mechanics1.asm
#include "ported.h"

// Moves Wario by the next entry of his current jump velocity table:
// negative entries move him up, others down.
PORT(ApplyJumpVelocity)
{
	static const uint16_t tables[] = {
		ADDR_JumpVelTable_KnockBack,      // JUMP_VEL_KNOCK_BACK
		ADDR_JumpVelTable_Normal,         // JUMP_VEL_NORMAL
		ADDR_JumpVelTable_HighJump,       // JUMP_VEL_HIGH_JUMP
		ADDR_JumpVelTable_BouncyJump,     // JUMP_VEL_BOUNCY_JUMP
		ADDR_JumpVelTable_BouncyHighJump, // JUMP_VEL_BOUNCY_HIGH_JUMP
	};

	// a chain of dec a / jr z
	uint8_t a = rd8(gb, wJumpVelTable);
	int table = -1;
	for (int i = 0; i < 5; i++) {
		a = alu_dec(gb, a);
		if (!a) {
			table = i;
			break;
		}
	}
	gb->cpu.a = a;
	if (table < 0)
		return;

	gb->cpu.e = rd8(gb, wJumpVelIndex);
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, tables[table], gb->cpu.e);
	uint8_t vel = rd8(gb, hl);
	if (alu_bit(gb, vel, 7)) {
		// rising: cpl / inc a
		gb->cpu.f |= FLAG_N | FLAG_H;
		gb->cpu.b = alu_inc(gb, (uint8_t)~vel);
		gb->cpu.a = gb->cpu.b;
		CALL(MoveWarioUp);
		SET_HL(gb, wJumpVelIndex);
		wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
		return;
	}

	// falling
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wDoFullJump, FALSE);
	gb->cpu.b = vel;
	CALL(MoveWarioDown);
	SET_HL(gb, wJumpVelIndex);
	gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelIndex));
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX);
	if (!(gb->cpu.f & FLAG_C))
		wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
}

// output: b = walk velocity; advances wWalkVelIndex
PORT(ApplyWalkVelocity)
{
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_WalkVelTable, REG_DE(gb));
	gb->cpu.b = rd8(gb, hl);
	SET_HL(gb, wWalkVelIndex);
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	alu_cp(gb, gb->cpu.a, 0x1b);
	if (gb->cpu.f & FLAG_Z) {
		// .cap
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfc);
		wr8(gb, wWalkVelIndex, gb->cpu.a);
		return;
	}
	wr8(gb, wWalkVelIndex, alu_inc(gb, gb->cpu.a));
}
