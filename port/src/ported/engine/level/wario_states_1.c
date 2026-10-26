// Port of src/engine/level/wario_states_1.asm
#include "ported.h"

// The same helpers as wario_states_2.c (see the notes there).
#define LJUMP(name) JUMP_TO(ADDR_##name)

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

// ld a, [addr] / bit n, a
static bool ld_bit(GB *gb, uint16_t addr, int bit)
{
	return alu_bit(gb, ld(gb, addr), bit);
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

// ld a, v / ld [wCollisionBox...], a, in this order
static void collision_box(GB *gb, int8_t bottom, int8_t top, int8_t left, int8_t right)
{
	st(gb, wCollisionBoxBottom, (uint8_t)bottom);
	st(gb, wCollisionBoxTop, (uint8_t)top);
	st(gb, wCollisionBoxLeft, (uint8_t)left);
	st(gb, wCollisionBoxRight, (uint8_t)right);
}

// xor a / ld [addr], a for each address, up to a 0
static void clear(GB *gb, const uint16_t *addrs)
{
	xor_a(gb);
	for (; *addrs; addrs++)
		wr8(gb, *addrs, 0);
}

// ld a, [wIsOnSteppableObject] / and a / ret z / ld b, 2 / call MoveWarioUp
static void move_up_if_on_steppable_object(GB *gb)
{
	if (!ld_and(gb, wIsOnSteppableObject))
		return;
	gb->cpu.b = 2;
	CALL(MoveWarioUp);
}

PORT_JUMP(SetState_Walking)
{
	st(gb, wWarioState, WST_WALKING);
	uint8_t joypad = ld(gb, wJoypadDown);
	if (alu_bit(gb, joypad, B_PAD_RIGHT))
		st(gb, wDirection, DIRECTION_RIGHT);
	else if (alu_bit(gb, joypad, B_PAD_LEFT))
		st(gb, wDirection, DIRECTION_LEFT);
	static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelIndex, wJumpVelTable,
		wIsSmashAttacking, wWarioStateCounter, wWarioStateCycles, 0};
	clear(gb, zero);
	LJUMP(Func_1e6ea); // fallthrough
}

PORT(SetState_Airborne)
{
	xor_a(gb);
	wr8(gb, wWarioSlopeInteraction, 0);
	st(gb, wWarioState, WST_AIRBORNE);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wIsCrouching, wAttackCounter, 0};
	clear(gb, zero);
	collision_box(gb, -1, -27, -9, 9);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	set_frameset(gb, facing_right(gb) ? ADDR_Frameset_15f97 : ADDR_Frameset_15f94);
	anim(gb, OAM1);
	move_up_if_on_steppable_object(gb);
}

PORT(SetState_Diving)
{
	st(gb, wWarioState, WST_DIVING);
	st(gb, wCollisionBoxBottom, (uint8_t)-1);
	st(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
	static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wWarioStateCounter,
		wWarioStateCycles, wIsSmashAttacking, wAttackCounter, wIsRolling, wSwimVelIndex,
		wIsCrouching, wc0e0, 0};
	clear(gb, zero);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
}

PORT(SetState_Submerged)
{
	st(gb, wWarioState, WST_SUBMERGED);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wSwimVelIndex,
		wc0e0, wSwimmingDirectionInput, 0};
	clear(gb, zero);
	load_gfx(gb, BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	set_oam(gb, BANK_OAM_15254, ADDR_OAM_15254);

	bool can_swim = false;
	ld_cp(gb, wPowerUpLevel, SWIMMING_FLIPPERS);
	if (!CF) {
		ld_cp(gb, wWaterInteraction, NON_SUBMERSIBLE_WATER);
		can_swim = !ZF;
	}
	if (can_swim)
		collision_box(gb, -1, -15, -9, 9);
	// else .cannot_swim_underwater
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	bool right = facing_right(gb);
	if (can_swim)
		set_frameset(gb, right ? ADDR_Frameset_1558b : ADDR_Frameset_15576);
	else
		set_frameset(gb, right ? ADDR_Frameset_155a3 : ADDR_Frameset_155a0);
	anim(gb, OAM1);
}

PORT(SetState_WaterSurfaceIdling)
{
	st(gb, wWarioState, WST_WATER_SURFACE_IDLING);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wSwimVelIndex, wc0e0, wSwimmingDirectionInput, 0};
	clear(gb, zero);
	if (!ld_and(gb, wWaterInteraction))
		st(gb, wWaterInteraction, SUBMERSIBLE_WATER);
	set_frameset(gb, facing_right(gb) ? ADDR_Frameset_1555b : ADDR_Frameset_15554);
	anim(gb, OAM1);
}

// hPos = wWarioPos + (dy, dx), the slide dust's position
static void get_pos_with_offset(GB *gb, int dy, int dx)
{
	gb->cpu.a = dx < 0 ? alu_sub(gb, rd8(gb, wWarioXPos + 1), (uint8_t)-dx)
		: alu_add(gb, rd8(gb, wWarioXPos + 1), (uint8_t)dx);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = dx < 0 ? alu_sbc(gb, rd8(gb, wWarioXPos), 0) : alu_adc(gb, rd8(gb, wWarioXPos), 0);
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), (uint8_t)-dy);
	wr8(gb, hXPosLo - 2, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos), 0);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioYPos);
	SET_DE(gb, hXPosLo - 3);
}

PORT(SetState_Sliding)
{
	st(gb, wWarioState, WST_SLIDING);
	static const uint16_t zero[] = {wSFXLoopCounter, wIsCrouching, wFrameDuration,
		wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wWalkVelIndex, wGrabState, 0};
	clear(gb, zero);
	st(gb, wIsRolling, TRUE);
	collision_box(gb, -1, -27, -9, 9);
	load_gfx(gb, BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
	set_oam(gb, BANK_OAM_1644a, ADDR_OAM_1644a);
	if (!ld_bit(gb, wWarioSlopeInteraction, LEFT_SLOPE_F)) {
		get_pos_with_offset(gb, -0x0c, 0x0c);
		gb->cpu.b = PARTICLE_SLIDE_DUST_LEFT;
		FARCALL(CreateParticle);
		st(gb, wDirection, DIRECTION_LEFT);
		set_frameset(gb, ADDR_Frameset_16708);
	} else {
		get_pos_with_offset(gb, -0x0c, -0x0c);
		gb->cpu.b = PARTICLE_SLIDE_DUST_RIGHT;
		FARCALL(CreateParticle);
		st(gb, wDirection, DIRECTION_RIGHT);
		set_frameset(gb, ADDR_Frameset_1671d);
	}
	anim(gb, OAM1);
}

PORT(SetState_RollingAirborne)
{
	xor_a(gb);
	wr8(gb, wWarioSlopeInteraction, 0);
	wr8(gb, wGrabState, 0);
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	st(gb, wWarioState, WST_ROLLING_AIRBORNE);
	st(gb, wIsRolling, 0x02);
	load_gfx(gb, BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
	set_oam(gb, BANK_OAM_1644a, ADDR_OAM_1644a);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, facing_right(gb) ? ADDR_Frameset_166f7 : ADDR_Frameset_166e6);
	anim(gb, OAM1);
}

PORT(SetState_CrouchWalking)
{
	collision_box(gb, -1, -15, -9, 9);
	st(gb, wWarioState, WST_CROUCH_WALKING);
	st(gb, wIsCrouching, TRUE);
	static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame,
		wWarioStateCounter, wWarioStateCycles, wAttackCounter, wGrabState, 0};
	clear(gb, zero);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	ld_cp(gb, wDirection, DIRECTION_RIGHT);
	set_frameset(gb, ZF ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
	anim(gb, OAM1);
}

PORT(SetState_CrouchAirborne)
{
	st(gb, wWarioState, WST_CROUCH_AIRBORNE);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, 0};
	clear(gb, zero);
	collision_box(gb, -1, -15, -9, 9);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	set_frameset(gb, facing_right(gb) ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
	anim(gb, OAM1);
	move_up_if_on_steppable_object(gb);
}

// flashes Wario's palette while he's invisible
PORT(Func_1f6c2)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (ZF) {
		SET_HL(gb, ADDR_WarioInvisiblePal);
		CALL(SetWarioPal);
		return;
	}
	alu_cp(gb, gb->cpu.a, 3);
	if (!ZF)
		return;
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
}

PORT(HandleWaterSurfaceFloatingMovement)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (!ZF)
		return;
	SET_HL(gb, wWaterSurfaceFloatingCounter);
	gb->cpu.a = alu_inc(gb, rd8(gb, wWaterSurfaceFloatingCounter));
	wr8(gb, wWaterSurfaceFloatingCounter, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 7);
	if (CF) {
		// go down
		gb->cpu.b = 1;
		CALL(MoveWarioDown_WithoutCameraOffset);
		return;
	}
	alu_cp(gb, gb->cpu.a, 12);
	if (!CF) {
		xor_a(gb);
		wr8(gb, wWaterSurfaceFloatingCounter, 0);
	}
	gb->cpu.b = 1;
	CALL(MoveWarioUp_WithoutCameraOffset);
}
