// Port of src/engine/level/objects/wormwould.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define ROLLING_ROCK_HIT (ADDR_RollingRockFunc + 0x2a) // .Hit
#define PALM_TREE_SPAWN_TREES (ADDR_PalmTreeSpawnerFunc + 0x16) // .SpawnTrees

// Sets update (.MoveLeft/.MoveRight) as the update function once
// wCurObjStateDuration runs out, then moves the rock.
static void rolling_rock(GB *gb, uint16_t update, bool left)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
		gb->cpu.a = (uint8_t)(update >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)update;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
	}
	if (left)
		CALL(MoveObjectLeft_Fast);
	else
		CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_RollingRockFunc);
}

PORT_JUMP(RollingRockFunc_Left) { rolling_rock(gb, ADDR_RollingRockFunc_Left + 0x13, true); }
PORT_JUMP(RollingRockFunc_Right) { rolling_rock(gb, ADDR_RollingRockFunc_Right + 0x13, false); }

PORT_JUMP(RollingRockFunc)
{
	gb->cpu.a = rd8(gb, wCurObjVar1);
	SET_BC(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Data_60900 : ADDR_Data_608b0);
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(ApplyObjYMovement);
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	gb->cpu.l = OBJ_UPDATE_FUNCTION + 1;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, (uint8_t)(ROLLING_ROCK_HIT >> 8));
	wr8(gb, hl--, (uint8_t)ROLLING_ROCK_HIT);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_016);
	RET();
}

PORT(PalmTreeSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(PALM_TREE_SPAWN_TREES >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)PALM_TREE_SPAWN_TREES;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_HL(gb, hl);
	wr8(gb, wCurObjStateDuration, 240);
	// .Wait
	wr8(gb, wCurObjFrameDuration, 2);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

#define PALM_TREE_UPDATE (ADDR_PalmTreeFunc + 0x16) // .Update

PORT_JUMP(PalmTreeFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(PALM_TREE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)PALM_TREE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, ADDR_PalmTreePals);
	SET_DE(gb, wTempPals2 + PAL_SIZE * 7);
	gb->cpu.c = 7;
	gb->cpu.b = 1;
	JUMP(CopyAndApplyOBPals);
}

// Rises while var1 counts down, then (with var2 left) keeps still.
PORT_LOCAL(PalmTreeFunc, Update)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjVar1;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, gb->cpu.a));
		CALL(MoveObjectUp);
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
	} else {
		// .open
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		hl = REG_HL(gb);
		gb->cpu.a = rd8(gb, hl);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			wr8(gb, hl, alu_dec(gb, gb->cpu.a));
			gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
			wr8(gb, wCurObjAction, gb->cpu.a);
			return;
		}
	}
	// .set_frame_duration
	gb->cpu.a = 10;
	wr8(gb, wCurObjFrameDuration, 10);
}

// --- Wormwould ---

#define WW(addr) ((uint16_t)(ADDR_WormwouldFunc + ((addr) - 0x4020)))
#define WW_INIT WW(0x4044)
#define WW_DO_NOTHING WW(0x4103)
#define WW_MAIN WW(0x41b5)
#define WW_UNDERGROUND WW(0x4335)
#define WW_DEFEATED_RAISE WW(0x435f)
#define WW_DEFEATED_WAIT WW(0x4387)
#define WW_DEFEATED_FALL WW(0x43d4)

static inline void ww_set(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << bit)));
}

static inline void ww_res(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << bit)));
}

static void ww_no_actions(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld [hld] the update pointer
static uint16_t ww_set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

// ld a, [hld] / rlca / ld_obj_frameset nc_fs, c_fs (hl = substate)
static void ww_dir_frameset(GB *gb, uint16_t hl, uint16_t nc_fs, uint16_t c_fs)
{
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? c_fs : nc_fs);
}

// The update pointer goes to .DoNothing and the boss music ends
static void ww_leave(GB *gb, uint16_t hl)
{
	ww_set_update(gb, hl, WW_DO_NOTHING);
	gb->cpu.a = NONE;
	wr8(gb, wBossState, NONE);
	CALL(UpdateLevelMusic);
}

// .check_change_side
static void ww_change_side(GB *gb)
{
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	bool left_side = gb->cpu.f & FLAG_C; // OBJSUBFLAG_HDIR_F
	gb->cpu.a = alu_rra(gb, rd8(gb, wCurObjFlags));
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	bool change = false;
	if (gb->cpu.f & FLAG_C) { // OBJFLAG_ON_SCREEN_F
		uint16_t first = left_side ? wCurObjScreenXPos : wWarioScreenXPos;
		uint16_t second = left_side ? wWarioScreenXPos : wCurObjScreenXPos;
		gb->cpu.a = alu_add(gb, rd8(gb, first), 0x2a);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_add(gb, rd8(gb, second), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_C) {
			change = true;
		} else {
			alu_cp(gb, gb->cpu.a, 0x38);
			change = gb->cpu.f & FLAG_C;
		}
	}
	if (!change) {
		SET_DE(gb, left_side ? ADDR_Frameset_696ce : ADDR_Frameset_6970a);
	} else if (left_side) {
		// .place_on_right_side
		ww_res(gb, hl, OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ADDR_Frameset_6970a);
		hl = LO(hl, OBJ_X_POS);
		gb->cpu.a = alu_add(gb, rd8(gb, hl), 0x70);
		wr8(gb, hl++, gb->cpu.a);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		SET_HL(gb, hl);
	} else {
		// .place_on_left_side
		ww_set(gb, hl, OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ADDR_Frameset_696ce);
		hl = LO(hl, OBJ_X_POS);
		gb->cpu.a = alu_sub(gb, rd8(gb, hl), 0x70);
		wr8(gb, hl++, gb->cpu.a);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		SET_HL(gb, hl);
	}

	// .set_direction_frameset
	CALL(SetObjectFramesetPtr);
	ww_set_update(gb, REG_HL(gb), WW_MAIN);
	hl = wCurObjFlags;
	ww_res(gb, hl, OBJFLAG_INVISIBLE_F);
	hl = LO(hl, OBJ_STATE_DURATION);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	gb->cpu.a = rd8(gb, wCurObjYPos + 0);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCurObjYPos + 1);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = 0x41;
	wr8(gb, hl, 0x41);
	gb->cpu.a = 0x2d;
	wr8(gb, wCurObjState, 0x2d);
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_WORMWOULD);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_11A);
}

PORT_JUMP(WormwouldFunc)
{
	uint16_t hl = ww_set_update(gb, wCurObjUpdateFunction + 1, WW_INIT);
	hl = LO(hl, OBJ_SUBSTATE);
	ww_res(gb, hl, OBJSUBFLAG_UNINITIALISED_F);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK);
	gb->cpu.a = alu_or(gb, gb->cpu.a, (3 + 1) | OBJSUBFLAG_UNK_4 | OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	gb->cpu.a = 7;
	wr8(gb, hl--, 7);
	gb->cpu.a = (uint8_t)-8;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 30;
	wr8(gb, hl--, 30);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_696ce);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL(WormwouldFunc, Init)
{
	ww_no_actions(gb);
	gb->cpu.c = rd8(gb, wWarioYPos + 1);
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = rd8(gb, wWarioYPos + 0);
	gb->cpu.a = 0x20;
	wr8(gb, wCurObjVar2, 0x20);
	CALL(MoveObjectDownByVar2);
	gb->cpu.a = BOSS_REGULAR;
	wr8(gb, wBossState, BOSS_REGULAR);
	CALL(UpdateLevelMusic);
	gb->cpu.a = TRUE;
	wr8(gb, wIsBossBattle, TRUE);
	ww_change_side(gb);
}

PORT_LOCAL(WormwouldFunc, DoNothing)
{
	ww_no_actions(gb);
}

// .set_burrowing (hl = wCurObjState)
static void ww_set_burrowing(GB *gb, uint16_t hl)
{
	gb->cpu.a = 0x5c;
	wr8(gb, hl--, 0x5c);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69701 : ADDR_Frameset_69734);
	JUMP(SetObjectFramesetPtr);
}

// .asm_5414b
static void ww_burrow_now(GB *gb, uint16_t hl)
{
	gb->cpu.a = 1;
	wr8(gb, wCurObjStateDuration, 1);
	ww_set_burrowing(gb, hl);
}

// .set_idle
static void ww_set_idle(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	gb->cpu.a = 43; // WORMWOULD_IDLE_DURATION
	wr8(gb, wCurObjStateDuration, 43);
	RET();
}

// .asm_54209 (hl = substate): starts shooting a rock
static void ww_start_shoot(GB *gb, uint16_t hl)
{
	ww_dir_frameset(gb, hl, ADDR_Frameset_69713, ADDR_Frameset_696d7);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 69; // WORMWOULD_SHOOT_ROCK_DURATION
	wr8(gb, hl++, 69);
	SET_HL(gb, hl);
	gb->cpu.a = 0x5a;
	wr8(gb, wCurObjState, 0x5a);
	RET();
}

// .spawn_low_rock / .spawn_high_rock (hl = substate)
static void ww_spawn_rock(GB *gb, uint16_t hl, uint16_t left, uint16_t right)
{
	play_sfx(gb, SFX_TEMPLE_ROCK);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? right : left);
	JUMP(CreateObjectAtRelativePos);
}

// ld l, OBJ_STATE_DURATION / dec [hl] / ld a, [hl] / ld l, OBJ_SUBSTATE
static uint16_t ww_tick(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_STATE_DURATION);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	gb->cpu.a = rd8(gb, hl);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	return hl;
}

PORT_LOCAL_JUMP(WormwouldFunc, Main)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_541e9
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x20);
		alu_cp(gb, gb->cpu.a, 0x20);
		if (gb->cpu.f & FLAG_C) {
			ww_leave(gb, hl);
			RET();
			return;
		}
		// .not_on_upper_part
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = LO(hl, OBJ_SUBSTATE);
		ww_start_shoot(gb, hl);
		return;
	}

	switch (gb->cpu.a) {
	case 0x34: { // .after_attack
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 16);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_0A6);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = LO(hl, OBJ_STATE);
		ww_set_burrowing(gb, hl);
		return;
	}
	case 0x5a: // .shoot_rock_high
		hl = ww_tick(gb, hl);
		alu_cp(gb, gb->cpu.a, 69 - 48);
		if (gb->cpu.f & FLAG_Z) {
			ww_spawn_rock(gb, hl, ADDR_ObjParams_HighRollingRockLeft, ADDR_ObjParams_HighRollingRockRight);
			return;
		}
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		ww_dir_frameset(gb, hl, ADDR_Frameset_6970a, ADDR_Frameset_696ce);
		CALL(SetObjectFramesetPtr);
		ww_set_idle(gb);
		return;
	case 0x2d: { // .raising
		hl = LO(hl, OBJ_VAR_3);
		SET_HL(gb, hl);
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
		alu_cp(gb, gb->cpu.a, 3);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = rd8(gb, hl);
			alu_cp(gb, gb->cpu.a, 0x1e);
			if (gb->cpu.f & FLAG_Z) {
				// .asm_54131
				gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
				SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_696c5 : ADDR_Frameset_696bc);
				CALL(SetObjectFramesetPtr);
				hl = REG_HL(gb);
				gb->cpu.a = 34;
				wr8(gb, hl++, 34);
				SET_HL(gb, hl);
				gb->cpu.a = 0x5b;
				wr8(gb, wCurObjState, 0x5b);
				RET();
				return;
			}
		}
		// .raise
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			JUMP(MoveObjectUp);
			return;
		}
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
		alu_cp(gb, gb->cpu.a, 3 + 1);
		if (gb->cpu.f & FLAG_C) {
			ww_start_shoot(gb, hl);
			return;
		}
		ww_set_idle(gb);
		return;
	}
	case 0x5b: // .shoot_rock_low
		hl = ww_tick(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			ww_spawn_rock(gb, hl, ADDR_ObjParams_LowRollingRockLeft, ADDR_ObjParams_LowRollingRockRight);
			return;
		}
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		hl = LO(hl, OBJ_STATE);
		ww_burrow_now(gb, hl);
		return;
	case 0x5c: { // .burrowing
		hl = LO(hl, OBJ_VAR_1);
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
		alu_cp(gb, gb->cpu.a, 3 + 1);
		if (!(gb->cpu.f & FLAG_C)) {
			hl = LO(hl, alu_dec(gb, (uint8_t)hl));
			gb->cpu.a = rd8(gb, hl++);
			alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z) {
				// lower slowly
				gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
				if (!(gb->cpu.f & FLAG_C)) {
					SET_HL(gb, hl);
					RET();
					return;
				}
			}
		}
		// .lower_quickly
		gb->cpu.c = rd8(gb, wCurObjYPos + 0);
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP(MoveObjectDown);
			return;
		}
		gb->cpu.a = rd8(gb, wCurObjYPos + 1);
		gb->cpu.c = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP(MoveObjectDown);
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			// .set_defeated
			gb->cpu.a = 0x21;
			wr8(gb, wCurObjVar3, 0x21);
			ww_set_update(gb, hl, WW_DEFEATED_RAISE);
			SET_HL(gb, wCurObjUnk02);
			gb->cpu.e = 0x05;
			FARCALL(Func_ba42);
			play_sfx(gb, SFX_0A7);
			RET();
			return;
		}
		gb->cpu.a = 30; // WORMWOULD_UNDERGROUND_DURATION
		wr8(gb, wCurObjStateDuration, 30);
		hl = ww_set_update(gb, hl, WW_UNDERGROUND);
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		ww_set(gb, hl, OBJFLAG_INVISIBLE_F);
		RET();
		return;
	}
	}

	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfe);
	alu_cp(gb, gb->cpu.a, OBJSTATE_ATTACKED_LEFT_START);
	if (gb->cpu.f & FLAG_Z) {
		// .attacked
		gb->cpu.a = 0x34;
		wr8(gb, hl--, 0x34);
		ww_dir_frameset(gb, hl, ADDR_Frameset_6971c, ADDR_Frameset_696e0);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		gb->cpu.a = 26; // WORMWOULD_BURROW_DELAY
		wr8(gb, hl++, 26);
		SET_HL(gb, hl);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		ww_burrow_now(gb, hl);
		return;
	}
	alu_cp(gb, gb->cpu.a, OBJSTATE_VANISH_TOUCH);
	if (gb->cpu.f & FLAG_Z) {
		// .smash_attacked
		gb->cpu.a = 0x3a;
		wr8(gb, hl--, 0x3a);
		ww_dir_frameset(gb, hl, ADDR_Frameset_69721, ADDR_Frameset_696e5);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		gb->cpu.a = 50; // WORMWOULD_HURT_DURATION
		wr8(gb, hl++, 50);
		hl = LO(hl, OBJ_INTERACTION_TYPE);
		gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_0B);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = (uint8_t)-16;
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
		CALL(DecrementRemainingBossHits);
		hl = REG_HL(gb);
		hl = LO(hl, alu_dec(gb, (uint8_t)hl));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		SET_HL(gb, hl);
		play_sfx(gb, SFX_VANISH);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x3a);
	if (gb->cpu.f & FLAG_Z) {
		// .hurt
		SET_BC(gb, ADDR_Data_60dd0);
		CALL(ApplyObjYMovement);
		hl = wCurObjStateDuration;
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 30);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_0A7);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = LO(hl, OBJ_STATE);
		gb->cpu.a = 0x5c;
		wr8(gb, hl--, 0x5c);
		SET_HL(gb, hl);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			// stop_music2
			gb->cpu.a = 0xff;
			wr8(gb, hMusicIDBackup + 0, 0xff);
			wr8(gb, hMusicID + 0, 0xff);
			gb->cpu.a = 0x00;
			wr8(gb, hMusicIDBackup + 1, 0x00);
			wr8(gb, hMusicID + 1, 0x00);
			RET();
			return;
		}
		// .asm_542b6
		gb->cpu.a = (uint8_t)-26;
		wr8(gb, wCurObjCollBoxTop, gb->cpu.a);
		gb->cpu.a = rd8(gb, hl--);
		SET_HL(gb, hl);
		gb->cpu.a = alu_rlca(gb, gb->cpu.a);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_696ea : ADDR_Frameset_69726);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	RET();
}

PORT_LOCAL(WormwouldFunc, Underground)
{
	ww_no_actions(gb);
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0x3a);
	if (gb->cpu.f & FLAG_C) {
		ww_leave(gb, wCurObjUpdateFunction + 1);
		return;
	}
	// .wait_underground
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	ww_res(gb, hl, OBJFLAG_INVISIBLE_F);
	ww_change_side(gb);
}

PORT_LOCAL_JUMP(WormwouldFunc, DefeatedRaise)
{
	ww_no_actions(gb);
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(MoveObjectUp_Fast);
		return;
	}
	hl = LO(hl, alu_inc(gb, (uint8_t)hl));
	ww_dir_frameset(gb, hl, ADDR_Frameset_6972f, ADDR_Frameset_696f3);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 30;
	wr8(gb, hl++, 30);
	ww_set_update(gb, hl, WW_DEFEATED_WAIT);
	RET();
}

// .DefeatedFall
static void ww_defeated_fall(GB *gb)
{
	ww_no_actions(gb);
	SET_BC(gb, ADDR_Data_60180);
	CALL(ApplyObjYMovement);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!(gb->cpu.f & FLAG_C))
		JUMP(MoveObjectRight);
	else
		JUMP(MoveObjectLeft);
}

PORT_LOCAL_JUMP(WormwouldFunc, DefeatedWait)
{
	ww_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .defeat_wait_over
		ww_set_update(gb, hl, WW_DEFEATED_FALL);
		play_sfx(gb, SFX_0A3);
		ww_defeated_fall(gb);
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		ww_set(gb, hl, OBJSUBFLAG_HDIR_F);
	else
		ww_res(gb, hl, OBJSUBFLAG_HDIR_F);
	ww_res(gb, hl, OBJSUBFLAG_UNK_4_F);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x80);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	ww_set(gb, hl, OBJFLAG_PRIORITY_F);
	gb->cpu.a = BOSS_DEFEATED;
	wr8(gb, wBossState, BOSS_DEFEATED);
	CALL(UpdateLevelMusic);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wIsBossBattle, 0);
	RET();
}

PORT_LOCAL_JUMP(WormwouldFunc, DefeatedFall)
{
	ww_defeated_fall(gb);
}
