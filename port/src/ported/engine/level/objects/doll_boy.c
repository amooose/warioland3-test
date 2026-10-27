// Port of src/engine/level/objects/doll_boy.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define HAMMER_PLATFORM_SPAWN (ADDR_HammerPlatformSpawnerFunc + 0x16) // .SpawnHammerPlatform

// Waits 150 frames, then spawns a hammer platform.
PORT(HammerPlatformSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HAMMER_PLATFORM_SPAWN >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HAMMER_PLATFORM_SPAWN;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_HL(gb, hl);
	wr8(gb, wCurObjStateDuration, 150);
	// .DoNothing
	wr8(gb, wCurObjFrameDuration, 2);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

PORT(UpdateDollBoyRemainingParts)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wCurObjSubState));
	if (!gb->cpu.a)
		return; // exit if wCurObjSubState == $1
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjId), 0x0f);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wDollBoyRemainingParts);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.b);
	if (!gb->cpu.a)
		return;
	// unset flag in unk7 from wDollBoyRemainingParts
	gb->cpu.a = alu_xor(gb, gb->cpu.b, 0xff);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.e);
	wr8(gb, wDollBoyRemainingParts, gb->cpu.a);
	gb->cpu.a = 0x08;
	wr8(gb, wCurObjVar1, 0x08);
	SET_HL(gb, wCurObjSubState);
	wr8(gb, wCurObjSubState, alu_dec(gb, rd8(gb, wCurObjSubState)));
}

// wDollBoyActiveBarrels flags
#define DOLLBOY_BARREL1 (1 << 0) // top barrel
#define DOLLBOY_BARREL2 (1 << 1) // middle barrel
#define DOLLBOY_BARREL3 (1 << 2) // bottom barrel
#define DOLLBOY_DOLL (1 << 3)

// wDollBoyHammerRange constants
#define HAMMER_SHORT 1
#define HAMMER_MID_SHORT 2
#define HAMMER_MID_LONG 3

#define BARREL_FLASH_DELAY 70 // delay before a barrel starts flashing
#define BARREL_FLASH_DURATION 240 // duration of barrel flashing
#define HAMMER_PLATFORM_DURATION 140 // duration for hammer platform movement (going up or down)

#define BARREL_START_DELAY (ADDR_DollBoyBarrelFunc + 0x2c) // .StartDelay
#define BARREL_MAIN (ADDR_DollBoyBarrelFunc + 0x40) // .Main
#define HAMMER_INIT (ADDR_DollBoyHammerFunc + 0x0e) // .Init
#define HAMMER_UPDATE (ADDR_DollBoyHammerFunc + 0x2a) // .Update
#define HAMMER_HIT_LEFT (ADDR_DollBoyHammerFunc + 0x92) // .HitLeft
#define HAMMER_HIT_RIGHT (ADDR_DollBoyHammerFunc + 0x97) // .HitRight
#define SPAWNER_DO_NOTHING (ADDR_HammerPlatformSpawnerFunc + 0x2a) // .DoNothing
#define PLATFORM_MOVE_DOWN (ADDR_HammerPlatformFunc + 0x13) // .MoveDown
#define PLATFORM_MOVE_UP (ADDR_HammerPlatformFunc + 0x48) // .MoveUp

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

static void set_bits(GB *gb, uint16_t hl, uint8_t set, uint8_t res)
{
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~res) | set));
}

static void store_update(GB *gb, uint16_t hl, uint16_t update)
{
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

static void set_frameset(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
}

static void no_actions(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// .DoNothing
static void do_nothing(GB *gb)
{
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	no_actions(gb);
}

PORT_LOCAL(HammerPlatformSpawnerFunc, SpawnHammerPlatform)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), SPAWNER_DO_NOTHING);
		SET_BC(gb, ADDR_ObjParams_HammerPlatform);
		CALL(CreateObjectAtRelativePos);
	}
	do_nothing(gb);
}

PORT_LOCAL(HammerPlatformSpawnerFunc, DoNothing) { do_nothing(gb); }

PORT_JUMP(DollBoyGroundShake)
{
	play_sfx(gb, SFX_061);
	gb->cpu.b = 24;
	JUMP(DoGroundShake);
}

// a = wCurObjSubState
PORT(DollBoyBarrelFunc)
{
	wr8(gb, wCurObjSubState, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	hl = LO(hl, OBJ_SUBSTATE);
	set_bits(gb, hl, 1 << OBJSUBFLAG_UNK_4_F, OBJSUBFLAG_UNINITIALISED);
	store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), BARREL_START_DELAY);
	set_frameset(gb, ADDR_Frameset_6969f);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 7);
	wr8(gb, hl--, (uint8_t)-8);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar1, 0);
}

PORT_JUMP(DollBoyBarrel1Func)
{
	gb->cpu.a = 0x03;
	JUMP_TO(ADDR_DollBoyBarrelFunc);
}

PORT_JUMP(DollBoyBarrel2Func)
{
	gb->cpu.a = 0x02;
	JUMP_TO(ADDR_DollBoyBarrelFunc);
}

PORT_JUMP(DollBoyBarrel3Func)
{
	gb->cpu.a = 0x01;
	JUMP_TO(ADDR_DollBoyBarrelFunc); // fallthrough
}

// Waits until it's possible to start flashing.
PORT_LOCAL(DollBoyBarrelFunc, StartDelay)
{
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	gb->cpu.a = rd8(gb, wDollBoyNoFlashingBarrel);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	store_update(gb, wCurObjUpdateFunction + 1, BARREL_MAIN);
}

static void set_no_barrel_flashing(GB *gb)
{
	gb->cpu.a = TRUE;
	wr8(gb, wDollBoyNoFlashingBarrel, TRUE);
}

// falls (var3 = movement index), shaking the ground at shake
static void barrel_fall(GB *gb, uint8_t shake, uint8_t end, uint16_t movement)
{
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, shake);
	if (gb->cpu.f & FLAG_Z) {
		gb_call(gb, ADDR_DollBoyGroundShake);
	} else {
		alu_cp(gb, gb->cpu.a, end);
		if (gb->cpu.f & FLAG_Z) {
			// .end_falling
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjVar1, 0);
			RET();
			return;
		}
	}
	SET_BC(gb, movement);
	JUMP(ApplyObjYMovement);
}

PORT_LOCAL_JUMP(DollBoyBarrelFunc, Main)
{
	gb->cpu.a = rd8(gb, wCurObjVar1);
	uint8_t a = gb->cpu.a;
	if (!alu_and(gb, a, a)) {
		// .not_flashing
		gb_call(gb, ADDR_UpdateDollBoyRemainingParts);
		gb->cpu.a = rd8(gb, wCurObjVar1);
		alu_cp(gb, gb->cpu.a, 0x08);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjId), 0x03);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		gb->cpu.a = rd8(gb, wDollBoyNoFlashingBarrel);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		gb->cpu.a = rd8(gb, wDollBoyActiveBarrels);
		bool flash = false;
		static const uint8_t single[] = {DOLLBOY_BARREL1, DOLLBOY_BARREL2, DOLLBOY_BARREL3};
		for (int i = 0; i < 3 && !flash; i++) {
			alu_cp(gb, gb->cpu.a, single[i]);
			flash = gb->cpu.f & FLAG_Z;
		}
		if (!flash) {
			gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
			gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
			gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
			alu_cp(gb, gb->cpu.a, 8);
			if (gb->cpu.f & FLAG_C) {
				RET();
				return;
			}
			alu_cp(gb, gb->cpu.a, (uint8_t)-8);
			if (!(gb->cpu.f & FLAG_C)) {
				RET();
				return;
			}
		}
		// .set_flashing
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wDollBoyNoFlashingBarrel, 0);
		gb->cpu.a = alu_and(gb, rd8(gb, rDIV), 0x03);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_add(gb, BARREL_FLASH_DELAY, gb->cpu.b);
		uint16_t hl = wCurObjStateDuration;
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = 0x01;
		wr8(gb, hl, 0x01); // wCurObjVar1
		SET_HL(gb, hl);
		RET();
		return;
	}
	alu_cp(gb, a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		// .wait_to_start_flashing
		uint16_t hl = wCurObjStateDuration;
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			set_frameset(gb, ADDR_Frameset_696a8);
			hl = REG_HL(gb);
			gb->cpu.a = BARREL_FLASH_DURATION;
			wr8(gb, hl++, BARREL_FLASH_DURATION);
			gb->cpu.a = 0x02;
			wr8(gb, hl, 0x02);
			SET_HL(gb, hl);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjState, 0);
		}
		RET();
		return;
	}
	alu_cp(gb, a, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		// .flashing
		uint16_t hl;
		gb->cpu.a = rd8(gb, wTransformation);
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_BOUNCY_WARIO);
		if (gb->cpu.f & FLAG_Z) {
			hl = wCurObjStateDuration;
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl++, 0);
		} else {
			hl = wCurObjFlags;
			SET_HL(gb, hl);
			if (alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
				gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
				if (gb->cpu.f & FLAG_Z)
					play_sfx(gb, SFX_06A);
			}
			hl = lo_hl(gb, OBJ_STATE);
			gb->cpu.a = rd8(gb, hl--);
			SET_HL(gb, hl);
			alu_cp(gb, gb->cpu.a, 0x05);
			if (gb->cpu.f & FLAG_Z) {
				// .attacked
				gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK);
				wr8(gb, hl--, gb->cpu.a);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, hl--, 0); // wCurObjVar3
				gb->cpu.a = 0x01;
				wr8(gb, hl--, 0x01); // wCurObjVar2
				SET_HL(gb, hl);
				set_frameset(gb, ADDR_Frameset_6969f);
				store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), ADDR_Func_3317);
				play_sfx(gb, SFX_017);
				hl = wDollBoyActiveBarrels;
				SET_HL(gb, hl);
				gb->cpu.a = rd8(gb, wCurObjId);
				uint8_t clear, remaining;
				alu_cp(gb, gb->cpu.a, DOLL_BOY_BARREL1);
				if (gb->cpu.f & FLAG_Z) {
					clear = DOLLBOY_BARREL1;
					remaining = DOLLBOY_DOLL;
				} else {
					alu_cp(gb, gb->cpu.a, 0x42);
					if (gb->cpu.f & FLAG_Z) {
						clear = DOLLBOY_BARREL2;
						remaining = DOLLBOY_BARREL1 | DOLLBOY_DOLL;
					} else {
						clear = DOLLBOY_BARREL3;
						remaining = DOLLBOY_BARREL1 | DOLLBOY_BARREL2 | DOLLBOY_DOLL;
					}
				}
				gb->cpu.a = alu_and(gb, rd8(gb, hl), (uint8_t)(0xff ^ clear));
				wr8(gb, hl, gb->cpu.a);
				gb->cpu.a = remaining;
				wr8(gb, wDollBoyRemainingParts, remaining);
				set_no_barrel_flashing(gb);
				RET();
				return;
			}
			hl = lo_hl(gb, OBJ_STATE_DURATION);
			uint8_t d = alu_dec(gb, rd8(gb, hl));
			wr8(gb, hl, d);
			if (d) {
				RET();
				return;
			}
			gb->cpu.l = alu_inc(gb, gb->cpu.l);
			hl = REG_HL(gb);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
		}
		// .end_flashing
		wr8(gb, hl, gb->cpu.a); // wCurObjVar1
		SET_HL(gb, hl);
		set_frameset(gb, ADDR_Frameset_6969f);
		store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), BARREL_MAIN);
		set_no_barrel_flashing(gb);
		RET();
		return;
	}
	alu_cp(gb, a, 0x08);
	bool start = gb->cpu.f & FLAG_Z;
	if (!start) {
		alu_cp(gb, a, 0x09);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	} else {
		// .start_fall
		uint16_t hl = wCurObjVar1;
		gb->cpu.a = 0x09;
		wr8(gb, hl++, 0x09);
		SET_HL(gb, hl);
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, REG_HL(gb), 0); // wCurObjVar3
	}
	// .fall
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, DOLL_BOY_BARREL1);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wDollBoyActiveBarrels);
		alu_cp(gb, gb->cpu.a, DOLLBOY_BARREL1 | DOLLBOY_BARREL2);
		if (gb->cpu.f & FLAG_Z) {
			barrel_fall(gb, 0x1a, 0x1e, ADDR_Data_60b80);
			return;
		}
	}
	barrel_fall(gb, 0x0a, 0x0e, ADDR_Data_60880);
}

PORT(DollBoyHammerFunc)
{
	store_update(gb, wCurObjUpdateFunction + 1, HAMMER_INIT);
	uint16_t hl = lo_hl(gb, OBJ_FLAGS);
	set_bits(gb, hl, OBJFLAG_NO_COLLISION, 0);
}

// A hammer thrown from below Wario flies straight right.
PORT_LOCAL(DollBoyHammerFunc, Init)
{
	store_update(gb, wCurObjUpdateFunction + 1, HAMMER_UPDATE);
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0xec);
	if (gb->cpu.f & FLAG_C)
		return;
	uint16_t hl = lo_hl(gb, OBJ_VAR_1);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

PORT_LOCAL_JUMP(DollBoyHammerFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .hit
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? HAMMER_HIT_LEFT : HAMMER_HIT_RIGHT);
		uint16_t hl = wCurObjUpdateFunction + 1;
		gb->cpu.a = gb->cpu.d;
		wr8(gb, hl--, gb->cpu.a);
		wr8(gb, hl, gb->cpu.e);
		SET_HL(gb, hl);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjVar3, 0);
		play_sfx(gb, SFX_049);
		SET_BC(gb, ADDR_ObjParams_StarsCentre);
		JUMP(CreateObjectFromCurObjPos);
		return;
	}
	// if var 1 is set, this hammer just moves right
	gb->cpu.a = rd8(gb, wCurObjVar1);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(MoveObjectRight_Fast);
		return;
	}
	SET_BC(gb, ADDR_Data_60860);
	CALL(ApplyObjYMovement);
	gb->cpu.a = rd8(gb, wDollBoyHammerRange);
	uint16_t movement = ADDR_Data_60b60;
	alu_cp(gb, gb->cpu.a, HAMMER_SHORT);
	if (gb->cpu.f & FLAG_Z) {
		movement = ADDR_Data_60ae0;
	} else {
		alu_cp(gb, gb->cpu.a, HAMMER_MID_SHORT);
		if (gb->cpu.f & FLAG_Z) {
			movement = ADDR_Data_60b00;
		} else {
			alu_cp(gb, gb->cpu.a, HAMMER_MID_LONG);
			if (gb->cpu.f & FLAG_Z)
				movement = ADDR_Data_60b30;
		}
	}
	SET_BC(gb, movement);
	JUMP(ApplyObjXMovement);
}

static void hit_fall(GB *gb)
{
	no_actions(gb);
	SET_BC(gb, ADDR_Data_60040);
	JUMP(ApplyObjYMovement);
}

PORT_LOCAL_JUMP(DollBoyHammerFunc, HitLeft)
{
	CALL(MoveObjectLeft);
	hit_fall(gb);
}

PORT_LOCAL_JUMP(DollBoyHammerFunc, HitRight)
{
	CALL(MoveObjectRight);
	hit_fall(gb);
}

// Moves down (carrying Wario) until it's hit or its duration runs out.
static void move_down(GB *gb)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	bool up = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (!up) {
		hl = lo_hl(gb, OBJ_STATE_DURATION);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		up = !d;
	}
	if (up) {
		// .switch_direction_up
		store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), PLATFORM_MOVE_UP);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjState, 0);
		RET();
		return;
	}
	hl = lo_hl(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectDown_Slow);
		return;
	}
	set_bits(gb, hl, 0, OBJFLAG_STEPPED);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	gb->cpu.b = 1;
	gb_call_ret(gb, ADDR_MoveWarioDown, ADDR_HammerPlatformFunc + 0x35);
	gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, ADDR_HammerPlatformFunc + 0x38);
	JUMP(MoveObjectDown);
}

PORT_JUMP(HammerPlatformFunc)
{
	store_update(gb, wCurObjUpdateFunction + 1, PLATFORM_MOVE_DOWN);
	uint16_t hl = lo_hl(gb, OBJ_Y_POS);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCurObjVar2, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurObjVar3, gb->cpu.a);
	SET_HL(gb, hl);
	move_down(gb);
}

PORT_LOCAL_JUMP(HammerPlatformFunc, MoveDown) { move_down(gb); }

// Moves back up to where it started.
PORT_LOCAL_JUMP(HammerPlatformFunc, MoveUp)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.b = rd8(gb, hl);
	gb->cpu.a = rd8(gb, wCurObjYPos + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool down = false;
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.b = rd8(gb, REG_HL(gb));
		gb->cpu.a = rd8(gb, wCurObjYPos + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		down = gb->cpu.f & FLAG_Z;
	}
	if (down) {
		// .switch_direction_down
		store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), PLATFORM_MOVE_DOWN);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjState, 0);
		gb->cpu.a = HAMMER_PLATFORM_DURATION;
		wr8(gb, wCurObjStateDuration, HAMMER_PLATFORM_DURATION);
		RET();
		return;
	}
	hl = lo_hl(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectUp_Slow);
		return;
	}
	set_bits(gb, hl, 0, OBJFLAG_STEPPED);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	gb->cpu.b = 1;
	gb_call_ret(gb, ADDR_MoveWarioUp, ADDR_HammerPlatformFunc + 0x70);
	gb_call_ret(gb, ADDR_HandleUpwardsFloorTransition, ADDR_HammerPlatformFunc + 0x73);
	JUMP(MoveObjectUp);
}

#define HAMMER_LONG 4
#define DOLLBOY_THROW_HAMMER_ODDS 250 // out of 256
#define DOLLBOY_START_DELAY 40 // delay before starting actions
#define DOLLBOY_THROW_HAMMER_DELAY 56 // delay before throwing a hammer
#define DOLLBOY_WALKING_PHASE_DELAY 80 // delay before beginning walking phase
#define DOLLBOY_JUMP_DELAY 20 // delay before jumping up/down
#define DOLLBOY_TURN_DELAY 32 // delay before turning around
#define DOLLBOY_AFTER_HIT_DELAY 60 // delay after hitting Wario with the hammer
#define DOLLBOY_STUN_DURATION 120 // stun duration where Doll Boy is vulnerable
#define DOLLBOY_DEFEAT_DURATION 43 // duration after getting defeated to fall off screen

#define DB(off) ((uint16_t)(ADDR_DollBoyFunc + (off)))
#define DB_INIT DB(0x34) // .Init
#define DB_BARREL_PHASE DB(0x65) // .BarrelPhase
#define DB_INIT_WALKING_PHASE DB(0x1ab) // .InitWalkingPhase
#define DB_WALKING_PHASE DB(0x2c4) // .WalkingPhase
#define DB_HIT_WARIO DB(0x485) // .HitWario
#define DB_WAIT_THEN_DO_NOTHING DB(0x4ca) // .WaitThenDoNothing
#define DB_DO_NOTHING DB(0x4e9) // .DoNothing
#define DB_DEFEATED DB(0x4ef) // .Defeated

// ld [hli], a with the hl SetObjectFramesetPtr left
static void ld_hli(GB *gb, uint8_t v)
{
	gb->cpu.a = v;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
}

static void stop_sfx(GB *gb)
{
	gb->cpu.a = 0xff;
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
}

// Copies pals (4 and 5) to wTempPals2 palette 4 and applies them.
static void copy_pals(GB *gb, uint16_t pals, bool jump)
{
	SET_HL(gb, pals);
	SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
	gb->cpu.c = 4;
	gb->cpu.b = 2;
	if (jump)
		JUMP(CopyAndApplyOBPals);
	else
		CALL(CopyAndApplyOBPals);
}

PORT(DollBoyFunc)
{
	gb->cpu.a = TRUE;
	wr8(gb, wIsBossBattle, TRUE);
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	hl = LO(hl, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK), 0x5 | OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), DB_INIT);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 11);
	wr8(gb, hl--, (uint8_t)-12);
	wr8(gb, hl--, 0);
	gb->cpu.a = (uint8_t)-28;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_69632);
	ld_hli(gb, DOLLBOY_START_DELAY);
}

// Waits on screen, then starts the battle.
PORT_LOCAL(DollBoyFunc, Init)
{
	gb->cpu.a = alu_rra(gb, rd8(gb, wCurObjFlags));
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C))
		return; // OBJFLAG_ON_SCREEN_F not set
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	store_update(gb, wCurObjUpdateFunction + 1, DB_BARREL_PHASE);
	gb->cpu.a = TRUE;
	wr8(gb, wDollBoyNoFlashingBarrel, TRUE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar1, 0);
	wr8(gb, wDollBoyFallImmediatelyMiddleBarrel, 0);
	wr8(gb, wDollBoyHammerRange, 0);
	gb->cpu.a = DOLLBOY_BARREL1 | DOLLBOY_BARREL2 | DOLLBOY_BARREL3;
	wr8(gb, wDollBoyActiveBarrels, gb->cpu.a);
	gb->cpu.a = BOSS_REGULAR;
	wr8(gb, wBossState, BOSS_REGULAR);
	CALL(UpdateLevelMusic);
}

// SetObjectFramesetPtr(Frameset_69632) / inc l / [hl] = 0
static void after_fall(GB *gb)
{
	set_frameset(gb, ADDR_Frameset_69632);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
	RET();
}

// Falls with the barrel below: frameset at var3 = frame, shake at shake,
// lands at end.
static void doll_fall(GB *gb, uint8_t frame, uint8_t shake, uint8_t end, uint16_t movement)
{
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, frame);
	if (gb->cpu.f & FLAG_Z) {
		set_frameset(gb, ADDR_Frameset_69646);
	} else {
		alu_cp(gb, gb->cpu.a, shake);
		if (gb->cpu.f & FLAG_Z) {
			gb_call(gb, ADDR_DollBoyGroundShake);
		} else {
			alu_cp(gb, gb->cpu.a, end);
			if (gb->cpu.f & FLAG_Z) {
				after_fall(gb);
				return;
			}
		}
	}
	SET_BC(gb, movement);
	JUMP(ApplyObjYMovement);
}

static void fall_wait_1_barrel(GB *gb) { doll_fall(gb, 18, 26, 30, ADDR_Data_60b80); }
static void fall_immediately(GB *gb) { doll_fall(gb, 2, 10, 14, ADDR_Data_60880); }

// Throws hammers, falls when a barrel below it is attacked.
PORT_LOCAL_JUMP(DollBoyFunc, BarrelPhase)
{
	gb->cpu.a = rd8(gb, wCurObjVar1);
	uint8_t a = gb->cpu.a;
	if (!alu_and(gb, a, a)) {
		// .check_if_wario_is_bouncy
		gb->cpu.a = rd8(gb, wTransformation);
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_BOUNCY_WARIO);
		if (gb->cpu.f & FLAG_Z) {
			store_update(gb, wCurObjUpdateFunction + 1, DB_DO_NOTHING);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wBossState, 0);
			RET();
			return;
		}
		// .not_bouncy
		gb->cpu.a = rd8(gb, wCurObjSubState);
		alu_cp(gb, gb->cpu.a, 0x2 | OBJSUBFLAG_HDIR);
		if (gb->cpu.f & FLAG_Z) {
			// .walking_phase
			store_update(gb, wCurObjUpdateFunction + 1, DB_INIT_WALKING_PHASE);
			uint16_t hl = lo_hl(gb, OBJ_INTERACTION_TYPE);
			gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_0B);
			wr8(gb, hl++, gb->cpu.a);
			SET_HL(gb, hl);
			gb->cpu.a = DOLLBOY_WALKING_PHASE_DELAY;
			wr8(gb, wCurObjStateDuration, gb->cpu.a);
			RET();
			return;
		}
		gb_call(gb, ADDR_UpdateDollBoyRemainingParts);
		gb->cpu.a = rd8(gb, wCurObjVar1);
		alu_cp(gb, gb->cpu.a, 0x08);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		gb->cpu.a = rd8(gb, rDIV);
		alu_cp(gb, gb->cpu.a, DOLLBOY_THROW_HAMMER_ODDS);
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		// throw hammer
		set_frameset(gb, ADDR_Frameset_6963b);
		ld_hli(gb, DOLLBOY_THROW_HAMMER_DELAY); // wCurObjStateDuration
		gb->cpu.a = 0x01;
		wr8(gb, REG_HL(gb), 0x01); // wCurObjVar1
		RET();
		return;
	}
	alu_cp(gb, a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		// .throw_hammer
		gb_call(gb, ADDR_UpdateDollBoyRemainingParts);
		gb->cpu.a = rd8(gb, wCurObjVar1);
		alu_cp(gb, gb->cpu.a, 0x08);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		uint16_t hl = wCurObjStateDuration;
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			// .after_hammer_throw
			after_fall(gb);
			return;
		}
		gb->cpu.a = d;
		alu_cp(gb, d, 0x13);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		hl = wDollBoyHammerRange;
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, HAMMER_LONG);
		if (gb->cpu.f & FLAG_Z) {
			// restart hammer range cycle
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
		}
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl))); // next hammer range
		play_sfx(gb, SFX_0A2);
		SET_BC(gb, ADDR_ObjParams_DollBoyHammer);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	alu_cp(gb, a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		// .start_fall
		set_frameset(gb, ADDR_Frameset_69632);
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		ld_hli(gb, 0x09); // wCurObjVar1
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, REG_HL(gb), 0); // wCurObjVar3
	} else {
		alu_cp(gb, a, 0x09);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	// .fall
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
	alu_cp(gb, gb->cpu.a, 0x4);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wDollBoyActiveBarrels);
		alu_cp(gb, gb->cpu.a, DOLLBOY_BARREL1 | DOLLBOY_BARREL2);
		if (gb->cpu.f & FLAG_Z) {
			// .fall_wait_2_barrels: bottom barrel attacked
			gb->cpu.a = TRUE;
			wr8(gb, wDollBoyFallImmediatelyMiddleBarrel, TRUE);
			doll_fall(gb, 34, 42, 46, ADDR_Data_60ba0);
			return;
		}
		alu_cp(gb, gb->cpu.a, DOLLBOY_BARREL1 | DOLLBOY_BARREL3);
		if (gb->cpu.f & FLAG_Z)
			fall_wait_1_barrel(gb); // middle barrel attacked
		else
			fall_immediately(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x3);
	if (!(gb->cpu.f & FLAG_Z)) {
		fall_immediately(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wDollBoyActiveBarrels);
	alu_cp(gb, gb->cpu.a, DOLLBOY_BARREL1);
	if (gb->cpu.f & FLAG_Z) {
		fall_wait_1_barrel(gb); // only top barrel
		return;
	}
	alu_cp(gb, gb->cpu.a, DOLLBOY_BARREL3);
	if (gb->cpu.f & FLAG_Z) {
		fall_immediately(gb); // only bottom barrel
		return;
	}
	gb->cpu.a = rd8(gb, wDollBoyFallImmediatelyMiddleBarrel);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		fall_immediately(gb);
	else
		fall_wait_1_barrel(gb);
}

PORT_LOCAL_JUMP(DollBoyFunc, InitWalkingPhase)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	set_frameset(gb, ADDR_Frameset_6964d);
	store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), DB_WALKING_PHASE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	wr8(gb, wCurObjVar1, 0);
	copy_pals(gb, ADDR_DollBoyGreenPals, true);
}

// hl = wCurObjState
static void set_stunned(GB *gb)
{
	play_sfx(gb, SFX_0A4);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0x5c;
	wr8(gb, hl--, 0x5c); // wCurObjState
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--)); // wCurObjSubState
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69675 : ADDR_Frameset_6967a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0); // wCurObjVar3
	CALL(SetObjectFramesetPtr);
	ld_hli(gb, DOLLBOY_STUN_DURATION); // wCurObjStateDuration
	gb->cpu.a = 0xff;
	wr8(gb, REG_HL(gb), 0xff); // wCurObjVar1
	copy_pals(gb, ADDR_DollBoyRedPals, true);
}

static void update_stunned(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, DB(0x254));
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	SET_BC(gb, ADDR_FallingYVel_Light);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(ApplyObjYMovement);
		return;
	}
	hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE_DURATION);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		gb->cpu.a = d;
		alu_cp(gb, d, 10);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		copy_pals(gb, ADDR_DollBoyGreenPals, true);
		return;
	}
	// .end_stun
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	bool right = gb->cpu.f & FLAG_C;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool left_of_wario = gb->cpu.f & FLAG_C;
	if (!right) {
		SET_DE(gb, left_of_wario ? ADDR_Frameset_6966a : ADDR_Frameset_69656);
		gb->cpu.b = left_of_wario ? 32 : 0;
	} else {
		SET_DE(gb, left_of_wario ? ADDR_Frameset_6964d : ADDR_Frameset_6965f);
		gb->cpu.b = left_of_wario ? 0 : 32;
	}
	// .asm_50d7c
	CALL(SetObjectFramesetPtr);
	wr8(gb, REG_HL(gb), gb->cpu.b); // wCurObjStateDuration
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar1, 0);
	wr8(gb, wCurObjVar3, 0);
	RET();
}

// Jumps to the platform below or above.
static void jump_platform(GB *gb, uint16_t movement)
{
	SET_BC(gb, movement);
	uint16_t hl = lo_hl(gb, OBJ_STATE_DURATION);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		RET();
		return;
	}
	CALL(ApplyObjYMovement);
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_DollBoyGroundShake);
	hl = wCurObjVar1;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = 0x3c;
	wr8(gb, hl, 0x3c);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	RET();
}

// .show_stars_left / .show_stars_right: hit Wario with the hammer
static void show_stars(GB *gb, uint16_t frameset, uint16_t params)
{
	SET_DE(gb, frameset);
	SET_BC(gb, params);
	store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), DB_HIT_WARIO);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	wr8(gb, wBossState, 0);
	stop_sfx(gb);
	CALL(SetObjectFramesetPtr);
	ld_hli(gb, 1); // wCurObjStateDuration
	JUMP(CreateObjectAtRelativePos);
}

// .handle_walk_movement
static void walk(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurObjFramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x06);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wCurObjFrameDuration);
		alu_cp(gb, gb->cpu.a, 0x01);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_0A2);
	}
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 4);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = gb->cpu.f & FLAG_C;
	if (right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 11);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 12);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, DB(right ? 0x45d : 0x431));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .turn_right / .turn_left
		set_frameset(gb, right ? ADDR_Frameset_6965f : ADDR_Frameset_6966a);
		ld_hli(gb, DOLLBOY_TURN_DELAY); // wCurObjStateDuration
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjVar3, 0);
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wCurObjFramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x06);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_BOUNCY);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (right)
		JUMP(MoveObjectRight);
	else
		JUMP(MoveObjectLeft);
}

// .switch_direction: turning around, or walking (and jumping between
// platforms after a while)
static void switch_direction(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			// .asm_50e6c
			gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
			SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_6964d : ADDR_Frameset_69656);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		hl = lo_hl(gb, OBJ_VAR_3);
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x17);
		if (gb->cpu.f & FLAG_Z) {
			// switch horizontal directions
			gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
			wr8(gb, hl, gb->cpu.a);
			gb_call(gb, ADDR_DollBoyGroundShake);
		}
		SET_BC(gb, ADDR_Data_60c00);
		JUMP(ApplyObjYMovement);
		return;
	}
	// .asm_50e7e
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl); // wCurObjVar1
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		walk(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wCurObjScreenXPos);
	alu_cp(gb, gb->cpu.a, 0x78);
	if (!(gb->cpu.f & FLAG_C)) {
		walk(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_C) {
		walk(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wWarioScreenYPos);
	alu_cp(gb, gb->cpu.a, 0x28);
	if (gb->cpu.f & FLAG_C) {
		walk(gb);
		return;
	}
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0xa0);
	if (!(gb->cpu.f & FLAG_C)) {
		walk(gb);
		return;
	}
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	uint8_t state;
	if (gb->cpu.f & FLAG_C) {
		alu_cp(gb, gb->cpu.a, (uint8_t)-0x1f);
		if (!(gb->cpu.f & FLAG_C)) {
			walk(gb);
			return;
		}
		state = 0x5a; // jump to platform below
	} else {
		alu_cp(gb, gb->cpu.a, 0x20);
		if (gb->cpu.f & FLAG_C) {
			walk(gb);
			return;
		}
		state = 0x5b; // jump to platform above
	}
	// .asm_50eb1
	gb->cpu.a = state;
	hl = lo_hl(gb, OBJ_STATE);
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0); // wCurObjVar3
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0x1e;
	wr8(gb, hl--, 0x1e); // wCurObjVar1
	gb->cpu.a = DOLLBOY_JUMP_DELAY;
	wr8(gb, hl, DOLLBOY_JUMP_DELAY); // wCurObjStateDuration
	SET_HL(gb, hl);
	RET();
}

PORT_LOCAL_JUMP(DollBoyFunc, WalkingPhase)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl++, gb->cpu.a);
	hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t a = gb->cpu.a;
	if (!alu_and(gb, a, a)) {
		switch_direction(gb);
		return;
	}
	static const uint8_t states[] = {0x5a, 0x5b, 0x5c, 0x02, 0x03, 0x32, 0x33, 0x0b, 0x0c};
	int i;
	for (i = 0; i < 9; i++) {
		alu_cp(gb, a, states[i]);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	switch (i) {
	case 0:
		jump_platform(gb, ADDR_Data_60890);
		return;
	case 1:
		jump_platform(gb, ADDR_Data_608a0);
		return;
	case 2:
		update_stunned(gb);
		return;
	case 3:
	case 4:
		// .asm_50cc4 / .asm_50cc9
		gb->cpu.a = i == 3 ? 0x32 : 0x33;
		wr8(gb, hl--, gb->cpu.a); // wCurObjState
		SET_HL(gb, hl);
		gb->cpu.a = 12;
		wr8(gb, wCurObjStateDuration, 12);
		gb->cpu.a = 0x02;
		wr8(gb, wCurObjVar2, 0x02);
		RET();
		return;
	case 5:
		JUMP(ObjState_BumpLeft);
		return;
	case 6:
		JUMP(ObjState_BumpRight);
		return;
	case 7:
		show_stars(gb, ADDR_Frameset_696b9, ADDR_ObjParams_DollBoyHammerStarsLeft);
		return;
	case 8:
		show_stars(gb, ADDR_Frameset_696b6, ADDR_ObjParams_DollBoyHammerStarsRight);
		return;
	}
	gb->cpu.a = alu_and(gb, a, 0xfe);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		set_stunned(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x04);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = alu_inc(gb, rd8(gb, wCurObjVar1));
		if (gb->cpu.f & FLAG_Z) {
			set_stunned(gb);
			return;
		}
		gb->cpu.a = 1;
		wr8(gb, wCurObjStateDuration, 1);
		update_stunned(gb);
		return;
	}
	// .asm_50de0
	gb->cpu.a = alu_inc(gb, rd8(gb, wCurObjVar1));
	if (!(gb->cpu.f & FLAG_Z)) {
		set_stunned(gb);
		return;
	}
	gb->cpu.a = 0x48;
	wr8(gb, hl--, 0x48); // wCurObjState
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK); // wCurObjSubState
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0); // wCurObjVar3
	SET_HL(gb, hl);
	gb->cpu.a = DOLLBOY_DEFEAT_DURATION;
	wr8(gb, wCurObjStateDuration, gb->cpu.a);
	SET_BC(gb, DB_DEFEATED);
	hl = lo_hl(gb, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_FLAGS);
	set_bits(gb, hl, OBJFLAG_PRIORITY, 0);
	lo_hl(gb, OBJ_UNK_02);
	gb->cpu.e = 0x05;
	FARCALL(Func_ba42);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);
	SET_DE(gb, ADDR_Frameset_6967f);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(DollBoyFunc, HitWario)
{
	no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		play_sfx(gb, SFX_049);
	}
	hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, DB(0x4ac));
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	SET_BC(gb, ADDR_FallingYVel_Light);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(ApplyObjYMovement);
		return;
	}
	hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), DB_WAIT_THEN_DO_NOTHING);
	gb->cpu.a = DOLLBOY_AFTER_HIT_DELAY;
	wr8(gb, wCurObjStateDuration, gb->cpu.a);
	RET();
}

PORT_LOCAL_JUMP(DollBoyFunc, WaitThenDoNothing)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), DB_DO_NOTHING);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69695 : ADDR_Frameset_6969a);
	JUMP(SetObjectFramesetPtr);
}

// Wario was hit by a hammer, so no more actions to process
PORT_LOCAL(DollBoyFunc, DoNothing) { no_actions(gb); }

PORT_LOCAL_JUMP(DollBoyFunc, Defeated)
{
	no_actions(gb);
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0xd0);
	if (!(gb->cpu.f & FLAG_C)) {
		// .set_defeat_music
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFlags, 0);
		gb->cpu.a = BOSS_DEFEATED;
		wr8(gb, wBossState, BOSS_DEFEATED);
		CALL(UpdateLevelMusic);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wIsBossBattle, 0);
		JUMP_TO(ADDR_DollBoyGroundShake);
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		alu_cp(gb, gb->cpu.a, 27);
		if (gb->cpu.f & FLAG_Z) {
			stop_sfx(gb);
			copy_pals(gb, ADDR_DollBoyRedPals, false);
			hl = wCurObjStateDuration;
			SET_HL(gb, hl);
		}
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d)
			play_sfx(gb, SFX_0A3);
	}
	SET_BC(gb, ADDR_Data_60bd0);
	JUMP(ApplyObjYMovement);
}
