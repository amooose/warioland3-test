// Port of src/engine/level/objects/water_drop.asm
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

#define WD(off) (ADDR_WaterDropFunc + (off))
#define WD_49D83 WD(0x1f)

// .asm_49ddd: back at the top, waiting to fall again
static void wd_49ddd(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_69761);
	CALL(SetObjectFramesetPtr);
	A = 0x41; wr_hli(gb, A);
	A = rd8(gb, wCurObjYPos + 0); wr_hli(gb, A);
	A = rd8(gb, wCurObjYPos + 1); wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	inc_l(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	RETURN;
}

PORT_JUMP(WaterDropFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(WD_49D83 >> 8); wr_hld(gb, A);
	A = (uint8_t)WD_49D83; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x04; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
	A = 0xff; wr_hl(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	wd_49ddd(gb);
}

// .asm_49dbd: moves back up to the start position (var1/var2)
static void wd_49dbd(GB *gb)
{
	A = 0x81;
	wr8(gb, wCurObjAction, A);
	ld_l(gb, OBJ_STATE_DURATION);
	A = rd_hl(gb);
	gb->cpu.e = A;
	if (alu_and(gb, A, A))
		dec_mhl(gb);
	inc_l(gb);
	A = rd_hli(gb);
	gb->cpu.c = A;
	gb->cpu.b = rd_hl(gb);
	ld_l(gb, OBJ_Y_POS);
	A = rd_hli(gb);
	alu_cp(gb, A, gb->cpu.c);
	if (!ZF) {
		JUMP(MoveObjectUp);
		return;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, gb->cpu.b);
	if (!ZF) {
		JUMP(MoveObjectUp);
		return;
	}
	A = gb->cpu.e;
	if (alu_and(gb, A, A))
		RETURN;
	wd_49ddd(gb);
}

// .asm_49da6: splashing
static void wd_49da6(GB *gb)
{
	A = 0x81;
	wr8(gb, wCurObjAction, A);
	ld_l(gb, OBJ_STATE_DURATION);
	if (dec_mhl(gb))
		RETURN;
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_INVISIBLE_F);
	A = 0x5b;
	wr8(gb, wCurObjState, A);
	A = 0x3c;
	wr8(gb, wCurObjStateDuration, A);
	wd_49dbd(gb);
}

// .asm_49d92: hits the ground (hl = OBJ_STATE)
static void wd_49d92(GB *gb)
{
	A = 0x5a;
	wr_hl(gb, A);
	SET_DE(gb, ADDR_Frameset_69775);
	CALL(SetObjectFramesetPtr);
	A = 0x18; wr_hli(gb, A);
	play_sfx(gb, SFX_0A1);
	wd_49da6(gb);
}

PORT_LOCAL_JUMP(WaterDropFunc, Func_49d83)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		// .asm_49df7
		ld_l(gb, OBJ_STATE_DURATION);
		A = rd_hl(gb);
		if (alu_and(gb, A, A)) {
			if (dec_mhl(gb))
				RETURN;
			SET_DE(gb, ADDR_Frameset_69772);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		obj_pos_to_hpos(gb);
		gb_call_ret(gb, ADDR_Func_352b, WD(0xb3));
		if (!alu_and(gb, A, A)) {
			A = rd8(gb, wIsInWaterOrSand);
			if (!alu_and(gb, A, A)) {
				SET_BC(gb, ADDR_FallingYVel_Light);
				JUMP(ApplyObjYMovement);
				return;
			}
		}
		ld_hl(gb, wCurObjYPos);
		A = rd8(gb, hYPosLo); wr_hli(gb, A);
		A = rd8(gb, hYPosHi); wr_hl(gb, A);
		ld_l(gb, OBJ_STATE);
		wd_49d92(gb);
		return;
	}
	alu_cp(gb, A, 0x5a);
	if (ZF) {
		wd_49da6(gb);
		return;
	}
	alu_cp(gb, A, 0x5b);
	if (ZF) {
		wd_49dbd(gb);
		return;
	}
	wd_49d92(gb);
}
