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
	gb_call(gb, ADDR_SetObjAction);
}

PORT(Func_20a69)
{
	gb->cpu.b = OBJACTION_0D;
	gb_call(gb, ADDR_SetObjAction);
}

PORT(Func_21cc9)
{
	gb->cpu.b = OBJACTION_13;
	gb_call(gb, ADDR_SetObjAction);
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

PORT(SetState_WaterStung)
{
	play_sfx(gb, SFX_STING);
	st(gb, wInvincibleCounter, 0x01);
	st(gb, wWarioState, WST_WATER_STUNG);
	static const uint16_t zero[] = {wSwimVelIndex, wJumpVelTable, wWarioStateCounter,
		wWarioStateCycles, 0};
	clear(gb, zero);
	st(gb, wIsIntangible, TRUE);
	collision_box(gb, -1, -15, -9, 9);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
	anim(gb, OAM1);
}

PORT(ObjInteraction_Rail)
{
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		return;
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_RAIL);
	if (ZF)
		return;
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	play_sfx(gb, SFX_HANG_GRAB);
	st(gb, wTransformation, TRANSFORMATION_RAIL);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_VULNERABLE);
	st(gb, wca94, 0x01);
	st(gb, wWarioState, WST_HANGING_RAIL);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
		wJumpVelIndex, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wAutoMoveState, gb->cpu.a);
	CALL(UpdateLevelMusic);
	reset_anim(gb);
	load_gfx(gb, BANK_WarioHangGfx, ADDR_WarioHangGfx);
	set_oam(gb, BANK_OAM_1fee05, ADDR_OAM_1fee05);
	st(gb, wDirection, rd8(gb, wObjDirection));
	set_frameset(gb, ADDR_Frameset_1feec4);
	anim(gb, OAM2);
}

PORT(GetTreasure)
{
	// stop_music
	wr8(gb, hMusicID + 0, 0xff);
	st(gb, hMusicID + 1, 0x00);
	LCALL(Func_20a63);
	LCALL(TreasureClearObjects);
	SET_HL(gb, wLevelEndScreen);
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	alu_cp(gb, gb->cpu.a, LVLEND_NO_TREASURE);
	if (ZF)
		return;
	wr8(gb, wLevelEndScreen, rd8(gb, wLevelEndScreen) | 0x80);
	st(gb, wResetDisabled, TRUE);
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_GET_TREASURE_START);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wJumpVelIndex,
		wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking, wIsCrouching,
		wAttackCounter, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	wr8(gb, wAutoMoveState, gb->cpu.a);
	if (ld_and(gb, wJumpVelTable))
		st(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14000, ADDR_OAM_14000);
	if (ld_bit(gb, wInteractionSide, INTERACTION_LEFT_F)) {
		gb->cpu.b = ld(gb, wc1c4);
		CALL(MoveWarioRight);
		st(gb, wDirection, DIRECTION_LEFT);
		set_frameset(gb, ADDR_Frameset_14252);
	} else {
		gb->cpu.b = ld(gb, wc1c3);
		CALL(MoveWarioLeft);
		st(gb, wDirection, DIRECTION_RIGHT);
		set_frameset(gb, ADDR_Frameset_1425f);
	}
	anim(gb, OAM1);
}

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

PORT(SetState_Stung)
{
	st(gb, wInvincibleCounter, 0x01);
	play_sfx(gb, SFX_STING);
	st(gb, wWarioState, WST_STUNG);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsRolling, wIsSmashAttacking, 0};
	clear(gb, zero);
	st(gb, wJumpVelIndex, 0x6);
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
		st(gb, wJumpVelTable, JUMP_VEL_KNOCK_BACK);
	}
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	reset_anim(gb);
	frameset_by_dir(gb, ADDR_Frameset_15f7f, ADDR_Frameset_15f70);
	anim(gb, OAM1);
}

PORT_JUMP(Func_206eb)
{
	ld_cp(gb, wTransformation, TRANSFORMATION_OWL_WARIO);
	if (ZF)
		CALL(ReleaseOwl);
	CALL(ClearTransformationValues);
	CALL(UpdateLevelMusic);
	LJUMP(SetState_Stung); // fallthrough
}

PORT_JUMP(Func_207ed)
{
	if (ld_and(gb, wInvincibleCounter) || !ld_and(gb, wWaterInteraction)) {
		RET();
		return;
	}
	if (ld_and(gb, wStingTouchState)) {
		alu_cp(gb, gb->cpu.a, TOUCH_BUMP);
		if (ZF) {
			JUMP_TO(ADDR_Func_208f2 + 0x08); // Func_208f2.start
			return;
		}
		alu_cp(gb, gb->cpu.a, TOUCH_VANISH);
		if (ZF) {
			LJUMP(Func_2092d);
			return;
		}
		RET();
		return;
	}
	gb->cpu.b = OBJACTION_06;
	gb_call(gb, ADDR_SetObjAction);
	LJUMP(SetState_WaterStung); // fallthrough
}

// stop_sfx
static void stop_sfx(GB *gb)
{
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
}

// the shared body of ObjInteraction_WaterTeleporting/Jamano
static void teleport(GB *gb, uint16_t stung, uint8_t state)
{
	st(gb, wRoomTransitionParam, ROOMTRANSITION_TELEPORT | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY);
	gb_call(gb, stung);
	stop_sfx(gb);
	st(gb, wWarioState, state);
	gb->cpu.b = OBJACTION_TELEPORT;
	LJUMP(SetObjAction);
}

PORT_JUMP(ObjInteraction_WaterTeleporting) { teleport(gb, ADDR_SetState_WaterStung, WST_WATER_TELEPORTING); }
PORT_JUMP(ObjInteraction_Jamano) { teleport(gb, ADDR_Func_206eb, WST_TELEPORTING); }

PORT_JUMP(Func_21e8a)
{
	FARCALL(SetState_BallShot);
	LJUMP(Func_21cc9);
}

PORT_JUMP(Func_20d81)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(Func_20899);
		return;
	}
	LJUMP(Func_208f2);
}

PORT(Func_2168b)
{
	ld_cp(gb, wWarioState, WST_FLAT_SQUISHED);
	if (ZF) {
		FARCALL(SetState_FlatSquishedLifting);
		return;
	}
	ld_cp(gb, wc1c2, 0x10);
	if (!CF)
		gb->cpu.a = 0x0c;
	gb->cpu.b = gb->cpu.a;
	CALL(MoveWarioDown);
	FARCALL(Func_19b25);
	if (ld_and(gb, wWaterInteraction)) {
		FARCALL(DiveFromSurface_SkipSplash);
		return;
	}
	FARCALL(CheckCentreCollision);
	bool ground = b_and(gb);
	if (ground)
		update_pos_y(gb);
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF)
		return;
	ld_cp(gb, wTransformation, TRANSFORMATION_FLAT_WARIO);
	if (ZF) {
		if (!ground) {
			FARCALL(SetState_FlatFalling);
			return;
		}
		ld_cp(gb, wWarioState, WST_FLAT_SQUISHED);
		if (ZF)
			return;
		FARCALL(SetState_FlatIdling);
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	if (ground) {
		xor_a(gb);
		wr8(gb, wGrabState, 0);
		FARCALL(SetState_Idling);
		return;
	}
	if (ld_and(gb, wIsCrouching)) {
		FARCALL(StartCrouchFall);
		return;
	}
	xor_a(gb);
	wr8(gb, wGrabState, 0);
	FARCALL(StartFall);
}

PORT_JUMP(ObjInteraction_Snow)
{
	if (ld_and(gb, wInvincibleCounter) || ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_20a69);
		return;
	}
	LCALL(Func_20a63);
	st(gb, wTransformation, TRANSFORMATION_SNOWMAN_WARIO);
	xor_a(gb);
	wr8(gb, wTransformationProgress, 0);
	touch_states(gb, TOUCH_ATTACK, 0x02);
	FARCALL(SetState_TurningIntoSnowman);
	RET();
}

PORT(ObjInteraction_Ice)
{
	if (ld_and(gb, wInvincibleCounter) || ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		return;
	if (ld_and(gb, wIsCrouching)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (b_and(gb)) {
			st(gb, wCollisionBoxTop, (uint8_t)-15);
			return;
		}
	}
	gb->cpu.b = OBJACTION_07;
	LCALL(SetObjAction);
	play_sfx(gb, SFX_028);
	st(gb, wTransformation, TRANSFORMATION_ICE_SKATIN);
	touch_states(gb, TOUCH_ATTACK, 0x02);
	FARCALL(SetState_IceSkatinStart);
}

PORT_JUMP(Func_209ca)
{
	ld_cp(gb, wWarioState, WST_ATTACKING_AIRBORNE);
	if (ZF) {
		xor_a(gb);
		wr8(gb, wAttackCounter, 0);
	}
	if (ld_and(gb, wWaterInteraction)) {
		RET();
		return;
	}
	ld_cp(gb, wWarioState, WST_DIVING);
	if (ZF) {
		RET();
		return;
	}
	st(gb, wIsOnSteppableObject, TRUE);
	if (ld_and(gb, wc1c1)) {
		alu_cp(gb, gb->cpu.a, 0x03);
		if (!CF)
			gb->cpu.a = 0x02;
		gb->cpu.b = gb->cpu.a;
		CALL(MoveWarioUp);
	}
	gb->cpu.b = OBJACTION_STEP_ON;
	LCALL(SetObjAction);
	if (!ld_and(gb, wJumpVelTable) || ld_and(gb, wAttackCounter) || ld_and(gb, wIsRolling)) {
		JUMP_TO(ProcessInteractions_next_obj);
		return;
	}
	CALL(TriggerFloorTransition_SkipUpdateYPos);
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF) {
		JUMP_TO(ProcessInteractions_next_obj);
		return;
	}
	if (ld_and(gb, wTransformation)) {
		RET();
		return;
	}
	if (ld_and(gb, wGrabState)) {
		FARCALL(SetState_GrabIdling);
	} else if (ld_and(gb, wIsCrouching)) {
		xor_a(gb);
		wr8(gb, wWaterInteraction, 0);
		FARCALL(CrouchOrSlide);
	} else {
		xor_a(gb);
		wr8(gb, wWaterInteraction, 0);
		FARCALL(SetState_Idling);
	}
	JUMP_TO(ProcessInteractions_next_obj);
}

PORT_JUMP(Func_20a6f)
{
	gb->cpu.b = ld(gb, wWarioScreenXPos);
	alu_cp(gb, ld(gb, wObjScreenXPos), gb->cpu.b);
	bool left = CF;
	for (;;) {
		if (!left) { // .asm_20a79
			if (!ld_and(gb, wDirection)) {
				JUMP_TO(ProcessInteractions_next_obj);
				return;
			}
			st(gb, wInteractionSide, INTERACTION_RIGHT);
			FARCALL(Func_197b1);
			if (b_and(gb))
				break;
			ld_cp(gb, wc1c3, 0xff);
			if (ZF) {
				left = true;
				continue;
			}
			gb->cpu.b = gb->cpu.a;
			CALL(MoveWarioLeft);
			break;
		} else { // .asm_20aa5
			if (ld_and(gb, wDirection)) {
				JUMP_TO(ProcessInteractions_next_obj);
				return;
			}
			st(gb, wInteractionSide, INTERACTION_LEFT);
			FARCALL(Func_19741);
			if (b_and(gb))
				break;
			ld_cp(gb, wc1c4, 0xff);
			if (ZF) {
				left = false;
				continue;
			}
			gb->cpu.b = gb->cpu.a;
			CALL(MoveWarioRight);
			break;
		}
	}
	// .asm_20acf
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BALL_O_STRING_WARIO);
	if (ZF) {
		ld_cp(gb, wTransformationProgress, 1);
		if (ZF)
			FARCALL(Func_28bd5);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_HOT_WARIO);
	if (ZF) {
		ld_cp(gb, wTransformationProgress, 1);
		if (ZF)
			FARCALL(Func_2afc5);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_ICE_SKATIN);
	if (ZF) {
		ld_cp(gb, wWarioState, WST_ICE_SKATIN_CRASH);
		if (!ZF)
			FARCALL(Func_1ec215);
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	if (!ld_and(gb, wIsRolling) && !ld_and(gb, wAttackCounter)) {
		JUMP_TO(ProcessInteractions_next_obj);
		return;
	}
	FARCALL(DoJumpingBump);
	RET();
}

// routine called when an object is attacked
// whether by Wario's B attack, Fat Wario bump, rolling, etc.
PORT(AttackObject)
{
	play_sfx(gb, SFX_017);
	gb->cpu.b = ld(gb, wWarioScreenXPos);
	alu_cp(gb, ld(gb, wObjScreenXPos), gb->cpu.b);
	bool left = CF;
	st(gb, wInteractionSide, left ? INTERACTION_LEFT : INTERACTION_RIGHT);
	// x = Wario's x +/- 8, y = Wario's y - 8
	gb->cpu.a = rd8(gb, wWarioXPos + 1);
	gb->cpu.a = left ? alu_sub(gb, gb->cpu.a, 0x08) : alu_add(gb, gb->cpu.a, 0x08);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, wWarioXPos + 0);
	gb->cpu.a = left ? alu_sbc(gb, gb->cpu.a, 0x00) : alu_adc(gb, gb->cpu.a, 0x00);
	wr8(gb, hXPosLo - 1, gb->cpu.a);
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioXPos - 1), 0x08);
	wr8(gb, hXPosLo - 2, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos - 2), 0x00);
	wr8(gb, hXPosLo - 3, gb->cpu.a);
	SET_HL(gb, wWarioXPos - 2);
	SET_DE(gb, hXPosLo - 3);

	// .spawn_stars
	gb->cpu.b = PARTICLE_STARS;
	FARCALL(CreateParticle);
	gb->cpu.b = OBJACTION_ATTACK;
	LCALL(SetObjAction);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FAT_WARIO);
	if (ZF) {
		FARCALL(SetState_FatBumping);
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_SNOWMAN_WARIO);
	if (ZF) {
		if (!ld_and(gb, wTransformationProgress))
			FARCALL(SetState_SnowmanBumped);
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	if (ld_and(gb, wIsRolling)) {
		if (!ld_bit(gb, wObjInteractionType, HEAVY_OBJ_F))
			return;
		ld_cp(gb, wPowerUpLevel, GARLIC);
		if (!CF)
			return;
		FARCALL(DoJumpingBump_NoSFX);
		return;
	}
	// .not_rolling
	if (ld_bit(gb, wObjInteractionType, HEAVY_OBJ_F)) {
		FARCALL(DoJumpingBump_NoSFX);
		return;
	}
	if (ld_and(gb, wJumpVelTable))
		FARCALL(StartFall);
	else
		FARCALL(SetState_Idling);
}

PORT(Func_217a9)
{
	FARCALL(DoJumpingBump);
}

PORT_JUMP(Func_20d8c)
{
	if (!ld_and(gb, wWaterInteraction)) {
		RET();
		return;
	}
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F)) {
		LJUMP(Func_207ed);
		return;
	}
	if (ld_and(gb, wTransformation)) {
		RET();
		return;
	}
	if (ld_bit(gb, wInteractionSide, INTERACTION_DOWN_F)) {
		FARCALL(StartDive);
		RET();
		return;
	}
	gb->cpu.b = ld(gb, wWarioScreenXPos);
	alu_cp(gb, ld(gb, wObjScreenXPos), gb->cpu.b);
	bool left = CF;
	gb->cpu.a = alu_or(gb, rd8(gb, wInteractionSide), left ? INTERACTION_LEFT : INTERACTION_RIGHT);
	wr8(gb, wInteractionSide, gb->cpu.a);
	st(gb, wDirection, left ? DIRECTION_RIGHT : DIRECTION_LEFT);
	FARCALL(SetState_SwimKnockBack);
	RET();
}

// ld a, [wInteractionSide] / bit side, a / jp nz, Func_207ed / jp Func_208f2
static void sting_or_bump(GB *gb, int side)
{
	if (ld_bit(gb, wInteractionSide, side))
		LJUMP(Func_207ed);
	else
		LJUMP(Func_208f2);
}

PORT_JUMP(Func_20deb)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(Func_20899);
		return;
	}
	if (!ld_and(gb, wObjDirection))
		sting_or_bump(gb, INTERACTION_RIGHT_F);
	else
		sting_or_bump(gb, INTERACTION_LEFT_F);
}

PORT_JUMP(ObjInteraction_Haridama)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(Func_20899);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		LJUMP(Func_207ed);
		return;
	}
	ld_cp(gb, wObjDirection, DIRECTION_RIGHT);
	if (ZF)
		sting_or_bump(gb, INTERACTION_LEFT_F);
	else
		sting_or_bump(gb, INTERACTION_RIGHT_F);
}

PORT_JUMP(ObjInteraction_Pesce)
{
	ld_cp(gb, wObjDirection, DIRECTION_RIGHT);
	bool swallow = ld_bit(gb, wInteractionSide, ZF ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F);
	if (!swallow) {
		LJUMP(Func_207ed);
		return;
	}
	gb->cpu.b = OBJACTION_11;
	LCALL(SetObjAction);
	FARCALL(SetState_Swallowed);
	RET();
}

PORT_JUMP(ObjInteraction_Scowler)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		LJUMP(Func_20899);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(ObjInteraction_WaterTeleporting);
		return;
	}
	LJUMP(Func_208f2);
}
