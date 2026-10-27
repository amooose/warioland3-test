// Port of src/engine/level/objects/para_goom.asm
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

#define PG(off) (ADDR_ParaGoomFunc + (off))
#define PG_INIT PG(0x0a)
#define PG_UPDATE PG(0x1e)

// ld a, [wGroundShakeCounter] / cp $10 / jr c, .no: with the ground shaking,
// the state becomes 4 or 5 (from DIV) and it returns.
static bool shaken(GB *gb)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (CF)
		return false;
	A = alu_rra(gb, rd8(gb, rDIV));
	A = alu_rla(gb, 0x04);
	wr8(gb, wCurObjState, A);
	RET();
	return true;
}

PORT(ParaGoomFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(PG_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)PG_INIT; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Update)
{
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, PG_UPDATE + 4));
}

// .asm_41505
static void pg_41505(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_684ba);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_TOP_STING);
	wr_hli(gb, A);
	A = 0xef; wr_hl(gb, A);
	RETURN;
}

static void pg_default(GB *gb)
{
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hli(gb, A);
	A = 0xf4; wr_hl(gb, A);
	ld_l(gb, OBJ_STATE);
	A = 0x30; wr_hld(gb, A);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (!CF) {
		res_mhl(gb, OBJFLAG_PRIORITY_F);
		dec_l(gb);
		xor_a(gb);
		wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_68439);
		CALL(SetObjectFramesetPtr);
		ld_l(gb, OBJ_COLLBOX_RIGHT);
		A = 0x06; wr_hld(gb, A);
		A = 0xf6; wr_hld(gb, A);
	} else {
		set_mhl(gb, OBJFLAG_PRIORITY_F);
		dec_l(gb);
		xor_a(gb);
		wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_68442);
		CALL(SetObjectFramesetPtr);
		ld_l(gb, OBJ_COLLBOX_RIGHT);
		A = 0x09; wr_hld(gb, A);
		A = 0xf9; wr_hld(gb, A);
	}
	// .asm_414a0
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, PG(0x15b));
	if (alu_and(gb, A, A))
		RETURN;
	ld_hl(gb, wCurObjState);
	A = 0x41; wr_hld(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjStateDuration, A);
	pg_41505(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Init)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(PG_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)PG_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	pg_default(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Default) { pg_default(gb); }

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41439)
{
	A = 0x5e;
	wr8(gb, wCurObjState, A);
	SET_DE(gb, ADDR_Frameset_684a1);
	CALL(SetObjectFramesetPtr);
	JUMP(ObjState_FullThrowLeft);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41447)
{
	A = 0x5f;
	wr8(gb, wCurObjState, A);
	SET_DE(gb, ADDR_Frameset_684a1);
	CALL(SetObjectFramesetPtr);
	JUMP(ObjState_FullThrowRight);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_414c0)
{
	ld_hl(gb, wCurObjState);
	A = 0x41; wr_hld(gb, A);
	A = rd8(gb, wCurObjCollBoxTop);
	alu_cp(gb, A, 0xef);
	bool top = ZF;
	A = alu_rlca(gb, rd_hld(gb));
	bool c = CF;
	xor_a(gb);
	wr_hld(gb, A);
	if (!top)
		SET_DE(gb, c ? ADDR_Frameset_6850a : ADDR_Frameset_684e7);
	else
		SET_DE(gb, c ? ADDR_Frameset_68546 : ADDR_Frameset_6852d);
	CALL(SetObjectFramesetPtr);
	A = top ? 0x29 : 0x38;
	wr_hld(gb, A);
	RETURN;
}

static void pg_41830(GB *gb);

PORT_LOCAL_JUMP(ParaGoomFunc, Func_414fb)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		pg_41830(gb);
		return;
	}
	if (dec_mhl(gb))
		RETURN;
	pg_41505(gb);
}

// .asm_41545
static void pg_41545(GB *gb)
{
	ld_l(gb, OBJ_STATE);
	A = 0x31; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_6844b, ADDR_Frameset_68454);
	CALL(SetObjectFramesetPtr);
	xor_a(gb);
	wr_hli(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_TOP_STING);
	wr_hli(gb, A);
	A = 0xef; wr_hl(gb, A);
	RETURN;
}

// .asm_41570
static void pg_41570(GB *gb)
{
	ld_l(gb, OBJ_STATE);
	A = 0x31; wr_hld(gb, A);
	A = alu_rlca(gb, rd_hld(gb));
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	if (!CF) {
		A = 0x06; wr_hld(gb, A);
		A = 0xf6; wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_68439);
	} else {
		A = 0x09; wr_hld(gb, A);
		A = 0xf9; wr_hld(gb, A);
		SET_DE(gb, ADDR_Frameset_68442);
	}
	CALL(SetObjectFramesetPtr);
	A = 0x0a; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41517)
{
	if (shaken(gb))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	pg_41570(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_4152f)
{
	if (shaken(gb))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	pg_41545(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41567)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	pg_41570(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41598)
{
	if (shaken(gb))
		return;
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		ld_l(gb, OBJ_INTERACTION_TYPE);
		heavy_or(gb, OBJ_INTERACTION_FRONT_STING);
		wr_hld(gb, A);
		RETURN;
	}
	// .asm_415bb
	inc_l(gb);
	if (!inc_mhl(gb)) {
		A = rd8(gb, wCurObjCollBoxTop);
		alu_cp(gb, A, 0xef);
		if (ZF) {
			ld_l(gb, OBJ_STATE);
			A = 0x2e; wr_hld(gb, A);
			frameset_by_dir_hld(gb, ADDR_Frameset_6846e, ADDR_Frameset_68490);
			CALL(SetObjectFramesetPtr);
			A = 0xa3; wr_hli(gb, A);
			ld_l(gb, OBJ_INTERACTION_TYPE);
			heavy_or(gb, OBJ_INTERACTION_01);
			wr_hli(gb, A);
			A = 0xf4; wr_hl(gb, A);
			RETURN;
		}
		// .asm_415e9
		ld_l(gb, OBJ_STATE);
		A = 0x2d; wr_hld(gb, A);
		frameset_by_dir_hld(gb, ADDR_Frameset_6845d, ADDR_Frameset_6847f);
		CALL(SetObjectFramesetPtr);
		A = 0xa3; wr_hli(gb, A);
		ld_l(gb, OBJ_INTERACTION_TYPE);
		heavy_or(gb, OBJ_INTERACTION_01);
		wr_hld(gb, A);
		ld_l(gb, OBJ_COLLBOX_RIGHT);
		A = 0x05; wr_hld(gb, A);
		A = 0xfa; wr_hld(gb, A);
		RETURN;
	}
	// .asm_41611
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_3543, PG(0x2cc));
	if (!alu_and(gb, A, A)) {
		A = 0x11;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	// .asm_4162c
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hli(gb, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		A = alu_cpl(gb, rd8(gb, wCurObjCollBoxLeft));
		A = alu_inc(gb, A);
		gb->cpu.b = A;
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_sub(gb, A, gb->cpu.b);
		bool blocked = false;
		if (CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, PG(0x2f3));
			A = alu_and(gb, A, 0x0f);
			blocked = ZF;
		}
		if (!blocked) {
			// .asm_4164e
			ld_hl(gb, wCurObjYPos);
			A = alu_sub(gb, rd_hli(gb), 0x04);
			wr8(gb, hYPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hYPosHi, A);
			A = alu_cpl(gb, rd8(gb, wCurObjCollBoxLeft));
			A = alu_inc(gb, A);
			gb->cpu.b = A;
			A = alu_sub(gb, rd_hli(gb), gb->cpu.b);
			wr8(gb, hXPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, PG(0x316));
			if (!alu_and(gb, A, A)) {
				JUMP(MoveObjectLeft_Slow);
				return;
			}
		}
	} else {
		// .asm_41677
		gb->cpu.b = rd8(gb, wCurObjCollBoxRight);
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_add(gb, A, gb->cpu.b);
		alu_cp(gb, A, 0x10);
		bool blocked = false;
		if (!CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnRight, PG(0x32f));
			A = alu_and(gb, A, 0x0f);
			blocked = ZF;
		}
		if (!blocked) {
			// .asm_4168a
			ld_hl(gb, wCurObjYPos);
			A = alu_sub(gb, rd_hli(gb), 0x04);
			wr8(gb, hYPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hYPosHi, A);
			A = rd8(gb, wCurObjCollBoxRight);
			A = alu_add(gb, A, rd_hl(gb));
			wr8(gb, hXPosLo, A);
			inc_l(gb);
			A = alu_adc(gb, rd_hl(gb), 0x00);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, PG(0x34f));
			if (!alu_and(gb, A, A)) {
				JUMP(MoveObjectRight_Slow);
				return;
			}
		}
	}
	// .asm_41671 / .asm_416aa
	A = 0x13;
	wr8(gb, wCurObjState, A);
	RETURN;
}

// .asm_416be
static void pg_416be(GB *gb)
{
	A = rd8(gb, wCurObjCollBoxTop);
	alu_cp(gb, A, 0xef);
	if (!ZF)
		frameset_by_dir_hld(gb, ADDR_Frameset_68439, ADDR_Frameset_68442);
	else
		frameset_by_dir_hld(gb, ADDR_Frameset_6844b, ADDR_Frameset_68454);
	// .asm_416d1
	CALL(SetObjectFramesetPtr);
	A = 0x0c; wr_hld(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x05; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_416b0)
{
	ld_hl(gb, wCurObjState);
	A = 0x32; wr_hld(gb, A);
	pg_416be(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_416b8)
{
	ld_hl(gb, wCurObjState);
	A = 0x33; wr_hld(gb, A);
	pg_416be(gb);
}

// sets the frameset (by direction), collision box and interaction 00 of the
// thrown/stunned states: .Func_416fb / .Func_41731
static void pg_thrown(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_684aa, ADDR_Frameset_684af);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x05; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_00);
	wr_hli(gb, A);
	A = 0xf4; wr_hl(gb, A);
}

// .Func_41723 / .Func_41759 / .Func_4196c / .Func_4197f tail
static void pg_var2_move(GB *gb, bool left, uint8_t state)
{
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (left)
			JUMP(Func_32ae);
		else
			JUMP(Func_32bd);
		return;
	}
	A = state;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_416fb)
{
	pg_thrown(gb, 0x46);
	ld_hl(gb, wCurObjVar2);
	pg_var2_move(gb, true, 0x0f);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41723)
{
	ld_hl(gb, wCurObjVar2);
	pg_var2_move(gb, true, 0x0f);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41731)
{
	pg_thrown(gb, 0x47);
	ld_hl(gb, wCurObjVar2);
	pg_var2_move(gb, false, 0x0f);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41759)
{
	ld_hl(gb, wCurObjVar2);
	pg_var2_move(gb, false, 0x0f);
}

// .asm_41775
static void pg_41775(GB *gb)
{
	frameset_by_dir_hld(gb, ADDR_Frameset_684aa, ADDR_Frameset_684af);
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x05; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_00);
	wr_hli(gb, A);
	A = 0xf4; wr_hl(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41767)
{
	ld_hl(gb, wCurObjState);
	A = 0x44; wr_hld(gb, A);
	pg_41775(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_4176f)
{
	ld_hl(gb, wCurObjState);
	A = 0x45; wr_hld(gb, A);
	pg_41775(gb);
}

static void pg_417b1(GB *gb)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		A = 0x1c;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_32ea);
		return;
	}
	A = 0x10;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_417a2)
{
	A = 0x3f;
	wr8(gb, wCurObjState, A);
	A = 0x64;
	wr8(gb, wCurObjStateDuration, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	pg_417b1(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_417b1) { pg_417b1(gb); }

static void pg_417e8(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		ld_l(gb, OBJ_Y_POS);
		A = alu_sub(gb, rd_hli(gb), 0x08);
		wr8(gb, hYPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
		wr8(gb, hYPosHi, A);
		A = rd_hli(gb); wr8(gb, hXPosLo, A);
		A = rd_hli(gb); wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_358b, PG(0x4ac));
		A = alu_and(gb, A, 0x0f);
		if (ZF) {
			SET_BC(gb, ADDR_Data_60530);
			JUMP(ApplyObjYMovement);
			return;
		}
	}
	// .asm_4180d
	A = 0x5a;
	wr8(gb, wCurObjState, A);
	SET_DE(gb, ADDR_Frameset_684b7);
	CALL(SetObjectFramesetPtr);
	A = 0x04; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_417cb)
{
	ld_hl(gb, wCurObjState);
	A = 0x40; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_684b4);
	CALL(SetObjectFramesetPtr);
	A = 0x08; wr_hli(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_TOP_STING);
	wr_hli(gb, A);
	A = 0xef; wr_hl(gb, A);
	pg_417e8(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_417e8) { pg_417e8(gb); }

PORT_LOCAL_JUMP(ParaGoomFunc, Func_4181c)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	ld_l(gb, OBJ_STATE);
	A = 0x5b; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_684ba);
	CALL(SetObjectFramesetPtr);
	RETURN;
}

static void pg_41830(GB *gb)
{
	CALL(MoveObjectDown_Slow);
	obj_pos_to_hpos(gb);
	xor_a(gb);
	wr8(gb, wObjSlopeInteraction, A);
	gb_call_ret(gb, ADDR_Func_352b, PG(0x4f2));
	if (!alu_and(gb, A, A)) {
		A = rd8(gb, wIsInWaterOrSand);
		if (!alu_and(gb, A, A))
			RETURN;
		A = 0x0a;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	// .asm_41857
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hl(gb, A);
	ld_l(gb, OBJ_STATE);
	A = 0x5c; wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_6855f);
	CALL(SetObjectFramesetPtr);
	A = 0x0a; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41830) { pg_41830(gb); }

PORT_LOCAL_JUMP(ParaGoomFunc, Func_4186f)
{
	if (shaken(gb))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	A = 0x00;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_4188b)
{
	ld_hl(gb, wCurObjState);
	A = 0x43; wr_hld(gb, A);
	A = rd8(gb, wCurObjCollBoxTop);
	alu_cp(gb, A, 0xef);
	if (!ZF)
		frameset_by_dir_hld(gb, ADDR_Frameset_684cb, ADDR_Frameset_684d2);
	else
		frameset_by_dir_hld(gb, ADDR_Frameset_684d9, ADDR_Frameset_684e0);
	CALL(SetObjectFramesetPtr);
	A = 0x44; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_418c4)
{
	if (shaken(gb))
		return;
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x27);
	if (!ZF) {
		if (dec_mhl(gb)) {
			JUMP(Func_3308);
			return;
		}
		A = rd8(gb, wCurObjCollBoxTop);
		alu_cp(gb, A, 0xef);
		if (ZF) {
			pg_41545(gb);
			return;
		}
		A = 0x01;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	// .asm_418ef
	dec_mhl(gb);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_rlca(gb, rd_hl(gb));
	if (!CF)
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

// .asm_41924
static void pg_41924(GB *gb)
{
	wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_684aa, ADDR_Frameset_684af);
	CALL(SetObjectFramesetPtr);
	A = 0x04; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x05; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_00);
	wr_hli(gb, A);
	A = 0xf4; wr_hl(gb, A);
	RETURN;
}

static void pg_stunned(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjAction);
	A = 0x8f; wr_hld(gb, A);
	A = state;
	pg_41924(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41904)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	pg_stunned(gb, CF ? 0x39 : 0x38);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41912) { pg_stunned(gb, 0x38); }
PORT_LOCAL_JUMP(ParaGoomFunc, Func_4191c) { pg_stunned(gb, 0x39); }

static void pg_41960(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
	RETURN;
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41954) { pg_41960(gb, 0x3b); }
PORT_LOCAL_JUMP(ParaGoomFunc, Func_4195b) { pg_41960(gb, 0x3c); }

static void pg_4196c(GB *gb, bool left)
{
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	ld_l(gb, OBJ_VAR_2);
	pg_var2_move(gb, left, 0x13);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_4196c) { pg_4196c(gb, false); }
PORT_LOCAL_JUMP(ParaGoomFunc, Func_4197f) { pg_4196c(gb, true); }

static void pg_419b8(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	gb->cpu.b = 0x02;
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	A = alu_and(gb, rd_hl(gb), 0xf0);
	wr_hld(gb, A);
	gb->cpu.c = A;
	xor_a(gb);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.b);
	A = alu_rlca(gb, gb->cpu.c);
	SET_DE(gb, CF ? ADDR_Frameset_684af : ADDR_Frameset_684aa);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41992) { pg_419b8(gb, 0x59); }
PORT_LOCAL_JUMP(ParaGoomFunc, Func_4199c) { pg_419b8(gb, 0x58); }
PORT_LOCAL_JUMP(ParaGoomFunc, Func_419a6) { pg_419b8(gb, 0x34); }
PORT_LOCAL_JUMP(ParaGoomFunc, Func_419b0) { pg_419b8(gb, 0x35); }

static void pg_slide(GB *gb, uint8_t state, uint8_t var2)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	dec_l(gb);
	A = 0x41; wr_hld(gb, A);
	A = var2; wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_419d5)
{
	pg_slide(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_419e8)
{
	pg_slide(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// .Func_419fb / .Func_41a2f up to their fallthrough: grabbed
static void pg_grabbed(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_684aa, ADDR_Frameset_684af);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x05; wr_hld(gb, A);
	A = 0xfa; wr_hld(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_GRABBED_F);
}

// flips the direction: ld hl, wCurObjSubState / ld a, [hl] / xor $80 / ld [hl], a
static void pg_flip(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), 0x80);
	wr_hl(gb, A);
}

static void pg_41a53(GB *gb);

static void pg_41a1f(GB *gb)
{
	A = rd8(gb, wDirection);
	if (alu_and(gb, A, A)) {
		JUMP(Func_3353);
		return;
	}
	pg_flip(gb);
	pg_grabbed(gb, 0x48);
	pg_41a53(gb);
}

static void pg_41a53(GB *gb)
{
	A = rd8(gb, wDirection);
	if (!alu_and(gb, A, A)) {
		JUMP(Func_3362);
		return;
	}
	pg_flip(gb);
	pg_grabbed(gb, 0x49);
	pg_41a1f(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_419fb)
{
	pg_grabbed(gb, 0x49);
	pg_41a1f(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41a1f) { pg_41a1f(gb); }

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41a2f)
{
	pg_grabbed(gb, 0x48);
	pg_41a53(gb);
}

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41a53) { pg_41a53(gb); }

PORT_LOCAL_JUMP(ParaGoomFunc, Func_41a64)
{
	ld_hl(gb, wCurObjAction);
	A = 0x9f; wr_hld(gb, A);
	A = 0x42; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_00);
	wr_hli(gb, A);
	A = 0xf4; wr_hl(gb, A);
	RETURN;
}
