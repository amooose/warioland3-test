// Port of src/engine/level/objects/spear_bot.asm
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


// Only the entry, the update dispatch and the states shared with the other
// enemies are ported; the rest of SpearBotFunc is still asm.
#define SB(off) (ADDR_SpearBotFunc + (off))
#define SB_INIT SB(0x0a)
#define SB_UPDATE SB(0x1e)
#define SB_45FBF SB(0xdc)

static void set_state(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
}

static void set_state_hld(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
}

static void collbox_5_fa(GB *gb)
{
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x05; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
}

static void interaction_hld(GB *gb, uint8_t x)
{
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, x);
	wr_hld(gb, A);
}

PORT(SpearBotFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(SB_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)SB_INIT; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(SpearBotFunc, Init)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(SB_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)SB_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	JUMP_TO(SB_45FBF);
}

PORT_LOCAL_JUMP(SpearBotFunc, Update)
{
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, SB_UPDATE + 4));
}

static void sb_463c3(GB *gb)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		set_state(gb, 0x1c);
		RETURN;
	}
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_32ea);
		return;
	}
	set_state(gb, 0x10);
	RETURN;
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_463b4)
{
	set_state(gb, 0x3f);
	A = 0x64;
	wr8(gb, wCurObjStateDuration, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	sb_463c3(gb);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_463c3) { sb_463c3(gb); }

static void sb_463eb(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_32f9);
		return;
	}
	A = 0x00;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_463dd)
{
	set_state_hld(gb, 0x40);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	A = 0x07;
	wr8(gb, wCurObjStateDuration, A);
	sb_463eb(gb);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_463eb) { sb_463eb(gb); }

PORT_LOCAL(SpearBotFunc, Func_463f8)
{
	set_state_hld(gb, 0x43);
	frameset_by_dir_hld(gb, ADDR_Frameset_68e33, ADDR_Frameset_68e3a);
	CALL(SetObjectFramesetPtr);
	A = 26; wr_hli(gb, A);
	collbox_5_fa(gb);
	interaction_hld(gb, OBJ_INTERACTION_01);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_46421)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		A = alu_rra(gb, rd8(gb, rDIV));
		A = alu_rla(gb, 0x04);
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x09);
	if (!ZF) {
		if (dec_mhl(gb)) {
			JUMP(Func_3308);
			return;
		}
		set_state(gb, 0x01);
		RETURN;
	}
	dec_mhl(gb);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_rlca(gb, rd_hl(gb));
	if (CF)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

static void sb_stunned(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjAction);
	A = 0x8f; wr_hld(gb, A);
	A = state; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_68e17, ADDR_Frameset_68e20);
	CALL(SetObjectFramesetPtr);
	A = 4; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	collbox_5_fa(gb);
	interaction_hld(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(SpearBotFunc, Func_46451) { sb_stunned(gb, 0x38); }
PORT_LOCAL(SpearBotFunc, Func_4645b) { sb_stunned(gb, 0x39); }

static void sb_46499(GB *gb, uint8_t state)
{
	set_state(gb, state);
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
}

PORT_LOCAL(SpearBotFunc, Func_4648d) { sb_46499(gb, 0x3b); }
PORT_LOCAL(SpearBotFunc, Func_46494) { sb_46499(gb, 0x3c); }

static void sb_464a5(GB *gb, bool left)
{
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	ld_l(gb, OBJ_VAR_2);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (left)
			JUMP(Func_32ae);
		else
			JUMP(Func_32bd);
		return;
	}
	SET_DE(gb, left ? ADDR_Frameset_68e08 : ADDR_Frameset_68dff);
	CALL(SetObjectFramesetPtr);
	A = 38; wr_hli(gb, A);
	set_state(gb, 0x5b);
	RETURN;
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_464a5) { sb_464a5(gb, false); }
PORT_LOCAL_JUMP(SpearBotFunc, Func_464b7) { sb_464a5(gb, true); }

static void sb_attacked(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	gb->cpu.b = 0x02;
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	A = alu_and(gb, rd_hl(gb), 0xf0);
	wr_hld(gb, A);
	gb->cpu.c = A;
	xor_a(gb);
	wr_hld(gb, A);
	A = gb->cpu.b;
	wr_hld(gb, A);
	A = alu_rlca(gb, gb->cpu.c);
	SET_DE(gb, CF ? ADDR_Frameset_68e2e : ADDR_Frameset_68e29);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_464d3) { sb_attacked(gb, 0x59); }
PORT_LOCAL_JUMP(SpearBotFunc, Func_464dd) { sb_attacked(gb, 0x58); }
PORT_LOCAL_JUMP(SpearBotFunc, Func_464e7) { sb_attacked(gb, 0x34); }
PORT_LOCAL_JUMP(SpearBotFunc, Func_464f1) { sb_attacked(gb, 0x35); }

static void sb_slide(GB *gb, uint8_t state, uint8_t var2)
{
	set_state_hld(gb, state);
	dec_l(gb);
	A = 0x41; wr_hld(gb, A);
	A = var2; wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_46517)
{
	sb_slide(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_4652a)
{
	sb_slide(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

static void sb_grabbed(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_68e17, ADDR_Frameset_68e20);
	CALL(SetObjectFramesetPtr);
	collbox_5_fa(gb);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_GRABBED_F);
}

static void sb_flip(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
}

static void sb_4658f(GB *gb);

static void sb_4655e(GB *gb)
{
	A = rd8(gb, wDirection);
	if (alu_and(gb, A, A)) {
		JUMP(Func_3353);
		return;
	}
	sb_flip(gb);
	sb_grabbed(gb, 0x48);
	sb_4658f(gb);
}

static void sb_4658f(GB *gb)
{
	A = rd8(gb, wDirection);
	if (!alu_and(gb, A, A)) {
		JUMP(Func_3362);
		return;
	}
	sb_flip(gb);
	sb_grabbed(gb, 0x49);
	sb_4655e(gb);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_4653d)
{
	sb_grabbed(gb, 0x49);
	sb_4655e(gb);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_4655e) { sb_4655e(gb); }

PORT_LOCAL_JUMP(SpearBotFunc, Func_4656e)
{
	sb_grabbed(gb, 0x48);
	sb_4658f(gb);
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_4658f) { sb_4658f(gb); }

PORT_LOCAL(SpearBotFunc, Func_465a0)
{
	ld_hl(gb, wCurObjAction);
	A = 0x9f; wr_hld(gb, A);
	A = 0x42; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	interaction_hld(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(SpearBotFunc, Func_45fbf)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	collbox_5_fa(gb);
	ld_l(gb, OBJ_STATE);
	A = 0x30; wr_hld(gb, A);
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
	SET_DE(gb, c ? ADDR_Frameset_68de8 : ADDR_Frameset_68ddf);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(SpearBotFunc, Func_45ffb)
{
	set_state_hld(gb, 0x41);
	A = alu_rlca(gb, rd_hld(gb));
	bool right = CF;
	xor_a(gb);
	wr_hld(gb, A);
	if (!right) {
		ld_l(gb, OBJ_COLLBOX_LEFT);
		A = rd_hl(gb);
		alu_cp(gb, A, 0xf6);
		if (ZF) {
			A = 0xfa;
			wr_hl(gb, A);
			SET_DE(gb, ADDR_Frameset_68e69);
		} else {
			SET_DE(gb, ADDR_Frameset_68ddf);
		}
	} else {
		ld_l(gb, OBJ_COLLBOX_RIGHT);
		A = rd_hl(gb);
		alu_cp(gb, A, 0x09);
		if (ZF) {
			A = 0x05;
			wr_hl(gb, A);
			SET_DE(gb, ADDR_Frameset_68e70);
		} else {
			SET_DE(gb, ADDR_Frameset_68de8);
		}
	}
	bool turning = ZF;
	CALL(SetObjectFramesetPtr);
	if (turning) {
		A = 14; wr_hli(gb, A);
	} else {
		xor_a(gb);
		wr_hl(gb, A);
	}
}

PORT_LOCAL_JUMP(SpearBotFunc, Func_46043)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		JUMP(ObjState_StandingFall);
		return;
	}
	if (dec_mhl(gb)) {
		RETURN;
	}
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, CF ? ADDR_Frameset_68de8 : ADDR_Frameset_68ddf);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL(SpearBotFunc, Func_4605f)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		A = alu_rra(gb, rd8(gb, rDIV));
		A = alu_rla(gb, 0x04);
		wr8(gb, wCurObjState, A);
		return;
	}
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	ld_l(gb, OBJ_STATE);
	A = 0x2e; wr_hld(gb, A);
	A = alu_rlca(gb, rd_hld(gb));
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	if (!CF) {
		A = 0x05; wr_hld(gb, A);
		A = 0xf6; wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_68e57);
	} else {
		A = 0x09; wr_hld(gb, A);
		A = 0xfa; wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_68e60);
	}
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	A = 0x02;
	wr_hl(gb, A);
	interaction_hld(gb, OBJ_INTERACTION_FRONT_STING);
}
