// Port of src/engine/level/objects/torch.asm
#include "ported.h"

#define TORCH_EMBER_DESTROY (ADDR_TorchEmberCommonFunc + 0x41) // .Destroy

// Moves the ember down, and sets it to be destroyed once it lands.
PORT(TorchEmberCommonFunc)
{
	SET_BC(gb, ADDR_Data_60320);
	CALL(ApplyObjYMovement);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, rd8(gb, wCurObjFlags) | (1 << OBJFLAG_NO_COLLISION_F));
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint16_t hl = wCurObjYPos;
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
		wr8(gb, hXPosLo, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_352b, ADDR_TorchEmberCommonFunc + 0x23);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
			return;
		wr8(gb, wCurObjYPos + 0, rd8(gb, hYPosLo));
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, wCurObjYPos + 1, gb->cpu.a);
		SET_HL(gb, wCurObjYPos + 1);
	}

	// .set_destroy
	gb->cpu.a = (uint8_t)(TORCH_EMBER_DESTROY >> 8);
	wr8(gb, wCurObjUpdateFunction + 1, gb->cpu.a);
	gb->cpu.a = (uint8_t)TORCH_EMBER_DESTROY;
	wr8(gb, wCurObjUpdateFunction + 0, gb->cpu.a);
	SET_HL(gb, wCurObjUpdateFunction - 1);
	SET_DE(gb, ADDR_Frameset_68359);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 24;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 24); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
}
