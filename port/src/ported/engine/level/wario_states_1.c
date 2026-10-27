// Port of src/engine/level/wario_states_1.asm
#include "wario_states.h"

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

PORT_JUMP(StartJump)
{
	ld_cp(gb, wPowerUpLevel, HIGH_JUMP_BOOTS);
	gb->cpu.a = JUMP_VEL_NORMAL;
	if (!CF) {
		SET_HL(gb, wJoypadDown);
		if (alu_bit(gb, rd8(gb, wJoypadDown), B_PAD_UP))
			gb->cpu.a = JUMP_VEL_HIGH_JUMP;
	}
	wr8(gb, wJumpVelTable, gb->cpu.a);
	LJUMP(SetState_Airborne); // fallthrough
}

PORT_JUMP(SetState_Walking_ResetWalkVel)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	LJUMP(SetState_Walking); // fallthrough
}

PORT_JUMP(CrouchOrSlide)
{
	if (ld_and(gb, wWarioSlopeInteraction)) {
		LJUMP(SetState_Sliding);
		return;
	}
	collision_box(gb, -1, -15, -9, 9);
	st(gb, wWarioState, WST_CROUCHING);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wWalkVelIndex, wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	st(gb, wIsCrouching, TRUE);
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
	if (!ZF) {
		LJUMP(SetState_CrouchWalking);
		return;
	}
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	ld_cp(gb, wDirection, DIRECTION_RIGHT);
	set_frameset(gb, ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(StartCrouchFall)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(SetState_CrouchAirborne);
}

PORT_JUMP(StartCrouchJump)
{
	play_sfx(gb, SFX_JUMP);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	LJUMP(SetState_CrouchAirborne); // fallthrough
}

PORT_JUMP(StartFall_SkipResetJumpUpwards)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(StartJump); // jr
}

PORT_JUMP(StartJump_FromInput)
{
	play_sfx(gb, SFX_JUMP);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wIsSmashAttacking, 0);
	st(gb, wJumpingUpwards, TRUE);
	LJUMP(StartJump); // fallthrough
}

PORT_JUMP(DiveFromSurface_SkipSplash)
{
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	set_oam(gb, BANK_OAM_15254, ADDR_OAM_15254);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, facing_right(gb) ? ADDR_Frameset_155a3 : ADDR_Frameset_155a0);
	anim(gb, OAM1);
	LJUMP(SetState_Diving); // jr
}

PORT_JUMP(SurfaceFromUnderwater)
{
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWaterSurfaceFloatingCounter, 0);
	load_gfx(gb, BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	set_oam(gb, BANK_OAM_15254, ADDR_OAM_15254);
	LJUMP(SetState_WaterSurfaceIdling); // fallthrough
}

PORT_JUMP(StartFall)
{
	xor_a(gb);
	wr8(gb, wJumpingUpwards, FALSE);
	LJUMP(StartFall_SkipResetJumpUpwards); // fallthrough
}

PORT_JUMP(StartJump_FromWaterSurface)
{
	play_sfx(gb, SFX_SPLASH);
	st(gb, wJumpVelIndex, 0x01);
	st(gb, wJumpingUpwards, TRUE);
	st(gb, wDoFullJump, TRUE);
	LJUMP(StartJump); // jr
}

PORT_JUMP(StartJump_FromSand)
{
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	st(gb, wJumpingUpwards, TRUE);
	st(gb, wDoFullJump, TRUE);
	ld_cp(gb, wPowerUpLevel, HIGH_JUMP_BOOTS);
	gb->cpu.a = JUMP_VEL_KNOCK_BACK;
	if (!CF) {
		SET_HL(gb, wJoypadDown);
		if (alu_bit(gb, rd8(gb, wJoypadDown), B_PAD_UP))
			gb->cpu.a = JUMP_VEL_NORMAL;
	}
	wr8(gb, wJumpVelTable, gb->cpu.a);
	LJUMP(SetState_Airborne); // jr
}

PORT_JUMP(DiveFromSurface)
{
	xor_a(gb);
	wr8(gb, wGrabState, 0);
	get_pos(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.b = PARTICLE_SPLASH;
	FARCALL(CreateParticle);
	play_sfx(gb, SFX_SPLASH);
	LJUMP(DiveFromSurface_SkipSplash); // fallthrough
}

PORT(SetState_TryingSubmerge)
{
	st(gb, wWarioState, WST_TRYING_SUBMERGE);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_155b0, ADDR_Frameset_155b7);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_TryingSubmerge)
{
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		LJUMP(SetState_WaterSurfaceIdling);
		return;
	}
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(Func_1e1e3)
{
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	LJUMP(SetState_RollingAirborne); // jr
}

PORT_JUMP(HandleInput_Idling)
{
	uint8_t pressed = ld(gb, wJoypadPressed);
	if (alu_bit(gb, pressed, B_PAD_B)) {
		LJUMP(SetState_Attacking);
		return;
	}
	if (alu_bit(gb, pressed, B_PAD_A)) {
		LJUMP(StartJump_FromInput);
		return;
	}
	uint8_t down = ld(gb, wJoypadDown);
	bool right;
	if (alu_bit(gb, down, B_PAD_RIGHT))
		right = true;
	else if (alu_bit(gb, down, B_PAD_LEFT))
		right = false;
	else {
		RET();
		return;
	}
	// walk if already facing that way, else turn
	if (ld_and(gb, wDirection) == right)
		LJUMP(SetState_Walking_ResetWalkVel);
	else
		LJUMP(SetState_Turning);
}
