// Port of src/engine/level/object_interactions.asm
#include "wario_states.h"

// b = OBJACTION_* constant
// Sets the action of the object at wObjPtr, and records which side Wario
// interacted with it from.
PORT(SetObjAction)
{
	uint16_t obj = (uint16_t)(rd8(gb, wObjPtr + 0) << 8 | rd8(gb, wObjPtr + 1));
	gb->cpu.d = 0;
	gb->cpu.e = OBJ_ACTION;
	uint16_t hl = alu_add16(gb, obj, OBJ_ACTION);
	wr8(gb, hl, gb->cpu.b);
	gb->cpu.h = (uint8_t)(hl >> 8);
	gb->cpu.l = alu_inc(gb, (uint8_t)hl); // inc l
	hl = REG_HL(gb);

	gb->cpu.b = rd8(gb, wInteractionSide);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), 0x0f), gb->cpu.b); // OBJ_UNK_1D
	wr8(gb, hl, gb->cpu.a);
}

PORT(Func_20a63)
{
	gb->cpu.b = OBJACTION_07;
	CALL(SetObjAction);
}

PORT(Func_20a69)
{
	gb->cpu.b = OBJACTION_0D;
	CALL(SetObjAction);
}

PORT(Func_21cc9)
{
	gb->cpu.b = OBJACTION_13;
	CALL(SetObjAction);
}

PORT(CollectKey)
{
	play_sfx(gb, SFX_KEY);
	gb_call(gb, ADDR_Func_20a63);
	gb->cpu.a = ST_COLLECT_KEY;
	wr8(gb, wState, ST_COLLECT_KEY);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
}

PORT(SetState_OnFire)
{
	st(gb, wWarioState, WST_ON_FIRE);
	static const uint16_t zero[] = {wInvisibleFrame, wSFXLoopCounter, wWarioStateCycles,
		wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	collision_box(gb, -1, -27, -9, 9);
	if (ld_and(gb, wWarioStateCounter))
		return;
	SET_HL(gb, ADDR_WarioOnFirePal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioHotGfx, ADDR_WarioHotGfx);
	set_oam(gb, BANK_OAM_1673c, ADDR_OAM_1673c);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_16b38, ADDR_Frameset_16b49);
	anim(gb, OAM1);
}
