// Port of src/engine/level/objects/mizuuo.asm
#include "ported.h"

#define MIZZOU_PROJECTILE_UPDATE (ADDR_MizzouProjectileFunc + 0x72) // .Update

PORT_JUMP(MizzouProjectileFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_MizzouProjectileFunc);
}

PORT_JUMP(MizzouProjectileFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_MizzouProjectileFunc); // fallthrough
}

// The projectile bursts when it hits a wall or its state changes, and
// disappears once out of water (with var1 set).
PORT_JUMP(MizzouProjectileFunc)
{
	gb->cpu.l = OBJ_FLAGS;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, ADDR_MizzouProjectileFunc + 0x16);
	bool burst = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (!burst) {
		gb->cpu.a = rd8(gb, wCurObjVar1);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			SET_BC(gb, ADDR_Data_60700);
		} else {
			gb_call_ret(gb, ADDR_Func_3543, ADDR_MizzouProjectileFunc + 0x22);
			if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
				burst = true;
			} else {
				gb->cpu.a = rd8(gb, wIsInWaterOrSand);
				if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
					// .in_water
					gb->cpu.a = 0;
					gb->cpu.f = FLAG_Z; // xor a
					wr8(gb, wCurObjFlags, 0);
					RET();
					return;
				}
				SET_BC(gb, ADDR_Data_60740);
			}
		}
	}
	if (!burst) {
		// .asm_4888e
		CALL(ApplyObjYMovement);
		gb->cpu.a = rd8(gb, wCurObjState);
		burst = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!burst) {
		hl = wCurObjStateDuration;
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		uint8_t d = alu_dec(gb, gb->cpu.a);
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
		SET_HL(gb, hl);
		gb->cpu.a = 0x01;
		wr8(gb, hl, 0x01);
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_686db : ADDR_Frameset_686d8);
		JUMP(SetObjectFramesetPtr);
		return;
	}

	// .asm_488b5
	hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(MIZZOU_PROJECTILE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)MIZZOU_PROJECTILE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_686de);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x17;
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x17); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
	RET();
}
