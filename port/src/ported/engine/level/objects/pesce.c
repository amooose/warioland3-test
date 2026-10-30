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

// --- Pesce ---

// PE(rom address in bank $14): PesceFunc's local labels and return
// addresses, checked against warioland3.sym / the ROM's call bytes.
#define PE(addr) (ADDR_PesceFunc + ((addr) - 0x5d2e))
#define PE_51E28 PE(0x5e28)
#define PE_521D0 PE(0x61d0)

static inline void pe_fs(GB *gb, uint16_t fs) { SET_DE(gb, fs); CALL(SetObjectFramesetPtr); }
static inline void pe_fs_jump(GB *gb, uint16_t fs) { SET_DE(gb, fs); JUMP(SetObjectFramesetPtr); }

// The stop_music2 macro
static void pe_stop_music2(GB *gb)
{
	A = 0xff;
	wr8(gb, hMusicIDBackup + 0, A);
	wr8(gb, hMusicID + 0, A);
	A = 0x00;
	wr8(gb, hMusicIDBackup + 1, A);
	wr8(gb, hMusicID + 1, A);
}

// One byte of a position, moved by op: 0 = copy, 1 = sub v / sbc 0,
// 2 = add v / adc 0 (hi = the high byte, using the carry)
static inline void pe_op(GB *gb, int op, uint8_t v, bool hi)
{
	if (op == 1)
		A = hi ? alu_sbc(gb, A, 0) : alu_sub(gb, A, v);
	else if (op == 2)
		A = hi ? alu_adc(gb, A, 0) : alu_add(gb, A, v);
}

// 4 bytes from [hl] (y then x, hl already set) -> hYPos / hXPos;
// last_hli: the last read is [hli] rather than [hl]
static void pe_hpos(GB *gb, int yop, uint8_t dy, int xop, uint8_t dx, bool last_hli)
{
	A = rd_hli(gb); pe_op(gb, yop, dy, false); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); pe_op(gb, yop, dy, true); wr8(gb, hYPosHi, A);
	A = rd_hli(gb); pe_op(gb, xop, dx, false); wr8(gb, hXPosLo, A);
	A = last_hli ? rd_hli(gb) : rd_hl(gb); pe_op(gb, xop, dx, true); wr8(gb, hXPosHi, A);
}

// ld a, [w1d147] / cp 1 / cp 2 (/ cp 3), else xor a / ld [w1d147], a;
// then sla b / ld_obj_frameset. Returns the level (0 to n-1).
static int pe_pick(GB *gb, int n, const uint16_t (*fs)[2])
{
	int level = 0;
	A = rd8(gb, w1d147);
	for (int i = 1; i < n; i++) {
		alu_cp(gb, A, (uint8_t)i);
		if (ZF) {
			level = i;
			break;
		}
	}
	if (!level) {
		xor_a(gb);
		wr8(gb, w1d147, A);
	}
	gb->cpu.b = alu_sla(gb, gb->cpu.b);
	SET_DE(gb, CF ? fs[level][1] : fs[level][0]);
	return level;
}

PORT(PesceFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, PE_51E28);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJSUBFLAG_UNK_4_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x0f; wr_hld(gb, A);
	A = 0xf0; wr_hld(gb, A);
	A = 0xfe; wr_hl(gb, A);
	pe_fs(gb, ADDR_Frameset_6990a);
	xor_a(gb);
	wr8(gb, w1d147, A);
	wr8(gb, wDollBoyHammerRange, A);
	wr_hli(gb, A);
	wr_hli(gb, A);
	A = 0x0a; wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	CALL(MoveObjectDownByVar2);
	A = 0x3c;
	wr8(gb, wCurObjVar2, A);
	A = BOSS_REGULAR;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	A = TRUE;
	wr8(gb, wIsBossBattle, A);
}

// .asm_52155
static void pe_52155(GB *gb)
{
	static const uint16_t fs[4][2] = {
		{ADDR_Frameset_6990a, ADDR_Frameset_69942},
		{ADDR_Frameset_69918, ADDR_Frameset_69950},
		{ADDR_Frameset_6991f, ADDR_Frameset_69957},
		{ADDR_Frameset_69926, ADDR_Frameset_6995e},
	};
	gb->cpu.b = rd8(gb, wCurObjSubState);
	pe_pick(gb, 4, fs);
	JUMP(SetObjectFramesetPtr);
}

// .asm_51e67
static void pe_51e67(GB *gb)
{
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	pe_52155(gb);
}

// .asm_52068 (h = wCurObj)
static void pe_52068(GB *gb)
{
	static const uint16_t fs[4][2] = {
		{ADDR_Frameset_69981, ADDR_Frameset_6997a},
		{ADDR_Frameset_6998f, ADDR_Frameset_69988},
		{ADDR_Frameset_6999d, ADDR_Frameset_69996},
		{ADDR_Frameset_699ab, ADDR_Frameset_699a4},
	};
	static const uint8_t cs[4] = {0x17, 0x1b, 0x1f, 0x25};
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_xor(gb, rd_hl(gb), OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
	gb->cpu.b = A;
	// the ld c comes before the sla, and doesn't touch the flags
	uint8_t a = rd8(gb, w1d147);
	int level = (a == 1 || a == 2 || a == 3) ? a : 0;
	gb->cpu.c = cs[level];
	pe_pick(gb, 4, fs);
	CALL(SetObjectFramesetPtr);
	wr_hl(gb, gb->cpu.c);
	RET();
}

// .asm_520c5
static void pe_520c5(GB *gb)
{
	static const uint16_t fs[3][2] = {
		{ADDR_Frameset_69911, ADDR_Frameset_69949},
		{ADDR_Frameset_69934, ADDR_Frameset_6996c},
		{ADDR_Frameset_6993b, ADDR_Frameset_69973},
	};
	A = 0x01;
	wr8(gb, wCurObjVar1, A);
	gb->cpu.b = rd8(gb, wCurObjSubState);
	pe_pick(gb, 3, fs);
	JUMP(SetObjectFramesetPtr);
}

// .asm_52108
static void pe_52108(GB *gb)
{
	static const uint16_t fs[3][2] = {
		{ADDR_Frameset_699b2, ADDR_Frameset_699c1},
		{ADDR_Frameset_699b7, ADDR_Frameset_699c6},
		{ADDR_Frameset_699bc, ADDR_Frameset_699cb},
	};
	play_sfx(gb, SFX_0AD);
	gb->cpu.b = rd8(gb, wCurObjSubState);
	pe_pick(gb, 3, fs);
	CALL(SetObjectFramesetPtr);
	A = 0x3c; wr_hli(gb, A);
	A = 0x05; wr_hl(gb, A);
	RET();
}

// .Func_521a5: de = the y/x screen position of the first active object
// with an id >= CHEESE, or $d0d0
static void pe_521a5(GB *gb)
{
	xor_a(gb);
	gb->cpu.b = A;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb); // OBJ_ID
		alu_cp(gb, A, CHEESE);
		if (!CF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = alu_rra(gb, rd_hl(gb));
			if (CF) {
				A = alu_add(gb, gb->cpu.l, 0x0d);
				gb->cpu.l = A;
				A = rd_hli(gb);
				gb->cpu.d = A;
				gb->cpu.e = rd_hl(gb);
				return;
			}
		}
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF) {
			gb->cpu.d = 0xd0;
			gb->cpu.e = 0xd0;
			return;
		}
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
}

// .asm_51f46
static void pe_51f46(GB *gb)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); A = alu_sub(gb, A, 0x04); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); A = alu_sbc(gb, A, 0x00); wr8(gb, hYPosHi, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!CF) {
		A = rd_hli(gb); A = alu_sub(gb, A, 0x10); wr8(gb, hXPosLo, A);
		A = rd_hli(gb); A = alu_sbc(gb, A, 0x00); wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3513, PE(0x5f66));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectLeft);
			return;
		}
	} else {
		A = rd_hli(gb); A = alu_add(gb, A, 0x0f); wr8(gb, hXPosLo, A);
		A = rd_hli(gb); A = alu_adc(gb, A, 0x00); wr8(gb, hXPosHi, A);
		gb_call_ret(gb, ADDR_Func_3513, PE(0x5f79));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectRight);
			return;
		}
	}
	// .asm_51f7d
	ld_hl(gb, wCurObjVar2);
	if (dec_mhl(gb))
		RETURN;
	A = 0x3c; wr_hld(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	pe_52068(gb);
}

// .asm_51d76 (hl = OBJ_STATE)
static void pe_51d76(GB *gb)
{
	xor_a(gb);
	wr_hl(gb, A);
	wr8(gb, wCurObjStateDuration, A);
	play_sfx(gb, SFX_0BB);
	ld_hl(gb, w1d147);
	A = inc_mhl(gb);
	A = rd_hl(gb);
	alu_cp(gb, A, 0x01);
	if (ZF) {
		A = 0xef;
	} else if (alu_cp(gb, A, 0x02), ZF) {
		A = 0xeb;
	} else if (alu_cp(gb, A, 0x03), ZF) {
		// .asm_51da6
		A = 0xe6;
		wr8(gb, wCurObjCollBoxTop, A);
		set_update(gb, PE_521D0);
		ld_l(gb, OBJ_INTERACTION_TYPE);
		heavy_or(gb, OBJ_INTERACTION_51);
		wr_hld(gb, A);
		pe_stop_music2(gb);
		pe_51e67(gb);
		return;
	} else {
		xor_a(gb);
		wr_hl(gb, A);
		A = 0xf2;
	}
	// .asm_51da0
	wr8(gb, wCurObjCollBoxTop, A);
	pe_51e67(gb);
}

// .asm_51dcb (hl = OBJ_STATE)
static void pe_51dcb(GB *gb)
{
	pe_stop_music2(gb);
	play_sfx(gb, SFX_FAT_WALK);
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_Y_POS);
	A = rd_hli(gb); A = alu_add(gb, A, 0x04); wr8(gb, wWarioYPos + 1, A);
	A = rd_hli(gb); A = alu_adc(gb, A, 0x00); wr8(gb, wWarioYPos + 0, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (CF) {
		// .asm_51e0e
		A = rd_hli(gb); wr8(gb, wWarioXPos + 1, A);
		A = rd_hl(gb); wr8(gb, wWarioXPos + 0, A);
		pe_fs(gb, ADDR_Frameset_69a15);
		A = 0x0d; wr_hli(gb, A);
		A = 0x02; wr_hl(gb, A);
		A = TRUE;
		wr8(gb, wIsTurning, A);
		RETURN;
	}
	A = rd_hli(gb); A = alu_sub(gb, A, 0x14); wr8(gb, wWarioXPos + 1, A);
	A = rd_hl(gb); A = alu_sbc(gb, A, 0x00); wr8(gb, wWarioXPos + 0, A);
	pe_fs(gb, ADDR_Frameset_6992d);
	A = 0x3c; wr_hli(gb, A);
	A = 0x03; wr_hl(gb, A);
	RETURN;
}

// .asm_51e5d
static void pe_51e5d(GB *gb)
{
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_GRABBED_F);
	ld_l(gb, OBJ_STATE_DURATION);
	if (dec_mhl(gb)) {
		pe_51f46(gb);
		return;
	}
	pe_51e67(gb);
}

// .asm_51e6e
static void pe_51e6e(GB *gb)
{
	ld_l(gb, OBJ_STATE_DURATION);
	if (dec_mhl(gb))
		RETURN;
	ld_l(gb, OBJ_X_POS);
	A = rd_hli(gb); A = alu_sub(gb, A, 0x14); wr8(gb, wWarioXPos + 1, A);
	A = rd_hl(gb); A = alu_sbc(gb, A, 0x00); wr8(gb, wWarioXPos + 0, A);
	pe_fs(gb, ADDR_Frameset_6992d);
	A = 0x3c; wr_hli(gb, A);
	A = 0x03; wr_hl(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_HDIR_F);
	RETURN;
}

// .asm_51e97
static void pe_51e97(GB *gb)
{
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x5f);
	if (ZF)
		play_sfx(gb, SFX_WATER_SURFACE);
	ld_l(gb, OBJ_STATE_DURATION);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
		RETURN;
	}
	// .asm_51eae
	ld_l(gb, OBJ_Y_POS);
	pe_hpos(gb, 2, 0x06, 0, 0, false);
	gb_call_ret(gb, ADDR_Func_352b, PE(0x5ec3));
	if (!alu_and(gb, A, A)) {
		A = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
		if (!ZF)
			RETURN;
		gb->cpu.b = 0x01;
		CALL(MoveWarioDown);
		JUMP(MoveObjectDown);
		return;
	}
	// .asm_51ed4
	xor_a(gb);
	wr8(gb, wAutoMoveState, A);
	play_sfx(gb, SFX_0B9);
	ld_hl(gb, wCurObjVar1);
	A = 0x04;
	wr_hli(gb, A);
	wr_hl(gb, A);
	CALL(MoveObjectUpByVar2);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	pe_fs_jump(gb, CF ? ADDR_Frameset_69942 : ADDR_Frameset_6990a);
}

// .asm_51efc
static void pe_51efc(GB *gb)
{
	A = rd8(gb, wCurObjFramesetOffset);
	alu_cp(gb, A, 0x02);
	if (ZF) {
		A = rd8(gb, wCurObjFrameDuration);
		alu_cp(gb, A, 0x01);
		if (ZF)
			play_sfx(gb, SFX_0AE);
	}
	// .asm_51f12
	ld_hl(gb, wCurObjYPos);
	pe_hpos(gb, 1, 0x18, 0, 0, true);
	gb_call_ret(gb, ADDR_Func_358b, PE(0x5f28));
	A = rd8(gb, wIsInWaterOrSand);
	if (alu_and(gb, A, A)) {
		gb->cpu.b = rd8(gb, wWarioScreenYPos);
		A = rd8(gb, wCurObjScreenYPos);
		alu_cp(gb, A, gb->cpu.b);
		if (!CF) {
			A = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
			if (ZF) {
				ld_hl(gb, wCurObjVar3);
				inc_mhl(gb);
				CALL(MoveObjectUp);
			}
		}
	}
	pe_51f46(gb);
}

// .asm_51f8a
static void pe_51f8a(GB *gb)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			RETURN;
		pe_52155(gb);
		return;
	}
	// .asm_51f96
	ld_l(gb, OBJ_VAR_3);
	A = rd_hl(gb);
	bool right;
	if (alu_and(gb, A, A)) {
		A = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (!CF) {
			dec_mhl(gb);
			CALL(MoveObjectDown);
		}
		// .asm_51fa6
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		right = CF;
	} else {
		// .asm_51fae
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		gb->cpu.c = A;
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		right = CF;
		A = rd8(gb, wWaterInteraction);
		if (alu_and(gb, A, A)) {
			A = gb->cpu.c;
			alu_cp(gb, A, gb->cpu.b);
			if (right ? CF : !CF) {
				pe_520c5(gb);
				return;
			}
		}
	}
	pe_521a5(gb);
	A = gb->cpu.d;
	alu_cp(gb, A, 0xd0);
	if (!ZF) {
		gb->cpu.c = 0x2a;
		A = alu_add(gb, A, gb->cpu.c);
		gb->cpu.d = A;
		A = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
		alu_cp(gb, A, gb->cpu.d);
		if (!CF) {
			A = alu_sub(gb, A, gb->cpu.d);
			alu_cp(gb, A, 0x18);
			if (CF) {
				if (!right) {
					A = alu_add(gb, gb->cpu.e, gb->cpu.c);
					gb->cpu.e = A;
					A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x0a);
					A = alu_sub(gb, A, gb->cpu.e);
				} else {
					A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x4a);
					gb->cpu.b = A;
					A = alu_add(gb, gb->cpu.e, gb->cpu.c);
					A = alu_sub(gb, A, gb->cpu.b);
				}
				alu_cp(gb, A, 0x30);
				if (CF) {
					pe_52108(gb);
					return;
				}
			}
		}
	}
	// .asm_51ff1 / .asm_52042
	ld_hl(gb, wCurObjYPos);
	if (!right) {
		pe_hpos(gb, 1, 0x04, 1, 0x10, true);
		gb_call_ret(gb, ADDR_Func_3513, PE(0x600b));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectLeft_Slow);
			return;
		}
	} else {
		pe_hpos(gb, 1, 0x04, 2, 0x0f, true);
		gb_call_ret(gb, ADDR_Func_3513, PE(0x605c));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectRight_Slow);
			return;
		}
	}
	// .asm_52060
	ld_hl(gb, wCurObjVar2);
	if (dec_mhl(gb))
		RETURN;
	A = 0x3c; wr_hl(gb, A);
	pe_52068(gb);
}

PORT_LOCAL_JUMP(PesceFunc, Func_51e28)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	ld_l(gb, OBJ_STATE);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		alu_cp(gb, A, 0x2a);
		if (ZF) {
			pe_51dcb(gb);
			return;
		}
		A = alu_and(gb, A, 0xfe);
		alu_cp(gb, A, 0x08);
		if (ZF) {
			pe_51d76(gb);
			return;
		}
		xor_a(gb);
		wr_hl(gb, A);
	}
	// .asm_51e40
	ld_l(gb, OBJ_VAR_1);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		pe_51f8a(gb);
		return;
	}
	static void (*const phases[])(GB *) = {
		NULL, pe_51efc, pe_51e6e, pe_51e97, NULL, pe_51e5d,
	};
	for (uint8_t i = 1; i <= 5; i++) {
		alu_cp(gb, A, i);
		if (!ZF)
			continue;
		if (i == 4) {
			// .asm_51e91
			no_actions(gb);
			RETURN;
		}
		phases[i](gb);
		return;
	}
	RETURN;
}

PORT_LOCAL_JUMP(PesceFunc, asm_51d76) { pe_51d76(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51dcb) { pe_51dcb(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51e5d) { pe_51e5d(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51e6e) { pe_51e6e(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51e97) { pe_51e97(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51efc) { pe_51efc(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51f46) { pe_51f46(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_51f8a) { pe_51f8a(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_52068) { pe_52068(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_520c5) { pe_520c5(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_52108) { pe_52108(gb); }
PORT_LOCAL_JUMP(PesceFunc, asm_52155) { pe_52155(gb); }

PORT_LOCAL_JUMP(PesceFunc, Func_521d0)
{
	ld_hl(gb, wCurObjVar1);
	A = rd_hl(gb);
	if (!alu_and(gb, A, A)) {
		// .asm_52239
		ld_l(gb, OBJ_Y_POS);
		pe_hpos(gb, 2, 0x06, 0, 0, false);
		gb_call_ret(gb, ADDR_Func_352b, PE(0x624e));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectDown_Slow);
			return;
		}
		A = alu_rlca(gb, rd8(gb, wCurObjSubState));
		if (CF) {
			// .asm_52265
			A = 0x02;
			wr8(gb, wCurObjVar1, A);
			RETURN;
		}
		pe_fs(gb, ADDR_Frameset_699a4);
		A = 0x25; wr_hli(gb, A);
		A = 0x01; wr_hl(gb, A);
		RETURN;
	}
	alu_cp(gb, A, 0x01);
	if (ZF) {
		// .asm_5222b
		ld_l(gb, OBJ_STATE_DURATION);
		if (dec_mhl(gb))
			RETURN;
		inc_l(gb);
		A = 0x02; wr_hl(gb, A);
		pe_fs_jump(gb, ADDR_Frameset_6995e);
		return;
	}
	alu_cp(gb, A, 0x02);
	if (ZF) {
		// .asm_521e9
		ld_hl(gb, wCurObjYPos);
		pe_hpos(gb, 1, 0x30, 0, 0, true);
		gb_call_ret(gb, ADDR_Func_3513, PE(0x61ff));
		if (!alu_and(gb, A, A)) {
			JUMP(MoveObjectLeft_Slow);
			return;
		}
		ld_hl(gb, wCurObjInteractionType);
		heavy_or(gb, OBJ_INTERACTION_51);
		wr_hli(gb, A);
		A = 0x03;
		wr8(gb, wCurObjVar1, A);
		ld_hl(gb, wCurObjUnk02);
		gb->cpu.e = 0x07;
		FARCALL(Func_ba42);
		A = 0x01;
		wr8(gb, wDollBoyHammerRange, A);
		RETURN;
	}
	alu_cp(gb, A, 0x03);
	if (ZF) {
		// .asm_521e4
		xor_a(gb);
		wr8(gb, wCurObjFlags, A);
	}
	RETURN;
}
