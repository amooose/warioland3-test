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

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// ld hl, wCurObjUpdateFunction + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static void set_update_func(GB *gb, uint16_t f)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// TorchFunc.got_frameset
static void torch_got_frameset(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0); // OBJ_STATE_DURATION
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 6);
	wr8(gb, hl--, (uint8_t)-7); // OBJ_COLLBOX_LEFT
	gb->cpu.a = (uint8_t)-3;
	wr8(gb, hl, gb->cpu.a); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}

// TorchFunc.got_update_func
static void torch_got_update_func(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_68360);
	CALL(SetObjectFramesetPtr);
	torch_got_frameset(gb);
}

PORT(TorchFunc)
{
	set_update_func(gb, ADDR_TorchFunc + 0x28); // .Update
	torch_got_update_func(gb);
}

PORT(TorchNoEmbersFunc)
{
	set_update_func(gb, ADDR_TorchNoEmbersFunc + 0x0b); // .Update
	torch_got_update_func(gb);
}

PORT(FlameBlockTorchFunc)
{
	set_update_func(gb, ADDR_FlameBlockTorchUpdate);
	SET_DE(gb, ADDR_Frameset_68369);
	CALL(SetObjectFramesetPtr);
	torch_got_frameset(gb);
}

// unreferenced?
PORT_JUMP(Func_40c19)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		CALL(MoveObjectRight);
	else
		CALL(MoveObjectLeft_Slow);
	SET_BC(gb, ADDR_Data_60320);
	CALL(ApplyObjYMovement);
	JUMP_TO(ADDR_TorchEmberCommonFunc);
}

PORT_JUMP(TorchEmberLeft1Func)
{
	CALL(MoveObjectLeft);
	JUMP_TO(ADDR_TorchEmberCommonFunc);
}

PORT_JUMP(TorchEmberRight1Func)
{
	CALL(MoveObjectRight);
	JUMP_TO(ADDR_TorchEmberCommonFunc);
}

PORT_JUMP(TorchEmberLeft2Func)
{
	CALL(MoveObjectLeft_Slow);
	JUMP_TO(ADDR_TorchEmberCommonFunc);
}

PORT_JUMP(TorchEmberRight2Func)
{
	CALL(MoveObjectRight_Slow);
	JUMP_TO(ADDR_TorchEmberCommonFunc); // fallthrough
}
