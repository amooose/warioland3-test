// Wario's states, part 4 (src/engine/level/wario_states_4.asm): ice
// skatin', the owl, the rail, snowman and snowball, split, fan, blind,
// swallowed and launched, magic and ball Wario, as plain C++ over
// WarioStateBase. Its hooks are the labels called from other files.
#include "frame.hpp"
#include "wario_state_base.hpp"

namespace wl3::level {

namespace {

class WarioStates4 : public WarioStateBase {
public:
	using WarioStateBase::WarioStateBase;

	void port_SetState_Splitting()
	{
		home::trigger_floor_transition(gb_);
		wr8(wWarioState, WST_SPLITTING);
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	void port_UpdateState_SplitHit()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioState, WST_SPLIT_KNOCKED_BACK);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1ffd19, ADDR_Frameset_1ffd13);
		anim(OAM2);
	}

	void port_SetState_BlindIdling()
	{
		wr8(wWarioState, WST_BLIND_IDLING);
		clear({wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter,
			wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, wInvisibleFrame});
		collision_box(-1, -27, -9, 9);
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	// the d-pad decides the direction (left first), else wDirection
	bool pad_right_left_first()
	{
		uint8_t pad = rd8(wJoypadDown);
		if (pad & PAD_LEFT)
			return false;
		if (pad & PAD_RIGHT)
			return true;
		return facing_right();
	}

	void port_SetState_BlindWalking()
	{
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_BLIND_WALKING);
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		reset_anim();
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		set_frameset(pad_right_left_first() ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);
		anim(OAM1);
	}

	// back to normal Wario, with his palette, then on to `next`
	void untransform()
	{
		home::clear_transformation_values(gb_);
		home::update_level_music(gb_);
		set_pal(ADDR_WarioDefaultPal);
	}

	void port_Func_1ede4d()
	{
		untransform();
		FARCALL(StartJump_FromInput);
	}

	void port_SetState_SnowballRolling()
	{
		wr8(wWarioState, WST_SNOWBALL_ROLLING);
		clear({wSFXLoopCounter, wJumpVelTable, wJumpVelIndex, wIsSmashAttacking});
	}

	void port_SetState_SnowmanAirborne()
	{
		wr8(wWarioState, WST_SNOWMAN_AIRBORNE);
		clear({wWarioStateCycles, wFrameDuration, wAnimationFrame});
		anim(OAM2);
	}

	void port_Func_1edd24()
	{
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wWarioState, WST_BALL_BOUNCING);
		reset_anim();
		anim(OAM3);
	}

	void port_SetState_FanSpinning()
	{
		wr8(wWarioState, WST_FAN_SPINNING);
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
		wr8(wWarioState, WST_BALL_SHOT);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex});
		wr8(wAutoMoveState, 1);
		wr8(wIsIntangible, 1);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd26c);
		anim(OAM3);
	}

	void port_SetState_FanRecovering()
	{
		wr8(wWarioState, WST_FAN_RECOVERING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dc384, ADDR_Frameset_1dc3c8);
		anim(OAM3);
	}

	void port_SetState_SnowmanTurning()
	{
		wr8(wWarioState, WST_SNOWMAN_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1ff605, ADDR_Frameset_1ff5fe);
		anim(OAM2);
	}

	void port_SetState_IceSkatin()
	{
		wr8(wWarioState, WST_ICE_SKATIN);
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame});
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
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_FAN_LIFTING);
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame});
		frameset_by_dir(ADDR_Frameset_1dc374, ADDR_Frameset_1dc3b8);
		anim(OAM3);
	}

	void port_SetState_IceSkatinCrash()
	{
		play_sfx(SFX_02E);
		wr8(wWarioState, WST_ICE_SKATIN_CRASH);
		reset_anim();
		set_frameset(flip_dir() ? ADDR_Frameset_1fdd9a : ADDR_Frameset_1fdda7);
		anim(OAM2);
	}

	void port_SetState_SnowmanBumped()
	{
		wr8(wWarioState, WST_SNOWMAN_BUMPED);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1ff628, ADDR_Frameset_1ff639);
		anim(OAM2);
	}

	void port_SetState_SnowmanIdle()
	{
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_SNOWMAN_IDLE);
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame});
		frameset_by_dir(ADDR_Frameset_1ff5f8, ADDR_Frameset_1ff5fb);
		anim(OAM2);
	}

	void port_SetState_BallTurning()
	{
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wWarioState, WST_BALL_TURNING);
		reset_anim();
		set_frameset(flip_dir() ? ADDR_Frameset_1dd287 : ADDR_Frameset_1dd28e);
		anim(OAM3);
	}

	void port_SetState_FanTurning()
	{
		flip_dir();
		wr8(wWarioState, WST_FAN_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dc38e, ADDR_Frameset_1dc389);
		anim(OAM3);
	}

	void port_SetState_SnowballCrash()
	{
		wr8(wTransformationProgress, 0);
		wr8(wWarioState, WST_SNOWBALL_CRASH);
		move_x(flip_dir() ? 2 : -2);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		reset_anim();
		set_frameset(ADDR_Frameset_1ff981);
		anim(OAM2);
	}

	void port_SetState_SnowmanWalking()
	{
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_SNOWMAN_WALKING);
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		reset_anim();
		set_frameset(pad_right_left_first() ? ADDR_Frameset_1ff5ef : ADDR_Frameset_1ff5e6);
		anim(OAM2);
	}

	void port_SetState_BlindTurning()
	{
		wr8(wWarioState, WST_BLIND_TURNING);
		reset_anim();
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		frameset_by_dir(ADDR_Frameset_14cbe, ADDR_Frameset_14cc5);
		anim(OAM1);
	}

	void port_SetState_OwlWario()
	{
		wr8(wWarioState, WST_OWL_SLOW);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
		reset_anim();
		if (ld_mask(wSwimmingDirectionInput, PAD_UP | PAD_DOWN))
			set_frameset(ADDR_Frameset_1fedd0);
		else
			frameset_by_dir(ADDR_Frameset_1fed7c, ADDR_Frameset_1fed91);
		anim(OAM2);
	}

	void port_Func_1ede69()
	{
		untransform();
		FARCALL(Func_1e174);
	}

	void port_Func_1edf2b()
	{
		collision(BANK_Func_19acd, ADDR_Func_19acd, check_func_19acd);
		if (!ld_and(wIsInAirCurrent)) {
			port_SetState_FanSpinning();
			return;
		}
		move_y(-1);
	}

	void port_UpdateState_BallBouncing()
	{
		anim(OAM3);
		if (anim_ended())
			port_SetState_BallAirborne(); // fallthrough
	}

	void port_UpdateState_BallTurning()
	{
		anim(OAM3);
		if (anim_ended())
			port_SetState_BallAirborne();
	}

	void port_UpdateState_SnowmanLanding()
	{
		anim(OAM2);
		if (!ld_and(wGroundShakeCounter))
			port_SetState_SnowmanIdle();
	}

	void port_UpdateState_IceSkatinStart()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWalkVelIndex, 0);
		port_SetState_IceSkatin(); // fallthrough
	}

	void port_UpdateWarioOwlDirection()
	{
		wr8(wSwimmingDirectionInput, facing_right() ? PAD_RIGHT : PAD_LEFT);
		port_SetState_OwlWario(); // fallthrough
	}

	void port_UpdateState_FanStart()
	{
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		if (anim_ended())
			port_SetState_FanLifting(); // fallthrough
	}

	void port_Func_1ec64d()
	{
		wr8(wWarioState, WST_RELEASING_OWL);
		home::release_owl(gb_);
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

	void port_Func_1edaca() { ball_bounce(ADDR_Frameset_1dd316, ADDR_Frameset_1dd323); }

	void port_Func_1edaf8() { ball_bounce(ADDR_Frameset_1dd230, ADDR_Frameset_1dd295); }

	void port_SetState_SnowmanAirborne_Fall()
	{
		wr8(wJumpVelIndex, 0x18);
		wr8(wIsSmashAttacking, 0x01);
		wr8(wWarioStateCounter, 0x01);
		wr8(wJumpVelTable, 0x02);
		frameset_by_dir(ADDR_Frameset_1ff622, ADDR_Frameset_1ff625);
		port_SetState_SnowmanAirborne();
	}

	void port_SetState_SnowmanAirborne_Jump()
	{
		play_sfx(SFX_JUMP);
		clear({wJumpVelIndex, wIsSmashAttacking, wWarioStateCounter});
		wr8(wJumpVelTable, 0x01);
		frameset_by_dir(ADDR_Frameset_1ff60c, ADDR_Frameset_1ff617);
		port_SetState_SnowmanAirborne();
	}

	void port_SetState_Swallowed()
	{
		wr8(wWarioState, WST_SWALLOWED);
		clear({wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter,
			wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable, wJumpVelIndex});
		wr8(wIsIntangible, 1);
		wr8(wAutoMoveState, 1);
		clear({wFrameDuration, wAnimationFrame, wIsTurning});
		set_pal(ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioLaunchGfx, ADDR_WarioLaunchGfx);
		set_oam(BANK_OAM_1dc3cd, ADDR_OAM_1dc3cd);
		uint8_t dir = rd8(wObjDirection);
		wr8(wDirection, dir);
		set_frameset(dir ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
		anim(OAM3);
	}

	void port_SetState_FanStart()
	{
		wr8(wWarioState, WST_FAN_START);
		collision_box(-1, -27, -9, 9);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching,
			wIsRolling, wIsSmashAttacking, wJumpVelIndex});
		wr8(wJumpVelTable, 1);
		home::update_level_music(gb_);
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
		clear({wInvisibleFrame, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching,
			wIsRolling, wIsSmashAttacking, wJumpVelTable, wJumpVelIndex});
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioIceSkatinPal);
		load_gfx(BANK_WarioBubbleGfx, ADDR_WarioBubbleGfx);
		set_oam(BANK_OAM_1fdb6d, ADDR_OAM_1fdb6d);
		uint8_t dir = rd8(wObjDirection);
		wr8(wDirection, dir);
		set_frameset(dir ? ADDR_Frameset_1fdd82 : ADDR_Frameset_1fdd8b);
		anim(OAM2);
	}

	void port_HandleWarioOwlInput()
	{
		wr8(wWarioState, WST_OWL_FAST);
		wr8(wSFXLoopCounter, 0);
		uint8_t joy = rd8(wJoypadDown);
		uint8_t in;
		if (joy & PAD_UP) {
			in = PAD_UP;
		} else if (joy & PAD_DOWN) {
			in = PAD_DOWN;
		} else if (joy & PAD_RIGHT) {
			wr8(wDirection, DIRECTION_RIGHT);
			in = PAD_RIGHT;
		} else {
			wr8(wDirection, DIRECTION_LEFT);
			in = PAD_LEFT;
		}
		wr8(wSwimmingDirectionInput, in);
		clear({wWarioStateCounter, wWarioStateCycles});
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
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking});
		wr8(wJumpVelTable, 0x01);
		wr8(wJumpVelIndex, 0x0e);
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioSplitPal);
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
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelIndex});
		if (rd8(wJumpVelTable))
			wr8(wJumpVelIndex, 0x18);
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioSnowmanPal);
		load_gfx(BANK_WarioSnowmanGfx, ADDR_WarioSnowmanGfx);
		set_oam(BANK_OAM_1feec7, ADDR_OAM_1feec7);
		frameset_by_dir(ADDR_Frameset_1ff560, ADDR_Frameset_1ff5a3);
		anim(OAM2);
	}

	void port_SetState_Launched()
	{
		wr8(wIsIntangible, 0);
		wr8(wTransformation, TRANSFORMATION_LAUNCHED);
		touch_states(TOUCH_VANISH, 0x02);
		wr8(wWarioState, WST_LAUNCHED);
		clear({wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter,
			wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable, wJumpVelIndex});
		collision_box(-1, -15, -9, 9);
		reset_anim();
		set_pal(ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioLaunchGfx, ADDR_WarioLaunchGfx);
		set_oam(BANK_OAM_1dc449, ADDR_OAM_1dc449);
		frameset_by_dir(ADDR_Frameset_1dc904, ADDR_Frameset_1dc8fb);
		anim(OAM3);
	}

	void port_SetState_MagicRising()
	{
		particle_at_wario(PARTICLE_MAGIC_1);
		particle_at_wario(PARTICLE_MAGIC_2);
		wr8(wWarioState, WST_MAGIC_RISING);
		collision_box(-1, -27, -9, 9);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching,
			wIsRolling, wIsSmashAttacking, wJumpVelIndex});
		wr8(wJumpVelTable, 1);
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioMagicPal);
		load_gfx(BANK_WarioMagicGfx, ADDR_WarioMagicGfx);
		set_oam(BANK_OAM_1dcd60, ADDR_OAM_1dcd60);
		set_frameset(ADDR_Frameset_1dcfb0);
		anim(OAM3);
	}

	void port_UpdateState_BallShot()
	{
		uint8_t state = rd8(wAutoMoveState);
		if (!state) {
			wr8(wDirection, DIRECTION_LEFT);
			wr8(wWarioState, WST_BALL_THROWN);
			clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
			wr8(wJumpVelIndex, 0x6);
			wr8(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
			reset_anim();
			set_frameset(ADDR_Frameset_1dd2ee);
			anim(OAM3);
			return;
		}
		if (state & (1 << 7)) {
			state &= 0x7f;
			wr8(wAutoMoveState, state);
			switch (state) {
			case 1: // .phase_3
				set_pal(ADDR_WarioBallPal);
				reset_anim();
				set_frameset(ADDR_Frameset_1dd26c);
				break;
			case 2: // .phase_1
				reset_anim();
				set_frameset(ADDR_Frameset_1dd2d1);
				break;
			case 3: // .phase_2
				set_pal(ADDR_WarioBallShotPal);
				reset_anim();
				set_frameset(ADDR_Frameset_1dd27a);
				break;
			default:
				return;
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
		uint8_t dir = static_cast<uint8_t>((rd8(wWarioSlopeInteraction) & 0x0f) - 1);
		wr8(wDirection, dir);
		set_frameset(dir ? ADDR_Frameset_1ff92b : ADDR_Frameset_1ff956);
		anim(OAM2);
		port_SetState_SnowballRolling();
	}

	void port_Func_1ec215()
	{
		move_x(facing_right() ? -3 : 3);
		port_SetState_IceSkatinCrash();
	}

	void port_HandleSnowmanInput()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SnowmanAirborne_Jump();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_SnowmanWalking();
	}

	void port_Func_1ede96()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SnowmanAirborne_Jump();
			return;
		}
		bool turn;
		if (!walk_or_turn(&turn)) {
			port_SetState_SnowmanIdle();
			return;
		}
		if (turn) {
			port_SetState_SnowmanTurning();
			return;
		}
		if (rd8(wWalkVelIndex) >= 0x08)
			wr8(wWalkVelIndex, 0x04);
	}

	void port_Func_1edf47()
	{
		bool right = facing_right();
		uint8_t pad = rd8(wJoypadDown);
		if (pad & (right ? PAD_LEFT : PAD_RIGHT)) {
			port_SetState_FanTurning();
			return;
		}
		if (!(pad & (right ? PAD_RIGHT : PAD_LEFT)))
			return;
		if (side_collision(right))
			return;
		move_x(right ? 1 : -1);
	}

	void port_UpdateState_MagicStopping()
	{
		if (inc_counter() < 0x20)
			return;
		home::recover_from_transformation(gb_); // fallthrough to UpdateState_MagicRecovering
	}

	void port_UpdateState_SnowmanBumped()
	{
		if (anim_and_ended(OAM2))
			port_SetState_SnowmanIdle();
	}

	void port_UpdateState_LaunchCrash()
	{
		if (anim_and_ended(OAM3))
			FARCALL(Func_2ad6a);
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
		home::apply_jump_velocity(gb_);
		if (!airborne_collision())
			return;
		update_pos_y();
		port_Func_1edaca(); // fallthrough
	}

	void port_UpdateStart_FanTurning()
	{
		port_Func_1edf2b();
		if (rd8(wWarioState) != WST_FAN_TURNING)
			return;
		sfx_loop(0x0c, SFX_07D);
		if (anim_and_ended(OAM3))
			port_SetState_FanLifting();
	}

	void port_UpdateState_Swallowed()
	{
		if (rd8(wIsTurning)) {
			clear({wIsTurning, wFrameDuration, wAnimationFrame});
			set_frameset(flip_dir() ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
		}
		anim(OAM3);
		if (ld_and(wAutoMoveState))
			return;
		wr8(wDirection, DIRECTION_LEFT);
		port_SetState_Launched(); // fallthrough
	}

	void port_UpdateState_BallSentUpwards()
	{
		anim(OAM3);
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			move_y(-3);
			if (!up_collision()) {
				home::handle_upwards_floor_transition(gb_);
				return;
			}
			wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			wr8(wJumpVelTable, JUMP_VEL_NORMAL);
			return;
		}
		home::apply_jump_velocity(gb_);
		if (airborne_collision()) {
			update_pos_y();
			clear({wIsIntangible, wJumpVelIndex, wJumpVelTable});
			FARCALL(Func_206eb);
		}
	}

	void port_UpdateState_MagicRising()
	{
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (up_collision()) {
			particle_at_wario(PARTICLE_MAGIC_STOP);
			wr8(wWarioState, WST_MAGIC_STOPPING);
			wr8(wWarioStateCounter, 0);
			reset_anim();
			anim(0x77);
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(0x77);
		move_y(-1);
		up_floor_transition();
	}

	void port_UpdateState_Launched()
	{
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (room_transition()) {
			home::trigger_room_transition(gb_);
			return;
		}
		sfx_loop(0x86, SFX_04C);
		anim(OAM3);
		if (!front_collision()) {
			move_by_dir(2);
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
		home::apply_jump_velocity(gb_);
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			move_x(-3);
			return;
		}
		move_x(-2);
		if (!airborne_collision())
			return;
		update_pos_y();
		clear({wIsIntangible, wJumpVelIndex, wJumpVelTable});
		// push_wram BANK(wShootGoals) / pop_wram
		uint8_t wbk = rd8(rWBK);
		wr8(rWBK, BANK_wShootGoals);
		bool three = rd8(wShootGoals) == 3;
		wr8(rWBK, wbk);
		if (!three) {
			FARCALL(Func_206eb);
			return;
		}
		wr8(wWarioState, WST_BALL_SENT_UPWARDS);
		play_sfx(SFX_066);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wJumpVelIndex});
		wr8(wIsIntangible, 1);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd2ee);
		anim(OAM3);
	}

	void port_SetState_BallStart()
	{
		home::update_level_music(gb_);
		collision_box(-1, -20, -9, 9);
		clear({wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking});
		set_pal(ADDR_WarioBallPal);
		load_gfx(BANK_WarioBallGfx, ADDR_WarioBallGfx);
		set_oam(BANK_OAM_1dcff1, ADDR_OAM_1dcff1);
		if (!ld_and(wJumpVelTable)) {
			port_Func_1edaca();
			return;
		}
		wr8(wWarioState, WST_BALL_START);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
		wr8(wJumpVelIndex, 0x18);
		reset_anim();
		set_frameset(ADDR_Frameset_1dd313);
		anim(OAM3);
	}

	void port_Func_1ee00f()
	{
		home::apply_jump_velocity(gb_);
		FARCALL(Func_2b17a);
		cap_walk(0x10, 0x0c);
	}

	void port_Func_1ee02e()
	{
		home::apply_jump_velocity(gb_);
		FARCALL(HandleWalk);
		cap_walk(0x08, 0x04);
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
		walk_with_velocity(facing_right());
		cap_walk(0x14, 0x10);
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
		home::apply_jump_velocity(gb_);
		FARCALL(HandleWalk);
		cap_walk(0x14, 0x10);
		if (!still_in(WST_ICE_SKATIN_AIRBORNE))
			return;
		if (!airborne_collision()) {
			if (still_in(WST_ICE_SKATIN_AIRBORNE))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (!still_in(WST_ICE_SKATIN_AIRBORNE))
			return;
		wr8(wWalkVelIndex, 0x10);
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
		return !still_in(state);
	}

	// the owl's collision check toward `pad`: the result in b
	uint8_t owl_collision(uint8_t pad)
	{
		switch (pad) {
		case PAD_RIGHT: return side_collision(true);
		case PAD_UP: return up_collision();
		case PAD_DOWN: return centre_collision();
		default: return side_collision(false);
		}
	}

	// the owl's move toward `pad`
	void owl_move(uint8_t pad, int n)
	{
		switch (pad) {
		case PAD_RIGHT: move_x(n); break;
		case PAD_UP: move_y(-n); break;
		case PAD_DOWN: move_y(n); break;
		default: move_x(-n); break;
		}
	}

	// wSwimmingDirectionInput's direction, in the order the owl tests it
	uint8_t owl_dir() const
	{
		uint8_t in = rd8(wSwimmingDirectionInput);
		if (in & PAD_RIGHT)
			return PAD_RIGHT;
		if (in & PAD_UP)
			return PAD_UP;
		if (in & PAD_DOWN)
			return PAD_DOWN;
		return PAD_LEFT;
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
		uint8_t pad = owl_dir();
		uint8_t blocked = owl_collision(pad);
		if (!still_in(WST_OWL_SLOW))
			return;
		if (!blocked) {
			owl_move(pad, 1);
			return;
		}
		// blocked: turn back
		switch (pad) {
		case PAD_RIGHT:
			wr8(wDirection, DIRECTION_LEFT);
			port_UpdateWarioOwlDirection();
			break;
		case PAD_UP:
			wr8(wSwimmingDirectionInput, PAD_DOWN);
			port_SetState_OwlWario();
			break;
		case PAD_DOWN:
			wr8(wSwimmingDirectionInput, PAD_UP);
			port_SetState_OwlWario();
			break;
		default:
			wr8(wDirection, DIRECTION_RIGHT);
			port_UpdateWarioOwlDirection();
			break;
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
		uint8_t pad = owl_dir();
		if (!ld_mask(wJoypadDown, pad)) {
			port_HandleWarioOwlInput();
			return;
		}
		uint8_t blocked = owl_collision(pad);
		if (!still_in(WST_OWL_FAST))
			return;
		if (blocked) {
			if (pad == PAD_DOWN)
				update_pos_y();
			return;
		}
		owl_move(pad, 2);
	}

	void port_UpdateState_SnowmanWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(0x1c, SFX_FAT_WALK);
		anim(OAM2);
		port_Func_1ede96();
		if (!still_in(WST_SNOWMAN_WALKING))
			return;
		if (!centre_collision()) {
			port_SetState_SnowmanAirborne_Fall();
			return;
		}
		if (!ld_and(wWarioSlopeInteraction))
			return;
		update_pos_y();
		port_TurnIntoSmallSnowball();
	}

	void port_UpdateState_SnowballRolling()
	{
		if (room_transition_check())
			return;
		uint8_t size = rd8(wTransformationProgress);
		if (size == 2) {
			// .medium_snowball
			if (rd8(wWarioStateCounter) >= 0xe0) {
				wr8(wTransformationProgress, 3);
				wr8(wFrameDuration, 0);
				wr8(wAnimationFrame, static_cast<uint8_t>(rd8(wAnimationFrame) + 0x02));
				frameset_by_dir(ADDR_Frameset_1ff970, ADDR_Frameset_1ff945);
			}
		} else if (size != 3) {
			// small snowball
			if (rd8(wWarioStateCounter) >= 0x70) {
				wr8(wTransformationProgress, 2);
				wr8(wFrameDuration, 0);
				frameset_by_dir(ADDR_Frameset_1ff963, ADDR_Frameset_1ff938);
			}
		}
		// .no_size_change
		sfx_loop(0x0c, SFX_ROLL);
		anim(OAM2);
		uint8_t moved = walk_with_velocity(facing_right());
		// .increment_counter_if_not_big
		if (rd8(wTransformationProgress) < 3) {
			uint8_t count = rd8(wWarioStateCounter);
			if (count < 0xf8)
				wr8(wWarioStateCounter, static_cast<uint8_t>(count + moved));
		}
		cap_walk(0x18, 0x14);
		if (front_collision() & 0x0f) {
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
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (!ld_and(wIsInAirCurrent)) {
			port_SetState_FanRecovering();
			return;
		}
		sfx_loop(0x0c, SFX_07D);
		anim(OAM3);
		port_Func_1edf2b();
		if (!still_in(WST_FAN_LIFTING))
			return;
		port_Func_1edf47();
	}

	void port_UpdateState_FanSpinning()
	{
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
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
		home::apply_jump_velocity(gb_);
		FARCALL(Func_2b17a);
		cap_walk(0x08, 0x04);
		if (rd8(wJumpVelIndex) == 0x18)
			wr8(wIsSmashAttacking, TRUE);
	}

	void port_UpdateState_BallAirborne()
	{
		anim(OAM3);
		port_Func_1ee02e();
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX)
			return;
		if (!airborne_collision())
			return;
		update_pos_y();
		port_Func_1edaf8();
	}

	void port_Func_1ed09b()
	{
		flip_dir();
		home::recover_from_transformation(gb_);
	}

	void port_UpdateState_MagicRecovering() { home::recover_from_transformation(gb_); }

	void port_UpdateStart_FanRecovering()
	{
		anim(OAM3);
		if (anim_ended())
			home::recover_from_transformation(gb_, false);
	}

	void port_UpdateState_IceSkatinCrash()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		particle_at_wario(PARTICLE_ICE);
		home::recover_from_transformation(gb_);
	}

	void port_UpdateState_SplitKnockedBack()
	{
		if (transition_or_water())
			return;
		home::apply_jump_velocity(gb_);
		if (!airborne_collision()) {
			FARCALL(HandleWalk);
			home::cap_walk_vel(gb_); // Func_1762
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		play_sfx(SFX_044);
		reset_anim();
		set_frameset(facing_right() ? ADDR_Frameset_1ffc8e : ADDR_Frameset_1ffccf);
		anim(OAM2);
		port_SetState_Splitting(); // fallthrough
	}

	void port_UpdateState_SnowmanIdle()
	{
		if (transition_or_water())
			return;
		port_HandleSnowmanInput();
		if (!still_in(WST_SNOWMAN_IDLE))
			return;
		if (!centre_collision())
			port_SetState_SnowmanAirborne_Fall();
	}

	void port_UpdateState_SnowmanTurning()
	{
		if (transition_or_water())
			return;
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SnowmanAirborne_Jump();
			return;
		}
		if (anim_ended())
			port_SetState_SnowmanWalking();
	}

	void port_UpdateState_SnowballCrash()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		particle_at_wario(PARTICLE_SNOW);
		home::recover_from_transformation(gb_);
	}

	void port_UpdateState_SnowmanStart()
	{
		if (transition_or_water())
			return;
		if (ld_and(wJumpVelTable)) {
			home::apply_jump_velocity(gb_);
			if (!airborne_collision()) {
				home::handle_downwards_floor_transition(gb_);
				return;
			}
			home::trigger_floor_transition(gb_);
			wr8(wJumpVelTable, 0);
			wr8(wJumpVelIndex, 0);
		}
		anim(OAM2);
		if (anim_ended())
			port_SetState_SnowmanIdle(); // fallthrough
	}

	void port_UpdateState_FallingFromOwl()
	{
		if (transition_or_water())
			return;
		home::apply_jump_velocity(gb_);
		FARCALL(Func_2b17a);
		cap_walk(0x08, 0x04);
		if (!airborne_collision())
			return;
		update_pos_y();
		home::recover_from_transformation(gb_);
	}

	void port_UpdateState_HangingRail()
	{
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			wr8(wWarioStateCounter, 0x01);
			reset_anim();
			set_frameset(ADDR_Frameset_1feeb4);
		}
		anim(OAM2);
		uint8_t pressed = rd8(wJoypadPressed);
		if (pressed & PAD_B) {
			home::recover_from_transformation(gb_, false);
			return;
		}
		if (pressed & PAD_A) {
			port_Func_1ede4d();
			return;
		}
		if (!ld_and(wAutoMoveState))
			port_Func_1ede69();
	}

	void port_UpdateState_SnowballAirborne()
	{
		if (transition_or_water())
			return;
		anim(OAM2);
		if (front_collision() & 0x0f) {
			port_SetState_SnowballCrash();
			return;
		}
		home::apply_jump_velocity(gb_);
		FARCALL(HandleWalk);
		cap_walk(0x18, 0x14);
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		port_SetState_SnowballRolling();
	}

	void port_UpdateState_SnowmanAirborne()
	{
		if (transition_or_water())
			return;
		anim(OAM2);
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			reset_anim();
			wr8(wWarioStateCounter, 1);
			set_frameset(facing_right() ? ADDR_Frameset_1ff625 : ADDR_Frameset_1ff622);
			anim(OAM2);
		}
		port_Func_1edf01();
		if (!still_in(WST_SNOWMAN_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < 0x18) {
			if (up_collision() && still_in(WST_SNOWMAN_AIRBORNE))
				port_SetState_SnowmanAirborne_Fall();
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		centre_collision();
		if (ld_and(wWarioSlopeInteraction)) {
			port_TurnIntoSmallSnowball();
			return;
		}
		if (!rd8(wGroundShakeCounter)) {
			wr8(wGroundShakeCounter, 0x20);
			wr8(wIsWarioGroundShaking, TRUE);
		}
		if (!rd8(wFloorTransitionDir))
			play_sfx(SFX_024);
		wr8(wWarioState, WST_SNOWMAN_LANDING);
		clear({wJumpVelTable, wJumpVelIndex, wIsSmashAttacking, wWarioStateCounter, wFrameDuration, wAnimationFrame});
		set_frameset(facing_right() ? ADDR_Frameset_1ff617 : ADDR_Frameset_1ff60c);
		anim(OAM2);
	}

	void port_UpdateState_Splitting()
	{
		anim(OAM2);
		if (anim_ended()) {
			port_Func_1ed09b();
			return;
		}
		if (centre_collision())
			return;
		wr8(wWarioState, WST_SPLITTING_AIRBORNE);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	}

	void port_UpdateState_SplittingAirborne()
	{
		if (transition_or_water())
			return;
		anim(OAM2);
		if (anim_ended()) {
			port_Func_1ed09b();
			return;
		}
		home::apply_jump_velocity(gb_);
		if (airborne_collision()) {
			port_SetState_Splitting();
			return;
		}
		home::handle_downwards_floor_transition(gb_);
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
		case 0x08: port_UpdateState_GrabbingOwl(); return;
		case 0x09: port_UpdateState_OwlSlow(); return;
		case 0x0a: port_UpdateState_OwlFast(); return;
		case 0x0b: port_UpdateState_ReleasingOwl(); return;
		case 0x0c: port_UpdateState_FallingFromOwl(); return;
		case 0x10: port_UpdateState_HangingRail(); return;
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
		case 0x20: port_UpdateState_SplitHit(); return;
		case 0x21: port_UpdateState_SplitKnockedBack(); return;
		case 0x22: port_UpdateState_Splitting(); return;
		case 0x23: port_UpdateState_SplittingAirborne(); return;
		case 0x24: port_UpdateState_FanStart(); return;
		case 0x25: port_UpdateStart_FanLifting(); return;
		case 0x26: port_UpdateStart_FanTurning(); return;
		case 0x27: port_UpdateState_FanSpinning(); return;
		case 0x28: port_UpdateStart_FanRecovering(); return;
		case 0x2c: port_UpdateState_BlindIdling(); return;
		case 0x2d: port_UpdateState_BlindWalking(); return;
		case 0x2e: port_UpdateState_BlindTurning(); return;
		case 0x2f: port_UpdateState_BlindAirborne(); return;
		case 0x30: port_UpdateState_Swallowed(); return;
		case 0x31: port_UpdateState_Launched(); return;
		case 0x32: port_UpdateState_LaunchCrash(); return;
		case 0x34: port_UpdateState_MagicRising(); return;
		case 0x35: port_UpdateState_MagicStopping(); return;
		case 0x36: port_UpdateState_MagicRecovering(); return;
		case 0x38: port_UpdateState_BallStart(); return;
		case 0x39: port_UpdateState_BallBouncing(); return;
		case 0x3a: port_UpdateState_BallAirborne(); return;
		case 0x3b: port_UpdateState_BallShot(); return;
		case 0x3c: port_UpdateState_BallThrown(); return;
		case 0x3d: port_UpdateState_BallSentUpwards(); return;
		case 0x3e: port_UpdateState_BallTurning(); return;
		case 0x04: case 0x05: case 0x06: case 0x07: case 0x0d: case 0x0e: case 0x0f: case 0x11: case 0x12:
		case 0x13: case 0x1e: case 0x1f: case 0x29: case 0x2a: case 0x2b: case 0x33: case 0x37: case 0x3f:
		case 0x40: case 0x41: case 0x42:
			CALL(InvalidWarioStateReset);
			return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	void port_SetState_BlindAirborne()
	{
		wr8(wWarioSlopeInteraction, 0);
		wr8(wWarioState, WST_BLIND_AIRBORNE);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wIsCrouching, wAttackCounter});
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f94, ADDR_Frameset_15f97);
		anim(OAM1);
		if (ld_and(wIsOnSteppableObject))
			port_UpdateState_BlindAirborne();
	}

	void port_Func_1ed548()
	{
		wr8(wJumpingUpwards, 0);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		port_SetState_BlindAirborne();
	}

	void port_Func_1ed558()
	{
		play_sfx(SFX_JUMP);
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wJumpingUpwards, TRUE);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		port_SetState_BlindAirborne(); // fallthrough
	}

	void port_UpdateState_BlindAirborne()
	{
		if (transition_or_water())
			return;
		{
			uint8_t hit = front_collision();
			if (!still_in(WST_BLIND_AIRBORNE))
				return;
			if (hit) {
				home::recover_from_transformation(gb_);
				return;
			}
		}
		port_Func_1ee00f();
		if (!still_in(WST_BLIND_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (up_collision() && still_in(WST_BLIND_AIRBORNE))
				port_Func_1ed548();
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		port_SetState_BlindIdling();
	}

	void port_UpdateState_BlindIdling()
	{
		anim(OAM1);
		port_Func_1edf93();
		if (!still_in(WST_BLIND_IDLING))
			return;
		if (!centre_collision())
			port_Func_1ed548();
	}

	void port_UpdateState_BlindWalking()
	{
		if (room_transition_check())
			return;
		anim(OAM1);
		{
			uint8_t hit = front_collision();
			if (!still_in(WST_BLIND_WALKING))
				return;
			if (hit) {
				home::recover_from_transformation(gb_);
				return;
			}
		}
		port_Func_1edfa4();
		if (!still_in(WST_BLIND_WALKING))
			return;
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
		if (anim_ended())
			port_SetState_BlindWalking();
	}

	void port_Func_1edf93()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ed558();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_BlindWalking();
	}

	void port_Func_1edfa4()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ed558();
			return;
		}
		bool turn;
		if (!walk_or_turn(&turn)) {
			port_SetState_BlindIdling();
			return;
		}
		if (turn) {
			port_SetState_BlindTurning();
			return;
		}
		cap_walk(0x10, 0x0c);
	}
};

// SetState_FanStart as the asm leaves the registers: BlockFunc_AirCurrent
// farcalls it from inside a collision check, and the block function's
// registers reach the check's register-level callers (the liveness script
// can't rule them out)
class FanStartExact : public AsmModule {
public:
	using AsmModule::AsmModule;

	void run()
	{
		st(wWarioState, WST_FAN_START);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
			wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
			wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		home::update_level_music(gb_);
		reset_anim();
		load_gfx(BANK_WarioFanGfx, ADDR_WarioFanGfx);
		set_oam(BANK_OAM_1dc000, ADDR_OAM_1dc000);
		frameset_by_dir(ADDR_Frameset_1dc37f, ADDR_Frameset_1dc3c3);
		anim(OAM3);
	}
};

} // namespace

} // namespace wl3::level

// Every caller of these reads no register after them (the asm liveness
// check): they clobber them all, but for SetState_FanStart (see above).
using wl3::level::WarioStates4;

PORT_COVERED(SetState_Splitting);
PORT_COVERED(UpdateState_SplitHit);
PORT_EX(SetState_BlindIdling, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_BlindIdling(); }
PORT_COVERED(SetState_BlindWalking);
PORT_COVERED(Func_1ede4d);
PORT_COVERED(SetState_SnowballRolling);
PORT_COVERED(SetState_SnowmanAirborne);
PORT_COVERED(Func_1edd24);
PORT_COVERED(SetState_FanSpinning);
PORT_COVERED(UpdateState_ReleasingOwl);
PORT_EX(SetState_BallShot, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_BallShot(); }
PORT_COVERED(SetState_FanRecovering);
PORT_COVERED(SetState_SnowmanTurning);
PORT_COVERED(SetState_IceSkatin);
PORT_COVERED(SetState_BallAirborne);
PORT_COVERED(SetState_FanLifting);
PORT_COVERED(SetState_IceSkatinCrash);
PORT_EX(SetState_SnowmanBumped, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_SnowmanBumped(); }
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
PORT_EX(SetState_Swallowed, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_Swallowed(); }
PORT_EX(SetState_FanStart, 0, ALL_REGISTERS) { wl3::level::FanStartExact(gb).run(); }
PORT_EX(SetState_IceSkatinStart, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_IceSkatinStart(); }
PORT_COVERED(HandleWarioOwlInput);
PORT_EX(SetState_SplitHit, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_SplitHit(); }
PORT_EX(SetState_TurningIntoSnowman, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_TurningIntoSnowman(); }
PORT_EX(SetState_Launched, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_Launched(); }
PORT_EX(SetState_MagicRising, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_MagicRising(); }
PORT_COVERED(UpdateState_BallShot);
PORT_COVERED(TurnIntoSmallSnowball);
PORT_EX(Func_1ec215, 0, ALL_REGISTERS) { WarioStates4(gb).port_Func_1ec215(); }
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
PORT_EX(SetState_BallStart, 0, ALL_REGISTERS) { WarioStates4(gb).port_SetState_BallStart(); }
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
PORT_EX(UpdateWarioStates_Group4, 0, ALL_REGISTERS) { wl3::level::update_wario_states_group4(gb); }
PORT_COVERED(SetState_BlindAirborne);
PORT_COVERED(Func_1ed548);
PORT_COVERED(Func_1ed558);
PORT_COVERED(UpdateState_BlindAirborne);
PORT_COVERED(UpdateState_BlindIdling);
PORT_COVERED(UpdateState_BlindWalking);
PORT_COVERED(UpdateState_BlindTurning);
PORT_COVERED(Func_1edf93);
PORT_COVERED(Func_1edfa4);

namespace wl3::level {
void update_wario_states_group4(GB *gb) { WarioStates4(gb).port_UpdateWarioStates_Group4(); }
} // namespace wl3::level
