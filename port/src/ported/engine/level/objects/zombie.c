// Port of src/engine/level/objects/zombie.asm
#include "ported.h"

#define ZOMBIE_FUNC_45C83 (ADDR_ZombieFunc + 0x1cf) // ZombieFunc.asm_45c83
#define ZOMBIE_HEAD_FLICKER (ADDR_ZombieHeadFunc + 0x2e) // .Flicker

// A zombie's head, flying off: once its state is in [$0b, $0f) it starts
// flickering, else ZombieFunc handles it.
PORT_JUMP(ZombieHeadFunc)
{
	CALL(ApplyObjXMovement);
	uint16_t hl = wCurObjState;
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x0b);
	if (gb->cpu.f & FLAG_C) {
		JUMP_TO(ZOMBIE_FUNC_45C83);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (!(gb->cpu.f & FLAG_C)) {
		JUMP_TO(ZOMBIE_FUNC_45C83);
		return;
	}
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	// ld_obj_frameset
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68dda : ADDR_Frameset_68dd5);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 40;
	hl = REG_HL(gb);
	wr8(gb, hl++, 40);
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(ZOMBIE_HEAD_FLICKER >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)ZOMBIE_HEAD_FLICKER;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	RET();
}

// Counts down wCurObjStateDuration; when it runs out, turns the head around.
// Despawns once it's off-screen with the duration at 0.
static void zombie_head_func(GB *gb, uint16_t frameset, uint16_t movement)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
		SET_HL(gb, hl);
		if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			RET();
			return;
		}
	} else {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
			SET_HL(gb, hl);
			gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
			wr8(gb, hl, gb->cpu.a);
			SET_DE(gb, frameset);
			CALL(SetObjectFramesetPtr);
		}
	}
	SET_BC(gb, movement);
	JUMP_TO(ADDR_ZombieHeadFunc);
}

PORT_JUMP(ZombieHeadFunc_Left) { zombie_head_func(gb, ADDR_Frameset_68dda, ADDR_Data_605a0); }
PORT_JUMP(ZombieHeadFunc_Right) { zombie_head_func(gb, ADDR_Frameset_68dd5, ADDR_Data_60560); }

// Flickers the head every other frame until its state duration runs out.
PORT_LOCAL(ZombieHeadFunc, Flicker)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint8_t d = alu_dec(gb, rd8(gb, wCurObjStateDuration));
	wr8(gb, wCurObjStateDuration, d);
	uint16_t hl = (uint16_t)((wCurObjStateDuration & 0xff00) | OBJ_FLAGS);
	SET_HL(gb, hl);
	if (!d) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 1);
	if (gb->cpu.a)
		return;
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJFLAG_INVISIBLE);
	wr8(gb, hl, gb->cpu.a);
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


// rrca: bit 0 into bit 7 and carry, z clear
static inline uint8_t rrca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 1) ? FLAG_C : 0;
	return (uint8_t)(v >> 1 | v << 7);
}

#define ZB(off) (ADDR_ZombieFunc + (off))
#define ZB_INIT ZB(0x1f)
#define ZB_ON_UNDERGROUND ZB(0x4f)
#define ZB_START_RISE ZB(0x127)
#define ZB_RISING ZB(0x16f)
#define ZB_ON_SURFACE ZB(0x1a9)
#define ZB_SUBSTATE_TABLE ZB(0x208)
#define ZB_HEADLESS ZB(0x371)

// ld hl, wCurObjUpdateFunction + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a  (with l set: ld l, OBJ_UPDATE_FUNCTION + 1)
static void zb_set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void zb_no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

// b = Wario's screen x + $2a, a = the object's + $2a, cp b
static void zb_cp_wario_x(GB *gb)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
}

// with c = $2a: a = (object's screen pos + c) - (Wario's + c)
static void zb_diff(GB *gb, uint16_t wario, uint16_t obj)
{
	A = alu_add(gb, rd8(gb, wario), gb->cpu.c);
	gb->cpu.b = A;
	A = alu_add(gb, rd8(gb, obj), gb->cpu.c);
	A = alu_sub(gb, A, gb->cpu.b);
}

PORT(ZombieFunc)
{
	zb_no_actions(gb);
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(ZB_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)ZB_INIT; wr_hld(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 3; wr_hld(gb, A);
	A = (uint8_t)-4; wr_hld(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	xor_a(gb);
	wr8(gb, wCurObjStateDuration, A);
}

PORT_LOCAL(ZombieFunc, Init)
{
	zb_no_actions(gb);
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(ZB_ON_UNDERGROUND >> 8); wr_hld(gb, A);
	A = (uint8_t)ZB_ON_UNDERGROUND; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	zb_cp_wario_x(gb);
	if (!CF) {
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
		A = 0x17;
	} else {
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
		A = 0x18;
	}
	wr8(gb, wCurObjFrame, A);
}

// The position 4 pixels up and dx pixels to the side into hYPos / hXPos
// (dx < 0: sub / sbc, else add / adc).
static void zb_ahead_pos_to_h(GB *gb, int dx)
{
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, rd_hli(gb), 4);
	wr8(gb, hYPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0);
	wr8(gb, hYPosHi, A);
	if (dx < 0) {
		A = alu_sub(gb, rd_hli(gb), (uint8_t)-dx);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0);
	} else {
		A = alu_add(gb, rd_hli(gb), (uint8_t)dx);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0);
	}
	wr8(gb, hXPosHi, A);
}

// Underground: walks along (turning at walls and ledges) until Wario comes
// near, then starts rising. The collision routines it calls run in bank 6
// at addresses inside this body (Func_19b51 at $5b51...).
PORT_LOCAL(ZombieFunc, OnUnderground)
{
	zb_no_actions(gb);
	A = 2;
	wr8(gb, wCurObjFrameDuration, A);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_3543, ZB(0x6b));
	if (!alu_and(gb, A, A)) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		return;
	}
	// .asm_45b27
	ld_hl(gb, wCurObjXPos);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool blocked;
	if (!CF) {
		A = alu_sub(gb, alu_and(gb, rd_hl(gb), 0x0f), 4);
		blocked = false;
		if (CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, ZB(0x86));
			blocked = !(A = alu_and(gb, A, 0x0f));
		}
		if (!blocked) {
			// .asm_45b3f
			zb_ahead_pos_to_h(gb, -4);
			gb_call_ret(gb, ADDR_Func_3513, ZB(0xa5));
			blocked = alu_and(gb, A, A) != 0;
			if (!blocked)
				gb_call_ret(gb, ADDR_MoveObjectLeft, ZB(0xab));
		}
	} else {
		// .asm_45b69
		A = alu_add(gb, alu_and(gb, rd_hl(gb), 0x0f), 3);
		alu_cp(gb, A, 16);
		blocked = false;
		if (!CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnRight, ZB(0xc1));
			blocked = !(A = alu_and(gb, A, 0x0f));
		}
		if (!blocked) {
			// .asm_45b79
			zb_ahead_pos_to_h(gb, 3);
			gb_call_ret(gb, ADDR_Func_3513, ZB(0xdf));
			blocked = alu_and(gb, A, A) != 0;
			if (!blocked)
				gb_call_ret(gb, ADDR_MoveObjectRight, ZB(0xe5));
		}
	}
	if (blocked) {
		// .asm_45b61
		ld_hl(gb, wCurObjSubState);
		A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
		wr_hl(gb, A);
		return;
	}
	// .asm_45b99
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
		return;
	}
	// .asm_45ba2
	gb->cpu.c = 0x2a;
	zb_diff(gb, wWarioScreenYPos, wCurObjScreenYPos);
	alu_cp(gb, A, 24);
	if (!CF) {
		alu_cp(gb, A, (uint8_t)-24);
		if (CF)
			return;
	}
	// .in_y_range
	zb_diff(gb, wWarioScreenXPos, wCurObjScreenXPos);
	alu_cp(gb, A, (uint8_t)-32);
	if (!CF)
		return;
	alu_cp(gb, A, 32);
	if (CF)
		return;
	alu_cp(gb, A, (uint8_t)-64);
	if (CF) {
		alu_cp(gb, A, 64);
		if (!CF)
			return;
	}
	// .in_x_range
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(ZB_START_RISE >> 8); wr_hld(gb, A);
	A = (uint8_t)ZB_START_RISE; wr_hld(gb, A);
	A = 20;
	wr8(gb, wCurObjStateDuration, A);
}

PORT_LOCAL(ZombieFunc, StartRise)
{
	zb_no_actions(gb);
	A = 2;
	wr8(gb, wCurObjFrameDuration, A);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	ld_l(gb, OBJ_SUBSTATE);
	zb_cp_wario_x(gb);
	if (!CF) {
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ADDR_Frameset_68d93);
	} else {
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ADDR_Frameset_68d9c);
	}
	CALL(SetObjectFramesetPtr);
	A = 20; wr_hli(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
	zb_set_update_l(gb, ZB_RISING);
	play_sfx(gb, SFX_074);
}

// .asm_45cca
static void zb_45cca(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	A = alu_or(gb, A, 0x01);
	wr_hl(gb, A);
	A = alu_rlca(gb, A);
	SET_DE(gb, CF ? ADDR_Frameset_68d8a : ADDR_Frameset_68d81);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ZombieFunc, Rising)
{
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_ZOMBIE_WARIO);
	if (ZF)
		zb_no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	dec_mhl(gb);
	A = rd_hl(gb);
	alu_cp(gb, A, 12);
	if (ZF) {
		A = (uint8_t)-12;
		wr8(gb, wCurObjCollBoxTop, A);
		RETURN;
	}
	alu_cp(gb, A, 6);
	if (ZF) {
		A = (uint8_t)-18;
		wr8(gb, wCurObjCollBoxTop, A);
		RETURN;
	}
	if (alu_and(gb, A, A))
		RETURN;
	A = (uint8_t)-24;
	wr8(gb, wCurObjCollBoxTop, A);
	zb_set_update_l(gb, ZB_ON_SURFACE);
	xor_a(gb);
	wr8(gb, wCurObjState, A);
	zb_45cca(gb);
}

// .asm_45c83
static void zb_45c83(GB *gb)
{
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hld(gb, A);
	A = 0x02; wr_hld(gb, A);
	play_sfx(gb, SFX_017);
	zb_cp_wario_x(gb);
	SET_BC(gb, CF ? ADDR_Func_33f8 : ADDR_Func_3407);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = gb->cpu.b;
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.c);
	RETURN;
}

PORT_LOCAL_JUMP(ZombieFunc, asm_45c83) { zb_45c83(gb); }

PORT_LOCAL_JUMP(ZombieFunc, OnSurface)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		A = rd8(gb, wTransformation);
		alu_cp(gb, A, TRANSFORMATION_ZOMBIE_WARIO);
		bool c70;
		if (!ZF) {
			// .asm_45cb2
			A = rd_hl(gb);
			alu_cp(gb, A, 0x0a);
			if (ZF) {
				JUMP(Func_3173);
				return;
			}
			c70 = true;
		} else {
			A = rd_hl(gb);
			alu_cp(gb, A, 0x0a);
			c70 = ZF;
		}
		if (c70) {
			// .asm_45c70
			dec_l(gb);
			A = alu_and(gb, rd_hl(gb), 0x0f);
			alu_cp(gb, A, 0x3);
			SET_DE(gb, ZF ? ADDR_Frameset_68d1f : ADDR_Frameset_68d1a);
			CALL(SetObjectFramesetPtr);
			zb_45c83(gb);
			return;
		}
		// .asm_45cba
		xor_a(gb);
		wr_hl(gb, A);
	}
	// .substate_table
	A = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
	JUMP_TO(jumptable(gb, ZB_SUBSTATE_TABLE + 6));
}

static void zb_45dcc(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	ld_hl(gb, wCurObjSubState);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	A = alu_or(gb, A, 0x2);
	wr_hl(gb, A);
	CALL(SetObjectFramesetPtr);
	A = 32; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(ZombieFunc, Func_45ce2)
{
	gb->cpu.c = 0x2a;
	zb_diff(gb, wWarioScreenYPos, wCurObjScreenYPos);
	alu_cp(gb, A, 32);
	bool near = CF;
	if (!near) {
		alu_cp(gb, A, (uint8_t)-32);
		near = !CF;
	}
	if (near) {
		// .asm_45cf6
		ld_hl(gb, wCurObjSubState);
		zb_diff(gb, wWarioScreenXPos, wCurObjScreenXPos);
		alu_cp(gb, A, (uint8_t)-48);
		bool d15 = !CF;
		bool go = true;
		if (!d15) {
			alu_cp(gb, A, 48);
			if (!CF)
				go = false;
		}
		if (go) {
			A = alu_rlca(gb, rd_hl(gb));
			if (!d15) {
				if (CF) {
					zb_45dcc(gb, ADDR_Frameset_68d2b);
					return;
				}
				SET_DE(gb, ADDR_Frameset_68da5);
			} else {
				if (!CF) {
					zb_45dcc(gb, ADDR_Frameset_68d24);
					return;
				}
				SET_DE(gb, ADDR_Frameset_68dac);
			}
			// .asm_45d1d
			A = rrca(gb, A);
			A = alu_and(gb, A, OBJSUBFLAGS_MASK);
			A = alu_or(gb, A, 0x3);
			wr_hl(gb, A);
			CALL(SetObjectFramesetPtr);
			A = 50; wr_hli(gb, A);
			play_sfx(gb, SFX_064);
			RETURN;
		}
	}
	// .asm_45d32
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_3543, ZB(0x290));
	if (!alu_and(gb, A, A)) {
		A = 0x30;
		wr8(gb, wCurObjStateDuration, A);
		ld_hl(gb, wCurObjUpdateFunction + 1);
		A = (uint8_t)(ZB_HEADLESS >> 8); wr_hld(gb, A);
		A = (uint8_t)ZB_HEADLESS; wr_hld(gb, A);
		RETURN;
	}
	// .asm_45d56
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hli(gb, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		A = alu_and(gb, rd_hl(gb), 0x0f);
		A = alu_sub(gb, A, 4);
		if (CF) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, ZB(0x2bb));
			A = alu_and(gb, A, 0x0f);
			if (ZF) {
				zb_45dcc(gb, ADDR_Frameset_68d24);
				return;
			}
		}
		ld_hl(gb, wCurObjYPos);
		A = alu_sub(gb, rd_hli(gb), 4);
		wr8(gb, hYPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0);
		wr8(gb, hYPosHi, A);
		A = alu_sub(gb, rd_hli(gb), 4);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0);
		wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3513, ZB(0x2da));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectLeft_Fast);
			return;
		}
		zb_45dcc(gb, ADDR_Frameset_68d24);
		return;
	}
	// .asm_45d94
	A = alu_and(gb, rd_hl(gb), 0x0f);
	A = alu_add(gb, A, 3);
	alu_cp(gb, A, 0x10);
	if (!CF) {
		gb_call_ret(gb, ADDR_ProcessBlockOnRight, ZB(0x2ec));
		A = alu_and(gb, A, 0x0f);
		if (ZF) {
			zb_45dcc(gb, ADDR_Frameset_68d2b);
			return;
		}
	}
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, rd_hli(gb), 4);
	wr8(gb, hYPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0);
	wr8(gb, hYPosHi, A);
	A = alu_add(gb, rd_hli(gb), 3);
	wr8(gb, hXPosLo, A);
	A = alu_adc(gb, rd_hli(gb), 0);
	wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_3513, ZB(0x30a));
	if (!alu_and(gb, A, A)) {
		JUMP(MoveObjectRight_Fast);
		return;
	}
	zb_45dcc(gb, ADDR_Frameset_68d2b);
}

PORT_LOCAL_JUMP(ZombieFunc, Func_45ddc)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	alu_cp(gb, A, 20);
	if (!ZF) {
		if (dec_mhl(gb)) {
			JUMP(Func_3308);
			return;
		}
		zb_45cca(gb);
		return;
	}
	dec_mhl(gb);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_rlca(gb, rd_hl(gb));
	if (!CF)
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

PORT_LOCAL_JUMP(ZombieFunc, ThrowHead)
{
	ld_hl(gb, wCurObjStateDuration);
	dec_mhl(gb);
	A = rd_hl(gb);
	alu_cp(gb, A, 16);
	if (ZF) {
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_BC(gb, CF ? ADDR_ObjParams_ZombieHeadRight : ADDR_ObjParams_ZombieHeadLeft);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	if (alu_and(gb, A, A))
		RETURN;
	A = 48;
	wr_hl(gb, A);
	zb_set_update_l(gb, ZB_HEADLESS);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	RETURN;
}

PORT_LOCAL(ZombieFunc, Headless)
{
	zb_no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	uint8_t d = dec_mhl(gb);
	ld_l(gb, OBJ_FLAGS);
	if (!d) {
		// .go_underground
		ld_hl(gb, wCurObjFlags);
		set_mhl(gb, OBJFLAG_INVISIBLE_F);
		xor_a(gb);
		wr8(gb, wCurObjState, A);
		A = 100;
		wr8(gb, wCurObjStateDuration, A);
		zb_set_update_l(gb, ZB_ON_UNDERGROUND);
		return;
	}
	A = alu_and(gb, rd8(gb, wGlobalCounter), 1);
	if (!ZF)
		return;
	A = alu_xor(gb, rd_hl(gb), OBJFLAG_INVISIBLE);
	wr_hl(gb, A);
}
