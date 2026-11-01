// Wario's states, part 3 (src/engine/level/wario_states_3.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "asm_module.hpp"

namespace wl3::level {

namespace {

class WarioStates3 : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_DecrementTransformationDuration()
	{
		// 16-bit big-endian counter
		uint8_t lo = alu_.sub(rd8(wTransformationDuration + 1), 1);
		wr8(wTransformationDuration + 1, lo);
		cpu_.a = alu_.sbc(rd8(wTransformationDuration), 0);
		wr8(wTransformationDuration, cpu_.a);
		SET_HL(gb_, wTransformationDuration);
	}

	void port_Func_28c15()
	{
		cpu_.a = FALLING_JUMP_VEL_INDEX;
		wr8(wJumpVelIndex, cpu_.a);
		cpu_.a = JUMP_VEL_HIGH_JUMP;
		wr8(wJumpVelTable, cpu_.a);
		cpu_.a = WST_BALL_O_STRING_AIRBORNE;
		wr8(wWarioState, cpu_.a);
	}

	void port_SetState_Hot()
	{
		st(wTransformationProgress, 2);
		st(wWarioState, WST_HOT);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, 0};
		clear(zero);
		st(wStingTouchState, TOUCH_VANISH);
		st(wTouchState, TOUCH_VANISH);
		st(wca94, 0x02);
		reset_anim();

		// face the way the d-pad points
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			st(wDirection, alu_.bit(cpu_.a, B_PAD_RIGHT) ? DIRECTION_RIGHT : DIRECTION_LEFT);
		frameset_by_dir(ADDR_Frameset_16b26, ADDR_Frameset_16b2f);
		anim(OAM1);
	}

	void port_SetState_FatIdling()
	{
		st(wWarioState, WST_FAT_IDLING);
		st(wTouchState, TOUCH_ATTACK);
		st(wStingTouchState, TOUCH_ATTACK);
		st(wca94, 0x01);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1781c, ADDR_Frameset_1781f);
		anim(OAM1);
	}

	void port_SetState_FatWalking()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_FAT_WALKING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_17822, ADDR_Frameset_1782b);
		anim(OAM1);
	}

	void port_SetState_FatTurning()
	{
		st(wWarioState, WST_FAT_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_178f3, ADDR_Frameset_178ea);
		anim(OAM1);
	}

	void port_SetState_FatBumping()
	{
		st(wWarioState, WST_FAT_BUMPING);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_178fc, ADDR_Frameset_17905);
		anim(OAM1);
	}

	void port_SetState_FatAirborne()
	{
		static const uint16_t zero[] = {wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		anim(OAM1);
		st(wWarioState, WST_FAT_AIRBORNE);
	}

	void port_SetState_FatLanding()
	{
		st(wWarioState, WST_FAT_LANDING);
		static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
			wWarioStateCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_17872, ADDR_Frameset_17887);
		anim(OAM1);
	}

	void port_SetState_FatRecovering()
	{
		st(wWarioState, WST_FAT_RECOVERING);
		touch_states(TOUCH_BUMP, 0x02);
		static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
			wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_17844, ADDR_Frameset_1785b);
		anim(OAM1);
	}

	void port_SetState_BallOString()
	{
		st(wWarioState, WST_BALL_O_STRING);
		st(wTransformationProgress, 1);
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCycles,
			wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		collision_box(-1, -24, -9, 9);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1741f, ADDR_Frameset_173fe);
		anim(OAM1);
	}

	void port_SetState_BallOStringKnockBack()
	{
		xor_a();
		wr8(wTransformationProgress, 0);
		play_sfx(SFX_BUMP);
		st(wWarioState, WST_BALL_O_STRING_KNOCK_BACK);
		st(wJumpVelIndex, 0x0a);
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		collision_box(-1, -27, -9, 9);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		reset_anim();
		set_frameset(ADDR_Frameset_1742a);
		anim(OAM1);
	}

	void port_SetState_ZombieIdling()
	{
		st(wWarioState, WST_ZOMBIE_IDLING);
		collision_box(-1, -27, -9, 9);
		touch_states(TOUCH_VANISH, 0x02);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wInvisibleFrame,
			wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioZombiePal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioZombieGfx, ADDR_WarioZombieGfx);
		set_oam(BANK_OAM_1fc48b, ADDR_OAM_1fc48b);
		frameset_by_dir(ADDR_Frameset_1fc8b6, ADDR_Frameset_1fc8b9);
		anim(OAM2);
	}

	void port_SetState_ZombieWalking()
	{
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
		st(wWarioState, WST_ZOMBIE_WALKING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc870, ADDR_Frameset_1fc879);
		anim(OAM2);
	}

	void port_SetState_ZombieTurning()
	{
		st(wWarioState, WST_ZOMBIE_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc89d, ADDR_Frameset_1fc894);
		anim(OAM2);
	}

	void port_SetState_ZombieAirborne()
	{
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		xor_a();
		wr8(wWarioStateCycles, 0);
		st(wWarioState, WST_ZOMBIE_AIRBORNE);
		reset_anim();
		anim(OAM2);
	}

	void port_SetState_ZombieKnockBack()
	{
		static const uint8_t skip[] = {WST_ZOMBIE_KNOCK_BACK, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR,
			WST_ZOMBIE_LANDING};
		ld(wWarioState);
		for (int i = 0; i < 3; i++) {
			alu_.cp(cpu_.a, skip[i]);
			if (ZF)
				return;
		}
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		st(wWarioState, WST_ZOMBIE_KNOCK_BACK);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc92e, ADDR_Frameset_1fc93b);
		anim(OAM2);
	}

	void port_SetState_ZombieRecovering()
	{
		st(wWarioState, WST_ZOMBIE_RECOVERING);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
	}

	void port_SetState_BouncyAirborne()
	{
		st(wWarioState, WST_BOUNCY_AIRBORNE);
		play_sfx(SFX_02B);
		reset_anim();
		set_frameset(ADDR_Frameset_1fcc6c);
		anim(OAM2);
	}

	void port_SetState_BouncyUpsideDown()
	{
		st(wWarioState, WST_BOUNCY_UPSIDE_DOWN);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		xor_a();
		wr8(wWarioStateCycles, 0);
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		reset_anim();
		SET_HL(gb_, ADDR_WarioBouncyPal);
		CALL(SetWarioPal);
		set_frameset(ADDR_Frameset_1fcc82);
		anim(OAM2);
	}

	void port_SetState_BouncyFloor()
	{
		static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wWarioStateCounter,
			wWalkVelIndex, 0};
		clear(zero);
		st(wWarioState, WST_BOUNCY_FLOOR);
		reset_anim();
		set_frameset(ADDR_Frameset_1fcc5f);
		anim(OAM2);
	}

	void port_SetState_BouncyLastBounce()
	{
		st(wWarioState, WST_BOUNCY_LAST_BOUNCE);
		xor_a();
		wr8(wJumpVelIndex, 0);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		anim(OAM2);
	}

	void port_UpdateState_BouncyUpsideLanding()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(Func_2a2e7); // fallthrough
	}

	void port_SetState_OnFireAirborne()
	{
		xor_a();
		wr8(wWarioStateCycles, 0);
		st(wWarioState, WST_ON_FIRE_AIRBORNE);
	}

	// Hot Wario bounces off a wall: turns around and moves back 3 pixels.
	void port_Func_2afc5()
	{
		cpu_.a = alu_.xor_(rd8(wDirection), 0x1);
		wr8(wDirection, cpu_.a);
		cpu_.b = 0x03;
		bool right = ld_and(wDirection);
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		bool counter = ld_and(wWarioStateCounter);
		if (right)
			set_frameset(counter ? ADDR_Frameset_16b1d : ADDR_Frameset_16b49);
		else
			set_frameset(counter ? ADDR_Frameset_16b14 : ADDR_Frameset_16b38);
		anim(OAM1);
	}

	// Returns carry on the frames the transformation flashes as it runs out:
	// every 4th frame in the last $78, every 16th from $168 left.
	void port_Func_2ae5c()
	{
		SET_HL(gb_, wTransformationDuration + 1);
		ld(wTransformationDuration);
		bool carry;
		alu_.cp(cpu_.a, 0x02);
		if (!CF) {
			carry = false;
		} else if (alu_.and_(cpu_.a, cpu_.a)) {
			ld_cp(wTransformationDuration + 1, 0x68);
			if (!CF) {
				carry = false;
			} else {
				cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b1111);
				carry = !cpu_.a;
			}
		} else {
			ld_cp(wTransformationDuration + 1, 0x78);
			if (!CF) {
				cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b1111);
				carry = !cpu_.a;
			} else {
				cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b11);
				if (cpu_.a) {
					carry = false;
				} else {
					cpu_.a = 0x01;
					carry = true;
				}
			}
		}
		// scf (/ ccf): n and h cleared, z kept
		cpu_.f = (uint8_t)((cpu_.f & FLAG_Z) | (carry ? FLAG_C : 0));
	}

	// Bouncy Wario flashes red as the transformation runs out.
	void port_Func_2ae3b()
	{
		LCALL(Func_2ae5c);
		if (!CF)
			return;
		SET_DE(gb_, 0);
		cpu_.a = alu_.xor_(rd8(wWarioStateCycles), 0x1);
		wr8(wWarioStateCycles, cpu_.a);
		uint16_t pal = ZF ? ADDR_WarioBouncyPal : ADDR_WarioBouncyRedPal;
		SET_HL(gb_, alu_.add16(pal, 0));
		CALL(SetWarioPal);
	}

	void port_Func_29bb9()
	{
		frameset_by_dir(ADDR_Frameset_1fc8b0, ADDR_Frameset_1fc8b3);
		st(wWarioStateCounter, 0x01);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_ZombieAirborne);
	}

	void port_Func_29be1()
	{
		frameset_by_dir(ADDR_Frameset_1fc8a6, ADDR_Frameset_1fc8ab);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wJumpVelIndex, 0);
		LJUMP(SetState_ZombieAirborne); // fallthrough
	}

	void port_Func_290d6()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wIsSmashAttacking, TRUE);
		wr8(wWarioStateCounter, TRUE);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		frameset_by_dir(ADDR_Frameset_1783e, ADDR_Frameset_17841);
		LJUMP(SetState_FatAirborne);
	}

	void port_Func_290a1()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wWarioStateCounter, 0);
		st(wJumpVelTable, 0x01);
		frameset_by_dir(ADDR_Frameset_17834, ADDR_Frameset_17839);
		LJUMP(SetState_FatAirborne);
	}

	void port_Func_28bd5()
	{
		cpu_.a = alu_.xor_(rd8(wDirection), 0x1);
		wr8(wDirection, cpu_.a);
		inc_counter();
		alu_.cp(cpu_.a, 0x05);
		if (!CF) {
			LJUMP(SetState_BallOStringKnockBack);
			return;
		}
		frameset_by_dir(ADDR_Frameset_1741f, ADDR_Frameset_173fe);
		anim(OAM1);
		RET();
	}

	void port_Func_2a0c7()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		LJUMP(SetState_BouncyAirborne); // fallthrough
	}

	void port_Func_2af75()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		LJUMP(SetState_OnFireAirborne);
	}

	void port_Func_2af81()
	{
		play_sfx(SFX_JUMP);
		xor_a();
		wr8(wJumpVelIndex, 0);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		LJUMP(SetState_OnFireAirborne); // fallthrough
	}

	void port_Func_2a0bb()
	{
		st(wJumpVelTable, JUMP_VEL_BOUNCY_HIGH_JUMP);
		LJUMP(Func_2a0c7);
	}

	void port_Func_2a0c2()
	{
		st(wJumpVelTable, JUMP_VEL_BOUNCY_JUMP);
		LJUMP(Func_2a0c7); // fallthrough
	}

	void port_UpdateState_FlatSquished()
	{
	}

	void port_SetState_FlatSquished()
	{
		st(wWarioState, WST_FLAT_SQUISHED);
		xor_a();
		wr8(wWarioStateCounter, 0);
	}

	void port_SetState_Hot_ResetDuration()
	{
		st(wTransformationDuration + 0, (uint8_t)(HOT_WARIO_TRANSITION_DURATION >> 8));
		st(wTransformationDuration + 1, (uint8_t)HOT_WARIO_TRANSITION_DURATION);
		LJUMP(SetState_Hot); // fallthrough
	}

	void port_Func_2b4d0()
	{
		get_pos();
		cpu_.b = PARTICLE_DUST;
		FARCALL(CreateParticle);
	}

	void port_Func_2b342()
	{
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 1);
		if (!ZF)
			return;
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
	}

	void port_Func_28f6d()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_290a1);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_FatWalking);
			return;
		}
		RET();
	}

	void port_SetState_BatFalling()
	{
		st(wWarioState, WST_BAT_FALLING);
		st(wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	}

	void port_UpdateState_ZombieWrithing()
	{
		anim(OAM2);
		if (ld_and(wJoypadDown)) {
			LJUMP(SetState_ZombieIdling);
			return;
		}
		RET();
	}

	void port_UpdateState_ZombieTurning()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_ZombieWalking);
	}

	void port_Func_2838a()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		st(wWarioState, WST_HOT_AIRBORNE);
		st(wWarioStateCounter, 0x08);
	}

	void port_SetState_VampireAirborne()
	{
		st(wWarioState, WST_VAMPIRE_AIRBORNE);
		st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
	}

	void port_UpdateState_GettingWrappedInString()
	{
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_28c15);
			return;
		}
		LJUMP(SetState_BallOString); // fallthrough
	}

	void port_Func_2ade4()
	{
		CALL(ClearTransformationValues);
		st(wInvincibleCounter, 0x10);
		CALL(UpdateLevelMusic);
		get_pos();
		cpu_.a = alu_.and_(rd8(hYPosLo), 0xf0);
		wr8(hYPosLo, cpu_.a);
		cpu_.b = PARTICLE_SMOKE_LARGE;
		FARCALL(CreateParticle);
		play_sfx(SFX_SPLASH);
		FARCALL(DiveFromSurface_SkipSplash);
	}

	void port_UpdateState_FatTurning()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_290a1);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_FatWalking);
	}

	void port_UpdateState_BouncyCeiling()
	{
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = alu_.or_(rd8(wTransformationDuration), rd8(wTransformationDuration + 1));
		if (ZF) {
			LJUMP(SetState_BouncyUpsideDown);
			return;
		}
		LCALL(Func_2ae3b);
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_BouncyAirborne);
	}

	void port_SetState_FlatSquishedLifting()
	{
		st(wIsIntangible, TRUE);
		st(wWarioState, WST_FLAT_SQUISHED_LIFTING);
		xor_a();
		wr8(wWarioStateCounter, 0);
		reset_anim();
		set_frameset(ADDR_Frameset_1716c);
		anim(OAM1);
	}

	// collision box: bottom -1, top -8, left -11, right 11 (flat Wario)
	void flat_box()
	{
		collision_box(-1, -8, -11, 11);
	}

	void port_UpdateState_GettingUnwrappedInString()
	{
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		play_sfx(SFX_03D);
		st(wWarioState, WST_BALL_O_STRING_DIZZY);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		reset_anim();
		load_gfx(BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
		set_oam(BANK_OAM_17bbc, ADDR_OAM_17bbc);
		set_frameset(ADDR_Frameset_17ce2);
		anim(OAM1);
		RET();
	}

	void port_Func_2ac7d()
	{
		st(wWarioState, WST_IN_BUBBLE);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioBubblePal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioBubbleGfx, ADDR_WarioBubbleGfx);
		set_oam(BANK_OAM_1fd87c, ADDR_OAM_1fd87c);
		ld(wObjDirection);
		wr8(wDirection, cpu_.a);
		wr8(wWarioStateCounter, cpu_.a);
		set_frameset(ADDR_Frameset_1fdb2c);
		anim(OAM2);
	}

	void port_SetState_VampireTransforming()
	{
		play_sfx(SFX_045);
		st(wWarioState, WST_VAMPIRE_TRANSFORMING);
		touch_states(TOUCH_VANISH, 0x02);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioVampirePal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioBatGfx, ADDR_WarioBatGfx);
		set_oam(BANK_OAM_1fd031, ADDR_OAM_1fd031);
		set_frameset(ADDR_Frameset_1fd439);
		anim(OAM2);
	}

	void port_SetState_VampireIdling()
	{
		st(wWarioState, WST_VAMPIRE_IDLING);
		reset_anim();
		load_gfx(BANK_WarioVampireGfx, ADDR_WarioVampireGfx);
		wr8(wWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_1fd498 >> 8));
		st(wWarioOAMPtr + 1, (uint8_t)ADDR_OAM_1fd498);
		set_frameset(ADDR_Frameset_1fd81c);
		anim(OAM2);
	}

	void port_SetState_FlatIdling()
	{
		st(wWarioState, WST_FLAT_IDLING);
		static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		flat_box();
		set_frameset(ADDR_Frameset_1718b);
		anim(OAM1);
	}

	void port_SetState_FlatSinking()
	{
		st(wWarioState, WST_FLAT_SINKING);
		static const uint16_t zero[] = {wWalkVelIndex, wWarioStateCounter, wWarioStateCycles,
			wJumpVelIndex, wJumpVelTable, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		flat_box();
		reset_anim();
		set_frameset(ADDR_Frameset_1719a);
		anim(OAM1);
	}

	// hPos = wWarioPos + (-ysub, dx) through hl/de walking down:
	// ld hl, wWarioXPos + 1 / ld de, hXPosLo / ld a, [hld] / add (or sub) / ...
	void pos_offset(bool add, uint8_t dx, uint8_t ysub)
	{
		if (add) {
			cpu_.a = alu_.add(rd8(wWarioXPos + 1), dx);
			wr8(hXPosLo, cpu_.a);
			cpu_.a = alu_.adc(rd8(wWarioXPos + 0), 0x00);
		} else {
			cpu_.a = alu_.sub(rd8(wWarioXPos + 1), dx);
			wr8(hXPosLo, cpu_.a);
			cpu_.a = alu_.sbc(rd8(wWarioXPos + 0), 0x00);
		}
		wr8(hXPosLo - 1, cpu_.a);
		cpu_.a = alu_.sub(rd8(wWarioXPos - 1), ysub);
		wr8(hXPosLo - 2, cpu_.a);
		cpu_.a = alu_.sbc(rd8(wWarioXPos - 2), 0x00);
		wr8(hXPosLo - 3, cpu_.a);
		SET_HL(gb_, wWarioXPos - 2);
		SET_DE(gb_, hXPosLo - 3);
	}

	void port_Func_289c5()
	{
		pos_offset(false, 0x04, 0x08);
		cpu_.b = PARTICLE_BUBBLES;
		FARCALL(CreateParticle);
		st(wWarioState, WST_FLAT_STRETCHING_UNDERWATER);
		static const uint16_t zero[] = {wWalkVelIndex, wWarioStateCounter, wWarioStateCycles,
			wJumpVelIndex, wJumpVelTable, 0};
		clear(zero);
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
		st(wWarioState, WST_ZOMBIE_WRITHING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc882, ADDR_Frameset_1fc88b);
		anim(OAM2);
	}

	void port_UpdateState_ZombieKnockBack()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_ZombieIdling);
	}

	void port_UpdateState_ZombieLanding()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		cpu_.a = alu_.dec(rd8(wWarioStateCounter));
		if (!ZF) {
			LJUMP(SetState_ZombieIdling);
			return;
		}
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		st(wWarioStateCycles, 0x06);
		st(wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fc928, ADDR_Frameset_1fc92b);
		anim(OAM2);
		RET();
	}

	void port_Func_2a2e7()
	{
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		SET_HL(gb_, ADDR_WarioBouncyPal);
		CALL(SetWarioPal);
		frameset_by_dir(ADDR_Frameset_1fccbf, ADDR_Frameset_1fccb2);
		reset_anim();
		LJUMP(SetState_BouncyLastBounce);
	}

	void port_UpdateState_BouncyFloor()
	{
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = alu_.or_(rd8(wTransformationDuration), rd8(wTransformationDuration + 1));
		if (ZF) {
			LJUMP(Func_2a2e7);
			return;
		}
		LCALL(Func_2ae3b);
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		if (ld_bit(wJoypadDown, B_PAD_A))
			LJUMP(Func_2a0bb);
		else
			LJUMP(Func_2a0c2);
	}

	void port_Func_2926a()
	{
		get_pos();
		cpu_.a = alu_.and_(rd8(hYPosLo), 0xf0);
		wr8(hYPosLo, cpu_.a);
		cpu_.b = PARTICLE_SPLASH;
		FARCALL(CreateParticle);
		play_sfx(SFX_SPLASH);
		st(wWarioState, WST_FAT_SINKING);
		static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
			wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		frameset_by_dir(ADDR_Frameset_17872, ADDR_Frameset_17887);
		anim(OAM1);
	}

	void port_UpdateState_FatLanding()
	{
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = rd8(wTransformationDuration);
		alu_.cp(cpu_.a, 0x02);
		if (!CF) {
			LJUMP(SetState_FatRecovering);
			return;
		}
		cpu_.a = alu_.or_(cpu_.a, rd8(wTransformationDuration + 1));
		if (ZF) {
			LJUMP(SetState_FatRecovering);
			return;
		}
		anim(OAM1);
		if (ld_and(wGroundShakeCounter)) {
			RET();
			return;
		}
		LJUMP(SetState_FatIdling);
	}

	void port_SetState_FlatWalking()
	{
		st(wWarioState, WST_FLAT_WALKING);
		static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wFrameDuration,
			wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		set_frameset(ADDR_Frameset_17175);
		anim(OAM1);
		if (ld_bit(wJoypadDown, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else
			st(wDirection, DIRECTION_LEFT);
	}

	void port_SetState_FlatFalling()
	{
		st(wWarioState, WST_FLAT_FALLING);
		static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles, wWarioSlopeInteraction, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelIndex, cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		frameset_by_dir(ADDR_Frameset_171b8, ADDR_Frameset_17186);
		anim(OAM1);
	}

	void port_Func_29317()
	{
		if (facing_right())
			pos_offset(true, 0x04, 0x10);
		else
			pos_offset(false, 0x04, 0x10);
		cpu_.b = PARTICLE_BUBBLES;
		FARCALL(CreateParticle);
	}

	void port_SetState_TurningInvisible()
	{
		play_sfx(SFX_03F);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		st(wWarioState, WST_TURNING_INVISIBLE);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero2[] = {wTouchState, wStingTouchState, wca94, 0};
		clear(zero2);
		reset_anim();
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
		set_oam(BANK_OAM_17cf7, ADDR_OAM_17cf7);
		frameset_by_dir(ADDR_Frameset_17e1b, ADDR_Frameset_17e3a);
		anim(OAM1);
	}

	void port_SetState_CrazyTurning()
	{
		st(wWarioState, WST_CRAZY_TURNING);
		reset_anim();
		set_frameset(flip_dir_and() ? ADDR_Frameset_1fcfd7 : ADDR_Frameset_1fcfd0);
		anim(OAM2);
	}

	void port_SetState_VampireTurning()
	{
		st(wWarioState, WST_VAMPIRE_TURNING);
		reset_anim();
		set_frameset(flip_dir_and() ? ADDR_Frameset_1fd83a : ADDR_Frameset_1fd841);
		anim(OAM2);
	}

	void port_SetState_CrazyAirborne()
	{
		st(wWarioState, WST_CRAZY_AIRBORNE);
		st(wJumpVelTable, JUMP_VEL_NORMAL);
		static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		frameset_by_dir(ADDR_Frameset_1fcf9f, ADDR_Frameset_1fcf6e);
		anim(OAM2);
	}

	void port_SetState_PuffyTurning()
	{
		flip_dir();
		st(wWarioState, WST_PUFFY_TURNING);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1dcce3, ADDR_Frameset_1dccd8);
		anim(ld(wWarioOAMBank));
	}

	void port_SetState_BatIdling()
	{
		st(wWarioState, WST_BAT_IDLING);
		static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelIndex, wJumpVelTable, 0};
		clear(zero);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1fd45f, ADDR_Frameset_1fd46c);
		anim(OAM2);
	}

	void port_SetState_PuffyDeflating()
	{
		play_sfx(SFX_027);
		st(wWarioState, WST_PUFFY_DEFLATING);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wFrameDuration,
			wAnimationFrame, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		frameset_by_dir(ADDR_Frameset_1dccee, ADDR_Frameset_1dcd1e);
		anim(ld(wWarioOAMBank));
	}

	void port_SetState_PuffyRising()
	{
		st(wWarioState, WST_PUFFY_RISING);
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		frameset_by_dir(ADDR_Frameset_1dcd4e, ADDR_Frameset_1dcd57);
		anim(ld(wWarioOAMBank));
	}

	void port_SetState_BatTransforming()
	{
		play_sfx(SFX_046);
		st(wWarioState, WST_BAT_TRANSFORMING);
		st(wCollisionBoxTop, (uint8_t)-15);
		reset_anim();
		load_gfx(BANK_WarioBatGfx, ADDR_WarioBatGfx);
		wr8(wWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_1fd031 >> 8));
		st(wWarioOAMPtr + 1, (uint8_t)ADDR_OAM_1fd031);
		frameset_by_dir(ADDR_Frameset_1fd40a, ADDR_Frameset_1fd479);
		anim(OAM2);
	}

	void port_SetState_VampireWalking()
	{
		st(wWarioState, WST_VAMPIRE_WALKING);
		static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		bool right;
		if (ld_bit(wJoypadDown, B_PAD_LEFT))
			right = false;
		else if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			right = true;
		else
			right = facing_right();
		if (right) {
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_1fd82e);
		} else {
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_1fd825);
		}
		anim(OAM2);
	}

	void port_Func_2a675()
	{
		xor_a();
		wr8(wJumpVelIndex, 0);
		LJUMP(SetState_CrazyAirborne);
	}

	void port_Func_2a67b()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_CrazyAirborne); // fallthrough
	}

	void port_Func_2a8a7()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(SetState_VampireAirborne);
	}

	void port_Func_2a0b2()
	{
		if (ld_bit(wJoypadDown, B_PAD_A))
			LJUMP(Func_2a0bb);
		else
			LJUMP(Func_2a0c2);
	}

	void port_Func_2a8ae()
	{
		play_sfx(SFX_01D);
		xor_a();
		wr8(wJumpVelIndex, 0);
		LJUMP(SetState_VampireAirborne); // fallthrough
	}

	void port_Func_28380()
	{
		center_on_ladder();
		LJUMP(Func_2838a); // fallthrough
	}

	void port_UpdateState_BatTransforming()
	{
		anim(OAM2);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_BatIdling); // fallthrough
	}

	void port_UpdateState_PuffyInflating()
	{
		anim(ld(wWarioOAMBank));
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_PuffyRising); // fallthrough
	}

	void port_UpdateState_GettingFlat()
	{
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		xor_a();
		wr8(wIsIntangible, 0);
		if (ld_and(wWarioStateCounter))
			LJUMP(SetState_FlatSquished);
		else
			LJUMP(SetState_FlatIdling); // fallthrough
	}

	void port_UpdateState_FatIdling()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		LCALL(Func_28f6d);
		ld_cp(wWarioState, WST_FAT_IDLING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_290d6);
			return;
		}
		RET();
	}

	void port_UpdateState_FlatSquishedLifting()
	{
		anim(OAM1);
		cpu_.b = 0x01;
		CALL(MoveWarioUp);
		inc_counter();
		alu_.cp(cpu_.a, 0x32);
		if (CF) {
			RET();
			return;
		}
		xor_a();
		wr8(wIsIntangible, 0);
		LJUMP(SetState_FlatFalling);
	}

	void port_Func_2b63b()
	{
		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			LJUMP(SetState_BatFalling);
			return;
		}
		SET_HL(gb_, wJumpVelIndex);
		cpu_.a = alu_.inc(rd8(wJumpVelIndex));
		wr8(wJumpVelIndex, cpu_.a);
		alu_.cp(cpu_.a, MAX_JUMP_VEL_INDEX + 1);
		if (!CF) {
			LJUMP(SetState_BatFalling);
			return;
		}
		cpu_.e = cpu_.a;
		cpu_.d = 0x00;
		FARCALL(Func_1f863);
		RET();
	}

	void port_Func_2b34e()
	{
		if (ld_bit(wJoypadDown, B_PAD_A)) {
			LJUMP(Func_29be1);
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, PAD_RIGHT | PAD_LEFT);
		if (!ZF) {
			LJUMP(SetState_ZombieWalking);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_29bb9);
			return;
		}
		cpu_.a = alu_.add(rd8(wWarioStateCounter), 0x01);
		wr8(wWarioStateCounter, cpu_.a);
		SET_HL(gb_, wWarioStateCounter + 1);
		cpu_.a = alu_.adc(rd8(wWarioStateCounter + 1), 0x00);
		wr8(wWarioStateCounter + 1, cpu_.a);
		alu_.cp(cpu_.a, 0x02);
		if (CF) {
			RET();
			return;
		}
		LJUMP(SetState_ZombieWrithing);
	}

	void port_UpdateState_FatEating()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			LJUMP(Func_2926a);
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
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);
		LJUMP(SetState_FatIdling); // fallthrough
	}

	void port_UpdateState_FlatSinking()
	{
		anim(OAM1);
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b1111);
		if (ZF) {
			// call z, .Func_2899a
			gb_push16(gb_, ADDR_UpdateState_FlatSinking + 0x2d);
			pos_offset(false, 0x04, 0x08);
			cpu_.b = PARTICLE_BUBBLES;
			FARCALL(CreateParticle);
			gb_pop16(gb_);
		}
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0b1);
		if (!ZF) {
			RET();
			return;
		}
		cpu_.b = 0x01;
		CALL(MoveWarioDown);
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			RET();
			return;
		}
		LJUMP(Func_289c5);
	}

	void port_Func_2ad6a()
	{
		CALL(ClearTransformationValues);
		st(wInvincibleCounter, 0x10);
		CALL(UpdateLevelMusic);
		FARCALL(SetState_Submerged);
		reset_anim();
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wWarioStateCycles, cpu_.a);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		frameset_by_dir(ADDR_Frameset_155a6, ADDR_Frameset_155ab);
		anim(OAM1);
		JUMP(PlayRecoverySFX);
	}

	void port_SetState_Crazy()
	{
		play_sfx(SFX_043);
		st(wWarioState, WST_CRAZY);
		st(wTouchState, TOUCH_BUMP);
		st(wStingTouchState, TOUCH_VULNERABLE);
		st(wca94, 0x01);
		static const uint16_t zero[] = {wWalkVelIndex, wWarioStateCounter, wWarioStateCycles,
			wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		reset_anim();
		load_gfx(BANK_WarioCrazyGfx, ADDR_WarioCrazyGfx);
		set_oam(BANK_OAM_1fccd5, ADDR_OAM_1fccd5);
		frameset_by_dir(ADDR_Frameset_1fcf9f, ADDR_Frameset_1fcf6e);
		anim(OAM2);
	}

	void port_SetState_CrazySpinning()
	{
		play_sfx(SFX_042);
		st(wWarioState, WST_CRAZY_SPINNING);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0};
		clear(zero);
		if (ld_and(wJumpVelTable))
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		CALL(UpdateLevelMusic);
		reset_anim();
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		cpu_.a = rd8(wObjDirection);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_14ce1 : ADDR_Frameset_14ccc);
		anim(OAM1);
	}

	void port_SetState_PuffyInflating()
	{
		st(wTransformation, TRANSFORMATION_PUFFY_WARIO);
		st(wStingTouchState, TOUCH_VULNERABLE);
		st(wTouchState, TOUCH_BUMP);
		st(wca94, 0x01);
		play_sfx(SFX_025);
		st(wWarioState, WST_PUFFY_INFLATING);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wSwimVelIndex, wWaterSurfaceFloatingCounter, wWaterCurrent,
			wLastWaterCurrent, wCurWaterCurrent, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		reset_anim();
		load_gfx(BANK_WarioPuffyGfx, ADDR_WarioPuffyGfx);
		set_oam(BANK_OAM_1dc93f, ADDR_OAM_1dc93f);
		frameset_by_dir(ADDR_Frameset_1dccc0, ADDR_Frameset_1dccc9);
		anim(rd8(wWarioOAMBank));
	}

	void port_UpdateState_GettingFlatAirborne()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_and(wWaterInteraction)) {
			LJUMP(SetState_FlatSinking);
			return;
		}
		cpu_.b = 0x02;
		CALL(MoveWarioDown);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		st(wWarioState, WST_GETTING_FLAT);
		static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
			wWarioStateCycles, wJumpVelIndex, wJumpVelTable, wAutoMoveState, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		collision_box(-1, -8, -11, 11);
		set_frameset(ADDR_Frameset_1715f);
		anim(OAM1);
		RET();
	}

	void port_SetState_FlatAirborne()
	{
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
			return;
		ld_cp(wTransformation, TRANSFORMATION_OWL_WARIO);
		if (ZF)
			CALL(ReleaseOwl);
		// push_wram BANK("Level Objects WRAM")
		cpu_.a = rd8(rWBK);
		push_af();
		st(rWBK, BANK_wObjects);
		uint16_t hl = (uint16_t)((rd8(wObjPtr + 0) << 8) | rd8(wObjPtr + 1));
		SET_DE(gb_, OBJ_X_POS);
		hl = alu_.add16(hl, OBJ_X_POS);
		cpu_.b = 0;
		cpu_.a = alu_.and_(rd8(wObjInteractionType), INTERACTION_MASK);
		cpu_.a = alu_.sub(cpu_.a, OBJ_INTERACTION_RICHTERTOFFEN);
		if (ZF) {
			wr8(wWarioStateCounter, cpu_.a);
		} else {
			st(wWarioStateCounter, 0x01);
			cpu_.b = 0x8;
		}
		cpu_.a = alu_.sub(rd8(hl), cpu_.b);
		wr8(wWarioXPos + 1, cpu_.a);
		hl++;
		cpu_.a = alu_.sbc(rd8(hl), 0x00);
		wr8(wWarioXPos + 0, cpu_.a);
		SET_HL(gb_, hl);
		pop_af();
		wr8(rWBK, cpu_.a);
		st(wTransformation, TRANSFORMATION_FLAT_WARIO);
		play_sfx(SFX_01B);
		CALL(UpdateLevelMusic);
		st(wTouchState, TOUCH_BUMP);
		st(wStingTouchState, TOUCH_BUMP);
		st(wca94, 0x02);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCycles,
			wGrabState, wAttackCounter, wJumpVelIndex, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wAutoMoveState, cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		st(wWarioState, WST_GETTING_FLAT_AIRBORNE);
		collision_box(-1, -8, -11, 11);
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_16e9d, ADDR_OAM_16e9d);
		set_frameset(ADDR_Frameset_171bd);
		anim(OAM1);
	}

	void port_UpdateState_FatBumping()
	{
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_FatIdling);
	}

	void port_UpdateState_ZombieIdling()
	{
		FARCALL(Func_19b25);
		ld_cp(wWarioState, WST_ZOMBIE_IDLING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_ZOMBIE_IDLING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(Func_29bb9);
			return;
		}
		update_pos_y();
		anim(OAM2);
		LCALL(Func_2b34e);
		RET();
	}

	// farcall f / ld a, b / and a; true if nz
	bool farcall_b(uint8_t bank, uint16_t addr)
	{
		port_farcall(gb_, bank, addr);
		return b_and();
	}

	void port_Func_2b1cc()
	{
		if (!facing_right()) {
			cpu_.b = 2;
			CALL(MoveWarioLeft);
			if (farcall_b(BANK_Func_197b1, ADDR_Func_197b1)) {
				cpu_.b = 2;
				CALL(MoveWarioRight);
			}
		} else {
			cpu_.b = 2;
			CALL(MoveWarioRight);
			if (farcall_b(BANK_Func_19741, ADDR_Func_19741)) {
				cpu_.b = 2;
				CALL(MoveWarioLeft);
			}
		}
		cpu_.a = rd8(wJumpVelIndex);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(ADDR_JumpVelTable_Normal, REG_DE(gb_));
		SET_HL(gb_, hl);
		uint8_t v = rd8(hl);
		if (alu_.bit(v, 7)) {
			cpu_.a = alu_.inc(alu_.cpl(rd8(hl)));
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

	void port_UpdateState_BallOString()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		sfx_loop(12, SFX_ROLL);
		anim(OAM1);
		if (!facing_right()) {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		} else {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		}
		ld_cp(wWalkVelIndex, 0x18);
		if (!CF)
			st(wWalkVelIndex, 0x14);
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			LJUMP(Func_28bd5);
			return;
		}
		if (farcall_b(BANK_CheckCentreCollision, ADDR_CheckCentreCollision) == false) {
			LJUMP(Func_28c15);
			return;
		}
		update_pos_y();
		if (!ld_and(wWarioSlopeInteraction)) {
			RET();
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		cpu_.a = alu_.dec(cpu_.a);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.xor_(rd8(wDirection), cpu_.b);
		if (cpu_.a) {
			LJUMP(Func_28bd5);
			return;
		}
		RET();
	}

	// turns to dir with the new frameset if facing the other way, then
	// farcall CheckFrontCollision / ret nz / ld b, 1 / move
	void port_HandleHotWarioInput()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_RIGHT | PAD_LEFT);
		if (!cpu_.a)
			return;
		cpu_.a = alu_.and_(cpu_.a, PAD_RIGHT);
		bool right = cpu_.a != 0;
		bool turn = facing_right() != right;
		if (turn) {
			st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
			reset_anim();
			set_frameset(right ? ADDR_Frameset_16b2f : ADDR_Frameset_16b26);
			anim(OAM1);
		}
		if (farcall_b(BANK_CheckFrontCollision, ADDR_CheckFrontCollision))
			return;
		cpu_.b = 1;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	}

	void port_SetState_ZombieLanding()
	{
		play_sfx(SFX_02A);
		touch_states(TOUCH_BUMP, 0x02);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		st(wWarioState, WST_ZOMBIE_LANDING);
		static const uint16_t zero[] = {wWarioStateCounter, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		FARCALL(Func_19a77);
		if (ld_and(wc1ca)) {
			st(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_1fc8bc, ADDR_Frameset_1fc8d5);
		} else {
			frameset_by_dir(ADDR_Frameset_1fc948, ADDR_Frameset_1fc953);
		}
		anim(OAM2);
	}

	void port_SetState_BouncyStart()
	{
		st(wWarioState, WST_BOUNCY_START);
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(wJumpVelTable, 0x02);
		collision_box(-1, -29, -9, 9);
		static const uint16_t zero[] = {wInvisibleFrame, wWalkVelIndex, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioBouncyPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioBouncyGfx, ADDR_WarioBouncyGfx);
		set_oam(BANK_OAM_1fc95e, ADDR_OAM_1fc95e);
		set_frameset(ADDR_Frameset_1fcc6c);
		anim(OAM2);
		if (!farcall_b(BANK_Func_197b1, ADDR_Func_197b1))
			return;
		cpu_.b = 2;
		CALL(MoveWarioRight);
	}

	void port_UpdateState_TurningInvisible()
	{
		anim(OAM1);
		if (!anim_ended())
			return;
		xor_a();
		wr8(wIsIntangible, 0);
		st(wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
		CALL(UpdateLevelMusic);
		FARCALL(SetState_Idling);
	}

	void port_Func_2b17a()
	{
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			FARCALL(WalkRight);
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			FARCALL(WalkLeft);
	}

	void port_HandleWalk()
	{
		if (facing_right())
			FARCALL(WalkRight);
		else
			FARCALL(WalkLeft);
	}

	// ld a, [wWalkVelIndex] / cp max / jr c / ld a, v / ld [wWalkVelIndex], a
	void cap_walk_vel(uint8_t max, uint8_t v)
	{
		ld_cp(wWalkVelIndex, max);
		if (!CF) {
			cpu_.a = v;
			wr8(wWalkVelIndex, v);
		}
	}

	void port_Func_2b239()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_290a1);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			if (!ld_and(wDirection)) {
				st(wDirection, DIRECTION_RIGHT);
				LJUMP(SetState_FatTurning);
				return;
			}
			FARCALL(WalkIfPossible_Right);
			cap_walk_vel(0x08, 0x04);
			RET();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			ld_cp(wDirection, 0x00);
			if (!ZF) {
				st(wDirection, DIRECTION_LEFT);
				LJUMP(SetState_FatTurning);
				return;
			}
			FARCALL(WalkIfPossible_Left);
			cap_walk_vel(0x08, 0x04);
			RET();
			return;
		}
		LJUMP(SetState_FatIdling);
	}

	void port_Func_2b381()
	{
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_29be1);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			if (!ld_and(wDirection)) {
				st(wDirection, DIRECTION_RIGHT);
				LJUMP(SetState_ZombieTurning);
				return;
			}
			FARCALL(WalkIfPossible_Right);
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			if (ld_and(wDirection)) {
				st(wDirection, DIRECTION_LEFT);
				LJUMP(SetState_ZombieTurning);
				return;
			}
			FARCALL(WalkIfPossible_Left);
		} else {
			LJUMP(SetState_ZombieIdling);
			return;
		}
		cap_walk_vel(0x04, 0x00);
		RET();
	}

	void port_Func_2ae8a()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				LJUMP(Func_2af81);
				return;
			}
		}
		bool right = facing_right();
		FARCALL(CheckFrontCollision);
		if (!b_and()) {
			if (right) {
				CALL(ApplyWalkVelocity_Right);
				CALL(MoveWarioRight);
			} else {
				CALL(ApplyWalkVelocity_Left);
				CALL(MoveWarioLeft);
			}
			cap_walk_vel(0x14, 0x10);
			RET();
			return;
		}
		// bumped into a wall: turn around
		cpu_.b = 0x02;
		if (right) {
			CALL(MoveWarioLeft);
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ld_and(wWarioStateCounter) ? ADDR_Frameset_16b14 : ADDR_Frameset_16b38);
		} else {
			CALL(MoveWarioRight);
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ld_and(wWarioStateCounter) ? ADDR_Frameset_16b1d : ADDR_Frameset_16b49);
		}
		anim(OAM1);
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			RET();
			return;
		}
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			LJUMP(SetState_Hot_ResetDuration);
			return;
		}
		RET();
	}

	void port_UpdateState_Hot()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		sfx_loop(16, SFX_01F);
		anim(OAM1);
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = rd8(wTransformationDuration);
		alu_.cp(cpu_.a, 0);
		if (ZF) {
			uint16_t pal = 0;
			cpu_.a = rd8(wTransformationDuration + 1);
			alu_.cp(cpu_.a, 224);
			if (ZF) {
				pal = ADDR_WarioHot1Pal;
			} else if (alu_.cp(cpu_.a, 164), ZF) {
				pal = ADDR_WarioHot2Pal;
			} else if (alu_.cp(cpu_.a, 60), ZF) {
				pal = ADDR_WarioHot3Pal;
			} else if (alu_.cp(cpu_.a, 1), ZF) {
				// .burnt
				play_sfx(SFX_022);
				st(wWarioState, WST_BURNT);
				xor_a();
				wr8(wFrameDuration, 0);
				wr8(wAnimationFrame, 0);
				wr8(wWarioStateCounter, 0);
				wr8(wWarioStateCycles, 0);
				SET_HL(gb_, ADDR_WarioOnFirePal);
				CALL(SetWarioPal);
				load_gfx(BANK_WarioBurntGfx, ADDR_WarioBurntGfx);
				set_oam(BANK_OAM_16b6c, ADDR_OAM_16b6c);
				frameset_by_dir(ADDR_Frameset_16e47, ADDR_Frameset_16e72);
				anim(OAM1);
				RET();
				return;
			}
			if (pal) {
				SET_HL(gb_, pal);
				CALL(SetWarioPal);
			}
		}
		LCALL(HandleHotWarioInput);
		ld_cp(wWarioState, WST_HOT);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_28380);
			return;
		}
		update_pos_y();
		RET();
	}

	void port_SetState_BatFlying()
	{
		st(wWarioState, WST_BAT_FLYING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		LJUMP(Func_2abc1);
	}

	void port_SetState_FlatJumping()
	{
		play_sfx(SFX_01D);
		st(wWarioState, WST_FLAT_JUMPING);
		static const uint16_t clr[] = {
			wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wJumpVelIndex, 0,
		};
		clear(clr);
		st(wJumpVelTable, ld_bit(wJoypadDown, B_PAD_UP) ? JUMP_VEL_NORMAL : JUMP_VEL_KNOCK_BACK);
		set_frameset(ADDR_Frameset_17190);
		anim(OAM1);
		if (!ld_and(wIsOnSteppableObject)) {
			RET();
			return;
		}
		LJUMP(UpdateState_FlatJumping); // fallthrough
	}

	// moves Wario b pixels forward unless a wall is in front, in which case
	// he turns (the .asm_2b45d/.asm_2b495 paths of Func_2b42b)
	void crazy_move(uint8_t b)
	{
		bool right = facing_right();
		if (farcall_b(right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1)) {
			LJUMP(SetState_CrazyTurning);
			return;
		}
		cpu_.b = b;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		RET();
	}

	void port_Func_2b42b()
	{
		SET_HL(gb_, wWalkVelIndex);
		cpu_.a = alu_.inc(rd8(wWalkVelIndex));
		wr8(wWalkVelIndex, cpu_.a);
		uint8_t a = cpu_.a;
		alu_.cp(a, 0x06);
		if (CF) { crazy_move(2); return; }
		if (ZF) { LJUMP(Func_2b4d0); return; }
		alu_.cp(a, 0x09);
		if (CF) { RET(); return; }
		alu_.cp(a, 0x12);
		if (CF) { crazy_move(2); return; }
		if (ZF) { LJUMP(Func_2b4d0); return; }
		alu_.cp(a, 0x15);
		if (CF) { RET(); return; }
		alu_.cp(a, 0x18);
		if (CF) { crazy_move(2); return; }
		alu_.cp(a, 0x22);
		if (CF) { crazy_move(1); return; }
		if (ZF) { LJUMP(Func_2b4d0); return; }
		alu_.cp(a, 0x25);
		if (CF) { RET(); return; }
		alu_.cp(a, 0x2c);
		if (!CF) { RET(); return; }
		if (!ld_mask(wGlobalCounter, 1)) {
			RET();
			return;
		}
		crazy_move(1);
	}

	void port_Func_2b4f3()
	{
		cpu_.a = rd8(wJoypadPressed);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(SetState_BatTransforming);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_2a8ae);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_VampireWalking);
			return;
		}
		RET();
	}

	void port_Func_2b509()
	{
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			LJUMP(SetState_BatTransforming);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_A)) {
			LJUMP(Func_2a8ae);
			return;
		}
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT))
			right = true;
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			right = false;
		else {
			LJUMP(SetState_VampireIdling);
			return;
		}
		if (facing_right() != right) {
			LJUMP(SetState_VampireTurning);
			return;
		}
		if (right)
			FARCALL(WalkIfPossible_Right);
		else
			FARCALL(WalkIfPossible_Left);
		cap_walk_vel(0x10, 0x0c);
		RET();
	}

	void port_Func_2b2c2()
	{
		LCALL(Func_2b342);
		FARCALL(CheckUpCollision);
		ld_cp(wWarioState, WST_PUFFY_RISING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and()) {
			LJUMP(SetState_PuffyDeflating);
			return;
		}
		cpu_.a = alu_.and_(rd8(wCameraConfigFlags), CAM_SCROLLING_MASK);
		alu_.cp(cpu_.a, CAM_TRANSITIONS);
		if (!CF) {
			CALL(GetFloorForYPos);
			cpu_.a = alu_.sub(rd8(wFloor), cpu_.c);
			if (CF)
				CALL(StartUpwardsFloorTransition);
		}
		bool right = facing_right();
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
			LJUMP(SetState_PuffyTurning);
			return;
		}
		if (!alu_.bit(cpu_.a, right ? B_PAD_RIGHT : B_PAD_LEFT)) {
			RET();
			return;
		}
		if (farcall_b(right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1)) {
			RET();
			return;
		}
		cpu_.b = 0x01;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		RET();
	}

	// UpdateWarioAnimation in OAM bank `bank`, then jp `to` once the animation
	// has ended
	void anim_then(uint8_t bank, uint16_t to)
	{
		anim(bank);
		if (!anim_ended()) {
			RET();
			return;
		}
		JUMP_TO(to);
	}

	void port_UpdateState_CrazyDizzy() { anim_then(OAM1, ADDR_SetState_Crazy); }
	void port_UpdateState_VampireTurning() { anim_then(OAM2, ADDR_SetState_VampireWalking); }
	void port_UpdateState_VampireTransforming() { anim_then(OAM2, ADDR_SetState_VampireIdling); }
	void port_UpdateState_FlatStretchingUnderwater() { anim_then(OAM1, ADDR_Func_2ad6a); }

	void port_UpdateState_CrazyTurning()
	{
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_2a675);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_Crazy);
	}

	void port_UpdateState_FatSinking()
	{
		anim(OAM1);
		if (!ld_mask(wGlobalCounter, 0x0f))
			LCALL(Func_29317);
		cpu_.b = 0x01;
		CALL(MoveWarioDown);
		FARCALL(CheckCentreCollision);
		if (!b_and())
			return;
		LCALL(Func_2ad6a);
	}

	void port_UpdateState_PuffyTurning()
	{
		cpu_.a = rd8(wWarioOAMBank);
		anim(cpu_.a);
		LCALL(Func_2b342);
		FARCALL(CheckUpCollision);
		ld_cp(wWarioState, WST_PUFFY_TURNING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and()) {
			LJUMP(SetState_PuffyDeflating);
			return;
		}
		if (!anim_ended()) {
			RET();
			return;
		}
		LJUMP(SetState_PuffyRising);
	}

	void port_Func_2af9c()
	{
		CALL(ApplyJumpVelocity);
		LCALL(HandleWalk);
		cap_walk_vel(0x14, 0x10);
		FARCALL(CheckFrontCollision);
		if (!b_and()) {
			RET();
			return;
		}
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		LJUMP(Func_2afc5); // fallthrough
	}

	void port_Func_2b2a4()
	{
		CALL(ApplyJumpVelocity);
		LCALL(Func_2b17a);
		cap_walk_vel(0x08, 0x04);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (!ZF)
			return;
		st(wIsSmashAttacking, TRUE);
	}

	void port_Func_2b3dd()
	{
		CALL(ApplyJumpVelocity);
		SET_HL(gb_, wWarioStateCycles);
		if (ld_and(wWarioStateCycles)) {
			wr8(wWarioStateCycles, alu_.dec(cpu_.a));
			return;
		}
		LCALL(Func_2b17a);
		cap_walk_vel(0x04, 0x00);
	}

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

	// farcall CheckCentreCollision / ld a, b / and a / jp z, fall / update_pos_y / ret
	void ground_check(uint16_t fall)
	{
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			JUMP_TO(fall);
			return;
		}
		update_pos_y();
		RET();
	}

	void port_UpdateState_OnFire()
	{
		if (room_transition_check())
			return;
		sfx_loop(14, SFX_01E);
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			st(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_16b14, ADDR_Frameset_16b1d);
		}
		anim(OAM1);
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		ld_cp(wTransformationDuration, (uint8_t)(HOT_WARIO_TRANSITION_DURATION >> 8));
		if (ZF) {
			ld_cp(wTransformationDuration + 1, (uint8_t)HOT_WARIO_TRANSITION_DURATION);
			if (ZF) {
				LJUMP(SetState_Hot);
				return;
			}
		}
		LCALL(Func_2ae8a);
		if (!still_in(WST_ON_FIRE)) {
			RET();
			return;
		}
		ground_check(ADDR_Func_2af75);
	}

	void port_UpdateState_FatWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(28, SFX_FAT_WALK);
		anim(OAM1);
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = rd8(wTransformationDuration);
		alu_.cp(cpu_.a, (uint8_t)((FAT_WARIO_DURATION >> 8) + 1)); // in case it underflowed
		if (!CF) {
			LJUMP(SetState_FatRecovering);
			return;
		}
		cpu_.a = alu_.or_(cpu_.a, rd8(wTransformationDuration + 1));
		if (ZF) {
			LJUMP(SetState_FatRecovering);
			return;
		}
		LCALL(Func_2b239);
		if (!still_in(WST_FAT_WALKING)) {
			RET();
			return;
		}
		ground_check(ADDR_Func_290d6);
	}

	void port_UpdateState_ZombieWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(36, SFX_ZOMBIE_WALK);
		anim(OAM2);
		LCALL(Func_2b381);
		if (!still_in(WST_ZOMBIE_WALKING)) {
			RET();
			return;
		}
		ground_check(ADDR_Func_29bb9);
	}

	void port_UpdateState_FlatJumping()
	{
		if (room_transition_check())
			return;
		CALL(ApplyJumpVelocity);
		LCALL(Func_2b17a);
		CALL(Func_1762);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				RET();
				return;
			}
		}
		LJUMP(SetState_FlatFalling);
	}

	void port_UpdateState_FlatFalling()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(SetState_FlatSinking);
			return;
		}
		if (!anim_ended()) {
			anim(OAM1);
			if (anim_ended()) {
				reset_anim();
				set_frameset(ADDR_Frameset_17190);
				anim(OAM1);
				st(wWarioAnimationEnded, 0x01);
			}
		}
		// .asm_287ff
		SET_HL(gb_, wWalkVelIndex);
		cpu_.e = rd8(wWalkVelIndex);
		cpu_.d = 0x00;
		wr8(wWalkVelIndex, alu_.inc(cpu_.e));
		gb_push16(gb_, REG_DE(gb_));
		bool right = facing_right();
		uint16_t hl = alu_.add16(ADDR_Data_2b68c, REG_DE(gb_));
		SET_HL(gb_, hl);
		cpu_.b = rd8(hl);
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);

		// .asm_28816
		ld_cp(wWalkVelIndex, 0x28);
		if (CF) {
			FARCALL(CheckFrontCollision);
			if (!b_and()) {
				// .asm_288b8
				SET_DE(gb_, gb_pop16(gb_));
				hl = alu_.add16(ADDR_Data_2b664, REG_DE(gb_));
				SET_HL(gb_, hl);
				cpu_.b = rd8(hl);
				CALL(MoveWarioDown);
				FARCALL(CheckAirborneCollision);
				if (!b_and()) {
					JUMP(TriggerDownwardsFloorTransition);
					return;
				}
				play_sfx(SFX_020);
				CALL(TriggerFloorTransition);
				LJUMP(SetState_FlatIdling);
				return;
			}
			play_sfx(SFX_BUMP);
			flip_dir();
			cpu_.b = 0x03;
			move_by_dir();
			cpu_.a = 0x00;
		} else {
			// .asm_28857
			bool held;
			if (facing_right())
				held = ld_bit(wJoypadDown, B_PAD_RIGHT);
			else
				held = ld_bit(wJoypadDown, B_PAD_LEFT);
			if (!held) {
				// .asm_2886d
				xor_a();
				wr8(wFrameDuration, 0);
				wr8(wAnimationFrame, 0);
				wr8(wWarioAnimationEnded, 0);
				set_frameset(flip_dir_and() ? ADDR_Frameset_17186 : ADDR_Frameset_171b8);
				anim(OAM1);
			}
			xor_a();
		}
		// .asm_288a8
		wr8(wWalkVelIndex, cpu_.a);
		SET_DE(gb_, gb_pop16(gb_));
		RET();
	}

	void port_UpdateState_PuffyRising()
	{
		if (room_transition_check())
			return;
		sfx_loop(32, SFX_PUFF_RAISE);
		anim(ld(wWarioOAMBank));
		LCALL(Func_2b2c2);
		RET();
	}

	void port_UpdateState_VampireIdling()
	{
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_VAMPIRE_IDLING)) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(Func_2a8a7);
			return;
		}
		anim(OAM2);
		LCALL(Func_2b4f3);
		RET();
	}

	void port_UpdateState_VampireWalking()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_VAMPIRE_WALKING)) {
			RET();
			return;
		}
		anim(OAM2);
		LCALL(Func_2b509);
		if (!still_in(WST_VAMPIRE_WALKING)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_VAMPIRE_WALKING) || b_and()) {
			RET();
			return;
		}
		LJUMP(Func_2a8a7);
	}

	void port_UpdateState_BatIdling()
	{
		FARCALL(CheckCentreCollision);
		if (!still_in(WST_BAT_IDLING)) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(SetState_BatFalling);
			return;
		}
		sfx_loop(0x20, SFX_FLAP);
		anim(OAM2);
		cpu_.a = rd8(wJoypadPressed);
		if (!alu_.bit(cpu_.a, B_PAD_B)) {
			if (alu_.bit(cpu_.a, B_PAD_A)) {
				LJUMP(SetState_BatFlying);
				return;
			}
			RET();
			return;
		}
		// .asm_2aa55
		st(wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (!still_in(WST_BAT_IDLING)) {
			RET();
			return;
		}
		if (!b_and()) {
			LJUMP(SetState_VampireTransforming);
			return;
		}
		st(wCollisionBoxTop, (uint8_t)-15);
		play_sfx(SFX_0E5);
		RET();
	}

	void port_Func_2b027()
	{
		CALL(ApplyJumpVelocity);
		SET_HL(gb_, wWarioStateCounter);
		if (ld_and(wWarioStateCounter)) {
			wr8(wWarioStateCounter, alu_.dec(cpu_.a));
			return;
		}
		LCALL(Func_2b17a);
		cap_walk_vel(0x08, 0x04);
		if (!ld_and(wIsTurning))
			return;
		reset_anim();
		frameset_by_dir(ADDR_Frameset_16b26, ADDR_Frameset_16b2f);
		anim(OAM1);
	}

	void port_Func_2b10a()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_FlatJumping);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_FlatWalking);
			return;
		}
		RET();
	}

	void port_Func_2b11b()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_FlatJumping);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		bool right;
		if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			right = true;
		} else {
			LJUMP(SetState_FlatIdling);
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
		cap_walk_vel(0x04, 0x00);
		RET();
	}

	void port_Func_2b55c()
	{
		CALL(ApplyJumpVelocity);
		LCALL(Func_2b17a);
		cap_walk_vel(0x10, 0x0c);
	}

	void port_Func_2b3f9()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(Func_2a675);
			return;
		}
		LCALL(Func_2b42b);
		if (!still_in(WST_CRAZY) || !anim_ended()) {
			RET(); // done if not crazy any more
			return;
		}
		bool held;
		if (facing_right())
			held = ld_bit(wJoypadDown, B_PAD_RIGHT);
		else
			held = ld_bit(wJoypadDown, B_PAD_LEFT);
		if (!held) {
			LJUMP(SetState_CrazyTurning);
			return;
		}
		xor_a();
		wr8(wWalkVelIndex, 0);
		RET();
	}

	// ld a, [wWarioStateCounter] / and a / jr nz / ld a, [wWarioAnimationEnded] /
	// and a / jr z: true if counter is 0 and the animation ended
	bool first_anim_ended()
	{
		return !ld_and(wWarioStateCounter) && anim_ended();
	}

	void port_UpdateState_OnFireAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(Func_2ade4);
			return;
		}
		if (first_anim_ended()) {
			st(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_16b14, ADDR_Frameset_16b1d);
		}
		anim(OAM1);
		LCALL(Func_2af9c);
		if (!still_in(WST_ON_FIRE_AIRBORNE)) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_ON_FIRE_AIRBORNE)) {
				RET();
				return;
			}
			LJUMP(Func_2af75);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			if (!still_in(WST_ON_FIRE_AIRBORNE)) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (still_in(WST_ON_FIRE_AIRBORNE))
			FARCALL(SetState_OnFire);
		RET();
	}

	void port_UpdateState_HotAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(Func_2ade4);
			return;
		}
		anim(OAM1);
		LCALL(Func_2b027);
		if (!still_in(WST_HOT_AIRBORNE)) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			// rising
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_HOT_AIRBORNE)) {
				RET();
				return;
			}
			LJUMP(Func_2838a);
			return;
		}
		// .falling
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			if (!still_in(WST_HOT_AIRBORNE)) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!still_in(WST_HOT_AIRBORNE)) {
			RET();
			return;
		}
		LJUMP(SetState_Hot);
	}

	void port_UpdateState_FlatIdling()
	{
		anim(OAM1);
		LCALL(Func_2b10a);
		FARCALL(CheckCentreCollision);
		if (b_and()) {
			RET();
			return;
		}
		LJUMP(SetState_FlatFalling);
	}

	void port_UpdateState_FlatWalking()
	{
		if (room_transition_check())
			return;
		sfx_loop(10, SFX_FLAT_WALK);
		anim(OAM1);
		LCALL(Func_2b11b);
		ground_check(ADDR_SetState_FlatFalling);
	}

	void port_UpdateState_Crazy()
	{
		if (room_transition_check())
			return;
		anim(OAM2);
		LCALL(Func_2b3f9);
		if (!still_in(WST_CRAZY)) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(Func_2a67b);
			return;
		}
		RET();
	}

	void port_UpdateState_FatAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			LJUMP(Func_2926a);
			return;
		}
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = alu_.or_(rd8(wTransformationDuration), rd8(wTransformationDuration + 1));
		if (ZF)
			wr8(wTransformationDuration + 1, alu_.inc(rd8(wTransformationDuration + 1)));
		anim(OAM1);
		if (first_anim_ended()) {
			reset_anim();
			st(wWarioStateCounter, 0x01);
			frameset_by_dir(ADDR_Frameset_1783e, ADDR_Frameset_17841);
			anim(OAM1);
		}
		LCALL(Func_2b2a4);
		if (!still_in(WST_FAT_AIRBORNE)) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and() || !still_in(WST_FAT_AIRBORNE)) {
				RET();
				return;
			}
			LJUMP(Func_290d6);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		if (!ld_and(wGroundShakeCounter)) {
			st(wGroundShakeCounter, 0x20);
			st(wIsWarioGroundShaking, TRUE);
		}
		if (!ld_and(wFloorTransitionDir))
			play_sfx(SFX_024);
		LJUMP(SetState_FatLanding); // fallthrough
	}

	// ld a, BANK(oam) / hcall UpdateWarioAnimation / anim ended? jp RecoverFromTransformation
	void anim_then_recover(uint8_t bank)
	{
		anim(bank);
		if (!anim_ended()) {
			RET();
			return;
		}
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_FatRecovering()
	{
		anim_then_recover(OAM1);
	}

	void port_UpdateState_Burnt()
	{
		anim_then_recover(OAM1);
	}

	void port_UpdateState_BallOStringDizzy()
	{
		anim_then_recover(OAM1);
	}

	void port_UpdateState_ElectricDizzy()
	{
		anim_then_recover(OAM1);
	}

	void port_UpdateState_FlatStretching()
	{
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		xor_a();
		wr8(wAutoMoveState, 0);
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_ZombieRecovering()
	{
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x03);
		if (cpu_.a) {
			RET();
			return;
		}
		wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
		cpu_.a = rd8(wWarioStateCounter);
		uint16_t hl = wWarioStateCounter + 1;
		alu_.cp(cpu_.a, 0x08);
		if (cpu_.f & FLAG_C) {
			wr8(hl, cpu_.a);
		} else {
			alu_.cp(cpu_.a, 0x0b);
			if (ZF) {
				SET_HL(gb_, hl);
				JUMP(RecoverFromTransformation);
				return;
			}
			wr8(hl, alu_.dec(rd8(hl)));
			cpu_.a = rd8(hl);
		}
		for (int i = 0; i < 4; i++)
			cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0x00;
		SET_HL(gb_, alu_.add16(ADDR_WarioZombiePal, REG_DE(gb_)));
		CALL(SetWarioPal);
		RET();
	}

	void port_UpdateState_BallOStringAirborne()
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
		anim(OAM1);
		FARCALL(CheckFrontCollision);
		cpu_.a = alu_.and_(cpu_.b, 0x0f);
		if (cpu_.a) {
			LJUMP(Func_28bd5);
			return;
		}
		CALL(ApplyJumpVelocity);
		LCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x18);
		if (!CF)
			st(wWalkVelIndex, 0x14);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		LJUMP(SetState_BallOString);
	}

	void port_UpdateState_BouncyStart()
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
		LCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		ld_cp(wWarioState, WST_BOUNCY_START);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			ld_cp(wWarioState, WST_BOUNCY_START);
			if (!ZF) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		ld_cp(wWarioState, WST_BOUNCY_START);
		if (!ZF) {
			RET();
			return;
		}
		LJUMP(SetState_BouncyFloor); // fallthrough
	}

	void port_UpdateState_BouncyUpsideDown()
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
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		ld_cp(wWarioState, WST_BOUNCY_UPSIDE_DOWN);
		if (!ZF) {
			RET();
			return;
		}
		st(wWarioState, WST_BOUNCY_UPSIDE_DOWN_LANDING);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(ADDR_Frameset_1fcc85);
		anim(OAM2);
		RET();
	}

	void port_UpdateState_BouncyLastBounce()
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
		if (!anim_ended())
			anim(OAM2);
		CALL(ApplyJumpVelocity);
		LCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				RET();
				return;
			}
			ld_cp(wWarioState, WST_BOUNCY_LAST_BOUNCE);
			if (ZF)
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
		ld_cp(wWarioState, WST_BOUNCY_LAST_BOUNCE);
		if (!ZF) {
			RET();
			return;
		}
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_BallOStringKnockBack()
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
		anim(OAM1);
		LCALL(Func_2b1cc);
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		flip_dir();
		CALL(TriggerFloorTransition);
		play_sfx(SFX_03C);
		st(wWarioState, WST_GETTING_UNWRAPPED_IN_STRING);
		collision_box(-1, -27, -9, 9);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(ADDR_Frameset_17409);
		anim(OAM1);
		RET();
	}

	void port_UpdateState_ZombieSlippingThroughFloor()
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
		ld_cp(wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
		if (!ZF) {
			RET();
			return;
		}
		LCALL(Func_2b3dd);
		anim(OAM2);
		ld_cp(wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			ld_cp(wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
			if (!ZF) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		ld_cp(wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
		if (!ZF) {
			RET();
			return;
		}
		LCALL(SetState_ZombieLanding);
		if (ld_and(wDirection))
			set_frameset(ADDR_Frameset_1fc90b);
		else
			set_frameset(ADDR_Frameset_1fc8ee);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioAnimationEnded, 0);
		st(wWarioStateCounter, 0x02);
		anim(OAM2);
		RET();
	}

	void port_Func_2b56f()
	{
		cpu_.a = alu_.and_(rd8(wGlobalCounter), 0x01);
		if (cpu_.a) {
			cpu_.b = 0x01;
			CALL(MoveWarioUp);
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				RET();
				return;
			}
			get_pos();
			cpu_.b = PARTICLE_BUBBLE_BURST;
			FARCALL(CreateParticle);
			JUMP(RecoverFromTransformation);
			return;
		}
		cpu_.a = rd8(wJoypadDown);
		bool blocked;
		if (alu_.bit(cpu_.a, B_PAD_RIGHT)) {
			FARCALL(Func_19741);
			blocked = b_and();
			if (!blocked) {
				cpu_.b = 0x01;
				CALL(MoveWarioRight);
				goto clear_vel;
			}
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			FARCALL(Func_197b1);
			blocked = b_and();
			if (!blocked) {
				cpu_.b = 0x01;
				CALL(MoveWarioLeft);
				goto clear_vel;
			}
		} else {
			st(wDirection, ld(wWarioStateCounter));
			FARCALL(CheckFrontCollision);
			if (!b_and()) {
				cpu_.a = rd8(wJumpVelIndex);
				cpu_.e = cpu_.a;
				cpu_.d = 0x00;
				uint16_t hl = alu_.add16(ADDR_Data_2b6b4, REG_DE(gb_));
				SET_HL(gb_, hl);
				cpu_.b = rd8(hl);
				if (ld_and(wWarioStateCounter))
					CALL(MoveWarioRight);
				else
					CALL(MoveWarioLeft);
				SET_HL(gb_, wJumpVelIndex);
				wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
				cpu_.a = rd8(wJumpVelIndex);
				alu_.cp(cpu_.a, 0x2f);
				if (CF) {
					RET();
					return;
				}
			}
		}
		cpu_.a = alu_.xor_(rd8(wWarioStateCounter), 0x1);
		wr8(wWarioStateCounter, cpu_.a);
		wr8(wDirection, cpu_.a);
	clear_vel:
		xor_a();
		wr8(wJumpVelIndex, 0);
		RET();
	}

	void port_UpdateState_ZombieAirborne()
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
		ld_cp(wWarioState, WST_ZOMBIE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		LCALL(Func_2b3dd);
		ld_cp(wWarioState, WST_ZOMBIE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			cpu_.a = alu_.inc(0);
			wr8(wWarioStateCounter, cpu_.a);
			if (ld_and(wDirection))
				set_frameset(ADDR_Frameset_1fc8b3);
			else
				set_frameset(ADDR_Frameset_1fc8b0);
		}
		anim(OAM2);
		ld_cp(wWarioState, WST_ZOMBIE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				RET();
				return;
			}
			ld_cp(wWarioState, WST_ZOMBIE_AIRBORNE);
			if (!ZF) {
				RET();
				return;
			}
			LJUMP(Func_29bb9);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		ld_cp(wWarioState, WST_ZOMBIE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		LJUMP(SetState_ZombieLanding); // fallthrough
	}

	void port_UpdateState_BouncyAirborne()
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
		if (!anim_ended()) {
			anim(OAM2);
			ld_cp(wJumpVelTable, JUMP_VEL_BOUNCY_HIGH_JUMP);
			if (ZF && anim_ended()) {
				get_pos();
				cpu_.b = PARTICLE_BOUNCY_JUMP;
				FARCALL(CreateParticle);
			}
		}
		LCALL(DecrementTransformationDuration);
		SET_HL(gb_, wTransformationDuration + 1);
		cpu_.a = alu_.or_(rd8(wTransformationDuration), rd8(wTransformationDuration + 1));
		if (ZF) {
			LJUMP(SetState_BouncyUpsideDown);
			return;
		}
		LCALL(Func_2ae3b);
		CALL(ApplyJumpVelocity);
		LCALL(Func_2b17a);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		ld_cp(wWarioState, WST_BOUNCY_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			if (!b_and()) {
				RET();
				return;
			}
			ld_cp(wWarioState, WST_BOUNCY_AIRBORNE);
			if (!ZF) {
				RET();
				return;
			}
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			st(wWarioState, WST_BOUNCY_CEILING);
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			set_frameset(ADDR_Frameset_1fcc75);
			anim(OAM2);
			RET();
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			ld_cp(wWarioState, WST_BOUNCY_AIRBORNE);
			if (!ZF) {
				RET();
				return;
			}
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		ld_cp(wWarioState, WST_BOUNCY_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		LJUMP(SetState_BouncyFloor);
	}

	// restore hYPosHi/Lo and hXPosHi/Lo from their backups
	void restore_pos_backup()
	{
		cpu_.a = rd8(hYPosHiBackup);
		wr8(hYPosHi, cpu_.a);
		cpu_.a = rd8(hYPosLoBackup);
		wr8(hYPosLo, cpu_.a);
		cpu_.a = rd8(hXPosHiBackup);
		wr8(hXPosHi, cpu_.a);
		cpu_.a = rd8(hXPosLoBackup);
		wr8(hXPosLo, cpu_.a);
	}

	void port_UpdateState_ElectricStart()
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
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		st(wInvincibleCounter, 0x01);
		st(wWarioState, WST_ELECTRIC);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		st(wJumpVelIndex, 0x06);
		st(wCollisionBoxBottom, (uint8_t)-1);
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);
		bool low_ceiling = false;
		if (ld_and(wIsCrouching)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			cpu_.a = rd8(hYPosHi);
			wr8(hYPosHiBackup, cpu_.a);
			cpu_.a = rd8(hYPosLo);
			wr8(hYPosLoBackup, cpu_.a);
			cpu_.a = rd8(hXPosHi);
			wr8(hXPosHiBackup, cpu_.a);
			cpu_.a = rd8(hXPosLo);
			wr8(hXPosLoBackup, cpu_.a);
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_ELECTRIC);
			if (!ZF) {
				RET();
				return;
			}
			if (b_and()) {
				low_ceiling = true;
			} else {
				xor_a();
				wr8(wIsCrouching, 0);
				restore_pos_backup();
			}
		}
		if (low_ceiling) {
			st(wCollisionBoxTop, (uint8_t)-15);
			xor_a();
			wr8(wJumpVelTable, 0);
			restore_pos_backup();
		} else {
			st(wCollisionBoxTop, (uint8_t)-27);
			st(wJumpVelTable, 0x01);
		}
		if (ld_and(wDirection))
			set_frameset(ADDR_Frameset_17b7c);
		else
			set_frameset(ADDR_Frameset_17b8d);
		anim(OAM1);
		RET();
	}

	void port_UpdateState_Electric()
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
		anim(OAM1);
		FARCALL(CheckFrontCollision);
		if (!b_and()) {
			bool right = ld_and(wDirection);
			cpu_.b = 0x01;
			ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			if (CF)
				cpu_.b = alu_.inc(cpu_.b);
			if (right)
				CALL(MoveWarioRight);
			else
				CALL(MoveWarioLeft);
		} else {
			bool right = ld_and(wDirection);
			cpu_.b = 0x01;
			if (right)
				CALL(MoveWarioLeft);
			else
				CALL(MoveWarioRight);
		}

		if (!ld_and(wJumpVelTable)) {
			SET_HL(gb_, wJumpVelIndex);
			wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
			cpu_.a = rd8(wJumpVelIndex);
			alu_.cp(cpu_.a, MAX_JUMP_VEL_INDEX);
			if (!CF)
				goto dizzy;
			FARCALL(CheckCentreCollision);
			cpu_.a = alu_.and_(cpu_.b, 0x0f);
			if (cpu_.a) {
				RET();
				return;
			}
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
			st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
			RET();
			return;
		}

		for (;;) {
			cpu_.a = rd8(wJumpVelIndex);
			cpu_.e = cpu_.a;
			cpu_.d = 0x00;
			uint16_t hl = alu_.add16(ADDR_JumpVelTable_Normal, REG_DE(gb_));
			SET_HL(gb_, hl);
			if (!alu_.bit(rd8(hl), 7)) {
				// falling
				cpu_.b = rd8(hl);
				CALL(MoveWarioDown);
				SET_HL(gb_, wJumpVelIndex);
				wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
				cpu_.a = rd8(wJumpVelIndex);
				alu_.cp(cpu_.a, MAX_JUMP_VEL_INDEX);
				if (!CF)
					wr8(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
				break;
			}
			// rising
			cpu_.a = alu_.cpl(rd8(hl));
			cpu_.a = alu_.inc(cpu_.a);
			cpu_.b = cpu_.a;
			CALL(MoveWarioUp);
			SET_HL(gb_, wJumpVelIndex);
			wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
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

	dizzy:
		play_sfx(SFX_041);
		{
			bool right = ld_and(wDirection);
			uint8_t xlo = rd8(wWarioXPos + 1);
			uint8_t xhi = rd8(wWarioXPos + 0);
			if (right) {
				wr8(hXPosLo, alu_.add(xlo, 0x08));
				wr8(hXPosHi, alu_.adc(xhi, 0x00));
			} else {
				wr8(hXPosLo, alu_.sub(xlo, 0x08));
				wr8(hXPosHi, alu_.sbc(xhi, 0x00));
			}
			wr8(hYPosLo, alu_.sub(rd8(wWarioYPos + 1), 0x20));
			cpu_.a = alu_.sbc(rd8(wWarioYPos + 0), 0x00);
			wr8(hYPosHi, cpu_.a);
			SET_HL(gb_, wWarioYPos);
			SET_DE(gb_, hYPosHi);
		}
		cpu_.b = PARTICLE_SMOKE_SMALL;
		FARCALL(CreateParticle);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		flip_dir();
		st(wWarioState, WST_ELECTRIC_DIZZY);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		if (ld_and(wDirection))
			set_frameset(ADDR_Frameset_17bad);
		else
			set_frameset(ADDR_Frameset_17b9e);
		anim(OAM1);
		RET();
	}

	void port_UpdateState_PuffyDeflating()
	{
		if (!ld_and(wWarioStateCounter)) {
			anim(rd8(wWarioOAMBank));
			if (!anim_ended()) {
				RET();
				return;
			}
			st(wWarioStateCounter, 0x01);
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			if (ld_and(wDirection))
				set_frameset(ADDR_Frameset_1dcd31);
			else
				set_frameset(ADDR_Frameset_1dcd01);
		}
		anim(rd8(wWarioOAMBank));
		if (!anim_ended()) {
			RET();
			return;
		}
		JUMP(RecoverFromTransformation);
	}

	void port_UpdateState_CrazyAirborne()
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
		CALL(ApplyJumpVelocity);
		LCALL(HandleWalk);
		ld_cp(wWalkVelIndex, 0x10);
		if (!CF)
			st(wWalkVelIndex, 0x0c);
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
		LJUMP(SetState_Crazy);
	}

	void port_UpdateState_BatFalling()
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
		ld_cp(wWarioState, WST_BAT_FALLING);
		if (!ZF) {
			RET();
			return;
		}
		anim(OAM2);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_BatFlying);
			return;
		}
		CALL(ApplyJumpVelocity);
		LCALL(Func_2b17a);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		if (ld_and(wIsTurning))
			LCALL(Func_2abc1);
		FARCALL(CheckAirborneCollision);
		ld_cp(wWarioState, WST_BAT_FALLING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and()) {
			RET();
			return;
		}
		update_pos_y();
		LJUMP(SetState_BatIdling);
	}

	void port_UpdateState_VampireAirborne()
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
		ld_cp(wWarioState, WST_VAMPIRE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		anim(OAM2);
		LCALL(Func_2b55c);
		ld_cp(wWarioState, WST_VAMPIRE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF) {
			FARCALL(CheckUpCollision);
			ld_cp(wWarioState, WST_VAMPIRE_AIRBORNE);
			if (!ZF) {
				RET();
				return;
			}
			if (!b_and()) {
				RET();
				return;
			}
			LJUMP(Func_2a8a7);
			return;
		}
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		ld_cp(wWarioState, WST_VAMPIRE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		LJUMP(SetState_VampireIdling);
	}

	void port_UpdateState_BatFlying()
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
		ld_cp(wWarioState, WST_BAT_FLYING);
		if (!ZF) {
			RET();
			return;
		}
		sfx_loop(0x10, SFX_FLAP);
		anim(OAM2);
		FARCALL(CheckUpCollision);
		ld_cp(wWarioState, WST_BAT_FLYING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and()) {
			LJUMP(SetState_BatFalling);
			return;
		}
		LCALL(Func_2b63b);
		ld_cp(wWarioState, WST_BAT_FLYING);
		if (!ZF) {
			RET();
			return;
		}
		LCALL(Func_2b17a);
		ld_cp(wWalkVelIndex, 0x08);
		if (!CF)
			st(wWalkVelIndex, 0x04);
		if (!ld_and(wIsTurning)) {
			RET();
			return;
		}
		LJUMP(Func_2abc1); // fallthrough
	}

	void port_UpdateState_CrazySpinning()
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
		anim(OAM1);
		SET_HL(gb_, wWarioStateCounter);
		wr8(wWarioStateCounter, alu_.inc(rd8(wWarioStateCounter)));
		cpu_.a = rd8(wWarioStateCounter);
		alu_.cp(cpu_.a, 0x80);
		if (CF) {
			RET();
			return;
		}
		wr8(wWarioStateCounter, 0x00);
		play_sfx(SFX_043);
		st(wWarioState, WST_CRAZY_DIZZY);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
			wJumpVelIndex, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		load_gfx(BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
		set_oam(BANK_OAM_17bbc, ADDR_OAM_17bbc);
		set_frameset(ADDR_Frameset_17ce2);
		anim(OAM1);
		RET();
	}

	void port_UpdateState_InBubble()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wWaterInteraction)) {
			// .asm_2ad45
			get_pos();
			cpu_.b = PARTICLE_BUBBLE_BURST;
			FARCALL(CreateParticle);
			JUMP(RecoverFromTransformation);
			return;
		}
		sfx_loop(0x0e, SFX_023);
		anim(OAM2);
		LCALL(Func_2b56f);
		RET();
	}

	// ld a, [wWarioState] / sub $60 / rst JumpTable (table after the rst)
	void port_UpdateWarioStates_Group3()
	{
		cpu_.a = alu_.sub(rd8(wWarioState), 0x60);
		gb_push16(gb_, ADDR_UpdateWarioStates_Group3 + 6); // rst JumpTable
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
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates3;

PORT_EX(DecrementTransformationDuration, 0, 0) { WarioStates3(gb).port_DecrementTransformationDuration(); }
PORT_EX(Func_28c15, 0, 0) { WarioStates3(gb).port_Func_28c15(); }
PORT_EX(SetState_Hot, 0, 0) { WarioStates3(gb).port_SetState_Hot(); }
PORT_EX(SetState_FatIdling, 0, 0) { WarioStates3(gb).port_SetState_FatIdling(); }
PORT_EX(SetState_FatWalking, 0, 0) { WarioStates3(gb).port_SetState_FatWalking(); }
PORT_EX(SetState_FatTurning, 0, 0) { WarioStates3(gb).port_SetState_FatTurning(); }
PORT_EX(SetState_FatBumping, 0, 0) { WarioStates3(gb).port_SetState_FatBumping(); }
PORT_EX(SetState_FatAirborne, 0, 0) { WarioStates3(gb).port_SetState_FatAirborne(); }
PORT_EX(SetState_FatLanding, 0, 0) { WarioStates3(gb).port_SetState_FatLanding(); }
PORT_EX(SetState_FatRecovering, 0, 0) { WarioStates3(gb).port_SetState_FatRecovering(); }
PORT_EX(SetState_BallOString, 0, 0) { WarioStates3(gb).port_SetState_BallOString(); }
PORT_EX(SetState_BallOStringKnockBack, 0, 0) { WarioStates3(gb).port_SetState_BallOStringKnockBack(); }
PORT_EX(SetState_ZombieIdling, 0, 0) { WarioStates3(gb).port_SetState_ZombieIdling(); }
PORT_EX(SetState_ZombieWalking, 0, 0) { WarioStates3(gb).port_SetState_ZombieWalking(); }
PORT_EX(SetState_ZombieTurning, 0, 0) { WarioStates3(gb).port_SetState_ZombieTurning(); }
PORT_EX(SetState_ZombieAirborne, 0, 0) { WarioStates3(gb).port_SetState_ZombieAirborne(); }
PORT_EX(SetState_ZombieKnockBack, 0, 0) { WarioStates3(gb).port_SetState_ZombieKnockBack(); }
PORT_EX(SetState_ZombieRecovering, 0, 0) { WarioStates3(gb).port_SetState_ZombieRecovering(); }
PORT_EX(SetState_BouncyAirborne, 0, 0) { WarioStates3(gb).port_SetState_BouncyAirborne(); }
PORT_EX(SetState_BouncyUpsideDown, 0, 0) { WarioStates3(gb).port_SetState_BouncyUpsideDown(); }
PORT_EX(SetState_BouncyFloor, 0, 0) { WarioStates3(gb).port_SetState_BouncyFloor(); }
PORT_EX(SetState_BouncyLastBounce, 0, 0) { WarioStates3(gb).port_SetState_BouncyLastBounce(); }
PORT_EX(UpdateState_BouncyUpsideLanding, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyUpsideLanding(); }
PORT_EX(SetState_OnFireAirborne, 0, 0) { WarioStates3(gb).port_SetState_OnFireAirborne(); }
PORT_EX(Func_2afc5, 0, 0) { WarioStates3(gb).port_Func_2afc5(); }
PORT_EX(Func_2ae5c, 0, 0) { WarioStates3(gb).port_Func_2ae5c(); }
PORT_EX(Func_2ae3b, 0, 0) { WarioStates3(gb).port_Func_2ae3b(); }
PORT_EX(Func_29bb9, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_29bb9(); }
PORT_EX(Func_29be1, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_29be1(); }
PORT_EX(Func_290d6, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_290d6(); }
PORT_EX(Func_290a1, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_290a1(); }
PORT_EX(Func_28bd5, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_28bd5(); }
PORT_EX(Func_2a0c7, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a0c7(); }
PORT_EX(Func_2af75, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2af75(); }
PORT_EX(Func_2af81, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2af81(); }
PORT_EX(Func_2a0bb, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a0bb(); }
PORT_EX(Func_2a0c2, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a0c2(); }
PORT_EX(UpdateState_FlatSquished, 0, 0) { WarioStates3(gb).port_UpdateState_FlatSquished(); }
PORT_EX(SetState_FlatSquished, 0, 0) { WarioStates3(gb).port_SetState_FlatSquished(); }
PORT_EX(SetState_Hot_ResetDuration, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_SetState_Hot_ResetDuration(); }
PORT_EX(Func_2b4d0, 0, 0) { WarioStates3(gb).port_Func_2b4d0(); }
PORT_EX(Func_2b342, 0, 0) { WarioStates3(gb).port_Func_2b342(); }
PORT_EX(Func_28f6d, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_28f6d(); }
PORT_EX(SetState_BatFalling, 0, 0) { WarioStates3(gb).port_SetState_BatFalling(); }
PORT_EX(UpdateState_ZombieWrithing, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieWrithing(); }
PORT_EX(UpdateState_ZombieTurning, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieTurning(); }
PORT_EX(Func_2838a, 0, 0) { WarioStates3(gb).port_Func_2838a(); }
PORT_EX(SetState_VampireAirborne, 0, 0) { WarioStates3(gb).port_SetState_VampireAirborne(); }
PORT_EX(UpdateState_GettingWrappedInString, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_GettingWrappedInString(); }
PORT_EX(Func_2ade4, 0, 0) { WarioStates3(gb).port_Func_2ade4(); }
PORT_EX(UpdateState_FatTurning, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatTurning(); }
PORT_EX(UpdateState_BouncyCeiling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyCeiling(); }
PORT_EX(SetState_FlatSquishedLifting, 0, 0) { WarioStates3(gb).port_SetState_FlatSquishedLifting(); }
PORT_EX(UpdateState_GettingUnwrappedInString, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_GettingUnwrappedInString(); }
PORT_EX(Func_2ac7d, 0, 0) { WarioStates3(gb).port_Func_2ac7d(); }
PORT_EX(SetState_VampireTransforming, 0, 0) { WarioStates3(gb).port_SetState_VampireTransforming(); }
PORT_EX(SetState_VampireIdling, 0, 0) { WarioStates3(gb).port_SetState_VampireIdling(); }
PORT_EX(SetState_FlatIdling, 0, 0) { WarioStates3(gb).port_SetState_FlatIdling(); }
PORT_EX(SetState_FlatSinking, 0, 0) { WarioStates3(gb).port_SetState_FlatSinking(); }
PORT_EX(Func_289c5, 0, 0) { WarioStates3(gb).port_Func_289c5(); }
PORT_EX(Func_2abc1, 0, 0) { WarioStates3(gb).port_Func_2abc1(); }
PORT_EX(SetState_ZombieWrithing, 0, 0) { WarioStates3(gb).port_SetState_ZombieWrithing(); }
PORT_EX(UpdateState_ZombieKnockBack, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieKnockBack(); }
PORT_EX(UpdateState_ZombieLanding, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieLanding(); }
PORT_EX(Func_2a2e7, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a2e7(); }
PORT_EX(UpdateState_BouncyFloor, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyFloor(); }
PORT_EX(Func_2926a, 0, 0) { WarioStates3(gb).port_Func_2926a(); }
PORT_EX(UpdateState_FatLanding, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatLanding(); }
PORT_EX(SetState_FlatWalking, 0, 0) { WarioStates3(gb).port_SetState_FlatWalking(); }
PORT_EX(SetState_FlatFalling, 0, 0) { WarioStates3(gb).port_SetState_FlatFalling(); }
PORT_EX(Func_29317, 0, 0) { WarioStates3(gb).port_Func_29317(); }
PORT_EX(SetState_TurningInvisible, 0, 0) { WarioStates3(gb).port_SetState_TurningInvisible(); }
PORT_EX(SetState_CrazyTurning, 0, 0) { WarioStates3(gb).port_SetState_CrazyTurning(); }
PORT_EX(SetState_VampireTurning, 0, 0) { WarioStates3(gb).port_SetState_VampireTurning(); }
PORT_EX(SetState_CrazyAirborne, 0, 0) { WarioStates3(gb).port_SetState_CrazyAirborne(); }
PORT_EX(SetState_PuffyTurning, 0, 0) { WarioStates3(gb).port_SetState_PuffyTurning(); }
PORT_EX(SetState_BatIdling, 0, 0) { WarioStates3(gb).port_SetState_BatIdling(); }
PORT_EX(SetState_PuffyDeflating, 0, 0) { WarioStates3(gb).port_SetState_PuffyDeflating(); }
PORT_EX(SetState_PuffyRising, 0, 0) { WarioStates3(gb).port_SetState_PuffyRising(); }
PORT_EX(SetState_BatTransforming, 0, 0) { WarioStates3(gb).port_SetState_BatTransforming(); }
PORT_EX(SetState_VampireWalking, 0, 0) { WarioStates3(gb).port_SetState_VampireWalking(); }
PORT_EX(Func_2a675, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a675(); }
PORT_EX(Func_2a67b, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a67b(); }
PORT_EX(Func_2a8a7, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a8a7(); }
PORT_EX(Func_2a0b2, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a0b2(); }
PORT_EX(Func_2a8ae, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2a8ae(); }
PORT_EX(Func_28380, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_28380(); }
PORT_EX(UpdateState_BatTransforming, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BatTransforming(); }
PORT_EX(UpdateState_PuffyInflating, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_PuffyInflating(); }
PORT_EX(UpdateState_GettingFlat, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_GettingFlat(); }
PORT_EX(UpdateState_FatIdling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatIdling(); }
PORT_EX(UpdateState_FlatSquishedLifting, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatSquishedLifting(); }
PORT_EX(Func_2b63b, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b63b(); }
PORT_EX(Func_2b34e, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b34e(); }
PORT_EX(UpdateState_FatEating, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatEating(); }
PORT_EX(UpdateState_FlatSinking, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatSinking(); }
PORT_EX(Func_2ad6a, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2ad6a(); }
PORT_EX(SetState_Crazy, 0, 0) { WarioStates3(gb).port_SetState_Crazy(); }
PORT_EX(SetState_CrazySpinning, 0, 0) { WarioStates3(gb).port_SetState_CrazySpinning(); }
PORT_EX(SetState_PuffyInflating, 0, 0) { WarioStates3(gb).port_SetState_PuffyInflating(); }
PORT_EX(UpdateState_GettingFlatAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_GettingFlatAirborne(); }
PORT_EX(SetState_FlatAirborne, 0, 0) { WarioStates3(gb).port_SetState_FlatAirborne(); }
PORT_EX(UpdateState_FatBumping, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatBumping(); }
PORT_EX(UpdateState_ZombieIdling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieIdling(); }
PORT_EX(Func_2b1cc, 0, 0) { WarioStates3(gb).port_Func_2b1cc(); }
PORT_EX(UpdateState_BallOString, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BallOString(); }
PORT_EX(HandleHotWarioInput, 0, 0) { WarioStates3(gb).port_HandleHotWarioInput(); }
PORT_EX(SetState_ZombieLanding, 0, 0) { WarioStates3(gb).port_SetState_ZombieLanding(); }
PORT_EX(SetState_BouncyStart, 0, 0) { WarioStates3(gb).port_SetState_BouncyStart(); }
PORT_EX(UpdateState_TurningInvisible, 0, 0) { WarioStates3(gb).port_UpdateState_TurningInvisible(); }
PORT_EX(Func_2b17a, 0, 0) { WarioStates3(gb).port_Func_2b17a(); }
PORT_EX(HandleWalk, 0, 0) { WarioStates3(gb).port_HandleWalk(); }
PORT_EX(Func_2b239, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b239(); }
PORT_EX(Func_2b381, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b381(); }
PORT_EX(Func_2ae8a, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2ae8a(); }
PORT_EX(UpdateState_Hot, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_Hot(); }
PORT_EX(SetState_BatFlying, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_SetState_BatFlying(); }
PORT_EX(SetState_FlatJumping, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_SetState_FlatJumping(); }
PORT_EX(Func_2b42b, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b42b(); }
PORT_EX(Func_2b4f3, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b4f3(); }
PORT_EX(Func_2b509, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b509(); }
PORT_EX(Func_2b2c2, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b2c2(); }
PORT_EX(UpdateState_CrazyDizzy, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_CrazyDizzy(); }
PORT_EX(UpdateState_VampireTurning, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_VampireTurning(); }
PORT_EX(UpdateState_VampireTransforming, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_VampireTransforming(); }
PORT_EX(UpdateState_FlatStretchingUnderwater, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatStretchingUnderwater(); }
PORT_EX(UpdateState_CrazyTurning, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_CrazyTurning(); }
PORT_EX(UpdateState_FatSinking, 0, 0) { WarioStates3(gb).port_UpdateState_FatSinking(); }
PORT_EX(UpdateState_PuffyTurning, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_PuffyTurning(); }
PORT_EX(Func_2af9c, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2af9c(); }
PORT_EX(Func_2b2a4, 0, 0) { WarioStates3(gb).port_Func_2b2a4(); }
PORT_EX(Func_2b3dd, 0, 0) { WarioStates3(gb).port_Func_2b3dd(); }
PORT_EX(UpdateState_OnFire, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_OnFire(); }
PORT_EX(UpdateState_FatWalking, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatWalking(); }
PORT_EX(UpdateState_ZombieWalking, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieWalking(); }
PORT_EX(UpdateState_FlatJumping, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatJumping(); }
PORT_EX(UpdateState_FlatFalling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatFalling(); }
PORT_EX(UpdateState_PuffyRising, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_PuffyRising(); }
PORT_EX(UpdateState_VampireIdling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_VampireIdling(); }
PORT_EX(UpdateState_VampireWalking, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_VampireWalking(); }
PORT_EX(UpdateState_BatIdling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BatIdling(); }
PORT_EX(Func_2b027, 0, 0) { WarioStates3(gb).port_Func_2b027(); }
PORT_EX(Func_2b10a, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b10a(); }
PORT_EX(Func_2b11b, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b11b(); }
PORT_EX(Func_2b55c, 0, 0) { WarioStates3(gb).port_Func_2b55c(); }
PORT_EX(Func_2b3f9, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b3f9(); }
PORT_EX(UpdateState_OnFireAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_OnFireAirborne(); }
PORT_EX(UpdateState_HotAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_HotAirborne(); }
PORT_EX(UpdateState_FlatIdling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatIdling(); }
PORT_EX(UpdateState_FlatWalking, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatWalking(); }
PORT_EX(UpdateState_Crazy, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_Crazy(); }
PORT_EX(UpdateState_FatAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatAirborne(); }
PORT_EX(UpdateState_FatRecovering, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FatRecovering(); }
PORT_EX(UpdateState_Burnt, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_Burnt(); }
PORT_EX(UpdateState_BallOStringDizzy, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BallOStringDizzy(); }
PORT_EX(UpdateState_ElectricDizzy, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ElectricDizzy(); }
PORT_EX(UpdateState_FlatStretching, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_FlatStretching(); }
PORT_EX(UpdateState_ZombieRecovering, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieRecovering(); }
PORT_EX(UpdateState_BallOStringAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BallOStringAirborne(); }
PORT_EX(UpdateState_BouncyStart, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyStart(); }
PORT_EX(UpdateState_BouncyUpsideDown, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyUpsideDown(); }
PORT_EX(UpdateState_BouncyLastBounce, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyLastBounce(); }
PORT_EX(UpdateState_BallOStringKnockBack, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BallOStringKnockBack(); }
PORT_EX(UpdateState_ZombieSlippingThroughFloor, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieSlippingThroughFloor(); }
PORT_EX(Func_2b56f, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_Func_2b56f(); }
PORT_EX(UpdateState_ZombieAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ZombieAirborne(); }
PORT_EX(UpdateState_BouncyAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BouncyAirborne(); }
PORT_EX(UpdateState_ElectricStart, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_ElectricStart(); }
PORT_EX(UpdateState_Electric, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_Electric(); }
PORT_EX(UpdateState_PuffyDeflating, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_PuffyDeflating(); }
PORT_EX(UpdateState_CrazyAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_CrazyAirborne(); }
PORT_EX(UpdateState_BatFalling, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BatFalling(); }
PORT_EX(UpdateState_VampireAirborne, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_VampireAirborne(); }
PORT_EX(UpdateState_BatFlying, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_BatFlying(); }
PORT_EX(UpdateState_CrazySpinning, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_CrazySpinning(); }
PORT_EX(UpdateState_InBubble, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateState_InBubble(); }
PORT_EX(UpdateWarioStates_Group3, HOOK_MANUAL_RET, 0) { WarioStates3(gb).port_UpdateWarioStates_Group3(); }
