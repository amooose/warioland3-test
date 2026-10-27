// Port of src/engine/level/objects/webber.asm
#include "ported.h"

#define WEBBER_PROJECTILE_DESTROY (ADDR_WebberProjectileFunc + 0x54) // .Destroy

// Homes into Wario while falling, and sets itself to be destroyed once it
// lands or its state changes.
PORT(WebberProjectileFunc)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		CALL(MoveObjectRight_Slow);
	else
		CALL(MoveObjectLeft_Slow);
	CALL(MoveObjectDown);

	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));

	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint16_t hl = wCurObjYPos;
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
		wr8(gb, hXPosLo, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_352b, ADDR_WebberProjectileFunc + 0x36);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
			return;
		wr8(gb, wCurObjYPos + 0, rd8(gb, hYPosLo));
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, wCurObjYPos + 1, gb->cpu.a);
		SET_HL(gb, wCurObjYPos + 1);
	}

	// .set_destroy
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(WEBBER_PROJECTILE_DESTROY >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)WEBBER_PROJECTILE_DESTROY;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_682db);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 24;
	hl = REG_HL(gb);
	wr8(gb, hl++, 24); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
}
