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

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define MIZUUO_UPDATE1 (ADDR_MizuuoFunc + 0x23) // .Update1
#define MIZUUO_UPDATE2 (ADDR_MizuuoFunc + 0x8f) // .Update2
#define MIZUUO_UPDATE3 (ADDR_MizuuoFunc + 0xb6) // .Update3
#define MIZUUO_UPDATE4 (ADDR_MizuuoFunc + 0xff) // .Update4

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static inline uint16_t set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld a, no_actions_for 1 / ld [wCurObjAction], a
static inline void no_actions_for_1(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// ld a, [wCurObjSubState] / rlca / ld_obj_frameset right_dir?b:a
static inline void frameset_by_hdir(GB *gb, uint16_t left, uint16_t right)
{
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? right : left);
}

// call SetObjectFramesetPtr / ld a, v / ld [hli], a
static inline void set_frameset_duration(GB *gb, uint8_t v)
{
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = v;
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
}

PORT_JUMP(MizuuoFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = set_update(gb, hl, MIZUUO_UPDATE1);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_686e5);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	hl = LO(hl, alu_inc(gb, (uint8_t)hl)); // inc l
	wr8(gb, hl++, 0x30);
	gb->cpu.a = 0x08;
	wr8(gb, hl, 0x08);
	SET_HL(gb, hl);
	JUMP(MoveObjectLeftByVar2);
}

// Swims back and forth, turning around now and then, until it's time to
// surface.
PORT_LOCAL_JUMP(MizuuoFunc, Update1)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
		wr8(gb, hl, gb->cpu.a);
		gb->cpu.a = alu_rlca(gb, gb->cpu.a);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_686ea : ADDR_Frameset_686e5);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	// .asm_48776
	hl = LO(hl, alu_inc(gb, (uint8_t)hl)); // inc l
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x5f);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_487a1
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = set_update(gb, hl, MIZUUO_UPDATE2);
		SET_HL(gb, hl);
		frameset_by_hdir(gb, ADDR_Frameset_686ef, ADDR_Frameset_686f8);
		set_frameset_duration(gb, 0x19);
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		if (gb->cpu.f & FLAG_C)
			JUMP(MoveObjectRight);
		else
			JUMP(MoveObjectLeft);
		return;
	}
	// .asm_48789
	frameset_by_hdir(gb, ADDR_Frameset_68713, ADDR_Frameset_6871e);
	set_frameset_duration(gb, 0x22);
	gb->cpu.a = 0x60;
	wr8(gb, REG_HL(gb), 0x60);
	RET();
}

PORT_LOCAL(MizuuoFunc, Update2)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	SET_HL(gb, set_update(gb, hl, MIZUUO_UPDATE3));
	frameset_by_hdir(gb, ADDR_Frameset_686a4, ADDR_Frameset_686b9);
	set_frameset_duration(gb, 0x5d);
}

// Spits a projectile partway through, unless it gets hit.
PORT_LOCAL_JUMP(MizuuoFunc, Update3)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .asm_48810
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	} else {
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			gb->cpu.a = d;
			alu_cp(gb, d, 0x24);
			if (!(gb->cpu.f & FLAG_Z)) {
				RET();
				return;
			}
			play_sfx(gb, SFX_07F);
			gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
			SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_MizzouProjectileRight : ADDR_ObjParams_MizzouProjectileLeft);
			JUMP(CreateObjectAtRelativePos);
			return;
		}
	}
	// .asm_48812
	frameset_by_hdir(gb, ADDR_Frameset_68701, ADDR_Frameset_6870a);
	set_frameset_duration(gb, 0x19);
	SET_HL(gb, set_update(gb, REG_HL(gb), MIZUUO_UPDATE4));
	RET();
}

PORT_LOCAL_JUMP(MizuuoFunc, Update4)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	SET_HL(gb, set_update(gb, hl, MIZUUO_UPDATE1));
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_686ea : ADDR_Frameset_686e5);
	JUMP(SetObjectFramesetPtr);
}

// The burst projectile disappears when its state duration runs out.
PORT_LOCAL(MizzouProjectileFunc, Update)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
