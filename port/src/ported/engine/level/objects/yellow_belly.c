// Port of src/engine/level/objects/yellow_belly.asm
#include "ported.h"

PORT(Func_51c77)
{
	SET_HL(gb, wCurObjStateDuration);
	uint8_t d = alu_dec(gb, rd8(gb, wCurObjStateDuration));
	wr8(gb, wCurObjStateDuration, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
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


#define YA(off) (ADDR_YellowBellyArrowFunc + (off))
#define YH(off) (ADDR_YellowBellyHeadFunc + (off))
#define YP(off) (ADDR_YellowBellyPlatformFunc + (off))

static void set_update(GB *gb, uint16_t f)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

// stop_music2 / xor a / ld [wBossState], a
static void stop_boss_music(GB *gb)
{
	A = 0xff;
	wr8(gb, hMusicIDBackup + 0, A);
	wr8(gb, hMusicID + 0, A);
	A = 0x00;
	wr8(gb, hMusicIDBackup + 1, A);
	wr8(gb, hMusicID + 1, A);
	xor_a(gb);
	wr8(gb, wBossState, A);
}

// .Func_51b4b: no actions unless the arrow is above Wario and flying
// towards him
static void ya_51b4b(GB *gb)
{
	gb->cpu.b = rd8(gb, wWarioScreenYPos);
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, gb->cpu.b);
	if (CF) {
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		bool c = CF;
		gb->cpu.b = rd8(gb, wWarioScreenXPos);
		A = rd8(gb, wCurObjScreenXPos);
		alu_cp(gb, A, gb->cpu.b);
		if (c ? (bool)CF : !CF)
			return;
	}
	no_actions(gb);
}

PORT_LOCAL(YellowBellyArrowFunc, Func_51b4b) { ya_51b4b(gb); }

// .Func_51a7d / .Func_51ad9
static void ya_update(GB *gb, bool left)
{
	gb_push16(gb, YA(left ? 0x1b : 0x77)); // call .Func_51b4b
	ya_51b4b(gb);
	gb_pop16(gb);
	A = rd8(gb, wCurObjState);
	alu_cp(gb, A, 0x28);
	bool up = ZF;
	if (!up) {
		alu_cp(gb, A, 0x29);
		up = ZF;
	}
	if (up) {
		set_update(gb, YA(left ? 0x45 : 0xcf));
		A = 0x04;
		wr8(gb, wCurObjVar2, A);
		RETURN;
	}
	if (alu_and(gb, A, A)) {
		// .asm_51af7
		A = 0x1e;
		wr8(gb, wCurObjStateDuration, A);
		set_update(gb, YA(0xb1));
		stop_boss_music(gb);
		RETURN;
	}
	CALL(MoveObjectDownByVar2);
	A = rd8(gb, wDollBoyHammerRange);
	if (!alu_and(gb, A, A)) {
		if (left)
			JUMP(MoveObjectLeftByVar2);
		else
			JUMP(MoveObjectRightByVar2);
	} else {
		if (left)
			JUMP(MoveObjectLeft);
		else
			JUMP(MoveObjectRight);
	}
}

static void ya_start(GB *gb, bool left)
{
	set_update(gb, YA(left ? 0x18 : 0x74));
	A = rd8(gb, wDollBoyHammerRange);
	if (alu_and(gb, A, A)) {
		ld_l(gb, OBJ_FLAGS);
		set_mhl(gb, OBJFLAG_GRABBED_F);
		A = 0x02;
		wr8(gb, wCurObjVar2, A);
	}
	ya_update(gb, left);
}

PORT_JUMP(YellowBellyArrowFunc) { ya_start(gb, true); }
PORT_LOCAL_JUMP(YellowBellyArrowFunc, Right) { ya_start(gb, false); }
PORT_LOCAL_JUMP(YellowBellyArrowFunc, Func_51a7d) { ya_update(gb, true); }
PORT_LOCAL_JUMP(YellowBellyArrowFunc, Func_51ad9) { ya_update(gb, false); }

// .Func_51aaa / .Func_51b34: flies off upwards, gone at the top
static void ya_fly_off(GB *gb, bool right)
{
	no_actions(gb);
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, 0x40);
	if (CF) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		RETURN;
	}
	CALL(MoveObjectUpByVar2);
	if (right)
		JUMP(MoveObjectRight_Fast);
	else
		JUMP(MoveObjectLeft_Fast);
}

PORT_LOCAL_JUMP(YellowBellyArrowFunc, Func_51aaa) { ya_fly_off(gb, true); }
PORT_LOCAL_JUMP(YellowBellyArrowFunc, Func_51b34) { ya_fly_off(gb, false); }

PORT_LOCAL(YellowBellyArrowFunc, Func_51b16)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}

static void yh_51b7b(GB *gb)
{
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_PUFFY_WARIO);
	if (ZF)
		no_actions(gb);
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		xor_a(gb);
		wr_hl(gb, A);
		A = 0x1e;
		wr8(gb, wCurObjStateDuration, A);
		A = 0x01;
		wr8(gb, wDollBoyActiveBarrels, A);
		stop_boss_music(gb);
	}
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (!dec_mhl(gb)) {
			xor_a(gb);
			wr8(gb, wDollBoyActiveBarrels, A);
		}
	}
	// .asm_51bb8: find the body
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb);
		alu_cp(gb, A, YELLOW_BELLY_BODY);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = alu_rra(gb, rd_hli(gb));
			if (CF)
				break;
		}
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF)
			return;
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
	// .asm_51bd8
	inc_l(gb);
	inc_l(gb);
	A = alu_add(gb, rd_hli(gb), 0x10);
	wr8(gb, wCurObjYPos + 0, A);
	A = alu_adc(gb, rd_hli(gb), 0x00);
	wr8(gb, wCurObjYPos + 1, A);
	A = rd_hli(gb);
	gb->cpu.e = A;
	gb->cpu.d = rd_hl(gb);
	A = alu_add(gb, gb->cpu.l, OBJ_VAR_1 - (OBJ_X_POS + 1));
	gb->cpu.l = A;
	A = rd_hli(gb);
	if (alu_and(gb, A, A)) {
		A = alu_inc(gb, A);
		if (ZF) {
			xor_a(gb);
			wr8(gb, wCurObjFlags, A);
			return;
		}
		no_actions(gb);
	}
	// .asm_51bf9
	inc_l(gb);
	inc_l(gb);
	bool vdir = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F);
	ld_hl(gb, wCurObjXPos);
	A = gb->cpu.e;
	if (vdir) {
		A = alu_add(gb, A, 0x08);
		wr_hli(gb, A);
		A = alu_adc(gb, gb->cpu.d, 0x00);
	} else {
		A = alu_sub(gb, A, 0x08);
		wr_hli(gb, A);
		A = alu_sbc(gb, gb->cpu.d, 0x00);
	}
	wr_hl(gb, A);
}

PORT(YellowBellyHeadFunc)
{
	ld_hl(gb, wCurObjFlags);
	set_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, YH(0x0d));
	yh_51b7b(gb);
}

PORT_LOCAL(YellowBellyHeadFunc, Func_51b7b) { yh_51b7b(gb); }

PORT(YellowBellyPlatformFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_mhl(gb, 3);
	set_update_l(gb, YP(0x27));
	SET_DE(gb, ADDR_Frameset_69893);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJSUBFLAG_UNK_4_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x07; wr_hld(gb, A);
	A = 0xf8; wr_hld(gb, A);
	A = 0xf6; wr_hl(gb, A);
}

// Func_51c62
static void yb_51c62(GB *gb)
{
	ld_hl(gb, wCurObjUnk02);
	gb->cpu.e = 0x07;
	FARCALL(Func_ba42);
}

PORT(Func_51c62) { yb_51c62(gb); }

PORT_LOCAL(YellowBellyPlatformFunc, Func_51c3f)
{
	A = rd8(gb, wCurObjState);
	alu_cp(gb, A, 0x2f);
	if (!ZF)
		return;
	SET_DE(gb, ADDR_Frameset_69896);
	CALL(SetObjectFramesetPtr);
	A = 0x5a; wr_hli(gb, A);
	set_update_l(gb, ADDR_Func_51c77);
	A = BOSS_DEFEATED;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	xor_a(gb);
	wr8(gb, wIsBossBattle, A);
	yb_51c62(gb);
}

#define PU(off) (ADDR_PumpFunc + (off))
#define PU_518C6 PU(0x0f)
#define PU_51947 PU(0x90)

static void pu_518c6(GB *gb)
{
	no_actions(gb);
	SET_BC(gb, ADDR_Data_609e0);
	CALL(ApplyObjYMovement);
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
	} else {
		obj_pos_to_hpos(gb);
		gb_call_ret(gb, ADDR_Func_352b, PU(0x36));
		if (alu_and(gb, A, A)) {
			// .asm_51927: landed
			ld_hl(gb, wCurObjYPos);
			A = rd8(gb, hYPosLo); wr_hli(gb, A);
			A = rd8(gb, hYPosHi); wr_hl(gb, A);
			SET_DE(gb, ADDR_Frameset_6987f);
			CALL(SetObjectFramesetPtr);
			set_update_l(gb, PU_51947);
			xor_a(gb);
			wr8(gb, wCurObjState, A);
			ld_l(gb, OBJ_FLAGS);
			res_mhl(gb, OBJFLAG_PRIORITY_F);
			RET();
			return;
		}
	}
	// .asm_518f0
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, rd_hli(gb), 0x04);
	wr8(gb, hYPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0x00);
	wr8(gb, hYPosHi, A);
	A = alu_rra(gb, rd8(gb, wCurObjVar1));
	bool right = CF;
	if (right) {
		A = alu_add(gb, rd_hli(gb), 0x05);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0x00);
	} else {
		A = alu_sub(gb, rd_hli(gb), 0x06);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
	}
	wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_3513, PU(right ? 0x6b : 0x59));
	if (alu_and(gb, A, A)) {
		RET();
		return;
	}
	if (right)
		JUMP(MoveObjectRight_Slow);
	else
		JUMP(MoveObjectLeft_Slow);
}

PORT_JUMP(PumpFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(PU_518C6 >> 8); wr_hld(gb, A);
	A = (uint8_t)PU_518C6; wr_hld(gb, A);
	A = rd8(gb, wGlobalCounter);
	wr8(gb, wCurObjVar1, A);
	pu_518c6(gb);
}

PORT_LOCAL_JUMP(PumpFunc, Func_518c6) { pu_518c6(gb); }

// .asm_51a1d: carried off by the arrow
static void pu_51a1d(GB *gb)
{
	no_actions(gb);
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, 0x48);
	if (CF) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		RET();
		return;
	}
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb);
		alu_cp(gb, A, YELLOW_BELLY_ARROW_LEFT);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = alu_rra(gb, rd_hli(gb));
			if (CF)
				break;
		}
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF) {
			RET();
			return;
		}
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
	inc_l(gb);
	inc_l(gb);
	A = alu_add(gb, rd_hli(gb), 0x08);
	wr8(gb, wCurObjYPos + 0, A);
	A = alu_adc(gb, rd_hli(gb), 0x00);
	wr8(gb, wCurObjYPos + 1, A);
	A = rd_hli(gb);
	wr8(gb, wCurObjXPos + 0, A);
	A = rd_hl(gb);
	wr8(gb, wCurObjXPos + 1, A);
	RET();
}

PORT_LOCAL_JUMP(PumpFunc, Func_51947)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x31);
	if (ZF) {
		// .asm_5198b: rolling
		ld_hl(gb, wCurObjStateDuration);
		if (!dec_mhl(gb)) {
			xor_a(gb);
			wr8(gb, wCurObjState, A);
			RET();
			return;
		}
		ld_hl(gb, wCurObjYPos);
		A = alu_sub(gb, rd_hli(gb), 0x04);
		wr8(gb, hYPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
		wr8(gb, hYPosHi, A);
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		bool right;
		if (!CF) {
			A = alu_sub(gb, rd_hli(gb), 0x06);
			wr8(gb, hXPosLo, A);
			A = alu_sbc(gb, rd_hli(gb), 0x00);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, PU(0xff));
			right = alu_and(gb, A, A);
			if (right) {
				ld_hl(gb, wCurObjSubState);
				set_mhl(gb, 7);
			}
		} else {
			A = alu_add(gb, rd_hli(gb), 0x05);
			wr8(gb, hXPosLo, A);
			A = alu_adc(gb, rd_hli(gb), 0x00);
			wr8(gb, hXPosHi, A);
			gb_call_ret(gb, ADDR_Func_3513, PU(0x11c));
			right = !alu_and(gb, A, A);
			if (!right) {
				ld_hl(gb, wCurObjSubState);
				res_mhl(gb, 7);
			}
		}
		SET_BC(gb, right ? ADDR_Data_60a10 : ADDR_Data_60a00);
		JUMP(ApplyObjXMovement);
		return;
	}
	alu_cp(gb, A, 0x0a);
	if (ZF) {
		// .asm_519e3
		A = 0x3a;
		wr_hl(gb, A);
		wr8(gb, w1d147, A);
		xor_a(gb);
		wr8(gb, wDollBoyHammerRange, A);
		SET_DE(gb, ADDR_Frameset_69882);
		CALL(SetObjectFramesetPtr);
		A = 0x1d; wr_hli(gb, A);
		xor_a(gb);
		wr8(gb, wCurObjVar3, A);
		play_sfx(gb, SFX_0BA);
		RET();
		return;
	}
	alu_cp(gb, A, 0x3a);
	if (ZF) {
		// .asm_51a03
		no_actions(gb);
		ld_l(gb, OBJ_STATE_DURATION);
		A = rd_hl(gb);
		if (alu_and(gb, A, A)) {
			dec_mhl(gb);
			RET();
			return;
		}
		SET_BC(gb, ADDR_Data_60040);
		JUMP(ApplyObjYMovement);
		return;
	}
	alu_cp(gb, A, 0x58);
	if (ZF) {
		pu_51a1d(gb);
		return;
	}
	A = alu_and(gb, A, 0xfe);
	if (ZF) {
		RET();
		return;
	}
	alu_cp(gb, A, 0x28);
	if (ZF) {
		// .asm_51a16
		A = 0x58;
		wr_hl(gb, A);
		xor_a(gb);
		wr8(gb, wDollBoyHammerRange, A);
		pu_51a1d(gb);
		return;
	}
	A = 0x31; wr_hld(gb, A);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	xor_a(gb);
	wr8(gb, wCurObjVar2, A);
	A = 0x3c;
	wr8(gb, wCurObjStateDuration, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
	RET();
}

// YellowBellyBodyFunc. Its local labels are given by their ROM address
// (warioland3.sym); the assert catches a shifted ROM.
_Static_assert(ADDR_YellowBellyBodyFunc == 0x5356, "YellowBellyBodyFunc moved");
#define YB(a) (ADDR_YellowBellyBodyFunc + ((a) - 0x5356))

// stop_music2
static void stop_music2(GB *gb)
{
	A = 0xff;
	wr8(gb, hMusicIDBackup + 0, A);
	wr8(gb, hMusicID + 0, A);
	A = 0x00;
	wr8(gb, hMusicIDBackup + 1, A);
	wr8(gb, hMusicID + 1, A);
}

// stop_sfx
static void stop_sfx(GB *gb)
{
	A = 0xff;
	wr8(gb, hSFXID + 0, A);
	A = 0x00;
	wr8(gb, hSFXID + 1, A);
}

// ld hl, pals / ld de, wTempPals2 palette 4 / ld c, 4 / ld b, 3 /
// jp CopyAndApplyOBPals
static void yb_copy_pals(GB *gb, uint16_t pals)
{
	ld_hl(gb, pals);
	SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
	gb->cpu.c = 4;
	gb->cpu.b = 3;
	JUMP(CopyAndApplyOBPals);
}

// Looks for an active object with this id in wObj1..wObj8. Found: hl is
// just past its OBJ_FLAGS. Not found: where the asm does ret z.
static bool yb_find_obj(GB *gb, uint8_t id)
{
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb);
		alu_cp(gb, A, id);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = alu_rra(gb, rd_hli(gb));
			if (CF)
				return true;
		}
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF)
			return false;
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
}

// jp SetObjectFramesetPtr. ret: when the code was reached through a call,
// that call's return address, which is still on the stack (0 otherwise).
static void yb_set_frameset_tail(GB *gb, uint16_t de, uint16_t ret)
{
	SET_DE(gb, de);
	if (ret)
		gb_call_ret(gb, ADDR_SetObjectFramesetPtr, ret);
	else
		JUMP(SetObjectFramesetPtr);
}

PORT(YellowBellyBodyFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, YB(0x539d));
	SET_DE(gb, ADDR_Frameset_69780);
	CALL(SetObjectFramesetPtr);
	A = 0x3c; wr_hli(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = alu_or(gb, alu_and(gb, rd_hl(gb), 0xf0), 0x04);
	wr_hl(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x07; wr_hld(gb, A);
	A = 0xf8; wr_hld(gb, A);
	A = 0x0a; wr_hl(gb, A);
	xor_a(gb);
	wr8(gb, wDollBoyActiveBarrels, A);
	wr8(gb, w1d147, A);
	wr8(gb, wDollBoyHammerRange, A);
	A = TRUE;
	wr8(gb, wIsBossBattle, A);
	stop_music2(gb);
}

// .Func_5139d / .Func_513bf / .Func_513d9: the palette fade-in steps
static void yb_fade_step(GB *gb, uint16_t next, uint16_t sfx, uint16_t pals)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		RETURN;
	A = 0x1e;
	wr_hl(gb, A);
	set_update_l(gb, next);
	if (sfx)
		play_sfx(gb, sfx);
	yb_copy_pals(gb, pals);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_5139d) { yb_fade_step(gb, YB(0x53bf), SFX_0CA, ADDR_Pals_51c81); }
PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_513bf) { yb_fade_step(gb, YB(0x53d9), 0, ADDR_Pals_51c99); }
PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_513d9) { yb_fade_step(gb, YB(0x53fb), 0, ADDR_Pals_51cb1); }

PORT_LOCAL(YellowBellyBodyFunc, Func_513fb)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	A = 0x20;
	wr_hl(gb, A);
	set_update_l(gb, YB(0x5419));
	A = BOSS_REGULAR;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
}

// ld b, $03 / ldh a, [rDIV] / and $01 / add b / ld [hli], a
static void yb_rand_duration(GB *gb)
{
	gb->cpu.b = 0x03;
	A = alu_add(gb, alu_and(gb, rd8(gb, rDIV), 0x01), gb->cpu.b);
	wr_hli(gb, A);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_51419)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectUp_Slow);
		return;
	}
	yb_rand_duration(gb);
	xor_a(gb); wr_hli(gb, A);
	A = 0x01; wr_hli(gb, A);
	xor_a(gb); wr_hli(gb, A);
	set_update_l(gb, YB(0x5652));
	SET_BC(gb, ADDR_ObjParams_YellowBellyHead);
	JUMP(CreateObjectFromCurObjPos);
}

// call SetObjectFramesetPtr / inc l / ld [hl], $44 / ld l, OBJ_SUBSTATE /
// ld a, [hl] / xor OBJSUBFLAG_VDIR | OBJSUBFLAG_HDIR / ld [hl], a
static void yb_turn_frameset(GB *gb, uint16_t de)
{
	SET_DE(gb, de);
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	A = 0x44;
	wr_hl(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_VDIR | OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
}

// call SetObjectFramesetPtr / ld a, $62 / ld [hli], a
static void yb_frameset_62(GB *gb, uint16_t de)
{
	SET_DE(gb, de);
	CALL(SetObjectFramesetPtr);
	A = 0x62;
	wr_hli(gb, A);
}

// .asm_51442
static void yb_51442(GB *gb)
{
	set_update_l(gb, YB(0x54a4));
	A = rd8(gb, wDollBoyHammerRange);
	if (alu_and(gb, A, A)) {
		xor_a(gb);
		wr8(gb, wCurObjVar1, A);
		ld_l(gb, OBJ_SUBSTATE);
		if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
			yb_frameset_62(gb, ADDR_Frameset_697a1);
		else
			yb_frameset_62(gb, ADDR_Frameset_69792);
		RETURN;
	}
	// .asm_5145c
	ld_l(gb, OBJ_X_POS);
	A = rd_hli(gb);
	gb->cpu.e = A;
	gb->cpu.d = rd_hl(gb);
	A = rd8(gb, wWarioXPos + 0);
	gb->cpu.b = A;
	A = rd8(gb, wWarioXPos + 1);
	gb->cpu.c = A;
	A = gb->cpu.b;
	alu_cp(gb, A, gb->cpu.d);
	bool less = CF;
	if (!less && ZF) {
		A = gb->cpu.c;
		alu_cp(gb, A, gb->cpu.e);
		less = CF;
	}
	ld_l(gb, OBJ_SUBSTATE);
	bool vdir = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F);
	if (!less) {
		// .asm_51473
		if (vdir)
			yb_frameset_62(gb, ADDR_Frameset_697a1);
		else
			yb_turn_frameset(gb, ADDR_Frameset_697b0);
	} else {
		// .asm_51483
		if (!vdir)
			yb_frameset_62(gb, ADDR_Frameset_69792);
		else
			yb_turn_frameset(gb, ADDR_Frameset_697bb);
	}
	RETURN;
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, asm_51442) { yb_51442(gb); }

// .asm_514eb. ret: the return address of a call to it, 0 for a jump.
static void yb_514eb(GB *gb, uint16_t ret)
{
	A = rd8(gb, wDollBoyHammerRange);
	if (!alu_and(gb, A, A)) {
		set_update(gb, YB(0x5652));
		ld_l(gb, OBJ_FLAGS);
		set_mhl(gb, OBJFLAG_PRIORITY_F);
		ld_l(gb, OBJ_STATE_DURATION);
		yb_rand_duration(gb);
		inc_l(gb);
		A = 0x01;
		wr_hli(gb, A);
		wr_hli(gb, A);
		xor_a(gb);
		wr8(gb, wCurObjState, A);
		ld_l(gb, OBJ_SUBSTATE);
		if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
			yb_set_frameset_tail(gb, ADDR_Frameset_697eb, ret);
		else
			yb_set_frameset_tail(gb, ADDR_Frameset_697e6, ret);
		return;
	}
	// .asm_51523
	set_update(gb, YB(0x55dd));
	ld_l(gb, OBJ_STATE_DURATION);
	A = 0x03; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjState, A);
	ld_l(gb, OBJ_SUBSTATE);
	if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
		yb_set_frameset_tail(gb, ADDR_Frameset_69789, ret);
	else
		yb_set_frameset_tail(gb, ADDR_Frameset_69780, ret);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, asm_514eb) { yb_514eb(gb, 0); }

// .asm_5179a: hit
static void yb_5179a(GB *gb)
{
	xor_a(gb);
	wr8(gb, w1d147, A);
	set_update(gb, YB(0x57ee));
	CALL(DecrementRemainingBossHits);
	A = alu_dec(gb, alu_and(gb, rd_hl(gb), 0x0f));
	if (!ZF) {
		SET_DE(gb, ADDR_Frameset_697d5);
		CALL(SetObjectFramesetPtr);
		A = 0x64; wr_hli(gb, A);
		A = 0x01; wr_hl(gb, A);
		RETURN;
	}
	// .asm_517bd: defeated
	set_update_l(gb, YB(0x584a));
	SET_DE(gb, ADDR_Frameset_69845);
	CALL(SetObjectFramesetPtr);
	A = 0x65; wr_hli(gb, A);
	A = 0xff; wr_hl(gb, A);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	heavy_or(gb, OBJ_INTERACTION_0B);
	wr_hli(gb, A);
	play_sfx(gb, SFX_0A9);
	stop_music2(gb);
	RETURN;
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, asm_5179a) { yb_5179a(gb); }

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_514a4)
{
	A = rd8(gb, w1d147);
	if (alu_and(gb, A, A)) {
		yb_5179a(gb);
		return;
	}
	ld_hl(gb, wCurObjVar1);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		ld_l(gb, OBJ_SUBSTATE);
		if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
			yb_frameset_62(gb, ADDR_Frameset_697a1);
		else
			yb_frameset_62(gb, ADDR_Frameset_69792);
		RETURN;
	}
	// .asm_514c9
	dec_l(gb);
	if (!dec_mhl(gb)) {
		yb_514eb(gb, 0);
		return;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, 0x2f);
	if (!ZF)
		RETURN;
	play_sfx(gb, SFX_07C);
	ld_l(gb, OBJ_SUBSTATE);
	if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
		SET_BC(gb, ADDR_ObjParams_YellowBellyArrowRight);
	else
		SET_BC(gb, ADDR_ObjParams_YellowBellyArrowLeft);
	JUMP(CreateObjectAtRelativePos);
}

// .asm_5154e
static void yb_5154e(GB *gb)
{
	stop_sfx(gb);
	A = 0x01;
	wr8(gb, wDollBoyHammerRange, A);
	set_update_l(gb, YB(0x5572));
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_PRIORITY_F);
	A = 0xa6;
	wr8(gb, wCurObjVar1, A);
	SET_DE(gb, ADDR_Frameset_697c6);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, asm_5154e) { yb_5154e(gb); }

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_51572)
{
	no_actions(gb);
	A = rd8(gb, wCurObjVar1);
	bool reload = true;
	alu_cp(gb, A, 0xa6);
	if (ZF) {
		play_sfx(gb, SFX_VANISH);
	} else {
		alu_cp(gb, A, 0x88);
		if (ZF) {
			play_sfx(gb, SFX_0A9);
		} else {
			alu_cp(gb, A, 0x25);
			if (ZF) {
				play_sfx(gb, SFX_0A5);
				SET_BC(gb, ADDR_ObjParams_Pump);
				CALL(CreateObjectFromCurObjPos);
			} else {
				reload = false;
			}
		}
	}
	if (reload)
		A = rd8(gb, wCurObjVar1); // .asm_515a8
	// .asm_515ab
	alu_cp(gb, A, 0x92);
	if (CF) {
		A = rd8(gb, wCurObjScreenYPos);
		alu_cp(gb, A, 0x39);
		if (!CF)
			CALL(MoveObjectUp);
	}
	// .asm_515b7
	ld_hl(gb, wCurObjVar1);
	if (!dec_mhl(gb)) {
		yb_514eb(gb, 0);
		return;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, 0x37);
	if (CF)
		RETURN;
	if (!ZF) {
		A = alu_and(gb, A, 0x0f);
		if (!ZF) {
			alu_cp(gb, A, 0x08);
			if (!ZF)
				RETURN;
			yb_copy_pals(gb, ADDR_Pals_51cc9);
			return;
		}
	}
	yb_copy_pals(gb, ADDR_Pals_51cb1); // .asm_515d0
}

// .asm_5168e
static void yb_5168e(GB *gb)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		alu_cp(gb, A, 0x0a);
		if (ZF) {
			yb_5154e(gb);
			return;
		}
		xor_a(gb);
		wr_hld(gb, A);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.c = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		A = alu_rlca(gb, rd_hl(gb));
		if (!CF) {
			A = gb->cpu.c;
			alu_cp(gb, A, gb->cpu.b);
		} else {
			// .asm_516b2
			A = gb->cpu.b;
			alu_cp(gb, A, gb->cpu.c);
		}
		if (!CF) {
			// .asm_516b6
			A = alu_xor(gb, rd_hl(gb), 0x80);
			wr_hl(gb, A);
		}
		// .asm_516ba
		A = 0x02;
		wr8(gb, wCurObjVar2, A);
	}
	// .asm_516bf
	ld_l(gb, OBJ_VAR_1);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		A = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (!CF) {
			dec_l(gb);
			A = rd_hl(gb);
			alu_cp(gb, A, 0x02);
			if (!CF)
				dec_mhl(gb);
		}
		// .asm_516d4
		ld_l(gb, OBJ_SUBSTATE);
		A = alu_dec(gb, rd8(gb, wDollBoyHammerRange));
		if (!ZF) {
			if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
				SET_DE(gb, ADDR_Frameset_697eb);
			else
				SET_DE(gb, ADDR_Frameset_697e6);
		} else {
			// .asm_516ec
			play_sfx(gb, SFX_101);
			if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F))
				SET_DE(gb, ADDR_Frameset_69789);
			else
				SET_DE(gb, ADDR_Frameset_69780);
		}
		JUMP(SetObjectFramesetPtr);
		return;
	}
	// .asm_51704
	ld_l(gb, OBJ_VAR_3);
	A = alu_dec(gb, rd_hl(gb));
	if (ZF) {
		A = rd8(gb, wTransformation);
		alu_cp(gb, A, TRANSFORMATION_PUFFY_WARIO);
		if (!ZF) {
			gb->cpu.b = rd8(gb, wWarioScreenYPos);
			A = alu_sub(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.b);
			if (CF) {
				ld_l(gb, OBJ_STATE_DURATION);
				if (!dec_mhl(gb)) {
					yb_51442(gb);
					return;
				}
				play_sfx(gb, SFX_0A8);
			}
		}
	}
	// .asm_51729
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, rd_hli(gb), 0x04);
	wr8(gb, hYPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0x00);
	wr8(gb, hYPosHi, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = CF;
	if (!right) {
		A = alu_sub(gb, rd_hli(gb), 0x18);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
		wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3513, YB(0x5749));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectLeftByVar2);
			return;
		}
	} else {
		// .asm_51759
		A = alu_add(gb, rd_hli(gb), 0x18);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0x00);
		wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3513, YB(0x5766));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectRightByVar2);
			return;
		}
	}
	ld_hl(gb, wCurObjSubState);
	bool vdir = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F);
	if (right ? vdir : !vdir) {
		yb_turn_frameset(gb, right ? ADDR_Frameset_697bb : ADDR_Frameset_697b0);
	} else {
		// .asm_51782
		A = alu_xor(gb, rd_hl(gb), 0x80);
		wr_hl(gb, A);
	}
	// .asm_51785
	A = 0x01;
	wr8(gb, wCurObjVar2, A);
	RETURN;
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, asm_5168e) { yb_5168e(gb); }

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_515dd)
{
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_PUFFY_WARIO);
	if (ZF) {
		xor_a(gb);
		wr8(gb, wDollBoyHammerRange, A);
		yb_514eb(gb, 0);
		return;
	}
	A = rd8(gb, w1d147);
	if (alu_and(gb, A, A)) {
		yb_5179a(gb);
		return;
	}
	A = rd8(gb, wDollBoyHammerRange);
	if (!alu_and(gb, A, A)) {
		yb_514eb(gb, 0);
		return;
	}
	if (!yb_find_obj(gb, PUMP))
		RETURN;
	// .asm_51619
	inc_l(gb); inc_l(gb); inc_l(gb); inc_l(gb);
	A = alu_and(gb, rd_hli(gb), 0xfe);
	gb->cpu.e = A;
	gb->cpu.d = rd_hl(gb);
	ld_hl(gb, wCurObjXPos);
	A = alu_and(gb, rd_hli(gb), 0xfe);
	gb->cpu.c = A;
	gb->cpu.b = rd_hl(gb);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_rlca(gb, rd_hl(gb));
	if (!CF) {
		A = alu_sub(gb, gb->cpu.c, 0x38);
		gb->cpu.c = A;
		A = alu_sbc(gb, gb->cpu.b, 0x00);
		gb->cpu.b = A;
	} else {
		A = alu_add(gb, gb->cpu.c, 0x38);
		gb->cpu.c = A;
		A = alu_adc(gb, gb->cpu.b, 0x00);
		gb->cpu.b = A;
	}
	// .asm_51642
	A = gb->cpu.e;
	alu_cp(gb, A, gb->cpu.c);
	if (ZF) {
		A = gb->cpu.d;
		alu_cp(gb, A, gb->cpu.b);
		if (ZF) {
			yb_51442(gb);
			return;
		}
	}
	// .asm_5164b
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
	yb_5168e(gb);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_51652)
{
	A = rd8(gb, wDollBoyActiveBarrels);
	if (alu_and(gb, A, A)) {
		no_actions(gb);
		xor_a(gb);
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	// .asm_51662
	A = rd8(gb, w1d147);
	if (alu_and(gb, A, A)) {
		yb_5179a(gb);
		return;
	}
	ld_hl(gb, wCurObjVar1);
	A = rd_hli(gb);
	if (!alu_and(gb, A, A)) {
		inc_l(gb);
		A = alu_dec(gb, rd_hl(gb));
		if (ZF) {
			gb->cpu.b = rd8(gb, wWarioScreenYPos);
			A = alu_sub(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.b);
			if (!CF) {
				set_update_l(gb, YB(0x578c));
				RETURN;
			}
		}
	}
	// .asm_51688
	SET_BC(gb, ADDR_Data_60950);
	CALL(ApplyObjYMovement_Loop);
	yb_5168e(gb);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_5178c)
{
	no_actions(gb);
	A = rd8(gb, wCurObjScreenXPos);
	alu_cp(gb, A, 0xc8);
	if (!CF)
		RETURN;
	JUMP(MoveObjectDown);
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_517ee)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (!dec_mhl(gb)) {
			play_sfx(gb, SFX_0B9); // .asm_5180a
		} else {
			A = alu_and(gb, rd_hl(gb), 0x0f);
			if (!ZF) {
				alu_cp(gb, A, 0x08);
				if (!ZF)
					RETURN;
				yb_copy_pals(gb, ADDR_Pals_51ce1);
				return;
			}
		}
		yb_copy_pals(gb, ADDR_Pals_51cb1); // .asm_51812
		return;
	}
	// .asm_5181f
	dec_l(gb);
	A = rd_hld(gb);
	alu_cp(gb, A, 0x10);
	if (!ZF)
		RETURN;
	A = rd_hld(gb);
	if (alu_and(gb, A, A))
		RETURN;
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	yb_514eb(gb, YB(0x582e)); // call .asm_514eb
	A = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
	alu_cp(gb, A, 0x03);
	if (ZF) {
		SET_DE(gb, ADDR_OAM_191940);
	} else {
		alu_cp(gb, A, 0x02);
		if (!ZF)
			RETURN;
		SET_DE(gb, ADDR_OAM_191f04);
	}
	// .asm_51844
	ld_l(gb, OBJ_OAM_PTR);
	A = gb->cpu.e;
	wr_hli(gb, A);
	wr_hl(gb, gb->cpu.d);
	RETURN;
}

PORT_LOCAL_JUMP(YellowBellyBodyFunc, Func_5184a)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb)) {
			A = alu_and(gb, rd_hl(gb), 0x0f);
			if (!ZF) {
				alu_cp(gb, A, 0x08);
				if (!ZF)
					RETURN;
				yb_copy_pals(gb, ADDR_Pals_51ce1);
				return;
			}
		}
		yb_copy_pals(gb, ADDR_Pals_51cb1); // .asm_51861
		return;
	}
	// .asm_5186e
	dec_l(gb);
	A = rd_hld(gb);
	alu_cp(gb, A, 0x14);
	if (ZF) {
		A = rd_hld(gb);
		if (!alu_and(gb, A, A))
			play_sfx(gb, SFX_0AA);
	}
	// .asm_51880
	ld_l(gb, OBJ_FRAMESET_OFFSET);
	A = rd_hld(gb);
	alu_cp(gb, A, 0x20);
	if (!ZF)
		RETURN;
	A = rd_hld(gb);
	if (alu_and(gb, A, A))
		RETURN;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
	if (!yb_find_obj(gb, YELLOW_BELLY_PLATFORM))
		RETURN;
	// .asm_518ad
	A = alu_add(gb, gb->cpu.l, 0x1a);
	gb->cpu.l = A;
	A = 0x2f;
	wr_hl(gb, A);
	JUMP_TO(ADDR_Func_51c62);
}
