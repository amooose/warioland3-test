// Port of src/engine/level/objects/beam_bot.asm
#include "ported.h"

PORT_JUMP(BeamFunc)
{
	SET_HL(gb, wCurObjState);
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjState, 0);
		play_sfx(gb, SFX_07D);
	}
	JUMP(ApplyObjXMovement);
}

// Sets the beam's update function to the .Update that follows, makes it
// non-collidable and plays the beam sfx, then runs .Update: changes frameset
// when wCurObjStateDuration runs out and moves the beam with movement.
static void beam_func(GB *gb, uint16_t update, uint16_t frameset, uint16_t movement)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	play_sfx(gb, SFX_07C);

	// .Update
	hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, gb->cpu.a);
		wr8(gb, hl, d);
		if (!d) {
			SET_DE(gb, frameset);
			CALL(SetObjectFramesetPtr);
		}
	}
	SET_BC(gb, movement);
	JUMP_TO(ADDR_BeamFunc);
}

PORT_JUMP(BeamFunc_Left)
{
	beam_func(gb, ADDR_BeamFunc_Left + 0x15, ADDR_Frameset_68ef3, ADDR_Data_60610);
}

PORT_JUMP(BeamFunc_Right)
{
	beam_func(gb, ADDR_BeamFunc_Right + 0x15, ADDR_Frameset_68ef8, ADDR_Data_60630);
}
