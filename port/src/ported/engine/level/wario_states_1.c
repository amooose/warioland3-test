// Port of src/engine/level/wario_states_1.asm
#include "ported.h"

// The same helpers as wario_states_2.c (see the notes there).
#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)

#define OAM1 BANK_OAM_14000   // BANK("Wario OAM 1")
#define OAM2 BANK_OAM_1fc000  // BANK("Wario OAM 2")

// ld a, v / ld [addr], a
static void st(GB *gb, uint16_t addr, uint8_t v)
{
	gb->cpu.a = v;
	wr8(gb, addr, v);
}

// ld a, [addr]
static uint8_t ld(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	return gb->cpu.a;
}

static void xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

// ld a, [addr] / and a; true if nz
static bool ld_and(GB *gb, uint16_t addr)
{
	uint8_t v = ld(gb, addr);
	return alu_and(gb, v, v) != 0;
}

// ld a, [addr] / cp v
static void ld_cp(GB *gb, uint16_t addr, uint8_t v)
{
	alu_cp(gb, ld(gb, addr), v);
}

// wDMASourceBank/Ptr = gfx, call LoadWarioGfx
static void load_gfx(GB *gb, uint8_t bank, uint16_t ptr)
{
	wr8(gb, wDMASourceBank, bank);
	wr8(gb, wDMASourcePtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wDMASourcePtr + 1, (uint8_t)ptr);
	CALL(LoadWarioGfx);
}

static void set_oam(GB *gb, uint8_t bank, uint16_t ptr)
{
	wr8(gb, wWarioOAMBank, bank);
	wr8(gb, wWarioOAMPtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wWarioOAMPtr + 1, (uint8_t)ptr);
}

static void set_frameset(GB *gb, uint16_t ptr)
{
	wr8(gb, wWarioFramesetPtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wWarioFramesetPtr + 1, (uint8_t)ptr);
}

// ld a, BANK("Wario OAM n") / ldh [hCallFuncBank], a / hcall UpdateWarioAnimation
static void anim(GB *gb, uint8_t bank)
{
	FARCALL_BANK(bank, UpdateWarioAnimation);
}

// ld a, [wDirection] / and a: true if facing right
static bool facing_right(GB *gb)
{
	return ld_and(gb, wDirection);
}

PORT(Func_1e6ea)
{
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	set_frameset(gb, facing_right(gb) ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);
	anim(gb, OAM1);
}

PORT(SetState_Turning)
{
	st(gb, wWarioState, WST_TURNING);
	xor_a(gb);
	wr8(gb, wAttackCounter, 0);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	// switch direction
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), 0x1);
	wr8(gb, wDirection, gb->cpu.a);
	bool right = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	set_frameset(gb, right ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
	anim(gb, OAM1);
}

PORT(SetState_Attacking)
{
	st(gb, wAttackCounter, MAX_ATTACK_COUNTER);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	st(gb, wWarioState, WST_ATTACKING);
	st(gb, wCollisionBoxBottom, (uint8_t)-1);
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	st(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);

	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wIsCrouching, 0);

	set_oam(gb, BANK_OAM_14d1b, ADDR_OAM_14d1b);
	load_gfx(gb, BANK_WarioAttackGfx, ADDR_WarioAttackGfx);

	// with the garlic power-up, Wario's attack is stronger
	ld_cp(gb, wPowerUpLevel, GARLIC);
	if (CF)
		set_frameset(gb, facing_right(gb) ? ADDR_Frameset_151fd : ADDR_Frameset_151e4);
	else
		set_frameset(gb, facing_right(gb) ? ADDR_Frameset_1522d : ADDR_Frameset_1521c);
	anim(gb, OAM1);
}
