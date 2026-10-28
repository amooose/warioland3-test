// Port of src/engine/level/objects/robo_mouse.asm
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


#define RM(off) (ADDR_RoboMouseFunc + (off))
#define RM_488F7 RM(0x1b)
#define RM_4892A RM(0x4e)

static void set_state(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
}

PORT(RoboMouseFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(RM_488F7 >> 8); wr_hld(gb, A);
	A = (uint8_t)RM_488F7; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x0b; wr_hld(gb, A);
	A = 0xf4; wr_hld(gb, A);
	A = 0xfc; wr_hl(gb, A);
}

PORT_LOCAL_JUMP(RoboMouseFunc, Func_488f7)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(RM_4892A >> 8); wr_hld(gb, A);
	A = (uint8_t)RM_4892A; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	bool c = CF;
	if (c)
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, c ? ADDR_Frameset_691de : ADDR_Frameset_69220);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(RoboMouseFunc, Func_4892a)
{
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, RM_4892A + 4));
}

PORT_LOCAL(RoboMouseFunc, Func_489e8)
{
	ld_hl(gb, wCurObjState);
	A = 0x5a; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_69200, ADDR_Frameset_691fb);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(RoboMouseFunc, Func_489fe) {}

// .Func_48cec / .asm_48ce8 (left) and .asm_48d3d / .asm_48d39 (right):
// breaks the blocks in front, at three heights; called with the return
// address pushed
static void rm_break(GB *gb, bool right, uint8_t dx)
{
	gb->cpu.b = dx;
	ld_hl(gb, wCurObjXPos);
	if (right) {
		A = alu_add(gb, rd_hli(gb), gb->cpu.b);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0x00);
	} else {
		A = alu_sub(gb, rd_hli(gb), gb->cpu.b);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
	}
	wr8(gb, hXPosHi, A);
	static const uint8_t dys[] = {0x04, 0x14, 0x24};
	for (int i = 0; i < 3; i++) {
		ld_hl(gb, wCurObjYPos);
		A = alu_sub(gb, rd_hli(gb), dys[i]);
		wr8(gb, hYPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
		wr8(gb, hYPosHi, A);
		A = COLLISION_ROBO_MOUSE_BREAK;
		wr8(gb, wBlockFuncSpecialCollision, A);
		if (i < 2) {
			gb_call_ret(gb, ADDR_Func_3513, RM((right ? 0x484 : 0x433) + i * 0x15));
		} else {
			// jp Func_3513: it returns to this routine's caller
			uint16_t ret = gb_pop16(gb);
			gb_call_ret(gb, ADDR_Func_3513, ret);
			gb_push16(gb, ret);
		}
	}
}

PORT_LOCAL_JUMP(RoboMouseFunc, Func_48cec)
{
	rm_break(gb, false, 0x0e);
	RET();
}

// the wrappers: break blocks, then jp to the shared state code
static void rm_wrapper(GB *gb, uint16_t off, bool right, uint16_t target)
{
	gb_push16(gb, RM(off + 3)); // call .Func_48cec / .asm_48d3d
	rm_break(gb, right, right ? 0x0d : 0x0e);
	gb_pop16(gb);
	JUMP_TO(target);
}

PORT_LOCAL_JUMP(RoboMouseFunc, Func_489ff) { rm_wrapper(gb, 0x123, false, ADDR_Func_3209); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a05) { rm_wrapper(gb, 0x129, false, ADDR_Func_3218); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a0b) { rm_wrapper(gb, 0x12f, true, ADDR_Func_3227); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a11) { rm_wrapper(gb, 0x135, true, ADDR_Func_3236); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a17) { rm_wrapper(gb, 0x13b, false, ADDR_Func_31cd); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a1d) { rm_wrapper(gb, 0x141, false, ADDR_Func_31dc); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a23) { rm_wrapper(gb, 0x147, true, ADDR_Func_31eb); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a29) { rm_wrapper(gb, 0x14d, true, ADDR_Func_31fa); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a2f) { rm_wrapper(gb, 0x153, false, ADDR_Func_3245); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a35) { rm_wrapper(gb, 0x159, false, ADDR_Func_32cc); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a3b) { rm_wrapper(gb, 0x15f, true, ADDR_Func_3254); }
PORT_LOCAL_JUMP(RoboMouseFunc, Func_48a41) { rm_wrapper(gb, 0x165, true, ADDR_Func_32db); }

PORT_LOCAL(RoboMouseFunc, Func_48ae9)
{
	CALL(MoveObjectDown_Slow);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, RM(0x222));
	if (!alu_and(gb, A, A))
		return;
	A = alu_and(gb, rd8(gb, wCurObjInteractionType), 0x7f);
	alu_cp(gb, A, 0x0b);
	set_state(gb, ZF ? 0x13 : 0x31);
}

PORT_LOCAL(RoboMouseFunc, Func_48b13)
{
	ld_hl(gb, wCurObjState);
	A = 0x30; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
}

PORT_LOCAL(RoboMouseFunc, Func_48b21)
{
	ld_hl(gb, wCurObjState);
	A = 0x41; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
}

// ld l, OBJ_INTERACTION_TYPE / ... OBJ_INTERACTION_ROBO_MOUSE / ld [hld], a
static void rm_48c9c(GB *gb)
{
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_ROBO_MOUSE);
	wr_hld(gb, A);
}

static void rm_48c60(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_3308);
		return;
	}
	ld_l(gb, OBJ_X_POS);
	A = rd_hli(gb);
	gb->cpu.e = A;
	gb->cpu.d = rd_hl(gb);
	ld_l(gb, OBJ_SUBSTATE);
	A = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, A, gb->cpu.d);
	bool left = CF;
	if (!left && ZF) {
		A = rd8(gb, wWarioXPos + 1);
		alu_cp(gb, A, gb->cpu.e);
		left = CF;
	}
	A = alu_rlca(gb, rd_hli(gb));
	bool turn = left ? CF : !CF;
	if (turn) {
		if (!left) {
			A = 0x2d; wr_hld(gb, A);
			set_mhl(gb, OBJSUBFLAG_HDIR_F);
			SET_DE(gb, ADDR_Frameset_69227);
		} else {
			A = 0x2e; wr_hld(gb, A);
			res_mhl(gb, OBJSUBFLAG_HDIR_F);
			SET_DE(gb, ADDR_Frameset_691f2);
		}
		CALL(SetObjectFramesetPtr);
		A = 0x31; wr_hli(gb, A);
		rm_48c9c(gb);
		RET();
		return;
	}
	// .asm_48ca5
	A = 0x31; wr_hld(gb, A);
	A = alu_and(gb, rd8(gb, wCurObjInteractionType), 0x7f);
	alu_cp(gb, A, 0x0b);
	if (ZF) {
		frameset_by_dir_hld(gb, ADDR_Frameset_69220, ADDR_Frameset_691de);
		CALL(SetObjectFramesetPtr);
		ld_l(gb, OBJ_FLAGS);
		set_mhl(gb, OBJFLAG_PRIORITY_F);
	}
	rm_48c9c(gb);
	RET();
}

PORT_LOCAL_JUMP(RoboMouseFunc, Func_48c4d)
{
	set_state(gb, 0x43);
	A = 0x01;
	ld_hl(gb, wCurObjFlags);
	if (alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F))
		A = 0x0a;
	wr8(gb, wCurObjStateDuration, A);
	rm_48c60(gb);
}

PORT_LOCAL_JUMP(RoboMouseFunc, Func_48c60) { rm_48c60(gb); }

static void rm_wake(GB *gb, uint16_t frameset)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	set_state(gb, 0x31);
}

PORT_LOCAL(RoboMouseFunc, Func_48cc6) { rm_wake(gb, ADDR_Frameset_691de); }
PORT_LOCAL(RoboMouseFunc, Func_48cd7) { rm_wake(gb, ADDR_Frameset_69220); }
