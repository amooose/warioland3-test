// Port of src/engine/level/objects/tadpole.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

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

// res OBJSUBFLAG_UNINITIALISED_F of hl's object
static inline uint16_t set_initialised(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	return hl;
}

// Sets hl's object's collision box (right, left, bottom), a = bottom.
static inline uint16_t set_collbox(GB *gb, uint16_t hl, uint8_t right, uint8_t left, uint8_t bottom)
{
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, right);
	wr8(gb, hl--, left);
	gb->cpu.a = bottom;
	wr8(gb, hl, bottom);
	return hl;
}

// ld hl, wCurObjFlags / res OBJFLAG_INVISIBLE_F, [hl] / set OBJFLAG_NO_COLLISION_F, [hl]
static inline uint16_t show_no_collision(GB *gb)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	return hl;
}

PORT(TadpoleSpawnerFunc)
{
	uint16_t hl = set_update(gb, wCurObjUpdateFunction + 1, ADDR_TadpoleSpawnerFunc + 0x17); // .Func_49e4b
	hl = set_initialised(gb, hl);
	gb->cpu.a = 180;
	wr8(gb, wCurObjStateDuration, 180);
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
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


#define TP(off) (ADDR_TadpoleFunc + (off))
#define TP_49EB3 TP(0x13)
#define TP_49ECE TP(0x2e)
#define TP_49F34 TP(0x94)
#define TP_49FB5 TP(0x115)

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void tp_action(GB *gb)
{
	A = 0x81;
	wr8(gb, wCurObjAction, A);
}

PORT(TadpoleFunc)
{
	tp_action(gb);
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(TP_49EB3 >> 8); wr_hld(gb, A);
	A = (uint8_t)TP_49EB3; wr_hld(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_INVISIBLE_F);
}

PORT_LOCAL_JUMP(TadpoleFunc, Func_49eb3)
{
	tp_action(gb);
	ld_hl(gb, wCurObjFlags);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F))
		RETURN;
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, TP_49ECE);
	SET_DE(gb, ADDR_Frameset_69a33);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(TadpoleFunc, Func_49ece)
{
	tp_action(gb);
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		SET_DE(gb, ADDR_Frameset_69a3d);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	SET_DE(gb, ADDR_Frameset_69a1a);
	CALL(SetObjectFramesetPtr);
	A = 0x20; wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	set_update_l(gb, TP_49F34);
	play_sfx(gb, SFX_07F);
	RETURN;
}

// .asm_49f1c: a/c = the update function, b = var2
static void tp_49f1c(GB *gb)
{
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.c);
	ld_l(gb, OBJ_VAR_3);
	xor_a(gb);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.b);
	tp_action(gb);
	SET_DE(gb, ADDR_Frameset_69a30);
	JUMP(SetObjectFramesetPtr);
}

static void tp_bump(GB *gb, uint16_t f)
{
	A = (uint8_t)(f >> 8);
	gb->cpu.c = (uint8_t)f;
	gb->cpu.b = 0x02;
	tp_49f1c(gb);
}

PORT_LOCAL_JUMP(TadpoleFunc, Func_49f34)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		alu_cp(gb, A, 0x0b);
		if (ZF) {
			// .asm_49efe
			ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
			A = (uint8_t)(ADDR_Func_3407 >> 8); wr_hld(gb, A);
			A = (uint8_t)ADDR_Func_3407; wr_hl(gb, A);
			ld_l(gb, OBJ_VAR_3);
			xor_a(gb);
			wr_hld(gb, A);
			A = 0x01; wr_hl(gb, A);
			RETURN;
		}
		static const struct { uint8_t state; bool left; } bumps[] = {
			{0x08, false}, {0x09, true}, {0x28, true}, {0x29, false},
		};
		for (int i = 0; i < 4; i++) {
			alu_cp(gb, A, bumps[i].state);
			if (ZF) {
				tp_bump(gb, bumps[i].left ? ADDR_Func_33f8 : ADDR_Func_3407);
				return;
			}
		}
		alu_cp(gb, A, 0x0a);
		if (ZF) {
			JUMP(Func_3173);
			return;
		}
		alu_cp(gb, A, 0x04);
		if (ZF) {
			tp_bump(gb, ADDR_Func_3407);
			return;
		}
		alu_cp(gb, A, 0x05);
		if (ZF) {
			tp_bump(gb, ADDR_Func_33f8);
			return;
		}
		xor_a(gb);
		wr_hl(gb, A);
	}
	// .asm_49f5e
	ld_l(gb, OBJ_VAR_1);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		// .asm_49f97
		ld_l(gb, OBJ_STATE_DURATION);
		A = rd_hl(gb);
		if (alu_and(gb, A, A)) {
			dec_mhl(gb);
			SET_BC(gb, ADDR_Data_60a20);
			CALL(ApplyObjYMovement);
			JUMP(MoveObjectLeft_Fast);
			return;
		}
		xor_a(gb);
		wr8(gb, wCurObjVar3, A);
		A = alu_inc(gb, A);
		wr8(gb, wCurObjVar1, A);
		SET_DE(gb, ADDR_Frameset_69a1d);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, TP(0xd6));
	A = rd8(gb, wIsInWaterOrSand);
	if (!alu_and(gb, A, A)) {
		SET_BC(gb, ADDR_Data_60a40);
		CALL(ApplyObjYMovement);
		JUMP(MoveObjectLeft_Fast);
		return;
	}
	SET_DE(gb, ADDR_Frameset_69a26);
	CALL(SetObjectFramesetPtr);
	A = 0x09; wr_hli(gb, A);
	set_update_l(gb, TP_49FB5);
	RETURN;
}

PORT_LOCAL(TadpoleFunc, Func_49fb5)
{
	tp_action(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}
