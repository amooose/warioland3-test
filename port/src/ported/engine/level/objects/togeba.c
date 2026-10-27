// Port of src/engine/level/objects/togeba.asm
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

#define TG(off) (ADDR_TogebaFunc + (off))
#define TG_48DB7 TG(0x2d)
#define TG_48E02 TG(0x78)
#define TG_48E3E TG(0xb4)
#define TG_48E46 TG(0xbc)
#define TG_48ECC TG(0x142)

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(f) / ld [hld], a / ld a, LOW(f) /
// ld [hld], a
static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void interaction_hld(GB *gb, uint8_t x)
{
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, x);
	wr_hld(gb, A);
}

// .Func_48d9b
static void tg_48d9b(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_69230);
	CALL(SetObjectFramesetPtr);
	xor_a(gb);
	wr_hl(gb, A);
	wr8(gb, wCurObjState, A);
	set_update_l(gb, TG_48DB7);
	interaction_hld(gb, OBJ_INTERACTION_FULL_STING);
}

PORT(TogebaFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x0b; wr_hld(gb, A);
	A = 0xf4; wr_hld(gb, A);
	tg_48d9b(gb);
}

PORT_LOCAL(TogebaFunc, Func_48d9b) { tg_48d9b(gb); }

PORT_LOCAL(TogebaFunc, Func_48db7)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		ld_hl(gb, wCurObjUpdateFunction + 1);
		A = (uint8_t)(TG_48E02 >> 8); wr_hld(gb, A);
		A = (uint8_t)TG_48E02; wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_69256);
		CALL(SetObjectFramesetPtr);
		A = 0x30; wr_hli(gb, A);
		interaction_hld(gb, OBJ_INTERACTION_0B);
		xor_a(gb);
		wr8(gb, wCurObjVar3, A);
		return;
	}
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			return;
		SET_DE(gb, ADDR_Frameset_69230);
		CALL(SetObjectFramesetPtr);
		return;
	}
	ld_l(gb, OBJ_STATE);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A))
		return;
	xor_a(gb);
	wr_hl(gb, A);
	SET_DE(gb, ADDR_Frameset_6924b);
	CALL(SetObjectFramesetPtr);
	A = 0x16; wr_hli(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
}

PORT_LOCAL(TogebaFunc, Func_48e02)
{
	SET_BC(gb, ADDR_Data_60760);
	CALL(ApplyObjYMovement);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	set_update_l(gb, TG_48E46);
	SET_DE(gb, ADDR_Frameset_69263);
	CALL(SetObjectFramesetPtr);
	xor_a(gb);
	wr8(gb, wCurObjVar3, A);
	wr8(gb, wCurObjState, A);
	interaction_hld(gb, OBJ_INTERACTION_WALKABLE);
}

static void tg_48e2b(GB *gb)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(TG_48E3E >> 8); wr_hld(gb, A);
	A = (uint8_t)TG_48E3E; wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_69275);
	CALL(SetObjectFramesetPtr);
	A = 0x10; wr_hli(gb, A);
}

PORT_LOCAL(TogebaFunc, Func_48e2b) { tg_48e2b(gb); }

PORT_LOCAL(TogebaFunc, Func_48e3e)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	tg_48d9b(gb);
}

PORT_LOCAL_JUMP(TogebaFunc, Func_48e46)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); wr8(gb, hYPosHi, A);
	A = alu_sub(gb, rd_hli(gb), 0x0c);
	wr8(gb, hXPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0x00);
	wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_352b, TG(0xd2));
	if (alu_and(gb, A, A)) {
		tg_48e2b(gb);
		RETURN;
	}
	ld_hl(gb, wCurObjXPos);
	A = alu_add(gb, rd_hli(gb), 0x0b);
	wr8(gb, hXPosLo, A);
	A = alu_adc(gb, rd_hli(gb), 0x00);
	wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_352b, TG(0xe5));
	if (alu_and(gb, A, A)) {
		tg_48e2b(gb);
		RETURN;
	}
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (!dec_mhl(gb)) {
			SET_DE(gb, ADDR_Frameset_69263);
			CALL(SetObjectFramesetPtr);
		}
	}
	// .asm_48e82
	ld_l(gb, OBJ_STATE);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		alu_cp(gb, A, 0x0a);
		if (ZF) {
			xor_a(gb);
			wr_hl(gb, A);
			SET_DE(gb, ADDR_Frameset_6926c);
			CALL(SetObjectFramesetPtr);
			A = 0x1b; wr_hli(gb, A);
		} else {
			xor_a(gb);
			wr_hl(gb, A);
			SET_DE(gb, ADDR_Frameset_6927c);
			CALL(SetObjectFramesetPtr);
			A = 0x0a; wr_hli(gb, A);
			A = alu_rla(gb, rd8(gb, wCurObjUnk1d));
			if (CF) {
				set_update_l(gb, TG_48ECC);
				RETURN;
			}
		}
	}
	// .asm_48eb3
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!CF)
		RETURN;
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectDown);
		return;
	}
	res_mhl(gb, OBJFLAG_STEPPED_F);
	gb->cpu.b = 0x01;
	gb_call_ret(gb, ADDR_MoveWarioDown, TG(0x13c));
	gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, TG(0x13f));
	JUMP(MoveObjectDown);
}

PORT_LOCAL(TogebaFunc, Func_48ecc)
{
	CALL(MoveObjectUp_Fast);
	ld_l(gb, OBJ_STATE_DURATION);
	if (dec_mhl(gb))
		return;
	set_update_l(gb, TG_48E46);
	SET_DE(gb, ADDR_Frameset_69263);
	CALL(SetObjectFramesetPtr);
}
