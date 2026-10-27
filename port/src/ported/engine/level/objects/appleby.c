// Port of src/engine/level/objects/appleby.asm
#include "ported.h"

#define APPLE_UPDATE1_LEFT (ADDR_AppleFunc + 0x12) // .Update1_Left
#define APPLE_UPDATE1_RIGHT (ADDR_AppleFunc + 0x34) // .Update1_Right

PORT_JUMP(AppleFunc_Left)
{
	gb->cpu.a = (uint8_t)(APPLE_UPDATE1_LEFT >> 8);
	gb->cpu.b = (uint8_t)APPLE_UPDATE1_LEFT;
	JUMP_TO(ADDR_AppleFunc);
}

PORT_JUMP(AppleFunc_Right)
{
	gb->cpu.a = (uint8_t)(APPLE_UPDATE1_RIGHT >> 8);
	gb->cpu.b = (uint8_t)APPLE_UPDATE1_RIGHT;
	JUMP_TO(ADDR_AppleFunc); // fallthrough
}

// ab = update function
PORT(AppleFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.b);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	play_sfx(gb, SFX_067);
}
