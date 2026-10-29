// Port of src/engine/level/wario_states_4.asm
#include "wario_states.h"

PORT(SetState_Splitting)
{
	CALL(TriggerFloorTransition);
	st(gb, wWarioState, WST_SPLITTING);
	xor_a(gb);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
}

PORT(UpdateState_SplitHit)
{
	anim(gb, OAM2);
	if (!anim_ended(gb))
		return;
	st(gb, wWarioState, WST_SPLIT_KNOCKED_BACK);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_1ffd19, ADDR_Frameset_1ffd13);
	anim(gb, OAM2);
}

PORT(SetState_BlindIdling)
{
	st(gb, wWarioState, WST_BLIND_IDLING);
	static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
		wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, wInvisibleFrame, 0};
	clear(gb, zero);
	collision_box(gb, -1, -27, -9, 9);
	xor_a(gb);
	CALL(UpdateLevelMusic);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14000, ADDR_OAM_14000);
	frameset_by_dir(gb, ADDR_Frameset_14252, ADDR_Frameset_1425f);
	anim(gb, OAM1);
}

PORT(SetState_BlindWalking)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_BLIND_WALKING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wJumpVelIndex, 0);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	// the d-pad decides the direction, else wDirection
	bool right;
	uint8_t pad = ld(gb, wJoypadDown);
	if (alu_bit(gb, pad, B_PAD_LEFT))
		right = false;
	else if (alu_bit(gb, pad, B_PAD_RIGHT))
		right = true;
	else
		right = ld_and(gb, wDirection);
	set_frameset(gb, right ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);
	anim(gb, OAM1);
}

PORT(Func_1ede4d)
{
	CALL(ClearTransformationValues);
	CALL(UpdateLevelMusic);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	FARCALL(StartJump_FromInput);
}

PORT(SetState_SnowballRolling)
{
	st(gb, wWarioState, WST_SNOWBALL_ROLLING);
	static const uint16_t zero[] = {wSFXLoopCounter, wJumpVelTable, wJumpVelIndex,
		wIsSmashAttacking, 0};
	clear(gb, zero);
}

PORT(SetState_SnowmanAirborne)
{
	st(gb, wWarioState, WST_SNOWMAN_AIRBORNE);
	static const uint16_t zero[] = {wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	anim(gb, OAM2);
}

PORT(Func_1edd24)
{
	xor_a(gb);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
	st(gb, wWarioState, WST_BALL_BOUNCING);
	reset_anim(gb);
	anim(gb, OAM3);
}

PORT(SetState_FanSpinning)
{
	st(gb, wWarioState, WST_FAN_SPINNING);
	reset_anim(gb);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1dc393);
	anim(gb, OAM3);
}

PORT(UpdateState_ReleasingOwl)
{
	anim(gb, OAM2);
	if (!anim_ended(gb))
		return;
	st(gb, wWarioState, WST_FALLING_FROM_OWL);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wJumpVelTable, 0x02);
	st(gb, wJumpVelIndex, 0x18);
}

PORT(SetState_BallShot)
{
	st(gb, wWarioState, WST_BALL_SHOT);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
		wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wAutoMoveState, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1dd26c);
	anim(gb, OAM3);
}

PORT(SetState_FanRecovering)
{
	st(gb, wWarioState, WST_FAN_RECOVERING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1dc384, ADDR_Frameset_1dc3c8);
	anim(gb, OAM3);
}

PORT(SetState_SnowmanTurning)
{
	st(gb, wWarioState, WST_SNOWMAN_TURNING);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1ff605, ADDR_Frameset_1ff5fe);
	anim(gb, OAM2);
}

PORT(SetState_IceSkatin)
{
	st(gb, wWarioState, WST_ICE_SKATIN);
	static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_1fdd97, ADDR_Frameset_1fdd94);
	anim(gb, OAM2);
}

PORT(SetState_BallAirborne)
{
	play_sfx(gb, SFX_066);
	st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	st(gb, wWarioState, WST_BALL_AIRBORNE);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1dd2f3, ADDR_Frameset_1dd2f6);
	anim(gb, OAM3);
}

PORT(SetState_FanLifting)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_FAN_LIFTING);
	static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_1dc374, ADDR_Frameset_1dc3b8);
	anim(gb, OAM3);
}

PORT(SetState_IceSkatinCrash)
{
	play_sfx(gb, SFX_02E);
	st(gb, wWarioState, WST_ICE_SKATIN_CRASH);
	reset_anim(gb);
	set_frameset(gb, flip_dir_and(gb) ? ADDR_Frameset_1fdd9a : ADDR_Frameset_1fdda7);
	anim(gb, OAM2);
}

PORT(SetState_SnowmanBumped)
{
	st(gb, wWarioState, WST_SNOWMAN_BUMPED);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1ff628, ADDR_Frameset_1ff639);
	anim(gb, OAM2);
}

PORT(SetState_SnowmanIdle)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_SNOWMAN_IDLE);
	static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	frameset_by_dir(gb, ADDR_Frameset_1ff5f8, ADDR_Frameset_1ff5fb);
	anim(gb, OAM2);
}

PORT(SetState_BallTurning)
{
	xor_a(gb);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
	st(gb, wWarioState, WST_BALL_TURNING);
	reset_anim(gb);
	set_frameset(gb, flip_dir_and(gb) ? ADDR_Frameset_1dd287 : ADDR_Frameset_1dd28e);
	anim(gb, OAM3);
}

PORT(SetState_FanTurning)
{
	flip_dir(gb);
	st(gb, wWarioState, WST_FAN_TURNING);
	reset_anim(gb);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1dc38e, ADDR_Frameset_1dc389);
	anim(gb, OAM3);
}

PORT(SetState_SnowballCrash)
{
	xor_a(gb);
	wr8(gb, wTransformationProgress, 0);
	st(gb, wWarioState, WST_SNOWBALL_CRASH);
	bool right = flip_dir_and(gb);
	gb->cpu.b = 2;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	reset_anim(gb);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1ff981);
	anim(gb, OAM2);
}

PORT(SetState_SnowmanWalking)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_SNOWMAN_WALKING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wJumpVelIndex, 0);
	reset_anim(gb);
	bool right;
	if (ld_bit(gb, wJoypadDown, B_PAD_LEFT))
		right = false;
	else if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT))
		right = true;
	else
		right = facing_right(gb);
	set_frameset(gb, right ? ADDR_Frameset_1ff5ef : ADDR_Frameset_1ff5e6);
	anim(gb, OAM2);
}

PORT(SetState_BlindTurning)
{
	st(gb, wWarioState, WST_BLIND_TURNING);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	frameset_by_dir(gb, ADDR_Frameset_14cbe, ADDR_Frameset_14cc5);
	anim(gb, OAM1);
}

PORT(SetState_OwlWario)
{
	st(gb, wWarioState, WST_OWL_SLOW);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, 0};
	clear(gb, zero);
	reset_anim(gb);
	if (ld_mask(gb, wSwimmingDirectionInput, PAD_UP | PAD_DOWN))
		set_frameset(gb, ADDR_Frameset_1fedd0);
	else
		frameset_by_dir(gb, ADDR_Frameset_1fed7c, ADDR_Frameset_1fed91);
	anim(gb, OAM2);
}

PORT(Func_1ede69)
{
	CALL(ClearTransformationValues);
	CALL(UpdateLevelMusic);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	FARCALL(Func_1e174);
}

PORT_JUMP(Func_1edf2b)
{
	FARCALL(Func_19acd);
	if (!ld_and(gb, wIsInAirCurrent)) {
		LJUMP(SetState_FanSpinning);
		return;
	}
	gb->cpu.b = 1;
	CALL(MoveWarioUp);
	RET();
}

PORT_JUMP(UpdateState_BallBouncing)
{
	anim(gb, OAM3);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_BallAirborne); // fallthrough
}

PORT_JUMP(UpdateState_BallTurning)
{
	anim(gb, OAM3);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_BallAirborne);
}

PORT_JUMP(UpdateState_SnowmanLanding)
{
	anim(gb, OAM2);
	if (ld_and(gb, wGroundShakeCounter)) {
		RET();
		return;
	}
	LJUMP(SetState_SnowmanIdle);
}

PORT_JUMP(UpdateState_IceSkatinStart)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	LJUMP(SetState_IceSkatin); // fallthrough
}

PORT_JUMP(UpdateWarioOwlDirection)
{
	st(gb, wSwimmingDirectionInput, facing_right(gb) ? PAD_RIGHT : PAD_LEFT);
	LJUMP(SetState_OwlWario); // fallthrough
}

PORT_JUMP(UpdateState_FanStart)
{
	FARCALL(Func_19b25);
	sfx_loop(gb, 0x0c, SFX_07D);
	anim(gb, OAM3);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_FanLifting); // fallthrough
}

PORT(Func_1ec64d)
{
	st(gb, wWarioState, WST_RELEASING_OWL);
	CALL(ReleaseOwl);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1fedfe);
	anim(gb, OAM2);
}

// shared by Func_1edaca and Func_1edaf8: turn if the d-pad points back,
// else pick a frameset and bounce
static void ball_bounce(GB *gb, uint16_t left, uint16_t right)
{
	bool r = facing_right(gb);
	if (ld_bit(gb, wJoypadDown, r ? B_PAD_LEFT : B_PAD_RIGHT)) {
		LJUMP(SetState_BallTurning);
		return;
	}
	set_frameset(gb, r ? right : left);
	LJUMP(Func_1edd24);
}

PORT_JUMP(Func_1edaca)
{
	ball_bounce(gb, ADDR_Frameset_1dd316, ADDR_Frameset_1dd323);
}

PORT_JUMP(Func_1edaf8)
{
	ball_bounce(gb, ADDR_Frameset_1dd230, ADDR_Frameset_1dd295);
}

PORT_JUMP(SetState_SnowmanAirborne_Fall)
{
	st(gb, wJumpVelIndex, 0x18);
	st(gb, wIsSmashAttacking, 0x01);
	wr8(gb, wWarioStateCounter, 0x01);
	st(gb, wJumpVelTable, 0x02);
	frameset_by_dir(gb, ADDR_Frameset_1ff622, ADDR_Frameset_1ff625);
	LJUMP(SetState_SnowmanAirborne);
}

PORT_JUMP(SetState_SnowmanAirborne_Jump)
{
	play_sfx(gb, SFX_JUMP);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wIsSmashAttacking, 0);
	wr8(gb, wWarioStateCounter, 0);
	st(gb, wJumpVelTable, 0x01);
	frameset_by_dir(gb, ADDR_Frameset_1ff60c, ADDR_Frameset_1ff617);
	LJUMP(SetState_SnowmanAirborne);
}

PORT(SetState_Swallowed)
{
	st(gb, wWarioState, WST_SWALLOWED);
	static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
		wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	wr8(gb, wAutoMoveState, gb->cpu.a);
	static const uint16_t zero2[] = {wFrameDuration, wAnimationFrame, wIsTurning, 0};
	clear(gb, zero2);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioLaunchGfx, ADDR_WarioLaunchGfx);
	set_oam(gb, BANK_OAM_1dc3cd, ADDR_OAM_1dc3cd);
	gb->cpu.a = rd8(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
	anim(gb, OAM3);
}

PORT(SetState_FanStart)
{
	st(gb, wWarioState, WST_FAN_START);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
		wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
		wJumpVelIndex, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioFanGfx, ADDR_WarioFanGfx);
	set_oam(gb, BANK_OAM_1dc000, ADDR_OAM_1dc000);
	frameset_by_dir(gb, ADDR_Frameset_1dc37f, ADDR_Frameset_1dc3c3);
	anim(gb, OAM3);
}

PORT(SetState_IceSkatinStart)
{
	st(gb, wWarioState, WST_ICE_SKATIN_START);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wInvisibleFrame, wWarioStateCounter, wWarioStateCycles,
		wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
		wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioIceSkatinPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioBubbleGfx, ADDR_WarioBubbleGfx);
	set_oam(gb, BANK_OAM_1fdb6d, ADDR_OAM_1fdb6d);
	gb->cpu.a = rd8(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_1fdd82 : ADDR_Frameset_1fdd8b);
	anim(gb, OAM2);
}

PORT(HandleWarioOwlInput)
{
	st(gb, wWarioState, WST_OWL_FAST);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	uint8_t joy = ld(gb, wJoypadDown);
	uint8_t in;
	if (alu_bit(gb, joy, B_PAD_UP))
		in = PAD_UP;
	else if (alu_bit(gb, joy, B_PAD_DOWN))
		in = PAD_DOWN;
	else if (alu_bit(gb, joy, B_PAD_RIGHT)) {
		st(gb, wDirection, DIRECTION_RIGHT);
		in = PAD_RIGHT;
	} else {
		st(gb, wDirection, DIRECTION_LEFT);
		in = PAD_LEFT;
	}
	st(gb, wSwimmingDirectionInput, in);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, 0};
	clear(gb, zero);
	reset_anim(gb);
	if (ld_mask(gb, wSwimmingDirectionInput, PAD_UP | PAD_DOWN))
		set_frameset(gb, ADDR_Frameset_1fede5);
	else
		frameset_by_dir(gb, ADDR_Frameset_1feda6, ADDR_Frameset_1fedbb);
	anim(gb, OAM2);
}

PORT(SetState_SplitHit)
{
	st(gb, wWarioState, WST_SPLIT_HIT);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0};
	clear(gb, zero);
	st(gb, wJumpVelTable, 0x01);
	st(gb, wJumpVelIndex, 0x0e);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioSplitPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioSnowballGfx, ADDR_WarioSnowballGfx);
	set_oam(gb, BANK_OAM_1ffa12, ADDR_OAM_1ffa12);
	st(gb, wDirection, rd8(gb, wObjDirection));
	frameset_by_dir(gb, ADDR_Frameset_1ffd16, ADDR_Frameset_1ffd10);
	anim(gb, OAM2);
}

PORT(SetState_TurningIntoSnowman)
{
	play_sfx(gb, SFX_047);
	st(gb, wWarioState, WST_SNOWMAN_START);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelIndex, 0};
	clear(gb, zero);
	if (ld_and(gb, wJumpVelTable))
		st(gb, wJumpVelIndex, 0x18);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioSnowmanPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioSnowmanGfx, ADDR_WarioSnowmanGfx);
	set_oam(gb, BANK_OAM_1feec7, ADDR_OAM_1feec7);
	frameset_by_dir(gb, ADDR_Frameset_1ff560, ADDR_Frameset_1ff5a3);
	anim(gb, OAM2);
}

PORT(SetState_Launched)
{
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	st(gb, wTransformation, TRANSFORMATION_LAUNCHED);
	st(gb, wTouchState, TOUCH_VANISH);
	st(gb, wStingTouchState, TOUCH_VANISH);
	st(gb, wca94, 0x02);
	st(gb, wWarioState, WST_LAUNCHED);
	static const uint16_t zero[] = {wSFXLoopCounter, wWalkVelIndex, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
		wIsSmashAttacking, wJumpVelTable, wJumpVelIndex, 0};
	clear(gb, zero);
	collision_box(gb, -1, -15, -9, 9);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioLaunchGfx, ADDR_WarioLaunchGfx);
	set_oam(gb, BANK_OAM_1dc449, ADDR_OAM_1dc449);
	frameset_by_dir(gb, ADDR_Frameset_1dc904, ADDR_Frameset_1dc8fb);
	anim(gb, OAM3);
}

// ld hl, wWarioPos / ld de, hPos / copy 4 bytes (hl ends at wWarioPos + 3)
static void copy_pos_to_hpos(GB *gb)
{
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wWarioPos + i);
		wr8(gb, hPos + i, gb->cpu.a);
	}
	SET_HL(gb, wWarioPos + 3);
	SET_DE(gb, hPos + 3);
}

PORT(SetState_MagicRising)
{
	copy_pos_to_hpos(gb);
	gb->cpu.b = PARTICLE_MAGIC_1;
	FARCALL(CreateParticle);
	copy_pos_to_hpos(gb);
	gb->cpu.b = PARTICLE_MAGIC_2;
	FARCALL(CreateParticle);
	st(gb, wWarioState, WST_MAGIC_RISING);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles,
		wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking,
		wJumpVelIndex, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioMagicPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioMagicGfx, ADDR_WarioMagicGfx);
	set_oam(gb, BANK_OAM_1dcd60, ADDR_OAM_1dcd60);
	set_frameset(gb, ADDR_Frameset_1dcfb0);
	anim(gb, OAM3);
}

PORT(UpdateState_BallShot)
{
	if (!ld_and(gb, wAutoMoveState)) {
		st(gb, wDirection, DIRECTION_LEFT);
		st(gb, wWarioState, WST_BALL_THROWN);
		static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(gb, zero);
		st(gb, wJumpVelIndex, 0x6);
		st(gb, wJumpVelTable, JUMP_VEL_HIGH_JUMP);
		reset_anim(gb);
		set_frameset(gb, ADDR_Frameset_1dd2ee);
		anim(gb, OAM3);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, 7)) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x7f);
		wr8(gb, wAutoMoveState, gb->cpu.a);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (ZF) {
			// .phase_3
			SET_HL(gb, ADDR_WarioBallPal);
			CALL(SetWarioPal);
			reset_anim(gb);
			set_frameset(gb, ADDR_Frameset_1dd26c);
		} else {
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (ZF) {
				// .phase_1
				reset_anim(gb);
				set_frameset(gb, ADDR_Frameset_1dd2d1);
			} else {
				gb->cpu.a = alu_dec(gb, gb->cpu.a);
				if (!ZF)
					return;
				// .phase_2
				SET_HL(gb, ADDR_WarioBallShotPal);
				CALL(SetWarioPal);
				reset_anim(gb);
				set_frameset(gb, ADDR_Frameset_1dd27a);
			}
		}
	}
	anim(gb, OAM3);
	ld_cp(gb, wAutoMoveState, 0x02);
	if (!ZF)
		return;
	if (!anim_ended(gb))
		return;
	st(gb, wAutoMoveState, 0x03 | (1 << 7));
}

PORT_JUMP(TurnIntoSmallSnowball)
{
	st(gb, wTransformationProgress, 1);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	load_gfx(gb, BANK_WarioSnowballGfx, ADDR_WarioSnowballGfx);
	set_oam(gb, BANK_OAM_1ff64a, ADDR_OAM_1ff64a);
	reset_anim(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioSlopeInteraction), 0x0f);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_1ff92b : ADDR_Frameset_1ff956);
	anim(gb, OAM2);
	LJUMP(SetState_SnowballRolling);
}

PORT_JUMP(Func_1ec215)
{
	gb->cpu.b = 3;
	if (facing_right(gb))
		CALL(MoveWarioLeft);
	else
		CALL(MoveWarioRight);
	LJUMP(SetState_IceSkatinCrash);
}

PORT_JUMP(HandleSnowmanInput)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_SnowmanAirborne_Jump);
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_SnowmanWalking);
		return;
	}
	RET();
}

PORT_JUMP(Func_1ede96)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_SnowmanAirborne_Jump);
		return;
	}
	gb->cpu.a = rd8(gb, wJoypadDown);
	bool turn;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		turn = !ld_and(gb, wDirection);
		if (!turn)
			FARCALL(WalkIfPossible_Right);
		else
			st(gb, wDirection, DIRECTION_RIGHT);
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		ld_cp(gb, wDirection, DIRECTION_LEFT);
		turn = !ZF;
		if (!turn)
			FARCALL(WalkIfPossible_Left);
		else
			st(gb, wDirection, DIRECTION_LEFT);
	} else {
		LJUMP(SetState_SnowmanIdle);
		return;
	}
	if (turn) {
		LJUMP(SetState_SnowmanTurning);
		return;
	}
	ld_cp(gb, wWalkVelIndex, 0x08);
	if (!CF)
		st(gb, wWalkVelIndex, 0x04);
	RET();
}

PORT_JUMP(Func_1edf47)
{
	bool right = facing_right(gb);
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
		LJUMP(SetState_FanTurning);
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, right ? B_PAD_RIGHT : B_PAD_LEFT)) {
		RET();
		return;
	}
	port_farcall(gb, right ? BANK_Func_19741 : BANK_Func_197b1, right ? ADDR_Func_19741 : ADDR_Func_197b1);
	if (b_and(gb)) {
		RET();
		return;
	}
	gb->cpu.b = 1;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	RET();
}

PORT_JUMP(UpdateState_MagicStopping)
{
	gb->cpu.a = inc_counter(gb);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (CF) {
		RET();
		return;
	}
	JUMP(RecoverFromTransformation); // fallthrough to UpdateState_MagicRecovering
}

// UpdateWarioAnimation in OAM bank `bank`, then true once it has ended
// (ret z otherwise)
static bool anim_and_ended(GB *gb, uint8_t bank)
{
	anim(gb, bank);
	if (!anim_ended(gb)) {
		RET();
		return false;
	}
	return true;
}

PORT_JUMP(UpdateState_SnowmanBumped)
{
	if (anim_and_ended(gb, OAM2))
		LJUMP(SetState_SnowmanIdle);
}

PORT_JUMP(UpdateState_LaunchCrash)
{
	if (anim_and_ended(gb, OAM3)) {
		FARCALL(Func_2ad6a);
		RET();
	}
}

PORT_JUMP(UpdateState_GrabbingOwl)
{
	if (!anim_and_ended(gb, OAM2))
		return;
	if (ld_mask(gb, wJoypadDown, PAD_CTRL_PAD)) {
		LJUMP(HandleWarioOwlInput);
		return;
	}
	LJUMP(UpdateWarioOwlDirection); // fallthrough
}

PORT_JUMP(UpdateState_BallStart)
{
	CALL(ApplyJumpVelocity);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	update_pos_y(gb);
	LJUMP(Func_1edaca); // fallthrough
}

PORT_JUMP(UpdateStart_FanTurning)
{
	LCALL(Func_1edf2b);
	ld_cp(gb, wWarioState, WST_FAN_TURNING);
	if (!ZF) {
		RET();
		return;
	}
	sfx_loop(gb, 0x0c, SFX_07D);
	if (anim_and_ended(gb, OAM3))
		LJUMP(SetState_FanLifting);
}

PORT_JUMP(UpdateState_Swallowed)
{
	if (ld_and(gb, wIsTurning)) {
		xor_a(gb);
		wr8(gb, wIsTurning, 0);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		set_frameset(gb, flip_dir_and(gb) ? ADDR_Frameset_1dc42c : ADDR_Frameset_1dc40f);
	}
	anim(gb, OAM3);
	if (ld_and(gb, wAutoMoveState)) {
		RET();
		return;
	}
	st(gb, wDirection, DIRECTION_LEFT);
	LJUMP(SetState_Launched); // fallthrough
}

PORT_JUMP(UpdateState_BallSentUpwards)
{
	anim(gb, OAM3);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		gb->cpu.b = 3;
		CALL(MoveWarioUp);
		FARCALL(CheckUpCollision);
		if (!b_and(gb)) {
			JUMP(HandleUpwardsFloorTransition);
			return;
		}
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		st(gb, wJumpVelTable, JUMP_VEL_NORMAL);
		RET();
		return;
	}
	CALL(ApplyJumpVelocity);
	FARCALL(CheckAirborneCollision);
	if (b_and(gb)) {
		update_pos_y(gb);
		xor_a(gb);
		wr8(gb, wIsIntangible, 0);
		wr8(gb, wJumpVelIndex, 0);
		wr8(gb, wJumpVelTable, 0);
		FARCALL(Func_206eb);
	}
	RET();
}

PORT(UpdateState_MagicRising)
{
	FARCALL(Func_19b25);
	FARCALL(CheckUpCollision);
	if (b_and(gb)) {
		get_pos(gb);
		gb->cpu.b = PARTICLE_MAGIC_STOP;
		FARCALL(CreateParticle);
		st(gb, wWarioState, WST_MAGIC_STOPPING);
		xor_a(gb);
		wr8(gb, wWarioStateCounter, 0);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		anim(gb, 0x77);
		return;
	}
	sfx_loop(gb, 0x0c, SFX_07D);
	anim(gb, 0x77);
	gb->cpu.b = 1;
	CALL(MoveWarioUp);
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	if (CF)
		return;
	CALL(GetFloorForYPos);
	gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
	if (CF)
		CALL(StartUpwardsFloorTransition);
}

PORT_JUMP(UpdateState_Launched)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	sfx_loop(gb, 0x86, SFX_04C);
	anim(gb, OAM3);
	FARCALL(CheckFrontCollision);
	if (!b_and(gb)) {
		gb->cpu.b = 2;
		move_by_dir(gb);
		RET();
		return;
	}
	play_sfx(gb, SFX_048);
	st(gb, wWarioState, WST_LAUCH_CRASH);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_1dc926, ADDR_Frameset_1dc90d);
	anim(gb, OAM3);
	RET();
}

PORT(UpdateState_BallThrown)
{
	anim(gb, OAM3);
	CALL(ApplyJumpVelocity);
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (CF) {
		gb->cpu.b = 3;
		CALL(MoveWarioLeft);
		return;
	}
	gb->cpu.b = 2;
	CALL(MoveWarioLeft);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb))
		return;
	update_pos_y(gb);
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	// push_wram BANK(wShootGoals)
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	st(gb, rWBK, BANK_wShootGoals);
	ld_cp(gb, wShootGoals, 3);
	if (!ZF) {
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
		FARCALL(Func_206eb);
		return;
	}
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
	st(gb, wWarioState, WST_BALL_SENT_UPWARDS);
	play_sfx(gb, SFX_066);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wJumpVelIndex, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1dd2ee);
	anim(gb, OAM3);
}

PORT_JUMP(SetState_BallStart)
{
	CALL(UpdateLevelMusic);
	collision_box(gb, -1, -20, -9, 9);
	static const uint16_t clr[] = {
		wGrabState, wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, 0,
	};
	clear(gb, clr);
	SET_HL(gb, ADDR_WarioBallPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioBallGfx, ADDR_WarioBallGfx);
	set_oam(gb, BANK_OAM_1dcff1, ADDR_OAM_1dcff1);
	if (!ld_and(gb, wJumpVelTable)) {
		LJUMP(Func_1edaca);
		return;
	}
	st(gb, wWarioState, WST_BALL_START);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wJumpVelIndex, 0x18);
	reset_anim(gb);
	set_frameset(gb, ADDR_Frameset_1dd313);
	anim(gb, OAM3);
	RET();
}
