// Port of src/engine/level/object_interactions.asm
#include "wario_states.h"

#define ProcessInteractions_next_obj (ADDR_ProcessInteractions + 0x28)
#define ProcessInteractions_table (ADDR_ProcessInteractions + 0x150) // after the $ef

// a = [lo] + [hi]-style "ld a, [x] / add [hl] / sub e"
static uint8_t pi_edge(GB *gb, uint16_t pos, uint16_t box)
{
	SET_HL(gb, box);
	gb->cpu.a = alu_add(gb, rd8(gb, pos), rd8(gb, box));
	return gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.e);
}

// ProcessInteractions from .loop_objects (or .next_obj), up to the
// interaction's jumptable (or the ret at the end of the objects)
static void pi_run(GB *gb, bool next)
{
	uint16_t hl;
	if (next)
		goto next_obj;
loop_objects:
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, (uint8_t)((wObjects >> 8) + 1));
	if (ZF) {
		RET();
		return;
	}
	wr8(gb, wObjPtr + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, wObjPtr + 1, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, REG_HL(gb)), OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	alu_cp(gb, gb->cpu.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	if (ZF)
		goto is_visible;
next_obj:
	gb->cpu.a = rd8(gb, wObjPtr + 0);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = rd8(gb, wObjPtr + 1);
	gb->cpu.l = gb->cpu.a;
	SET_DE(gb, OBJ_STRUCT_LENGTH);
	SET_HL(gb, alu_add16(gb, REG_HL(gb), OBJ_STRUCT_LENGTH));
	goto loop_objects;

is_visible:
	hl = REG_HL(gb);
	gb_push16(gb, hl);
	gb->cpu.e = OBJ_UNK_1D;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, OBJ_UNK_1D);
	gb->cpu.a = rd8(gb, hl);
	hl--;
	wr8(gb, wUnused_c1b8, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	hl = gb_pop16(gb);
	SET_HL(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!ZF)
		goto next_obj;

	gb->cpu.e = OBJ_INTERACTION_TYPE;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, OBJ_INTERACTION_TYPE);
	static const uint16_t dst[] = {wObjInteractionType, wObjCollBoxTop, wObjCollBoxBottom,
		wObjCollBoxLeft, wObjCollBoxRight, wObjScreenYPos, wObjScreenXPos};
	for (int i = 0; i < 7; i++) {
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, dst[i], gb->cpu.a);
		if (i < 6)
			hl++;
	}
	gb->cpu.e = OBJ_SUBSTATE - OBJ_SCREEN_X_POS;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, OBJ_SUBSTATE - OBJ_SCREEN_X_POS);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.a = alu_bit(gb, gb->cpu.a, OBJSUBFLAG_HDIR_F) ? DIRECTION_RIGHT : DIRECTION_LEFT;
	wr8(gb, wObjDirection, gb->cpu.a);

	xor_a(gb);
	wr8(gb, wInteractionSide, 0);
	gb->cpu.a = 0xff;
	wr8(gb, wc1c2, 0xff);
	wr8(gb, wc1c1, 0xff);
	wr8(gb, wc1c3, 0xff);
	wr8(gb, wc1c4, 0xff);

	gb->cpu.e = 0xd0;
	gb->cpu.b = pi_edge(gb, wObjScreenYPos, wObjCollBoxTop);
	pi_edge(gb, wWarioScreenYPos, wCollisionBoxBottom);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	if (CF)
		goto next_obj;
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = pi_edge(gb, wWarioScreenYPos, wCollisionBoxTop);
	pi_edge(gb, wObjScreenYPos, wObjCollBoxBottom);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	if (CF)
		goto next_obj;
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, gb->cpu.d, gb->cpu.c);
	if (!CF) {
		gb->cpu.a = gb->cpu.c;
		wr8(gb, wc1c1, gb->cpu.a);
		if (ZF) {
			gb->cpu.a = gb->cpu.d;
			wr8(gb, wc1c2, gb->cpu.a);
		}
	} else {
		gb->cpu.a = gb->cpu.d;
		wr8(gb, wc1c2, gb->cpu.a);
	}

	gb->cpu.b = pi_edge(gb, wObjScreenXPos, wObjCollBoxLeft);
	pi_edge(gb, wWarioScreenXPos, wCollisionBoxRight);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	if (CF)
		goto next_obj;
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = pi_edge(gb, wWarioScreenXPos, wCollisionBoxLeft);
	pi_edge(gb, wObjScreenXPos, wObjCollBoxRight);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	if (CF)
		goto next_obj;
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.c);
	if (!CF) {
		gb->cpu.a = gb->cpu.c;
		wr8(gb, wc1c3, gb->cpu.a);
		if (ZF) {
			gb->cpu.a = gb->cpu.d;
			wr8(gb, wc1c4, gb->cpu.a);
		}
	} else {
		gb->cpu.a = gb->cpu.d;
		wr8(gb, wc1c4, gb->cpu.a);
	}

	{
		uint8_t side = INTERACTION_LEFT | INTERACTION_RIGHT | INTERACTION_UP | INTERACTION_DOWN;
		SET_HL(gb, wInteractionSide);
		gb->cpu.a = side;
		wr8(gb, wInteractionSide, side);
		gb->cpu.a = rd8(gb, wc1c4);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = rd8(gb, wc1c3);
		gb->cpu.c = gb->cpu.a;
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (!ZF) {
			if (!CF) {
				side &= (uint8_t)~INTERACTION_RIGHT;
			} else {
				side &= (uint8_t)~INTERACTION_LEFT;
				gb->cpu.b = gb->cpu.c;
			}
			wr8(gb, wInteractionSide, side);
		}
		gb->cpu.a = rd8(gb, wc1c2);
		gb->cpu.d = gb->cpu.a;
		gb->cpu.a = rd8(gb, wc1c1);
		gb->cpu.e = gb->cpu.a;
		alu_cp(gb, gb->cpu.a, gb->cpu.d);
		if (!ZF) {
			if (!CF) {
				side &= (uint8_t)~INTERACTION_UP;
			} else {
				side &= (uint8_t)~INTERACTION_DOWN;
				gb->cpu.d = gb->cpu.e;
			}
			wr8(gb, wInteractionSide, side);
		}
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, gb->cpu.d);
		if (!ZF) {
			if (!CF)
				side &= (uint8_t)~(INTERACTION_LEFT | INTERACTION_RIGHT);
			else
				side &= (uint8_t)~(INTERACTION_UP | INTERACTION_DOWN);
			wr8(gb, wInteractionSide, side);
		}
	}

	// .do_interaction
	gb->cpu.a = alu_and(gb, rd8(gb, wObjInteractionType), INTERACTION_MASK);
	// rst JumpTable
	gb_push16(gb, ProcessInteractions_table);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	hl = gb_pop16(gb);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

PORT_JUMP(ProcessInteractions)
{
	if (ld_and(gb, wIsIntangible) || ld_and(gb, wIsFloorTransition) || ld_and(gb, wAutoMoveState)) {
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wIsOnSteppableObject, 0);
	SET_HL(gb, wObjects);
	pi_run(gb, false);
}

PORT_LOCAL_JUMP(ProcessInteractions, next_obj)
{
	pi_run(gb, true);
}

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

// the chest interactions: open it with the matching key
static void obj_treasure(GB *gb, int key, int treasure, uint8_t screen)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F)) {
		LJUMP(Func_209ca);
		return;
	}
	if (ld_and(gb, wTransformation)) {
		LJUMP(Func_20a6f);
		return;
	}
	SET_HL(gb, wKeyAndTreasureFlags);
	if (!alu_bit(gb, rd8(gb, wKeyAndTreasureFlags), key)) {
		LJUMP(Func_20a6f);
		return;
	}
	wr8(gb, wKeyAndTreasureFlags, (uint8_t)(rd8(gb, wKeyAndTreasureFlags) | (1 << treasure)));
	st(gb, wLevelEndScreen, screen);
	LJUMP(GetTreasure);
}

PORT_JUMP(ObjInteraction_GreyTreasure) { obj_treasure(gb, GREY_KEY_F, GREY_TREASURE_F, LVLEND_GREY_TREASURE); }
PORT_JUMP(ObjInteraction_RedTreasure) { obj_treasure(gb, RED_KEY_F, RED_TREASURE_F, LVLEND_RED_TREASURE); }
PORT_JUMP(ObjInteraction_GreenTreasure) { obj_treasure(gb, GREEN_KEY_F, GREEN_TREASURE_F, LVLEND_GREEN_TREASURE); }
PORT_JUMP(ObjInteraction_BlueTreasure) { obj_treasure(gb, BLUE_KEY_F, BLUE_TREASURE_F, LVLEND_BLUE_TREASURE); }

// stepped on from above: Func_209ca; attacked: Func_217a9; else Func_20a6f.
// Returns true if it jumped (false: touched from below).
static bool solid_side(GB *gb)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		LJUMP(Func_209ca);
		return true;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F))
		return false;
	if (ld_and(gb, wAttackCounter))
		LJUMP(Func_217a9);
	else
		LJUMP(Func_20a6f);
	return true;
}

PORT_JUMP(ObjInteraction_Solid)
{
	if (solid_side(gb))
		return;
	LJUMP(Func_2168b);
}

PORT_JUMP(ObjInteraction_Omodon)
{
	if (solid_side(gb))
		return;
	if (ld_and(gb, wInvincibleCounter) || ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		RET();
		return;
	}
	gb->cpu.b = OBJACTION_FLATTEN;
	LCALL(SetObjAction);
	FARCALL(SetState_FlatAirborne);
	RET();
}

PORT_JUMP(Func_21abb)
{
	gb->cpu.b = OBJACTION_ATTACK;
	LCALL(SetObjAction);
	LJUMP(Func_20a6f);
}

PORT_JUMP(Func_21f01)
{
	if (!ld_bit(gb, wInteractionSide, INTERACTION_UP_F)) {
		LJUMP(Func_20d8c);
		return;
	}
	gb->cpu.a = alu_or(gb, rd8(gb, wInteractionSide), INTERACTION_LEFT);
	wr8(gb, wInteractionSide, gb->cpu.a);
	st(gb, wDirection, DIRECTION_RIGHT);
	FARCALL(SetState_SwimKnockBack);
	RET();
}

PORT_JUMP(Func_20602)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_INVISIBLE_WARIO);
	if (!ZF && alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// has transformation except invisible
		LJUMP(VanishObjectAndKnockBackIfZombie);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_STATE_MASK);
	alu_cp(gb, gb->cpu.a, GRAB_IDLE);
	if (ZF) {
		FARCALL(Func_1ede9);
	} else {
		xor_a(gb);
		wr8(gb, wGrabState, 0);
		FARCALL(StartJump_FromInput);
	}
	st(gb, wDoFullJump, TRUE);
	st(gb, wJumpVelIndex, 0x0a);
	if (ld_bit(gb, wJoypadDown, B_PAD_A)) {
		ld_cp(gb, wPowerUpLevel, HIGH_JUMP_BOOTS);
		if (!CF) {
			xor_a(gb);
			wr8(gb, wJumpVelIndex, 0);
		}
	}
	gb->cpu.b = OBJACTION_VANISH_TOUCH;
	LJUMP(SetObjAction); // fallthrough
}

PORT_JUMP(Func_21aac)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wIsSmashAttacking));
	if (!ZF) {
		LJUMP(Func_209ca);
		return;
	}
	// smash attack object
	LCALL(Func_20602);
	gb->cpu.b = OBJACTION_SMASH_ATTACK_WALKABLE;
	LJUMP(SetObjAction);
}

PORT_JUMP(ObjInteraction_Stove)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		LJUMP(Func_21aac);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(Func_2168b);
		return;
	}
	if (ld_and(gb, wAttackCounter))
		LJUMP(Func_21abb);
	else
		LJUMP(Func_20a6f);
}

PORT_JUMP(ObjInteraction_WaterDrop)
{
	gb->cpu.b = OBJACTION_06;
	gb_call(gb, ADDR_SetObjAction);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ProcessInteractions_next_obj);
		return;
	}
	JUMP(RecoverFromTransformation);
}

// ld a, [wWarioScreenXPos] / ld b, a / ld a, [wObjScreenXPos] / cp b, then
// sets INTERACTION_LEFT (object left of Wario) or INTERACTION_RIGHT
static void side_by_x(GB *gb)
{
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	gb->cpu.a = rd8(gb, wObjScreenXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	uint8_t bit = CF ? INTERACTION_LEFT : INTERACTION_RIGHT;
	gb->cpu.a = alu_or(gb, rd8(gb, wInteractionSide), bit);
	wr8(gb, wInteractionSide, gb->cpu.a);
}

// Func_2023b.asm_20257
static void func_2023b_bump(GB *gb)
{
	if (ld_and(gb, wAttackCounter)) {
		LJUMP(AttackObject);
		return;
	}
	gb->cpu.b = OBJACTION_BUMP;
	LCALL(SetObjAction);
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF) {
		RET(); // is invincible
		return;
	}
	ld_cp(gb, wTransformation, TRANSFORMATION_FLAT_WARIO);
	if (ZF) {
		LJUMP(Func_2028a);
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	gb->cpu.a = rd8(gb, wObjScreenXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	st(gb, wDirection, CF ? DIRECTION_RIGHT : DIRECTION_LEFT);
	LJUMP(SetState_EnemyBumping);
}

PORT_JUMP(Func_2023b)
{
	if (!ld_and(gb, wTouchState)) {
		func_2023b_bump(gb);
		return;
	}
	uint8_t a = gb->cpu.a;
	alu_cp(gb, a, TOUCH_BUMP);
	if (ZF) {
		func_2023b_bump(gb);
		return;
	}
	alu_cp(gb, a, TOUCH_VULNERABLE);
	if (ZF) {
		LJUMP(Func_20939); // this causes an infinite loop!
		return;
	}
	alu_cp(gb, a, TOUCH_ATTACK);
	if (ZF) {
		LJUMP(AttackObject);
		return;
	}
	alu_cp(gb, a, TOUCH_VANISH);
	if (ZF) {
		LJUMP(VanishObjectAndKnockBackIfZombie);
		return;
	}
	JUMP_TO(ProcessInteractions_next_obj);
}

PORT_JUMP(Func_2022c)
{
	if (ld_and(gb, wIsRolling)) {
		LJUMP(AttackObject);
		return;
	}
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(StepOnObject);
		return;
	}
	LJUMP(Func_2023b); // fallthrough
}

PORT_JUMP(Func_20939)
{
	if (ld_and(gb, wTouchState)) {
		LJUMP(Func_2023b);
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF) {
		RET();
		return;
	}
	FARCALL(DoJumpingBump);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	xor_a(gb);
	wr8(gb, wGrabState, 0);
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	gb->cpu.a = rd8(gb, wObjScreenXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!CF) {
		st(gb, wInteractionSide, INTERACTION_RIGHT);
		st(gb, wDirection, DIRECTION_LEFT);
		set_frameset(gb, ADDR_Frameset_14a79);
	} else {
		st(gb, wInteractionSide, INTERACTION_LEFT);
		st(gb, wDirection, DIRECTION_RIGHT);
		set_frameset(gb, ADDR_Frameset_14a76);
	}
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(Func_20932)
{
	if (ld_and(gb, wAttackCounter)) {
		LJUMP(AttackObject);
		return;
	}
	LJUMP(Func_20939); // fallthrough
}

PORT_JUMP(StepOnObject)
{
	// no stun if Wario is invincible
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF) {
		RET();
		return;
	}
	gb->cpu.a = alu_dec(gb, rd8(gb, wIsSmashAttacking));
	if (ZF) {
		// Wario is smash attacking
		ld_cp(gb, wTransformation, TRANSFORMATION_FAT_WARIO);
		if (ZF)
			LJUMP(VanishObjectAndKnockBackIfZombie);
		else
			LJUMP(Func_20602);
		return;
	}
	if (ld_and(gb, wTransformation)) {
		// .Transformed
		ld_cp(gb, wTransformation, TRANSFORMATION_HOT_WARIO);
		if (ZF) {
			ld_cp(gb, wTransformationProgress, 2);
			if (!CF) {
				LJUMP(VanishObjectAndKnockBackIfZombie);
				return;
			}
		}
		ld_cp(gb, wTransformation, TRANSFORMATION_FAT_WARIO);
		if (ZF) {
			LJUMP(AttackObject);
			return;
		}
		side_by_x(gb);
		play_sfx(gb, SFX_014);
		gb->cpu.b = OBJACTION_WOBBLE;
		LCALL(SetObjAction);
		ld_cp(gb, wTransformation, TRANSFORMATION_FLAT_WARIO);
		if (ZF) {
			LJUMP(Func_2028a);
			return;
		}
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_BOUNCY_WARIO);
		if (ZF) {
			ld_cp(gb, wTouchState, TOUCH_PASS_THROUGH);
			if (!ZF)
				FARCALL(Func_2a0b2);
		}
		RET();
		return;
	}

	// not transformed
	xor_a(gb);
	wr8(gb, wAttackCounter, 0);
	// stepping on object while rolling is attacking
	if (ld_and(gb, wIsRolling)) {
		LJUMP(AttackObject);
		return;
	}
	side_by_x(gb);
	bool below = ld_bit(gb, wInteractionSide, INTERACTION_DOWN_F);
	if (!below) {
		gb->cpu.b = rd8(gb, wWarioScreenYPos);
		gb->cpu.a = rd8(gb, wObjScreenYPos);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		below = CF;
	}
	if (!below) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_STATE_MASK);
		alu_cp(gb, gb->cpu.a, GRAB_IDLE);
		if (ZF) {
			FARCALL(Func_1ede9); // .grabbing
		} else {
			xor_a(gb);
			wr8(gb, wGrabState, 0);
			if (ld_and(gb, wIsCrouching)) {
				FARCALL(StartCrouchJump);
			} else if (ld_and(gb, wJoypadDown)) {
				FARCALL(StartJump_FromInput);
			} else {
				// .no_input
				LCALL(Func_20939);
				goto sfx;
			}
		}
		st(gb, wDoFullJump, TRUE);
		st(gb, wJumpVelIndex, 0x0a);
		if (ld_bit(gb, wJoypadDown, B_PAD_A)) {
			ld_cp(gb, wPowerUpLevel, HIGH_JUMP_BOOTS);
			if (!CF) {
				xor_a(gb); // full jump
				wr8(gb, wJumpVelIndex, 0);
			}
		}
	} else {
		// .asm_20506
		ld_cp(gb, wInvincibleCounter, 0x01);
		if (ZF) {
			RET();
			return;
		}
		if (ld_and(gb, wIsCrouching)) {
			if (ld_and(gb, wJumpVelTable))
				FARCALL(StartCrouchFall);
		} else {
			gb->cpu.a = alu_and(gb, rd8(gb, wGrabState), GRAB_STATE_MASK);
			alu_cp(gb, gb->cpu.a, GRAB_IDLE);
			if (ZF) {
				if (ld_and(gb, wJumpVelTable))
					FARCALL(Func_1edd3);
			} else {
				xor_a(gb);
				wr8(gb, wGrabState, 0);
				if (ld_and(gb, wJumpVelTable))
					FARCALL(StartFall);
				else
					FARCALL(SetState_Idling);
			}
		}
	}
sfx:
	play_sfx(gb, SFX_014);
	gb->cpu.b = OBJACTION_WOBBLE;
	LJUMP(SetObjAction);
}

PORT_JUMP(Func_20670)
{
	if (ld_and(gb, wInvincibleCounter)) {
		LJUMP(Func_20939);
		return;
	}
	// if Wario is rolling, do attack
	ld_cp(gb, wPowerUpLevel, GARLIC);
	bool check_rolling = !CF;
	if (!check_rolling)
		check_rolling = !ld_bit(gb, wObjInteractionType, HEAVY_OBJ_F);
	if (check_rolling && ld_and(gb, wIsRolling)) {
		LJUMP(AttackObject);
		return;
	}
	// .asm_2068c
	if (ld_and(gb, wStingTouchState)) {
		uint8_t a = gb->cpu.a;
		alu_cp(gb, a, TOUCH_VULNERABLE);
		if (!ZF) {
			alu_cp(gb, a, TOUCH_BUMP);
			if (ZF) {
				LJUMP(Func_2023b);
				return;
			}
			alu_cp(gb, a, TOUCH_ATTACK);
			if (ZF) {
				LJUMP(AttackObject);
				return;
			}
			alu_cp(gb, a, TOUCH_VANISH);
			if (ZF) {
				LJUMP(VanishObjectAndKnockBackIfZombie);
				return;
			}
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
	}
	// .asm_206a8
	side_by_x(gb);
	if (!ld_bit(gb, wInteractionSide, INTERACTION_RIGHT_F))
		st(gb, wDirection, DIRECTION_RIGHT);
	else
		st(gb, wDirection, DIRECTION_LEFT);
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	ld_cp(gb, wInvincibleCounter, 0x01);
	if (ZF) {
		RET();
		return;
	}
	ld_cp(gb, wStingTouchState, TOUCH_VULNERABLE);
	if (ZF)
		LJUMP(Func_206eb);
	else
		LJUMP(SetState_Stung);
}

// Func_20b6b.set_wario_pick_up
static void set_wario_pick_up(GB *gb)
{
	play_sfx(gb, SFX_GRAB);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	st(gb, wWarioState, WST_PICKING_UP);
	collision_box(gb, -1, -27, -9, 9);
	load_gfx(gb, BANK_WarioThrowGfx, ADDR_WarioThrowGfx);
	set_oam(gb, BANK_OAM_1606a, ADDR_OAM_1606a);
	uint16_t fs;
	if (!ld_and(gb, wDirection)) {
		if (ld_bit(gb, wWarioSlopeInteraction, LEFT_SLOPE_F))
			goto grab_idling;
		fs = ld_mask(gb, wGrabState, 1 << GRAB_HEAVY_F) ? ADDR_Frameset_163f5 : ADDR_Frameset_163ec;
	} else {
		if (ld_bit(gb, wWarioSlopeInteraction, RIGHT_SLOPE_F))
			goto grab_idling;
		fs = ld_mask(gb, wGrabState, 1 << GRAB_HEAVY_F) ? ADDR_Frameset_16415 : ADDR_Frameset_1640c;
	}
	set_frameset(gb, fs);
	anim(gb, OAM1);
	RET();
	return;
grab_idling:
	FARCALL(SetState_GrabIdling);
	RET();
}

PORT_JUMP(Func_20b6b)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F))
		goto step_on_obj;
	if (ld_and(gb, wTouchState)) {
		uint8_t a = gb->cpu.a;
		alu_cp(gb, a, TOUCH_ATTACK);
		if (ZF) {
			LJUMP(AttackObject);
			return;
		}
		alu_cp(gb, a, TOUCH_VANISH);
		if (ZF) {
			LJUMP(VanishObjectAndKnockBackIfZombie);
			return;
		}
		alu_cp(gb, a, TOUCH_PASS_THROUGH);
		if (ZF) {
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
		goto step_on_obj;
	}
	// .check_water
	ld_cp(gb, wWarioState, WST_DIVING);
	if (!CF) {
		alu_cp(gb, gb->cpu.a, WST_TRYING_SUBMERGE + 1);
		if (CF) {
			LJUMP(Func_20899);
			return;
		}
	}
	// .not_in_water
	gb->cpu.b = ld(gb, wIsRolling);
	gb->cpu.a = alu_or(gb, rd8(gb, wAttackCounter), gb->cpu.b);
	if (!ZF) {
		LJUMP(AttackObject);
		return;
	}
	gb->cpu.b = ld(gb, wIsCrouching);
	gb->cpu.a = alu_or(gb, rd8(gb, wGrabState), gb->cpu.b);
	if (!ZF)
		goto step_on_obj;
	ld_cp(gb, wPowerUpLevel, GRAB_GLOVE);
	if (CF)
		goto step_on_obj;
	ld_cp(gb, wWarioState, WST_LADDER_SCRATCHING);
	if (ZF)
		goto step_on_obj;
	alu_cp(gb, gb->cpu.a, WST_LADDER_CLIMBING);
	if (!CF) {
		alu_cp(gb, gb->cpu.a, WST_LADDER_SLIDING + 1);
		if (CF)
			goto step_on_obj;
	}
	// .not_on_ladder
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	gb->cpu.a = rd8(gb, wObjScreenXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!CF) {
		st(gb, wInteractionSide, INTERACTION_RIGHT);
		if (!ld_and(gb, wDirection))
			goto step_on_obj;
	} else {
		st(gb, wInteractionSide, INTERACTION_LEFT);
		if (ld_and(gb, wDirection))
			goto step_on_obj;
	}
	// .check_invincible_counter
	ld_cp(gb, wInvincibleCounter, 1);
	if (ZF) {
		RET();
		return;
	}
	{
		uint16_t hl = wWarioXPos + 1;
		uint16_t de = hXPosLo;
		gb->cpu.a = rd8(gb, hl--);
		wr8(gb, de--, gb->cpu.a);
		gb->cpu.a = rd8(gb, hl--);
		wr8(gb, de--, gb->cpu.a);
		gb->cpu.a = alu_sub(gb, rd8(gb, hl--), 0x08);
		wr8(gb, de--, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl), 0x00);
		wr8(gb, de, gb->cpu.a);
		SET_HL(gb, hl);
		SET_DE(gb, de);
	}
	if (!ld_bit(gb, wObjInteractionType, HEAVY_OBJ_F)) {
		st(gb, wGrabState, GRAB_PICK_UP);
	} else {
		ld_cp(gb, wPowerUpLevel, SUPER_GRAB_GLOVES);
		if (CF)
			goto step_on_obj;
		st(gb, wGrabState, GRAB_PICK_UP | (1 << GRAB_HEAVY_F));
	}
	// .set_obj_pick_up
	gb->cpu.b = OBJACTION_PICK_UP;
	LCALL(SetObjAction);
	if (!ld_and(gb, wJumpVelTable)) {
		set_wario_pick_up(gb);
		return;
	}
	if (ld_bit(gb, wObjInteractionType, HEAVY_OBJ_F))
		goto step_on_obj;
	FARCALL(Func_1edd3);
	RET();
	return;
step_on_obj:
	LJUMP(StepOnObject);
}

PORT_JUMP(Func_20b41)
{
	if (ld_and(gb, wTouchState)) {
		uint8_t a = gb->cpu.a;
		alu_cp(gb, a, TOUCH_BUMP);
		if (!ZF) {
			alu_cp(gb, a, TOUCH_VULNERABLE);
			if (!ZF) {
				alu_cp(gb, a, TOUCH_ATTACK);
				if (ZF) {
					LJUMP(AttackObject);
					return;
				}
				alu_cp(gb, a, TOUCH_VANISH);
				if (ZF) {
					LJUMP(VanishObjectAndKnockBackIfZombie);
					return;
				}
			}
		}
		LJUMP(Func_20a6f);
		return;
	}
	// .asm_20b5e
	if (ld_and(gb, wTouchState)) {
		RET();
		return;
	}
	LCALL(Func_20939);
	gb->cpu.b = OBJACTION_12;
	LJUMP(SetObjAction);
}

PORT_JUMP(Func_20d1d)
{
	LJUMP(Func_2022c);
}

// sting on the object's front (front = true) or back
static void obj_sting(GB *gb, bool front)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(StepOnObject);
		return;
	}
	ld_cp(gb, wObjDirection, DIRECTION_RIGHT);
	int bit = (ZF != 0) == front ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
	if (ld_bit(gb, wInteractionSide, bit))
		LJUMP(Func_20670);
	else
		LJUMP(Func_2022c);
}

PORT_JUMP(ObjInteraction_FrontSting) { obj_sting(gb, true); }
PORT_JUMP(ObjInteraction_BackSting) { obj_sting(gb, false); }

PORT_JUMP(ObjInteraction_TopSting)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		LJUMP(Func_20670);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(StepOnObject);
		return;
	}
	LJUMP(Func_2022c);
}

PORT_JUMP(ObjInteraction_FullSting)
{
	LJUMP(Func_20670);
}

PORT_JUMP(Func_20e39)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(StepOnObject);
		return;
	}
	ld_cp(gb, wObjDirection, DIRECTION_RIGHT);
	int bit = ZF ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
	if (ld_bit(gb, wInteractionSide, bit))
		LJUMP(Func_20939);
	else
		LJUMP(Func_2022c);
}

PORT_JUMP(Func_20e60)
{
	LJUMP(Func_20939);
}

PORT_JUMP(Func_20e6a)
{
	LCALL(Func_20939);
	if (ld_and(gb, wTouchState)) {
		RET();
		return;
	}
	gb->cpu.b = OBJACTION_0A;
	LJUMP(SetObjAction);
}

PORT_JUMP(Func_20e63)
{
	if (ld_and(gb, wAttackCounter)) {
		LJUMP(AttackObject);
		return;
	}
	LJUMP(Func_20e6a); // fallthrough
}

PORT_JUMP(ObjInteraction_Walkable)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F))
		LJUMP(Func_21aac);
	else
		LJUMP(Func_20932);
}

PORT_JUMP(ObjInteraction_Fire)
{
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	st(gb, wTransformation, TRANSFORMATION_HOT_WARIO);
	st(gb, wTransformationProgress, 1);
	touch_states(gb, TOUCH_BUMP, 0x02);
	st(gb, wTransformationDuration + 0, (uint8_t)(HOT_WARIO_DURATION >> 8));
	st(gb, wTransformationDuration + 1, (uint8_t)HOT_WARIO_DURATION);
	CALL(UpdateLevelMusic);
	LJUMP(SetState_OnFire_ResetStateCounter); // fallthrough
}

PORT_JUMP(Func_2126a)
{
	if (ld_and(gb, wIsCrouching)) {
		st(gb, wCollisionBoxTop, (uint8_t)-27);
		FARCALL(CheckUpCollision);
		if (b_and(gb)) {
			st(gb, wCollisionBoxTop, (uint8_t)-15);
			LJUMP(Func_2022c);
			return;
		}
	}
	// .asm_21290
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2023b);
		return;
	}
	gb->cpu.b = rd8(gb, wWarioScreenXPos);
	gb->cpu.a = rd8(gb, wObjScreenXPos);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	st(gb, wDirection, CF ? DIRECTION_RIGHT : DIRECTION_LEFT);
	// .set_ball_o_string
	play_sfx(gb, SFX_039);
	st(gb, wTransformation, TRANSFORMATION_BALL_O_STRING_WARIO);
	xor_a(gb);
	wr8(gb, wTransformationProgress, 0);
	touch_states(gb, TOUCH_ATTACK, 0x01);
	CALL(UpdateLevelMusic);
	st(gb, wWarioState, WST_GETTING_WRAPPED_IN_STRING);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
		wIsSmashAttacking, 0};
	clear(gb, zero);
	collision_box(gb, -1, -27, -9, 9);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	SET_HL(gb, ADDR_WarioStringPal);
	CALL(SetWarioPal);
	load_gfx(gb, BANK_WarioStringGfx, ADDR_WarioStringGfx);
	set_oam(gb, BANK_OAM_171c0, ADDR_OAM_171c0);
	set_frameset(gb, ADDR_Frameset_17414);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(ObjInteraction_Silky)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(StepOnObject);
		return;
	}
	ld_cp(gb, wObjDirection, DIRECTION_RIGHT);
	int bit = ZF ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
	if (ld_bit(gb, wInteractionSide, bit))
		LJUMP(Func_2126a);
	else
		LJUMP(Func_2022c);
}

// Whether Wario touched the object's front (front) or back:
// ld a, [wObjDirection] / cp DIRECTION_RIGHT / jr z (use_cp), or and a / jr nz,
// then ld a, [wInteractionSide] / bit INTERACTION_LEFT_F or _RIGHT_F, a
static bool obj_side(GB *gb, bool use_cp, bool front)
{
	bool right;
	if (use_cp) {
		ld_cp(gb, wObjDirection, DIRECTION_RIGHT);
		right = ZF != 0;
	} else {
		right = ld_and(gb, wObjDirection);
	}
	return ld_bit(gb, wInteractionSide, right == front ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F);
}

// ld a, [wInteractionSide] / and INTERACTION_UP | INTERACTION_DOWN / jp nz, StepOnObject
static bool vert_step(GB *gb)
{
	if (!ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN))
		return false;
	LJUMP(StepOnObject);
	return true;
}

static void zero_pals_hblank(GB *gb, uint16_t index_reg, uint8_t index, uint8_t n, uint16_t data_reg)
{
	gb->cpu.a = index;
	wr8(gb, index_reg, index);
	gb->cpu.b = n;
	gb->cpu.c = (uint8_t)data_reg;
	do {
		// wait_not_hblank / wait_hblank
		while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		xor_a(gb);
		for (int i = 0; i < 2 * PAL_SIZE; i++)
			wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), 0);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
}

PORT_JUMP(ObjInteraction_Teruteru)
{
	if (!ld_bit(gb, wInteractionSide, INTERACTION_DOWN_F)) {
		LJUMP(Func_2022c);
		return;
	}
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_BLIND);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_VULNERABLE);
	st(gb, wca94, 0x01);
	FARCALL(SetState_BlindIdling);
	zero_pals_hblank(gb, rBGPI, BGPI_AUTOINC | (0 * PAL_SIZE), 4, rBGPD);
	zero_pals_hblank(gb, rOBPI, OBPI_AUTOINC | (3 * PAL_SIZE), 2, rOBPD);
	RET();
}

PORT_JUMP(ObjInteraction_Beam)
{
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_SPLIT);
	touch_states(gb, TOUCH_PASS_THROUGH, 0x02);
	FARCALL(SetState_SplitHit);
	RET();
}

PORT_JUMP(Func_21b3a)
{
	LCALL(Func_20939);
	gb->cpu.b = OBJACTION_NONE;
	LJUMP(SetObjAction);
}

PORT_JUMP(Func_21b78)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F) && ld_and(gb, wIsSmashAttacking)) {
		LJUMP(Func_20602);
		return;
	}
	LJUMP(Func_20939);
}

PORT_JUMP(ObjInteraction_YellowBellyArrow)
{
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
	if (ZF) {
		LJUMP(Func_2022c);
		return;
	}
	ld_cp(gb, wTransformation, TRANSFORMATION_OWL_WARIO);
	if (ZF)
		CALL(ReleaseOwl);
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	FARCALL(SetState_PuffyInflating);
	RET();
}

PORT_JUMP(ObjInteraction_Pneumo)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_DOWN_F))
		LJUMP(ObjInteraction_YellowBellyArrow);
	else
		LJUMP(Func_2022c);
}

PORT_JUMP(Func_2160a)
{
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BOUNCY_WARIO);
	if (ZF) {
		LJUMP(Func_2022c);
		return;
	}
	st(gb, wTransformation, TRANSFORMATION_BOUNCY_WARIO);
	touch_states(gb, TOUCH_BUMP, 0x01);
	st(gb, wTransformationDuration + 0, (uint8_t)(BOUNCY_WARIO_DURATION >> 8));
	st(gb, wTransformationDuration + 1, (uint8_t)BOUNCY_WARIO_DURATION);
	FARCALL(SetState_BouncyStart);
	RET();
}

PORT_JUMP(ObjInteraction_Bouncy)
{
	if (vert_step(gb))
		return;
	if (obj_side(gb, true, true))
		LJUMP(Func_2160a);
	else
		LJUMP(Func_2022c);
}

PORT_JUMP(ObjInteraction_FireBot)
{
	if (vert_step(gb))
		return;
	if (obj_side(gb, true, true))
		LJUMP(ObjInteraction_Fire);
	else
		LJUMP(Func_2022c);
}

PORT_JUMP(Func_21c56)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F))
		LJUMP(Func_21aac);
	else
		LJUMP(Func_20b6b);
}

PORT_JUMP(ObjInteraction_Tadpole)
{
	if (vert_step(gb))
		return;
	if (obj_side(gb, false, true))
		LJUMP(ObjInteraction_RollingRock);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(ObjInteraction_Hebarii)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(Func_20e6a);
		return;
	}
	if (ld_and(gb, wAttackCounter))
		LJUMP(AttackObject);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(ObjInteraction_PrinceFroggy)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F))
		LJUMP(Func_209ca);
	else if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F))
		LJUMP(Func_2168b);
	else
		LJUMP(Func_20b41);
}

PORT_JUMP(Func_21d3b)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F) || alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(Func_20e6a);
		return;
	}
	if (obj_side(gb, false, true))
		LJUMP(Func_20670);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(Func_21d64)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F))
		LJUMP(Func_20670);
	else
		LJUMP(Func_20e6a);
}

// Func_21d6f.asm_21d7e
static void func_21d7e(GB *gb)
{
	if (ld_and(gb, wIsSmashAttacking))
		LJUMP(Func_20602);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(Func_21d6f)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F))
		func_21d7e(gb);
	else if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F))
		LJUMP(Func_20670);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(Func_21f28)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		func_21d7e(gb);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(Func_20e6a);
		return;
	}
	if (obj_side(gb, false, true))
		LJUMP(Func_20670);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(Func_21dac)
{
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	gb->cpu.a = rd8(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	FARCALL(SetState_Launched);
	gb->cpu.b = OBJACTION_06;
	LJUMP(SetObjAction);
}

PORT_JUMP(Func_21d88)
{
	if (ld_mask(gb, wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
		LJUMP(Func_20899);
		return;
	}
	if (obj_side(gb, false, true))
		LJUMP(Func_21dac);
	else
		LJUMP(Func_208f2);
}

PORT_JUMP(Func_21ea6)
{
	gb->cpu.a = rd8(gb, wObjDirection);
	gb->cpu.b = gb->cpu.a;
	bool right = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (ld_bit(gb, wInteractionSide, right ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F)) {
		gb->cpu.a = alu_xor(gb, rd8(gb, wObjDirection), 0x1);
		wr8(gb, wDirection, gb->cpu.a);
		LJUMP(Func_20a6f);
		return;
	}
	LJUMP(Func_21b3a);
}

PORT_JUMP(Func_21ecd)
{
	if (!obj_side(gb, false, true)) {
		LJUMP(Func_20e6a);
		return;
	}
	play_sfx(gb, SFX_FAT_WALK);
	FARCALL(SetState_DraggedDown);
	LJUMP(Func_21cc9);
}

PORT_JUMP(ObjInteraction_MagicalSpiral)
{
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_MAGIC);
	touch_states(gb, TOUCH_PASS_THROUGH, 0x02);
	FARCALL(SetState_MagicRising);
	RET();
}

PORT_JUMP(ObjInteraction_BottomSting)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F))
		LJUMP(Func_20670);
	else if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F))
		LJUMP(StepOnObject);
	else
		LJUMP(Func_2022c);
}

PORT_JUMP(ObjInteraction_AnonsterSilk)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F))
		LJUMP(StepOnObject);
	else
		LJUMP(ObjInteraction_AnonsterWave); // fallthrough
}

PORT_JUMP(ObjInteraction_AnonsterWave)
{
	LJUMP(Func_2126a);
}

PORT_JUMP(ObjInteraction_RoboMouse)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F)) {
		if (ld_and(gb, wIsSmashAttacking))
			LJUMP(Func_20602);
		else
			LJUMP(Func_20e6a);
		return;
	}
	if (obj_side(gb, false, true))
		LJUMP(ObjInteraction_Jamano);
	else
		LJUMP(Func_20e6a);
}

PORT_JUMP(Func_21af1)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		LJUMP(Func_209ca);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(StepOnObject);
		return;
	}
	if (ld_and(gb, wAttackCounter))
		LJUMP(Func_217a9);
	else
		LJUMP(Func_20a6f);
}

PORT_JUMP(ObjInteraction_DollBoyHammer)
{
	LJUMP(Func_2160a);
}

PORT_JUMP(ObjInteraction_Wormwould)
{
	if (vert_step(gb))
		return;
	if (ld_and(gb, wAttackCounter))
		LJUMP(AttackObject);
	else
		LJUMP(Func_21b3a); // fallthrough
}

PORT_JUMP(ObjInteraction_Turtle)
{
	if (ld_bit(gb, wInteractionSide, INTERACTION_UP_F)) {
		LJUMP(Func_209ca);
		return;
	}
	if (ld_and(gb, wTransformation)) {
		RET();
		return;
	}
	if (!ld_and(gb, wWaterInteraction)) {
		LJUMP(Func_21b3a);
		return;
	}
	FARCALL(StartDive);
	RET();
}

PORT_JUMP(ObjInteraction_Shoot)
{
	ld_cp(gb, wTransformation, TRANSFORMATION_BALL);
	if (ZF) {
		LJUMP(Func_21e8a);
		return;
	}
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (!alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F))
			LJUMP(StepOnObject);
		else
			LJUMP(Func_20e63);
		return;
	}
	// .interaction_down
	if (ld_and(gb, wInvincibleCounter)) {
		RET(); // skip if invincible
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	// turn Wario into ball form
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_BALL);
	touch_states(gb, TOUCH_PASS_THROUGH, 0x02);
	FARCALL(SetState_BallStart);
	RET();
}

PORT_JUMP(Func_21e9c)
{
	ld_cp(gb, wTransformation, TRANSFORMATION_BALL);
	if (ZF)
		LJUMP(Func_21e8a);
	else
		LJUMP(Func_20939);
}

PORT_JUMP(ObjInteraction_Richtertoffen)
{
	gb->cpu.a = rd8(gb, wInteractionSide);
	if (alu_bit(gb, gb->cpu.a, INTERACTION_UP_F)) {
		gb->cpu.a = alu_dec(gb, rd8(gb, wIsSmashAttacking));
		if (ZF)
			LJUMP(Func_20602);
		else
			LJUMP(StepOnObject);
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, INTERACTION_DOWN_F)) {
		LJUMP(Func_20932);
		return;
	}
	// .try_flatten_wario
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	st(gb, wCollisionBoxLeft, (uint8_t)-11);
	st(gb, wCollisionBoxRight, 11);
	ld_cp(gb, wLadderInteraction, LADDER_UNDERNEATH);
	if (!ZF) {
		FARCALL(Func_197b1);
		if (!b_and(gb)) {
			FARCALL(Func_19741);
			if (!b_and(gb)) {
				gb->cpu.b = OBJACTION_FLATTEN;
				LCALL(SetObjAction);
				FARCALL(SetState_FlatAirborne);
				RET();
				return;
			}
		}
	}
	st(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
	LJUMP(StepOnObject);
}

PORT_JUMP(ObjInteraction_Omodonmeka)
{
	if (ld_and(gb, wJumpVelTable)) {
		LJUMP(Func_2022c);
		return;
	}
	gb->cpu.b = OBJACTION_0C;
	LCALL(SetObjAction);
	ld_cp(gb, wTransformation, TRANSFORMATION_FLAT_WARIO);
	if (ZF) {
		LJUMP(SetState_FlatStretching);
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		LJUMP(Func_2022c);
		return;
	}
	// pick Wario up
	st(gb, wAutoMoveState, 0x01);
	st(gb, wWarioState, WST_PICKED_UP);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wIsSmashAttacking, wAttackCounter, wIsCrouching,
		wGrabState, wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
	clear(gb, zero);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	st(gb, wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	frameset_by_dir(gb, ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
	anim(gb, OAM1);
	RET();
}

PORT_JUMP(ObjInteraction_OrangeBird)
{
	if (vert_step(gb))
		return;
	if (!obj_side(gb, true, true)) {
		LJUMP(Func_2022c);
		return;
	}
	// .asm_217de
	if (ld_and(gb, wInvincibleCounter)) {
		RET();
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		LJUMP(Func_2022c);
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_CRAZY_WARIO);
	if (ZF) {
		LJUMP(Func_2022c);
		return;
	}
	gb->cpu.b = OBJACTION_06;
	LCALL(SetObjAction);
	st(gb, wTransformation, TRANSFORMATION_CRAZY_WARIO);
	touch_states(gb, TOUCH_ATTACK, 0x01);
	FARCALL(SetState_CrazySpinning);
	RET();
}

PORT_JUMP(ObjInteraction_Owl)
{
	if (!ld_bit(gb, wInteractionSide, INTERACTION_DOWN_F)) {
		LJUMP(Func_20939);
		return;
	}
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_OWL_WARIO);
	if (ZF) {
		RET();
		return;
	}
	LCALL(Func_20a63);
	play_sfx(gb, SFX_HANG_GRAB);
	st(gb, wTransformation, TRANSFORMATION_OWL_WARIO);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_VULNERABLE);
	st(gb, wca94, 0x01);
	st(gb, wWarioState, WST_GRABBING_OWL);
	collision_box(gb, -1, -27, -9, 9);
	static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
		wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
		wJumpVelIndex, 0};
	clear(gb, zero);
	CALL(UpdateLevelMusic);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	load_gfx(gb, BANK_WarioHangGfx, ADDR_WarioHangGfx);
	set_oam(gb, BANK_OAM_1fddb4, ADDR_OAM_1fddb4);
	gb->cpu.a = rd8(gb, wObjDirection);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, ADDR_Frameset_1fed53);
	anim(gb, OAM2);
	RET();
}
