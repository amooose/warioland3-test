// Port of src/engine/level/objects/muddee.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Inits the turtle, and destroys it if a TURTLE1 object is active.
PORT(Turtle2Func)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(ADDR_TurtleCommonFunc >> 8));
	wr8(gb, hl--, (uint8_t)ADDR_TurtleCommonFunc);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x0a);
	wr8(gb, hl--, 0xf5); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0x04); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69d5c);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x10;
	wr8(gb, wCurObjVar2, 0x10);
	CALL(MoveObjectUpByVar2);

	gb->cpu.a = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		// .asm_55e14
		gb->cpu.l = OBJ_ID;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.l);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_HL(gb)); // OBJ_ID
		alu_cp(gb, gb->cpu.a, TURTLE1);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = gb->cpu.a;
			gb->cpu.a = alu_rra(gb, rd8(gb, REG_HL(gb))); // OBJ_FLAGS
			if (gb->cpu.f & FLAG_C) {
				// .asm_55e2e
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wCurObjFlags, 0);
				return;
			}
		}
		// .asm_55e25
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, (uint8_t)wObj8);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = alu_add(gb, gb->cpu.a, OBJ_STRUCT_LENGTH);
		gb->cpu.b = gb->cpu.a;
	}
}

PORT(Func_55e33)
{
	uint16_t hl = wCurObjFrameDuration;
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_084);
}

#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define T1(off) (ADDR_Turtle1Func + (off))
#define TURTLE1_FUNC_55C7D T1(0x33)
#define TURTLE1_FUNC_55C91 T1(0x47)
#define TURTLE1_FUNC_55CDD T1(0x93)
#define TURTLE1_FUNC_55CF9 T1(0xaf)
#define TURTLE1_FUNC_55D18 T1(0xce)
#define TURTLE1_FUNC_55D4C T1(0x102)
#define TURTLE_COMMON_FUNC_55D90 (ADDR_TurtleCommonFunc + 0x14)
#define TURTLE_COMMON_FUNC_55DE0 (ADDR_TurtleCommonFunc + 0x64)

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

PORT_JUMP(Turtle1Func)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMuddeeAmbushState, 0);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, TURTLE1_FUNC_55C7D);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x0a);
	wr8(gb, hl--, 0xf5); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0x04); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69d5c);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0xa0);
	SET_HL(gb, hl);
	gb->cpu.a = 0x10;
	wr8(gb, wCurObjVar2, 0x10);
	JUMP(MoveObjectUpByVar2);
}

// call SetOneWayPlatformAction / once Wario steps on it, switches to f.
static void wait_stepped(GB *gb, uint16_t f)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F))
		return;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
	SET_HL(gb, set_update(gb, hl, f));
}

PORT_LOCAL(Turtle1Func, Func_55c7d) { wait_stepped(gb, TURTLE1_FUNC_55C91); }
PORT(TurtleCommonFunc) { wait_stepped(gb, TURTLE_COMMON_FUNC_55D90); }

// Carries Wario along if he's standing on it.
static void carry_wario(GB *gb, uint16_t hl)
{
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		RET();
		return;
	}
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
	FARCALL(Func_19706);
	gb->cpu.a = gb->cpu.b;
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	JUMP(MoveWarioRight);
}

// Swims right, carrying Wario, then stops.
PORT_LOCAL_JUMP(Turtle1Func, Func_55c91)
{
	CALL(SetOneWayPlatformAction);
	gb_call(gb, ADDR_Func_55e33);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		CALL(MoveObjectRight);
		carry_wario(gb, LO(REG_HL(gb), OBJ_FLAGS));
		return;
	}
	// .asm_55cc3
	SET_HL(gb, set_update(gb, hl, TURTLE1_FUNC_55CDD));
	SET_DE(gb, ADDR_Frameset_69d65);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x3c);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_0CA);
	RET();
}

PORT_LOCAL(Turtle1Func, Func_55cdd)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	wr8(gb, hl++, 0x28);
	wr8(gb, hl, 0x40);
	SET_HL(gb, set_update(gb, hl, TURTLE1_FUNC_55CF9));
	gb->cpu.a = 1;
	wr8(gb, wMuddeeAmbushState, 1);
}

// Waits, then sinks.
PORT_LOCAL_JUMP(Turtle1Func, Func_55cf9)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		RET();
		return;
	}
	no_actions_for_1(gb);
	hl = INC_L(hl);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(MoveObjectDown);
		return;
	}
	SET_HL(gb, set_update(gb, hl, TURTLE1_FUNC_55D18));
	RET();
}

// Waits for Muddee's defeat.
PORT_LOCAL(Turtle1Func, Func_55d18)
{
	no_actions_for_1(gb);
	gb->cpu.a = rd8(gb, wMuddeeAmbushState);
	alu_cp(gb, gb->cpu.a, 2);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	SET_DE(gb, ADDR_Frameset_69d5c);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x3c);
	gb->cpu.a = 0x40;
	wr8(gb, hl, 0x40);
	set_update(gb, hl, TURTLE1_FUNC_55D4C);
	SET_HL(gb, wCurObjUnk02);
	gb->cpu.e = 0x06;
	FARCALL(Func_ba42);
}

// Resurfaces, then becomes a regular turtle.
PORT_LOCAL_JUMP(Turtle1Func, Func_55d4c)
{
	CALL(SetOneWayPlatformAction);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		gb->cpu.a = BOSS_DEFEATED;
		wr8(gb, wBossState, gb->cpu.a);
		CALL(UpdateLevelMusic);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wIsBossBattle, 0);
		RET();
		return;
	}
	// .asm_55d6a
	hl = INC_L(hl);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(MoveObjectUp);
		return;
	}
	hl = set_update(gb, hl, ADDR_TurtleCommonFunc);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 5)));
	RET();
}

// Swims right until it hits a wall, carrying Wario.
PORT_LOCAL_JUMP(TurtleCommonFunc, Func_55d90)
{
	CALL(SetOneWayPlatformAction);
	gb_call(gb, ADDR_Func_55e33);
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x30);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, ADDR_TurtleCommonFunc + 0x35);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .asm_55dd6
		hl = wCurObjUpdateFunction + 1;
		gb->cpu.a = (uint8_t)(TURTLE_COMMON_FUNC_55DE0 >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)TURTLE_COMMON_FUNC_55DE0;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
		RET();
		return;
	}
	CALL(MoveObjectRight);
	carry_wario(gb, wCurObjFlags);
}

PORT_LOCAL(TurtleCommonFunc, Func_55de0) { CALL(SetOneWayPlatformAction); }

#define MD(off) (ADDR_MuddeeFunc + (off))
#define MD_FUNC_5564E MD(0x3d)
#define MD_FUNC_55676 MD(0x65)
#define MD_FUNC_55696 MD(0x85)
#define MD_FUNC_556F0 MD(0xdf)
#define MD_FUNC_55711 MD(0x100)
#define MD_JUMPTABLE MD(0x14d + 4) // after .asm_5575e's rst JumpTable
#define MD_FUNC_55BA7 MD(0x596)

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

// .Func_55c3a: flickers.
static void md_flicker(GB *gb)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (gb->cpu.a)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 4)));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 4)));
}

// .Func_55bd8
static void md_sfx_0c1(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x0f);
	if (gb->cpu.a)
		return;
	play_sfx(gb, SFX_0C1);
}

// .Func_55be7: the swimming sound, faster in the faster modes.
static void md_swim_sfx(GB *gb)
{
	gb->cpu.a = rd8(gb, wGlobalCounter);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
	uint8_t mask;
	alu_cp(gb, gb->cpu.a, 0x05);
	if (gb->cpu.f & FLAG_Z) {
		mask = 0x1f;
	} else {
		alu_cp(gb, gb->cpu.a, 0x04);
		mask = (gb->cpu.f & FLAG_Z) ? 0x0f : 0x07;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.b, mask);
	if (gb->cpu.a)
		return;
	play_sfx(gb, SFX_085);
}

// .Func_55c11: sets the frameset de with frame 0, duration c, and b
static void md_set_frameset(GB *gb)
{
	uint16_t hl = wCurObjFramesetPtr;
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl++, gb->cpu.e);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl++, gb->cpu.d);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, hl++, gb->cpu.c);
	wr8(gb, hl, gb->cpu.b);
	SET_HL(gb, hl);
}

// .Func_55c1e: cp of the screen y positions (c: Muddee above Wario)
static void md_cp_y(GB *gb)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
}

// .Func_55c2b: z if Muddee is level with Wario (in 8-pixel steps)
static void md_cp_row(GB *gb)
{
	gb->cpu.a = alu_and(gb, alu_sub(gb, rd8(gb, wWarioScreenYPos), 0x10), 0xf8);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjScreenYPos), 0xf8);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
}

PORT_LOCAL(MuddeeFunc, Func_55bd8) { md_sfx_0c1(gb); }
PORT_LOCAL(MuddeeFunc, Func_55be7) { md_swim_sfx(gb); }
PORT_LOCAL(MuddeeFunc, Func_55c11) { md_set_frameset(gb); }
PORT_LOCAL(MuddeeFunc, Func_55c1e) { md_cp_y(gb); }
PORT_LOCAL(MuddeeFunc, Func_55c2b) { md_cp_row(gb); }
PORT_LOCAL(MuddeeFunc, Func_55c3a) { md_flicker(gb); }

PORT_JUMP(MuddeeFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, MD_FUNC_5564E);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK), 0x05);
	wr8(gb, hl, gb->cpu.a);
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x07);
	wr8(gb, hl--, 0xf8);
	wr8(gb, hl, 0x07);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69d73);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x3c);
	SET_HL(gb, hl);
	gb->cpu.a = 0x70;
	wr8(gb, wCurObjVar2, 0x70);
	CALL(MoveObjectRightByVar2);
	gb->cpu.a = 0x04;
	wr8(gb, wCurObjVar2, 0x04);
	JUMP(MoveObjectUpByVar2);
}

// Waits hidden for the turtle to dive.
PORT_LOCAL_JUMP(MuddeeFunc, Func_5564e)
{
	no_actions_for_1(gb);
	md_flicker(gb);
	gb->cpu.a = alu_dec(gb, rd8(gb, wMuddeeAmbushState));
	if (gb->cpu.a) {
		RET();
		return;
	}
	gb->cpu.a = TRUE;
	wr8(gb, wIsBossBattle, TRUE);
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(MD_FUNC_55676 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)MD_FUNC_55676;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, ADDR_Pals_55e46);
	SET_DE(gb, wTempPals2 + PAL_SIZE * 6);
	gb->cpu.c = 6;
	gb->cpu.b = 1;
	JUMP(CopyAndApplyOBPals);
}

PORT_LOCAL(MuddeeFunc, Func_55676)
{
	no_actions_for_1(gb);
	md_flicker(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	SET_HL(gb, set_update(gb, hl, MD_FUNC_55696));
	SET_DE(gb, ADDR_Frameset_69d85);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
}

// .asm_556dc: spawns the stingers.
static void spawn_stingers(GB *gb)
{
	play_sfx(gb, SFX_0B2);
	SET_BC(gb, ADDR_ObjParams_MuddeeStinger1);
	CALL(CreateObjectFromCurObjPos);
	SET_BC(gb, ADDR_ObjParams_MuddeeStinger2);
	JUMP(CreateObjectFromCurObjPos);
}

// Ambushes Wario once he's in range.
PORT_LOCAL_JUMP(MuddeeFunc, Func_55696)
{
	no_actions_for_1(gb);
	md_flicker(gb);
	gb->cpu.a = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, gb->cpu.a, 0x09);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = rd8(gb, wWarioXPos + 1);
		alu_cp(gb, gb->cpu.a, 0x40);
		if (!(gb->cpu.f & FLAG_C)) {
			RET();
			return;
		}
	}
	// .asm_556ab
	gb->cpu.a = rd8(gb, wWarioScreenYPos);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x48);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x96);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(MD_FUNC_556F0 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)MD_FUNC_556F0;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69cf2);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	hl = LO(REG_HL(gb), OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	gb->cpu.a = BOSS_REGULAR;
	wr8(gb, wBossState, gb->cpu.a);
	CALL(UpdateLevelMusic);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d147, 0);
	spawn_stingers(gb);
}

PORT_LOCAL_JUMP(MuddeeFunc, Func_556f0)
{
	no_actions_for_1(gb);
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, 0x0e);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_BC(gb, ADDR_Data_60d30);
		JUMP(ApplyObjYMovement);
		return;
	}
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(MD_FUNC_55711 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)MD_FUNC_55711;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0x01;
	wr8(gb, wCurObjVar1, 0x01);
	RET();
}

// .asm_557b1: hit; sinks, then the turtle resurfaces.
static void md_defeated(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		alu_cp(gb, gb->cpu.a, 0x28);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_0C0);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		// .asm_55806
		SET_DE(gb, ADDR_Frameset_69d6a);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		wr8(gb, hl++, 0x28);
		gb->cpu.a = 0x14;
		wr8(gb, hl, 0x14);
		hl = set_update(gb, hl, MD_FUNC_55BA7);
		hl = LO(hl, OBJ_SUBSTATE);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x80);
		wr8(gb, hl, gb->cpu.a);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 7)));
		else
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 7)));
		hl = DEC_L(hl);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		hl = LO(hl, OBJ_FLAGS);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_PRIORITY));
		SET_HL(gb, wCurObjUnk02);
		gb->cpu.e = 0x07;
		FARCALL(Func_ba42);
		stop_music2(gb);
		gb->cpu.a = 2;
		wr8(gb, wMuddeeAmbushState, 2);
		RET();
		return;
	}
	// .asm_557cf
	hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, MD(0x1d0));
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		SET_BC(gb, ADDR_Data_60d40);
		JUMP(ApplyObjYMovement);
		return;
	}
	// .in_water
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d147, 0);
	SET_DE(gb, ADDR_Frameset_69dc0);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x2c);
	gb->cpu.a = 0x04;
	wr8(gb, hl, 0x04);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_083);
	RET();
}

// The fight: sets the interaction by frame, then runs the current move
// (OBJ_VAR_1), or handles being hit.
PORT_LOCAL_JUMP(MuddeeFunc, Func_55711)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wCurObjFrame);
	int8_t b = -1;
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		no_actions_for_1(gb); // .asm_55734
	} else if (alu_cp(gb, gb->cpu.a, 0x0c), !(gb->cpu.f & FLAG_C)) {
		b = 0x0b;
	} else if (alu_cp(gb, gb->cpu.a, 0x09), !(gb->cpu.f & FLAG_C)) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 7)));
		b = 0x46;
	} else if (alu_cp(gb, gb->cpu.a, 0x06), !(gb->cpu.f & FLAG_C)) {
		b = 0x47;
	} else if (alu_cp(gb, gb->cpu.a, 0x03), !(gb->cpu.f & FLAG_C)) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 7)));
		b = 0x46;
	} else {
		b = 0x48;
	}
	if (b >= 0) {
		// .asm_5574b
		gb->cpu.b = (uint8_t)b;
		hl = LO(hl, OBJ_INTERACTION_TYPE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), gb->cpu.b);
		wr8(gb, hl, gb->cpu.a);
	}
	// .asm_55752
	gb->cpu.a = rd8(gb, w1d147);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .asm_5575e
		gb->cpu.a = rd8(gb, wCurObjVar1);
		gb_push16(gb, MD_JUMPTABLE); // rst JumpTable
		hl = gb_pop16(gb); // pop hl
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.d = 0;
		hl = alu_add16(gb, hl, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl);
		gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
		gb->cpu.l = gb->cpu.a;
		JUMP_TO(REG_HL(gb));
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		md_defeated(gb, hl);
		return;
	}
	// .asm_55774
	play_sfx(gb, SFX_VANISH);
	gb->cpu.a = 0x02;
	wr8(gb, w1d147, 0x02);
	gb->cpu.a = rd8(gb, wCurObjInteractionType);
	alu_cp(gb, gb->cpu.a, 0x48);
	if (gb->cpu.f & FLAG_Z) {
		SET_DE(gb, ADDR_Frameset_69d48);
	} else if (alu_cp(gb, gb->cpu.a, 0x47), gb->cpu.f & FLAG_Z) {
		SET_DE(gb, ADDR_Frameset_69d4d);
	} else {
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		SET_DE(gb, alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F) ? ADDR_Frameset_69d52 : ADDR_Frameset_69d57);
	}
	// .asm_557a4
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x3c);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	JUMP(DecrementRemainingBossHits);
}

// The state's countdown before the move: when it runs out, sets the
// frameset. Returns true when the move should run (duration 0).
static bool md_wait(GB *gb, uint16_t frameset)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return true;
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return false;
	}
	SET_DE(gb, frameset);
	JUMP(SetObjectFramesetPtr);
	return false;
}

// Loads the position plus (dy, dx) into hYPos / hXPos (negative: sub/sbc).
static void md_pos_to_h(GB *gb, int dy, int dx)
{
	uint16_t hl = wCurObjYPos;
	for (int i = 0; i < 2; i++) {
		int v = i ? dx : dy;
		uint16_t lo = i ? hXPosLo : hYPosLo, hi = i ? hXPosHi : hYPosHi;
		if (v < 0) {
			gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-v);
			wr8(gb, lo, gb->cpu.a);
			gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		} else if (v > 0) {
			gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)v);
			wr8(gb, lo, gb->cpu.a);
			gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
		} else {
			wr8(gb, lo, rd8(gb, hl++));
			gb->cpu.a = rd8(gb, hl++);
		}
		wr8(gb, hi, gb->cpu.a);
	}
	SET_HL(gb, hl);
}

// Moves with MoveObject* (slow when OBJ_SUBSTATE's low nibble is 5).
static void md_move(GB *gb, uint16_t normal, uint16_t slow)
{
	md_swim_sfx(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
	alu_cp(gb, gb->cpu.a, 0x05);
	JUMP_TO((gb->cpu.f & FLAG_Z) ? slow : normal);
}

// Compares Wario's x position with Muddee's; returns true if Wario is to
// the left (c).
static bool md_wario_left(GB *gb)
{
	uint16_t hl = LO(REG_HL(gb), OBJ_X_POS);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (gb->cpu.f & FLAG_C)
		return true;
	if (!(gb->cpu.f & FLAG_Z))
		return false;
	gb->cpu.a = rd8(gb, wWarioXPos + 1);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	return gb->cpu.f & FLAG_C;
}

// ld a, var1 / ld [wCurObjVar1], a / de / c / b / jp .Func_55c11
static void md_next(GB *gb, uint8_t var1, uint16_t frameset, uint8_t c)
{
	gb->cpu.a = var1;
	if (!var1)
		gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar1, var1);
	SET_DE(gb, frameset);
	gb->cpu.c = c;
	gb->cpu.b = 0x7d;
	md_set_frameset(gb);
	RET();
}

// Swims up to Wario's height.
PORT_LOCAL_JUMP(MuddeeFunc, Func_55861)
{
	if (!md_wait(gb, ADDR_Frameset_69d04))
		return;
	md_pos_to_h(gb, -0x1c, 0);
	gb_call_ret(gb, ADDR_Func_358b, MD(0x275));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		md_cp_row(gb);
		if (!(gb->cpu.f & FLAG_Z)) {
			md_move(gb, ADDR_MoveObjectUp, ADDR_MoveObjectUp_Slow);
			return;
		}
	}
	// .asm_5589e
	SET_HL(gb, wCurObjXPos);
	if (md_wario_left(gb))
		md_next(gb, 0x02, ADDR_Frameset_69d16, 0x0e);
	else
		md_next(gb, 0x03, ADDR_Frameset_69d2f, 0x0e);
}

// Swims down to Wario's height.
PORT_LOCAL_JUMP(MuddeeFunc, Func_558d0)
{
	if (!md_wait(gb, ADDR_Frameset_69cf2))
		return;
	md_pos_to_h(gb, 0x30, 0);
	gb_call_ret(gb, ADDR_Func_352b, MD(0x2e4));
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		md_cp_row(gb);
		if (!(gb->cpu.f & FLAG_Z)) {
			md_move(gb, ADDR_MoveObjectDown, ADDR_MoveObjectDown_Slow);
			return;
		}
	}
	// .asm_55910
	SET_HL(gb, wCurObjXPos);
	if (md_wario_left(gb))
		md_next(gb, 0x02, ADDR_Frameset_69d2f, 0x02);
	else
		md_next(gb, 0x03, ADDR_Frameset_69d16, 0x02);
}

// Swims left (right) until it hits a wall, then up or down.
static void md_swim_x(GB *gb, bool right, uint16_t frameset, uint16_t ret,
	uint16_t fs_down, uint16_t fs_up)
{
	if (!md_wait(gb, frameset))
		return;
	md_pos_to_h(gb, 0, right ? 0x18 : -0x18);
	gb_call_ret(gb, ADDR_Func_3513, ret);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			md_move(gb, ADDR_MoveObjectRight, ADDR_MoveObjectRight_Slow);
		else
			md_move(gb, ADDR_MoveObjectLeft, ADDR_MoveObjectLeft_Slow);
		return;
	}
	md_cp_y(gb);
	if (gb->cpu.f & FLAG_C)
		md_next(gb, 0x01, fs_up, 0x14);
	else
		md_next(gb, 0x00, fs_down, 0x08);
}

PORT_LOCAL_JUMP(MuddeeFunc, Func_55942)
{
	md_swim_x(gb, false, ADDR_Frameset_69d0d, MD(0x356), ADDR_Frameset_69d2f, ADDR_Frameset_69d16);
}

PORT_LOCAL_JUMP(MuddeeFunc, Func_5599c)
{
	md_swim_x(gb, true, ADDR_Frameset_69cfb, MD(0x3b0), ADDR_Frameset_69d16, ADDR_Frameset_69d2f);
}

PORT_LOCAL_JUMP(MuddeeFunc, Func_559f6)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	SET_DE(gb, ADDR_Frameset_69d85);
	CALL(SetObjectFramesetPtr);
	hl = INC_L(REG_HL(gb));
	wr8(gb, hl++, 0x05);
	gb->cpu.a = 0x08;
	wr8(gb, hl, 0x08);
	SET_HL(gb, hl);
	JUMP(MoveObjectDownByVar2);
}

PORT_LOCAL_JUMP(MuddeeFunc, Func_55a0b)
{
	md_pos_to_h(gb, 0x12, 0);
	gb_call_ret(gb, ADDR_Func_352b, MD(0x410));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(MoveObjectDown);
		return;
	}
	gb->cpu.a = 0x06;
	wr8(gb, wCurObjVar1, 0x06);
	RET();
}

// Follows Wario along the floor, then throws more stingers.
PORT_LOCAL_JUMP(MuddeeFunc, Func_55a2b)
{
	uint16_t hl = wCurObjXPos;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x40);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	int dir; // -1 left, 1 right, 0 there
	gb->cpu.a = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (gb->cpu.f & FLAG_C) {
		dir = -1;
	} else if (!(gb->cpu.f & FLAG_Z)) {
		dir = 1;
	} else {
		gb->cpu.a = rd8(gb, wWarioXPos + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		dir = (gb->cpu.f & FLAG_C) ? -1 : (gb->cpu.f & FLAG_Z) ? 0 : 1;
	}
	if (!dir) {
		// .asm_55a72
		SET_DE(gb, ADDR_Frameset_69d04);
		CALL(SetObjectFramesetPtr);
		hl = INC_L(REG_HL(gb));
		wr8(gb, hl++, 0x07);
		hl = INC_L(hl);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		SET_HL(gb, hl);
		spawn_stingers(gb);
		return;
	}
	if (dir > 0) {
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
	gb_call_ret(gb, ADDR_Func_3513, dir > 0 ? MD(0x44a) : MD(0x45c));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	JUMP_TO(dir > 0 ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
}

// .asm_55aa3: starts swimming towards Wario.
static void md_charge(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	SET_HL(gb, hl);
	bool left = md_wario_left(gb);
	SET_DE(gb, left ? ADDR_Frameset_69d8e : ADDR_Frameset_69da7);
	gb->cpu.a = left ? 0 : 1;
	if (left)
		gb->cpu.f = FLAG_Z; // xor a
	// .asm_55ac1
	wr8(gb, wCurObjVar2, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	md_cp_y(gb);
	hl = LO(REG_HL(gb), OBJ_SUBSTATE);
	SET_HL(gb, hl);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << 6)));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << 6)));
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(MuddeeFunc, Func_55a82)
{
	uint16_t hl = wCurObjVar3;
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x0e);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_BC(gb, ADDR_Data_60d30);
		JUMP(ApplyObjYMovement);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
	alu_cp(gb, gb->cpu.a, 0x03);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjVar1, 0);
		RET();
		return;
	}
	hl = LO(hl, OBJ_STATE_DURATION);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	gb->cpu.a = 0x08;
	wr8(gb, hl, 0x08);
	SET_HL(gb, hl);
	md_charge(gb);
}

// Charges around the room.
PORT_LOCAL_JUMP(MuddeeFunc, Func_55ad9)
{
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = alu_inc(gb, rd8(gb, hl));
	if (gb->cpu.a) {
		hl = LO(hl, OBJ_FRAME_DURATION);
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			gb->cpu.a = rd8(gb, hl);
			alu_cp(gb, gb->cpu.a, 0x18);
			if (gb->cpu.f & FLAG_Z) {
				hl = LO(hl, OBJ_VAR_3);
				SET_HL(gb, hl);
				gb->cpu.a = alu_inc(gb, rd8(gb, hl));
				wr8(gb, hl, gb->cpu.a);
				alu_cp(gb, gb->cpu.a, 0x03);
				if (gb->cpu.f & FLAG_Z) {
					gb->cpu.a = 0xff;
					wr8(gb, hl, 0xff);
					SET_DE(gb, ADDR_Frameset_69cf2);
					CALL(SetObjectFramesetPtr);
					gb->cpu.a = 0x3c;
					wr8(gb, REG_HL(gb), 0x3c);
					RET();
					return;
				}
			}
		}
	}
	// .asm_55b00
	hl = LO(REG_HL(gb), OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = LO(hl, OBJ_VAR_3);
		SET_HL(gb, hl);
		gb->cpu.a = alu_inc(gb, rd8(gb, hl));
		if (gb->cpu.a) {
			RET();
			return;
		}
		wr8(gb, hl, gb->cpu.a);
		md_charge(gb);
		return;
	}
	// .asm_55b10
	md_sfx_0c1(gb);
	gb->cpu.a = alu_rra(gb, rd8(gb, wCurObjVar2));
	bool right = gb->cpu.f & FLAG_C;
	hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hYPosHi, gb->cpu.a);
	if (right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x18);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x18);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, right ? MD(0x533) : MD(0x51e));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			CALL(MoveObjectRight_Slow);
		else
			CALL(MoveObjectLeft_Slow);
	} else {
		// .asm_55b4c
		hl = wCurObjVar2;
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x01);
		wr8(gb, hl, gb->cpu.a);
		hl = LO(hl, OBJ_STATE_DURATION);
		gb->cpu.a = 0x28;
		wr8(gb, hl, 0x28);
		SET_HL(gb, hl);
	}
	// .asm_55b58
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	bool flip;
	if (!alu_bit(gb, rd8(gb, hl), 6)) {
		md_pos_to_h(gb, -0x1c, 0);
		gb_call_ret(gb, ADDR_Func_358b, MD(0x564));
		flip = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
		if (!flip) {
			JUMP(MoveObjectUp_Slow);
			return;
		}
	} else {
		md_pos_to_h(gb, 0x30, 0);
		gb_call_ret(gb, ADDR_Func_352b, MD(0x580));
		gb->cpu.a = rd8(gb, wIsInWaterOrSand);
		flip = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
		if (!flip) {
			JUMP(MoveObjectDown_Slow);
			return;
		}
	}
	// .asm_55b9a
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_VDIR);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = 0x28;
	wr8(gb, wCurObjStateDuration, 0x28);
	RET();
}

// Defeated: floats off.
PORT_LOCAL_JUMP(MuddeeFunc, Func_55ba7)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjVar1;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	} else {
		hl = DEC_L(hl);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			uint8_t d = alu_dec(gb, rd8(gb, hl));
			wr8(gb, hl, d);
			if (!d)
				play_sfx(gb, SFX_0A3);
			RET();
			return;
		}
	}
	// .asm_55bc6
	SET_BC(gb, ADDR_Data_60180);
	CALL(ApplyObjYMovement);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	JUMP_TO((gb->cpu.f & FLAG_C) ? ADDR_MoveObjectLeft : ADDR_MoveObjectRight);
}
