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

PORT_JUMP(Func_1e174)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(SetState_RollingAirborne); // fallthrough
}

PORT(Func_1f863)
{
	uint16_t hl = alu_add16(gb, ADDR_SwimStraightVelTable, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.b = rd8(gb, hl);
	CALL(MoveWarioUp);
}

PORT(SetState_Throwing)
{
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wWarioState, WST_THROWING);
}

PORT(Func_1f3a7)
{
	FARCALL(CheckCentreCollision);
	if (b_and(gb)) {
		update_pos_y(gb);
		st(gb, wc0e1, PAD_DOWN);
		return;
	}
	SET_HL(gb, wCurWaterCurrent - 1);
	gb->cpu.a = alu_or(gb, rd8(gb, wCurWaterCurrent), rd8(gb, wCurWaterCurrent - 1));
	gb->cpu.a = alu_and(gb, gb->cpu.a, CURRENT_UP | CURRENT_DOWN);
	if (!ZF)
		return;
	gb->cpu.b = 0x01;
	CALL(MoveWarioDown);
}

PORT(SetState_SmashAttacking)
{
	st(gb, wIsSmashAttacking, 0x81);
	st(gb, wWarioState, WST_SMASH_ATTACKING);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelIndex,
		wJumpVelTable, 0};
	clear(gb, zero);
	collision_box(gb, -1, -27, -9, 9);
}

PORT(SetState_LadderShakeStunned)
{
	st(gb, wWarioState, WST_LADDER_SHAKE_SLIDING);
	play_sfx(gb, SFX_SLIDE);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
		wIsGettingOffLadder, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_15948);
	anim(gb, OAM1);
}

PORT_JUMP(StartDive)
{
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_15569, ADDR_Frameset_15573);
	anim(gb, OAM1);
	LJUMP(SetState_Diving); // fallthrough
}

// ld a, [wWarioState] / cp WST_IDLING: true if still idling
static bool still_idling(GB *gb)
{
	ld_cp(gb, wWarioState, WST_IDLING);
	return ZF;
}

PORT_JUMP(UpdateState_Idling)
{
	FARCALL(CheckCentreCollision);
	if (!still_idling(gb)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(StartFall);
		return;
	}
	update_pos_y(gb);
	if (!still_idling(gb)) {
		RET();
		return;
	}

	if (ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		FARCALL(Func_19b12);
		if (!still_idling(gb)) {
			RET();
			return;
		}
		ld_cp(gb, wLadderInteraction, LADDER_UNDERNEATH);
		if (!ZF) {
			LJUMP(CrouchOrSlide);
			return;
		}
		FARCALL(SetState_LadderClimbing);
		RET();
		return;
	}

	anim(gb, OAM1);
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wLadderInteraction) && ld_bit(gb, wJoypadDown, B_PAD_UP)) {
		FARCALL(SetState_LadderClimbing);
		RET();
		return;
	}
	if (ld_and(gb, wIsNearFence) && ld_bit(gb, wJoypadDown, B_PAD_UP)) {
		FARCALL(SetState_FenceMovingVertical);
		RET();
		return;
	}
	if (!still_idling(gb)) {
		RET();
		return;
	}
	LCALL(HandleInput_Idling);
	if (!still_idling(gb) || ld_and(gb, wIsOnSteppableObject)) {
		RET();
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioStateCounter), 1);
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, wWarioStateCycles), 0);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, NUM_SLEEPING_IDLE_CYCLES);
	if (!CF)
		FARCALL(SetState_Sleeping);
	RET();
}

PORT(SetState_ThrowingAirborne)
{
	st(gb, wWarioState, WST_THROWING_AIRBORNE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK), GRAB_THROW);
	wr8(gb, wGrabState, gb->cpu.a);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_15fae, ADDR_Frameset_15fb3);
	anim(gb, OAM1);
}

PORT(SetState_WaterSurfaceMoving)
{
	st(gb, wWarioState, WST_WATER_SURFACE_MOVING);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wSwimVelIndex, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_15554, ADDR_Frameset_1555b);
	anim(gb, OAM1);
}

PORT(SetState_Rolling)
{
	static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelIndex, wJumpVelTable, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_ROLLING);
	collision_box(gb, -1, -14, -9, 9);
	st(gb, wIsRolling, 0x02);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_166e6, ADDR_Frameset_166f7);
	anim(gb, OAM1);
}

PORT(SetState_SwimKnockBack)
{
	play_sfx(gb, SFX_BUMP);
	st(gb, wWarioState, WST_SWIM_KNOCK_BACK);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wSwimVelIndex,
		wSwimmingDirectionInput, wc0e0, 0};
	clear(gb, zero);
	load_gfx(gb, BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_15573, ADDR_Frameset_15569);
	anim(gb, OAM1);
}

PORT(SetState_ThrowCharging)
{
	play_sfx(gb, SFX_02C);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_THROW_CHARGING);
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK), GRAB_CHARGE_THROW);
	wr8(gb, wGrabState, gb->cpu.a);
	load_gfx(gb, BANK_WarioThrowGfx, ADDR_WarioThrowGfx);
	set_oam(gb, BANK_OAM_1606a, ADDR_OAM_1606a);
	frameset_by_dir(gb, ADDR_Frameset_1642c, ADDR_Frameset_1642f);
	anim(gb, OAM1);
}

PORT(SetState_CrouchSliding)
{
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wAttackCounter, 0);
	play_sfx(gb, SFX_SLIDE);
	st(gb, wWarioState, WST_CROUCH_SLIDING);
	st(gb, wIsCrouching, TRUE);
	collision_box(gb, -1, -15, -9, 9);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	ld_cp(gb, wDirection, DIRECTION_RIGHT);
	set_frameset(gb, ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_PickedUp)
{
	bool wiggle;
	bool right = facing_right(gb);
	if (!right)
		wiggle = ld_mask(gb, wJoypadPressed, PAD_A | PAD_B | PAD_RIGHT);
	else
		wiggle = ld_mask(gb, wJoypadPressed, PAD_A | PAD_B | PAD_LEFT);
	if (!wiggle) {
		anim(gb, OAM1);
		SET_HL(gb, wPickedUpFrameCounter);
		wr8(gb, wPickedUpFrameCounter, alu_dec(gb, rd8(gb, wPickedUpFrameCounter))); // dec [hl]
		if (!ZF) {
			RET();
			return;
		}
		st(gb, wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
		st(gb, wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
		RET();
		return;
	}
	st(gb, wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
	if (!right) {
		st(gb, wDirection, DIRECTION_RIGHT);
		set_frameset(gb, ADDR_Frameset_15f7f);
	} else {
		st(gb, wDirection, DIRECTION_LEFT);
		set_frameset(gb, ADDR_Frameset_15f70);
	}
	reset_anim(gb);
	anim(gb, OAM1);
	st(gb, wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
	SET_HL(gb, wPickedUpWiggleCounter);
	wr8(gb, wPickedUpWiggleCounter, alu_dec(gb, rd8(gb, wPickedUpWiggleCounter))); // dec [hl]
	if (!ZF) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wAutoMoveState, 0);
	LJUMP(StartFall);
}

PORT(SetState_UnderwaterThrusting)
{
	st(gb, wWarioState, WST_UNDERWATER_THRUSTING);
	collision_box(gb, -1, -15, -9, 9);
	static const uint16_t zero[] = {wSFXLoopCounter, wSwimVelIndex, wWarioStateCycles,
		wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	bool right;
	if (!ld_and(gb, wSwimmingDirectionInput)) {
		ld_cp(gb, wDirection, DIRECTION_RIGHT);
		right = ZF;
		st(gb, wSwimmingDirectionInput, right ? PAD_RIGHT : PAD_LEFT);
	} else if (ld_bit(gb, wSwimmingDirectionInput, B_PAD_LEFT)) {
		right = false;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		right = true;
	} else {
		right = facing_right(gb);
	}
	st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
	ld_cp(gb, wPowerUpLevel, PRINCE_FROGS_GLOVES);
	if (right)
		set_frameset(gb, CF ? ADDR_Frameset_155c5 : ADDR_Frameset_1556c);
	else
		set_frameset(gb, CF ? ADDR_Frameset_155be : ADDR_Frameset_15562);
	anim(gb, OAM1);
}

PORT_JUMP(StartUpwardsUnderwaterThrusting)
{
	st(gb, wSwimmingDirectionInput, PAD_UP);
	LJUMP(SetState_UnderwaterThrusting);
}

PORT_JUMP(SetState_SwimKnockBack_SwitchDirection)
{
	flip_dir(gb);
	LJUMP(SetState_SwimKnockBack); // fallthrough
}

PORT_JUMP(StartUnderwaterThrusting)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_CTRL_PAD);
	wr8(gb, wSwimmingDirectionInput, gb->cpu.a);
	LJUMP(SetState_UnderwaterThrusting); // fallthrough
}

PORT_JUMP(Func_1de3f)
{
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_163fe, ADDR_Frameset_1641e);
	anim(gb, OAM1);
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK), GRAB_THROW);
	wr8(gb, wGrabState, gb->cpu.a);
	LJUMP(SetState_Throwing); // fallthrough
}

// airborne gfx and OAM, then a frameset by direction and the animation
static void airborne_gfx(GB *gb, uint16_t left, uint16_t right)
{
	reset_anim(gb);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	wr8(gb, wWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_15955 >> 8));
	st(gb, wWarioOAMPtr + 1, (uint8_t)ADDR_OAM_15955);
	frameset_by_dir(gb, left, right);
	anim(gb, OAM1);
}

PORT_JUMP(DoGroundPound)
{
	play_sfx(gb, SFX_GROUND_POUND);
	airborne_gfx(gb, ADDR_Frameset_15ff2, ADDR_Frameset_16009);
	LJUMP(SetState_SmashAttacking); // fallthrough
}

PORT_JUMP(DoWarioGroundShake)
{
	if (!ld_and(gb, wGroundShakeCounter)) {
		st(gb, wGroundShakeCounter, 32);
		st(gb, wIsWarioGroundShaking, TRUE);
	}
	if (!ld_and(gb, wFloorTransitionDir))
		play_sfx(gb, SFX_GROUND_SHAKE);
	airborne_gfx(gb, ADDR_Frameset_15fb8, ADDR_Frameset_15fc5);
	LJUMP(SetState_SmashAttacking);
}

PORT_JUMP(HandleGroundShake)
{
	if (ld_and(gb, wLevelEndScreen)) {
		RET();
		return;
	}
	SET_HL(gb, wGroundShakeCounter);
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		xor_a(gb);
		wr8(gb, wIsWarioGroundShaking, 0);
		RET();
		return;
	}
	bool cont = true;
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!CF) {
		static const uint16_t checks[] = {wIsGettingOffLadder, wIsWarioGroundShaking,
			wJumpVelTable, wWaterInteraction, wTransformation, wRoomTransitionParam,
			wIsIntangible, wIsOnSteppableObject, 0};
		const uint16_t *p;
		for (p = checks; *p; p++)
			if (ld_and(gb, *p))
				break;
		if (!*p) {
			ld_cp(gb, wWarioState, WST_GROUND_SHAKE_STUNNED);
			if (!ZF)
				cont = false;
		}
	}
	if (cont) {
		gb->cpu.a = alu_dec(gb, rd8(gb, wGroundShakeCounter));
		wr8(gb, wGroundShakeCounter, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x02);
		st(gb, wSCYShake, gb->cpu.a ? 2 : 0);
		if (!gb->cpu.a) {
			// xor a
			gb->cpu.f = FLAG_Z;
		}
		RET();
		return;
	}
	// .wario_shake_stun
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, WST_LADDER_CLIMBING);
	if (!ZF)
		alu_cp(gb, gb->cpu.a, WST_LADDER_IDLING);
	if (!ZF)
		alu_cp(gb, gb->cpu.a, WST_LADDER_SCRATCHING);
	if (ZF) {
		LJUMP(SetState_LadderShakeStunned);
		return;
	}
	st(gb, wWarioState, WST_GROUND_SHAKE_STUNNED);
	static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wAttackCounter,
		wIsRolling, wGrabState, 0};
	clear(gb, zero);
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	if (!ld_and(gb, wIsCrouching)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		st(gb, wJumpVelIndex, 10);
		st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	}
	st(gb, wCollisionBoxBottom, (uint8_t)-1);
	st(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
	static const uint16_t zero2[] = {wWarioStateCounter, wWarioStateCycles,
		wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero2);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	frameset_by_dir(gb, ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(SetState_Idling)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_IDLING);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wJumpVelIndex,
		wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking,
		wIsCrouching, wAttackCounter, wGrabState, 0};
	clear(gb, zero);
	coll_box_blrt(gb);
	FARCALL(CheckUpCollision);
	ld_cp(gb, wWarioState, WST_IDLING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb)) {
		LJUMP(CrouchOrSlide);
		return;
	}
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14000, ADDR_OAM_14000);
	frameset_by_dir(gb, ADDR_Frameset_14252, ADDR_Frameset_1425f);
	anim(gb, OAM1);
	RET();
}

// farcall CheckFrontCollision / ld a, b / and $0f / ret nz
static bool front_blocked(GB *gb)
{
	FARCALL(CheckFrontCollision);
	gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
	return gb->cpu.a != 0;
}

PORT(WalkIfPossible_Right)
{
	if (front_blocked(gb))
		return;
	CALL(ApplyWalkVelocity_Right);
	CALL(MoveWarioRight);
}

PORT(WalkIfPossible_Left)
{
	if (front_blocked(gb))
		return;
	CALL(ApplyWalkVelocity_Left);
	CALL(MoveWarioLeft);
}

PORT_JUMP(Func_1ec19)
{
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	FARCALL(CheckUpCollision);
	ld_cp(gb, wWarioState, WST_CROUCHING);
	if (!ZF) {
		alu_cp(gb, gb->cpu.a, WST_CROUCH_WALKING);
		if (!ZF) {
			RET();
			return;
		}
	}
	if (!b_and(gb)) {
		LJUMP(StartCrouchJump);
		return;
	}
	xor_a(gb);
	wr8(gb, wJumpVelTable, 0);
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	RET();
}

PORT_JUMP(Func_1ec4b)
{
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	FARCALL(CheckUpCollision);
	if (!b_and(gb)) {
		LJUMP(SetState_Attacking);
		return;
	}
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	RET();
}

// farcall Func_19b12 / ld a, [wWaterInteraction] / cp NON_SUBMERSIBLE_WATER
static bool non_submersible(GB *gb)
{
	FARCALL(Func_19b12);
	ld_cp(gb, wWaterInteraction, NON_SUBMERSIBLE_WATER);
	return ZF;
}

PORT_JUMP(HandleWaterSurfaceInput)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(StartJump_FromWaterSurface);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		ld_cp(gb, wPowerUpLevel, SWIMMING_FLIPPERS);
		if (!CF) {
			if (ld_bit(gb, wJoypadDown, B_PAD_UP)) {
				RET();
				return;
			}
			if (non_submersible(gb)) {
				LJUMP(SetState_TryingSubmerge);
				return;
			}
			gb->cpu.b = 3;
			CALL(MoveWarioDown);
			LJUMP(StartUnderwaterThrusting);
			return;
		}
	}
	// .no_thrust
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		RET();
		return;
	}
	ld_cp(gb, wPowerUpLevel, SWIMMING_FLIPPERS);
	if (CF || non_submersible(gb)) {
		LJUMP(SetState_TryingSubmerge);
		return;
	}
	LJUMP(SetState_Submerged);
}

// farcall f / ld a, b / and a; true if nz
static bool farcall_b(GB *gb, uint8_t bank, uint16_t addr)
{
	port_farcall(gb, bank, addr);
	return b_and(gb);
}

PORT_JUMP(HandleBumpMovement)
{
	ld_cp(gb, wJumpVelIndex, 28);
	if (!CF) {
		gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (gb->cpu.a) {
			if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
				st(gb, wDirection, DIRECTION_RIGHT);
				if (!farcall_b(gb, BANK_Func_197b1, ADDR_Func_197b1)) {
					gb->cpu.b = 1;
					CALL(MoveWarioLeft);
				}
			} else {
				st(gb, wDirection, DIRECTION_LEFT);
				if (!farcall_b(gb, BANK_Func_19741, ADDR_Func_19741)) {
					gb->cpu.b = 1;
					CALL(MoveWarioRight);
				}
			}
			xor_a(gb);
			wr8(gb, wWalkVelIndex, 0);
			LJUMP(StartFall);
			return;
		}
	}
	// .no_control
	ld_cp(gb, wDirection, DIRECTION_RIGHT);
	if (ZF) {
		gb->cpu.b = 1;
		CALL(MoveWarioRight);
		if (farcall_b(gb, BANK_Func_19741, ADDR_Func_19741)) {
			gb->cpu.b = 1;
			CALL(MoveWarioLeft);
		}
	} else {
		gb->cpu.b = 1;
		CALL(MoveWarioLeft);
		if (farcall_b(gb, BANK_Func_197b1, ADDR_Func_197b1)) {
			gb->cpu.b = 1;
			CALL(MoveWarioRight);
		}
	}
	// .handle_vertical
	gb->cpu.a = rd8(gb, wJumpVelIndex);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	SET_HL(gb, hl);
	uint8_t v = rd8(gb, hl);
	if (alu_bit(gb, v, 7)) {
		gb->cpu.a = rd8(gb, hl);
		gb->cpu.a = alu_cpl(gb, gb->cpu.a);
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		gb->cpu.b = gb->cpu.a;
		CALL(MoveWarioUp);
		SET_HL(gb, wJumpVelIndex);
		wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
	} else {
		gb->cpu.b = v;
		CALL(MoveWarioDown);
		SET_HL(gb, wJumpVelIndex);
		gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelIndex));
		wr8(gb, wJumpVelIndex, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX);
		if (!CF)
			wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
	}
	RET();
}

// call .ret_label: a call to a bare ret, which only leaves the return
// address below sp
static void call_to_ret(GB *gb, uint16_t ret)
{
	gb_push16(gb, ret);
	gb_pop16(gb);
}

PORT(SetState_Bumping)
{
	st(gb, wWarioState, WST_BUMPING);
	flip_dir(gb);
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wAttackCounter, 0};
	clear(gb, zero);
	if (!ld_and(gb, wIsRolling)) {
		frameset_by_dir(gb, ADDR_Frameset_15219, ADDR_Frameset_15216);
		anim(gb, OAM1);
		return;
	}
	xor_a(gb);
	wr8(gb, wIsRolling, 0);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	if (!facing_right(gb)) {
		call_to_ret(gb, ADDR_SetState_Bumping + 0xa7); // 4aed
		set_frameset(gb, ADDR_Frameset_15f7f);
	} else {
		call_to_ret(gb, ADDR_SetState_Bumping + 0xb6); // 4afc
		set_frameset(gb, ADDR_Frameset_15f70);
	}
	anim(gb, OAM1);
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	FARCALL(CheckUpCollision);
	if (b_and(gb))
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
}

PORT(HandleDivingHorizontalMovement)
{
	ld_cp(gb, wPowerUpLevel, SWIMMING_FLIPPERS);
	if (CF)
		return;
	gb->cpu.a = rd8(gb, wJoypadDown);
	bool right;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		right = true;
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		right = false;
	else
		return;
	uint8_t dir = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
	ld_cp(gb, wDirection, dir);
	if (!ZF)
		st(gb, wDirection, dir);
	if (farcall_b(gb, BANK_CheckFrontCollision, ADDR_CheckFrontCollision))
		return;
	gb->cpu.b = 1;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
}

PORT_JUMP(DoHardLand)
{
	CALL(TriggerFloorTransition_SkipUpdateYPos);
	static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wFrameDuration,
		wAnimationFrame, wWalkVelIndex, 0};
	clear(gb, zero);
	if (ld_and(gb, wIsSmashAttacking)) {
		ld_cp(gb, wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (CF)
			LJUMP(DoGroundPound);
		else
			LJUMP(DoWarioGroundShake);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_DOWN);
	if (gb->cpu.a) {
		LJUMP(CrouchOrSlide);
		return;
	}
	play_sfx(gb, SFX_LAND);
	st(gb, wWarioState, WST_LANDING);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	frameset_by_dir(gb, ADDR_Frameset_15f9a, ADDR_Frameset_15fa1);
	anim(gb, OAM1);
	RET();
}

PORT(HandleInvincibility)
{
	if (!ld_and(gb, wInvincibleCounter))
		return;
	alu_cp(gb, gb->cpu.a, 0x16);
	if (!CF) {
		// .fast
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b1);
		if (gb->cpu.a)
			return;
		gb->cpu.a = alu_inc(gb, rd8(gb, wInvincibleCounter));
		wr8(gb, wInvincibleCounter, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 0x28);
		if (!CF) {
			xor_a(gb);
			wr8(gb, wInvincibleCounter, 0);
			wr8(gb, wInvisibleFrame, 0);
			gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT | PAD_DOWN);
			if (gb->cpu.a)
				return;
			FARCALL(CheckUpCollision);
			return;
		}
	} else {
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!CF) {
			// .slow
			gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b11);
			if (gb->cpu.a)
				return;
			gb->cpu.a = alu_inc(gb, rd8(gb, wInvincibleCounter));
			wr8(gb, wInvincibleCounter, gb->cpu.a);
		} else {
			ld_cp(gb, wWarioState, WST_STUNG);
			if (ZF)
				return;
			alu_cp(gb, gb->cpu.a, WST_STUNG_RECOVERY);
			if (ZF)
				return;
			st(gb, wInvincibleCounter, 0x10);
			ld_cp(gb, wWarioPalsPtr + 0, (uint8_t)(ADDR_WarioInvisiblePal >> 8));
			if (!ZF)
				return;
			ld_cp(gb, wWarioPalsPtr + 1, (uint8_t)ADDR_WarioInvisiblePal);
			if (!ZF)
				return;
			SET_HL(gb, ADDR_WarioDefaultPal);
			CALL(SetWarioPal);
			return;
		}
	}
	// .alternate_invisibility
	gb->cpu.a = alu_xor(gb, rd8(gb, wInvisibleFrame), 0x1);
	wr8(gb, wInvisibleFrame, gb->cpu.a);
}

PORT_JUMP(DoSoftLand)
{
	CALL(TriggerFloorTransition_SkipUpdateYPos);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	if (ld_and(gb, wIsSmashAttacking)) {
		ld_cp(gb, wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (CF)
			LJUMP(DoGroundPound);
		else
			LJUMP(DoWarioGroundShake);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_DOWN);
	if (gb->cpu.a) {
		LJUMP(CrouchOrSlide);
		return;
	}
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	FARCALL(CheckUpCollision);
	if (b_and(gb)) {
		LJUMP(CrouchOrSlide);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
	if (gb->cpu.a)
		LJUMP(SetState_Walking);
	else
		LJUMP(SetState_Idling);
}

PORT_JUMP(WalkRight)
{
	xor_a(gb);
	wr8(gb, wIsTurning, 0);
	if (!facing_right(gb)) {
		st(gb, wDirection, DIRECTION_RIGHT);
		st(gb, wIsTurning, TRUE);
	}
	LJUMP(WalkIfPossible_Right);
}

PORT_JUMP(WalkLeft)
{
	xor_a(gb);
	wr8(gb, wIsTurning, 0);
	if (facing_right(gb)) {
		st(gb, wDirection, DIRECTION_LEFT);
		st(gb, wIsTurning, TRUE);
	}
	LJUMP(WalkIfPossible_Left);
}

PORT_JUMP(DoBumpOrJumpingBump)
{
	play_sfx(gb, SFX_BUMP);
	st(gb, wJumpVelIndex, ld_and(gb, wJumpVelTable) ? FALLING_JUMP_VEL_INDEX : 10);
	LJUMP(SetState_Bumping);
}

PORT_JUMP(DoJumpingBump_NoSFX)
{
	st(gb, wJumpVelIndex, 10);
	LJUMP(SetState_Bumping); // fallthrough
}

PORT_JUMP(UpdateState_Turning)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(StartJump_FromInput);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(SetState_Attacking);
		return;
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
	if (gb->cpu.a)
		LJUMP(SetState_Walking_ResetWalkVel);
	else
		LJUMP(SetState_Idling);
}

PORT_JUMP(Func_1eb46)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(Func_1ec19);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(Func_1ec4b);
		return;
	}
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		ld_cp(gb, wWarioState, WST_CROUCHING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and(gb)) {
			LJUMP(SetState_Idling);
			return;
		}
	}
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
	if (!gb->cpu.a) {
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		st(gb, wDirection, DIRECTION_RIGHT);
	else
		st(gb, wDirection, DIRECTION_LEFT);
	LJUMP(SetState_CrouchWalking);
}

PORT_JUMP(UpdateState_Diving)
{
	FARCALL(CheckCentreCollision);
	ld_cp(gb, wWarioState, WST_DIVING);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LCALL(HandleDivingHorizontalMovement);
		gb->cpu.a = inc_counter(gb);
		alu_cp(gb, gb->cpu.a, 10);
		if (CF) {
			gb->cpu.b = 2;
		} else {
			alu_cp(gb, gb->cpu.a, 14);
			if (!CF) {
				LJUMP(SetState_Submerged);
				return;
			}
			gb->cpu.b = 1;
		}
		CALL(MoveWarioDown);
		FARCALL(CheckCentreCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
	}
	// .submerge
	update_pos_y(gb);
	LJUMP(SetState_Submerged); // fallthrough
}

PORT_JUMP(Func_1f825)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		xor_a(gb);
		wr8(gb, wIsRolling, 0);
		LJUMP(StartJump_FromInput);
		return;
	}
	bool right = facing_right(gb);
	if (ld_bit(gb, wJoypadPressed, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
		xor_a(gb);
		wr8(gb, wIsRolling, 0);
		LJUMP(SetState_Idling);
		return;
	}
	if (right)
		LCALL(WalkIfPossible_Right);
	else
		LCALL(WalkIfPossible_Left);
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	RET();
}

PORT_JUMP(UpdateState_StungRecovery)
{
	LCALL(Func_1f6c2);
	anim(gb, OAM1);
	gb->cpu.a = inc_counter(gb);
	alu_cp(gb, gb->cpu.a, 0x14);
	if (CF) {
		RET();
		return;
	}
	flip_dir(gb);
	st(gb, wInvincibleCounter, 0x10);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	FARCALL(CheckUpCollision);
	ld_cp(gb, wWarioState, WST_STUNG_RECOVERY);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
			LJUMP(SetState_Idling);
			return;
		}
	}
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	LJUMP(CrouchOrSlide);
}

// the shared tail of UpdateState_PipeGoingDown/Up; true if it jumped
static bool pipe_counter(GB *gb, bool up)
{
	gb->cpu.a = inc_counter(gb);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (ZF) {
		CALL(ClearTransformationValues);
		SET_HL(gb, wRoomTransitionParam);
		wr8(gb, wRoomTransitionParam, rd8(gb, wRoomTransitionParam) & ~(1 << ROOMTRANSITIONF_DELAY_F));
		JUMP(TriggerRoomTransition);
		return true;
	}
	alu_cp(gb, gb->cpu.a, 0x21);
	if (ZF) {
		play_sfx(gb, SFX_PIPE_EXIT);
		RET();
		return true;
	}
	alu_cp(gb, gb->cpu.a, 0x40);
	if (CF) {
		RET();
		return true;
	}
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos + 1), 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	if (up)
		CALL(UpdateFloor);
	LJUMP(SetState_Idling);
	return true;
}

PORT_JUMP(UpdateState_PipeGoingDown)
{
	st(gb, wIsIntangible, TRUE);
	anim(gb, OAM1);
	gb->cpu.b = 1;
	CALL(MoveWarioDown);
	pipe_counter(gb, false);
}

PORT_JUMP(UpdateState_PipeGoingUp)
{
	st(gb, wIsIntangible, TRUE);
	anim(gb, OAM1);
	gb->cpu.b = 1;
	CALL(MoveWarioUp);
	pipe_counter(gb, true);
}

PORT_JUMP(UpdateState_Landing)
{
	if (ld_mask(gb, wJoypadPressed, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_Walking_ResetWalkVel);
		return;
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	bool jump = false;
	if (!ld_and(gb, wJumpingUpwards) && ld_bit(gb, wJoypadDown, B_PAD_A))
		jump = true;
	if (!jump && !ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		if (ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
			LJUMP(CrouchOrSlide);
			return;
		}
		LJUMP(SetState_Idling);
		return;
	}
	LCALL(StartJump_FromInput);
	ld_cp(gb, wPowerUpLevel, HIGH_JUMP_BOOTS);
	if (!CF)
		st(gb, wJumpVelTable, JUMP_VEL_HIGH_JUMP);
	RET();
}

PORT_JUMP(HandleInput_Walking)
{
	if (!ld_and(gb, wJumpingUpwards) && ld_bit(gb, wJoypadDown, B_PAD_A)) {
		LJUMP(StartJump_FromInput);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(SetState_Attacking);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(StartJump_FromInput);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		CALL(IsOnSlipperyGround);
		if (ZF) {
			LJUMP(CrouchOrSlide);
			return;
		}
		FARCALL(SetState_CrouchSlipping);
		RET();
		return;
	}
	bool right;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		right = true;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		right = false;
	} else {
		CALL(IsOnSlipperyGround);
		if (ZF) {
			LJUMP(SetState_Idling);
			return;
		}
		FARCALL(SetState_Slipping);
		RET();
		return;
	}
	bool facing = facing_right(gb);
	if (facing != right) {
		LJUMP(SetState_Turning);
		return;
	}
	st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
	FARCALL(CheckFrontCollision);
	if (b_and(gb)) {
		RET();
		return;
	}
	if (right) {
		CALL(ApplyWalkVelocity_Right);
		CALL(MoveWarioRight);
	} else {
		CALL(ApplyWalkVelocity_Left);
		CALL(MoveWarioLeft);
	}
	ld_cp(gb, wWalkVelIndex, 0x10);
	if (!CF)
		st(gb, wWalkVelIndex, 0x0c);
	RET();
}

PORT_JUMP(Func_1ec6c)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(Func_1ec19);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(Func_1ec4b);
		return;
	}
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		ld_cp(gb, wWarioState, WST_CROUCH_WALKING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and(gb)) {
			LJUMP(SetState_Idling);
			return;
		}
	}
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	gb->cpu.a = rd8(gb, wJoypadDown);
	bool right;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		right = true;
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		right = false;
	else {
		LJUMP(CrouchOrSlide);
		return;
	}
	if (facing_right(gb) != right) {
		st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		reset_anim(gb);
		set_frameset(gb, right ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
	}
	FARCALL(CheckFrontCollision);
	gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
	if (gb->cpu.a) {
		RET();
		return;
	}
	if (right) {
		CALL(ApplyWalkVelocity_Right);
		CALL(MoveWarioRight);
	} else {
		CALL(ApplyWalkVelocity_Left);
		CALL(MoveWarioLeft);
	}
	ld_cp(gb, wWalkVelIndex, 0x04);
	if (!CF) {
		gb->cpu.a = 0;
		wr8(gb, wWalkVelIndex, 0);
	}
	RET();
}

PORT_JUMP(SetState_GrabIdling)
{
	if (!ld_and(gb, wGrabState)) {
		LJUMP(SetState_Idling);
		return;
	}
	static const uint16_t clr[] = {
		wJumpVelIndex, wJumpVelTable, wIsSmashAttacking, wIsCrouching,
		wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, 0,
	};
	clear(gb, clr);
	gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK);
	gb->cpu.a = alu_add(gb, gb->cpu.a, GRAB_IDLE);
	wr8(gb, wGrabState, gb->cpu.a);
	st(gb, wWarioState, WST_GRAB_IDLING);
	coll_box_btlr(gb);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	frameset_by_dir(gb, ADDR_Frameset_14a20, ADDR_Frameset_14a23);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(DoBumpWithBigBackOffset)
{
	gb->cpu.b = 3;
	if (facing_right(gb))
		CALL(MoveWarioLeft);
	else
		CALL(MoveWarioRight);
	LJUMP(DoBumpOrJumpingBump);
}

PORT_JUMP(DoBumpWithSmallBackOffset)
{
	gb->cpu.b = 2;
	if (facing_right(gb))
		CALL(MoveWarioLeft);
	else
		CALL(MoveWarioRight);
	LJUMP(DoBumpOrJumpingBump);
}

PORT_JUMP(DoJumpingBump)
{
	play_sfx(gb, SFX_BUMP);
	LJUMP(DoJumpingBump_NoSFX); // fallthrough
}

PORT_JUMP(UpdateState_Crouching)
{
	FARCALL(Func_19b25);
	ld_cp(gb, wWarioState, WST_CROUCHING);
	if (!ZF) {
		RET();
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return;
	}
	LCALL(Func_1eb46);
	ld_cp(gb, wWarioState, WST_CROUCHING);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(StartCrouchFall);
		return;
	}
	RET();
}

// farcall Func_19b25, then the room transition, water and sand checks of
// UpdateState_Bumping/Stung; true if it jumped
static bool bump_stung_start(GB *gb)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return true;
	}
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return true;
	}
	if (ld_and(gb, wIsInSand)) {
		FARCALL(SetState_SandFalling);
		RET();
		return true;
	}
	return false;
}

// ld hl, wJumpVelIndex / inc [hl] / ld a, [hl]
static uint8_t inc_jump_vel_index(GB *gb)
{
	SET_HL(gb, wJumpVelIndex);
	gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelIndex));
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	return gb->cpu.a;
}

PORT_JUMP(UpdateState_Bumping)
{
	if (bump_stung_start(gb))
		return;
	ld_cp(gb, wWarioState, WST_BUMPING);
	if (!ZF) {
		RET();
		return;
	}
	LCALL(HandleBumpMovement);
	ld_cp(gb, wWarioState, WST_BUMPING);
	if (!ZF) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		ld_cp(gb, wWarioState, WST_BUMPING);
		if (ZF && b_and(gb))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	update_pos_y(gb);
	flip_dir(gb);
	ld_cp(gb, wWarioState, WST_BUMPING);
	if (!ZF) {
		RET();
		return;
	}
	LJUMP(DoSoftLand);
}

PORT_JUMP(UpdateState_Stung)
{
	if (bump_stung_start(gb))
		return;
	LCALL(Func_1f6c2);
	anim(gb, OAM1);
	FARCALL(CheckFrontCollision);
	if (!b_and(gb)) {
		bool right = facing_right(gb);
		gb->cpu.b = 1;
		ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF)
			gb->cpu.b = alu_inc(gb, gb->cpu.b);
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	} else {
		gb->cpu.b = 1;
		if (facing_right(gb))
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
	}
	bool recover = false;
	if (!ld_and(gb, wJumpVelTable)) {
		alu_cp(gb, inc_jump_vel_index(gb), MAX_JUMP_VEL_INDEX);
		if (!CF) {
			recover = true;
		} else {
			FARCALL(CheckCentreCollision);
			gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
			if (!gb->cpu.a) {
				st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
				st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
			}
			RET();
			return;
		}
	} else {
		for (;;) {
			gb->cpu.e = rd8(gb, wJumpVelIndex);
			gb->cpu.a = gb->cpu.e;
			gb->cpu.d = 0;
			uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
			SET_HL(gb, hl);
			uint8_t v = rd8(gb, hl);
			if (!alu_bit(gb, v, 7)) {
				gb->cpu.b = v;
				CALL(MoveWarioDown);
				alu_cp(gb, inc_jump_vel_index(gb), MAX_JUMP_VEL_INDEX);
				if (!CF)
					wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
				break;
			}
			gb->cpu.a = alu_inc(gb, alu_cpl(gb, v));
			gb->cpu.b = gb->cpu.a;
			CALL(MoveWarioUp);
			inc_jump_vel_index(gb);
			FARCALL(CheckUpCollision);
			if (!b_and(gb)) {
				RET();
				return;
			}
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and(gb)) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelTable));
		wr8(gb, wJumpVelTable, gb->cpu.a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x02);
		if (ZF) {
			st(gb, wJumpVelIndex, 0x0a);
			RET();
			return;
		}
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (ZF) {
			st(gb, wJumpVelIndex, 0x0e);
			RET();
			return;
		}
		recover = true;
	}
	(void)recover;
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wWarioState, WST_STUNG_RECOVERY);
	RET();
}

PORT_JUMP(UpdateState_EnemyBumping)
{
	if (bump_stung_start(gb))
		return;
	ld_cp(gb, wWarioState, WST_ENEMY_BUMPING);
	if (!ZF) {
		RET();
		return;
	}
	anim(gb, OAM1);
	LCALL(HandleBumpMovement);
	ld_cp(gb, wWarioState, WST_ENEMY_BUMPING);
	if (!ZF) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (b_and(gb))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	flip_dir(gb);
	update_pos_y(gb);
	LJUMP(DoSoftLand);
}

PORT_JUMP(UpdateState_EnteringDoor)
{
	sfx_loop(gb, 36, SFX_WALK);
	anim(gb, OAM1);
	gb->cpu.a = rd8(gb, wWarioStateCounter);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (ZF) {
		if (anim_ended(gb)) {
			SET_HL(gb, wWarioStateCounter);
			wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
			SET_HL(gb, wRoomTransitionParam);
			wr8(gb, wRoomTransitionParam, rd8(gb, wRoomTransitionParam) & ~(1 << ROOMTRANSITIONF_DELAY_F));
			JUMP(TriggerRoomTransition);
			return;
		}
		RET();
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (ZF) {
		SET_HL(gb, wWarioStateCounter);
		wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
		RET();
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (ZF) {
		if (!anim_ended(gb)) {
			RET();
			return;
		}
		if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_Walking_ResetWalkVel);
			return;
		}
		LJUMP(SetState_Idling);
		return;
	}
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_14d15);
	anim(gb, OAM1);
	SET_HL(gb, wWarioStateCounter);
	wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
	RET();
}

// one direction of ApplyLastWaterCurrentMovement: with the current, the
// pad bit `with`, against it `against`; returns true to reset the current
static bool water_current_push_b(GB *gb, int with, int against, uint8_t bank, uint16_t coll, uint16_t move,
	uint8_t step);
static bool water_current_push(GB *gb, int with, int against, uint8_t bank, uint16_t coll, uint16_t move)
{
	return water_current_push_b(gb, with, against, bank, coll, move, 1);
}

// b = the distance moved (1 for the lingering current, 2 for the current itself)
static bool water_current_push_b(GB *gb, int with, int against, uint8_t bank, uint16_t coll, uint16_t move,
	uint8_t step)
{
	if (ld_bit(gb, wc0e1, with))
		return true;
	port_farcall(gb, bank, coll);
	if (b_and(gb))
		return true;
	gb->cpu.a = rd8(gb, wSwimmingDirectionInput);
	if (alu_bit(gb, gb->cpu.a, with))
		return false;
	if (alu_bit(gb, gb->cpu.a, against)) {
		ld_cp(gb, wPowerUpLevel, PRINCE_FROGS_GLOVES);
		if (!CF)
			return false;
	}
	gb->cpu.b = step;
	gb_call(gb, move);
	return false;
}

PORT(ApplyLastWaterCurrentMovement)
{
	if (!ld_and(gb, wLastWaterCurrent))
		return;
	bool reset;
	if (alu_bit(gb, gb->cpu.a, CURRENT_RIGHT_F))
		reset = water_current_push(gb, B_PAD_RIGHT, B_PAD_LEFT, BANK_Func_19741, ADDR_Func_19741, ADDR_MoveWarioRight);
	else if (alu_bit(gb, gb->cpu.a, CURRENT_LEFT_F))
		reset = water_current_push(gb, B_PAD_LEFT, B_PAD_RIGHT, BANK_Func_197b1, ADDR_Func_197b1, ADDR_MoveWarioLeft);
	else if (alu_bit(gb, gb->cpu.a, CURRENT_UP_F))
		reset = water_current_push(gb, B_PAD_UP, B_PAD_DOWN, BANK_CheckUpCollision, ADDR_CheckUpCollision, ADDR_MoveWarioUp);
	else if (alu_bit(gb, gb->cpu.a, CURRENT_DOWN_F))
		reset = water_current_push(gb, B_PAD_DOWN, B_PAD_UP, BANK_CheckCentreCollision, ADDR_CheckCentreCollision, ADDR_MoveWarioDown);
	else
		return;
	if (!reset) {
		gb->cpu.a = alu_sub(gb, rd8(gb, wLastWaterCurrent), 1 << 4);
		wr8(gb, wLastWaterCurrent, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xf0);
		if (gb->cpu.a)
			return;
	}
	xor_a(gb);
	wr8(gb, wLastWaterCurrent, 0);
}

PORT_JUMP(HandleInput_Airborne)
{
	if (!ld_bit(gb, wJoypadDown, B_PAD_A)) {
		xor_a(gb);
		wr8(gb, wJumpingUpwards, 0);
		ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF && !ld_and(gb, wDoFullJump))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	}
	gb->cpu.e = rd8(gb, wJumpVelIndex);
	gb->cpu.d = 0;
	uint16_t table;
	gb->cpu.a = alu_dec(gb, rd8(gb, wJumpVelTable));
	if (ZF) {
		table = ADDR_JumpVelTable_KnockBack;
	} else {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		table = ZF ? ADDR_JumpVelTable_Normal : ADDR_JumpVelTable_HighJump;
	}
	uint16_t hl = alu_add16(gb, table, REG_DE(gb));
	SET_HL(gb, hl);
	uint8_t v = rd8(gb, hl);
	if (alu_bit(gb, v, 7)) {
		gb->cpu.a = alu_inc(gb, alu_cpl(gb, v));
		gb->cpu.b = gb->cpu.a;
		CALL(MoveWarioUp);
		inc_jump_vel_index(gb);
	} else {
		xor_a(gb);
		wr8(gb, wDoFullJump, 0);
		gb->cpu.b = rd8(gb, hl);
		CALL(MoveWarioDown);
		alu_cp(gb, inc_jump_vel_index(gb), MAX_JUMP_VEL_INDEX);
		if (!CF)
			wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		LCALL(WalkRight);
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		LCALL(WalkLeft);
	} else {
		RET();
		return;
	}
	JUMP(Func_1762);
}

PORT_JUMP(UpdateState_SmashAttacking)
{
	anim(gb, OAM1);
	alu_cp(gb, inc_counter(gb), 0x1b);
	if (CF) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wIsSmashAttacking, 0);
	LJUMP(SetState_Idling);
}

// the shared body of UpdateState_Throwing/ThrowingAirborne
static void throwing(GB *gb, uint16_t done)
{
	if (!ld_and(gb, wGrabState)) {
		JUMP_TO(done);
		return;
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wGrabState, 0);
	JUMP_TO(done);
}

PORT_JUMP(UpdateState_Throwing) { throwing(gb, ADDR_SetState_Idling); }
PORT_JUMP(UpdateState_ThrowingAirborne) { throwing(gb, ADDR_StartFall); }

PORT_JUMP(Func_1dd78)
{
	xor_a(gb);
	wr8(gb, wGrabState, 0);
	LJUMP(StartJump_FromInput);
}

PORT_JUMP(UpdateState_SwimKnockBack)
{
	FARCALL(CheckFrontCollision);
	if (!b_and(gb)) {
		gb->cpu.a = inc_counter(gb);
		alu_cp(gb, gb->cpu.a, 0x04);
		bool move = true;
		if (CF) {
			gb->cpu.b = 0x02;
		} else {
			alu_cp(gb, gb->cpu.a, 0x08);
			if (CF)
				gb->cpu.b = 0x01;
			else
				move = false;
		}
		if (move) {
			move_by_dir(gb);
			RET();
			return;
		}
	}
	flip_dir(gb);
	LJUMP(SetState_Submerged);
}

// xor a / ld [wIsIntangible], a / ld a, $10 / ld [wInvincibleCounter], a / jp
static void water_stung_end(GB *gb, uint16_t to)
{
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	st(gb, wInvincibleCounter, 0x10);
	JUMP_TO(to);
}

PORT_JUMP(UpdateState_WaterStung)
{
	anim(gb, OAM1);
	SET_HL(gb, wWarioStateCounter);
	wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
	FARCALL(CheckUpCollision);
	if (b_and(gb)) {
		ld_cp(gb, wWarioStateCounter, 0x78);
		if (CF) {
			RET();
			return;
		}
		st(gb, wc0e1, PAD_UP);
		water_stung_end(gb, ADDR_SetState_Submerged);
		return;
	}
	FARCALL(Func_19acd);
	if (!ld_and(gb, wWaterInteraction)) {
		ld_cp(gb, wWarioStateCounter, 0x78);
		if (CF) {
			RET();
			return;
		}
		water_stung_end(gb, ADDR_SurfaceFromUnderwater);
		return;
	}
	ld_cp(gb, wWarioStateCounter, 0x78);
	if (!CF) {
		water_stung_end(gb, ADDR_SetState_Submerged);
		return;
	}
	if (ld_mask(gb, wGlobalCounter, 1)) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	CALL(MoveWarioUp);
	if (!ld_and(gb, wWaterCurrent)) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wWaterCurrent, 0);
	water_stung_end(gb, ADDR_SetState_Submerged);
}

// ld hl, wCurWaterCurrent / ld a, [hld] / or [hl] / and mask; true if nz
static bool water_current_mask(GB *gb, uint8_t mask)
{
	SET_HL(gb, wCurWaterCurrent - 1);
	gb->cpu.a = alu_or(gb, rd8(gb, wCurWaterCurrent), rd8(gb, wCurWaterCurrent - 1));
	gb->cpu.a = alu_and(gb, gb->cpu.a, mask);
	return gb->cpu.a != 0;
}

PORT(Func_1f310)
{
	FARCALL(CheckFrontCollision);
	if (b_and(gb)) {
		st(gb, wc0e1, facing_right(gb) ? PAD_RIGHT : PAD_LEFT);
		return;
	}
	bool right = facing_right(gb);
	if (water_current_mask(gb, CURRENT_RIGHT | CURRENT_LEFT))
		return;
	gb->cpu.b = 0x01;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
}

PORT_JUMP(Func_1f3d9)
{
	FARCALL(CheckUpCollision);
	if (b_and(gb)) {
		RET();
		return;
	}
	FARCALL(Func_19acd);
	if (!ld_and(gb, wWaterInteraction)) {
		LJUMP(SurfaceFromUnderwater);
		return;
	}
	if (water_current_mask(gb, CURRENT_UP | CURRENT_DOWN)) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	CALL(MoveWarioUp);
	RET();
}

// ApplyWaterCurrentMovement.ChangeCurrentDirection
static void change_current_direction(GB *gb)
{
	if (ld_and(gb, wLastWaterCurrent))
		return;
	gb->cpu.a = alu_add(gb, rd8(gb, wCurWaterCurrent), 15 << 4);
	wr8(gb, wLastWaterCurrent, gb->cpu.a);
}

// one direction of ApplyWaterCurrentMovement
static void water_current(GB *gb, int bit, uint8_t others, int with, int against, uint8_t bank,
	uint16_t coll, uint16_t move)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurWaterCurrent), others);
	if (gb->cpu.a)
		change_current_direction(gb);
	SET_HL(gb, wWaterCurrent);
	gb->cpu.a = rd8(gb, wWaterCurrent);
	wr8(gb, wCurWaterCurrent, gb->cpu.a);
	wr8(gb, wWaterCurrent, (uint8_t)(rd8(gb, wWaterCurrent) & ~(1 << bit)));
	water_current_push_b(gb, with, against, bank, coll, move, 2);
}

PORT(ApplyWaterCurrentMovement)
{
	LCALL(ApplyLastWaterCurrentMovement);
	gb->cpu.a = rd8(gb, wWaterCurrent);
	if (alu_bit(gb, gb->cpu.a, CURRENT_RIGHT_F))
		water_current(gb, CURRENT_RIGHT_F, CURRENT_LEFT | CURRENT_UP | CURRENT_DOWN, B_PAD_RIGHT, B_PAD_LEFT,
			BANK_Func_19741, ADDR_Func_19741, ADDR_MoveWarioRight);
	else if (alu_bit(gb, gb->cpu.a, CURRENT_LEFT_F))
		water_current(gb, CURRENT_LEFT_F, CURRENT_RIGHT | CURRENT_UP | CURRENT_DOWN, B_PAD_LEFT, B_PAD_RIGHT,
			BANK_Func_197b1, ADDR_Func_197b1, ADDR_MoveWarioLeft);
	else if (alu_bit(gb, gb->cpu.a, CURRENT_UP_F))
		water_current(gb, CURRENT_UP_F, CURRENT_RIGHT | CURRENT_LEFT | CURRENT_DOWN, B_PAD_UP, B_PAD_DOWN,
			BANK_CheckUpCollision, ADDR_CheckUpCollision, ADDR_MoveWarioUp);
	else if (alu_bit(gb, gb->cpu.a, CURRENT_DOWN_F))
		water_current(gb, CURRENT_DOWN_F, CURRENT_RIGHT | CURRENT_LEFT | CURRENT_UP, B_PAD_DOWN, B_PAD_UP,
			BANK_CheckCentreCollision, ADDR_CheckCentreCollision, ADDR_MoveWarioDown);
	else {
		if (ld_and(gb, wCurWaterCurrent))
			change_current_direction(gb);
		xor_a(gb);
		wr8(gb, wCurWaterCurrent, 0);
	}
}

PORT(HandleAttackingAirborneMovement)
{
	CALL(ApplyJumpVelocity);
	FARCALL(HandleWalk);
	ld_cp(gb, wWalkVelIndex, 0x14);
	if (!CF)
		st(gb, wWalkVelIndex, 0x10);
}

// ld a, [wWarioState] / cp state: true if still in that state
static bool still_in(GB *gb, uint8_t state)
{
	ld_cp(gb, wWarioState, state);
	return ZF;
}

// update_pos
static void update_pos(GB *gb)
{
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hPos + i);
		wr8(gb, wWarioPos + i, gb->cpu.a);
	}
	SET_HL(gb, hPos + 3);
	SET_DE(gb, wWarioPos + 3);
}

// farcall Func_19b25 / room transition check; true if it jumped
static bool room_transition_check(GB *gb)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return true;
	}
	return false;
}

// get_pos / ld b, PARTICLE_DUST / farcall CreateParticle
static void dust(GB *gb)
{
	get_pos(gb);
	gb->cpu.b = PARTICLE_DUST;
	FARCALL(CreateParticle);
}

PORT_JUMP(UpdateState_Walking)
{
	sfx_loop(gb, 36, SFX_WALK);
	anim(gb, OAM1);
	if (anim_ended(gb))
		dust(gb);
	LCALL(HandleInput_Walking);
	if (!still_in(gb, WST_WALKING)) {
		RET();
		return;
	}
	if (room_transition_check(gb))
		return;
	if (!still_in(gb, WST_WALKING)) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!still_in(gb, WST_WALKING)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(StartFall);
		return;
	}
	update_pos_y(gb);
	RET();
}

// the framesets of UpdateState_Airborne and UpdateState_GrabAirborne
typedef struct {
	uint16_t left, right; // not smash attacking
	uint16_t smash_left, smash_right, super_smash_left, super_smash_right;
} AirborneFramesets;

// the smash attack checks after HandleInput_Airborne, up to the
// UpdateWarioAnimation call
static void airborne_smash_attack(GB *gb, const AirborneFramesets *f)
{
	enum { CONTINUE, SET_SMASH, NO_SMASH } next = CONTINUE;
	ld_cp(gb, wPowerUpLevel, LEAD_OVERALLS);
	if (!CF) {
		if (ld_and(gb, wIsSmashAttacking)) {
			// .check_still_smash_attacking
			if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
				xor_a(gb);
				wr8(gb, wIsSmashAttacking, FALSE);
				next = NO_SMASH;
			}
		} else {
			ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			if (!CF && ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
				st(gb, wIsSmashAttacking, TRUE);
				next = SET_SMASH;
			}
		}
	}
	if (next == CONTINUE) {
		if (ld_and(gb, wIsTurning))
			next = ld_and(gb, wIsSmashAttacking) ? SET_SMASH : NO_SMASH;
	}
	if (next == SET_SMASH) {
		reset_anim(gb);
		bool right = facing_right(gb);
		ld_cp(gb, wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (right)
			set_frameset(gb, CF ? f->smash_right : f->super_smash_right);
		else
			set_frameset(gb, CF ? f->smash_left : f->super_smash_left);
	} else if (next == NO_SMASH) {
		reset_anim(gb);
		frameset_by_dir(gb, f->left, f->right);
	}
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_Airborne)
{
	if (room_transition_check(gb))
		return;
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (!CF) {
		// falling
		if (ld_and(gb, wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		if (ld_and(gb, wIsInSand)) {
			FARCALL(SetState_SandFalling);
			RET();
			return;
		}
		if (ld_and(gb, wLadderInteraction) && ld_bit(gb, wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_LadderClimbing);
			RET();
			return;
		}
		if (!still_in(gb, WST_AIRBORNE)) {
			RET();
			return;
		}
		FARCALL(Func_19afb);
		if (ld_and(gb, wIsNearFence) && ld_bit(gb, wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_FenceMovingVertical);
			RET();
			return;
		}
	}
	// .handle_input
	if (!still_in(gb, WST_AIRBORNE)) {
		RET();
		return;
	}
	LCALL(HandleInput_Airborne);
	if (!still_in(gb, WST_AIRBORNE)) {
		RET();
		return;
	}

	static const AirborneFramesets f = {ADDR_Frameset_15f94, ADDR_Frameset_15f97, ADDR_Frameset_15f8e,
		ADDR_Frameset_15f91, ADDR_Frameset_1604e, ADDR_Frameset_16055};
	airborne_smash_attack(gb, &f);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb) || !still_in(gb, WST_AIRBORNE)) {
			RET();
			return;
		}
		if (ld_and(gb, wJumpCollision)) {
			LJUMP(StartFall_SkipResetJumpUpwards);
			return;
		}
		center_on_ladder(gb);
		RET();
		return;
	}
	// .falling
	FARCALL(CheckAirborneCollision);
	if (!still_in(gb, WST_AIRBORNE)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	update_pos_y(gb);
	dust(gb);
	ld_cp(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
	if (CF) {
		LJUMP(DoSoftLand);
		return;
	}
	if (!ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(DoHardLand);
		return;
	}
	LJUMP(DoSoftLand); // fallthrough
}

PORT_JUMP(UpdateState_CrouchWalking)
{
	sfx_loop(gb, 21, SFX_CROUCH_WALK);
	anim(gb, OAM1);
	if (room_transition_check(gb))
		return;
	if (!still_in(gb, WST_CROUCH_WALKING)) {
		RET();
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return;
	}
	LCALL(Func_1ec6c);
	if (!still_in(gb, WST_CROUCH_WALKING)) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(StartCrouchFall);
		return;
	}
	update_pos_y(gb);
	if (ld_and(gb, wWarioSlopeInteraction)) {
		LJUMP(SetState_Sliding);
		return;
	}
	RET();
}

PORT_JUMP(UpdateState_CrouchAirborne)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return;
	}
	if (ld_and(gb, wIsInSand)) {
		FARCALL(SetState_SandFalling);
		RET();
		return;
	}
	if (!still_in(gb, WST_CROUCH_AIRBORNE)) {
		RET();
		return;
	}
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (!still_in(gb, WST_CROUCH_AIRBORNE)) {
			RET();
			return;
		}
		if (!b_and(gb)) {
			LJUMP(StartJump);
			return;
		}
	}
	// .asm_1d583
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	LCALL(HandleInput_Airborne);
	if (!still_in(gb, WST_CROUCH_AIRBORNE)) {
		RET();
		return;
	}
	if (ld_and(gb, wIsTurning)) {
		reset_anim(gb);
		frameset_by_dir(gb, ADDR_Frameset_14a38, ADDR_Frameset_14a3b);
		anim(gb, OAM1);
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		// rising
		FARCALL(CheckUpCollision);
		if (b_and(gb) && still_in(gb, WST_CROUCH_AIRBORNE))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	// .falling
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		if (!still_in(gb, WST_CROUCH_AIRBORNE)) {
			RET();
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	update_pos_y(gb);
	if (!still_in(gb, WST_CROUCH_AIRBORNE)) {
		RET();
		return;
	}
	LCALL(DoSoftLand);
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		RET();
		return;
	}
	LJUMP(CrouchOrSlide);
}

// ld a, b / and $0f: true if nz
static bool b_low_nibble(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
	return gb->cpu.a != 0;
}

PORT_JUMP(UpdateState_Sliding)
{
	sfx_loop(gb, 12, SFX_ZIP_LINE_SLIDE);
	anim(gb, OAM1);
	LCALL(Func_1f825);
	if (!still_in(gb, WST_SLIDING)) {
		RET();
		return;
	}
	FARCALL(Func_198c7);
	if (!b_and(gb)) {
		LJUMP(Func_1e174);
		return;
	}
	update_pos(gb);
	FARCALL(CheckFrontCollision);
	if (b_low_nibble(gb)) {
		LJUMP(DoBumpWithSmallBackOffset);
		return;
	}
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_Rolling); // fallthrough
}

// move by direction with the walk velocity, then cap it at $18 (to $14)
static void roll_move(GB *gb)
{
	if (facing_right(gb)) {
		CALL(ApplyWalkVelocity_Right);
		CALL(MoveWarioRight);
	} else {
		CALL(ApplyWalkVelocity_Left);
		CALL(MoveWarioLeft);
	}
	ld_cp(gb, wWalkVelIndex, 0x18);
	if (!CF)
		st(gb, wWalkVelIndex, 0x14);
}

// a = wWarioSlopeInteraction: and $0f / dec a / ld b, a / ld a, [wDirection] /
// xor b; true if nz (rolling against the slope)
static bool against_slope(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), gb->cpu.b);
	return gb->cpu.a != 0;
}

PORT_JUMP(UpdateState_Rolling)
{
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 12, SFX_ROLL);
	anim(gb, OAM1);
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		// .asm_1e145
		FARCALL(CheckCentreCollision);
		update_pos_y(gb);
		if (ld_and(gb, wWarioSlopeInteraction) && against_slope(gb)) {
			LJUMP(DoBumpWithSmallBackOffset);
			return;
		}
		LJUMP(Func_1e1e3);
		return;
	}
	roll_move(gb);
	FARCALL(CheckFrontCollision);
	if (b_low_nibble(gb)) {
		LJUMP(DoBumpWithBigBackOffset);
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(Func_1e174);
		return;
	}
	update_pos_y(gb);
	if (ld_and(gb, wWarioSlopeInteraction) && against_slope(gb)) {
		LJUMP(DoBumpWithSmallBackOffset);
		return;
	}
	RET();
}

PORT_JUMP(UpdateState_RollingAirborne)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return;
	}
	if (ld_and(gb, wIsInSand)) {
		FARCALL(SetState_SandFalling);
		RET();
		return;
	}
	anim(gb, OAM1);
	roll_move(gb);
	FARCALL(CheckFrontCollision);
	if (b_low_nibble(gb)) {
		LJUMP(DoBumpWithBigBackOffset);
		return;
	}
	gb->cpu.a = rd8(gb, wJumpVelIndex);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_HighJump, REG_DE(gb));
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), 7)) {
		// rising
		gb->cpu.a = alu_inc(gb, alu_cpl(gb, rd8(gb, hl)));
		gb->cpu.b = gb->cpu.a;
		CALL(MoveWarioUp);
		SET_HL(gb, wJumpVelIndex);
		wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
	} else {
		gb->cpu.b = rd8(gb, hl);
		CALL(MoveWarioDown);
		SET_HL(gb, wJumpVelIndex);
		gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelIndex));
		wr8(gb, wJumpVelIndex, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX);
		if (!CF)
			wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (b_and(gb))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	LJUMP(SetState_Rolling);
}

PORT_JUMP(UpdateState_PickingUp)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(Func_1dd78);
		return;
	}
	if (!ld_and(gb, wGrabState)) {
		LJUMP(SetState_Idling);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (ZF) {
		LJUMP(SetState_GrabIdling);
		return;
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_GrabIdling);
}

PORT_JUMP(UpdateState_ThrowCharging)
{
	if (!ld_and(gb, wGrabState)) {
		LJUMP(SetState_Idling);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(Func_1de3f);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(Func_1dd78);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		LJUMP(CrouchOrSlide);
		return;
	}
	gb->cpu.b = ld_bit(gb, wGrabState, GRAB_HEAVY_F) ? THROW_CHARGE_FRAMES_HEAVY : THROW_CHARGE_FRAMES_LIGHT;
	gb->cpu.a = alu_inc(gb, rd8(gb, wWarioStateCounter));
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (CF) {
		RET();
		return;
	}
	static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame,
		wWarioStateCounter, wWarioStateCycles, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_THROW_FULLY_CHARGED);
	gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK);
	gb->cpu.a = alu_add(gb, gb->cpu.a, GRAB_THROW_FULLY_CHARGED);
	wr8(gb, wGrabState, gb->cpu.a);
	frameset_by_dir(gb, ADDR_Frameset_16432, ADDR_Frameset_16435);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(UpdateState_ThrowFullyCharged)
{
	sfx_loop(gb, 30, SFX_FULLY_CHARGED_THROW);
	if (!ld_and(gb, wGrabState)) {
		LJUMP(SetState_Idling);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, GRAB_STATE_MASK);
	alu_cp(gb, gb->cpu.a, GRAB_HOLD_FULL_CHARGE);
	if (!ZF && !ld_and(gb, wWarioStateCounter) && anim_ended(gb)) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK);
		gb->cpu.a = alu_add(gb, gb->cpu.a, GRAB_HOLD_FULL_CHARGE);
		wr8(gb, wGrabState, gb->cpu.a);
		reset_anim(gb);
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		wr8(gb, wWarioStateCounter, gb->cpu.a);
		frameset_by_dir(gb, ADDR_Frameset_16438, ADDR_Frameset_16441);
	}
	anim(gb, OAM1);
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
			LJUMP(Func_1dd78);
			return;
		}
		if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
			LJUMP(CrouchOrSlide);
			return;
		}
		RET();
		return;
	}
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_16403, ADDR_Frameset_16423);
	anim(gb, OAM1);
	gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_FLAGS_MASK);
	gb->cpu.a = alu_add(gb, gb->cpu.a, GRAB_THROW | (1 << GRAB_FULL_CHARGE_F));
	wr8(gb, wGrabState, gb->cpu.a);
	LJUMP(SetState_Throwing);
}

PORT_JUMP(UpdateState_GrabSmashAttacking)
{
	if (!ld_and(gb, wGrabState)) {
		LJUMP(SetState_Idling);
		return;
	}
	anim(gb, OAM1);
	alu_cp(gb, inc_counter(gb), 0x1b);
	if (CF) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wIsSmashAttacking, FALSE);
	LJUMP(SetState_GrabIdling);
}

PORT_JUMP(UpdateState_GroundShakeStunned)
{
	anim(gb, OAM1);
	if (!ld_and(gb, wGroundShakeCounter)) {
		// .no_more_ground_shake
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (b_and(gb)) {
			LJUMP(CrouchOrSlide);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and(gb)) {
			LJUMP(StartFall);
			return;
		}
		LJUMP(SetState_Idling);
		return;
	}
	if (!ld_and(gb, wJumpVelTable)) {
		RET();
		return;
	}
	CALL(ApplyJumpVelocity);
	FARCALL(HandleWalk);
	CALL(Func_1762);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (b_and(gb))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	xor_a(gb);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
	RET();
}

// UpdateState_CrouchSliding.end_slide
static void crouch_slide_end(GB *gb)
{
	if (!ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (!still_in(gb, WST_CROUCH_SLIDING)) {
			RET();
			return;
		}
		if (!b_and(gb)) {
			LJUMP(SetState_Idling);
			return;
		}
	}
	// .crouching
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	LJUMP(CrouchOrSlide);
}

PORT_JUMP(UpdateState_CrouchSliding)
{
	if (room_transition_check(gb))
		return;
	if (!still_in(gb, WST_CROUCH_SLIDING)) {
		RET();
		return;
	}
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		// .try_jump
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (!still_in(gb, WST_CROUCH_SLIDING)) {
			RET();
			return;
		}
		bool blocked = b_and(gb);
		st(gb, wCollisionBoxTop, (uint8_t)-15);
		if (blocked) {
			RET();
			return;
		}
		LJUMP(StartCrouchJump);
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!still_in(gb, WST_CROUCH_SLIDING)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(StartCrouchFall);
		return;
	}
	if (ld_and(gb, wWarioSlopeInteraction)) {
		update_pos_y(gb);
		LJUMP(SetState_Sliding);
		return;
	}
	FARCALL(CheckFrontCollision);
	if (!still_in(gb, WST_CROUCH_SLIDING)) {
		RET();
		return;
	}
	if (b_and(gb)) {
		crouch_slide_end(gb);
		return;
	}
	// if Wario has Garlic power-up, then slide further
	SET_HL(gb, wWarioStateCounter);
	wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
	ld_cp(gb, wPowerUpLevel, GARLIC);
	bool garlic = !CF;
	gb->cpu.a = rd8(gb, wWarioStateCounter);
	uint8_t step;
	alu_cp(gb, gb->cpu.a, garlic ? 0x1e : 0x14);
	if (CF) {
		step = 2; // .slide_fast
	} else {
		alu_cp(gb, gb->cpu.a, garlic ? 0x36 : 0x24);
		if (CF) {
			step = 1; // .slide_slow
		} else {
			alu_cp(gb, gb->cpu.a, garlic ? 0x3c : 0x28);
			if (CF) {
				// .slide_slower
				gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1);
				if (ZF) {
					RET();
					return;
				}
				step = 1;
			} else if (ZF) {
				crouch_slide_end(gb);
				return;
			} else {
				RET();
				return;
			}
		}
	}
	gb->cpu.b = step;
	move_by_dir(gb);
	RET();
}

PORT_JUMP(UpdateState_GrabAirborne)
{
	if (room_transition_check(gb))
		return;
	if (!still_in(gb, WST_GRAB_AIRBORNE)) {
		RET();
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return;
	}
	if (!ld_and(gb, wGrabState)) {
		LJUMP(StartFall);
		return;
	}
	if (ld_and(gb, wIsInSand)) {
		FARCALL(SetState_SandFalling);
		RET();
		return;
	}
	if (ld_bit(gb, wJoypadPressed, B_PAD_B)) {
		LJUMP(SetState_ThrowingAirborne);
		return;
	}
	LCALL(HandleInput_Airborne);
	if (!still_in(gb, WST_GRAB_AIRBORNE)) {
		RET();
		return;
	}
	static const AirborneFramesets f = {ADDR_Frameset_15fa8, ADDR_Frameset_15fab, ADDR_Frameset_15fec,
		ADDR_Frameset_15fef, ADDR_Frameset_1605c, ADDR_Frameset_16063};
	airborne_smash_attack(gb, &f);

	if (ld_bit(gb, wGrabState, GRAB_HEAVY_F)) {
		ld_cp(gb, wWalkVelIndex, 0x08);
		if (!CF)
			st(gb, wWalkVelIndex, 0x04);
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (b_and(gb))
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!still_in(gb, WST_GRAB_AIRBORNE)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	if (!ld_and(gb, wFloorTransitionDir))
		play_sfx(gb, SFX_LAND);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wWalkVelIndex, 0);
	if (!ld_and(gb, wIsSmashAttacking)) {
		LJUMP(SetState_GrabIdling);
		return;
	}
	ld_cp(gb, wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
	if (CF) {
		// .asm_1dc25
		play_sfx(gb, SFX_GROUND_POUND);
		reset_anim(gb);
		frameset_by_dir(gb, ADDR_Frameset_16020, ADDR_Frameset_16037);
		anim(gb, OAM1);
	} else {
		// .continue
		if (!ld_and(gb, wGroundShakeCounter)) {
			st(gb, wGroundShakeCounter, 0x20);
			st(gb, wIsWarioGroundShaking, TRUE);
		}
		if (!ld_and(gb, wFloorTransitionDir))
			play_sfx(gb, SFX_GROUND_SHAKE);
		reset_anim(gb);
		frameset_by_dir(gb, ADDR_Frameset_15fd2, ADDR_Frameset_15fdf);
		anim(gb, OAM1);
	}
	// .asm_1dc5f
	st(gb, wIsSmashAttacking, 0x81);
	st(gb, wWarioState, WST_GRAB_SMASH_ATTACKING);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable, 0};
	clear(gb, zero);
	collision_box(gb, -1, -27, -9, 9);
	RET();
}

PORT_JUMP(Func_1f357)
{
	FARCALL(CheckCentreCollision);
	if (b_and(gb))
		update_pos_y(gb);
	if (still_in(gb, WST_WATER_STUNG)) {
		RET();
		return;
	}
	if (!ld_and(gb, wDirection)) {
		LJUMP(Func_1f310);
		return;
	}
	st(gb, wDirection, DIRECTION_LEFT);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_15576);
	anim(gb, OAM1);
	RET();
}

// ld a, [wSwimVelIndex] / ld e, a / ld d, $00 / ld hl, table / ld a, [wSwimmingDirectionInput] /
// and mask / jr z / ld hl, alt / add hl, de / ld b, [hl]
static void swim_vel(GB *gb, uint8_t mask, uint16_t alt)
{
	gb->cpu.a = rd8(gb, wSwimVelIndex);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = ADDR_SwimStraightVelTable;
	if (ld_mask(gb, wSwimmingDirectionInput, mask))
		hl = alt;
	hl = alu_add16(gb, hl, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.b = rd8(gb, hl);
}

// ld hl, wc0e1 / set bit, [hl]
static void set_c0e1(GB *gb, int bit)
{
	SET_HL(gb, wc0e1);
	wr8(gb, wc0e1, (uint8_t)(rd8(gb, wc0e1) | (1 << bit)));
}

// HandleUnderwaterThrustingInput.up
static void thrust_up(GB *gb)
{
	FARCALL(CheckUpCollision);
	if (b_and(gb)) {
		set_c0e1(gb, B_PAD_UP);
		LJUMP(StartDive);
		return;
	}
	FARCALL(Func_19acd);
	if (!ld_and(gb, wWaterInteraction)) {
		LJUMP(SurfaceFromUnderwater);
		return;
	}
	swim_vel(gb, PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894);
	CALL(MoveWarioUp);
	RET();
}

// HandleUnderwaterThrustingInput.down
static void thrust_down(GB *gb)
{
	FARCALL(CheckCentreCollision);
	if (b_and(gb)) {
		update_pos_y(gb);
		set_c0e1(gb, B_PAD_DOWN);
		RET();
		return;
	}
	swim_vel(gb, PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894);
	CALL(MoveWarioDown);
	RET();
}

// HandleUnderwaterThrustingInput.right / .left
static void thrust_horizontal(GB *gb, bool right)
{
	FARCALL(CheckCentreCollision);
	if (b_and(gb))
		update_pos_y(gb);
	if (still_in(gb, WST_WATER_STUNG)) {
		RET();
		return;
	}
	if (right)
		FARCALL(Func_19741);
	else
		FARCALL(Func_197b1);
	if (b_and(gb)) {
		LJUMP(SetState_SwimKnockBack_SwitchDirection);
		return;
	}
	swim_vel(gb, PAD_UP | PAD_DOWN, ADDR_SwimDiagonalVelTable);
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	gb->cpu.a = rd8(gb, wSwimmingDirectionInput);
	if (alu_bit(gb, gb->cpu.a, B_PAD_UP)) {
		thrust_up(gb);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		thrust_down(gb);
		return;
	}
	// .handle_diagonal_movement
	if (!ld_mask(gb, wJoypadDown, PAD_UP | PAD_DOWN)) {
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		// .diagonal_down
		FARCALL(CheckCentreCollision);
		if (!b_and(gb)) {
			gb->cpu.b = 1;
			CALL(MoveWarioDown);
			RET();
			return;
		}
		update_pos_y(gb);
		set_c0e1(gb, B_PAD_DOWN);
		RET();
		return;
	}
	// diagonal up
	FARCALL(CheckUpCollision);
	if (b_and(gb)) {
		set_c0e1(gb, B_PAD_UP);
		RET();
		return;
	}
	FARCALL(Func_19acd);
	if (!ld_and(gb, wWaterInteraction)) {
		LJUMP(SurfaceFromUnderwater);
		return;
	}
	gb->cpu.b = 1;
	CALL(MoveWarioUp);
	RET();
}

PORT_JUMP(HandleUnderwaterThrustingInput)
{
	SET_HL(gb, wSwimVelIndex);
	gb->cpu.a = alu_inc(gb, rd8(gb, wSwimVelIndex));
	wr8(gb, wSwimVelIndex, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 40);
	if (!CF) {
		xor_a(gb);
		wr8(gb, wSwimVelIndex, 0);
		LJUMP(SetState_Submerged);
		return;
	}
	if (!ld_mask(gb, wJoypadDown, PAD_A | PAD_B)) {
		LJUMP(SetState_Submerged);
		return;
	}
	gb->cpu.a = rd8(gb, wSwimmingDirectionInput);
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		thrust_horizontal(gb, true);
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		thrust_horizontal(gb, false);
	else if (alu_bit(gb, gb->cpu.a, B_PAD_UP))
		thrust_up(gb);
	else if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN))
		thrust_down(gb);
	else
		RET();
}

PORT_JUMP(UpdateState_AttackingAirborne)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(DiveFromSurface);
		return;
	}
	if (ld_and(gb, wIsInSand)) {
		FARCALL(SetState_SandFalling);
		RET();
		return;
	}
	if (!still_in(gb, WST_ATTACKING_AIRBORNE)) {
		RET();
		return;
	}
	sfx_loop(gb, 6, SFX_ATTACK);
	anim(gb, OAM1);
	LCALL(HandleAttackingAirborneMovement);
	if (!still_in(gb, WST_ATTACKING_AIRBORNE)) {
		RET();
		return;
	}
	FARCALL(CheckFrontCollision);
	if (!still_in(gb, WST_ATTACKING_AIRBORNE)) {
		RET();
		return;
	}
	if (b_low_nibble(gb)) {
		LJUMP(DoBumpWithSmallBackOffset);
		return;
	}
	gb->cpu.a = alu_dec(gb, rd8(gb, wAttackCounter));
	if (!ZF)
		wr8(gb, wAttackCounter, gb->cpu.a);
	bool stop;
	if (facing_right(gb))
		stop = ld_bit(gb, wJoypadPressed, B_PAD_LEFT);
	else
		stop = ld_bit(gb, wJoypadPressed, B_PAD_RIGHT);
	if (stop) {
		xor_a(gb);
		wr8(gb, wAttackCounter, 0);
		LJUMP(StartFall);
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb) || !still_in(gb, WST_ATTACKING_AIRBORNE)) {
			RET();
			return;
		}
		ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF)
			st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	if (!still_in(gb, WST_ATTACKING_AIRBORNE)) {
		RET();
		return;
	}
	LJUMP(SetState_Idling);
}

PORT_JUMP(UpdateState_WaterSurfaceIdling)
{
	xor_a(gb);
	wr8(gb, wc0e1, 0);
	FARCALL(Func_19b25);
	if (!still_in(gb, WST_WATER_SURFACE_IDLING)) {
		RET();
		return;
	}
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	sfx_loop(gb, 28, SFX_WATER_SURFACE);
	anim(gb, OAM1);
	LCALL(HandleWaterSurfaceFloatingMovement);
	LCALL(HandleWaterSurfaceInput);
	if (!still_in(gb, WST_WATER_SURFACE_IDLING)) {
		RET();
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_WaterSurfaceMoving);
		return;
	}
	LCALL(ApplyWaterCurrentMovement);
	RET();
}

// UpdateState_WaterSurfaceMoving.HandleDirectionalInput
static void water_surface_directional_input(GB *gb)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	bool right;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		right = true;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		right = false;
	} else {
		LCALL(SetState_WaterSurfaceIdling); // jp from a called routine
		return;
	}
	if (facing_right(gb) != right) {
		st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		reset_anim(gb);
		set_frameset(gb, right ? ADDR_Frameset_1555b : ADDR_Frameset_15554);
		anim(gb, OAM1);
		return;
	}
	FARCALL(CheckFrontCollision);
	if (b_and(gb))
		return;
	// current overrides player input
	if (water_current_mask(gb, CURRENT_RIGHT | CURRENT_LEFT))
		return;
	if (right) {
		CALL(ApplyWalkVelocity_Right);
		CALL(MoveWarioRight);
	} else {
		CALL(ApplyWalkVelocity_Left);
		CALL(MoveWarioLeft);
	}
	// .done_moving
	ld_cp(gb, wWalkVelIndex, 0x04);
	if (!CF)
		st(gb, wWalkVelIndex, 0x00);
}

PORT_JUMP(UpdateState_WaterSurfaceMoving)
{
	xor_a(gb);
	wr8(gb, wc0e1, 0);
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 28, SFX_WATER_SURFACE);
	anim(gb, OAM1);
	LCALL(HandleWaterSurfaceFloatingMovement);
	LCALL(ApplyWaterCurrentMovement);
	water_surface_directional_input(gb);
	LCALL(HandleWaterSurfaceInput);
	RET();
}

PORT_JUMP(UpdateState_UnderwaterThrusting)
{
	xor_a(gb);
	wr8(gb, wc0e1, 0);
	if (room_transition_check(gb))
		return;
	if (!still_in(gb, WST_UNDERWATER_THRUSTING)) {
		RET();
		return;
	}
	sfx_loop(gb, 15, SFX_00F);
	anim(gb, OAM1);
	LCALL(HandleUnderwaterThrustingInput);
	if (!still_in(gb, WST_UNDERWATER_THRUSTING)) {
		RET();
		return;
	}
	LCALL(ApplyWaterCurrentMovement);
	RET();
}

PORT(SetState_GrabAirborne)
{
	st(gb, wWarioState, WST_GRAB_AIRBORNE);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	frameset_by_dir(gb, ADDR_Frameset_15fa8, ADDR_Frameset_15fab);
	anim(gb, OAM1);
	if (!ld_and(gb, wIsOnSteppableObject))
		return;
	LCALL(UpdateState_GrabAirborne);
}

PORT_JUMP(Func_1f24c)
{
	sfx_loop(gb, 37, SFX_00E);
	if (ld_and(gb, wWarioStateCycles)) {
		static const uint16_t zero[] = {wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(gb, zero);
		frameset_by_dir(gb, ADDR_Frameset_15576, ADDR_Frameset_1558b);
	}
	anim(gb, OAM1);
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1);
	if (ZF) {
		gb->cpu.a = rd8(gb, wJoypadDown);
		if (alu_bit(gb, gb->cpu.a, B_PAD_UP)) {
			LJUMP(Func_1f3d9);
			return;
		}
		if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
			LJUMP(Func_1f3a7);
			return;
		}
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
			LJUMP(Func_1f357);
			return;
		}
		RET();
		return;
	}
	// .asm_1f2c0
	FARCALL(CheckCentreCollision);
	if (b_and(gb))
		update_pos_y(gb);
	if (still_in(gb, WST_WATER_STUNG)) {
		RET();
		return;
	}
	if (ld_and(gb, wDirection)) {
		LJUMP(Func_1f310);
		return;
	}
	st(gb, wDirection, DIRECTION_RIGHT);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1558b);
	anim(gb, OAM1);
	RET();
}
