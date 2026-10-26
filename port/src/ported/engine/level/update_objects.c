// Port of src/engine/level/update_objects.asm
#include "ported.h"

#define OBJ_STRUCT_SIZE (wObj2Flags - wObj1Flags)

// Draws every active, on-screen, visible object whose flags (masked as
// below) match.
static void draw_objects(GB *gb, uint8_t match)
{
	for (int n = 0; n < NUM_OBJECTS; n++) {
		uint16_t obj = (uint16_t)(wObj1Flags + n * OBJ_STRUCT_SIZE);
		SET_HL(gb, obj);
		gb->cpu.a = alu_and(gb, rd8(gb, obj),
			OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_INVISIBLE | OBJFLAG_PRIORITY);
		alu_cp(gb, gb->cpu.a, match);
		if (!(gb->cpu.f & FLAG_Z))
			continue;
		gb->cpu.e = (uint8_t)(wObj1ScreenYPos + n * OBJ_STRUCT_SIZE);
		gb->cpu.l = (uint8_t)(wObj1Id + n * OBJ_STRUCT_SIZE);
		CALL(UpdateObjSprite);
	}
}

PORT(DrawObjects_NoPriority)
{
	draw_objects(gb, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_PRIORITY);
}

PORT(DrawObjects_WithPriority)
{
	draw_objects(gb, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
}
