// Port of src/engine/level/objects/teruteru.asm
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
static uint16_t jumptable(GB *gb, uint16_t table)
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

#define TT(off) (ADDR_TeruteruFunc + (off))
#define TT_INIT TT(0x0a)
#define TT_UPDATE TT(0x2c)

// ld l, OBJ_INTERACTION_TYPE / interaction, keeping HEAVY_OBJ
static void tt_interaction(GB *gb, uint8_t x)
{
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, x);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a
static void set_state_hld(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
}

static void set_state(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
}

PORT(TeruteruFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(TT_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)TT_INIT; wr_hld(gb, A);
}

static void tt_44a97(GB *gb)
{
	set_state_hld(gb, 0x31);
	frameset_by_dir_hld(gb, ADDR_Frameset_68b64, ADDR_Frameset_68b6d);
	xor_a(gb);
	wr_hld(gb, A);
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	A = 0x32; wr_hl(gb, A);
	tt_interaction(gb, OBJ_INTERACTION_01);
	wr_hli(gb, A);
	A = 0xec; wr_hli(gb, A);
	A = 0xf1; wr_hl(gb, A);
}

static void tt_44a72(GB *gb)
{
	ld_hl(gb, wCurObjCollBoxRight);
	A = 0x06; wr_hld(gb, A);
	A = 0xf9; wr_hld(gb, A);
	A = 0xf1; wr_hld(gb, A);
	A = 0xec; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF)
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	tt_44a97(gb);
}

PORT_LOCAL(TeruteruFunc, Init)
{
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_BLIND);
	if (!ZF) {
		ld_hl(gb, wCurObjFlags);
		res_mhl(gb, OBJFLAG_INVISIBLE_F);
	}
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(TT_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)TT_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = alu_and(gb, rd_hl(gb), 0xf0);
	A = alu_or(gb, A, 0x08);
	wr_hl(gb, A);
	tt_44a72(gb);
}

PORT_LOCAL_JUMP(TeruteruFunc, Update)
{
	ld_hl(gb, wCurObjFlags);
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_BLIND);
	if (ZF)
		set_mhl(gb, OBJFLAG_INVISIBLE_F);
	else
		res_mhl(gb, OBJFLAG_INVISIBLE_F);
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, TT_UPDATE + 0x14));
}

PORT_LOCAL(TeruteruFunc, Func_44a72) { tt_44a72(gb); }
PORT_LOCAL(TeruteruFunc, Func_44a97) { tt_44a97(gb); }

PORT_LOCAL(TeruteruFunc, Func_449d1) { set_state(gb, 0x5b); }

static void tt_stunned(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjAction);
	A = 0x8f; wr_hld(gb, A);
	A = state;
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_68ba5);
	CALL(SetObjectFramesetPtr);
	A = 4; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	tt_interaction(gb, OBJ_INTERACTION_00);
	wr_hli(gb, A);
	A = 0xf9; wr_hli(gb, A);
	A = 0xff; wr_hl(gb, A);
}

PORT_LOCAL(TeruteruFunc, Func_44c3f) { tt_stunned(gb, 0x38); }
PORT_LOCAL(TeruteruFunc, Func_44c49) { tt_stunned(gb, 0x39); }

PORT_LOCAL(TeruteruFunc, Func_449d7)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_BLIND);
	if (!ZF) {
		A = rd8(gb, wDirection);
		tt_stunned(gb, alu_and(gb, A, A) ? 0x39 : 0x38);
		return;
	}
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_Y_POS);
	A = alu_sub(gb, rd8(gb, wWarioYPos + 1), 0x0c);
	wr_hli(gb, A);
	A = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0x00);
	wr_hli(gb, A);
	A = rd8(gb, wWarioXPos + 1);
	wr_hli(gb, A);
	A = rd8(gb, wWarioXPos + 0);
	wr_hli(gb, A);
}

PORT_LOCAL(TeruteruFunc, Func_44a09)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	set_state(gb, 0x2e);
	SET_DE(gb, ADDR_Frameset_68b8b);
	CALL(SetObjectFramesetPtr);
	A = 40; wr_hli(gb, A);
	tt_interaction(gb, OBJ_INTERACTION_TERUTERU);
	wr_hli(gb, A);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44a25)
{
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		set_state(gb, 0x2f);
		SET_DE(gb, ADDR_Frameset_68bae);
		CALL(SetObjectFramesetPtr);
		A = 80; wr_hli(gb, A);
		tt_interaction(gb, OBJ_INTERACTION_01);
		wr_hli(gb, A);
		RETURN;
	}
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (!ZF) {
		if (CF)
			CALL(MoveObjectRight);
		else
			CALL(MoveObjectLeft);
	}
	JUMP(MoveObjectDown_Fast);
}

// .asm_44b4c
static void tt_44b4c(GB *gb)
{
	ld_l(gb, OBJ_STATE);
	A = 0x31; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_68b64, ADDR_Frameset_68b6d);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44a5d)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x3c);
	if (ZF) {
		SET_DE(gb, ADDR_Frameset_68b90);
		CALL(SetObjectFramesetPtr);
	}
	if (dec_mhl(gb)) {
		JUMP(MoveObjectUp);
		return;
	}
	tt_44b4c(gb);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44ac1)
{
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_BLIND);
	bool move = ZF;
	if (!move) {
		gb->cpu.c = 0x2a;
		A = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
		gb->cpu.b = A;
		A = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
		A = alu_sub(gb, A, gb->cpu.b);
		alu_cp(gb, A, 0x18);
		if (CF) {
			move = true;
		} else {
			alu_cp(gb, A, 0x80);
			if (!CF)
				move = true;
		}
		if (!move) {
			A = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
			gb->cpu.b = A;
			A = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
			A = alu_sub(gb, A, gb->cpu.b);
			alu_cp(gb, A, 0xe8);
			if (CF) {
				alu_cp(gb, A, 0x18);
				if (!CF)
					move = true;
			}
		}
		if (!move) {
			// .asm_44aee
			set_state(gb, 0x2d);
			SET_DE(gb, ADDR_Frameset_68b84);
			CALL(SetObjectFramesetPtr);
			A = 20; wr_hli(gb, A);
			RETURN;
		}
	}
	// .asm_44afd
	SET_BC(gb, ADDR_Data_604c0);
	CALL(ApplyObjYMovement_Loop);
	ld_hl(gb, wCurObjVar1);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		dec_mhl(gb);
		A = rd_hl(gb);
		if (alu_and(gb, A, A)) {
			JUMP(MoveObjectLeft);
			return;
		}
		SET_DE(gb, ADDR_Frameset_68b76);
	} else {
		inc_mhl(gb);
		A = rd_hl(gb);
		alu_cp(gb, A, 0x64);
		if (!ZF) {
			JUMP(MoveObjectRight);
			return;
		}
		SET_DE(gb, ADDR_Frameset_68b7d);
	}
	CALL(SetObjectFramesetPtr);
	A = 26; wr_hli(gb, A);
	set_state(gb, 0x5a);
	RETURN;
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44b2d)
{
	SET_BC(gb, ADDR_Data_604c0);
	CALL(ApplyObjYMovement_Loop);
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x09);
	if (!ZF) {
		if (!dec_mhl(gb)) {
			tt_44b4c(gb);
			return;
		}
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

static void tt_var2_move(GB *gb, bool left)
{
	ld_hl(gb, wCurObjVar2);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (left)
			JUMP(Func_32ae);
		else
			JUMP(Func_32bd);
		return;
	}
	set_state(gb, 0x0f);
	RETURN;
}

static void tt_hit(GB *gb, uint8_t state, bool left)
{
	set_state_hld(gb, state);
	CALL(DecrementRemainingBossHits);
	A = alu_and(gb, rd_hl(gb), 0x0f);
	A = alu_dec(gb, A);
	if (ZF) {
		JUMP(VanishObject2);
		return;
	}
	SET_DE(gb, ADDR_Frameset_68ba5);
	CALL(SetObjectFramesetPtr);
	tt_interaction(gb, OBJ_INTERACTION_00);
	wr_hld(gb, A);
	tt_var2_move(gb, left);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44b61) { tt_hit(gb, 0x46, true); }
PORT_LOCAL_JUMP(TeruteruFunc, Func_44b7f) { tt_var2_move(gb, true); }
PORT_LOCAL_JUMP(TeruteruFunc, Func_44b8d) { tt_hit(gb, 0x47, false); }
PORT_LOCAL_JUMP(TeruteruFunc, Func_44bab) { tt_var2_move(gb, false); }

static void tt_44bc5(GB *gb, uint8_t state)
{
	set_state(gb, state);
	SET_DE(gb, ADDR_Frameset_68ba5);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_VAR_2);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	tt_interaction(gb, OBJ_INTERACTION_00);
	wr_hli(gb, A);
	A = 0xf9; wr_hli(gb, A);
	A = 0xff; wr_hl(gb, A);
}

PORT_LOCAL(TeruteruFunc, Func_44bb9) { tt_44bc5(gb, 0x44); }
PORT_LOCAL(TeruteruFunc, Func_44bc0) { tt_44bc5(gb, 0x45); }

static void tt_44bf8(GB *gb)
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

PORT_LOCAL_JUMP(TeruteruFunc, Func_44be1)
{
	set_state(gb, 0x3f);
	A = 0x64;
	wr8(gb, wCurObjStateDuration, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	A = alu_or(gb, A, 0x08);
	wr_hl(gb, A);
	tt_44bf8(gb);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44bf8) { tt_44bf8(gb); }

static void tt_44c32(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectUp);
		return;
	}
	A = 0x00;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44c12)
{
	set_state_hld(gb, 0x40);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_68b99);
	CALL(SetObjectFramesetPtr);
	A = 80; wr_hli(gb, A);
	tt_interaction(gb, OBJ_INTERACTION_01);
	wr_hli(gb, A);
	A = 0xec; wr_hli(gb, A);
	A = 0xf1; wr_hl(gb, A);
	tt_44c32(gb);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44c32) { tt_44c32(gb); }

static void tt_44c96(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	gb->cpu.b = 0x02;
	A = alu_and(gb, rd_hl(gb), 0xf0);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hld(gb, A);
	A = gb->cpu.b;
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_68ba2);
	CALL(SetObjectFramesetPtr);
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

PORT_LOCAL(TeruteruFunc, Func_44c70) { tt_44c96(gb, 0x59); }
PORT_LOCAL(TeruteruFunc, Func_44c7a) { tt_44c96(gb, 0x58); }
PORT_LOCAL(TeruteruFunc, Func_44c84) { tt_44c96(gb, 0x34); }
PORT_LOCAL(TeruteruFunc, Func_44c8e) { tt_44c96(gb, 0x35); }

static void tt_slide(GB *gb, uint8_t state, uint8_t var2)
{
	set_state_hld(gb, state);
	dec_l(gb);
	A = 0x41; wr_hld(gb, A);
	A = var2; wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44caa)
{
	tt_slide(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44cbd)
{
	tt_slide(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

static void tt_grabbed(GB *gb, uint8_t state)
{
	set_state(gb, state);
	SET_DE(gb, ADDR_Frameset_68ba5);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_COLLBOX_TOP);
	A = 0xf9; wr_hli(gb, A);
	A = 0xff; wr_hl(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_GRABBED_F);
}

static void tt_flip(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
}

static void tt_44d16(GB *gb);

static void tt_44cef(GB *gb)
{
	A = rd8(gb, wDirection);
	if (alu_and(gb, A, A)) {
		JUMP(Func_3353);
		return;
	}
	tt_flip(gb);
	tt_grabbed(gb, 0x48);
	tt_44d16(gb);
}

static void tt_44cd0(GB *gb)
{
	tt_grabbed(gb, 0x49);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	A = alu_or(gb, A, 0x08);
	wr_hl(gb, A);
	tt_44cef(gb);
}

static void tt_44d16(GB *gb)
{
	A = rd8(gb, wDirection);
	if (!alu_and(gb, A, A)) {
		JUMP(Func_3362);
		return;
	}
	tt_flip(gb);
	tt_44cd0(gb);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44cd0) { tt_44cd0(gb); }
PORT_LOCAL_JUMP(TeruteruFunc, Func_44cef) { tt_44cef(gb); }

PORT_LOCAL_JUMP(TeruteruFunc, Func_44cff)
{
	tt_grabbed(gb, 0x48);
	tt_44d16(gb);
}

PORT_LOCAL_JUMP(TeruteruFunc, Func_44d16) { tt_44d16(gb); }

PORT_LOCAL(TeruteruFunc, Func_44d27)
{
	ld_hl(gb, wCurObjAction);
	A = 0x9f; wr_hld(gb, A);
	A = 0x42; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	tt_interaction(gb, OBJ_INTERACTION_00);
	wr_hld(gb, A);
}
