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
