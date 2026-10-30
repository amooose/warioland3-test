// Port of src/engine/level/wario_states_3.asm
#include "wario_states.h"

PORT(DecrementTransformationDuration)
{
	// 16-bit big-endian counter
	uint8_t lo = alu_sub(gb, rd8(gb, wTransformationDuration + 1), 1);
	wr8(gb, wTransformationDuration + 1, lo);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wTransformationDuration), 0);
	wr8(gb, wTransformationDuration, gb->cpu.a);
	SET_HL(gb, wTransformationDuration);
}

PORT(Func_28c15)
{
	gb->cpu.a = FALLING_JUMP_VEL_INDEX;
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	gb->cpu.a = JUMP_VEL_HIGH_JUMP;
	wr8(gb, wJumpVelTable, gb->cpu.a);
	gb->cpu.a = WST_BALL_O_STRING_AIRBORNE;
	wr8(gb, wWarioState, gb->cpu.a);
}

PORT(SetState_Hot)
{
	st(gb, wTransformationProgress, 2);
	st(gb, wWarioState, WST_HOT);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, 0};
	clear(gb, zero);
	st(gb, wStingTouchState, TOUCH_VANISH);
	st(gb, wTouchState, TOUCH_VANISH);
	st(gb, wca94, 0x02);
	reset_anim(gb);

	// face the way the d-pad points
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
	if (gb->cpu.a)
		st(gb, wDirection, alu_bit(gb, gb->cpu.a, B_PAD_RIGHT) ? DIRECTION_RIGHT : DIRECTION_LEFT);
	frameset_by_dir(gb, ADDR_Frameset_16b26, ADDR_Frameset_16b2f);
	anim(gb, OAM1);
}

PORT(SetState_FatIdling)
{
	st(gb, wWarioState, WST_FAT_IDLING);
	st(gb, wTouchState, TOUCH_ATTACK);
	st(gb, wStingTouchState, TOUCH_ATTACK);
	st(gb, wca94, 0x01);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1781c, ADDR_Frameset_1781f);
	anim(gb, OAM1);
}

PORT(SetState_FatWalking)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_FAT_WALKING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wJumpVelIndex, 0);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_17822, ADDR_Frameset_1782b);
	anim(gb, OAM1);
}

PORT(SetState_FatTurning)
{
	st(gb, wWarioState, WST_FAT_TURNING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_178f3, ADDR_Frameset_178ea);
	anim(gb, OAM1);
}

PORT(SetState_FatBumping)
{
	st(gb, wWarioState, WST_FAT_BUMPING);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_178fc, ADDR_Frameset_17905);
	anim(gb, OAM1);
}

PORT(SetState_FatAirborne)
{
	static const uint16_t zero[] = {wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	anim(gb, OAM1);
	st(gb, wWarioState, WST_FAT_AIRBORNE);
}

PORT(SetState_FatLanding)
{
	st(gb, wWarioState, WST_FAT_LANDING);
	static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
		wWarioStateCounter, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_17872, ADDR_Frameset_17887);
	anim(gb, OAM1);
}

PORT(SetState_FatRecovering)
{
	st(gb, wWarioState, WST_FAT_RECOVERING);
	touch_states(gb, TOUCH_BUMP, 0x02);
	static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
		wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_17844, ADDR_Frameset_1785b);
	anim(gb, OAM1);
}

PORT(SetState_BallOString)
{
	st(gb, wWarioState, WST_BALL_O_STRING);
	st(gb, wTransformationProgress, 1);
	static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCycles,
		wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	collision_box(gb, -1, -24, -9, 9);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1741f, ADDR_Frameset_173fe);
	anim(gb, OAM1);
}

PORT(SetState_BallOStringKnockBack)
{
	xor_a(gb);
	wr8(gb, wTransformationProgress, 0);
	play_sfx(gb, SFX_BUMP);
	st(gb, wWarioState, WST_BALL_O_STRING_KNOCK_BACK);
	st(gb, wJumpVelIndex, 0x0a);
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	collision_box(gb, -1, -27, -9, 9);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1742a);
	anim(gb, OAM1);
}

PORT(SetState_ZombieIdling)
{
	st(gb, wWarioState, WST_ZOMBIE_IDLING);
	collision_box(gb, -1, -27, -9, 9);
	touch_states(gb, TOUCH_VANISH, 0x02);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wInvisibleFrame,
		wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioZombiePal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioZombieGfx, ADDR_WarioZombieGfx);
	set_oam(gb, BANK_OAM_1fc48b, ADDR_OAM_1fc48b);
	frameset_by_dir(gb, ADDR_Frameset_1fc8b6, ADDR_Frameset_1fc8b9);
	anim(gb, OAM2);
}

PORT(SetState_ZombieWalking)
{
	static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_ZOMBIE_WALKING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fc870, ADDR_Frameset_1fc879);
	anim(gb, OAM2);
}

PORT(SetState_ZombieTurning)
{
	st(gb, wWarioState, WST_ZOMBIE_TURNING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fc89d, ADDR_Frameset_1fc894);
	anim(gb, OAM2);
}

PORT(SetState_ZombieAirborne)
{
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	xor_a(gb);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wWarioState, WST_ZOMBIE_AIRBORNE);
	reset_anim(gb);
	anim(gb, OAM2);
}

PORT(SetState_ZombieKnockBack)
{
	static const uint8_t skip[] = {WST_ZOMBIE_KNOCK_BACK, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR,
		WST_ZOMBIE_LANDING};
	ld(gb, wWarioState);
	for (int i = 0; i < 3; i++) {
		alu_cp(gb, gb->cpu.a, skip[i]);
		if (ZF)
			return;
	}
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	st(gb, wWarioState, WST_ZOMBIE_KNOCK_BACK);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fc92e, ADDR_Frameset_1fc93b);
	anim(gb, OAM2);
}

PORT(SetState_ZombieRecovering)
{
	st(gb, wWarioState, WST_ZOMBIE_RECOVERING);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
}

PORT(SetState_BouncyAirborne)
{
	st(gb, wWarioState, WST_BOUNCY_AIRBORNE);
	play_sfx(gb, SFX_02B);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1fcc6c);
	anim(gb, OAM2);
}

PORT(SetState_BouncyUpsideDown)
{
	st(gb, wWarioState, WST_BOUNCY_UPSIDE_DOWN);
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	xor_a(gb);
	wr8(gb, wWarioStateCycles, 0);
	touch_states(gb, TOUCH_PASS_THROUGH, 0x02);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioBouncyPal);
	CALL(SetWarioPal);
	set_frameset(gb, ADDR_Frameset_1fcc82);
	anim(gb, OAM2);
}

PORT(SetState_BouncyFloor)
{
	static const uint16_t zero[] = {wJumpVelIndex, wJumpVelTable, wWarioStateCounter,
		wWalkVelIndex, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_BOUNCY_FLOOR);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1fcc5f);
	anim(gb, OAM2);
}

PORT(SetState_BouncyLastBounce)
{
	st(gb, wWarioState, WST_BOUNCY_LAST_BOUNCE);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	anim(gb, OAM2);
}

PORT_JUMP(UpdateState_BouncyUpsideLanding)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(Func_2a2e7); // fallthrough
}

PORT(SetState_OnFireAirborne)
{
	xor_a(gb);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wWarioState, WST_ON_FIRE_AIRBORNE);
}

// Hot Wario bounces off a wall: turns around and moves back 3 pixels.
PORT(Func_2afc5)
{
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), 0x1);
	wr8(gb, wDirection, gb->cpu.a);
	gb->cpu.b = 0x03;
	bool right = ld_and(gb, wDirection);
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	bool counter = ld_and(gb, wWarioStateCounter);
	if (right)
		set_frameset(gb, counter ? ADDR_Frameset_16b1d : ADDR_Frameset_16b49);
	else
		set_frameset(gb, counter ? ADDR_Frameset_16b14 : ADDR_Frameset_16b38);
	anim(gb, OAM1);
}

// Returns carry on the frames the transformation flashes as it runs out:
// every 4th frame in the last $78, every 16th from $168 left.
PORT(Func_2ae5c)
{
	SET_HL(gb, wTransformationDuration + 1);
	ld(gb, wTransformationDuration);
	bool carry;
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!CF) {
		carry = false;
	} else if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		ld_cp(gb, wTransformationDuration + 1, 0x68);
		if (!CF) {
			carry = false;
		} else {
			gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b1111);
			carry = !gb->cpu.a;
		}
	} else {
		ld_cp(gb, wTransformationDuration + 1, 0x78);
		if (!CF) {
			gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b1111);
			carry = !gb->cpu.a;
		} else {
			gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b11);
			if (gb->cpu.a) {
				carry = false;
			} else {
				gb->cpu.a = 0x01;
				carry = true;
			}
		}
	}
	// scf (/ ccf): n and h cleared, z kept
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | (carry ? FLAG_C : 0));
}

// Bouncy Wario flashes red as the transformation runs out.
PORT(Func_2ae3b)
{
	LCALL(Func_2ae5c);
	if (!CF)
		return;
	SET_DE(gb, 0);
	gb->cpu.a = alu_xor(gb, rd8(gb, wWarioStateCycles), 0x1);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	uint16_t pal = ZF ? ADDR_WarioBouncyPal : ADDR_WarioBouncyRedPal;
	SET_HL(gb, alu_add16(gb, pal, 0));
	CALL(SetWarioPal);
}

PORT_JUMP(Func_29bb9)
{
	frameset_by_dir(gb, ADDR_Frameset_1fc8b0, ADDR_Frameset_1fc8b3);
	st(gb, wWarioStateCounter, 0x01);
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(SetState_ZombieAirborne);
}

PORT_JUMP(Func_29be1)
{
	frameset_by_dir(gb, ADDR_Frameset_1fc8a6, ADDR_Frameset_1fc8ab);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wJumpVelIndex, 0);
	LJUMP(SetState_ZombieAirborne); // fallthrough
}

PORT_JUMP(Func_290d6)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	st(gb, wIsSmashAttacking, TRUE);
	wr8(gb, wWarioStateCounter, TRUE);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	frameset_by_dir(gb, ADDR_Frameset_1783e, ADDR_Frameset_17841);
	LJUMP(SetState_FatAirborne);
}

PORT_JUMP(Func_290a1)
{
	play_sfx(gb, SFX_JUMP);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wIsSmashAttacking, 0);
	wr8(gb, wWarioStateCounter, 0);
	st(gb, wJumpVelTable, 0x01);
	frameset_by_dir(gb, ADDR_Frameset_17834, ADDR_Frameset_17839);
	LJUMP(SetState_FatAirborne);
}

PORT_JUMP(Func_28bd5)
{
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), 0x1);
	wr8(gb, wDirection, gb->cpu.a);
	inc_counter(gb);
	alu_cp(gb, gb->cpu.a, 0x05);
	if (!CF) {
		LJUMP(SetState_BallOStringKnockBack);
		return;
	}
	frameset_by_dir(gb, ADDR_Frameset_1741f, ADDR_Frameset_173fe);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(Func_2a0c7)
{
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	LJUMP(SetState_BouncyAirborne); // fallthrough
}

PORT_JUMP(Func_2af75)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	st(gb, wJumpVelTable, JUMP_VEL_HIGH_JUMP);
	LJUMP(SetState_OnFireAirborne);
}

PORT_JUMP(Func_2af81)
{
	play_sfx(gb, SFX_JUMP);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	LJUMP(SetState_OnFireAirborne); // fallthrough
}

PORT_JUMP(Func_2a0bb)
{
	st(gb, wJumpVelTable, JUMP_VEL_BOUNCY_HIGH_JUMP);
	LJUMP(Func_2a0c7);
}

PORT_JUMP(Func_2a0c2)
{
	st(gb, wJumpVelTable, JUMP_VEL_BOUNCY_JUMP);
	LJUMP(Func_2a0c7); // fallthrough
}

PORT(UpdateState_FlatSquished)
{
}

PORT(SetState_FlatSquished)
{
	st(gb, wWarioState, WST_FLAT_SQUISHED);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
}

PORT_JUMP(SetState_Hot_ResetDuration)
{
	st(gb, wTransformationDuration + 0, (uint8_t)(HOT_WARIO_TRANSITION_DURATION >> 8));
	st(gb, wTransformationDuration + 1, (uint8_t)HOT_WARIO_TRANSITION_DURATION);
	LJUMP(SetState_Hot); // fallthrough
}

PORT(Func_2b4d0)
{
	get_pos(gb);
	gb->cpu.b = PARTICLE_DUST;
	FARCALL(CreateParticle);
}

PORT(Func_2b342)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 1);
	if (!ZF)
		return;
	gb->cpu.b = 0x01;
	CALL(MoveWarioUp);
}

PORT_JUMP(Func_28f6d)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(Func_290a1);
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_FatWalking);
		return;
	}
	RET();
}

PORT(SetState_BatFalling)
{
	st(gb, wWarioState, WST_BAT_FALLING);
	st(gb, wJumpVelTable, JUMP_VEL_HIGH_JUMP);
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
}

PORT_JUMP(UpdateState_ZombieWrithing)
{
	anim(gb, OAM2);
	if (ld_and(gb, wJoypadDown)) {
		LJUMP(SetState_ZombieIdling);
		return;
	}
	RET();
}

PORT_JUMP(UpdateState_ZombieTurning)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_ZombieWalking);
}

PORT(Func_2838a)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	st(gb, wWarioState, WST_HOT_AIRBORNE);
	st(gb, wWarioStateCounter, 0x08);
}

PORT(SetState_VampireAirborne)
{
	st(gb, wWarioState, WST_VAMPIRE_AIRBORNE);
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
}

PORT_JUMP(UpdateState_GettingWrappedInString)
{
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(Func_28c15);
		return;
	}
	LJUMP(SetState_BallOString); // fallthrough
}

PORT(Func_2ade4)
{
	CALL(ClearTransformationValues);
	st(gb, wInvincibleCounter, 0x10);
	CALL(UpdateLevelMusic);
	get_pos(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.b = PARTICLE_SMOKE_LARGE;
	FARCALL(CreateParticle);
	play_sfx(gb, SFX_SPLASH);
	FARCALL(DiveFromSurface_SkipSplash);
}

PORT_JUMP(UpdateState_FatTurning)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	anim(gb, OAM1);
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(Func_290a1);
		return;
	}
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_FatWalking);
}

PORT_JUMP(UpdateState_BouncyCeiling)
{
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = alu_or(gb, rd8(gb, wTransformationDuration), rd8(gb, wTransformationDuration + 1));
	if (ZF) {
		LJUMP(SetState_BouncyUpsideDown);
		return;
	}
	LCALL(Func_2ae3b);
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(SetState_BouncyAirborne);
}

PORT(SetState_FlatSquishedLifting)
{
	st(gb, wIsIntangible, TRUE);
	st(gb, wWarioState, WST_FLAT_SQUISHED_LIFTING);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1716c);
	anim(gb, OAM1);
}

// collision box: bottom -1, top -8, left -11, right 11 (flat Wario)
static void flat_box(GB *gb)
{
	collision_box(gb, -1, -8, -11, 11);
}

PORT_JUMP(UpdateState_GettingUnwrappedInString)
{
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_03D);
	st(gb, wWarioState, WST_BALL_O_STRING_DIZZY);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
		wJumpVelIndex, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
	set_oam(gb, BANK_OAM_17bbc, ADDR_OAM_17bbc);
	set_frameset(gb, ADDR_Frameset_17ce2);
	anim(gb, OAM1);
	RET();
}

PORT(Func_2ac7d)
{
	st(gb, wWarioState, WST_IN_BUBBLE);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
		wJumpVelIndex, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioBubblePal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioBubbleGfx, ADDR_WarioBubbleGfx);
	set_oam(gb, BANK_OAM_1fd87c, ADDR_OAM_1fd87c);
	ld(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	set_frameset(gb, ADDR_Frameset_1fdb2c);
	anim(gb, OAM2);
}

PORT(SetState_VampireTransforming)
{
	play_sfx(gb, SFX_045);
	st(gb, wWarioState, WST_VAMPIRE_TRANSFORMING);
	touch_states(gb, TOUCH_VANISH, 0x02);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
		wJumpVelIndex, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioVampirePal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioBatGfx, ADDR_WarioBatGfx);
	set_oam(gb, BANK_OAM_1fd031, ADDR_OAM_1fd031);
	set_frameset(gb, ADDR_Frameset_1fd439);
	anim(gb, OAM2);
}

PORT(SetState_VampireIdling)
{
	st(gb, wWarioState, WST_VAMPIRE_IDLING);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioVampireGfx, ADDR_WarioVampireGfx);
	wr8(gb, wWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_1fd498 >> 8));
	st(gb, wWarioOAMPtr + 1, (uint8_t)ADDR_OAM_1fd498);
	set_frameset(gb, ADDR_Frameset_1fd81c);
	anim(gb, OAM2);
}

PORT(SetState_FlatIdling)
{
	st(gb, wWarioState, WST_FLAT_IDLING);
	static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
		wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	flat_box(gb);
	set_frameset(gb, ADDR_Frameset_1718b);
	anim(gb, OAM1);
}

PORT(SetState_FlatSinking)
{
	st(gb, wWarioState, WST_FLAT_SINKING);
	static const uint16_t zero[] = {wWalkVelIndex, wWarioStateCounter, wWarioStateCycles,
		wJumpVelIndex, wJumpVelTable, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	flat_box(gb);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1719a);
	anim(gb, OAM1);
}

// hPos = wWarioPos + (-ysub, dx) through hl/de walking down:
// ld hl, wWarioXPos + 1 / ld de, hXPosLo / ld a, [hld] / add (or sub) / ...
static void pos_offset(GB *gb, bool add, uint8_t dx, uint8_t ysub)
{
	if (add) {
		gb->cpu.a = alu_add(gb, rd8(gb, wWarioXPos + 1), dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, wWarioXPos + 0), 0x00);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos + 1), dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos + 0), 0x00);
	}
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos - 1), ysub);
	wr8(gb, hXPosLo - 2, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos - 2), 0x00);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioXPos - 2);
	SET_DE(gb, hXPosLo - 3);
}

PORT(Func_289c5)
{
	pos_offset(gb, false, 0x04, 0x08);
	gb->cpu.b = PARTICLE_BUBBLES;
	FARCALL(CreateParticle);
	st(gb, wWarioState, WST_FLAT_STRETCHING_UNDERWATER);
	static const uint16_t zero[] = {wWalkVelIndex, wWarioStateCounter, wWarioStateCycles,
		wJumpVelIndex, wJumpVelTable, 0};
	clear(gb, zero);
	flat_box(gb);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_171a3);
	anim(gb, OAM1);
}

PORT(Func_2abc1)
{
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fd42c, ADDR_Frameset_1fd434);
	anim(gb, OAM2);
}

PORT(SetState_ZombieWrithing)
{
	st(gb, wWarioState, WST_ZOMBIE_WRITHING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fc882, ADDR_Frameset_1fc88b);
	anim(gb, OAM2);
}

PORT_JUMP(UpdateState_ZombieKnockBack)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_ZombieIdling);
}

PORT_JUMP(UpdateState_ZombieLanding)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	gb->cpu.a = alu_dec(gb, rd8(gb, wWarioStateCounter));
	if (!ZF) {
		LJUMP(SetState_ZombieIdling);
		return;
	}
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	st(gb, wWarioStateCycles, 0x06);
	st(gb, wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fc928, ADDR_Frameset_1fc92b);
	anim(gb, OAM2);
	RET();
}

PORT_JUMP(Func_2a2e7)
{
	touch_states(gb, TOUCH_PASS_THROUGH, 0x02);
	SET_HL(gb, ADDR_WarioBouncyPal);
	CALL(SetWarioPal);
	frameset_by_dir(gb, ADDR_Frameset_1fccbf, ADDR_Frameset_1fccb2);
	reset_anim(gb);
	LJUMP(SetState_BouncyLastBounce);
}

PORT_JUMP(UpdateState_BouncyFloor)
{
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = alu_or(gb, rd8(gb, wTransformationDuration), rd8(gb, wTransformationDuration + 1));
	if (ZF) {
		LJUMP(Func_2a2e7);
		return;
	}
	LCALL(Func_2ae3b);
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	if (ld_bit(gb, wJoypadDown, B_PAD_A))
		LJUMP(Func_2a0bb);
	else
		LJUMP(Func_2a0c2);
}

PORT(Func_2926a)
{
	get_pos(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.b = PARTICLE_SPLASH;
	FARCALL(CreateParticle);
	play_sfx(gb, SFX_SPLASH);
	st(gb, wWarioState, WST_FAT_SINKING);
	static const uint16_t zero[] = {wJumpVelTable, wJumpVelIndex, wIsSmashAttacking,
		wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	frameset_by_dir(gb, ADDR_Frameset_17872, ADDR_Frameset_17887);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_FatLanding)
{
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = rd8(gb, wTransformationDuration);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!CF) {
		LJUMP(SetState_FatRecovering);
		return;
	}
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, wTransformationDuration + 1));
	if (ZF) {
		LJUMP(SetState_FatRecovering);
		return;
	}
	anim(gb, OAM1);
	if (ld_and(gb, wGroundShakeCounter)) {
		RET();
		return;
	}
	LJUMP(SetState_FatIdling);
}

PORT(SetState_FlatWalking)
{
	st(gb, wWarioState, WST_FLAT_WALKING);
	static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wFrameDuration,
		wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
		wJumpVelIndex, 0};
	clear(gb, zero);
	set_frameset(gb, ADDR_Frameset_17175);
	anim(gb, OAM1);
	if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT))
		st(gb, wDirection, DIRECTION_RIGHT);
	else
		st(gb, wDirection, DIRECTION_LEFT);
}

PORT(SetState_FlatFalling)
{
	st(gb, wWarioState, WST_FLAT_FALLING);
	static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
		wWarioStateCounter, wWarioStateCycles, wWarioSlopeInteraction, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	frameset_by_dir(gb, ADDR_Frameset_171b8, ADDR_Frameset_17186);
	anim(gb, OAM1);
}

PORT(Func_29317)
{
	if (facing_right(gb))
		pos_offset(gb, true, 0x04, 0x10);
	else
		pos_offset(gb, false, 0x04, 0x10);
	gb->cpu.b = PARTICLE_BUBBLES;
	FARCALL(CreateParticle);
}

PORT(SetState_TurningInvisible)
{
	play_sfx(gb, SFX_03F);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	st(gb, wWarioState, WST_TURNING_INVISIBLE);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero2[] = {wTouchState, wStingTouchState, wca94, 0};
	clear(gb, zero2);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
	set_oam(gb, BANK_OAM_17cf7, ADDR_OAM_17cf7);
	frameset_by_dir(gb, ADDR_Frameset_17e1b, ADDR_Frameset_17e3a);
	anim(gb, OAM1);
}

PORT(SetState_CrazyTurning)
{
	st(gb, wWarioState, WST_CRAZY_TURNING);
	reset_anim(gb);
	set_frameset(gb, flip_dir_and(gb) ? ADDR_Frameset_1fcfd7 : ADDR_Frameset_1fcfd0);
	anim(gb, OAM2);
}

PORT(SetState_VampireTurning)
{
	st(gb, wWarioState, WST_VAMPIRE_TURNING);
	reset_anim(gb);
	set_frameset(gb, flip_dir_and(gb) ? ADDR_Frameset_1fd83a : ADDR_Frameset_1fd841);
	anim(gb, OAM2);
}

PORT(SetState_CrazyAirborne)
{
	st(gb, wWarioState, WST_CRAZY_AIRBORNE);
	st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
	static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_1fcf9f, ADDR_Frameset_1fcf6e);
	anim(gb, OAM2);
}

PORT(SetState_PuffyTurning)
{
	flip_dir(gb);
	st(gb, wWarioState, WST_PUFFY_TURNING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1dcce3, ADDR_Frameset_1dccd8);
	anim(gb, ld(gb, wWarioOAMBank));
}

PORT(SetState_BatIdling)
{
	st(gb, wWarioState, WST_BAT_IDLING);
	static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelIndex, wJumpVelTable, 0};
	clear(gb, zero);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1fd45f, ADDR_Frameset_1fd46c);
	anim(gb, OAM2);
}

PORT(SetState_PuffyDeflating)
{
	play_sfx(gb, SFX_027);
	st(gb, wWarioState, WST_PUFFY_DEFLATING);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wFrameDuration,
		wAnimationFrame, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	frameset_by_dir(gb, ADDR_Frameset_1dccee, ADDR_Frameset_1dcd1e);
	anim(gb, ld(gb, wWarioOAMBank));
}

PORT(SetState_PuffyRising)
{
	st(gb, wWarioState, WST_PUFFY_RISING);
	static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	frameset_by_dir(gb, ADDR_Frameset_1dcd4e, ADDR_Frameset_1dcd57);
	anim(gb, ld(gb, wWarioOAMBank));
}

PORT(SetState_BatTransforming)
{
	play_sfx(gb, SFX_046);
	st(gb, wWarioState, WST_BAT_TRANSFORMING);
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioBatGfx, ADDR_WarioBatGfx);
	wr8(gb, wWarioOAMPtr + 0, (uint8_t)(ADDR_OAM_1fd031 >> 8));
	st(gb, wWarioOAMPtr + 1, (uint8_t)ADDR_OAM_1fd031);
	frameset_by_dir(gb, ADDR_Frameset_1fd40a, ADDR_Frameset_1fd479);
	anim(gb, OAM2);
}

PORT(SetState_VampireWalking)
{
	st(gb, wWarioState, WST_VAMPIRE_WALKING);
	static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
		wWarioStateCounter, wWarioStateCycles, wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	bool right;
	if (ld_bit(gb, wJoypadDown, B_PAD_LEFT))
		right = false;
	else if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		right = true;
	else
		right = facing_right(gb);
	if (right) {
		st(gb, wDirection, DIRECTION_RIGHT);
		set_frameset(gb, ADDR_Frameset_1fd82e);
	} else {
		st(gb, wDirection, DIRECTION_LEFT);
		set_frameset(gb, ADDR_Frameset_1fd825);
	}
	anim(gb, OAM2);
}

PORT_JUMP(Func_2a675)
{
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	LJUMP(SetState_CrazyAirborne);
}

PORT_JUMP(Func_2a67b)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(SetState_CrazyAirborne); // fallthrough
}

PORT_JUMP(Func_2a8a7)
{
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(SetState_VampireAirborne);
}

PORT_JUMP(Func_2a0b2)
{
	if (ld_bit(gb, wJoypadDown, B_PAD_A))
		LJUMP(Func_2a0bb);
	else
		LJUMP(Func_2a0c2);
}

PORT_JUMP(Func_2a8ae)
{
	play_sfx(gb, SFX_01D);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	LJUMP(SetState_VampireAirborne); // fallthrough
}

PORT_JUMP(Func_28380)
{
	center_on_ladder(gb);
	LJUMP(Func_2838a); // fallthrough
}

PORT_JUMP(UpdateState_BatTransforming)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_BatIdling); // fallthrough
}

PORT_JUMP(UpdateState_PuffyInflating)
{
	anim(gb, ld(gb, wWarioOAMBank));
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_PuffyRising); // fallthrough
}

PORT_JUMP(UpdateState_GettingFlat)
{
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	if (ld_and(gb, wWarioStateCounter))
		LJUMP(SetState_FlatSquished);
	else
		LJUMP(SetState_FlatIdling); // fallthrough
}

PORT_JUMP(UpdateState_FatIdling)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	LCALL(Func_28f6d);
	ld_cp(gb, wWarioState, WST_FAT_IDLING);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(Func_290d6);
		return;
	}
	RET();
}

PORT_JUMP(UpdateState_FlatSquishedLifting)
{
	anim(gb, OAM1);
	gb->cpu.b = 0x01;
	CALL(MoveWarioUp);
	inc_counter(gb);
	alu_cp(gb, gb->cpu.a, 0x32);
	if (CF) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	LJUMP(SetState_FlatFalling);
}

PORT_JUMP(Func_2b63b)
{
	if (!ld_bit(gb, wJoypadDown, B_PAD_A)) {
		LJUMP(SetState_BatFalling);
		return;
	}
	SET_HL(gb, wJumpVelIndex);
	gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelIndex));
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX + 1);
	if (!CF) {
		LJUMP(SetState_BatFalling);
		return;
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	FARCALL(Func_1f863);
	RET();
}

PORT_JUMP(Func_2b34e)
{
	if (ld_bit(gb, wJoypadDown, B_PAD_A)) {
		LJUMP(Func_29be1);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, PAD_RIGHT | PAD_LEFT);
	if (!ZF) {
		LJUMP(SetState_ZombieWalking);
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(Func_29bb9);
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioStateCounter), 0x01);
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	SET_HL(gb, wWarioStateCounter + 1);
	gb->cpu.a = alu_adc(gb, rd8(gb, wWarioStateCounter + 1), 0x00);
	wr8(gb, wWarioStateCounter + 1, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (CF) {
		RET();
		return;
	}
	LJUMP(SetState_ZombieWrithing);
}

PORT_JUMP(UpdateState_FatEating)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(Func_2926a);
		return;
	}
	if (ld_and(gb, wJumpVelTable)) {
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (!b_and(gb)) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		xor_a(gb);
		wr8(gb, wJumpVelTable, 0);
		wr8(gb, wJumpVelIndex, 0);
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	st(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
	LJUMP(SetState_FatIdling); // fallthrough
}

PORT_JUMP(UpdateState_FlatSinking)
{
	anim(gb, OAM1);
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b1111);
	if (ZF) {
		// call z, .Func_2899a
		gb_push16(gb, ADDR_UpdateState_FlatSinking + 0x2d);
		pos_offset(gb, false, 0x04, 0x08);
		gb->cpu.b = PARTICLE_BUBBLES;
		FARCALL(CreateParticle);
		gb_pop16(gb);
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b1);
	if (!ZF) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	CALL(MoveWarioDown);
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	LJUMP(Func_289c5);
}

PORT_JUMP(Func_2ad6a)
{
	CALL(ClearTransformationValues);
	st(gb, wInvincibleCounter, 0x10);
	CALL(UpdateLevelMusic);
	FARCALL(SetState_Submerged);
	reset_anim(gb);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	set_oam(gb, BANK_OAM_15254, ADDR_OAM_15254);
	frameset_by_dir(gb, ADDR_Frameset_155a6, ADDR_Frameset_155ab);
	anim(gb, OAM1);
	JUMP(PlayRecoverySFX);
}

PORT(SetState_Crazy)
{
	play_sfx(gb, SFX_043);
	st(gb, wWarioState, WST_CRAZY);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_VULNERABLE);
	st(gb, wca94, 0x01);
	static const uint16_t zero[] = {wWalkVelIndex, wWarioStateCounter, wWarioStateCycles,
		wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioCrazyGfx, ADDR_WarioCrazyGfx);
	set_oam(gb, BANK_OAM_1fccd5, ADDR_OAM_1fccd5);
	frameset_by_dir(gb, ADDR_Frameset_1fcf9f, ADDR_Frameset_1fcf6e);
	anim(gb, OAM2);
}

PORT(SetState_CrazySpinning)
{
	play_sfx(gb, SFX_042);
	st(gb, wWarioState, WST_CRAZY_SPINNING);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0};
	clear(gb, zero);
	if (ld_and(gb, wJumpVelTable))
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	gb->cpu.a = rd8(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_14ce1 : ADDR_Frameset_14ccc);
	anim(gb, OAM1);
}

PORT(SetState_PuffyInflating)
{
	st(gb, wTransformation, TRANSFORMATION_PUFFY_WARIO);
	st(gb, wStingTouchState, TOUCH_VULNERABLE);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wca94, 0x01);
	play_sfx(gb, SFX_025);
	st(gb, wWarioState, WST_PUFFY_INFLATING);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
		wIsSmashAttacking, wSwimVelIndex, wWaterSurfaceFloatingCounter, wWaterCurrent,
		wLastWaterCurrent, wCurWaterCurrent, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioPuffyGfx, ADDR_WarioPuffyGfx);
	set_oam(gb, BANK_OAM_1dc93f, ADDR_OAM_1dc93f);
	frameset_by_dir(gb, ADDR_Frameset_1dccc0, ADDR_Frameset_1dccc9);
	anim(gb, rd8(gb, wWarioOAMBank));
}

PORT_JUMP(UpdateState_GettingFlatAirborne)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(SetState_FlatSinking);
		return;
	}
	gb->cpu.b = 0x02;
	CALL(MoveWarioDown);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	st(gb, wWarioState, WST_GETTING_FLAT);
	static const uint16_t zero[] = {wWalkVelIndex, wFrameDuration, wAnimationFrame,
		wWarioStateCycles, wJumpVelIndex, wJumpVelTable, wAutoMoveState, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	collision_box(gb, -1, -8, -11, 11);
	set_frameset(gb, ADDR_Frameset_1715f);
	anim(gb, OAM1);
	RET();
}

PORT(SetState_FlatAirborne)
{
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		return;
	ld_cp(gb, wTransformation, TRANSFORMATION_OWL_WARIO);
	if (ZF)
		CALL(ReleaseOwl);
	// push_wram BANK("Level Objects WRAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	st(gb, rWBK, BANK_wObjects);
	uint16_t hl = (uint16_t)((rd8(gb, wObjPtr + 0) << 8) | rd8(gb, wObjPtr + 1));
	SET_DE(gb, OBJ_X_POS);
	hl = alu_add16(gb, hl, OBJ_X_POS);
	gb->cpu.b = 0;
	gb->cpu.a = alu_and(gb, rd8(gb, wObjInteractionType), INTERACTION_MASK);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, OBJ_INTERACTION_RICHTERTOFFEN);
	if (ZF) {
		wr8(gb, wWarioStateCounter, gb->cpu.a);
	} else {
		st(gb, wWarioStateCounter, 0x01);
		gb->cpu.b = 0x8;
	}
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), gb->cpu.b);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
	hl++;
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl), 0x00);
	wr8(gb, wWarioXPos + 0, gb->cpu.a);
	SET_HL(gb, hl);
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
	st(gb, wTransformation, TRANSFORMATION_FLAT_WARIO);
	play_sfx(gb, SFX_01B);
	CALL(UpdateLevelMusic);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_BUMP);
	st(gb, wca94, 0x02);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCycles,
		wGrabState, wAttackCounter, wJumpVelIndex, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wAutoMoveState, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	st(gb, wWarioState, WST_GETTING_FLAT_AIRBORNE);
	collision_box(gb, -1, -8, -11, 11);
	load_gfx(gb, BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
	set_oam(gb, BANK_OAM_16e9d, ADDR_OAM_16e9d);
	set_frameset(gb, ADDR_Frameset_171bd);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_FatBumping)
{
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_FatIdling);
}

PORT_JUMP(UpdateState_ZombieIdling)
{
	FARCALL(Func_19b25);
	ld_cp(gb, wWarioState, WST_ZOMBIE_IDLING);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	ld_cp(gb, wWarioState, WST_ZOMBIE_IDLING);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(Func_29bb9);
		return;
	}
	update_pos_y(gb);
	anim(gb, OAM2);
	LCALL(Func_2b34e);
	RET();
}

// farcall f / ld a, b / and a; true if nz
static bool farcall_b(GB *gb, uint8_t bank, uint16_t addr)
{
	port_farcall(gb, bank, addr);
	return b_and(gb);
}

PORT(Func_2b1cc)
{
	if (!facing_right(gb)) {
		gb->cpu.b = 2;
		CALL(MoveWarioLeft);
		if (farcall_b(gb, BANK_Func_197b1, ADDR_Func_197b1)) {
			gb->cpu.b = 2;
			CALL(MoveWarioRight);
		}
	} else {
		gb->cpu.b = 2;
		CALL(MoveWarioRight);
		if (farcall_b(gb, BANK_Func_19741, ADDR_Func_19741)) {
			gb->cpu.b = 2;
			CALL(MoveWarioLeft);
		}
	}
	gb->cpu.a = rd8(gb, wJumpVelIndex);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
	SET_HL(gb, hl);
	uint8_t v = rd8(gb, hl);
	if (alu_bit(gb, v, 7)) {
		gb->cpu.a = alu_inc(gb, alu_cpl(gb, rd8(gb, hl)));
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
}

PORT_JUMP(UpdateState_BallOString)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	sfx_loop(gb, 12, SFX_ROLL);
	anim(gb, OAM1);
	if (!facing_right(gb)) {
		CALL(ApplyWalkVelocity_Left);
		CALL(MoveWarioLeft);
	} else {
		CALL(ApplyWalkVelocity_Right);
		CALL(MoveWarioRight);
	}
	ld_cp(gb, wWalkVelIndex, 0x18);
	if (!CF)
		st(gb, wWalkVelIndex, 0x14);
	FARCALL(CheckFrontCollision);
	gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
	if (gb->cpu.a) {
		LJUMP(Func_28bd5);
		return;
	}
	if (farcall_b(gb, BANK_CheckCentreCollision, ADDR_CheckCentreCollision) == false) {
		LJUMP(Func_28c15);
		return;
	}
	update_pos_y(gb);
	if (!ld_and(gb, wWarioSlopeInteraction)) {
		RET();
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), gb->cpu.b);
	if (gb->cpu.a) {
		LJUMP(Func_28bd5);
		return;
	}
	RET();
}

// turns to dir with the new frameset if facing the other way, then
// farcall CheckFrontCollision / ret nz / ld b, 1 / move
PORT(HandleHotWarioInput)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_RIGHT | PAD_LEFT);
	if (!gb->cpu.a)
		return;
	gb->cpu.a = alu_and(gb, gb->cpu.a, PAD_RIGHT);
	bool right = gb->cpu.a != 0;
	bool turn = facing_right(gb) != right;
	if (turn) {
		st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		reset_anim(gb);
		set_frameset(gb, right ? ADDR_Frameset_16b2f : ADDR_Frameset_16b26);
		anim(gb, OAM1);
	}
	if (farcall_b(gb, BANK_CheckFrontCollision, ADDR_CheckFrontCollision))
		return;
	gb->cpu.b = 1;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
}

PORT(SetState_ZombieLanding)
{
	play_sfx(gb, SFX_02A);
	touch_states(gb, TOUCH_BUMP, 0x02);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	st(gb, wWarioState, WST_ZOMBIE_LANDING);
	static const uint16_t zero[] = {wWarioStateCounter, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	FARCALL(Func_19a77);
	if (ld_and(gb, wc1ca)) {
		st(gb, wWarioStateCounter, 0x01);
		frameset_by_dir(gb, ADDR_Frameset_1fc8bc, ADDR_Frameset_1fc8d5);
	} else {
		frameset_by_dir(gb, ADDR_Frameset_1fc948, ADDR_Frameset_1fc953);
	}
	anim(gb, OAM2);
}

PORT(SetState_BouncyStart)
{
	st(gb, wWarioState, WST_BOUNCY_START);
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	st(gb, wJumpVelTable, 0x02);
	collision_box(gb, -1, -29, -9, 9);
	static const uint16_t zero[] = {wInvisibleFrame, wWalkVelIndex, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioBouncyPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioBouncyGfx, ADDR_WarioBouncyGfx);
	set_oam(gb, BANK_OAM_1fc95e, ADDR_OAM_1fc95e);
	set_frameset(gb, ADDR_Frameset_1fcc6c);
	anim(gb, OAM2);
	if (!farcall_b(gb, BANK_Func_197b1, ADDR_Func_197b1))
		return;
	gb->cpu.b = 2;
	CALL(MoveWarioRight);
}

PORT(UpdateState_TurningInvisible)
{
	anim(gb, OAM1);
	if (!anim_ended(gb))
		return;
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	st(gb, wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
	CALL(UpdateLevelMusic);
	FARCALL(SetState_Idling);
}

PORT(Func_2b17a)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		FARCALL(WalkRight);
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		FARCALL(WalkLeft);
}

PORT(HandleWalk)
{
	if (facing_right(gb))
		FARCALL(WalkRight);
	else
		FARCALL(WalkLeft);
}

// ld a, [wWalkVelIndex] / cp max / jr c / ld a, v / ld [wWalkVelIndex], a
static void cap_walk_vel(GB *gb, uint8_t max, uint8_t v)
{
	ld_cp(gb, wWalkVelIndex, max);
	if (!CF) {
		gb->cpu.a = v;
		wr8(gb, wWalkVelIndex, v);
	}
}

PORT_JUMP(Func_2b239)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(Func_290a1);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		if (!ld_and(gb, wDirection)) {
			st(gb, wDirection, DIRECTION_RIGHT);
			LJUMP(SetState_FatTurning);
			return;
		}
		FARCALL(WalkIfPossible_Right);
		cap_walk_vel(gb, 0x08, 0x04);
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		ld_cp(gb, wDirection, 0x00);
		if (!ZF) {
			st(gb, wDirection, DIRECTION_LEFT);
			LJUMP(SetState_FatTurning);
			return;
		}
		FARCALL(WalkIfPossible_Left);
		cap_walk_vel(gb, 0x08, 0x04);
		RET();
		return;
	}
	LJUMP(SetState_FatIdling);
}

PORT_JUMP(Func_2b381)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(Func_29be1);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		if (!ld_and(gb, wDirection)) {
			st(gb, wDirection, DIRECTION_RIGHT);
			LJUMP(SetState_ZombieTurning);
			return;
		}
		FARCALL(WalkIfPossible_Right);
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		if (ld_and(gb, wDirection)) {
			st(gb, wDirection, DIRECTION_LEFT);
			LJUMP(SetState_ZombieTurning);
			return;
		}
		FARCALL(WalkIfPossible_Left);
	} else {
		LJUMP(SetState_ZombieIdling);
		return;
	}
	cap_walk_vel(gb, 0x04, 0x00);
	RET();
}

PORT_JUMP(Func_2ae8a)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			LJUMP(Func_2af81);
			return;
		}
	}
	bool right = facing_right(gb);
	FARCALL(CheckFrontCollision);
	if (!b_and(gb)) {
		if (right) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		cap_walk_vel(gb, 0x14, 0x10);
		RET();
		return;
	}
	// bumped into a wall: turn around
	gb->cpu.b = 0x02;
	if (right) {
		CALL(MoveWarioLeft);
		st(gb, wDirection, DIRECTION_LEFT);
		set_frameset(gb, ld_and(gb, wWarioStateCounter) ? ADDR_Frameset_16b14 : ADDR_Frameset_16b38);
	} else {
		CALL(MoveWarioRight);
		st(gb, wDirection, DIRECTION_RIGHT);
		set_frameset(gb, ld_and(gb, wWarioStateCounter) ? ADDR_Frameset_16b1d : ADDR_Frameset_16b49);
	}
	anim(gb, OAM1);
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	FARCALL(CheckFrontCollision);
	if (b_and(gb)) {
		LJUMP(SetState_Hot_ResetDuration);
		return;
	}
	RET();
}

PORT_JUMP(UpdateState_Hot)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	sfx_loop(gb, 16, SFX_01F);
	anim(gb, OAM1);
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = rd8(gb, wTransformationDuration);
	alu_cp(gb, gb->cpu.a, 0);
	if (ZF) {
		uint16_t pal = 0;
		gb->cpu.a = rd8(gb, wTransformationDuration + 1);
		alu_cp(gb, gb->cpu.a, 224);
		if (ZF) {
			pal = ADDR_WarioHot1Pal;
		} else if (alu_cp(gb, gb->cpu.a, 164), ZF) {
			pal = ADDR_WarioHot2Pal;
		} else if (alu_cp(gb, gb->cpu.a, 60), ZF) {
			pal = ADDR_WarioHot3Pal;
		} else if (alu_cp(gb, gb->cpu.a, 1), ZF) {
			// .burnt
			play_sfx(gb, SFX_022);
			st(gb, wWarioState, WST_BURNT);
			xor_a(gb);
			wr8(gb, wFrameDuration, 0);
			wr8(gb, wAnimationFrame, 0);
			wr8(gb, wWarioStateCounter, 0);
			wr8(gb, wWarioStateCycles, 0);
			SET_HL(gb, ADDR_WarioOnFirePal);
			CALL(SetWarioPal);
			load_gfx(gb, BANK_WarioBurntGfx, ADDR_WarioBurntGfx);
			set_oam(gb, BANK_OAM_16b6c, ADDR_OAM_16b6c);
			frameset_by_dir(gb, ADDR_Frameset_16e47, ADDR_Frameset_16e72);
			anim(gb, OAM1);
			RET();
			return;
		}
		if (pal) {
			SET_HL(gb, pal);
			CALL(SetWarioPal);
		}
	}
	LCALL(HandleHotWarioInput);
	ld_cp(gb, wWarioState, WST_HOT);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(Func_28380);
		return;
	}
	update_pos_y(gb);
	RET();
}

PORT_JUMP(SetState_BatFlying)
{
	st(gb, wWarioState, WST_BAT_FLYING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wJumpVelIndex, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	LJUMP(Func_2abc1);
}

PORT_JUMP(SetState_FlatJumping)
{
	play_sfx(gb, SFX_01D);
	st(gb, wWarioState, WST_FLAT_JUMPING);
	static const uint16_t clr[] = {
		wWalkVelIndex, wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wJumpVelIndex, 0,
	};
	clear(gb, clr);
	st(gb, wJumpVelTable, ld_bit(gb, wJoypadDown, B_PAD_UP) ? JUMP_VEL_NORMAL : JUMP_VEL_KNOCK_BACK);
	set_frameset(gb, ADDR_Frameset_17190);
	anim(gb, OAM1);
	if (!ld_and(gb, wIsOnSteppableObject)) {
		RET();
		return;
	}
	LJUMP(UpdateState_FlatJumping); // fallthrough
}

// moves Wario b pixels forward unless a wall is in front, in which case
// he turns (the .asm_2b45d/.asm_2b495 paths of Func_2b42b)
static void crazy_move(GB *gb, uint8_t b)
{
	bool right = facing_right(gb);
	if (farcall_b(gb, right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1)) {
		LJUMP(SetState_CrazyTurning);
		return;
	}
	gb->cpu.b = b;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	RET();
}

PORT_JUMP(Func_2b42b)
{
	SET_HL(gb, wWalkVelIndex);
	gb->cpu.a = alu_inc(gb, rd8(gb, wWalkVelIndex));
	wr8(gb, wWalkVelIndex, gb->cpu.a);
	uint8_t a = gb->cpu.a;
	alu_cp(gb, a, 0x06);
	if (CF) { crazy_move(gb, 2); return; }
	if (ZF) { LJUMP(Func_2b4d0); return; }
	alu_cp(gb, a, 0x09);
	if (CF) { RET(); return; }
	alu_cp(gb, a, 0x12);
	if (CF) { crazy_move(gb, 2); return; }
	if (ZF) { LJUMP(Func_2b4d0); return; }
	alu_cp(gb, a, 0x15);
	if (CF) { RET(); return; }
	alu_cp(gb, a, 0x18);
	if (CF) { crazy_move(gb, 2); return; }
	alu_cp(gb, a, 0x22);
	if (CF) { crazy_move(gb, 1); return; }
	if (ZF) { LJUMP(Func_2b4d0); return; }
	alu_cp(gb, a, 0x25);
	if (CF) { RET(); return; }
	alu_cp(gb, a, 0x2c);
	if (!CF) { RET(); return; }
	if (!ld_mask(gb, wGlobalCounter, 1)) {
		RET();
		return;
	}
	crazy_move(gb, 1);
}

PORT_JUMP(Func_2b4f3)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(SetState_BatTransforming);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(Func_2a8ae);
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_VampireWalking);
		return;
	}
	RET();
}

PORT_JUMP(Func_2b509)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		LJUMP(SetState_BatTransforming);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		LJUMP(Func_2a8ae);
		return;
	}
	bool right;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		right = true;
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		right = false;
	else {
		LJUMP(SetState_VampireIdling);
		return;
	}
	if (facing_right(gb) != right) {
		LJUMP(SetState_VampireTurning);
		return;
	}
	if (right)
		FARCALL(WalkIfPossible_Right);
	else
		FARCALL(WalkIfPossible_Left);
	cap_walk_vel(gb, 0x10, 0x0c);
	RET();
}

PORT_JUMP(Func_2b2c2)
{
	LCALL(Func_2b342);
	FARCALL(CheckUpCollision);
	ld_cp(gb, wWarioState, WST_PUFFY_RISING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb)) {
		LJUMP(SetState_PuffyDeflating);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	if (!CF) {
		CALL(GetFloorForYPos);
		gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
		if (CF)
			CALL(StartUpwardsFloorTransition);
	}
	bool right = facing_right(gb);
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
		LJUMP(SetState_PuffyTurning);
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, right ? B_PAD_RIGHT : B_PAD_LEFT)) {
		RET();
		return;
	}
	if (farcall_b(gb, right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1)) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	RET();
}

// UpdateWarioAnimation in OAM bank `bank`, then jp `to` once the animation
// has ended
static void anim_then(GB *gb, uint8_t bank, uint16_t to)
{
	anim(gb, bank);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	JUMP_TO(to);
}

PORT_JUMP(UpdateState_CrazyDizzy) { anim_then(gb, OAM1, ADDR_SetState_Crazy); }
PORT_JUMP(UpdateState_VampireTurning) { anim_then(gb, OAM2, ADDR_SetState_VampireWalking); }
PORT_JUMP(UpdateState_VampireTransforming) { anim_then(gb, OAM2, ADDR_SetState_VampireIdling); }
PORT_JUMP(UpdateState_FlatStretchingUnderwater) { anim_then(gb, OAM1, ADDR_Func_2ad6a); }

PORT_JUMP(UpdateState_CrazyTurning)
{
	anim(gb, OAM2);
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(Func_2a675);
		return;
	}
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_Crazy);
}

PORT(UpdateState_FatSinking)
{
	anim(gb, OAM1);
	if (!ld_mask(gb, wGlobalCounter, 0x0f))
		LCALL(Func_29317);
	gb->cpu.b = 0x01;
	CALL(MoveWarioDown);
	FARCALL(CheckCentreCollision);
	if (!b_and(gb))
		return;
	LCALL(Func_2ad6a);
}

PORT_JUMP(UpdateState_PuffyTurning)
{
	gb->cpu.a = rd8(gb, wWarioOAMBank);
	anim(gb, gb->cpu.a);
	LCALL(Func_2b342);
	FARCALL(CheckUpCollision);
	ld_cp(gb, wWarioState, WST_PUFFY_TURNING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb)) {
		LJUMP(SetState_PuffyDeflating);
		return;
	}
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_PuffyRising);
}

PORT_JUMP(Func_2af9c)
{
	CALL(ApplyJumpVelocity);
	LCALL(HandleWalk);
	cap_walk_vel(gb, 0x14, 0x10);
	FARCALL(CheckFrontCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	LJUMP(Func_2afc5); // fallthrough
}

PORT(Func_2b2a4)
{
	CALL(ApplyJumpVelocity);
	LCALL(Func_2b17a);
	cap_walk_vel(gb, 0x08, 0x04);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (!ZF)
		return;
	st(gb, wIsSmashAttacking, TRUE);
}

PORT(Func_2b3dd)
{
	CALL(ApplyJumpVelocity);
	SET_HL(gb, wWarioStateCycles);
	if (ld_and(gb, wWarioStateCycles)) {
		wr8(gb, wWarioStateCycles, alu_dec(gb, gb->cpu.a));
		return;
	}
	LCALL(Func_2b17a);
	cap_walk_vel(gb, 0x04, 0x00);
}

// ld a, [wWarioState] / cp state: true if still in that state
static bool still_in(GB *gb, uint8_t state)
{
	ld_cp(gb, wWarioState, state);
	return ZF;
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

// farcall CheckCentreCollision / ld a, b / and a / jp z, fall / update_pos_y / ret
static void ground_check(GB *gb, uint16_t fall)
{
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		JUMP_TO(fall);
		return;
	}
	update_pos_y(gb);
	RET();
}

PORT_JUMP(UpdateState_OnFire)
{
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 14, SFX_01E);
	if (!ld_and(gb, wWarioStateCounter) && anim_ended(gb)) {
		st(gb, wWarioStateCounter, 0x01);
		frameset_by_dir(gb, ADDR_Frameset_16b14, ADDR_Frameset_16b1d);
	}
	anim(gb, OAM1);
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	ld_cp(gb, wTransformationDuration, (uint8_t)(HOT_WARIO_TRANSITION_DURATION >> 8));
	if (ZF) {
		ld_cp(gb, wTransformationDuration + 1, (uint8_t)HOT_WARIO_TRANSITION_DURATION);
		if (ZF) {
			LJUMP(SetState_Hot);
			return;
		}
	}
	LCALL(Func_2ae8a);
	if (!still_in(gb, WST_ON_FIRE)) {
		RET();
		return;
	}
	ground_check(gb, ADDR_Func_2af75);
}

PORT_JUMP(UpdateState_FatWalking)
{
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 28, SFX_FAT_WALK);
	anim(gb, OAM1);
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = rd8(gb, wTransformationDuration);
	alu_cp(gb, gb->cpu.a, (uint8_t)((FAT_WARIO_DURATION >> 8) + 1)); // in case it underflowed
	if (!CF) {
		LJUMP(SetState_FatRecovering);
		return;
	}
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, wTransformationDuration + 1));
	if (ZF) {
		LJUMP(SetState_FatRecovering);
		return;
	}
	LCALL(Func_2b239);
	if (!still_in(gb, WST_FAT_WALKING)) {
		RET();
		return;
	}
	ground_check(gb, ADDR_Func_290d6);
}

PORT_JUMP(UpdateState_ZombieWalking)
{
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 36, SFX_ZOMBIE_WALK);
	anim(gb, OAM2);
	LCALL(Func_2b381);
	if (!still_in(gb, WST_ZOMBIE_WALKING)) {
		RET();
		return;
	}
	ground_check(gb, ADDR_Func_29bb9);
}

PORT_JUMP(UpdateState_FlatJumping)
{
	if (room_transition_check(gb))
		return;
	CALL(ApplyJumpVelocity);
	LCALL(Func_2b17a);
	CALL(Func_1762);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
	}
	LJUMP(SetState_FlatFalling);
}

PORT_JUMP(UpdateState_FlatFalling)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(SetState_FlatSinking);
		return;
	}
	if (!anim_ended(gb)) {
		anim(gb, OAM1);
		if (anim_ended(gb)) {
			reset_anim(gb);
			set_frameset(gb, ADDR_Frameset_17190);
			anim(gb, OAM1);
			st(gb, wWarioAnimationEnded, 0x01);
		}
	}
	// .asm_287ff
	SET_HL(gb, wWalkVelIndex);
	gb->cpu.e = rd8(gb, wWalkVelIndex);
	gb->cpu.d = 0x00;
	wr8(gb, wWalkVelIndex, alu_inc(gb, gb->cpu.e));
	gb_push16(gb, REG_DE(gb));
	bool right = facing_right(gb);
	uint16_t hl = alu_add16(gb, ADDR_Data_2b68c, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.b = rd8(gb, hl);
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);

	// .asm_28816
	ld_cp(gb, wWalkVelIndex, 0x28);
	if (CF) {
		FARCALL(CheckFrontCollision);
		if (!b_and(gb)) {
			// .asm_288b8
			SET_DE(gb, gb_pop16(gb));
			hl = alu_add16(gb, ADDR_Data_2b664, REG_DE(gb));
			SET_HL(gb, hl);
			gb->cpu.b = rd8(gb, hl);
			CALL(MoveWarioDown);
			FARCALL(CheckAirborneCollision);
			if (!b_and(gb)) {
				JUMP(TriggerDownwardsFloorTransition);
				return;
			}
			play_sfx(gb, SFX_020);
			CALL(TriggerFloorTransition);
			LJUMP(SetState_FlatIdling);
			return;
		}
		play_sfx(gb, SFX_BUMP);
		flip_dir(gb);
		gb->cpu.b = 0x03;
		move_by_dir(gb);
		gb->cpu.a = 0x00;
	} else {
		// .asm_28857
		bool held;
		if (facing_right(gb))
			held = ld_bit(gb, wJoypadDown, B_PAD_RIGHT);
		else
			held = ld_bit(gb, wJoypadDown, B_PAD_LEFT);
		if (!held) {
			// .asm_2886d
			xor_a(gb);
			wr8(gb, wFrameDuration, 0);
			wr8(gb, wAnimationFrame, 0);
			wr8(gb, wWarioAnimationEnded, 0);
			set_frameset(gb, flip_dir_and(gb) ? ADDR_Frameset_17186 : ADDR_Frameset_171b8);
			anim(gb, OAM1);
		}
		xor_a(gb);
	}
	// .asm_288a8
	wr8(gb, wWalkVelIndex, gb->cpu.a);
	SET_DE(gb, gb_pop16(gb));
	RET();
}

PORT_JUMP(UpdateState_PuffyRising)
{
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 32, SFX_PUFF_RAISE);
	anim(gb, ld(gb, wWarioOAMBank));
	LCALL(Func_2b2c2);
	RET();
}

PORT_JUMP(UpdateState_VampireIdling)
{
	FARCALL(CheckCentreCollision);
	if (!still_in(gb, WST_VAMPIRE_IDLING)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(Func_2a8a7);
		return;
	}
	anim(gb, OAM2);
	LCALL(Func_2b4f3);
	RET();
}

PORT_JUMP(UpdateState_VampireWalking)
{
	if (room_transition_check(gb))
		return;
	if (!still_in(gb, WST_VAMPIRE_WALKING)) {
		RET();
		return;
	}
	anim(gb, OAM2);
	LCALL(Func_2b509);
	if (!still_in(gb, WST_VAMPIRE_WALKING)) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!still_in(gb, WST_VAMPIRE_WALKING) || b_and(gb)) {
		RET();
		return;
	}
	LJUMP(Func_2a8a7);
}

PORT_JUMP(UpdateState_BatIdling)
{
	FARCALL(CheckCentreCollision);
	if (!still_in(gb, WST_BAT_IDLING)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(SetState_BatFalling);
		return;
	}
	sfx_loop(gb, 0x20, SFX_FLAP);
	anim(gb, OAM2);
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
			LJUMP(SetState_BatFlying);
			return;
		}
		RET();
		return;
	}
	// .asm_2aa55
	st(gb, wCollisionBoxTop, (uint8_t)-27);
	FARCALL(CheckUpCollision);
	if (!still_in(gb, WST_BAT_IDLING)) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		LJUMP(SetState_VampireTransforming);
		return;
	}
	st(gb, wCollisionBoxTop, (uint8_t)-15);
	play_sfx(gb, SFX_0E5);
	RET();
}

PORT(Func_2b027)
{
	CALL(ApplyJumpVelocity);
	SET_HL(gb, wWarioStateCounter);
	if (ld_and(gb, wWarioStateCounter)) {
		wr8(gb, wWarioStateCounter, alu_dec(gb, gb->cpu.a));
		return;
	}
	LCALL(Func_2b17a);
	cap_walk_vel(gb, 0x08, 0x04);
	if (!ld_and(gb, wIsTurning))
		return;
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_16b26, ADDR_Frameset_16b2f);
	anim(gb, OAM1);
}

PORT_JUMP(Func_2b10a)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_FlatJumping);
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_FlatWalking);
		return;
	}
	RET();
}

PORT_JUMP(Func_2b11b)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_FlatJumping);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	bool right;
	if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		right = false;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		right = true;
	} else {
		LJUMP(SetState_FlatIdling);
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
	cap_walk_vel(gb, 0x04, 0x00);
	RET();
}

PORT(Func_2b55c)
{
	CALL(ApplyJumpVelocity);
	LCALL(Func_2b17a);
	cap_walk_vel(gb, 0x10, 0x0c);
}

PORT_JUMP(Func_2b3f9)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(Func_2a675);
		return;
	}
	LCALL(Func_2b42b);
	if (!still_in(gb, WST_CRAZY) || !anim_ended(gb)) {
		RET(); // done if not crazy any more
		return;
	}
	bool held;
	if (facing_right(gb))
		held = ld_bit(gb, wJoypadDown, B_PAD_RIGHT);
	else
		held = ld_bit(gb, wJoypadDown, B_PAD_LEFT);
	if (!held) {
		LJUMP(SetState_CrazyTurning);
		return;
	}
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	RET();
}

// ld a, [wWarioStateCounter] / and a / jr nz / ld a, [wWarioAnimationEnded] /
// and a / jr z: true if counter is 0 and the animation ended
static bool first_anim_ended(GB *gb)
{
	return !ld_and(gb, wWarioStateCounter) && anim_ended(gb);
}

PORT_JUMP(UpdateState_OnFireAirborne)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(Func_2ade4);
		return;
	}
	if (first_anim_ended(gb)) {
		st(gb, wWarioStateCounter, 0x01);
		frameset_by_dir(gb, ADDR_Frameset_16b14, ADDR_Frameset_16b1d);
	}
	anim(gb, OAM1);
	LCALL(Func_2af9c);
	if (!still_in(gb, WST_ON_FIRE_AIRBORNE)) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb) || !still_in(gb, WST_ON_FIRE_AIRBORNE)) {
			RET();
			return;
		}
		LJUMP(Func_2af75);
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		if (!still_in(gb, WST_ON_FIRE_AIRBORNE)) {
			RET();
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	if (still_in(gb, WST_ON_FIRE_AIRBORNE))
		FARCALL(SetState_OnFire);
	RET();
}

PORT_JUMP(UpdateState_HotAirborne)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(Func_2ade4);
		return;
	}
	anim(gb, OAM1);
	LCALL(Func_2b027);
	if (!still_in(gb, WST_HOT_AIRBORNE)) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		// rising
		FARCALL(CheckUpCollision);
		if (!b_and(gb) || !still_in(gb, WST_HOT_AIRBORNE)) {
			RET();
			return;
		}
		LJUMP(Func_2838a);
		return;
	}
	// .falling
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		if (!still_in(gb, WST_HOT_AIRBORNE)) {
			RET();
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	if (!still_in(gb, WST_HOT_AIRBORNE)) {
		RET();
		return;
	}
	LJUMP(SetState_Hot);
}

PORT_JUMP(UpdateState_FlatIdling)
{
	anim(gb, OAM1);
	LCALL(Func_2b10a);
	FARCALL(CheckCentreCollision);
	if (b_and(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_FlatFalling);
}

PORT_JUMP(UpdateState_FlatWalking)
{
	if (room_transition_check(gb))
		return;
	sfx_loop(gb, 10, SFX_FLAT_WALK);
	anim(gb, OAM1);
	LCALL(Func_2b11b);
	ground_check(gb, ADDR_SetState_FlatFalling);
}

PORT_JUMP(UpdateState_Crazy)
{
	if (room_transition_check(gb))
		return;
	anim(gb, OAM2);
	LCALL(Func_2b3f9);
	if (!still_in(gb, WST_CRAZY)) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(Func_2a67b);
		return;
	}
	RET();
}

PORT_JUMP(UpdateState_FatAirborne)
{
	if (room_transition_check(gb))
		return;
	if (ld_and(gb, wWaterInteraction)) {
		LJUMP(Func_2926a);
		return;
	}
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = alu_or(gb, rd8(gb, wTransformationDuration), rd8(gb, wTransformationDuration + 1));
	if (ZF)
		wr8(gb, wTransformationDuration + 1, alu_inc(gb, rd8(gb, wTransformationDuration + 1)));
	anim(gb, OAM1);
	if (first_anim_ended(gb)) {
		reset_anim(gb);
		st(gb, wWarioStateCounter, 0x01);
		frameset_by_dir(gb, ADDR_Frameset_1783e, ADDR_Frameset_17841);
		anim(gb, OAM1);
	}
	LCALL(Func_2b2a4);
	if (!still_in(gb, WST_FAT_AIRBORNE)) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb) || !still_in(gb, WST_FAT_AIRBORNE)) {
			RET();
			return;
		}
		LJUMP(Func_290d6);
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	if (!ld_and(gb, wGroundShakeCounter)) {
		st(gb, wGroundShakeCounter, 0x20);
		st(gb, wIsWarioGroundShaking, TRUE);
	}
	if (!ld_and(gb, wFloorTransitionDir))
		play_sfx(gb, SFX_024);
	LJUMP(SetState_FatLanding); // fallthrough
}

// ld a, BANK(oam) / hcall UpdateWarioAnimation / anim ended? jp RecoverFromTransformation
static void anim_then_recover(GB *gb, uint8_t bank)
{
	anim(gb, bank);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	JUMP(RecoverFromTransformation);
}

PORT_JUMP(UpdateState_FatRecovering)
{
	anim_then_recover(gb, OAM1);
}

PORT_JUMP(UpdateState_Burnt)
{
	anim_then_recover(gb, OAM1);
}

PORT_JUMP(UpdateState_BallOStringDizzy)
{
	anim_then_recover(gb, OAM1);
}

PORT_JUMP(UpdateState_ElectricDizzy)
{
	anim_then_recover(gb, OAM1);
}

PORT_JUMP(UpdateState_FlatStretching)
{
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wAutoMoveState, 0);
	JUMP(RecoverFromTransformation);
}

PORT_JUMP(UpdateState_ZombieRecovering)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (gb->cpu.a) {
		RET();
		return;
	}
	wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
	gb->cpu.a = rd8(gb, wWarioStateCounter);
	uint16_t hl = wWarioStateCounter + 1;
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, hl, gb->cpu.a);
	} else {
		alu_cp(gb, gb->cpu.a, 0x0b);
		if (ZF) {
			SET_HL(gb, hl);
			JUMP(RecoverFromTransformation);
			return;
		}
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = rd8(gb, hl);
	}
	for (int i = 0; i < 4; i++)
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	SET_HL(gb, alu_add16(gb, ADDR_WarioZombiePal, REG_DE(gb)));
	CALL(SetWarioPal);
	RET();
}

PORT_JUMP(UpdateState_BallOStringAirborne)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	anim(gb, OAM1);
	FARCALL(CheckFrontCollision);
	gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
	if (gb->cpu.a) {
		LJUMP(Func_28bd5);
		return;
	}
	CALL(ApplyJumpVelocity);
	LCALL(HandleWalk);
	ld_cp(gb, wWalkVelIndex, 0x18);
	if (!CF)
		st(gb, wWalkVelIndex, 0x14);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	LJUMP(SetState_BallOString);
}

PORT_JUMP(UpdateState_BouncyStart)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	CALL(ApplyJumpVelocity);
	LCALL(HandleWalk);
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	ld_cp(gb, wWarioState, WST_BOUNCY_START);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		ld_cp(gb, wWarioState, WST_BOUNCY_START);
		if (!ZF) {
			RET();
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	ld_cp(gb, wWarioState, WST_BOUNCY_START);
	if (!ZF) {
		RET();
		return;
	}
	LJUMP(SetState_BouncyFloor); // fallthrough
}

PORT_JUMP(UpdateState_BouncyUpsideDown)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	CALL(ApplyJumpVelocity);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	ld_cp(gb, wWarioState, WST_BOUNCY_UPSIDE_DOWN);
	if (!ZF) {
		RET();
		return;
	}
	st(gb, wWarioState, WST_BOUNCY_UPSIDE_DOWN_LANDING);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, ADDR_Frameset_1fcc85);
	anim(gb, OAM2);
	RET();
}

PORT_JUMP(UpdateState_BouncyLastBounce)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	if (!anim_ended(gb))
		anim(gb, OAM2);
	CALL(ApplyJumpVelocity);
	LCALL(HandleWalk);
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
		ld_cp(gb, wWarioState, WST_BOUNCY_LAST_BOUNCE);
		if (ZF)
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
	ld_cp(gb, wWarioState, WST_BOUNCY_LAST_BOUNCE);
	if (!ZF) {
		RET();
		return;
	}
	JUMP(RecoverFromTransformation);
}

PORT_JUMP(UpdateState_BallOStringKnockBack)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	anim(gb, OAM1);
	LCALL(Func_2b1cc);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	flip_dir(gb);
	CALL(TriggerFloorTransition);
	play_sfx(gb, SFX_03C);
	st(gb, wWarioState, WST_GETTING_UNWRAPPED_IN_STRING);
	collision_box(gb, -1, -27, -9, 9);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, ADDR_Frameset_17409);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(UpdateState_ZombieSlippingThroughFloor)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	ld_cp(gb, wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	if (!ZF) {
		RET();
		return;
	}
	LCALL(Func_2b3dd);
	anim(gb, OAM2);
	ld_cp(gb, wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		ld_cp(gb, wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
		if (!ZF) {
			RET();
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	ld_cp(gb, wWarioState, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	if (!ZF) {
		RET();
		return;
	}
	LCALL(SetState_ZombieLanding);
	if (ld_and(gb, wDirection))
		set_frameset(gb, ADDR_Frameset_1fc90b);
	else
		set_frameset(gb, ADDR_Frameset_1fc8ee);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioAnimationEnded, 0);
	st(gb, wWarioStateCounter, 0x02);
	anim(gb, OAM2);
	RET();
}

PORT_JUMP(Func_2b56f)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x01);
	if (gb->cpu.a) {
		gb->cpu.b = 0x01;
		CALL(MoveWarioUp);
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
		get_pos(gb);
		gb->cpu.b = PARTICLE_BUBBLE_BURST;
		FARCALL(CreateParticle);
		JUMP(RecoverFromTransformation);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	bool blocked;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		FARCALL(Func_19741);
		blocked = b_and(gb);
		if (!blocked) {
			gb->cpu.b = 0x01;
			CALL(MoveWarioRight);
			goto clear_vel;
		}
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		FARCALL(Func_197b1);
		blocked = b_and(gb);
		if (!blocked) {
			gb->cpu.b = 0x01;
			CALL(MoveWarioLeft);
			goto clear_vel;
		}
	} else {
		st(gb, wDirection, ld(gb, wWarioStateCounter));
		FARCALL(CheckFrontCollision);
		if (!b_and(gb)) {
			gb->cpu.a = rd8(gb, wJumpVelIndex);
			gb->cpu.e = gb->cpu.a;
			gb->cpu.d = 0x00;
			uint16_t hl = alu_add16(gb, ADDR_Data_2b6b4, REG_DE(gb));
			SET_HL(gb, hl);
			gb->cpu.b = rd8(gb, hl);
			if (ld_and(gb, wWarioStateCounter))
				CALL(MoveWarioRight);
			else
				CALL(MoveWarioLeft);
			SET_HL(gb, wJumpVelIndex);
			wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
			gb->cpu.a = rd8(gb, wJumpVelIndex);
			alu_cp(gb, gb->cpu.a, 0x2f);
			if (CF) {
				RET();
				return;
			}
		}
	}
	gb->cpu.a = alu_xor(gb, rd8(gb, wWarioStateCounter), 0x1);
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	wr8(gb, wDirection, gb->cpu.a);
clear_vel:
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	RET();
}

PORT_JUMP(UpdateState_ZombieAirborne)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	ld_cp(gb, wWarioState, WST_ZOMBIE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	LCALL(Func_2b3dd);
	ld_cp(gb, wWarioState, WST_ZOMBIE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	if (!ld_and(gb, wWarioStateCounter) && anim_ended(gb)) {
		xor_a(gb);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		gb->cpu.a = alu_inc(gb, 0);
		wr8(gb, wWarioStateCounter, gb->cpu.a);
		if (ld_and(gb, wDirection))
			set_frameset(gb, ADDR_Frameset_1fc8b3);
		else
			set_frameset(gb, ADDR_Frameset_1fc8b0);
	}
	anim(gb, OAM2);
	ld_cp(gb, wWarioState, WST_ZOMBIE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
		ld_cp(gb, wWarioState, WST_ZOMBIE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		LJUMP(Func_29bb9);
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	ld_cp(gb, wWarioState, WST_ZOMBIE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	LJUMP(SetState_ZombieLanding); // fallthrough
}

PORT_JUMP(UpdateState_BouncyAirborne)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	if (!anim_ended(gb)) {
		anim(gb, OAM2);
		ld_cp(gb, wJumpVelTable, JUMP_VEL_BOUNCY_HIGH_JUMP);
		if (ZF && anim_ended(gb)) {
			get_pos(gb);
			gb->cpu.b = PARTICLE_BOUNCY_JUMP;
			FARCALL(CreateParticle);
		}
	}
	LCALL(DecrementTransformationDuration);
	SET_HL(gb, wTransformationDuration + 1);
	gb->cpu.a = alu_or(gb, rd8(gb, wTransformationDuration), rd8(gb, wTransformationDuration + 1));
	if (ZF) {
		LJUMP(SetState_BouncyUpsideDown);
		return;
	}
	LCALL(Func_2ae3b);
	CALL(ApplyJumpVelocity);
	LCALL(Func_2b17a);
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	ld_cp(gb, wWarioState, WST_BOUNCY_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
		ld_cp(gb, wWarioState, WST_BOUNCY_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(gb, wWarioState, WST_BOUNCY_CEILING);
		xor_a(gb);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		set_frameset(gb, ADDR_Frameset_1fcc75);
		anim(gb, OAM2);
		RET();
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		ld_cp(gb, wWarioState, WST_BOUNCY_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	ld_cp(gb, wWarioState, WST_BOUNCY_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	LJUMP(SetState_BouncyFloor);
}

// restore hYPosHi/Lo and hXPosHi/Lo from their backups
static void restore_pos_backup(GB *gb)
{
	gb->cpu.a = rd8(gb, hYPosHiBackup);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosLoBackup);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hXPosHiBackup);
	wr8(gb, hXPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hXPosLoBackup);
	wr8(gb, hXPosLo, gb->cpu.a);
}

PORT_JUMP(UpdateState_ElectricStart)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	st(gb, wInvincibleCounter, 0x01);
	st(gb, wWarioState, WST_ELECTRIC);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	st(gb, wJumpVelIndex, 0x06);
	st(gb, wCollisionBoxBottom, (uint8_t)-1);
	st(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
	bool low_ceiling = false;
	if (ld_and(gb, wIsCrouching)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, hYPosHiBackup, gb->cpu.a);
		gb->cpu.a = rd8(gb, hYPosLo);
		wr8(gb, hYPosLoBackup, gb->cpu.a);
		gb->cpu.a = rd8(gb, hXPosHi);
		wr8(gb, hXPosHiBackup, gb->cpu.a);
		gb->cpu.a = rd8(gb, hXPosLo);
		wr8(gb, hXPosLoBackup, gb->cpu.a);
		FARCALL(CheckUpCollision);
		ld_cp(gb, wWarioState, WST_ELECTRIC);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and(gb)) {
			low_ceiling = true;
		} else {
			xor_a(gb);
			wr8(gb, wIsCrouching, 0);
			restore_pos_backup(gb);
		}
	}
	if (low_ceiling) {
		st(gb, wCollisionBoxTop, (uint8_t)-15);
		xor_a(gb);
		wr8(gb, wJumpVelTable, 0);
		restore_pos_backup(gb);
	} else {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		st(gb, wJumpVelTable, 0x01);
	}
	if (ld_and(gb, wDirection))
		set_frameset(gb, ADDR_Frameset_17b7c);
	else
		set_frameset(gb, ADDR_Frameset_17b8d);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(UpdateState_Electric)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	anim(gb, OAM1);
	FARCALL(CheckFrontCollision);
	if (!b_and(gb)) {
		bool right = ld_and(gb, wDirection);
		gb->cpu.b = 0x01;
		ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (CF)
			gb->cpu.b = alu_inc(gb, gb->cpu.b);
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	} else {
		bool right = ld_and(gb, wDirection);
		gb->cpu.b = 0x01;
		if (right)
			CALL(MoveWarioLeft);
		else
			CALL(MoveWarioRight);
	}

	if (!ld_and(gb, wJumpVelTable)) {
		SET_HL(gb, wJumpVelIndex);
		wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
		gb->cpu.a = rd8(gb, wJumpVelIndex);
		alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX);
		if (!CF)
			goto dizzy;
		FARCALL(CheckCentreCollision);
		gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
		if (gb->cpu.a) {
			RET();
			return;
		}
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		RET();
		return;
	}

	for (;;) {
		gb->cpu.a = rd8(gb, wJumpVelIndex);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.d = 0x00;
		uint16_t hl = alu_add16(gb, ADDR_JumpVelTable_Normal, REG_DE(gb));
		SET_HL(gb, hl);
		if (!alu_bit(gb, rd8(gb, hl), 7)) {
			// falling
			gb->cpu.b = rd8(gb, hl);
			CALL(MoveWarioDown);
			SET_HL(gb, wJumpVelIndex);
			wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
			gb->cpu.a = rd8(gb, wJumpVelIndex);
			alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX);
			if (!CF)
				wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
			break;
		}
		// rising
		gb->cpu.a = alu_cpl(gb, rd8(gb, hl));
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		gb->cpu.b = gb->cpu.a;
		CALL(MoveWarioUp);
		SET_HL(gb, wJumpVelIndex);
		wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
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

dizzy:
	play_sfx(gb, SFX_041);
	{
		bool right = ld_and(gb, wDirection);
		uint8_t xlo = rd8(gb, wWarioXPos + 1);
		uint8_t xhi = rd8(gb, wWarioXPos + 0);
		if (right) {
			wr8(gb, hXPosLo, alu_add(gb, xlo, 0x08));
			wr8(gb, hXPosHi, alu_adc(gb, xhi, 0x00));
		} else {
			wr8(gb, hXPosLo, alu_sub(gb, xlo, 0x08));
			wr8(gb, hXPosHi, alu_sbc(gb, xhi, 0x00));
		}
		wr8(gb, hYPosLo, alu_sub(gb, rd8(gb, wWarioYPos + 1), 0x20));
		gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0x00);
		wr8(gb, hYPosHi, gb->cpu.a);
		SET_HL(gb, wWarioYPos);
		SET_DE(gb, hYPosHi);
	}
	gb->cpu.b = PARTICLE_SMOKE_SMALL;
	FARCALL(CreateParticle);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	flip_dir(gb);
	st(gb, wWarioState, WST_ELECTRIC_DIZZY);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	if (ld_and(gb, wDirection))
		set_frameset(gb, ADDR_Frameset_17bad);
	else
		set_frameset(gb, ADDR_Frameset_17b9e);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(UpdateState_PuffyDeflating)
{
	if (!ld_and(gb, wWarioStateCounter)) {
		anim(gb, rd8(gb, wWarioOAMBank));
		if (!anim_ended(gb)) {
			RET();
			return;
		}
		st(gb, wWarioStateCounter, 0x01);
		xor_a(gb);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		if (ld_and(gb, wDirection))
			set_frameset(gb, ADDR_Frameset_1dcd31);
		else
			set_frameset(gb, ADDR_Frameset_1dcd01);
	}
	anim(gb, rd8(gb, wWarioOAMBank));
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	JUMP(RecoverFromTransformation);
}

PORT_JUMP(UpdateState_CrazyAirborne)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	anim(gb, OAM2);
	CALL(ApplyJumpVelocity);
	LCALL(HandleWalk);
	ld_cp(gb, wWalkVelIndex, 0x10);
	if (!CF)
		st(gb, wWalkVelIndex, 0x0c);
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
	LJUMP(SetState_Crazy);
}

PORT_JUMP(UpdateState_BatFalling)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	ld_cp(gb, wWarioState, WST_BAT_FALLING);
	if (!ZF) {
		RET();
		return;
	}
	anim(gb, OAM2);
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_BatFlying);
		return;
	}
	CALL(ApplyJumpVelocity);
	LCALL(Func_2b17a);
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	if (ld_and(gb, wIsTurning))
		LCALL(Func_2abc1);
	FARCALL(CheckAirborneCollision);
	ld_cp(gb, wWarioState, WST_BAT_FALLING);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		RET();
		return;
	}
	update_pos_y(gb);
	LJUMP(SetState_BatIdling);
}

PORT_JUMP(UpdateState_VampireAirborne)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	ld_cp(gb, wWarioState, WST_VAMPIRE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	anim(gb, OAM2);
	LCALL(Func_2b55c);
	ld_cp(gb, wWarioState, WST_VAMPIRE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		FARCALL(CheckUpCollision);
		ld_cp(gb, wWarioState, WST_VAMPIRE_AIRBORNE);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and(gb)) {
			RET();
			return;
		}
		LJUMP(Func_2a8a7);
		return;
	}
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		JUMP(TriggerDownwardsFloorTransition);
		return;
	}
	CALL(TriggerFloorTransition);
	ld_cp(gb, wWarioState, WST_VAMPIRE_AIRBORNE);
	if (!ZF) {
		RET();
		return;
	}
	LJUMP(SetState_VampireIdling);
}

PORT_JUMP(UpdateState_BatFlying)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	ld_cp(gb, wWarioState, WST_BAT_FLYING);
	if (!ZF) {
		RET();
		return;
	}
	sfx_loop(gb, 0x10, SFX_FLAP);
	anim(gb, OAM2);
	FARCALL(CheckUpCollision);
	ld_cp(gb, wWarioState, WST_BAT_FLYING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb)) {
		LJUMP(SetState_BatFalling);
		return;
	}
	LCALL(Func_2b63b);
	ld_cp(gb, wWarioState, WST_BAT_FLYING);
	if (!ZF) {
		RET();
		return;
	}
	LCALL(Func_2b17a);
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	if (!ld_and(gb, wIsTurning)) {
		RET();
		return;
	}
	LJUMP(Func_2abc1); // fallthrough
}

PORT_JUMP(UpdateState_CrazySpinning)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_and(gb, wWaterInteraction)) {
		JUMP(RecoverFromTransformation);
		return;
	}
	if (ld_and(gb, wJumpVelTable)) {
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (!b_and(gb)) {
			JUMP(TriggerDownwardsFloorTransition);
			return;
		}
		CALL(TriggerFloorTransition);
		xor_a(gb);
		wr8(gb, wJumpVelTable, 0);
		wr8(gb, wJumpVelIndex, 0);
	}
	anim(gb, OAM1);
	SET_HL(gb, wWarioStateCounter);
	wr8(gb, wWarioStateCounter, alu_inc(gb, rd8(gb, wWarioStateCounter)));
	gb->cpu.a = rd8(gb, wWarioStateCounter);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (CF) {
		RET();
		return;
	}
	wr8(gb, wWarioStateCounter, 0x00);
	play_sfx(gb, SFX_043);
	st(gb, wWarioState, WST_CRAZY_DIZZY);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wJumpVelTable,
		wJumpVelIndex, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	load_gfx(gb, BANK_WarioDizzyGfx, ADDR_WarioDizzyGfx);
	set_oam(gb, BANK_OAM_17bbc, ADDR_OAM_17bbc);
	set_frameset(gb, ADDR_Frameset_17ce2);
	anim(gb, OAM1);
	RET();
}
