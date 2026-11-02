// Wario's states, part 2 (src/engine/level/wario_states_2.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "../asm_module.hpp"
#include "../home/wario.hpp"

namespace wl3::level {

namespace {

class WarioStates2 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The slipping movement shared by the slipping states, from
	// ld hl, wWarioStateCounter / inc [hl] on. Returns true if it ends at the
	// "stop slipping" label; otherwise the routine returns.
	bool slip_move(uint8_t t1, uint8_t t2, uint8_t t3, bool stop_at_or_above_t3)
	{
		uint8_t v = inc_counter();
		alu_.cp(v, t1);
		if (CF) {
			cpu_.b = 2;
			move_by_dir();
			return false;
		}
		alu_.cp(v, t2);
		if (CF) {
			cpu_.b = 1;
			move_by_dir();
			return false;
		}
		alu_.cp(v, t3);
		if (CF) {
			cpu_.a = alu_.and_(rd8(wGlobalCounter), 1);
			if (ZF)
				return false;
			cpu_.b = 1;
			move_by_dir();
			return false;
		}
		if (stop_at_or_above_t3 || ZF)
			return true;
		return false;
	}

	// Dispatch on wWarioState: ld a, [wWarioState] / sub $30 / rst JumpTable
	// wWarioState - $30, as the asm's jumptable
	void port_UpdateWarioStates_Group2()
	{
		switch (static_cast<uint8_t>(rd8(wWarioState) - 0x30)) {
		case 0x00: port_UpdateState_Slipping(); return;
		case 0x01: port_UpdateState_CrouchSlipping(); return;
		case 0x02: port_UpdateState_DraggedDown(); return;
		case 0x03: port_UpdateState_Teleporting(); return;
		case 0x04: port_UpdateState_WaterTeleporting(); return;
		case 0x05: port_UpdateState_SandFalling(); return;
		case 0x06: port_UpdateState_SandJumping(); return;
		case 0x07: port_UpdateState_SandIdling(); return;
		case 0x08: port_UpdateState_SandWalking(); return;
		case 0x09: port_UpdateState_SandTurning(); return;
		case 0x0a: port_UpdateState_LadderClimbing(); return;
		case 0x0b: port_UpdateState_LadderIdling(); return;
		case 0x0c: port_UpdateState_LadderShakeSlipping(); return;
		case 0x0d: port_UpdateState_GettingOffLadder(); return;
		case 0x0e: port_UpdateState_LadderSliding(); return;
		case 0x0f: port_UpdateState_GrabSlipping(); return;
		case 0x10: port_UpdateState_GetTreasureStart(); return;
		case 0x11: port_UpdateState_GetTreasureWalk(); return;
		case 0x12: port_UpdateState_GetTreasureTurnFront(); return;
		case 0x13: port_UpdateState_GetTreasureLaugh(); return;
		case 0x14: port_UpdateState_GetTreasureTurnBack(); return;
		case 0x15: port_UpdateState_GetTreasureOpen(); return;
		case 0x16: port_UpdateState_GetTreasureTurnWalk(); return;
		case 0x17: port_UpdateState_GetTreasureWalk(); return;
		case 0x18: port_UpdateState_GetTreasureTurnFront(); return;
		case 0x19: port_UpdateState_GetTreasureLookFront(); return;
		case 0x1a: port_UpdateState_GetTreasureClear(); return;
		case 0x1b: port_UpdateState_Sleeping(); return;
		case 0x1c: port_UpdateState_LadderScratching(); return;
		case 0x1d: port_UpdateState_FenceShakeSliding(); return;
		case 0x1e: port_UpdateState_FenceMovingVertical(); return;
		case 0x1f: port_UpdateState_FenceIdling(); return;
		case 0x20: port_UpdateState_FenceMovingHorizontal(); return;
		case 0x21: port_UpdateState_FenceSliding(); return;
		case 0x22: CALL(InvalidWarioStateReset); return;
		case 0x23: CALL(InvalidWarioStateReset); return;
		case 0x24: CALL(InvalidWarioStateReset); return;
		case 0x25: CALL(InvalidWarioStateReset); return;
		case 0x26: CALL(InvalidWarioStateReset); return;
		case 0x27: CALL(InvalidWarioStateReset); return;
		case 0x28: CALL(InvalidWarioStateReset); return;
		case 0x29: CALL(InvalidWarioStateReset); return;
		case 0x2a: CALL(InvalidWarioStateReset); return;
		case 0x2b: CALL(InvalidWarioStateReset); return;
		case 0x2c: CALL(InvalidWarioStateReset); return;
		case 0x2d: CALL(InvalidWarioStateReset); return;
		case 0x2e: CALL(InvalidWarioStateReset); return;
		case 0x2f: CALL(InvalidWarioStateReset); return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	void port_SetState_Slipping()
	{
		wr8(wWarioState, WST_SLIPPING);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wAttackCounter, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		ld(wDirection);
		wr8(wWarioStateCycles, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_1425f : ADDR_Frameset_14252);
		anim(OAM1);
	}

	void port_UpdateState_Slipping()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}

		uint8_t cycles = ld(wWarioStateCycles);
		alu_.cp(cycles, 0x04);
		if (!CF)
			goto anim_and_input;
		alu_.cp(cycles, 0x02);
		if (!CF)
			goto walk_turn;
		cpu_.b = cycles;
		if (!ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			goto anim_and_input;
		cpu_.a = (uint8_t)(cpu_.a << 3 | cpu_.a >> 5); // rlca x3
		cpu_.a = alu_.and_(cpu_.a, 0x01);
		alu_.cp(cpu_.a, cpu_.b);
		if (!ZF)
			goto anim_and_input;
		cpu_.a = alu_.xor_(cpu_.a, 1);
		cpu_.a = alu_.add(cpu_.a, 2);
		wr8(wWarioStateCycles, cpu_.a);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		set_frameset(ld_mask(wWarioStateCycles, 0x01) ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
		goto reset_anim;

	walk_turn:
		cpu_.b = cycles;
		if (!anim_ended())
			goto anim_and_input;
		cpu_.a = alu_.add(cpu_.b, 0x02);
		wr8(wWarioStateCycles, cpu_.a);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		set_frameset(ld_mask(wWarioStateCycles, 0x01) ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);

	reset_anim:
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);

	anim_and_input:
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(StartJump_FromInput);
			return;
		}
		if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
			port_Func_19e89();
			return;
		}

		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_SLIPPING);
		if (!ZF) {
			return;
		}
		if (!b_and()) {
			FARCALL(StartFall);
			return;
		}

		update_pos_y();
		FARCALL(CheckFrontCollision);
		ld_cp(wWarioState, WST_SLIPPING);
		if (!ZF) {
			return;
		}
		if (b_and())
			goto stop;
		CALL(IsOnSlipperyGround);
		if (ZF)
			goto stop;
		if (slip_move(0x08, 0x0f, 0x15, false))
			goto stop;
		return;

	stop:
		cpu_.a = alu_.and_(rd8(wWarioStateCycles), 0x01);
		wr8(wDirection, cpu_.a);
		FARCALL(SetState_Idling);
	}

	void port_SetState_CrouchSlipping()
	{
		xor_a();
		wr8(wWarioStateCounter, 0);
		ld(wDirection);
		wr8(wWarioStateCycles, cpu_.a);
		port_Func_19e89();
	}

	void port_Func_19e89()
	{
		wr8(wWarioState, WST_CROUCH_SLIPPING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wAttackCounter, 0);
		wr8(wGrabState, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsCrouching, cpu_.a);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		ld_cp(wWarioStateCycles, 0x01);
		set_frameset(ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
	}

	void port_UpdateState_CrouchSlipping()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(StartCrouchJump);
			return;
		}

		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_CROUCH_SLIPPING);
		if (!ZF) {
			return;
		}
		if (!b_and()) {
			FARCALL(StartCrouchFall);
			return;
		}

		update_pos_y();
		FARCALL(CheckFrontCollision);
		ld_cp(wWarioState, WST_CROUCH_SLIPPING);
		if (!ZF) {
			return;
		}
		if (b_and())
			goto stop;
		CALL(IsOnSlipperyGround);
		if (ZF)
			goto stop;
		if (slip_move(0x06, 0x0a, 0x0e, true))
			goto stop;
		return;

	stop:
		cpu_.a = alu_.and_(rd8(wWarioStateCycles), 0x01);
		wr8(wDirection, cpu_.a);
		FARCALL(CrouchOrSlide);
	}

	void port_SetState_DraggedDown()
	{
		wr8(wWarioState, WST_DRAGGED_DOWN);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wAttackCounter, 0);
		wr8(wGrabState, 0);
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wIsCrouching, 0);
		wr8(wIsRolling, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wAutoMoveState, cpu_.a);
		coll_box_blrt();
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
	}

	void port_UpdateState_DraggedDown()
	{
		anim(OAM1);
		if (rd8(wAutoMoveState))
			return;
		if (rd8(wLevel) == THE_TEMPLE) {
			st(wLevelEndScreen, LVLEND_GAME_OVER);
			return;
		}
		FARCALL(StartFall);
	}

	void teleporting(uint8_t sfx_frame, uint8_t next_state)
	{
		wr8(wIsIntangible, TRUE);
		anim(OAM1);
		uint8_t v = inc_counter();
		alu_.cp(v, sfx_frame);
		if (ZF) {
			// jr z, PlayTeleportingSFX
			play_sfx(SFX_082);
			return;
		}
		alu_.cp(v, 0xb4);
		if (CF) {
			return;
		}
		wr8(wWarioStateCounter, 0x00);
		wr8(wWarioState, next_state);
		wr8(wInvincibleCounter, 0x01);
		SET_HL(gb_, wRoomTransitionParam);
		wr8(wRoomTransitionParam,
			rd8(wRoomTransitionParam) & (uint8_t)~(1 << ROOMTRANSITIONF_DELAY_F));
		CALL(TriggerRoomTransition);
	}

	void port_UpdateState_Teleporting()
	{
		teleporting(0x01, WST_STUNG);
	}

	void port_PlayTeleportingSFX()
	{
		play_sfx(SFX_082);
	}

	void port_UpdateState_WaterTeleporting()
	{
		teleporting(0x11, WST_WATER_STUNG);
	}

	void port_SetState_SandFalling()
	{
		xor_a();
		wr8(wGrabState, 0);
		wr8(wWarioState, WST_SAND_FALLING);
		wr8(wCollisionBoxBottom, (uint8_t)-1);
		wr8(wCollisionBoxTop, (uint8_t)-27);
		wr8(wCollisionBoxLeft, (uint8_t)-9);
		wr8(wCollisionBoxRight, 9);

		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wAttackCounter, 0);
		wr8(wIsRolling, 0);
		wr8(wSwimVelIndex, 0);
		wr8(wIsCrouching, 0);
		st(wc0e0, 0);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		LCALL(SetWarioPal); // waits for HBlank
		port_Func_1a12a(); // fallthrough
	}

	void port_Func_1a12a()
	{
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		port_Func_1a14b(); // fallthrough
	}

	void port_Func_1a14b()
	{
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_15f94, ADDR_Frameset_15f97);
		anim(OAM1);
	}

	void port_UpdateState_SandFalling()
	{
		port_Func_1b2c0();
		ld_cp(wWarioState, WST_SAND_FALLING);
		if (!ZF) {
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			return;
		}
		update_pos_y();
		port_SetState_SandIdling();
	}

	void port_SetState_SandJumping()
	{
		play_sfx(SFX_JUMP);
		wr8(wWarioState, WST_SAND_JUMPING);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		port_Func_1a12a();
	}

	// ld a, [wWalkVelIndex] / cp $04 / jr c / ld a, $00 / ld [wWalkVelIndex], a
	void clamp_walk_vel()
	{
		ld_cp(wWalkVelIndex, 0x04);
		if (!CF)
			st(wWalkVelIndex, 0x00);
	}

	void port_UpdateState_SandJumping()
	{
		FARCALL(Func_19b25);
		if (!ld_and(wIsInSand)) {
			FARCALL(StartJump_FromSand);
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(Func_2b17a);
		clamp_walk_vel();
		if (ld_and(wIsTurning))
			port_Func_1a14b();

		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			port_SetState_SandFalling();
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (!CF) {
			port_SetState_SandFalling();
			return;
		}
		FARCALL(CheckUpCollision);
		if (!b_and()) {
			return;
		}
		port_SetState_SandFalling();
	}

	void port_SetState_SandIdling()
	{
		wr8(wWarioState, WST_SAND_IDLING);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	void port_UpdateState_SandIdling()
	{
		port_Func_1b322();
	}

	void port_SetState_SandWalking()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_SAND_WALKING);
		if (ld_bit(wJoypadDown, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			wr8(wDirection, DIRECTION_LEFT);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		FARCALL(Func_1e6ea);
	}

	void port_UpdateState_SandWalking()
	{
		sfx_loop(0x24, SFX_WALK);
		anim(OAM1);
		port_Func_1b333();
		ld_cp(wWarioState, WST_SAND_WALKING);
		if (!ZF) {
			return;
		}
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			port_SetState_SandFalling();
			return;
		}
	}

	void port_SetState_SandTurning()
	{
		wr8(wWarioState, WST_SAND_TURNING);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		cpu_.a = alu_.xor_(rd8(wDirection), 1);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
		anim(OAM1);
	}

	void port_UpdateState_SandTurning()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_SandWalking();
		else
			port_SetState_SandIdling();
	}

	void port_SetState_LadderClimbing()
	{
		wr8(wWarioState, WST_LADDER_CLIMBING);
		center_on_ladder();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wIsGettingOffLadder, 0);
		coll_box_blrt();
		load_gfx(BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
		set_oam(BANK_OAM_155cc, ADDR_OAM_155cc);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(ADDR_Frameset_158d6);
		anim(OAM1);
	}

	void port_UpdateState_LadderClimbing()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		sfx_loop(32, SFX_CLIMB);
		anim(OAM1);
		if (anim_ended()) {
			get_pos();
			cpu_.b = PARTICLE_DUST;
			FARCALL(CreateParticle);
		}
		port_HandleLadderInput();
	}

	void port_SetState_LadderIdling()
	{
		wr8(wWarioState, WST_LADDER_IDLING);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		wr8(wIsGettingOffLadder, 0);
		coll_box_blrt();
		load_gfx(BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
		set_oam(BANK_OAM_155cc, ADDR_OAM_155cc);
		frameset_by_dir(ADDR_Frameset_15900, ADDR_Frameset_158fd);
		anim(OAM1);
	}

	void port_UpdateState_LadderIdling()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (!ld_and(wLadderInteraction)) {
			FARCALL(StartFall);
			return;
		}
		anim(OAM1);
		port_Func_1b480();
	}

	void port_UpdateState_LadderShakeSlipping()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (!ld_and(wLadderInteraction)) {
			FARCALL(StartFall);
			return;
		}
		anim(OAM1);
		cpu_.b = 0x02;
		CALL(MoveWarioDown);
		if (down_floor_check())
			CALL(StartDownwardsFloorTransition);
		FARCALL(CheckAirborneCollision);
		if (b_and()) {
			update_pos_y();
			FARCALL(SetState_Idling);
			return;
		}
		uint8_t v = inc_counter();
		alu_.cp(v, 0x40);
		if (CF) {
			return;
		}
		wr8(wWarioStateCounter, 0x00);
		port_SetState_LadderIdling();
	}

	void port_SetState_GettingOffLadder()
	{
		wr8(wWarioState, WST_GETTING_OFF_LADDER);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsGettingOffLadder, cpu_.a);
		wr8(wIsIntangible, cpu_.a);
		if (ld_bit(wJoypadDown, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else
			st(wDirection, DIRECTION_LEFT);
	}

	void port_UpdateState_GettingOffLadder()
	{
		sfx_loop(32, SFX_CLIMB);
		bool right = ld_and(wDirection);
		cpu_.b = 0x01;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		FARCALL(Func_19b25);
		if (rd8(wLadderInteraction))
			return;
		wr8(wIsGettingOffLadder, 0);
		wr8(wIsIntangible, 0);
		FARCALL(StartFall);
	}

	void set_state_sliding(uint8_t state)
	{
		wr8(wWarioState, state);
		play_sfx(SFX_SLIDE);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsGettingOffLadder, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		if (rd8(wPowerUpLevel) >= LEAD_OVERALLS)
			wr8(wIsSmashAttacking, TRUE);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(ADDR_Frameset_15945);
		anim(OAM1);
	}

	void port_SetState_LadderSliding()
	{
		set_state_sliding(WST_LADDER_SLIDING);
	}

	// The shared body of UpdateState_LadderSliding and UpdateState_FenceSliding.
	void update_sliding(bool fence)
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (!rd8(fence ? wIsNearFence : wLadderInteraction)) {
			xor_a();
			wr8(wIsSmashAttacking, FALSE);
			FARCALL(StartFall);
			return;
		}
		anim(OAM1);
		if (ld_bit(wJoypadDown, B_PAD_UP)) {
			if (fence)
				port_SetState_FenceMovingVertical();
			else
				port_SetState_LadderClimbing();
			return;
		}
		cpu_.b = 0x02;
		CALL(MoveWarioDown);
		if (down_floor_check())
			CALL(StartDownwardsFloorTransition);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			return;
		}
		update_pos_y();
		if (!ld_and(wIsSmashAttacking)) {
			FARCALL(SetState_Idling);
			return;
		}
		ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (CF)
			FARCALL(DoGroundPound);
		else
			FARCALL(DoWarioGroundShake);
	}

	void port_UpdateState_LadderSliding()
	{
		update_sliding(false);
	}

	void port_SetState_GrabSlipping()
	{
		wr8(wWarioState, WST_GRAB_SLIPPING);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wAttackCounter, 0);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_14a20, ADDR_Frameset_14a23);
		anim(OAM1);
	}

	void port_UpdateState_GrabSlipping()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (!ld_and(wGrabState)) {
			FARCALL(SetState_Idling);
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(Func_1ede9);
			return;
		}
		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_GRAB_SLIPPING);
		if (!ZF) {
			return;
		}
		if (!b_and()) {
			FARCALL(Func_1edd3);
			return;
		}
		FARCALL(CheckFrontCollision);
		ld_cp(wWarioState, WST_GRAB_SLIPPING);
		if (!ZF) {
			return;
		}
		if (b_and())
			goto stop;
		CALL(IsOnSlipperyGround);
		if (ZF)
			goto stop;
		if (slip_move(0x08, 0x0f, 0x15, false))
			goto stop;
		return;

	stop:
		FARCALL(SetState_GrabIdling);
	}

	void port_UpdateState_GetTreasureStart()
	{
		anim(OAM1);
		if (ld_and(wJumpVelTable)) {
			CALL(ApplyJumpVelocity);
			FARCALL(CheckAirborneCollision);
			if (!b_and()) {
				return;
			}
			wr8(wWarioPos + 0, rd8(hYPosHi));
			cpu_.a = rd8(hYPosHi + 1);
			wr8(wWarioPos + 1, cpu_.a);
			SET_HL(gb_, hYPosHi + 1);
			SET_DE(gb_, wWarioPos + 1);
			xor_a();
			wr8(wJumpVelTable, 0);
			wr8(wJumpVelIndex, 0);
		}
		uint8_t v = inc_counter();
		alu_.cp(v, 48);
		if (CF) {
			return;
		}
		st(wWarioStateCounter, 24);
		port_SetNextGetTreasureState(); // fallthrough
	}

	void port_SetNextGetTreasureState()
	{
		SET_HL(gb_, wWarioState);
		wr8(wWarioState, alu_.inc(rd8(wWarioState)));
		wr8(wWarioStateCycles, 0);
		FARCALL(Func_1e6ea);
	}

	void port_UpdateState_GetTreasureWalk()
	{
		anim(OAM1);
		bool right = ld_and(wDirection);
		cpu_.b = 1;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);

		SET_HL(gb_, wWarioStateCounter);
		uint8_t v = alu_.dec(rd8(wWarioStateCounter));
		wr8(wWarioStateCounter, v);
		if (v)
			return;
		SET_HL(gb_, wWarioState);
		wr8(wWarioState, alu_.inc(rd8(wWarioState))); // WST_GET_TREASURE_TURN_FRONT_1
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
		frameset_by_dir(ADDR_Frameset_1fc46a, ADDR_Frameset_1fc461);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureTurnFront()
	{
		anim(OAM2);
		if (!anim_ended()) {
			return;
		}
		ld_cp(wWarioState, WST_GET_TREASURE_TURN_FRONT_2);
		if (ZF) {
			port_SetState_GetTreasureLookFront();
			return;
		}
		wr8(wWarioState, WST_GET_TREASURE_LAUGH);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioClearGfx, ADDR_WarioClearGfx);
		set_oam(BANK_OAM_1fc000, ADDR_OAM_1fc000);
		set_frameset(ADDR_Frameset_1fc2ab);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureLaugh()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioState, WST_GET_TREASURE_TURN_BACK);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
		frameset_by_dir(ADDR_Frameset_1fc446, ADDR_Frameset_1fc439);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureTurnBack()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioState, WST_GET_TREASURE_OPEN);
		play_sfx(SFX_03E);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioClearGfx, ADDR_WarioClearGfx);
		set_oam(BANK_OAM_1fc000, ADDR_OAM_1fc000);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(ADDR_Frameset_1fc2c4);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureOpen()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		st(wWarioState, WST_GET_TREASURE_TURN_WALK);
		CALL(LoadCollectedTreasurePal_Level);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
		cpu_.a = alu_.xor_(rd8(wDirection), 1);
		wr8(wDirection, cpu_.a);
		set_frameset(alu_.and_(cpu_.a, cpu_.a) ? ADDR_Frameset_1fc453 : ADDR_Frameset_1fc45a);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureTurnWalk()
	{
		anim(OAM2);
		if (!anim_ended()) {
			return;
		}
		wr8(wWarioStateCounter, 32);
		xor_a();
		wr8(wAutoMoveState, 0);
		port_SetNextGetTreasureState();
	}

	void port_SetState_GetTreasureLookFront()
	{
		wr8(wWarioState, WST_GET_TREASURE_LOOK_FRONT);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioClearGfx, ADDR_WarioClearGfx);
		set_oam(BANK_OAM_1fc000, ADDR_OAM_1fc000);
		set_frameset(ADDR_Frameset_1fc30d);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureLookFront()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioState, WST_GET_TREASURE_CLEAR);
		get_pos();
		cpu_.b = PARTICLE_TREASURE_GLOW;
		FARCALL(CreateParticle);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
	}

	void port_UpdateState_GetTreasureClear()
	{
		SET_HL(gb_, wWarioStateCounter);
		if (!(rd8(wJoypadDown) >> B_PAD_A & 1)) {
			cpu_.a = alu_.inc(rd8(wWarioStateCounter));
			wr8(wWarioStateCounter, cpu_.a);
			alu_.cp(cpu_.a, 180);
			if (CF)
				return;
		}
		if (!rd8(wCurParticleAnimEnded))
			return;
		wr8(wWarioStateCounter, 0);
		SET_HL(gb_, wLevelEndScreen);
		wr8(wLevelEndScreen, rd8(wLevelEndScreen) & 0x7f);
	}

	void port_SetState_Sleeping()
	{
		wr8(wWarioState, WST_SLEEPING);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_1421c, ADDR_Frameset_14225);
		anim(OAM1);
	}

	void port_UpdateState_Sleeping()
	{
		anim(OAM1);
		cpu_.a = alu_.dec(rd8(wWarioStateCounter));
		if (ZF) {
			// .asm_1acd0
			if (anim_ended())
				play_sfx(SFX_SLEEP);
			if (!rd8(wJoypadDown))
				return;
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			inc_counter();
			frameset_by_dir(ADDR_Frameset_14240, ADDR_Frameset_14249);
			anim(OAM1);
			return;
		}
		cpu_.a = alu_.dec(cpu_.a);
		if (ZF) {
			// .asm_1ad0c
			if (!anim_ended())
				return;
			FARCALL(SetState_Idling);
			return;
		}
		if (!anim_ended())
			return;
		play_sfx(SFX_SLEEP);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		inc_counter();
		frameset_by_dir(ADDR_Frameset_1422e, ADDR_Frameset_14237);
		anim(OAM1);
	}

	void port_SetState_LadderScratching()
	{
		wr8(wWarioState, WST_LADDER_SCRATCHING);
		coll_box_btlr();
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsGettingOffLadder, 0);
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		frameset_by_dir(ADDR_Frameset_15903, ADDR_Frameset_15924);
		anim(OAM1);
	}

	void port_UpdateState_LadderScratching()
	{
		anim(OAM1);
		if (!ld_and(wJoypadDown)) {
			if (!anim_ended()) {
				return;
			}
		}
		port_SetState_LadderIdling();
	}

	void port_SetState_FenceShakeSliding()
	{
		wr8(wWarioState, WST_FENCE_SHAKE_SLIDING);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		st(wAutoMoveState, 0x03);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		frameset_by_dir(ADDR_Frameset_14d0b, ADDR_Frameset_14d10);
		anim(OAM1);
	}

	void port_UpdateState_FenceShakeSliding()
	{
		anim(OAM1);
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			xor_a();
			wr8(wFrameDuration, 0);
			wr8(wAnimationFrame, 0);
			set_frameset(ADDR_Frameset_14d15);
			anim(OAM1);
			inc_counter();
		}
		if (!ld_and(wJumpVelTable))
			return;
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (!b_and())
			return;
		update_pos_y();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	void port_SetState_FenceMovingVertical()
	{
		wr8(wWarioState, WST_FENCE_MOVING_VERTICAL);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		coll_box_blrt();
		load_gfx(BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
		set_oam(BANK_OAM_155cc, ADDR_OAM_155cc);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		set_frameset(ADDR_Frameset_158d6);
		anim(OAM1);
	}

	void port_UpdateState_FenceMovingVertical()
	{
		sfx_loop(0x20, SFX_CLIMB);
		anim(OAM1);
		if (anim_ended()) {
			get_pos();
			cpu_.b = PARTICLE_DUST;
			FARCALL(CreateParticle);
		}
		port_Func_1b168();
	}

	void port_SetState_FenceIdling()
	{
		wr8(wWarioState, WST_FENCE_IDLING);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		coll_box_blrt();
		load_gfx(BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
		set_oam(BANK_OAM_155cc, ADDR_OAM_155cc);
		frameset_by_dir(ADDR_Frameset_15900, ADDR_Frameset_158fd);
		anim(OAM1);
	}

	void port_UpdateState_FenceIdling()
	{
		anim(OAM1);
		port_Func_1b21a();
	}

	void port_SetState_FenceMovingHorizontal()
	{
		wr8(wWarioState, WST_FENCE_MOVING_HORIZONTAL);
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		coll_box_blrt();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		bool right;
		if (ld_bit(wJoypadDown, B_PAD_RIGHT))
			right = true;
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			right = false;
		else
			right = ld_and(wDirection);
		set_frameset(right ? ADDR_Frameset_158e7 : ADDR_Frameset_158f2);
		anim(OAM1);
	}

	void port_UpdateState_FenceMovingHorizontal()
	{
		FARCALL(Func_19afb);
		if (!ld_and(wIsNearFence)) {
			FARCALL(StartFall);
			return;
		}
		sfx_loop(0x20, SFX_CLIMB);
		anim(OAM1);
		port_Func_1b24f();
	}

	void port_SetState_FenceSliding()
	{
		set_state_sliding(WST_FENCE_SLIDING);
	}

	void port_UpdateState_FenceSliding()
	{
		update_sliding(true);
	}

	// The jump input check at the start of Func_1b168, Func_1b21a and
	// Func_1b24f. Returns true if it jumped (and returned).
	bool fence_jump_input()
	{
		if (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A))
			goto jump;
		if (ld_bit(wJoypadPressed, B_PAD_A))
			goto jump;
		return false;
	jump:
		FARCALL(StartJump_FromInput);
		return true;
	}

	void port_Func_1b168()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_B | PAD_DOWN);
		alu_.cp(cpu_.a, PAD_B | PAD_DOWN);
		if (ZF) {
			port_SetState_FenceSliding();
			return;
		}
		if (fence_jump_input())
			return;

		if (ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(Func_19ae4);
			if (!ld_and(wIsNearFence)) {
				return;
			}
			cpu_.b = 0x01;
			CALL(MoveWarioUp);
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			home::move_wario_y(gb_, 0x01);
			FARCALL(CheckCentreCollision);
			if (b_and()) {
				update_pos_y();
				FARCALL(SetState_Idling);
				return;
			}
			FARCALL(Func_19afb);
			if (ld_and(wIsNearFence)) {
				return;
			}
			FARCALL(StartFall);
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			port_SetState_FenceMovingHorizontal();
		else
			port_SetState_FenceIdling();
	}

	void port_Func_1b21a()
	{
		if (fence_jump_input())
			return;
		if (ld_mask(wJoypadDown, PAD_UP | PAD_DOWN)) {
			port_SetState_FenceMovingVertical();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_FenceMovingHorizontal();
			return;
		}
	}

	void port_Func_1b24f()
	{
		if (fence_jump_input())
			return;
		bool right;
		if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else {
			cpu_.a = alu_.and_(cpu_.a, PAD_UP | PAD_DOWN);
			if (cpu_.a)
				port_SetState_FenceMovingVertical();
			else
				port_SetState_FenceIdling();
			return;
		}
		wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			return;
		}
		cpu_.b = 0x01;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	}

	void port_Func_1b2c0()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		port_Func_1b302();
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			return;
		}
		FARCALL(Func_2b17a);
		clamp_walk_vel();
		if (ld_and(wIsTurning))
			port_Func_1a14b();
	}

	void port_Func_1b302()
	{
		FARCALL(CheckCentreCollision);
		if (b_and())
			return;
		cpu_.b = 0x01;
		if ((rd8(wJoypadDown) >> B_PAD_DOWN & 1))
			cpu_.b = alu_.inc(cpu_.b);
		CALL(MoveWarioDown);
	}

	void port_Func_1b322()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_SandWalking();
			return;
		}
	}

	void port_Func_1b333()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		bool right;
		if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else {
			port_SetState_SandIdling();
			return;
		}
		// (buggy: wDirection was already switched by the sand idling input
		// handling, so Sand Turning is never reached)
		if (ld_and(wDirection) != right) {
			port_SetState_SandTurning();
			return;
		}
		wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			return;
		}
		if (right) {
			CALL(ApplyWalkVelocity_Right);
			CALL(MoveWarioRight);
		} else {
			CALL(ApplyWalkVelocity_Left);
			CALL(MoveWarioLeft);
		}
		clamp_walk_vel();
	}

	void port_HandleLadderInput()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_B | PAD_DOWN);
		alu_.cp(cpu_.a, PAD_B | PAD_DOWN);
		if (ZF) {
			port_SetState_LadderSliding();
			return;
		}
		if (ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(CheckUpCollision);
			if (ld_and(wJumpCollision)) {
				return;
			}
			cpu_.b = 1;
			CALL(MoveWarioUp);
			cpu_.a = alu_.and_(rd8(wCameraConfigFlags), CAM_SCROLLING_MASK);
			alu_.cp(cpu_.a, CAM_TRANSITIONS);
			if (!CF) {
				CALL(GetFloorForYPos);
				cpu_.a = alu_.sub(rd8(wFloor), cpu_.c);
				if (CF) {
					CALL(StartUpwardsFloorTransition);
					return;
				}
			}
			if (ld_and(wLadderInteraction)) {
				return;
			}
			FARCALL(Func_19b12);
			if (ld_and(wLadderInteraction)) {
				return;
			}
			goto idle;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			cpu_.b = 0x01;
			CALL(MoveWarioDown);
			if (down_floor_check()) {
				CALL(StartDownwardsFloorTransition);
				return;
			}
			FARCALL(Func_19b12);
			if (rd8(wLadderInteraction)) {
				FARCALL(CheckCentreCollision);
				if (b_and())
					center_on_ladder();
				return;
			}
			// .no_more_ladder_interaction
			FARCALL(CheckCentreCollision);
			if (b_and())
				goto idle;
			FARCALL(StartFall);
			return;
		}
		port_SetState_LadderIdling();
		return;

	idle:
		FARCALL(SetState_Idling);
	}

	void port_Func_1b480()
	{
		if (ld_bit(wJoypadDown, B_PAD_B) && ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			// .asm_1b4a7
			bool ok;
			if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
				wr8(wCollisionBoxRight, 16);
				FARCALL(Func_19741);
				ok = !b_and();
			} else {
				wr8(wCollisionBoxLeft, (uint8_t)-16);
				FARCALL(Func_197b1);
				ok = !b_and();
			}
			wr8(wCollisionBoxLeft, (uint8_t)-9);
			st(wCollisionBoxRight, 9);
			if (ok)
				port_SetState_GettingOffLadder();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_UP | PAD_DOWN)) {
			port_SetState_LadderClimbing();
			return;
		}
		cpu_.a = alu_.add(rd8(wWarioStateCounter), 1);
		wr8(wWarioStateCounter, cpu_.a);
		cpu_.a = alu_.adc(rd8(wWarioStateCounter + 1), 0);
		wr8(wWarioStateCounter + 1, cpu_.a);
		SET_HL(gb_, wWarioStateCounter + 1);
		alu_.cp(cpu_.a, 0x02);
		if (CF) {
			return;
		}
		port_SetState_LadderScratching();
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates2;

PORT_EX(UpdateWarioStates_Group2, 0, ALL_REGISTERS) { WarioStates2(gb).port_UpdateWarioStates_Group2(); }
PORT_EX(SetState_Slipping, 0, 0) { WarioStates2(gb).port_SetState_Slipping(); }
PORT_COVERED(UpdateState_Slipping);
PORT_EX(SetState_CrouchSlipping, 0, 0) { WarioStates2(gb).port_SetState_CrouchSlipping(); }
PORT_COVERED(Func_19e89);
PORT_COVERED(UpdateState_CrouchSlipping);
PORT_EX(SetState_DraggedDown, 0, 0) { WarioStates2(gb).port_SetState_DraggedDown(); }
PORT_COVERED(UpdateState_DraggedDown);
PORT_COVERED(UpdateState_Teleporting);
PORT_COVERED(PlayTeleportingSFX);
PORT_COVERED(UpdateState_WaterTeleporting);
PORT_EX(SetState_SandFalling, 0, 0) { WarioStates2(gb).port_SetState_SandFalling(); }
PORT_COVERED(Func_1a12a);
PORT_COVERED(Func_1a14b);
PORT_COVERED(UpdateState_SandFalling);
PORT_COVERED(SetState_SandJumping);
PORT_COVERED(UpdateState_SandJumping);
PORT_COVERED(SetState_SandIdling);
PORT_COVERED(UpdateState_SandIdling);
PORT_COVERED(SetState_SandWalking);
PORT_COVERED(UpdateState_SandWalking);
PORT_COVERED(SetState_SandTurning);
PORT_COVERED(UpdateState_SandTurning);
PORT_EX(SetState_LadderClimbing, 0, 0) { WarioStates2(gb).port_SetState_LadderClimbing(); }
PORT_COVERED(UpdateState_LadderClimbing);
PORT_COVERED(SetState_LadderIdling);
PORT_COVERED(UpdateState_LadderIdling);
PORT_COVERED(UpdateState_LadderShakeSlipping);
PORT_COVERED(SetState_GettingOffLadder);
PORT_COVERED(UpdateState_GettingOffLadder);
PORT_COVERED(SetState_LadderSliding);
PORT_COVERED(UpdateState_LadderSliding);
PORT_EX(SetState_GrabSlipping, 0, 0) { WarioStates2(gb).port_SetState_GrabSlipping(); }
PORT_COVERED(UpdateState_GrabSlipping);
PORT_COVERED(UpdateState_GetTreasureStart);
PORT_COVERED(SetNextGetTreasureState);
PORT_COVERED(UpdateState_GetTreasureWalk);
PORT_COVERED(UpdateState_GetTreasureTurnFront);
PORT_COVERED(UpdateState_GetTreasureLaugh);
PORT_COVERED(UpdateState_GetTreasureTurnBack);
PORT_COVERED(UpdateState_GetTreasureOpen);
PORT_COVERED(UpdateState_GetTreasureTurnWalk);
PORT_COVERED(SetState_GetTreasureLookFront);
PORT_COVERED(UpdateState_GetTreasureLookFront);
PORT_COVERED(UpdateState_GetTreasureClear);
// its callers (all in the Wario states and interactions) read no a, f or b after it
PORT_EX(SetState_Sleeping, 0, R_A | R_F | R_B) { WarioStates2(gb).port_SetState_Sleeping(); }
PORT_COVERED(UpdateState_Sleeping);
PORT_COVERED(SetState_LadderScratching);
PORT_COVERED(UpdateState_LadderScratching);
PORT_EX(SetState_FenceShakeSliding, 0, 0) { WarioStates2(gb).port_SetState_FenceShakeSliding(); }
PORT_COVERED(UpdateState_FenceShakeSliding);
PORT_EX(SetState_FenceMovingVertical, 0, 0) { WarioStates2(gb).port_SetState_FenceMovingVertical(); }
PORT_COVERED(UpdateState_FenceMovingVertical);
PORT_COVERED(SetState_FenceIdling);
PORT_COVERED(UpdateState_FenceIdling);
PORT_COVERED(SetState_FenceMovingHorizontal);
PORT_COVERED(UpdateState_FenceMovingHorizontal);
PORT_COVERED(SetState_FenceSliding);
PORT_COVERED(UpdateState_FenceSliding);
PORT_COVERED(Func_1b168);
PORT_COVERED(Func_1b21a);
PORT_COVERED(Func_1b24f);
PORT_COVERED(Func_1b2c0);
PORT_COVERED(Func_1b302);
PORT_COVERED(Func_1b322);
PORT_COVERED(Func_1b333);
PORT_COVERED(HandleLadderInput);
PORT_COVERED(Func_1b480);
