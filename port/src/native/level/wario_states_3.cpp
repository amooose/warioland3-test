// Wario's states, part 3 (src/engine/level/wario_states_3.asm): on fire and
// hot, flat, ball o' string, fat, electric, invisible, puffy, zombie,
// bouncy, crazy, vampire and bat, in bubble, as plain C++ over
// WarioStateBase. Its hooks are the labels called from other files.
#include "wario_state_base.hpp"

namespace wl3::level {

namespace {

class WarioStates3 : public WarioStateBase {
public:
	using WarioStateBase::WarioStateBase;

	// the 16-bit big-endian counter, decremented: the new value
	uint16_t port_DecrementTransformationDuration()
	{
		uint16_t v = static_cast<uint16_t>((rd8(wTransformationDuration) << 8 | rd8(wTransformationDuration + 1)) - 1);
		wr8(wTransformationDuration + 1, static_cast<uint8_t>(v));
		wr8(wTransformationDuration, static_cast<uint8_t>(v >> 8));
		return v;
	}

	void port_Func_28c15()
	{
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		wr8(wWarioState, WST_BALL_O_STRING_AIRBORNE);
	}

	void port_SetState_Hot()
	{
		wr8(wTransformationProgress, 2);
		wr8(wWarioState, WST_HOT);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
		wr8(wStingTouchState, TOUCH_VANISH);
		wr8(wTouchState, TOUCH_VANISH);
		wr8(wca94, 0x02);
		reset_anim();

		// face the way the d-pad points
		uint8_t pad = rd8(wJoypadDown) & (PAD_RIGHT | PAD_LEFT);
		if (pad)
			wr8(wDirection, (pad & PAD_RIGHT) ? DIRECTION_RIGHT : DIRECTION_LEFT);
		frameset_by_dir(ADDR_Frameset_16b26, ADDR_Frameset_16b2f);
		anim(OAM1);
	}

	void port_SetState_FatIdling()
	{
		wr8(wWarioState, WST_FAT_IDLING);
		wr8(wTouchState, TOUCH_ATTACK);
		wr8(wStingTouchState, TOUCH_ATTACK);
		wr8(wca94, 0x01);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1781c, ADDR_Frameset_1781f);
		anim(OAM1);
	}

	void port_SetState_FatWalking()
	{
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_FAT_WALKING);
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_17822, ADDR_Frameset_1782b);
		anim(OAM1);
	}

	void port_SetState_FatTurning()
	{
		wr8(wWarioState, WST_FAT_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_178f3, ADDR_Frameset_178ea);
		anim(OAM1);
	}

	void port_SetState_FatBumping()
	{
		wr8(wWarioState, WST_FAT_BUMPING);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_178fc, ADDR_Frameset_17905);
		anim(OAM1);
	}

	void port_SetState_FatAirborne()
	{
		clear({wWarioStateCycles, wFrameDuration, wAnimationFrame});
		anim(OAM1);
		wr8(wWarioState, WST_FAT_AIRBORNE);
	}

	void port_SetState_FatLanding()
	{
		wr8(wWarioState, WST_FAT_LANDING);
		clear({wJumpVelTable, wJumpVelIndex, wIsSmashAttacking, wWarioStateCounter, wFrameDuration, wAnimationFrame});
		frameset_by_dir(ADDR_Frameset_17872, ADDR_Frameset_17887);
		anim(OAM1);
	}

	void port_SetState_FatRecovering()
	{
		wr8(wWarioState, WST_FAT_RECOVERING);
		touch_states(TOUCH_BUMP, 0x02);
		clear({wJumpVelTable, wJumpVelIndex, wIsSmashAttacking, wWarioStateCounter, wWarioStateCycles,
			wFrameDuration, wAnimationFrame});
		frameset_by_dir(ADDR_Frameset_17844, ADDR_Frameset_1785b);
		anim(OAM1);
	}

	void port_SetState_BallOString()
	{
		wr8(wWarioState, WST_BALL_O_STRING);
		wr8(wTransformationProgress, 1);
		clear({wSFXLoopCounter, wWalkVelIndex, wWarioStateCycles, wJumpVelTable, wJumpVelIndex});
		collision_box(-1, -24, -9, 9);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1741f, ADDR_Frameset_173fe);
		anim(OAM1);
	}

	void port_SetState_BallOStringKnockBack()
	{
		wr8(wTransformationProgress, 0);
		play_sfx(SFX_BUMP);
		wr8(wWarioState, WST_BALL_O_STRING_KNOCK_BACK);
		wr8(wJumpVelIndex, 0x0a);
		wr8(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		collision_box(-1, -27, -9, 9);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		reset_anim();
		set_frameset(ADDR_Frameset_1742a);
		anim(OAM1);
	}

	void port_SetState_ZombieIdling()
	{
		wr8(wWarioState, WST_ZOMBIE_IDLING);
		collision_box(-1, -27, -9, 9);
		touch_states(TOUCH_VANISH, 0x02);
		clear({wWarioStateCounter, wWarioStateCycles, wInvisibleFrame, wGrabState, wAttackCounter, wJumpVelIndex,
			wJumpVelTable, wIsCrouching, wIsRolling, wIsSmashAttacking});
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioZombiePal);
		load_gfx(BANK_WarioZombieGfx, ADDR_WarioZombieGfx);
		set_oam(BANK_OAM_1fc48b, ADDR_OAM_1fc48b);
		frameset_by_dir(ADDR_Frameset_1fc8b6, ADDR_Frameset_1fc8b9);
		anim(OAM2);
	}

	void port_SetState_ZombieWalking()
	{
		clear({wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter, wWarioStateCycles});
		wr8(wWarioState, WST_ZOMBIE_WALKING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc870, ADDR_Frameset_1fc879);
		anim(OAM2);
	}

	void port_SetState_ZombieTurning()
	{
		wr8(wWarioState, WST_ZOMBIE_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc89d, ADDR_Frameset_1fc894);
		anim(OAM2);
	}

	void port_SetState_ZombieAirborne()
	{
		wr8(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		wr8(wWarioStateCycles, 0);
		wr8(wWarioState, WST_ZOMBIE_AIRBORNE);
		reset_anim();
		anim(OAM2);
	}

	void port_SetState_ZombieKnockBack()
	{
		uint8_t state = rd8(wWarioState);
		if (state == WST_ZOMBIE_KNOCK_BACK || state == WST_ZOMBIE_SLIPPING_THROUGH_FLOOR ||
			state == WST_ZOMBIE_LANDING)
			return;
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wWarioState, WST_ZOMBIE_KNOCK_BACK);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc92e, ADDR_Frameset_1fc93b);
		anim(OAM2);
	}

	void port_SetState_ZombieRecovering()
	{
		wr8(wWarioState, WST_ZOMBIE_RECOVERING);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
	}

	void port_SetState_BouncyAirborne()
	{
		wr8(wWarioState, WST_BOUNCY_AIRBORNE);
		play_sfx(SFX_02B);
		reset_anim();
		set_frameset(ADDR_Frameset_1fcc6c);
		anim(OAM2);
	}

	void port_SetState_BouncyUpsideDown()
	{
		wr8(wWarioState, WST_BOUNCY_UPSIDE_DOWN);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		wr8(wWarioStateCycles, 0);
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		reset_anim();
		set_pal(ADDR_WarioBouncyPal);
		set_frameset(ADDR_Frameset_1fcc82);
		anim(OAM2);
	}

	void port_SetState_BouncyFloor()
	{
		clear({wJumpVelIndex, wJumpVelTable, wWarioStateCounter, wWalkVelIndex});
		wr8(wWarioState, WST_BOUNCY_FLOOR);
		reset_anim();
		set_frameset(ADDR_Frameset_1fcc5f);
		anim(OAM2);
	}

	void port_SetState_BouncyLastBounce()
	{
		wr8(wWarioState, WST_BOUNCY_LAST_BOUNCE);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		anim(OAM2);
	}

	void port_UpdateState_BouncyUpsideLanding()
	{
		if (anim_and_ended(OAM2))
			port_Func_2a2e7(); // fallthrough
	}

	void port_SetState_OnFireAirborne()
	{
		wr8(wWarioStateCycles, 0);
		wr8(wWarioState, WST_ON_FIRE_AIRBORNE);
	}

	// Hot Wario bounces off a wall: turns around and moves back 3 pixels.
	void port_Func_2afc5()
	{
		bool right = flip_dir();
		move_x(right ? 3 : -3);
		bool counter = ld_and(wWarioStateCounter);
		if (right)
			set_frameset(counter ? ADDR_Frameset_16b1d : ADDR_Frameset_16b49);
		else
			set_frameset(counter ? ADDR_Frameset_16b14 : ADDR_Frameset_16b38);
		anim(OAM1);
	}

	// True (the asm's carry) on the frames the transformation flashes as it
	// runs out: every 4th frame in the last $78, every 16th from $168 left.
	bool port_Func_2ae5c() const
	{
		uint8_t hi = rd8(wTransformationDuration);
		uint8_t lo = rd8(wTransformationDuration + 1);
		uint8_t counter = rd8(wGlobalCounter);
		if (hi >= 0x02)
			return false;
		if (hi)
			return lo < 0x68 && !(counter & 0b1111);
		if (lo >= 0x78)
			return !(counter & 0b1111);
		return !(counter & 0b11);
	}

	// Bouncy Wario flashes red as the transformation runs out.
	void port_Func_2ae3b()
	{
		if (!port_Func_2ae5c())
			return;
		uint8_t red = rd8(wWarioStateCycles) ^ 0x1;
		wr8(wWarioStateCycles, red);
		set_pal(red ? ADDR_WarioBouncyRedPal : ADDR_WarioBouncyPal);
	}

	void port_Func_29bb9()
	{
		frameset_by_dir(ADDR_Frameset_1fc8b0, ADDR_Frameset_1fc8b3);
		wr8(wWarioStateCounter, 0x01);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_ZombieAirborne();
	}

	void port_Func_29be1()
	{
		frameset_by_dir(ADDR_Frameset_1fc8a6, ADDR_Frameset_1fc8ab);
		wr8(wWarioStateCounter, 0);
		wr8(wJumpVelIndex, 0);
		port_SetState_ZombieAirborne(); // fallthrough
	}

	void port_Func_290d6()
	{
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wIsSmashAttacking, TRUE);
		wr8(wWarioStateCounter, TRUE);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		frameset_by_dir(ADDR_Frameset_1783e, ADDR_Frameset_17841);
		port_SetState_FatAirborne();
	}

	void port_Func_290a1()
	{
		play_sfx(SFX_JUMP);
		clear({wJumpVelIndex, wIsSmashAttacking, wWarioStateCounter});
		wr8(wJumpVelTable, 0x01);
		frameset_by_dir(ADDR_Frameset_17834, ADDR_Frameset_17839);
		port_SetState_FatAirborne();
	}

	void port_Func_28bd5()
	{
		flip_dir();
		if (inc_counter() >= 0x05) {
			port_SetState_BallOStringKnockBack();
			return;
		}
		frameset_by_dir(ADDR_Frameset_1741f, ADDR_Frameset_173fe);
		anim(OAM1);
	}

	void port_Func_2a0c7()
	{
		wr8(wJumpVelIndex, 0);
		port_SetState_BouncyAirborne(); // fallthrough
	}

	void port_Func_2af75()
	{
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		port_SetState_OnFireAirborne();
	}

	void port_Func_2af81()
	{
		play_sfx(SFX_JUMP);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		port_SetState_OnFireAirborne(); // fallthrough
	}

	void port_Func_2a0bb()
	{
		wr8(wJumpVelTable, JUMP_VEL_BOUNCY_HIGH_JUMP);
		port_Func_2a0c7();
	}

	void port_Func_2a0c2()
	{
		wr8(wJumpVelTable, JUMP_VEL_BOUNCY_JUMP);
		port_Func_2a0c7(); // fallthrough
	}

	void port_UpdateState_FlatSquished() {}

	void port_SetState_FlatSquished()
	{
		wr8(wWarioState, WST_FLAT_SQUISHED);
		wr8(wWarioStateCounter, 0);
	}

	void port_SetState_Hot_ResetDuration()
	{
		wr8(wTransformationDuration + 0, static_cast<uint8_t>(HOT_WARIO_TRANSITION_DURATION >> 8));
		wr8(wTransformationDuration + 1, static_cast<uint8_t>(HOT_WARIO_TRANSITION_DURATION));
		port_SetState_Hot(); // fallthrough
	}

	void port_Func_2b4d0() { particle_at_wario(PARTICLE_DUST); }

	void port_Func_2b342()
	{
		if (!(rd8(wGlobalCounter) & 1))
			move_y(-1);
	}

	void port_Func_28f6d()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_290a1();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_FatWalking();
	}

	void port_SetState_BatFalling()
	{
		wr8(wWarioState, WST_BAT_FALLING);
		wr8(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	}

	void port_UpdateState_ZombieWrithing()
	{
		anim(OAM2);
		if (ld_and(wJoypadDown))
			port_SetState_ZombieIdling();
	}

	void port_UpdateState_ZombieTurning()
	{
		if (anim_and_ended(OAM2))
			port_SetState_ZombieWalking();
	}

	void port_Func_2838a()
	{
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		wr8(wWarioState, WST_HOT_AIRBORNE);
		wr8(wWarioStateCounter, 0x08);
	}

	void port_SetState_VampireAirborne()
	{
		wr8(wWarioState, WST_VAMPIRE_AIRBORNE);
		wr8(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
	}

	void port_UpdateState_GettingWrappedInString()
	{
		if (!anim_and_ended(OAM1))
			return;
		if (!centre_collision()) {
			port_Func_28c15();
			return;
		}
		port_SetState_BallOString(); // fallthrough
	}

	// a particle at Wario's position, on the block row
	void particle_on_row(uint8_t id)
	{
		get_pos();
		wr8(hYPosLo, rd8(hYPosLo) & 0xf0);
		particle(id);
	}

	void port_Func_2ade4()
	{
		home::clear_transformation_values(gb_);
		wr8(wInvincibleCounter, 0x10);
		home::update_level_music(gb_);
		particle_on_row(PARTICLE_SMOKE_LARGE);
		play_sfx(SFX_SPLASH);
		FARCALL(DiveFromSurface_SkipSplash);
	}

	void port_UpdateState_FatTurning()
	{
		if (room_transition_check())
			return;
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_290a1();
			return;
		}
		if (anim_ended())
			port_SetState_FatWalking();
	}

	void port_UpdateState_BouncyCeiling()
	{
		if (!port_DecrementTransformationDuration()) {
			port_SetState_BouncyUpsideDown();
			return;
		}
		port_Func_2ae3b();
		if (!anim_and_ended(OAM2))
			return;
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_BouncyAirborne();
	}

	void port_SetState_FlatSquishedLifting()
	{
		wr8(wIsIntangible, TRUE);
		wr8(wWarioState, WST_FLAT_SQUISHED_LIFTING);
		wr8(wWarioStateCounter, 0);
		reset_anim();
		set_frameset(ADDR_Frameset_1716c);
		anim(OAM1);
	}

	// collision box: bottom -1, top -8, left -11, right 11 (flat Wario)
	void flat_box() { collision_box(-1, -8, -11, 11); }

	void port_UpdateState_GettingUnwrappedInString()
	{
		if (!anim_and_ended(OAM1))
			return;
		play_sfx(SFX_03D);
		wr8(wWarioState, WST_BALL_O_STRING_DIZZY);
		collision_box(-1, -27, -9, 9);
		clear({wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex});
		wr8(wIsIntangible, 1);
		reset_anim();
		load_gfx(BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
		set_oam(BANK_OAM_17bbc, ADDR_OAM_17bbc);
		set_frameset(ADDR_Frameset_17ce2);
		anim(OAM1);
	}

	void port_Func_2ac7d()
	{
		wr8(wWarioState, WST_IN_BUBBLE);
		collision_box(-1, -27, -9, 9);
		clear({wSFXLoopCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelTable, wJumpVelIndex});
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioBubblePal);
		load_gfx(BANK_WarioBubbleGfx, ADDR_WarioBubbleGfx);
		set_oam(BANK_OAM_1fd87c, ADDR_OAM_1fd87c);
		uint8_t dir = rd8(wObjDirection);
		wr8(wDirection, dir);
		wr8(wWarioStateCounter, dir);
		set_frameset(ADDR_Frameset_1fdb2c);
		anim(OAM2);
	}

	void port_SetState_VampireTransforming()
	{
		play_sfx(SFX_045);
		wr8(wWarioState, WST_VAMPIRE_TRANSFORMING);
		touch_states(TOUCH_VANISH, 0x02);
		collision_box(-1, -27, -9, 9);
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wJumpVelTable, wJumpVelIndex});
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioVampirePal);
		load_gfx(BANK_WarioBatGfx, ADDR_WarioBatGfx);
		set_oam(BANK_OAM_1fd031, ADDR_OAM_1fd031);
		set_frameset(ADDR_Frameset_1fd439);
		anim(OAM2);
	}

	// wWarioOAMPtr alone (the bank stays)
	void set_oam_ptr(uint16_t ptr)
	{
		wr8(wWarioOAMPtr + 0, static_cast<uint8_t>(ptr >> 8));
		wr8(wWarioOAMPtr + 1, static_cast<uint8_t>(ptr));
	}

	void port_SetState_VampireIdling()
	{
		wr8(wWarioState, WST_VAMPIRE_IDLING);
		reset_anim();
		load_gfx(BANK_WarioVampireGfx, ADDR_WarioVampireGfx);
		set_oam_ptr(ADDR_OAM_1fd498);
		set_frameset(ADDR_Frameset_1fd81c);
		anim(OAM2);
	}

	void port_SetState_FlatIdling()
	{
		wr8(wWarioState, WST_FLAT_IDLING);
		clear({wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
			wJumpVelIndex});
		flat_box();
		set_frameset(ADDR_Frameset_1718b);
		anim(OAM1);
	}

	void port_SetState_FlatSinking()
	{
		wr8(wWarioState, WST_FLAT_SINKING);
		clear({wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable});
		wr8(wIsIntangible, 1);
		flat_box();
		reset_anim();
		set_frameset(ADDR_Frameset_1719a);
		anim(OAM1);
	}

	// hPos = Wario's position + (-ysub, +-dx)
	void pos_offset(bool add, uint8_t dx, uint8_t ysub)
	{
		uint16_t x = static_cast<uint16_t>(rd8(wWarioXPos + 0) << 8 | rd8(wWarioXPos + 1));
		x = static_cast<uint16_t>(add ? x + dx : x - dx);
		wr8(hXPosLo, static_cast<uint8_t>(x));
		wr8(hXPosLo - 1, static_cast<uint8_t>(x >> 8));
		uint16_t y = static_cast<uint16_t>((rd8(wWarioXPos - 2) << 8 | rd8(wWarioXPos - 1)) - ysub);
		wr8(hXPosLo - 2, static_cast<uint8_t>(y));
		wr8(hXPosLo - 3, static_cast<uint8_t>(y >> 8));
	}

	void port_Func_289c5()
	{
		pos_offset(false, 0x04, 0x08);
		particle(PARTICLE_BUBBLES);
		wr8(wWarioState, WST_FLAT_STRETCHING_UNDERWATER);
		clear({wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable});
		flat_box();
		reset_anim();
		set_frameset(ADDR_Frameset_171a3);
		anim(OAM1);
	}

	void port_Func_2abc1()
	{
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fd42c, ADDR_Frameset_1fd434);
		anim(OAM2);
	}

	void port_SetState_ZombieWrithing()
	{
		wr8(wWarioState, WST_ZOMBIE_WRITHING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc882, ADDR_Frameset_1fc88b);
		anim(OAM2);
	}

	void port_UpdateState_ZombieKnockBack()
	{
		if (anim_and_ended(OAM2))
			port_SetState_ZombieIdling();
	}

	void port_UpdateState_ZombieLanding()
	{
		if (!anim_and_ended(OAM2))
			return;
		if (rd8(wWarioStateCounter) != 1) {
			port_SetState_ZombieIdling();
			return;
		}
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		wr8(wWarioStateCycles, 0x06);
		wr8(wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc928, ADDR_Frameset_1fc92b);
		anim(OAM2);
	}

	void port_Func_2a2e7()
	{
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		set_pal(ADDR_WarioBouncyPal);
		frameset_by_dir(ADDR_Frameset_1fccbf, ADDR_Frameset_1fccb2);
		reset_anim();
		port_SetState_BouncyLastBounce();
	}

	void port_UpdateState_BouncyFloor()
	{
		if (!port_DecrementTransformationDuration()) {
			port_Func_2a2e7();
			return;
		}
		port_Func_2ae3b();
		if (!anim_and_ended(OAM2))
			return;
		port_Func_2a0b2();
	}

	void port_Func_2926a()
	{
		particle_on_row(PARTICLE_SPLASH);
		play_sfx(SFX_SPLASH);
		wr8(wWarioState, WST_FAT_SINKING);
		clear({wJumpVelTable, wJumpVelIndex, wIsSmashAttacking, wWarioStateCounter, wWarioStateCycles,
			wFrameDuration, wAnimationFrame});
		wr8(wIsIntangible, 1);
		frameset_by_dir(ADDR_Frameset_17872, ADDR_Frameset_17887);
		anim(OAM1);
	}

	void port_UpdateState_FatLanding()
	{
		uint16_t left = port_DecrementTransformationDuration();
		if (left >= 0x200 || !left) {
			port_SetState_FatRecovering();
			return;
		}
		anim(OAM1);
		if (!ld_and(wGroundShakeCounter))
			port_SetState_FatIdling();
	}

	void port_SetState_FlatWalking()
	{
		wr8(wWarioState, WST_FLAT_WALKING);
		clear({wSFXLoopCounter, wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wJumpVelTable, wJumpVelIndex});
		set_frameset(ADDR_Frameset_17175);
		anim(OAM1);
		wr8(wDirection, ld_bit(wJoypadDown, B_PAD_RIGHT) ? DIRECTION_RIGHT : DIRECTION_LEFT);
	}

	void port_SetState_FlatFalling()
	{
		wr8(wWarioState, WST_FLAT_FALLING);
		clear({wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles,
			wWarioSlopeInteraction});
		wr8(wJumpVelIndex, 1);
		wr8(wJumpVelTable, 1);
		frameset_by_dir(ADDR_Frameset_171b8, ADDR_Frameset_17186);
		anim(OAM1);
	}

	void port_Func_29317()
	{
		pos_offset(facing_right(), 0x04, 0x10);
		particle(PARTICLE_BUBBLES);
	}

	void port_SetState_TurningInvisible()
	{
		play_sfx(SFX_03F);
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable,
			wIsCrouching, wIsRolling, wIsSmashAttacking});
		wr8(wIsIntangible, 1);
		wr8(wWarioState, WST_TURNING_INVISIBLE);
		collision_box(-1, -27, -9, 9);
		clear({wTouchState, wStingTouchState, wca94});
		reset_anim();
		set_pal(ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
		set_oam(BANK_OAM_17cf7, ADDR_OAM_17cf7);
		frameset_by_dir(ADDR_Frameset_17e1b, ADDR_Frameset_17e3a);
		anim(OAM1);
	}

	void port_SetState_CrazyTurning()
	{
		wr8(wWarioState, WST_CRAZY_TURNING);
		reset_anim();
		set_frameset(flip_dir() ? ADDR_Frameset_1fcfd7 : ADDR_Frameset_1fcfd0);
		anim(OAM2);
	}

	void port_SetState_VampireTurning()
	{
		wr8(wWarioState, WST_VAMPIRE_TURNING);
		reset_anim();
		set_frameset(flip_dir() ? ADDR_Frameset_1fd83a : ADDR_Frameset_1fd841);
		anim(OAM2);
	}

	void port_SetState_CrazyAirborne()
	{
		wr8(wWarioState, WST_CRAZY_AIRBORNE);
		wr8(wJumpVelTable, JUMP_VEL_NORMAL);
		clear({wWalkVelIndex, wFrameDuration, wAnimationFrame});
		frameset_by_dir(ADDR_Frameset_1fcf9f, ADDR_Frameset_1fcf6e);
		anim(OAM2);
	}

	void port_SetState_PuffyTurning()
	{
		flip_dir();
		wr8(wWarioState, WST_PUFFY_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dcce3, ADDR_Frameset_1dccd8);
		anim(rd8(wWarioOAMBank));
	}

	void port_SetState_BatIdling()
	{
		wr8(wWarioState, WST_BAT_IDLING);
		clear({wSFXLoopCounter, wJumpVelIndex, wJumpVelTable});
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fd45f, ADDR_Frameset_1fd46c);
		anim(OAM2);
	}

	void port_SetState_PuffyDeflating()
	{
		play_sfx(SFX_027);
		wr8(wWarioState, WST_PUFFY_DEFLATING);
		clear({wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame});
		wr8(wIsIntangible, 1);
		frameset_by_dir(ADDR_Frameset_1dccee, ADDR_Frameset_1dcd1e);
		anim(rd8(wWarioOAMBank));
	}

	void port_SetState_PuffyRising()
	{
		wr8(wWarioState, WST_PUFFY_RISING);
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame});
		wr8(wJumpVelTable, 1);
		set_pal(ADDR_WarioDefaultPal);
		frameset_by_dir(ADDR_Frameset_1dcd4e, ADDR_Frameset_1dcd57);
		anim(rd8(wWarioOAMBank));
	}

	void port_SetState_BatTransforming()
	{
		play_sfx(SFX_046);
		wr8(wWarioState, WST_BAT_TRANSFORMING);
		wr8(wCollisionBoxTop, static_cast<uint8_t>(-15));
		reset_anim();
		load_gfx(BANK_WarioBatGfx, ADDR_WarioBatGfx);
		set_oam_ptr(ADDR_OAM_1fd031);
		frameset_by_dir(ADDR_Frameset_1fd40a, ADDR_Frameset_1fd479);
		anim(OAM2);
	}

	void port_SetState_VampireWalking()
	{
		wr8(wWarioState, WST_VAMPIRE_WALKING);
		clear({wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
			wJumpVelIndex});
		uint8_t pad = rd8(wJoypadDown);
		bool right;
		if (pad & PAD_LEFT)
			right = false;
		else if (pad & PAD_RIGHT)
			right = true;
		else
			right = facing_right();
		wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		set_frameset(right ? ADDR_Frameset_1fd82e : ADDR_Frameset_1fd825);
		anim(OAM2);
	}

	void port_Func_2a675()
	{
		wr8(wJumpVelIndex, 0);
		port_SetState_CrazyAirborne();
	}

	void port_Func_2a67b()
	{
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_CrazyAirborne(); // fallthrough
	}

	void port_Func_2a8a7()
	{
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_SetState_VampireAirborne();
	}

	void port_Func_2a0b2()
	{
		if (ld_bit(wJoypadDown, B_PAD_A))
			port_Func_2a0bb();
		else
			port_Func_2a0c2();
	}

	void port_Func_2a8ae()
	{
		play_sfx(SFX_01D);
		wr8(wJumpVelIndex, 0);
		port_SetState_VampireAirborne(); // fallthrough
	}

	void port_Func_28380()
	{
		center_on_ladder();
		port_Func_2838a(); // fallthrough
	}

	void port_UpdateState_BatTransforming()
	{
		if (anim_and_ended(OAM2))
			port_SetState_BatIdling(); // fallthrough
	}

	void port_UpdateState_PuffyInflating()
	{
		if (anim_and_ended(rd8(wWarioOAMBank)))
			port_SetState_PuffyRising(); // fallthrough
	}

	void port_UpdateState_GettingFlat()
	{
		if (!anim_and_ended(OAM1))
			return;
		wr8(wIsIntangible, 0);
		if (ld_and(wWarioStateCounter))
			port_SetState_FlatSquished();
		else
			port_SetState_FlatIdling(); // fallthrough
	}

	void port_UpdateState_FatIdling()
	{
		if (room_transition_check())
			return;
		port_Func_28f6d();
		if (!still_in(WST_FAT_IDLING))
			return;
		if (!centre_collision())
			port_Func_290d6();
	}

	void port_UpdateState_FlatSquishedLifting()
	{
		anim(OAM1);
		move_y(-1);
		if (inc_counter() < 0x32)
			return;
		wr8(wIsIntangible, 0);
		port_SetState_FlatFalling();
	}

	void port_Func_2b63b()
	{
		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			port_SetState_BatFalling();
			return;
		}
		uint8_t index = static_cast<uint8_t>(rd8(wJumpVelIndex) + 1);
		wr8(wJumpVelIndex, index);
		if (index >= MAX_JUMP_VEL_INDEX + 1) {
			port_SetState_BatFalling();
			return;
		}
		// de = the jump velocity index
		cpu_.d = 0x00;
		cpu_.e = index;
		FARCALL(Func_1f863);
	}

	void port_Func_2b34e()
	{
		uint8_t pad = rd8(wJoypadDown);
		if (pad & PAD_A) {
			port_Func_29be1();
			return;
		}
		if (pad & (PAD_RIGHT | PAD_LEFT)) {
			port_SetState_ZombieWalking();
			return;
		}
		if (!centre_collision()) {
			port_Func_29bb9();
			return;
		}
		uint8_t lo = rd8(wWarioStateCounter);
		wr8(wWarioStateCounter, static_cast<uint8_t>(lo + 1));
		uint8_t hi = static_cast<uint8_t>(rd8(wWarioStateCounter + 1) + (lo == 0xff));
		wr8(wWarioStateCounter + 1, hi);
		if (hi >= 0x02)
			port_SetState_ZombieWrithing();
	}

	void port_UpdateState_FatEating()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_Func_2926a();
			return;
		}
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
		if (!anim_and_ended(OAM1))
			return;
		wr8(wCollisionBoxLeft, static_cast<uint8_t>(-9));
		wr8(wCollisionBoxRight, 9);
		port_SetState_FatIdling(); // fallthrough
	}

	void port_UpdateState_FlatSinking()
	{
		anim(OAM1);
		if (room_transition_check())
			return;
		if (!(rd8(wGlobalCounter) & 0b1111)) {
			// call z, .Func_2899a
			pos_offset(false, 0x04, 0x08);
			particle(PARTICLE_BUBBLES);
		}
		if (rd8(wGlobalCounter) & 0b1)
			return;
		move_y(1);
		if (centre_collision())
			port_Func_289c5();
	}

	void port_Func_2ad6a()
	{
		home::clear_transformation_values(gb_);
		wr8(wInvincibleCounter, 0x10);
		home::update_level_music(gb_);
		FARCALL(SetState_Submerged);
		reset_anim();
		wr8(wWarioStateCycles, 1);
		set_pal(ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		frameset_by_dir(ADDR_Frameset_155a6, ADDR_Frameset_155ab);
		anim(OAM1);
		play_sfx(SFX_RECOVERY);
	}

	void port_SetState_Crazy()
	{
		play_sfx(SFX_043);
		wr8(wWarioState, WST_CRAZY);
		wr8(wTouchState, TOUCH_BUMP);
		wr8(wStingTouchState, TOUCH_VULNERABLE);
		wr8(wca94, 0x01);
		clear({wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex});
		reset_anim();
		load_gfx(BANK_WarioCrazyGfx, ADDR_WarioCrazyGfx);
		set_oam(BANK_OAM_1fccd5, ADDR_OAM_1fccd5);
		frameset_by_dir(ADDR_Frameset_1fcf9f, ADDR_Frameset_1fcf6e);
		anim(OAM2);
	}

	void port_SetState_CrazySpinning()
	{
		play_sfx(SFX_042);
		wr8(wWarioState, WST_CRAZY_SPINNING);
		collision_box(-1, -27, -9, 9);
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking});
		if (rd8(wJumpVelTable))
			wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		home::update_level_music(gb_);
		reset_anim();
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		uint8_t dir = rd8(wObjDirection);
		wr8(wDirection, dir);
		set_frameset(dir ? ADDR_Frameset_14ce1 : ADDR_Frameset_14ccc);
		anim(OAM1);
	}

	void port_SetState_PuffyInflating()
	{
		wr8(wTransformation, TRANSFORMATION_PUFFY_WARIO);
		wr8(wStingTouchState, TOUCH_VULNERABLE);
		wr8(wTouchState, TOUCH_BUMP);
		wr8(wca94, 0x01);
		play_sfx(SFX_025);
		wr8(wWarioState, WST_PUFFY_INFLATING);
		collision_box(-1, -27, -9, 9);
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable,
			wIsCrouching, wIsRolling, wIsSmashAttacking, wSwimVelIndex, wWaterSurfaceFloatingCounter, wWaterCurrent,
			wLastWaterCurrent, wCurWaterCurrent});
		home::update_level_music(gb_);
		reset_anim();
		load_gfx(BANK_WarioPuffyGfx, ADDR_WarioPuffyGfx);
		set_oam(BANK_OAM_1dc93f, ADDR_OAM_1dc93f);
		frameset_by_dir(ADDR_Frameset_1dccc0, ADDR_Frameset_1dccc9);
		anim(rd8(wWarioOAMBank));
	}

	void port_UpdateState_GettingFlatAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_SetState_FlatSinking();
			return;
		}
		move_y(2);
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		wr8(wWarioState, WST_GETTING_FLAT);
		clear({wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCycles, wJumpVelIndex, wJumpVelTable,
			wAutoMoveState});
		wr8(wIsIntangible, 1);
		flat_box();
		set_frameset(ADDR_Frameset_1715f);
		anim(OAM1);
	}

	void port_SetState_FlatAirborne()
	{
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
			return;
		if (rd8(wTransformation) == TRANSFORMATION_OWL_WARIO)
			home::release_owl(gb_);
		// push_wram BANK("Level Objects WRAM"): Wario lands at the object's x
		// (8 to its left, but for Richtertoffen)
		uint8_t wbk = rd8(rWBK);
		wr8(rWBK, BANK_wObjects);
		uint16_t obj_x = static_cast<uint16_t>((rd8(wObjPtr + 0) << 8 | rd8(wObjPtr + 1)) + OBJ_X_POS);
		uint8_t dx = 0;
		if ((rd8(wObjInteractionType) & INTERACTION_MASK) == OBJ_INTERACTION_RICHTERTOFFEN) {
			wr8(wWarioStateCounter, 0);
		} else {
			wr8(wWarioStateCounter, 0x01);
			dx = 0x8;
		}
		uint8_t lo = rd8(obj_x);
		wr8(wWarioXPos + 1, static_cast<uint8_t>(lo - dx));
		wr8(wWarioXPos + 0, static_cast<uint8_t>(rd8(static_cast<uint16_t>(obj_x + 1)) - (lo < dx)));
		wr8(rWBK, wbk);
		wr8(wTransformation, TRANSFORMATION_FLAT_WARIO);
		play_sfx(SFX_01B);
		home::update_level_music(gb_);
		wr8(wTouchState, TOUCH_BUMP);
		wr8(wStingTouchState, TOUCH_BUMP);
		wr8(wca94, 0x02);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCycles, wGrabState, wAttackCounter, wJumpVelIndex,
			wIsCrouching, wIsRolling, wIsSmashAttacking});
		wr8(wAutoMoveState, 1);
		wr8(wJumpVelTable, 1);
		wr8(wWarioState, WST_GETTING_FLAT_AIRBORNE);
		flat_box();
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_16e9d, ADDR_OAM_16e9d);
		set_frameset(ADDR_Frameset_171bd);
		anim(OAM1);
	}

	void port_UpdateState_FatBumping()
	{
		if (anim_and_ended(OAM1))
			port_SetState_FatIdling();
	}

	void port_UpdateState_ZombieIdling()
	{
		FARCALL(Func_19b25);
		if (!still_in(WST_ZOMBIE_IDLING))
			return;
		uint8_t hit = centre_collision();
		if (!still_in(WST_ZOMBIE_IDLING))
			return;
		if (!hit) {
			port_Func_29bb9();
			return;
		}
		update_pos_y();
		anim(OAM2);
		port_Func_2b34e();
	}

	void port_Func_2b1cc()
	{
		bool right = facing_right();
		move_x(right ? 2 : -2);
		if (side_collision(right))
			move_x(right ? -2 : 2);
		jump_vel_move();
	}

	void port_UpdateState_BallOString()
	{
		if (room_transition_check())
			return;
		sfx_loop(12, SFX_ROLL);
		anim(OAM1);
		walk_with_velocity(facing_right());
		cap_walk(0x18, 0x14);
		if (front_collision() & 0x0f) {
			port_Func_28bd5();
			return;
		}
		if (!centre_collision()) {
			port_Func_28c15();
			return;
		}
		update_pos_y();
		uint8_t slope = rd8(wWarioSlopeInteraction);
		if (!slope)
			return;
		// a slope down the other way
		if (static_cast<uint8_t>((slope & 0x0f) - 1) ^ rd8(wDirection))
			port_Func_28bd5();
	}

	// turns to the d-pad's way with the new frameset if facing the other
	// way, then moves 1 unless a wall is in front
	void port_HandleHotWarioInput()
	{
		uint8_t pad = rd8(wJoypadDown) & (PAD_RIGHT | PAD_LEFT);
		if (!pad)
			return;
		bool right = pad & PAD_RIGHT;
		if (facing_right() != right) {
			wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
			reset_anim();
			set_frameset(right ? ADDR_Frameset_16b2f : ADDR_Frameset_16b26);
			anim(OAM1);
		}
		if (front_collision())
			return;
		move_x(right ? 1 : -1);
	}

	void port_SetState_ZombieLanding()
	{
		play_sfx(SFX_02A);
		touch_states(TOUCH_BUMP, 0x02);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wWarioState, WST_ZOMBIE_LANDING);
		clear({wWarioStateCounter, wFrameDuration, wAnimationFrame});
		FARCALL(Func_19a77);
		if (rd8(wc1ca)) {
			wr8(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_1fc8bc, ADDR_Frameset_1fc8d5);
		} else {
			frameset_by_dir(ADDR_Frameset_1fc948, ADDR_Frameset_1fc953);
		}
		anim(OAM2);
	}

	void port_SetState_BouncyStart()
	{
		wr8(wWarioState, WST_BOUNCY_START);
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		wr8(wJumpVelTable, 0x02);
		collision_box(-1, -29, -9, 9);
		clear({wInvisibleFrame, wWalkVelIndex, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter,
			wIsCrouching, wIsRolling, wIsSmashAttacking});
		home::update_level_music(gb_);
		reset_anim();
		set_pal(ADDR_WarioBouncyPal);
		load_gfx(BANK_WarioBouncyGfx, ADDR_WarioBouncyGfx);
		set_oam(BANK_OAM_1fc95e, ADDR_OAM_1fc95e);
		set_frameset(ADDR_Frameset_1fcc6c);
		anim(OAM2);
		if (side_collision(false))
			move_x(2);
	}

	void port_UpdateState_TurningInvisible()
	{
		if (!anim_and_ended(OAM1))
			return;
		wr8(wIsIntangible, 0);
		wr8(wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
		home::update_level_music(gb_);
		FARCALL(SetState_Idling);
	}

	void port_Func_2b17a()
	{
		uint8_t pad = rd8(wJoypadDown);
		if (pad & PAD_RIGHT)
			FARCALL(WalkRight);
		else if (pad & PAD_LEFT)
			FARCALL(WalkLeft);
	}

	void port_HandleWalk()
	{
		if (facing_right())
			FARCALL(WalkRight);
		else
			FARCALL(WalkLeft);
	}

	void port_Func_2b239()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_290a1();
			return;
		}
		bool turn;
		if (!walk_or_turn(&turn)) {
			port_SetState_FatIdling();
			return;
		}
		if (turn) {
			port_SetState_FatTurning();
			return;
		}
		cap_walk(0x08, 0x04);
	}

	void port_Func_2b381()
	{
		if (ld_bit(wJoypadDown, B_PAD_A)) {
			port_Func_29be1();
			return;
		}
		bool turn;
		if (!walk_or_turn(&turn)) {
			port_SetState_ZombieIdling();
			return;
		}
		if (turn) {
			port_SetState_ZombieTurning();
			return;
		}
		cap_walk(0x04, 0x00);
	}

	void port_Func_2ae8a()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A) && !up_collision()) {
			port_Func_2af81();
			return;
		}
		bool right = facing_right();
		if (!front_collision()) {
			walk_with_velocity(right);
			cap_walk(0x14, 0x10);
			return;
		}
		// bumped into a wall: turn around
		bool counter;
		if (right) {
			move_x(-2);
			wr8(wDirection, DIRECTION_LEFT);
			counter = ld_and(wWarioStateCounter);
			set_frameset(counter ? ADDR_Frameset_16b14 : ADDR_Frameset_16b38);
		} else {
			move_x(2);
			wr8(wDirection, DIRECTION_RIGHT);
			counter = ld_and(wWarioStateCounter);
			set_frameset(counter ? ADDR_Frameset_16b1d : ADDR_Frameset_16b49);
		}
		anim(OAM1);
		if (!centre_collision())
			return;
		if (front_collision())
			port_SetState_Hot_ResetDuration();
	}

	void port_UpdateState_Hot()
	{
		if (room_transition_check())
			return;
		sfx_loop(16, SFX_01F);
		anim(OAM1);
		uint16_t left = port_DecrementTransformationDuration();
		if (left == 1) {
			// .burnt
			play_sfx(SFX_022);
			wr8(wWarioState, WST_BURNT);
			clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
			set_pal(ADDR_WarioOnFirePal);
			load_gfx(BANK_WarioBurntGfx, ADDR_WarioBurntGfx);
			set_oam(BANK_OAM_16b6c, ADDR_OAM_16b6c);
			frameset_by_dir(ADDR_Frameset_16e47, ADDR_Frameset_16e72);
			anim(OAM1);
			return;
		}
		// the palette gets hotter as the time runs out
		if (left == 224)
			set_pal(ADDR_WarioHot1Pal);
		else if (left == 164)
			set_pal(ADDR_WarioHot2Pal);
		else if (left == 60)
			set_pal(ADDR_WarioHot3Pal);
		port_HandleHotWarioInput();
		if (!still_in(WST_HOT))
			return;
		if (!centre_collision()) {
			port_Func_28380();
			return;
		}
		update_pos_y();
	}

	void port_SetState_BatFlying()
	{
		wr8(wWarioState, WST_BAT_FLYING);
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 1);
		port_Func_2abc1();
	}

	void port_SetState_FlatJumping()
	{
		play_sfx(SFX_01D);
		wr8(wWarioState, WST_FLAT_JUMPING);
		clear({wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelIndex});
		wr8(wJumpVelTable, ld_bit(wJoypadDown, B_PAD_UP) ? JUMP_VEL_NORMAL : JUMP_VEL_KNOCK_BACK);
		set_frameset(ADDR_Frameset_17190);
		anim(OAM1);
		if (ld_and(wIsOnSteppableObject))
			port_UpdateState_FlatJumping(); // fallthrough
	}

	// moves Wario n pixels forward unless a wall is in front, in which case
	// he turns (the .asm_2b45d/.asm_2b495 paths of Func_2b42b)
	void crazy_move(int n)
	{
		bool right = facing_right();
		if (side_collision(right)) {
			port_SetState_CrazyTurning();
			return;
		}
		move_x(right ? n : -n);
	}

	// Crazy Wario's stagger, by the step count in wWalkVelIndex
	void port_Func_2b42b()
	{
		uint8_t step = static_cast<uint8_t>(rd8(wWalkVelIndex) + 1);
		wr8(wWalkVelIndex, step);
		if (step < 0x06)
			crazy_move(2);
		else if (step == 0x06)
			port_Func_2b4d0();
		else if (step < 0x09)
			return;
		else if (step < 0x12)
			crazy_move(2);
		else if (step == 0x12)
			port_Func_2b4d0();
		else if (step < 0x15)
			return;
		else if (step < 0x18)
			crazy_move(2);
		else if (step < 0x22)
			crazy_move(1);
		else if (step == 0x22)
			port_Func_2b4d0();
		else if (step < 0x25 || step >= 0x2c)
			return;
		else if (ld_mask(wGlobalCounter, 1))
			crazy_move(1);
	}

	void port_Func_2b4f3()
	{
		uint8_t pressed = rd8(wJoypadPressed);
		if (pressed & PAD_B) {
			port_SetState_BatTransforming();
			return;
		}
		if (pressed & PAD_A) {
			port_Func_2a8ae();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_VampireWalking();
	}

	void port_Func_2b509()
	{
		uint8_t pad = rd8(wJoypadDown);
		if (pad & PAD_B) {
			port_SetState_BatTransforming();
			return;
		}
		if (pad & PAD_A) {
			port_Func_2a8ae();
			return;
		}
		bool right;
		if (pad & PAD_RIGHT) {
			right = true;
		} else if (pad & PAD_LEFT) {
			right = false;
		} else {
			port_SetState_VampireIdling();
			return;
		}
		if (facing_right() != right) {
			port_SetState_VampireTurning();
			return;
		}
		if (right)
			FARCALL(WalkIfPossible_Right);
		else
			FARCALL(WalkIfPossible_Left);
		cap_walk(0x10, 0x0c);
	}

	void port_Func_2b2c2()
	{
		port_Func_2b342();
		uint8_t hit = up_collision();
		if (!still_in(WST_PUFFY_RISING))
			return;
		if (hit) {
			port_SetState_PuffyDeflating();
			return;
		}
		up_floor_transition();
		bool right = facing_right();
		uint8_t pad = rd8(wJoypadDown);
		if (pad & (right ? PAD_LEFT : PAD_RIGHT)) {
			port_SetState_PuffyTurning();
			return;
		}
		if (!(pad & (right ? PAD_RIGHT : PAD_LEFT)))
			return;
		if (side_collision(right))
			return;
		move_x(right ? 1 : -1);
	}

	void port_UpdateState_CrazyDizzy()
	{
		if (anim_and_ended(OAM1))
			port_SetState_Crazy();
	}
	void port_UpdateState_VampireTurning()
	{
		if (anim_and_ended(OAM2))
			port_SetState_VampireWalking();
	}
	void port_UpdateState_VampireTransforming()
	{
		if (anim_and_ended(OAM2))
			port_SetState_VampireIdling();
	}
	void port_UpdateState_FlatStretchingUnderwater()
	{
		if (anim_and_ended(OAM1))
			port_Func_2ad6a();
	}

	void port_UpdateState_CrazyTurning()
	{
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_2a675();
			return;
		}
		if (anim_ended())
			port_SetState_Crazy();
	}

	void port_UpdateState_FatSinking()
	{
		anim(OAM1);
		if (!ld_mask(wGlobalCounter, 0x0f))
			port_Func_29317();
		move_y(1);
		if (centre_collision())
			port_Func_2ad6a();
	}

	void port_UpdateState_PuffyTurning()
	{
		anim(rd8(wWarioOAMBank));
		port_Func_2b342();
		uint8_t hit = up_collision();
		if (!still_in(WST_PUFFY_TURNING))
			return;
		if (hit) {
			port_SetState_PuffyDeflating();
			return;
		}
		if (anim_ended())
			port_SetState_PuffyRising();
	}

	void port_Func_2af9c()
	{
		home::apply_jump_velocity(gb_);
		port_HandleWalk();
		cap_walk(0x14, 0x10);
		if (!front_collision())
			return;
		wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_Func_2afc5(); // fallthrough
	}

	void port_Func_2b2a4()
	{
		home::apply_jump_velocity(gb_);
		port_Func_2b17a();
		cap_walk(0x08, 0x04);
		if (rd8(wJumpVelIndex) == FALLING_JUMP_VEL_INDEX)
			wr8(wIsSmashAttacking, TRUE);
	}

	// dec the counter at addr if it's nz: true if it was
	bool count_down(uint16_t addr)
	{
		uint8_t v = rd8(addr);
		if (!v)
			return false;
		wr8(addr, static_cast<uint8_t>(v - 1));
		return true;
	}

	void port_Func_2b3dd()
	{
		home::apply_jump_velocity(gb_);
		if (count_down(wWarioStateCycles))
			return;
		port_Func_2b17a();
		cap_walk(0x04, 0x00);
	}

	// farcall CheckCentreCollision / ld a, b / and a / jp z, fall / update_pos_y / ret
	void ground_check(void (WarioStates3::*fall)())
	{
		if (!centre_collision()) {
			(this->*fall)();
			return;
		}
		update_pos_y();
	}

	// ld a, [wWarioStateCounter] / and a / jr nz / ld a, [wWarioAnimationEnded] /
	// and a / jr z: true if counter is 0 and the animation ended
	bool first_anim_ended() const { return !ld_and(wWarioStateCounter) && anim_ended(); }

	void port_UpdateState_OnFire()
	{
		if (room_transition_check())
			return;
		sfx_loop(14, SFX_01E);
		if (first_anim_ended()) {
			wr8(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_16b14, ADDR_Frameset_16b1d);
		}
		anim(OAM1);
		if (port_DecrementTransformationDuration() == HOT_WARIO_TRANSITION_DURATION) {
			port_SetState_Hot();
			return;
		}
		port_Func_2ae8a();
		if (!still_in(WST_ON_FIRE))
			return;
		ground_check(&WarioStates3::port_Func_2af75);
	}

	void port_UpdateState_FatWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(28, SFX_FAT_WALK);
		anim(OAM1);
		uint16_t left = port_DecrementTransformationDuration();
		// (FAT_WARIO_DURATION >> 8) + 1 and up: it underflowed
		if ((left >> 8) >= ((FAT_WARIO_DURATION >> 8) + 1) || !left) {
			port_SetState_FatRecovering();
			return;
		}
		port_Func_2b239();
		if (!still_in(WST_FAT_WALKING))
			return;
		ground_check(&WarioStates3::port_Func_290d6);
	}

	void port_UpdateState_ZombieWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(36, SFX_ZOMBIE_WALK);
		anim(OAM2);
		port_Func_2b381();
		if (!still_in(WST_ZOMBIE_WALKING))
			return;
		ground_check(&WarioStates3::port_Func_29bb9);
	}

	void port_UpdateState_FlatJumping()
	{
		if (room_transition_check())
			return;
		home::apply_jump_velocity(gb_);
		port_Func_2b17a();
		home::cap_walk_vel(gb_); // Func_1762
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX && !up_collision())
			return;
		port_SetState_FlatFalling();
	}

	void port_UpdateState_FlatFalling()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_SetState_FlatSinking();
			return;
		}
		if (!anim_ended()) {
			anim(OAM1);
			if (anim_ended()) {
				reset_anim();
				set_frameset(ADDR_Frameset_17190);
				anim(OAM1);
				wr8(wWarioAnimationEnded, 0x01);
			}
		}
		// .asm_287ff: Wario slides on along Data_2b68c, falling along
		// Data_2b664, by the step in wWalkVelIndex
		uint8_t step = rd8(wWalkVelIndex);
		wr8(wWalkVelIndex, static_cast<uint8_t>(step + 1));
		uint8_t dx = rd8(static_cast<uint16_t>(ADDR_Data_2b68c + step));
		move_x(facing_right() ? dx : -dx);

		// .asm_28816
		if (rd8(wWalkVelIndex) < 0x28) {
			if (!front_collision()) {
				// .asm_288b8
				move_y(rd8(static_cast<uint16_t>(ADDR_Data_2b664 + step)));
				if (!airborne_collision()) {
					home::handle_downwards_floor_transition(gb_);
					return;
				}
				play_sfx(SFX_020);
				home::trigger_floor_transition(gb_);
				port_SetState_FlatIdling();
				return;
			}
			play_sfx(SFX_BUMP);
			flip_dir();
			move_by_dir(3);
		} else if (!ld_bit(wJoypadDown, facing_right() ? B_PAD_RIGHT : B_PAD_LEFT)) {
			// .asm_2886d
			clear({wFrameDuration, wAnimationFrame, wWarioAnimationEnded});
			set_frameset(flip_dir() ? ADDR_Frameset_17186 : ADDR_Frameset_171b8);
			anim(OAM1);
		}
		// .asm_288a8
		wr8(wWalkVelIndex, 0);
	}

	void port_UpdateState_PuffyRising()
	{
		if (room_transition_check())
			return;
		sfx_loop(32, SFX_PUFF_RAISE);
		anim(rd8(wWarioOAMBank));
		port_Func_2b2c2();
	}

	void port_UpdateState_VampireIdling()
	{
		uint8_t hit = centre_collision();
		if (!still_in(WST_VAMPIRE_IDLING))
			return;
		if (!hit) {
			port_Func_2a8a7();
			return;
		}
		anim(OAM2);
		port_Func_2b4f3();
	}

	void port_UpdateState_VampireWalking()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_VAMPIRE_WALKING))
			return;
		anim(OAM2);
		port_Func_2b509();
		if (!still_in(WST_VAMPIRE_WALKING))
			return;
		uint8_t hit = centre_collision();
		if (!still_in(WST_VAMPIRE_WALKING) || hit)
			return;
		port_Func_2a8a7();
	}

	void port_UpdateState_BatIdling()
	{
		uint8_t hit = centre_collision();
		if (!still_in(WST_BAT_IDLING))
			return;
		if (!hit) {
			port_SetState_BatFalling();
			return;
		}
		sfx_loop(0x20, SFX_FLAP);
		anim(OAM2);
		uint8_t pressed = rd8(wJoypadPressed);
		if (!(pressed & PAD_B)) {
			if (pressed & PAD_A)
				port_SetState_BatFlying();
			return;
		}
		// .asm_2aa55: room to stand up?
		wr8(wCollisionBoxTop, static_cast<uint8_t>(-27));
		hit = up_collision();
		if (!still_in(WST_BAT_IDLING))
			return;
		if (!hit) {
			port_SetState_VampireTransforming();
			return;
		}
		wr8(wCollisionBoxTop, static_cast<uint8_t>(-15));
		play_sfx(SFX_0E5);
	}

	void port_Func_2b027()
	{
		home::apply_jump_velocity(gb_);
		if (count_down(wWarioStateCounter))
			return;
		port_Func_2b17a();
		cap_walk(0x08, 0x04);
		if (!ld_and(wIsTurning))
			return;
		reset_anim();
		frameset_by_dir(ADDR_Frameset_16b26, ADDR_Frameset_16b2f);
		anim(OAM1);
	}

	void port_Func_2b10a()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_FlatJumping();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_FlatWalking();
	}

	void port_Func_2b11b()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_FlatJumping();
			return;
		}
		uint8_t pad = rd8(wJoypadDown);
		bool right;
		if (pad & PAD_LEFT) {
			right = false;
		} else if (pad & PAD_RIGHT) {
			right = true;
		} else {
			port_SetState_FlatIdling();
			return;
		}
		wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		if (front_collision())
			return;
		walk_with_velocity(right);
		cap_walk(0x04, 0x00);
	}

	void port_Func_2b55c()
	{
		home::apply_jump_velocity(gb_);
		port_Func_2b17a();
		cap_walk(0x10, 0x0c);
	}

	void port_Func_2b3f9()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_2a675();
			return;
		}
		port_Func_2b42b();
		if (!still_in(WST_CRAZY) || !anim_ended()) // done if not crazy any more
			return;
		if (!ld_bit(wJoypadDown, facing_right() ? B_PAD_RIGHT : B_PAD_LEFT)) {
			port_SetState_CrazyTurning();
			return;
		}
		wr8(wWalkVelIndex, 0);
	}

	void port_UpdateState_OnFireAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_Func_2ade4();
			return;
		}
		if (first_anim_ended()) {
			wr8(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_16b14, ADDR_Frameset_16b1d);
		}
		anim(OAM1);
		port_Func_2af9c();
		if (!still_in(WST_ON_FIRE_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (up_collision() && still_in(WST_ON_FIRE_AIRBORNE))
				port_Func_2af75();
			return;
		}
		if (!airborne_collision()) {
			if (still_in(WST_ON_FIRE_AIRBORNE))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_ON_FIRE_AIRBORNE))
			FARCALL(SetState_OnFire);
	}

	void port_UpdateState_HotAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_Func_2ade4();
			return;
		}
		anim(OAM1);
		port_Func_2b027();
		if (!still_in(WST_HOT_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			// rising
			if (up_collision() && still_in(WST_HOT_AIRBORNE))
				port_Func_2838a();
			return;
		}
		// .falling
		if (!airborne_collision()) {
			if (still_in(WST_HOT_AIRBORNE))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_HOT_AIRBORNE))
			port_SetState_Hot();
	}

	void port_UpdateState_FlatIdling()
	{
		anim(OAM1);
		port_Func_2b10a();
		if (!centre_collision())
			port_SetState_FlatFalling();
	}

	void port_UpdateState_FlatWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(10, SFX_FLAT_WALK);
		anim(OAM1);
		port_Func_2b11b();
		ground_check(&WarioStates3::port_SetState_FlatFalling);
	}

	void port_UpdateState_Crazy()
	{
		if (room_transition_check())
			return;
		anim(OAM2);
		port_Func_2b3f9();
		if (!still_in(WST_CRAZY))
			return;
		if (!centre_collision())
			port_Func_2a67b();
	}

	// the ground shakes for $20 frames, unless it already does
	void land_shake()
	{
		if (!ld_and(wGroundShakeCounter)) {
			wr8(wGroundShakeCounter, 0x20);
			wr8(wIsWarioGroundShaking, TRUE);
		}
	}

	void port_UpdateState_FatAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_Func_2926a();
			return;
		}
		if (!port_DecrementTransformationDuration())
			wr8(wTransformationDuration + 1, static_cast<uint8_t>(rd8(wTransformationDuration + 1) + 1));
		anim(OAM1);
		if (first_anim_ended()) {
			reset_anim();
			wr8(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_1783e, ADDR_Frameset_17841);
			anim(OAM1);
		}
		port_Func_2b2a4();
		if (!still_in(WST_FAT_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (up_collision() && still_in(WST_FAT_AIRBORNE))
				port_Func_290d6();
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		land_shake();
		if (!ld_and(wFloorTransitionDir))
			play_sfx(SFX_024);
		port_SetState_FatLanding(); // fallthrough
	}

	// ld a, BANK(oam) / hcall UpdateWarioAnimation / anim ended? jp RecoverFromTransformation
	void anim_then_recover(uint8_t bank)
	{
		if (anim_and_ended(bank))
			home::recover_from_transformation(gb_);
	}

	void port_UpdateState_FatRecovering() { anim_then_recover(OAM1); }

	void port_UpdateState_Burnt() { anim_then_recover(OAM1); }

	void port_UpdateState_BallOStringDizzy() { anim_then_recover(OAM1); }

	void port_UpdateState_ElectricDizzy() { anim_then_recover(OAM1); }

	void port_UpdateState_FlatStretching()
	{
		if (!anim_and_ended(OAM1))
			return;
		wr8(wAutoMoveState, 0);
		home::recover_from_transformation(gb_);
	}

	void port_UpdateState_ZombieRecovering()
	{
		if (rd8(wGlobalCounter) & 0x03)
			return;
		// the palette steps in until $08, then back out until $0b
		uint8_t count = static_cast<uint8_t>(rd8(wWarioStateCounter) + 1);
		wr8(wWarioStateCounter, count);
		uint8_t step;
		if (count < 0x08) {
			step = count;
			wr8(wWarioStateCounter + 1, step);
		} else if (count == 0x0b) {
			home::recover_from_transformation(gb_);
			return;
		} else {
			step = static_cast<uint8_t>(rd8(wWarioStateCounter + 1) - 1);
			wr8(wWarioStateCounter + 1, step);
		}
		set_pal(static_cast<uint16_t>(ADDR_WarioZombiePal + static_cast<uint8_t>(step << 4)));
	}

	void port_UpdateState_BallOStringAirborne()
	{
		if (transition_or_water())
			return;
		anim(OAM1);
		if (front_collision() & 0x0f) {
			port_Func_28bd5();
			return;
		}
		home::apply_jump_velocity(gb_);
		port_HandleWalk();
		cap_walk(0x18, 0x14);
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		port_SetState_BallOString();
	}

	void port_UpdateState_BouncyStart()
	{
		if (transition_or_water())
			return;
		home::apply_jump_velocity(gb_);
		port_HandleWalk();
		cap_walk(0x08, 0x04);
		if (!still_in(WST_BOUNCY_START))
			return;
		if (!airborne_collision()) {
			if (still_in(WST_BOUNCY_START))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_BOUNCY_START))
			port_SetState_BouncyFloor(); // fallthrough
	}

	void port_UpdateState_BouncyUpsideDown()
	{
		if (transition_or_water())
			return;
		home::apply_jump_velocity(gb_);
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (!still_in(WST_BOUNCY_UPSIDE_DOWN))
			return;
		wr8(wWarioState, WST_BOUNCY_UPSIDE_DOWN_LANDING);
		clear({wJumpVelIndex, wJumpVelTable, wFrameDuration, wAnimationFrame});
		set_frameset(ADDR_Frameset_1fcc85);
		anim(OAM2);
	}

	void port_UpdateState_BouncyLastBounce()
	{
		if (transition_or_water())
			return;
		if (!anim_ended())
			anim(OAM2);
		home::apply_jump_velocity(gb_);
		port_HandleWalk();
		cap_walk(0x08, 0x04);
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (up_collision() && still_in(WST_BOUNCY_LAST_BOUNCE))
				wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_BOUNCY_LAST_BOUNCE))
			home::recover_from_transformation(gb_);
	}

	void port_UpdateState_BallOStringKnockBack()
	{
		if (transition_or_water())
			return;
		anim(OAM1);
		port_Func_2b1cc();
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX)
			return;
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		flip_dir();
		home::trigger_floor_transition(gb_);
		play_sfx(SFX_03C);
		wr8(wWarioState, WST_GETTING_UNWRAPPED_IN_STRING);
		collision_box(-1, -27, -9, 9);
		clear({wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex, wFrameDuration, wAnimationFrame});
		set_frameset(ADDR_Frameset_17409);
		anim(OAM1);
	}

	void port_UpdateState_ZombieSlippingThroughFloor()
	{
		if (transition_or_water())
			return;
		if (!still_in(WST_ZOMBIE_SLIPPING_THROUGH_FLOOR))
			return;
		port_Func_2b3dd();
		anim(OAM2);
		if (!still_in(WST_ZOMBIE_SLIPPING_THROUGH_FLOOR))
			return;
		if (!airborne_collision()) {
			if (still_in(WST_ZOMBIE_SLIPPING_THROUGH_FLOOR))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (!still_in(WST_ZOMBIE_SLIPPING_THROUGH_FLOOR))
			return;
		port_SetState_ZombieLanding();
		set_frameset(facing_right() ? ADDR_Frameset_1fc90b : ADDR_Frameset_1fc8ee);
		clear({wFrameDuration, wAnimationFrame, wWarioAnimationEnded});
		wr8(wWarioStateCounter, 0x02);
		anim(OAM2);
	}

	// In Bubble: up every other frame (bursting on the ceiling), sideways by
	// the d-pad, else drifting along Data_2b6b4 and turning at its end or a wall
	void port_Func_2b56f()
	{
		if (rd8(wGlobalCounter) & 0x01) {
			move_y(-1);
			if (!up_collision())
				return;
			particle_at_wario(PARTICLE_BUBBLE_BURST);
			home::recover_from_transformation(gb_);
			return;
		}
		uint8_t pad = rd8(wJoypadDown);
		if (pad & (PAD_RIGHT | PAD_LEFT)) {
			bool right = pad & PAD_RIGHT;
			if (!side_collision(right)) {
				move_x(right ? 1 : -1);
				wr8(wJumpVelIndex, 0);
				return;
			}
		} else {
			uint8_t drift = rd8(wWarioStateCounter);
			wr8(wDirection, drift);
			if (!front_collision()) {
				uint8_t dx = jump_vel(ADDR_Data_2b6b4);
				move_x(drift ? dx : -dx);
				uint8_t index = static_cast<uint8_t>(rd8(wJumpVelIndex) + 1);
				wr8(wJumpVelIndex, index);
				if (index < 0x2f)
					return;
			}
		}
		uint8_t drift = rd8(wWarioStateCounter) ^ 0x1;
		wr8(wWarioStateCounter, drift);
		wr8(wDirection, drift);
		wr8(wJumpVelIndex, 0);
	}

	void port_UpdateState_ZombieAirborne()
	{
		if (transition_or_water())
			return;
		if (!still_in(WST_ZOMBIE_AIRBORNE))
			return;
		port_Func_2b3dd();
		if (!still_in(WST_ZOMBIE_AIRBORNE))
			return;
		if (first_anim_ended()) {
			reset_anim();
			wr8(wWarioStateCounter, 1);
			set_frameset(facing_right() ? ADDR_Frameset_1fc8b3 : ADDR_Frameset_1fc8b0);
		}
		anim(OAM2);
		if (!still_in(WST_ZOMBIE_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (up_collision() && still_in(WST_ZOMBIE_AIRBORNE))
				port_Func_29bb9();
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_ZOMBIE_AIRBORNE))
			port_SetState_ZombieLanding(); // fallthrough
	}

	void port_UpdateState_BouncyAirborne()
	{
		if (transition_or_water())
			return;
		if (!anim_ended()) {
			anim(OAM2);
			if (rd8(wJumpVelTable) == JUMP_VEL_BOUNCY_HIGH_JUMP && anim_ended())
				particle_at_wario(PARTICLE_BOUNCY_JUMP);
		}
		if (!port_DecrementTransformationDuration()) {
			port_SetState_BouncyUpsideDown();
			return;
		}
		port_Func_2ae3b();
		home::apply_jump_velocity(gb_);
		port_Func_2b17a();
		cap_walk(0x08, 0x04);
		if (!still_in(WST_BOUNCY_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (!up_collision() || !still_in(WST_BOUNCY_AIRBORNE))
				return;
			wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			wr8(wWarioState, WST_BOUNCY_CEILING);
			reset_anim();
			set_frameset(ADDR_Frameset_1fcc75);
			anim(OAM2);
			return;
		}
		if (!airborne_collision()) {
			if (still_in(WST_BOUNCY_AIRBORNE))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_BOUNCY_AIRBORNE))
			port_SetState_BouncyFloor();
	}

	// hYPos / hXPos backed up, and back
	void backup_pos()
	{
		wr8(hYPosHiBackup, rd8(hYPosHi));
		wr8(hYPosLoBackup, rd8(hYPosLo));
		wr8(hXPosHiBackup, rd8(hXPosHi));
		wr8(hXPosLoBackup, rd8(hXPosLo));
	}
	void restore_pos_backup()
	{
		wr8(hYPosHi, rd8(hYPosHiBackup));
		wr8(hYPosLo, rd8(hYPosLoBackup));
		wr8(hXPosHi, rd8(hXPosHiBackup));
		wr8(hXPosLo, rd8(hXPosLoBackup));
	}

	void port_UpdateState_ElectricStart()
	{
		if (transition_or_water())
			return;
		if (!anim_and_ended(OAM1))
			return;
		wr8(wInvincibleCounter, 0x01);
		wr8(wWarioState, WST_ELECTRIC);
		reset_anim();
		wr8(wJumpVelIndex, 0x06);
		wr8(wCollisionBoxBottom, static_cast<uint8_t>(-1));
		wr8(wCollisionBoxLeft, static_cast<uint8_t>(-9));
		wr8(wCollisionBoxRight, 9);
		bool low_ceiling = false;
		if (rd8(wIsCrouching)) {
			// room to stand up?
			wr8(wCollisionBoxTop, static_cast<uint8_t>(-27));
			backup_pos();
			uint8_t hit = up_collision();
			if (!still_in(WST_ELECTRIC))
				return;
			if (hit) {
				low_ceiling = true;
			} else {
				wr8(wIsCrouching, 0);
				restore_pos_backup();
			}
		}
		if (low_ceiling) {
			wr8(wCollisionBoxTop, static_cast<uint8_t>(-15));
			wr8(wJumpVelTable, 0);
			restore_pos_backup();
		} else {
			wr8(wCollisionBoxTop, static_cast<uint8_t>(-27));
			wr8(wJumpVelTable, 0x01);
		}
		set_frameset(facing_right() ? ADDR_Frameset_17b7c : ADDR_Frameset_17b8d);
		anim(OAM1);
	}

	void port_UpdateState_Electric()
	{
		if (transition_or_water())
			return;
		anim(OAM1);
		if (!front_collision())
			move_by_dir(rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX ? 2 : 1);
		else
			move_by_dir(-1);

		if (!rd8(wJumpVelTable)) {
			// crouched: shaking in place
			uint8_t index = static_cast<uint8_t>(rd8(wJumpVelIndex) + 1);
			wr8(wJumpVelIndex, index);
			if (index >= MAX_JUMP_VEL_INDEX)
				goto dizzy;
			if (centre_collision() & 0x0f)
				return;
			wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			wr8(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
			return;
		}

		// the jumps: rising until a ceiling, then falling
		while (jump_vel_move()) {
			if (!up_collision())
				return;
			wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		}

		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		{
			// three bounces, then dizzy
			uint8_t bounce = static_cast<uint8_t>(rd8(wJumpVelTable) + 1);
			wr8(wJumpVelTable, bounce);
			if (bounce == 2) {
				wr8(wJumpVelIndex, 0x0a);
				return;
			}
			if (bounce == 3) {
				wr8(wJumpVelIndex, 0x0e);
				return;
			}
		}

	dizzy:
		play_sfx(SFX_041);
		{
			// smoke 8 in front, $20 up
			uint16_t x = static_cast<uint16_t>(rd8(wWarioXPos + 0) << 8 | rd8(wWarioXPos + 1));
			x = static_cast<uint16_t>(facing_right() ? x + 0x08 : x - 0x08);
			wr8(hXPosLo, static_cast<uint8_t>(x));
			wr8(hXPosHi, static_cast<uint8_t>(x >> 8));
			uint16_t y = static_cast<uint16_t>((rd8(wWarioYPos + 0) << 8 | rd8(wWarioYPos + 1)) - 0x20);
			wr8(hYPosLo, static_cast<uint8_t>(y));
			wr8(hYPosHi, static_cast<uint8_t>(y >> 8));
		}
		particle(PARTICLE_SMOKE_SMALL);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		flip_dir();
		wr8(wWarioState, WST_ELECTRIC_DIZZY);
		reset_anim();
		set_frameset(facing_right() ? ADDR_Frameset_17bad : ADDR_Frameset_17b9e);
		anim(OAM1);
	}

	void port_UpdateState_PuffyDeflating()
	{
		if (!ld_and(wWarioStateCounter)) {
			if (!anim_and_ended(rd8(wWarioOAMBank)))
				return;
			wr8(wWarioStateCounter, 0x01);
			reset_anim();
			set_frameset(facing_right() ? ADDR_Frameset_1dcd31 : ADDR_Frameset_1dcd01);
		}
		if (anim_and_ended(rd8(wWarioOAMBank)))
			home::recover_from_transformation(gb_);
	}

	void port_UpdateState_CrazyAirborne()
	{
		if (transition_or_water())
			return;
		anim(OAM2);
		home::apply_jump_velocity(gb_);
		port_HandleWalk();
		cap_walk(0x10, 0x0c);
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			if (up_collision())
				wr8(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		port_SetState_Crazy();
	}

	void port_UpdateState_BatFalling()
	{
		if (transition_or_water())
			return;
		if (!still_in(WST_BAT_FALLING))
			return;
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_BatFlying();
			return;
		}
		home::apply_jump_velocity(gb_);
		port_Func_2b17a();
		cap_walk(0x08, 0x04);
		if (ld_and(wIsTurning))
			port_Func_2abc1();
		uint8_t hit = airborne_collision();
		if (!still_in(WST_BAT_FALLING) || !hit)
			return;
		update_pos_y();
		port_SetState_BatIdling();
	}

	void port_UpdateState_VampireAirborne()
	{
		if (transition_or_water())
			return;
		if (!still_in(WST_VAMPIRE_AIRBORNE))
			return;
		anim(OAM2);
		port_Func_2b55c();
		if (!still_in(WST_VAMPIRE_AIRBORNE))
			return;
		if (rd8(wJumpVelIndex) < FALLING_JUMP_VEL_INDEX) {
			uint8_t hit = up_collision();
			if (still_in(WST_VAMPIRE_AIRBORNE) && hit)
				port_Func_2a8a7();
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_VAMPIRE_AIRBORNE))
			port_SetState_VampireIdling();
	}

	void port_UpdateState_BatFlying()
	{
		if (transition_or_water())
			return;
		if (!still_in(WST_BAT_FLYING))
			return;
		sfx_loop(0x10, SFX_FLAP);
		anim(OAM2);
		uint8_t hit = up_collision();
		if (!still_in(WST_BAT_FLYING))
			return;
		if (hit) {
			port_SetState_BatFalling();
			return;
		}
		port_Func_2b63b();
		if (!still_in(WST_BAT_FLYING))
			return;
		port_Func_2b17a();
		cap_walk(0x08, 0x04);
		if (ld_and(wIsTurning))
			port_Func_2abc1(); // fallthrough
	}

	void port_UpdateState_CrazySpinning()
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
		anim(OAM1);
		if (inc_counter() < 0x80)
			return;
		wr8(wWarioStateCounter, 0x00);
		play_sfx(SFX_043);
		wr8(wWarioState, WST_CRAZY_DIZZY);
		clear({wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex, wFrameDuration, wAnimationFrame});
		load_gfx(BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
		set_oam(BANK_OAM_17bbc, ADDR_OAM_17bbc);
		set_frameset(ADDR_Frameset_17ce2);
		anim(OAM1);
	}

	void port_UpdateState_InBubble()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wWaterInteraction)) {
			// .asm_2ad45
			particle_at_wario(PARTICLE_BUBBLE_BURST);
			home::recover_from_transformation(gb_);
			return;
		}
		sfx_loop(0x0e, SFX_023);
		anim(OAM2);
		port_Func_2b56f();
	}

	// ld a, [wWarioState] / sub $60 / rst JumpTable (table after the rst)
	// wWarioState - $60, as the asm's jumptable
	void port_UpdateWarioStates_Group3()
	{
		switch (static_cast<uint8_t>(rd8(wWarioState) - 0x60)) {
		case 0x00: port_UpdateState_OnFire(); return;
		case 0x01: port_UpdateState_OnFireAirborne(); return;
		case 0x02: port_UpdateState_Hot(); return;
		case 0x03: port_UpdateState_HotAirborne(); return;
		case 0x04: port_UpdateState_Burnt(); return;
		case 0x05: port_UpdateState_GettingFlatAirborne(); return;
		case 0x06: port_UpdateState_GettingFlat(); return;
		case 0x07: port_UpdateState_FlatIdling(); return;
		case 0x08: port_UpdateState_FlatWalking(); return;
		case 0x09: port_UpdateState_FlatJumping(); return;
		case 0x0a: port_UpdateState_FlatFalling(); return;
		case 0x0b: port_UpdateState_FlatStretching(); return;
		case 0x0c: port_UpdateState_FlatSinking(); return;
		case 0x0d: port_UpdateState_FlatStretchingUnderwater(); return;
		case 0x0e: port_UpdateState_FlatSquished(); return;
		case 0x0f: port_UpdateState_FlatSquishedLifting(); return;
		case 0x10: port_UpdateState_GettingWrappedInString(); return;
		case 0x11: port_UpdateState_BallOString(); return;
		case 0x12: port_UpdateState_BallOStringAirborne(); return;
		case 0x13: port_UpdateState_BallOStringKnockBack(); return;
		case 0x14: port_UpdateState_GettingUnwrappedInString(); return;
		case 0x15: port_UpdateState_BallOStringDizzy(); return;
		case 0x16: port_UpdateState_FatBumping(); return;
		case 0x17: port_UpdateState_FatEating(); return;
		case 0x18: port_UpdateState_FatIdling(); return;
		case 0x19: port_UpdateState_FatWalking(); return;
		case 0x1a: port_UpdateState_FatTurning(); return;
		case 0x1b: port_UpdateState_FatAirborne(); return;
		case 0x1c: port_UpdateState_FatLanding(); return;
		case 0x1d: port_UpdateState_FatSinking(); return;
		case 0x1e: port_UpdateState_FatRecovering(); return;
		case 0x1f: port_UpdateState_ElectricStart(); return;
		case 0x20: port_UpdateState_Electric(); return;
		case 0x21: port_UpdateState_ElectricDizzy(); return;
		case 0x22: port_UpdateState_TurningInvisible(); return;
		case 0x23: port_UpdateState_PuffyInflating(); return;
		case 0x24: port_UpdateState_PuffyRising(); return;
		case 0x25: port_UpdateState_PuffyTurning(); return;
		case 0x26: port_UpdateState_PuffyDeflating(); return;
		case 0x27: port_UpdateState_ZombieIdling(); return;
		case 0x28: port_UpdateState_ZombieWalking(); return;
		case 0x29: port_UpdateState_ZombieTurning(); return;
		case 0x2a: port_UpdateState_ZombieAirborne(); return;
		case 0x2b: port_UpdateState_ZombieLanding(); return;
		case 0x2c: port_UpdateState_ZombieSlippingThroughFloor(); return;
		case 0x2d: port_UpdateState_ZombieRecovering(); return;
		case 0x2e: port_UpdateState_ZombieKnockBack(); return;
		case 0x2f: port_UpdateState_ZombieWrithing(); return;
		case 0x30: port_UpdateState_BouncyStart(); return;
		case 0x31: port_UpdateState_BouncyFloor(); return;
		case 0x32: port_UpdateState_BouncyAirborne(); return;
		case 0x33: port_UpdateState_BouncyCeiling(); return;
		case 0x34: port_UpdateState_BouncyUpsideDown(); return;
		case 0x35: port_UpdateState_BouncyUpsideLanding(); return;
		case 0x36: port_UpdateState_BouncyLastBounce(); return;
		case 0x37: port_UpdateState_CrazySpinning(); return;
		case 0x38: port_UpdateState_CrazyDizzy(); return;
		case 0x39: port_UpdateState_Crazy(); return;
		case 0x3a: port_UpdateState_CrazyTurning(); return;
		case 0x3b: port_UpdateState_CrazyAirborne(); return;
		case 0x3c: port_UpdateState_VampireIdling(); return;
		case 0x3d: port_UpdateState_VampireWalking(); return;
		case 0x3e: port_UpdateState_VampireTurning(); return;
		case 0x3f: port_UpdateState_VampireAirborne(); return;
		case 0x40: port_UpdateState_BatTransforming(); return;
		case 0x41: port_UpdateState_BatIdling(); return;
		case 0x42: port_UpdateState_VampireTransforming(); return;
		case 0x43: port_UpdateState_BatFlying(); return;
		case 0x44: port_UpdateState_BatFalling(); return;
		case 0x45: port_UpdateState_InBubble(); return;
		case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
			CALL(InvalidWarioStateReset);
			return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}
};

} // namespace

} // namespace wl3::level

// Every caller of these reads no register after them (the asm liveness
// check): they clobber them all, but for SetState_ZombieRecovering.
using wl3::level::WarioStates3;

PORT_COVERED(DecrementTransformationDuration);
PORT_COVERED(Func_28c15);
PORT_COVERED(SetState_Hot);
PORT_COVERED(SetState_FatIdling);
PORT_COVERED(SetState_FatWalking);
PORT_COVERED(SetState_FatTurning);
PORT_EX(SetState_FatBumping, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_FatBumping(); }
PORT_COVERED(SetState_FatAirborne);
PORT_COVERED(SetState_FatLanding);
PORT_COVERED(SetState_FatRecovering);
PORT_COVERED(SetState_BallOString);
PORT_COVERED(SetState_BallOStringKnockBack);
PORT_EX(SetState_ZombieIdling, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_ZombieIdling(); }
PORT_COVERED(SetState_ZombieWalking);
PORT_COVERED(SetState_ZombieTurning);
PORT_COVERED(SetState_ZombieAirborne);
PORT_EX(SetState_ZombieKnockBack, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_ZombieKnockBack(); }
// BlockFunc_LightSource farcalls it from inside a collision check, and the
// check's register-level callers may read what the block function leaves:
// it leaves b to l alone, and a and f as its xor a
PORT_EX(SetState_ZombieRecovering, 0, ALL_REGISTERS)
{
	WarioStates3(gb).port_SetState_ZombieRecovering();
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}
PORT_COVERED(SetState_BouncyAirborne);
PORT_COVERED(SetState_BouncyUpsideDown);
PORT_COVERED(SetState_BouncyFloor);
PORT_COVERED(SetState_BouncyLastBounce);
PORT_COVERED(UpdateState_BouncyUpsideLanding);
PORT_COVERED(SetState_OnFireAirborne);
PORT_EX(Func_2afc5, 0, ALL_REGISTERS) { WarioStates3(gb).port_Func_2afc5(); }
PORT_COVERED(Func_2ae5c);
PORT_COVERED(Func_2ae3b);
PORT_COVERED(Func_29bb9);
PORT_COVERED(Func_29be1);
PORT_COVERED(Func_290d6);
PORT_COVERED(Func_290a1);
PORT_EX(Func_28bd5, 0, ALL_REGISTERS) { WarioStates3(gb).port_Func_28bd5(); }
PORT_COVERED(Func_2a0c7);
PORT_COVERED(Func_2af75);
PORT_COVERED(Func_2af81);
PORT_COVERED(Func_2a0bb);
PORT_COVERED(Func_2a0c2);
PORT_COVERED(UpdateState_FlatSquished);
PORT_COVERED(SetState_FlatSquished);
PORT_COVERED(SetState_Hot_ResetDuration);
PORT_COVERED(Func_2b4d0);
PORT_COVERED(Func_2b342);
PORT_COVERED(Func_28f6d);
PORT_COVERED(SetState_BatFalling);
PORT_COVERED(UpdateState_ZombieWrithing);
PORT_COVERED(UpdateState_ZombieTurning);
PORT_COVERED(Func_2838a);
PORT_COVERED(SetState_VampireAirborne);
PORT_COVERED(UpdateState_GettingWrappedInString);
PORT_COVERED(Func_2ade4);
PORT_COVERED(UpdateState_FatTurning);
PORT_COVERED(UpdateState_BouncyCeiling);
PORT_EX(SetState_FlatSquishedLifting, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_FlatSquishedLifting(); }
PORT_COVERED(UpdateState_GettingUnwrappedInString);
PORT_EX(Func_2ac7d, 0, ALL_REGISTERS) { WarioStates3(gb).port_Func_2ac7d(); }
PORT_EX(SetState_VampireTransforming, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_VampireTransforming(); }
PORT_COVERED(SetState_VampireIdling);
PORT_EX(SetState_FlatIdling, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_FlatIdling(); }
PORT_COVERED(SetState_FlatSinking);
PORT_COVERED(Func_289c5);
PORT_COVERED(Func_2abc1);
PORT_COVERED(SetState_ZombieWrithing);
PORT_COVERED(UpdateState_ZombieKnockBack);
PORT_COVERED(UpdateState_ZombieLanding);
PORT_COVERED(Func_2a2e7);
PORT_COVERED(UpdateState_BouncyFloor);
PORT_COVERED(Func_2926a);
PORT_COVERED(UpdateState_FatLanding);
PORT_COVERED(SetState_FlatWalking);
PORT_EX(SetState_FlatFalling, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_FlatFalling(); }
PORT_COVERED(Func_29317);
PORT_EX(SetState_TurningInvisible, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_TurningInvisible(); }
PORT_COVERED(SetState_CrazyTurning);
PORT_COVERED(SetState_VampireTurning);
PORT_COVERED(SetState_CrazyAirborne);
PORT_COVERED(SetState_PuffyTurning);
PORT_COVERED(SetState_BatIdling);
PORT_COVERED(SetState_PuffyDeflating);
PORT_COVERED(SetState_PuffyRising);
PORT_COVERED(SetState_BatTransforming);
PORT_COVERED(SetState_VampireWalking);
PORT_COVERED(Func_2a675);
PORT_COVERED(Func_2a67b);
PORT_COVERED(Func_2a8a7);
PORT_EX(Func_2a0b2, 0, ALL_REGISTERS) { WarioStates3(gb).port_Func_2a0b2(); }
PORT_COVERED(Func_2a8ae);
PORT_COVERED(Func_28380);
PORT_COVERED(UpdateState_BatTransforming);
PORT_COVERED(UpdateState_PuffyInflating);
PORT_COVERED(UpdateState_GettingFlat);
PORT_COVERED(UpdateState_FatIdling);
PORT_COVERED(UpdateState_FlatSquishedLifting);
PORT_COVERED(Func_2b63b);
PORT_COVERED(Func_2b34e);
PORT_COVERED(UpdateState_FatEating);
PORT_COVERED(UpdateState_FlatSinking);
PORT_EX(Func_2ad6a, 0, ALL_REGISTERS) { WarioStates3(gb).port_Func_2ad6a(); }
PORT_COVERED(SetState_Crazy);
PORT_EX(SetState_CrazySpinning, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_CrazySpinning(); }
PORT_EX(SetState_PuffyInflating, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_PuffyInflating(); }
PORT_COVERED(UpdateState_GettingFlatAirborne);
PORT_EX(SetState_FlatAirborne, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_FlatAirborne(); }
PORT_COVERED(UpdateState_FatBumping);
PORT_COVERED(UpdateState_ZombieIdling);
PORT_COVERED(Func_2b1cc);
PORT_COVERED(UpdateState_BallOString);
PORT_COVERED(HandleHotWarioInput);
PORT_COVERED(SetState_ZombieLanding);
PORT_EX(SetState_BouncyStart, 0, ALL_REGISTERS) { WarioStates3(gb).port_SetState_BouncyStart(); }
PORT_COVERED(UpdateState_TurningInvisible);
PORT_EX(Func_2b17a, 0, ALL_REGISTERS) { WarioStates3(gb).port_Func_2b17a(); }
PORT_EX(HandleWalk, 0, ALL_REGISTERS) { WarioStates3(gb).port_HandleWalk(); }
PORT_COVERED(Func_2b239);
PORT_COVERED(Func_2b381);
PORT_COVERED(Func_2ae8a);
PORT_COVERED(UpdateState_Hot);
PORT_COVERED(SetState_BatFlying);
PORT_COVERED(SetState_FlatJumping);
PORT_COVERED(Func_2b42b);
PORT_COVERED(Func_2b4f3);
PORT_COVERED(Func_2b509);
PORT_COVERED(Func_2b2c2);
PORT_COVERED(UpdateState_CrazyDizzy);
PORT_COVERED(UpdateState_VampireTurning);
PORT_COVERED(UpdateState_VampireTransforming);
PORT_COVERED(UpdateState_FlatStretchingUnderwater);
PORT_COVERED(UpdateState_CrazyTurning);
PORT_COVERED(UpdateState_FatSinking);
PORT_COVERED(UpdateState_PuffyTurning);
PORT_COVERED(Func_2af9c);
PORT_COVERED(Func_2b2a4);
PORT_COVERED(Func_2b3dd);
PORT_COVERED(UpdateState_OnFire);
PORT_COVERED(UpdateState_FatWalking);
PORT_COVERED(UpdateState_ZombieWalking);
PORT_COVERED(UpdateState_FlatJumping);
PORT_COVERED(UpdateState_FlatFalling);
PORT_COVERED(UpdateState_PuffyRising);
PORT_COVERED(UpdateState_VampireIdling);
PORT_COVERED(UpdateState_VampireWalking);
PORT_COVERED(UpdateState_BatIdling);
PORT_COVERED(Func_2b027);
PORT_COVERED(Func_2b10a);
PORT_COVERED(Func_2b11b);
PORT_COVERED(Func_2b55c);
PORT_COVERED(Func_2b3f9);
PORT_COVERED(UpdateState_OnFireAirborne);
PORT_COVERED(UpdateState_HotAirborne);
PORT_COVERED(UpdateState_FlatIdling);
PORT_COVERED(UpdateState_FlatWalking);
PORT_COVERED(UpdateState_Crazy);
PORT_COVERED(UpdateState_FatAirborne);
PORT_COVERED(UpdateState_FatRecovering);
PORT_COVERED(UpdateState_Burnt);
PORT_COVERED(UpdateState_BallOStringDizzy);
PORT_COVERED(UpdateState_ElectricDizzy);
PORT_COVERED(UpdateState_FlatStretching);
PORT_COVERED(UpdateState_ZombieRecovering);
PORT_COVERED(UpdateState_BallOStringAirborne);
PORT_COVERED(UpdateState_BouncyStart);
PORT_COVERED(UpdateState_BouncyUpsideDown);
PORT_COVERED(UpdateState_BouncyLastBounce);
PORT_COVERED(UpdateState_BallOStringKnockBack);
PORT_COVERED(UpdateState_ZombieSlippingThroughFloor);
PORT_COVERED(Func_2b56f);
PORT_COVERED(UpdateState_ZombieAirborne);
PORT_COVERED(UpdateState_BouncyAirborne);
PORT_COVERED(UpdateState_ElectricStart);
PORT_COVERED(UpdateState_Electric);
PORT_COVERED(UpdateState_PuffyDeflating);
PORT_COVERED(UpdateState_CrazyAirborne);
PORT_COVERED(UpdateState_BatFalling);
PORT_COVERED(UpdateState_VampireAirborne);
PORT_COVERED(UpdateState_BatFlying);
PORT_COVERED(UpdateState_CrazySpinning);
PORT_COVERED(UpdateState_InBubble);
PORT_EX(UpdateWarioStates_Group3, 0, ALL_REGISTERS) { WarioStates3(gb).port_UpdateWarioStates_Group3(); }
