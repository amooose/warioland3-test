// Port of src/engine/level/objects/snake.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SNAKE_FIRE_UPDATE (ADDR_SnakeFireFunc + 0x13) // .Update

PORT(SnakeFireFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(SNAKE_FIRE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SNAKE_FIRE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}
