// Port of src/engine/level/objects/wolfenboss.asm
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


#define MS(off) (ADDR_MagicSpiralFunc + (off))
#define MS_GROW MS(0x15)
#define MS_MOVE MS(0x40)
#define MS_DISAPPEAR MS(0xb8)
#define WP(off) (ADDR_WolfenbossPlatformFunc + (off))

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

static void ms_grow(GB *gb)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		A = 8;
		wr_hl(gb, A);
		set_update_l(gb, MS_MOVE);
		RETURN;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, 69);
	if (ZF) {
		SET_DE(gb, ADDR_Frameset_69e39);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, A, 38);
	if (!ZF)
		RETURN;
	SET_DE(gb, ADDR_Frameset_69e42);
	JUMP(SetObjectFramesetPtr);
}

PORT_JUMP(MagicSpiralFunc)
{
	ld_hl(gb, wCurObjFlags);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	set_update_l(gb, MS_GROW);
	play_sfx(gb, SFX_0C2);
	ms_grow(gb);
}

PORT_LOCAL_JUMP(MagicSpiralFunc, Grow) { ms_grow(gb); }

// .disappear (returns like the asm: the caller does RET)
static void ms_disappear(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_69eb2);
	CALL(SetObjectFramesetPtr);
	A = 27; wr_hli(gb, A);
	set_update_l(gb, MS_DISAPPEAR);
	no_actions(gb);
}

// .reflect: counts down the bounces in the substate; returns whether
// it went on to .disappear
static void ms_reflect(GB *gb)
{
	A = dec_mhl(gb);
	A = alu_and(gb, A, 0x0f);
	if (!ZF)
		return;
	ms_disappear(gb);
}

PORT_LOCAL_JUMP(MagicSpiralFunc, Move)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (!dec_mhl(gb))
			play_sfx(gb, SFX_07C);
	}
	A = rd8(gb, wTransformation);
	if (alu_and(gb, A, A)) {
		ms_disappear(gb);
		RETURN;
	}
	A = alu_dec(gb, rd8(gb, wWolfenbossRemainingHits));
	if (ZF) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		RETURN;
	}
	ld_hl(gb, wCurObjSubState);
	bool down = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F);
	A = rd8(gb, wCurObjScreenYPos);
	bool turn = false;
	if (!down) {
		alu_cp(gb, A, 24);
		if (CF) {
			set_mhl(gb, OBJSUBFLAG_VDIR_F);
			turn = true;
		} else {
			CALL(MoveObjectUp);
		}
	} else {
		alu_cp(gb, A, SCREEN_HEIGHT_PX);
		if (!CF) {
			res_mhl(gb, OBJSUBFLAG_VDIR_F);
			turn = true;
		} else {
			CALL(MoveObjectDown);
		}
	}
	if (turn) {
		gb_push16(gb, MS(0x83)); // call .reflect
		ms_reflect(gb);
		gb_pop16(gb);
	}
	// .done_vertical_movement
	ld_l(gb, OBJ_SUBSTATE);
	bool right = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
	A = rd8(gb, wCurObjScreenXPos);
	if (!right) {
		alu_cp(gb, A, 16);
		if (!CF) {
			JUMP(MoveObjectLeft);
			return;
		}
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	} else {
		alu_cp(gb, A, SCREEN_WIDTH_PX);
		if (CF) {
			JUMP(MoveObjectRight);
			return;
		}
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	}
	ms_reflect(gb);
	RETURN;
}

PORT_LOCAL(MagicSpiralFunc, Disappear)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}

PORT(WolfenbossPlatformFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	set_update_l(gb, WP(0x27));
	SET_DE(gb, ADDR_Frameset_69e72);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJSUBFLAG_UNK_4_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 7; wr_hld(gb, A);
	A = (uint8_t)-8; wr_hld(gb, A);
	A = 8; wr_hl(gb, A);
}

PORT_LOCAL(WolfenbossPlatformFunc, WaitOpen)
{
	A = 2;
	wr8(gb, wCurObjFrameDuration, A);
	A = rd8(gb, wCurObjState);
	alu_cp(gb, A, 0x2f);
	if (!ZF)
		return;
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(WP(0x41) >> 8); wr_hld(gb, A);
	A = (uint8_t)WP(0x41); wr_hld(gb, A);
	A = 80;
	wr8(gb, wCurObjStateDuration, A);
}

PORT_LOCAL(WolfenbossPlatformFunc, Open)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	ld_l(gb, OBJ_VAR_2);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	set_update_l(gb, WP(0x56));
}

static void wb_56326(GB *gb)
{
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
	ld_hl(gb, wCurObjUnk02);
	gb->cpu.e = 0x07;
	FARCALL(Func_ba42);
}

PORT(Func_56326) { wb_56326(gb); }

PORT_LOCAL_JUMP(WolfenbossPlatformFunc, Disappear)
{
	ld_hl(gb, wCurObjFlags);
	if (alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F)) {
		JUMP(Func_3326);
		return;
	}
	wb_56326(gb);
	RETURN;
}

#define KU(off) (ADDR_KuriFunc + (off))
#define KU_LANDED KU(0x52)
#define KU_LAUNCHED KU(0xe0)

PORT(KuriFunc)
{
	no_actions(gb);
	SET_BC(gb, ADDR_Data_60d60);
	CALL(ApplyObjYMovement);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, KU(0x1d));
	if (!alu_and(gb, A, A))
		return;
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hl(gb, A);
	set_update_l(gb, KU_LANDED);
	ld_l(gb, OBJ_SUBSTATE);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF)
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
}

// .vanish
static void ku_vanish(GB *gb)
{
	ld_hl(gb, wWolfenbossIgaguriOrKuriActive);
	dec_mhl(gb);
	JUMP(VanishObject2);
}

PORT_LOCAL_JUMP(KuriFunc, Landed)
{
	A = rd8(gb, wTransformation);
	if (alu_and(gb, A, A))
		no_actions(gb);
	ld_hl(gb, wCurObjState);
	A = rd_hld(gb);
	if (alu_and(gb, A, A)) {
		bool sw = true; // .switch_direction
		alu_cp(gb, A, OBJSTATE_BUMP_LEFT_START);
		if (ZF) {
			sw = !alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
		} else {
			alu_cp(gb, A, OBJSTATE_BUMP_RIGHT_START);
			if (ZF) {
				sw = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
			} else {
				alu_cp(gb, A, OBJSTATE_VANISH_TOUCH);
				if (ZF) {
					ku_vanish(gb);
					return;
				}
				alu_cp(gb, A, OBJSTATE_ATTACKED_LEFT_START);
				bool left = ZF;
				bool attacked = left;
				if (!attacked) {
					alu_cp(gb, A, OBJSTATE_ATTACKED_RIGHT_START);
					attacked = ZF;
				}
				if (attacked) {
					if (left)
						set_mhl(gb, OBJSUBFLAG_HDIR_F);
					else
						res_mhl(gb, OBJSUBFLAG_HDIR_F);
					SET_DE(gb, ADDR_Frameset_69e60);
					// .asm_565b3
					inc_l(gb);
					xor_a(gb);
					wr_hl(gb, A);
					ld_l(gb, OBJ_FLAGS);
					set_mhl(gb, OBJFLAG_GRABBED_F);
					A = 3;
					wr8(gb, wCurObjVar2, A);
					set_update_l(gb, KU_LAUNCHED);
					JUMP(SetObjectFramesetPtr);
					return;
				}
			}
		}
		if (sw) {
			A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
			wr_hl(gb, A);
		}
		// .set_walking
		inc_l(gb);
		xor_a(gb);
		wr_hld(gb, A);
	}
	// .walking
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F)) {
		xor_a(gb);
		wr_hl(gb, A);
		RETURN;
	}
	A = rd8(gb, wTransformation);
	if (!alu_and(gb, A, A)) {
		A = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
		if (ZF)
			play_sfx(gb, SFX_06A);
	}
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_rlca(gb, rd_hl(gb));
	bool right = CF;
	A = rd8(gb, wCurObjScreenXPos);
	if (!right) {
		alu_cp(gb, A, 0x10);
		if (!CF) {
			JUMP(MoveObjectLeft);
			return;
		}
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	} else {
		alu_cp(gb, A, SCREEN_WIDTH_PX);
		if (CF) {
			JUMP(MoveObjectRight);
			return;
		}
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	}
	RETURN;
}

PORT_LOCAL_JUMP(KuriFunc, Launched)
{
	no_actions(gb);
	A = alu_and(gb, rd8(gb, wCurObjState), 0xfe);
	alu_cp(gb, A, 0x28);
	if (ZF) {
		ku_vanish(gb);
		return;
	}
	CALL(MoveObjectUp_Fast);
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F)) {
		xor_a(gb);
		wr_hl(gb, A);
		ld_hl(gb, wWolfenbossIgaguriOrKuriActive);
		dec_mhl(gb);
		RETURN;
	}
	ld_hl(gb, wCurObjSubState);
	bool right = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
	A = rd8(gb, wCurObjScreenXPos);
	if (!right) {
		alu_cp(gb, A, 0x10);
		if (!CF) {
			JUMP(MoveObjectLeftByVar2);
			return;
		}
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	} else {
		alu_cp(gb, A, SCREEN_WIDTH_PX);
		if (CF) {
			JUMP(MoveObjectRightByVar2);
			return;
		}
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	}
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, 0x11);
	if (CF)
		RETURN;
	play_sfx(gb, SFX_01A);
	RETURN;
}

#define IG(off) (ADDR_IgaguriFunc + (off))
#define IG_FALL IG(0x63)
#define IG_BOUNCE IG(0xf1)
#define IG_HATCH IG(0x149)

PORT_JUMP(IgaguriFunc)
{
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (ZF)
		play_sfx(gb, SFX_06B);
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		A = rd_hl(gb);
		alu_cp(gb, A, 43);
		if (ZF) {
			SET_DE(gb, ADDR_Frameset_69e80);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		alu_cp(gb, A, 24);
		if (!ZF) {
			RET();
			return;
		}
		SET_DE(gb, ADDR_Frameset_69e4b);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	// .done_grow
	set_update_l(gb, IG_FALL);
	SET_DE(gb, ADDR_Frameset_69e4b);
	CALL(SetObjectFramesetPtr);
	A = 40; wr_hli(gb, A);
	A = 0x03; wr_hli(gb, A);
	A = rd8(gb, wWolfenbossRemainingHits);
	alu_cp(gb, A, 2 + 1);
	if (!CF) {
		play_sfx(gb, SFX_101);
		A = 0x02;
	} else {
		play_sfx(gb, SFX_067);
		A = 0x01;
	}
	wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	RET();
}

static void ig_no_actions_if_transformed(GB *gb)
{
	A = rd8(gb, wTransformation);
	if (alu_and(gb, A, A))
		no_actions(gb);
}

// .ground_shake
static void ig_ground_shake(GB *gb)
{
	play_sfx(gb, SFX_061);
	gb->cpu.b = 24;
}

// .do_horizontal_movement
static void ig_move_x(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	bool right = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
	A = rd8(gb, wCurObjScreenXPos);
	if (!right) {
		alu_cp(gb, A, 0x10);
		if (!CF) {
			JUMP(MoveObjectLeftByVar2);
			return;
		}
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	} else {
		alu_cp(gb, A, SCREEN_WIDTH_PX);
		if (CF) {
			JUMP(MoveObjectRightByVar2);
			return;
		}
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	}
	RET();
}

// .skip_bounce
static void ig_skip_bounce(GB *gb)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(IG_HATCH >> 8); wr_hld(gb, A);
	A = (uint8_t)IG_HATCH; wr_hld(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	RET();
}

// hYPos -> wCurObjYPos
static void ig_land(GB *gb)
{
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hl(gb, A);
}

PORT_LOCAL_JUMP(IgaguriFunc, Fall)
{
	ig_no_actions_if_transformed(gb);
	A = 0x02;
	wr8(gb, wCurObjFrameDuration, A);
	A = rd8(gb, wWolfenbossRemainingHits);
	alu_cp(gb, A, 2 + 1);
	if (!CF) {
		ld_hl(gb, wCurObjYPos);
		A = alu_add(gb, rd_hl(gb), 0x4);
		wr_hli(gb, A);
		if (CF)
			inc_mhl(gb);
	} else {
		SET_BC(gb, ADDR_Data_60d80);
		CALL(ApplyObjYMovement);
	}
	// .check_floor
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, IG(0x9e));
	if (!alu_and(gb, A, A)) {
		ig_move_x(gb);
		return;
	}
	ig_land(gb);
	gb_push16(gb, IG(0xad)); // call .ground_shake
	ig_ground_shake(gb);
	gb_call_ret(gb, ADDR_DoGroundShake, gb_pop16(gb));
	A = rd8(gb, wWolfenbossRemainingHits);
	alu_cp(gb, A, 2 + 1);
	if (!CF) {
		ig_skip_bounce(gb);
		return;
	}
	ld_hl(gb, wCurObjSubState);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF)
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
	// .do_bounce
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	set_update_l(gb, IG_BOUNCE);
	RET();
}

PORT_LOCAL_JUMP(IgaguriFunc, Bounce)
{
	ig_no_actions_if_transformed(gb);
	A = 0x2;
	wr8(gb, wCurObjFrameDuration, A);
	SET_BC(gb, ADDR_Data_60e00);
	CALL(ApplyObjYMovement);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, IG(0x119));
	if (!alu_and(gb, A, A)) {
		ig_move_x(gb);
		return;
	}
	ig_land(gb);
	ld_l(gb, OBJ_VAR_1);
	if (!dec_mhl(gb)) {
		ig_skip_bounce(gb);
		return;
	}
	xor_a(gb);
	wr8(gb, wCurObjVar3, A);
	ig_ground_shake(gb);
	JUMP(DoGroundShake);
}

PORT_LOCAL_JUMP(IgaguriFunc, Hatch)
{
	ig_no_actions_if_transformed(gb);
	ld_hl(gb, wCurObjVar1);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		A = 0x2;
		wr8(gb, wCurObjFrameDuration, A);
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb)) {
			RET();
			return;
		}
		A = 60; wr_hli(gb, A);
		inc_mhl(gb);
		RET();
		return;
	}
	alu_cp(gb, A, 0x01);
	if (ZF) {
		A = alu_and(gb, rd8(gb, wGlobalCounter), 0x1f);
		if (ZF)
			play_sfx(gb, SFX_0C5);
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb)) {
			RET();
			return;
		}
		A = 14; wr_hli(gb, A);
		inc_mhl(gb);
		SET_DE(gb, ADDR_Frameset_69e59);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, A, 0x02);
	if (!ZF) {
		RET();
		return;
	}
	ld_l(gb, OBJ_STATE_DURATION);
	if (!dec_mhl(gb)) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		RET();
		return;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, 5);
	if (!ZF) {
		RET();
		return;
	}
	play_sfx(gb, SFX_0C4);
	SET_BC(gb, ADDR_ObjParams_Kuri);
	JUMP(CreateObjectFromCurObjPos);
}

// --- Wolfenboss ---

#define WB(addr) ((uint16_t)(ADDR_WolfenbossFunc + ((addr) - 0x5e4e)))
#define WB_WHISP_WAITING WB(0x5e8d)
#define WB_WHISP_RISING WB(0x5ec4)
#define WB_SMOKE_ENTRANCE WB(0x5eea)
#define WB_EMERGE_FROM_SMOKE WB(0x5f20)
#define WB_START_WAIT WB(0x5f41)
#define WB_SMOKE_EXIT WB(0x5f5f)
#define WB_DISAPPEAR WB(0x5f6e)
#define WB_MAIN WB(0x5fb3)
#define WB_DEFEAT_FALL WB(0x61a9)

static inline void stop_music2(GB *gb)
{
	A = 0xff;
	wr8(gb, hMusicIDBackup + 0, A);
	wr8(gb, hMusicID + 0, A);
	A = 0x00;
	wr8(gb, hMusicIDBackup + 1, A);
	wr8(gb, hMusicID + 1, A);
}

// call SetObjectFramesetPtr / ld a, n / ld [hli], a
static void wb_frameset_dur(GB *gb, uint16_t fs, uint8_t n)
{
	SET_DE(gb, fs);
	CALL(SetObjectFramesetPtr);
	A = n;
	wr_hli(gb, A);
}

PORT(WolfenbossFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, WB_WHISP_WAITING);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	A = alu_or(gb, A, 3 + 1);
	wr_hl(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 2; wr_hld(gb, A);
	A = (uint8_t)-3; wr_hld(gb, A);
	A = 1; wr_hl(gb, A);
	wb_frameset_dur(gb, ADDR_Frameset_69e97, 20);
	inc_l(gb);
	xor_a(gb);
	wr_hl(gb, A);
	wr8(gb, wWolfenbossNextAttack, A);
	wr8(gb, wWolfenbossIgaguriOrKuriActive, A);
	A = 3 + 1;
	wr8(gb, wWolfenbossRemainingHits, A);
	A = 2;
	wr8(gb, wWolfenbossConsecutiveSpirals, A);
}

PORT_LOCAL_JUMP(WolfenbossFunc, WhispWaiting)
{
	no_actions(gb);
	ld_hl(gb, wCurObjFlags);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F))
		RETURN;
	ld_l(gb, OBJ_STATE_DURATION);
	dec_mhl(gb);
	SET_BC(gb, ADDR_Data_60650);
	if (!ZF) {
		JUMP(ApplyObjXMovement_Loop);
		return;
	}
	set_update_l(gb, WB_WHISP_RISING);
	wb_frameset_dur(gb, ADDR_Frameset_69ea0, 64);
	A = TRUE;
	wr8(gb, wIsBossBattle, A);
	stop_music2(gb);
	RETURN;
}

PORT_LOCAL_JUMP(WolfenbossFunc, WhispRising)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectUp);
		return;
	}
	set_update_l(gb, WB_SMOKE_ENTRANCE);
	wb_frameset_dur(gb, ADDR_Frameset_69ea9, 19);
	play_sfx(gb, SFX_0AA);
	RETURN;
}

PORT_LOCAL(WolfenbossFunc, SmokeEntrance)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	set_update_l(gb, WB_EMERGE_FROM_SMOKE);
	ld_l(gb, OBJ_FRAMESET_PTR);
	A = (uint8_t)ADDR_Frameset_69e0e; wr_hli(gb, A);
	A = (uint8_t)(ADDR_Frameset_69e0e >> 8); wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
	A = alu_inc(gb, A);
	A = alu_inc(gb, A);
	wr_hli(gb, A);
	ld_l(gb, OBJ_COLLBOX_TOP);
	A = (uint8_t)-8; wr_hli(gb, A);
	A = 12; wr_hli(gb, A);
	A = (uint8_t)-8; wr_hli(gb, A);
	A = 7; wr_hli(gb, A);
	A = BOSS_REGULAR;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
}

PORT_LOCAL_JUMP(WolfenbossFunc, EmergeFromSmoke)
{
	no_actions(gb);
	A = rd8(gb, wCurObjFrame);
	if (alu_and(gb, A, A)) {
		JUMP(MoveObjectUp_Slow);
		return;
	}
	wb_frameset_dur(gb, ADDR_Frameset_69dcb, 40);
	xor_a(gb);
	wr_hli(gb, A);
	wr_hl(gb, A);
	set_update_l(gb, WB_START_WAIT);
	RETURN;
}

PORT_LOCAL(WolfenbossFunc, StartWait)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	inc_l(gb);
	xor_a(gb);
	wr_hli(gb, A);
	inc_l(gb);
	wr_hli(gb, A);
	set_update_l(gb, WB_MAIN);
	SET_DE(gb, ADDR_Frameset_69dec);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(WolfenbossFunc, SmokeExit)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}

PORT_LOCAL(WolfenbossFunc, Disappear)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	wb_frameset_dur(gb, ADDR_Frameset_69ea9, 14);
	set_update_l(gb, WB_SMOKE_EXIT);
	play_sfx(gb, SFX_0AA);
}

// .start_movement
static void wb_start_movement(GB *gb)
{
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, CF ? ADDR_Frameset_69de7 : ADDR_Frameset_69dec);
	JUMP(SetObjectFramesetPtr);
}

// .start_attack (de = frameset)
static void wb_start_attack(GB *gb)
{
	CALL(SetObjectFramesetPtr);
	bool spiral;
	A = rd8(gb, wWolfenbossConsecutiveSpirals);
	if (alu_and(gb, A, A)) {
		A = alu_dec(gb, A);
		wr8(gb, wWolfenbossConsecutiveSpirals, A);
		spiral = true;
	} else {
		A = rd8(gb, wWolfenbossIgaguriOrKuriActive);
		spiral = alu_and(gb, A, A) != 0;
	}
	if (spiral) {
		gb->cpu.b = 0x00;
		A = 103; // WOLFENBOSS_SPIRAL_ATTACK_DURATION
	} else {
		A = TRUE;
		wr8(gb, wWolfenbossIgaguriOrKuriActive, A);
		gb->cpu.b = 0x01;
		A = 72; // WOLFENBOSS_IGUGARI_ATTACK_DURATION
	}
	// .got_next_attack
	wr_hli(gb, A);
	A = 0x01;
	wr_hl(gb, A);
	A = gb->cpu.b;
	wr8(gb, wWolfenbossNextAttack, A);
	RETURN;
}

// .start_turning (de = frameset)
static void wb_start_turning(GB *gb)
{
	CALL(SetObjectFramesetPtr);
	A = 27; wr_hli(gb, A);
	A = 0x03; wr_hl(gb, A);
	RETURN;
}

// .defeated
static void wb_defeated(GB *gb)
{
	set_update_l(gb, WB_DEFEAT_FALL);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_PRIORITY_F);
	stop_music2(gb);
	SET_DE(gb, ADDR_Frameset_69e05);
	JUMP(SetObjectFramesetPtr);
}

// .wait_then_start_movement
static void wb_wait_then_start_movement(GB *gb)
{
	ld_l(gb, OBJ_STATE_DURATION);
	if (dec_mhl(gb))
		RETURN;
	wb_start_movement(gb);
}

// .do_movement
static void wb_do_movement(GB *gb)
{
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x1f);
	if (ZF)
		play_sfx(gb, SFX_0C3);
	SET_BC(gb, ADDR_Data_604c0);
	CALL(ApplyObjYMovement_Loop);

	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		// moving left
		A = rd8(gb, wGlobalCounter);
		if (!alu_and(gb, A, A)) {
			gb->cpu.b = rd8(gb, wWarioScreenXPos);
			A = rd8(gb, wCurObjScreenXPos);
			alu_cp(gb, A, gb->cpu.b);
			if (!CF) {
				// .asm_56115
				SET_DE(gb, ADDR_Frameset_69df1);
				wb_start_attack(gb);
				return;
			}
			A = 0x01;
			wr8(gb, wCurObjVar2, A);
		} else {
			// .move_left
			A = rd8(gb, wCurObjScreenXPos);
			alu_cp(gb, A, 0x18);
			if (!ZF) {
				JUMP(MoveObjectLeft_Slow);
				return;
			}
		}
		// .asm_56122
		SET_DE(gb, ADDR_Frameset_69dd5);
		wb_start_turning(gb);
		return;
	}
	// .moving_right
	A = rd8(gb, wGlobalCounter);
	if (!alu_and(gb, A, A)) {
		gb->cpu.b = rd8(gb, wCurObjScreenXPos);
		A = rd8(gb, wWarioScreenXPos);
		alu_cp(gb, A, gb->cpu.b);
		if (!CF) {
			// .asm_5613e
			SET_DE(gb, ADDR_Frameset_69df6);
			wb_start_attack(gb);
			return;
		}
		A = 0x01;
		wr8(gb, wCurObjVar2, A);
	} else {
		// .move_right
		A = rd8(gb, wCurObjScreenXPos);
		alu_cp(gb, A, 0x98);
		if (!ZF) {
			JUMP(MoveObjectRight_Slow);
			return;
		}
	}
	// .asm_56176
	SET_DE(gb, ADDR_Frameset_69dde);
	wb_start_turning(gb);
}

PORT_LOCAL_JUMP(WolfenbossFunc, Main)
{
	no_actions(gb);

	A = rd8(gb, wTransformation);
	if (alu_and(gb, A, A)) {
		// .start_disappear
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, CF ? ADDR_Frameset_69e1f : ADDR_Frameset_69e0e);
		CALL(SetObjectFramesetPtr);
		A = 70; // WOLFENBOSS_DISAPPEAR_DELAY
		wr_hli(gb, A);
		set_update_l(gb, WB_DISAPPEAR);
		xor_a(gb);
		wr8(gb, wBossState, A);
		RETURN;
	}

	ld_hl(gb, wCurObjState);
	A = alu_and(gb, rd_hl(gb), 0xfe);
	if (!ZF) {
		alu_cp(gb, A, 0x08);
		if (ZF) {
			A = 0x38;
			wr_hld(gb, A);
			frameset_by_dir_hld(gb, ADDR_Frameset_69e85, ADDR_Frameset_69e8e);
			CALL(SetObjectFramesetPtr);
			A = 60; // WOLFENBOSS_HURT_DURATION
			wr_hli(gb, A);
			ld_l(gb, OBJ_SUBSTATE);
			A = alu_and(gb, rd_hl(gb), 0x0f);
			A = alu_dec(gb, A);
			if (ZF)
				RETURN;
			dec_mhl(gb);
			ld_hl(gb, wWolfenbossRemainingHits);
			dec_mhl(gb);
			RETURN;
		}
		// .check_hurt_animation
		alu_cp(gb, A, 0x38);
		if (ZF) {
			ld_l(gb, OBJ_STATE_DURATION);
			if (!dec_mhl(gb)) {
				// .done_hurt
				A = 40; // WOLFENBOSS_AFTER_HURT_DELAY
				wr_hli(gb, A);
				A = 0x04;
				wr_hl(gb, A);
				ld_l(gb, OBJ_FLAGS);
				res_mhl(gb, OBJFLAG_NO_COLLISION_F);
				ld_l(gb, OBJ_STATE);
				xor_a(gb);
				wr_hld(gb, A);
				alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
				SET_DE(gb, ZF ? ADDR_Frameset_69e0e : ADDR_Frameset_69e1f);
				JUMP(SetObjectFramesetPtr);
				return;
			}
			A = rd_hl(gb);
			alu_cp(gb, A, 41);
			if (CF) {
				JUMP(MoveObjectDown_Slow);
				return;
			}
			if (!ZF)
				RETURN;
			play_sfx(gb, SFX_0A9);
			RETURN;
		}
		// .restart_attacks_and_movement
		xor_a(gb);
		wr_hl(gb, A);
	}

	// .do_attacks_and_movement
	ld_l(gb, OBJ_VAR_1);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		wb_do_movement(gb);
		return;
	}
	alu_cp(gb, A, 0x01);
	if (ZF) {
		// .do_attack
		A = rd8(gb, wWolfenbossNextAttack);
		gb->cpu.c = A;
		alu_and(gb, A, A);
		gb->cpu.b = ZF ? 103 - 16 : 72 - 16;
		// .got_attack_delay
		ld_l(gb, OBJ_STATE_DURATION);
		A = rd_hl(gb);
		alu_cp(gb, A, gb->cpu.b);
		if (ZF) {
			// .spawn_igugari_or_spiral
			dec_mhl(gb);
			A = alu_rlca(gb, rd8(gb, wCurObjSubState));
			bool right = CF;
			gb->cpu.c = alu_dec(gb, gb->cpu.c); // keeps the carry
			bool igaguri = ZF;
			if (!right)
				SET_BC(gb, igaguri ? ADDR_ObjParams_IgaguriLeft : ADDR_ObjParams_MagicSpiralLeft);
			else
				SET_BC(gb, igaguri ? ADDR_ObjParams_IgaguriRight : ADDR_ObjParams_MagicSpiralRight);
			JUMP(CreateObjectAtRelativePos);
			return;
		}
		if (dec_mhl(gb))
			RETURN;
		A = 19; // WOLFENBOSS_AFTER_ATTACK_DELAY
		wr_hli(gb, A);
		A = 0x02;
		wr_hl(gb, A);
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, CF ? ADDR_Frameset_69e00 : ADDR_Frameset_69dfb);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, A, 0x02);
	if (ZF) {
		wb_wait_then_start_movement(gb);
		return;
	}
	alu_cp(gb, A, 0x03);
	if (ZF) {
		// .turning
		SET_BC(gb, ADDR_Data_604c0);
		CALL(ApplyObjYMovement_Loop);
		ld_hl(gb, wCurObjStateDuration);
		if (dec_mhl(gb))
			RETURN;
		ld_l(gb, OBJ_SUBSTATE);
		A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
		wr_hl(gb, A);
		A = rd8(gb, wCurObjVar2);
		if (!alu_and(gb, A, A)) {
			wb_start_movement(gb);
			return;
		}
		// .asm_560d9
		xor_a(gb);
		wr8(gb, wCurObjVar2, A);
		alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ZF ? ADDR_Frameset_69df1 : ADDR_Frameset_69df6);
		wb_start_attack(gb);
		return;
	}
	alu_cp(gb, A, 0x04);
	if (!ZF)
		RETURN;
	// .after_hurt
	CALL(MoveObjectUp_Slow);
	ld_l(gb, OBJ_FRAMESET_OFFSET);
	A = rd_hld(gb);
	alu_cp(gb, A, 0x4);
	if (ZF) {
		A = rd_hld(gb);
		if (!alu_and(gb, A, A)) {
			A = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
			alu_cp(gb, A, 1);
			if (ZF) {
				wb_defeated(gb);
				return;
			}
		}
	}
	wb_wait_then_start_movement(gb);
}

PORT_LOCAL_JUMP(WolfenbossFunc, DefeatFall)
{
	no_actions(gb);
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, 0xc0);
	if (CF) {
		A = rd8(gb, wCurObjVar3);
		alu_cp(gb, A, 0x2b);
		if (ZF)
			play_sfx(gb, SFX_0A3);
		SET_BC(gb, ADDR_Data_60da0);
		JUMP(ApplyObjYMovement);
		return;
	}
	// .asm_561ca
	A = BOSS_DEFEATED;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	xor_a(gb);
	wr8(gb, wIsBossBattle, A);
	gb_call(gb, ADDR_Func_56326);

	// search for the platform's object struct
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		// .loop_objects
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb);
		alu_cp(gb, A, WOLFENBOSS_PLATFORM);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = alu_rra(gb, rd_hli(gb));
			if (CF) {
				// .found_platform_obj
				A = alu_add(gb, gb->cpu.l, OBJ_STATE - OBJ_UNK_01);
				gb->cpu.l = A;
				A = 0x2f;
				wr_hl(gb, A);
				RETURN;
			}
		}
		// .next_object
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF)
			RETURN;
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
}
