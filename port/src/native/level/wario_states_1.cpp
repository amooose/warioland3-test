// Wario's states, part 1 (src/engine/level/wario_states_1.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "asm_module.hpp"

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
		LJUMP(Func_1e6ea); // fallthrough
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
		LJUMP(SetState_Airborne); // fallthrough
	}

	void port_SetState_Walking_ResetWalkVel()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		LJUMP(SetState_Walking); // fallthrough
	}

	void port_CrouchOrSlide()
	{
		if (ld_and(wWarioSlopeInteraction)) {
			LJUMP(SetState_Sliding);
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
			LJUMP(SetState_CrouchWalking);
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
		RET();
	}

	void port_StartCrouchFall()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_CrouchAirborne);
	}

	void port_StartCrouchJump()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		LJUMP(SetState_CrouchAirborne); // fallthrough
	}

	void port_StartFall_SkipResetJumpUpwards()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(StartJump); // jr
	}

	void port_StartJump_FromInput()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		st(wJumpingUpwards, TRUE);
		LJUMP(StartJump); // fallthrough
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
		LJUMP(SetState_Diving); // jr
	}

	void port_SurfaceFromUnderwater()
	{
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWaterSurfaceFloatingCounter, 0);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		LJUMP(SetState_WaterSurfaceIdling); // fallthrough
	}

	void port_StartFall()
	{
		xor_a();
		wr8(wJumpingUpwards, FALSE);
		LJUMP(StartFall_SkipResetJumpUpwards); // fallthrough
	}

	void port_StartJump_FromWaterSurface()
	{
		play_sfx(SFX_SPLASH);
		st(wJumpVelIndex, 0x01);
		st(wJumpingUpwards, TRUE);
		st(wDoFullJump, TRUE);
		LJUMP(StartJump); // jr
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
		LJUMP(SetState_Airborne); // jr
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
		LJUMP(DiveFromSurface_SkipSplash); // fallthrough
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
			LJUMP(SetState_WaterSurfaceIdling);
			return;
		}
		anim(OAM1);
		RET();
	}

	void port_Func_1e1e3()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		LJUMP(SetState_RollingAirborne); // jr
	}

	void port_HandleInput_Idling()
	{
		uint8_t pressed = ld(wJoypadPressed);
		if (alu_.bit(pressed, B_PAD_B)) {
			LJUMP(SetState_Attacking);
			return;
		}
		if (alu_.bit(pressed, B_PAD_A)) {
			LJUMP(StartJump_FromInput);
			return;
		}
		uint8_t down = ld(wJoypadDown);
		bool right;
		if (alu_.bit(down, B_PAD_RIGHT))
			right = true;
		else if (alu_.bit(down, B_PAD_LEFT))
			right = false;
		else {
			RET();
			return;
		}
		// walk if already facing that way, else turn
		if (ld_and(wDirection) == right)
			LJUMP(SetState_Walking_ResetWalkVel);
		else
			LJUMP(SetState_Turning);
	}

	void port_Func_1e174()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_RollingAirborne); // fallthrough
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
		LJUMP(SetState_Diving); // fallthrough
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
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(StartFall);
			return;
		}
		update_pos_y();
		if (!still_idling()) {
			RET();
			return;
		}

		if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
			FARCALL(Func_19b12);
			if (!still_idling()) {
				RET();
				return;
			}
			ld_cp(wLadderInteraction, LADDER_UNDERNEATH);
			if (!ZF) {
				LJUMP(CrouchOrSlide);
				return;
			}
			FARCALL(SetState_LadderClimbing);
			RET();
			return;
		}

		anim(OAM1);
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wLadderInteraction) && ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_LadderClimbing);
			RET();
			return;
		}
		if (ld_and(wIsNearFence) && ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_FenceMovingVertical);
			RET();
			return;
		}
		if (!still_idling()) {
			RET();
			return;
		}
		LCALL(HandleInput_Idling);
		if (!still_idling() || ld_and(wIsOnSteppableObject)) {
			RET();
			return;
		}
		cpu_.a = alu_.add(rd8(wWarioStateCounter), 1);
		wr8(wWarioStateCounter, cpu_.a);
		cpu_.a = alu_.adc(rd8(wWarioStateCycles), 0);
		wr8(wWarioStateCycles, cpu_.a);
		alu_.cp(cpu_.a, NUM_SLEEPING_IDLE_CYCLES);
		if (!CF)
			FARCALL(SetState_Sleeping);
		RET();
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
				RET();
				return;
			}
			st(wPickedUpFrameCounter, MAX_PICKEB_PAD_UPRAME_COUNTER);
			st(wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
			RET();
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
			RET();
			return;
		}
		xor_a();
		wr8(wAutoMoveState, 0);
		LJUMP(StartFall);
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
		LJUMP(SetState_UnderwaterThrusting);
	}

	void port_SetState_SwimKnockBack_SwitchDirection()
	{
		flip_dir();
		LJUMP(SetState_SwimKnockBack); // fallthrough
	}

	void port_StartUnderwaterThrusting()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_CTRL_PAD);
		wr8(wSwimmingDirectionInput, cpu_.a);
		LJUMP(SetState_UnderwaterThrusting); // fallthrough
	}

	void port_Func_1de3f()
	{
		reset_anim();
		frameset_by_dir(ADDR_Frameset_163fe, ADDR_Frameset_1641e);
		anim(OAM1);
		cpu_.a = alu_.add(alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK), GRAB_THROW);
		wr8(wGrabState, cpu_.a);
		LJUMP(SetState_Throwing); // fallthrough
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
		LJUMP(SetState_SmashAttacking); // fallthrough
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
		LJUMP(SetState_SmashAttacking);
	}

	void port_HandleGroundShake()
	{
		if (ld_and(wLevelEndScreen)) {
			RET();
			return;
		}
		SET_HL(gb_, wGroundShakeCounter);
		cpu_.a = rd8(wGroundShakeCounter);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			xor_a();
			wr8(wIsWarioGroundShaking, 0);
			RET();
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
			RET();
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
			LJUMP(SetState_LadderShakeStunned);
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
		RET();
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
			RET();
			return;
		}
		if (b_and()) {
			LJUMP(CrouchOrSlide);
			return;
		}
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
		RET();
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
				RET();
				return;
			}
		}
		if (!b_and()) {
			LJUMP(StartCrouchJump);
			return;
		}
		xor_a();
		wr8(wJumpVelTable, 0);
		st(wCollisionBoxTop, (uint8_t)-15);
		RET();
	}

	void port_Func_1ec4b()
	{
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (!b_and()) {
			LJUMP(SetState_Attacking);
			return;
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		RET();
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
			LJUMP(StartJump_FromWaterSurface);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
			if (!CF) {
				if (ld_bit(wJoypadDown, B_PAD_UP)) {
					RET();
					return;
				}
				if (non_submersible()) {
					LJUMP(SetState_TryingSubmerge);
					return;
				}
				cpu_.b = 3;
				CALL(MoveWarioDown);
				LJUMP(StartUnderwaterThrusting);
				return;
			}
		}
		// .no_thrust
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			RET();
			return;
		}
		ld_cp(wPowerUpLevel, SWIMMING_FLIPPERS);
		if (CF || non_submersible()) {
			LJUMP(SetState_TryingSubmerge);
			return;
		}
		LJUMP(SetState_Submerged);
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
				LJUMP(StartFall);
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
		RET();
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
				LJUMP(DoGroundPound);
			else
				LJUMP(DoWarioGroundShake);
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_DOWN);
		if (cpu_.a) {
			LJUMP(CrouchOrSlide);
			return;
		}
		play_sfx(SFX_LAND);
		st(wWarioState, WST_LANDING);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f9a, ADDR_Frameset_15fa1);
		anim(OAM1);
		RET();
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
				LJUMP(DoGroundPound);
			else
				LJUMP(DoWarioGroundShake);
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_DOWN);
		if (cpu_.a) {
			LJUMP(CrouchOrSlide);
			return;
		}
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (b_and()) {
			LJUMP(CrouchOrSlide);
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			LJUMP(SetState_Walking);
		else
			LJUMP(SetState_Idling);
	}

	void port_WalkRight()
	{
		xor_a();
		wr8(wIsTurning, 0);
		if (!facing_right()) {
			st(wDirection, DIRECTION_RIGHT);
			st(wIsTurning, TRUE);
		}
		LJUMP(WalkIfPossible_Right);
	}

	void port_WalkLeft()
	{
		xor_a();
		wr8(wIsTurning, 0);
		if (facing_right()) {
			st(wDirection, DIRECTION_LEFT);
			st(wIsTurning, TRUE);
		}
		LJUMP(WalkIfPossible_Left);
	}

	void port_DoBumpOrJumpingBump()
	{
		play_sfx(SFX_BUMP);
		st(wJumpVelIndex, ld_and(wJumpVelTable) ? FALLING_JUMP_VEL_INDEX : 10);
		LJUMP(SetState_Bumping);
	}

	void port_DoJumpingBump_NoSFX()
	{
		st(wJumpVelIndex, 10);
		LJUMP(SetState_Bumping); // fallthrough
	}

	void port_UpdateState_Turning()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(StartJump_FromInput);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(SetState_Attacking);
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			LJUMP(SetState_Walking_ResetWalkVel);
		else
			LJUMP(SetState_Idling);
	}

	void port_Func_1eb46()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_1ec19);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(Func_1ec4b);
			return;
		}
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_CROUCHING);
			if (!ZF) {
				RET();
				return;
			}
			if (!b_and()) {
				LJUMP(SetState_Idling);
				return;
			}
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (!cpu_.a) {
			RET();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else
			st(wDirection, DIRECTION_LEFT);
		LJUMP(SetState_CrouchWalking);
	}

	void port_UpdateState_Diving()
	{
		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_DIVING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and()) {
			LCALL(HandleDivingHorizontalMovement);
			cpu_.a = inc_counter();
			alu_.cp(cpu_.a, 10);
			if (CF) {
				cpu_.b = 2;
			} else {
				alu_.cp(cpu_.a, 14);
				if (!CF) {
					LJUMP(SetState_Submerged);
					return;
				}
				cpu_.b = 1;
			}
			CALL(MoveWarioDown);
			FARCALL(CheckCentreCollision);
			if (!b_and()) {
				RET();
				return;
			}
		}
		// .submerge
		update_pos_y();
		LJUMP(SetState_Submerged); // fallthrough
	}

	void port_Func_1f825()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			xor_a();
			wr8(wIsRolling, 0);
			LJUMP(StartJump_FromInput);
			return;
		}
		bool right = facing_right();
		if (ld_bit(wJoypadPressed, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
			xor_a();
			wr8(wIsRolling, 0);
			LJUMP(SetState_Idling);
			return;
		}
		if (right)
			LCALL(WalkIfPossible_Right);
		else
			LCALL(WalkIfPossible_Left);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		RET();
	}

	void port_UpdateState_StungRecovery()
	{
		LCALL(Func_1f6c2);
		anim(OAM1);
		cpu_.a = inc_counter();
		alu_.cp(cpu_.a, 0x14);
		if (CF) {
			RET();
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
			RET();
			return;
		}
		if (!b_and()) {
			if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
				LJUMP(SetState_Idling);
				return;
			}
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		LJUMP(CrouchOrSlide);
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
			JUMP(TriggerRoomTransition);
			return true;
		}
		alu_.cp(cpu_.a, 0x21);
		if (ZF) {
			play_sfx(SFX_PIPE_EXIT);
			RET();
			return true;
		}
		alu_.cp(cpu_.a, 0x40);
		if (CF) {
			RET();
			return true;
		}
		cpu_.a = alu_.sub(rd8(wWarioXPos + 1), 0x08);
		wr8(wWarioXPos + 1, cpu_.a);
		xor_a();
		wr8(wIsIntangible, 0);
		if (up)
			CALL(UpdateFloor);
		LJUMP(SetState_Idling);
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
			LJUMP(SetState_Walking_ResetWalkVel);
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		bool jump = false;
		if (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A))
			jump = true;
		if (!jump && !ld_bit(wJoypadPressed, B_PAD_A)) {
			if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
				LJUMP(CrouchOrSlide);
				return;
			}
			LJUMP(SetState_Idling);
			return;
		}
		LCALL(StartJump_FromInput);
		ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
		if (!CF)
			st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		RET();
	}

	void port_HandleInput_Walking()
	{
		if (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A)) {
			LJUMP(StartJump_FromInput);
			return;
		}
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(SetState_Attacking);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(StartJump_FromInput);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
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
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
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
		bool facing = facing_right();
		if (facing != right) {
			LJUMP(SetState_Turning);
			return;
		}
		st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
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
		ld_cp(wWalkVelIndex, 0x10);
		if (!CF)
			st(wWalkVelIndex, 0x0c);
		RET();
	}

	void port_Func_1ec6c()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_1ec19);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(Func_1ec4b);
			return;
		}
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_CROUCH_WALKING);
			if (!ZF) {
				RET();
				return;
			}
			if (!b_and()) {
				LJUMP(SetState_Idling);
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
			LJUMP(CrouchOrSlide);
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
		ld_cp(wWalkVelIndex, 0x04);
		if (!CF) {
			cpu_.a = 0;
			wr8(wWalkVelIndex, 0);
		}
		RET();
	}

	void port_SetState_GrabIdling()
	{
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Idling);
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
		RET();
	}

	void port_DoBumpWithBigBackOffset()
	{
		cpu_.b = 3;
		if (facing_right())
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
		LJUMP(DoBumpOrJumpingBump);
	}

	void port_DoBumpWithSmallBackOffset()
	{
		cpu_.b = 2;
		if (facing_right())
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
		LJUMP(DoBumpOrJumpingBump);
	}

	void port_DoJumpingBump()
	{
		play_sfx(SFX_BUMP);
		LJUMP(DoJumpingBump_NoSFX); // fallthrough
	}

	void port_UpdateState_Crouching()
	{
		FARCALL(Func_19b25);
		ld_cp(wWarioState, WST_CROUCHING);
		if (!ZF) {
			RET();
			return;
		}
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		LCALL(Func_1eb46);
		ld_cp(wWarioState, WST_CROUCHING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(StartCrouchFall);
			return;
		}
		RET();
	}

	// farcall Func_19b25, then the room transition, water and sand checks of
	// UpdateState_Bumping/Stung; true if it jumped
	bool bump_stung_start()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return true;
		}
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return true;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			RET();
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
			RET();
			return;
		}
		LCALL(HandleBumpMovement);
		ld_cp(wWarioState, WST_BUMPING);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_BUMPING);
			if (ZF && b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		update_pos_y();
		flip_dir();
		ld_cp(wWarioState, WST_BUMPING);
		if (!ZF) {
			RET();
			return;
		}
		LJUMP(DoSoftLand);
	}

	void port_UpdateState_Stung()
	{
		if (bump_stung_start())
			return;
		LCALL(Func_1f6c2);
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
				RET();
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
					RET();
					return;
				}
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			}
			FARCALL(CheckAirborneCollision);
			if (!b_and()) {
				JUMP(TriggerDownwardsFloorTransition);
				return;
			}
			CALL(TriggerFloorTransition);
			cpu_.a = alu_.inc(rd8(wJumpVelTable));
			wr8(wJumpVelTable, cpu_.a);
			cpu_.a = alu_.sub(cpu_.a, 0x02);
			if (ZF) {
				st(wJumpVelIndex, 0x0a);
				RET();
				return;
			}
			cpu_.a = alu_.dec(cpu_.a);
			if (ZF) {
				st(wJumpVelIndex, 0x0e);
				RET();
				return;
			}
			recover = true;
		}
		(void)recover;
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		st(wWarioState, WST_STUNG_RECOVERY);
		RET();
	}

	void port_UpdateState_EnemyBumping()
	{
		if (bump_stung_start())
			return;
		ld_cp(wWarioState, WST_ENEMY_BUMPING);
		if (!ZF) {
			RET();
			return;
		}
		anim(OAM1);
		LCALL(HandleBumpMovement);
		ld_cp(wWarioState, WST_ENEMY_BUMPING);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (b_and())
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		flip_dir();
		update_pos_y();
		LJUMP(DoSoftLand);
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
				JUMP(TriggerRoomTransition);
				return;
			}
			RET();
			return;
		}
		cpu_.a = alu_.dec(cpu_.a);
		if (ZF) {
			SET_HL(gb_, wWarioStateCounter);
			wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
			RET();
			return;
		}
		cpu_.a = alu_.dec(cpu_.a);
		if (ZF) {
			if (!anim_ended()) {
				RET();
				return;
			}
			if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
				LJUMP(SetState_Walking_ResetWalkVel);
				return;
			}
			LJUMP(SetState_Idling);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		reset_anim();
		set_frameset(ADDR_Frameset_14d15);
		anim(OAM1);
		SET_HL(gb_, wWarioStateCounter);
		wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
		RET();
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
			LCALL(WalkRight);
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			LCALL(WalkLeft);
		} else {
			RET();
			return;
		}
		JUMP(Func_1762);
	}

	void port_UpdateState_SmashAttacking()
	{
		anim(OAM1);
		alu_.cp(inc_counter(), 0x1b);
		if (CF) {
			RET();
			return;
		}
		xor_a();
		wr8(wIsSmashAttacking, 0);
		LJUMP(SetState_Idling);
	}

	// the shared body of UpdateState_Throwing/ThrowingAirborne
	void throwing(uint16_t done)
	{
		if (!ld_and(wGrabState)) {
			JUMP_TO(done);
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		xor_a();
		wr8(wGrabState, 0);
		JUMP_TO(done);
	}

	void port_UpdateState_Throwing() { throwing(ADDR_SetState_Idling); }
	void port_UpdateState_ThrowingAirborne() { throwing(ADDR_StartFall); }

	void port_Func_1dd78()
	{
		xor_a();
		wr8(wGrabState, 0);
		LJUMP(StartJump_FromInput);
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
				RET();
				return;
			}
		}
		flip_dir();
		LJUMP(SetState_Submerged);
	}

	// xor a / ld [wIsIntangible], a / ld a, $10 / ld [wInvincibleCounter], a / jp
	void water_stung_end(uint16_t to)
	{
		xor_a();
		wr8(wIsIntangible, 0);
		st(wInvincibleCounter, 0x10);
		JUMP_TO(to);
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
				RET();
				return;
			}
			st(wc0e1, PAD_UP);
			water_stung_end(ADDR_SetState_Submerged);
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			ld_cp(wWarioStateCounter, 0x78);
			if (CF) {
				RET();
				return;
			}
			water_stung_end(ADDR_SurfaceFromUnderwater);
			return;
		}
		ld_cp(wWarioStateCounter, 0x78);
		if (!CF) {
			water_stung_end(ADDR_SetState_Submerged);
			return;
		}
		if (ld_mask(wGlobalCounter, 1)) {
			RET();
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
		if (!ld_and(wWaterCurrent)) {
			RET();
			return;
		}
		xor_a();
		wr8(wWaterCurrent, 0);
		water_stung_end(ADDR_SetState_Submerged);
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
			RET();
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			LJUMP(SurfaceFromUnderwater);
			return;
		}
		if (water_current_mask(CURRENT_UP | CURRENT_DOWN)) {
			RET();
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
		RET();
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
		LCALL(ApplyLastWaterCurrentMovement);
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
			JUMP(TriggerRoomTransition);
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
		LCALL(HandleInput_Walking);
		if (!still_in(WST_WALKING)) {
			RET();
			return;
		}
		if (room_transition_check())
			return;
		if (!still_in(WST_WALKING)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_WALKING)) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(StartFall);
			return;
		}
		update_pos_y();
		RET();
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
				LJUMP(DiveFromSurface);
				return;
			}
			if (ld_and(wIsInSand)) {
				FARCALL(SetState_SandFalling);
				RET();
				return;
			}
			if (ld_and(wLadderInteraction) && ld_bit(wJoypadDown, B_PAD_UP)) {
				FARCALL(SetState_LadderClimbing);
				RET();
				return;
			}
			if (!still_in(WST_AIRBORNE)) {
				RET();
				return;
			}
			FARCALL(Func_19afb);
			if (ld_and(wIsNearFence) && ld_bit(wJoypadDown, B_PAD_UP)) {
				FARCALL(SetState_FenceMovingVertical);
				RET();
				return;
			}
		}
		// .handle_input
		if (!still_in(WST_AIRBORNE)) {
			RET();
			return;
		}
		LCALL(HandleInput_Airborne);
		if (!still_in(WST_AIRBORNE)) {
			RET();
			return;
		}

		static const AirborneFramesets f = {ADDR_Frameset_15f94, ADDR_Frameset_15f97, ADDR_Frameset_15f8e,
			ADDR_Frameset_15f91, ADDR_Frameset_1604e, ADDR_Frameset_16055};
		airborne_smash_attack(&f);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_AIRBORNE)) {
				RET();
				return;
			}
			if (ld_and(wJumpCollision)) {
				LJUMP(StartFall_SkipResetJumpUpwards);
				return;
			}
			center_on_ladder();
			RET();
			return;
		}
		// .falling
		FARCALL(CheckAirborneCollision);
		if (!still_in(WST_AIRBORNE)) {
			RET();
			return;
		}
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		update_pos_y();
		dust();
		ld_cp(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
		if (CF) {
			LJUMP(DoSoftLand);
			return;
		}
		if (!ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(DoHardLand);
			return;
		}
		LJUMP(DoSoftLand); // fallthrough
	}

	void port_UpdateState_CrouchWalking()
	{
		sfx_loop(21, SFX_CROUCH_WALK);
		anim(OAM1);
		if (room_transition_check())
			return;
		if (!still_in(WST_CROUCH_WALKING)) {
			RET();
			return;
		}
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		LCALL(Func_1ec6c);
		if (!still_in(WST_CROUCH_WALKING)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(StartCrouchFall);
			return;
		}
		update_pos_y();
		if (ld_and(wWarioSlopeInteraction)) {
			LJUMP(SetState_Sliding);
			return;
		}
		RET();
	}

	void port_UpdateState_CrouchAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			RET();
			return;
		}
		if (!still_in(WST_CROUCH_AIRBORNE)) {
			RET();
			return;
		}
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (!still_in(WST_CROUCH_AIRBORNE)) {
				RET();
				return;
			}
			if (!b_and()) {
				LJUMP(StartJump);
				return;
			}
		}
		// .asm_1d583
		st(wCollisionBoxTop, (uint8_t)-15);
		LCALL(HandleInput_Airborne);
		if (!still_in(WST_CROUCH_AIRBORNE)) {
			RET();
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
			RET();
			return;
		}
		// .falling
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			if (!still_in(WST_CROUCH_AIRBORNE)) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		update_pos_y();
		if (!still_in(WST_CROUCH_AIRBORNE)) {
			RET();
			return;
		}
		LCALL(DoSoftLand);
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			RET();
			return;
		}
		LJUMP(CrouchOrSlide);
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
		LCALL(Func_1f825);
		if (!still_in(WST_SLIDING)) {
			RET();
			return;
		}
		FARCALL(Func_198c7);
		if (!b_and()) {
			LJUMP(Func_1e174);
			return;
		}
		update_pos();
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			LJUMP(DoBumpWithSmallBackOffset);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_Rolling); // fallthrough
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
				LJUMP(DoBumpWithSmallBackOffset);
				return;
			}
			LJUMP(Func_1e1e3);
			return;
		}
		roll_move();
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			LJUMP(DoBumpWithBigBackOffset);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_1e174);
			return;
		}
		update_pos_y();
		if (ld_and(wWarioSlopeInteraction) && against_slope()) {
			LJUMP(DoBumpWithSmallBackOffset);
			return;
		}
		RET();
	}

	void port_UpdateState_RollingAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			RET();
			return;
		}
		anim(OAM1);
		roll_move();
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
			LJUMP(DoBumpWithBigBackOffset);
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
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		LJUMP(SetState_Rolling);
	}

	void port_UpdateState_PickingUp()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_1dd78);
			return;
		}
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Idling);
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		alu_.cp(cpu_.a, 0x02);
		if (ZF) {
			LJUMP(SetState_GrabIdling);
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_GrabIdling);
	}

	void port_UpdateState_ThrowCharging()
	{
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Idling);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (!alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(Func_1de3f);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_1dd78);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			LJUMP(CrouchOrSlide);
			return;
		}
		cpu_.b = ld_bit(wGrabState, GRAB_HEAVY_F) ? THROW_CHARGE_FRAMES_HEAVY : THROW_CHARGE_FRAMES_LIGHT;
		cpu_.a = alu_.inc(rd8(wWarioStateCounter));
		wr8(wWarioStateCounter, cpu_.a);
		alu_.cp(cpu_.a, cpu_.b);
		if (CF) {
			RET();
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
		RET();
	}

	void port_UpdateState_ThrowFullyCharged()
	{
		sfx_loop(30, SFX_FULLY_CHARGED_THROW);
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Idling);
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
				LJUMP(Func_1dd78);
				return;
			}
			if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
				LJUMP(CrouchOrSlide);
				return;
			}
			RET();
			return;
		}
		reset_anim();
		frameset_by_dir(ADDR_Frameset_16403, ADDR_Frameset_16423);
		anim(OAM1);
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
		cpu_.a = alu_.add(cpu_.a, GRAB_THROW | (1 << GRAB_FULL_CHARGE_F));
		wr8(wGrabState, cpu_.a);
		LJUMP(SetState_Throwing);
	}

	void port_UpdateState_GrabSmashAttacking()
	{
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Idling);
			return;
		}
		anim(OAM1);
		alu_.cp(inc_counter(), 0x1b);
		if (CF) {
			RET();
			return;
		}
		xor_a();
		wr8(wIsSmashAttacking, FALSE);
		LJUMP(SetState_GrabIdling);
	}

	void port_UpdateState_GroundShakeStunned()
	{
		anim(OAM1);
		if (!ld_and(wGroundShakeCounter)) {
			// .no_more_ground_shake
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (b_and()) {
				LJUMP(CrouchOrSlide);
				return;
			}
			FARCALL(CheckCentreCollision);
			if (!b_and()) {
				LJUMP(StartFall);
				return;
			}
			LJUMP(SetState_Idling);
			return;
		}
		if (!ld_and(wJumpVelTable)) {
			RET();
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
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		RET();
	}

	// UpdateState_CrouchSliding.end_slide
	void crouch_slide_end()
	{
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (!still_in(WST_CROUCH_SLIDING)) {
				RET();
				return;
			}
			if (!b_and()) {
				LJUMP(SetState_Idling);
				return;
			}
		}
		// .crouching
		st(wCollisionBoxTop, (uint8_t)-15);
		LJUMP(CrouchOrSlide);
	}

	void port_UpdateState_CrouchSliding()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_CROUCH_SLIDING)) {
			RET();
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			// .try_jump
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (!still_in(WST_CROUCH_SLIDING)) {
				RET();
				return;
			}
			bool blocked = b_and();
			st(wCollisionBoxTop, (uint8_t)-15);
			if (blocked) {
				RET();
				return;
			}
			LJUMP(StartCrouchJump);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_CROUCH_SLIDING)) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(StartCrouchFall);
			return;
		}
		if (ld_and(wWarioSlopeInteraction)) {
			update_pos_y();
			LJUMP(SetState_Sliding);
			return;
		}
		FARCALL(CheckFrontCollision);
		if (!still_in(WST_CROUCH_SLIDING)) {
			RET();
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
						RET();
						return;
					}
					step = 1;
				} else if (ZF) {
					crouch_slide_end();
					return;
				} else {
					RET();
					return;
				}
			}
		}
		cpu_.b = step;
		move_by_dir();
		RET();
	}

	void port_UpdateState_GrabAirborne()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_AIRBORNE)) {
			RET();
			return;
		}
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		if (!ld_and(wGrabState)) {
			LJUMP(StartFall);
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			RET();
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_B)) {
			LJUMP(SetState_ThrowingAirborne);
			return;
		}
		LCALL(HandleInput_Airborne);
		if (!still_in(WST_GRAB_AIRBORNE)) {
			RET();
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
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!still_in(WST_GRAB_AIRBORNE)) {
			RET();
			return;
		}
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
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
			LJUMP(SetState_GrabIdling);
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
		RET();
	}

	void port_Func_1f357()
	{
		FARCALL(CheckCentreCollision);
		if (b_and())
			update_pos_y();
		if (still_in(WST_WATER_STUNG)) {
			RET();
			return;
		}
		if (!ld_and(wDirection)) {
			LJUMP(Func_1f310);
			return;
		}
		st(wDirection, DIRECTION_LEFT);
		reset_anim();
		set_frameset(ADDR_Frameset_15576);
		anim(OAM1);
		RET();
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
			LJUMP(StartDive);
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			LJUMP(SurfaceFromUnderwater);
			return;
		}
		swim_vel(PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894);
		CALL(MoveWarioUp);
		RET();
	}

	// HandleUnderwaterThrustingInput.down
	void thrust_down()
	{
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			update_pos_y();
			set_c0e1(B_PAD_DOWN);
			RET();
			return;
		}
		swim_vel(PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894);
		CALL(MoveWarioDown);
		RET();
	}

	// HandleUnderwaterThrustingInput.right / .left
	void thrust_horizontal(bool right)
	{
		FARCALL(CheckCentreCollision);
		if (b_and())
			update_pos_y();
		if (still_in(WST_WATER_STUNG)) {
			RET();
			return;
		}
		if (right)
			FARCALL(Func_19741);
		else
			FARCALL(Func_197b1);
		if (b_and()) {
			LJUMP(SetState_SwimKnockBack_SwitchDirection);
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
			RET();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			// .diagonal_down
			FARCALL(CheckCentreCollision);
			if (!b_and()) {
				cpu_.b = 1;
				CALL(MoveWarioDown);
				RET();
				return;
			}
			update_pos_y();
			set_c0e1(B_PAD_DOWN);
			RET();
			return;
		}
		// diagonal up
		FARCALL(CheckUpCollision);
		if (b_and()) {
			set_c0e1(B_PAD_UP);
			RET();
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			LJUMP(SurfaceFromUnderwater);
			return;
		}
		cpu_.b = 1;
		CALL(MoveWarioUp);
		RET();
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
			LJUMP(SetState_Submerged);
			return;
		}
		if (!ld_mask(wJoypadDown, PAD_A | PAD_B)) {
			LJUMP(SetState_Submerged);
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
		else
			RET();
	}

	void port_UpdateState_AttackingAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(DiveFromSurface);
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			RET();
			return;
		}
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			RET();
			return;
		}
		sfx_loop(6, SFX_ATTACK);
		anim(OAM1);
		LCALL(HandleAttackingAirborneMovement);
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			RET();
			return;
		}
		FARCALL(CheckFrontCollision);
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			RET();
			return;
		}
		if (b_low_nibble()) {
			LJUMP(DoBumpWithSmallBackOffset);
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
			LJUMP(StartFall);
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_ATTACKING_AIRBORNE)) {
				RET();
				return;
			}
			ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			if (CF)
				st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!still_in(WST_ATTACKING_AIRBORNE)) {
			RET();
			return;
		}
		LJUMP(SetState_Idling);
	}

	void port_UpdateState_WaterSurfaceIdling()
	{
		xor_a();
		wr8(wc0e1, 0);
		FARCALL(Func_19b25);
		if (!still_in(WST_WATER_SURFACE_IDLING)) {
			RET();
			return;
		}
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		sfx_loop(28, SFX_WATER_SURFACE);
		anim(OAM1);
		LCALL(HandleWaterSurfaceFloatingMovement);
		LCALL(HandleWaterSurfaceInput);
		if (!still_in(WST_WATER_SURFACE_IDLING)) {
			RET();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_WaterSurfaceMoving);
			return;
		}
		LCALL(ApplyWaterCurrentMovement);
		RET();
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
			LCALL(SetState_WaterSurfaceIdling); // jp from a called routine
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
		LCALL(HandleWaterSurfaceFloatingMovement);
		LCALL(ApplyWaterCurrentMovement);
		water_surface_directional_input();
		LCALL(HandleWaterSurfaceInput);
		RET();
	}

	void port_UpdateState_UnderwaterThrusting()
	{
		xor_a();
		wr8(wc0e1, 0);
		if (room_transition_check())
			return;
		if (!still_in(WST_UNDERWATER_THRUSTING)) {
			RET();
			return;
		}
		sfx_loop(15, SFX_00F);
		anim(OAM1);
		LCALL(HandleUnderwaterThrustingInput);
		if (!still_in(WST_UNDERWATER_THRUSTING)) {
			RET();
			return;
		}
		LCALL(ApplyWaterCurrentMovement);
		RET();
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
		LCALL(UpdateState_GrabAirborne);
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
				LJUMP(Func_1f3d9);
				return;
			}
			if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
				LJUMP(Func_1f3a7);
				return;
			}
			RET();
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (!alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
				LJUMP(Func_1f357);
				return;
			}
			RET();
			return;
		}
		// .asm_1f2c0
		FARCALL(CheckCentreCollision);
		if (b_and())
			update_pos_y();
		if (still_in(WST_WATER_STUNG)) {
			RET();
			return;
		}
		if (ld_and(wDirection)) {
			LJUMP(Func_1f310);
			return;
		}
		st(wDirection, DIRECTION_RIGHT);
		reset_anim();
		set_frameset(ADDR_Frameset_1558b);
		anim(OAM1);
		RET();
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
		LCALL(UpdateState_AttackingAirborne);
	}

	void port_Func_1edd3()
	{
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_FLAGS_MASK);
		cpu_.a = alu_.add(cpu_.a, GRAB_IDLE);
		wr8(wGrabState, cpu_.a);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		LJUMP(SetState_GrabAirborne);
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
		LJUMP(SetState_GrabAirborne); // fallthrough
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
					LJUMP(StartUnderwaterThrusting);
					return;
				}
				if (alu_.bit(cpu_.a, B_PAD_A)) {
					LJUMP(StartUpwardsUnderwaterThrusting);
					return;
				}
				if (ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
					LJUMP(Func_1f24c);
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
			RET();
			return;
		}
		FARCALL(Func_19acd);
		if (!ld_and(wWaterInteraction)) {
			LJUMP(SurfaceFromUnderwater);
			return;
		}
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x1);
		if (!ZF || water_current_mask(CURRENT_UP | CURRENT_DOWN)) {
			RET();
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
		RET();
	}

	void port_StartFallingAirborneAttack()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_AttackingAirborne);
	}

	void port_StartJumpingAirborneAttack()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		st(wJumpingUpwards, TRUE);
		LJUMP(SetState_AttackingAirborne); // fallthrough
	}

	void port_UpdateState_Submerged()
	{
		xor_a();
		wr8(wc0e1, 0);
		if (room_transition_check())
			return;
		if (!still_in(WST_SUBMERGED)) {
			RET();
			return;
		}
		LCALL(ApplyWaterCurrentMovement);
		LCALL(Func_1f1a9);
		RET();
	}

	void port_Func_1ee88()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_1ede9);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(SetState_ThrowCharging);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			LJUMP(CrouchOrSlide);
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, PAD_RIGHT | PAD_LEFT);
		if (ZF) {
			RET();
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
		RET();
	}

	void port_Func_1eefc()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(SetState_ThrowCharging);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_1ede9);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			// .asm_1efaf
			CALL(IsOnSlipperyGround);
			if (ZF) {
				LJUMP(CrouchOrSlide);
				return;
			}
			FARCALL(SetState_CrouchSlipping);
			RET();
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
				RET();
				return;
			}
			LJUMP(SetState_GrabIdling);
			return;
		}
		if (facing_right() != right) {
			LJUMP(SetState_GrabIdling);
			return;
		}
		FARCALL(CheckFrontCollision);
		if (b_low_nibble()) {
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
		RET();
	}

	void port_HandleInput_Attacking()
	{
		bool turn;
		if (facing_right())
			turn = ld_bit(wJoypadDown, B_PAD_LEFT);
		else
			turn = ld_bit(wJoypadDown, B_PAD_RIGHT);
		if (turn) {
			LJUMP(SetState_Turning);
			return;
		}
		// .check_pressed
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(StartJumpingAirborneAttack);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			LJUMP(SetState_CrouchSliding);
			return;
		}
		RET();
	}

	void port_UpdateState_GrabIdling()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_IDLING)) {
			RET();
			return;
		}
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Idling);
			return;
		}
		LCALL(Func_1ee88);
		if (!still_in(WST_GRAB_IDLING)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_GRAB_IDLING)) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(Func_1edd3);
			return;
		}
		update_pos_y();
		RET();
	}

	void port_UpdateState_GrabWalking()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_WALKING)) {
			RET();
			return;
		}
		if (!ld_and(wGrabState)) {
			LJUMP(SetState_Walking_ResetWalkVel);
			return;
		}
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		LCALL(Func_1eefc);
		if (!still_in(WST_GRAB_WALKING)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_1edd3);
			return;
		}
		update_pos_y();
		RET();
	}

	void port_UpdateState_Attacking()
	{
		ld_cp(wPowerUpLevel, GARLIC);
		sfx_loop(6, CF ? SFX_ATTACK : SFX_ATTACK_POWERED_UP);
		anim(OAM1);
		LCALL(HandleInput_Attacking);
		if (!still_in(WST_ATTACKING)) {
			RET();
			return;
		}
		if (room_transition_check())
			return;
		if (!still_in(WST_ATTACKING)) {
			RET();
			return;
		}
		FARCALL(CheckFrontCollision);
		if (!still_in(WST_ATTACKING)) {
			RET();
			return;
		}
		if (b_low_nibble()) {
			LJUMP(DoBumpWithSmallBackOffset);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(StartFallingAirborneAttack);
			return;
		}
		if (!still_in(WST_ATTACKING)) {
			RET();
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
			RET();
			return;
		}
		SET_HL(gb_, wAttackCounter);
		wr8(wAttackCounter, alu_.dec(rd8(wAttackCounter)));
		if (!ZF) {
			RET();
			return;
		}
		CALL(IsOnSlipperyGround);
		if (!ZF) {
			FARCALL(SetState_Slipping);
			RET();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_Walking_ResetWalkVel);
			return;
		}
		LJUMP(SetState_Idling);
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
	void port_UpdateWarioStates_Group1()
	{
		JUMP_TO(ws1_jumptable(ADDR_UpdateWarioStates_Group1 + 1));
	}

	void port_UpdateWarioStates()
	{
		SET_HL(gb_, wCoinCooldown);
		cpu_.a = rd8(wCoinCooldown);
		if (alu_.and_(cpu_.a, cpu_.a))
			wr8(wCoinCooldown, alu_.dec(cpu_.a));
		LCALL(HandleGroundShake);
		LCALL(HandleInvincibility);

		cpu_.a = rd8(wWarioState);
		alu_.cp(cpu_.a, WARIO_STATES_GROUP_1);
		if (CF) {
			LJUMP(UpdateWarioStates_Group1);
			return;
		}
		alu_.cp(cpu_.a, WARIO_STATES_GROUP_2);
		if (CF) {
			// farcall UpdateWarioStates_Group2 / ret
			FARCALL_RET(UpdateWarioStates_Group2, ADDR_UpdateWarioStates + 0x4c);
			RET();
			return;
		}
		alu_.cp(cpu_.a, WARIO_STATES_GROUP_3);
		if (CF) {
			FARCALL_RET(UpdateWarioStates_Group3, ADDR_UpdateWarioStates + 0x3c);
			RET();
			return;
		}
		FARCALL_RET(UpdateWarioStates_Group4, ADDR_UpdateWarioStates + 0x2c);
		RET();
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates1;

PORT_EX(Func_1e6ea, 0, 0) { WarioStates1(gb).port_Func_1e6ea(); }
PORT_EX(SetState_Turning, 0, 0) { WarioStates1(gb).port_SetState_Turning(); }
PORT_EX(SetState_Attacking, 0, 0) { WarioStates1(gb).port_SetState_Attacking(); }
PORT_EX(SetState_Walking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_SetState_Walking(); }
PORT_EX(SetState_Airborne, 0, 0) { WarioStates1(gb).port_SetState_Airborne(); }
PORT_EX(SetState_Diving, 0, 0) { WarioStates1(gb).port_SetState_Diving(); }
PORT_EX(SetState_Submerged, 0, 0) { WarioStates1(gb).port_SetState_Submerged(); }
PORT_EX(SetState_WaterSurfaceIdling, 0, 0) { WarioStates1(gb).port_SetState_WaterSurfaceIdling(); }
PORT_EX(SetState_Sliding, 0, 0) { WarioStates1(gb).port_SetState_Sliding(); }
PORT_EX(SetState_RollingAirborne, 0, 0) { WarioStates1(gb).port_SetState_RollingAirborne(); }
PORT_EX(SetState_CrouchWalking, 0, 0) { WarioStates1(gb).port_SetState_CrouchWalking(); }
PORT_EX(SetState_CrouchAirborne, 0, 0) { WarioStates1(gb).port_SetState_CrouchAirborne(); }
PORT_EX(Func_1f6c2, 0, 0) { WarioStates1(gb).port_Func_1f6c2(); }
PORT_EX(HandleWaterSurfaceFloatingMovement, 0, 0) { WarioStates1(gb).port_HandleWaterSurfaceFloatingMovement(); }
PORT_EX(StartJump, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartJump(); }
PORT_EX(SetState_Walking_ResetWalkVel, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_SetState_Walking_ResetWalkVel(); }
PORT_EX(CrouchOrSlide, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_CrouchOrSlide(); }
PORT_EX(StartCrouchFall, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartCrouchFall(); }
PORT_EX(StartCrouchJump, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartCrouchJump(); }
PORT_EX(StartFall_SkipResetJumpUpwards, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartFall_SkipResetJumpUpwards(); }
PORT_EX(StartJump_FromInput, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartJump_FromInput(); }
PORT_EX(DiveFromSurface_SkipSplash, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DiveFromSurface_SkipSplash(); }
PORT_EX(SurfaceFromUnderwater, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_SurfaceFromUnderwater(); }
PORT_EX(StartFall, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartFall(); }
PORT_EX(StartJump_FromWaterSurface, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartJump_FromWaterSurface(); }
PORT_EX(StartJump_FromSand, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartJump_FromSand(); }
PORT_EX(DiveFromSurface, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DiveFromSurface(); }
PORT_EX(SetState_TryingSubmerge, 0, 0) { WarioStates1(gb).port_SetState_TryingSubmerge(); }
PORT_EX(UpdateState_TryingSubmerge, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_TryingSubmerge(); }
PORT_EX(Func_1e1e3, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1e1e3(); }
PORT_EX(HandleInput_Idling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleInput_Idling(); }
PORT_EX(Func_1e174, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1e174(); }
PORT_EX(Func_1f863, 0, 0) { WarioStates1(gb).port_Func_1f863(); }
PORT_EX(SetState_Throwing, 0, 0) { WarioStates1(gb).port_SetState_Throwing(); }
PORT_EX(Func_1f3a7, 0, 0) { WarioStates1(gb).port_Func_1f3a7(); }
PORT_EX(SetState_SmashAttacking, 0, 0) { WarioStates1(gb).port_SetState_SmashAttacking(); }
PORT_EX(SetState_LadderShakeStunned, 0, 0) { WarioStates1(gb).port_SetState_LadderShakeStunned(); }
PORT_EX(StartDive, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartDive(); }
PORT_EX(UpdateState_Idling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Idling(); }
PORT_EX(SetState_ThrowingAirborne, 0, 0) { WarioStates1(gb).port_SetState_ThrowingAirborne(); }
PORT_EX(SetState_WaterSurfaceMoving, 0, 0) { WarioStates1(gb).port_SetState_WaterSurfaceMoving(); }
PORT_EX(SetState_Rolling, 0, 0) { WarioStates1(gb).port_SetState_Rolling(); }
PORT_EX(SetState_SwimKnockBack, 0, 0) { WarioStates1(gb).port_SetState_SwimKnockBack(); }
PORT_EX(SetState_ThrowCharging, 0, 0) { WarioStates1(gb).port_SetState_ThrowCharging(); }
PORT_EX(SetState_CrouchSliding, 0, 0) { WarioStates1(gb).port_SetState_CrouchSliding(); }
PORT_EX(UpdateState_PickedUp, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_PickedUp(); }
PORT_EX(SetState_UnderwaterThrusting, 0, 0) { WarioStates1(gb).port_SetState_UnderwaterThrusting(); }
PORT_EX(StartUpwardsUnderwaterThrusting, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartUpwardsUnderwaterThrusting(); }
PORT_EX(SetState_SwimKnockBack_SwitchDirection, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_SetState_SwimKnockBack_SwitchDirection(); }
PORT_EX(StartUnderwaterThrusting, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartUnderwaterThrusting(); }
PORT_EX(Func_1de3f, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1de3f(); }
PORT_EX(DoGroundPound, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoGroundPound(); }
PORT_EX(DoWarioGroundShake, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoWarioGroundShake(); }
PORT_EX(HandleGroundShake, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleGroundShake(); }
PORT_EX(SetState_Idling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_SetState_Idling(); }
PORT_EX(WalkIfPossible_Right, 0, 0) { WarioStates1(gb).port_WalkIfPossible_Right(); }
PORT_EX(WalkIfPossible_Left, 0, 0) { WarioStates1(gb).port_WalkIfPossible_Left(); }
PORT_EX(Func_1ec19, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1ec19(); }
PORT_EX(Func_1ec4b, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1ec4b(); }
PORT_EX(HandleWaterSurfaceInput, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleWaterSurfaceInput(); }
PORT_EX(HandleBumpMovement, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleBumpMovement(); }
PORT_EX(SetState_Bumping, 0, 0) { WarioStates1(gb).port_SetState_Bumping(); }
PORT_EX(HandleDivingHorizontalMovement, 0, 0) { WarioStates1(gb).port_HandleDivingHorizontalMovement(); }
PORT_EX(DoHardLand, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoHardLand(); }
PORT_EX(HandleInvincibility, 0, 0) { WarioStates1(gb).port_HandleInvincibility(); }
PORT_EX(DoSoftLand, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoSoftLand(); }
PORT_EX(WalkRight, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_WalkRight(); }
PORT_EX(WalkLeft, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_WalkLeft(); }
PORT_EX(DoBumpOrJumpingBump, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoBumpOrJumpingBump(); }
PORT_EX(DoJumpingBump_NoSFX, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoJumpingBump_NoSFX(); }
PORT_EX(UpdateState_Turning, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Turning(); }
PORT_EX(Func_1eb46, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1eb46(); }
PORT_EX(UpdateState_Diving, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Diving(); }
PORT_EX(Func_1f825, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1f825(); }
PORT_EX(UpdateState_StungRecovery, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_StungRecovery(); }
PORT_EX(UpdateState_PipeGoingDown, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_PipeGoingDown(); }
PORT_EX(UpdateState_PipeGoingUp, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_PipeGoingUp(); }
PORT_EX(UpdateState_Landing, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Landing(); }
PORT_EX(HandleInput_Walking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleInput_Walking(); }
PORT_EX(Func_1ec6c, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1ec6c(); }
PORT_EX(SetState_GrabIdling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_SetState_GrabIdling(); }
PORT_EX(DoBumpWithBigBackOffset, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoBumpWithBigBackOffset(); }
PORT_EX(DoBumpWithSmallBackOffset, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoBumpWithSmallBackOffset(); }
PORT_EX(DoJumpingBump, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_DoJumpingBump(); }
PORT_EX(UpdateState_Crouching, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Crouching(); }
PORT_EX(UpdateState_Bumping, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Bumping(); }
PORT_EX(UpdateState_Stung, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Stung(); }
PORT_EX(UpdateState_EnemyBumping, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_EnemyBumping(); }
PORT_EX(UpdateState_EnteringDoor, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_EnteringDoor(); }
PORT_EX(ApplyLastWaterCurrentMovement, 0, 0) { WarioStates1(gb).port_ApplyLastWaterCurrentMovement(); }
PORT_EX(HandleInput_Airborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleInput_Airborne(); }
PORT_EX(UpdateState_SmashAttacking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_SmashAttacking(); }
PORT_EX(UpdateState_Throwing, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Throwing(); }
PORT_EX(UpdateState_ThrowingAirborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_ThrowingAirborne(); }
PORT_EX(Func_1dd78, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1dd78(); }
PORT_EX(UpdateState_SwimKnockBack, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_SwimKnockBack(); }
PORT_EX(UpdateState_WaterStung, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_WaterStung(); }
PORT_EX(Func_1f310, 0, 0) { WarioStates1(gb).port_Func_1f310(); }
PORT_EX(Func_1f3d9, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1f3d9(); }
PORT_EX(ApplyWaterCurrentMovement, 0, 0) { WarioStates1(gb).port_ApplyWaterCurrentMovement(); }
PORT_EX(HandleAttackingAirborneMovement, 0, 0) { WarioStates1(gb).port_HandleAttackingAirborneMovement(); }
PORT_EX(UpdateState_Walking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Walking(); }
PORT_EX(UpdateState_Airborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Airborne(); }
PORT_EX(UpdateState_CrouchWalking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_CrouchWalking(); }
PORT_EX(UpdateState_CrouchAirborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_CrouchAirborne(); }
PORT_EX(UpdateState_Sliding, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Sliding(); }
PORT_EX(UpdateState_Rolling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Rolling(); }
PORT_EX(UpdateState_RollingAirborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_RollingAirborne(); }
PORT_EX(UpdateState_PickingUp, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_PickingUp(); }
PORT_EX(UpdateState_ThrowCharging, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_ThrowCharging(); }
PORT_EX(UpdateState_ThrowFullyCharged, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_ThrowFullyCharged(); }
PORT_EX(UpdateState_GrabSmashAttacking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_GrabSmashAttacking(); }
PORT_EX(UpdateState_GroundShakeStunned, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_GroundShakeStunned(); }
PORT_EX(UpdateState_CrouchSliding, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_CrouchSliding(); }
PORT_EX(UpdateState_GrabAirborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_GrabAirborne(); }
PORT_EX(Func_1f357, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1f357(); }
PORT_EX(HandleUnderwaterThrustingInput, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleUnderwaterThrustingInput(); }
PORT_EX(UpdateState_AttackingAirborne, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_AttackingAirborne(); }
PORT_EX(UpdateState_WaterSurfaceIdling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_WaterSurfaceIdling(); }
PORT_EX(UpdateState_WaterSurfaceMoving, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_WaterSurfaceMoving(); }
PORT_EX(UpdateState_UnderwaterThrusting, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_UnderwaterThrusting(); }
PORT_EX(SetState_GrabAirborne, 0, 0) { WarioStates1(gb).port_SetState_GrabAirborne(); }
PORT_EX(Func_1f24c, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1f24c(); }
PORT_EX(SetState_AttackingAirborne, 0, 0) { WarioStates1(gb).port_SetState_AttackingAirborne(); }
PORT_EX(Func_1edd3, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1edd3(); }
PORT_EX(Func_1ede9, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1ede9(); }
PORT_EX(Func_1f1a9, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1f1a9(); }
PORT_EX(StartFallingAirborneAttack, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartFallingAirborneAttack(); }
PORT_EX(StartJumpingAirborneAttack, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_StartJumpingAirborneAttack(); }
PORT_EX(UpdateState_Submerged, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Submerged(); }
PORT_EX(Func_1ee88, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1ee88(); }
PORT_EX(Func_1eefc, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_Func_1eefc(); }
PORT_EX(HandleInput_Attacking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_HandleInput_Attacking(); }
PORT_EX(UpdateState_GrabIdling, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_GrabIdling(); }
PORT_EX(UpdateState_GrabWalking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_GrabWalking(); }
PORT_EX(UpdateState_Attacking, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateState_Attacking(); }
PORT_EX(UpdateWarioStates_Group1, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateWarioStates_Group1(); }
PORT_EX(UpdateWarioStates, HOOK_MANUAL_RET, 0) { WarioStates1(gb).port_UpdateWarioStates(); }
