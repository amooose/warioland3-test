// Port of src/engine/level/objects/pesce.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define STRONG_WATER_CURRENT_UPDATE (ADDR_StrongWaterCurrentFunc + 0x34) // .Func_51d2d

PORT_JUMP(StrongWaterCurrentFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(STRONG_WATER_CURRENT_UPDATE >> 8));
	wr8(gb, hl--, (uint8_t)STRONG_WATER_CURRENT_UPDATE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x0f);
	wr8(gb, hl--, 0xf0); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0xfe); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69965);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x2a;
	wr8(gb, wCurObjVar2, 0x2a);
	CALL(MoveObjectDownByVar2);
	gb->cpu.a = 0x69;
	wr8(gb, wCurObjVar2, 0x69);
	JUMP(MoveObjectLeftByVar2);
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


#define DS(off) (ADDR_DragonflySpawnerFunc + (off))
#define D1(off) (ADDR_Dragonfly1Func + (off))
#define D2(off) (ADDR_Dragonfly2Func + (off))
#define FS(off) (ADDR_FlySpawnerFunc + (off))
#define FL(off) (ADDR_FlyFunc + (off))

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void set_update(GB *gb, uint16_t f)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

// no_actions_for 1 / frame duration 2
static void no_actions_fd2(GB *gb)
{
	no_actions(gb);
	A = 0x02;
	wr8(gb, wCurObjFrameDuration, A);
}

// Looks through the objects (a = b = first offset, h = HIGH(wObj1)) for an
// active one with this id. Returns true (with carry, hl past its flags)
// when found, false at the end (a = b = LOW(wObj8), z).
static bool find_active_obj(GB *gb, uint8_t id)
{
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

// the end of a boss battle: e = the index of the object, for Func_ba42
static void spawner_done(GB *gb, uint8_t e)
{
	ld_hl(gb, wCurObjUnk02);
	gb->cpu.e = e;
	FARCALL(Func_ba42);
}

PORT(DragonflySpawnerFunc)
{
	set_update(gb, DS(0x17));
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = 0x3c;
	wr8(gb, wCurObjStateDuration, A);
}

PORT_LOCAL(DragonflySpawnerFunc, Func_52282)
{
	no_actions_fd2(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	set_update_l(gb, DS(0x2f));
}

PORT_LOCAL_JUMP(DragonflySpawnerFunc, Func_5229a)
{
	no_actions_fd2(gb);
	A = rd8(gb, w1d147);
	alu_cp(gb, A, 0x03);
	if (ZF) {
		set_update(gb, DS(0x6e));
		RETURN;
	}
	xor_a(gb);
	gb->cpu.b = A;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	if (find_active_obj(gb, DRAGONFLY1))
		RETURN;
	SET_BC(gb, ADDR_ObjParams_Dragonfly1);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(DragonflySpawnerFunc, Func_522d9)
{
	no_actions_fd2(gb);
	A = rd8(gb, wDollBoyHammerRange);
	if (!alu_and(gb, A, A))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
	spawner_done(gb, 0x08);
	A = BOSS_DEFEATED;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	xor_a(gb);
	wr8(gb, wIsBossBattle, A);
	A = 0x01;
	wr8(gb, wSwitchStateUpdated, A);
	A = 0x60;
	wr8(gb, wSwitchState, A);
}

PORT(Dragonfly1Func) { set_update(gb, D1(0x0a)); }

PORT_LOCAL_JUMP(Dragonfly1Func, Func_52321)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		JUMP(MoveObjectDown_Fast);
		return;
	}
	A = 0x70; wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	set_update_l(gb, D1(0x24));
	RETURN;
}

// moves Wario along when he stands on it
static void move_stepped_wario(GB *gb, bool right)
{
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F))
		RETURN;
	res_mhl(gb, OBJFLAG_STEPPED_F);
	gb->cpu.b = 0x01;
	if (right)
		JUMP(MoveWarioRight);
	else
		JUMP(MoveWarioLeft);
}

PORT_LOCAL_JUMP(Dragonfly1Func, Func_5233b)
{
	CALL(SetOneWayPlatformAction);
	ld_hl(gb, wCurObjVar1);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, CF ? ADDR_Frameset_699d5 : ADDR_Frameset_699d0);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	A = rd8(gb, w1d147);
	alu_cp(gb, A, 0x03);
	if (ZF) {
		set_update_l(gb, D1(0x9a));
		no_actions(gb);
		RETURN;
	}
	dec_l(gb);
	if (!dec_mhl(gb)) {
		A = 0x70; wr_hli(gb, A);
		A = 0x11; wr_hl(gb, A);
		ld_l(gb, OBJ_SUBSTATE);
		A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
		wr_hl(gb, A);
		A = alu_rlca(gb, A);
		SET_DE(gb, CF ? ADDR_Frameset_699da : ADDR_Frameset_699df);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (CF) {
		CALL(MoveObjectRight);
		move_stepped_wario(gb, true);
	} else {
		CALL(MoveObjectLeft);
		move_stepped_wario(gb, false);
	}
}

PORT_LOCAL_JUMP(Dragonfly1Func, Func_523b1)
{
	no_actions(gb);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (CF)
		JUMP(MoveObjectRight_Fast);
	else
		JUMP(MoveObjectLeft_Fast);
}

PORT_JUMP(Dragonfly2Func)
{
	SET_DE(gb, ADDR_Frameset_699d0);
	CALL(SetObjectFramesetPtr);
	A = 0x28; wr_hli(gb, A);
	set_update_l(gb, D2(0x32));
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJSUBFLAG_UNK_4_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x06; wr_hld(gb, A);
	A = 0xf9; wr_hld(gb, A);
	A = 0xf9; wr_hl(gb, A);
	A = 0x40;
	wr8(gb, wCurObjVar2, A);
	CALL(MoveObjectLeftByVar2);
	A = 0x50;
	wr8(gb, wCurObjVar2, A);
	JUMP(MoveObjectUpByVar2);
}

PORT_LOCAL(Dragonfly2Func, Func_523f2)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	A = 0x50; wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	set_update_l(gb, D2(0x4f));
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
}

PORT_LOCAL_JUMP(Dragonfly2Func, Func_5240f)
{
	CALL(SetOneWayPlatformAction);
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!CF)
		RETURN;
	ld_hl(gb, wCurObjVar1);
	A = rd_hl(gb);
	bool up;
	if (!alu_and(gb, A, A)) {
		up = false;
	} else {
		alu_cp(gb, A, 0x01);
		if (!ZF)
			RETURN;
		up = true;
	}
	ld_l(gb, OBJ_STATE_DURATION);
	if (!dec_mhl(gb)) {
		A = 0x50; wr_hli(gb, A);
		if (up)
			xor_a(gb);
		else
			A = 0x01;
		wr_hl(gb, A);
		RETURN;
	}
	if (up)
		CALL(MoveObjectUp);
	else
		CALL(MoveObjectDown);
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F))
		RETURN;
	res_mhl(gb, OBJFLAG_STEPPED_F);
	gb->cpu.b = 0x01;
	if (up) {
		gb_call_ret(gb, ADDR_MoveWarioUp, D2(0x95));
		JUMP(HandleUpwardsFloorTransition);
	} else {
		gb_call_ret(gb, ADDR_MoveWarioDown, D2(0x77));
		JUMP(HandleDownwardsFloorTransition);
	}
}

PORT(FlySpawnerFunc)
{
	set_update(gb, FS(0x17));
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	A = 0xc8;
	wr8(gb, wCurObjStateDuration, A);
}

PORT_LOCAL(FlySpawnerFunc, Func_52475)
{
	no_actions_fd2(gb);
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, 0xa0);
	if (!CF)
		return;
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	set_update_l(gb, FS(0x39));
	xor_a(gb);
	wr8(gb, wDollBoyActiveBarrels, A);
}

PORT_LOCAL_JUMP(FlySpawnerFunc, Func_52497)
{
	no_actions_fd2(gb);
	A = rd8(gb, w1d147);
	alu_cp(gb, A, 0x03);
	if (ZF) {
		set_update(gb, FS(0x8d));
		RETURN;
	}
	A = rd8(gb, wDollBoyActiveBarrels);
	if (alu_and(gb, A, A))
		RETURN;
	A = rd8(gb, wTransformation);
	if (alu_and(gb, A, A))
		RETURN;
	A = rd8(gb, wAutoMoveState);
	if (alu_and(gb, A, A)) {
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		RETURN;
	}
	xor_a(gb);
	gb->cpu.b = A;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	if (find_active_obj(gb, FLY))
		RETURN;
	SET_BC(gb, ADDR_ObjParams_Fly);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(FlySpawnerFunc, Func_524eb)
{
	no_actions_fd2(gb);
	A = rd8(gb, wDollBoyHammerRange);
	if (!alu_and(gb, A, A))
		return;
	spawner_done(gb, 0x09);
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}

PORT_JUMP(FlyFunc)
{
	set_update(gb, FL(0x20));
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	A = 0x03;
	wr8(gb, wDollBoyActiveBarrels, A);
	play_sfx(gb, SFX_0BE);
	SET_BC(gb, ADDR_ObjParams_Cheese);
	JUMP(CreateObjectFromCurObjPos);
}

PORT_LOCAL_JUMP(FlyFunc, Func_52533)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x5a);
	if (ZF) {
		no_actions(gb);
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb)) {
			JUMP(MoveObjectDown);
			return;
		}
		A = 0x70;
		wr_hl(gb, A);
		A = 0x5b;
		wr8(gb, wCurObjState, A);
		A = 0x01;
		wr8(gb, wDollBoyActiveBarrels, A);
		RETURN;
	}
	alu_cp(gb, A, 0x5b);
	if (ZF) {
		SET_BC(gb, ADDR_Data_604c0);
		CALL(ApplyObjYMovement_Loop);
		A = rd8(gb, wDollBoyActiveBarrels);
		if (alu_and(gb, A, A)) {
			A = alu_rra(gb, rd8(gb, wGlobalCounter));
			if (!CF)
				RETURN;
		}
		CALL(MoveObjectLeft);
		ld_hl(gb, wCurObjStateDuration);
		if (dec_mhl(gb))
			RETURN;
		A = 0x5c;
		wr8(gb, wCurObjState, A);
		RETURN;
	}
	alu_cp(gb, A, 0x5c);
	if (ZF) {
		no_actions(gb);
		ld_l(gb, OBJ_FLAGS);
		if (alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F)) {
			JUMP(MoveObjectUp);
			return;
		}
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
		wr8(gb, wDollBoyActiveBarrels, A);
		RETURN;
	}
	// .asm_52591
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	uint16_t f = CF ? ADDR_Func_33f8 : ADDR_Func_3407;
	A = (uint8_t)(f >> 8);
	gb->cpu.c = (uint8_t)f;
	ld_hl(gb, wCurObjUpdateFunction + 1);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.c);
	ld_l(gb, OBJ_VAR_3);
	xor_a(gb);
	wr_hld(gb, A);
	A = 0x01; wr_hl(gb, A);
	ld_hl(gb, wDollBoyActiveBarrels);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A))
		RETURN;
	A = 0x02;
	wr_hl(gb, A);
	RETURN;
}

PORT_JUMP(CheeseFunc)
{
	no_actions(gb);
	A = rd8(gb, wCurObjState);
	if (alu_and(gb, A, A))
		goto asm_52634;
	A = rd8(gb, wDollBoyActiveBarrels);
	if (!alu_and(gb, A, A))
		goto asm_52638;
	alu_cp(gb, A, 0x02);
	if (ZF) {
		obj_pos_to_hpos(gb);
		gb_call_ret(gb, ADDR_Func_352b, ADDR_CheeseFunc + 0x67);
		A = rd8(gb, wIsInWaterOrSand);
		if (alu_and(gb, A, A)) {
			// .in_water
			no_actions(gb);
			ld_hl(gb, wCurObjFlags);
			if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F))
				goto asm_52634;
			CALL(MoveObjectDown_Slow);
			ld_l(gb, OBJ_STATE_DURATION);
			if (dec_mhl(gb))
				RETURN;
			A = 0x18;
			wr_hl(gb, A);
			gb->cpu.b = PARTICLE_BUBBLES;
			FARCALL(CreateParticle);
			RETURN;
		}
		ld_hl(gb, wCurObjFlags);
		if (alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F)) {
			JUMP(MoveObjectDown_Fast);
			return;
		}
		goto asm_52634;
	}
	alu_cp(gb, A, 0x03);
	if (ZF)
		no_actions(gb);
	// .find_fly_obj
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	if (!find_active_obj(gb, FLY))
		RETURN;
	// .adjust_coord
	inc_l(gb);
	inc_l(gb);
	A = alu_add(gb, rd_hli(gb), 0x0b);
	wr8(gb, wCurObjYPos + 0, A);
	A = alu_adc(gb, rd_hli(gb), 0x00);
	wr8(gb, wCurObjYPos + 1, A);
	A = rd_hli(gb);
	wr8(gb, wCurObjXPos + 0, A);
	A = rd_hl(gb);
	wr8(gb, wCurObjXPos + 1, A);
	RETURN;

asm_52634:
	xor_a(gb);
	wr8(gb, wDollBoyActiveBarrels, A);
asm_52638:
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
	RETURN;
}
