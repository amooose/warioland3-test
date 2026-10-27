// Port of src/engine/level/objects/bubble.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define BUBBLE_HOLE_UPDATE (ADDR_BubbleHoleFunc + 0x13) // .Update
#define BUBBLE_UPDATE (ADDR_BubbleFunc + 0x1f) // .Update
#define BUBBLE_FUNC_47C88 (ADDR_BubbleFunc + 0x52) // .Func_47c88

// Spawns a bubble, unless there are already enough of them (1 while Wario is
// Bubble Wario, else 2).
PORT_JUMP(BubbleHoleFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(BUBBLE_HOLE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)BUBBLE_HOLE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);

	// .asm_47c08
	CALL(CountActiveTransientObjects);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BUBBLE);
	gb->cpu.a = gb->cpu.e;
	alu_cp(gb, gb->cpu.a, (gb->cpu.f & FLAG_Z) ? 1 : 2);
	if (!(gb->cpu.f & FLAG_C)) {
		// .wait
		gb->cpu.a = 1;
		wr8(gb, wCurObjStateDuration, 1);
		RET();
		return;
	}
	// .create_bubble
	gb->cpu.a = 208;
	wr8(gb, wCurObjStateDuration, 208);
	play_sfx(gb, SFX_069);
	SET_BC(gb, ADDR_ObjParams_Bubble);
	JUMP(CreateObjectFromCurObjPos);
}

PORT_JUMP(BubbleFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP_TO(BUBBLE_FUNC_47C88);
		return;
	}
	SET_DE(gb, ADDR_Frameset_690f4);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(BUBBLE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)BUBBLE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	RET();
}
