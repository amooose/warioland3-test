// Port of src/engine/level/objects/scowler.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define FLOATING_RING_SPAWN (ADDR_FloatingRingSpawnerFunc + 0x16) // .Func_54997

// Waits $40 frames, then spawns a floating ring.
PORT(FloatingRingSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(FLOATING_RING_SPAWN >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FLOATING_RING_SPAWN;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_HL(gb, hl);
	wr8(gb, wCurObjStateDuration, 0x40);
	// .Func_549ab
	wr8(gb, wCurObjFrameDuration, 0x02);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// Asm-mirroring helpers: each is one instruction on the real registers.
#define A (gb->cpu.a)
#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)
#define RETURN do { RET(); return; } while (0)
static inline void ld_hl(GB *gb, uint16_t v) { SET_HL(gb, v); }
static inline void ld_l(GB *gb, uint8_t v) { gb->cpu.l = v; }
static inline uint8_t rd_hl(GB *gb) { return rd8(gb, REG_HL(gb)); }
static inline void wr_hl(GB *gb, uint8_t v) { wr8(gb, REG_HL(gb), v); }
static inline uint8_t rd_hli(GB *gb) { uint16_t hl = REG_HL(gb); SET_HL(gb, hl + 1); return rd8(gb, hl); }
static inline uint8_t rd_hld(GB *gb) { uint16_t hl = REG_HL(gb); SET_HL(gb, hl - 1); return rd8(gb, hl); }
static inline void wr_hli(GB *gb, uint8_t v) { uint16_t hl = REG_HL(gb); wr8(gb, hl, v); SET_HL(gb, hl + 1); }
static inline void wr_hld(GB *gb, uint8_t v) { uint16_t hl = REG_HL(gb); wr8(gb, hl, v); SET_HL(gb, hl - 1); }
static inline void xor_a(GB *gb) { A = 0; gb->cpu.f = FLAG_Z; }
static inline void inc_l(GB *gb) { gb->cpu.l = alu_inc(gb, gb->cpu.l); }
static inline void dec_l(GB *gb) { gb->cpu.l = alu_dec(gb, gb->cpu.l); }
static inline uint8_t inc_mhl(GB *gb) { uint8_t v = alu_inc(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline uint8_t dec_mhl(GB *gb) { uint8_t v = alu_dec(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline void set_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) | (1 << bit))); }
static inline void res_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) & ~(1 << bit))); }
// ld a, [hl] / and HEAVY_OBJ / or x
static inline void heavy_or(GB *gb, uint8_t x) { A = alu_or(gb, alu_and(gb, rd_hl(gb), HEAVY_OBJ), x); }
// ld a, [hld] / rlca / ld_obj_frameset left, right
static inline void frameset_by_dir_hld(GB *gb, uint16_t nc, uint16_t c)
{
	A = alu_rlca(gb, rd_hld(gb));
	SET_DE(gb, CF ? c : nc);
}
// ld hl, wCurObjYPos / copy the position to hYPos / hXPos
static inline void obj_pos_to_hpos(GB *gb)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); wr8(gb, hYPosHi, A);
	A = rd_hli(gb); wr8(gb, hXPosLo, A);
	A = rd_hl(gb); wr8(gb, hXPosHi, A);
}

// rst JumpTable, with the table at `table`.
static inline uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}


#define FR(off) (ADDR_FloatingRingFunc + (off))
#define TC(off) (ADDR_ScrowlerTentacleFunc + (off))
#define INK(off) (ADDR_InkFunc + (off))

static void set_update(GB *gb, uint16_t f)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

PORT_JUMP(FloatingRingFunc)
{
	no_actions(gb);
	set_update(gb, FR(0x11));
	JUMP(MoveObjectLeftByVar2);
}

PORT_LOCAL_JUMP(FloatingRingFunc, Func_549c7)
{
	no_actions(gb);
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (CF)
		RETURN;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectUp);
		return;
	}
	A = 0x3c;
	wr_hl(gb, A);
	set_update_l(gb, FR(0x38));
	ld_l(gb, OBJ_Y_POS);
	A = rd_hli(gb);
	wr8(gb, wCurObjVar2, A);
	A = rd_hl(gb);
	wr8(gb, wCurObjVar3, A);
	RETURN;
}

PORT_LOCAL(FloatingRingFunc, Func_549ee)
{
	CALL(SetOneWayPlatformAction);
	ld_hl(gb, wCurObjFlags);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F))
		return;
	res_mhl(gb, OBJFLAG_STEPPED_F);
	set_update_l(gb, FR(0x4c));
}

PORT_LOCAL_JUMP(FloatingRingFunc, Func_54a02)
{
	CALL(SetOneWayPlatformAction);
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!CF)
		RETURN;
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		set_update_l(gb, FR(0x77));
		RETURN;
	}
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectUp);
		return;
	}
	res_mhl(gb, OBJFLAG_STEPPED_F);
	gb->cpu.b = 0x01;
	gb_call_ret(gb, ADDR_MoveWarioUp, FR(0x68));
	gb_call_ret(gb, ADDR_HandleUpwardsFloorTransition, FR(0x6b));
	JUMP(MoveObjectUp);
}

PORT_LOCAL_JUMP(FloatingRingFunc, Func_54a2d)
{
	CALL(SetOneWayPlatformAction);
	ld_hl(gb, wCurObjVar2);
	gb->cpu.b = rd_hl(gb);
	A = rd8(gb, wCurObjYPos + 0);
	alu_cp(gb, A, gb->cpu.b);
	bool back = false;
	if (ZF) {
		inc_l(gb);
		gb->cpu.b = rd_hl(gb);
		A = rd8(gb, wCurObjYPos + 1);
		alu_cp(gb, A, gb->cpu.b);
		back = ZF;
	}
	if (back) {
		// .asm_54a5b
		ld_l(gb, OBJ_FLAGS);
		if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F))
			RETURN;
		res_mhl(gb, OBJFLAG_STEPPED_F);
		set_update_l(gb, FR(0x4c));
		A = 0x3c;
		wr8(gb, wCurObjStateDuration, A);
		RETURN;
	}
	// .asm_54a42
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectDown_Slow);
		return;
	}
	res_mhl(gb, 5);
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!CF)
		RETURN;
	gb->cpu.b = 0x01;
	gb_call_ret(gb, ADDR_MoveWarioDown, FR(0x9f));
	gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, FR(0xa2));
	JUMP(MoveObjectDown);
}

PORT(ScrowlerTentacleFunc)
{
	ld_hl(gb, wCurObjFlags);
	set_mhl(gb, 4);
	set_update(gb, TC(0x27));
}

// sets wCurObjYPos to (bc - dy, de + dx), dy and dx being signed
static void tc_set_pos(GB *gb, uint8_t dy, int dx)
{
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, gb->cpu.c, dy);
	wr_hli(gb, A);
	A = alu_sbc(gb, gb->cpu.b, 0x00);
	wr_hli(gb, A);
	if (dx >= 0) {
		A = alu_add(gb, gb->cpu.e, (uint8_t)dx);
		wr_hli(gb, A);
		A = alu_adc(gb, gb->cpu.d, 0x00);
	} else {
		A = alu_sub(gb, gb->cpu.e, (uint8_t)-dx);
		wr_hli(gb, A);
		A = alu_sbc(gb, gb->cpu.d, 0x00);
	}
	wr_hli(gb, A);
}

// .asm_54b2e / .asm_54b5a
static void tc_follow(GB *gb, bool other_side)
{
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, wCurObjId);
	alu_cp(gb, A, SCROWLER_TENTACLE_RIGHT);
	bool right = ZF;
	if (!other_side) {
		if (right)
			tc_set_pos(gb, 0x04, -0x17);
		else
			tc_set_pos(gb, 0x0b, 0x0f);
	} else {
		if (right)
			tc_set_pos(gb, 0x0b, -0x10);
		else
			tc_set_pos(gb, 0x04, 0x16);
	}
}

// (a = SCROWLER_TENTACLE_RIGHT?) / jr z, .asm_54b5a / jr .asm_54b2e
static void tc_by_id(GB *gb)
{
	A = rd8(gb, wCurObjId);
	alu_cp(gb, A, SCROWLER_TENTACLE_RIGHT);
	tc_follow(gb, ZF);
}

static void tc_interaction(GB *gb, uint8_t x, uint8_t top, uint8_t bottom)
{
	no_actions(gb);
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, x);
	wr_hli(gb, A);
	A = top; wr_hli(gb, A);
	A = bottom; wr_hli(gb, A);
}

PORT_LOCAL(ScrowlerTentacleFunc, Func_54a97)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x2a);
	if (ZF) {
		// .asm_54a7f
		A = 0x5a;
		wr_hl(gb, A);
		play_sfx(gb, SFX_040);
		A = 0xff;
		wr8(gb, hMusicIDBackup + 0, A);
		wr8(gb, hMusicID + 0, A);
		A = 0x00;
		wr8(gb, hMusicIDBackup + 1, A);
		wr8(gb, hMusicID + 1, A);
		return;
	}
	A = rd8(gb, wRoomTransitionParam);
	if (alu_and(gb, A, A))
		return;
	xor_a(gb);
	wr_hl(gb, A);
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb);
		alu_cp(gb, A, SCOWLER);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = alu_rra(gb, rd_hli(gb));
			if (CF)
				break;
		}
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF)
			return;
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
	// .asm_54ac6
	inc_l(gb);
	inc_l(gb);
	A = rd_hli(gb);
	gb->cpu.c = A;
	A = rd_hli(gb);
	gb->cpu.b = A;
	A = rd_hli(gb);
	gb->cpu.e = A;
	gb->cpu.d = rd_hl(gb);
	A = alu_add(gb, gb->cpu.l, OBJ_VAR_1 - (OBJ_X_POS + 1));
	gb->cpu.l = A;
	A = rd_hli(gb);
	if (!alu_and(gb, A, A)) {
		// .asm_54b28
		inc_l(gb);
		inc_l(gb);
		tc_follow(gb, alu_bit(gb, rd_hl(gb), 7));
		return;
	}
	alu_cp(gb, A, 0xfc);
	if (ZF) {
		tc_by_id(gb);
		return;
	}
	alu_cp(gb, A, 0xfd);
	if (ZF) {
		tc_interaction(gb, OBJ_INTERACTION_43, 0xfd, 0x02);
		return;
	}
	alu_cp(gb, A, 0xfe);
	if (ZF) {
		tc_interaction(gb, OBJ_INTERACTION_WATER_TELEPORTING, 0xf6, 0xfe);
		return;
	}
	alu_cp(gb, A, 0xff);
	if (ZF) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		return;
	}
	tc_by_id(gb);
}

// .Left / .Right: drifts for the state duration, then towards Wario, and
// rises in water; disappears when hit or out of water
static void ink_func(GB *gb, bool right)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
		if (right)
			CALL(MoveObjectRight_Slow);
		else
			CALL(MoveObjectLeft_Slow);
	} else {
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		alu_cp(gb, A, gb->cpu.b);
		if (CF)
			CALL(MoveObjectRight_Slow);
		else
			CALL(MoveObjectLeft_Slow);
	}
	// .asm_54bb6
	A = rd8(gb, wCurObjState);
	if (alu_and(gb, A, A)) {
		A = 0x01;
		wr8(gb, w1d147, A);
	} else {
		ld_hl(gb, wCurObjYPos);
		A = alu_sub(gb, rd_hli(gb), 0x10);
		wr8(gb, hYPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
		wr8(gb, hYPosHi, A);
		A = rd_hli(gb); wr8(gb, hXPosLo, A);
		A = rd_hli(gb); wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_358b, INK(0x4c));
		A = rd8(gb, wIsInWaterOrSand);
		if (alu_and(gb, A, A)) {
			A = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
			if (!ZF)
				RETURN;
			JUMP(MoveObjectUp);
			return;
		}
	}
	// .asm_54be6
	SET_DE(gb, ADDR_Frameset_69af5);
	CALL(SetObjectFramesetPtr);
	A = 0x19; wr_hli(gb, A);
	set_update_l(gb, INK(0x72));
	RETURN;
}

PORT_JUMP(InkFunc) { ink_func(gb, false); }
PORT_LOCAL_JUMP(InkFunc, Right) { ink_func(gb, true); }

PORT_LOCAL(InkFunc, Func_54bf8)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}

PORT_JUMP(WriteScowlerOBPals)
{
	gb->cpu.c = 5;
	gb->cpu.b = 3;
	JUMP(SetOBPals);
}

PORT_JUMP(WriteScowlerBGPals)
{
	gb->cpu.c = 0;
	gb->cpu.b = 8;
	JUMP(SetBGPals);
}

// The palette flashes of the Scowler fight, timed by w1d147.
PORT_JUMP(Func_54c07)
{
	ld_hl(gb, w1d147);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A))
		RETURN;
	gb->cpu.b = A;
	A = rd8(gb, wDollBoyActiveBarrels);
	if (alu_and(gb, A, A)) {
		ld_hl(gb, wDollBoyActiveBarrels);
		dec_mhl(gb);
		RETURN;
	}
	inc_mhl(gb);
	A = gb->cpu.b;
	alu_cp(gb, A, 0x02);
	if (ZF) {
		play_sfx(gb, SFX_0B5);
		goto asm_54c39;
	}
	alu_cp(gb, A, 0x3c);
	if (ZF) {
		play_sfx(gb, SFX_0B6);
		A = 0xff;
		wr8(gb, wDollBoyActiveBarrels, A);
		gb->cpu.c = 5;
		gb->cpu.b = 3;
		CALL(BlackOutOBPals);
		gb->cpu.c = 0;
		gb->cpu.b = 8;
		JUMP(BlackOutBGPals);
		return;
	}
	alu_cp(gb, A, 0x3d);
	if (ZF) {
		play_sfx(gb, SFX_0B7);
		goto asm_54c39;
	}
	alu_cp(gb, A, 0x78);
	if (!ZF)
		RETURN;
	play_sfx(gb, SFX_0B8);
	xor_a(gb);
	wr_hl(gb, A);
	ld_hl(gb, ADDR_Pals_54c87);
	gb_call(gb, ADDR_WriteScowlerOBPals);
	ld_hl(gb, ADDR_Pals_54cb7);
	JUMP_TO(ADDR_WriteScowlerBGPals);
	return;

asm_54c39:
	ld_hl(gb, ADDR_Pals_54c9f);
	gb_call(gb, ADDR_WriteScowlerOBPals);
	ld_hl(gb, ADDR_Pals_54cf7);
	JUMP_TO(ADDR_WriteScowlerBGPals);
}
