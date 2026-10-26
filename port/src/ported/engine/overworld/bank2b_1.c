// Port of src/engine/overworld/bank2b_1.asm
#include "ported.h"

// moves scene Wario 1.5 pixels to right
// if d-right is pressed, moves 2 pixels
PORT(ApplySceneWarioMovementRight)
{
	gb->cpu.b = alu_swap(gb, rd8(gb, wJoypadDown));
	// bit 0 is set if d-right is down or wGlobalCounter is odd
	uint8_t a = alu_or(gb, rd8(gb, wGlobalCounter), gb->cpu.b);
	SET_HL(gb, wSceneWarioXCoord);
	wr8(gb, wSceneWarioXCoord, alu_inc(gb, rd8(gb, wSceneWarioXCoord)));
	if (alu_bit(gb, a, 0))
		wr8(gb, wSceneWarioXCoord, alu_inc(gb, rd8(gb, wSceneWarioXCoord)));
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneWarioXCoord), 0xfe);
}

PORT(Func_aca6d)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	SET_HL(gb, w2d014 - 1);
	wr8(gb, w2d014 - 1, alu_inc(gb, rd8(gb, w2d014 - 1))); // w2d013
}

// bc = OW obj: updates its animation with the credits OAM
PORT(Func_ac000)
{
	gb->cpu.h = gb->cpu.b;
	gb->cpu.l = (uint8_t)(gb->cpu.c & ~(1 << 1));
	gb->cpu.b = BANK_OAM_15e44a; // BANK("Credits OAM")
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, REG_BC(gb));
}

PORT_JUMP(Func_ac042) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSpriteWithScroll); }
PORT_JUMP(Func_ac047) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSpriteWithScroll); }
PORT_JUMP(Func_ac04c) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSpriteWithScroll); }
PORT_JUMP(Func_ac051) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSprite); }

// return nz if animation is not finished yet
// e = new state
PORT(SetObjStateIfAnimationIsFinished)
{
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	uint16_t hl = (uint16_t)(REG_HL(gb) | 0x04); // set 2, l
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0); // duration
	wr8(gb, hl++, 0); // frameset offset
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl, gb->cpu.a); // state
	SET_HL(gb, hl);
	SET_BC(gb, hl);
}

// Moves scene Wario 1 pixel, and another if bit 0 of a (the d-pad
// direction, or'ed with wGlobalCounter) is set.
static void apply_scene_wario_movement(GB *gb, uint8_t dpad, int dir)
{
	gb->cpu.b = dpad;
	gb->cpu.a = alu_or(gb, rd8(gb, wGlobalCounter), gb->cpu.b);
	SET_HL(gb, wSceneWarioXCoord);
	uint8_t x = rd8(gb, wSceneWarioXCoord);
	x = dir < 0 ? alu_dec(gb, x) : alu_inc(gb, x);
	wr8(gb, wSceneWarioXCoord, x);
	if (alu_bit(gb, gb->cpu.a, 0)) {
		x = dir < 0 ? alu_dec(gb, x) : alu_inc(gb, x);
		wr8(gb, wSceneWarioXCoord, x);
	}
	gb->cpu.a = alu_and(gb, x, 0xfe);
}

// moves scene Wario 1.5 pixels to left
// if d-left is pressed, moves 2 pixels
PORT(ApplySceneWarioMovementLeft)
{
	uint8_t a = alu_swap(gb, rd8(gb, wJoypadDown));
	// rra (carry is clear after swap)
	gb->cpu.f = (a & 1) ? FLAG_C : 0;
	a >>= 1;
	apply_scene_wario_movement(gb, a, -1);
}
