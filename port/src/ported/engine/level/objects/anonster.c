// Port of src/engine/level/objects/anonster.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SILK_PLATFORMS_CREATE_PLATFORM1 (ADDR_SilkPlatformsFunc + 0x26) // .CreatePlatform1
#define ANONSTER_SILK_DESTROY (ADDR_AnonsterSilkFunc + 0x14f) // AnonsterSilkFunc.destroy

// Spawns the three silk platforms one after the other.
PORT_JUMP(SilkPlatformsFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SILK_PLATFORMS_CREATE_PLATFORM1 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SILK_PLATFORMS_CREATE_PLATFORM1;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	wr8(gb, wCurObjStateDuration, 0x28);

	// .asm_50746
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = 0x32;
	wr8(gb, hl, 0x32);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	JUMP(CreateObjectAtRelativePos);
}

PORT_JUMP(SilkPlatformFunc)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	wr8(gb, hl, alu_dec(gb, gb->cpu.a));
	JUMP(MoveObjectDown_Slow);
}

PORT_JUMP(AnonsteWaveLFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_AnonsteWaveFunc);
}

PORT_JUMP(AnonsterWaveFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_AnonsteWaveFunc); // fallthrough
}

PORT_JUMP(AnonsteWaveFunc)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(ANONSTER_SILK_DESTROY);
		return;
	}
	RET();
}
