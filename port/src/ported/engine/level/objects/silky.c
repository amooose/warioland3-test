// Port of src/engine/level/objects/silky.asm
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

#define SK(off) (ADDR_SilkyFunc + (off))
#define SK_INIT SK(0x0a)
#define SK_UPDATE SK(0x1e)

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

static void set_state(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a
static void set_state_hld(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
}

static void collbox_rl(GB *gb, uint8_t right, uint8_t left)
{
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = right; wr_hld(gb, A);
	A = left; wr_hld(gb, A);
}

static void interaction_hld(GB *gb, uint8_t x)
{
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, x);
	wr_hld(gb, A);
}

PORT(SilkyFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(SK_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)SK_INIT; wr_hld(gb, A);
}

static void sk_default(GB *gb)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	ld_l(gb, OBJ_STATE);
	A = 0x30; wr_hld(gb, A);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	bool c = CF;
	if (!c)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, c ? ADDR_Frameset_6874d : ADDR_Frameset_68744);
	CALL(SetObjectFramesetPtr);
	if (!c)
		collbox_rl(gb, 0x06, 0xf8);
	else
		collbox_rl(gb, 0x07, 0xf9);
}

PORT_LOCAL(SilkyFunc, Default) { sk_default(gb); }

PORT_LOCAL(SilkyFunc, Init)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(SK_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)SK_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	sk_default(gb);
}

PORT_LOCAL_JUMP(SilkyFunc, Update)
{
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, SK_UPDATE + 4));
}

static void sk_full_throw_start(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_6879e, ADDR_Frameset_687a7);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42dff)
{
	sk_full_throw_start(gb, 0x5e);
	JUMP(ObjState_FullThrowLeft);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42e1b)
{
	sk_full_throw_start(gb, 0x5f);
	JUMP(ObjState_FullThrowRight);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42e7b)
{
	set_state_hld(gb, 0x41);
	A = alu_rlca(gb, rd_hld(gb));
	bool c = CF;
	xor_a(gb);
	wr_hld(gb, A);
	SET_DE(gb, c ? ADDR_Frameset_6874d : ADDR_Frameset_68744);
	JUMP(SetObjectFramesetPtr);
}

// .asm_42f44
static void sk_42f44(GB *gb)
{
	if (shaken(gb))
		return;
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_3543, SK(0x24a));
	if (!alu_and(gb, A, A)) {
		set_state(gb, 0x11);
		RETURN;
	}
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hli(gb, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_sub(gb, A, 0x08);
		bool blocked = false;
		if (CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, SK(0x26c));
			A = alu_and(gb, A, 0x0f);
			blocked = ZF;
		}
		if (!blocked) {
			ld_hl(gb, wCurObjYPos);
			A = alu_sub(gb, rd_hli(gb), 0x04);
			wr8(gb, hYPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hYPosHi, A);
			A = alu_sub(gb, rd_hli(gb), 0x08);
			wr8(gb, hXPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, SK(0x28a));
			if (!alu_and(gb, A, A)) {
				JUMP(MoveObjectLeft_Slow);
				return;
			}
		}
	} else {
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_add(gb, A, 0x07);
		alu_cp(gb, A, 0x10);
		bool blocked = false;
		if (!CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnRight, SK(0x2a0));
			A = alu_and(gb, A, 0x0f);
			blocked = ZF;
		}
		if (!blocked) {
			ld_hl(gb, wCurObjYPos);
			A = alu_sub(gb, rd_hli(gb), 0x04);
			wr8(gb, hYPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hYPosHi, A);
			A = alu_add(gb, rd_hli(gb), 0x07);
			wr8(gb, hXPosLo, A);
			A = alu_adc(gb, rd_hli(gb), 0x00);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, SK(0x2be));
			if (!alu_and(gb, A, A)) {
				JUMP(MoveObjectRight_Slow);
				return;
			}
		}
	}
	set_state(gb, 0x13);
	RETURN;
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42e95)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		sk_42f44(gb);
		return;
	}
	if (shaken(gb))
		return;
	if (dec_mhl(gb))
		RETURN;
	interaction_hld(gb, OBJ_INTERACTION_SILKY);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, CF ? ADDR_Frameset_6875f : ADDR_Frameset_68756);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42eca)
{
	ld_hl(gb, wCurObjInteractionType);
	heavy_or(gb, OBJ_INTERACTION_01);
	wr_hld(gb, A);
	ld_l(gb, OBJ_STATE);
	A = 0x31; wr_hld(gb, A);
	A = alu_rlca(gb, rd_hld(gb));
	if (!CF) {
		collbox_rl(gb, 0x06, 0xf8);
		SET_DE(gb, ADDR_Frameset_68744);
	} else {
		collbox_rl(gb, 0x07, 0xf9);
		SET_DE(gb, ADDR_Frameset_6874d);
	}
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42ef8)
{
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_BALL_O_STRING_WARIO);
	if (ZF) {
		sk_42f44(gb);
		return;
	}
	gb->cpu.c = 0x2a;
	A = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.b = A;
	A = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	A = alu_sub(gb, A, gb->cpu.b);
	alu_cp(gb, A, 0x0f);
	if (!CF) {
		alu_cp(gb, A, 0xf1);
		if (CF) {
			sk_42f44(gb);
			return;
		}
	}
	// .asm_42f13
	A = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
	gb->cpu.b = A;
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
	alu_cp(gb, A, gb->cpu.b);
	ld_hl(gb, wCurObjSubState);
	alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
	bool c = CF;
	bool turn = c ? ZF : !ZF;
	if (turn) {
		set_state(gb, 0x13);
		RETURN;
	}
	SET_DE(gb, c ? ADDR_Frameset_687bb : ADDR_Frameset_687b0);
	CALL(SetObjectFramesetPtr);
	A = 0x23; wr_hli(gb, A);
	set_state(gb, 0x2d);
	RETURN;
}

static void sk_var2_move(GB *gb, bool left, uint8_t state)
{
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (left)
			JUMP(Func_32ae);
		else
			JUMP(Func_32bd);
		return;
	}
	set_state(gb, state);
	RETURN;
}

static void sk_stunned(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_6877a, ADDR_Frameset_68783);
	CALL(SetObjectFramesetPtr);
	collbox_rl(gb, 0x05, 0xfa);
	interaction_hld(gb, OBJ_INTERACTION_00);
	ld_hl(gb, wCurObjVar2);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_42fe5)
{
	sk_stunned(gb, 0x46);
	sk_var2_move(gb, true, 0x0f);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_4300a)
{
	ld_hl(gb, wCurObjVar2);
	sk_var2_move(gb, true, 0x0f);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_43018)
{
	sk_stunned(gb, 0x47);
	sk_var2_move(gb, false, 0x0f);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_4303d)
{
	ld_hl(gb, wCurObjVar2);
	sk_var2_move(gb, false, 0x0f);
}

static void sk_43059(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_6877a, ADDR_Frameset_68783);
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	collbox_rl(gb, 0x05, 0xfa);
	interaction_hld(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(SilkyFunc, Func_4304b) { sk_43059(gb, 0x44); }
PORT_LOCAL(SilkyFunc, Func_43053) { sk_43059(gb, 0x45); }

static void sk_430a0(GB *gb)
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

PORT_LOCAL_JUMP(SilkyFunc, Func_43083)
{
	set_state_hld(gb, 0x3f);
	frameset_by_dir_hld(gb, ADDR_Frameset_6877a, ADDR_Frameset_68783);
	CALL(SetObjectFramesetPtr);
	A = 0x64; wr_hli(gb, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	sk_430a0(gb);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_430a0) { sk_430a0(gb); }

static void sk_430c8(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_32f9);
		return;
	}
	set_state(gb, 0x00);
	RETURN;
}

PORT_LOCAL_JUMP(SilkyFunc, Func_430ba)
{
	set_state_hld(gb, 0x40);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	A = 0x07;
	wr8(gb, wCurObjStateDuration, A);
	sk_430c8(gb);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_430c8) { sk_430c8(gb); }

PORT_LOCAL(SilkyFunc, Func_430d5)
{
	set_state_hld(gb, 0x43);
	frameset_by_dir_hld(gb, ADDR_Frameset_68768, ADDR_Frameset_68771);
	CALL(SetObjectFramesetPtr);
	A = 0x23; wr_hli(gb, A);
	interaction_hld(gb, OBJ_INTERACTION_01);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_430f6)
{
	if (shaken(gb))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(Func_3308);
		return;
	}
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	set_state_hld(gb, 0x01);
	if (CF)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

static void sk_43138(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjAction);
	A = 0x8f; wr_hld(gb, A);
	A = state; wr_hld(gb, A);
	frameset_by_dir_hld(gb, ADDR_Frameset_6878c, ADDR_Frameset_68795);
	CALL(SetObjectFramesetPtr);
	A = 0x04; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	collbox_rl(gb, 0x05, 0xfa);
	interaction_hld(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(SilkyFunc, Func_43126) { sk_43138(gb, 0x38); }
PORT_LOCAL(SilkyFunc, Func_43130) { sk_43138(gb, 0x39); }

static void sk_4316e(GB *gb, uint8_t state)
{
	set_state(gb, state);
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
}

PORT_LOCAL(SilkyFunc, Func_43162) { sk_4316e(gb, 0x3b); }
PORT_LOCAL(SilkyFunc, Func_43169) { sk_4316e(gb, 0x3c); }

static void sk_4317a(GB *gb, bool left)
{
	ld_hl(gb, wCurObjFrameDuration);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hli(gb, A);
	ld_l(gb, OBJ_VAR_2);
	sk_var2_move(gb, left, 0x13);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_4317a) { sk_4317a(gb, false); }
PORT_LOCAL_JUMP(SilkyFunc, Func_4318d) { sk_4317a(gb, true); }

static void sk_431c6(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	gb->cpu.b = 0x02;
	A = alu_and(gb, rd_hl(gb), 0xf0);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hld(gb, A);
	A = gb->cpu.b;
	wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_687c6);
	CALL(SetObjectFramesetPtr);
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

PORT_LOCAL(SilkyFunc, Func_431a0) { sk_431c6(gb, 0x59); }
PORT_LOCAL(SilkyFunc, Func_431aa) { sk_431c6(gb, 0x58); }
PORT_LOCAL(SilkyFunc, Func_431b4) { sk_431c6(gb, 0x34); }
PORT_LOCAL(SilkyFunc, Func_431be) { sk_431c6(gb, 0x35); }

static void sk_slide(GB *gb, uint8_t state, uint8_t var2)
{
	set_state_hld(gb, state);
	dec_l(gb);
	A = 0x41; wr_hld(gb, A);
	A = var2; wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_431da)
{
	sk_slide(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_431ed)
{
	sk_slide(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

static void sk_grabbed(GB *gb, uint8_t state)
{
	set_state_hld(gb, state);
	frameset_by_dir_hld(gb, ADDR_Frameset_6877a, ADDR_Frameset_68783);
	CALL(SetObjectFramesetPtr);
	collbox_rl(gb, 0x05, 0xfa);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_GRABBED_F);
}

static void sk_flip(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), 0x80);
	wr_hl(gb, A);
}

static void sk_43252(GB *gb);

static void sk_43221(GB *gb)
{
	A = rd8(gb, wDirection);
	if (alu_and(gb, A, A)) {
		JUMP(Func_3353);
		return;
	}
	sk_flip(gb);
	sk_grabbed(gb, 0x48);
	sk_43252(gb);
}

static void sk_43252(GB *gb)
{
	A = rd8(gb, wDirection);
	if (!alu_and(gb, A, A)) {
		JUMP(Func_3362);
		return;
	}
	sk_flip(gb);
	sk_grabbed(gb, 0x49);
	sk_43221(gb);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_43200)
{
	sk_grabbed(gb, 0x49);
	sk_43221(gb);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_43221) { sk_43221(gb); }

PORT_LOCAL_JUMP(SilkyFunc, Func_43231)
{
	sk_grabbed(gb, 0x48);
	sk_43252(gb);
}

PORT_LOCAL_JUMP(SilkyFunc, Func_43252) { sk_43252(gb); }

PORT_LOCAL(SilkyFunc, Func_43263)
{
	ld_hl(gb, wCurObjAction);
	A = 0x9f; wr_hld(gb, A);
	A = 0x42; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	interaction_hld(gb, OBJ_INTERACTION_00);
}
