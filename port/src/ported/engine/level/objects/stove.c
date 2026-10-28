// Port of src/engine/level/objects/stove.asm
#include "ported.h"

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


#define ST(off) (ADDR_StoveFunc + (off))
#define ST_UPDATE ST(0x6f)

PORT(StoveFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(ST_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)ST_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 10; wr_hld(gb, A);
	A = (uint8_t)-9; wr_hld(gb, A);
	A = (uint8_t)-9; wr_hl(gb, A);
	SET_DE(gb, ADDR_Frameset_69615);
	CALL(SetObjectFramesetPtr);
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
}

// .set_fall (hl = OBJ_STATE)
static void st_set_fall(GB *gb)
{
	A = OBJSTATE_FALL; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_69615);
	JUMP(SetObjectFramesetPtr);
}

// .set_land
static void st_set_land(GB *gb)
{
	A = OBJSTATE_LAND;
	wr8(gb, wCurObjState, A);
	SET_DE(gb, ADDR_Frameset_69615);
	JUMP(SetObjectFramesetPtr);
}

// .State34 / .AttackedRightStart: slides, beeping, until var2 runs out or
// it hits a wall
static void st_slide(GB *gb, bool right)
{
	ld_l(gb, OBJ_VAR_2);
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (ZF) {
		play_sfx(gb, SFX_078);
		if (!dec_mhl(gb)) {
			st_set_land(gb);
			return;
		}
	}
	ld_l(gb, OBJ_Y_POS);
	A = alu_sub(gb, rd_hli(gb), 4);
	wr8(gb, hYPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0);
	wr8(gb, hYPosHi, A);
	if (right) {
		A = alu_add(gb, rd_hli(gb), 10);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0);
	} else {
		A = alu_sub(gb, rd_hli(gb), 9);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0);
	}
	wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_3513, ST(right ? 0xe5 : 0x130));
	A = alu_and(gb, A, 0x0f);
	if (ZF) {
		if (right)
			JUMP(MoveObjectRightByVar2);
		else
			JUMP(MoveObjectLeftByVar2);
		return;
	}
	st_set_land(gb);
}

// .asm_49813 (hl = OBJ_STATE)
static void st_attacked(GB *gb, uint8_t state)
{
	A = state;
	wr_hl(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
	SET_DE(gb, ADDR_Frameset_69618);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(StoveFunc, Update)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	alu_cp(gb, A, OBJSTATE_FALL);
	if (ZF) {
		// .Fall
		SET_BC(gb, ADDR_Data_605e0);
		CALL(ApplyObjYMovement);
		obj_pos_to_hpos(gb);
		gb_call_ret(gb, ADDR_Func_352b, ST(0x4d));
		if (!alu_and(gb, A, A))
			RETURN;
		ld_hl(gb, wCurObjYPos);
		A = rd8(gb, hYPosLo); wr_hli(gb, A);
		A = rd8(gb, hYPosHi); wr_hl(gb, A);
		A = OBJSTATE_LAND;
		wr8(gb, wCurObjState, A);
		A = rd8(gb, wCurObjVar1);
		if (!alu_and(gb, A, A))
			RETURN;
		play_sfx(gb, SFX_063);
		gb->cpu.b = 24;
		JUMP(DoGroundShake);
		return;
	}
	if (!alu_and(gb, A, A)) {
		st_set_fall(gb);
		return;
	}
	alu_cp(gb, A, OBJSTATE_LAND);
	if (ZF) {
		// .Land
		ld_l(gb, OBJ_Y_POS);
		A = rd_hli(gb); wr8(gb, hYPosLo, A);
		A = rd_hli(gb); wr8(gb, hYPosHi, A);
		A = rd_hli(gb); wr8(gb, hXPosLo, A);
		A = rd_hl(gb); wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3543, ST(0x148));
		alu_cp(gb, A, 0x10);
		ld_hl(gb, wCurObjYPos);
		A = rd8(gb, hYPosLo); wr_hli(gb, A);
		A = rd8(gb, hYPosHi); wr_hl(gb, A);
		if (ZF)
			RETURN;
		ld_hl(gb, wCurObjXPos);
		A = alu_sub(gb, rd_hli(gb), 7);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0);
		wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_35a3, ST(0x164));
		if (alu_and(gb, A, A))
			RETURN;
		ld_hl(gb, wCurObjXPos);
		A = alu_add(gb, rd_hli(gb), 8);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0);
		wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_35a3, ST(0x176));
		if (alu_and(gb, A, A))
			RETURN;
		A = OBJSTATE_00;
		wr8(gb, wCurObjState, A);
		A = alu_inc(gb, A);
		wr8(gb, wCurObjVar1, A);
		RETURN;
	}
	alu_cp(gb, A, OBJSTATE_ATTACKED_LEFT);
	if (ZF) {
		st_slide(gb, true);
		return;
	}
	alu_cp(gb, A, OBJSTATE_ATTACKED_LEFT_START);
	if (ZF) {
		st_attacked(gb, OBJSTATE_ATTACKED_LEFT);
		return;
	}
	alu_cp(gb, A, OBJSTATE_ATTACKED_RIGHT);
	if (ZF) {
		st_slide(gb, false);
		return;
	}
	alu_cp(gb, A, OBJSTATE_ATTACKED_RIGHT_START);
	if (ZF) {
		st_attacked(gb, OBJSTATE_ATTACKED_RIGHT);
		return;
	}
	alu_cp(gb, A, OBJSTATE_VANISH_TOUCH);
	if (ZF) {
		A = rd8(gb, wTransformation);
		if (alu_and(gb, A, A)) {
			JUMP(Func_3173);
			return;
		}
		A = OBJSTATE_3A;
		wr_hl(gb, A);
		SET_DE(gb, ADDR_Frameset_69625);
		CALL(SetObjectFramesetPtr);
		A = 44; wr_hli(gb, A);
		RETURN;
	}
	alu_cp(gb, A, OBJSTATE_3A);
	if (ZF) {
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb))
			RETURN;
		st_set_land(gb);
		return;
	}
	st_set_fall(gb);
}
