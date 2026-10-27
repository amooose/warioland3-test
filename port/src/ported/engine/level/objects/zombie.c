// Port of src/engine/level/objects/zombie.asm
#include "ported.h"

#define ZOMBIE_FUNC_45C83 (ADDR_ZombieFunc + 0x1cf) // ZombieFunc.asm_45c83
#define ZOMBIE_HEAD_FLICKER (ADDR_ZombieHeadFunc + 0x2e) // .Flicker

// A zombie's head, flying off: once its state is in [$0b, $0f) it starts
// flickering, else ZombieFunc handles it.
PORT_JUMP(ZombieHeadFunc)
{
	CALL(ApplyObjXMovement);
	uint16_t hl = wCurObjState;
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x0b);
	if (gb->cpu.f & FLAG_C) {
		JUMP_TO(ZOMBIE_FUNC_45C83);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (!(gb->cpu.f & FLAG_C)) {
		JUMP_TO(ZOMBIE_FUNC_45C83);
		return;
	}
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	// ld_obj_frameset
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68dda : ADDR_Frameset_68dd5);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 40;
	hl = REG_HL(gb);
	wr8(gb, hl++, 40);
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(ZOMBIE_HEAD_FLICKER >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)ZOMBIE_HEAD_FLICKER;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	RET();
}

// Counts down wCurObjStateDuration; when it runs out, turns the head around.
// Despawns once it's off-screen with the duration at 0.
static void zombie_head_func(GB *gb, uint16_t frameset, uint16_t movement)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
		SET_HL(gb, hl);
		if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			RET();
			return;
		}
	} else {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
			SET_HL(gb, hl);
			gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
			wr8(gb, hl, gb->cpu.a);
			SET_DE(gb, frameset);
			CALL(SetObjectFramesetPtr);
		}
	}
	SET_BC(gb, movement);
	JUMP_TO(ADDR_ZombieHeadFunc);
}

PORT_JUMP(ZombieHeadFunc_Left) { zombie_head_func(gb, ADDR_Frameset_68dda, ADDR_Data_605a0); }
PORT_JUMP(ZombieHeadFunc_Right) { zombie_head_func(gb, ADDR_Frameset_68dd5, ADDR_Data_60560); }
