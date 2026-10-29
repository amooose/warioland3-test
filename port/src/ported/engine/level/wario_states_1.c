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
