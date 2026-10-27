// Port of src/engine/level/objects/clear_gate.asm
#include "ported.h"

// de = frameset
PORT_JUMP(ClearGateFunc)
{
	CALL(SetObjectFramesetPtr);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	uint16_t update = ADDR_ClearGateFunc + 0x2e; // .Update
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = (uint16_t)((hl & 0xff00) | OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x05);
	wr8(gb, hl--, 0xea); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 0xfe;
	wr8(gb, hl, 0xfe); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wIsMinigameCleared);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = 32;
	wr8(gb, wCurObjVar2, 32);
	JUMP(MoveObjectUpByVar2);
}

PORT_JUMP(ClearGate3Func)
{
	SET_DE(gb, ADDR_Frameset_698ae);
	JUMP_TO(ADDR_ClearGateFunc);
}

PORT_JUMP(ClearGate2Func)
{
	SET_DE(gb, ADDR_Frameset_698ab);
	JUMP_TO(ADDR_ClearGateFunc);
}

PORT_JUMP(ClearGate1Func)
{
	SET_DE(gb, ADDR_Frameset_698a8);
	JUMP_TO(ADDR_ClearGateFunc); // fallthrough
}
