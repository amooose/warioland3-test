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

// --- Scowler ---

// SC(rom address in bank $15): ScowlerFunc's local labels and return
// addresses, all checked against warioland3.sym / the ROM's call bytes.
#define SC(addr) (ADDR_ScowlerFunc + ((addr) - 0x44e5))
#define SC_54520 SC(0x4520)
#define SC_54542 SC(0x4542)
#define SC_5458A SC(0x458a)
#define SC_546E0 SC(0x46e0)
#define SC_54770 SC(0x4770)
#define SC_54806 SC(0x4806)
#define SC_54853 SC(0x4853)
#define SC_5487D SC(0x487d)
#define SC_548C5 SC(0x48c5)
#define SC_5490B SC(0x490b)
#define SC_54931 SC(0x4931)

static inline void sc_fs(GB *gb, uint16_t fs) { SET_DE(gb, fs); CALL(SetObjectFramesetPtr); }
static inline void sc_fs_jump(GB *gb, uint16_t fs) { SET_DE(gb, fs); JUMP(SetObjectFramesetPtr); }

// call Func_54c07, returning to ret
static inline void sc_54c07(GB *gb, uint16_t ret) { gb_call_ret(gb, ADDR_Func_54c07, ret); }

// The stop_music2 macro
static void sc_stop_music2(GB *gb)
{
	A = 0xff;
	wr8(gb, hMusicIDBackup + 0, A);
	wr8(gb, hMusicID + 0, A);
	A = 0x00;
	wr8(gb, hMusicIDBackup + 1, A);
	wr8(gb, hMusicID + 1, A);
}

// ld hl, wCurObjYPos / [hli] (sub|add) dy, sbc|adc 0 -> hYPos /
// [hli] -> hXPos (op: 1 = sub, 2 = add)
static void sc_hpos_dy(GB *gb, int op, uint8_t dy)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb);
	A = op == 1 ? alu_sub(gb, A, dy) : alu_add(gb, A, dy);
	wr8(gb, hYPosLo, A);
	A = rd_hli(gb);
	A = op == 1 ? alu_sbc(gb, A, 0) : alu_adc(gb, A, 0);
	wr8(gb, hYPosHi, A);
	A = rd_hli(gb); wr8(gb, hXPosLo, A);
	A = rd_hli(gb); wr8(gb, hXPosHi, A);
}

// ld a, [hld] (or [hl]) / and $0f / swap a / add a / add $48 / ld [hl], a
static void sc_reset_var3(GB *gb)
{
	A = alu_and(gb, A, 0x0f);
	A = alu_swap(gb, A);
	A = alu_add(gb, A, A);
	A = alu_add(gb, A, 0x48);
	wr_hl(gb, A);
}

PORT(ScowlerFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, SC_54520);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = rd_hl(gb);
	A = alu_and(gb, A, 0xf0);
	A = alu_or(gb, A, 0x04);
	wr_hld(gb, A);
	sc_reset_var3(gb);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x08; wr_hld(gb, A);
	A = 0xf7; wr_hld(gb, A);
	A = 0xf8; wr_hl(gb, A);
	sc_fs(gb, ADDR_Frameset_69a5f);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	wr_hli(gb, A);
	wr8(gb, wDollBoyActiveBarrels, A);
	wr8(gb, w1d147, A);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_54520)
{
	A = TRUE;
	wr8(gb, wIsBossBattle, A);
	A = BOSS_REGULAR;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	set_update(gb, SC_5458A);
	SET_BC(gb, ADDR_ObjParams_ScrowlerTentacleRight);
	CALL(CreateObjectFromCurObjPos);
	SET_BC(gb, ADDR_ObjParams_ScrowlerTentacleLeft);
	JUMP(CreateObjectFromCurObjPos);
}

PORT_LOCAL(ScowlerFunc, Func_54542)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A))
		return;
	if (dec_mhl(gb))
		return;
	sc_fs(gb, ADDR_Frameset_69b03);
	ld_l(gb, OBJ_Y_POS);
	A = rd_hli(gb); wr8(gb, wWarioYPos + 1, A);
	A = rd_hli(gb); wr8(gb, wWarioYPos + 0, A);
	A = rd_hli(gb); wr8(gb, wWarioXPos + 1, A);
	A = rd_hl(gb); wr8(gb, wWarioXPos + 0, A);
}

// .asm_54646 / .asm_547aa: moves forward unless a wall is ahead.
// Returns false when it jumped to MoveObjectLeft/Right, true when blocked.
static bool sc_walk(GB *gb, uint16_t ret_left, uint16_t ret_right)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); A = alu_sub(gb, A, 0x04); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); A = alu_sbc(gb, A, 0x00); wr8(gb, hYPosHi, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		A = rd_hli(gb); A = alu_sub(gb, A, 0x11); wr8(gb, hXPosLo, A);
		A = rd_hli(gb); A = alu_sbc(gb, A, 0x00); wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3513, ret_left);
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectLeft);
			return false;
		}
		return true;
	}
	A = rd_hli(gb); A = alu_add(gb, A, 0x10); wr8(gb, hXPosLo, A);
	A = rd_hli(gb); A = alu_adc(gb, A, 0x00); wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_3513, ret_right);
	if (!alu_and(gb, A, A)) {
		JUMP(MoveObjectRight);
		return false;
	}
	return true;
}

// .Func_5469d: moves down unless the floor is below. tail: reached with
// a jump (jp MoveObjectDown is a tail jump) rather than a call.
static void sc_5469d(GB *gb, bool tail)
{
	sc_hpos_dy(gb, 2, 0x05);
	gb_call_ret(gb, ADDR_Func_352b, SC(0x46b3));
	if (!alu_and(gb, A, A)) {
		if (tail)
			JUMP(MoveObjectDown);
		else
			CALL(MoveObjectDown);
		return;
	}
	ld_hl(gb, wCurObjSubState);
	res_mhl(gb, OBJSUBFLAG_VDIR_F);
	if (tail)
		RET();
}

// .Func_546bd: moves up while in water
static void sc_546bd(GB *gb, bool tail)
{
	sc_hpos_dy(gb, 1, 0x2c);
	gb_call_ret(gb, ADDR_Func_358b, SC(0x46d3));
	A = rd8(gb, wIsInWaterOrSand);
	if (alu_and(gb, A, A)) {
		if (tail)
			JUMP(MoveObjectUp);
		else
			CALL(MoveObjectUp);
		return;
	}
	ld_hl(gb, wCurObjSubState);
	set_mhl(gb, OBJSUBFLAG_VDIR_F);
	if (tail)
		RET();
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_5469d) { sc_5469d(gb, true); }
PORT_LOCAL_JUMP(ScowlerFunc, Func_546bd) { sc_546bd(gb, true); }

// .asm_54895 (h = wCurObj)
static void sc_54895(GB *gb)
{
	ld_l(gb, OBJ_SUBSTATE);
	A = rd_hld(gb);
	sc_reset_var3(gb);
	ld_l(gb, OBJ_STATE);
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	wr_hld(gb, A);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF) {
		set_mhl(gb, 7);
		sc_fs_jump(gb, ADDR_Frameset_69a68);
		return;
	}
	res_mhl(gb, 7);
	sc_fs_jump(gb, ADDR_Frameset_69a5f);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_5458a)
{
	sc_54c07(gb, SC(0x458d));
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x2a);
	if (ZF) {
		// .asm_54572
		A = 0x5a;
		wr_hl(gb, A);
		play_sfx(gb, SFX_040);
		sc_stop_music2(gb);
		RETURN;
	}
	gb->cpu.b = A;
	A = rd8(gb, wRoomTransitionParam);
	if (alu_and(gb, A, A)) {
		// .asm_54563
		set_update(gb, SC_54542);
		A = 0x1e;
		wr8(gb, wCurObjStateDuration, A);
		RETURN;
	}
	A = gb->cpu.b;
	if (!alu_and(gb, A, A)) {
		// .asm_545f8
		ld_hl(gb, wCurObjVar1);
		A = rd_hl(gb);
		if (alu_and(gb, A, A)) {
			if (dec_mhl(gb)) {
				// .asm_5468e
				gb->cpu.b = alu_and(gb, rd8(gb, wWarioScreenYPos), 0xfe);
				A = alu_and(gb, rd8(gb, wCurObjScreenYPos), 0xfe);
				alu_cp(gb, A, gb->cpu.b);
				if (ZF)
					RETURN;
				if (!CF)
					sc_546bd(gb, true);
				else
					sc_5469d(gb, true);
				return;
			}
			A = alu_rlca(gb, rd8(gb, wCurObjSubState));
			sc_fs_jump(gb, CF ? ADDR_Frameset_69a68 : ADDR_Frameset_69a5f);
			return;
		}
		// .asm_54615
		A = rd8(gb, w1d147);
		if (!alu_and(gb, A, A)) {
			ld_l(gb, OBJ_VAR_3);
			if (!dec_mhl(gb)) {
				inc_l(gb);
				A = rd_hld(gb);
				sc_reset_var3(gb);
				gb->cpu.b = rd8(gb, wWarioScreenYPos);
				A = rd8(gb, wCurObjScreenYPos);
				alu_cp(gb, A, gb->cpu.b);
				if (!CF) {
					A = 0x2f;
					wr8(gb, wCurObjState, A);
					sc_fs(gb, ADDR_Frameset_69a7a);
					A = 0x48; wr_hli(gb, A);
					A = 0xfe; wr_hl(gb, A);
					RETURN;
				}
			}
		}
		// .asm_54646
		if (!sc_walk(gb, SC(0x4666), SC(0x4679)))
			return;
		// .asm_5467d
		sc_fs(gb, ADDR_Frameset_69a71);
		inc_l(gb);
		A = 0x2b; wr_hl(gb, A);
		ld_l(gb, OBJ_SUBSTATE);
		A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
		wr_hl(gb, A);
		RETURN;
	}
	alu_cp(gb, A, 0x2f);
	if (ZF) {
		// .asm_545d3
		ld_hl(gb, wCurObjStateDuration);
		if (!dec_mhl(gb)) {
			sc_54895(gb);
			return;
		}
		A = rd_hl(gb);
		alu_cp(gb, A, 0x20);
		if (!ZF)
			RETURN;
		play_sfx(gb, SFX_07E);
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_BC(gb, CF ? ADDR_ObjParams_InkRight : ADDR_ObjParams_InkLeft);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	A = alu_and(gb, A, 0xfe);
	alu_cp(gb, A, 0x08);
	if (!ZF)
		RETURN;
	// .asm_545ab
	sc_fs(gb, ADDR_Frameset_69a95);
	A = 0x44; wr_hli(gb, A);
	A = 0xfd; wr_hli(gb, A);
	inc_l(gb);
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_07);
	wr_hld(gb, A);
	set_update_l(gb, SC_546E0);
	play_sfx(gb, SFX_0B3);
	RETURN;
}

PORT_LOCAL(ScowlerFunc, Func_546e0)
{
	sc_54c07(gb, SC(0x46e3));
	sc_hpos_dy(gb, 2, 0x05);
	gb_call_ret(gb, ADDR_Func_352b, SC(0x46f9));
	alu_and(gb, A, A);
	SET_BC(gb, ADDR_Data_60a60);
	if (ZF)
		CALL(ApplyObjYMovement);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	sc_fs(gb, ADDR_Frameset_69ab3);
	A = 0xff; wr_hli(gb, A);
	A = 0xfc; wr_hli(gb, A);
	A = 0x02; wr_hl(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_06);
	wr_hld(gb, A);
	set_update_l(gb, SC_54770);
	ld_l(gb, OBJ_STATE);
	xor_a(gb);
	wr_hld(gb, A);
	gb->cpu.b = rd8(gb, wWarioScreenYPos);
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, gb->cpu.b);
	if (!CF) {
		res_mhl(gb, 6);
		return;
	}
	set_mhl(gb, 6);
	res_mhl(gb, 6);
}

// .asm_547e9
static void sc_547e9(GB *gb)
{
	sc_fs(gb, ADDR_Frameset_69aa8);
	A = 0x28; wr_hli(gb, A);
	A = 0xfe; wr_hl(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_07);
	wr_hld(gb, A);
	set_update_l(gb, SC_5487D);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_54770)
{
	sc_54c07(gb, SC(0x4773));
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		A = alu_and(gb, A, 0xfe);
		alu_cp(gb, A, 0x08);
		if (ZF) {
			// .asm_54737
			gb->cpu.b = rd8(gb, wWarioScreenYPos);
			A = rd8(gb, wCurObjScreenYPos);
			alu_cp(gb, A, gb->cpu.b);
			if (CF) {
				play_sfx(gb, SFX_0B4);
				sc_fs(gb, ADDR_Frameset_69abc);
				A = 0x1e; wr_hli(gb, A);
				set_update_l(gb, SC_54806);
				ld_l(gb, OBJ_INTERACTION_TYPE);
				heavy_or(gb, OBJ_INTERACTION_07);
				wr_hld(gb, A);
				JUMP(DecrementRemainingBossHits);
				return;
			}
			// .asm_54765
			xor_a(gb);
			wr_hld(gb, A);
			set_mhl(gb, 6);
			A = 0x02;
			wr8(gb, wCurObjVar2, A);
		} else {
			xor_a(gb);
			wr_hl(gb, A);
		}
	}
	// .asm_54782
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (ZF)
		play_sfx(gb, SFX_0AB);
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		ld_l(gb, OBJ_VAR_2);
		if (!dec_mhl(gb)) {
			sc_547e9(gb);
			RETURN;
		}
	}
	// .asm_5479c
	ld_l(gb, OBJ_SUBSTATE);
	if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
		sc_5469d(gb, false);
	else
		sc_546bd(gb, false);
	// .asm_547aa
	if (!sc_walk(gb, SC(0x47ca), SC(0x47dd)))
		return;
	// .asm_547e1
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
	RETURN;
}

PORT_LOCAL(ScowlerFunc, Func_54806)
{
	sc_54c07(gb, SC(0x4809));
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	ld_l(gb, OBJ_VAR_3);
	xor_a(gb);
	wr_hli(gb, A);
	A = rd_hl(gb);
	A = alu_and(gb, A, 0x0f);
	A = alu_dec(gb, A);
	if (ZF) {
		// .asm_5482a
		A = alu_and(gb, rd_hl(gb), 0x80);
		wr_hl(gb, A);
		set_update_l(gb, SC_548C5);
		sc_fs(gb, ADDR_Frameset_69ac3);
		inc_l(gb);
		A = 0xff; wr_hli(gb, A);
		A = 0x14; wr_hl(gb, A);
		CALL(MoveObjectUpByVar2);
		sc_stop_music2(gb);
		return;
	}
	set_update_l(gb, SC_54853);
	sc_fs(gb, ADDR_Frameset_69b08);
	A = 0x78; wr_hli(gb, A);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_54853)
{
	sc_54c07(gb, SC(0x4856));
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		sc_547e9(gb);
		RETURN;
	}
	sc_hpos_dy(gb, 1, 0x2c);
	gb_call_ret(gb, ADDR_Func_358b, SC(0x4872));
	A = rd8(gb, wIsInWaterOrSand);
	if (!alu_and(gb, A, A))
		RETURN;
	SET_BC(gb, ADDR_Data_60a70);
	JUMP(ApplyObjYMovement);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_5487d)
{
	sc_54c07(gb, SC(0x4880));
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_SCOWLER);
	wr_hld(gb, A);
	set_update_l(gb, SC_5458A);
	sc_54895(gb);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_548c5)
{
	no_actions(gb);
	sc_54c07(gb, SC(0x48cd));
	sc_hpos_dy(gb, 1, 0x18);
	gb_call_ret(gb, ADDR_Func_358b, SC(0x48e3));
	A = rd8(gb, wIsInWaterOrSand);
	alu_and(gb, A, A);
	SET_BC(gb, ADDR_Data_60a70);
	if (!ZF) {
		JUMP(ApplyObjYMovement);
		return;
	}
	ld_hl(gb, wCurObjStateDuration);
	A = 0x0f; wr_hli(gb, A);
	inc_l(gb);
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_PRIORITY_F);
	set_update_l(gb, SC_5490B);
	play_sfx(gb, SFX_0AC);
	RETURN;
}

PORT_LOCAL(ScowlerFunc, Func_5490b)
{
	no_actions(gb);
	sc_54c07(gb, SC(0x4913));
	CALL(MoveObjectDown_Slow);
	SET_BC(gb, ADDR_Data_60a80);
	CALL(ApplyObjXMovement);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	A = 0xb4; wr_hl(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjVar2, A);
	set_update_l(gb, SC_54931);
}

PORT_LOCAL_JUMP(ScowlerFunc, Func_54931)
{
	no_actions(gb);
	sc_54c07(gb, SC(0x4939));
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A) && !dec_mhl(gb)) {
		ld_hl(gb, wCurObjUnk02);
		gb->cpu.e = 0x05;
		FARCALL(Func_ba42);
		A = BOSS_DEFEATED;
		wr8(gb, wBossState, A);
		CALL(UpdateLevelMusic);
		xor_a(gb);
		wr8(gb, wIsBossBattle, A);
		ld_hl(gb, w1d147);
		A = rd_hl(gb);
		if (alu_and(gb, A, A)) {
			xor_a(gb);
			wr_hl(gb, A);
			ld_hl(gb, ADDR_Pals_54c87);
			gb_call_ret(gb, ADDR_WriteScowlerOBPals, SC(0x4972));
			ld_hl(gb, ADDR_Pals_54cb7);
			gb_call_ret(gb, ADDR_WriteScowlerBGPals, SC(0x4978));
		}
	}
	// .asm_54978
	CALL(MoveObjectDown_Slow);
	SET_BC(gb, ADDR_Data_60a90);
	JUMP(ApplyObjXMovement_Loop);
}
