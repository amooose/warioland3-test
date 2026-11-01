// Wario's states, part 4 (src/engine/level/wario_states_4.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "../asm_module.hpp"

namespace wl3::level {

namespace {

class WarioStates4 : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_SetState_Splitting()
	{
		CALL(TriggerFloorTransition);
		st(wWarioState, WST_SPLITTING);
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	void port_UpdateState_SplitHit()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		st(wWarioState, WST_SPLIT_KNOCKED_BACK);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_1ffd19, ADDR_Frameset_1ffd13);
		anim(OAM2);
	}

	void port_SetState_BlindIdling()
	{
		st(wWarioState, WST_BLIND_IDLING);
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, wInvisibleFrame, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		xor_a();
		CALL(UpdateLevelMusic);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	void port_SetState_BlindWalking()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_BLIND_WALKING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		// the d-pad decides the direction, else wDirection
		bool right;
		uint8_t pad = ld(wJoypadDown);
		if (alu_.bit(pad, B_PAD_LEFT))
			right = false;
		else if (alu_.bit(pad, B_PAD_RIGHT))
			right = true;
		else
			right = ld_and(wDirection);
		set_frameset(right ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);
		anim(OAM1);
	}

	void port_Func_1ede4d()
	{
		CALL(ClearTransformationValues);
		CALL(UpdateLevelMusic);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		FARCALL(StartJump_FromInput);
	}

	void port_SetState_SnowballRolling()
	{
		st(wWarioState, WST_SNOWBALL_ROLLING);
		static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelTable, wJumpVelIndex,
			wIsSmashAttacking, 0};
		clear(zero);
	}

	void port_SetState_SnowmanAirborne()
	{
		st(wWarioState, WST_SNOWMAN_AIRBORNE);
		static const uint16_t zero[] = {wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		anim(OAM2);
	}

	void port_Func_1edd24()
	{
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		st(wWarioState, WST_BALL_BOUNCING);
		reset_anim();
		anim(OAM3);
	}

	void port_SetState_FanSpinning()
	{
		st(wWarioState, WST_FAN_SPINNING);
		reset_anim();
		reset_anim();
		set_frameset(ADDR_Frameset_1dc393);
		anim(OAM3);
	}

	void port_UpdateState_ReleasingOwl()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		st(wWarioState, WST_FALLING_FROM_OWL);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		st(wJumpVelTable, 0x02);
		st(wJumpVelIndex, 0x18);
	}

	void port_SetState_BallShot()
	{
		st(wWarioState, WST_BALL_SHOT);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
			wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wAutoMoveState, cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd26c);
		anim(OAM3);
	}

	void port_SetState_FanRecovering()
	{
		st(wWarioState, WST_FAN_RECOVERING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dc384, ADDR_Frameset_1dc3c8);
		anim(OAM3);
	}

	void port_SetState_SnowmanTurning()
	{
		st(wWarioState, WST_SNOWMAN_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1ff605, ADDR_Frameset_1ff5fe);
		anim(OAM2);
	}

	void port_SetState_IceSkatin()
	{
		st(wWarioState, WST_ICE_SKATIN);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_1fdd97, ADDR_Frameset_1fdd94);
		anim(OAM2);
	}

	void port_SetState_BallAirborne()
	{
		play_sfx(SFX_066);
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		st(wWarioState, WST_BALL_AIRBORNE);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dd2f3, ADDR_Frameset_1dd2f6);
		anim(OAM3);
	}

	void port_SetState_FanLifting()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_FAN_LIFTING);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_1dc374, ADDR_Frameset_1dc3b8);
		anim(OAM3);
	}

	void port_SetState_IceSkatinCrash()
	{
		play_sfx(SFX_02E);
		st(wWarioState, WST_ICE_SKATIN_CRASH);
		reset_anim();
		set_frameset(flip_dir_and() ? ADDR_Frameset_1fdd9a : ADDR_Frameset_1fdda7);
		anim(OAM2);
	}

	void port_SetState_SnowmanBumped()
	{
		st(wWarioState, WST_SNOWMAN_BUMPED);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1ff628, ADDR_Frameset_1ff639);
		anim(OAM2);
	}

	void port_SetState_SnowmanIdle()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_SNOWMAN_IDLE);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_1ff5f8, ADDR_Frameset_1ff5fb);
		anim(OAM2);
	}

	void port_SetState_BallTurning()
	{
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		st(wWarioState, WST_BALL_TURNING);
		reset_anim();
		set_frameset(flip_dir_and() ? ADDR_Frameset_1dd287 : ADDR_Frameset_1dd28e);
		anim(OAM3);
	}

	void port_SetState_FanTurning()
	{
		flip_dir();
		st(wWarioState, WST_FAN_TURNING);
		reset_anim();
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dc38e, ADDR_Frameset_1dc389);
		anim(OAM3);
	}

	void port_SetState_SnowballCrash()
	{
		xor_a();
		wr8(wTransformationProgress, 0);
		st(wWarioState, WST_SNOWBALL_CRASH);
		bool right = flip_dir_and();
		cpu_.b = 2;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		reset_anim();
		reset_anim();
		set_frameset(ADDR_Frameset_1ff981);
		anim(OAM2);
	}

	void port_SetState_SnowmanWalking()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_SNOWMAN_WALKING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		reset_anim();
		bool right;
		if (ld_bit(wJoypadDown, B_PAD_LEFT))
			right = false;
		else if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			right = true;
		else
			right = facing_right();
		set_frameset(right ? ADDR_Frameset_1ff5ef : ADDR_Frameset_1ff5e6);
		anim(OAM2);
	}

	void port_SetState_BlindTurning()
	{
		st(wWarioState, WST_BLIND_TURNING);
		reset_anim();
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		frameset_by_dir(ADDR_Frameset_14cbe, ADDR_Frameset_14cc5);
		anim(OAM1);
	}

	void port_SetState_OwlWario()
	{
		st(wWarioState, WST_OWL_SLOW);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, 0};
		clear(zero);
		reset_anim();
		if (ld_mask(wSwimmingDirectionInput, PAD_UP | PAD_DOWN))
			set_frameset(ADDR_Frameset_1fedd0);
		else
			frameset_by_dir(ADDR_Frameset_1fed7c, ADDR_Frameset_1fed91);
		anim(OAM2);
	}

	void port_Func_1ede69()
	{
		CALL(ClearTransformationValues);
		CALL(UpdateLevelMusic);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		FARCALL(Func_1e174);
	}

	void port_Func_1edf2b()
	{
		FARCALL(Func_19acd);
		if (!ld_and(wIsInAirCurrent)) {
			LJUMP(SetState_FanSpinning);
			return;
		}
		cpu_.b = 1;
		CALL(MoveWarioUp);
		RET();
	}

	void port_UpdateState_BallBouncing()
	{
		anim(OAM3);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_BallAirborne); // fallthrough
	}

	void port_UpdateState_BallTurning()
	{
		anim(OAM3);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_BallAirborne);
	}

	void port_UpdateState_SnowmanLanding()
	{
		anim(OAM2);
		if (ld_and(wGroundShakeCounter)) {
			RET();
			return;
		}
		LJUMP(SetState_SnowmanIdle);
	}

	void port_UpdateState_IceSkatinStart()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		xor_a();
		wr8(wWalkVelIndex, 0);
		LJUMP(SetState_IceSkatin); // fallthrough
	}

	void port_UpdateWarioOwlDirection()
	{
		st(wSwimmingDirectionInput, facing_right() ? PAD_RIGHT : PAD_LEFT);
		LJUMP(SetState_OwlWario); // fallthrough
	}

	void port_UpdateState_FanStart()
	{
		FARCALL(Func_19b25);
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_FanLifting); // fallthrough
	}

	void port_Func_1ec64d()
	{
		st(wWarioState, WST_RELEASING_OWL);
		CALL(ReleaseOwl);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		reset_anim();
		set_frameset(ADDR_Frameset_1fedfe);
		anim(OAM2);
	}

	// shared by Func_1edaca and Func_1edaf8: turn if the d-pad points back,
	// else pick a frameset and bounce
	void ball_bounce(uint16_t left, uint16_t right)
	{
		bool r = facing_right();
		if (ld_bit(wJoypadDown, r ? B_PAD_LEFT : B_PAD_RIGHT)) {
			LJUMP(SetState_BallTurning);
			return;
		}
		set_frameset(r ? right : left);
		LJUMP(Func_1edd24);
	}

	void port_Func_1edaca()
	{
		ball_bounce(ADDR_Frameset_1dd316, ADDR_Frameset_1dd323);
	}

	void port_Func_1edaf8()
	{
		ball_bounce(ADDR_Frameset_1dd230, ADDR_Frameset_1dd295);
	}

	void port_SetState_SnowmanAirborne_Fall()
	{
		st(wJumpVelIndex, 0x18);
		st(wIsSmashAttacking, 0x01);
		wr8(wWarioStateCounter, 0x01);
		st(wJumpVelTable, 0x02);
		frameset_by_dir(ADDR_Frameset_1ff622, ADDR_Frameset_1ff625);
		LJUMP(SetState_SnowmanAirborne);
	}

	void port_SetState_SnowmanAirborne_Jump()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wWarioStateCounter, 0);
		st(wJumpVelTable, 0x01);
		frameset_by_dir(ADDR_Frameset_1ff60c, ADDR_Frameset_1ff617);
		LJUMP(SetState_SnowmanAirborne);
	}

	void port_SetState_Swallowed()
	{
		st(wWarioState, WST_SWALLOWED);
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		wr8(wAutoMoveState, cpu_.a);
		static const uint16_t zero2[] = {wFrameDuration, wAnimationFrame, wIsTurning, 0};
		clear(zero2);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioLaunchGfx, ADDR_WarioLaunchGfx);
		set_oam(BANK_OAM_1dc3cd, ADDR_OAM_1dc3cd);
		cpu_.a = rd8(wObjDirection);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
		anim(OAM3);
	}

	void port_SetState_FanStart()
	{
		st(wWarioState, WST_FAN_START);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
			wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
			wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		CALL(UpdateLevelMusic);
		reset_anim();
		load_gfx(BANK_WarioFanGfx, ADDR_WarioFanGfx);
		set_oam(BANK_OAM_1dc000, ADDR_OAM_1dc000);
		frameset_by_dir(ADDR_Frameset_1dc37f, ADDR_Frameset_1dc3c3);
		anim(OAM3);
	}

	void port_SetState_IceSkatinStart()
	{
		st(wWarioState, WST_ICE_SKATIN_START);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wInvisibleFrame, wWarioStateCounter, wWarioStateCycles,
			wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
			wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioIceSkatinPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioBubbleGfx, ADDR_WarioBubbleGfx);
		set_oam(BANK_OAM_1fdb6d, ADDR_OAM_1fdb6d);
		cpu_.a = rd8(wObjDirection);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_1fdd82 : ADDR_Frameset_1fdd8b);
		anim(OAM2);
	}

	void port_HandleWarioOwlInput()
	{
		st(wWarioState, WST_OWL_FAST);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		uint8_t joy = ld(wJoypadDown);
		uint8_t in;
		if (alu_.bit(joy, B_PAD_UP))
			in = PAD_UP;
		else if (alu_.bit(joy, B_PAD_DOWN))
			in = PAD_DOWN;
		else if (alu_.bit(joy, B_PAD_RIGHT)) {
			st(wDirection, DIRECTION_RIGHT);
			in = PAD_RIGHT;
		} else {
			st(wDirection, DIRECTION_LEFT);
			in = PAD_LEFT;
		}
		st(wSwimmingDirectionInput, in);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, 0};
		clear(zero);
		reset_anim();
		if (ld_mask(wSwimmingDirectionInput, PAD_UP | PAD_DOWN))
			set_frameset(ADDR_Frameset_1fede5);
		else
			frameset_by_dir(ADDR_Frameset_1feda6, ADDR_Frameset_1fedbb);
		anim(OAM2);
	}

	void port_SetState_SplitHit()
	{
		st(wWarioState, WST_SPLIT_HIT);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0};
		clear(zero);
		st(wJumpVelTable, 0x01);
		st(wJumpVelIndex, 0x0e);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioSplitPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioSnowballGfx, ADDR_WarioSnowballGfx);
		set_oam(BANK_OAM_1ffa12, ADDR_OAM_1ffa12);
		st(wDirection, rd8(wObjDirection));
		frameset_by_dir(ADDR_Frameset_1ffd16, ADDR_Frameset_1ffd10);
		anim(OAM2);
	}

	void port_SetState_TurningIntoSnowman()
	{
		play_sfx(SFX_047);
		st(wWarioState, WST_SNOWMAN_START);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelIndex, 0};
		clear(zero);
		if (ld_and(wJumpVelTable))
			st(wJumpVelIndex, 0x18);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioSnowmanPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioSnowmanGfx, ADDR_WarioSnowmanGfx);
		set_oam(BANK_OAM_1feec7, ADDR_OAM_1feec7);
		frameset_by_dir(ADDR_Frameset_1ff560, ADDR_Frameset_1ff5a3);
		anim(OAM2);
	}

	void port_SetState_Launched()
	{
		xor_a();
		wr8(wIsIntangible, 0);
		st(wTransformation, TRANSFORMATION_LAUNCHED);
		st(wTouchState, TOUCH_VANISH);
		st(wStingTouchState, TOUCH_VANISH);
		st(wca94, 0x02);
		st(wWarioState, WST_LAUNCHED);
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		collision_box(-1, -15, -9, 9);
		reset_anim();
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioLaunchGfx, ADDR_WarioLaunchGfx);
		set_oam(BANK_OAM_1dc449, ADDR_OAM_1dc449);
		frameset_by_dir(ADDR_Frameset_1dc904, ADDR_Frameset_1dc8fb);
		anim(OAM3);
	}

	// ld hl, wWarioPos / ld de, hPos / copy 4 bytes (hl ends at wWarioPos + 3)
	void copy_pos_to_hpos()
	{
		for (int i = 0; i < 4; i++) {
			cpu_.a = rd8(wWarioPos + i);
			wr8(hPos + i, cpu_.a);
		}
		SET_HL(gb_, wWarioPos + 3);
		SET_DE(gb_, hPos + 3);
	}

	void port_SetState_MagicRising()
	{
		copy_pos_to_hpos();
		cpu_.b = PARTICLE_MAGIC_1;
		FARCALL(CreateParticle);
		copy_pos_to_hpos();
		cpu_.b = PARTICLE_MAGIC_2;
		FARCALL(CreateParticle);
		st(wWarioState, WST_MAGIC_RISING);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
			wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
			wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioMagicPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioMagicGfx, ADDR_WarioMagicGfx);
		set_oam(BANK_OAM_1dcd60, ADDR_OAM_1dcd60);
		set_frameset(ADDR_Frameset_1dcfb0);
		anim(OAM3);
	}

	void port_UpdateState_BallShot()
	{
		if (!ld_and(wAutoMoveState)) {
			st(wDirection, DIRECTION_LEFT);
			st(wWarioState, WST_BALL_THROWN);
			static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter,
				wWarioStateCycles, 0};
			clear(zero);
			st(wJumpVelIndex, 0x6);
			st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
			reset_anim();
			set_frameset(ADDR_Frameset_1dd2ee);
			anim(OAM3);
			return;
		}
		if (alu_.bit(cpu_.a, 7)) {
			cpu_.a = alu_.and_(cpu_.a, 0x7f);
			wr8(wAutoMoveState, cpu_.a);
			cpu_.a = alu_.dec(cpu_.a);
			if (ZF) {
				// .phase_3
				SET_HL(gb_, ADDR_WarioBallPal);
				CALL(SetWarioPal);
				reset_anim();
				set_frameset(ADDR_Frameset_1dd26c);
			} else {
				cpu_.a = alu_.dec(cpu_.a);
				if (ZF) {
					// .phase_1
					reset_anim();
					set_frameset(ADDR_Frameset_1dd2d1);
				} else {
					cpu_.a = alu_.dec(cpu_.a);
					if (!ZF)
						return;
					// .phase_2
					SET_HL(gb_, ADDR_WarioBallShotPal);
					CALL(SetWarioPal);
					reset_anim();
					set_frameset(ADDR_Frameset_1dd27a);
				}
			}
		}
		anim(OAM3);
		ld_cp(wAutoMoveState, 0x02);
		if (!ZF)
			return;
		if (!anim_ended())
			return;
		st(wAutoMoveState, 0x03 | (1 << 7));
	}

	void port_TurnIntoSmallSnowball()
	{
		st(wTransformationProgress, 1);
		xor_a();
		wr8(wWarioStateCounter, 0);
		load_gfx(BANK_WarioSnowballGfx, ADDR_WarioSnowballGfx);
		set_oam(BANK_OAM_1ff64a, ADDR_OAM_1ff64a);
		reset_anim();
		cpu_.a = alu_.and_(rd8(wWarioSlopeInteraction), 0x0f);
		cpu_.a = alu_.dec(cpu_.a);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_1ff92b : ADDR_Frameset_1ff956);
		anim(OAM2);
		LJUMP(SetState_SnowballRolling);
	}

	void port_Func_1ec215()
	{
		cpu_.b = 3;
		if (facing_right())
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
		LJUMP(SetState_IceSkatinCrash);
	}

	void port_HandleSnowmanInput()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_SnowmanAirborne_Jump);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_SnowmanWalking);
			return;
		}
		RET();
	}

	void port_Func_1ede96()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_SnowmanAirborne_Jump);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		bool turn;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			turn = !ld_and(wDirection);
			if (!turn)
				FARCALL(WalkIfPossible_Right);
			else
				st(wDirection, DIRECTION_RIGHT);
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			ld_cp(wDirection, DIRECTION_LEFT);
			turn = !ZF;
			if (!turn)
				FARCALL(WalkIfPossible_Left);
			else
				st(wDirection, DIRECTION_LEFT);
		} else {
			LJUMP(SetState_SnowmanIdle);
			return;
		}
		if (turn) {
			LJUMP(SetState_SnowmanTurning);
			return;
		}
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		RET();
	}

	void port_Func_1edf47()
	{
		bool right = facing_right();
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
			LJUMP(SetState_FanTurning);
			return;
		}
		if (!alu_.bit(cpu_.a, right ? B_PAD_RIGHT : B_PAD_LEFT)) {
			RET();
			return;
		}
		port_farcall(gb_, right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1);
		if (b_and()) {
			RET();
			return;
		}
		cpu_.b = 1;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		RET();
	}

	void port_UpdateState_MagicStopping()
	{
		cpu_.a = inc_counter();
		alu_.cp(cpu_.a, 0x20);
		if (CF) {
			RET();
			return;
		}
		JUMP(RecoverFromTransformation); // fallthrough to UpdateState_MagicRecovering
	}

	// UpdateWarioAnimation in OAM bank `bank`, then true once it has ended
	// (ret z otherwise)
	bool anim_and_ended(uint8_t bank)
	{
		anim(bank);
		if (!anim_ended()) {
			RET();
			return false;
		}
		return true;
	}

	void port_UpdateState_SnowmanBumped()
	{
		if (anim_and_ended(OAM2))
			LJUMP(SetState_SnowmanIdle);
	}

	void port_UpdateState_LaunchCrash()
	{
		if (anim_and_ended(OAM3)) {
			FARCALL(Func_2ad6a);
			RET();
		}
	}

	void port_UpdateState_GrabbingOwl()
	{
		if (!anim_and_ended(OAM2))
			return;
		if (ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
			LJUMP(HandleWarioOwlInput);
			return;
		}
		LJUMP(UpdateWarioOwlDirection); // fallthrough
	}

	void port_UpdateState_BallStart()
	{
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			RET();
			return;
		}
		update_pos_y();
		LJUMP(Func_1edaca); // fallthrough
	}

	void port_UpdateStart_FanTurning()
	{
		LCALL(Func_1edf2b);
		ld_cp(wWarioState, WST_FAN_TURNING);
		if (!ZF) {
			RET();
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		if (anim_and_ended(OAM3))
			LJUMP(SetState_FanLifting);
	}

	void port_UpdateState_Swallowed()
	{
		if (ld_and(wIsTurning)) {
			xor_a();
			wr8(wIsTurning, 0);
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			set_frameset(flip_dir_and() ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
		}
		anim(OAM3);
		if (ld_and(wAutoMoveState)) {
			RET();
			return;
		}
		st(wDirection, DIRECTION_LEFT);
		LJUMP(SetState_Launched); // fallthrough
	}

	void port_UpdateState_BallSentUpwards()
	{
		anim(OAM3);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			cpu_.b = 3;
			CALL(MoveWarioUp);
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				JUMP(HandleUpwardsFloorTransition);
				return;
			}
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			st(wJumpVelTable, JUMP_VEL_NORMAL);
			RET();
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (b_and()) {
			update_pos_y();
			xor_a();
			wr8(wIsIntangible, 0);
			wr8(wJumpVelIndex, 0);
			wr8(wJumpVelTable, 0);
			FARCALL(Func_206eb);
		}
		RET();
	}

	void port_UpdateState_MagicRising()
	{
		FARCALL(Func_19b25);
		FARCALL(CheckUpCollision);
		if (b_and()) {
			get_pos();
			cpu_.b = PARTICLE_MAGIC_STOP;
			FARCALL(CreateParticle);
			st(wWarioState, WST_MAGIC_STOPPING);
			xor_a();
			wr8(wWarioStateCounter, 0);
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			anim(0x77);
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(0x77);
		cpu_.b = 1;
		CALL(MoveWarioUp);
		cpu_.a = alu_.and_(rd8(wCameraConfigFlags), CAM_SCROLLING_MASK);
		alu_.cp(cpu_.a, CAM_TRANSITIONS);
		if (CF)
			return;
		CALL(GetFloorForYPos);
		cpu_.a = alu_.sub(rd8(wFloor), cpu_.c);
		if (CF)
			CALL(StartUpwardsFloorTransition);
	}

	void port_UpdateState_Launched()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		sfx_loop(0x86, SFX_04C);
		anim(OAM3);
		FARCALL(CheckFrontCollision);
		if (!b_and()) {
			cpu_.b = 2;
			move_by_dir();
			RET();
			return;
		}
		play_sfx(SFX_048);
		st(wWarioState, WST_LAUCH_CRASH);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dc926, ADDR_Frameset_1dc90d);
		anim(OAM3);
		RET();
	}

	void port_UpdateState_BallThrown()
	{
		anim(OAM3);
		CALL(ApplyJumpVelocity);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			cpu_.b = 3;
			CALL(MoveWarioLeft);
			return;
		}
		cpu_.b = 2;
		CALL(MoveWarioLeft);
		FARCALL(CheckAirborneCollision);
		if (!b_and())
			return;
		update_pos_y();
		xor_a();
		wr8(wIsIntangible, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		// push_wram BANK(wShootGoals)
		cpu_.a = rd8(rWBK);
		push_af();
		st(rWBK, BANK_wShootGoals);
		ld_cp(wShootGoals, 3);
		if (!ZF) {
			pop_af();
			wr8(rWBK, cpu_.a);
			FARCALL(Func_206eb);
			return;
		}
		pop_af();
		wr8(rWBK, cpu_.a);
		st(wWarioState, WST_BALL_SENT_UPWARDS);
		play_sfx(SFX_066);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wJumpVelIndex, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd2ee);
		anim(OAM3);
	}

	void port_SetState_BallStart()
	{
		CALL(UpdateLevelMusic);
		collision_box(-1, -20, -9, 9);
		static const uint16_t clr[] = {
			wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0,
		};
		clear(clr);
		SET_HL(gb_, ADDR_WarioBallPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioBallGfx, ADDR_WarioBallGfx);
		set_oam(BANK_OAM_1dcff1, ADDR_OAM_1dcff1);
		if (!ld_and(wJumpVelTable)) {
			LJUMP(Func_1edaca);
			return;
		}
		st(wWarioState, WST_BALL_START);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		st(wJumpVelIndex, 0x18);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd313);
		anim(OAM3);
		RET();
	}

	// call ApplyJumpVelocity / farcall walk / cap the walk velocity
	void jump_and_walk(uint8_t bank, uint16_t walk, uint8_t max, uint8_t v)
	{
		CALL(ApplyJumpVelocity);
		port_farcall(gb_, bank, walk);
		ld_cp(wWalkVelIndex, max);
		if (!CF)
			st(wWalkVelIndex, v);
	}

	void port_Func_1ee00f() { jump_and_walk(BANK_Func_2b17a, ADDR_Func_2b17a, 0x10, 0x0c); }
	void port_Func_1ee02e() { jump_and_walk(BANK_HandleWalk, ADDR_HandleWalk, 0x08, 0x04); }

	// ld a, [wWarioState] / cp state: true if still in that state
	bool still_in(uint8_t state)
	{
		ld_cp(wWarioState, state);
		return ZF;
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

	// move by direction with the walk velocity, then cap it at max (to v)
	void walk_move(uint8_t max, uint8_t v)
	{
		if (facing_right()) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		ld_cp(wWalkVelIndex, max);
		if (!CF)
			st(wWalkVelIndex, v);
	}

	void port_UpdateState_IceSkatin()
	{
		if (room_transition_check())
			return;
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			LJUMP(Func_1ec215);
			return;
		}
		sfx_loop(0x0c, SFX_ZIP_LINE_SLIDE);
		anim(OAM2);
		walk_move(0x14, 0x10);
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			update_pos_y();
			RET();
			return;
		}
		st(wWarioState, WST_ICE_SKATIN_AIRBORNE);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		RET();
	}

	void port_UpdateState_IceSkatinAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(SetState_IceSkatinCrash);
			return;
		}
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			LJUMP(Func_1ec215);
			return;
		}
		anim(OAM2);
		CALL(ApplyJumpVelocity);
		FARCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x14);
		if (!CF)
			st(wWalkVelIndex, 0x10);
		if (!still_in(WST_ICE_SKATIN_AIRBORNE)) {
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			if (!still_in(WST_ICE_SKATIN_AIRBORNE)) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!still_in(WST_ICE_SKATIN_AIRBORNE)) {
			RET();
			return;
		}
		st(wWalkVelIndex, 0x10);
		LJUMP(SetState_IceSkatin);
	}

	// the start of UpdateState_OwlSlow / UpdateState_OwlFast; true if it jumped
	bool owl_start(uint8_t state)
	{
		if (ld_bit(wJoypadPressed, B_PAD_B)) {
			LJUMP(Func_1ec64d);
			return true;
		}
		if (room_transition_check())
			return true;
		if (ld_and(wWaterInteraction)) {
			LJUMP(Func_1ec64d);
			return true;
		}
		if (!still_in(state)) {
			RET();
			return true;
		}
		return false;
	}

	// farcall coll / state check / ld a, b / and a; true if it returned
	// (*blocked: b was nz)
	bool owl_coll(uint8_t state, uint8_t bank, uint16_t coll, bool *blocked)
	{
		port_farcall(gb_, bank, coll);
		if (!still_in(state)) {
			RET();
			return true;
		}
		*blocked = b_and();
		return false;
	}

	void port_UpdateState_OwlSlow()
	{
		if (owl_start(WST_OWL_SLOW))
			return;
		if (ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
			LJUMP(HandleWarioOwlInput);
			return;
		}
		sfx_loop(0x20, SFX_FLAP);
		anim(OAM2);
		bool blocked;
		cpu_.a = rd8(wSwimmingDirectionInput);
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			if (owl_coll(WST_OWL_SLOW, BANK_Func_19741, ADDR_Func_19741, &blocked))
				return;
			if (blocked) {
				st(wDirection, DIRECTION_LEFT);
				LJUMP(UpdateWarioOwlDirection);
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioRight);
		} else if (alu_.bit(cpu_.a, B_PAD_UP)) {
			if (owl_coll(WST_OWL_SLOW, BANK_CheckUpCollision, ADDR_CheckUpCollision, &blocked))
				return;
			if (blocked) {
				st(wSwimmingDirectionInput, PAD_DOWN);
				LJUMP(SetState_OwlWario);
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioUp);
		} else if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			if (owl_coll(WST_OWL_SLOW, BANK_CheckCentreCollision, ADDR_CheckCentreCollision, &blocked))
				return;
			if (blocked) {
				st(wSwimmingDirectionInput, PAD_UP);
				LJUMP(SetState_OwlWario);
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioDown);
		} else {
			// go left
			if (owl_coll(WST_OWL_SLOW, BANK_Func_197b1, ADDR_Func_197b1, &blocked))
				return;
			if (blocked) {
				st(wDirection, DIRECTION_RIGHT);
				LJUMP(UpdateWarioOwlDirection);
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioLeft);
		}
		RET();
	}

	void port_UpdateState_OwlFast()
	{
		if (owl_start(WST_OWL_FAST))
			return;
		if (!ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
			LJUMP(SetState_OwlWario);
			return;
		}
		sfx_loop(0x10, SFX_FLAP);
		anim(OAM2);
		uint8_t pad;
		uint8_t bank;
		uint16_t coll, move;
		cpu_.a = rd8(wSwimmingDirectionInput);
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			pad = PAD_RIGHT, bank = BANK_Func_19741, coll = ADDR_Func_19741, move = ADDR_MoveWarioRight;
		} else if (alu_.bit(cpu_.a, B_PAD_UP)) {
			pad = PAD_UP, bank = BANK_CheckUpCollision, coll = ADDR_CheckUpCollision, move = ADDR_MoveWarioUp;
		} else if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			pad = PAD_DOWN, bank = BANK_CheckCentreCollision, coll = ADDR_CheckCentreCollision,
			move = ADDR_MoveWarioDown;
		} else {
			pad = PAD_LEFT, bank = BANK_Func_197b1, coll = ADDR_Func_197b1, move = ADDR_MoveWarioLeft;
		}
		if (!ld_mask(wJoypadDown, pad)) {
			LJUMP(HandleWarioOwlInput);
			return;
		}
		bool blocked;
		if (owl_coll(WST_OWL_FAST, bank, coll, &blocked))
			return;
		if (blocked) {
			if (pad == PAD_DOWN)
				update_pos_y();
			RET();
			return;
		}
		cpu_.b = 2;
		gb_call(gb_, move);
		RET();
	}

	void port_UpdateState_SnowmanWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(0x1c, SFX_FAT_WALK);
		anim(OAM2);
		LCALL(Func_1ede96);
		if (!still_in(WST_SNOWMAN_WALKING)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(SetState_SnowmanAirborne_Fall);
			return;
		}
		if (!ld_and(wWarioSlopeInteraction)) {
			RET();
			return;
		}
		update_pos_y();
		LJUMP(TurnIntoSmallSnowball);
	}

	void port_UpdateState_SnowballRolling()
	{
		if (room_transition_check())
			return;
		cpu_.a = rd8(wTransformationProgress);
		alu_.cp(cpu_.a, 2);
		if (ZF) {
			// .medium_snowball
			ld_cp(wWarioStateCounter, 0xe0);
			if (!CF) {
				st(wTransformationProgress, 3);
				xor_a();
				wr8(wFrameDuration, 0);
				cpu_.a = alu_.add(rd8(wAnimationFrame), 0x02);
				wr8(wAnimationFrame, cpu_.a);
				frameset_by_dir(ADDR_Frameset_1ff970, ADDR_Frameset_1ff945);
			}
		} else {
			alu_.cp(cpu_.a, 3);
			if (!ZF) {
				// small snowball
				ld_cp(wWarioStateCounter, 0x70);
				if (!CF) {
					st(wTransformationProgress, 2);
					xor_a();
					wr8(wFrameDuration, 0);
					frameset_by_dir(ADDR_Frameset_1ff963, ADDR_Frameset_1ff938);
				}
			}
		}
		// .no_size_change
		sfx_loop(0x0c, SFX_ROLL);
		anim(OAM2);
		if (facing_right()) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		// .increment_counter_if_not_big
		ld_cp(wTransformationProgress, 3);
		if (CF) {
			SET_HL(gb_, wWarioStateCounter);
			cpu_.a = rd8(wWarioStateCounter);
			alu_.cp(cpu_.a, 0xf8);
			if (CF) {
				cpu_.a = alu_.add(cpu_.a, cpu_.b);
				wr8(wWarioStateCounter, cpu_.a);
			}
		}
		ld_cp(wWalkVelIndex, 0x18);
		if (!CF)
			st(wWalkVelIndex, 0x14);
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			LJUMP(SetState_SnowballCrash);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			update_pos_y();
			RET();
			return;
		}
		st(wWarioState, WST_SNOWBALL_AIRBORNE);
		st(wJumpVelIndex, 0x18);
		st(wJumpVelTable, 0x03);
		RET();
	}

	void port_UpdateStart_FanLifting()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wIsInAirCurrent)) {
			LJUMP(SetState_FanRecovering);
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		LCALL(Func_1edf2b);
		if (!still_in(WST_FAN_LIFTING)) {
			RET();
			return;
		}
		LCALL(Func_1edf47);
		RET();
	}

	void port_UpdateState_FanSpinning()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wIsInAirCurrent)) {
			LJUMP(SetState_FanRecovering);
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		LCALL(Func_1edf47);
		RET();
	}

	void port_Func_1edf01()
	{
		CALL(ApplyJumpVelocity);
		FARCALL(Func_2b17a);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		ld_cp(wJumpVelIndex, 0x18);
		if (!ZF)
			return;
		st(wIsSmashAttacking, TRUE);
	}

	void port_UpdateState_BallAirborne()
	{
		anim(OAM3);
		LCALL(Func_1ee02e);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			RET();
			return;
		}
		update_pos_y();
		LJUMP(Func_1edaf8);
	}

	void port_Func_1ed09b()
	{
		flip_dir();
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_MagicRecovering()
	{
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateStart_FanRecovering()
	{
		anim(OAM3);
		if (!anim_ended()) {
			RET();
			return;
		}
		JUMP(RecoverFromTransformation_WithoutInvincibility);
	}

	void port_UpdateState_IceSkatinCrash()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		get_pos();
		cpu_.b = PARTICLE_ICE;
		FARCALL(CreateParticle);
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_SplitKnockedBack()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			FARCALL(HandleWalk);
			CALL(Func_1762);
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		play_sfx(SFX_044);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		if (ld_and(wDirection))
			set_frameset(ADDR_Frameset_1ffc8e);
		else
			set_frameset(ADDR_Frameset_1ffccf);
		anim(OAM2);
		LJUMP(SetState_Splitting); // fallthrough
	}

	void port_UpdateState_SnowmanIdle()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		LCALL(HandleSnowmanInput);
		ld_cp(wWarioState, WST_SNOWMAN_IDLE);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(SetState_SnowmanAirborne_Fall);
			return;
		}
		RET();
	}

	void port_UpdateState_SnowmanTurning()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_SnowmanAirborne_Jump);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_SnowmanWalking);
	}

	void port_UpdateState_SnowballCrash()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		get_pos();
		cpu_.b = PARTICLE_SNOW;
		FARCALL(CreateParticle);
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_SnowmanStart()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		if (ld_and(wJumpVelTable)) {
			CALL(ApplyJumpVelocity);
			FARCALL(CheckAirborneCollision);
			if (!b_and()) {
				JUMP(TriggerDownwardsFloorTransition);
				return;
			}
			CALL(TriggerFloorTransition);
			xor_a();
			wr8(wJumpVelTable, 0);
			wr8(wJumpVelIndex, 0);
		}
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_SnowmanIdle); // fallthrough
	}

	void port_UpdateState_FallingFromOwl()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(Func_2b17a);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			RET();
			return;
		}
		update_pos_y();
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_HangingRail()
	{
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			st(wWarioStateCounter, 0x01);
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			set_frameset(ADDR_Frameset_1feeb4);
		}
		anim(OAM2);
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			JUMP(RecoverFromTransformation_WithoutInvincibility);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_1ede4d);
			return;
		}
		if (!ld_and(wAutoMoveState)) {
			LJUMP(Func_1ede69);
			return;
		}
		RET();
	}

	void port_UpdateState_SnowballAirborne()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			LJUMP(SetState_SnowballCrash);
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x18);
		if (!CF)
			st(wWalkVelIndex, 0x14);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		LJUMP(SetState_SnowballRolling);
	}

	void port_UpdateState_SnowmanAirborne()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			st(wWarioStateCounter, 1);
			if (ld_and(wDirection))
				set_frameset(ADDR_Frameset_1ff625);
			else
				set_frameset(ADDR_Frameset_1ff622);
			anim(OAM2);
		}
		LCALL(Func_1edf01);
		ld_cp(wWarioState, WST_SNOWMAN_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, 0x18);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				RET();
				return;
			}
			ld_cp(wWarioState, WST_SNOWMAN_AIRBORNE);
			if (!ZF) {
				RET();
				return;
			}
			LJUMP(SetState_SnowmanAirborne_Fall);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		FARCALL(CheckCentreCollision);
		if (ld_and(wWarioSlopeInteraction)) {
			LJUMP(TurnIntoSmallSnowball);
			return;
		}
		if (!ld_and(wGroundShakeCounter)) {
			st(wGroundShakeCounter, 0x20);
			st(wIsWarioGroundShaking, TRUE);
		}
		if (!ld_and(wFloorTransitionDir))
			play_sfx(SFX_024);
		st(wWarioState, WST_SNOWMAN_LANDING);
		static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
			wWarioStateCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		if (ld_and(wDirection))
			set_frameset(ADDR_Frameset_1ff617);
		else
			set_frameset(ADDR_Frameset_1ff60c);
		anim(OAM2);
		RET();
	}

	void port_UpdateState_Splitting()
	{
		anim(OAM2);
		if (anim_ended()) {
			LJUMP(Func_1ed09b);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			RET();
			return;
		}
		st(wWarioState, WST_SPLITTING_AIRBORNE);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		RET();
	}

	void port_UpdateState_SplittingAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		if (anim_ended()) {
			LJUMP(Func_1ed09b);
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (b_and()) {
			LJUMP(SetState_Splitting);
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
	}

	// ld a, [wWarioState] / sub $b0 / rst JumpTable (table after the rst)
	void port_UpdateWarioStates_Group4()
	{
		cpu_.a = alu_.sub(rd8(wWarioState), 0xb0);
		gb_push16(gb_, ADDR_UpdateWarioStates_Group4 + 6); // rst JumpTable
		uint16_t hl = gb_pop16(gb_);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		hl = alu_.add16(hl, cpu_.e);
		cpu_.a = rd8(hl);
		cpu_.h = rd8((uint16_t)(hl + 1));
		cpu_.l = cpu_.a;
		JUMP_TO(REG_HL(gb_));
	}

	void port_SetState_BlindAirborne()
	{
		xor_a();
		wr8(wWarioSlopeInteraction, 0);
		st(wWarioState, WST_BLIND_AIRBORNE);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wIsCrouching, wAttackCounter, 0};
		clear(zero);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f94, ADDR_Frameset_15f97);
		anim(OAM1);
		if (!ld_and(wIsOnSteppableObject))
			return;
		gb_call_ret(gb_, ADDR_UpdateState_BlindAirborne, ADDR_SetState_BlindAirborne + 0x70);
	}

	void port_Func_1ed548()
	{
		xor_a();
		wr8(wJumpingUpwards, 0);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		LJUMP(SetState_BlindAirborne);
	}

	void port_Func_1ed558()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		st(wJumpingUpwards, TRUE);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		LJUMP(SetState_BlindAirborne); // fallthrough
	}

	void port_UpdateState_BlindAirborne()
	{
		FARCALL_RET(Func_19b25, ADDR_UpdateState_BlindAirborne + 0x0f);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			JUMP(RecoverFromTransformation);
			return;
		}
		FARCALL_RET(CheckFrontCollision, ADDR_UpdateState_BlindAirborne + 0x2c);
		ld_cp(wWarioState, WST_BLIND_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and()) {
			JUMP(RecoverFromTransformation);
			return;
		}
		gb_call_ret(gb_, ADDR_Func_1ee00f, ADDR_UpdateState_BlindAirborne + 0x3a);
		ld_cp(wWarioState, WST_BLIND_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL_RET(CheckUpCollision, ADDR_UpdateState_BlindAirborne + 0x56);
			if (!b_and()) {
				RET();
				return;
			}
			ld_cp(wWarioState, WST_BLIND_AIRBORNE);
			if (!ZF) {
				RET();
				return;
			}
			LJUMP(Func_1ed548);
			return;
		}
		FARCALL_RET(CheckAirborneCollision, ADDR_UpdateState_BlindAirborne + 0x71);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		gb_call_ret(gb_, ADDR_TriggerFloorTransition, ADDR_UpdateState_BlindAirborne + 0x7b);
		LJUMP(SetState_BlindIdling);
	}

	void port_UpdateState_BlindIdling()
	{
		port_farcall_ret(gb_, OAM1, ADDR_UpdateWarioAnimation, ADDR_UpdateState_BlindIdling + 0x0f);
		gb_call_ret(gb_, ADDR_Func_1edf93, ADDR_UpdateState_BlindIdling + 0x12);
		ld_cp(wWarioState, WST_BLIND_IDLING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL_RET(CheckCentreCollision, ADDR_UpdateState_BlindIdling + 0x27);
		if (!b_and()) {
			LJUMP(Func_1ed548);
			return;
		}
		RET();
	}

	void port_UpdateState_BlindWalking()
	{
		FARCALL_RET(Func_19b25, ADDR_UpdateState_BlindWalking + 0x0f);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		port_farcall_ret(gb_, OAM1, ADDR_UpdateWarioAnimation, ADDR_UpdateState_BlindWalking + 0x25);
		FARCALL_RET(CheckFrontCollision, ADDR_UpdateState_BlindWalking + 0x34);
		ld_cp(wWarioState, WST_BLIND_WALKING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and()) {
			JUMP(RecoverFromTransformation);
			return;
		}
		gb_call_ret(gb_, ADDR_Func_1edfa4, ADDR_UpdateState_BlindWalking + 0x42);
		ld_cp(wWarioState, WST_BLIND_WALKING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL_RET(CheckCentreCollision, ADDR_UpdateState_BlindWalking + 0x57);
		if (!b_and()) {
			LJUMP(Func_1ed548);
			return;
		}
		update_pos_y();
		RET();
	}

	void port_UpdateState_BlindTurning()
	{
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_1ed558);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_BlindWalking);
	}

	void port_Func_1edf93()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_1ed558);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_BlindWalking);
			return;
		}
		RET();
	}

	void port_Func_1edfa4()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_1ed558);
			return;
		}
		uint8_t pad = ld(wJoypadDown);
		if (alu_.bit(pad, B_PAD_RIGHT)) {
			if (!ld_and(wDirection)) {
				st(wDirection, DIRECTION_RIGHT);
				LJUMP(SetState_BlindTurning);
				return;
			}
			FARCALL_RET(WalkIfPossible_Right, ADDR_Func_1edfa4 + 0x2b);
		} else if (alu_.bit(pad, B_PAD_LEFT)) {
			ld_cp(wDirection, DIRECTION_LEFT);
			if (!ZF) {
				st(wDirection, DIRECTION_LEFT);
				LJUMP(SetState_BlindTurning);
				return;
			}
			FARCALL_RET(WalkIfPossible_Left, ADDR_Func_1edfa4 + 0x56);
		} else {
			LJUMP(SetState_BlindIdling);
			return;
		}
		ld_cp(wWalkVelIndex, 0x10);
		if (!CF)
			st(wWalkVelIndex, 0x0c);
		RET();
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates4;

PORT_EX(SetState_Splitting, 0, 0) { WarioStates4(gb).port_SetState_Splitting(); }
PORT_EX(UpdateState_SplitHit, 0, 0) { WarioStates4(gb).port_UpdateState_SplitHit(); }
PORT_EX(SetState_BlindIdling, 0, 0) { WarioStates4(gb).port_SetState_BlindIdling(); }
PORT_EX(SetState_BlindWalking, 0, 0) { WarioStates4(gb).port_SetState_BlindWalking(); }
PORT_EX(Func_1ede4d, 0, 0) { WarioStates4(gb).port_Func_1ede4d(); }
PORT_EX(SetState_SnowballRolling, 0, 0) { WarioStates4(gb).port_SetState_SnowballRolling(); }
PORT_EX(SetState_SnowmanAirborne, 0, 0) { WarioStates4(gb).port_SetState_SnowmanAirborne(); }
PORT_EX(Func_1edd24, 0, 0) { WarioStates4(gb).port_Func_1edd24(); }
PORT_EX(SetState_FanSpinning, 0, 0) { WarioStates4(gb).port_SetState_FanSpinning(); }
PORT_EX(UpdateState_ReleasingOwl, 0, 0) { WarioStates4(gb).port_UpdateState_ReleasingOwl(); }
PORT_EX(SetState_BallShot, 0, 0) { WarioStates4(gb).port_SetState_BallShot(); }
PORT_EX(SetState_FanRecovering, 0, 0) { WarioStates4(gb).port_SetState_FanRecovering(); }
PORT_EX(SetState_SnowmanTurning, 0, 0) { WarioStates4(gb).port_SetState_SnowmanTurning(); }
PORT_EX(SetState_IceSkatin, 0, 0) { WarioStates4(gb).port_SetState_IceSkatin(); }
PORT_EX(SetState_BallAirborne, 0, 0) { WarioStates4(gb).port_SetState_BallAirborne(); }
PORT_EX(SetState_FanLifting, 0, 0) { WarioStates4(gb).port_SetState_FanLifting(); }
PORT_EX(SetState_IceSkatinCrash, 0, 0) { WarioStates4(gb).port_SetState_IceSkatinCrash(); }
PORT_EX(SetState_SnowmanBumped, 0, 0) { WarioStates4(gb).port_SetState_SnowmanBumped(); }
PORT_EX(SetState_SnowmanIdle, 0, 0) { WarioStates4(gb).port_SetState_SnowmanIdle(); }
PORT_EX(SetState_BallTurning, 0, 0) { WarioStates4(gb).port_SetState_BallTurning(); }
PORT_EX(SetState_FanTurning, 0, 0) { WarioStates4(gb).port_SetState_FanTurning(); }
PORT_EX(SetState_SnowballCrash, 0, 0) { WarioStates4(gb).port_SetState_SnowballCrash(); }
PORT_EX(SetState_SnowmanWalking, 0, 0) { WarioStates4(gb).port_SetState_SnowmanWalking(); }
PORT_EX(SetState_BlindTurning, 0, 0) { WarioStates4(gb).port_SetState_BlindTurning(); }
PORT_EX(SetState_OwlWario, 0, 0) { WarioStates4(gb).port_SetState_OwlWario(); }
PORT_EX(Func_1ede69, 0, 0) { WarioStates4(gb).port_Func_1ede69(); }
PORT_EX(Func_1edf2b, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1edf2b(); }
PORT_EX(UpdateState_BallBouncing, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BallBouncing(); }
PORT_EX(UpdateState_BallTurning, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BallTurning(); }
PORT_EX(UpdateState_SnowmanLanding, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanLanding(); }
PORT_EX(UpdateState_IceSkatinStart, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_IceSkatinStart(); }
PORT_EX(UpdateWarioOwlDirection, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateWarioOwlDirection(); }
PORT_EX(UpdateState_FanStart, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_FanStart(); }
PORT_EX(Func_1ec64d, 0, 0) { WarioStates4(gb).port_Func_1ec64d(); }
PORT_EX(Func_1edaca, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1edaca(); }
PORT_EX(Func_1edaf8, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1edaf8(); }
PORT_EX(SetState_SnowmanAirborne_Fall, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_SetState_SnowmanAirborne_Fall(); }
PORT_EX(SetState_SnowmanAirborne_Jump, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_SetState_SnowmanAirborne_Jump(); }
PORT_EX(SetState_Swallowed, 0, 0) { WarioStates4(gb).port_SetState_Swallowed(); }
PORT_EX(SetState_FanStart, 0, 0) { WarioStates4(gb).port_SetState_FanStart(); }
PORT_EX(SetState_IceSkatinStart, 0, 0) { WarioStates4(gb).port_SetState_IceSkatinStart(); }
PORT_EX(HandleWarioOwlInput, 0, 0) { WarioStates4(gb).port_HandleWarioOwlInput(); }
PORT_EX(SetState_SplitHit, 0, 0) { WarioStates4(gb).port_SetState_SplitHit(); }
PORT_EX(SetState_TurningIntoSnowman, 0, 0) { WarioStates4(gb).port_SetState_TurningIntoSnowman(); }
PORT_EX(SetState_Launched, 0, 0) { WarioStates4(gb).port_SetState_Launched(); }
PORT_EX(SetState_MagicRising, 0, 0) { WarioStates4(gb).port_SetState_MagicRising(); }
PORT_EX(UpdateState_BallShot, 0, 0) { WarioStates4(gb).port_UpdateState_BallShot(); }
PORT_EX(TurnIntoSmallSnowball, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_TurnIntoSmallSnowball(); }
PORT_EX(Func_1ec215, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1ec215(); }
PORT_EX(HandleSnowmanInput, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_HandleSnowmanInput(); }
PORT_EX(Func_1ede96, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1ede96(); }
PORT_EX(Func_1edf47, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1edf47(); }
PORT_EX(UpdateState_MagicStopping, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_MagicStopping(); }
PORT_EX(UpdateState_SnowmanBumped, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanBumped(); }
PORT_EX(UpdateState_LaunchCrash, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_LaunchCrash(); }
PORT_EX(UpdateState_GrabbingOwl, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_GrabbingOwl(); }
PORT_EX(UpdateState_BallStart, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BallStart(); }
PORT_EX(UpdateStart_FanTurning, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateStart_FanTurning(); }
PORT_EX(UpdateState_Swallowed, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_Swallowed(); }
PORT_EX(UpdateState_BallSentUpwards, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BallSentUpwards(); }
PORT_EX(UpdateState_MagicRising, 0, 0) { WarioStates4(gb).port_UpdateState_MagicRising(); }
PORT_EX(UpdateState_Launched, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_Launched(); }
PORT_EX(UpdateState_BallThrown, 0, 0) { WarioStates4(gb).port_UpdateState_BallThrown(); }
PORT_EX(SetState_BallStart, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_SetState_BallStart(); }
PORT_EX(Func_1ee00f, 0, 0) { WarioStates4(gb).port_Func_1ee00f(); }
PORT_EX(Func_1ee02e, 0, 0) { WarioStates4(gb).port_Func_1ee02e(); }
PORT_EX(UpdateState_IceSkatin, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_IceSkatin(); }
PORT_EX(UpdateState_IceSkatinAirborne, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_IceSkatinAirborne(); }
PORT_EX(UpdateState_OwlSlow, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_OwlSlow(); }
PORT_EX(UpdateState_OwlFast, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_OwlFast(); }
PORT_EX(UpdateState_SnowmanWalking, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanWalking(); }
PORT_EX(UpdateState_SnowballRolling, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowballRolling(); }
PORT_EX(UpdateStart_FanLifting, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateStart_FanLifting(); }
PORT_EX(UpdateState_FanSpinning, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_FanSpinning(); }
PORT_EX(Func_1edf01, 0, 0) { WarioStates4(gb).port_Func_1edf01(); }
PORT_EX(UpdateState_BallAirborne, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BallAirborne(); }
PORT_EX(Func_1ed09b, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1ed09b(); }
PORT_EX(UpdateState_MagicRecovering, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_MagicRecovering(); }
PORT_EX(UpdateStart_FanRecovering, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateStart_FanRecovering(); }
PORT_EX(UpdateState_IceSkatinCrash, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_IceSkatinCrash(); }
PORT_EX(UpdateState_SplitKnockedBack, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SplitKnockedBack(); }
PORT_EX(UpdateState_SnowmanIdle, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanIdle(); }
PORT_EX(UpdateState_SnowmanTurning, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanTurning(); }
PORT_EX(UpdateState_SnowballCrash, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowballCrash(); }
PORT_EX(UpdateState_SnowmanStart, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanStart(); }
PORT_EX(UpdateState_FallingFromOwl, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_FallingFromOwl(); }
PORT_EX(UpdateState_HangingRail, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_HangingRail(); }
PORT_EX(UpdateState_SnowballAirborne, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowballAirborne(); }
PORT_EX(UpdateState_SnowmanAirborne, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SnowmanAirborne(); }
PORT_EX(UpdateState_Splitting, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_Splitting(); }
PORT_EX(UpdateState_SplittingAirborne, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_SplittingAirborne(); }
PORT_EX(UpdateWarioStates_Group4, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateWarioStates_Group4(); }
PORT_EX(SetState_BlindAirborne, 0, 0) { WarioStates4(gb).port_SetState_BlindAirborne(); }
PORT_EX(Func_1ed548, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1ed548(); }
PORT_EX(Func_1ed558, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1ed558(); }
PORT_EX(UpdateState_BlindAirborne, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BlindAirborne(); }
PORT_EX(UpdateState_BlindIdling, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BlindIdling(); }
PORT_EX(UpdateState_BlindWalking, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BlindWalking(); }
PORT_EX(UpdateState_BlindTurning, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_UpdateState_BlindTurning(); }
PORT_EX(Func_1edf93, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1edf93(); }
PORT_EX(Func_1edfa4, HOOK_MANUAL_RET, 0) { WarioStates4(gb).port_Func_1edfa4(); }
