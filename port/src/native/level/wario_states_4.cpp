// Wario's states, part 4 (src/engine/level/wario_states_4.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "../asm_module.hpp"
#include "../home/wario.hpp"

namespace wl3::level {

namespace {

class WarioStates4 : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_SetState_Splitting()
	{
		CALL(TriggerFloorTransition);
		wr8(wWarioState, WST_SPLITTING);
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	void port_UpdateState_SplitHit()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioState, WST_SPLIT_KNOCKED_BACK);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_1ffd19, ADDR_Frameset_1ffd13);
		anim(OAM2);
	}

	void port_SetState_BlindIdling()
	{
		wr8(wWarioState, WST_BLIND_IDLING);
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, wInvisibleFrame, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
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
		wr8(wWarioState, WST_BLIND_WALKING);
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
		wr8(wWarioState, WST_FALLING_FROM_OWL);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wJumpVelTable, 0x02);
		wr8(wJumpVelIndex, 0x18);
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
		wr8(wWarioState, WST_ICE_SKATIN);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_1fdd97, ADDR_Frameset_1fdd94);
		anim(OAM2);
	}

	void port_SetState_BallAirborne()
	{
		play_sfx(SFX_066);
		wr8(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		wr8(wWarioState, WST_BALL_AIRBORNE);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dd2f3, ADDR_Frameset_1dd2f6);
		anim(OAM3);
	}

	void port_SetState_FanLifting()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_FAN_LIFTING);
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
		wr8(wWarioState, WST_SNOWMAN_BUMPED);
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
		wr8(wWarioState, WST_SNOWBALL_CRASH);
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
		wr8(wWarioState, WST_SNOWMAN_WALKING);
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
		wr8(wWarioState, WST_OWL_SLOW);
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
			port_SetState_FanSpinning();
			return;
		}
		cpu_.b = 1;
		CALL(MoveWarioUp);
	}

	void port_UpdateState_BallBouncing()
	{
		anim(OAM3);
		if (!anim_ended()) {
			return;
		}
		port_SetState_BallAirborne(); // fallthrough
	}

	void port_UpdateState_BallTurning()
	{
		anim(OAM3);
		if (!anim_ended()) {
			return;
		}
		port_SetState_BallAirborne();
	}

	void port_UpdateState_SnowmanLanding()
	{
		anim(OAM2);
		if (ld_and(wGroundShakeCounter)) {
			return;
		}
		port_SetState_SnowmanIdle();
	}

	void port_UpdateState_IceSkatinStart()
	{
		anim(OAM2);
		if (!anim_ended()) {
			return;
		}
		xor_a();
		wr8(wWalkVelIndex, 0);
		port_SetState_IceSkatin(); // fallthrough
	}

	void port_UpdateWarioOwlDirection()
	{
		st(wSwimmingDirectionInput, facing_right() ? PAD_RIGHT : PAD_LEFT);
		port_SetState_OwlWario(); // fallthrough
	}

	void port_UpdateState_FanStart()
	{
		FARCALL(Func_19b25);
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		if (!anim_ended()) {
			return;
		}
		port_SetState_FanLifting(); // fallthrough
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
			port_SetState_BallTurning();
			return;
		}
		set_frameset(r ? right : left);
		port_Func_1edd24();
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
		wr8(wJumpVelIndex, 0x18);
		wr8(wIsSmashAttacking, 0x01);
		wr8(wWarioStateCounter, 0x01);
		st(wJumpVelTable, 0x02);
		frameset_by_dir(ADDR_Frameset_1ff622, ADDR_Frameset_1ff625);
		port_SetState_SnowmanAirborne();
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
		port_SetState_SnowmanAirborne();
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
		wr8(wWarioState, WST_ICE_SKATIN_START);
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
		wr8(wWarioState, WST_OWL_FAST);
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
		wr8(wSwimmingDirectionInput, in);
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
		wr8(wWarioState, WST_SPLIT_HIT);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0};
		clear(zero);
		wr8(wJumpVelTable, 0x01);
		wr8(wJumpVelIndex, 0x0e);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioSplitPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioSnowballGfx, ADDR_WarioSnowballGfx);
		set_oam(BANK_OAM_1ffa12, ADDR_OAM_1ffa12);
		wr8(wDirection, rd8(wObjDirection));
		frameset_by_dir(ADDR_Frameset_1ffd16, ADDR_Frameset_1ffd10);
		anim(OAM2);
	}

	void port_SetState_TurningIntoSnowman()
	{
		play_sfx(SFX_047);
		wr8(wWarioState, WST_SNOWMAN_START);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelIndex, 0};
		clear(zero);
		if (rd8(wJumpVelTable))
			wr8(wJumpVelIndex, 0x18);
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
		wr8(wTransformation, TRANSFORMATION_LAUNCHED);
		wr8(wTouchState, TOUCH_VANISH);
		wr8(wStingTouchState, TOUCH_VANISH);
		wr8(wca94, 0x02);
		wr8(wWarioState, WST_LAUNCHED);
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
			wr8(wDirection, DIRECTION_LEFT);
			wr8(wWarioState, WST_BALL_THROWN);
			static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter,
				wWarioStateCycles, 0};
			clear(zero);
			wr8(wJumpVelIndex, 0x6);
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
		if (rd8(wAutoMoveState) != 0x02)
			return;
		if (!anim_ended())
			return;
		wr8(wAutoMoveState, 0x03 | (1 << 7));
	}

	void port_TurnIntoSmallSnowball()
	{
		wr8(wTransformationProgress, 1);
		wr8(wWarioStateCounter, 0);
		load_gfx(BANK_WarioSnowballGfx, ADDR_WarioSnowballGfx);
		set_oam(BANK_OAM_1ff64a, ADDR_OAM_1ff64a);
		reset_anim();
		cpu_.a = alu_.and_(rd8(wWarioSlopeInteraction), 0x0f);
		cpu_.a = alu_.dec(cpu_.a);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_1ff92b : ADDR_Frameset_1ff956);
		anim(OAM2);
		port_SetState_SnowballRolling();
	}

	void port_Func_1ec215()
	{
		cpu_.b = 3;
		if (facing_right())
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
		port_SetState_IceSkatinCrash();
	}

	void port_HandleSnowmanInput()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SnowmanAirborne_Jump();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_SnowmanWalking();
			return;
		}
	}

	void port_Func_1ede96()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SnowmanAirborne_Jump();
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
			port_SetState_SnowmanIdle();
			return;
		}
		if (turn) {
			port_SetState_SnowmanTurning();
			return;
		}
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
	}

	void port_Func_1edf47()
	{
		bool right = facing_right();
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
			port_SetState_FanTurning();
			return;
		}
		if (!alu_.bit(cpu_.a, right ? B_PAD_RIGHT : B_PAD_LEFT)) {
			return;
		}
		port_farcall(gb_, right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1);
		if (b_and()) {
			return;
		}
		cpu_.b = 1;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	}

	void port_UpdateState_MagicStopping()
	{
		cpu_.a = inc_counter();
		alu_.cp(cpu_.a, 0x20);
		if (CF) {
			return;
		}
		CALL(RecoverFromTransformation); // fallthrough to UpdateState_MagicRecovering
	}

	// UpdateWarioAnimation in OAM bank `bank`, then true once it has ended
	// (ret z otherwise)
	bool anim_and_ended(uint8_t bank)
	{
		anim(bank);
		if (!anim_ended()) {
			return false;
		}
		return true;
	}

	void port_UpdateState_SnowmanBumped()
	{
		if (anim_and_ended(OAM2))
			port_SetState_SnowmanIdle();
	}

	void port_UpdateState_LaunchCrash()
	{
		if (anim_and_ended(OAM3)) {
			FARCALL(Func_2ad6a);
		}
	}

	void port_UpdateState_GrabbingOwl()
	{
		if (!anim_and_ended(OAM2))
			return;
		if (ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
			port_HandleWarioOwlInput();
			return;
		}
		port_UpdateWarioOwlDirection(); // fallthrough
	}

	void port_UpdateState_BallStart()
	{
		CALL(ApplyJumpVelocity);
		if (!airborne_collision()) {
			return;
		}
		update_pos_y();
		port_Func_1edaca(); // fallthrough
	}

	void port_UpdateStart_FanTurning()
	{
		port_Func_1edf2b();
		ld_cp(wWarioState, WST_FAN_TURNING);
		if (!ZF) {
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		if (anim_and_ended(OAM3))
			port_SetState_FanLifting();
	}

	void port_UpdateState_Swallowed()
	{
		if (rd8(wIsTurning)) {
			xor_a();
			wr8(wIsTurning, 0);
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			set_frameset(flip_dir_and() ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
		}
		anim(OAM3);
		if (ld_and(wAutoMoveState)) {
			return;
		}
		st(wDirection, DIRECTION_LEFT);
		port_SetState_Launched(); // fallthrough
	}

	void port_UpdateState_BallSentUpwards()
	{
		anim(OAM3);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			home::move_wario_y(gb_, -static_cast<int>(3));
			if (!up_collision()) {
				CALL(HandleUpwardsFloorTransition);
				return;
			}
			wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			st(wJumpVelTable, JUMP_VEL_NORMAL);
			return;
		}
		CALL(ApplyJumpVelocity);
		if (airborne_collision()) {
			update_pos_y();
			wr8(wIsIntangible, 0);
			wr8(wJumpVelIndex, 0);
			wr8(wJumpVelTable, 0);
			FARCALL(Func_206eb);
		}
	}

	void port_UpdateState_MagicRising()
	{
		FARCALL(Func_19b25);
		if (up_collision()) {
			get_pos();
			cpu_.b = PARTICLE_MAGIC_STOP;
			FARCALL(CreateParticle);
			wr8(wWarioState, WST_MAGIC_STOPPING);
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
			CALL(TriggerRoomTransition);
			return;
		}
		sfx_loop(0x86, SFX_04C);
		anim(OAM3);
		if (!front_collision()) {
			cpu_.b = 2;
			move_by_dir();
			return;
		}
		play_sfx(SFX_048);
		wr8(wWarioState, WST_LAUCH_CRASH);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dc926, ADDR_Frameset_1dc90d);
		anim(OAM3);
	}

	void port_UpdateState_BallThrown()
	{
		anim(OAM3);
		CALL(ApplyJumpVelocity);
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			cpu_.b = 3;
			CALL(MoveWarioLeft);
			return;
		}
		home::move_wario_x(gb_, -static_cast<int>(2));
		if (!airborne_collision())
			return;
		update_pos_y();
		wr8(wIsIntangible, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		// push_wram BANK(wShootGoals)
		cpu_.a = rd8(rWBK);
		push_af();
		wr8(rWBK, BANK_wShootGoals);
		ld_cp(wShootGoals, 3);
		if (!ZF) {
			pop_af();
			wr8(rWBK, cpu_.a);
			FARCALL(Func_206eb);
			return;
		}
		pop_af();
		wr8(rWBK, cpu_.a);
		wr8(wWarioState, WST_BALL_SENT_UPWARDS);
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
			port_Func_1edaca();
			return;
		}
		wr8(wWarioState, WST_BALL_START);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wJumpVelIndex, 0x18);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd313);
		anim(OAM3);
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
			CALL(TriggerRoomTransition);
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
		if (rd8(wWalkVelIndex) >= max)
			wr8(wWalkVelIndex, v);
	}

	void port_UpdateState_IceSkatin()
	{
		if (room_transition_check())
			return;
		if (front_collision()) {
			port_Func_1ec215();
			return;
		}
		sfx_loop(0x0c, SFX_ZIP_LINE_SLIDE);
		anim(OAM2);
		walk_move(0x14, 0x10);
		if (centre_collision()) {
			update_pos_y();
			return;
		}
		wr8(wWarioState, WST_ICE_SKATIN_AIRBORNE);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
	}

	void port_UpdateState_IceSkatinAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_SetState_IceSkatinCrash();
			return;
		}
		if (front_collision()) {
			port_Func_1ec215();
			return;
		}
		anim(OAM2);
		CALL(ApplyJumpVelocity);
		FARCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x14);
		if (!CF)
			st(wWalkVelIndex, 0x10);
		if (!still_in(WST_ICE_SKATIN_AIRBORNE)) {
			return;
		}
		if (!airborne_collision()) {
			if (!still_in(WST_ICE_SKATIN_AIRBORNE)) {
				return;
			}
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!still_in(WST_ICE_SKATIN_AIRBORNE)) {
			return;
		}
		st(wWalkVelIndex, 0x10);
		port_SetState_IceSkatin();
	}

	// the start of UpdateState_OwlSlow / UpdateState_OwlFast; true if it jumped
	bool owl_start(uint8_t state)
	{
		if (ld_bit(wJoypadPressed, B_PAD_B)) {
			port_Func_1ec64d();
			return true;
		}
		if (room_transition_check())
			return true;
		if (ld_and(wWaterInteraction)) {
			port_Func_1ec64d();
			return true;
		}
		if (!still_in(state)) {
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
			port_HandleWarioOwlInput();
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
				port_UpdateWarioOwlDirection();
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioRight);
		} else if (alu_.bit(cpu_.a, B_PAD_UP)) {
			if (owl_coll(WST_OWL_SLOW, BANK_CheckUpCollision, ADDR_CheckUpCollision, &blocked))
				return;
			if (blocked) {
				st(wSwimmingDirectionInput, PAD_DOWN);
				port_SetState_OwlWario();
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioUp);
		} else if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			if (owl_coll(WST_OWL_SLOW, BANK_CheckCentreCollision, ADDR_CheckCentreCollision, &blocked))
				return;
			if (blocked) {
				st(wSwimmingDirectionInput, PAD_UP);
				port_SetState_OwlWario();
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
				port_UpdateWarioOwlDirection();
				return;
			}
			home::move_wario_x(gb_, -static_cast<int>(1));
		}
	}

	void port_UpdateState_OwlFast()
	{
		if (owl_start(WST_OWL_FAST))
			return;
		if (!ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
			port_SetState_OwlWario();
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
			port_HandleWarioOwlInput();
			return;
		}
		bool blocked;
		if (owl_coll(WST_OWL_FAST, bank, coll, &blocked))
			return;
		if (blocked) {
			if (pad == PAD_DOWN)
				update_pos_y();
			return;
		}
		cpu_.b = 2;
		gb_call(gb_, move);
	}

	void port_UpdateState_SnowmanWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(0x1c, SFX_FAT_WALK);
		anim(OAM2);
		port_Func_1ede96();
		if (!still_in(WST_SNOWMAN_WALKING)) {
			return;
		}
		if (!centre_collision()) {
			port_SetState_SnowmanAirborne_Fall();
			return;
		}
		if (!ld_and(wWarioSlopeInteraction)) {
			return;
		}
		update_pos_y();
		port_TurnIntoSmallSnowball();
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
				wr8(wTransformationProgress, 3);
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
					wr8(wTransformationProgress, 2);
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
		if (rd8(wTransformationProgress) < 3) {
			SET_HL(gb_, wWarioStateCounter);
			cpu_.a = rd8(wWarioStateCounter);
			alu_.cp(cpu_.a, 0xf8);
			if (CF) {
				cpu_.a = alu_.add(cpu_.a, cpu_.b);
				wr8(wWarioStateCounter, cpu_.a);
			}
		}
		if (rd8(wWalkVelIndex) >= 0x18)
			wr8(wWalkVelIndex, 0x14);
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			port_SetState_SnowballCrash();
			return;
		}
		if (centre_collision()) {
			update_pos_y();
			return;
		}
		wr8(wWarioState, WST_SNOWBALL_AIRBORNE);
		wr8(wJumpVelIndex, 0x18);
		wr8(wJumpVelTable, 0x03);
	}

	void port_UpdateStart_FanLifting()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wIsInAirCurrent)) {
			port_SetState_FanRecovering();
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		port_Func_1edf2b();
		if (!still_in(WST_FAN_LIFTING)) {
			return;
		}
		port_Func_1edf47();
	}

	void port_UpdateState_FanSpinning()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wIsInAirCurrent)) {
			port_SetState_FanRecovering();
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		port_Func_1edf47();
	}

	void port_Func_1edf01()
	{
		CALL(ApplyJumpVelocity);
		FARCALL(Func_2b17a);
		if (rd8(wWalkVelIndex) >= 0x08)
			wr8(wWalkVelIndex, 0x04);
		ld_cp(wJumpVelIndex, 0x18);
		if (!ZF)
			return;
		st(wIsSmashAttacking, TRUE);
	}

	void port_UpdateState_BallAirborne()
	{
		anim(OAM3);
		port_Func_1ee02e();
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			return;
		}
		if (!airborne_collision()) {
			return;
		}
		update_pos_y();
		port_Func_1edaf8();
	}

	void port_Func_1ed09b()
	{
		flip_dir();
		CALL(RecoverFromTransformation);
	}

	void port_UpdateState_MagicRecovering()
	{
		CALL(RecoverFromTransformation);
	}

	void port_UpdateStart_FanRecovering()
	{
		anim(OAM3);
		if (!anim_ended()) {
			return;
		}
		CALL(RecoverFromTransformation_WithoutInvincibility);
	}

	void port_UpdateState_IceSkatinCrash()
	{
		anim(OAM2);
		if (!anim_ended()) {
			return;
		}
		get_pos();
		cpu_.b = PARTICLE_ICE;
		FARCALL(CreateParticle);
		CALL(RecoverFromTransformation);
	}

	void port_UpdateState_SplitKnockedBack()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		CALL(ApplyJumpVelocity);
		if (!airborne_collision()) {
			FARCALL(HandleWalk);
			CALL(Func_1762);
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		play_sfx(SFX_044);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		if (ld_and(wDirection))
			set_frameset(ADDR_Frameset_1ffc8e);
		else
			set_frameset(ADDR_Frameset_1ffccf);
		anim(OAM2);
		port_SetState_Splitting(); // fallthrough
	}

	void port_UpdateState_SnowmanIdle()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		port_HandleSnowmanInput();
		ld_cp(wWarioState, WST_SNOWMAN_IDLE);
		if (!ZF) {
			return;
		}
		if (!centre_collision()) {
			port_SetState_SnowmanAirborne_Fall();
			return;
		}
	}

	void port_UpdateState_SnowmanTurning()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (rd8(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SnowmanAirborne_Jump();
			return;
		}
		if (!anim_ended()) {
			return;
		}
		port_SetState_SnowmanWalking();
	}

	void port_UpdateState_SnowballCrash()
	{
		anim(OAM2);
		if (!anim_ended()) {
			return;
		}
		get_pos();
		cpu_.b = PARTICLE_SNOW;
		FARCALL(CreateParticle);
		CALL(RecoverFromTransformation);
	}

	void port_UpdateState_SnowmanStart()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (rd8(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		if (ld_and(wJumpVelTable)) {
			CALL(ApplyJumpVelocity);
			if (!airborne_collision()) {
				CALL(TriggerDownwardsFloorTransition);
				return;
			}
			CALL(TriggerFloorTransition);
			xor_a();
			wr8(wJumpVelTable, 0);
			wr8(wJumpVelIndex, 0);
		}
		anim(OAM2);
		if (!anim_ended()) {
			return;
		}
		port_SetState_SnowmanIdle(); // fallthrough
	}

	void port_UpdateState_FallingFromOwl()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(Func_2b17a);
		if (rd8(wWalkVelIndex) >= 0x08)
			wr8(wWalkVelIndex, 0x04);
		if (!airborne_collision()) {
			return;
		}
		update_pos_y();
		CALL(RecoverFromTransformation);
	}

	void port_UpdateState_HangingRail()
	{
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			wr8(wWarioStateCounter, 0x01);
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			set_frameset(ADDR_Frameset_1feeb4);
		}
		anim(OAM2);
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			CALL(RecoverFromTransformation_WithoutInvincibility);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			port_Func_1ede4d();
			return;
		}
		if (!ld_and(wAutoMoveState)) {
			port_Func_1ede69();
			return;
		}
	}

	void port_UpdateState_SnowballAirborne()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (rd8(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			port_SetState_SnowballCrash();
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(HandleWalk);
		if (rd8(wWalkVelIndex) >= 0x18)
			wr8(wWalkVelIndex, 0x14);
		if (!airborne_collision()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		port_SetState_SnowballRolling();
	}

	void port_UpdateState_SnowmanAirborne()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (rd8(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			wr8(wWarioStateCounter, 1);
			if (ld_and(wDirection))
				set_frameset(ADDR_Frameset_1ff625);
			else
				set_frameset(ADDR_Frameset_1ff622);
			anim(OAM2);
		}
		port_Func_1edf01();
		ld_cp(wWarioState, WST_SNOWMAN_AIRBORNE);
		if (!ZF) {
			return;
		}
		if (rd8(wJumpVelIndex) < 0x18) {
			if (!up_collision()) {
				return;
			}
			ld_cp(wWarioState, WST_SNOWMAN_AIRBORNE);
			if (!ZF) {
				return;
			}
			port_SetState_SnowmanAirborne_Fall();
			return;
		}
		if (!airborne_collision()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		FARCALL(CheckCentreCollision);
		if (ld_and(wWarioSlopeInteraction)) {
			port_TurnIntoSmallSnowball();
			return;
		}
		if (!rd8(wGroundShakeCounter)) {
			wr8(wGroundShakeCounter, 0x20);
			st(wIsWarioGroundShaking, TRUE);
		}
		if (!rd8(wFloorTransitionDir))
			play_sfx(SFX_024);
		wr8(wWarioState, WST_SNOWMAN_LANDING);
		static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
			wWarioStateCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		if (rd8(wDirection))
			set_frameset(ADDR_Frameset_1ff617);
		else
			set_frameset(ADDR_Frameset_1ff60c);
		anim(OAM2);
	}

	void port_UpdateState_Splitting()
	{
		anim(OAM2);
		if (anim_ended()) {
			port_Func_1ed09b();
			return;
		}
		if (centre_collision()) {
			return;
		}
		wr8(wWarioState, WST_SPLITTING_AIRBORNE);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	}

	void port_UpdateState_SplittingAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		anim(OAM2);
		if (anim_ended()) {
			port_Func_1ed09b();
			return;
		}
		CALL(ApplyJumpVelocity);
		if (airborne_collision()) {
			port_SetState_Splitting();
			return;
		}
		CALL(TriggerDownwardsFloorTransition);
	}

	// ld a, [wWarioState] / sub $b0 / rst JumpTable (table after the rst)
	// wWarioState - $b0, as the asm's jumptable
	void port_UpdateWarioStates_Group4()
	{
		switch (static_cast<uint8_t>(rd8(wWarioState) - 0xb0)) {
		case 0x00: port_UpdateState_IceSkatinStart(); return;
		case 0x01: port_UpdateState_IceSkatin(); return;
		case 0x02: port_UpdateState_IceSkatinAirborne(); return;
		case 0x03: port_UpdateState_IceSkatinCrash(); return;
		case 0x04: CALL(InvalidWarioStateReset); return;
		case 0x05: CALL(InvalidWarioStateReset); return;
		case 0x06: CALL(InvalidWarioStateReset); return;
		case 0x07: CALL(InvalidWarioStateReset); return;
		case 0x08: port_UpdateState_GrabbingOwl(); return;
		case 0x09: port_UpdateState_OwlSlow(); return;
		case 0x0a: port_UpdateState_OwlFast(); return;
		case 0x0b: port_UpdateState_ReleasingOwl(); return;
		case 0x0c: port_UpdateState_FallingFromOwl(); return;
		case 0x0d: CALL(InvalidWarioStateReset); return;
		case 0x0e: CALL(InvalidWarioStateReset); return;
		case 0x0f: CALL(InvalidWarioStateReset); return;
		case 0x10: port_UpdateState_HangingRail(); return;
		case 0x11: CALL(InvalidWarioStateReset); return;
		case 0x12: CALL(InvalidWarioStateReset); return;
		case 0x13: CALL(InvalidWarioStateReset); return;
		case 0x14: port_UpdateState_SnowmanStart(); return;
		case 0x15: port_UpdateState_SnowmanIdle(); return;
		case 0x16: port_UpdateState_SnowmanWalking(); return;
		case 0x17: port_UpdateState_SnowmanTurning(); return;
		case 0x18: port_UpdateState_SnowmanAirborne(); return;
		case 0x19: port_UpdateState_SnowmanLanding(); return;
		case 0x1a: port_UpdateState_SnowmanBumped(); return;
		case 0x1b: port_UpdateState_SnowballRolling(); return;
		case 0x1c: port_UpdateState_SnowballAirborne(); return;
		case 0x1d: port_UpdateState_SnowballCrash(); return;
		case 0x1e: CALL(InvalidWarioStateReset); return;
		case 0x1f: CALL(InvalidWarioStateReset); return;
		case 0x20: port_UpdateState_SplitHit(); return;
		case 0x21: port_UpdateState_SplitKnockedBack(); return;
		case 0x22: port_UpdateState_Splitting(); return;
		case 0x23: port_UpdateState_SplittingAirborne(); return;
		case 0x24: port_UpdateState_FanStart(); return;
		case 0x25: port_UpdateStart_FanLifting(); return;
		case 0x26: port_UpdateStart_FanTurning(); return;
		case 0x27: port_UpdateState_FanSpinning(); return;
		case 0x28: port_UpdateStart_FanRecovering(); return;
		case 0x29: CALL(InvalidWarioStateReset); return;
		case 0x2a: CALL(InvalidWarioStateReset); return;
		case 0x2b: CALL(InvalidWarioStateReset); return;
		case 0x2c: port_UpdateState_BlindIdling(); return;
		case 0x2d: port_UpdateState_BlindWalking(); return;
		case 0x2e: port_UpdateState_BlindTurning(); return;
		case 0x2f: port_UpdateState_BlindAirborne(); return;
		case 0x30: port_UpdateState_Swallowed(); return;
		case 0x31: port_UpdateState_Launched(); return;
		case 0x32: port_UpdateState_LaunchCrash(); return;
		case 0x33: CALL(InvalidWarioStateReset); return;
		case 0x34: port_UpdateState_MagicRising(); return;
		case 0x35: port_UpdateState_MagicStopping(); return;
		case 0x36: port_UpdateState_MagicRecovering(); return;
		case 0x37: CALL(InvalidWarioStateReset); return;
		case 0x38: port_UpdateState_BallStart(); return;
		case 0x39: port_UpdateState_BallBouncing(); return;
		case 0x3a: port_UpdateState_BallAirborne(); return;
		case 0x3b: port_UpdateState_BallShot(); return;
		case 0x3c: port_UpdateState_BallThrown(); return;
		case 0x3d: port_UpdateState_BallSentUpwards(); return;
		case 0x3e: port_UpdateState_BallTurning(); return;
		case 0x3f: CALL(InvalidWarioStateReset); return;
		case 0x40: CALL(InvalidWarioStateReset); return;
		case 0x41: CALL(InvalidWarioStateReset); return;
		case 0x42: CALL(InvalidWarioStateReset); return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	void port_SetState_BlindAirborne()
	{
		xor_a();
		wr8(wWarioSlopeInteraction, 0);
		wr8(wWarioState, WST_BLIND_AIRBORNE);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wIsCrouching, wAttackCounter, 0};
		clear(zero);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f94, ADDR_Frameset_15f97);
		anim(OAM1);
		if (!ld_and(wIsOnSteppableObject))
			return;
		LCALL(UpdateState_BlindAirborne);
	}

	void port_Func_1ed548()
	{
		xor_a();
		wr8(wJumpingUpwards, 0);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		port_SetState_BlindAirborne();
	}

	void port_Func_1ed558()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wJumpingUpwards, TRUE);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		port_SetState_BlindAirborne(); // fallthrough
	}

	void port_UpdateState_BlindAirborne()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (rd8(wWaterInteraction)) {
			CALL(RecoverFromTransformation);
			return;
		}
		front_collision();
		ld_cp(wWarioState, WST_BLIND_AIRBORNE);
		if (!ZF) {
			return;
		}
		if (b_and()) {
			CALL(RecoverFromTransformation);
			return;
		}
		LCALL(Func_1ee00f);
		ld_cp(wWarioState, WST_BLIND_AIRBORNE);
		if (!ZF) {
			return;
		}
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (!up_collision()) {
				return;
			}
			ld_cp(wWarioState, WST_BLIND_AIRBORNE);
			if (!ZF) {
				return;
			}
			port_Func_1ed548();
			return;
		}
		if (!airborne_collision()) {
			CALL(TriggerDownwardsFloorTransition);
			return;
		}
		LCALL(TriggerFloorTransition);
		port_SetState_BlindIdling();
	}

	void port_UpdateState_BlindIdling()
	{
		anim(OAM1);
		LCALL(Func_1edf93);
		ld_cp(wWarioState, WST_BLIND_IDLING);
		if (!ZF) {
			return;
		}
		if (!centre_collision()) {
			port_Func_1ed548();
			return;
		}
	}

	void port_UpdateState_BlindWalking()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		anim(OAM1);
		front_collision();
		ld_cp(wWarioState, WST_BLIND_WALKING);
		if (!ZF) {
			return;
		}
		if (b_and()) {
			CALL(RecoverFromTransformation);
			return;
		}
		LCALL(Func_1edfa4);
		ld_cp(wWarioState, WST_BLIND_WALKING);
		if (!ZF) {
			return;
		}
		if (!centre_collision()) {
			port_Func_1ed548();
			return;
		}
		update_pos_y();
	}

	void port_UpdateState_BlindTurning()
	{
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ed558();
			return;
		}
		if (!anim_ended()) {
			return;
		}
		port_SetState_BlindWalking();
	}

	void port_Func_1edf93()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ed558();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_BlindWalking();
			return;
		}
	}

	void port_Func_1edfa4()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ed558();
			return;
		}
		uint8_t pad = ld(wJoypadDown);
		if (alu_.bit(pad, B_PAD_RIGHT)) {
			if (!ld_and(wDirection)) {
				st(wDirection, DIRECTION_RIGHT);
				port_SetState_BlindTurning();
				return;
			}
			FARCALL(WalkIfPossible_Right);
		} else if (alu_.bit(pad, B_PAD_LEFT)) {
			ld_cp(wDirection, DIRECTION_LEFT);
			if (!ZF) {
				st(wDirection, DIRECTION_LEFT);
				port_SetState_BlindTurning();
				return;
			}
			FARCALL(WalkIfPossible_Left);
		} else {
			port_SetState_BlindIdling();
			return;
		}
		ld_cp(wWalkVelIndex, 0x10);
		if (!CF)
			st(wWalkVelIndex, 0x0c);
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates4;

PORT_COVERED(SetState_Splitting);
PORT_COVERED(UpdateState_SplitHit);
PORT_EX(SetState_BlindIdling, 0, 0) { WarioStates4(gb).port_SetState_BlindIdling(); }
PORT_COVERED(SetState_BlindWalking);
PORT_COVERED(Func_1ede4d);
PORT_COVERED(SetState_SnowballRolling);
PORT_COVERED(SetState_SnowmanAirborne);
PORT_COVERED(Func_1edd24);
PORT_COVERED(SetState_FanSpinning);
PORT_COVERED(UpdateState_ReleasingOwl);
PORT_EX(SetState_BallShot, 0, 0) { WarioStates4(gb).port_SetState_BallShot(); }
PORT_COVERED(SetState_FanRecovering);
PORT_COVERED(SetState_SnowmanTurning);
PORT_COVERED(SetState_IceSkatin);
PORT_COVERED(SetState_BallAirborne);
PORT_COVERED(SetState_FanLifting);
PORT_COVERED(SetState_IceSkatinCrash);
PORT_EX(SetState_SnowmanBumped, 0, 0) { WarioStates4(gb).port_SetState_SnowmanBumped(); }
PORT_COVERED(SetState_SnowmanIdle);
PORT_COVERED(SetState_BallTurning);
PORT_COVERED(SetState_FanTurning);
PORT_COVERED(SetState_SnowballCrash);
PORT_COVERED(SetState_SnowmanWalking);
PORT_COVERED(SetState_BlindTurning);
PORT_COVERED(SetState_OwlWario);
PORT_COVERED(Func_1ede69);
PORT_COVERED(Func_1edf2b);
PORT_COVERED(UpdateState_BallBouncing);
PORT_COVERED(UpdateState_BallTurning);
PORT_COVERED(UpdateState_SnowmanLanding);
PORT_COVERED(UpdateState_IceSkatinStart);
PORT_COVERED(UpdateWarioOwlDirection);
PORT_COVERED(UpdateState_FanStart);
PORT_COVERED(Func_1ec64d);
PORT_COVERED(Func_1edaca);
PORT_COVERED(Func_1edaf8);
PORT_COVERED(SetState_SnowmanAirborne_Fall);
PORT_COVERED(SetState_SnowmanAirborne_Jump);
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_Swallowed, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_Swallowed(); }
PORT_EX(SetState_FanStart, 0, 0) { WarioStates4(gb).port_SetState_FanStart(); }
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_IceSkatinStart, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_IceSkatinStart(); }
PORT_COVERED(HandleWarioOwlInput);
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_SplitHit, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_SplitHit(); }
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_TurningIntoSnowman, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_TurningIntoSnowman(); }
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_Launched, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_Launched(); }
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_MagicRising, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_MagicRising(); }
PORT_COVERED(UpdateState_BallShot);
PORT_COVERED(TurnIntoSmallSnowball);
PORT_EX(Func_1ec215, 0, 0) { WarioStates4(gb).port_Func_1ec215(); }
PORT_COVERED(HandleSnowmanInput);
PORT_COVERED(Func_1ede96);
PORT_COVERED(Func_1edf47);
PORT_COVERED(UpdateState_MagicStopping);
PORT_COVERED(UpdateState_SnowmanBumped);
PORT_COVERED(UpdateState_LaunchCrash);
PORT_COVERED(UpdateState_GrabbingOwl);
PORT_COVERED(UpdateState_BallStart);
PORT_COVERED(UpdateStart_FanTurning);
PORT_COVERED(UpdateState_Swallowed);
PORT_COVERED(UpdateState_BallSentUpwards);
PORT_COVERED(UpdateState_MagicRising);
PORT_COVERED(UpdateState_Launched);
PORT_COVERED(UpdateState_BallThrown);
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_BallStart, 0, R_A | R_F | R_B) { WarioStates4(gb).port_SetState_BallStart(); }
PORT_COVERED(Func_1ee00f);
PORT_COVERED(Func_1ee02e);
PORT_COVERED(UpdateState_IceSkatin);
PORT_COVERED(UpdateState_IceSkatinAirborne);
PORT_COVERED(UpdateState_OwlSlow);
PORT_COVERED(UpdateState_OwlFast);
PORT_COVERED(UpdateState_SnowmanWalking);
PORT_COVERED(UpdateState_SnowballRolling);
PORT_COVERED(UpdateStart_FanLifting);
PORT_COVERED(UpdateState_FanSpinning);
PORT_COVERED(Func_1edf01);
PORT_COVERED(UpdateState_BallAirborne);
PORT_COVERED(Func_1ed09b);
PORT_COVERED(UpdateState_MagicRecovering);
PORT_COVERED(UpdateStart_FanRecovering);
PORT_COVERED(UpdateState_IceSkatinCrash);
PORT_COVERED(UpdateState_SplitKnockedBack);
PORT_COVERED(UpdateState_SnowmanIdle);
PORT_COVERED(UpdateState_SnowmanTurning);
PORT_COVERED(UpdateState_SnowballCrash);
PORT_COVERED(UpdateState_SnowmanStart);
PORT_COVERED(UpdateState_FallingFromOwl);
PORT_COVERED(UpdateState_HangingRail);
PORT_COVERED(UpdateState_SnowballAirborne);
PORT_COVERED(UpdateState_SnowmanAirborne);
PORT_COVERED(UpdateState_Splitting);
PORT_COVERED(UpdateState_SplittingAirborne);
PORT_EX(UpdateWarioStates_Group4, 0, ALL_REGISTERS) { WarioStates4(gb).port_UpdateWarioStates_Group4(); }
PORT_COVERED(SetState_BlindAirborne);
PORT_COVERED(Func_1ed548);
PORT_COVERED(Func_1ed558);
PORT_COVERED(UpdateState_BlindAirborne);
PORT_COVERED(UpdateState_BlindIdling);
PORT_COVERED(UpdateState_BlindWalking);
PORT_COVERED(UpdateState_BlindTurning);
PORT_COVERED(Func_1edf93);
PORT_COVERED(Func_1edfa4);
