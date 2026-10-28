// Port of src/engine/level/objects/prince_froggy.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define PRINCE_FROGGY_UPDATE_STUCK (ADDR_PrinceFroggyFunc + 0x30) // .Update_Stuck
#define PRINCE_FROGGY_UPDATE_SITTING (ADDR_PrinceFroggyFunc + 0xe1) // .Update_Sitting
#define PRINCE_FROGGY_FUNC_441DC (ADDR_PrinceFroggyFunc + 0x118) // .Func_441dc

// Sets the update function at OBJ_UPDATE_FUNCTION of hl's object.
static uint16_t set_update(GB *gb, uint16_t hl, uint16_t update)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// Initialised, with its collision box at hl's object.
static uint16_t init_collbox(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x03);
	wr8(gb, hl--, 0xec);
	gb->cpu.a = 0x00;
	wr8(gb, hl--, 0x00);
	return hl;
}

PORT_JUMP(GreyPrinceFroggyFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, PRINCE_FROGGY_UPDATE_SITTING);
	hl = init_collbox(gb, hl);
	SET_HL(gb, LO(hl, OBJ_STATE));
	JUMP_TO(PRINCE_FROGGY_FUNC_441DC);
}

PORT(RedPrinceFroggyFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	SET_HL(gb, set_update(gb, hl, PRINCE_FROGGY_UPDATE_STUCK));
	SET_DE(gb, ADDR_Frameset_68ac6);
	CALL(SetObjectFramesetPtr);
	SET_HL(gb, init_collbox(gb, REG_HL(gb)));
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


#define PF(off) (ADDR_PrinceFroggyFunc + (off))
#define PF_UPDATE_SITTING PF(0xe1)
#define PF_44220 PF(0x15c)

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

// .asm_440da: thrown/attacked, with the update function f
static void pf_hit(GB *gb, uint16_t f)
{
	A = (uint8_t)(f >> 8);
	gb->cpu.c = (uint8_t)f;
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.c);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	SET_DE(gb, ADDR_Frameset_68ae6);
	JUMP(SetObjectFramesetPtr);
}

PORT_JUMP(PrinceFroggyFunc) { pf_hit(gb, ADDR_Func_33f8); }
PORT_LOCAL_JUMP(PrinceFroggyFunc, Func_440ca) { pf_hit(gb, ADDR_Func_3407); }
PORT_LOCAL_JUMP(PrinceFroggyFunc, Func_440d0) { pf_hit(gb, ADDR_Func_3326); }
PORT_LOCAL_JUMP(PrinceFroggyFunc, Func_440d6) { pf_hit(gb, ADDR_Func_3317); }

// .asm_44185: the ground shaking knocks him over
static void pf_44185(GB *gb)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (CF)
		RETURN;
	ld_l(gb, OBJ_STATE);
	A = 0x1b; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_68ae6);
	CALL(SetObjectFramesetPtr);
	A = 13; wr_hli(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_0B);
	wr_hli(gb, A);
	RETURN;
}

// .asm_44151
static void pf_44151(GB *gb)
{
	A = 0x41; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	RETURN;
}

// .Func_441dc (hl = OBJ_STATE)
static void pf_441dc(GB *gb)
{
	A = 0x43;
	wr_hl(gb, A);
	SET_DE(gb, ADDR_Frameset_68b01);
	CALL(SetObjectFramesetPtr);
	A = 73; wr_hli(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_PRINCE_FROGGY);
	wr_hli(gb, A);
	A = 0xe6; wr_hl(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(PrinceFroggyFunc, Func_441dc) { pf_441dc(gb); }

// .asm_4420a
static void pf_4420a(GB *gb)
{
	set_update_l(gb, PF_44220);
	SET_DE(gb, ADDR_Frameset_68ae6);
	CALL(SetObjectFramesetPtr);
	A = 16; wr_hli(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjVar3, A);
	RETURN;
}

// the states both updates handle alike; returns false if a isn't one
static bool pf_common_state(GB *gb)
{
	static const struct { uint8_t state; uint16_t f; } hits[] = {
		{0x28, ADDR_Func_33f8}, {0x29, ADDR_Func_3407}, {0x04, ADDR_Func_3326}, {0x05, ADDR_Func_3317},
	};
	for (int i = 0; i < 4; i++) {
		alu_cp(gb, A, hits[i].state);
		if (ZF) {
			pf_hit(gb, hits[i].f);
			return true;
		}
	}
	return false;
}

PORT_LOCAL_JUMP(PrinceFroggyFunc, Update_Stuck)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		ld_l(gb, OBJ_FLAGS);
		if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
			pf_44185(gb);
			return;
		}
		res_mhl(gb, OBJFLAG_STEPPED_F);
		A = 0x5a;
		wr8(gb, wCurObjState, A);
		SET_DE(gb, ADDR_Frameset_68add);
		CALL(SetObjectFramesetPtr);
		A = 111; wr_hli(gb, A);
		RETURN;
	}
	alu_cp(gb, A, 0x1b);
	if (ZF) {
		JUMP(Func_3272);
		return;
	}
	alu_cp(gb, A, 0x41);
	if (ZF) {
		JUMP(ObjState_StandingFall);
		return;
	}
	alu_cp(gb, A, 0x13);
	if (ZF) {
		set_update_l(gb, PF_UPDATE_SITTING);
		ld_l(gb, OBJ_STATE);
		pf_441dc(gb);
		return;
	}
	alu_cp(gb, A, 0x11);
	if (ZF) {
		pf_44151(gb);
		return;
	}
	alu_cp(gb, A, 0x5a);
	if (ZF) {
		ld_l(gb, OBJ_FLAGS);
		if (alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
			res_mhl(gb, OBJFLAG_STEPPED_F);
			RETURN;
		}
		goto asm_44166;
	}
	alu_cp(gb, A, 0x3b);
	if (ZF) {
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb)) {
			pf_44185(gb);
			return;
		}
		goto asm_44166;
	}
	if (pf_common_state(gb))
		return;
	alu_cp(gb, A, 0x0a);
	if (ZF) {
		pf_4420a(gb);
		return;
	}
	// .asm_44172
	A = 0x3b;
	wr8(gb, wCurObjState, A);
	SET_DE(gb, ADDR_Frameset_68af2);
	CALL(SetObjectFramesetPtr);
	A = 40; wr_hli(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
	RETURN;

asm_44166:
	xor_a(gb);
	wr8(gb, wCurObjState, A);
	SET_DE(gb, ADDR_Frameset_68ac6);
	CALL(SetObjectFramesetPtr);
	pf_44185(gb);
}

PORT_LOCAL_JUMP(PrinceFroggyFunc, Update_Sitting)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		pf_44185(gb);
		return;
	}
	alu_cp(gb, A, 0x11);
	if (ZF) {
		pf_44151(gb);
		return;
	}
	alu_cp(gb, A, 0x1b);
	if (ZF) {
		JUMP(Func_3272);
		return;
	}
	alu_cp(gb, A, 0x41);
	if (ZF) {
		JUMP(ObjState_StandingFall);
		return;
	}
	if (pf_common_state(gb))
		return;
	alu_cp(gb, A, 0x13);
	if (ZF) {
		pf_441dc(gb);
		return;
	}
	alu_cp(gb, A, 0x43);
	if (ZF) {
		// .asm_441f8
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb)) {
			pf_44185(gb);
			return;
		}
		xor_a(gb);
		wr8(gb, wCurObjState, A);
		SET_DE(gb, ADDR_Frameset_68ae9);
		CALL(SetObjectFramesetPtr);
		pf_44185(gb);
		return;
	}
	alu_cp(gb, A, 0x0a);
	if (ZF) {
		pf_4420a(gb);
		return;
	}
	xor_a(gb);
	wr_hl(gb, A);
	pf_441dc(gb);
}

PORT_LOCAL_JUMP(PrinceFroggyFunc, Func_44220)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	SET_BC(gb, ADDR_Data_60040);
	CALL(ApplyObjYMovement);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	A = 0x08;
	wr8(gb, wCurObjVar2, A);
	CALL(MoveObjectLeftByVar2);
	JUMP(VanishObject2);
}
