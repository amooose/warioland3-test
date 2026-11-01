// Wario's states, part 2 (src/engine/level/wario_states_2.asm): the state
// handlers and their SetState_ routines, as the asm runs them.
//
// Register-level C and the level's frame call these, so they keep the
// original's registers, flags and return addresses (see asm_module.hpp).
// Converted with tools/c2cpp.py, then fuzzed hook by hook.
#include "asm_module.hpp"

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
	void port_UpdateWarioStates_Group2()
	{
		cpu_.a = alu_.sub(rd8(wWarioState), 0x30);
		// rst JumpTable: pushes the address of the table after the rst
		gb_push16(gb_, ADDR_UpdateWarioStates_Group2 + 6);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		uint16_t hl = gb_pop16(gb_);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		hl = alu_.add16(hl, REG_DE(gb_));
		cpu_.a = rd8(hl);
		hl = (uint16_t)(rd8(hl + 1) << 8 | cpu_.a);
		SET_HL(gb_, hl);
		JUMP_TO(hl);
	}

	void port_SetState_Slipping()
	{
		st(wWarioState, WST_SLIPPING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wAttackCounter, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		xor_a();
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
			JUMP(TriggerRoomTransition);
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
			RET();
			return;
		}
		if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
			LJUMP(Func_19e89);
			return;
		}

		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_SLIPPING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and()) {
			FARCALL(StartFall);
			RET();
			return;
		}

		update_pos_y();
		FARCALL(CheckFrontCollision);
		ld_cp(wWarioState, WST_SLIPPING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and())
			goto stop;
		CALL(IsOnSlipperyGround);
		if (ZF)
			goto stop;
		if (slip_move(0x08, 0x0f, 0x15, false))
			goto stop;
		RET();
		return;

	stop:
		cpu_.a = alu_.and_(rd8(wWarioStateCycles), 0x01);
		wr8(wDirection, cpu_.a);
		FARCALL(SetState_Idling);
		RET();
	}

	void port_SetState_CrouchSlipping()
	{
		xor_a();
		wr8(wWarioStateCounter, 0);
		ld(wDirection);
		wr8(wWarioStateCycles, cpu_.a);
		LJUMP(Func_19e89);
	}

	void port_Func_19e89()
	{
		st(wWarioState, WST_CROUCH_SLIPPING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wAttackCounter, 0);
		wr8(wGrabState, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wIsCrouching, cpu_.a);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		xor_a();
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
			JUMP(TriggerRoomTransition);
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(StartCrouchJump);
			RET();
			return;
		}

		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_CROUCH_SLIPPING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and()) {
			FARCALL(StartCrouchFall);
			RET();
			return;
		}

		update_pos_y();
		FARCALL(CheckFrontCollision);
		ld_cp(wWarioState, WST_CROUCH_SLIPPING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and())
			goto stop;
		CALL(IsOnSlipperyGround);
		if (ZF)
			goto stop;
		if (slip_move(0x06, 0x0a, 0x0e, true))
			goto stop;
		RET();
		return;

	stop:
		cpu_.a = alu_.and_(rd8(wWarioStateCycles), 0x01);
		wr8(wDirection, cpu_.a);
		FARCALL(CrouchOrSlide);
		RET();
	}

	void port_SetState_DraggedDown()
	{
		st(wWarioState, WST_DRAGGED_DOWN);
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
		if (ld_and(wAutoMoveState))
			return;
		ld_cp(wLevel, THE_TEMPLE);
		if (ZF) {
			st(wLevelEndScreen, LVLEND_GAME_OVER);
			return;
		}
		FARCALL(StartFall);
	}

	void teleporting(uint8_t sfx_frame, uint8_t next_state)
	{
		st(wIsIntangible, TRUE);
		anim(OAM1);
		uint8_t v = inc_counter();
		alu_.cp(v, sfx_frame);
		if (ZF) {
			// jr z, PlayTeleportingSFX
			play_sfx(SFX_082);
			RET();
			return;
		}
		alu_.cp(v, 0xb4);
		if (CF) {
			RET();
			return;
		}
		wr8(wWarioStateCounter, 0x00);
		st(wWarioState, next_state);
		st(wInvincibleCounter, 0x01);
		SET_HL(gb_, wRoomTransitionParam);
		wr8(wRoomTransitionParam,
			rd8(wRoomTransitionParam) & (uint8_t)~(1 << ROOMTRANSITIONF_DELAY_F));
		JUMP(TriggerRoomTransition);
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
		st(wGrabState, 0);
		st(wWarioState, WST_SAND_FALLING);
		st(wCollisionBoxBottom, (uint8_t)-1);
		st(wCollisionBoxTop, (uint8_t)-27);
		st(wCollisionBoxLeft, (uint8_t)-9);
		st(wCollisionBoxRight, 9);

		xor_a();
		st(wJumpVelIndex, 0);
		st(wJumpVelTable, 0);
		st(wWarioStateCounter, 0);
		st(wWarioStateCycles, 0);
		st(wIsSmashAttacking, 0);
		st(wAttackCounter, 0);
		st(wIsRolling, 0);
		st(wSwimVelIndex, 0);
		st(wIsCrouching, 0);
		st(wc0e0, 0);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		LCALL(SetWarioPal); // waits for HBlank
		LJUMP(Func_1a12a); // fallthrough
	}

	void port_Func_1a12a()
	{
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		LJUMP(Func_1a14b); // fallthrough
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
		LCALL(Func_1b2c0);
		ld_cp(wWarioState, WST_SAND_FALLING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			RET();
			return;
		}
		update_pos_y();
		LJUMP(SetState_SandIdling);
	}

	void port_SetState_SandJumping()
	{
		play_sfx(SFX_JUMP);
		st(wWarioState, WST_SAND_JUMPING);
		xor_a();
		wr8(wJumpVelIndex, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		LJUMP(Func_1a12a);
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
			RET();
			return;
		}
		CALL(ApplyJumpVelocity);
		FARCALL(Func_2b17a);
		clamp_walk_vel();
		if (ld_and(wIsTurning))
			LCALL(Func_1a14b);

		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			LJUMP(SetState_SandFalling);
			return;
		}
		ld_cp(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		if (!CF) {
			LJUMP(SetState_SandFalling);
			return;
		}
		FARCALL(CheckUpCollision);
		if (!b_and()) {
			RET();
			return;
		}
		LJUMP(SetState_SandFalling);
	}

	void port_SetState_SandIdling()
	{
		st(wWarioState, WST_SAND_IDLING);
		xor_a();
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
		LCALL(Func_1b322);
	}

	void port_SetState_SandWalking()
	{
		xor_a();
		wr8(wWalkVelIndex, 0);
		st(wWarioState, WST_SAND_WALKING);
		if (ld_bit(wJoypadDown, B_PAD_RIGHT))
			st(wDirection, DIRECTION_RIGHT);
		else if (alu_.bit(cpu_.a, B_PAD_LEFT))
			st(wDirection, DIRECTION_LEFT);
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
		LCALL(Func_1b333);
		ld_cp(wWarioState, WST_SAND_WALKING);
		if (!ZF) {
			RET();
			return;
		}
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		FARCALL(CheckCentreCollision);
		if (!b_and()) {
			LJUMP(SetState_SandFalling);
			return;
		}
		RET();
	}

	void port_SetState_SandTurning()
	{
		st(wWarioState, WST_SAND_TURNING);
		xor_a();
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
			LJUMP(SetState_SandJumping);
			return;
		}
		anim(OAM1);
		if (!anim_ended()) {
			RET();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			LJUMP(SetState_SandWalking);
		else
			LJUMP(SetState_SandIdling);
	}

	void port_SetState_LadderClimbing()
	{
		st(wWarioState, WST_LADDER_CLIMBING);
		center_on_ladder();
		xor_a();
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
			JUMP(TriggerRoomTransition);
			return;
		}
		sfx_loop(32, SFX_CLIMB);
		anim(OAM1);
		if (anim_ended()) {
			get_pos();
			cpu_.b = PARTICLE_DUST;
			FARCALL(CreateParticle);
		}
		LCALL(HandleLadderInput);
		RET();
	}

	void port_SetState_LadderIdling()
	{
		st(wWarioState, WST_LADDER_IDLING);
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
			JUMP(TriggerRoomTransition);
			return;
		}
		if (!ld_and(wLadderInteraction)) {
			FARCALL(StartFall);
			RET();
			return;
		}
		anim(OAM1);
		LCALL(Func_1b480);
		RET();
	}

	void port_UpdateState_LadderShakeSlipping()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			JUMP(TriggerRoomTransition);
			return;
		}
		if (!ld_and(wLadderInteraction)) {
			FARCALL(StartFall);
			RET();
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
			RET();
			return;
		}
		uint8_t v = inc_counter();
		alu_.cp(v, 0x40);
		if (CF) {
			RET();
			return;
		}
		wr8(wWarioStateCounter, 0x00);
		LJUMP(SetState_LadderIdling);
	}

	void port_SetState_GettingOffLadder()
	{
		st(wWarioState, WST_GETTING_OFF_LADDER);
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
		if (ld_and(wLadderInteraction))
			return;
		xor_a();
		wr8(wIsGettingOffLadder, 0);
		wr8(wIsIntangible, 0);
		FARCALL(StartFall);
	}

	void set_state_sliding(uint8_t state)
	{
		st(wWarioState, state);
		play_sfx(SFX_SLIDE);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsGettingOffLadder, 0);
		cpu_.a = alu_.inc(cpu_.a);
		wr8(wJumpVelTable, cpu_.a);
		ld_cp(wPowerUpLevel, LEAD_OVERALLS);
		if (!CF)
			st(wIsSmashAttacking, TRUE);
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
			JUMP(TriggerRoomTransition);
			return;
		}
		if (!ld_and(fence ? wIsNearFence : wLadderInteraction)) {
			xor_a();
			wr8(wIsSmashAttacking, FALSE);
			FARCALL(StartFall);
			RET();
			return;
		}
		anim(OAM1);
		if (ld_bit(wJoypadDown, B_PAD_UP)) {
			if (fence)
				LJUMP(SetState_FenceMovingVertical);
			else
				LJUMP(SetState_LadderClimbing);
			return;
		}
		cpu_.b = 0x02;
		CALL(MoveWarioDown);
		if (down_floor_check())
			CALL(StartDownwardsFloorTransition);
		FARCALL(CheckAirborneCollision);
		if (!b_and()) {
			RET();
			return;
		}
		update_pos_y();
		if (!ld_and(wIsSmashAttacking)) {
			FARCALL(SetState_Idling);
			RET();
			return;
		}
		ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (CF)
			FARCALL(DoGroundPound);
		else
			FARCALL(DoWarioGroundShake);
		RET();
	}

	void port_UpdateState_LadderSliding()
	{
		update_sliding(false);
	}

	void port_SetState_GrabSlipping()
	{
		st(wWarioState, WST_GRAB_SLIPPING);
		xor_a();
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
			JUMP(TriggerRoomTransition);
			return;
		}
		if (!ld_and(wGrabState)) {
			FARCALL(SetState_Idling);
			RET();
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(Func_1ede9);
			RET();
			return;
		}
		FARCALL(CheckCentreCollision);
		ld_cp(wWarioState, WST_GRAB_SLIPPING);
		if (!ZF) {
			RET();
			return;
		}
		if (!b_and()) {
			FARCALL(Func_1edd3);
			RET();
			return;
		}
		FARCALL(CheckFrontCollision);
		ld_cp(wWarioState, WST_GRAB_SLIPPING);
		if (!ZF) {
			RET();
			return;
		}
		if (b_and())
			goto stop;
		CALL(IsOnSlipperyGround);
		if (ZF)
			goto stop;
		if (slip_move(0x08, 0x0f, 0x15, false))
			goto stop;
		RET();
		return;

	stop:
		FARCALL(SetState_GrabIdling);
		RET();
	}

	void port_UpdateState_GetTreasureStart()
	{
		anim(OAM1);
		if (ld_and(wJumpVelTable)) {
			CALL(ApplyJumpVelocity);
			FARCALL(CheckAirborneCollision);
			if (!b_and()) {
				RET();
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
			RET();
			return;
		}
		st(wWarioStateCounter, 24);
		LJUMP(SetNextGetTreasureState); // fallthrough
	}

	void port_SetNextGetTreasureState()
	{
		SET_HL(gb_, wWarioState);
		wr8(wWarioState, alu_.inc(rd8(wWarioState)));
		xor_a();
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
		xor_a();
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
			RET();
			return;
		}
		ld_cp(wWarioState, WST_GET_TREASURE_TURN_FRONT_2);
		if (ZF) {
			LJUMP(SetState_GetTreasureLookFront);
			return;
		}
		st(wWarioState, WST_GET_TREASURE_LAUGH);
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioClearGfx, ADDR_WarioClearGfx);
		set_oam(BANK_OAM_1fc000, ADDR_OAM_1fc000);
		set_frameset(ADDR_Frameset_1fc2ab);
		anim(OAM2);
		RET();
	}

	void port_UpdateState_GetTreasureLaugh()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		st(wWarioState, WST_GET_TREASURE_TURN_BACK);
		xor_a();
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
		st(wWarioState, WST_GET_TREASURE_OPEN);
		play_sfx(SFX_03E);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioClearGfx, ADDR_WarioClearGfx);
		set_oam(BANK_OAM_1fc000, ADDR_OAM_1fc000);
		xor_a();
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
		xor_a();
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
			RET();
			return;
		}
		st(wWarioStateCounter, 32);
		xor_a();
		wr8(wAutoMoveState, 0);
		LJUMP(SetNextGetTreasureState);
	}

	void port_SetState_GetTreasureLookFront()
	{
		st(wWarioState, WST_GET_TREASURE_LOOK_FRONT);
		xor_a();
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
		st(wWarioState, WST_GET_TREASURE_CLEAR);
		get_pos();
		cpu_.b = PARTICLE_TREASURE_GLOW;
		FARCALL(CreateParticle);
		xor_a();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
	}

	void port_UpdateState_GetTreasureClear()
	{
		SET_HL(gb_, wWarioStateCounter);
		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			cpu_.a = alu_.inc(rd8(wWarioStateCounter));
			wr8(wWarioStateCounter, cpu_.a);
			alu_.cp(cpu_.a, 180);
			if (CF)
				return;
		}
		if (!ld_and(wCurParticleAnimEnded))
			return;
		wr8(wWarioStateCounter, 0);
		SET_HL(gb_, wLevelEndScreen);
		wr8(wLevelEndScreen, rd8(wLevelEndScreen) & 0x7f);
	}

	void port_SetState_Sleeping()
	{
		st(wWarioState, WST_SLEEPING);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		xor_a();
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
			if (!ld_and(wJoypadDown))
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
		xor_a();
		wr8(wFrameDuration, 0);
		wr8(wAnimationFrame, 0);
		inc_counter();
		frameset_by_dir(ADDR_Frameset_1422e, ADDR_Frameset_14237);
		anim(OAM1);
	}

	void port_SetState_LadderScratching()
	{
		st(wWarioState, WST_LADDER_SCRATCHING);
		coll_box_btlr();
		xor_a();
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
				RET();
				return;
			}
		}
		LJUMP(SetState_LadderIdling);
	}

	void port_SetState_FenceShakeSliding()
	{
		st(wWarioState, WST_FENCE_SHAKE_SLIDING);
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
		xor_a();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	void port_SetState_FenceMovingVertical()
	{
		st(wWarioState, WST_FENCE_MOVING_VERTICAL);
		xor_a();
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
		LCALL(Func_1b168);
	}

	void port_SetState_FenceIdling()
	{
		st(wWarioState, WST_FENCE_IDLING);
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
		LCALL(Func_1b21a);
	}

	void port_SetState_FenceMovingHorizontal()
	{
		st(wWarioState, WST_FENCE_MOVING_HORIZONTAL);
		xor_a();
		wr8(wSFXLoopCounter, 0);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		wr8(wIsSmashAttacking, 0);
		coll_box_blrt();
		xor_a();
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
		LCALL(Func_1b24f);
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
		RET();
		return true;
	}

	void port_Func_1b168()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_B | PAD_DOWN);
		alu_.cp(cpu_.a, PAD_B | PAD_DOWN);
		if (ZF) {
			LJUMP(SetState_FenceSliding);
			return;
		}
		if (fence_jump_input())
			return;

		if (ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(Func_19ae4);
			if (!ld_and(wIsNearFence)) {
				RET();
				return;
			}
			cpu_.b = 0x01;
			CALL(MoveWarioUp);
			RET();
			return;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			cpu_.b = 0x01;
			CALL(MoveWarioDown);
			FARCALL(CheckCentreCollision);
			if (b_and()) {
				update_pos_y();
				FARCALL(SetState_Idling);
				RET();
				return;
			}
			FARCALL(Func_19afb);
			if (ld_and(wIsNearFence)) {
				RET();
				return;
			}
			FARCALL(StartFall);
			RET();
			return;
		}
		cpu_.a = alu_.and_(cpu_.a, PAD_RIGHT | PAD_LEFT);
		if (cpu_.a)
			LJUMP(SetState_FenceMovingHorizontal);
		else
			LJUMP(SetState_FenceIdling);
	}

	void port_Func_1b21a()
	{
		if (fence_jump_input())
			return;
		if (ld_mask(wJoypadDown, PAD_UP | PAD_DOWN)) {
			LJUMP(SetState_FenceMovingVertical);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_FenceMovingHorizontal);
			return;
		}
		RET();
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
				LJUMP(SetState_FenceMovingVertical);
			else
				LJUMP(SetState_FenceIdling);
			return;
		}
		st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			RET();
			return;
		}
		cpu_.b = 0x01;
		if (right)
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
		RET();
	}

	void port_Func_1b2c0()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_SandJumping);
			return;
		}
		LCALL(Func_1b302);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			RET();
			return;
		}
		FARCALL(Func_2b17a);
		clamp_walk_vel();
		if (ld_and(wIsTurning))
			LCALL(Func_1a14b);
		RET();
	}

	void port_Func_1b302()
	{
		FARCALL(CheckCentreCollision);
		if (b_and())
			return;
		cpu_.b = 0x01;
		if (ld_bit(wJoypadDown, B_PAD_DOWN))
			cpu_.b = alu_.inc(cpu_.b);
		CALL(MoveWarioDown);
	}

	void port_Func_1b322()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_SandJumping);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			LJUMP(SetState_SandWalking);
			return;
		}
		RET();
	}

	void port_Func_1b333()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			LJUMP(SetState_SandJumping);
			return;
		}
		bool right;
		if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
			right = true;
		} else if (alu_.bit(cpu_.a, B_PAD_LEFT)) {
			right = false;
		} else {
			LJUMP(SetState_SandIdling);
			return;
		}
		// (buggy: wDirection was already switched by the sand idling input
		// handling, so Sand Turning is never reached)
		if (ld_and(wDirection) != right) {
			LJUMP(SetState_SandTurning);
			return;
		}
		st(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		FARCALL(CheckFrontCollision);
		if (b_and()) {
			RET();
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
		RET();
	}

	void port_HandleLadderInput()
	{
		cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_B | PAD_DOWN);
		alu_.cp(cpu_.a, PAD_B | PAD_DOWN);
		if (ZF) {
			LJUMP(SetState_LadderSliding);
			return;
		}
		if (ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(CheckUpCollision);
			if (ld_and(wJumpCollision)) {
				RET();
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
					JUMP(StartUpwardsFloorTransition);
					return;
				}
			}
			if (ld_and(wLadderInteraction)) {
				RET();
				return;
			}
			FARCALL(Func_19b12);
			if (ld_and(wLadderInteraction)) {
				RET();
				return;
			}
			goto idle;
		}
		if (alu_.bit(cpu_.a, B_PAD_DOWN)) {
			cpu_.b = 0x01;
			CALL(MoveWarioDown);
			if (down_floor_check()) {
				JUMP(StartDownwardsFloorTransition);
				return;
			}
			FARCALL(Func_19b12);
			if (ld_and(wLadderInteraction)) {
				FARCALL(CheckCentreCollision);
				if (b_and())
					center_on_ladder();
				RET();
				return;
			}
			// .no_more_ladder_interaction
			FARCALL(CheckCentreCollision);
			if (b_and())
				goto idle;
			FARCALL(StartFall);
			RET();
			return;
		}
		LJUMP(SetState_LadderIdling);
		return;

	idle:
		FARCALL(SetState_Idling);
		RET();
	}

	void port_Func_1b480()
	{
		if (ld_bit(wJoypadDown, B_PAD_B) && ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			// .asm_1b4a7
			bool ok;
			if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
				st(wCollisionBoxRight, 16);
				FARCALL(Func_19741);
				ok = !b_and();
			} else {
				st(wCollisionBoxLeft, (uint8_t)-16);
				FARCALL(Func_197b1);
				ok = !b_and();
			}
			wr8(wCollisionBoxLeft, (uint8_t)-9);
			st(wCollisionBoxRight, 9);
			if (ok)
				LJUMP(SetState_GettingOffLadder);
			else
				RET();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_UP | PAD_DOWN)) {
			LJUMP(SetState_LadderClimbing);
			return;
		}
		cpu_.a = alu_.add(rd8(wWarioStateCounter), 1);
		wr8(wWarioStateCounter, cpu_.a);
		cpu_.a = alu_.adc(rd8(wWarioStateCounter + 1), 0);
		wr8(wWarioStateCounter + 1, cpu_.a);
		SET_HL(gb_, wWarioStateCounter + 1);
		alu_.cp(cpu_.a, 0x02);
		if (CF) {
			RET();
			return;
		}
		LJUMP(SetState_LadderScratching);
	}
};

} // namespace

} // namespace wl3::level

using wl3::level::WarioStates2;

PORT_EX(UpdateWarioStates_Group2, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateWarioStates_Group2(); }
PORT_EX(SetState_Slipping, 0, 0) { WarioStates2(gb).port_SetState_Slipping(); }
PORT_EX(UpdateState_Slipping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_Slipping(); }
PORT_EX(SetState_CrouchSlipping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_SetState_CrouchSlipping(); }
PORT_EX(Func_19e89, 0, 0) { WarioStates2(gb).port_Func_19e89(); }
PORT_EX(UpdateState_CrouchSlipping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_CrouchSlipping(); }
PORT_EX(SetState_DraggedDown, 0, 0) { WarioStates2(gb).port_SetState_DraggedDown(); }
PORT_EX(UpdateState_DraggedDown, 0, 0) { WarioStates2(gb).port_UpdateState_DraggedDown(); }
PORT_EX(UpdateState_Teleporting, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_Teleporting(); }
PORT_EX(PlayTeleportingSFX, 0, 0) { WarioStates2(gb).port_PlayTeleportingSFX(); }
PORT_EX(UpdateState_WaterTeleporting, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_WaterTeleporting(); }
PORT_EX(SetState_SandFalling, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_SetState_SandFalling(); }
PORT_EX(Func_1a12a, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1a12a(); }
PORT_EX(Func_1a14b, 0, 0) { WarioStates2(gb).port_Func_1a14b(); }
PORT_EX(UpdateState_SandFalling, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_SandFalling(); }
PORT_EX(SetState_SandJumping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_SetState_SandJumping(); }
PORT_EX(UpdateState_SandJumping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_SandJumping(); }
PORT_EX(SetState_SandIdling, 0, 0) { WarioStates2(gb).port_SetState_SandIdling(); }
PORT_EX(UpdateState_SandIdling, 0, 0) { WarioStates2(gb).port_UpdateState_SandIdling(); }
PORT_EX(SetState_SandWalking, 0, 0) { WarioStates2(gb).port_SetState_SandWalking(); }
PORT_EX(UpdateState_SandWalking, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_SandWalking(); }
PORT_EX(SetState_SandTurning, 0, 0) { WarioStates2(gb).port_SetState_SandTurning(); }
PORT_EX(UpdateState_SandTurning, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_SandTurning(); }
PORT_EX(SetState_LadderClimbing, 0, 0) { WarioStates2(gb).port_SetState_LadderClimbing(); }
PORT_EX(UpdateState_LadderClimbing, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_LadderClimbing(); }
PORT_EX(SetState_LadderIdling, 0, 0) { WarioStates2(gb).port_SetState_LadderIdling(); }
PORT_EX(UpdateState_LadderIdling, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_LadderIdling(); }
PORT_EX(UpdateState_LadderShakeSlipping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_LadderShakeSlipping(); }
PORT_EX(SetState_GettingOffLadder, 0, 0) { WarioStates2(gb).port_SetState_GettingOffLadder(); }
PORT_EX(UpdateState_GettingOffLadder, 0, 0) { WarioStates2(gb).port_UpdateState_GettingOffLadder(); }
PORT_EX(SetState_LadderSliding, 0, 0) { WarioStates2(gb).port_SetState_LadderSliding(); }
PORT_EX(UpdateState_LadderSliding, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_LadderSliding(); }
PORT_EX(SetState_GrabSlipping, 0, 0) { WarioStates2(gb).port_SetState_GrabSlipping(); }
PORT_EX(UpdateState_GrabSlipping, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_GrabSlipping(); }
PORT_EX(UpdateState_GetTreasureStart, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_GetTreasureStart(); }
PORT_EX(SetNextGetTreasureState, 0, 0) { WarioStates2(gb).port_SetNextGetTreasureState(); }
PORT_EX(UpdateState_GetTreasureWalk, 0, 0) { WarioStates2(gb).port_UpdateState_GetTreasureWalk(); }
PORT_EX(UpdateState_GetTreasureTurnFront, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_GetTreasureTurnFront(); }
PORT_EX(UpdateState_GetTreasureLaugh, 0, 0) { WarioStates2(gb).port_UpdateState_GetTreasureLaugh(); }
PORT_EX(UpdateState_GetTreasureTurnBack, 0, 0) { WarioStates2(gb).port_UpdateState_GetTreasureTurnBack(); }
PORT_EX(UpdateState_GetTreasureOpen, 0, 0) { WarioStates2(gb).port_UpdateState_GetTreasureOpen(); }
PORT_EX(UpdateState_GetTreasureTurnWalk, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_GetTreasureTurnWalk(); }
PORT_EX(SetState_GetTreasureLookFront, 0, 0) { WarioStates2(gb).port_SetState_GetTreasureLookFront(); }
PORT_EX(UpdateState_GetTreasureLookFront, 0, 0) { WarioStates2(gb).port_UpdateState_GetTreasureLookFront(); }
PORT_EX(UpdateState_GetTreasureClear, 0, 0) { WarioStates2(gb).port_UpdateState_GetTreasureClear(); }
PORT_EX(SetState_Sleeping, 0, 0) { WarioStates2(gb).port_SetState_Sleeping(); }
PORT_EX(UpdateState_Sleeping, 0, 0) { WarioStates2(gb).port_UpdateState_Sleeping(); }
PORT_EX(SetState_LadderScratching, 0, 0) { WarioStates2(gb).port_SetState_LadderScratching(); }
PORT_EX(UpdateState_LadderScratching, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_LadderScratching(); }
PORT_EX(SetState_FenceShakeSliding, 0, 0) { WarioStates2(gb).port_SetState_FenceShakeSliding(); }
PORT_EX(UpdateState_FenceShakeSliding, 0, 0) { WarioStates2(gb).port_UpdateState_FenceShakeSliding(); }
PORT_EX(SetState_FenceMovingVertical, 0, 0) { WarioStates2(gb).port_SetState_FenceMovingVertical(); }
PORT_EX(UpdateState_FenceMovingVertical, 0, 0) { WarioStates2(gb).port_UpdateState_FenceMovingVertical(); }
PORT_EX(SetState_FenceIdling, 0, 0) { WarioStates2(gb).port_SetState_FenceIdling(); }
PORT_EX(UpdateState_FenceIdling, 0, 0) { WarioStates2(gb).port_UpdateState_FenceIdling(); }
PORT_EX(SetState_FenceMovingHorizontal, 0, 0) { WarioStates2(gb).port_SetState_FenceMovingHorizontal(); }
PORT_EX(UpdateState_FenceMovingHorizontal, 0, 0) { WarioStates2(gb).port_UpdateState_FenceMovingHorizontal(); }
PORT_EX(SetState_FenceSliding, 0, 0) { WarioStates2(gb).port_SetState_FenceSliding(); }
PORT_EX(UpdateState_FenceSliding, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_UpdateState_FenceSliding(); }
PORT_EX(Func_1b168, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b168(); }
PORT_EX(Func_1b21a, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b21a(); }
PORT_EX(Func_1b24f, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b24f(); }
PORT_EX(Func_1b2c0, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b2c0(); }
PORT_EX(Func_1b302, 0, 0) { WarioStates2(gb).port_Func_1b302(); }
PORT_EX(Func_1b322, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b322(); }
PORT_EX(Func_1b333, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b333(); }
PORT_EX(HandleLadderInput, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_HandleLadderInput(); }
PORT_EX(Func_1b480, HOOK_MANUAL_RET, 0) { WarioStates2(gb).port_Func_1b480(); }
