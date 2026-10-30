// Port of src/engine/level/objects/shoot.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Sets up a goal counter, with update as its update function.
static void goal_counter(GB *gb, uint16_t update)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
	wr8(gb, wCurObjFrame, 0x3c);
	// move up $58 pixels
	gb->cpu.a = 0x58;
	wr8(gb, wCurObjVar2, 0x58);
	JUMP(MoveObjectUpByVar2);
}

PORT_JUMP(ShootGoalCounterFunc) { goal_counter(gb, ADDR_ShootGoalCounterFunc + 0x20); }
PORT_JUMP(WarioGoalCounterFunc) { goal_counter(gb, ADDR_WarioGoalCounterFunc + 0x20); }

// a = [bc + var3], the next entry of a $80-terminated table of y offsets
// (stays on the last one); b = a
static void kick_y_offset(GB *gb)
{
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	gb->cpu.b = gb->cpu.a;
}

// moves Wario up by b: xor a / sub b / ld b, a / call MoveWarioUp /
// jp HandleUpwardsFloorTransition
static void kick_wario_up(GB *gb, uint16_t ret)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.a = alu_sub(gb, 0, gb->cpu.b);
	gb->cpu.b = gb->cpu.a;
	gb_call_ret(gb, ADDR_MoveWarioUp, ret);
	JUMP(HandleUpwardsFloorTransition);
}

// bc = the y offsets: moves the object and Wario along them
PORT_JUMP(DoPreKickingJumpYMovement)
{
	kick_y_offset(gb);
	gb->cpu.l = OBJ_Y_POS;
	uint16_t hl = REG_HL(gb);
	alu_cp(gb, gb->cpu.a, (uint8_t)-128);
	gb->cpu.c = rd8(gb, hl);
	if (!(gb->cpu.f & FLAG_C)) {
		// .moving_up
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (!(gb->cpu.f & FLAG_C))
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		kick_wario_up(gb, ADDR_DoPreKickingJumpYMovement + 0x2e);
		return;
	}
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	gb_call_ret(gb, ADDR_MoveWarioDown, ADDR_DoPreKickingJumpYMovement + 0x20);
	JUMP(HandleDownwardsFloorTransition);
}

PORT_JUMP(ApplyYOffsetOnKickedWarioBall)
{
	kick_y_offset(gb);
	gb_call_ret(gb, ADDR_MoveWarioDown, ADDR_ApplyYOffsetOnKickedWarioBall + 0x14);
	JUMP(HandleDownwardsFloorTransition);
}

PORT_JUMP(Func_5725b) { kick_wario_up(gb, ADDR_Func_5725b + 0x06); }

// --- GK Tortoise platform ---

#define GKTP_MOVE_LEFT (ADDR_GKTortoisePlatformFunc + 0x35)
#define GKTP_MOVE_UP_AND_DOWN (ADDR_GKTortoisePlatformFunc + 0x58)

static inline void res_bit(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << bit)));
}

// ld l, OBJ_UPDATE_FUNCTION + 1 / store f, big-endian downwards
static uint16_t set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

PORT_JUMP(GKTortoisePlatformFunc)
{
	uint16_t hl = wCurObjFlags;
	res_bit(gb, hl, OBJFLAG_INVISIBLE_F);
	hl = set_update(gb, hl, GKTP_MOVE_LEFT);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~(1 << OBJSUBFLAG_UNINITIALISED_F)) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	gb->cpu.a = 5;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)-6;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69fdc);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 32;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = 60;
	wr8(gb, hl++, gb->cpu.a); // OBJ_VAR_1
	// move 64 pixels right and 16 pixels down
	gb->cpu.a = 64;
	wr8(gb, hl, gb->cpu.a); // OBJ_VAR_2
	SET_HL(gb, hl);
	CALL(MoveObjectRightByVar2);
	gb->cpu.a = 16;
	wr8(gb, wCurObjVar2, gb->cpu.a);
	JUMP(MoveObjectDownByVar2);
}

PORT_LOCAL_JUMP(GKTortoisePlatformFunc, MoveLeft)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
		wr8(gb, wCurObjAction, gb->cpu.a);
		JUMP(MoveObjectLeft);
		return;
	}
	hl = LO(hl, alu_inc(gb, (uint8_t)hl));
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl))); // OBJ_VAR_1
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	hl = LO(hl, alu_dec(gb, (uint8_t)hl));
	gb->cpu.a = 176;
	wr8(gb, hl, gb->cpu.a); // OBJ_STATE_DURATION
	set_update(gb, hl, GKTP_MOVE_UP_AND_DOWN);
	RET();
}

PORT_LOCAL_JUMP(GKTortoisePlatformFunc, MoveUpAndDown)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjVar1;
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl))); // OBJ_STATE_DURATION
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, 176);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0); // OBJ_VAR_1
			wr8(gb, wCurObjState, 0);
			RET();
			return;
		}
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
			res_bit(gb, hl, OBJFLAG_STEPPED_F);
			gb->cpu.b = 1;
			CALL(MoveWarioDown);
			CALL(HandleDownwardsFloorTransition);
		}
		JUMP(MoveObjectDown);
		return;
	}
	// moving up
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl))); // OBJ_STATE_DURATION
	if (gb->cpu.f & FLAG_Z) {
		hl = LO(hl, alu_inc(gb, (uint8_t)hl));
		SET_HL(gb, hl);
		gb->cpu.a = 0x01;
		wr8(gb, hl, gb->cpu.a); // OBJ_VAR_1
		RET();
		return;
	}
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		res_bit(gb, hl, OBJFLAG_STEPPED_F);
		gb->cpu.b = 1;
		CALL(MoveWarioUp);
		CALL(HandleUpwardsFloorTransition);
	}
	JUMP(MoveObjectUp);
}

// --- Asm-mirroring helpers: each is one instruction on the real registers ---

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
static inline void and_a(GB *gb) { alu_and(gb, A, A); }
static inline void cp_a(GB *gb, uint8_t v) { alu_cp(gb, A, v); }
static inline void inc_l(GB *gb) { gb->cpu.l = alu_inc(gb, gb->cpu.l); }
static inline void inc_e(GB *gb) { gb->cpu.e = alu_inc(gb, gb->cpu.e); }
static inline uint8_t dec_mhl(GB *gb) { uint8_t v = alu_dec(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline void set_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) | (1 << bit))); }
static inline void res_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) & ~(1 << bit))); }
static inline uint8_t rd_de(GB *gb) { return rd8(gb, REG_DE(gb)); }
static inline void set_frameset(GB *gb, uint16_t fs) { SET_DE(gb, fs); CALL(SetObjectFramesetPtr); }

// rst JumpTable, with the table at `table`.
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// One byte of a position, moved by op: 0 = copy, 1 = sub v / sbc 0,
// 2 = add v / adc 0 (hi = the high byte, using the carry)
static inline void pos_op(GB *gb, int op, uint8_t v, bool hi)
{
	if (op == 1)
		A = hi ? alu_sbc(gb, A, 0) : alu_sub(gb, A, v);
	else if (op == 2)
		A = hi ? alu_adc(gb, A, 0) : alu_add(gb, A, v);
}

// ld de, wCurObjYPos, then 4 bytes [de] -> [hli] (y then x), with inc e
// between them
static void copy_pos_de_to_hli(GB *gb, int yop, uint8_t yv, int xop, uint8_t xv)
{
	SET_DE(gb, wCurObjYPos);
	A = rd_de(gb); pos_op(gb, yop, yv, false); wr_hli(gb, A);
	inc_e(gb);
	A = rd_de(gb); pos_op(gb, yop, yv, true); wr_hli(gb, A);
	inc_e(gb);
	A = rd_de(gb); pos_op(gb, xop, xv, false); wr_hli(gb, A);
	inc_e(gb);
	A = rd_de(gb); pos_op(gb, xop, xv, true); wr_hli(gb, A);
}

// 4 bytes [hli] (the object's y then x) -> Wario's position
static void obj_pos_to_wario(GB *gb, int yop, uint8_t yv, int xop, uint8_t xv)
{
	A = rd_hli(gb); pos_op(gb, yop, yv, false); wr8(gb, wWarioYPos + 1, A);
	A = rd_hli(gb); pos_op(gb, yop, yv, true); wr8(gb, wWarioYPos + 0, A);
	A = rd_hli(gb); pos_op(gb, xop, xv, false); wr8(gb, wWarioXPos + 1, A);
	A = rd_hli(gb); pos_op(gb, xop, xv, true); wr8(gb, wWarioXPos + 0, A);
}

// --- GK Tortoise ---

#define SHOOTBALL_WARIO_KICKED 5
#define SHOOTBALL_WARIO_GOAL 6
#define GKTORTOISEPHASE_1 1
#define GKTORTOISEPHASE_5 5
#define GKTORTOISEPHASE_DUCKING 7
#define GKTORTOISEPHASE_8 8
#define GKTORTOISEPHASE_9 9
#define NUM_WARIO_GOALS_NEEDED 3

#define GK(off) (ADDR_GKTortoiseFunc + (off))
#define GK_MAIN GK(0x42)  // .Main
#define GK_TABLE GK(0x5f) // the table after .Main's rst JumpTable

PORT_JUMP(GKTortoiseFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update(gb, REG_HL(gb), GK_MAIN);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJSUBFLAG_UNK_4_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 5; wr_hld(gb, A);
	A = (uint8_t)-6; wr_hld(gb, A);
	set_frameset(gb, ADDR_Frameset_69fed);
	inc_l(gb);
	xor_a(gb); // GKTORTOISEPHASE_0
	wr_hli(gb, A); // OBJ_VAR_1
	// move 64 pixels right and 16 pixels down
	A = 64; wr_hl(gb, A);
	CALL(MoveObjectRightByVar2);
	A = 16; wr8(gb, wCurObjVar2, A);
	JUMP(MoveObjectDownByVar2);
}

static void gk_set_ducking(GB *gb)
{
	set_frameset(gb, ADDR_Frameset_6a017);
	inc_l(gb);
	A = GKTORTOISEPHASE_DUCKING; wr_hl(gb, A); // OBJ_VAR_1
	A = (uint8_t)-10; wr8(gb, wCurObjCollBoxTop, A);
}

PORT_LOCAL_JUMP(GKTortoiseFunc, Main)
{
	A = rd8(gb, wTransformation);
	and_a(gb);
	if (!ZF) {
		A = (1 << NO_ACTIONS_F) | 1;
		wr8(gb, wCurObjAction, A);
	}
	// GK Tortoise will automatically duck
	// if Shoot kicks Wario towards net
	A = rd8(gb, wShootBallState);
	cp_a(gb, SHOOTBALL_WARIO_KICKED);
	if (ZF) {
		A = rd8(gb, wCurObjVar1);
		cp_a(gb, GKTORTOISEPHASE_DUCKING);
		if (!ZF) {
			gk_set_ducking(gb);
			RETURN;
		}
	}
	A = rd8(gb, wCurObjVar1);
	JUMP_TO(jumptable(gb, GK_TABLE));
}

PORT_LOCAL(GKTortoiseFunc, Func_56edd)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
	ld_hl(gb, wCurObjUnk02);
	gb->cpu.e = 0x08;
	FARCALL(Func_ba42);
	A = BOSS_DEFEATED;
	wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	xor_a(gb);
	wr8(gb, wIsBossBattle, A);
}

static void gk_56fd2(GB *gb)
{
	A = 0x07; wr8(gb, wShootBallState, A);
	set_frameset(gb, ADDR_Frameset_69fba);
	inc_l(gb);
	A = 0x06; wr_hl(gb, A);
	A = (uint8_t)-20; wr8(gb, wCurObjCollBoxTop, A);
	play_sfx(gb, SFX_FAT_WALK);
}

PORT_LOCAL(GKTortoiseFunc, Ducking)
{
	ld_hl(gb, wCurObjFramesetOffset);
	A = rd_hld(gb);
	cp_a(gb, 10);
	if (ZF) {
		// .duck_delay
		A = rd_hl(gb); // OBJ_FRAME_DURATION
		and_a(gb);
		if (!ZF)
			return;
		gk_56fd2(gb);
		return;
	}
	cp_a(gb, 6);
	if (CF)
		return;
	// add delay if Wario still didn't enter net
	A = rd8(gb, wShootBallState);
	cp_a(gb, SHOOTBALL_WARIO_GOAL);
	if (ZF)
		return;
	A = 16; wr_hl(gb, A); // OBJ_FRAME_DURATION
}

// .GetShootPosPtr: hl = the first active Shoot's y position
static void gk_get_shoot_pos_ptr(GB *gb)
{
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb); // OBJ_ID
		cp_a(gb, SHOOT);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = rd_hli(gb); // OBJ_FLAGS
			A = alu_rra(gb, A);
			if (CF) {
				inc_l(gb);
				inc_l(gb);
				return;
			}
		}
		A = gb->cpu.b;
		cp_a(gb, (uint8_t)wObj8);
		if (ZF)
			return;
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
}

// .Func_57170: stops the first active Shoot's actions for 8 frames
static void gk_57170(GB *gb)
{
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_ID);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		A = rd_hl(gb); // OBJ_ID
		cp_a(gb, SHOOT);
		if (ZF) {
			A = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = A;
			A = rd_hl(gb); // OBJ_FLAGS
			A = alu_rra(gb, A);
			if (CF) {
				A = alu_add(gb, gb->cpu.l, OBJ_ACTION - OBJ_FLAGS);
				gb->cpu.l = A;
				A = (1 << NO_ACTIONS_F) | 8;
				wr_hl(gb, A);
				return;
			}
		}
		A = gb->cpu.b;
		cp_a(gb, (uint8_t)wObj8);
		if (ZF)
			return;
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
}

// .set_shoot_pos_11x_16y
static void gk_set_shoot_pos_11x_16y(GB *gb)
{
	gk_get_shoot_pos_ptr(gb);
	copy_pos_de_to_hli(gb, 1, 16, 1, 11);
}

// .asm_56f7d (hl = OBJ_STATE)
static void gk_56f7d(GB *gb)
{
	xor_a(gb);
	wr_hl(gb, A);
	set_frameset(gb, ADDR_Frameset_69fcf);
	inc_l(gb);
	A = 0x02; wr_hl(gb, A);
	A = (uint8_t)-10; wr8(gb, wCurObjCollBoxTop, A);
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
}

// .asm_57009
static void gk_57009(GB *gb)
{
	set_frameset(gb, ADDR_Frameset_69fa9);
	inc_l(gb);
	A = GKTORTOISEPHASE_1; wr_hl(gb, A); // OBJ_VAR_1
	xor_a(gb);
	wr8(gb, wCurObjState, A);
	A = (uint8_t)-20; wr8(gb, wCurObjCollBoxTop, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_NO_COLLISION_F);
}

static void gk_56f2e(GB *gb)
{
	A = rd8(gb, wShootBallState);
	cp_a(gb, 0x02);
	if (ZF) {
		// .asm_56f93
		A = 0x03; wr8(gb, wShootBallState, A);
		set_frameset(gb, ADDR_Frameset_69fba);
		inc_l(gb);
		A = 0x04; wr_hl(gb, A);
		return;
	}
	ld_hl(gb, wCurObjAction);
	xor_a(gb);
	wr_hld(gb, A);
	A = rd_hl(gb); // OBJ_STATE
	and_a(gb);
	if (ZF)
		return;
	cp_a(gb, 0x2f);
	if (ZF) {
		// .asm_56fa3
		xor_a(gb);
		wr_hl(gb, A); // OBJ_STATE
		set_frameset(gb, ADDR_Frameset_69ff2);
		inc_l(gb);
		A = GKTORTOISEPHASE_5; wr_hl(gb, A); // OBJ_VAR_1
		play_sfx(gb, SFX_FAT_WALK);
		// .set_wario_pos_11x_16y
		ld_l(gb, OBJ_Y_POS);
		obj_pos_to_wario(gb, 1, 16, 1, 11);
		return;
	}
	A = alu_and(gb, A, 0xfe);
	cp_a(gb, 0x08);
	if (!ZF) {
		gk_56f7d(gb);
		return;
	}
	xor_a(gb);
	wr_hl(gb, A); // OBJ_STATE
	set_frameset(gb, ADDR_Frameset_69ff2);
	inc_l(gb);
	A = GKTORTOISEPHASE_9; wr_hl(gb, A); // OBJ_VAR_1
	A = 0x03; wr8(gb, wShootBallState, A);
	play_sfx(gb, SFX_FAT_WALK);
	gk_57170(gb);
	gk_set_shoot_pos_11x_16y(gb);
}

PORT_LOCAL(GKTortoiseFunc, Func_56f27)
{
	A = rd8(gb, wShootBallState);
	and_a(gb);
	if (!ZF) {
		gk_57009(gb);
		return;
	}
	gk_56f2e(gb);
}

PORT_LOCAL(GKTortoiseFunc, Func_56f2e) { gk_56f2e(gb); }

PORT_LOCAL(GKTortoiseFunc, Func_56fef)
{
	A = rd8(gb, wShootBallState);
	cp_a(gb, 0x02);
	if (!ZF) {
		ld_hl(gb, wCurObjState);
		A = rd_hl(gb);
		A = alu_and(gb, A, 0xfe);
		if (!ZF) {
			gk_56f7d(gb);
			return;
		}
	}
	// .asm_56fff
	ld_hl(gb, wCurObjFramesetOffset);
	A = rd_hld(gb);
	cp_a(gb, 0x0c);
	if (!ZF)
		return;
	A = rd_hld(gb);
	and_a(gb);
	if (!ZF)
		return;
	gk_57009(gb);
}

PORT_LOCAL(GKTortoiseFunc, Func_57021)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, 3);
	ld_l(gb, OBJ_STATE);
	A = rd_hl(gb);
	A = alu_and(gb, A, 0xfe);
	if (!ZF) {
		gk_56f7d(gb);
		return;
	}
	ld_l(gb, OBJ_FRAMESET_OFFSET);
	A = rd_hld(gb);
	cp_a(gb, 0x06);
	if (!ZF)
		return;
	A = rd_hld(gb);
	and_a(gb);
	if (!ZF)
		return;
	gk_57009(gb);
}

// ld hl, wCurObjFrameDuration / ld a, [hli] / and a / ret nz / ld a, [hl]:
// returns whether the frame ended (a = OBJ_FRAMESET_OFFSET)
static bool gk_frame_ended(GB *gb)
{
	ld_hl(gb, wCurObjFrameDuration);
	A = rd_hli(gb);
	and_a(gb);
	if (!ZF)
		return false;
	A = rd_hl(gb);
	return true;
}

PORT_LOCAL(GKTortoiseFunc, Func_57039)
{
	if (!gk_frame_ended(gb))
		return;
	cp_a(gb, 2);
	if (ZF) {
		// .set_wario_pos_22y
		ld_l(gb, OBJ_Y_POS);
		obj_pos_to_wario(gb, 1, 22, 0, 0);
		return;
	}
	cp_a(gb, 4);
	if (ZF) {
		// .set_wario_pos_11x_16y
		ld_l(gb, OBJ_Y_POS);
		obj_pos_to_wario(gb, 1, 16, 1, 11);
		return;
	}
	cp_a(gb, 6);
	if (ZF) {
		// .asm_57069
		xor_a(gb);
		wr8(gb, wAutoMoveState, A);
		A = (1 << NO_ACTIONS_F) | 15;
		wr8(gb, wCurObjAction, A);
		play_sfx(gb, SFX_067);
		return;
	}
	cp_a(gb, 10);
	if (!ZF)
		return;
	gk_57009(gb);
}

PORT_LOCAL(GKTortoiseFunc, Func_5707b)
{
	ld_hl(gb, wCurObjFlags);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_FRAME_DURATION);
	A = rd_hli(gb);
	and_a(gb);
	if (!ZF)
		return;
	A = rd_hl(gb);
	cp_a(gb, 0x06);
	if (ZF) {
		// .asm_570c1
		play_sfx(gb, SFX_FAT_WALK);
		gk_get_shoot_pos_ptr(gb);
		copy_pos_de_to_hli(gb, 0, 0, 2, 0x0c);
		return;
	}
	cp_a(gb, 0x08);
	if (ZF) {
		// .asm_570df
		gk_get_shoot_pos_ptr(gb);
		copy_pos_de_to_hli(gb, 1, 0x16, 0, 0);
		return;
	}
	cp_a(gb, 0x0e);
	if (ZF) {
		// .asm_570f5
		gk_get_shoot_pos_ptr(gb);
		copy_pos_de_to_hli(gb, 1, 0x10, 1, 0x0b);
		return;
	}
	cp_a(gb, 0x10);
	if (ZF) {
		// .asm_570b3
		A = 0x04; wr8(gb, wShootBallState, A);
		play_sfx(gb, SFX_067);
		return;
	}
	cp_a(gb, 0x14);
	if (!ZF)
		return;
	A = rd8(gb, wWarioGoals);
	cp_a(gb, NUM_WARIO_GOALS_NEEDED);
	if (!ZF) {
		gk_57009(gb);
		return;
	}
	set_frameset(gb, ADDR_Frameset_6a00e);
	A = 180; wr_hli(gb, A); // OBJ_STATE_DURATION
	A = GKTORTOISEPHASE_8; wr_hl(gb, A); // OBJ_VAR_1
	A = (uint8_t)-10; wr8(gb, wCurObjCollBoxTop, A);
}

PORT_LOCAL(GKTortoiseFunc, Func_5710f)
{
	if (!gk_frame_ended(gb))
		return;
	cp_a(gb, 2);
	if (ZF) {
		// .set_shoot_pos_22y
		gk_get_shoot_pos_ptr(gb);
		copy_pos_de_to_hli(gb, 1, 22, 0, 0);
		return;
	}
	cp_a(gb, 4);
	if (ZF) {
		gk_set_shoot_pos_11x_16y(gb);
		return;
	}
	cp_a(gb, 6);
	if (ZF) {
		// .asm_5713f
		A = 0x04; wr8(gb, wShootBallState, A);
		play_sfx(gb, SFX_067);
		return;
	}
	cp_a(gb, 10);
	if (!ZF)
		return;
	gk_57009(gb);
}

PORT_LOCAL(GKTortoiseFunc, Func_57198)
{
	if (!gk_frame_ended(gb))
		return;
	cp_a(gb, 0x06);
	if (ZF) {
		// .asm_571c7
		ld_hl(gb, wCurObjYPos);
		obj_pos_to_wario(gb, 0, 0, 2, 0x0c);
		return;
	}
	cp_a(gb, 0x08);
	if (ZF) {
		// .asm_571df
		ld_hl(gb, wCurObjYPos);
		obj_pos_to_wario(gb, 1, 0x16, 0, 0);
		return;
	}
	cp_a(gb, 0x0e);
	if (ZF) {
		// .asm_571f7
		ld_hl(gb, wCurObjYPos);
		obj_pos_to_wario(gb, 1, 0x10, 1, 0x0b);
		return;
	}
	cp_a(gb, 0x10);
	if (ZF) {
		// .asm_571b5
		A = 0x01; wr8(gb, wShootBallState, A);
		xor_a(gb);
		wr8(gb, wAutoMoveState, A);
		play_sfx(gb, SFX_067);
		return;
	}
	cp_a(gb, 0x14);
	if (!ZF)
		return;
	gk_57009(gb);
}
