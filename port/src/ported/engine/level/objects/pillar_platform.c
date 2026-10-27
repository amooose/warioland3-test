// Port of src/engine/level/objects/pillar_platform.asm
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

#define PP(off) (ADDR_PillarPlatformFunc + (off))
#define PP_49CC2 PP(0x34)
#define PP_49CCE PP(0x40)
#define PP_49CE5 PP(0x57)

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void pp_func(GB *gb)
{
	A = gb->cpu.b;
	wr8(gb, wCurObjVar1, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	set_update_l(gb, PP_49CE5);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x00; wr_hld(gb, A);
	A = 0xf9; wr_hld(gb, A);
	A = 0xf4; wr_hl(gb, A);
	A = alu_rra(gb, rd8(gb, wDayNight));
	SET_DE(gb, CF ? ADDR_Frameset_6975e : ADDR_Frameset_6975b);
	JUMP(SetObjectFramesetPtr);
}

PORT_JUMP(PillarPlatformFunc) { pp_func(gb); }

PORT_JUMP(PillarPlatform1Func)
{
	gb->cpu.b = 0x01;
	pp_func(gb);
}

PORT_JUMP(PillarPlatform2Func)
{
	gb->cpu.b = 0x02;
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, rd_hl(gb), 0x10);
	wr_hli(gb, A);
	if (CF)
		dec_mhl(gb);
	pp_func(gb);
}

PORT_JUMP(PillarPlatform3Func)
{
	gb->cpu.b = 0x00;
	ld_hl(gb, wCurObjYPos);
	A = alu_add(gb, rd_hl(gb), 0x10);
	wr_hli(gb, A);
	if (CF)
		inc_mhl(gb);
	pp_func(gb);
}

// .asm_49cd8
static void pp_49cd8(GB *gb)
{
	xor_a(gb);
	wr8(gb, wCurObjState, A);
	set_update_l(gb, PP_49CE5);
	RETURN;
}

PORT_LOCAL_JUMP(PillarPlatformFunc, Func_49cc2)
{
	CALL(SetOneWayPlatformAction);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectUp_Fast);
		return;
	}
	pp_49cd8(gb);
}

PORT_LOCAL_JUMP(PillarPlatformFunc, Func_49cce)
{
	CALL(SetOneWayPlatformAction);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectDown_Fast);
		return;
	}
	pp_49cd8(gb);
}

// .Func_49d43: sets the state of the objects of type $20 whose OBJ_UNK_01 is
// e to c (a = b = first object offset)
static void pp_49d43(GB *gb)
{
	gb->cpu.h = (uint8_t)(wObjects >> 8);
	for (;;) {
		ld_l(gb, OBJ_UNK_01);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb);
		alu_cp(gb, A, gb->cpu.e);
		if (ZF) {
			A = alu_add(gb, gb->cpu.l, 0x06);
			gb->cpu.l = A;
			A = rd_hl(gb);
			alu_cp(gb, A, 0x20);
			if (ZF) {
				A = alu_add(gb, gb->cpu.l, 0x14);
				gb->cpu.l = A;
				wr_hl(gb, gb->cpu.c);
			}
		}
		// .asm_49d5b
		A = gb->cpu.b;
		alu_cp(gb, A, 0xe0);
		if (ZF)
			return;
		A = alu_add(gb, A, 0x20);
		gb->cpu.b = A;
	}
}

PORT_LOCAL(PillarPlatformFunc, Func_49d43) { pp_49d43(gb); }

PORT_LOCAL(PillarPlatformFunc, Func_49ce5)
{
	CALL(SetOneWayPlatformAction);
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A))
		return;
	alu_cp(gb, A, 0x2b);
	if (ZF) {
		ld_l(gb, OBJ_VAR_1);
		A = rd_hl(gb);
		alu_cp(gb, A, 0x02);
		if (!CF) {
			xor_a(gb);
			wr8(gb, wCurObjState, A);
			return;
		}
		inc_mhl(gb);
		dec_l(gb);
		A = 0x09; wr_hl(gb, A);
		set_update_l(gb, PP_49CC2);
		return;
	}
	alu_cp(gb, A, 0x0a);
	if (!ZF) {
		xor_a(gb);
		wr_hl(gb, A);
		return;
	}
	// .asm_49d13
	ld_l(gb, OBJ_VAR_1);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		wr8(gb, wCurObjState, A);
		return;
	}
	dec_mhl(gb);
	dec_l(gb);
	A = 0x09; wr_hl(gb, A);
	set_update_l(gb, PP_49CCE);
	A = alu_add(gb, rd8(gb, wCurObjUnk01), 0x03);
	gb->cpu.e = A;
	gb->cpu.c = 0x2b;
	A = 0x00;
	gb->cpu.b = 0x00;
	gb_push16(gb, PP(0xab)); // call .Func_49d43
	pp_49d43(gb);
	gb_pop16(gb);
	A = alu_sub(gb, rd8(gb, wCurObjUnk01), 0x03);
	gb->cpu.e = A;
	A = 0x00;
	gb->cpu.b = 0x00;
	pp_49d43(gb);
}
