// Wario's interactions with objects (src/engine/level/object_interactions.asm):
// ProcessInteractions finds the objects Wario touches and runs the
// interaction of each (by its type): collecting coins, keys, treasure and
// musical coins, being stung or bumped, stepping on, grabbing and
// attacking objects, the transformations objects cause, and more.
//
// Called by the level's frame and register-level C, so these keep the
// original's registers, flags and return addresses (see asm_module.hpp).
#include "frame.hpp"
#include "../asm_module.hpp"
#include "../home/system.hpp"
#include "../home/wario.hpp"
#include "../routines.hpp"
#include "object.hpp"

namespace wl3::level {

namespace {

class ObjectInteractions : public AsmModule {
public:
	using AsmModule::AsmModule;

	// a farcalled collision check: its result
	uint8_t collision(uint8_t bank, uint16_t addr, uint8_t (*check)(GB *))
	{
		uint8_t result = 0;
		Routines(gb_).in_bank(bank, addr, [&] { result = check(gb_); });
		return result;
	}

	#define ProcessInteractions_table (ADDR_ProcessInteractions + 0x150) // after the $ef

	// an edge of a collision box on screen: pos + the box's offset - $d0
	uint8_t edge(uint16_t pos, uint16_t box) { return static_cast<uint8_t>(rd8(pos) + rd8(box) - 0xd0); }

	// How far two boxes overlap along one axis, from both ends (false if
	// they don't): Wario's box from a_start to a_end, the object's from
	// b_start to b_end
	bool overlap(uint8_t a_end, uint8_t b_start, uint8_t b_end, uint8_t a_start, uint8_t &first, uint8_t &second)
	{
		if (a_end < b_start || b_end < a_start)
			return false;
		first = static_cast<uint8_t>(a_end - b_start);
		second = static_cast<uint8_t>(b_end - a_start);
		return true;
	}

	// the smaller overlap in lo (both, if equal), else in hi
	void record_overlap(uint8_t first, uint8_t second, uint16_t lo, uint16_t hi)
	{
		if (second >= first) {
			wr8(lo, first);
			if (second == first)
				wr8(hi, second);
		} else {
			wr8(hi, second);
		}
	}

	// ProcessInteractions from .loop_objects (or .next_obj, after the object
	// at wObjPtr): the next active, on-screen object Wario overlaps, its
	// interaction data copied to wObj*, the sides he's touching it from in
	// wInteractionSide; the handler of its interaction, or 0 past the
	// objects. (The handlers read no register: the asm's entry liveness.)
	uint16_t pi_run(bool next)
	{
		uint16_t obj = next ? static_cast<uint16_t>((rd8(wObjPtr + 0) << 8 | rd8(wObjPtr + 1)) + OBJ_STRUCT_LENGTH)
				    : static_cast<uint16_t>(wObjects);
		for (;; obj = static_cast<uint16_t>(obj + OBJ_STRUCT_LENGTH)) {
			if ((obj >> 8) == (wObjects >> 8) + 1)
				return 0; // past the last object
			wr8(wObjPtr + 0, static_cast<uint8_t>(obj >> 8));
			wr8(wObjPtr + 1, static_cast<uint8_t>(obj));
			if ((rd8(obj) & (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN)) != (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN))
				continue;
			mem_.unused_c1b8() = rd8(static_cast<uint16_t>(obj + OBJ_UNK_1D));
			if (rd8(static_cast<uint16_t>(obj + OBJ_UNK_1D - 1)))
				continue;
			static const uint16_t dst[] = {wObjInteractionType, wObjCollBoxTop, wObjCollBoxBottom,
				wObjCollBoxLeft, wObjCollBoxRight, wObjScreenYPos, wObjScreenXPos};
			for (int i = 0; i < 7; i++)
				wr8(dst[i], rd8(static_cast<uint16_t>(obj + OBJ_INTERACTION_TYPE + i)));
			bool right = rd8(static_cast<uint16_t>(obj + OBJ_SUBSTATE)) & (1 << OBJSUBFLAG_HDIR_F);
			mem_.obj_direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
			mem_.interaction_side() = 0;
			for (uint16_t at : {wc1c2, wc1c1, wc1c3, wc1c4})
				wr8(at, 0xff);

			uint8_t y1, y2, x1, x2;
			if (!overlap(edge(wWarioScreenYPos, wCollisionBoxBottom), edge(wObjScreenYPos, wObjCollBoxTop),
				    edge(wObjScreenYPos, wObjCollBoxBottom), edge(wWarioScreenYPos, wCollisionBoxTop), y1, y2))
				continue;
			record_overlap(y1, y2, wc1c1, wc1c2);
			if (!overlap(edge(wWarioScreenXPos, wCollisionBoxRight), edge(wObjScreenXPos, wObjCollBoxLeft),
				    edge(wObjScreenXPos, wObjCollBoxRight), edge(wWarioScreenXPos, wCollisionBoxLeft), x1, x2))
				continue;
			record_overlap(x1, x2, wc1c3, wc1c4);

			// the side Wario's touching it from: the smaller overlap of each
			// axis, then of the two
			uint8_t side = INTERACTION_LEFT | INTERACTION_RIGHT | INTERACTION_UP | INTERACTION_DOWN;
			uint8_t x = mem_.unk_c1c4(), x_left = mem_.unk_c1c3();
			if (x_left != x) {
				if (x_left > x) {
					side &= static_cast<uint8_t>(~INTERACTION_RIGHT);
				} else {
					side &= static_cast<uint8_t>(~INTERACTION_LEFT);
					x = x_left;
				}
			}
			uint8_t y = mem_.unk_c1c2(), y_up = mem_.unk_c1c1();
			if (y_up != y) {
				if (y_up > y) {
					side &= static_cast<uint8_t>(~INTERACTION_UP);
				} else {
					side &= static_cast<uint8_t>(~INTERACTION_DOWN);
					y = y_up;
				}
			}
			if (x != y)
				side &= static_cast<uint8_t>(x > y ? ~(INTERACTION_LEFT | INTERACTION_RIGHT)
								   : ~(INTERACTION_UP | INTERACTION_DOWN));
			mem_.interaction_side() = side;

			// .do_interaction: the jumptable after the $ef
			uint16_t entry = static_cast<uint16_t>(ProcessInteractions_table + (mem_.obj_interaction_type() & INTERACTION_MASK) * 2);
			return static_cast<uint16_t>(rd8(entry) | rd8(static_cast<uint16_t>(entry + 1)) << 8);
		}
	}

	void port_ProcessInteractions()
	{
		if (mem_.is_intangible() || mem_.is_floor_transition() || mem_.auto_move_state())
			return;
		mem_.is_on_steppable_object() = 0;
		// each interaction ends the loop (the asm's ret) or goes on with the
		// next object (its jp .next_obj)
		for (bool next = false;; next = true) {
			uint16_t handler = pi_run(next);
			if (!handler)
				return;
			next_obj_ = false;
			interaction(handler);
			if (!next_obj_)
				return;
		}
	}

	// the interaction at handler, as the asm's jumptable (.table) jumps there
	void interaction(uint16_t handler)
	{
		switch (handler) {
		case ADDR_Func_20b6b: port_Func_20b6b(); return;
		case ADDR_Func_20d1d: port_Func_20d1d(); return;
		case ADDR_ObjInteraction_FrontSting: port_ObjInteraction_FrontSting(); return;
		case ADDR_ObjInteraction_BackSting: port_ObjInteraction_BackSting(); return;
		case ADDR_ObjInteraction_TopSting: port_ObjInteraction_TopSting(); return;
		case ADDR_ObjInteraction_FullSting: port_ObjInteraction_FullSting(); return;
		case ADDR_Func_20d81: port_Func_20d81(); return;
		case ADDR_Func_20d8c: port_Func_20d8c(); return;
		case ADDR_Func_20deb: port_Func_20deb(); return;
		case ADDR_Func_20e39: port_Func_20e39(); return;
		case ADDR_Func_20e60: port_Func_20e60(); return;
		case ADDR_Func_20e6a: port_Func_20e6a(); return;
		case ADDR_ObjInteraction_Walkable: port_ObjInteraction_Walkable(); return;
		case ADDR_ObjInteraction_MusicalCoin: port_ObjInteraction_MusicalCoin(); return;
		case ADDR_ObjInteraction_Fire: port_ObjInteraction_Fire(); return;
		case ADDR_ObjInteraction_Richtertoffen: port_ObjInteraction_Richtertoffen(); return;
		case ADDR_ObjInteraction_GreyKey: port_ObjInteraction_GreyKey(); return;
		case ADDR_ObjInteraction_RedKey: port_ObjInteraction_RedKey(); return;
		case ADDR_ObjInteraction_GreenKey: port_ObjInteraction_GreenKey(); return;
		case ADDR_ObjInteraction_BlueKey: port_ObjInteraction_BlueKey(); return;
		case ADDR_ObjInteraction_GreyTreasure: port_ObjInteraction_GreyTreasure(); return;
		case ADDR_ObjInteraction_RedTreasure: port_ObjInteraction_RedTreasure(); return;
		case ADDR_ObjInteraction_GreenTreasure: port_ObjInteraction_GreenTreasure(); return;
		case ADDR_ObjInteraction_BlueTreasure: port_ObjInteraction_BlueTreasure(); return;
		case ADDR_ObjInteraction_Omodonmeka: port_ObjInteraction_Omodonmeka(); return;
		case ADDR_ObjInteraction_Silky: port_ObjInteraction_Silky(); return;
		case ADDR_ObjInteraction_Food: port_ObjInteraction_Food(); return;
		case ADDR_ObjInteraction_RegularCoin: port_ObjInteraction_RegularCoin(); return;
		case ADDR_ObjInteraction_Electric: port_ObjInteraction_Electric(); return;
		case ADDR_ObjInteraction_InvisibilityPotion: port_ObjInteraction_InvisibilityPotion(); return;
		case ADDR_ObjInteraction_Pneumo: port_ObjInteraction_Pneumo(); return;
		case ADDR_ObjInteraction_ZombieHead: port_ObjInteraction_ZombieHead(); return;
		case ADDR_ObjInteraction_Bouncy: port_ObjInteraction_Bouncy(); return;
		case ADDR_ObjInteraction_FireBot: port_ObjInteraction_FireBot(); return;
		case ADDR_ObjInteraction_Solid: port_ObjInteraction_Solid(); return;
		case ADDR_ObjInteraction_Omodon: port_ObjInteraction_Omodon(); return;
		case ADDR_ObjInteraction_OrangeBird: port_ObjInteraction_OrangeBird(); return;
		case ADDR_ObjInteraction_Vampire: port_ObjInteraction_Vampire(); return;
		case ADDR_ObjInteraction_Bubble: port_ObjInteraction_Bubble(); return;
		case ADDR_ObjInteraction_Ice: port_ObjInteraction_Ice(); return;
		case ADDR_ObjInteraction_Owl: port_ObjInteraction_Owl(); return;
		case ADDR_ObjInteraction_Rail: port_ObjInteraction_Rail(); return;
		case ADDR_ObjInteraction_Haridama: port_ObjInteraction_Haridama(); return;
		case ADDR_ObjInteraction_AnonsterWave: port_ObjInteraction_AnonsterWave(); return;
		case ADDR_ObjInteraction_AnonsterSilk: port_ObjInteraction_AnonsterSilk(); return;
		case ADDR_ObjInteraction_Snow: port_ObjInteraction_Snow(); return;
		case ADDR_ObjInteraction_UnlitTorch: port_ObjInteraction_UnlitTorch(); return;
		case ADDR_ObjInteraction_Stove: port_ObjInteraction_Stove(); return;
		case ADDR_ObjInteraction_RoboMouse: port_ObjInteraction_RoboMouse(); return;
		case ADDR_Func_21af1: port_Func_21af1(); return;
		case ADDR_ObjInteraction_DollBoyHammer: port_ObjInteraction_DollBoyHammer(); return;
		case ADDR_ObjInteraction_RollingRock: port_ObjInteraction_RollingRock(); return;
		case ADDR_ObjInteraction_Wormwould: port_ObjInteraction_Wormwould(); return;
		case ADDR_ObjInteraction_Beam: port_ObjInteraction_Beam(); return;
		case ADDR_Func_21b78: port_Func_21b78(); return;
		case ADDR_ObjInteraction_YellowBellyArrow: port_ObjInteraction_YellowBellyArrow(); return;
		case ADDR_ObjInteraction_Teruteru: port_ObjInteraction_Teruteru(); return;
		case ADDR_ObjInteraction_WaterDrop: port_ObjInteraction_WaterDrop(); return;
		case ADDR_ObjInteraction_Pesce: port_ObjInteraction_Pesce(); return;
		case ADDR_Func_21c56: port_Func_21c56(); return;
		case ADDR_ObjInteraction_Tadpole: port_ObjInteraction_Tadpole(); return;
		case ADDR_ObjInteraction_Hebarii: port_ObjInteraction_Hebarii(); return;
		case ADDR_ObjInteraction_PrinceFroggy: port_ObjInteraction_PrinceFroggy(); return;
		case ADDR_ObjInteraction_Hand: port_ObjInteraction_Hand(); return;
		case ADDR_ObjInteraction_WaterTeleporting: port_ObjInteraction_WaterTeleporting(); return;
		case ADDR_ObjInteraction_Scowler: port_ObjInteraction_Scowler(); return;
		case ADDR_ObjInteraction_Ink: port_ObjInteraction_Ink(); return;
		case ADDR_Func_207ed: port_Func_207ed(); return;
		case ADDR_ObjInteraction_Jamano: port_ObjInteraction_Jamano(); return;
		case ADDR_ObjInteraction_Turtle: port_ObjInteraction_Turtle(); return;
		case ADDR_Func_21d3b: port_Func_21d3b(); return;
		case ADDR_Func_21d64: port_Func_21d64(); return;
		case ADDR_Func_21d6f: port_Func_21d6f(); return;
		case ADDR_Func_21d88: port_Func_21d88(); return;
		case ADDR_ObjInteraction_ColourCoin: port_ObjInteraction_ColourCoin(); return;
		case ADDR_ObjInteraction_MagicalSpiral: port_ObjInteraction_MagicalSpiral(); return;
		case ADDR_ObjInteraction_BottomSting: port_ObjInteraction_BottomSting(); return;
		case ADDR_ObjInteraction_Shoot: port_ObjInteraction_Shoot(); return;
		case ADDR_Func_21e9c: port_Func_21e9c(); return;
		case ADDR_Func_21ea6: port_Func_21ea6(); return;
		case ADDR_Func_21ecd: port_Func_21ecd(); return;
		case ADDR_Func_21f01: port_Func_21f01(); return;
		case ADDR_Func_21f28: port_Func_21f28(); return;
		case ADDR_ObjInteraction_None: port_ObjInteraction_None(); return;
		}
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	bool next_obj_ = false; // an interaction went on to .next_obj

	// b = OBJACTION_* constant
	// Sets the action of the object at wObjPtr, and records which side Wario
	// interacted with it from.
	// SetObjAction: the action of the object at wObjPtr, and the sides
	// Wario touched it from (in OBJ_UNK_1D's high nibble)
	void set_obj_action(uint8_t action)
	{
		uint16_t obj = static_cast<uint16_t>(rd8(wObjPtr + 0) << 8 | rd8(wObjPtr + 1));
		uint16_t at = static_cast<uint16_t>(obj + OBJ_ACTION);
		wr8(at, action);
		at = static_cast<uint16_t>((at & 0xff00) | static_cast<uint8_t>(at + 1)); // inc l
		wr8(at, static_cast<uint8_t>((rd8(at) & 0x0f) | mem_.interaction_side()));
	}

	void port_Func_20a63()
	{
		set_obj_action(OBJACTION_07);
	}

	void port_Func_20a69()
	{
		set_obj_action(OBJACTION_0D);
	}

	void port_Func_21cc9()
	{
		set_obj_action(OBJACTION_13);
	}

	void port_CollectKey()
	{
		play_sfx(SFX_KEY);
		port_Func_20a63();
		mem_.state() = ST_COLLECT_KEY;
		mem_.timer() = 0;
	}

	void port_SetState_OnFire()
	{
		mem_.wario_state() = WST_ON_FIRE;
		static const uint16_t zero[] = {wInvisibleFrame, wSFXLoopCounter, wWarioStateCycles,
			wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		if (ld_and(wWarioStateCounter))
			return;
		home::set_wario_pal(gb_, ADDR_WarioOnFirePal);
		load_gfx(BANK_WarioHotGfx, ADDR_WarioHotGfx);
		set_oam(BANK_OAM_1673c, ADDR_OAM_1673c);
		xor_a();
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		frameset_by_dir(ADDR_Frameset_16b38, ADDR_Frameset_16b49);
		anim(OAM1);
	}

	void port_VanishObjectAndKnockBackIfZombie()
	{
		set_obj_action(OBJACTION_VANISH_TOUCH);
		if (mem_.transformation() == TRANSFORMATION_ZOMBIE_WARIO)
			FARCALL(SetState_ZombieKnockBack);
	}

	void port_SetState_OnFire_ResetStateCounter()
	{
		xor_a();
		mem_.wario_state_counter() = 0;
		port_SetState_OnFire(); // fallthrough
	}

	// ld hl, wKeyAndTreasureFlags / set bit, [hl] / jr CollectKey
	void obj_key(int bit)
	{
		mem_.key_and_treasure_flags() = static_cast<uint8_t>(mem_.key_and_treasure_flags() | (1 << bit));
		port_CollectKey();
	}

	void port_ObjInteraction_RedKey() { obj_key(RED_KEY_F); }
	void port_ObjInteraction_GreyKey() { obj_key(GREY_KEY_F); }
	void port_ObjInteraction_GreenKey() { obj_key(GREEN_KEY_F); }
	void port_ObjInteraction_BlueKey() { obj_key(BLUE_KEY_F); }

	void port_Func_20fe8()
	{
		set_obj_action(OBJACTION_06);
	}

	void port_ObjInteraction_None()
	{
	}

	void port_ObjInteraction_Ink()
	{
		set_obj_action(OBJACTION_06);
	}

	void port_Func_2092d()
	{
		set_obj_action(OBJACTION_VANISH_TOUCH);
	}

	void port_ObjInteraction_Hand()
	{
		if (mem_.invincible_counter() || mem_.transformation())
			return;
		play_sfx(SFX_FAT_WALK);
		FARCALL(SetState_DraggedDown);
		port_Func_21cc9(); // fallthrough
	}

	void port_ObjInteraction_UnlitTorch()
	{
		if (mem_.transformation() == TRANSFORMATION_HOT_WARIO)
			set_obj_action(OBJACTION_0E);
	}

	void port_SetState_FlatStretching()
	{
		mem_.auto_move_state() = 0x01;
		mem_.wario_state() = WST_FLAT_STRETCHING;
		collision_box(-1, -8, -11, 11);
		st(wPickedUpWiggleCounter, NUM_WIGGLES_TO_ESCAPE);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wFrameDuration,
			wAnimationFrame, 0};
		clear(zero);
		set_frameset(ADDR_Frameset_17193);
		anim(OAM1);
	}

	// every active object but this one (on screen or not) vanishes for the
	// treasure
	void port_TreasureClearObjects()
	{
		for (uint16_t obj = wObjects; (obj >> 8) != (wCurObj >> 8); obj = static_cast<uint16_t>(obj + OBJ_STRUCT_LENGTH)) {
			uint8_t flags = rd8(obj) & (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
			if (!(flags & OBJFLAG_ACTIVE) || rd8(wObjPtr + 1) == static_cast<uint8_t>(obj))
				continue;
			wr8(static_cast<uint16_t>(obj + OBJ_ACTION), OBJACTION_VANISH_TREASURE);
		}
	}

	void port_SetState_EnemyBumping()
	{
		play_sfx(SFX_BUMP);
		mem_.jump_vel_index() = 0x0e;
		mem_.jump_vel_table() = 0x01;
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wIsSmashAttacking, wGrabState, 0};
		clear(zero);
		mem_.wario_state() = WST_ENEMY_BUMPING;
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		if (mem_.is_crouching())
			frameset_by_dir(ADDR_Frameset_14a3b, ADDR_Frameset_14a38);
		else
			frameset_by_dir(ADDR_Frameset_14a71, ADDR_Frameset_14a6c);
		anim(OAM1);
	}

	void port_ObjInteraction_Food()
	{
		if (mem_.invincible_counter() && !mem_.is_crouching())
			return;
		uint8_t transformation = mem_.transformation();
		if (mem_.is_crouching() || (transformation & (1 << TRANSFORMATIONF_PERSISTENT_F))
			|| transformation == TRANSFORMATION_FAT_WARIO) {
			port_Func_20a69();
			return;
		}
		port_Func_20a63();
		mem_.transformation() = TRANSFORMATION_FAT_WARIO;
		touch_states(TOUCH_BUMP, 0x02);
		wr8(wTransformationDuration + 0, (uint8_t)(FAT_WARIO_DURATION >> 8));
		wr8(wTransformationDuration + 1, (uint8_t)FAT_WARIO_DURATION);
		static const uint16_t zero[] = {wInvisibleFrame, wSFXLoopCounter, wWarioStateCounter,
			wWarioStateCycles, wGrabState, wAttackCounter, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		if (mem_.jump_vel_table())
			mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		play_sfx(SFX_03A);
		home::update_level_music(gb_);
		mem_.wario_state() = WST_FAT_EATING;
		collision_box(-1, -27, -9, 9);
		reset_anim();
		load_gfx(BANK_WarioFatGfx, ADDR_WarioFatGfx);
		set_oam(BANK_OAM_1742d, ADDR_OAM_1742d);
		frameset_by_dir(ADDR_Frameset_1789c, ADDR_Frameset_178c3);
		anim(OAM1);
	}

	void port_ObjInteraction_Electric()
	{
		wario_side_of_obj();
		set_obj_action(OBJACTION_06);
		if (mem_.invincible_counter() || persistent_or(TRANSFORMATION_ELECTRIC))
			return;
		mem_.direction() = (mem_.interaction_side() & (1 << INTERACTION_RIGHT_F)) ? DIRECTION_LEFT : DIRECTION_RIGHT;
		play_sfx(SFX_040);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, wInvisibleFrame, 0};
		clear(zero);
		mem_.wario_state() = WST_ELECTRIC_START;
		collision_box(-1, -27, -9, 9);
		mem_.transformation() = TRANSFORMATION_ELECTRIC;
		touch_states(TOUCH_VANISH, 0x01);
		home::update_level_music(gb_);
		reset_anim();
		home::set_wario_pal(gb_, ADDR_WarioElectricPal);
		load_gfx(BANK_WarioElectricGfx, ADDR_WarioElectricGfx);
		set_oam(BANK_OAM_1790e, ADDR_OAM_1790e);
		frameset_by_dir(ADDR_Frameset_17b79, ADDR_Frameset_17b76);
		anim(OAM1);
	}

	// Adds n (BCD) to wNumCoins, capped at MAX_NUM_COINS
	void add_coins(uint8_t n)
	{
		uint8_t lo = alu_.daa(alu_.add(rd8(wNumCoins + 1), n));
		wr8(wNumCoins + 1, lo);
		uint8_t hi = alu_.daa(alu_.adc(mem_.num_coins(), 0x00));
		mem_.num_coins() = hi;
		if (!(hi & 0xf0))
			return;
		wr8(wNumCoins + 0, static_cast<uint8_t>(MAX_NUM_COINS >> 8));
		wr8(wNumCoins + 1, static_cast<uint8_t>(MAX_NUM_COINS));
	}

	// the side Wario's on (by x on screen) joins the sides he touched the
	// object from
	void wario_side_of_obj()
	{
		bool left = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos();
		mem_.interaction_side() = static_cast<uint8_t>(mem_.interaction_side() | (left ? INTERACTION_LEFT : INTERACTION_RIGHT));
	}

	// a persistent transformation, or this one already
	bool persistent_or(uint8_t transformation)
	{
		uint8_t t = mem_.transformation();
		return (t & (1 << TRANSFORMATIONF_PERSISTENT_F)) || t == transformation;
	}

	void port_ObjInteraction_RegularCoin()
	{
		add_coins(0x1);
		play_sfx(SFX_REGULAR_COIN);
		port_Func_20a63();
	}


	void port_CollectMusicalCoin()
	{
		add_coins(0x10);
		port_Func_20a63();
		next_obj_ = true;
	}

	void port_ObjInteraction_ColourCoin()
	{
		play_sfx(SFX_COLOUR_COIN);
		port_CollectMusicalCoin(); // fallthrough
	}

	void port_ObjInteraction_MusicalCoin()
	{
		uint8_t coins = mem_.num_musical_coins();
		if (coins >= NUM_LEVEL_MUSICAL_COINS) {
			next_obj_ = true;
			return;
		}
		mem_.num_musical_coins() = static_cast<uint8_t>(coins + 1);
		play_sfx(SFX_MUSICAL_COIN);
		port_CollectMusicalCoin();
	}

	void port_ObjInteraction_InvisibilityPotion()
	{
		port_Func_20a63();
		if (mem_.invincible_counter() || persistent_or(TRANSFORMATION_INVISIBLE_WARIO))
			return;
		FARCALL(SetState_TurningInvisible);
	}

	void port_ObjInteraction_RollingRock()
	{
		set_obj_action(OBJACTION_06);
		if (mem_.is_rolling())
			return;
		mem_.direction() = mem_.obj_direction();
		FARCALL(Func_1e174);
	}

	// ld a, [wInvincibleCounter] / and a / ret nz /
	// ld a, [wTransformation] / bit TRANSFORMATIONF_PERSISTENT_F, a / ret nz:
	// true if it returns
	bool invincible_or_persistent()
	{
		return mem_.invincible_counter() || (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F));
	}

	void port_ObjInteraction_Vampire()
	{
		if (invincible_or_persistent())
			return;
		set_obj_action(OBJACTION_06);
		mem_.transformation() = TRANSFORMATION_VAMPIRE_WARIO;
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (mem_.transformation() != TRANSFORMATION_VAMPIRE_WARIO)
			return;
		FARCALL(SetState_VampireTransforming);
	}

	void port_ObjInteraction_Bubble()
	{
		if (invincible_or_persistent())
			return;
		set_obj_action(OBJACTION_07);
		mem_.transformation() = TRANSFORMATION_BUBBLE;
		touch_states(TOUCH_BUMP, 0x01);
		FARCALL(Func_2ac7d);
	}

	void port_ObjInteraction_ZombieHead()
	{
		uint8_t transformation = mem_.transformation();
		if (!mem_.invincible_counter() && transformation == TRANSFORMATION_ZOMBIE_WARIO) {
			port_VanishObjectAndKnockBackIfZombie();
			return;
		}
		if (mem_.invincible_counter() || (transformation & (1 << TRANSFORMATIONF_PERSISTENT_F))) {
			port_Func_20a69();
			return;
		}
		set_obj_action(OBJACTION_06);
		play_sfx(SFX_028);
		mem_.transformation() = TRANSFORMATION_ZOMBIE_WARIO;
		touch_states(TOUCH_VANISH, 0x02);
		FARCALL(SetState_ZombieIdling);
	}

	void port_Func_2028a()
	{
		if (!mem_.jump_vel_table())
			return;
		mem_.direction() = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos() ? DIRECTION_RIGHT : DIRECTION_LEFT;
		FARCALL(SetState_FlatFalling);
	}

	// ld a, [wStingTouchState] / and a / jr z / cp TOUCH_BUMP / jr z /
	// cp TOUCH_VANISH / jp z, Func_2092d / ret: 0 = go on, 1 = return,
	// 2 = jumped to Func_2092d
	int sting_touch()
	{
		uint8_t touch = mem_.sting_touch_state();
		if (!touch || touch == TOUCH_BUMP)
			return 0;
		if (touch == TOUCH_VANISH) {
			port_Func_2092d();
			return 2;
		}
		return 1;
	}

	// Func_208f2 is jr .start, so the routine and .start run the same code.
	void func_208f2_start()
	{
		if (sting_touch())
			return;
		if (!mem_.water_interaction())
			return;
		set_obj_action(OBJACTION_BUMP); // .Bump
		if (mem_.invincible_counter() == 0x01 || mem_.transformation())
			return;
		FARCALL(StartDive);
	}

	void port_Func_208f2() { func_208f2_start(); }
	void port_Func_208f2__start() { func_208f2_start(); }

	void port_Func_20899()
	{
		if (sting_touch())
			return;
		if (!mem_.water_interaction())
			return;
		wario_side_of_obj();
		play_sfx(SFX_014);
		set_obj_action(OBJACTION_WOBBLE);
		if (mem_.transformation() || (mem_.interaction_side() & (1 << INTERACTION_UP_F)))
			return;
		FARCALL(StartDive);
	}

	void port_SetState_WaterStung()
	{
		play_sfx(SFX_STING);
		mem_.invincible_counter() = 0x01;
		mem_.wario_state() = WST_WATER_STUNG;
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
		if (persistent_or(TRANSFORMATION_RAIL))
			return;
		set_obj_action(OBJACTION_06);
		play_sfx(SFX_HANG_GRAB);
		mem_.transformation() = TRANSFORMATION_RAIL;
		mem_.touch_state() = TOUCH_BUMP;
		mem_.sting_touch_state() = TOUCH_VULNERABLE;
		mem_.unk_ca94() = 0x01;
		mem_.wario_state() = WST_HANGING_RAIL;
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		mem_.auto_move_state() = 0x01;
		home::update_level_music(gb_);
		reset_anim();
		load_gfx(BANK_WarioHangGfx, ADDR_WarioHangGfx);
		set_oam(BANK_OAM_1fee05, ADDR_OAM_1fee05);
		mem_.direction() = mem_.obj_direction();
		set_frameset(ADDR_Frameset_1feec4);
		anim(OAM2);
	}

	void port_GetTreasure()
	{
		// stop_music
		wr8(hMusicID + 0, 0xff);
		mem_.music().lo() = 0x00;
		port_Func_20a63();
		port_TreasureClearObjects();
		if (mem_.level_end_screen() == LVLEND_NO_TREASURE)
			return;
		mem_.level_end_screen() = mem_.level_end_screen() | 0x80;
		mem_.reset_disabled() = TRUE;
		mem_.walk_vel_index() = 0;
		mem_.wario_state() = WST_GET_TREASURE_START;
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wJumpVelIndex,
			wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking, wIsCrouching,
			wAttackCounter, 0};
		clear(zero);
		mem_.is_intangible() = 0x01;
		mem_.auto_move_state() = 0x01;
		if (mem_.jump_vel_table())
			mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		home::set_wario_pal(gb_, ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		// out of the treasure, facing it
		if (mem_.interaction_side() & (1 << INTERACTION_LEFT_F)) {
			home::move_wario_x(gb_, mem_.unk_c1c4());
			mem_.direction() = DIRECTION_LEFT;
			set_frameset(ADDR_Frameset_14252);
		} else {
			home::move_wario_x(gb_, -mem_.unk_c1c3());
			mem_.direction() = DIRECTION_RIGHT;
			set_frameset(ADDR_Frameset_1425f);
		}
		anim(OAM1);
	}

	void restore_pos_backup()
	{
		mem_.y_pos_hi() = mem_.y_pos_hi_backup();
		mem_.y_pos_lo() = mem_.y_pos_lo_backup();
		mem_.x_pos_hi() = mem_.x_pos_hi_backup();
		mem_.x_pos_lo() = mem_.x_pos_lo_backup();
	}

	void port_SetState_Stung()
	{
		mem_.invincible_counter() = 0x01;
		play_sfx(SFX_STING);
		mem_.wario_state() = WST_STUNG;
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsRolling, wIsSmashAttacking, 0};
		clear(zero);
		mem_.jump_vel_index() = 0x6;
		mem_.collision_box_bottom() = (uint8_t)-1;
		mem_.collision_box_left() = (uint8_t)-9;
		mem_.collision_box_right() = 9;
		bool low_ceiling = false;
		if (mem_.is_crouching()) {
			mem_.collision_box_top() = (uint8_t)-27;
			mem_.y_pos_hi_backup() = mem_.y_pos_hi();
			mem_.y_pos_lo_backup() = mem_.y_pos_lo();
			mem_.x_pos_hi_backup() = mem_.x_pos_hi();
			mem_.x_pos_lo_backup() = mem_.x_pos_lo();
			if (up_collision()) {
				low_ceiling = true;
			} else {
				xor_a();
				mem_.is_crouching() = 0;
				restore_pos_backup();
			}
		}
		if (low_ceiling) {
			mem_.collision_box_top() = (uint8_t)-15;
			xor_a();
			mem_.jump_vel_table() = 0;
			restore_pos_backup();
		} else {
			mem_.collision_box_top() = (uint8_t)-27;
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
			home::release_owl(gb_);
		home::clear_transformation_values(gb_);
		home::update_level_music(gb_);
		port_SetState_Stung(); // fallthrough
	}

	void port_Func_207ed()
	{
		if (mem_.invincible_counter() || !mem_.water_interaction())
			return;
		uint8_t touch = mem_.sting_touch_state();
		if (touch == TOUCH_BUMP) {
			port_Func_208f2__start();
			return;
		}
		if (touch == TOUCH_VANISH) {
			port_Func_2092d();
			return;
		}
		if (touch)
			return;
		set_obj_action(OBJACTION_06);
		port_SetState_WaterStung(); // fallthrough
	}

	// stop_sfx
	void stop_sfx()
	{
		wr8(hSFXID + 0, 0xff);
		mem_.sfx().lo() = 0x00;
	}

	// the shared body of ObjInteraction_WaterTeleporting/Jamano
	template <class F>
	void teleport(F stung, uint8_t state)
	{
		mem_.room_transition_param() = ROOMTRANSITION_TELEPORT | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY;
		stung();
		stop_sfx();
		mem_.wario_state() = state;
		set_obj_action(OBJACTION_TELEPORT);
	}

	void port_ObjInteraction_WaterTeleporting() { teleport([this] { port_SetState_WaterStung(); }, WST_WATER_TELEPORTING); }
	void port_ObjInteraction_Jamano() { teleport([this] { port_Func_206eb(); }, WST_TELEPORTING); }

	void port_Func_21e8a()
	{
		FARCALL(SetState_BallShot);
		port_Func_21cc9();
	}

	void port_Func_20d81()
	{
		if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) {
			port_Func_20899();
			return;
		}
		port_Func_208f2();
	}

	void port_Func_2168b()
	{
		if (mem_.wario_state() == WST_FLAT_SQUISHED) {
			FARCALL(SetState_FlatSquishedLifting);
			return;
		}
		uint8_t dy = mem_.unk_c1c2();
		home::move_wario_y(gb_, dy >= 0x10 ? 0x0c : dy);
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (mem_.water_interaction()) {
			FARCALL(DiveFromSurface_SkipSplash);
			return;
		}
		bool ground = collision(BANK_CheckCentreCollision, ADDR_CheckCentreCollision, check_centre_collision);
		if (ground)
			update_pos_y();
		if (mem_.invincible_counter() == 0x01)
			return;
		uint8_t transformation = mem_.transformation();
		if (transformation == TRANSFORMATION_FLAT_WARIO) {
			if (!ground)
				FARCALL(SetState_FlatFalling);
			else if (mem_.wario_state() != WST_FLAT_SQUISHED)
				FARCALL(SetState_FlatIdling);
			return;
		}
		if (transformation)
			return;
		if (ground) {
			mem_.grab_state() = 0;
			FARCALL(SetState_Idling);
		} else if (mem_.is_crouching()) {
			FARCALL(StartCrouchFall);
		} else {
			mem_.grab_state() = 0;
			FARCALL(StartFall);
		}
	}

	void port_ObjInteraction_Snow()
	{
		if (invincible_or_persistent()) {
			port_Func_20a69();
			return;
		}
		port_Func_20a63();
		mem_.transformation() = TRANSFORMATION_SNOWMAN_WARIO;
		mem_.transformation_progress() = 0;
		touch_states(TOUCH_ATTACK, 0x02);
		FARCALL(SetState_TurningIntoSnowman);
	}

	void port_ObjInteraction_Ice()
	{
		if (invincible_or_persistent())
			return;
		if (mem_.is_crouching()) {
			mem_.collision_box_top() = static_cast<uint8_t>(-27);
			if (up_collision()) {
				mem_.collision_box_top() = static_cast<uint8_t>(-15);
				return;
			}
		}
		set_obj_action(OBJACTION_07);
		play_sfx(SFX_028);
		mem_.transformation() = TRANSFORMATION_ICE_SKATIN;
		touch_states(TOUCH_ATTACK, 0x02);
		FARCALL(SetState_IceSkatinStart);
	}

	void port_Func_209ca()
	{
		if (mem_.wario_state() == WST_ATTACKING_AIRBORNE)
			mem_.attack_counter() = 0;
		if (mem_.water_interaction() || mem_.wario_state() == WST_DIVING)
			return;
		mem_.is_on_steppable_object() = TRUE;
		// on top of it
		if (uint8_t dy = mem_.unk_c1c1())
			home::move_wario_y(gb_, -(dy >= 0x03 ? 0x02 : dy));
		set_obj_action(OBJACTION_STEP_ON);
		if (!mem_.jump_vel_table() || mem_.attack_counter() || mem_.is_rolling()) {
			next_obj_ = true;
			return;
		}
		home::trigger_floor_transition(gb_, false);
		if (mem_.invincible_counter() == 0x01) {
			next_obj_ = true;
			return;
		}
		if (mem_.transformation())
			return;
		if (mem_.grab_state()) {
			FARCALL(SetState_GrabIdling);
		} else {
			mem_.water_interaction() = 0;
			if (mem_.is_crouching())
				FARCALL(CrouchOrSlide);
			else
				FARCALL(SetState_Idling);
		}
		next_obj_ = true;
	}

	void port_Func_20a6f()
	{
		// pushed out of it sideways, unless a wall's there
		bool left = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos();
		for (;;) {
			if (!left) { // .asm_20a79
				if (!mem_.direction()) {
					next_obj_ = true;
					return;
				}
				mem_.interaction_side() = INTERACTION_RIGHT;
				if (collision(BANK_Func_197b1, ADDR_Func_197b1, check_func_197b1))
					break;
				uint8_t dx = mem_.unk_c1c3();
				if (dx == 0xff) {
					left = true;
					continue;
				}
				home::move_wario_x(gb_, -dx);
				break;
			} else { // .asm_20aa5
				if (mem_.direction()) {
					next_obj_ = true;
					return;
				}
				mem_.interaction_side() = INTERACTION_LEFT;
				if (collision(BANK_Func_19741, ADDR_Func_19741, check_func_19741))
					break;
				uint8_t dx = mem_.unk_c1c4();
				if (dx == 0xff) {
					left = false;
					continue;
				}
				home::move_wario_x(gb_, dx);
				break;
			}
		}
		// .asm_20acf
		switch (mem_.transformation()) {
		case TRANSFORMATION_BALL_O_STRING_WARIO:
			if (mem_.transformation_progress() == 1)
				FARCALL(Func_28bd5);
			return;
		case TRANSFORMATION_HOT_WARIO:
			if (mem_.transformation_progress() == 1)
				FARCALL(Func_2afc5);
			return;
		case TRANSFORMATION_ICE_SKATIN:
			if (mem_.wario_state() != WST_ICE_SKATIN_CRASH)
				FARCALL(Func_1ec215);
			return;
		}
		mem_.walk_vel_index() = 0;
		if (!mem_.is_rolling() && !mem_.attack_counter()) {
			next_obj_ = true;
			return;
		}
		FARCALL(DoJumpingBump);
	}

	// routine called when an object is attacked
	// whether by Wario's B attack, Fat Wario bump, rolling, etc.
	void port_AttackObject()
	{
		play_sfx(SFX_017);
		bool left = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos();
		mem_.interaction_side() = left ? INTERACTION_LEFT : INTERACTION_RIGHT;
		// stars 8 pixels up and towards the object
		uint16_t x = static_cast<uint16_t>(rd8(wWarioXPos + 0) << 8 | mem_.wario_x().lo());
		x = static_cast<uint16_t>(left ? x - 8 : x + 8);
		mem_.x_pos_hi() = static_cast<uint8_t>(x >> 8);
		mem_.x_pos_lo() = static_cast<uint8_t>(x);
		uint16_t y = static_cast<uint16_t>((rd8(wWarioYPos + 0) << 8 | mem_.wario_y().lo()) - 8);
		mem_.y_pos_hi() = static_cast<uint8_t>(y >> 8);
		mem_.y_pos_lo() = static_cast<uint8_t>(y);

		// .spawn_stars
		Routines(gb_).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, PARTICLE_STARS); });
		set_obj_action(OBJACTION_ATTACK);
		uint8_t transformation = mem_.transformation();
		if (transformation == TRANSFORMATION_FAT_WARIO) {
			FARCALL(SetState_FatBumping);
			return;
		}
		if (transformation == TRANSFORMATION_SNOWMAN_WARIO) {
			if (!mem_.transformation_progress())
				FARCALL(SetState_SnowmanBumped);
			return;
		}
		if (transformation)
			return;
		bool heavy = mem_.obj_interaction_type() & (1 << HEAVY_OBJ_F);
		if (mem_.is_rolling()) {
			// a heavy object stops the roll, without garlic
			if (heavy && mem_.power_up_level() < GARLIC)
				FARCALL(DoJumpingBump_NoSFX);
			return;
		}
		// .not_rolling
		if (heavy)
			FARCALL(DoJumpingBump_NoSFX);
		else if (mem_.jump_vel_table())
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
		if (!mem_.water_interaction())
			return;
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F)) {
			port_Func_207ed();
			return;
		}
		if (mem_.transformation())
			return;
		if (side & (1 << INTERACTION_DOWN_F)) {
			FARCALL(StartDive);
			return;
		}
		bool left = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos();
		mem_.interaction_side() = static_cast<uint8_t>(side | (left ? INTERACTION_LEFT : INTERACTION_RIGHT));
		mem_.direction() = left ? DIRECTION_RIGHT : DIRECTION_LEFT;
		FARCALL(SetState_SwimKnockBack);
	}

	// ld a, [wInteractionSide] / bit side, a / jp nz, Func_207ed / jp Func_208f2
	void sting_or_bump(int side)
	{
		if (mem_.interaction_side() & (1 << side))
			port_Func_207ed();
		else
			port_Func_208f2();
	}

	void port_Func_20deb()
	{
		if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) {
			port_Func_20899();
			return;
		}
		if (!mem_.obj_direction())
			sting_or_bump(INTERACTION_RIGHT_F);
		else
			sting_or_bump(INTERACTION_LEFT_F);
	}

	void port_ObjInteraction_Haridama()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_DOWN_F))
			port_Func_20899();
		else if (side & (1 << INTERACTION_UP_F))
			port_Func_207ed();
		else
			sting_or_bump(mem_.obj_direction() == DIRECTION_RIGHT ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F);
	}

	void port_ObjInteraction_Pesce()
	{
		// swallowed from the side it's facing
		int mouth = mem_.obj_direction() == DIRECTION_RIGHT ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
		if (!(mem_.interaction_side() & (1 << mouth))) {
			port_Func_207ed();
			return;
		}
		set_obj_action(OBJACTION_11);
		FARCALL(SetState_Swallowed);
	}

	void port_ObjInteraction_Scowler()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			port_Func_20899();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_ObjInteraction_WaterTeleporting();
		else
			port_Func_208f2();
	}

	// the chest interactions: open it with the matching key
	void obj_treasure(int key, int treasure, uint8_t screen)
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F)) {
			port_Func_209ca();
			return;
		}
		uint8_t flags = mem_.key_and_treasure_flags();
		if (mem_.transformation() || !(flags & (1 << key))) {
			port_Func_20a6f();
			return;
		}
		mem_.key_and_treasure_flags() = static_cast<uint8_t>(flags | (1 << treasure));
		mem_.level_end_screen() = screen;
		port_GetTreasure();
	}

	void port_ObjInteraction_GreyTreasure() { obj_treasure(GREY_KEY_F, GREY_TREASURE_F, LVLEND_GREY_TREASURE); }
	void port_ObjInteraction_RedTreasure() { obj_treasure(RED_KEY_F, RED_TREASURE_F, LVLEND_RED_TREASURE); }
	void port_ObjInteraction_GreenTreasure() { obj_treasure(GREEN_KEY_F, GREEN_TREASURE_F, LVLEND_GREEN_TREASURE); }
	void port_ObjInteraction_BlueTreasure() { obj_treasure(BLUE_KEY_F, BLUE_TREASURE_F, LVLEND_BLUE_TREASURE); }

	// stepped on from above: Func_209ca; attacked: Func_217a9; else Func_20a6f.
	// Returns true if it jumped (false: touched from below).
	bool solid_side()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F)) {
			port_Func_209ca();
			return true;
		}
		if (side & (1 << INTERACTION_DOWN_F))
			return false;
		if (mem_.attack_counter())
			port_Func_217a9();
		else
			port_Func_20a6f();
		return true;
	}

	void port_ObjInteraction_Solid()
	{
		if (solid_side())
			return;
		port_Func_2168b();
	}

	void port_ObjInteraction_Omodon()
	{
		if (solid_side() || invincible_or_persistent())
			return;
		set_obj_action(OBJACTION_FLATTEN);
		FARCALL(SetState_FlatAirborne);
	}

	void port_Func_21abb()
	{
		set_obj_action(OBJACTION_ATTACK);
		port_Func_20a6f();
	}

	void port_Func_21f01()
	{
		if (!(mem_.interaction_side() & (1 << INTERACTION_UP_F))) {
			port_Func_20d8c();
			return;
		}
		mem_.interaction_side() = static_cast<uint8_t>(mem_.interaction_side() | INTERACTION_LEFT);
		mem_.direction() = DIRECTION_RIGHT;
		FARCALL(SetState_SwimKnockBack);
	}

	void port_Func_20602()
	{
		uint8_t transformation = mem_.transformation();
		if (transformation && transformation != TRANSFORMATION_INVISIBLE_WARIO) {
			// has transformation except invisible
			port_VanishObjectAndKnockBackIfZombie();
			return;
		}
		if ((mem_.grab_state() & GRAB_STATE_MASK) == GRAB_IDLE) {
			FARCALL(Func_1ede9);
		} else {
			mem_.grab_state() = 0;
			FARCALL(StartJump_FromInput);
		}
		mem_.do_full_jump() = TRUE;
		mem_.jump_vel_index() = 0x0a;
		if ((mem_.joypad_down() & (1 << B_PAD_A)) && mem_.power_up_level() >= HIGH_JUMP_BOOTS)
			mem_.jump_vel_index() = 0;
		set_obj_action(OBJACTION_VANISH_TOUCH); // fallthrough
	}

	void port_Func_21aac()
	{
		if (mem_.is_smash_attacking() != 1) {
			port_Func_209ca();
			return;
		}
		// smash attack object
		port_Func_20602();
		set_obj_action(OBJACTION_SMASH_ATTACK_WALKABLE);
	}

	void port_ObjInteraction_Stove()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			port_Func_21aac();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_Func_2168b();
		else if (mem_.attack_counter())
			port_Func_21abb();
		else
			port_Func_20a6f();
	}

	void port_ObjInteraction_WaterDrop()
	{
		set_obj_action(OBJACTION_06);
		if (!mem_.transformation()) {
			next_obj_ = true;
			return;
		}
		home::recover_from_transformation(gb_);
	}

	// ld a, [wWarioScreenXPos] / ld b, a / ld a, [wObjScreenXPos] / cp b, then
	// sets INTERACTION_LEFT (object left of Wario) or INTERACTION_RIGHT
	void side_by_x() { wario_side_of_obj(); }

	// Func_2023b.asm_20257
	void func_2023b_bump()
	{
		if (mem_.attack_counter()) {
			port_AttackObject();
			return;
		}
		set_obj_action(OBJACTION_BUMP);
		if (mem_.invincible_counter() == 0x01) // is invincible
			return;
		uint8_t transformation = mem_.transformation();
		if (transformation == TRANSFORMATION_FLAT_WARIO) {
			port_Func_2028a();
			return;
		}
		if (transformation)
			return;
		mem_.direction() = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos() ? DIRECTION_RIGHT : DIRECTION_LEFT;
		port_SetState_EnemyBumping();
	}

	void port_Func_2023b()
	{
		switch (mem_.touch_state()) {
		case 0:
		case TOUCH_BUMP: func_2023b_bump(); return;
		case TOUCH_VULNERABLE: port_Func_20939(); return; // this causes an infinite loop!
		case TOUCH_ATTACK: port_AttackObject(); return;
		case TOUCH_VANISH: port_VanishObjectAndKnockBackIfZombie(); return;
		}
		next_obj_ = true;
	}

	void port_Func_2022c()
	{
		if (mem_.is_rolling())
			port_AttackObject();
		else if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN))
			port_StepOnObject();
		else
			port_Func_2023b(); // fallthrough
	}

	void port_Func_20939()
	{
		if (mem_.touch_state()) {
			port_Func_2023b();
			return;
		}
		set_obj_action(OBJACTION_06);
		if (mem_.invincible_counter() == 0x01)
			return;
		FARCALL(DoJumpingBump);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		mem_.grab_state() = 0;
		// bumped back, facing the object
		if (mem_.obj_screen_x_pos() >= mem_.wario_screen_x_pos()) {
			mem_.interaction_side() = INTERACTION_RIGHT;
			mem_.direction() = DIRECTION_LEFT;
			set_frameset(ADDR_Frameset_14a79);
		} else {
			mem_.interaction_side() = INTERACTION_LEFT;
			mem_.direction() = DIRECTION_RIGHT;
			set_frameset(ADDR_Frameset_14a76);
		}
		anim(OAM1);
	}

	void port_Func_20932()
	{
		if (mem_.attack_counter())
			port_AttackObject();
		else
			port_Func_20939(); // fallthrough
	}

	void port_StepOnObject()
	{
		// no stun if Wario is invincible
		if (mem_.invincible_counter() == 0x01)
			return;
		uint8_t transformation = mem_.transformation();
		if (mem_.is_smash_attacking() == 1) {
			// Wario is smash attacking
			if (transformation == TRANSFORMATION_FAT_WARIO)
				port_VanishObjectAndKnockBackIfZombie();
			else
				port_Func_20602();
			return;
		}
		if (transformation) {
			// .Transformed
			if (transformation == TRANSFORMATION_HOT_WARIO && mem_.transformation_progress() >= 2) {
				port_VanishObjectAndKnockBackIfZombie();
				return;
			}
			if (transformation == TRANSFORMATION_FAT_WARIO) {
				port_AttackObject();
				return;
			}
			side_by_x();
			play_sfx(SFX_014);
			set_obj_action(OBJACTION_WOBBLE);
			if (transformation == TRANSFORMATION_FLAT_WARIO)
				port_Func_2028a();
			else if (transformation == TRANSFORMATION_BOUNCY_WARIO && mem_.touch_state() != TOUCH_PASS_THROUGH)
				FARCALL(Func_2a0b2);
			return;
		}

		// not transformed
		mem_.attack_counter() = 0;
		// stepping on object while rolling is attacking
		if (mem_.is_rolling()) {
			port_AttackObject();
			return;
		}
		side_by_x();
		bool below = (mem_.interaction_side() & (1 << INTERACTION_DOWN_F)) || mem_.obj_screen_y_pos() < mem_.wario_screen_y_pos();
		if (!below) {
			if ((mem_.grab_state() & GRAB_STATE_MASK) == GRAB_IDLE) {
				FARCALL(Func_1ede9); // .grabbing
			} else {
				mem_.grab_state() = 0;
				if (mem_.is_crouching()) {
					FARCALL(StartCrouchJump);
				} else if (mem_.joypad_down()) {
					FARCALL(StartJump_FromInput);
				} else {
					// .no_input
					port_Func_20939();
					play_sfx(SFX_014);
					set_obj_action(OBJACTION_WOBBLE);
					return;
				}
			}
			mem_.do_full_jump() = TRUE;
			mem_.jump_vel_index() = 0x0a;
			if ((mem_.joypad_down() & (1 << B_PAD_A)) && mem_.power_up_level() >= HIGH_JUMP_BOOTS)
				mem_.jump_vel_index() = 0; // full jump
		} else {
			// .asm_20506
			if (mem_.invincible_counter() == 0x01)
				return;
			bool falling = mem_.jump_vel_table();
			if (mem_.is_crouching()) {
				if (falling)
					FARCALL(StartCrouchFall);
			} else if ((mem_.grab_state() & GRAB_STATE_MASK) == GRAB_IDLE) {
				if (falling)
					FARCALL(Func_1edd3);
			} else {
				mem_.grab_state() = 0;
				if (falling)
					FARCALL(StartFall);
				else
					FARCALL(SetState_Idling);
			}
		}
		play_sfx(SFX_014);
		set_obj_action(OBJACTION_WOBBLE);
	}

	void port_Func_20670()
	{
		if (mem_.invincible_counter()) {
			port_Func_20939();
			return;
		}
		// if Wario is rolling, do attack (a heavy object only with garlic)
		bool attack = mem_.power_up_level() >= GARLIC || !(mem_.obj_interaction_type() & (1 << HEAVY_OBJ_F));
		if (attack && mem_.is_rolling()) {
			port_AttackObject();
			return;
		}
		// .asm_2068c
		uint8_t touch = mem_.sting_touch_state();
		if (touch && touch != TOUCH_VULNERABLE) {
			if (touch == TOUCH_BUMP)
				port_Func_2023b();
			else if (touch == TOUCH_ATTACK)
				port_AttackObject();
			else if (touch == TOUCH_VANISH)
				port_VanishObjectAndKnockBackIfZombie();
			else
				next_obj_ = true;
			return;
		}
		// .asm_206a8
		side_by_x();
		mem_.direction() = (mem_.interaction_side() & (1 << INTERACTION_RIGHT_F)) ? DIRECTION_LEFT : DIRECTION_RIGHT;
		set_obj_action(OBJACTION_06);
		if (mem_.invincible_counter() == 0x01)
			return;
		if (mem_.sting_touch_state() == TOUCH_VULNERABLE)
			port_Func_206eb();
		else
			port_SetState_Stung();
	}

	// Func_20b6b.set_wario_pick_up
	void set_wario_pick_up()
	{
		play_sfx(SFX_GRAB);
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		mem_.wario_state_counter() = 0;
		mem_.wario_state_cycles() = 0;
		mem_.wario_state() = WST_PICKING_UP;
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioThrowGfx, ADDR_WarioThrowGfx);
		set_oam(BANK_OAM_1606a, ADDR_OAM_1606a);
		bool right = mem_.direction();
		// on a slope going up ahead, straight to grab idling
		if (mem_.wario_slope_interaction() & (1 << (right ? RIGHT_SLOPE_F : LEFT_SLOPE_F))) {
			FARCALL(SetState_GrabIdling);
			return;
		}
		bool heavy = mem_.grab_state() & (1 << GRAB_HEAVY_F);
		if (right)
			set_frameset(heavy ? ADDR_Frameset_16415 : ADDR_Frameset_1640c);
		else
			set_frameset(heavy ? ADDR_Frameset_163f5 : ADDR_Frameset_163ec);
		anim(OAM1);
	}

	void port_Func_20b6b()
	{
		if (!try_pick_up())
			port_StepOnObject(); // step_on_obj
	}

	// Func_20b6b up to .step_on_obj: picks the object up if it can (false:
	// stepped on instead)
	bool try_pick_up()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			return false;
		switch (mem_.touch_state()) {
		case 0: break;
		case TOUCH_ATTACK: port_AttackObject(); return true;
		case TOUCH_VANISH: port_VanishObjectAndKnockBackIfZombie(); return true;
		case TOUCH_PASS_THROUGH: next_obj_ = true; return true;
		default: return false;
		}
		// .check_water
		uint8_t state = mem_.wario_state();
		if (state >= WST_DIVING && state <= WST_TRYING_SUBMERGE) {
			port_Func_20899();
			return true;
		}
		// .not_in_water
		if (mem_.attack_counter() | mem_.is_rolling()) {
			port_AttackObject();
			return true;
		}
		if ((mem_.grab_state() | mem_.is_crouching()) || mem_.power_up_level() < GRAB_GLOVE)
			return false;
		if (state == WST_LADDER_SCRATCHING || (state >= WST_LADDER_CLIMBING && state <= WST_LADDER_SLIDING))
			return false;
		// .not_on_ladder: only facing it
		if (mem_.obj_screen_x_pos() >= mem_.wario_screen_x_pos()) {
			mem_.interaction_side() = INTERACTION_RIGHT;
			if (!mem_.direction())
				return false;
		} else {
			mem_.interaction_side() = INTERACTION_LEFT;
			if (mem_.direction())
				return false;
		}
		// .check_invincible_counter
		if (mem_.invincible_counter() == 1)
			return true;
		// hPos = Wario's position, 8 pixels up
		mem_.x_pos_lo() = mem_.wario_x().lo();
		mem_.x_pos_hi() = rd8(wWarioXPos + 0);
		uint16_t y = static_cast<uint16_t>((rd8(wWarioYPos + 0) << 8 | mem_.wario_y().lo()) - 8);
		mem_.y_pos_lo() = static_cast<uint8_t>(y);
		mem_.y_pos_hi() = static_cast<uint8_t>(y >> 8);
		bool heavy = mem_.obj_interaction_type() & (1 << HEAVY_OBJ_F);
		if (heavy && mem_.power_up_level() < SUPER_GRAB_GLOVES)
			return false;
		mem_.grab_state() = heavy ? GRAB_PICK_UP | (1 << GRAB_HEAVY_F) : GRAB_PICK_UP;
		// .set_obj_pick_up
		set_obj_action(OBJACTION_PICK_UP);
		if (!mem_.jump_vel_table()) {
			set_wario_pick_up();
			return true;
		}
		if (heavy)
			return false;
		FARCALL(Func_1edd3);
		return true;
	}

	void port_Func_20b41()
	{
		switch (mem_.touch_state()) {
		case 0:
			// .asm_20b5e
			port_Func_20939();
			set_obj_action(OBJACTION_12);
			return;
		case TOUCH_ATTACK: port_AttackObject(); return;
		case TOUCH_VANISH: port_VanishObjectAndKnockBackIfZombie(); return;
		}
		port_Func_20a6f();
	}

	void port_Func_20d1d()
	{
		port_Func_2022c();
	}

	// sting on the object's front (front = true) or back
	void obj_sting(bool front)
	{
		if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) {
			port_StepOnObject();
			return;
		}
		bool facing_right = mem_.obj_direction() == DIRECTION_RIGHT;
		int bit = facing_right == front ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
		if (mem_.interaction_side() & (1 << bit))
			port_Func_20670();
		else
			port_Func_2022c();
	}

	void port_ObjInteraction_FrontSting() { obj_sting(true); }
	void port_ObjInteraction_BackSting() { obj_sting(false); }

	void port_ObjInteraction_TopSting()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			port_Func_20670();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_StepOnObject();
		else
			port_Func_2022c();
	}

	void port_ObjInteraction_FullSting()
	{
		port_Func_20670();
	}

	void port_Func_20e39()
	{
		if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) {
			port_StepOnObject();
			return;
		}
		int bit = mem_.obj_direction() == DIRECTION_RIGHT ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F;
		if (mem_.interaction_side() & (1 << bit))
			port_Func_20939();
		else
			port_Func_2022c();
	}

	void port_Func_20e60()
	{
		port_Func_20939();
	}

	void port_Func_20e6a()
	{
		port_Func_20939();
		if (!mem_.touch_state())
			set_obj_action(OBJACTION_0A);
	}

	void port_Func_20e63()
	{
		if (mem_.attack_counter())
			port_AttackObject();
		else
			port_Func_20e6a(); // fallthrough
	}

	void port_ObjInteraction_Walkable()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			port_Func_21aac();
		else
			port_Func_20932();
	}

	void port_ObjInteraction_Fire()
	{
		set_obj_action(OBJACTION_06);
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		mem_.transformation() = TRANSFORMATION_HOT_WARIO;
		mem_.transformation_progress() = 1;
		touch_states(TOUCH_BUMP, 0x02);
		wr8(wTransformationDuration + 0, (uint8_t)(HOT_WARIO_DURATION >> 8));
		wr8(wTransformationDuration + 1, (uint8_t)HOT_WARIO_DURATION);
		home::update_level_music(gb_);
		port_SetState_OnFire_ResetStateCounter(); // fallthrough
	}

	void port_Func_2126a()
	{
		if (mem_.is_crouching()) {
			mem_.collision_box_top() = static_cast<uint8_t>(-27);
			if (up_collision()) {
				mem_.collision_box_top() = static_cast<uint8_t>(-15);
				port_Func_2022c();
				return;
			}
		}
		// .asm_21290
		set_obj_action(OBJACTION_06);
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2023b();
			return;
		}
		mem_.direction() = mem_.obj_screen_x_pos() < mem_.wario_screen_x_pos() ? DIRECTION_RIGHT : DIRECTION_LEFT;
		// .set_ball_o_string
		play_sfx(SFX_039);
		mem_.transformation() = TRANSFORMATION_BALL_O_STRING_WARIO;
		mem_.transformation_progress() = 0;
		touch_states(TOUCH_ATTACK, 0x01);
		home::update_level_music(gb_);
		mem_.wario_state() = WST_GETTING_WRAPPED_IN_STRING;
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wJumpVelIndex, wJumpVelTable, wIsCrouching, wIsRolling,
			wIsSmashAttacking, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		home::set_wario_pal(gb_, ADDR_WarioStringPal);
		load_gfx(BANK_WarioStringGfx, ADDR_WarioStringGfx);
		set_oam(BANK_OAM_171c0, ADDR_OAM_171c0);
		set_frameset(ADDR_Frameset_17414);
		anim(OAM1);
	}

	void port_ObjInteraction_Silky()
	{
		if (vert_step())
			return;
		if (obj_side(true, true))
			port_Func_2126a();
		else
			port_Func_2022c();
	}

	// Whether Wario touched the object's front (front) or back:
	// ld a, [wObjDirection] / cp DIRECTION_RIGHT / jr z (use_cp), or and a / jr nz,
	// then ld a, [wInteractionSide] / bit INTERACTION_LEFT_F or _RIGHT_F, a
	bool obj_side(bool use_cp, bool front)
	{
		uint8_t dir = mem_.obj_direction();
		bool right = use_cp ? dir == DIRECTION_RIGHT : dir != 0;
		return mem_.interaction_side() & (1 << (right == front ? INTERACTION_LEFT_F : INTERACTION_RIGHT_F));
	}

	// ld a, [wInteractionSide] / and INTERACTION_UP | INTERACTION_DOWN / jp nz, StepOnObject
	bool vert_step()
	{
		if (!(mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)))
			return false;
		port_StepOnObject();
		return true;
	}

	// n palette pairs from index black, a pair per H-Blank (as the asm's
	// wait_not_hblank / wait_hblank polls)
	void zero_pals_hblank(uint16_t index_reg, uint8_t index, uint8_t n, uint16_t data_reg)
	{
		wr8(index_reg, index);
		do {
			while (!(rd8(rSTAT) & STAT_MODE))
				idle(32);
			while (rd8(rSTAT) & STAT_MODE)
				idle(32);
			for (int i = 0; i < 2 * PAL_SIZE; i++)
				wr8(data_reg, 0);
		} while (--n);
	}

	void port_ObjInteraction_Teruteru()
	{
		if (!(mem_.interaction_side() & (1 << INTERACTION_DOWN_F))) {
			port_Func_2022c();
			return;
		}
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		set_obj_action(OBJACTION_06);
		mem_.transformation() = TRANSFORMATION_BLIND;
		mem_.touch_state() = TOUCH_BUMP;
		mem_.sting_touch_state() = TOUCH_VULNERABLE;
		mem_.unk_ca94() = 0x01;
		FARCALL(SetState_BlindIdling);
		zero_pals_hblank(rBGPI, BGPI_AUTOINC | (0 * PAL_SIZE), 4, rBGPD);
		zero_pals_hblank(rOBPI, OBPI_AUTOINC | (3 * PAL_SIZE), 2, rOBPD);
	}

	void port_ObjInteraction_Beam()
	{
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		set_obj_action(OBJACTION_06);
		mem_.transformation() = TRANSFORMATION_SPLIT;
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		FARCALL(SetState_SplitHit);
	}

	void port_Func_21b3a()
	{
		port_Func_20939();
		set_obj_action(OBJACTION_NONE);
	}

	void port_Func_21b78()
	{
		if ((mem_.interaction_side() & (1 << INTERACTION_UP_F)) && mem_.is_smash_attacking()) {
			port_Func_20602();
			return;
		}
		port_Func_20939();
	}

	void port_ObjInteraction_YellowBellyArrow()
	{
		if (mem_.invincible_counter())
			return;
		if (persistent_or(TRANSFORMATION_PUFFY_WARIO)) {
			port_Func_2022c();
			return;
		}
		if (mem_.transformation() == TRANSFORMATION_OWL_WARIO)
			home::release_owl(gb_);
		set_obj_action(OBJACTION_06);
		FARCALL(SetState_PuffyInflating);
	}

	void port_ObjInteraction_Pneumo()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_DOWN_F))
			port_ObjInteraction_YellowBellyArrow();
		else
			port_Func_2022c();
	}

	void port_Func_2160a()
	{
		if (mem_.invincible_counter())
			return;
		set_obj_action(OBJACTION_06);
		if (persistent_or(TRANSFORMATION_BOUNCY_WARIO)) {
			port_Func_2022c();
			return;
		}
		mem_.transformation() = TRANSFORMATION_BOUNCY_WARIO;
		touch_states(TOUCH_BUMP, 0x01);
		wr8(wTransformationDuration + 0, (uint8_t)(BOUNCY_WARIO_DURATION >> 8));
		wr8(wTransformationDuration + 1, (uint8_t)BOUNCY_WARIO_DURATION);
		FARCALL(SetState_BouncyStart);
	}

	void port_ObjInteraction_Bouncy()
	{
		if (vert_step())
			return;
		if (obj_side(true, true))
			port_Func_2160a();
		else
			port_Func_2022c();
	}

	void port_ObjInteraction_FireBot()
	{
		if (vert_step())
			return;
		if (obj_side(true, true))
			port_ObjInteraction_Fire();
		else
			port_Func_2022c();
	}

	void port_Func_21c56()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			port_Func_21aac();
		else
			port_Func_20b6b();
	}

	void port_ObjInteraction_Tadpole()
	{
		if (vert_step())
			return;
		if (obj_side(false, true))
			port_ObjInteraction_RollingRock();
		else
			port_Func_20e6a();
	}

	void port_ObjInteraction_Hebarii()
	{
		if (!(mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) && mem_.attack_counter())
			port_AttackObject();
		else
			port_Func_20e6a();
	}

	void port_ObjInteraction_PrinceFroggy()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			port_Func_209ca();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_Func_2168b();
		else
			port_Func_20b41();
	}

	void port_Func_21d3b()
	{
		if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) {
			port_Func_20e6a();
			return;
		}
		if (obj_side(false, true))
			port_Func_20670();
		else
			port_Func_20e6a();
	}

	void port_Func_21d64()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			port_Func_20670();
		else
			port_Func_20e6a();
	}

	// Func_21d6f.asm_21d7e
	void func_21d7e()
	{
		if (mem_.is_smash_attacking())
			port_Func_20602();
		else
			port_Func_20e6a();
	}

	void port_Func_21d6f()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			func_21d7e();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_Func_20670();
		else
			port_Func_20e6a();
	}

	void port_Func_21f28()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			func_21d7e();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_Func_20e6a();
		else if (obj_side(false, true))
			port_Func_20670();
		else
			port_Func_20e6a();
	}

	void port_Func_21dac()
	{
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		mem_.direction() = mem_.obj_direction();
		FARCALL(SetState_Launched);
		set_obj_action(OBJACTION_06);
	}

	void port_Func_21d88()
	{
		if (mem_.interaction_side() & (INTERACTION_UP | INTERACTION_DOWN)) {
			port_Func_20899();
			return;
		}
		if (obj_side(false, true))
			port_Func_21dac();
		else
			port_Func_208f2();
	}

	void port_Func_21ea6()
	{
		if (obj_side(false, true)) {
			// facing away from it
			mem_.direction() = mem_.obj_direction() ^ 1;
			port_Func_20a6f();
			return;
		}
		port_Func_21b3a();
	}

	void port_Func_21ecd()
	{
		if (!obj_side(false, true)) {
			port_Func_20e6a();
			return;
		}
		play_sfx(SFX_FAT_WALK);
		FARCALL(SetState_DraggedDown);
		port_Func_21cc9();
	}

	void port_ObjInteraction_MagicalSpiral()
	{
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		set_obj_action(OBJACTION_06);
		mem_.transformation() = TRANSFORMATION_MAGIC;
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		FARCALL(SetState_MagicRising);
	}

	void port_ObjInteraction_BottomSting()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_DOWN_F))
			port_Func_20670();
		else if (side & (1 << INTERACTION_UP_F))
			port_StepOnObject();
		else
			port_Func_2022c();
	}

	void port_ObjInteraction_AnonsterSilk()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			port_StepOnObject();
		else
			port_ObjInteraction_AnonsterWave(); // fallthrough
	}

	void port_ObjInteraction_AnonsterWave()
	{
		port_Func_2126a();
	}

	void port_ObjInteraction_RoboMouse()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			func_21d7e();
		else if (obj_side(false, true))
			port_ObjInteraction_Jamano();
		else
			port_Func_20e6a();
	}

	void port_Func_21af1()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F))
			port_Func_209ca();
		else if (side & (1 << INTERACTION_DOWN_F))
			port_StepOnObject();
		else if (mem_.attack_counter())
			port_Func_217a9();
		else
			port_Func_20a6f();
	}

	void port_ObjInteraction_DollBoyHammer()
	{
		port_Func_2160a();
	}

	void port_ObjInteraction_Wormwould()
	{
		if (vert_step())
			return;
		if (mem_.attack_counter())
			port_AttackObject();
		else
			port_Func_21b3a(); // fallthrough
	}

	void port_ObjInteraction_Turtle()
	{
		if (mem_.interaction_side() & (1 << INTERACTION_UP_F))
			port_Func_209ca();
		else if (mem_.transformation())
			return;
		else if (!mem_.water_interaction())
			port_Func_21b3a();
		else
			FARCALL(StartDive);
	}

	void port_ObjInteraction_Shoot()
	{
		if (mem_.transformation() == TRANSFORMATION_BALL) {
			port_Func_21e8a();
			return;
		}
		uint8_t side = mem_.interaction_side();
		if (!(side & (1 << INTERACTION_DOWN_F))) {
			if (side & (1 << INTERACTION_UP_F))
				port_StepOnObject();
			else
				port_Func_20e63();
			return;
		}
		// .interaction_down
		if (mem_.invincible_counter()) // skip if invincible
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		// turn Wario into ball form
		set_obj_action(OBJACTION_06);
		mem_.transformation() = TRANSFORMATION_BALL;
		touch_states(TOUCH_PASS_THROUGH, 0x02);
		FARCALL(SetState_BallStart);
	}

	void port_Func_21e9c()
	{
		if (mem_.transformation() == TRANSFORMATION_BALL)
			port_Func_21e8a();
		else
			port_Func_20939();
	}

	void port_ObjInteraction_Richtertoffen()
	{
		uint8_t side = mem_.interaction_side();
		if (side & (1 << INTERACTION_UP_F)) {
			if (mem_.is_smash_attacking() == 1)
				port_Func_20602();
			else
				port_StepOnObject();
			return;
		}
		if (!(side & (1 << INTERACTION_DOWN_F))) {
			port_Func_20932();
			return;
		}
		// .try_flatten_wario
		if (mem_.invincible_counter())
			return;
		if (mem_.transformation() & (1 << TRANSFORMATIONF_PERSISTENT_F)) {
			port_Func_2022c();
			return;
		}
		// flattened, with room for the wider box
		mem_.collision_box_left() = static_cast<uint8_t>(-11);
		mem_.collision_box_right() = 11;
		if (mem_.ladder_interaction() != LADDER_UNDERNEATH) {
			if (!collision(BANK_Func_197b1, ADDR_Func_197b1, check_func_197b1)) {
				if (!collision(BANK_Func_19741, ADDR_Func_19741, check_func_19741)) {
					set_obj_action(OBJACTION_FLATTEN);
					FARCALL(SetState_FlatAirborne);
					return;
				}
			}
		}
		mem_.collision_box_left() = static_cast<uint8_t>(-9);
		mem_.collision_box_right() = 9;
		port_StepOnObject();
	}

	void port_ObjInteraction_Omodonmeka()
	{
		if (mem_.jump_vel_table()) {
			port_Func_2022c();
			return;
		}
		set_obj_action(OBJACTION_0C);
		uint8_t transformation = mem_.transformation();
		if (transformation == TRANSFORMATION_FLAT_WARIO) {
			port_SetState_FlatStretching();
			return;
		}
		if (transformation) {
			port_Func_2022c();
			return;
		}
		// pick Wario up
		mem_.auto_move_state() = 0x01;
		mem_.wario_state() = WST_PICKED_UP;
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wIsSmashAttacking, wAttackCounter, wIsCrouching,
			wGrabState, wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame, 0};
		clear(zero);
		mem_.jump_vel_table() = 0x01;
		mem_.picked_up_wiggle_counter() = NUM_WIGGLES_TO_ESCAPE;
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
	}

	void port_ObjInteraction_OrangeBird()
	{
		if (vert_step())
			return;
		if (!obj_side(true, true)) {
			port_Func_2022c();
			return;
		}
		// .asm_217de
		if (mem_.invincible_counter())
			return;
		if (persistent_or(TRANSFORMATION_CRAZY_WARIO)) {
			port_Func_2022c();
			return;
		}
		set_obj_action(OBJACTION_06);
		mem_.transformation() = TRANSFORMATION_CRAZY_WARIO;
		touch_states(TOUCH_ATTACK, 0x01);
		FARCALL(SetState_CrazySpinning);
	}

	void port_ObjInteraction_Owl()
	{
		if (!(mem_.interaction_side() & (1 << INTERACTION_DOWN_F))) {
			port_Func_20939();
			return;
		}
		if (persistent_or(TRANSFORMATION_OWL_WARIO))
			return;
		port_Func_20a63();
		play_sfx(SFX_HANG_GRAB);
		mem_.transformation() = TRANSFORMATION_OWL_WARIO;
		mem_.touch_state() = TOUCH_BUMP;
		mem_.sting_touch_state() = TOUCH_VULNERABLE;
		mem_.unk_ca94() = 0x01;
		mem_.wario_state() = WST_GRABBING_OWL;
		collision_box(-1, -27, -9, 9);
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wIsCrouching, wIsRolling, wIsSmashAttacking, wJumpVelTable,
			wJumpVelIndex, 0};
		clear(zero);
		home::update_level_music(gb_);
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		load_gfx(BANK_WarioHangGfx, ADDR_WarioHangGfx);
		set_oam(BANK_OAM_1fddb4, ADDR_OAM_1fddb4);
		mem_.direction() = mem_.obj_direction();
		set_frameset(ADDR_Frameset_1fed53);
		anim(OAM2);
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::ObjectInteractions;

// farcalled by update_level: hCallFunc pops af, and what follows (xor a,
// DrawObjects_NoPriority) sets every register it uses
PORT_EX(ProcessInteractions, 0, ALL_REGISTERS) { wl3::level::process_interactions(gb); }
PORT_COVERED(SetObjAction);
PORT_COVERED(Func_20a63);
PORT_COVERED(Func_20a69);
PORT_COVERED(Func_21cc9);
PORT_COVERED(CollectKey);
PORT_EX(SetState_OnFire, 0, ALL_REGISTERS) { ObjectInteractions(gb).port_SetState_OnFire(); }
PORT_COVERED(VanishObjectAndKnockBackIfZombie);
PORT_EX(SetState_OnFire_ResetStateCounter, 0, ALL_REGISTERS) { ObjectInteractions(gb).port_SetState_OnFire_ResetStateCounter(); }
PORT_COVERED(ObjInteraction_RedKey);
PORT_COVERED(ObjInteraction_GreyKey);
PORT_COVERED(ObjInteraction_GreenKey);
PORT_COVERED(ObjInteraction_BlueKey);
PORT_COVERED(Func_20fe8);
PORT_COVERED(ObjInteraction_None);
PORT_COVERED(ObjInteraction_Ink);
PORT_COVERED(Func_2092d);
PORT_COVERED(ObjInteraction_Hand);
PORT_COVERED(ObjInteraction_UnlitTorch);
PORT_COVERED(SetState_FlatStretching);
PORT_COVERED(TreasureClearObjects);
PORT_COVERED(SetState_EnemyBumping);
PORT_COVERED(ObjInteraction_Food);
PORT_COVERED(ObjInteraction_Electric);
PORT_COVERED(ObjInteraction_RegularCoin);
PORT_COVERED(CollectMusicalCoin);
PORT_COVERED(ObjInteraction_ColourCoin);
PORT_COVERED(ObjInteraction_MusicalCoin);
PORT_COVERED(ObjInteraction_InvisibilityPotion);
PORT_COVERED(ObjInteraction_RollingRock);
PORT_COVERED(ObjInteraction_Vampire);
PORT_COVERED(ObjInteraction_Bubble);
PORT_COVERED(ObjInteraction_ZombieHead);
PORT_COVERED(Func_2028a);
PORT_COVERED(Func_208f2);
PORT_COVERED(Func_20899);
PORT_EX(SetState_WaterStung, 0, ALL_REGISTERS) { ObjectInteractions(gb).port_SetState_WaterStung(); }
PORT_COVERED(ObjInteraction_Rail);
PORT_COVERED(GetTreasure);
PORT_EX(SetState_Stung, 0, ALL_REGISTERS) { ObjectInteractions(gb).port_SetState_Stung(); }
PORT_EX(Func_206eb, 0, ALL_REGISTERS) { ObjectInteractions(gb).port_Func_206eb(); }
PORT_COVERED(Func_207ed);
PORT_COVERED(ObjInteraction_WaterTeleporting);
PORT_COVERED(ObjInteraction_Jamano);
PORT_COVERED(Func_21e8a);
PORT_COVERED(Func_20d81);
PORT_COVERED(Func_2168b);
PORT_COVERED(ObjInteraction_Snow);
PORT_COVERED(ObjInteraction_Ice);
PORT_COVERED(Func_209ca);
PORT_COVERED(Func_20a6f);
PORT_COVERED(AttackObject);
PORT_COVERED(Func_217a9);
PORT_COVERED(Func_20d8c);
PORT_COVERED(Func_20deb);
PORT_COVERED(ObjInteraction_Haridama);
PORT_COVERED(ObjInteraction_Pesce);
PORT_COVERED(ObjInteraction_Scowler);
PORT_COVERED(ObjInteraction_GreyTreasure);
PORT_COVERED(ObjInteraction_RedTreasure);
PORT_COVERED(ObjInteraction_GreenTreasure);
PORT_COVERED(ObjInteraction_BlueTreasure);
PORT_COVERED(ObjInteraction_Solid);
PORT_COVERED(ObjInteraction_Omodon);
PORT_COVERED(Func_21abb);
PORT_COVERED(Func_21f01);
PORT_COVERED(Func_20602);
PORT_COVERED(Func_21aac);
PORT_COVERED(ObjInteraction_Stove);
PORT_COVERED(ObjInteraction_WaterDrop);
PORT_COVERED(Func_2023b);
PORT_COVERED(Func_2022c);
PORT_COVERED(Func_20939);
PORT_COVERED(Func_20932);
PORT_COVERED(StepOnObject);
PORT_COVERED(Func_20670);
PORT_COVERED(Func_20b6b);
PORT_COVERED(Func_20b41);
PORT_COVERED(Func_20d1d);
PORT_COVERED(ObjInteraction_FrontSting);
PORT_COVERED(ObjInteraction_BackSting);
PORT_COVERED(ObjInteraction_TopSting);
PORT_COVERED(ObjInteraction_FullSting);
PORT_COVERED(Func_20e39);
PORT_COVERED(Func_20e60);
PORT_COVERED(Func_20e6a);
PORT_COVERED(Func_20e63);
PORT_COVERED(ObjInteraction_Walkable);
PORT_COVERED(ObjInteraction_Fire);
PORT_COVERED(Func_2126a);
PORT_COVERED(ObjInteraction_Silky);
PORT_COVERED(ObjInteraction_Teruteru);
PORT_COVERED(ObjInteraction_Beam);
PORT_COVERED(Func_21b3a);
PORT_COVERED(Func_21b78);
PORT_COVERED(ObjInteraction_YellowBellyArrow);
PORT_COVERED(ObjInteraction_Pneumo);
PORT_COVERED(Func_2160a);
PORT_COVERED(ObjInteraction_Bouncy);
PORT_COVERED(ObjInteraction_FireBot);
PORT_COVERED(Func_21c56);
PORT_COVERED(ObjInteraction_Tadpole);
PORT_COVERED(ObjInteraction_Hebarii);
PORT_COVERED(ObjInteraction_PrinceFroggy);
PORT_COVERED(Func_21d3b);
PORT_COVERED(Func_21d64);
PORT_COVERED(Func_21d6f);
PORT_COVERED(Func_21f28);
PORT_COVERED(Func_21dac);
PORT_COVERED(Func_21d88);
PORT_COVERED(Func_21ea6);
PORT_COVERED(Func_21ecd);
PORT_COVERED(ObjInteraction_MagicalSpiral);
PORT_COVERED(ObjInteraction_BottomSting);
PORT_COVERED(ObjInteraction_AnonsterSilk);
PORT_COVERED(ObjInteraction_AnonsterWave);
PORT_COVERED(ObjInteraction_RoboMouse);
PORT_COVERED(Func_21af1);
PORT_COVERED(ObjInteraction_DollBoyHammer);
PORT_COVERED(ObjInteraction_Wormwould);
PORT_COVERED(ObjInteraction_Turtle);
PORT_COVERED(ObjInteraction_Shoot);
PORT_COVERED(Func_21e9c);
PORT_COVERED(ObjInteraction_Richtertoffen);
PORT_COVERED(ObjInteraction_Omodonmeka);
PORT_COVERED(ObjInteraction_OrangeBird);
PORT_COVERED(ObjInteraction_Owl);

namespace wl3::level {
void process_interactions(GB *gb) { ObjectInteractions(gb).port_ProcessInteractions(); }
} // namespace wl3::level
