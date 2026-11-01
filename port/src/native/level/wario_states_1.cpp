// Wario's states, part 1 (src/engine/level/wario_states_1.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "../asm_module.hpp"

namespace wl3::level {

namespace {

class WarioStates1 : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_Func_1e6ea()
	{
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		set_frameset(facing_right() ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);
		anim(OAM1);
	}

	void port_SetState_Turning()
	{
		st(wWarioState, WST_TURNING);
		xor_a();
		wr8(wAttackCounter, 0);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		// switch direction
		cpu_.a = alu_.xor_(rd8(wDirection), 0x1);
		wr8(wDirection, cpu_.a);
		bool right = alu_.and_(cpu_.a, cpu_.a) != 0;
		set_frameset(right ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
		anim(OAM1);
	}

	void port_SetState_Attacking()
	{
		st(wAttackCounter, MAX_ATTACK_COUNTER);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		st(wWarioState, WST_ATTACKING);
		st(wCollisionBoxBottom, (uint8_t)-1);
		st(wCollisionBoxTop, (uint8_t)-27);
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);

		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wIsCrouching, 0);

		set_oam(BANK_OAM_14d1b, ADDR_OAM_14d1b);
		load_gfx(BANK_WarioAttackGfx, ADDR_WarioAttackGfx);

		// with the garlic power-up, Wario's attack is stronger
		ld_cp(wPowerUpLevel, GARLIC);
		if (CF)
			set_frameset(facing_right() ? ADDR_Frameset_151fd : ADDR_Frameset_151e4);
		else
			set_frameset(facing_right() ? ADDR_Frameset_1522d : ADDR_Frameset_1521c);
		anim(OAM1);
	}

	// ld a, [wIsOnSteppableObject] / and a / ret z / ld b, 2 / call MoveWarioUp
	void move_up_if_on_steppable_object()
	{
		if (!ld_and(wIsOnSteppableObject))
			return;
		cpu_.b = 2;
		CALL(MoveWarioUp);
	}

	void port_SetState_Walking()
	{
		st(wWarioState, WST_WALKING);
		uint8_t joypad = ld(wJoypadDown);
		if (alu_.bit(joypad, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else if (alu_.bit(joypad, B_PAD_LEFT))
			st(wDirection, DIRECTION_LEFT);
		static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelIndex, wJumpVelTable,
			wIsSmashAttacking, wWarioStateCounter, wWarioStateCycles, 0};
		clear(zero);
		port_Func_1e6ea(); // fallthrough
	}

	void port_SetState_Airborne()
	{
		xor_a();
		wr8(wWarioSlopeInteraction, 0);
		st(wWarioState, WST_AIRBORNE);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wIsCrouching, wAttackCounter, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		set_frameset(facing_right() ? ADDR_Frameset_15f97 : ADDR_Frameset_15f94);
		anim(OAM1);
		move_up_if_on_steppable_object();
	}

	void port_SetState_Diving()
	{
		st(wWarioState, WST_DIVING);
		st(wCollisionBoxBottom, (uint8_t)-1);
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);
		static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wWarioStateCounter,
			wWarioStateCycles, wIsSmashAttacking, wAttackCounter, wIsRolling, wSwimVelIndex,
			wIsCrouching, wc0e0, 0};
		clear(zero);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	}

	void port_SetState_Submerged()
	{
		st(wWarioState, WST_SUBMERGED);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wSwimVelIndex,
			wc0e0, wSwimmingDirectionInput, 0};
		clear(zero);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);

		bool can_swim = false;
		ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
		if (!CF) {
			ld_cp(wWaterInteraction, NON_SUBMERSIBLE_WATER);
			can_swim = !ZF;
		}
		if (can_swim)
			collision_box(-1, -15, -9, 9);
		// else .cannot_swim_underwater
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		bool right = facing_right();
		if (can_swim)
			set_frameset(right ? ADDR_Frameset_1558b : ADDR_Frameset_15576);
		else
			set_frameset(right ? ADDR_Frameset_155a3 : ADDR_Frameset_155a0);
		anim(OAM1);
	}

	void port_SetState_WaterSurfaceIdling()
	{
		st(wWarioState, WST_WATER_SURFACE_IDLING);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wSwimVelIndex, wc0e0, wSwimmingDirectionInput, 0};
		clear(zero);
		if (!ld_and(wWaterInteraction))
			st(wWaterInteraction, SUBMERSIBLE_WATER);
		set_frameset(facing_right() ? ADDR_Frameset_1555b : ADDR_Frameset_15554);
		anim(OAM1);
	}

	// hPos = wWarioPos + (dy, dx), the slide dust's position
	void get_pos_with_offset(int dy, int dx)
	{
		cpu_.a = dx < 0 ? alu_.sub(rd8(wWarioXPos + 1), (uint8_t)-dx)
			: alu_.add(rd8(wWarioXPos + 1), (uint8_t)dx);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = dx < 0 ? alu_.sbc(rd8(wWarioXPos), 0) : alu_.adc(rd8(wWarioXPos), 0);
		wr8(hXPosLo - 1, cpu_.a);
		cpu_.a = alu_.sub(rd8(wWarioYPos + 1), (uint8_t)-dy);
		wr8(hXPosLo - 2, cpu_.a);
		cpu_.a = alu_.sbc(rd8(wWarioYPos), 0);
		wr8(hXPosLo - 3, cpu_.a);
		SET_HL(gb_, wWarioYPos);
		SET_DE(gb_, hXPosLo - 3);
	}

	void port_SetState_Sliding()
	{
		st(wWarioState, WST_SLIDING);
		static const uint16_t zero[] = {wSFXLoopCounter, wIsCrouching, wFrameDuration,
			wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wWalkVelIndex, wGrabState, 0};
		clear(zero);
		st(wIsRolling, TRUE);
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_1644a, ADDR_OAM_1644a);
		if (!ld_bit(wWarioSlopeInteraction, LEFT_SLOPE_F)) {
			get_pos_with_offset(-0x0c, 0x0c);
			cpu_.b = PARTICLE_SLIDE_DUST_LEFT;
			FARCALL(CreateParticle);
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_16708);
		} else {
			get_pos_with_offset(-0x0c, -0x0c);
			cpu_.b = PARTICLE_SLIDE_DUST_RIGHT;
			FARCALL(CreateParticle);
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_1671d);
		}
		anim(OAM1);
	}

	void port_SetState_RollingAirborne()
	{
		xor_a();
		wr8(wWarioSlopeInteraction, 0);
		wr8(wGrabState, 0);
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		st(wWarioState, WST_ROLLING_AIRBORNE);
		st(wIsRolling, 0x02);
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_1644a, ADDR_OAM_1644a);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(facing_right() ? ADDR_Frameset_166f7 : ADDR_Frameset_166e6);
		anim(OAM1);
	}

	void port_SetState_CrouchWalking()
	{
		collision_box(-1, -15, -9, 9);
		st(wWarioState, WST_CROUCH_WALKING);
		st(wIsCrouching, TRUE);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles, wAttackCounter, wGrabState, 0};
		clear(zero);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		ld_cp(wDirection, DIRECTION_RIGHT);
		set_frameset(ZF ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
		anim(OAM1);
	}

	void port_SetState_CrouchAirborne()
	{
		st(wWarioState, WST_CROUCH_AIRBORNE);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, 0};
		clear(zero);
		collision_box(-1, -15, -9, 9);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		set_frameset(facing_right() ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
		move_up_if_on_steppable_object();
	}

	// flashes Wario's palette while he's invisible
	void port_Func_1f6c2()
	{
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x07);
		if (ZF) {
			SET_HL(gb_, ADDR_WarioInvisiblePal);
			CALL(SetWarioPal);
			return;
		}
		alu_.cp(cpu_.a, 3);
		if (!ZF)
			return;
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
	}

	void port_HandleWaterSurfaceFloatingMovement()
	{
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x03);
		if (!ZF)
			return;
		SET_HL(gb_, wWaterSurfaceFloatingCounter);
		cpu_.a = alu_.inc(rd8(wWaterSurfaceFloatingCounter));
		wr8(wWaterSurfaceFloatingCounter, cpu_.a);
		alu_.cp(cpu_.a, 7);
		if (CF) {
			// go down
			cpu_.b = 1;
			CALL(MoveWarioDown_WithoutCameraOffset);
			return;
		}
		alu_.cp(cpu_.a, 12);
		if (!CF) {
			xor_a();
			wr8(wWaterSurfaceFloatingCounter, 0);
		}
		cpu_.b = 1;
		CALL(MoveWarioUp_WithoutCameraOffset);
	}

	void port_StartJump()
	{
		ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
		cpu_.a = JUMP_VEL_NORMAL;
		if (!CF) {
			SET_HL(gb_, wJoypadDown);
			if (alu_.bit(rd8(wJoypadDown), B_PAD_UP))
				cpu_.a = JUMP_VEL_HIGH_JUMP;
		}
		wr8(wJumpVelTable, cpu_.a);
		port_SetState_Airborne(); // fallthrough
	}

	void port_SetState_Walking_ResetWalkVel()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		port_SetState_Walking(); // fallthrough
	}

	void port_CrouchOrSlide()
	{
		if (ld_and(wWarioSlopeInteraction)) {
			port_SetState_Sliding();
			return;
		}
		collision_box(-1, -15, -9, 9);
		st(wWarioState, WST_CROUCHING);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wWalkVelIndex, wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		st(wIsCrouching, TRUE);
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (!ZF) {
			port_SetState_CrouchWalking();
			return;
		}
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		ld_cp(wDirection, DIRECTION_RIGHT);
		set_frameset(ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
	}

	void port_StartCrouchFall()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_CrouchAirborne();
	}

	void port_StartCrouchJump()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		port_SetState_CrouchAirborne(); // fallthrough
	}

	void port_StartFall_SkipResetJumpUpwards()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_StartJump(); // jr
	}

	void port_StartJump_FromInput()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		st(wJumpingUpwards, TRUE);
		port_StartJump(); // fallthrough
	}

	void port_DiveFromSurface_SkipSplash()
	{
		st(wCollisionBoxTop, (uint8_t)-27);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(facing_right() ? ADDR_Frameset_155a3 : ADDR_Frameset_155a0);
		anim(OAM1);
		port_SetState_Diving(); // jr
	}

	void port_SurfaceFromUnderwater()
	{
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWaterSurfaceFloatingCounter, 0);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		port_SetState_WaterSurfaceIdling(); // fallthrough
	}

	void port_StartFall()
	{
		xor_a();
		wr8(wJumpingUpwards, FALSE);
		port_StartFall_SkipResetJumpUpwards(); // fallthrough
	}

	void port_StartJump_FromWaterSurface()
	{
		play_sfx(SFX_SPLASH);
		st(wJumpVelIndex, 0x01);
		st(wJumpingUpwards, TRUE);
		st(wDoFullJump, TRUE);
		port_StartJump(); // jr
	}

	void port_StartJump_FromSand()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		st(wJumpingUpwards, TRUE);
		st(wDoFullJump, TRUE);
		ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
		cpu_.a = JUMP_VEL_KNOCK_BACK;
		if (!CF) {
			SET_HL(gb_, wJoypadDown);
			if (alu_.bit(rd8(wJoypadDown), B_PAD_UP))
				cpu_.a = JUMP_VEL_NORMAL;
		}
		wr8(wJumpVelTable, cpu_.a);
		port_SetState_Airborne(); // jr
	}

	void port_DiveFromSurface()
	{
		xor_a();
		wr8(wGrabState, 0);
		get_pos();
		cpu_.a = alu_.and_(rd8(hYPosLo), 0xf0);
		wr8(hYPosLo, cpu_.a);
		cpu_.b = PARTICLE_SPLASH;
		FARCALL(CreateParticle);
		play_sfx(SFX_SPLASH);
		port_DiveFromSurface_SkipSplash(); // fallthrough
	}

	void port_SetState_TryingSubmerge()
	{
		st(wWarioState, WST_TRYING_SUBMERGE);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_155b0, ADDR_Frameset_155b7);
		anim(OAM1);
	}

	void port_UpdateState_TryingSubmerge()
	{
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			port_SetState_WaterSurfaceIdling();
			return;
		}
		anim(OAM1);
	}

	void port_Func_1e1e3()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		port_SetState_RollingAirborne(); // jr
	}

	void port_HandleInput_Idling()
	{
		uint8_t pressed = ld(wJoypadPressed);
		if (alu_.bit(pressed, B_PAD_B)) {
			port_SetState_Attacking();
			return;
		}
		if (alu_.bit(pressed, B_PAD_A)) {
			port_StartJump_FromInput();
			return;
		}
		uint8_t down = ld(wJoypadDown);
		bool right;
		if (alu_.bit(down, B_PAD_RIGHT))
			right = true;
		else if (alu_.bit(down, B_PAD_LEFT))
			right = false;
		else {
			return;
		}
		// walk if already facing that way, else turn
		if (ld_and(wDirection) == right)
			port_SetState_Walking_ResetWalkVel();
		else
			port_SetState_Turning();
	}

	void port_Func_1e174()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_RollingAirborne(); // fallthrough
	}

	void port_Func_1f863()
	{
		uint16_t hl = alu_.add16(ADDR_SwimStraightVelTable, REG_DE(gb_));
		SET_HL(gb_, hl);
		cpu_.b = rd8(hl);
		CALL(MoveWarioUp);
	}

	void port_SetState_Throwing()
	{
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		st(wWarioState, WST_THROWING);
	}

	void port_Func_1f3a7()
	{
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			update_pos_y();
			st(wc0e1, PAD_DOWN);
			return;
		}
		SET_HL(gb_, wCurWaterCurrent - 1);
		cpu_.a = alu_.or_(rd8(wCurWaterCurrent), rd8(wCurWaterCurrent - 1));
		cpu_.a = alu_.and_(cpu_.a, CURRENT_UP | CURRENT_DOWN);
		if (!ZF)
			return;
		cpu_.b = 0x01;
		CALL(MoveWarioDown);
	}

	void port_SetState_SmashAttacking()
	{
		st(wIsSmashAttacking, 0x81);
		st(wWarioState, WST_SMASH_ATTACKING);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelIndex,
			wJumpVelTable, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
	}

	void port_SetState_LadderShakeStunned()
	{
		st(wWarioState, WST_LADDER_SHAKE_SLIDING);
		play_sfx(SFX_SLIDE);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
			wIsGettingOffLadder, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		reset_anim();
		set_frameset(ADDR_Frameset_15948);
		anim(OAM1);
	}

	void port_StartDive()
	{
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15569, ADDR_Frameset_15573);
		anim(OAM1);
		port_SetState_Diving(); // fallthrough
	}

	// ld a, [wWarioState] / cp WST_IDLING: true if still idling
	bool still_idling()
	{
		ld_cp(wWarioState, WST_IDLING);
		return ZF;
	}

	void port_UpdateState_Idling()
	{
		FARCALL(CheckCentreCollision);
		if (!still_idling()) {
			return;
		}
		if (!b_and()) {
			port_StartFall();
			return;
		}
		update_pos_y();
		if (!still_idling()) {
			return;
		}

		if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
			FARCALL(Func_19b12);
			if (!still_idling()) {
				return;
			}
			ld_cp(wLadderInteraction, LADDER_UNDERNEATH);
			if (!ZF) {
				port_CrouchOrSlide();
				return;
			}
			FARCALL(SetState_LadderClimbing);
			return;
		}

		anim(OAM1);
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (ld_and(wLadderInteraction) && ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_LadderClimbing);
			return;
		}
		if (ld_and(wIsNearFence) && ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_FenceMovingVertical);
			return;
		}
		if (!still_idling()) {
			return;
		}
		port_HandleInput_Idling();
		if (!still_idling() || ld_and(wIsOnSteppableObject)) {
			return;
		}
		cpu_.a = alu_.add(rd8(wWarioStateCounter), 1);
		wr8(wWarioStateCounter, cpu_.a);
		cpu_.a = alu_.adc(rd8(wWarioStateCycles), 0);
		wr8(wWarioStateCycles, cpu_.a);
		alu_.cp(cpu_.a, NUM_SLEEPING_IDLE_CYCLES);
		if (!CF)
			FARCALL(SetState_Sleeping);
	}

	void port_SetState_ThrowingAirborne()
	{
		st(wWarioState, WST_THROWING_AIRBORNE);
		cpu_.a = alu_.or_(alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK), GRAB_THROW);
		wr8(wGrabState, cpu_.a);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_15fae, ADDR_Frameset_15fb3);
		anim(OAM1);
	}

	void port_SetState_WaterSurfaceMoving()
	{
		st(wWarioState, WST_WATER_SURFACE_MOVING);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wSwimVelIndex, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_15554, ADDR_Frameset_1555b);
		anim(OAM1);
	}

	void port_SetState_Rolling()
	{
		static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelIndex, wJumpVelTable, 0};
		clear(zero);
		st(wWarioState, WST_ROLLING);
		collision_box(-1, -14, -9, 9);
		st(wIsRolling, 0x02);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_166e6, ADDR_Frameset_166f7);
		anim(OAM1);
	}

	void port_SetState_SwimKnockBack()
	{
		play_sfx(SFX_BUMP);
		st(wWarioState, WST_SWIM_KNOCK_BACK);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wSwimVelIndex,
			wSwimmingDirectionInput, wc0e0, 0};
		clear(zero);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15573, ADDR_Frameset_15569);
		anim(OAM1);
	}

	void port_SetState_ThrowCharging()
	{
		play_sfx(SFX_02C);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
		st(wWarioState, WST_THROW_CHARGING);
		cpu_.a = alu_.add(alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK), GRAB_CHARGE_THROW);
		wr8(wGrabState, cpu_.a);
		load_gfx(BANK_WarioThrowGfx, ADDR_WarioThrowGfx);
		set_oam(BANK_OAM_1606a, ADDR_OAM_1606a);
		frameset_by_dir(ADDR_Frameset_1642c, ADDR_Frameset_1642f);
		anim(OAM1);
	}

	void port_SetState_CrouchSliding()
	{
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wAttackCounter, 0);
		play_sfx(SFX_SLIDE);
		st(wWarioState, WST_CROUCH_SLIDING);
		st(wIsCrouching, TRUE);
		collision_box(-1, -15, -9, 9);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		ld_cp(wDirection, DIRECTION_RIGHT);
		set_frameset(ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
	}

	void port_UpdateState_PickedUp()
	{
		bool wiggle;
		bool right = facing_right();
		if (!right)
			wiggle = ld_mask(wJoypadPressed, PAD_A | PAD_B | PAD_RIGHT);
		else
			wiggle = ld_mask(wJoypadPressed, PAD_A | PAD_B | PAD_LEFT);
		if (!wiggle) {
			anim(OAM1);
			SET_HL(gb_, wPickedUpFrameCounter);
			wr8(wPickedUpFrameCounter, alu_.dec(rd8(wPickedUpFrameCounter))); // dec [hl]
			if (!ZF) {
				return;
			}
			st(wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
			st(wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
			return;
		}
		st(wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
		if (!right) {
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_15f7f);
		} else {
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_15f70);
		}
		reset_anim();
		anim(OAM1);
		st(wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
		SET_HL(gb_, wPickedUpWiggleCounter);
		wr8(wPickedUpWiggleCounter, alu_.dec(rd8(wPickedUpWiggleCounter))); // dec [hl]
		if (!ZF) {
			return;
		}
		xor_a();
		wr8(wAutoMoveState, 0);
		port_StartFall();
	}

	void port_SetState_UnderwaterThrusting()
	{
		st(wWarioState, WST_UNDERWATER_THRUSTING);
		collision_box(-1, -15, -9, 9);
		static const uint16_t zero[] = {wSFXLoopCounter, wSwimVelIndex, wWarioStateCycles,
			wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		bool right;
		if (!ld_and(wSwimmingDirectionInput)) {
			ld_cp(wDirection, DIRECTION_RIGHT);
			right = ZF;
			st(wSwimmingDirectionInput, right ? PAD_RIGHT : PAD_LEFT);
		} else if (ld_bit(wSwimmingDirectionInput, B_PAD_LEFT)) {
			right = false;
		} else if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			right = true;
		} else {
			right = facing_right();
		}
		st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		ld_cp(wPowerUpLevel, PRINCE_FROGS_GLOVES);
		if (right)
			set_frameset(CF ? ADDR_Frameset_155c5 : ADDR_Frameset_1556c);
		else
			set_frameset(CF ? ADDR_Frameset_155be : ADDR_Frameset_15562);
		anim(OAM1);
	}

	void port_StartUpwardsUnderwaterThrusting()
	{
		st(wSwimmingDirectionInput, PAD_UP);
		port_SetState_UnderwaterThrusting();
	}

	void port_SetState_SwimKnockBack_SwitchDirection()
	{
		flip_dir();
		port_SetState_SwimKnockBack(); // fallthrough
	}

	void port_StartUnderwaterThrusting()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_CTRL_PAD);
		wr8(wSwimmingDirectionInput, cpu_.a);
		port_SetState_UnderwaterThrusting(); // fallthrough
	}

	void port_Func_1de3f()
	{
		reset_anim();
		frameset_by_dir(ADDR_Frameset_163fe, ADDR_Frameset_1641e);
		anim(OAM1);
		cpu_.a = alu_.add(alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK), GRAB_THROW);
		wr8(wGrabState, cpu_.a);
		port_SetState_Throwing(); // fallthrough
	}

	// airborne gfx and OAM, then a frameset by direction and the animation
	void airborne_gfx(uint16_t left, uint16_t right)
	{
		reset_anim();
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		wr8(wWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_15955 >> 8));
		st(wWarioOAMPtr + 1, (uint8_t)ADDR_OAM_15955);
		frameset_by_dir(left, right);
		anim(OAM1);
	}

	void port_DoGroundPound()
	{
		play_sfx(SFX_GROUND_POUND);
		airborne_gfx(ADDR_Frameset_15ff2, ADDR_Frameset_16009);
		port_SetState_SmashAttacking(); // fallthrough
	}

	void port_DoWarioGroundShake()
	{
		if (!ld_and(wGroundShakeCounter)) {
			st(wGroundShakeCounter, 32);
			st(wIsWarioGroundShaking, TRUE);
		}
		if (!ld_and(wFloorTransitionDir))
			play_sfx(SFX_GROUND_SHAKE);
		airborne_gfx(ADDR_Frameset_15fb8, ADDR_Frameset_15fc5);
		port_SetState_SmashAttacking();
	}

	void port_HandleGroundShake()
	{
		if (ld_and(wLevelEndScreen)) {
			return;
		}
		SET_HL(gb_, wGroundShakeCounter);
		cpu_.a = rd8(wGroundShakeCounter);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			xor_a();
			wr8(wIsWarioGroundShaking, 0);
			return;
		}
		bool cont = true;
		alu_.cp(cpu_.a, 0x10);
		if (!CF) {
			static const uint16_t checks[] = {wIsGettingOffLadder, wIsWarioGroundShaking,
				wJumpVelTable, wWaterInteraction, wTransformation, wRoomTransitionParam,
				wIsIntangible, wIsOnSteppableObject, 0};
			const uint16_t *p;
			for (p = checks; *p; p++)
				if (ld_and(*p))
					break;
			if (!*p) {
				ld_cp(wWarioState, WST_GROUND_SHAKE_STUNNED);
				if (!ZF)
					cont = false;
			}
		}
		if (cont) {
			cpu_.a = alu_.dec(rd8(wGroundShakeCounter));
			wr8(wGroundShakeCounter, cpu_.a);
			cpu_.a = alu_.and_(cpu_.a, 0x02);
			st(wSCYShake, cpu_.a ? 2 : 0);
			if (!cpu_.a) {
				// xor a
				cpu_.f = FLAG_Z;
			}
			return;
		}
		// .wario_shake_stun
		cpu_.a = rd8(wWarioState);
		alu_.cp(cpu_.a, WST_LADDER_CLIMBING);
		if (!ZF)
			alu_.cp(cpu_.a, WST_LADDER_IDLING);
		if (!ZF)
			alu_.cp(cpu_.a, WST_LADDER_SCRATCHING);
		if (ZF) {
			port_SetState_LadderShakeStunned();
			return;
		}
		st(wWarioState, WST_GROUND_SHAKE_STUNNED);
		static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wAttackCounter,
			wIsRolling, wGrabState, 0};
		clear(zero);
		st(wCollisionBoxTop, (uint8_t)-15);
		if (!ld_and(wIsCrouching)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			st(wJumpVelIndex, 10);
			st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		}
		st(wCollisionBoxBottom, (uint8_t)-1);
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);
		static const uint16_t zero2[] = {wWarioStateCounter, wWarioStateCycles,
			wFrameDuration, wAnimationFrame, 0};
		clear(zero2);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
	}

	void port_SetState_Idling()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_IDLING);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wJumpVelIndex,
			wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking,
			wIsCrouching, wAttackCounter, wGrabState, 0};
		clear(zero);
		coll_box_blrt();
		FARCALL(CheckUpCollision);
		ld_cp(wWarioState, WST_IDLING);
		if (!ZF) {
			return;
		}
		if (b_and()) {
			port_CrouchOrSlide();
			return;
		}
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	// farcall CheckFrontCollision / ld a, b / and $0f / ret nz
	bool front_blocked()
	{
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		return cpu_.a != 0;
	}

	void port_WalkIfPossible_Right()
	{
		if (front_blocked())
			return;
		CALL(ApplyWalkVelocity_Right);
		CALL(MoveWarioRight);
	}

	void port_WalkIfPossible_Left()
	{
		if (front_blocked())
			return;
		CALL(ApplyWalkVelocity_Left);
		CALL(MoveWarioLeft);
	}

	void port_Func_1ec19()
	{
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		ld_cp(wWarioState, WST_CROUCHING);
		if (!ZF) {
			alu_.cp(cpu_.a, WST_CROUCH_WALKING);
			if (!ZF) {
				return;
			}
		}
		if (!b_and()) {
			port_StartCrouchJump();
			return;
		}
		xor_a();
		wr8(wJumpVelTable, 0);
		st(wCollisionBoxTop, (uint8_t)-15);
	}

	void port_Func_1ec4b()
	{
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (!b_and()) {
			port_SetState_Attacking();
			return;
		}
		st(wCollisionBoxTop, (uint8_t)-15);
	}

	// farcall Func_19b12 / ld a, [wWaterInteraction] / cp NON_SUBMERSIBLE_WATER
	bool non_submersible()
	{
		FARCALL(Func_19b12);
		ld_cp(wWaterInteraction, NON_SUBMERSIBLE_WATER);
		return ZF;
	}

	void port_HandleWaterSurfaceInput()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_StartJump_FromWaterSurface();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
			if (!CF) {
				if (ld_bit(wJoypadDown, B_PAD_UP)) {
					return;
				}
				if (non_submersible()) {
					port_SetState_TryingSubmerge();
					return;
				}
				cpu_.b = 3;
				CALL(MoveWarioDown);
				port_StartUnderwaterThrusting();
				return;
			}
		}
		// .no_thrust
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			return;
		}
		ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
		if (CF || non_submersible()) {
			port_SetState_TryingSubmerge();
			return;
		}
		port_SetState_Submerged();
	}

	// farcall f / ld a, b / and a; true if nz
	bool farcall_b(uint8_t bank, uint16_t addr)
	{
		port_farcall(gb_, bank, addr);
		return b_and();
	}

	void port_HandleBumpMovement()
	{
		ld_cp(wJumpVelIndex, 28);
		if (!CF) {
			cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
			if (cpu_.a) {
				if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
					st(wDirection, DIRECTION_RIGHT);
					if (!farcall_b(BANK_Func_197b1, ADDR_Func_197b1)) {
						cpu_.b = 1;
						CALL(MoveWarioLeft);
					}
				} else {
					st(wDirection, DIRECTION_LEFT);
					if (!farcall_b(BANK_Func_19741, ADDR_Func_19741)) {
						cpu_.b = 1;
						CALL(MoveWarioRight);
					}
				}
				xor_a();
				wr8(wWalkVelIndex, 0);
				port_StartFall();
				return;
			}
		}
		// .no_control
		ld_cp(wDirection, DIRECTION_RIGHT);
		if (ZF) {
			cpu_.b = 1;
			CALL(MoveWarioRight);
			if (farcall_b(BANK_Func_19741, ADDR_Func_19741)) {
				cpu_.b = 1;
				CALL(MoveWarioLeft);
			}
		} else {
			cpu_.b = 1;
			CALL(MoveWarioLeft);
			if (farcall_b(BANK_Func_197b1, ADDR_Func_197b1)) {
				cpu_.b = 1;
				CALL(MoveWarioRight);
			}
		}
		// .handle_vertical
		cpu_.a = rd8(wJumpVelIndex);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(ADDR_JumpVelTable_Normal, REG_DE(gb_));
		SET_HL(gb_, hl);
		uint8_t v = rd8(hl);
		if (alu_.bit(v, 7)) {
			cpu_.a = rd8(hl);
			cpu_.a = alu_.cpl(cpu_.a);
			cpu_.a = alu_.inc(cpu_.a);
			cpu_.b = cpu_.a;
			CALL(MoveWarioUp);
			SET_HL(gb_, wJumpVelIndex);
			wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
		} else {
			cpu_.b = v;
			CALL(MoveWarioDown);
			SET_HL(gb_, wJumpVelIndex);
			cpu_.a = alu_.inc(rd8(wJumpVelIndex));
			wr8(wJumpVelIndex, cpu_.a);
			alu_.cp(cpu_.a, MAX_JUMP_VEL_INDEX);
			if (!CF)
				wr8(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
		}
	}

	// call .ret_label: a call to a bare ret, which only leaves the return
	// address below sp
	void call_to_ret(uint16_t ret)
	{
		gb_push16(gb_, ret);
		gb_pop16(gb_);
	}

	void port_SetState_Bumping()
	{
		st(wWarioState, WST_BUMPING);
		flip_dir();
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wAttackCounter, 0};
		clear(zero);
		if (!ld_and(wIsRolling)) {
			frameset_by_dir(ADDR_Frameset_15219, ADDR_Frameset_15216);
			anim(OAM1);
			return;
		}
		xor_a();
		wr8(wIsRolling, 0);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		if (!facing_right()) {
			call_to_ret(ADDR_SetState_Bumping + 0xa7); // 4aed
			set_frameset(ADDR_Frameset_15f7f);
		} else {
			call_to_ret(ADDR_SetState_Bumping + 0xb6); // 4afc
			set_frameset(ADDR_Frameset_15f70);
		}
		anim(OAM1);
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (b_and())
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	}

	void port_HandleDivingHorizontalMovement()
	{
		ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
		if (CF)
			return;
		cpu_.a = rd8(wJoypadDown);
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			right = true;
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			right = false;
		else
			return;
		uint8_t dir = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
		ld_cp(wDirection, dir);
		if (!ZF)
			st(wDirection, dir);
		if (farcall_b(BANK_CheckFrontCollision, ADDR_CheckFrontCollision))
			return;
		cpu_.b = 1;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	}

	void port_DoHardLand()
	{
		CALL(TriggerFloorTransition_SkipUpdateYPos);
		static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wFrameDuration,
			wAnimationFrame, wWalkVelIndex, 0};
		clear(zero);
		if (ld_and(wIsSmashAttacking)) {
			ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
			if (CF)
				port_DoGroundPound();
			else
				port_DoWarioGroundShake();
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_DOWN);
		if (cpu_.a) {
			port_CrouchOrSlide();
			return;
		}
		play_sfx(SFX_LAND);
		st(wWarioState, WST_LANDING);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f9a, ADDR_Frameset_15fa1);
		anim(OAM1);
	}

	void port_HandleInvincibility()
	{
		if (!ld_and(wInvincibleCounter))
			return;
		alu_.cp(cpu_.a, 0x16);
		if (!CF) {
			// .fast
			cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b1);
			if (cpu_.a)
				return;
			cpu_.a = alu_.inc(rd8(wInvincibleCounter));
			wr8(wInvincibleCounter, cpu_.a);
			alu_.cp(cpu_.a, 0x28);
			if (!CF) {
				xor_a();
				wr8(wInvincibleCounter, 0);
				wr8(wInvisibleFrame, 0);
				cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT | PAD_DOWN);
				if (cpu_.a)
					return;
				FARCALL(CheckUpCollision);
				return;
			}
		} else {
			alu_.cp(cpu_.a, 0x10);
			if (!CF) {
				// .slow
				cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b11);
				if (cpu_.a)
					return;
				cpu_.a = alu_.inc(rd8(wInvincibleCounter));
				wr8(wInvincibleCounter, cpu_.a);
			} else {
				ld_cp(wWarioState, WST_STUNG);
				if (ZF)
					return;
				alu_.cp(cpu_.a, WST_STUNG_RECOVERY);
				if (ZF)
					return;
				st(wInvincibleCounter, 0x10);
				ld_cp(wWarioPalsPtr + 0, (uint8_t)(ADDR_WarioInvisiblePal >> 8));
				if (!ZF)
					return;
				ld_cp(wWarioPalsPtr + 1, (uint8_t)ADDR_WarioInvisiblePal);
				if (!ZF)
					return;
				SET_HL(gb_, ADDR_WarioDefaultPal);
				CALL(SetWarioPal);
				return;
			}
		}
		// .alternate_invisibility
		cpu_.a = alu_.xor_(rd8(wInvisibleFrame), 0x1);
		wr8(wInvisibleFrame, cpu_.a);
	}

	void port_DoSoftLand()
	{
		CALL(TriggerFloorTransition_SkipUpdateYPos);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		if (ld_and(wIsSmashAttacking)) {
			ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
			if (CF)
				port_DoGroundPound();
			else
				port_DoWarioGroundShake();
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_DOWN);
		if (cpu_.a) {
			port_CrouchOrSlide();
			return;
		}
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (b_and()) {
			port_CrouchOrSlide();
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			port_SetState_Walking();
		else
			port_SetState_Idling();
	}

	void port_WalkRight()
	{
		xor_a();
		wr8(wIsTurning, 0);
		if (!facing_right()) {
			st(wDirection, DIRECTION_RIGHT);
			st(wIsTurning, TRUE);
		}
		port_WalkIfPossible_Right();
	}

	void port_WalkLeft()
	{
		xor_a();
		wr8(wIsTurning, 0);
		if (facing_right()) {
			st(wDirection, DIRECTION_LEFT);
			st(wIsTurning, TRUE);
		}
		port_WalkIfPossible_Left();
	}

	void port_DoBumpOrJumpingBump()
	{
		play_sfx(SFX_BUMP);
		st(wJumpVelIndex, ld_and(wJumpVelTable) ? FALLING_JUMP_VEL_INDEX : 10);
		port_SetState_Bumping();
	}

	void port_DoJumpingBump_NoSFX()
	{
		st(wJumpVelIndex, 10);
		port_SetState_Bumping(); // fallthrough
	}

	void port_UpdateState_Turning()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_StartJump_FromInput();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			port_SetState_Attacking();
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			port_SetState_Walking_ResetWalkVel();
		else
			port_SetState_Idling();
	}

	void port_Func_1eb46()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_Func_1ec19();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			port_Func_1ec4b();
			return;
		}
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_CROUCHING);
			if (!ZF) {
				return;
			}
			if (!b_and()) {
				port_SetState_Idling();
				return;
			}
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (!cpu_.a) {
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else
			st(wDirection, DIRECTION_LEFT);
		port_SetState_CrouchWalking();
	}

	void port_UpdateState_Diving()
	{
		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_DIVING);
		if (!ZF) {
			return;
		}
		if (!b_and()) {
			port_HandleDivingHorizontalMovement();
			cpu_.a = inc_counter();
			alu_.cp(cpu_.a, 10);
			if (CF) {
				cpu_.b = 2;
			} else {
				alu_.cp(cpu_.a, 14);
				if (!CF) {
					port_SetState_Submerged();
					return;
				}
				cpu_.b = 1;
			}
			CALL(MoveWarioDown);
			FARCALL(CheckCentreCollision);
			if (!b_and()) {
				return;
			}
		}
		// .submerge
		update_pos_y();
		port_SetState_Submerged(); // fallthrough
	}

	void port_Func_1f825()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			xor_a();
			wr8(wIsRolling, 0);
			port_StartJump_FromInput();
			return;
		}
		bool right = facing_right();
		if (ld_bit(wJoypadPressed, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
			xor_a();
			wr8(wIsRolling, 0);
			port_SetState_Idling();
			return;
		}
		if (right)
			port_WalkIfPossible_Right();
		else
			port_WalkIfPossible_Left();
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
	}

	void port_UpdateState_StungRecovery()
	{
		port_Func_1f6c2();
		anim(OAM1);
		cpu_.a = inc_counter();
		alu_.cp(cpu_.a, 0x14);
		if (CF) {
			return;
		}
		flip_dir();
		st(wInvincibleCounter, 0x10);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		ld_cp(wWarioState, WST_STUNG_RECOVERY);
		if (!ZF) {
			return;
		}
		if (!b_and()) {
			if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
				port_SetState_Idling();
				return;
			}
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		port_CrouchOrSlide();
	}

	// the shared tail of UpdateState_PipeGoingDown/Up; true if it jumped
	bool pipe_counter(bool up)
	{
		cpu_.a = inc_counter();
		alu_.cp(cpu_.a, 0x20);
		if (ZF) {
			CALL(ClearTransformationValues);
			SET_HL(gb_, wRoomTransitionParam);
			wr8(wRoomTransitionParam, rd8(wRoomTransitionParam) & ~(1 << ROOMTRANSITIONF_DELAY_F));
			CALL(TriggerRoomTransition);
			return true;
		}
		alu_.cp(cpu_.a, 0x21);
		if (ZF) {
			play_sfx(SFX_PIPE_EXIT);
			return true;
		}
		alu_.cp(cpu_.a, 0x40);
		if (CF) {
			return true;
		}
		cpu_.a = alu_.sub(rd8(wWarioXPos + 1), 0x08);
		wr8(wWarioXPos + 1, cpu_.a);
		xor_a();
		wr8(wIsIntangible, 0);
		if (up)
			CALL(UpdateFloor);
		port_SetState_Idling();
		return true;
	}

	void port_UpdateState_PipeGoingDown()
	{
		st(wIsIntangible, TRUE);
		anim(OAM1);
		cpu_.b = 1;
		CALL(MoveWarioDown);
		pipe_counter(false);
	}

	void port_UpdateState_PipeGoingUp()
	{
		st(wIsIntangible, TRUE);
		anim(OAM1);
		cpu_.b = 1;
		CALL(MoveWarioUp);
		pipe_counter(true);
	}

	void port_UpdateState_Landing()
	{
		if (ld_mask(wJoypadPressed, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_Walking_ResetWalkVel();
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			return;
		}
		bool jump = false;
		if (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A))
			jump = true;
		if (!jump && !ld_bit(wJoypadPressed, B_PAD_A)) {
			if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
				port_CrouchOrSlide();
				return;
			}
			port_SetState_Idling();
			return;
		}
		port_StartJump_FromInput();
		ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
		if (!CF)
			st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
	}

	void port_HandleInput_Walking()
	{
		if (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A)) {
			port_StartJump_FromInput();
			return;
		}
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			port_SetState_Attacking();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_StartJump_FromInput();
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			CALL(IsOnSlipperyGround);
			if (ZF) {
				port_CrouchOrSlide();
				return;
			}
			FARCALL(SetState_CrouchSlipping);
			return;
		}
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else {
			CALL(IsOnSlipperyGround);
			if (ZF) {
				port_SetState_Idling();
				return;
			}
			FARCALL(SetState_Slipping);
			return;
		}
		bool facing = facing_right();
		if (facing != right) {
			port_SetState_Turning();
			return;
		}
		st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			return;
		}
		if (right) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		ld_cp(wWalkVelIndex, 0x10);
		if (!CF)
			st(wWalkVelIndex, 0x0c);
	}

	void port_Func_1ec6c()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_Func_1ec19();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			port_Func_1ec4b();
			return;
		}
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_CROUCH_WALKING);
			if (!ZF) {
				return;
			}
			if (!b_and()) {
				port_SetState_Idling();
				return;
			}
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		cpu_.a = rd8(wJoypadDown);
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			right = true;
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			right = false;
		else {
			port_CrouchOrSlide();
			return;
		}
		if (facing_right() != right) {
			st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
			reset_anim();
			set_frameset(right ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
		}
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			return;
		}
		if (right) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		ld_cp(wWalkVelIndex, 0x04);
		if (!CF) {
			cpu_.a = 0;
			wr8(wWalkVelIndex, 0);
		}
	}

	void port_SetState_GrabIdling()
	{
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		static const uint16_t clr[] = {
			wJumpVelIndex, wJumpVelTable, wIsSmashAttacking, wIsCrouching,
			wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, 0,
		};
		clear(clr);
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
		cpu_.a = alu_.add(cpu_.a, GRAB_IDLE);
		wr8(wGrabState, cpu_.a);
		st(wWarioState, WST_GRAB_IDLING);
		coll_box_btlr();
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		frameset_by_dir(ADDR_Frameset_14a20, ADDR_Frameset_14a23);
		anim(OAM1);
	}

	void port_DoBumpWithBigBackOffset()
	{
		cpu_.b = 3;
		if (facing_right())
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
		port_DoBumpOrJumpingBump();
	}

	void port_DoBumpWithSmallBackOffset()
	{
		cpu_.b = 2;
		if (facing_right())
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
		port_DoBumpOrJumpingBump();
	}

	void port_DoJumpingBump()
	{
		play_sfx(SFX_BUMP);
		port_DoJumpingBump_NoSFX(); // fallthrough
	}

	void port_UpdateState_Crouching()
	{
		FARCALL(Func_19b25);
		ld_cp(wWarioState, WST_CROUCHING);
		if (!ZF) {
			return;
		}
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		port_Func_1eb46();
		ld_cp(wWarioState, WST_CROUCHING);
		if (!ZF) {
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			port_StartCrouchFall();
			return;
		}
	}

	// farcall Func_19b25, then the room transition, water and sand checks of
	// UpdateState_Bumping/Stung; true if it jumped
	bool bump_stung_start()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return true;
		}
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return true;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return true;
		}
		return false;
	}

	// ld hl, wJumpVelIndex / inc [hl] / ld a, [hl]
	uint8_t inc_jump_vel_index()
	{
		SET_HL(gb_, wJumpVelIndex);
		cpu_.a = alu_.inc(rd8(wJumpVelIndex));
		wr8(wJumpVelIndex, cpu_.a);
		return cpu_.a;
	}

	void port_UpdateState_Bumping()
	{
		if (bump_stung_start())
			return;
		ld_cp(wWarioState, WST_BUMPING);
		if (!ZF) {
			return;
		}
		port_HandleBumpMovement();
		ld_cp(wWarioState, WST_BUMPING);
		if (!ZF) {
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_BUMPING);
			if (ZF && b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		update_pos_y();
		flip_dir();
		ld_cp(wWarioState, WST_BUMPING);
		if (!ZF) {
			return;
		}
		port_DoSoftLand();
	}

	void port_UpdateState_Stung()
	{
		if (bump_stung_start())
			return;
		port_Func_1f6c2();
		anim(OAM1);
		FARCALL(CheckFrontCollision);
		if (!b_and()) {
			bool right = facing_right();
			cpu_.b = 1;
			ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			if (CF)
				cpu_.b = alu_.inc(cpu_.b);
			if (right)
				CALL(MoveWarioRight);
			else
				CALL(MoveWarioLeft);
		} else {
			cpu_.b = 1;
			if (facing_right())
				CALL(MoveWarioLeft);
			else
				CALL(MoveWarioRight);
		}
		bool recover = false;
		if (!ld_and(wJumpVelTable)) {
			alu_.cp(inc_jump_vel_index(), MAX_JUMP_VEL_INDEX);
			if (!CF) {
				recover = true;
			} else {
				FARCALL(CheckCentreCollision);
				cpu_.a = alu_.and_(cpu_.b, 0x0f);
				if (!cpu_.a) {
					st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
					st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
				}
				return;
			}
		} else {
			for (;;) {
				cpu_.e = rd8(wJumpVelIndex);
				cpu_.a = cpu_.e;
				cpu_.d = 0;
				uint16_t hl = alu_.add16(ADDR_JumpVelTable_Normal, REG_DE(gb_));
				SET_HL(gb_, hl);
				uint8_t v = rd8(hl);
				if (!alu_.bit(v, 7)) {
					cpu_.b = v;
					CALL(MoveWarioDown);
					alu_.cp(inc_jump_vel_index(), MAX_JUMP_VEL_INDEX);
					if (!CF)
						wr8(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
					break;
				}
				cpu_.a = alu_.inc(alu_.cpl(v));
				cpu_.b = cpu_.a;
				CALL(MoveWarioUp);
				inc_jump_vel_index();
				FARCALL(CheckUpCollision);
				if (!b_and()) {
					return;
				}
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			}
			FARCALL(CheckAirborneCollision);
			if (!b_and()) {
				CALL(TriggerDownwardsFloorTransition);
				return;
			}
			CALL(TriggerFloorTransition);
			cpu_.a = alu_.inc(rd8(wJumpVelTable));
			wr8(wJumpVelTable, cpu_.a);
			cpu_.a = alu_.sub(cpu_.a, 0x02);
			if (ZF) {
				st(wJumpVelIndex, 0x0a);
				return;
			}
			cpu_.a = alu_.dec(cpu_.a);
			if (ZF) {
				st(wJumpVelIndex, 0x0e);
				return;
			}
			recover = true;
		}
		(void)recover;
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		st(wWarioState, WST_STUNG_RECOVERY);
	}

	void port_UpdateState_EnemyBumping()
	{
		if (bump_stung_start())
			return;
		ld_cp(wWarioState, WST_ENEMY_BUMPING);
		if (!ZF) {
			return;
		}
		anim(OAM1);
		port_HandleBumpMovement();
		ld_cp(wWarioState, WST_ENEMY_BUMPING);
		if (!ZF) {
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		flip_dir();
		update_pos_y();
		port_DoSoftLand();
	}

	void port_UpdateState_EnteringDoor()
	{
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		cpu_.a = rd8(wWarioStateCounter);
		cpu_.a = alu_.dec(cpu_.a);
		if (ZF) {
			if (anim_ended()) {
				SET_HL(gb_, wWarioStateCounter);
				wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
				SET_HL(gb_, wRoomTransitionParam);
				wr8(wRoomTransitionParam, rd8(wRoomTransitionParam) & ~(1 << ROOMTRANSITIONF_DELAY_F));
				CALL(TriggerRoomTransition);
				return;
			}
			return;
		}
		cpu_.a = alu_.dec(cpu_.a);
		if (ZF) {
			SET_HL(gb_, wWarioStateCounter);
			wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
			return;
		}
		cpu_.a = alu_.dec(cpu_.a);
		if (ZF) {
			if (!anim_ended()) {
				return;
			}
			if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
				port_SetState_Walking_ResetWalkVel();
				return;
			}
			port_SetState_Idling();
			return;
		}
		if (!anim_ended()) {
			return;
		}
		reset_anim();
		set_frameset(ADDR_Frameset_14d15);
		anim(OAM1);
		SET_HL(gb_, wWarioStateCounter);
		wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
	}

	// one direction of ApplyLastWaterCurrentMovement: with the current, the
	// pad bit `with`, against it `against`; returns true to reset the current
	bool water_current_push(int with, int against, uint8_t bank, uint16_t coll, uint16_t move)
	{
		return water_current_push_b(with, against, bank, coll, move, 1);
	}

	// b = the distance moved (1 for the lingering current, 2 for the current itself)
	bool water_current_push_b(int with, int against, uint8_t bank, uint16_t coll, uint16_t move,
		uint8_t step)
	{
		if (ld_bit(wc0e1, with))
			return true;
		port_farcall(gb_, bank, coll);
		if (b_and())
			return true;
		cpu_.a = rd8(wSwimmingDirectionInput);
		if (alu_.bit(cpu_.a, with))
			return false;
		if (alu_.bit(cpu_.a, against)) {
			ld_cp(wPowerUpLevel, PRINCE_FROGS_GLOVES);
			if (!CF)
				return false;
		}
		cpu_.b = step;
		gb_call(gb_, move);
		return false;
	}

	void port_ApplyLastWaterCurrentMovement()
	{
		if (!ld_and(wLastWaterCurrent))
			return;
		bool reset;
		if (alu_.bit(cpu_.a, CURRENT_RIGHT_F))
			reset = water_current_push(B_PAD_RIGHT, B_PAD_LEFT, BANK_Func_19741, ADDR_Func_19741, ADDR_MoveWarioRight);
		else if (alu_.bit(cpu_.a, CURRENT_LEFT_F))
			reset = water_current_push(B_PAD_LEFT, B_PAD_RIGHT, BANK_Func_197b1, ADDR_Func_197b1, ADDR_MoveWarioLeft);
		else if (alu_.bit(cpu_.a, CURRENT_UP_F))
			reset = water_current_push(B_PAD_UP, B_PAD_DOWN, BANK_CheckUpCollision, ADDR_CheckUpCollision, ADDR_MoveWarioUp);
		else if (alu_.bit(cpu_.a, CURRENT_DOWN_F))
			reset = water_current_push(B_PAD_DOWN, B_PAD_UP, BANK_CheckCentreCollision, ADDR_CheckCentreCollision, ADDR_MoveWarioDown);
		else
			return;
		if (!reset) {
			cpu_.a = alu_.sub(rd8(wLastWaterCurrent), 1 << 4);
			wr8(wLastWaterCurrent, cpu_.a);
			cpu_.a = alu_.and_(cpu_.a, 0xf0);
			if (cpu_.a)
				return;
		}
		xor_a();
		wr8(wLastWaterCurrent, 0);
	}

	void port_HandleInput_Airborne()
	{
		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			xor_a();
			wr8(wJumpingUpwards, 0);
			ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			if (CF && !ld_and(wDoFullJump))
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		}
		cpu_.e = rd8(wJumpVelIndex);
		cpu_.d = 0;
		uint16_t table;
		cpu_.a = alu_.dec(rd8(wJumpVelTable));
		if (ZF) {
			table = ADDR_JumpVelTable_KnockBack;
		} else {
			cpu_.a = alu_.dec(cpu_.a);
			table = ZF ? ADDR_JumpVelTable_Normal : ADDR_JumpVelTable_HighJump;
		}
		uint16_t hl = alu_.add16(table, REG_DE(gb_));
		SET_HL(gb_, hl);
		uint8_t v = rd8(hl);
		if (alu_.bit(v, 7)) {
			cpu_.a = alu_.inc(alu_.cpl(v));
			cpu_.b = cpu_.a;
			CALL(MoveWarioUp);
			inc_jump_vel_index();
		} else {
			xor_a();
			wr8(wDoFullJump, 0);
			cpu_.b = rd8(hl);
			CALL(MoveWarioDown);
			alu_.cp(inc_jump_vel_index(), MAX_JUMP_VEL_INDEX);
			if (!CF)
				wr8(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			port_WalkRight();
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			port_WalkLeft();
		} else {
			return;
		}
		CALL(Func_1762);
	}

	void port_UpdateState_SmashAttacking()
	{
		anim(OAM1);
		alu_.cp(inc_counter(), 0x1b);
		if (CF) {
			return;
		}
		xor_a();
		wr8(wIsSmashAttacking, 0);
		port_SetState_Idling();
	}

	// the shared body of UpdateState_Throwing/ThrowingAirborne
	void throwing(void (WarioStates1::*done)())
	{
		if (!ld_and(wGrabState)) {
			(this->*done)();
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			return;
		}
		xor_a();
		wr8(wGrabState, 0);
		(this->*done)();
	}

	void port_UpdateState_Throwing() { throwing(&WarioStates1::port_SetState_Idling); }
	void port_UpdateState_ThrowingAirborne() { throwing(&WarioStates1::port_StartFall); }

	void port_Func_1dd78()
	{
		xor_a();
		wr8(wGrabState, 0);
		port_StartJump_FromInput();
	}

	void port_UpdateState_SwimKnockBack()
	{
		FARCALL(CheckFrontCollision);
		if (!b_and()) {
			cpu_.a = inc_counter();
			alu_.cp(cpu_.a, 0x04);
			bool move = true;
			if (CF) {
				cpu_.b = 0x02;
			} else {
				alu_.cp(cpu_.a, 0x08);
				if (CF)
					cpu_.b = 0x01;
				else
					move = false;
			}
			if (move) {
				move_by_dir();
				return;
			}
		}
		flip_dir();
		port_SetState_Submerged();
	}

	// xor a / ld [wIsIntangible], a / ld a, $10 / ld [wInvincibleCounter], a / jp
	void water_stung_end(void (WarioStates1::*to)())
	{
		xor_a();
		wr8(wIsIntangible, 0);
		st(wInvincibleCounter, 0x10);
		(this->*to)();
	}

	void port_UpdateState_WaterStung()
	{
		anim(OAM1);
		SET_HL(gb_, wWarioStateCounter);
		wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
		FARCALL(CheckUpCollision);
		if (b_and()) {
			ld_cp(wWarioStateCounter, 0x78);
			if (CF) {
				return;
			}
			st(wc0e1, PAD_UP);
			water_stung_end(&WarioStates1::port_SetState_Submerged);
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			ld_cp(wWarioStateCounter, 0x78);
			if (CF) {
				return;
			}
			water_stung_end(&WarioStates1::port_SurfaceFromUnderwater);
			return;
		}
		ld_cp(wWarioStateCounter, 0x78);
		if (!CF) {
			water_stung_end(&WarioStates1::port_SetState_Submerged);
			return;
		}
		if (ld_mask(wGlobalCounter, 1)) {
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
		if (!ld_and(wWaterCurrent)) {
			return;
		}
		xor_a();
		wr8(wWaterCurrent, 0);
		water_stung_end(&WarioStates1::port_SetState_Submerged);
	}

	// ld hl, wCurWaterCurrent / ld a, [hld] / or [hl] / and mask; true if nz
	bool water_current_mask(uint8_t mask)
	{
		SET_HL(gb_, wCurWaterCurrent - 1);
		cpu_.a = alu_.or_(rd8(wCurWaterCurrent), rd8(wCurWaterCurrent - 1));
		cpu_.a = alu_.and_(cpu_.a, mask);
		return cpu_.a != 0;
	}

	void port_Func_1f310()
	{
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			st(wc0e1, facing_right() ? PAD_RIGHT : PAD_LEFT);
			return;
		}
		bool right = facing_right();
		if (water_current_mask(CURRENT_RIGHT | CURRENT_LEFT))
			return;
		cpu_.b = 0x01;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	}

	void port_Func_1f3d9()
	{
		FARCALL(CheckUpCollision);
		if (b_and()) {
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		if (water_current_mask(CURRENT_UP | CURRENT_DOWN)) {
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
	}

	// ApplyWaterCurrentMovement.ChangeCurrentDirection
	void change_current_direction()
	{
		if (ld_and(wLastWaterCurrent))
			return;
		cpu_.a = alu_.add(rd8(wCurWaterCurrent), 15 << 4);
		wr8(wLastWaterCurrent, cpu_.a);
	}

	// one direction of ApplyWaterCurrentMovement
	void water_current(int bit, uint8_t others, int with, int against, uint8_t bank,
		uint16_t coll, uint16_t move)
	{
		cpu_.a = alu_.and_(rd8(wCurWaterCurrent), others);
		if (cpu_.a)
			change_current_direction();
		SET_HL(gb_, wWaterCurrent);
		cpu_.a = rd8(wWaterCurrent);
		wr8(wCurWaterCurrent, cpu_.a);
		wr8(wWaterCurrent, (uint8_t)(rd8(wWaterCurrent) & ~(1 << bit)));
		water_current_push_b(with, against, bank, coll, move, 2);
	}

	void port_ApplyWaterCurrentMovement()
	{
		port_ApplyLastWaterCurrentMovement();
		cpu_.a = rd8(wWaterCurrent);
		if (alu_.bit(cpu_.a, CURRENT_RIGHT_F))
			water_current(CURRENT_RIGHT_F, CURRENT_LEFT | CURRENT_UP | CURRENT_DOWN, B_PAD_RIGHT, B_PAD_LEFT,
				BANK_Func_19741, ADDR_Func_19741, ADDR_MoveWarioRight);
		else if (alu_.bit(cpu_.a, CURRENT_LEFT_F))
			water_current(CURRENT_LEFT_F, CURRENT_RIGHT | CURRENT_UP | CURRENT_DOWN, B_PAD_LEFT, B_PAD_RIGHT,
				BANK_Func_197b1, ADDR_Func_197b1, ADDR_MoveWarioLeft);
		else if (alu_.bit(cpu_.a, CURRENT_UP_F))
			water_current(CURRENT_UP_F, CURRENT_RIGHT | CURRENT_LEFT | CURRENT_DOWN, B_PAD_UP, B_PAD_DOWN,
				BANK_CheckUpCollision, ADDR_CheckUpCollision, ADDR_MoveWarioUp);
		else if (alu_.bit(cpu_.a, CURRENT_DOWN_F))
			water_current(CURRENT_DOWN_F, CURRENT_RIGHT | CURRENT_LEFT | CURRENT_UP, B_PAD_DOWN, B_PAD_UP,
				BANK_CheckCentreCollision, ADDR_CheckCentreCollision, ADDR_MoveWarioDown);
		else {
			if (ld_and(wCurWaterCurrent))
				change_current_direction();
			xor_a();
			wr8(wCurWaterCurrent, 0);
		}
	}

	void port_HandleAttackingAirborneMovement()
	{
		CALL(ApplyJumpVelocity);
		FARCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x14);
		if (!CF)
			st(wWalkVelIndex, 0x10);
	}

	// ld a, [wWarioState] / cp state: true if still in that state
	bool still_in(uint8_t state)
	{
		ld_cp(wWarioState, state);
		return ZF;
	}

	// update_pos
	void update_pos()
	{
		for (int i = 0; i < 4; i++) {
			cpu_.a = rd8(hPos + i);
			wr8(wWarioPos + i, cpu_.a);
		}
		SET_HL(gb_, hPos + 3);
		SET_DE(gb_, wWarioPos + 3);
	}

	// farcall Func_19b25 / room transition check; true if it jumped
	bool room_transition_check()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return true;
		}
		return false;
	}

	// get_pos / ld b, PARTICLE_DUST / farcall CreateParticle
	void dust()
	{
		get_pos();
		cpu_.b = PARTICLE_DUST;
		FARCALL(CreateParticle);
	}

	void port_UpdateState_Walking()
	{
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		if (anim_ended())
			dust();
		port_HandleInput_Walking();
		if (!still_in(WST_WALKING)) {
			return;
		}
		if (room_transition_check())
			return;
		if (!still_in(WST_WALKING)) {
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_WALKING)) {
			return;
		}
		if (!b_and()) {
			port_StartFall();
			return;
		}
		update_pos_y();
	}

	// the framesets of UpdateState_Airborne and UpdateState_GrabAirborne
	typedef struct {
		uint16_t left, right; // not smash attacking
		uint16_t smash_left, smash_right, super_smash_left, super_smash_right;
	} AirborneFramesets;

	// the smash attack checks after HandleInput_Airborne, up to the
	// UpdateWarioAnimation call
	void airborne_smash_attack(const AirborneFramesets *f)
	{
		enum { CONTINUE, SET_SMASH, NO_SMASH } next = CONTINUE;
		ld_cp(wPowerUpLevel, LEAD_OVERALLS);
		if (!CF) {
			if (ld_and(wIsSmashAttacking)) {
				// .check_still_smash_attacking
				if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
					xor_a();
					wr8(wIsSmashAttacking, FALSE);
					next = NO_SMASH;
				}
			} else {
				ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
				if (!CF && ld_bit(wJoypadDown, B_PAD_DOWN)) {
					st(wIsSmashAttacking, TRUE);
					next = SET_SMASH;
				}
			}
		}
		if (next == CONTINUE) {
			if (ld_and(wIsTurning))
				next = ld_and(wIsSmashAttacking) ? SET_SMASH : NO_SMASH;
		}
		if (next == SET_SMASH) {
			reset_anim();
			bool right = facing_right();
			ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
			if (right)
				set_frameset(CF ? f->smash_right : f->super_smash_right);
			else
				set_frameset(CF ? f->smash_left : f->super_smash_left);
		} else if (next == NO_SMASH) {
			reset_anim();
			frameset_by_dir(f->left, f->right);
		}
		anim(OAM1);
	}

	void port_UpdateState_Airborne()
	{
		if (room_transition_check())
			return;
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (!CF) {
			// falling
			if (ld_and(wWaterInteraction)) {
				port_DiveFromSurface();
				return;
			}
			if (ld_and(wIsInSand)) {
				FARCALL(SetState_SandFalling);
				return;
			}
			if (ld_and(wLadderInteraction) && ld_bit(wJoypadDown, B_PAD_UP)) {
				FARCALL(SetState_LadderClimbing);
				return;
			}
			if (!still_in(WST_AIRBORNE)) {
				return;
			}
			FARCALL(Func_19afb);
			if (ld_and(wIsNearFence) && ld_bit(wJoypadDown, B_PAD_UP)) {
				FARCALL(SetState_FenceMovingVertical);
				return;
			}
		}
		// .handle_input
		if (!still_in(WST_AIRBORNE)) {
			return;
		}
		port_HandleInput_Airborne();
		if (!still_in(WST_AIRBORNE)) {
			return;
		}

		static const AirborneFramesets f = {ADDR_Frameset_15f94, ADDR_Frameset_15f97, ADDR_Frameset_15f8e,
			ADDR_Frameset_15f91, ADDR_Frameset_1604e, ADDR_Frameset_16055};
		airborne_smash_attack(&f);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_AIRBORNE)) {
				return;
			}
			if (ld_and(wJumpCollision)) {
				port_StartFall_SkipResetJumpUpwards();
				return;
			}
			center_on_ladder();
			return;
		}
		// .falling
		FARCALL(CheckAirborneCollision);
		if (!still_in(WST_AIRBORNE)) {
			return;
		}
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		update_pos_y();
		dust();
		ld_cp(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
		if (CF) {
			port_DoSoftLand();
			return;
		}
		if (!ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_DoHardLand();
			return;
		}
		port_DoSoftLand(); // fallthrough
	}

	void port_UpdateState_CrouchWalking()
	{
		sfx_loop(21, SFX_CROUCH_WALK);
		anim(OAM1);
		if (room_transition_check())
			return;
		if (!still_in(WST_CROUCH_WALKING)) {
			return;
		}
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		port_Func_1ec6c();
		if (!still_in(WST_CROUCH_WALKING)) {
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			port_StartCrouchFall();
			return;
		}
		update_pos_y();
		if (ld_and(wWarioSlopeInteraction)) {
			port_SetState_Sliding();
			return;
		}
	}

	void port_UpdateState_CrouchAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return;
		}
		if (!still_in(WST_CROUCH_AIRBORNE)) {
			return;
		}
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (!still_in(WST_CROUCH_AIRBORNE)) {
				return;
			}
			if (!b_and()) {
				port_StartJump();
				return;
			}
		}
		// .asm_1d583
		st(wCollisionBoxTop, (uint8_t)-15);
		port_HandleInput_Airborne();
		if (!still_in(WST_CROUCH_AIRBORNE)) {
			return;
		}
		if (ld_and(wIsTurning)) {
			reset_anim();
			frameset_by_dir(ADDR_Frameset_14a38, ADDR_Frameset_14a3b);
			anim(OAM1);
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			// rising
			FARCALL(CheckUpCollision);
			if (b_and() && still_in(WST_CROUCH_AIRBORNE))
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		// .falling
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			if (!still_in(WST_CROUCH_AIRBORNE)) {
				return;
			}
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		update_pos_y();
		if (!still_in(WST_CROUCH_AIRBORNE)) {
			return;
		}
		port_DoSoftLand();
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			return;
		}
		port_CrouchOrSlide();
	}

	// ld a, b / and $0f: true if nz
	bool b_low_nibble()
	{
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		return cpu_.a != 0;
	}

	void port_UpdateState_Sliding()
	{
		sfx_loop(12, SFX_ZIP_LINE_SLIDE);
		anim(OAM1);
		port_Func_1f825();
		if (!still_in(WST_SLIDING)) {
			return;
		}
		FARCALL(Func_198c7);
		if (!b_and()) {
			port_Func_1e174();
			return;
		}
		update_pos();
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
		if (!anim_ended()) {
			return;
		}
		port_SetState_Rolling(); // fallthrough
	}

	// move by direction with the walk velocity, then cap it at $18 (to $14)
	void roll_move()
	{
		if (facing_right()) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		ld_cp(wWalkVelIndex, 0x18);
		if (!CF)
			st(wWalkVelIndex, 0x14);
	}

	// a = wWarioSlopeInteraction: and $0f / dec a / ld b, a / ld a, [wDirection] /
	// xor b; true if nz (rolling against the slope)
	bool against_slope()
	{
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		cpu_.a = alu_.dec(cpu_.a);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.xor_(rd8(wDirection), cpu_.b);
		return cpu_.a != 0;
	}

	void port_UpdateState_Rolling()
	{
		if (room_transition_check())
			return;
		sfx_loop(12, SFX_ROLL);
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			// .asm_1e145
			FARCALL(CheckCentreCollision);
			update_pos_y();
			if (ld_and(wWarioSlopeInteraction) && against_slope()) {
				port_DoBumpWithSmallBackOffset();
				return;
			}
			port_Func_1e1e3();
			return;
		}
		roll_move();
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			port_DoBumpWithBigBackOffset();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			port_Func_1e174();
			return;
		}
		update_pos_y();
		if (ld_and(wWarioSlopeInteraction) && against_slope()) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
	}

	void port_UpdateState_RollingAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return;
		}
		anim(OAM1);
		roll_move();
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			port_DoBumpWithBigBackOffset();
			return;
		}
		cpu_.a = rd8(wJumpVelIndex);
		cpu_.e = cpu_.a;
		cpu_.d = 0x00;
		uint16_t hl = alu_.add16(ADDR_JumpVelTable_HighJump, REG_DE(gb_));
		SET_HL(gb_, hl);
		if (alu_.bit(rd8(hl), 7)) {
			// rising
			cpu_.a = alu_.inc(alu_.cpl(rd8(hl)));
			cpu_.b = cpu_.a;
			CALL(MoveWarioUp);
			SET_HL(gb_, wJumpVelIndex);
			wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
		} else {
			cpu_.b = rd8(hl);
			CALL(MoveWarioDown);
			SET_HL(gb_, wJumpVelIndex);
			cpu_.a = alu_.inc(rd8(wJumpVelIndex));
			wr8(wJumpVelIndex, cpu_.a);
			alu_.cp(cpu_.a, MAX_JUMP_VEL_INDEX);
			if (!CF)
				wr8(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		port_SetState_Rolling();
	}

	void port_UpdateState_PickingUp()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1dd78();
			return;
		}
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		alu_.cp(cpu_.a, 0x02);
		if (ZF) {
			port_SetState_GrabIdling();
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			return;
		}
		port_SetState_GrabIdling();
	}

	void port_UpdateState_ThrowCharging()
	{
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (!alu_.bit(cpu_.a, B_PAD_B)) {
			port_Func_1de3f();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_Func_1dd78();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			port_CrouchOrSlide();
			return;
		}
		cpu_.b = ld_bit(wGrabState, GRAB_HEAVY_F) ? THROW_CHARGE_FRAMES_HEAVY : THROW_CHARGE_FRAMES_LIGHT;
		cpu_.a = alu_.inc(rd8(wWarioStateCounter));
		wr8(wWarioStateCounter, cpu_.a);
		alu_.cp(cpu_.a, cpu_.b);
		if (CF) {
			return;
		}
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles, 0};
		clear(zero);
		st(wWarioState, WST_THROW_FULLY_CHARGED);
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
		cpu_.a = alu_.add(cpu_.a, GRAB_THROW_FULLY_CHARGED);
		wr8(wGrabState, cpu_.a);
		frameset_by_dir(ADDR_Frameset_16432, ADDR_Frameset_16435);
		anim(OAM1);
	}

	void port_UpdateState_ThrowFullyCharged()
	{
		sfx_loop(30, SFX_FULLY_CHARGED_THROW);
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, GRAB_STATE_MASK);
		alu_.cp(cpu_.a, GRAB_HOLD_FULL_CHARGE);
		if (!ZF && !ld_and(wWarioStateCounter) && anim_ended()) {
			cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
			cpu_.a = alu_.add(cpu_.a, GRAB_HOLD_FULL_CHARGE);
			wr8(wGrabState, cpu_.a);
			reset_anim();
			cpu_.a = alu_.inc(cpu_.a);
			wr8(wWarioStateCounter, cpu_.a);
			frameset_by_dir(ADDR_Frameset_16438, ADDR_Frameset_16441);
		}
		anim(OAM1);
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			if (alu_.bit(cpu_.a, B_PAD_A)) {
				port_Func_1dd78();
				return;
			}
			if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
				port_CrouchOrSlide();
				return;
			}
			return;
		}
		reset_anim();
		frameset_by_dir(ADDR_Frameset_16403, ADDR_Frameset_16423);
		anim(OAM1);
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
		cpu_.a = alu_.add(cpu_.a, GRAB_THROW | (1 << GRAB_FULL_CHARGE_F));
		wr8(wGrabState, cpu_.a);
		port_SetState_Throwing();
	}

	void port_UpdateState_GrabSmashAttacking()
	{
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		anim(OAM1);
		alu_.cp(inc_counter(), 0x1b);
		if (CF) {
			return;
		}
		xor_a();
		wr8(wIsSmashAttacking, FALSE);
		port_SetState_GrabIdling();
	}

	void port_UpdateState_GroundShakeStunned()
	{
		anim(OAM1);
		if (!ld_and(wGroundShakeCounter)) {
			// .no_more_ground_shake
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (b_and()) {
				port_CrouchOrSlide();
				return;
			}
			FARCALL(CheckCentreCollision);
			if (!b_and()) {
				port_StartFall();
				return;
			}
			port_SetState_Idling();
			return;
		}
		if (!ld_and(wJumpVelTable)) {
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(HandleWalk);
		CALL(Func_1762);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	// UpdateState_CrouchSliding.end_slide
	void crouch_slide_end()
	{
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (!still_in(WST_CROUCH_SLIDING)) {
				return;
			}
			if (!b_and()) {
				port_SetState_Idling();
				return;
			}
		}
		// .crouching
		st(wCollisionBoxTop, (uint8_t)-15);
		port_CrouchOrSlide();
	}

	void port_UpdateState_CrouchSliding()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_CROUCH_SLIDING)) {
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			// .try_jump
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (!still_in(WST_CROUCH_SLIDING)) {
				return;
			}
			bool blocked = b_and();
			st(wCollisionBoxTop, (uint8_t)-15);
			if (blocked) {
				return;
			}
			port_StartCrouchJump();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_CROUCH_SLIDING)) {
			return;
		}
		if (!b_and()) {
			port_StartCrouchFall();
			return;
		}
		if (ld_and(wWarioSlopeInteraction)) {
			update_pos_y();
			port_SetState_Sliding();
			return;
		}
		FARCALL(CheckFrontCollision);
		if (!still_in(WST_CROUCH_SLIDING)) {
			return;
		}
		if (b_and()) {
			crouch_slide_end();
			return;
		}
		// if Wario has Garlic power-up, then slide further
		SET_HL(gb_, wWarioStateCounter);
		wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
		ld_cp(wPowerUpLevel, GARLIC);
		bool garlic = !CF;
		cpu_.a = rd8(wWarioStateCounter);
		uint8_t step;
		alu_.cp(cpu_.a, garlic ? 0x1e : 0x14);
		if (CF) {
			step = 2; // .slide_fast
		} else {
			alu_.cp(cpu_.a, garlic ? 0x36 : 0x24);
			if (CF) {
				step = 1; // .slide_slow
			} else {
				alu_.cp(cpu_.a, garlic ? 0x3c : 0x28);
				if (CF) {
					// .slide_slower
					cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x1);
					if (ZF) {
						return;
					}
					step = 1;
				} else if (ZF) {
					crouch_slide_end();
					return;
				} else {
					return;
				}
			}
		}
		cpu_.b = step;
		move_by_dir();
	}

	void port_UpdateState_GrabAirborne()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_AIRBORNE)) {
			return;
		}
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		if (!ld_and(wGrabState)) {
			port_StartFall();
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_B)) {
			port_SetState_ThrowingAirborne();
			return;
		}
		port_HandleInput_Airborne();
		if (!still_in(WST_GRAB_AIRBORNE)) {
			return;
		}
		static const AirborneFramesets f = {ADDR_Frameset_15fa8, ADDR_Frameset_15fab, ADDR_Frameset_15fec,
			ADDR_Frameset_15fef, ADDR_Frameset_1605c, ADDR_Frameset_16063};
		airborne_smash_attack(&f);

		if (ld_bit(wGrabState, GRAB_HEAVY_F)) {
			ld_cp(wWalkVelIndex, 0x08);
			if (!CF)
				st(wWalkVelIndex, 0x04);
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!still_in(WST_GRAB_AIRBORNE)) {
			return;
		}
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!ld_and(wFloorTransitionDir))
			play_sfx(SFX_LAND);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wWalkVelIndex, 0);
		if (!ld_and(wIsSmashAttacking)) {
			port_SetState_GrabIdling();
			return;
		}
		ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (CF) {
			// .asm_1dc25
			play_sfx(SFX_GROUND_POUND);
			reset_anim();
			frameset_by_dir(ADDR_Frameset_16020, ADDR_Frameset_16037);
			anim(OAM1);
		} else {
			// .continue
			if (!ld_and(wGroundShakeCounter)) {
				st(wGroundShakeCounter, 0x20);
				st(wIsWarioGroundShaking, TRUE);
			}
			if (!ld_and(wFloorTransitionDir))
				play_sfx(SFX_GROUND_SHAKE);
			reset_anim();
			frameset_by_dir(ADDR_Frameset_15fd2, ADDR_Frameset_15fdf);
			anim(OAM1);
		}
		// .asm_1dc5f
		st(wIsSmashAttacking, 0x81);
		st(wWarioState, WST_GRAB_SMASH_ATTACKING);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
	}

	void port_Func_1f357()
	{
		FARCALL(CheckCentreCollision);
		if (b_and())
			update_pos_y();
		if (still_in(WST_WATER_STUNG)) {
			return;
		}
		if (!ld_and(wDirection)) {
			port_Func_1f310();
			return;
		}
		st(wDirection, DIRECTION_LEFT);
		reset_anim();
		set_frameset(ADDR_Frameset_15576);
		anim(OAM1);
	}

	// ld a, [wSwimVelIndex] / ld e, a / ld d, $00 / ld hl, table / ld a, [wSwimmingDirectionInput] /
	// and mask / jr z / ld hl, alt / add hl, de / ld b, [hl]
	void swim_vel(uint8_t mask, uint16_t alt)
	{
		cpu_.a = rd8(wSwimVelIndex);
		cpu_.e = cpu_.a;
		cpu_.d = 0x00;
		uint16_t hl = ADDR_SwimStraightVelTable;
		if (ld_mask(wSwimmingDirectionInput, mask))
			hl = alt;
		hl = alu_.add16(hl, REG_DE(gb_));
		SET_HL(gb_, hl);
		cpu_.b = rd8(hl);
	}

	// ld hl, wc0e1 / set bit, [hl]
	void set_c0e1(int bit)
	{
		SET_HL(gb_, wc0e1);
		wr8(wc0e1, (uint8_t)(rd8(wc0e1) | (1 << bit)));
	}

	// HandleUnderwaterThrustingInput.up
	void thrust_up()
	{
		FARCALL(CheckUpCollision);
		if (b_and()) {
			set_c0e1(B_PAD_UP);
			port_StartDive();
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		swim_vel(PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894);
		CALL(MoveWarioUp);
	}

	// HandleUnderwaterThrustingInput.down
	void thrust_down()
	{
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			update_pos_y();
			set_c0e1(B_PAD_DOWN);
			return;
		}
		swim_vel(PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894);
		CALL(MoveWarioDown);
	}

	// HandleUnderwaterThrustingInput.right / .left
	void thrust_horizontal(bool right)
	{
		FARCALL(CheckCentreCollision);
		if (b_and())
			update_pos_y();
		if (still_in(WST_WATER_STUNG)) {
			return;
		}
		if (right)
			FARCALL(Func_19741);
		else
			FARCALL(Func_197b1);
		if (b_and()) {
			port_SetState_SwimKnockBack_SwitchDirection();
			return;
		}
		swim_vel(PAD_UP | PAD_DOWN, ADDR_SwimDiagonalVelTable);
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		cpu_.a = rd8(wSwimmingDirectionInput);
		if (alu_.bit(cpu_.a, B_PAD_UP)) {
			thrust_up();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			thrust_down();
			return;
		}
		// .handle_diagonal_movement
		if (!ld_mask(wJoypadDown, PAD_UP | PAD_DOWN)) {
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			// .diagonal_down
			FARCALL(CheckCentreCollision);
			if (!b_and()) {
				cpu_.b = 1;
				CALL(MoveWarioDown);
				return;
			}
			update_pos_y();
			set_c0e1(B_PAD_DOWN);
			return;
		}
		// diagonal up
		FARCALL(CheckUpCollision);
		if (b_and()) {
			set_c0e1(B_PAD_UP);
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		cpu_.b = 1;
		CALL(MoveWarioUp);
	}

	void port_HandleUnderwaterThrustingInput()
	{
		SET_HL(gb_, wSwimVelIndex);
		cpu_.a = alu_.inc(rd8(wSwimVelIndex));
		wr8(wSwimVelIndex, cpu_.a);
		alu_.cp(cpu_.a, 40);
		if (!CF) {
			xor_a();
			wr8(wSwimVelIndex, 0);
			port_SetState_Submerged();
			return;
		}
		if (!ld_mask(wJoypadDown, PAD_A | PAD_B)) {
			port_SetState_Submerged();
			return;
		}
		cpu_.a = rd8(wSwimmingDirectionInput);
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			thrust_horizontal(true);
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			thrust_horizontal(false);
		else if (alu_.bit(cpu_.a, B_PAD_UP))
			thrust_up();
		else if (alu_.bit(cpu_.a, B_PAD_DOWN))
			thrust_down();
	}

	void port_UpdateState_AttackingAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return;
		}
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			return;
		}
		sfx_loop(6, SFX_ATTACK);
		anim(OAM1);
		port_HandleAttackingAirborneMovement();
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			return;
		}
		FARCALL(CheckFrontCollision);
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			return;
		}
		if (b_low_nibble()) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
		cpu_.a = alu_.dec(rd8(wAttackCounter));
		if (!ZF)
			wr8(wAttackCounter, cpu_.a);
		bool stop;
		if (facing_right())
			stop = ld_bit(wJoypadPressed, B_PAD_LEFT);
		else
			stop = ld_bit(wJoypadPressed, B_PAD_RIGHT);
		if (stop) {
			xor_a();
			wr8(wAttackCounter, 0);
			port_StartFall();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_ATTACKING_AIRBORNE)) {
				return;
			}
			ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			if (CF)
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			return;
		}
		port_SetState_Idling();
	}

	void port_UpdateState_WaterSurfaceIdling()
	{
		xor_a();
		wr8(wc0e1, 0);
		FARCALL(Func_19b25);
		if (!still_in(WST_WATER_SURFACE_IDLING)) {
			return;
		}
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		sfx_loop(28, SFX_WATER_SURFACE);
		anim(OAM1);
		port_HandleWaterSurfaceFloatingMovement();
		port_HandleWaterSurfaceInput();
		if (!still_in(WST_WATER_SURFACE_IDLING)) {
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_WaterSurfaceMoving();
			return;
		}
		port_ApplyWaterCurrentMovement();
	}

	// UpdateState_WaterSurfaceMoving.HandleDirectionalInput
	void water_surface_directional_input()
	{
		cpu_.a = rd8(wJoypadDown);
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else {
			port_SetState_WaterSurfaceIdling(); // jp from a called routine
			return;
		}
		if (facing_right() != right) {
			st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
			reset_anim();
			set_frameset(right ? ADDR_Frameset_1555b : ADDR_Frameset_15554);
			anim(OAM1);
			return;
		}
		FARCALL(CheckFrontCollision);
		if (b_and())
			return;
		// current overrides player input
		if (water_current_mask(CURRENT_RIGHT | CURRENT_LEFT))
			return;
		if (right) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		// .done_moving
		ld_cp(wWalkVelIndex, 0x04);
		if (!CF)
			st(wWalkVelIndex, 0x00);
	}

	void port_UpdateState_WaterSurfaceMoving()
	{
		xor_a();
		wr8(wc0e1, 0);
		if (room_transition_check())
			return;
		sfx_loop(28, SFX_WATER_SURFACE);
		anim(OAM1);
		port_HandleWaterSurfaceFloatingMovement();
		port_ApplyWaterCurrentMovement();
		water_surface_directional_input();
		port_HandleWaterSurfaceInput();
	}

	void port_UpdateState_UnderwaterThrusting()
	{
		xor_a();
		wr8(wc0e1, 0);
		if (room_transition_check())
			return;
		if (!still_in(WST_UNDERWATER_THRUSTING)) {
			return;
		}
		sfx_loop(15, SFX_00F);
		anim(OAM1);
		port_HandleUnderwaterThrustingInput();
		if (!still_in(WST_UNDERWATER_THRUSTING)) {
			return;
		}
		port_ApplyWaterCurrentMovement();
	}

	void port_SetState_GrabAirborne()
	{
		st(wWarioState, WST_GRAB_AIRBORNE);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15fa8, ADDR_Frameset_15fab);
		anim(OAM1);
		if (!ld_and(wIsOnSteppableObject))
			return;
		port_UpdateState_GrabAirborne();
	}

	void port_Func_1f24c()
	{
		sfx_loop(37, SFX_00E);
		if (ld_and(wWarioStateCycles)) {
			static const uint16_t zero[] = {wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
			clear(zero);
			frameset_by_dir(ADDR_Frameset_15576, ADDR_Frameset_1558b);
		}
		anim(OAM1);
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x1);
		if (ZF) {
			cpu_.a = rd8(wJoypadDown);
			if (alu_.bit(cpu_.a, B_PAD_UP)) {
				port_Func_1f3d9();
				return;
			}
			if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
				port_Func_1f3a7();
				return;
			}
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (!alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
				port_Func_1f357();
				return;
			}
			return;
		}
		// .asm_1f2c0
		FARCALL(CheckCentreCollision);
		if (b_and())
			update_pos_y();
		if (still_in(WST_WATER_STUNG)) {
			return;
		}
		if (ld_and(wDirection)) {
			port_Func_1f310();
			return;
		}
		st(wDirection, DIRECTION_RIGHT);
		reset_anim();
		set_frameset(ADDR_Frameset_1558b);
		anim(OAM1);
	}

	void port_SetState_AttackingAirborne()
	{
		xor_a();
		wr8(wWarioSlopeInteraction, 0);
		st(wWarioState, WST_ATTACKING_AIRBORNE);
		reset_anim();
		ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
		cpu_.a = JUMP_VEL_NORMAL;
		if (!CF)
			cpu_.a = alu_.inc(cpu_.a); // JUMP_VEL_HIGH_JUMP
		wr8(wJumpVelTable, cpu_.a);
		ld_cp(wPowerUpLevel, GARLIC);
		if (CF)
			frameset_by_dir(ADDR_Frameset_15216, ADDR_Frameset_15219);
		else
			frameset_by_dir(ADDR_Frameset_1523e, ADDR_Frameset_15249);
		anim(OAM1);
		if (!ld_and(wIsOnSteppableObject))
			return;
		port_UpdateState_AttackingAirborne();
	}

	void port_Func_1edd3()
	{
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
		cpu_.a = alu_.add(cpu_.a, GRAB_IDLE);
		wr8(wGrabState, cpu_.a);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		port_SetState_GrabAirborne();
	}

	void port_Func_1ede9()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
		cpu_.a = JUMP_VEL_NORMAL;
		if (!CF) {
			SET_HL(gb_, wJoypadDown);
			if (alu_.bit(rd8(wJoypadDown), B_PAD_UP))
				cpu_.a = JUMP_VEL_HIGH_JUMP;
		}
		wr8(wJumpVelTable, cpu_.a);
		port_SetState_GrabAirborne(); // fallthrough
	}

	void port_Func_1f1a9()
	{
		bool skip = true;
		ld_cp(wWaterInteraction, NON_SUBMERSIBLE_WATER);
		if (!ZF) {
			ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
			if (!CF) {
				cpu_.a = rd8(wJoypadPressed);
				if (alu_.bit(cpu_.a, B_PAD_B)) {
					port_StartUnderwaterThrusting();
					return;
				}
				if (alu_.bit(cpu_.a, B_PAD_A)) {
					port_StartUpwardsUnderwaterThrusting();
					return;
				}
				if (ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
					port_Func_1f24c();
					return;
				}
				skip = ld_and(wWarioStateCycles);
			}
		}
		if (!skip) {
			reset_anim();
			frameset_by_dir(ADDR_Frameset_15569, ADDR_Frameset_15573);
		}
		// .asm_1f1f5
		anim(OAM1);
		st(wWarioStateCycles, 0x01);
		FARCALL(CheckUpCollision);
		if (b_and()) {
			st(wc0e1, PAD_UP);
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x1);
		if (!ZF || water_current_mask(CURRENT_UP | CURRENT_DOWN)) {
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
	}

	void port_StartFallingAirborneAttack()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_AttackingAirborne();
	}

	void port_StartJumpingAirborneAttack()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		st(wJumpingUpwards, TRUE);
		port_SetState_AttackingAirborne(); // fallthrough
	}

	void port_UpdateState_Submerged()
	{
		xor_a();
		wr8(wc0e1, 0);
		if (room_transition_check())
			return;
		if (!still_in(WST_SUBMERGED)) {
			return;
		}
		port_ApplyWaterCurrentMovement();
		port_Func_1f1a9();
	}

	void port_Func_1ee88()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ede9();
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			port_SetState_ThrowCharging();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			port_CrouchOrSlide();
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, PAD_RIGHT | PAD_LEFT);
		if (ZF) {
			return;
		}
		st(wWarioState, WST_GRAB_WALKING);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_14a0f);
		} else {
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_149fe);
		}
		anim(OAM1);
	}

	void port_Func_1eefc()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			port_SetState_ThrowCharging();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_Func_1ede9();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			// .asm_1efaf
			CALL(IsOnSlipperyGround);
			if (ZF) {
				port_CrouchOrSlide();
				return;
			}
			FARCALL(SetState_CrouchSlipping);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else {
			CALL(IsOnSlipperyGround);
			if (!ZF) {
				FARCALL(SetState_GrabSlipping);
				return;
			}
			port_SetState_GrabIdling();
			return;
		}
		if (facing_right() != right) {
			port_SetState_GrabIdling();
			return;
		}
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			return;
		}
		if (right) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		// uphill: .asm_1efc5, else .asm_1ef8d
		bool uphill = false;
		if (ld_and(wWarioSlopeInteraction)) {
			bool right_slope = alu_.bit(cpu_.a, RIGHT_SLOPE_F);
			uphill = right ? right_slope : !right_slope;
		}
		bool heavy = ld_bit(wGrabState, GRAB_HEAVY_F);
		ld_cp(wWalkVelIndex, uphill ? (heavy ? 0x04 : 0x08) : (heavy ? 0x08 : 0x10));
		if (!CF)
			st(wWalkVelIndex, uphill ? (heavy ? 0x00 : 0x04) : (heavy ? 0x04 : 0x0c));
	}

	void port_HandleInput_Attacking()
	{
		bool turn;
		if (facing_right())
			turn = ld_bit(wJoypadDown, B_PAD_LEFT);
		else
			turn = ld_bit(wJoypadDown, B_PAD_RIGHT);
		if (turn) {
			port_SetState_Turning();
			return;
		}
		// .check_pressed
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_StartJumpingAirborneAttack();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			port_SetState_CrouchSliding();
			return;
		}
	}

	void port_UpdateState_GrabIdling()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_IDLING)) {
			return;
		}
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		port_Func_1ee88();
		if (!still_in(WST_GRAB_IDLING)) {
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_GRAB_IDLING)) {
			return;
		}
		if (!b_and()) {
			port_Func_1edd3();
			return;
		}
		update_pos_y();
	}

	void port_UpdateState_GrabWalking()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_WALKING)) {
			return;
		}
		if (!ld_and(wGrabState)) {
			port_SetState_Walking_ResetWalkVel();
			return;
		}
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		port_Func_1eefc();
		if (!still_in(WST_GRAB_WALKING)) {
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			port_Func_1edd3();
			return;
		}
		update_pos_y();
	}

	void port_UpdateState_Attacking()
	{
		ld_cp(wPowerUpLevel, GARLIC);
		sfx_loop(6, CF ? SFX_ATTACK : SFX_ATTACK_POWERED_UP);
		anim(OAM1);
		port_HandleInput_Attacking();
		if (!still_in(WST_ATTACKING)) {
			return;
		}
		if (room_transition_check())
			return;
		if (!still_in(WST_ATTACKING)) {
			return;
		}
		FARCALL(CheckFrontCollision);
		if (!still_in(WST_ATTACKING)) {
			return;
		}
		if (b_low_nibble()) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			port_StartFallingAirborneAttack();
			return;
		}
		if (!still_in(WST_ATTACKING)) {
			return;
		}
		ld_cp(wAttackCounter, CHARGE_ATTACK_COUNTER);
		if (CF) {
			if (facing_right()) {
				CALL(ApplyWalkVelocity_Right);
				CALL(MoveWarioRight);
			} else {
				CALL(ApplyWalkVelocity_Left);
				CALL(MoveWarioLeft);
			}
			ld_cp(wWalkVelIndex, 0x14);
			if (!CF)
				st(wWalkVelIndex, 0x10);
		}
		// .check_collision
		FARCALL(CheckCentreCollision);
		update_pos_y();
		if (!still_in(WST_ATTACKING)) {
			return;
		}
		SET_HL(gb_, wAttackCounter);
		wr8(wAttackCounter, alu_.dec(rd8(wAttackCounter)));
		if (!ZF) {
			return;
		}
		CALL(IsOnSlipperyGround);
		if (!ZF) {
			FARCALL(SetState_Slipping);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_Walking_ResetWalkVel();
			return;
		}
		port_SetState_Idling();
	}

	// rst JumpTable with the table at `table` (right after the rst): leaves
	// every register as JumpTable does and returns the target (= hl).
	uint16_t ws1_jumptable(uint16_t table)
	{
		gb_push16(gb_, table); // rst JumpTable
		uint16_t hl = gb_pop16(gb_); // pop hl
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		hl = alu_.add16(hl, cpu_.e);
		cpu_.a = rd8(hl);
		cpu_.h = rd8((uint16_t)(hl + 1));
		cpu_.l = cpu_.a;
		return REG_HL(gb_);
	}

	// Dispatch on wWarioState (a): rst JumpTable, table right after it.
	// a (wWarioState, as UpdateWarioStates jumps here), as the asm's
	// jumptable
	void port_UpdateWarioStates_Group1()
	{
		switch (cpu_.a) {
		case 0x00: port_UpdateState_Idling(); return;
		case 0x01: port_UpdateState_Walking(); return;
		case 0x02: port_UpdateState_Turning(); return;
		case 0x03: CALL(InvalidWarioStateReset); return;
		case 0x04: port_UpdateState_Airborne(); return;
		case 0x05: port_UpdateState_Landing(); return;
		case 0x06: port_UpdateState_CrouchSliding(); return;
		case 0x07: port_UpdateState_Attacking(); return;
		case 0x08: port_UpdateState_AttackingAirborne(); return;
		case 0x09: port_UpdateState_Bumping(); return;
		case 0x0a: port_UpdateState_Diving(); return;
		case 0x0b: port_UpdateState_Submerged(); return;
		case 0x0c: port_UpdateState_WaterSurfaceIdling(); return;
		case 0x0d: port_UpdateState_WaterSurfaceMoving(); return;
		case 0x0e: port_UpdateState_UnderwaterThrusting(); return;
		case 0x0f: port_UpdateState_SwimKnockBack(); return;
		case 0x10: port_UpdateState_WaterStung(); return;
		case 0x11: port_UpdateState_TryingSubmerge(); return;
		case 0x12: port_UpdateState_Crouching(); return;
		case 0x13: port_UpdateState_CrouchWalking(); return;
		case 0x14: port_UpdateState_CrouchAirborne(); return;
		case 0x15: port_UpdateState_Stung(); return;
		case 0x16: port_UpdateState_StungRecovery(); return;
		case 0x17: port_UpdateState_PipeGoingDown(); return;
		case 0x18: port_UpdateState_PipeGoingUp(); return;
		case 0x19: port_UpdateState_EnemyBumping(); return;
		case 0x1a: port_UpdateState_SmashAttacking(); return;
		case 0x1b: CALL(InvalidWarioStateReset); return;
		case 0x1c: CALL(InvalidWarioStateReset); return;
		case 0x1d: CALL(InvalidWarioStateReset); return;
		case 0x1e: port_UpdateState_PickingUp(); return;
		case 0x1f: port_UpdateState_GrabIdling(); return;
		case 0x20: port_UpdateState_GrabWalking(); return;
		case 0x21: port_UpdateState_GrabAirborne(); return;
		case 0x22: port_UpdateState_ThrowCharging(); return;
		case 0x23: port_UpdateState_ThrowFullyCharged(); return;
		case 0x24: port_UpdateState_Throwing(); return;
		case 0x25: port_UpdateState_ThrowingAirborne(); return;
		case 0x26: port_UpdateState_GrabSmashAttacking(); return;
		case 0x27: port_UpdateState_Sliding(); return;
		case 0x28: port_UpdateState_Rolling(); return;
		case 0x29: port_UpdateState_RollingAirborne(); return;
		case 0x2a: port_UpdateState_PickedUp(); return;
		case 0x2b: port_UpdateState_GroundShakeStunned(); return;
		case 0x2c: port_UpdateState_EnteringDoor(); return;
		case 0x2d: CALL(InvalidWarioStateReset); return;
		case 0x2e: CALL(InvalidWarioStateReset); return;
		case 0x2f: CALL(InvalidWarioStateReset); return;
		case 0x30: CALL(InvalidWarioStateReset); return;
		case 0x31: CALL(InvalidWarioStateReset); return;
		case 0x32: CALL(InvalidWarioStateReset); return;
		case 0x33: CALL(InvalidWarioStateReset); return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	void port_UpdateWarioStates()
	{
		SET_HL(gb_, wCoinCooldown);
		cpu_.a = rd8(wCoinCooldown);
		if (alu_.and_(cpu_.a, cpu_.a))
			wr8(wCoinCooldown, alu_.dec(cpu_.a));
		port_HandleGroundShake();
		port_HandleInvincibility();

		cpu_.a = rd8(wWarioState);
		alu_.cp(cpu_.a, WARIO_STATES_GROUP_1);
		if (CF) {
			CALL(UpdateWarioStates_Group1);
			return;
		}
		alu_.cp(cpu_.a, WARIO_STATES_GROUP_2);
		if (CF) {
			// farcall UpdateWarioStates_Group2 / ret
			FARCALL_RET(UpdateWarioStates_Group2, ADDR_UpdateWarioStates + 0x4c);
			return;
		}
		alu_.cp(cpu_.a, WARIO_STATES_GROUP_3);
		if (CF) {
			FARCALL_RET(UpdateWarioStates_Group3, ADDR_UpdateWarioStates + 0x3c);
			return;
		}
		FARCALL_RET(UpdateWarioStates_Group4, ADDR_UpdateWarioStates + 0x2c);
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates1;

PORT_EX(Func_1e6ea, 0, 0) { WarioStates1(gb).port_Func_1e6ea(); }
PORT_COVERED(SetState_Turning);
PORT_COVERED(SetState_Attacking);
PORT_COVERED(SetState_Walking);
PORT_COVERED(SetState_Airborne);
PORT_COVERED(SetState_Diving);
PORT_EX(SetState_Submerged, 0, 0) { WarioStates1(gb).port_SetState_Submerged(); }
PORT_COVERED(SetState_WaterSurfaceIdling);
PORT_COVERED(SetState_Sliding);
PORT_COVERED(SetState_RollingAirborne);
PORT_COVERED(SetState_CrouchWalking);
PORT_COVERED(SetState_CrouchAirborne);
PORT_COVERED(Func_1f6c2);
PORT_COVERED(HandleWaterSurfaceFloatingMovement);
PORT_COVERED(StartJump);
PORT_COVERED(SetState_Walking_ResetWalkVel);
PORT_EX(CrouchOrSlide, 0, 0) { WarioStates1(gb).port_CrouchOrSlide(); }
PORT_EX(StartCrouchFall, 0, 0) { WarioStates1(gb).port_StartCrouchFall(); }
PORT_EX(StartCrouchJump, 0, 0) { WarioStates1(gb).port_StartCrouchJump(); }
PORT_COVERED(StartFall_SkipResetJumpUpwards);
PORT_EX(StartJump_FromInput, 0, 0) { WarioStates1(gb).port_StartJump_FromInput(); }
PORT_EX(DiveFromSurface_SkipSplash, 0, 0) { WarioStates1(gb).port_DiveFromSurface_SkipSplash(); }
PORT_COVERED(SurfaceFromUnderwater);
PORT_EX(StartFall, 0, 0) { WarioStates1(gb).port_StartFall(); }
PORT_COVERED(StartJump_FromWaterSurface);
PORT_EX(StartJump_FromSand, 0, 0) { WarioStates1(gb).port_StartJump_FromSand(); }
PORT_COVERED(DiveFromSurface);
PORT_COVERED(SetState_TryingSubmerge);
PORT_COVERED(UpdateState_TryingSubmerge);
PORT_COVERED(Func_1e1e3);
PORT_COVERED(HandleInput_Idling);
PORT_EX(Func_1e174, 0, 0) { WarioStates1(gb).port_Func_1e174(); }
PORT_EX(Func_1f863, 0, 0) { WarioStates1(gb).port_Func_1f863(); }
PORT_COVERED(SetState_Throwing);
PORT_COVERED(Func_1f3a7);
PORT_COVERED(SetState_SmashAttacking);
PORT_COVERED(SetState_LadderShakeStunned);
PORT_EX(StartDive, 0, 0) { WarioStates1(gb).port_StartDive(); }
PORT_COVERED(UpdateState_Idling);
PORT_COVERED(SetState_ThrowingAirborne);
PORT_COVERED(SetState_WaterSurfaceMoving);
PORT_COVERED(SetState_Rolling);
PORT_EX(SetState_SwimKnockBack, 0, 0) { WarioStates1(gb).port_SetState_SwimKnockBack(); }
PORT_COVERED(SetState_ThrowCharging);
PORT_COVERED(SetState_CrouchSliding);
PORT_COVERED(UpdateState_PickedUp);
PORT_COVERED(SetState_UnderwaterThrusting);
PORT_COVERED(StartUpwardsUnderwaterThrusting);
PORT_COVERED(SetState_SwimKnockBack_SwitchDirection);
PORT_COVERED(StartUnderwaterThrusting);
PORT_COVERED(Func_1de3f);
PORT_EX(DoGroundPound, 0, 0) { WarioStates1(gb).port_DoGroundPound(); }
PORT_EX(DoWarioGroundShake, 0, 0) { WarioStates1(gb).port_DoWarioGroundShake(); }
PORT_COVERED(HandleGroundShake);
PORT_EX(SetState_Idling, 0, 0) { WarioStates1(gb).port_SetState_Idling(); }
PORT_EX(WalkIfPossible_Right, 0, 0) { WarioStates1(gb).port_WalkIfPossible_Right(); }
PORT_EX(WalkIfPossible_Left, 0, 0) { WarioStates1(gb).port_WalkIfPossible_Left(); }
PORT_COVERED(Func_1ec19);
PORT_COVERED(Func_1ec4b);
PORT_COVERED(HandleWaterSurfaceInput);
PORT_COVERED(HandleBumpMovement);
PORT_COVERED(SetState_Bumping);
PORT_COVERED(HandleDivingHorizontalMovement);
PORT_COVERED(DoHardLand);
PORT_COVERED(HandleInvincibility);
PORT_COVERED(DoSoftLand);
PORT_EX(WalkRight, 0, 0) { WarioStates1(gb).port_WalkRight(); }
PORT_EX(WalkLeft, 0, 0) { WarioStates1(gb).port_WalkLeft(); }
PORT_COVERED(DoBumpOrJumpingBump);
PORT_EX(DoJumpingBump_NoSFX, 0, 0) { WarioStates1(gb).port_DoJumpingBump_NoSFX(); }
PORT_COVERED(UpdateState_Turning);
PORT_COVERED(Func_1eb46);
PORT_COVERED(UpdateState_Diving);
PORT_COVERED(Func_1f825);
PORT_COVERED(UpdateState_StungRecovery);
PORT_COVERED(UpdateState_PipeGoingDown);
PORT_COVERED(UpdateState_PipeGoingUp);
PORT_COVERED(UpdateState_Landing);
PORT_COVERED(HandleInput_Walking);
PORT_COVERED(Func_1ec6c);
PORT_EX(SetState_GrabIdling, 0, 0) { WarioStates1(gb).port_SetState_GrabIdling(); }
PORT_COVERED(DoBumpWithBigBackOffset);
PORT_COVERED(DoBumpWithSmallBackOffset);
PORT_EX(DoJumpingBump, 0, 0) { WarioStates1(gb).port_DoJumpingBump(); }
PORT_COVERED(UpdateState_Crouching);
PORT_COVERED(UpdateState_Bumping);
PORT_COVERED(UpdateState_Stung);
PORT_COVERED(UpdateState_EnemyBumping);
PORT_COVERED(UpdateState_EnteringDoor);
PORT_COVERED(ApplyLastWaterCurrentMovement);
PORT_COVERED(HandleInput_Airborne);
PORT_COVERED(UpdateState_SmashAttacking);
PORT_COVERED(UpdateState_Throwing);
PORT_COVERED(UpdateState_ThrowingAirborne);
PORT_COVERED(Func_1dd78);
PORT_COVERED(UpdateState_SwimKnockBack);
PORT_COVERED(UpdateState_WaterStung);
PORT_COVERED(Func_1f310);
PORT_COVERED(Func_1f3d9);
PORT_COVERED(ApplyWaterCurrentMovement);
PORT_COVERED(HandleAttackingAirborneMovement);
PORT_COVERED(UpdateState_Walking);
PORT_COVERED(UpdateState_Airborne);
PORT_COVERED(UpdateState_CrouchWalking);
PORT_COVERED(UpdateState_CrouchAirborne);
PORT_COVERED(UpdateState_Sliding);
PORT_COVERED(UpdateState_Rolling);
PORT_COVERED(UpdateState_RollingAirborne);
PORT_COVERED(UpdateState_PickingUp);
PORT_COVERED(UpdateState_ThrowCharging);
PORT_COVERED(UpdateState_ThrowFullyCharged);
PORT_COVERED(UpdateState_GrabSmashAttacking);
PORT_COVERED(UpdateState_GroundShakeStunned);
PORT_COVERED(UpdateState_CrouchSliding);
PORT_COVERED(UpdateState_GrabAirborne);
PORT_COVERED(Func_1f357);
PORT_COVERED(HandleUnderwaterThrustingInput);
PORT_COVERED(UpdateState_AttackingAirborne);
PORT_COVERED(UpdateState_WaterSurfaceIdling);
PORT_COVERED(UpdateState_WaterSurfaceMoving);
PORT_COVERED(UpdateState_UnderwaterThrusting);
PORT_COVERED(SetState_GrabAirborne);
PORT_COVERED(Func_1f24c);
PORT_COVERED(SetState_AttackingAirborne);
PORT_EX(Func_1edd3, 0, 0) { WarioStates1(gb).port_Func_1edd3(); }
PORT_EX(Func_1ede9, 0, 0) { WarioStates1(gb).port_Func_1ede9(); }
PORT_COVERED(Func_1f1a9);
PORT_COVERED(StartFallingAirborneAttack);
PORT_COVERED(StartJumpingAirborneAttack);
PORT_COVERED(UpdateState_Submerged);
PORT_COVERED(Func_1ee88);
PORT_COVERED(Func_1eefc);
PORT_COVERED(HandleInput_Attacking);
PORT_COVERED(UpdateState_GrabIdling);
PORT_COVERED(UpdateState_GrabWalking);
PORT_COVERED(UpdateState_Attacking);
PORT_EX(UpdateWarioStates_Group1, 0, ALL_REGISTERS) { WarioStates1(gb).port_UpdateWarioStates_Group1(); }
// update_level farcalls it and reloads every register it uses after
PORT_EX(UpdateWarioStates, 0, ALL_REGISTERS) { WarioStates1(gb).port_UpdateWarioStates(); }
