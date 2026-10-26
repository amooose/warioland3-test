// Port of src/home/wario_mechanics2.asm
#include "ported.h"

PORT(OpenActionHelp)
{
	gb->cpu.a = ST_ACTION_HELP;
	wr8(gb, wState, ST_ACTION_HELP);
	wr8(gb, wState + 1, 0); // wSubState
	SET_HL(gb, wState + 1);
}

// Caps wWalkVelIndex: at $0c from $10 on the ground or while jumping up,
// at $04 from $08 while falling.
PORT(Func_1762)
{
	gb->cpu.a = rd8(gb, wJumpVelIndex);
	alu_cp(gb, gb->cpu.a, FALLING_JUMP_VEL_INDEX);
	bool falling = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	alu_cp(gb, gb->cpu.a, falling ? 0x08 : 0x10);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = falling ? 0x04 : 0x0c;
	wr8(gb, wWalkVelIndex, gb->cpu.a);
}

// Copies wWarioPos to hPos with x moved to the left or right edge of the
// collision box, 3 pixels in, as the asm does it: from wWarioXPos + 1 down.
static void get_pos_at_box_edge(GB *gb, bool left)
{
	if (left) {
		gb->cpu.a = (uint8_t)(~rd8(gb, wCollisionBoxLeft) + 1);
		gb->cpu.c = alu_sub(gb, gb->cpu.a, 3);
		gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos + 1), gb->cpu.c);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos), 0x00);
	} else {
		gb->cpu.c = alu_sub(gb, rd8(gb, wCollisionBoxRight), 3);
		gb->cpu.a = alu_add(gb, rd8(gb, wWarioXPos + 1), gb->cpu.c);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, wWarioXPos), 0x00);
	}
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	wr8(gb, hXPosLo - 2, rd8(gb, wWarioYPos + 1));
	gb->cpu.a = rd8(gb, wWarioYPos);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioYPos);
	SET_DE(gb, hXPosLo - 3);
}

// returns nz if standing on slippery ground
// The farcalls push the asm's own return addresses: Func_19b51 runs
// ProcessBlock, whose block functions may look at the stack.
PORT(IsOnSlipperyGround)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wIsOnSlipperyGround, 0);

	get_pos_at_box_edge(gb, true);
	FARCALL_RET(Func_19b51, (uint16_t)(ADDR_IsOnSlipperyGround + 0x2f));
	gb->cpu.a = rd8(gb, wIsOnSlipperyGround);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;

	get_pos_at_box_edge(gb, false);
	FARCALL_RET(Func_19b51, (uint16_t)(ADDR_IsOnSlipperyGround + 0x5d));
	gb->cpu.a = rd8(gb, wIsOnSlipperyGround);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
}
