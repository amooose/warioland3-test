// Port of src/engine/level/objects/jamano.asm
#include "ported.h"

// Writes the OAM pointer for the hammer range's frame (flipped by bit 7 of
// [hl]) big-endian to [e] and [e - 1] of hl's page.
PORT(Func_554bc)
{
	static const uint8_t lows[4][2] = {
		{0x37, 0xbc}, // 1
		{0x2e, 0xb3}, // 2
		{0x25, 0xaa}, // 3
		{0x1c, 0xa1}, // 4
	};
	gb->cpu.a = rd8(gb, wDollBoyHammerRange);
	int range;
	for (range = 1; range <= 4; range++) {
		alu_cp(gb, gb->cpu.a, (uint8_t)range);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	if (range > 4)
		return; // ret nz
	bool flip = alu_bit(gb, rd8(gb, REG_HL(gb)), 7);
	gb->cpu.b = 0x5c;
	gb->cpu.c = lows[range - 1][flip];
	// .asm_5550c
	gb->cpu.l = gb->cpu.e;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
}

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define JAMANO_HAT_FUNC_55125 (ADDR_JamanoHatFunc + 0x1b)
#define HAT_PLATFORM_FUNC_55194 (ADDR_HatPlatformFunc + 0x39)
#define SKULL_SPAWNER_FUNC_5522C (ADDR_SkullSpawnerFunc + 0x47)

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static inline uint16_t set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld a, no_actions_for 1 / ld [wCurObjAction], a
static inline void no_actions_for_1(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
}

PORT(JamanoHatFunc)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	wr8(gb, hl, 0x3c);
	gb->cpu.a = 0x03;
	wr8(gb, wCurObjVar2, 0x03);
	SET_HL(gb, set_update(gb, hl, JAMANO_HAT_FUNC_55125));
}

// The hat flies off; once it's off-screen, the boss is defeated.
PORT_LOCAL_JUMP(JamanoHatFunc, Func_55125)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
		JUMP(MoveObjectUpByVar2);
		return;
	}
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
	SET_HL(gb, wCurObjUnk02);
	gb->cpu.e = 0x06;
	FARCALL(Func_ba42);
	gb->cpu.a = BOSS_DEFEATED;
	wr8(gb, wBossState, gb->cpu.a);
	CALL(UpdateLevelMusic);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wIsBossBattle, 0);
	RET();
}

PORT_JUMP(HatPlatformFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = set_update(gb, hl, HAT_PLATFORM_FUNC_55194);
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x05);
	wr8(gb, hl--, 0xfa);
	wr8(gb, hl, 0x00);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69cef);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, wCurObjState, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0x08;
	wr8(gb, wCurObjVar2, 0x08);
	CALL(MoveObjectLeftByVar2);
	gb->cpu.a = 0x5a;
	wr8(gb, wCurObjVar2, 0x5a);
	JUMP(MoveObjectUpByVar2);
}

// Moves the platform (and Wario on it) by one pixel.
static void move_platform(GB *gb, uint16_t hl, bool down)
{
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
		gb->cpu.b = 0x01;
		if (down) {
			CALL(MoveWarioDown);
			CALL(HandleDownwardsFloorTransition);
		} else {
			CALL(MoveWarioUp);
			CALL(HandleUpwardsFloorTransition);
		}
	}
	if (down)
		JUMP(MoveObjectDown);
	else
		JUMP(MoveObjectUp);
}

// Sinks while stood on, then rises back.
PORT_LOCAL_JUMP(HatPlatformFunc, Func_55194)
{
	CALL(SetOneWayPlatformAction);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjVar1;
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x30);
		bool stop = gb->cpu.f & FLAG_Z;
		if (!stop) {
			gb->cpu.a = rd8(gb, wCurObjState);
			stop = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
		}
		if (stop) {
			// .asm_551c3
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			wr8(gb, wCurObjState, 0);
			RET();
			return;
		}
		move_platform(gb, hl, true);
		return;
	}
	// .asm_551c9
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		hl = INC_L(hl);
		gb->cpu.a = 0x01;
		wr8(gb, hl, 0x01);
		SET_HL(gb, hl);
		RET();
		return;
	}
	move_platform(gb, hl, false);
}

PORT(SkullSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SKULL_SPAWNER_FUNC_5522C >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SKULL_SPAWNER_FUNC_5522C;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	gb->cpu.a = 0x08;
	wr8(gb, wCurObjVar2, 0x08);
	CALL(MoveObjectLeftByVar2);
	gb->cpu.a = 0x14;
	wr8(gb, wCurObjVar2, 0x14);
	CALL(MoveObjectUpByVar2);
	SET_BC(gb, ADDR_ObjParams_Skull1);
	CALL(CreateObjectAtRelativePos);
	SET_BC(gb, ADDR_ObjParams_Skull2);
	CALL(CreateObjectAtRelativePos);
	SET_BC(gb, ADDR_ObjParams_Skull3);
	CALL(CreateObjectAtRelativePos);
	SET_BC(gb, ADDR_ObjParams_Skull4);
	CALL(CreateObjectAtRelativePos);
	gb->cpu.a = 0x0f; // JAMANO_SKULL1 | JAMANO_SKULL2 | JAMANO_SKULL3 | JAMANO_SKULL4
	wr8(gb, wJamanoActiveSkulls, 0x0f);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d147, 0);
	wr8(gb, wDollBoyHammerRange, 0);
	gb->cpu.a = 0x01;
	wr8(gb, w1d149, 0x01);
}

// Respawns skulls that were destroyed, until the fight is over.
PORT_LOCAL_JUMP(SkullSpawnerFunc, Func_5522c)
{
	no_actions_for_1(gb);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjFrameDuration, 0x02);
	gb->cpu.a = rd8(gb, w1d149);
	bool active = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (!active) {
		gb->cpu.a = rd8(gb, wDollBoyHammerRange);
		active = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!active) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFlags, 0);
		SET_HL(gb, wCurObjUnk02);
		gb->cpu.e = 0x07;
		FARCALL(Func_ba42);
		RET();
		return;
	}
	// .asm_5525b
	gb_call_ret(gb, ADDR_Func_55407, ADDR_SkullSpawnerFunc + 0x79);
	uint16_t hl = wJamanoActiveSkulls;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	static const uint16_t skulls[4] = {
		ADDR_ObjParams_Skull1, ADDR_ObjParams_Skull2, ADDR_ObjParams_Skull3, ADDR_ObjParams_Skull4,
	};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = alu_rra(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_C)
			continue;
		// .spawn_skull_N
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << i)));
		SET_BC(gb, skulls[i]);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	RET();
}

#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)

#define SKULL(off) (ADDR_SkullFunc + (off))
#define SKULL_FUNC_5529F SKULL(0x12)
#define SKULL_FUNC_552BA SKULL(0x2d)
#define SKULL_FUNC_552D4 SKULL(0x47)
#define SKULL_FUNC_5531C SKULL(0x8f)
#define SKULL_FUNC_55347 SKULL(0xba)
#define SKULL_FUNC_55396 SKULL(0x109)
#define SKULL_FUNC_553A0 SKULL(0x113)
#define FUNC_55407(off) (ADDR_Func_55407 + (off))
#define FUNC_55407_FUNC_5548B FUNC_55407(0x84)

// .Func_553e8: flickers while the fight goes on, else despawns.
static void skull_flicker(GB *gb)
{
	gb->cpu.a = rd8(gb, w1d149);
	bool on = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (!on) {
		gb->cpu.a = rd8(gb, wDollBoyHammerRange);
		on = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!on) {
		wr8(gb, wCurObjFlags, gb->cpu.a);
		return;
	}
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 4)));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 4)));
}

PORT_LOCAL(SkullFunc, Func_553e8) { skull_flicker(gb); }

PORT(SkullFunc)
{
	skull_flicker(gb);
	no_actions_for_1(gb);
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SKULL_FUNC_5529F >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SKULL_FUNC_5529F;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL(SkullFunc, Func_5529f)
{
	skull_flicker(gb);
	no_actions_for_1(gb);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjFrameDuration, 0x02);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	SET_HL(gb, set_update(gb, hl, SKULL_FUNC_552BA));
}

PORT_LOCAL(SkullFunc, Func_552ba)
{
	skull_flicker(gb);
	no_actions_for_1(gb);
	uint16_t hl = wCurObjVar1;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = set_update(gb, hl, SKULL_FUNC_552D4);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_PRIORITY));
}

// Flies to the screen edge on its side, bobbing.
PORT_LOCAL_JUMP(SkullFunc, Func_552d4)
{
	skull_flicker(gb);
	no_actions_for_1(gb);
	gb->cpu.a = rd8(gb, wCurObjId);
	gb->cpu.b = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, SKULL3);
	bool right = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = rd8(gb, wCurObjScreenXPos);
	alu_cp(gb, gb->cpu.a, right ? SCREEN_WIDTH_PX : 0x10);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_55309
		uint16_t hl = wCurObjUpdateFunction + 1;
		gb->cpu.a = (uint8_t)(SKULL_FUNC_5531C >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)SKULL_FUNC_5531C;
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = 0x27;
		wr8(gb, wCurObjStateDuration, 0x27);
		hl = w1d147;
		SET_HL(gb, hl);
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		RET();
		return;
	}
	if (right)
		CALL(MoveObjectRight);
	else
		CALL(MoveObjectLeft);
	gb->cpu.a = alu_rra(gb, gb->cpu.b);
	bool c = gb->cpu.f & FLAG_C;
	SET_BC(gb, c ? ADDR_Data_60d00 : ADDR_Data_60ce0);
	JUMP(ApplyObjYMovement);
}

PORT_LOCAL_JUMP(SkullFunc, Func_5531c)
{
	skull_flicker(gb);
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wCurObjFramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~((1 << 7) | (1 << 4))));
	SET_HL(gb, set_update(gb, hl, SKULL_FUNC_55347));
	SET_DE(gb, ADDR_Frameset_69cd6);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL(SkullFunc, Func_55347)
{
	gb->cpu.a = rd8(gb, w1d149);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		no_actions_for_1(gb);
		skull_flicker(gb);
		return;
	}
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	gb->cpu.a = rd8(gb, wCurObjState);
	uint16_t func;
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_Z) {
		func = SKULL_FUNC_553A0;
	} else {
		alu_cp(gb, gb->cpu.a, 0x05);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		func = SKULL_FUNC_55396;
	}
	// .asm_55370
	gb->cpu.a = (uint8_t)(func >> 8);
	gb->cpu.c = (uint8_t)func;
	hl = wCurObjUpdateFunction + 1;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	hl = LO(hl, OBJ_VAR_3);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69cec);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0xff);
	no_actions_for_1(gb);
	play_sfx(gb, SFX_017);
	hl = w1d147;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
}

// .asm_553a8: the skull falls off the screen, then frees its slot.
static void skull_fall(GB *gb)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0xf7);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_0BF);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, SKULL1);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.b = 0xff ^ (1 << 0);
	} else if (alu_cp(gb, gb->cpu.a, SKULL2), gb->cpu.f & FLAG_Z) {
		gb->cpu.b = 0xff ^ (1 << 1);
	} else if (alu_cp(gb, gb->cpu.a, SKULL3), gb->cpu.f & FLAG_Z) {
		gb->cpu.b = 0xff ^ (1 << 2);
	} else {
		gb->cpu.b = 0xff ^ (1 << 3);
	}
	// .got_skull_mask
	gb->cpu.a = alu_and(gb, rd8(gb, wJamanoActiveSkulls), gb->cpu.b);
	wr8(gb, wJamanoActiveSkulls, gb->cpu.a);
	RET();
}

// .Func_55396 / .Func_553a0
static void skull_knocked(GB *gb, uint16_t func)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
		JUMP_TO(func);
		return;
	}
	skull_fall(gb);
}

PORT_LOCAL_JUMP(SkullFunc, Func_55396) { skull_knocked(gb, ADDR_Func_3317); }
PORT_LOCAL_JUMP(SkullFunc, Func_553a0) { skull_knocked(gb, ADDR_Func_3326); }

// .Func_5548b: finds the active Jamano and updates its hammer frame.
PORT_LOCAL_JUMP(Func_55407, Func_5548b)
{
	gb->cpu.a = rd8(gb, w1d149);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.b = 0;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		// .asm_55494
		gb->cpu.l = OBJ_ID;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.l);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_HL(gb)); // OBJ_ID
		alu_cp(gb, gb->cpu.a, JAMANO);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_sub(gb, gb->cpu.l, 0x07);
			gb->cpu.l = gb->cpu.a;
			gb->cpu.a = alu_rra(gb, rd8(gb, REG_HL(gb)));
			if (gb->cpu.f & FLAG_C)
				break;
		}
		// .asm_554a5
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, (uint8_t)wObj8);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		gb->cpu.a = alu_add(gb, gb->cpu.a, OBJ_STRUCT_LENGTH);
		gb->cpu.b = gb->cpu.a;
	}
	// .asm_554ae
	gb->cpu.a = alu_add(gb, gb->cpu.l, 0x16);
	gb->cpu.l = gb->cpu.a;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	hl = DEC_L(hl);
	hl = DEC_L(hl);
	gb->cpu.e = (uint8_t)hl;
	gb->cpu.a = alu_add(gb, (uint8_t)hl, 0x07);
	hl = LO(hl, gb->cpu.a);
	SET_HL(gb, hl);
	JUMP_TO(ADDR_Func_554bc); // fallthrough
}

// inc/dec [hl] / call .Func_5548b / ld c, 0 / ld b, 8 / jp SetBGPals
// (BlackOutBGPals when pals is 0)
static void hammer_range_changed(GB *gb, bool inc, bool call, uint16_t ret, uint16_t pals)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, inc ? alu_inc(gb, rd8(gb, hl)) : alu_dec(gb, rd8(gb, hl)));
	if (call)
		gb_call_ret(gb, FUNC_55407_FUNC_5548B, ret);
	if (!pals) {
		gb->cpu.c = 0;
		gb->cpu.b = 8;
		JUMP(BlackOutBGPals);
		return;
	}
	SET_HL(gb, pals);
	// .asm_55484
	gb->cpu.c = 0;
	gb->cpu.b = 8;
	JUMP(SetBGPals);
}

// ld a, b / cp v; returns whether w1d147 (b) is v
static bool b_is(GB *gb, uint8_t v)
{
	gb->cpu.a = gb->cpu.b;
	alu_cp(gb, gb->cpu.a, v);
	return gb->cpu.f & FLAG_Z;
}

// Updates the boss's palettes and hammer as skulls are knocked out.
PORT_JUMP(Func_55407)
{
	gb->cpu.a = rd8(gb, w1d147);
	gb->cpu.b = gb->cpu.a;
	uint16_t hl = wDollBoyHammerRange;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t r = gb->cpu.a;
	if (!alu_and(gb, r, r)) {
		// .asm_5542b
		if (!b_is(gb, 0x01)) {
			RET();
			return;
		}
		hammer_range_changed(gb, true, true, FUNC_55407(0x2c), ADDR_Pals_55551);
		return;
	}
	alu_cp(gb, r, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_55438
		if (b_is(gb, 0x02)) {
			hammer_range_changed(gb, true, true, FUNC_55407(0x3a), ADDR_Pals_55591);
			return;
		}
		// .asm_55446
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		hammer_range_changed(gb, false, false, 0, ADDR_Pals_55511);
		return;
	}
	alu_cp(gb, r, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_5544e
		if (b_is(gb, 0x03)) {
			hammer_range_changed(gb, true, true, FUNC_55407(0x50), ADDR_Pals_555d1);
			return;
		}
		// .asm_5545c
		alu_cp(gb, gb->cpu.a, 0x01);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		hammer_range_changed(gb, false, true, FUNC_55407(0x5c), ADDR_Pals_55551);
		return;
	}
	alu_cp(gb, r, 0x03);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_55468
		if (b_is(gb, 0x04)) {
			hammer_range_changed(gb, true, true, FUNC_55407(0x6a), 0);
			return;
		}
		// .asm_55478
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		hammer_range_changed(gb, false, true, FUNC_55407(0x78), ADDR_Pals_55591);
		return;
	}
	if (!b_is(gb, 0x03)) {
		RET();
		return;
	}
	hammer_range_changed(gb, false, true, FUNC_55407(0x1f), ADDR_Pals_555d1);
}

#define JM(off) (ADDR_JamanoFunc + (off))
#define JAMANO_FUNC_54D7D JM(0x46)
#define JAMANO_FUNC_54DAA JM(0x73)
#define JAMANO_FUNC_54E10 JM(0xd9)
#define JAMANO_FUNC_55097 JM(0x360)
#define JAMANO_FUNC_550B3 JM(0x37c)
#define JAMANO_FUNC_550DB JM(0x3a4)
#define JAMANO_FUNC_550FF JM(0x3c8)

// The play_music2 macro
static void play_music2(GB *gb, uint16_t music)
{
	gb->cpu.a = (uint8_t)(music >> 8);
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
	wr8(gb, hMusicID + 1, gb->cpu.a);
}

// The stop_music2 macro
static void stop_music2(GB *gb)
{
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);
}

PORT_JUMP(JamanoFunc)
{
	gb->cpu.a = TRUE;
	wr8(gb, wIsBossBattle, TRUE);
	stop_music2(gb);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, JAMANO_FUNC_54D7D);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x03);
	wr8(gb, hl--, 0xfc);
	wr8(gb, hl, 0x06);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69ce9);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0xff);
	SET_HL(gb, hl);
	gb->cpu.a = 0x08;
	wr8(gb, wCurObjVar2, 0x08);
	CALL(MoveObjectLeftByVar2);
	gb->cpu.a = 0x04;
	wr8(gb, wCurObjVar2, 0x04);
	JUMP(MoveObjectUpByVar2);
}

PORT_LOCAL(JamanoFunc, Func_54d7d)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0xc0);
	if (gb->cpu.f & FLAG_Z)
		play_music2(gb, MUSIC_BOSS_REVEAL);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	SET_DE(gb, ADDR_Frameset_69cdb);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0xff);
	SET_HL(gb, set_update(gb, hl, JAMANO_FUNC_54DAA));
}

// Starts the fight.
PORT_LOCAL_JUMP(JamanoFunc, Func_54daa)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x30);
	if (gb->cpu.f & FLAG_Z)
		stop_music2(gb);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = BOSS_REGULAR;
	wr8(gb, wBossState, gb->cpu.a);
	CALL(UpdateLevelMusic);
	hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 7)));
	hl = set_update(gb, hl, JAMANO_FUNC_54E10);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	hl = LO(hl, OBJ_STATE_DURATION);
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, w1d149, 0);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool right = gb->cpu.f & FLAG_C;
	if (right)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 7)));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 7)));
	hl = INC_L(hl);
	gb->cpu.a = right ? 0x5b : 0x5a;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, right ? ADDR_Frameset_69ca1 : ADDR_Frameset_69c1c);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL(JamanoFunc, Func_54e0a) { no_actions_for_1(gb); }

// ld hl, wCurObjStateDuration / dec [hl] / ret nz / next update f /
// frameset with duration $28; returns false on ret nz
static bool next_step(GB *gb, uint16_t f, uint16_t frameset)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return false;
	SET_HL(gb, set_update(gb, hl, f));
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x28;
	wr8(gb, hl++, 0x28);
	SET_HL(gb, hl);
	return true;
}

PORT_LOCAL(JamanoFunc, Func_5507b) { next_step(gb, JAMANO_FUNC_55097, ADDR_Frameset_69c45); }
PORT_LOCAL(JamanoFunc, Func_55097) { next_step(gb, JAMANO_FUNC_550B3, ADDR_Frameset_69c4a); }

// Loses its hat.
PORT_LOCAL_JUMP(JamanoFunc, Func_550b3)
{
	if (!next_step(gb, JAMANO_FUNC_550DB, ADDR_Frameset_69c4f)) {
		RET();
		return;
	}
	uint16_t hl = INC_L(REG_HL(gb));
	gb->cpu.a = 0x0f;
	wr8(gb, hl, 0x0f);
	SET_HL(gb, hl);
	CALL(MoveObjectUpByVar2);
	SET_BC(gb, ADDR_ObjParams_JamanoHat);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(JamanoFunc, Func_550db)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = set_update(gb, hl, JAMANO_FUNC_550FF);
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xf0);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_0A3);
}

PORT_LOCAL_JUMP(JamanoFunc, Func_550ff)
{
	no_actions_for_1(gb);
	SET_BC(gb, ADDR_Data_60d20);
	JUMP(ApplyObjYMovement);
}

#define JAMANO_FUNC_54E0A JM(0xd3)
#define JAMANO_FUNC_54EE4 JM(0x1ad)
#define JAMANO_FUNC_54F55 JM(0x21e)
#define JAMANO_FUNC_54F9D JM(0x266)
#define JAMANO_FUNC_5507B JM(0x344)

// .Func_54ee4: bobs towards Wario's height.
PORT_LOCAL(JamanoFunc, Func_54ee4)
{
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 6)));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 6)));
}

// .Func_54f55: turns to face Wario, swinging the hammer around.
PORT_LOCAL(JamanoFunc, Func_54f55)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d)
			return;
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.e = 0x13;
		gb_call_ret(gb, ADDR_Func_554bc, JM(0x22e));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFrameDuration, 0);
		wr8(gb, wCurObjFramesetOffset, 0);
		return;
	}
	// .asm_54f6d
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool right = gb->cpu.f & FLAG_C;
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	bool c = gb->cpu.f & FLAG_C;
	if (right ? c : !c)
		return;
	if (right) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 7)));
		SET_DE(gb, ADDR_Frameset_69c92);
	} else {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 7)));
		SET_DE(gb, ADDR_Frameset_69c7a);
	}
	// .asm_54f8e
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x14;
	wr8(gb, REG_HL(gb), 0x14);
	play_sfx(gb, SFX_0BC);
}

// Func_3513 at the position 8 pixels to the side (right: add, else sub)
static void wall_ahead(GB *gb, uint16_t hl, bool right, uint16_t ret)
{
	hl = LO(hl, OBJ_Y_POS);
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	if (right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x08);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x08);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, ret);
}

// The ground below, 10 pixels down: Func_352b; hl = OBJ_Y_POS's page
static void ground_below(GB *gb, uint16_t hl, uint16_t ret)
{
	hl = LO(hl, OBJ_Y_POS);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x0a);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, ret);
}

// .asm_54efb: bobs up and down.
static void jamano_bob(GB *gb)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wCurObjVar3));
	if (!gb->cpu.a)
		gb_call_ret(gb, JAMANO_FUNC_54EE4, JM(0x1cd));
	// .asm_54f04
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	bool up = true; // .asm_54f1a, else .asm_54f4f
	if (!alu_bit(gb, rd8(gb, hl), 6)) {
		alu_cp(gb, gb->cpu.a, 0x28);
		if (gb->cpu.f & FLAG_C) {
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 6)));
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjVar3, 0);
			up = false;
		}
	} else {
		// .asm_54f20
		bool flip;
		alu_cp(gb, gb->cpu.a, 0x98);
		if (!(gb->cpu.f & FLAG_C)) {
			flip = true;
		} else if (alu_cp(gb, gb->cpu.a, 0x7c), gb->cpu.f & FLAG_C) {
			flip = false;
		} else if (alu_cp(gb, gb->cpu.a, 0x80), !(gb->cpu.f & FLAG_C)) {
			flip = false;
		} else {
			ground_below(gb, hl, JM(0x20a));
			flip = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
			if (flip) {
				hl = wCurObjSubState;
				SET_HL(gb, hl);
			}
		}
		if (flip) {
			// .asm_54f47
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 6)));
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjVar3, 0);
		} else {
			up = false;
		}
	}
	SET_BC(gb, up ? ADDR_Data_60c80 : ADDR_Data_60c20);
	JUMP(ApplyObjYMovement_Loop);
}

// The fight: flies back and forth, swinging its hammer.
PORT_LOCAL_JUMP(JamanoFunc, Func_54e10)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 4)));
		RET();
		return;
	}
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 4)));
	gb->cpu.a = rd8(gb, wDollBoyHammerRange);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		hl = set_update(gb, hl, JAMANO_FUNC_54F9D);
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
		SET_DE(gb, ADDR_Frameset_69ce0);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		wr8(gb, hl++, 0xa0);
		SET_HL(gb, hl);
		stop_music2(gb);
		RET();
		return;
	}
	// .asm_54e46
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	uint8_t s = gb->cpu.a;
	alu_cp(gb, s, 0x5a);
	bool left = gb->cpu.f & FLAG_Z;
	bool right = false;
	if (!left) {
		alu_cp(gb, s, 0x5b);
		right = gb->cpu.f & FLAG_Z;
	}
	if (!left && !right) {
		SET_DE(gb, alu_bit(gb, rd8(gb, hl), 7) ? ADDR_Frameset_69c7a : ADDR_Frameset_69c92);
		hl = LO(hl, OBJ_FLAGS);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
		SET_HL(gb, set_update(gb, hl, JAMANO_FUNC_54E0A));
		stop_music2(gb);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	// .asm_54e78 / .asm_54eae
	gb_call_ret(gb, JAMANO_FUNC_54F55, left ? JM(0x144) : JM(0x17a));
	gb->cpu.a = alu_dec(gb, rd8(gb, wCurObjVar2));
	bool turn = false;
	if (!gb->cpu.a) {
		hl = LO(REG_HL(gb), OBJ_SUBSTATE);
		SET_HL(gb, hl);
		bool hdir = alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F);
		turn = left ? hdir : !hdir;
	}
	if (!turn) {
		wall_ahead(gb, REG_HL(gb), right, left ? JM(0x165) : JM(0x19b));
		turn = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (turn) {
		gb->cpu.a = left ? 0x5b : 0x5a;
		wr8(gb, wCurObjState, gb->cpu.a);
	} else {
		SET_BC(gb, left ? ADDR_Data_60c80 : ADDR_Data_60c20);
		CALL(ApplyObjXMovement_Loop);
	}
	jamano_bob(gb);
}

// Hatless: bounces around the room until it's back in the middle.
PORT_LOCAL_JUMP(JamanoFunc, Func_54f9d)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	} else {
		// .asm_54fac
		gb->cpu.a = rd8(gb, wCurObjScreenXPos);
		alu_cp(gb, gb->cpu.a, 0x58);
		if (gb->cpu.f & FLAG_Z) {
			SET_HL(gb, set_update(gb, hl, JAMANO_FUNC_5507B));
			SET_DE(gb, ADDR_Frameset_69c40);
			CALL(SetObjectFramesetPtr);
			hl = REG_HL(gb);
			wr8(gb, hl++, 0x28);
			SET_HL(gb, hl);
			play_sfx(gb, SFX_0BD);
			RET();
			return;
		}
	}
	// .asm_54fcd
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x5a);
	bool left = gb->cpu.f & FLAG_Z;
	if (!left) {
		alu_cp(gb, gb->cpu.a, 0x5b);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	// .asm_54fd9 / .asm_55006
	wall_ahead(gb, wCurObjYPos, !left, left ? JM(0x2b8) : JM(0x2e5));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		play_sfx(gb, SFX_BUMP);
		gb->cpu.a = left ? 0x5b : 0x5a;
		wr8(gb, wCurObjState, gb->cpu.a);
	} else if (left) {
		CALL(MoveObjectLeft);
	} else {
		CALL(MoveObjectRight);
	}
	// .asm_55031
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), 6)) {
		alu_cp(gb, gb->cpu.a, 0x30);
		if (!(gb->cpu.f & FLAG_C)) {
			JUMP(MoveObjectUp);
			return;
		}
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 6)));
		play_sfx(gb, SFX_BUMP);
		JUMP(MoveObjectDown);
		return;
	}
	// .asm_5504d
	alu_cp(gb, gb->cpu.a, 0x7c);
	if (gb->cpu.f & FLAG_C) {
		JUMP(MoveObjectDown);
		return;
	}
	ground_below(gb, hl, JM(0x330));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(MoveObjectDown);
		return;
	}
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_VDIR));
	play_sfx(gb, SFX_BUMP);
	JUMP(MoveObjectUp);
}
