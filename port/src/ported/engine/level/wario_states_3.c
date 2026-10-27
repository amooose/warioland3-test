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

// xor a / ld [wFrameDuration], a / ld [wAnimationFrame], a
static void reset_anim(GB *gb)
{
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
}

// ld a, t / ld [wTouchState], a / ld a, t / ld [wStingTouchState], a /
// ld a, ca94 / ld [wca94], a
static void touch_states(GB *gb, uint8_t t, uint8_t ca94)
{
	st(gb, wTouchState, t);
	st(gb, wStingTouchState, t);
	st(gb, wca94, ca94);
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
