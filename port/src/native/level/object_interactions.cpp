// Wario's interactions with objects (src/engine/level/object_interactions.asm):
// ProcessInteractions finds the objects Wario touches and runs the
// interaction of each (by its type): collecting coins, keys, treasure and
// musical coins, being stung or bumped, stepping on, grabbing and
// attacking objects, the transformations objects cause, and more.
//
// Called by the level's frame and register-level C, so these keep the
// original's registers, flags and return addresses (see asm_module.hpp).
#include "asm_module.hpp"

namespace wl3::level {

namespace {

class ObjectInteractions : public AsmModule {
public:
	using AsmModule::AsmModule;

	#define ProcessInteractions_next_obj (ADDR_ProcessInteractions + 0x28)
	#define ProcessInteractions_table (ADDR_ProcessInteractions + 0x150) // after the $ef

	// a = [lo] + [hi]-style "ld a, [x] / add [hl] / sub e"
	uint8_t pi_edge(uint16_t pos, uint16_t box)
	{
		SET_HL(gb_, box);
		cpu_.a = alu_.add(rd8(pos), rd8(box));
		return cpu_.a = alu_.sub(cpu_.a, cpu_.e);
	}

	// ProcessInteractions from .loop_objects (or .next_obj), up to the
	// interaction's jumptable (or the ret at the end of the objects)
	void pi_run(bool next)
	{
		uint16_t hl;
		if (next)
			goto next_obj;
	loop_objects:
		cpu_.a = cpu_.h;
		alu_.cp(cpu_.a, (uint8_t)((wObjects >> 8) + 1));
		if (ZF) {
			RET();
			return;
		}
		wr8(wObjPtr + 0, cpu_.a);
		cpu_.a = cpu_.l;
		wr8(wObjPtr + 1, cpu_.a);
		cpu_.a = alu_.and_(rd8(REG_HL(gb_)), OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
		alu_.cp(cpu_.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
		if (ZF)
			goto is_visible;
	next_obj:
		cpu_.a = rd8(wObjPtr + 0);
		cpu_.h = cpu_.a;
		cpu_.a = rd8(wObjPtr + 1);
		cpu_.l = cpu_.a;
		SET_DE(gb_, OBJ_STRUCT_LENGTH);
		SET_HL(gb_, alu_.add16(REG_HL(gb_), OBJ_STRUCT_LENGTH));
		goto loop_objects;

	is_visible:
		hl = REG_HL(gb_);
		gb_push16(gb_, hl);
		cpu_.e = OBJ_UNK_1D;
		cpu_.d = 0;
		hl = alu_.add16(hl, OBJ_UNK_1D);
		cpu_.a = rd8(hl);
		hl--;
		wr8(wUnused_c1b8, cpu_.a);
		cpu_.a = rd8(hl);
		hl = gb_pop16(gb_);
		SET_HL(gb_, hl);
		alu_.and_(cpu_.a, cpu_.a);
		if (!ZF)
			goto next_obj;

		cpu_.e = OBJ_INTERACTION_TYPE;
		cpu_.d = 0;
		hl = alu_.add16(hl, OBJ_INTERACTION_TYPE);
		static const uint16_t dst[] = {wObjInteractionType, wObjCollBoxTop, wObjCollBoxBottom,
			wObjCollBoxLeft, wObjCollBoxRight, wObjScreenYPos, wObjScreenXPos};
		for (int i = 0; i < 7; i++) {
			cpu_.a = rd8(hl);
			wr8(dst[i], cpu_.a);
			if (i < 6)
				hl++;
		}
		cpu_.e = OBJ_SUBSTATE - OBJ_SCREEN_X_POS;
		cpu_.d = 0;
		hl = alu_.add16(hl, OBJ_SUBSTATE - OBJ_SCREEN_X_POS);
		SET_HL(gb_, hl);
		cpu_.a = rd8(hl);
		cpu_.a = alu_.bit(cpu_.a, OBJSUBFLAG_HDIR_F) ? DIRECTION_RIGHT : DIRECTION_LEFT;
		wr8(wObjDirection, cpu_.a);

		xor_a();
		wr8(wInteractionSide, 0);
		cpu_.a = 0xff;
		wr8(wc1c2, 0xff);
		wr8(wc1c1, 0xff);
		wr8(wc1c3, 0xff);
		wr8(wc1c4, 0xff);

		cpu_.e = 0xd0;
		cpu_.b = pi_edge(wObjScreenYPos, wObjCollBoxTop);
		pi_edge(wWarioScreenYPos, wCollisionBoxBottom);
		cpu_.a = alu_.sub(cpu_.a, cpu_.b);
		if (CF)
			goto next_obj;
		cpu_.c = cpu_.a;
		cpu_.b = pi_edge(wWarioScreenYPos, wCollisionBoxTop);
		pi_edge(wObjScreenYPos, wObjCollBoxBottom);
		cpu_.a = alu_.sub(cpu_.a, cpu_.b);
		if (CF)
			goto next_obj;
		cpu_.d = cpu_.a;
		cpu_.a = alu_.sub(cpu_.d, cpu_.c);
		if (!CF) {
			cpu_.a = cpu_.c;
			wr8(wc1c1, cpu_.a);
			if (ZF) {
				cpu_.a = cpu_.d;
				wr8(wc1c2, cpu_.a);
			}
		} else {
			cpu_.a = cpu_.d;
			wr8(wc1c2, cpu_.a);
		}

		cpu_.b = pi_edge(wObjScreenXPos, wObjCollBoxLeft);
		pi_edge(wWarioScreenXPos, wCollisionBoxRight);
		cpu_.a = alu_.sub(cpu_.a, cpu_.b);
		if (CF)
			goto next_obj;
		cpu_.c = cpu_.a;
		cpu_.b = pi_edge(wWarioScreenXPos, wCollisionBoxLeft);
		pi_edge(wObjScreenXPos, wObjCollBoxRight);
		cpu_.a = alu_.sub(cpu_.a, cpu_.b);
		if (CF)
			goto next_obj;
		cpu_.d = cpu_.a;
		cpu_.a = alu_.sub(cpu_.a, cpu_.c);
		if (!CF) {
			cpu_.a = cpu_.c;
			wr8(wc1c3, cpu_.a);
			if (ZF) {
				cpu_.a = cpu_.d;
				wr8(wc1c4, cpu_.a);
			}
		} else {
			cpu_.a = cpu_.d;
			wr8(wc1c4, cpu_.a);
		}

		{
			uint8_t side = INTERACTION_LEFT | INTERACTION_RIGHT | INTERACTION_UP | INTERACTION_DOWN;
			SET_HL(gb_, wInteractionSide);
			cpu_.a = side;
			wr8(wInteractionSide, side);
			cpu_.a = rd8(wc1c4);
			cpu_.b = cpu_.a;
			cpu_.a = rd8(wc1c3);
			cpu_.c = cpu_.a;
			alu_.cp(cpu_.a, cpu_.b);
			if (!ZF) {
				if (!CF) {
					side &= (uint8_t)~INTERACTION_RIGHT;
				} else {
					side &= (uint8_t)~INTERACTION_LEFT;
					cpu_.b = cpu_.c;
				}
				wr8(wInteractionSide, side);
			}
			cpu_.a = rd8(wc1c2);
			cpu_.d = cpu_.a;
			cpu_.a = rd8(wc1c1);
			cpu_.e = cpu_.a;
			alu_.cp(cpu_.a, cpu_.d);
			if (!ZF) {
				if (!CF) {
					side &= (uint8_t)~INTERACTION_UP;
				} else {
					side &= (uint8_t)~INTERACTION_DOWN;
					cpu_.d = cpu_.e;
				}
				wr8(wInteractionSide, side);
			}
			cpu_.a = cpu_.b;
			alu_.cp(cpu_.a, cpu_.d);
			if (!ZF) {
				if (!CF)
					side &= (uint8_t)~(INTERACTION_LEFT | INTERACTION_RIGHT);
				else
					side &= (uint8_t)~(INTERACTION_UP | INTERACTION_DOWN);
				wr8(wInteractionSide, side);
			}
		}

		// .do_interaction
		cpu_.a = alu_.and_(rd8(wObjInteractionType), INTERACTION_MASK);
		// rst JumpTable
		gb_push16(gb_, ProcessInteractions_table);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		hl = gb_pop16(gb_);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		hl = alu_.add16(hl, REG_DE(gb_));
		cpu_.a = rd8(hl);
		cpu_.h = rd8((uint16_t)(hl + 1));
		cpu_.l = cpu_.a;
		JUMP_TO(REG_HL(gb_));
	}

	void port_ProcessInteractions()
	{
		if (ld_and(wIsIntangible) || ld_and(wIsFloorTransition) || ld_and(wAutoMoveState)) {
			RET();
			return;
		}
		xor_a();
		wr8(wIsOnSteppableObject, 0);
		SET_HL(gb_, wObjects);
		pi_run(false);
	}

	void port_ProcessInteractions__next_obj()
	{
		pi_run(true);
	}

	// b = OBJACTION_* constant
	// Sets the action of the object at wObjPtr, and records which side Wario
	// interacted with it from.
	void port_SetObjAction()
	{
		uint16_t obj = (uint16_t)(rd8(wObjPtr + 0) << 8 | rd8(wObjPtr + 1));
		cpu_.d = 0;
		cpu_.e = OBJ_ACTION;
		uint16_t hl = alu_.add16(obj, OBJ_ACTION);
		wr8(hl, cpu_.b);
		cpu_.h = (uint8_t)(hl >> 8);
		cpu_.l = alu_.inc((uint8_t)hl); // inc l
		hl = REG_HL(gb_);

		cpu_.b = rd8(wInteractionSide);
		cpu_.a = alu_.or_(alu_.and_(rd8(hl), 0x0f), cpu_.b); // OBJ_UNK_1D
		wr8(hl, cpu_.a);
	}

	void port_Func_20a63()
	{
		cpu_.b = OBJACTION_07;
		gb_call(gb_, ADDR_SetObjAction);
	}

	void port_Func_20a69()
	{
		cpu_.b = OBJACTION_0D;
		gb_call(gb_, ADDR_SetObjAction);
	}

	void port_Func_21cc9()
	{
		cpu_.b = OBJACTION_13;
		gb_call(gb_, ADDR_SetObjAction);
	}

	void port_CollectKey()
	{
		play_sfx(SFX_KEY);
		gb_call(gb_, ADDR_Func_20a63);
		cpu_.a = ST_COLLECT_KEY;
		wr8(wState, ST_COLLECT_KEY);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wTimer, 0);
	}

	void port_SetState_OnFire()
	{
		st(wWarioState, WST_ON_FIRE);
		static const uint16_t zero[] = {wInvisibleFrame, wSFXLoopCounter, wWarioStateCycles,
			wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		if (ld_and(wWarioStateCounter))
			return;
		SET_HL(gb_, ADDR_WarioOnFirePal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioHotGfx, ADDR_WarioHotGfx);
		set_oam(BANK_OAM_1673c, ADDR_OAM_1673c);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_16b38, ADDR_Frameset_16b49);
		anim(OAM1);
	}

	void port_VanishObjectAndKnockBackIfZombie()
	{
		cpu_.b = OBJACTION_VANISH_TOUCH;
		LCALL(SetObjAction);
		ld_cp(wTransformation, TRANSFORMATION_ZOMBIE_WARIO);
		if (!ZF)
			return;
		FARCALL(SetState_ZombieKnockBack);
	}

	void port_SetState_OnFire_ResetStateCounter()
	{
		xor_a();
		wr8(wWarioStateCounter, 0);
		LJUMP(SetState_OnFire); // fallthrough
	}

	// ld hl, wKeyAndTreasureFlags / set bit, [hl] / jr CollectKey
	void obj_key(int bit)
	{
		SET_HL(gb_, wKeyAndTreasureFlags);
		wr8(wKeyAndTreasureFlags, (uint8_t)(rd8(wKeyAndTreasureFlags) | (1 << bit)));
		LJUMP(CollectKey);
	}

	void port_ObjInteraction_RedKey() { obj_key(RED_KEY_F); }
	void port_ObjInteraction_GreyKey() { obj_key(GREY_KEY_F); }
	void port_ObjInteraction_GreenKey() { obj_key(GREEN_KEY_F); }
	void port_ObjInteraction_BlueKey() { obj_key(BLUE_KEY_F); }

	void port_Func_20fe8()
	{
		cpu_.b = OBJACTION_06;
		LJUMP(SetObjAction);
	}

	void port_ObjInteraction_None()
	{
	}

	void port_ObjInteraction_Ink()
	{
		cpu_.b = OBJACTION_06;
		LJUMP(SetObjAction);
	}

	void port_Func_2092d()
	{
		cpu_.b = OBJACTION_VANISH_TOUCH;
		LJUMP(SetObjAction);
	}

	void port_ObjInteraction_Hand()
	{
		if (ld_and(wInvincibleCounter) || ld_and(wTransformation)) {
			RET();
			return;
		}
		play_sfx(SFX_FAT_WALK);
		FARCALL(SetState_DraggedDown);
		LJUMP(Func_21cc9); // fallthrough
	}

	void port_ObjInteraction_UnlitTorch()
	{
		ld_cp(wTransformation, TRANSFORMATION_HOT_WARIO);
		if (!ZF) {
			RET();
			return;
		}
		cpu_.b = OBJACTION_0E;
		LJUMP(SetObjAction);
	}

	void port_SetState_FlatStretching()
	{
		st(wAutoMoveState, 0x01);
		st(wWarioState, WST_FLAT_STRETCHING);
		collision_box(-1, -8, -11, 11);
		st(wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wFrameDuration,
			wAnimationFrame, 0};
		clear(zero);
		set_frameset(ADDR_Frameset_17193);
		anim(OAM1);
	}

	void port_TreasureClearObjects()
	{
		uint16_t hl = wObjects;
		SET_DE(gb_, OBJ_STRUCT_LENGTH);
		for (;;) {
			cpu_.a = (uint8_t)(hl >> 8);
			alu_.cp(cpu_.a, (uint8_t)(wCurObj >> 8));
			if (ZF)
				break;
			cpu_.a = alu_.and_(rd8(hl), OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
			alu_.cp(cpu_.a, OBJFLAG_ACTIVE);
			bool vanish = ZF;
			if (!vanish) {
				alu_.cp(cpu_.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
				vanish = ZF;
			}
			if (vanish) {
				cpu_.a = rd8(wObjPtr + 1);
				alu_.cp(cpu_.a, (uint8_t)hl);
				if (!ZF) {
					gb_push16(gb_, hl); // push hl / pop hl
					cpu_.c = OBJ_ACTION;
					cpu_.b = 0x00;
					wr8(alu_.add16(hl, OBJ_ACTION), OBJACTION_VANISH_TREASURE);
					gb_pop16(gb_);
				}
			}
			hl = alu_.add16(hl, OBJ_STRUCT_LENGTH);
		}
		SET_HL(gb_, hl);
	}

	void port_SetState_EnemyBumping()
	{
		play_sfx(SFX_BUMP);
		st(wJumpVelIndex, 0x0e);
		st(wJumpVelTable, 0x01);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wIsSmashAttacking, wGrabState, 0};
		clear(zero);
		st(wWarioState, WST_ENEMY_BUMPING);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		if (ld_and(wIsCrouching))
			frameset_by_dir(ADDR_Frameset_14a3b, ADDR_Frameset_14a38);
		else
			frameset_by_dir(ADDR_Frameset_14a71, ADDR_Frameset_14a6c);
		anim(OAM1);
	}

	void port_ObjInteraction_Food()
	{
		if (ld_and(wIsCrouching)) {
			LJUMP(Func_20a69);
			return;
		}
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_20a69);
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_FAT_WARIO);
		if (ZF) {
			LJUMP(Func_20a69);
			return;
		}
		LCALL(Func_20a63);
		st(wTransformation, TRANSFORMATION_FAT_WARIO);
		touch_states(TOUCH_BUMP, 0x02);
		st(wTransformationDuration + 0, (uint8_t)(FAT_WARIO_DURATION >> 8));
		st(wTransformationDuration + 1, (uint8_t)FAT_WARIO_DURATION);
		static const uint16_t zero[] = {wInvisibleFrame, wSFXLoopCounter, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		if (ld_and(wJumpVelTable))
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		play_sfx(SFX_03A);
		CALL(UpdateLevelMusic);
		st(wWarioState, WST_FAT_EATING);
		collision_box(-1, -27, -9, 9);
		reset_anim();
		load_gfx(BANK_WarioFatGfx, ADDR_WarioFatGfx);
		set_oam(BANK_OAM_1742d, ADDR_OAM_1742d);
		frameset_by_dir(ADDR_Frameset_1789c, ADDR_Frameset_178c3);
		anim(OAM1);
		RET();
	}

	void port_ObjInteraction_Electric()
	{
		cpu_.b = rd8(wWarioScreenXPos);
		ld_cp(wObjScreenXPos, cpu_.b);
		cpu_.a = alu_.or_(rd8(wInteractionSide), CF ? INTERACTION_LEFT : INTERACTION_RIGHT);
		wr8(wInteractionSide, cpu_.a);
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);

		if (ld_and(wInvincibleCounter))
			return;
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
			return;
		alu_.cp(cpu_.a, TRANSFORMATION_ELECTRIC);
		if (ZF)
			return;
		if (ld_bit(wInteractionSide, INTERACTION_RIGHT_F))
			st(wDirection, DIRECTION_LEFT);
		else
			st(wDirection, DIRECTION_RIGHT);
		play_sfx(SFX_040);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wInvisibleFrame, 0};
		clear(zero);
		st(wWarioState, WST_ELECTRIC_START);
		collision_box(-1, -27, -9, 9);
		st(wTransformation, TRANSFORMATION_ELECTRIC);
		touch_states(TOUCH_VANISH, 0x01);
		CALL(UpdateLevelMusic);
		reset_anim();
		SET_HL(gb_, ADDR_WarioElectricPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioElectricGfx, ADDR_WarioElectricGfx);
		set_oam(BANK_OAM_1790e, ADDR_OAM_1790e);
		frameset_by_dir(ADDR_Frameset_17b79, ADDR_Frameset_17b76);
		anim(OAM1);
	}

	// Adds n (BCD) to wNumCoins, capped at MAX_NUM_COINS
	void add_coins(uint8_t n)
	{
		cpu_.a = alu_.daa(alu_.add(rd8(wNumCoins + 1), n));
		wr8(wNumCoins + 1, cpu_.a);
		cpu_.a = alu_.daa(alu_.adc(rd8(wNumCoins), 0x00));
		wr8(wNumCoins, cpu_.a);
		SET_HL(gb_, wNumCoins);
		cpu_.a = alu_.and_(cpu_.a, 0xf0);
		if (ZF)
			return;
		cpu_.a = (uint8_t)(MAX_NUM_COINS >> 8);
		wr8(wNumCoins, cpu_.a);
		cpu_.a = (uint8_t)MAX_NUM_COINS;
		wr8(wNumCoins + 1, cpu_.a);
		SET_HL(gb_, wNumCoins + 1);
	}

	void port_ObjInteraction_RegularCoin()
	{
		add_coins(0x1);
		play_sfx(SFX_REGULAR_COIN);
		LJUMP(Func_20a63);
	}


	void port_CollectMusicalCoin()
	{
		add_coins(0x10);
		LCALL(Func_20a63);
		JUMP_TO(ProcessInteractions_next_obj);
	}

	void port_ObjInteraction_ColourCoin()
	{
		play_sfx(SFX_COLOUR_COIN);
		LJUMP(CollectMusicalCoin); // fallthrough
	}

	void port_ObjInteraction_MusicalCoin()
	{
		SET_HL(gb_, wNumMusicalCoins);
		cpu_.a = rd8(wNumMusicalCoins);
		alu_.cp(cpu_.a, NUM_LEVEL_MUSICAL_COINS);
		if (!CF) {
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
		wr8(wNumMusicalCoins, alu_.inc(cpu_.a));
		play_sfx(SFX_MUSICAL_COIN);
		LJUMP(CollectMusicalCoin);
	}

	void port_ObjInteraction_InvisibilityPotion()
	{
		LCALL(Func_20a63);
		if (ld_and(wInvincibleCounter))
			return;
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
			return;
		alu_.cp(cpu_.a, TRANSFORMATION_INVISIBLE_WARIO);
		if (ZF)
			return;
		FARCALL(SetState_TurningInvisible);
	}

	void port_ObjInteraction_RollingRock()
	{
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		if (ld_and(wIsRolling))
			return;
		ld(wObjDirection);
		wr8(wDirection, cpu_.a);
		FARCALL(Func_1e174);
	}

	// ld a, [wInvincibleCounter] / and a / ret nz /
	// ld a, [wTransformation] / bit TRANSFORMATIONF_PERSISTENT_F, a / ret nz:
	// true if it returns
	bool invincible_or_persistent()
	{
		return ld_and(wInvincibleCounter) || ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F);
	}

	void port_ObjInteraction_Vampire()
	{
		if (invincible_or_persistent())
			return;
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_VAMPIRE_WARIO);
		FARCALL(Func_19b25);
		ld_cp(wTransformation, TRANSFORMATION_VAMPIRE_WARIO);
		if (!ZF)
			return;
		FARCALL(SetState_VampireTransforming);
	}

	void port_ObjInteraction_Bubble()
	{
		if (invincible_or_persistent())
			return;
		cpu_.b = OBJACTION_07;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_BUBBLE);
		touch_states(TOUCH_BUMP, 0x01);
		FARCALL(Func_2ac7d);
	}

	void port_ObjInteraction_ZombieHead()
	{
		if (ld_and(wInvincibleCounter)) {
			LJUMP(Func_20a69);
			return;
		}
		ld_cp(wTransformation, TRANSFORMATION_ZOMBIE_WARIO);
		if (ZF) {
			LJUMP(VanishObjectAndKnockBackIfZombie);
			return;
		}
		if (alu_.bit(cpu_.a, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_20a69);
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		play_sfx(SFX_028);
		st(wTransformation, TRANSFORMATION_ZOMBIE_WARIO);
		touch_states(TOUCH_VANISH, 0x02);
		FARCALL(SetState_ZombieIdling);
		RET();
	}

	void port_Func_2028a()
	{
		if (!ld_and(wJumpVelTable))
			return;
		cpu_.b = rd8(wWarioScreenXPos);
		ld_cp(wObjScreenXPos, cpu_.b);
		st(wDirection, CF ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(SetState_FlatFalling);
	}

	// ld a, [wStingTouchState] / and a / jr z / cp TOUCH_BUMP / jr z /
	// cp TOUCH_VANISH / jp z, Func_2092d / ret: 0 = go on, 1 = return,
	// 2 = jumped to Func_2092d
	int sting_touch()
	{
		if (!ld_and(wStingTouchState))
			return 0;
		alu_.cp(cpu_.a, TOUCH_BUMP);
		if (ZF)
			return 0;
		alu_.cp(cpu_.a, TOUCH_VANISH);
		if (ZF) {
			LJUMP(Func_2092d);
			return 2;
		}
		RET();
		return 1;
	}

	// Func_208f2 is jr .start, so the routine and .start run the same code.
	void func_208f2_start()
	{
		if (sting_touch())
			return;
		if (!ld_and(wWaterInteraction)) {
			RET();
			return;
		}
		// call .Bump
		gb_push16(gb_, ADDR_Func_208f2 + 0x20);
		cpu_.b = OBJACTION_BUMP;
		gb_call_ret(gb_, ADDR_SetObjAction, ADDR_Func_208f2 + 0x07);
		gb_pop16(gb_);
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF || ld_and(wTransformation)) {
			RET();
			return;
		}
		FARCALL(StartDive);
		RET();
	}

	void port_Func_208f2() { func_208f2_start(); }
	void port_Func_208f2__start() { func_208f2_start(); }

	void port_Func_20899()
	{
		if (sting_touch())
			return;
		if (!ld_and(wWaterInteraction)) {
			RET();
			return;
		}
		cpu_.b = rd8(wWarioScreenXPos);
		ld_cp(wObjScreenXPos, cpu_.b);
		cpu_.a = alu_.or_(rd8(wInteractionSide), CF ? INTERACTION_LEFT : INTERACTION_RIGHT);
		wr8(wInteractionSide, cpu_.a);
		play_sfx(SFX_014);
		cpu_.b = OBJACTION_WOBBLE;
		LCALL(SetObjAction);
		if (ld_and(wTransformation) || ld_bit(wInteractionSide, INTERACTION_UP_F)) {
			RET();
			return;
		}
		FARCALL(StartDive);
		RET();
	}

	void port_SetState_WaterStung()
	{
		play_sfx(SFX_STING);
		st(wInvincibleCounter, 0x01);
		st(wWarioState, WST_WATER_STUNG);
		static const uint16_t zero[] = {wSwimVelIndex, wJumpVelTable, wWarioStateCounter,
			wWarioStateCycles, 0};
		clear(zero);
		st(wIsIntangible, TRUE);
		collision_box(-1, -15, -9, 9);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
	}

	void port_ObjInteraction_Rail()
	{
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
			return;
		alu_.cp(cpu_.a, TRANSFORMATION_RAIL);
		if (ZF)
			return;
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		play_sfx(SFX_HANG_GRAB);
		st(wTransformation, TRANSFORMATION_RAIL);
		st(wTouchState, TOUCH_BUMP);
		st(wStingTouchState, TOUCH_VULNERABLE);
		st(wca94, 0x01);
		st(wWarioState, WST_HANGING_RAIL);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wAutoMoveState, cpu_.a);
		CALL(UpdateLevelMusic);
		reset_anim();
		load_gfx(BANK_WarioHangGfx, ADDR_WarioHangGfx);
		set_oam(BANK_OAM_1fee05, ADDR_OAM_1fee05);
		st(wDirection, rd8(wObjDirection));
		set_frameset(ADDR_Frameset_1feec4);
		anim(OAM2);
	}

	void port_GetTreasure()
	{
		// stop_music
		wr8(hMusicID + 0, 0xff);
		st(hMusicID + 1, 0x00);
		LCALL(Func_20a63);
		LCALL(TreasureClearObjects);
		SET_HL(gb_, wLevelEndScreen);
		cpu_.a = rd8(wLevelEndScreen);
		alu_.cp(cpu_.a, LVLEND_NO_TREASURE);
		if (ZF)
			return;
		wr8(wLevelEndScreen, rd8(wLevelEndScreen) | 0x80);
		st(wResetDisabled, TRUE);
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_GET_TREASURE_START);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wJumpVelIndex,
			wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking, wIsCrouching,
			wAttackCounter, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		wr8(wAutoMoveState, cpu_.a);
		if (ld_and(wJumpVelTable))
			st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		if (ld_bit(wInteractionSide, INTERACTION_LEFT_F)) {
			cpu_.b = ld(wc1c4);
			CALL(MoveWarioRight);
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_14252);
		} else {
			cpu_.b = ld(wc1c3);
			CALL(MoveWarioLeft);
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_1425f);
		}
		anim(OAM1);
	}

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

	void port_SetState_Stung()
	{
		st(wInvincibleCounter, 0x01);
		play_sfx(SFX_STING);
		st(wWarioState, WST_STUNG);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsRolling, wIsSmashAttacking, 0};
		clear(zero);
		st(wJumpVelIndex, 0x6);
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
			st(wJumpVelTable, JUMP_VEL_KNOCK_BACK);
		}
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15f7f, ADDR_Frameset_15f70);
		anim(OAM1);
	}

	void port_Func_206eb()
	{
		ld_cp(wTransformation, TRANSFORMATION_OWL_WARIO);
		if (ZF)
			CALL(ReleaseOwl);
		CALL(ClearTransformationValues);
		CALL(UpdateLevelMusic);
		LJUMP(SetState_Stung); // fallthrough
	}

	void port_Func_207ed()
	{
		if (ld_and(wInvincibleCounter) || !ld_and(wWaterInteraction)) {
			RET();
			return;
		}
		if (ld_and(wStingTouchState)) {
			alu_.cp(cpu_.a, TOUCH_BUMP);
			if (ZF) {
				JUMP_TO(ADDR_Func_208f2 + 0x08); // Func_208f2.start
				return;
			}
			alu_.cp(cpu_.a, TOUCH_VANISH);
			if (ZF) {
				LJUMP(Func_2092d);
				return;
			}
			RET();
			return;
		}
		cpu_.b = OBJACTION_06;
		gb_call(gb_, ADDR_SetObjAction);
		LJUMP(SetState_WaterStung); // fallthrough
	}

	// stop_sfx
	void stop_sfx()
	{
		wr8(hSFXID + 0, 0xff);
		cpu_.a = 0x00;
		wr8(hSFXID + 1, 0x00);
	}

	// the shared body of ObjInteraction_WaterTeleporting/Jamano
	void teleport(uint16_t stung, uint8_t state)
	{
		st(wRoomTransitionParam, ROOMTRANSITION_TELEPORT | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY);
		gb_call(gb_, stung);
		stop_sfx();
		st(wWarioState, state);
		cpu_.b = OBJACTION_TELEPORT;
		LJUMP(SetObjAction);
	}

	void port_ObjInteraction_WaterTeleporting() { teleport(ADDR_SetState_WaterStung, WST_WATER_TELEPORTING); }
	void port_ObjInteraction_Jamano() { teleport(ADDR_Func_206eb, WST_TELEPORTING); }

	void port_Func_21e8a()
	{
		FARCALL(SetState_BallShot);
		LJUMP(Func_21cc9);
	}

	void port_Func_20d81()
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(Func_20899);
			return;
		}
		LJUMP(Func_208f2);
	}

	void port_Func_2168b()
	{
		ld_cp(wWarioState, WST_FLAT_SQUISHED);
		if (ZF) {
			FARCALL(SetState_FlatSquishedLifting);
			return;
		}
		ld_cp(wc1c2, 0x10);
		if (!CF)
			cpu_.a = 0x0c;
		cpu_.b = cpu_.a;
		CALL(MoveWarioDown);
		FARCALL(Func_19b25);
		if (ld_and(wWaterInteraction)) {
			FARCALL(DiveFromSurface_SkipSplash);
			return;
		}
		FARCALL(CheckCentreCollision);
		bool ground = b_and();
		if (ground)
			update_pos_y();
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF)
			return;
		ld_cp(wTransformation, TRANSFORMATION_FLAT_WARIO);
		if (ZF) {
			if (!ground) {
				FARCALL(SetState_FlatFalling);
				return;
			}
			ld_cp(wWarioState, WST_FLAT_SQUISHED);
			if (ZF)
				return;
			FARCALL(SetState_FlatIdling);
			return;
		}
		if (alu_.and_(cpu_.a, cpu_.a))
			return;
		if (ground) {
			xor_a();
			wr8(wGrabState, 0);
			FARCALL(SetState_Idling);
			return;
		}
		if (ld_and(wIsCrouching)) {
			FARCALL(StartCrouchFall);
			return;
		}
		xor_a();
		wr8(wGrabState, 0);
		FARCALL(StartFall);
	}

	void port_ObjInteraction_Snow()
	{
		if (ld_and(wInvincibleCounter) || ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_20a69);
			return;
		}
		LCALL(Func_20a63);
		st(wTransformation, TRANSFORMATION_SNOWMAN_WARIO);
		xor_a();
		wr8(wTransformationProgress, 0);
		touch_states(TOUCH_ATTACK, 0x02);
		FARCALL(SetState_TurningIntoSnowman);
		RET();
	}

	void port_ObjInteraction_Ice()
	{
		if (ld_and(wInvincibleCounter) || ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
			return;
		if (ld_and(wIsCrouching)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (b_and()) {
				st(wCollisionBoxTop, (uint8_t)-15);
				return;
			}
		}
		cpu_.b = OBJACTION_07;
		LCALL(SetObjAction);
		play_sfx(SFX_028);
		st(wTransformation, TRANSFORMATION_ICE_SKATIN);
		touch_states(TOUCH_ATTACK, 0x02);
		FARCALL(SetState_IceSkatinStart);
	}

	void port_Func_209ca()
	{
		ld_cp(wWarioState, WST_ATTACKING_AIRBORNE);
		if (ZF) {
			xor_a();
			wr8(wAttackCounter, 0);
		}
		if (ld_and(wWaterInteraction)) {
			RET();
			return;
		}
		ld_cp(wWarioState, WST_DIVING);
		if (ZF) {
			RET();
			return;
		}
		st(wIsOnSteppableObject, TRUE);
		if (ld_and(wc1c1)) {
			alu_.cp(cpu_.a, 0x03);
			if (!CF)
				cpu_.a = 0x02;
			cpu_.b = cpu_.a;
			CALL(MoveWarioUp);
		}
		cpu_.b = OBJACTION_STEP_ON;
		LCALL(SetObjAction);
		if (!ld_and(wJumpVelTable) || ld_and(wAttackCounter) || ld_and(wIsRolling)) {
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
		CALL(TriggerFloorTransition_SkipUpdateYPos);
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF) {
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
		if (ld_and(wTransformation)) {
			RET();
			return;
		}
		if (ld_and(wGrabState)) {
			FARCALL(SetState_GrabIdling);
		} else if (ld_and(wIsCrouching)) {
			xor_a();
			wr8(wWaterInteraction, 0);
			FARCALL(CrouchOrSlide);
		} else {
			xor_a();
			wr8(wWaterInteraction, 0);
			FARCALL(SetState_Idling);
		}
		JUMP_TO(ProcessInteractions_next_obj);
	}

	void port_Func_20a6f()
	{
		cpu_.b = ld(wWarioScreenXPos);
		alu_.cp(ld(wObjScreenXPos), cpu_.b);
		bool left = CF;
		for (;;) {
			if (!left) { // .asm_20a79
				if (!ld_and(wDirection)) {
					JUMP_TO(ProcessInteractions_next_obj);
					return;
				}
				st(wInteractionSide, INTERACTION_RIGHT);
				FARCALL(Func_197b1);
				if (b_and())
					break;
				ld_cp(wc1c3, 0xff);
				if (ZF) {
					left = true;
					continue;
				}
				cpu_.b = cpu_.a;
				CALL(MoveWarioLeft);
				break;
			} else { // .asm_20aa5
				if (ld_and(wDirection)) {
					JUMP_TO(ProcessInteractions_next_obj);
					return;
				}
				st(wInteractionSide, INTERACTION_LEFT);
				FARCALL(Func_19741);
				if (b_and())
					break;
				ld_cp(wc1c4, 0xff);
				if (ZF) {
					left = false;
					continue;
				}
				cpu_.b = cpu_.a;
				CALL(MoveWarioRight);
				break;
			}
		}
		// .asm_20acf
		cpu_.a = rd8(wTransformation);
		alu_.cp(cpu_.a, TRANSFORMATION_BALL_O_STRING_WARIO);
		if (ZF) {
			ld_cp(wTransformationProgress, 1);
			if (ZF)
				FARCALL(Func_28bd5);
			RET();
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_HOT_WARIO);
		if (ZF) {
			ld_cp(wTransformationProgress, 1);
			if (ZF)
				FARCALL(Func_2afc5);
			RET();
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_ICE_SKATIN);
		if (ZF) {
			ld_cp(wWarioState, WST_ICE_SKATIN_CRASH);
			if (!ZF)
				FARCALL(Func_1ec215);
			RET();
			return;
		}
		xor_a();
		wr8(wWalkVelIndex, 0);
		if (!ld_and(wIsRolling) && !ld_and(wAttackCounter)) {
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
		FARCALL(DoJumpingBump);
		RET();
	}

	// routine called when an object is attacked
	// whether by Wario's B attack, Fat Wario bump, rolling, etc.
	void port_AttackObject()
	{
		play_sfx(SFX_017);
		cpu_.b = ld(wWarioScreenXPos);
		alu_.cp(ld(wObjScreenXPos), cpu_.b);
		bool left = CF;
		st(wInteractionSide, left ? INTERACTION_LEFT : INTERACTION_RIGHT);
		// x = Wario's x +/- 8, y = Wario's y - 8
		cpu_.a = rd8(wWarioXPos + 1);
		cpu_.a = left ? alu_.sub(cpu_.a, 0x08) : alu_.add(cpu_.a, 0x08);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = rd8(wWarioXPos + 0);
		cpu_.a = left ? alu_.sbc(cpu_.a, 0x00) : alu_.adc(cpu_.a, 0x00);
		wr8(hXPosLo - 1, cpu_.a);
		cpu_.a = alu_.sub(rd8(wWarioXPos - 1), 0x08);
		wr8(hXPosLo - 2, cpu_.a);
		cpu_.a = alu_.sbc(rd8(wWarioXPos - 2), 0x00);
		wr8(hXPosLo - 3, cpu_.a);
		SET_HL(gb_, wWarioXPos - 2);
		SET_DE(gb_, hXPosLo - 3);

		// .spawn_stars
		cpu_.b = PARTICLE_STARS;
		FARCALL(CreateParticle);
		cpu_.b = OBJACTION_ATTACK;
		LCALL(SetObjAction);
		cpu_.a = rd8(wTransformation);
		alu_.cp(cpu_.a, TRANSFORMATION_FAT_WARIO);
		if (ZF) {
			FARCALL(SetState_FatBumping);
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_SNOWMAN_WARIO);
		if (ZF) {
			if (!ld_and(wTransformationProgress))
				FARCALL(SetState_SnowmanBumped);
			return;
		}
		if (alu_.and_(cpu_.a, cpu_.a))
			return;
		if (ld_and(wIsRolling)) {
			if (!ld_bit(wObjInteractionType, HEAVY_OBJ_F))
				return;
			ld_cp(wPowerUpLevel, GARLIC);
			if (!CF)
				return;
			FARCALL(DoJumpingBump_NoSFX);
			return;
		}
		// .not_rolling
		if (ld_bit(wObjInteractionType, HEAVY_OBJ_F)) {
			FARCALL(DoJumpingBump_NoSFX);
			return;
		}
		if (ld_and(wJumpVelTable))
			FARCALL(StartFall);
		else
			FARCALL(SetState_Idling);
	}

	void port_Func_217a9()
	{
		FARCALL(DoJumpingBump);
	}

	void port_Func_20d8c()
	{
		if (!ld_and(wWaterInteraction)) {
			RET();
			return;
		}
		if (ld_bit(wInteractionSide, INTERACTION_UP_F)) {
			LJUMP(Func_207ed);
			return;
		}
		if (ld_and(wTransformation)) {
			RET();
			return;
		}
		if (ld_bit(wInteractionSide, INTERACTION_DOWN_F)) {
			FARCALL(StartDive);
			RET();
			return;
		}
		cpu_.b = ld(wWarioScreenXPos);
		alu_.cp(ld(wObjScreenXPos), cpu_.b);
		bool left = CF;
		cpu_.a = alu_.or_(rd8(wInteractionSide), left ? INTERACTION_LEFT : INTERACTION_RIGHT);
		wr8(wInteractionSide, cpu_.a);
		st(wDirection, left ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(SetState_SwimKnockBack);
		RET();
	}

	// ld a, [wInteractionSide] / bit side, a / jp nz, Func_207ed / jp Func_208f2
	void sting_or_bump(int side)
	{
		if (ld_bit(wInteractionSide, side))
			LJUMP(Func_207ed);
		else
			LJUMP(Func_208f2);
	}

	void port_Func_20deb()
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(Func_20899);
			return;
		}
		if (!ld_and(wObjDirection))
			sting_or_bump(INTERACTION_RIGHT_F);
		else
			sting_or_bump(INTERACTION_LEFT_F);
	}

	void port_ObjInteraction_Haridama()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(Func_20899);
			return;
		}
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			LJUMP(Func_207ed);
			return;
		}
		ld_cp(wObjDirection, DIRECTION_RIGHT);
		if (ZF)
			sting_or_bump(INTERACTION_LEFT_F);
		else
			sting_or_bump(INTERACTION_RIGHT_F);
	}

	void port_ObjInteraction_Pesce()
	{
		ld_cp(wObjDirection, DIRECTION_RIGHT);
		bool swallow = ld_bit(wInteractionSide, ZF ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F);
		if (!swallow) {
			LJUMP(Func_207ed);
			return;
		}
		cpu_.b = OBJACTION_11;
		LCALL(SetObjAction);
		FARCALL(SetState_Swallowed);
		RET();
	}

	void port_ObjInteraction_Scowler()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			LJUMP(Func_20899);
			return;
		}
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(ObjInteraction_WaterTeleporting);
			return;
		}
		LJUMP(Func_208f2);
	}

	// the chest interactions: open it with the matching key
	void obj_treasure(int key, int treasure, uint8_t screen)
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F)) {
			LJUMP(Func_209ca);
			return;
		}
		if (ld_and(wTransformation)) {
			LJUMP(Func_20a6f);
			return;
		}
		SET_HL(gb_, wKeyAndTreasureFlags);
		if (!alu_.bit(rd8(wKeyAndTreasureFlags), key)) {
			LJUMP(Func_20a6f);
			return;
		}
		wr8(wKeyAndTreasureFlags, (uint8_t)(rd8(wKeyAndTreasureFlags) | (1 << treasure)));
		st(wLevelEndScreen, screen);
		LJUMP(GetTreasure);
	}

	void port_ObjInteraction_GreyTreasure() { obj_treasure(GREY_KEY_F, GREY_TREASURE_F, LVLEND_GREY_TREASURE); }
	void port_ObjInteraction_RedTreasure() { obj_treasure(RED_KEY_F, RED_TREASURE_F, LVLEND_RED_TREASURE); }
	void port_ObjInteraction_GreenTreasure() { obj_treasure(GREEN_KEY_F, GREEN_TREASURE_F, LVLEND_GREEN_TREASURE); }
	void port_ObjInteraction_BlueTreasure() { obj_treasure(BLUE_KEY_F, BLUE_TREASURE_F, LVLEND_BLUE_TREASURE); }

	// stepped on from above: Func_209ca; attacked: Func_217a9; else Func_20a6f.
	// Returns true if it jumped (false: touched from below).
	bool solid_side()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			LJUMP(Func_209ca);
			return true;
		}
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F))
			return false;
		if (ld_and(wAttackCounter))
			LJUMP(Func_217a9);
		else
			LJUMP(Func_20a6f);
		return true;
	}

	void port_ObjInteraction_Solid()
	{
		if (solid_side())
			return;
		LJUMP(Func_2168b);
	}

	void port_ObjInteraction_Omodon()
	{
		if (solid_side())
			return;
		if (ld_and(wInvincibleCounter) || ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			RET();
			return;
		}
		cpu_.b = OBJACTION_FLATTEN;
		LCALL(SetObjAction);
		FARCALL(SetState_FlatAirborne);
		RET();
	}

	void port_Func_21abb()
	{
		cpu_.b = OBJACTION_ATTACK;
		LCALL(SetObjAction);
		LJUMP(Func_20a6f);
	}

	void port_Func_21f01()
	{
		if (!ld_bit(wInteractionSide, INTERACTION_UP_F)) {
			LJUMP(Func_20d8c);
			return;
		}
		cpu_.a = alu_.or_(rd8(wInteractionSide), INTERACTION_LEFT);
		wr8(wInteractionSide, cpu_.a);
		st(wDirection, DIRECTION_RIGHT);
		FARCALL(SetState_SwimKnockBack);
		RET();
	}

	void port_Func_20602()
	{
		cpu_.a = rd8(wTransformation);
		alu_.cp(cpu_.a, TRANSFORMATION_INVISIBLE_WARIO);
		if (!ZF && alu_.and_(cpu_.a, cpu_.a)) {
			// has transformation except invisible
			LJUMP(VanishObjectAndKnockBackIfZombie);
			return;
		}
		cpu_.a = alu_.and_(rd8(wGrabState), GRAB_STATE_MASK);
		alu_.cp(cpu_.a, GRAB_IDLE);
		if (ZF) {
			FARCALL(Func_1ede9);
		} else {
			xor_a();
			wr8(wGrabState, 0);
			FARCALL(StartJump_FromInput);
		}
		st(wDoFullJump, TRUE);
		st(wJumpVelIndex, 0x0a);
		if (ld_bit(wJoypadDown, B_PAD_A)) {
			ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
			if (!CF) {
				xor_a();
				wr8(wJumpVelIndex, 0);
			}
		}
		cpu_.b = OBJACTION_VANISH_TOUCH;
		LJUMP(SetObjAction); // fallthrough
	}

	void port_Func_21aac()
	{
		cpu_.a = alu_.dec(rd8(wIsSmashAttacking));
		if (!ZF) {
			LJUMP(Func_209ca);
			return;
		}
		// smash attack object
		LCALL(Func_20602);
		cpu_.b = OBJACTION_SMASH_ATTACK_WALKABLE;
		LJUMP(SetObjAction);
	}

	void port_ObjInteraction_Stove()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			LJUMP(Func_21aac);
			return;
		}
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(Func_2168b);
			return;
		}
		if (ld_and(wAttackCounter))
			LJUMP(Func_21abb);
		else
			LJUMP(Func_20a6f);
	}

	void port_ObjInteraction_WaterDrop()
	{
		cpu_.b = OBJACTION_06;
		gb_call(gb_, ADDR_SetObjAction);
		cpu_.a = rd8(wTransformation);
		alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.f & FLAG_Z) {
			JUMP_TO(ProcessInteractions_next_obj);
			return;
		}
		JUMP(RecoverFromTransformation);
	}

	// ld a, [wWarioScreenXPos] / ld b, a / ld a, [wObjScreenXPos] / cp b, then
	// sets INTERACTION_LEFT (object left of Wario) or INTERACTION_RIGHT
	void side_by_x()
	{
		cpu_.b = rd8(wWarioScreenXPos);
		cpu_.a = rd8(wObjScreenXPos);
		alu_.cp(cpu_.a, cpu_.b);
		uint8_t bit = CF ? INTERACTION_LEFT : INTERACTION_RIGHT;
		cpu_.a = alu_.or_(rd8(wInteractionSide), bit);
		wr8(wInteractionSide, cpu_.a);
	}

	// Func_2023b.asm_20257
	void func_2023b_bump()
	{
		if (ld_and(wAttackCounter)) {
			LJUMP(AttackObject);
			return;
		}
		cpu_.b = OBJACTION_BUMP;
		LCALL(SetObjAction);
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF) {
			RET(); // is invincible
			return;
		}
		ld_cp(wTransformation, TRANSFORMATION_FLAT_WARIO);
		if (ZF) {
			LJUMP(Func_2028a);
			return;
		}
		if (alu_.and_(cpu_.a, cpu_.a)) {
			RET();
			return;
		}
		cpu_.b = rd8(wWarioScreenXPos);
		cpu_.a = rd8(wObjScreenXPos);
		alu_.cp(cpu_.a, cpu_.b);
		st(wDirection, CF ? DIRECTION_RIGHT : DIRECTION_LEFT);
		LJUMP(SetState_EnemyBumping);
	}

	void port_Func_2023b()
	{
		if (!ld_and(wTouchState)) {
			func_2023b_bump();
			return;
		}
		uint8_t a = cpu_.a;
		alu_.cp(a, TOUCH_BUMP);
		if (ZF) {
			func_2023b_bump();
			return;
		}
		alu_.cp(a, TOUCH_VULNERABLE);
		if (ZF) {
			LJUMP(Func_20939); // this causes an infinite loop!
			return;
		}
		alu_.cp(a, TOUCH_ATTACK);
		if (ZF) {
			LJUMP(AttackObject);
			return;
		}
		alu_.cp(a, TOUCH_VANISH);
		if (ZF) {
			LJUMP(VanishObjectAndKnockBackIfZombie);
			return;
		}
		JUMP_TO(ProcessInteractions_next_obj);
	}

	void port_Func_2022c()
	{
		if (ld_and(wIsRolling)) {
			LJUMP(AttackObject);
			return;
		}
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(StepOnObject);
			return;
		}
		LJUMP(Func_2023b); // fallthrough
	}

	void port_Func_20939()
	{
		if (ld_and(wTouchState)) {
			LJUMP(Func_2023b);
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF) {
			RET();
			return;
		}
		FARCALL(DoJumpingBump);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		xor_a();
		wr8(wGrabState, 0);
		cpu_.b = rd8(wWarioScreenXPos);
		cpu_.a = rd8(wObjScreenXPos);
		alu_.cp(cpu_.a, cpu_.b);
		if (!CF) {
			st(wInteractionSide, INTERACTION_RIGHT);
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_14a79);
		} else {
			st(wInteractionSide, INTERACTION_LEFT);
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_14a76);
		}
		anim(OAM1);
		RET();
	}

	void port_Func_20932()
	{
		if (ld_and(wAttackCounter)) {
			LJUMP(AttackObject);
			return;
		}
		LJUMP(Func_20939); // fallthrough
	}

	void port_StepOnObject()
	{
		// no stun if Wario is invincible
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF) {
			RET();
			return;
		}
		cpu_.a = alu_.dec(rd8(wIsSmashAttacking));
		if (ZF) {
			// Wario is smash attacking
			ld_cp(wTransformation, TRANSFORMATION_FAT_WARIO);
			if (ZF)
				LJUMP(VanishObjectAndKnockBackIfZombie);
			else
				LJUMP(Func_20602);
			return;
		}
		if (ld_and(wTransformation)) {
			// .Transformed
			ld_cp(wTransformation, TRANSFORMATION_HOT_WARIO);
			if (ZF) {
				ld_cp(wTransformationProgress, 2);
				if (!CF) {
					LJUMP(VanishObjectAndKnockBackIfZombie);
					return;
				}
			}
			ld_cp(wTransformation, TRANSFORMATION_FAT_WARIO);
			if (ZF) {
				LJUMP(AttackObject);
				return;
			}
			side_by_x();
			play_sfx(SFX_014);
			cpu_.b = OBJACTION_WOBBLE;
			LCALL(SetObjAction);
			ld_cp(wTransformation, TRANSFORMATION_FLAT_WARIO);
			if (ZF) {
				LJUMP(Func_2028a);
				return;
			}
			alu_.cp(cpu_.a, TRANSFORMATION_BOUNCY_WARIO);
			if (ZF) {
				ld_cp(wTouchState, TOUCH_PASS_THROUGH);
				if (!ZF)
					FARCALL(Func_2a0b2);
			}
			RET();
			return;
		}

		// not transformed
		xor_a();
		wr8(wAttackCounter, 0);
		// stepping on object while rolling is attacking
		if (ld_and(wIsRolling)) {
			LJUMP(AttackObject);
			return;
		}
		side_by_x();
		bool below = ld_bit(wInteractionSide, INTERACTION_DOWN_F);
		if (!below) {
			cpu_.b = rd8(wWarioScreenYPos);
			cpu_.a = rd8(wObjScreenYPos);
			alu_.cp(cpu_.a, cpu_.b);
			below = CF;
		}
		if (!below) {
			cpu_.a = alu_.and_(rd8(wGrabState), GRAB_STATE_MASK);
			alu_.cp(cpu_.a, GRAB_IDLE);
			if (ZF) {
				FARCALL(Func_1ede9); // .grabbing
			} else {
				xor_a();
				wr8(wGrabState, 0);
				if (ld_and(wIsCrouching)) {
					FARCALL(StartCrouchJump);
				} else if (ld_and(wJoypadDown)) {
					FARCALL(StartJump_FromInput);
				} else {
					// .no_input
					LCALL(Func_20939);
					goto sfx;
				}
			}
			st(wDoFullJump, TRUE);
			st(wJumpVelIndex, 0x0a);
			if (ld_bit(wJoypadDown, B_PAD_A)) {
				ld_cp(wPowerUpLevel, HIGH_JUMP_BOOTS);
				if (!CF) {
					xor_a(); // full jump
					wr8(wJumpVelIndex, 0);
				}
			}
		} else {
			// .asm_20506
			ld_cp(wInvincibleCounter, 0x01);
			if (ZF) {
				RET();
				return;
			}
			if (ld_and(wIsCrouching)) {
				if (ld_and(wJumpVelTable))
					FARCALL(StartCrouchFall);
			} else {
				cpu_.a = alu_.and_(rd8(wGrabState), GRAB_STATE_MASK);
				alu_.cp(cpu_.a, GRAB_IDLE);
				if (ZF) {
					if (ld_and(wJumpVelTable))
						FARCALL(Func_1edd3);
				} else {
					xor_a();
					wr8(wGrabState, 0);
					if (ld_and(wJumpVelTable))
						FARCALL(StartFall);
					else
						FARCALL(SetState_Idling);
				}
			}
		}
	sfx:
		play_sfx(SFX_014);
		cpu_.b = OBJACTION_WOBBLE;
		LJUMP(SetObjAction);
	}

	void port_Func_20670()
	{
		if (ld_and(wInvincibleCounter)) {
			LJUMP(Func_20939);
			return;
		}
		// if Wario is rolling, do attack
		ld_cp(wPowerUpLevel, GARLIC);
		bool check_rolling = !CF;
		if (!check_rolling)
			check_rolling = !ld_bit(wObjInteractionType, HEAVY_OBJ_F);
		if (check_rolling && ld_and(wIsRolling)) {
			LJUMP(AttackObject);
			return;
		}
		// .asm_2068c
		if (ld_and(wStingTouchState)) {
			uint8_t a = cpu_.a;
			alu_.cp(a, TOUCH_VULNERABLE);
			if (!ZF) {
				alu_.cp(a, TOUCH_BUMP);
				if (ZF) {
					LJUMP(Func_2023b);
					return;
				}
				alu_.cp(a, TOUCH_ATTACK);
				if (ZF) {
					LJUMP(AttackObject);
					return;
				}
				alu_.cp(a, TOUCH_VANISH);
				if (ZF) {
					LJUMP(VanishObjectAndKnockBackIfZombie);
					return;
				}
				JUMP_TO(ProcessInteractions_next_obj);
				return;
			}
		}
		// .asm_206a8
		side_by_x();
		if (!ld_bit(wInteractionSide, INTERACTION_RIGHT_F))
			st(wDirection, DIRECTION_RIGHT);
		else
			st(wDirection, DIRECTION_LEFT);
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		ld_cp(wInvincibleCounter, 0x01);
		if (ZF) {
			RET();
			return;
		}
		ld_cp(wStingTouchState, TOUCH_VULNERABLE);
		if (ZF)
			LJUMP(Func_206eb);
		else
			LJUMP(SetState_Stung);
	}

	// Func_20b6b.set_wario_pick_up
	void set_wario_pick_up()
	{
		play_sfx(SFX_GRAB);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		st(wWarioState, WST_PICKING_UP);
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioThrowGfx, ADDR_WarioThrowGfx);
		set_oam(BANK_OAM_1606a, ADDR_OAM_1606a);
		uint16_t fs;
		if (!ld_and(wDirection)) {
			if (ld_bit(wWarioSlopeInteraction, LEFT_SLOPE_F))
				goto grab_idling;
			fs = ld_mask(wGrabState, 1 << GRAB_HEAVY_F) ? ADDR_Frameset_163f5 : ADDR_Frameset_163ec;
		} else {
			if (ld_bit(wWarioSlopeInteraction, RIGHT_SLOPE_F))
				goto grab_idling;
			fs = ld_mask(wGrabState, 1 << GRAB_HEAVY_F) ? ADDR_Frameset_16415 : ADDR_Frameset_1640c;
		}
		set_frameset(fs);
		anim(OAM1);
		RET();
		return;
	grab_idling:
		FARCALL(SetState_GrabIdling);
		RET();
	}

	void port_Func_20b6b()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F))
			goto step_on_obj;
		if (ld_and(wTouchState)) {
			uint8_t a = cpu_.a;
			alu_.cp(a, TOUCH_ATTACK);
			if (ZF) {
				LJUMP(AttackObject);
				return;
			}
			alu_.cp(a, TOUCH_VANISH);
			if (ZF) {
				LJUMP(VanishObjectAndKnockBackIfZombie);
				return;
			}
			alu_.cp(a, TOUCH_PASS_THROUGH);
			if (ZF) {
				JUMP_TO(ProcessInteractions_next_obj);
				return;
			}
			goto step_on_obj;
		}
		// .check_water
		ld_cp(wWarioState, WST_DIVING);
		if (!CF) {
			alu_.cp(cpu_.a, WST_TRYING_SUBMERGE + 1);
			if (CF) {
				LJUMP(Func_20899);
				return;
			}
		}
		// .not_in_water
		cpu_.b = ld(wIsRolling);
		cpu_.a = alu_.or_(rd8(wAttackCounter), cpu_.b);
		if (!ZF) {
			LJUMP(AttackObject);
			return;
		}
		cpu_.b = ld(wIsCrouching);
		cpu_.a = alu_.or_(rd8(wGrabState), cpu_.b);
		if (!ZF)
			goto step_on_obj;
		ld_cp(wPowerUpLevel, GRAB_GLOVE);
		if (CF)
			goto step_on_obj;
		ld_cp(wWarioState, WST_LADDER_SCRATCHING);
		if (ZF)
			goto step_on_obj;
		alu_.cp(cpu_.a, WST_LADDER_CLIMBING);
		if (!CF) {
			alu_.cp(cpu_.a, WST_LADDER_SLIDING + 1);
			if (CF)
				goto step_on_obj;
		}
		// .not_on_ladder
		cpu_.b = rd8(wWarioScreenXPos);
		cpu_.a = rd8(wObjScreenXPos);
		alu_.cp(cpu_.a, cpu_.b);
		if (!CF) {
			st(wInteractionSide, INTERACTION_RIGHT);
			if (!ld_and(wDirection))
				goto step_on_obj;
		} else {
			st(wInteractionSide, INTERACTION_LEFT);
			if (ld_and(wDirection))
				goto step_on_obj;
		}
		// .check_invincible_counter
		ld_cp(wInvincibleCounter, 1);
		if (ZF) {
			RET();
			return;
		}
		{
			uint16_t hl = wWarioXPos + 1;
			uint16_t de = hXPosLo;
			cpu_.a = rd8(hl--);
			wr8(de--, cpu_.a);
			cpu_.a = rd8(hl--);
			wr8(de--, cpu_.a);
			cpu_.a = alu_.sub(rd8(hl--), 0x08);
			wr8(de--, cpu_.a);
			cpu_.a = alu_.sbc(rd8(hl), 0x00);
			wr8(de, cpu_.a);
			SET_HL(gb_, hl);
			SET_DE(gb_, de);
		}
		if (!ld_bit(wObjInteractionType, HEAVY_OBJ_F)) {
			st(wGrabState, GRAB_PICK_UP);
		} else {
			ld_cp(wPowerUpLevel, SUPER_GRAB_GLOVES);
			if (CF)
				goto step_on_obj;
			st(wGrabState, GRAB_PICK_UP | (1 << GRAB_HEAVY_F));
		}
		// .set_obj_pick_up
		cpu_.b = OBJACTION_PICK_UP;
		LCALL(SetObjAction);
		if (!ld_and(wJumpVelTable)) {
			set_wario_pick_up();
			return;
		}
		if (ld_bit(wObjInteractionType, HEAVY_OBJ_F))
			goto step_on_obj;
		FARCALL(Func_1edd3);
		RET();
		return;
	step_on_obj:
		LJUMP(StepOnObject);
	}

	void port_Func_20b41()
	{
		if (ld_and(wTouchState)) {
			uint8_t a = cpu_.a;
			alu_.cp(a, TOUCH_BUMP);
			if (!ZF) {
				alu_.cp(a, TOUCH_VULNERABLE);
				if (!ZF) {
					alu_.cp(a, TOUCH_ATTACK);
					if (ZF) {
						LJUMP(AttackObject);
						return;
					}
					alu_.cp(a, TOUCH_VANISH);
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
		if (ld_and(wTouchState)) {
			RET();
			return;
		}
		LCALL(Func_20939);
		cpu_.b = OBJACTION_12;
		LJUMP(SetObjAction);
	}

	void port_Func_20d1d()
	{
		LJUMP(Func_2022c);
	}

	// sting on the object's front (front = true) or back
	void obj_sting(bool front)
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(StepOnObject);
			return;
		}
		ld_cp(wObjDirection, DIRECTION_RIGHT);
		int bit = (ZF != 0) == front ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
		if (ld_bit(wInteractionSide, bit))
			LJUMP(Func_20670);
		else
			LJUMP(Func_2022c);
	}

	void port_ObjInteraction_FrontSting() { obj_sting(true); }
	void port_ObjInteraction_BackSting() { obj_sting(false); }

	void port_ObjInteraction_TopSting()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			LJUMP(Func_20670);
			return;
		}
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(StepOnObject);
			return;
		}
		LJUMP(Func_2022c);
	}

	void port_ObjInteraction_FullSting()
	{
		LJUMP(Func_20670);
	}

	void port_Func_20e39()
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(StepOnObject);
			return;
		}
		ld_cp(wObjDirection, DIRECTION_RIGHT);
		int bit = ZF ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
		if (ld_bit(wInteractionSide, bit))
			LJUMP(Func_20939);
		else
			LJUMP(Func_2022c);
	}

	void port_Func_20e60()
	{
		LJUMP(Func_20939);
	}

	void port_Func_20e6a()
	{
		LCALL(Func_20939);
		if (ld_and(wTouchState)) {
			RET();
			return;
		}
		cpu_.b = OBJACTION_0A;
		LJUMP(SetObjAction);
	}

	void port_Func_20e63()
	{
		if (ld_and(wAttackCounter)) {
			LJUMP(AttackObject);
			return;
		}
		LJUMP(Func_20e6a); // fallthrough
	}

	void port_ObjInteraction_Walkable()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F))
			LJUMP(Func_21aac);
		else
			LJUMP(Func_20932);
	}

	void port_ObjInteraction_Fire()
	{
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		st(wTransformation, TRANSFORMATION_HOT_WARIO);
		st(wTransformationProgress, 1);
		touch_states(TOUCH_BUMP, 0x02);
		st(wTransformationDuration + 0, (uint8_t)(HOT_WARIO_DURATION >> 8));
		st(wTransformationDuration + 1, (uint8_t)HOT_WARIO_DURATION);
		CALL(UpdateLevelMusic);
		LJUMP(SetState_OnFire_ResetStateCounter); // fallthrough
	}

	void port_Func_2126a()
	{
		if (ld_and(wIsCrouching)) {
			st(wCollisionBoxTop, (uint8_t)-27);
			FARCALL(CheckUpCollision);
			if (b_and()) {
				st(wCollisionBoxTop, (uint8_t)-15);
				LJUMP(Func_2022c);
				return;
			}
		}
		// .asm_21290
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2023b);
			return;
		}
		cpu_.b = rd8(wWarioScreenXPos);
		cpu_.a = rd8(wObjScreenXPos);
		alu_.cp(cpu_.a, cpu_.b);
		st(wDirection, CF ? DIRECTION_RIGHT : DIRECTION_LEFT);
		// .set_ball_o_string
		play_sfx(SFX_039);
		st(wTransformation, TRANSFORMATION_BALL_O_STRING_WARIO);
		xor_a();
		wr8(wTransformationProgress, 0);
		touch_states(TOUCH_ATTACK, 0x01);
		CALL(UpdateLevelMusic);
		st(wWarioState, WST_GETTING_WRAPPED_IN_STRING);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		SET_HL(gb_, ADDR_WarioStringPal);
		CALL(SetWarioPal);
		load_gfx(BANK_WarioStringGfx, ADDR_WarioStringGfx);
		set_oam(BANK_OAM_171c0, ADDR_OAM_171c0);
		set_frameset(ADDR_Frameset_17414);
		anim(OAM1);
		RET();
	}

	void port_ObjInteraction_Silky()
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(StepOnObject);
			return;
		}
		ld_cp(wObjDirection, DIRECTION_RIGHT);
		int bit = ZF ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
		if (ld_bit(wInteractionSide, bit))
			LJUMP(Func_2126a);
		else
			LJUMP(Func_2022c);
	}

	// Whether Wario touched the object's front (front) or back:
	// ld a, [wObjDirection] / cp DIRECTION_RIGHT / jr z (use_cp), or and a / jr nz,
	// then ld a, [wInteractionSide] / bit INTERACTION_LEFT_F or _RIGHT_F, a
	bool obj_side(bool use_cp, bool front)
	{
		bool right;
		if (use_cp) {
			ld_cp(wObjDirection, DIRECTION_RIGHT);
			right = ZF != 0;
		} else {
			right = ld_and(wObjDirection);
		}
		return ld_bit(wInteractionSide, right == front ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F);
	}

	// ld a, [wInteractionSide] / and INTERACTION_UP | INTERACTION_DOWN / jp nz, StepOnObject
	bool vert_step()
	{
		if (!ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN))
			return false;
		LJUMP(StepOnObject);
		return true;
	}

	void zero_pals_hblank(uint16_t index_reg, uint8_t index, uint8_t n, uint16_t data_reg)
	{
		cpu_.a = index;
		wr8(index_reg, index);
		cpu_.b = n;
		cpu_.c = (uint8_t)data_reg;
		do {
			// wait_not_hblank / wait_hblank
			while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
				idle(32);
			while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
				idle(32);
			xor_a();
			for (int i = 0; i < 2 * PAL_SIZE; i++)
				wr8((uint16_t)(0xff00 + cpu_.c), 0);
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
	}

	void port_ObjInteraction_Teruteru()
	{
		if (!ld_bit(wInteractionSide, INTERACTION_DOWN_F)) {
			LJUMP(Func_2022c);
			return;
		}
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_BLIND);
		st(wTouchState, TOUCH_BUMP);
		st(wStingTouchState, TOUCH_VULNERABLE);
		st(wca94, 0x01);
		FARCALL(SetState_BlindIdling);
		zero_pals_hblank(rBGPI, BGPI_AUTOINC | (0 * PAL_SIZE), 4, rBGPD);
		zero_pals_hblank(rOBPI, OBPI_AUTOINC | (3 * PAL_SIZE), 2, rOBPD);
		RET();
	}

	void port_ObjInteraction_Beam()
	{
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_SPLIT);
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		FARCALL(SetState_SplitHit);
		RET();
	}

	void port_Func_21b3a()
	{
		LCALL(Func_20939);
		cpu_.b = OBJACTION_NONE;
		LJUMP(SetObjAction);
	}

	void port_Func_21b78()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F) && ld_and(wIsSmashAttacking)) {
			LJUMP(Func_20602);
			return;
		}
		LJUMP(Func_20939);
	}

	void port_ObjInteraction_YellowBellyArrow()
	{
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_PUFFY_WARIO);
		if (ZF) {
			LJUMP(Func_2022c);
			return;
		}
		ld_cp(wTransformation, TRANSFORMATION_OWL_WARIO);
		if (ZF)
			CALL(ReleaseOwl);
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		FARCALL(SetState_PuffyInflating);
		RET();
	}

	void port_ObjInteraction_Pneumo()
	{
		if (ld_bit(wInteractionSide, INTERACTION_DOWN_F))
			LJUMP(ObjInteraction_YellowBellyArrow);
		else
			LJUMP(Func_2022c);
	}

	void port_Func_2160a()
	{
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_BOUNCY_WARIO);
		if (ZF) {
			LJUMP(Func_2022c);
			return;
		}
		st(wTransformation, TRANSFORMATION_BOUNCY_WARIO);
		touch_states(TOUCH_BUMP, 0x01);
		st(wTransformationDuration + 0, (uint8_t)(BOUNCY_WARIO_DURATION >> 8));
		st(wTransformationDuration + 1, (uint8_t)BOUNCY_WARIO_DURATION);
		FARCALL(SetState_BouncyStart);
		RET();
	}

	void port_ObjInteraction_Bouncy()
	{
		if (vert_step())
			return;
		if (obj_side(true, true))
			LJUMP(Func_2160a);
		else
			LJUMP(Func_2022c);
	}

	void port_ObjInteraction_FireBot()
	{
		if (vert_step())
			return;
		if (obj_side(true, true))
			LJUMP(ObjInteraction_Fire);
		else
			LJUMP(Func_2022c);
	}

	void port_Func_21c56()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F))
			LJUMP(Func_21aac);
		else
			LJUMP(Func_20b6b);
	}

	void port_ObjInteraction_Tadpole()
	{
		if (vert_step())
			return;
		if (obj_side(false, true))
			LJUMP(ObjInteraction_RollingRock);
		else
			LJUMP(Func_20e6a);
	}

	void port_ObjInteraction_Hebarii()
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(Func_20e6a);
			return;
		}
		if (ld_and(wAttackCounter))
			LJUMP(AttackObject);
		else
			LJUMP(Func_20e6a);
	}

	void port_ObjInteraction_PrinceFroggy()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F))
			LJUMP(Func_209ca);
		else if (alu_.bit(cpu_.a, INTERACTION_DOWN_F))
			LJUMP(Func_2168b);
		else
			LJUMP(Func_20b41);
	}

	void port_Func_21d3b()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F) || alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(Func_20e6a);
			return;
		}
		if (obj_side(false, true))
			LJUMP(Func_20670);
		else
			LJUMP(Func_20e6a);
	}

	void port_Func_21d64()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F))
			LJUMP(Func_20670);
		else
			LJUMP(Func_20e6a);
	}

	// Func_21d6f.asm_21d7e
	void func_21d7e()
	{
		if (ld_and(wIsSmashAttacking))
			LJUMP(Func_20602);
		else
			LJUMP(Func_20e6a);
	}

	void port_Func_21d6f()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F))
			func_21d7e();
		else if (alu_.bit(cpu_.a, INTERACTION_DOWN_F))
			LJUMP(Func_20670);
		else
			LJUMP(Func_20e6a);
	}

	void port_Func_21f28()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			func_21d7e();
			return;
		}
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(Func_20e6a);
			return;
		}
		if (obj_side(false, true))
			LJUMP(Func_20670);
		else
			LJUMP(Func_20e6a);
	}

	void port_Func_21dac()
	{
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		cpu_.a = rd8(wObjDirection);
		wr8(wDirection, cpu_.a);
		FARCALL(SetState_Launched);
		cpu_.b = OBJACTION_06;
		LJUMP(SetObjAction);
	}

	void port_Func_21d88()
	{
		if (ld_mask(wInteractionSide, INTERACTION_UP | INTERACTION_DOWN)) {
			LJUMP(Func_20899);
			return;
		}
		if (obj_side(false, true))
			LJUMP(Func_21dac);
		else
			LJUMP(Func_208f2);
	}

	void port_Func_21ea6()
	{
		cpu_.a = rd8(wObjDirection);
		cpu_.b = cpu_.a;
		bool right = alu_.and_(cpu_.a, cpu_.a) != 0;
		if (ld_bit(wInteractionSide, right ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F)) {
			cpu_.a = alu_.xor_(rd8(wObjDirection), 0x1);
			wr8(wDirection, cpu_.a);
			LJUMP(Func_20a6f);
			return;
		}
		LJUMP(Func_21b3a);
	}

	void port_Func_21ecd()
	{
		if (!obj_side(false, true)) {
			LJUMP(Func_20e6a);
			return;
		}
		play_sfx(SFX_FAT_WALK);
		FARCALL(SetState_DraggedDown);
		LJUMP(Func_21cc9);
	}

	void port_ObjInteraction_MagicalSpiral()
	{
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_MAGIC);
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		FARCALL(SetState_MagicRising);
		RET();
	}

	void port_ObjInteraction_BottomSting()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F))
			LJUMP(Func_20670);
		else if (alu_.bit(cpu_.a, INTERACTION_UP_F))
			LJUMP(StepOnObject);
		else
			LJUMP(Func_2022c);
	}

	void port_ObjInteraction_AnonsterSilk()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F))
			LJUMP(StepOnObject);
		else
			LJUMP(ObjInteraction_AnonsterWave); // fallthrough
	}

	void port_ObjInteraction_AnonsterWave()
	{
		LJUMP(Func_2126a);
	}

	void port_ObjInteraction_RoboMouse()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F)) {
			if (ld_and(wIsSmashAttacking))
				LJUMP(Func_20602);
			else
				LJUMP(Func_20e6a);
			return;
		}
		if (obj_side(false, true))
			LJUMP(ObjInteraction_Jamano);
		else
			LJUMP(Func_20e6a);
	}

	void port_Func_21af1()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			LJUMP(Func_209ca);
			return;
		}
		if (alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(StepOnObject);
			return;
		}
		if (ld_and(wAttackCounter))
			LJUMP(Func_217a9);
		else
			LJUMP(Func_20a6f);
	}

	void port_ObjInteraction_DollBoyHammer()
	{
		LJUMP(Func_2160a);
	}

	void port_ObjInteraction_Wormwould()
	{
		if (vert_step())
			return;
		if (ld_and(wAttackCounter))
			LJUMP(AttackObject);
		else
			LJUMP(Func_21b3a); // fallthrough
	}

	void port_ObjInteraction_Turtle()
	{
		if (ld_bit(wInteractionSide, INTERACTION_UP_F)) {
			LJUMP(Func_209ca);
			return;
		}
		if (ld_and(wTransformation)) {
			RET();
			return;
		}
		if (!ld_and(wWaterInteraction)) {
			LJUMP(Func_21b3a);
			return;
		}
		FARCALL(StartDive);
		RET();
	}

	void port_ObjInteraction_Shoot()
	{
		ld_cp(wTransformation, TRANSFORMATION_BALL);
		if (ZF) {
			LJUMP(Func_21e8a);
			return;
		}
		cpu_.a = rd8(wInteractionSide);
		if (!alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			if (alu_.bit(cpu_.a, INTERACTION_UP_F))
				LJUMP(StepOnObject);
			else
				LJUMP(Func_20e63);
			return;
		}
		// .interaction_down
		if (ld_and(wInvincibleCounter)) {
			RET(); // skip if invincible
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		// turn Wario into ball form
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_BALL);
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		FARCALL(SetState_BallStart);
		RET();
	}

	void port_Func_21e9c()
	{
		ld_cp(wTransformation, TRANSFORMATION_BALL);
		if (ZF)
			LJUMP(Func_21e8a);
		else
			LJUMP(Func_20939);
	}

	void port_ObjInteraction_Richtertoffen()
	{
		cpu_.a = rd8(wInteractionSide);
		if (alu_.bit(cpu_.a, INTERACTION_UP_F)) {
			cpu_.a = alu_.dec(rd8(wIsSmashAttacking));
			if (ZF)
				LJUMP(Func_20602);
			else
				LJUMP(StepOnObject);
			return;
		}
		if (!alu_.bit(cpu_.a, INTERACTION_DOWN_F)) {
			LJUMP(Func_20932);
			return;
		}
		// .try_flatten_wario
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		st(wCollisionBoxLeft, (uint8_t)-11);
		st(wCollisionBoxRight, 11);
		ld_cp(wLadderInteraction, LADDER_UNDERNEATH);
		if (!ZF) {
			FARCALL(Func_197b1);
			if (!b_and()) {
				FARCALL(Func_19741);
				if (!b_and()) {
					cpu_.b = OBJACTION_FLATTEN;
					LCALL(SetObjAction);
					FARCALL(SetState_FlatAirborne);
					RET();
					return;
				}
			}
		}
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);
		LJUMP(StepOnObject);
	}

	void port_ObjInteraction_Omodonmeka()
	{
		if (ld_and(wJumpVelTable)) {
			LJUMP(Func_2022c);
			return;
		}
		cpu_.b = OBJACTION_0C;
		LCALL(SetObjAction);
		ld_cp(wTransformation, TRANSFORMATION_FLAT_WARIO);
		if (ZF) {
			LJUMP(SetState_FlatStretching);
			return;
		}
		if (alu_.and_(cpu_.a, cpu_.a)) {
			LJUMP(Func_2022c);
			return;
		}
		// pick Wario up
		st(wAutoMoveState, 0x01);
		st(wWarioState, WST_PICKED_UP);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wIsSmashAttacking, wAttackCounter, wIsCrouching,
			wGrabState, wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		st(wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
		RET();
	}

	void port_ObjInteraction_OrangeBird()
	{
		if (vert_step())
			return;
		if (!obj_side(true, true)) {
			LJUMP(Func_2022c);
			return;
		}
		// .asm_217de
		if (ld_and(wInvincibleCounter)) {
			RET();
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			LJUMP(Func_2022c);
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_CRAZY_WARIO);
		if (ZF) {
			LJUMP(Func_2022c);
			return;
		}
		cpu_.b = OBJACTION_06;
		LCALL(SetObjAction);
		st(wTransformation, TRANSFORMATION_CRAZY_WARIO);
		touch_states(TOUCH_ATTACK, 0x01);
		FARCALL(SetState_CrazySpinning);
		RET();
	}

	void port_ObjInteraction_Owl()
	{
		if (!ld_bit(wInteractionSide, INTERACTION_DOWN_F)) {
			LJUMP(Func_20939);
			return;
		}
		if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F)) {
			RET();
			return;
		}
		alu_.cp(cpu_.a, TRANSFORMATION_OWL_WARIO);
		if (ZF) {
			RET();
			return;
		}
		LCALL(Func_20a63);
		play_sfx(SFX_HANG_GRAB);
		st(wTransformation, TRANSFORMATION_OWL_WARIO);
		st(wTouchState, TOUCH_BUMP);
		st(wStingTouchState, TOUCH_VULNERABLE);
		st(wca94, 0x01);
		st(wWarioState, WST_GRABBING_OWL);
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		CALL(UpdateLevelMusic);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		load_gfx(BANK_WarioHangGfx, ADDR_WarioHangGfx);
		set_oam(BANK_OAM_1fddb4, ADDR_OAM_1fddb4);
		cpu_.a = rd8(wObjDirection);
		wr8(wDirection, cpu_.a);
		set_frameset(ADDR_Frameset_1fed53);
		anim(OAM2);
		RET();
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::ObjectInteractions;

PORT_EX(ProcessInteractions, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ProcessInteractions(); }
PORT_LOCAL_EX(ProcessInteractions, next_obj, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ProcessInteractions__next_obj(); }
PORT_EX(SetObjAction, 0, 0) { ObjectInteractions(gb).port_SetObjAction(); }
PORT_EX(Func_20a63, 0, 0) { ObjectInteractions(gb).port_Func_20a63(); }
PORT_EX(Func_20a69, 0, 0) { ObjectInteractions(gb).port_Func_20a69(); }
PORT_EX(Func_21cc9, 0, 0) { ObjectInteractions(gb).port_Func_21cc9(); }
PORT_EX(CollectKey, 0, 0) { ObjectInteractions(gb).port_CollectKey(); }
PORT_EX(SetState_OnFire, 0, 0) { ObjectInteractions(gb).port_SetState_OnFire(); }
PORT_EX(VanishObjectAndKnockBackIfZombie, 0, 0) { ObjectInteractions(gb).port_VanishObjectAndKnockBackIfZombie(); }
PORT_EX(SetState_OnFire_ResetStateCounter, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_SetState_OnFire_ResetStateCounter(); }
PORT_EX(ObjInteraction_RedKey, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_RedKey(); }
PORT_EX(ObjInteraction_GreyKey, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_GreyKey(); }
PORT_EX(ObjInteraction_GreenKey, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_GreenKey(); }
PORT_EX(ObjInteraction_BlueKey, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_BlueKey(); }
PORT_EX(Func_20fe8, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20fe8(); }
PORT_EX(ObjInteraction_None, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_None(); }
PORT_EX(ObjInteraction_Ink, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Ink(); }
PORT_EX(Func_2092d, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_2092d(); }
PORT_EX(ObjInteraction_Hand, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Hand(); }
PORT_EX(ObjInteraction_UnlitTorch, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_UnlitTorch(); }
PORT_EX(SetState_FlatStretching, 0, 0) { ObjectInteractions(gb).port_SetState_FlatStretching(); }
PORT_EX(TreasureClearObjects, 0, 0) { ObjectInteractions(gb).port_TreasureClearObjects(); }
PORT_EX(SetState_EnemyBumping, 0, 0) { ObjectInteractions(gb).port_SetState_EnemyBumping(); }
PORT_EX(ObjInteraction_Food, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Food(); }
PORT_EX(ObjInteraction_Electric, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_Electric(); }
PORT_EX(ObjInteraction_RegularCoin, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_RegularCoin(); }
PORT_EX(CollectMusicalCoin, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_CollectMusicalCoin(); }
PORT_EX(ObjInteraction_ColourCoin, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_ColourCoin(); }
PORT_EX(ObjInteraction_MusicalCoin, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_MusicalCoin(); }
PORT_EX(ObjInteraction_InvisibilityPotion, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_InvisibilityPotion(); }
PORT_EX(ObjInteraction_RollingRock, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_RollingRock(); }
PORT_EX(ObjInteraction_Vampire, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_Vampire(); }
PORT_EX(ObjInteraction_Bubble, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_Bubble(); }
PORT_EX(ObjInteraction_ZombieHead, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_ZombieHead(); }
PORT_EX(Func_2028a, 0, 0) { ObjectInteractions(gb).port_Func_2028a(); }
PORT_EX(Func_208f2, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_208f2(); }
PORT_LOCAL_EX(Func_208f2, start, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_208f2__start(); }
PORT_EX(Func_20899, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20899(); }
PORT_EX(SetState_WaterStung, 0, 0) { ObjectInteractions(gb).port_SetState_WaterStung(); }
PORT_EX(ObjInteraction_Rail, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_Rail(); }
PORT_EX(GetTreasure, 0, 0) { ObjectInteractions(gb).port_GetTreasure(); }
PORT_EX(SetState_Stung, 0, 0) { ObjectInteractions(gb).port_SetState_Stung(); }
PORT_EX(Func_206eb, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_206eb(); }
PORT_EX(Func_207ed, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_207ed(); }
PORT_EX(ObjInteraction_WaterTeleporting, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_WaterTeleporting(); }
PORT_EX(ObjInteraction_Jamano, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Jamano(); }
PORT_EX(Func_21e8a, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21e8a(); }
PORT_EX(Func_20d81, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20d81(); }
PORT_EX(Func_2168b, 0, 0) { ObjectInteractions(gb).port_Func_2168b(); }
PORT_EX(ObjInteraction_Snow, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Snow(); }
PORT_EX(ObjInteraction_Ice, 0, 0) { ObjectInteractions(gb).port_ObjInteraction_Ice(); }
PORT_EX(Func_209ca, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_209ca(); }
PORT_EX(Func_20a6f, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20a6f(); }
PORT_EX(AttackObject, 0, 0) { ObjectInteractions(gb).port_AttackObject(); }
PORT_EX(Func_217a9, 0, 0) { ObjectInteractions(gb).port_Func_217a9(); }
PORT_EX(Func_20d8c, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20d8c(); }
PORT_EX(Func_20deb, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20deb(); }
PORT_EX(ObjInteraction_Haridama, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Haridama(); }
PORT_EX(ObjInteraction_Pesce, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Pesce(); }
PORT_EX(ObjInteraction_Scowler, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Scowler(); }
PORT_EX(ObjInteraction_GreyTreasure, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_GreyTreasure(); }
PORT_EX(ObjInteraction_RedTreasure, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_RedTreasure(); }
PORT_EX(ObjInteraction_GreenTreasure, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_GreenTreasure(); }
PORT_EX(ObjInteraction_BlueTreasure, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_BlueTreasure(); }
PORT_EX(ObjInteraction_Solid, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Solid(); }
PORT_EX(ObjInteraction_Omodon, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Omodon(); }
PORT_EX(Func_21abb, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21abb(); }
PORT_EX(Func_21f01, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21f01(); }
PORT_EX(Func_20602, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20602(); }
PORT_EX(Func_21aac, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21aac(); }
PORT_EX(ObjInteraction_Stove, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Stove(); }
PORT_EX(ObjInteraction_WaterDrop, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_WaterDrop(); }
PORT_EX(Func_2023b, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_2023b(); }
PORT_EX(Func_2022c, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_2022c(); }
PORT_EX(Func_20939, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20939(); }
PORT_EX(Func_20932, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20932(); }
PORT_EX(StepOnObject, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_StepOnObject(); }
PORT_EX(Func_20670, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20670(); }
PORT_EX(Func_20b6b, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20b6b(); }
PORT_EX(Func_20b41, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20b41(); }
PORT_EX(Func_20d1d, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20d1d(); }
PORT_EX(ObjInteraction_FrontSting, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_FrontSting(); }
PORT_EX(ObjInteraction_BackSting, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_BackSting(); }
PORT_EX(ObjInteraction_TopSting, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_TopSting(); }
PORT_EX(ObjInteraction_FullSting, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_FullSting(); }
PORT_EX(Func_20e39, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20e39(); }
PORT_EX(Func_20e60, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20e60(); }
PORT_EX(Func_20e6a, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20e6a(); }
PORT_EX(Func_20e63, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_20e63(); }
PORT_EX(ObjInteraction_Walkable, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Walkable(); }
PORT_EX(ObjInteraction_Fire, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Fire(); }
PORT_EX(Func_2126a, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_2126a(); }
PORT_EX(ObjInteraction_Silky, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Silky(); }
PORT_EX(ObjInteraction_Teruteru, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Teruteru(); }
PORT_EX(ObjInteraction_Beam, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Beam(); }
PORT_EX(Func_21b3a, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21b3a(); }
PORT_EX(Func_21b78, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21b78(); }
PORT_EX(ObjInteraction_YellowBellyArrow, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_YellowBellyArrow(); }
PORT_EX(ObjInteraction_Pneumo, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Pneumo(); }
PORT_EX(Func_2160a, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_2160a(); }
PORT_EX(ObjInteraction_Bouncy, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Bouncy(); }
PORT_EX(ObjInteraction_FireBot, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_FireBot(); }
PORT_EX(Func_21c56, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21c56(); }
PORT_EX(ObjInteraction_Tadpole, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Tadpole(); }
PORT_EX(ObjInteraction_Hebarii, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Hebarii(); }
PORT_EX(ObjInteraction_PrinceFroggy, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_PrinceFroggy(); }
PORT_EX(Func_21d3b, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21d3b(); }
PORT_EX(Func_21d64, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21d64(); }
PORT_EX(Func_21d6f, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21d6f(); }
PORT_EX(Func_21f28, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21f28(); }
PORT_EX(Func_21dac, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21dac(); }
PORT_EX(Func_21d88, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21d88(); }
PORT_EX(Func_21ea6, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21ea6(); }
PORT_EX(Func_21ecd, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21ecd(); }
PORT_EX(ObjInteraction_MagicalSpiral, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_MagicalSpiral(); }
PORT_EX(ObjInteraction_BottomSting, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_BottomSting(); }
PORT_EX(ObjInteraction_AnonsterSilk, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_AnonsterSilk(); }
PORT_EX(ObjInteraction_AnonsterWave, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_AnonsterWave(); }
PORT_EX(ObjInteraction_RoboMouse, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_RoboMouse(); }
PORT_EX(Func_21af1, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21af1(); }
PORT_EX(ObjInteraction_DollBoyHammer, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_DollBoyHammer(); }
PORT_EX(ObjInteraction_Wormwould, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Wormwould(); }
PORT_EX(ObjInteraction_Turtle, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Turtle(); }
PORT_EX(ObjInteraction_Shoot, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Shoot(); }
PORT_EX(Func_21e9c, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_Func_21e9c(); }
PORT_EX(ObjInteraction_Richtertoffen, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Richtertoffen(); }
PORT_EX(ObjInteraction_Omodonmeka, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Omodonmeka(); }
PORT_EX(ObjInteraction_OrangeBird, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_OrangeBird(); }
PORT_EX(ObjInteraction_Owl, HOOK_MANUAL_RET, 0) { ObjectInteractions(gb).port_ObjInteraction_Owl(); }
