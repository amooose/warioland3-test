// Port of src/engine/level/objects/webber.asm
#include "ported.h"

#define WEBBER_PROJECTILE_DESTROY (ADDR_WebberProjectileFunc + 0x54) // .Destroy

// Homes into Wario while falling, and sets itself to be destroyed once it
// lands or its state changes.
PORT(WebberProjectileFunc)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		CALL(MoveObjectRight_Slow);
	else
		CALL(MoveObjectLeft_Slow);
	CALL(MoveObjectDown);

	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));

	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint16_t hl = wCurObjYPos;
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
		wr8(gb, hXPosLo, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_352b, ADDR_WebberProjectileFunc + 0x36);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
			return;
		wr8(gb, wCurObjYPos + 0, rd8(gb, hYPosLo));
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, wCurObjYPos + 1, gb->cpu.a);
		SET_HL(gb, wCurObjYPos + 1);
	}

	// .set_destroy
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(WEBBER_PROJECTILE_DESTROY >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)WEBBER_PROJECTILE_DESTROY;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_682db);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 24;
	hl = REG_HL(gb);
	wr8(gb, hl++, 24); // OBJ_STATE_DURATION
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


#define WB(off) (ADDR_WebberFunc + (off))
#define WB_UPDATE WB(0x70)

static void no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

// ld a, [hl] / and OBJSUBFLAGS_MASK (or mask) / or n / ld [hld], a
static void wb_substate(GB *gb, uint8_t mask, uint8_t n)
{
	A = alu_and(gb, rd_hl(gb), mask);
	A = alu_or(gb, A, n);
	wr_hld(gb, A);
}

// .set_fire_projectile
static void wb_set_fire(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	wb_substate(gb, OBJSUBFLAGS_MASK, 0x2);
	SET_DE(gb, ADDR_Frameset_682b7);
	CALL(SetObjectFramesetPtr);
	A = 64; wr_hli(gb, A);
	RETURN;
}

PORT_JUMP(WebberFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(WB_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)WB_UPDATE; wr_hld(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = (uint8_t)-2; wr_hld(gb, A);
	A = (uint8_t)-16; wr_hld(gb, A);
	A = (uint8_t)-4; wr_hl(gb, A);
	ld_l(gb, OBJ_VAR_1);
	A = rd8(gb, wCurObjYPos + 0); wr_hli(gb, A);
	A = rd8(gb, wCurObjYPos + 1); wr_hli(gb, A);
	A = 4; wr_hl(gb, A);
	wb_set_fire(gb);
}

// .set_shake
static void wb_set_shake(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	wb_substate(gb, OBJSUBFLAGS_MASK, 0x4);
	xor_a(gb);
	wr_hl(gb, A);
	SET_DE(gb, ADDR_Frameset_682a9);
	CALL(SetObjectFramesetPtr);
	A = 48; wr_hli(gb, A);
	RETURN;
}

// ld a, [wGroundShakeCounter] / cp n / jp nc, .set_shake
static bool wb_shaken(GB *gb, uint8_t n)
{
	A = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, A, n);
	if (CF)
		return false;
	wb_set_shake(gb);
	return true;
}

// .set_move_to_start_pos
static void wb_set_move_to_start(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	wb_substate(gb, OBJSUBFLAGS_MASK ^ OBJSUBFLAG_HDIR, 0x3);
	A = 5; wr_hl(gb, A);
	SET_DE(gb, ADDR_Frameset_682b2);
	JUMP(SetObjectFramesetPtr);
}

// .set_idle
static void wb_set_idle(GB *gb)
{
	ld_l(gb, OBJ_SUBSTATE);
	wb_substate(gb, OBJSUBFLAGS_MASK, 0x1);
	SET_DE(gb, ADDR_Frameset_682a4);
	JUMP(SetObjectFramesetPtr);
}

// substate 7: stepped on
static void wb_set_stepped(GB *gb)
{
	ld_l(gb, OBJ_SUBSTATE);
	wb_substate(gb, OBJSUBFLAGS_MASK, 0x7);
	SET_DE(gb, ADDR_Frameset_682d6);
	JUMP(SetObjectFramesetPtr);
}

static void wb_attacked(GB *gb, uint16_t f, uint16_t frameset)
{
	A = (uint8_t)(f >> 8);
	gb->cpu.c = (uint8_t)f;
	gb->cpu.b = 0x02;
	SET_DE(gb, frameset);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.c);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), OBJSUBFLAGS_MASK);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hld(gb, A);
	A = gb->cpu.b;
	wr_hld(gb, A);
	no_actions(gb);
	RETURN;
}

PORT_LOCAL_JUMP(WebberFunc, Update)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		alu_cp(gb, A, OBJSTATE_ATTACKED_LEFT_START);
		if (ZF) {
			wb_attacked(gb, ADDR_Func_3326, ADDR_Frameset_682c2);
			return;
		}
		alu_cp(gb, A, OBJSTATE_ATTACKED_RIGHT_START);
		if (ZF) {
			wb_attacked(gb, ADDR_Func_3317, ADDR_Frameset_682c5);
			return;
		}
		alu_cp(gb, A, OBJSTATE_VANISH_TOUCH);
		if (ZF) {
			// .set_smash_attacked
			xor_a(gb);
			wr_hld(gb, A);
			A = alu_rlca(gb, rd_hl(gb));
			if (CF) {
				JUMP(VanishObject);
				return;
			}
			wb_substate(gb, OBJSUBFLAGS_MASK, 0x5);
			xor_a(gb);
			wr_hl(gb, A);
			SET_DE(gb, ADDR_Frameset_682a9);
			CALL(SetObjectFramesetPtr);
			A = 48; wr_hli(gb, A);
			RETURN;
		}
		xor_a(gb);
		wr_hld(gb, A);
		wb_substate(gb, OBJSUBFLAGS_MASK, 0x6);
		SET_DE(gb, ADDR_Frameset_682d1);
		CALL(SetObjectFramesetPtr);
		A = 50; wr_hli(gb, A);
		ld_l(gb, OBJ_FLAGS);
		res_mhl(gb, OBJFLAG_NO_COLLISION_F);
	}
	// .Default
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), 0xff ^ OBJSUBFLAGS_MASK);
	JUMP_TO(jumptable(gb, WB(0x9e)));
}

// .move_up / .move_down
static void wb_move_y(GB *gb, bool up)
{
	ld_hl(gb, wCurObjFlags);
	if (alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		res_mhl(gb, OBJFLAG_STEPPED_F);
		A = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (!CF)
			RETURN;
		gb->cpu.b = 1;
		if (up) {
			gb_call_ret(gb, ADDR_MoveWarioUp, WB(0x103));
			gb_call_ret(gb, ADDR_HandleUpwardsFloorTransition, WB(0x106));
		} else {
			gb_call_ret(gb, ADDR_MoveWarioDown, WB(0x125));
			gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, WB(0x128));
		}
	} else {
		ld_l(gb, OBJ_FRAME_DURATION);
		A = rd_hl(gb);
		if (alu_and(gb, A, A))
			dec_mhl(gb);
	}
	if (up)
		JUMP(MoveObjectUp);
	else
		JUMP(MoveObjectDown);
}

PORT_LOCAL_JUMP(WebberFunc, MoveBackToStartPosition)
{
	if (wb_shaken(gb, 0x10))
		return;
	ld_hl(gb, wCurObjVar1);
	gb->cpu.e = rd_hl(gb);
	inc_l(gb);
	gb->cpu.d = rd_hl(gb);
	ld_l(gb, OBJ_Y_POS + 1);
	A = rd_hld(gb);
	alu_cp(gb, A, gb->cpu.d);
	if (ZF) {
		A = rd_hl(gb);
		alu_cp(gb, A, gb->cpu.e);
		if (ZF) {
			wb_set_idle(gb);
			return;
		}
	}
	wb_move_y(gb, !CF);
}

PORT_LOCAL_JUMP(WebberFunc, Shake)
{
	A = rd8(gb, wCurObjVar3);
	alu_cp(gb, A, 15);
	if (!ZF) {
		SET_BC(gb, ADDR_Data_604a0);
		CALL(ApplyObjYMovement);
		obj_pos_to_hpos(gb);
		gb_call_ret(gb, ADDR_Func_352b, WB(0x168));
		if (!alu_and(gb, A, A))
			RETURN;
		ld_hl(gb, wCurObjYPos);
		A = rd8(gb, hYPosLo); wr_hli(gb, A);
		A = rd8(gb, hYPosHi); wr_hl(gb, A);
		ld_l(gb, OBJ_SUBSTATE);
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	}
	// .end_shake
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		wb_set_move_to_start(gb);
		return;
	}
	if (wb_shaken(gb, 0x11))
		return;
	RETURN;
}

PORT_LOCAL_JUMP(WebberFunc, Idle)
{
	if (wb_shaken(gb, 0x10))
		return;
	ld_hl(gb, wCurObjFlags);
	if (alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		wb_set_stepped(gb);
		return;
	}
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x7f);
	if (!ZF)
		RETURN;
	ld_l(gb, OBJ_VAR_3);
	if (dec_mhl(gb)) {
		wb_set_fire(gb);
		return;
	}
	inc_l(gb);
	wb_substate(gb, OBJSUBFLAGS_MASK, 0x8);
	SET_DE(gb, ADDR_Frameset_682e2);
	CALL(SetObjectFramesetPtr);
	A = 162; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(WebberFunc, SteppedOn)
{
	ld_hl(gb, wCurObjFlags);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		wb_set_move_to_start(gb);
		return;
	}
	res_mhl(gb, OBJFLAG_STEPPED_F);
	RETURN;
}

PORT_LOCAL_JUMP(WebberFunc, SmashAttacked)
{
	if (wb_shaken(gb, 0x10))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		wb_set_move_to_start(gb);
		return;
	}
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_rlca(gb, rd_hl(gb));
	if (CF)
		RETURN;
	A = rd8(gb, wEffectivePowerUp);
	alu_cp(gb, A, SUPER_JUMP_SLAM_OVERALLS);
	SET_BC(gb, CF ? ADDR_Data_603f0 : ADDR_Data_60490);
	CALL(ApplyObjYMovement);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, WB(0x204));
	if (!alu_and(gb, A, A))
		RETURN;
	A = rd8(gb, wEffectivePowerUp);
	alu_cp(gb, A, SUPER_JUMP_SLAM_OVERALLS);
	if (!CF) {
		JUMP(VanishObject);
		return;
	}
	ld_hl(gb, wCurObjYPos);
	A = rd8(gb, hYPosLo); wr_hli(gb, A);
	A = rd8(gb, hYPosHi); wr_hl(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	set_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

PORT_LOCAL_JUMP(WebberFunc, FireProjectile)
{
	if (wb_shaken(gb, 0x10))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		wb_set_idle(gb);
		return;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, 40);
	if (!ZF)
		RETURN;
	SET_BC(gb, ADDR_ObjParams_WebberProjectile);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL_JUMP(WebberFunc, Climb)
{
	no_actions(gb);
	if (wb_shaken(gb, 0x10))
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		wb_set_move_to_start(gb);
		return;
	}
	A = rd_hl(gb);
	alu_cp(gb, A, 20);
	if (!CF) {
		JUMP(MoveObjectUp);
		return;
	}
	RETURN;
}

PORT_LOCAL_JUMP(WebberFunc, SpecialIdle)
{
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		A = 5;
		wr8(gb, wCurObjVar3, A);
		wb_set_idle(gb);
		return;
	}
	if (wb_shaken(gb, 0x10))
		return;
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F))
		RETURN;
	wb_set_stepped(gb);
}

PORT_LOCAL(WebberProjectileFunc, Destroy)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}
