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

PORT(VanishObjectAndKnockBackIfZombie)
{
	gb->cpu.b = OBJACTION_VANISH_TOUCH;
	LCALL(SetObjAction);
	ld_cp(gb, wTransformation, TRANSFORMATION_ZOMBIE_WARIO);
	if (!ZF)
		return;
	FARCALL(SetState_ZombieKnockBack);
}

PORT_JUMP(SetState_OnFire_ResetStateCounter)
{
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	LJUMP(SetState_OnFire); // fallthrough
}

// ld hl, wKeyAndTreasureFlags / set bit, [hl] / jr CollectKey
static void obj_key(GB *gb, int bit)
{
	SET_HL(gb, wKeyAndTreasureFlags);
	wr8(gb, wKeyAndTreasureFlags, (uint8_t)(rd8(gb, wKeyAndTreasureFlags) | (1 << bit)));
	LJUMP(CollectKey);
}

PORT_JUMP(ObjInteraction_RedKey) { obj_key(gb, RED_KEY_F); }
PORT_JUMP(ObjInteraction_GreyKey) { obj_key(gb, GREY_KEY_F); }
PORT_JUMP(ObjInteraction_GreenKey) { obj_key(gb, GREEN_KEY_F); }
PORT_JUMP(ObjInteraction_BlueKey) { obj_key(gb, BLUE_KEY_F); }

PORT_JUMP(Func_20fe8)
{
	gb->cpu.b = OBJACTION_06;
	LJUMP(SetObjAction);
}

PORT(ObjInteraction_None)
{
}

PORT_JUMP(ObjInteraction_Ink)
{
	gb->cpu.b = OBJACTION_06;
	LJUMP(SetObjAction);
}

PORT_JUMP(Func_2092d)
{
	gb->cpu.b = OBJACTION_VANISH_TOUCH;
	LJUMP(SetObjAction);
}

PORT_JUMP(ObjInteraction_Hand)
{
	if (ld_and(gb, wInvincibleCounter) || ld_and(gb, wTransformation)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_FAT_WALK);
	FARCALL(SetState_DraggedDown);
	LJUMP(Func_21cc9); // fallthrough
}

PORT_JUMP(ObjInteraction_UnlitTorch)
{
	ld_cp(gb, wTransformation, TRANSFORMATION_HOT_WARIO);
	if (!ZF) {
		RET();
		return;
	}
	gb->cpu.b = OBJACTION_0E;
	LJUMP(SetObjAction);
}

PORT(SetState_FlatStretching)
{
	st(gb, wAutoMoveState, 0x01);
	st(gb, wWarioState, WST_FLAT_STRETCHING);
	collision_box(gb, -1, -8, -11, 11);
	st(gb, wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wFrameDuration,
		wAnimationFrame, 0};
	clear(gb, zero);
	set_frameset(gb, ADDR_Frameset_17193);
	anim(gb, OAM1);
}

PORT(TreasureClearObjects)
{
	uint16_t hl = wObjects;
	SET_DE(gb, OBJ_STRUCT_LENGTH);
	for (;;) {
		gb->cpu.a = (uint8_t)(hl >> 8);
		alu_cp(gb, gb->cpu.a, (uint8_t)(wCurObj >> 8));
		if (ZF)
			break;
		gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
		alu_cp(gb, gb->cpu.a, OBJFLAG_ACTIVE);
		bool vanish = ZF;
		if (!vanish) {
			alu_cp(gb, gb->cpu.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
			vanish = ZF;
		}
		if (vanish) {
			gb->cpu.a = rd8(gb, wObjPtr + 1);
			alu_cp(gb, gb->cpu.a, (uint8_t)hl);
			if (!ZF) {
				gb_push16(gb, hl); // push hl / pop hl
				gb->cpu.c = OBJ_ACTION;
				gb->cpu.b = 0x00;
				wr8(gb, alu_add16(gb, hl, OBJ_ACTION), OBJACTION_VANISH_TREASURE);
				gb_pop16(gb);
			}
		}
		hl = alu_add16(gb, hl, OBJ_STRUCT_LENGTH);
	}
	SET_HL(gb, hl);
}

PORT(SetState_EnemyBumping)
{
	play_sfx(gb, SFX_BUMP);
	st(gb, wJumpVelIndex, 0x0e);
	st(gb, wJumpVelTable, 0x01);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wIsSmashAttacking, wGrabState, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_ENEMY_BUMPING);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	if (ld_and(gb, wIsCrouching))
		frameset_by_dir(gb, ADDR_Frameset_14a3b, ADDR_Frameset_14a38);
	else
		frameset_by_dir(gb, ADDR_Frameset_14a71, ADDR_Frameset_14a6c);
	anim(gb, OAM1);
}

PORT_JUMP(ObjInteraction_Food)
{
	if (ld_and(gb, wIsCrouching)) {
		LJUMP(Func_20a69);
		return;
	}
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_20a69);
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FAT_WARIO);
	if (ZF) {
		LJUMP(Func_20a69);
		return;
	}
	LCALL(Func_20a63);
	st(gb, wTransformation, TRANSFORMATION_FAT_WARIO);
	touch_states(gb, TOUCH_BUMP, 0x02);
	st(gb, wTransformationDuration + 0, (uint8_t)(FAT_WARIO_DURATION >> 8));
	st(gb, wTransformationDuration + 1, (uint8_t)FAT_WARIO_DURATION);
	static const uint16_t zero[] = {wInvisibleFrame, wSFXLoopCounter, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	if (ld_and(gb, wJumpVelTable))
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	play_sfx(gb, SFX_03A);
	CALL(UpdateLevelMusic);
	st(gb, wWarioState, WST_FAT_EATING);
	collision_box(gb, -1, -27, -9, 9);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioFatGfx, ADDR_WarioFatGfx);
	set_oam(gb, BANK_OAM_1742d, ADDR_OAM_1742d);
	frameset_by_dir(gb, ADDR_Frameset_1789c, ADDR_Frameset_178c3);
	anim(gb, OAM1);
	RET();
}

PORT(ObjInteraction_Electric)
{
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	ld_cp(gb, wObjScreenXPos, gb->cpu.b);
	gb->cpu.a = alu_or(gb, rd8(gb, wInteractionSide), CF ? INTERACTION_LEFT : INTERACTION_RIGHT);
	wr8(gb, wInteractionSide, gb->cpu.a);
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);

	if (ld_and(gb, wInvincibleCounter))
		return;
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		return;
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_ELECTRIC);
	if (ZF)
		return;
	if (ld_bit(gb, wInteractionSide, INTERACTION_RIGHT_F))
		st(gb, wDirection, DIRECTION_LEFT);
	else
		st(gb, wDirection, DIRECTION_RIGHT);
	play_sfx(gb, SFX_040);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
		wIsSmashAttacking, wInvisibleFrame, 0};
	clear(gb, zero);
	st(gb, wWarioState, WST_ELECTRIC_START);
	collision_box(gb, -1, -27, -9, 9);
	st(gb, wTransformation, TRANSFORMATION_ELECTRIC);
	touch_states(gb, TOUCH_VANISH, 0x01);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	SET_HL(gb, ADDR_WarioElectricPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioElectricGfx, ADDR_WarioElectricGfx);
	set_oam(gb, BANK_OAM_1790e, ADDR_OAM_1790e);
	frameset_by_dir(gb, ADDR_Frameset_17b79, ADDR_Frameset_17b76);
	anim(gb, OAM1);
}

// Adds n (BCD) to wNumCoins, capped at MAX_NUM_COINS
static void add_coins(GB *gb, uint8_t n)
{
	gb->cpu.a = alu_daa(gb, alu_add(gb, rd8(gb, wNumCoins + 1), n));
	wr8(gb, wNumCoins + 1, gb->cpu.a);
	gb->cpu.a = alu_daa(gb, alu_adc(gb, rd8(gb, wNumCoins), 0x00));
	wr8(gb, wNumCoins, gb->cpu.a);
	SET_HL(gb, wNumCoins);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xf0);
	if (ZF)
		return;
	gb->cpu.a = (uint8_t)(MAX_NUM_COINS >> 8);
	wr8(gb, wNumCoins, gb->cpu.a);
	gb->cpu.a = (uint8_t)MAX_NUM_COINS;
	wr8(gb, wNumCoins + 1, gb->cpu.a);
	SET_HL(gb, wNumCoins + 1);
}

PORT_JUMP(ObjInteraction_RegularCoin)
{
	add_coins(gb, 0x1);
	play_sfx(gb, SFX_REGULAR_COIN);
	LJUMP(Func_20a63);
}

#define ProcessInteractions_next_obj (ADDR_ProcessInteractions + 0x28)

PORT_JUMP(CollectMusicalCoin)
{
	add_coins(gb, 0x10);
	LCALL(Func_20a63);
	JUMP_TO(ProcessInteractions_next_obj);
}

PORT_JUMP(ObjInteraction_ColourCoin)
{
	play_sfx(gb, SFX_COLOUR_COIN);
	LJUMP(CollectMusicalCoin); // fallthrough
}

PORT_JUMP(ObjInteraction_MusicalCoin)
{
	SET_HL(gb, wNumMusicalCoins);
	gb->cpu.a = rd8(gb, wNumMusicalCoins);
	alu_cp(gb, gb->cpu.a, NUM_LEVEL_MUSICAL_COINS);
	if (!CF) {
		JUMP_TO(ProcessInteractions_next_obj);
		return;
	}
	wr8(gb, wNumMusicalCoins, alu_inc(gb, gb->cpu.a));
	play_sfx(gb, SFX_MUSICAL_COIN);
	LJUMP(CollectMusicalCoin);
}

PORT(ObjInteraction_InvisibilityPotion)
{
	LCALL(Func_20a63);
	if (ld_and(gb, wInvincibleCounter))
		return;
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		return;
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_INVISIBLE_WARIO);
	if (ZF)
		return;
	FARCALL(SetState_TurningInvisible);
}

PORT(ObjInteraction_RollingRock)
{
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	if (ld_and(gb, wIsRolling))
		return;
	ld(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	FARCALL(Func_1e174);
}

// ld a, [wInvincibleCounter] / and a / ret nz /
// ld a, [wTransformation] / bit TRANSFORMATIONF_PERSISTENT_F, a / ret nz:
// true if it returns
static bool invincible_or_persistent(GB *gb)
{
	return ld_and(gb, wInvincibleCounter) || ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F);
}

PORT(ObjInteraction_Vampire)
{
	if (invincible_or_persistent(gb))
		return;
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_VAMPIRE_WARIO);
	FARCALL(Func_19b25);
	ld_cp(gb, wTransformation, TRANSFORMATION_VAMPIRE_WARIO);
	if (!ZF)
		return;
	FARCALL(SetState_VampireTransforming);
}

PORT(ObjInteraction_Bubble)
{
	if (invincible_or_persistent(gb))
		return;
	gb->cpu.b = OBJACTION_07;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_BUBBLE);
	touch_states(gb, TOUCH_BUMP, 0x01);
	FARCALL(Func_2ac7d);
}

PORT_JUMP(ObjInteraction_ZombieHead)
{
	if (ld_and(gb, wInvincibleCounter)) {
		LJUMP(Func_20a69);
		return;
	}
	ld_cp(gb, wTransformation, TRANSFORMATION_ZOMBIE_WARIO);
	if (ZF) {
		LJUMP(VanishObjectAndKnockBackIfZombie);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_20a69);
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	play_sfx(gb, SFX_028);
	st(gb, wTransformation, TRANSFORMATION_ZOMBIE_WARIO);
	touch_states(gb, TOUCH_VANISH, 0x02);
	FARCALL(SetState_ZombieIdling);
	RET();
}

PORT(Func_2028a)
{
	if (!ld_and(gb, wJumpVelTable))
		return;
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	ld_cp(gb, wObjScreenXPos, gb->cpu.b);
	st(gb, wDirection, CF ? DIRECTION_RIGHT : DIRECTION_LEFT);
	FARCALL(SetState_FlatFalling);
}

// ld a, [wStingTouchState] / and a / jr z / cp TOUCH_BUMP / jr z /
// cp TOUCH_VANISH / jp z, Func_2092d / ret: 0 = go on, 1 = return,
// 2 = jumped to Func_2092d
static int sting_touch(GB *gb)
{
	if (!ld_and(gb, wStingTouchState))
		return 0;
	alu_cp(gb, gb->cpu.a, TOUCH_BUMP);
	if (ZF)
		return 0;
	alu_cp(gb, gb->cpu.a, TOUCH_VANISH);
	if (ZF) {
		LJUMP(Func_2092d);
		return 2;
	}
	RET();
	return 1;
}

PORT_JUMP(Func_208f2)
{
	if (sting_touch(gb))
		return;
	if (!ld_and(gb, wWaterInteraction)) {
		RET();
		return;
	}
	// call .Bump
	gb_push16(gb, ADDR_Func_208f2 + 0x20);
	gb->cpu.b = OBJACTION_BUMP;
	gb_call_ret(gb, ADDR_SetObjAction, ADDR_Func_208f2 + 0x07);
	gb_pop16(gb);
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF || ld_and(gb, wTransformation)) {
		RET();
		return;
	}
	FARCALL(StartDive);
	RET();
}

PORT_JUMP(Func_20899)
{
	if (sting_touch(gb))
		return;
	if (!ld_and(gb, wWaterInteraction)) {
		RET();
		return;
	}
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	ld_cp(gb, wObjScreenXPos, gb->cpu.b);
	gb->cpu.a = alu_or(gb, rd8(gb, wInteractionSide), CF ? INTERACTION_LEFT : INTERACTION_RIGHT);
	wr8(gb, wInteractionSide, gb->cpu.a);
	play_sfx(gb, SFX_014);
	gb->cpu.b = OBJACTION_WOBBLE;
	LCALL(SetObjAction);
	if (ld_and(gb, wTransformation) || ld_bit(gb, wInteractionSide, INTERACTION_UP_F)) {
		RET();
		return;
	}
	FARCALL(StartDive);
	RET();
}
