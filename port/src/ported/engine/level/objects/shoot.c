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

// --- Shoot ---

#define NUM_SHOOT_GOALS_NEEDED 3
#define NOACT(n) ((1 << NO_ACTIONS_F) | (n))

#define SH(off) (ADDR_ShootFunc + (off))
#define SH_START_WAIT SH(0x55)
#define SH_TAUNT SH(0x64)
#define SH_MAIN SH(0x106)
#define SH_NORMAL_TABLE SH(0x130)          // after .HandleNormalState's rst JumpTable
#define SH_MOVE_WARIO_BALL_AFTER_KICK SH(0x37b)
#define SH_DO_PREKICK_X SH(0x39f)          // .DoPreKickingXMovement
#define SH_MOVE_LEFT_OR_RIGHT SH(0x3b6)
#define SH_BALL_FORM SH(0x40b)
#define SH_BALL_TABLE SH(0x41f)            // after .BallForm's rst JumpTable
#define SH_FUNC_56CDB SH(0x6ca)
#define SH_DEFEAT_RISE SH(0x777)
#define SH_DEFEAT_SUSPEND SH(0x79e)

static inline void dec_l(GB *gb) { gb->cpu.l = alu_dec(gb, gb->cpu.l); }

static void stop_music2(GB *gb)
{
	A = 0xff;
	wr8(gb, hMusicIDBackup + 0, A);
	wr8(gb, hMusicID + 0, A);
	A = 0x00;
	wr8(gb, hMusicIDBackup + 1, A);
	wr8(gb, hMusicID + 1, A);
}

PORT_JUMP(ShootFunc)
{
	A = TRUE;
	wr8(gb, wIsBossBattle, A);
	stop_music2(gb);
	xor_a(gb);
	wr8(gb, wShootCanTurnAround, A);
	wr8(gb, wShootBallState, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update(gb, REG_HL(gb), SH_START_WAIT);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	set_mhl(gb, OBJSUBFLAG_UNK_4_F);
	A = rd_hl(gb);
	A = alu_and(gb, A, OBJSUBFLAGS_MASK);
	A = alu_or(gb, A, 0x5);
	wr_hl(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 7; wr_hld(gb, A);
	A = (uint8_t)-8; wr_hld(gb, A);
	A = (uint8_t)-8; wr_hl(gb, A);
	set_frameset(gb, ADDR_Frameset_69f7c);
	A = 80; wr_hli(gb, A); // SHOOT_TAUNT_DURATION
	// position Shoot 32 pixels up and 96 pixels right
	A = 32; wr8(gb, wCurObjVar2, A);
	CALL(MoveObjectUpByVar2);
	A = 96; wr8(gb, wCurObjVar2, A);
	JUMP(MoveObjectRightByVar2);
}

PORT_LOCAL(ShootFunc, StartWait)
{
	ld_hl(gb, wCurObjFlags);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ON_SCREEN_F))
		return;
	set_update(gb, REG_HL(gb), SH_TAUNT);
}

// .set_standing
static void sh_set_standing(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = rd_hl(gb);
	A = alu_rlca(gb, A);
	uint16_t de;
	if (CF) {
		set_mhl(gb, OBJSUBFLAG_VDIR_F);
		de = ADDR_Frameset_6a03a;
	} else {
		res_mhl(gb, OBJSUBFLAG_VDIR_F);
		de = ADDR_Frameset_6a022;
	}
	set_frameset(gb, de);
	A = 27; wr_hli(gb, A); // SHOOT_IDLE_DURATION
	xor_a(gb); wr_hli(gb, A); // SHOOTPHASE_STANDING
	A = 1; wr_hl(gb, A); // SHOOT_JUMP_X_SPEED
}

// .set_jumping_up, with de = the frameset
static void sh_set_jumping_up(GB *gb, uint16_t de)
{
	set_frameset(gb, de);
	A = 22; wr_hli(gb, A); // SHOOT_JUMP_UP_DURATION
	A = 1; wr_hli(gb, A); // SHOOTPHASE_JUMPING_UP
	inc_l(gb);
	xor_a(gb);
	wr_hl(gb, A); // OBJ_VAR_3
	play_sfx(gb, SFX_0C8);
}

PORT_LOCAL(ShootFunc, Taunt)
{
	// play SFX every 16 frames
	A = rd8(gb, wGlobalCounter);
	A = alu_and(gb, A, 0x0f);
	if (ZF)
		play_sfx(gb, SFX_0C6);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	set_update(gb, REG_HL(gb), SH_MAIN);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	A = rd_hl(gb);
	A = alu_and(gb, A, HEAVY_OBJ);
	A = alu_or(gb, A, OBJ_INTERACTION_SHOOT);
	wr_hli(gb, A);
	A = 0x01; wr8(gb, wShootBallState, A);
	A = BOSS_REGULAR; wr8(gb, wBossState, A);
	CALL(UpdateLevelMusic);
	sh_set_standing(gb);
}

// .jump_out_of_ball (hl = OBJ_SUBSTATE)
static void sh_jump_out_of_ball(GB *gb)
{
	A = rd_hld(gb);
	A = alu_rlca(gb, A);
	uint16_t de = CF ? ADDR_Frameset_6a03a + 2 : ADDR_Frameset_6a022 + 2;
	A = 2; wr8(gb, wCurObjVar2, A); // SHOOT_OUT_OF_BALL_X_SPEED
	sh_set_jumping_up(gb, de);
}

// .asm_566b7 / .asm_566bd (hl = OBJ_STATE)
static void sh_out_of_ball(GB *gb, bool vdir)
{
	xor_a(gb);
	wr_hld(gb, A);
	if (vdir)
		set_mhl(gb, OBJSUBFLAG_VDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_VDIR_F);
	sh_jump_out_of_ball(gb);
}

// .interacted_with_ball_wario (hl = OBJ_STATE)
static void sh_interacted_with_ball_wario(GB *gb)
{
	xor_a(gb);
	wr_hld(gb, A); // OBJ_STATE
	dec_l(gb);
	wr_hld(gb, A); // OBJ_VAR_3
	// move up 16 pixels
	A = 16; wr_hl(gb, A); // OBJ_VAR_2
	CALL(MoveObjectUpByVar2);
	set_frameset(gb, ADDR_Frameset_69ffd);
	inc_l(gb);
	A = 5; wr_hli(gb, A); // SHOOTPHASE_PREKICK_JUMP
	A = 1; wr_hl(gb, A); // SHOOT_PRESHOOT_X_SPEED
	// place Wario 16 pixels below Shoot
	ld_l(gb, OBJ_Y_POS);
	A = rd_hli(gb);
	A = alu_add(gb, A, 16);
	wr8(gb, wWarioYPos + 1, A);
	A = rd8(gb, wWarioYPos + 0);
	A = alu_adc(gb, A, 0);
	wr8(gb, wWarioYPos + 0, A);
	inc_l(gb);
	A = rd_hli(gb);
	wr8(gb, wWarioXPos + 1, A);
	play_sfx(gb, SFX_FAT_WALK);
	ld_l(gb, OBJ_SUBSTATE);
	set_mhl(gb, OBJSUBFLAG_HDIR_F);
	// bug: a holds the SFX, not the x position
	cp_a(gb, 123);
	if (CF)
		set_mhl(gb, OBJSUBFLAG_VDIR_F);
	else
		res_mhl(gb, OBJSUBFLAG_VDIR_F);
}

// .stepped_on, falling through into .BallForm
static void sh_stepped_on(GB *gb)
{
	set_frameset(gb, ADDR_Frameset_69f5d);
	set_update(gb, REG_HL(gb), SH_BALL_FORM);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	A = OBJ_INTERACTION_01; wr_hli(gb, A);
	A = (uint8_t)-18; wr_hli(gb, A); // OBJ_COLLBOX_TOP
	A = 0; wr_hl(gb, A); // OBJ_COLLBOX_BOTTOM
	JUMP_TO(SH_BALL_FORM);
}

PORT_LOCAL_JUMP(ShootFunc, Main)
{
	ld_hl(gb, wCurObjState);
	A = rd_hl(gb);
	and_a(gb);
	if (ZF) {
		// .HandleNormalState
		A = rd8(gb, wCurObjVar1);
		JUMP_TO(jumptable(gb, SH_NORMAL_TABLE));
		return;
	}
	cp_a(gb, 0x0e);
	if (ZF) {
		// .asm_566a9
		play_sfx(gb, SFX_0C9);
		A = rd8(gb, wCurObjSubState);
		A = alu_rlca(gb, A);
		sh_out_of_ball(gb, !CF);
		RETURN;
	}
	cp_a(gb, 0x04);
	if (ZF) {
		sh_out_of_ball(gb, true);
		RETURN;
	}
	cp_a(gb, 0x05);
	if (ZF) {
		sh_out_of_ball(gb, false);
		RETURN;
	}
	cp_a(gb, OBJSTATE_SPECIAL_3);
	if (ZF) {
		sh_interacted_with_ball_wario(gb);
		RETURN;
	}
	A = alu_and(gb, A, 0xfe);
	cp_a(gb, OBJSTATE_WOBBLE_LEFT_START);
	if (ZF) {
		sh_stepped_on(gb);
		return;
	}
	cp_a(gb, OBJSTATE_VANISH_TOUCH);
	if (ZF) {
		sh_stepped_on(gb);
		return;
	}
	xor_a(gb);
	wr_hl(gb, A);
	RETURN;
}

PORT_LOCAL(ShootFunc, Standing)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return; // not finished yet
	A = rd8(gb, wCurObjSubState);
	A = alu_rlca(gb, A);
	sh_set_jumping_up(gb, CF ? ADDR_Frameset_6a043 : ADDR_Frameset_6a02b);
}

PORT_LOCAL_JUMP(ShootFunc, JumpingUp)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		SET_BC(gb, ADDR_Data_603d0);
		CALL(ApplyObjYMovement);
		JUMP_TO(SH_MOVE_LEFT_OR_RIGHT);
		return;
	}
	// .set_jumping_down
	inc_l(gb);
	A = 2; wr_hli(gb, A); // SHOOTPHASE_JUMPING_DOWN
	inc_l(gb);
	xor_a(gb);
	wr_hl(gb, A); // OBJ_VAR_3
	RETURN;
}

// .still_more_goals_needed
static void sh_still_more_goals_needed(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = rd8(gb, wWarioXPos + 1);
	cp_a(gb, 192);
	bool turn = false;
	if (CF) {
		// x < 192
		gb->cpu.b = A;
		A = rd8(gb, wCurObjXPos);
		cp_a(gb, gb->cpu.b);
		// carry set if Wario is left of Shoot
		if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F))
			turn = !CF; // .facing_right: turn around if Wario to left
		else
			turn = CF; // turn around if Wario to right
	}
	if (!turn) {
		// .maybe_turn_around
		A = rd8(gb, wShootCanTurnAround);
		and_a(gb);
		turn = !ZF;
	}
	if (turn) {
		// .turn_around
		A = rd_hl(gb);
		A = alu_rlca(gb, A);
		set_frameset(gb, CF ? ADDR_Frameset_69f40 : ADDR_Frameset_69f35);
		A = 42; wr_hli(gb, A); // SHOOT_TURN_AROUND_DURATION
		A = 4; wr_hli(gb, A); // SHOOTPHASE_TURNING
		xor_a(gb);
		wr8(gb, wShootCanTurnAround, A);
		return;
	}
	// do a jump
	A = rd_hl(gb);
	A = alu_rlca(gb, A);
	set_frameset(gb, CF ? ADDR_Frameset_6a04f : ADDR_Frameset_6a037);
	// set jumping delay
	A = rd8(gb, wWarioGoals);
	and_a(gb);
	A = ZF ? 25 : 2; // SHOOT_JUMP_DELAY_LONG / _SHORT
	wr_hli(gb, A); // OBJ_STATE_DURATION
	A = 3; wr_hli(gb, A); // SHOOTPHASE_BETWEEN_JUMPS
}

// ld hl, wCurObjYPos / ld a, [hli] / add off / ldh [hYPosLo], a /
// ld a, [hli] / adc 0 / ldh [hYPosHi], a / call Func_352b / and a
static bool sh_check_down(GB *gb, uint8_t off, uint16_t ret)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb);
	A = alu_add(gb, A, off);
	wr8(gb, hYPosLo, A);
	A = rd_hli(gb);
	A = alu_adc(gb, A, 0);
	wr8(gb, hYPosHi, A);
	gb_call_ret(gb, ADDR_Func_352b, ret);
	and_a(gb);
	return !ZF;
}

PORT_LOCAL_JUMP(ShootFunc, JumpingDown)
{
	SET_BC(gb, ADDR_Data_603e0);
	CALL(ApplyObjYMovement);
	gb_call_ret(gb, SH_MOVE_LEFT_OR_RIGHT, SH(0x1ab));

	// check centre collision
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); wr8(gb, hYPosHi, A);
	A = rd_hli(gb); wr8(gb, hXPosLo, A);
	A = rd_hl(gb); wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_352b, SH(0x1bd));
	and_a(gb);
	if (!ZF) {
		// .grounded
		ld_hl(gb, wCurObjYPos);
		A = rd8(gb, hYPosLo); wr_hli(gb, A);
		A = rd8(gb, hYPosHi); wr_hl(gb, A);
		sh_still_more_goals_needed(gb);
		RETURN;
	}
	uint16_t de;
	if (sh_check_down(gb, 16, SH(0x1d0))) {
		// .high_16_px
		A = rd8(gb, wCurObjSubState);
		A = alu_rlca(gb, A);
		de = CF ? ADDR_Frameset_6a04c : ADDR_Frameset_6a034;
	} else if (sh_check_down(gb, 32, SH(0x1e3))) {
		// .high_32_px
		A = rd8(gb, wCurObjSubState);
		A = alu_rlca(gb, A);
		de = CF ? ADDR_Frameset_6a049 : ADDR_Frameset_6a031;
	} else if (sh_check_down(gb, 48, SH(0x1f6))) {
		// .high_48_px
		A = rd8(gb, wCurObjSubState);
		A = alu_rlca(gb, A);
		de = CF ? ADDR_Frameset_6a046 : ADDR_Frameset_6a02e;
	} else {
		RETURN;
	}
	SET_DE(gb, de);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL(ShootFunc, BetweenJumps)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	sh_set_standing(gb);
}

PORT_LOCAL(ShootFunc, Turning)
{
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		sh_set_standing(gb);
		return;
	}
	A = rd_hl(gb);
	cp_a(gb, 42 / 2);
	if (!ZF)
		return;
	ld_l(gb, OBJ_SUBSTATE);
	A = rd_hl(gb);
	A = alu_xor(gb, A, OBJSUBFLAG_HDIR);
	wr_hl(gb, A);
}

PORT_LOCAL(ShootFunc, PreKickJump)
{
	// handle movement in x and y axis
	gb_call_ret(gb, SH_DO_PREKICK_X, SH(0x2af));
	SET_BC(gb, ADDR_Data_54000);
	gb_call_ret(gb, ADDR_DoPreKickingJumpYMovement, SH(0x2b5));

	ld_hl(gb, wCurObjYPos);
	A = rd_hl(gb);
	gb->cpu.b = 224;
	cp_a(gb, gb->cpu.b);
	if (CF)
		return; // y < 224
	set_frameset(gb, ADDR_Frameset_6a000);
	inc_l(gb);
	A = 6; wr_hli(gb, A); // SHOOTPHASE_PREKICK_ANIMATION
	// move down 16 pixels
	A = 16; wr_hl(gb, A); // OBJ_VAR_2
	CALL(MoveObjectDownByVar2);
	// snap Shoot and Wario to y = 240
	// and position Wario to (Shoot's x + 8)
	ld_l(gb, OBJ_Y_POS);
	A = 240; wr_hli(gb, A);
	wr8(gb, wWarioYPos + 1, A);
	A = rd_hli(gb);
	wr8(gb, wWarioYPos + 0, A);
	A = rd_hli(gb); // OBJ_X_POS
	A = alu_add(gb, A, 8);
	wr8(gb, wWarioXPos + 1, A);
}

PORT_LOCAL_JUMP(ShootFunc, PreKickAnimation)
{
	// wait frame to finish
	ld_hl(gb, wCurObjFrameDuration);
	A = rd_hli(gb);
	and_a(gb);
	if (!ZF)
		RETURN;
	A = rd_hli(gb); // OBJ_FRAMESET_OFFSET
	cp_a(gb, 6);
	if (ZF) {
		// .frame_6: move Wario 8 pixels right
		A = rd8(gb, wWarioXPos + 1);
		A = alu_add(gb, A, 8);
		wr8(gb, wWarioXPos + 1, A);
		play_sfx(gb, SFX_HANG_GRAB);
		RETURN;
	}
	cp_a(gb, 10);
	if (!ZF)
		RETURN;
	inc_l(gb);
	A = 7; wr_hl(gb, A); // SHOOTPHASE_KICKING
	SET_DE(gb, ADDR_Frameset_69f54);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ShootFunc, Kicking)
{
	A = rd8(gb, wShootBallState);
	cp_a(gb, SHOOTBALL_WARIO_KICKED);
	if (ZF)
		gb_call_ret(gb, SH_MOVE_WARIO_BALL_AFTER_KICK, SH(0x311));
	ld_hl(gb, wCurObjFrameDuration);
	A = rd_hli(gb);
	and_a(gb);
	if (!ZF)
		RETURN;
	A = rd_hli(gb); // OBJ_FRAMESET_OFFSET
	cp_a(gb, 4);
	if (ZF) {
		// .shoot_wario
		A = SHOOTBALL_WARIO_KICKED; wr8(gb, wShootBallState, A);
		xor_a(gb);
		wr8(gb, wCurObjVar3, A);
		A = 0x2 | (1 << 7); wr8(gb, wAutoMoveState, A);
		play_sfx(gb, SFX_034);
		RETURN;
	}
	cp_a(gb, 8);
	if (!ZF)
		RETURN;
	inc_l(gb);
	A = 8; wr_hl(gb, A); // SHOOTPHASE_SCORE_GOAL
	SET_DE(gb, ADDR_Frameset_6a00b);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ShootFunc, ScoreGoal)
{
	A = rd8(gb, wShootBallState);
	cp_a(gb, SHOOTBALL_WARIO_GOAL);
	if (!ZF) {
		JUMP_TO(SH_MOVE_WARIO_BALL_AFTER_KICK);
		return;
	}
	ld_hl(gb, wShootGoals);
	A = rd_hl(gb);
	cp_a(gb, NUM_SHOOT_GOALS_NEEDED);
	if (!ZF) {
		wr_hl(gb, alu_inc(gb, rd_hl(gb)));
		gb->cpu.b = rd_hl(gb);
	}
	A = 0x1 | (1 << 7); wr8(gb, wAutoMoveState, A);
	play_sfx(gb, SFX_0C7);
	A = TRUE; wr8(gb, wShootCanTurnAround, A);
	ld_hl(gb, wCurObjSubState);
	res_mhl(gb, OBJSUBFLAG_VDIR_F);
	A = gb->cpu.b;
	cp_a(gb, NUM_SHOOT_GOALS_NEEDED);
	if (!ZF) {
		sh_still_more_goals_needed(gb);
		RETURN;
	}
	// reached target num of goals
	A = 9; wr8(gb, wCurObjVar1, A); // SHOOTPHASE_WON
	RETURN;
}

PORT_LOCAL(ShootFunc, Won)
{
	A = NOACT(1);
	wr8(gb, wCurObjAction, A);
}

PORT_LOCAL_JUMP(ShootFunc, MoveWarioBallAfterKick)
{
	A = rd8(gb, wWarioXPos + 1);
	cp_a(gb, 237);
	if (CF) {
		// move Wario right
		SET_BC(gb, ADDR_Data_54010);
		gb_call_ret(gb, ADDR_ApplyYOffsetOnKickedWarioBall, SH(0x388));
		gb->cpu.b = 3;
		JUMP(MoveWarioRight);
		return;
	}
	// .ball_wario_inside_net: move Wario down to the ground
	A = rd8(gb, wWarioYPos + 1);
	cp_a(gb, 240);
	if (CF) {
		gb->cpu.b = 3;
		JUMP(MoveWarioDown);
		return;
	}
	// .set_wario_ball_goal_state
	A = SHOOTBALL_WARIO_GOAL;
	wr8(gb, wShootBallState, A);
	RETURN;
}

PORT_LOCAL_JUMP(ShootFunc, DoPreKickingXMovement)
{
	A = rd8(gb, wCurObjVar2);
	gb->cpu.b = A;
	ld_hl(gb, wCurObjSubState);
	if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F)) {
		// .move_with_wario_right
		CALL(MoveObjectRightByVar2);
		JUMP(MoveWarioRight);
		return;
	}
	CALL(MoveObjectLeftByVar2);
	JUMP(MoveWarioLeft);
}

PORT_LOCAL_JUMP(ShootFunc, MoveLeftOrRight)
{
	A = rd8(gb, wCurObjSubState);
	A = alu_rlca(gb, A);
	A = alu_rlca(gb, A);
	A = rd8(gb, wCurObjXPos);
	if (CF) {
		// .moving_right
		cp_a(gb, 219);
		if (CF) {
			JUMP(MoveObjectRightByVar2);
			return;
		}
		play_sfx(gb, SFX_08D);
		ld_hl(gb, wCurObjSubState);
		res_mhl(gb, OBJSUBFLAG_VDIR_F);
		if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F))
			RETURN;
	} else {
		// moving left
		cp_a(gb, 27);
		if (!CF) {
			JUMP(MoveObjectLeftByVar2);
			return;
		}
		play_sfx(gb, SFX_08D);
		ld_hl(gb, wCurObjSubState);
		set_mhl(gb, OBJSUBFLAG_VDIR_F);
		if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F))
			RETURN;
	}
	// .set_can_turn_around
	A = TRUE;
	wr8(gb, wShootCanTurnAround, A);
	RETURN;
}

// --- Shoot, ball form ---

PORT_LOCAL_JUMP(ShootFunc, BallForm)
{
	ld_hl(gb, wCurObjFlags);
	A = rd8(gb, wCurObjXPos);
	cp_a(gb, 219);
	if (!CF)
		res_mhl(gb, OBJFLAG_GRABBED_F);
	else
		set_mhl(gb, OBJFLAG_GRABBED_F);
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, SH_BALL_TABLE));
}

PORT_LOCAL_JUMP(ShootFunc, Func_56af0)
{
	A = 0x2d;
	wr8(gb, wCurObjState, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	SET_DE(gb, ADDR_Frameset_69f5d);
	JUMP(SetObjectFramesetPtr);
}

// .return_to_regular_form
static void sh_return_to_regular_form(GB *gb)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	set_update(gb, REG_HL(gb), SH_MAIN);
	ld_l(gb, OBJ_STATE);
	A = 0x05; wr_hld(gb, A);
	res_mhl(gb, 7);
	ld_l(gb, OBJ_INTERACTION_TYPE);
	A = OBJ_INTERACTION_SHOOT | HEAVY_OBJ; wr_hli(gb, A);
	A = (uint8_t)-32; wr_hli(gb, A);
	A = (uint8_t)-8; wr_hl(gb, A);
}

PORT_LOCAL(ShootFunc, Func_56b20)
{
	ld_hl(gb, wCurObjState);
	A = 0x30; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
}

static void sh_56b47(GB *gb);

PORT_LOCAL_JUMP(ShootFunc, Func_56b2a)
{
	ld_hl(gb, wCurObjState);
	A = 0x31; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A); // OBJ_VAR_3
	dec_l(gb);
	A = rd8(gb, wWarioGoals);
	cp_a(gb, 2);
	if (CF)
		xor_a(gb); // treated as $100 (SHOOT_REGULAR_BALL_DURATION)
	else
		A = 160; // SHOOT_LAST_GOAL_BALL_DURATION
	wr_hl(gb, A); // OBJ_VAR_1
	set_frameset(gb, ADDR_Frameset_69f5d);
	sh_56b47(gb); // falls through
}

PORT_LOCAL_JUMP(ShootFunc, Func_56b47) { sh_56b47(gb); }

// .Func_56b47
static void sh_56b47(GB *gb)
{
	A = rd8(gb, wCurObjVar3);
	cp_a(gb, 0x22);
	if (ZF)
		play_sfx(gb, SFX_075);
	ld_hl(gb, wCurObjVar1);
	if (!dec_mhl(gb)) {
		sh_return_to_regular_form(gb);
		RETURN;
	}
	A = rd_hl(gb);
	cp_a(gb, 0x12);
	bool b81 = false;
	if (ZF) {
		set_frameset(gb, ADDR_Frameset_69f93);
		play_sfx(gb, SFX_0B7);
	} else if (!CF) {
		// .asm_56b71
		cp_a(gb, 0x64);
		if (ZF) {
			set_frameset(gb, ADDR_Frameset_69f66);
			b81 = true;
		} else if (CF) {
			b81 = true; // .asm_56b7f
		}
	}
	if (b81) {
		// .asm_56b81
		A = rd8(gb, wGlobalCounter);
		A = alu_and(gb, A, 0x0f);
		if (ZF)
			play_sfx(gb, SFX_06A);
	}
	// .asm_56b90
	SET_BC(gb, ADDR_Data_60e40);
	JUMP(ApplyObjYMovement_Loop);
}

// .asm_56bc0 tail of .Func_56bae / .Func_56bb8
static void sh_56bc0(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjAction);
	A = NOACT(15); wr_hld(gb, A);
	A = state; wr_hld(gb, A); // OBJ_STATE
	set_frameset(gb, ADDR_Frameset_69f5d);
	A = 4; wr_hli(gb, A);
	inc_l(gb);
	A = 0x02; wr_hli(gb, A); // OBJ_VAR_2
	xor_a(gb);
	wr_hli(gb, A);
}

PORT_LOCAL(ShootFunc, Func_56b96)
{
	play_sfx(gb, SFX_014);
	A = rd8(gb, wWarioScreenXPos);
	A = alu_add(gb, A, 0x2a);
	gb->cpu.b = A;
	A = rd8(gb, wCurObjScreenXPos);
	A = alu_add(gb, A, 0x2a);
	cp_a(gb, gb->cpu.b);
	sh_56bc0(gb, !CF ? 0x38 : 0x39);
}

PORT_LOCAL(ShootFunc, Func_56bae) { sh_56bc0(gb, 0x38); }
PORT_LOCAL(ShootFunc, Func_56bb8) { sh_56bc0(gb, 0x39); }

PORT_LOCAL(ShootFunc, Func_56bd1)
{
	ld_hl(gb, wCurObjState);
	A = 0x41; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
}

static void sh_56bdb(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	// .asm_56be9
	ld_l(gb, OBJ_VAR_2);
	A = 0x02; wr_hli(gb, A);
	xor_a(gb);
	wr_hli(gb, A);
}

PORT_LOCAL(ShootFunc, Func_56bdb) { sh_56bdb(gb, 0x44); }
PORT_LOCAL(ShootFunc, Func_56be3) { sh_56bdb(gb, 0x45); }

PORT_LOCAL(ShootFunc, Func_56bf1)
{
	ld_hl(gb, wCurObjState);
	A = 0x46; wr_hld(gb, A);
}

PORT_LOCAL(ShootFunc, Func_56bf8)
{
	ld_hl(gb, wCurObjState);
	A = 0x47; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(ShootFunc, Func_56bff)
{
	ld_hl(gb, wCurObjVar2);
	A = rd_hl(gb);
	and_a(gb);
	if (!ZF) {
		JUMP(Func_32ae);
		return;
	}
	A = 0x0f;
	wr8(gb, wCurObjState, A);
	RETURN;
}

PORT_LOCAL_JUMP(ShootFunc, Func_56c0d)
{
	ld_hl(gb, wCurObjVar2);
	A = rd_hl(gb);
	and_a(gb);
	if (!ZF) {
		JUMP(Func_32bd);
		return;
	}
	A = 0x0f;
	wr8(gb, wCurObjState, A);
	RETURN;
}

// .asm_56c25 (a = the state, de = the frameset, hl = OBJ_SUBSTATE)
static void sh_56c25(GB *gb)
{
	inc_l(gb);
	wr_hld(gb, A); // OBJ_STATE
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A);
	A = rd8(gb, wCurObjYPos);
	cp_a(gb, 216);
	A = CF ? 0x04 : 0x02;
	wr_hld(gb, A);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ShootFunc, Func_56c1b)
{
	A = 0x34;
	SET_DE(gb, ADDR_Frameset_69f9f);
	ld_hl(gb, wCurObjSubState);
	set_mhl(gb, OBJSUBFLAG_VDIR_F);
	sh_56c25(gb);
}

PORT_LOCAL_JUMP(ShootFunc, Func_56c3d)
{
	A = 0x35;
	SET_DE(gb, ADDR_Frameset_69fa4);
	ld_hl(gb, wCurObjSubState);
	res_mhl(gb, OBJSUBFLAG_VDIR_F);
	sh_56c25(gb);
}

// .set_defeated
static void sh_set_defeated(GB *gb)
{
	set_frameset(gb, ADDR_Frameset_69f8a);
	A = 28; wr_hli(gb, A); // SHOOT_DEFEAT_RISE_DURATION
	set_update(gb, REG_HL(gb), SH_DEFEAT_RISE);
	ld_l(gb, OBJ_STATE);
	A = 0x48; wr_hld(gb, A);
	A = rd_hl(gb); // OBJ_SUBSTATE
	A = alu_and(gb, A, OBJSUBFLAG_UNINITIALISED | OBJSUBFLAG_VDIR | OBJSUBFLAG_HDIR);
	wr_hld(gb, A);
	xor_a(gb);
	wr_hl(gb, A); // OBJ_VAR_3
	ld_l(gb, OBJ_FLAGS);
	set_mhl(gb, OBJFLAG_PRIORITY_F);
	ld_hl(gb, wCurObjUnk02);
	gb->cpu.e = 0x09;
	FARCALL(Func_ba42);
	play_sfx(gb, SFX_0A9);
}

PORT_LOCAL_JUMP(ShootFunc, Func_56c49)
{
	A = NOACT(1);
	wr8(gb, wCurObjAction, A);
	A = rd8(gb, wShootCanTurnAround);
	and_a(gb);
	if (!ZF) {
		xor_a(gb); // FALSE
		wr8(gb, wShootCanTurnAround, A);
		set_frameset(gb, ADDR_Frameset_69f5d);
	}
	// .asm_56c5e
	gb_call_ret(gb, SH_FUNC_56CDB, SH(0x650));
	A = rd8(gb, wCurObjVar2);
	cp_a(gb, 0x04);
	SET_BC(gb, ZF ? ADDR_Data_60e70 : ADDR_Data_60e20);
	CALL(ApplyObjYMovement);
	ld_hl(gb, wCurObjYPos);
	A = rd_hl(gb);
	gb->cpu.b = 240;
	cp_a(gb, gb->cpu.b);
	if (CF)
		RETURN; // y < 240
	// snap Shoot to y = 240
	wr_hl(gb, gb->cpu.b);

	A = rd8(gb, wShootBallState);
	cp_a(gb, 0x04);
	if (ZF) {
		A = rd8(gb, wWarioGoals);
		cp_a(gb, NUM_WARIO_GOALS_NEEDED);
		if (ZF) {
			sh_set_defeated(gb);
			RETURN;
		}
		A = 0x01;
		wr8(gb, wShootBallState, A);
		sh_return_to_regular_form(gb);
		RETURN;
	}
	// .asm_56c93
	ld_l(gb, OBJ_SUBSTATE);
	if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_VDIR_F)) {
		inc_l(gb);
		A = 0x09; wr_hl(gb, A); // OBJ_STATE
		RETURN;
	}
	// .asm_56c9e
	A = rd8(gb, wCurObjXPos);
	cp_a(gb, 219);
	if (CF) {
		inc_l(gb);
		A = 0x08; wr_hl(gb, A); // OBJ_STATE
		RETURN;
	}
	// .asm_56caa
	inc_l(gb);
	A = 0x2d; wr_hl(gb, A); // OBJ_STATE
	A = 0x02; wr8(gb, wShootBallState, A);
	play_sfx(gb, SFX_0C7);
	ld_hl(gb, wWarioGoals);
	A = rd_hl(gb);
	cp_a(gb, NUM_WARIO_GOALS_NEEDED);
	if (!ZF) {
		wr_hl(gb, alu_inc(gb, rd_hl(gb)));
		A = rd_hl(gb);
		cp_a(gb, NUM_WARIO_GOALS_NEEDED);
		if (ZF)
			stop_music2(gb);
	}
	// .asm_56cd5
	SET_DE(gb, ADDR_Frameset_69f9a);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ShootFunc, Func_56cdb)
{
	A = rd8(gb, wCurObjSubState);
	A = alu_rlca(gb, A);
	A = alu_rlca(gb, A);
	if (CF) {
		// .asm_56cf1
		A = rd8(gb, wCurObjYPos);
		cp_a(gb, 208);
		if (!CF) {
			// .asm_56d13
			A = rd8(gb, wCurObjXPos);
			cp_a(gb, 243);
			if (CF) {
				JUMP(MoveObjectRightByVar2);
				return;
			}
			RETURN;
		}
		A = rd8(gb, wCurObjXPos);
		cp_a(gb, 219);
		if (CF) {
			JUMP(MoveObjectRightByVar2);
			return;
		}
		ld_hl(gb, wCurObjSubState);
		res_mhl(gb, OBJSUBFLAG_VDIR_F);
	} else {
		A = rd8(gb, wCurObjXPos);
		cp_a(gb, 27);
		if (!CF) {
			JUMP(MoveObjectLeftByVar2);
			return;
		}
		ld_hl(gb, wCurObjSubState);
		set_mhl(gb, OBJSUBFLAG_VDIR_F);
	}
	// .asm_56d05
	A = TRUE;
	wr8(gb, wShootCanTurnAround, A);
	play_sfx(gb, SFX_01A);
	RETURN;
}

PORT_LOCAL(ShootFunc, Func_56d1c)
{
	A = NOACT(1);
	wr8(gb, wCurObjAction, A);
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	A = rd8(gb, wShootBallState);
	cp_a(gb, 0x04);
	if (!ZF)
		return;
	set_frameset(gb, ADDR_Frameset_69fa4);
	ld_l(gb, OBJ_STATE);
	A = 0x35; wr_hld(gb, A);
	res_mhl(gb, OBJSUBFLAG_VDIR_F); // OBJ_SUBSTATE
	dec_l(gb);
	xor_a(gb);
	wr_hld(gb, A); // OBJ_VAR_3
	A = rd8(gb, wWarioGoals);
	cp_a(gb, NUM_WARIO_GOALS_NEEDED);
	A = ZF ? 0x01 : 0x03;
	wr_hl(gb, A); // OBJ_VAR_2
}

PORT_LOCAL_JUMP(ShootFunc, DefeatRise)
{
	A = NOACT(1);
	wr8(gb, wCurObjAction, A);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb)) {
		SET_BC(gb, ADDR_Data_60e90);
		JUMP(ApplyObjYMovement);
		return;
	}
	// .asm_56d99
	xor_a(gb);
	wr8(gb, wCurObjVar3, A);
	set_frameset(gb, ADDR_Frameset_69f6f);
	A = 70; wr_hli(gb, A); // SHOOT_DEFEAT_SUSPENDED_DURATION
	set_update(gb, REG_HL(gb), SH_DEFEAT_SUSPEND);
	RETURN;
}

PORT_LOCAL_JUMP(ShootFunc, DefeatSuspendAndFall)
{
	A = NOACT(1);
	wr8(gb, wCurObjAction, A);
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	and_a(gb);
	if (ZF) {
		// .fall
		SET_BC(gb, ADDR_Data_60ea0);
		JUMP(ApplyObjYMovement);
		return;
	}
	// still suspended in air
	if (dec_mhl(gb))
		RETURN;
	play_sfx(gb, SFX_0A3);
	RETURN;
}

// --- Goal counters ---

static void update_score(GB *gb, uint16_t goals, bool shoot)
{
	A = NOACT(1);
	wr8(gb, wCurObjAction, A);
	A = 0x7f;
	wr8(gb, wCurObjFrameDuration, A);
	A = rd8(gb, goals);
	cp_a(gb, 1);
	if (ZF) {
		// .one_goal
		ld_hl(gb, wCurObjFlags);
		res_mhl(gb, OBJFLAG_INVISIBLE_F);
		return;
	}
	cp_a(gb, 2);
	if (ZF) {
		A = 0x3d;
		wr8(gb, wCurObjFrame, A);
		return;
	}
	cp_a(gb, 3);
	if (!ZF)
		return;
	A = 0x3e;
	wr8(gb, wCurObjFrame, A);
	if (shoot) {
		xor_a(gb);
		wr8(gb, wBossState, A);
	}
}

PORT_LOCAL(ShootGoalCounterFunc, UpdateScore) { update_score(gb, wShootGoals, true); }
PORT_LOCAL(WarioGoalCounterFunc, UpdateScore) { update_score(gb, wWarioGoals, false); }
