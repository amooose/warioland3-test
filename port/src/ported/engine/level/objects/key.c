// Port of src/engine/level/objects/key.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define UPDATE (ADDR_KeyFunc + 0x25) // .Update

PORT(KeyFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_6804f);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 4);
	wr8(gb, hl--, (uint8_t)-5); // OBJ_COLLBOX_LEFT
	gb->cpu.a = (uint8_t)-4;
	wr8(gb, hl, gb->cpu.a); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}

// Despawns the object once it's been collected (state OBJSTATE_18).
PORT_LOCAL(KeyFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, OBJSTATE_18);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
	SET_HL(gb, wCurObjUnk02);
	FARCALL(Func_bb2d);
}
