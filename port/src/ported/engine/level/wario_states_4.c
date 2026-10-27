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
