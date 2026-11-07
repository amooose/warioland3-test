// Wario's states, part 2 (src/engine/level/wario_states_2.asm): slipping,
// sand, ladders, fences, the treasure's pickup and sleeping, as plain C++
// over WarioStateBase. Its hooks are the labels called from other files.
#include "wario_state_base.hpp"

namespace wl3::level {

namespace {

class WarioStates2 : public WarioStateBase {
public:
	using WarioStateBase::WarioStateBase;

	// The slipping movement shared by the slipping states, from
	// ld hl, wWarioStateCounter / inc [hl] on. Returns true if it ends at the
	// "stop slipping" label; otherwise the routine returns.
	bool slip_move(uint8_t t1, uint8_t t2, uint8_t t3, bool stop_at_or_above_t3)
	{
		uint8_t v = inc_counter();
		if (v < t1) {
			move_by_dir(2);
			return false;
		}
		if (v < t2) {
			move_by_dir(1);
			return false;
		}
		if (v < t3) {
			if (rd8(wGlobalCounter) & 1)
				move_by_dir(1);
			return false;
		}
		return stop_at_or_above_t3 || v == t3;
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
		case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28:
		case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			CALL(InvalidWarioStateReset);
			return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	void port_SetState_Slipping()
	{
		wr8(wWarioState, WST_SLIPPING);
		clear({wSFXLoopCounter, wWarioStateCounter, wAttackCounter});
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		reset_anim();
		uint8_t dir = rd8(wDirection);
		wr8(wWarioStateCycles, dir);
		set_frameset(dir ? ADDR_Frameset_1425f : ADDR_Frameset_14252);
		anim(OAM1);
	}

	void port_UpdateState_Slipping()
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}

		uint8_t cycles = rd8(wWarioStateCycles);
		if (cycles >= 0x04)
			goto anim_and_input;
		if (cycles >= 0x02)
			goto walk_turn;
		{
			uint8_t joy = rd8(wJoypadDown) & (PAD_RIGHT | PAD_LEFT);
			if (!joy)
				goto anim_and_input;
			uint8_t left = (joy & PAD_LEFT) ? 1 : 0; // rlca x3 / and $01
			if (left != cycles)
				goto anim_and_input;
			wr8(wWarioStateCycles, static_cast<uint8_t>((left ^ 1) + 2));
		}
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		set_frameset(ld_mask(wWarioStateCycles, 0x01) ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
		goto reset_anim;

	walk_turn:
		if (!anim_ended())
			goto anim_and_input;
		wr8(wWarioStateCycles, static_cast<uint8_t>(cycles + 0x02));
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		set_frameset(ld_mask(wWarioStateCycles, 0x01) ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);

	reset_anim:
		reset_anim();

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

		{
			uint8_t hit = centre_collision();
			if (rd8(wWarioState) != WST_SLIPPING)
				return;
			if (!hit) {
				FARCALL(StartFall);
				return;
			}
		}

		update_pos_y();
		{
			uint8_t hit = front_collision();
			if (rd8(wWarioState) != WST_SLIPPING)
				return;
			if (hit)
				goto stop;
		}
		if (!on_slippery_ground())
			goto stop;
		if (slip_move(0x08, 0x0f, 0x15, false))
			goto stop;
		return;

	stop:
		wr8(wDirection, rd8(wWarioStateCycles) & 0x01);
		FARCALL(SetState_Idling);
	}

	void port_SetState_CrouchSlipping()
	{
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, rd8(wDirection));
		port_Func_19e89();
	}

	void port_Func_19e89()
	{
		wr8(wWarioState, WST_CROUCH_SLIPPING);
		clear({wSFXLoopCounter, wAttackCounter, wGrabState});
		wr8(wIsCrouching, TRUE);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		reset_anim();
		set_frameset(rd8(wWarioStateCycles) == 0x01 ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
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

		{
			uint8_t hit = centre_collision();
			if (rd8(wWarioState) != WST_CROUCH_SLIPPING)
				return;
			if (!hit) {
				FARCALL(StartCrouchFall);
				return;
			}
		}

		update_pos_y();
		{
			uint8_t hit = front_collision();
			if (rd8(wWarioState) != WST_CROUCH_SLIPPING)
				return;
			if (hit)
				goto stop;
		}
		if (!on_slippery_ground())
			goto stop;
		if (slip_move(0x06, 0x0a, 0x0e, true))
			goto stop;
		return;

	stop:
		wr8(wDirection, rd8(wWarioStateCycles) & 0x01);
		FARCALL(CrouchOrSlide);
	}

	void port_SetState_DraggedDown()
	{
		wr8(wWarioState, WST_DRAGGED_DOWN);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wAttackCounter, wGrabState, wJumpVelTable,
			wJumpVelIndex, wIsSmashAttacking, wIsCrouching, wIsRolling});
		wr8(wAutoMoveState, 1);
		coll_box_blrt();
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
	}

	void port_UpdateState_DraggedDown()
	{
		anim(OAM1);
		if (rd8(wAutoMoveState))
			return;
		if (rd8(wLevel) == THE_TEMPLE) {
			wr8(wLevelEndScreen, LVLEND_GAME_OVER);
			return;
		}
		FARCALL(StartFall);
	}

	void teleporting(uint8_t sfx_frame, uint8_t next_state)
	{
		wr8(wIsIntangible, TRUE);
		anim(OAM1);
		uint8_t v = inc_counter();
		if (v == sfx_frame) {
			port_PlayTeleportingSFX();
			return;
		}
		if (v < 0xb4)
			return;
		wr8(wWarioStateCounter, 0x00);
		wr8(wWarioState, next_state);
		wr8(wInvincibleCounter, 0x01);
		wr8(wRoomTransitionParam, rd8(wRoomTransitionParam) & static_cast<uint8_t>(~(1 << ROOMTRANSITIONF_DELAY_F)));
		CALL(TriggerRoomTransition);
	}

	void port_UpdateState_Teleporting() { teleporting(0x01, WST_STUNG); }

	void port_PlayTeleportingSFX() { play_sfx(SFX_082); }

	void port_UpdateState_WaterTeleporting() { teleporting(0x11, WST_WATER_STUNG); }

	void port_SetState_SandFalling()
	{
		wr8(wGrabState, 0);
		wr8(wWarioState, WST_SAND_FALLING);
		coll_box_btlr();
		clear({wJumpVelIndex, wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking, wAttackCounter,
			wIsRolling, wSwimVelIndex, wIsCrouching, wc0e0});
		set_pal(ADDR_WarioDefaultPal); // waits for HBlank
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
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15f94, ADDR_Frameset_15f97);
		anim(OAM1);
	}

	void port_UpdateState_SandFalling()
	{
		port_Func_1b2c0();
		if (rd8(wWarioState) != WST_SAND_FALLING)
			return;
		if (!centre_collision())
			return;
		update_pos_y();
		port_SetState_SandIdling();
	}

	void port_SetState_SandJumping()
	{
		play_sfx(SFX_JUMP);
		wr8(wWarioState, WST_SAND_JUMPING);
		clear({wJumpVelIndex, wWarioStateCounter, wWarioStateCycles});
		wr8(wJumpVelTable, 1);
		port_Func_1a12a();
	}

	// ld a, [wWalkVelIndex] / cp $04 / jr c / ld a, $00 / ld [wWalkVelIndex], a
	void clamp_walk_vel()
	{
		if (rd8(wWalkVelIndex) >= 0x04)
			wr8(wWalkVelIndex, 0x00);
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

		if (!ld_bit(wJoypadDown, B_PAD_A) || rd8(wJumpVelIndex) >= FALLING_JUMP_VEL_INDEX || up_collision())
			port_SetState_SandFalling();
	}

	void port_SetState_SandIdling()
	{
		wr8(wWarioState, WST_SAND_IDLING);
		wr8(wJumpVelIndex, 0);
		wr8(wJumpVelTable, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	void port_UpdateState_SandIdling() { port_Func_1b322(); }

	void port_SetState_SandWalking()
	{
		wr8(wWalkVelIndex, 0);
		wr8(wWarioState, WST_SAND_WALKING);
		uint8_t joy = rd8(wJoypadDown);
		if (joy & PAD_RIGHT)
			wr8(wDirection, DIRECTION_RIGHT);
		else if (joy & PAD_LEFT)
			wr8(wDirection, DIRECTION_LEFT);
		clear({wSFXLoopCounter, wJumpVelIndex, wJumpVelTable});
		FARCALL(Func_1e6ea);
	}

	void port_UpdateState_SandWalking()
	{
		sfx_loop(0x24, SFX_WALK);
		anim(OAM1);
		port_Func_1b333();
		if (rd8(wWarioState) != WST_SAND_WALKING)
			return;
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (!centre_collision())
			port_SetState_SandFalling();
	}

	void port_SetState_SandTurning()
	{
		wr8(wWarioState, WST_SAND_TURNING);
		reset_anim();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		set_frameset(flip_dir() ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
		anim(OAM1);
	}

	void port_UpdateState_SandTurning()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		anim(OAM1);
		if (!anim_ended())
			return;
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_SandWalking();
		else
			port_SetState_SandIdling();
	}

	void port_SetState_LadderClimbing()
	{
		wr8(wWarioState, WST_LADDER_CLIMBING);
		center_on_ladder();
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking, wJumpVelIndex, wJumpVelTable,
			wIsGettingOffLadder});
		coll_box_blrt();
		load_gfx(BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
		set_oam(BANK_OAM_155cc, ADDR_OAM_155cc);
		reset_anim();
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
		if (anim_ended())
			particle_at_wario(PARTICLE_DUST);
		port_HandleLadderInput();
	}

	void port_SetState_LadderIdling()
	{
		wr8(wWarioState, WST_LADDER_IDLING);
		reset_anim();
		clear({wJumpVelIndex, wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking,
			wIsGettingOffLadder});
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
		move_y(2);
		down_floor_transition();
		if (airborne_collision()) {
			update_pos_y();
			FARCALL(SetState_Idling);
			return;
		}
		if (inc_counter() < 0x40)
			return;
		wr8(wWarioStateCounter, 0x00);
		port_SetState_LadderIdling();
	}

	void port_SetState_GettingOffLadder()
	{
		wr8(wWarioState, WST_GETTING_OFF_LADDER);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
		wr8(wIsGettingOffLadder, TRUE);
		wr8(wIsIntangible, TRUE);
		wr8(wDirection, ld_bit(wJoypadDown, B_PAD_RIGHT) ? DIRECTION_RIGHT : DIRECTION_LEFT);
	}

	void port_UpdateState_GettingOffLadder()
	{
		sfx_loop(32, SFX_CLIMB);
		move_by_dir(1);
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
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wIsGettingOffLadder});
		wr8(wJumpVelTable, 1);
		if (rd8(wPowerUpLevel) >= LEAD_OVERALLS)
			wr8(wIsSmashAttacking, TRUE);
		reset_anim();
		set_frameset(ADDR_Frameset_15945);
		anim(OAM1);
	}

	void port_SetState_LadderSliding() { set_state_sliding(WST_LADDER_SLIDING); }

	// The shared body of UpdateState_LadderSliding and UpdateState_FenceSliding.
	void update_sliding(bool fence)
	{
		FARCALL(Func_19b25);
		if (room_transition()) {
			CALL(TriggerRoomTransition);
			return;
		}
		if (!rd8(fence ? wIsNearFence : wLadderInteraction)) {
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
		move_y(2);
		down_floor_transition();
		if (!airborne_collision())
			return;
		update_pos_y();
		if (!ld_and(wIsSmashAttacking)) {
			FARCALL(SetState_Idling);
			return;
		}
		if (rd8(wPowerUpLevel) < SUPER_JUMP_SLAM_OVERALLS)
			FARCALL(DoGroundPound);
		else
			FARCALL(DoWarioGroundShake);
	}

	void port_UpdateState_LadderSliding() { update_sliding(false); }

	void port_SetState_GrabSlipping()
	{
		wr8(wWarioState, WST_GRAB_SLIPPING);
		clear({wSFXLoopCounter, wWarioStateCounter, wAttackCounter});
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		reset_anim();
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
		{
			uint8_t hit = centre_collision();
			if (rd8(wWarioState) != WST_GRAB_SLIPPING)
				return;
			if (!hit) {
				FARCALL(Func_1edd3);
				return;
			}
		}
		{
			uint8_t hit = front_collision();
			if (rd8(wWarioState) != WST_GRAB_SLIPPING)
				return;
			if (hit)
				goto stop;
		}
		if (!on_slippery_ground())
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
			if (!airborne_collision())
				return;
			wr8(wWarioPos + 0, rd8(hYPosHi));
			wr8(wWarioPos + 1, rd8(hYPosHi + 1));
			wr8(wJumpVelTable, 0);
			wr8(wJumpVelIndex, 0);
		}
		if (inc_counter() < 48)
			return;
		wr8(wWarioStateCounter, 24);
		port_SetNextGetTreasureState(); // fallthrough
	}

	void port_SetNextGetTreasureState()
	{
		wr8(wWarioState, static_cast<uint8_t>(rd8(wWarioState) + 1));
		wr8(wWarioStateCycles, 0);
		FARCALL(Func_1e6ea);
	}

	void port_UpdateState_GetTreasureWalk()
	{
		anim(OAM1);
		move_by_dir(1);

		uint8_t v = static_cast<uint8_t>(rd8(wWarioStateCounter) - 1);
		wr8(wWarioStateCounter, v);
		if (v)
			return;
		wr8(wWarioState, static_cast<uint8_t>(rd8(wWarioState) + 1)); // WST_GET_TREASURE_TURN_FRONT_1
		reset_anim();
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
		frameset_by_dir(ADDR_Frameset_1fc46a, ADDR_Frameset_1fc461);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureTurnFront()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		if (rd8(wWarioState) == WST_GET_TREASURE_TURN_FRONT_2) {
			port_SetState_GetTreasureLookFront();
			return;
		}
		wr8(wWarioState, WST_GET_TREASURE_LAUGH);
		reset_anim();
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
		reset_anim();
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
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
		load_gfx(BANK_WarioClearGfx, ADDR_WarioClearGfx);
		set_oam(BANK_OAM_1fc000, ADDR_OAM_1fc000);
		reset_anim();
		set_frameset(ADDR_Frameset_1fc2c4);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureOpen()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioState, WST_GET_TREASURE_TURN_WALK);
		CALL(LoadCollectedTreasurePal_Level);
		reset_anim();
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
		set_frameset(flip_dir() ? ADDR_Frameset_1fc453 : ADDR_Frameset_1fc45a);
		anim(OAM2);
	}

	void port_UpdateState_GetTreasureTurnWalk()
	{
		anim(OAM2);
		if (!anim_ended())
			return;
		wr8(wWarioStateCounter, 32);
		wr8(wAutoMoveState, 0);
		port_SetNextGetTreasureState();
	}

	void port_SetState_GetTreasureLookFront()
	{
		wr8(wWarioState, WST_GET_TREASURE_LOOK_FRONT);
		reset_anim();
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
		particle_at_wario(PARTICLE_TREASURE_GLOW);
		wr8(wWarioStateCounter, 0);
		wr8(wWarioStateCycles, 0);
	}

	void port_UpdateState_GetTreasureClear()
	{
		if (!ld_bit(wJoypadDown, B_PAD_A) && inc_counter() < 180)
			return;
		if (!rd8(wCurParticleAnimEnded))
			return;
		wr8(wWarioStateCounter, 0);
		wr8(wLevelEndScreen, rd8(wLevelEndScreen) & 0x7f);
	}

	void port_SetState_Sleeping()
	{
		wr8(wWarioState, WST_SLEEPING);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles});
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_1421c, ADDR_Frameset_14225);
		anim(OAM1);
	}

	void port_UpdateState_Sleeping()
	{
		anim(OAM1);
		switch (rd8(wWarioStateCounter)) {
		case 1: // .asm_1acd0
			if (anim_ended())
				play_sfx(SFX_SLEEP);
			if (!rd8(wJoypadDown))
				return;
			reset_anim();
			inc_counter();
			frameset_by_dir(ADDR_Frameset_14240, ADDR_Frameset_14249);
			anim(OAM1);
			return;
		case 2: // .asm_1ad0c
			if (anim_ended())
				FARCALL(SetState_Idling);
			return;
		}
		if (!anim_ended())
			return;
		play_sfx(SFX_SLEEP);
		reset_anim();
		inc_counter();
		frameset_by_dir(ADDR_Frameset_1422e, ADDR_Frameset_14237);
		anim(OAM1);
	}

	void port_SetState_LadderScratching()
	{
		wr8(wWarioState, WST_LADDER_SCRATCHING);
		coll_box_btlr();
		clear({wJumpVelIndex, wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsGettingOffLadder});
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15903, ADDR_Frameset_15924);
		anim(OAM1);
	}

	void port_UpdateState_LadderScratching()
	{
		anim(OAM1);
		if (!ld_and(wJoypadDown) && !anim_ended())
			return;
		port_SetState_LadderIdling();
	}

	void port_SetState_FenceShakeSliding()
	{
		wr8(wWarioState, WST_FENCE_SHAKE_SLIDING);
		reset_anim();
		wr8(wWarioStateCounter, 0);
		wr8(wAutoMoveState, 0x03);
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		frameset_by_dir(ADDR_Frameset_14d0b, ADDR_Frameset_14d10);
		anim(OAM1);
	}

	void port_UpdateState_FenceShakeSliding()
	{
		anim(OAM1);
		if (!ld_and(wWarioStateCounter) && anim_ended()) {
			reset_anim();
			set_frameset(ADDR_Frameset_14d15);
			anim(OAM1);
			inc_counter();
		}
		if (!ld_and(wJumpVelTable))
			return;
		CALL(ApplyJumpVelocity);
		if (!airborne_collision())
			return;
		update_pos_y();
		wr8(wJumpVelTable, 0);
		wr8(wJumpVelIndex, 0);
	}

	void port_SetState_FenceMovingVertical()
	{
		wr8(wWarioState, WST_FENCE_MOVING_VERTICAL);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking});
		coll_box_blrt();
		load_gfx(BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
		set_oam(BANK_OAM_155cc, ADDR_OAM_155cc);
		reset_anim();
		set_frameset(ADDR_Frameset_158d6);
		anim(OAM1);
	}

	void port_UpdateState_FenceMovingVertical()
	{
		sfx_loop(0x20, SFX_CLIMB);
		anim(OAM1);
		if (anim_ended())
			particle_at_wario(PARTICLE_DUST);
		port_Func_1b168();
	}

	void port_SetState_FenceIdling()
	{
		wr8(wWarioState, WST_FENCE_IDLING);
		reset_anim();
		clear({wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking});
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
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking});
		coll_box_blrt();
		reset_anim();
		uint8_t joy = rd8(wJoypadDown);
		bool right;
		if (joy & PAD_RIGHT)
			right = true;
		else if (joy & PAD_LEFT)
			right = false;
		else
			right = facing_right();
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

	void port_SetState_FenceSliding() { set_state_sliding(WST_FENCE_SLIDING); }

	void port_UpdateState_FenceSliding() { update_sliding(true); }

	// The jump input check at the start of Func_1b168, Func_1b21a and
	// Func_1b24f. Returns true if it jumped (and returned).
	bool fence_jump_input()
	{
		if ((!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A)) || ld_bit(wJoypadPressed, B_PAD_A)) {
			FARCALL(StartJump_FromInput);
			return true;
		}
		return false;
	}

	void port_Func_1b168()
	{
		if ((rd8(wJoypadDown) & (PAD_B | PAD_DOWN)) == (PAD_B | PAD_DOWN)) {
			port_SetState_FenceSliding();
			return;
		}
		if (fence_jump_input())
			return;

		uint8_t joy = rd8(wJoypadDown);
		if (joy & PAD_UP) {
			FARCALL(Func_19ae4);
			if (ld_and(wIsNearFence))
				move_y(-1);
			return;
		}
		if (joy & PAD_DOWN) {
			move_y(1);
			if (centre_collision()) {
				update_pos_y();
				FARCALL(SetState_Idling);
				return;
			}
			FARCALL(Func_19afb);
			if (!ld_and(wIsNearFence))
				FARCALL(StartFall);
			return;
		}
		if (joy & (PAD_RIGHT | PAD_LEFT))
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
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_FenceMovingHorizontal();
	}

	void port_Func_1b24f()
	{
		if (fence_jump_input())
			return;
		uint8_t joy = rd8(wJoypadDown);
		bool right;
		if (joy & PAD_RIGHT) {
			right = true;
		} else if (joy & PAD_LEFT) {
			right = false;
		} else {
			if (joy & (PAD_UP | PAD_DOWN))
				port_SetState_FenceMovingVertical();
			else
				port_SetState_FenceIdling();
			return;
		}
		wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		if (front_collision())
			return;
		move_x(right ? 1 : -1);
	}

	void port_Func_1b2c0()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		port_Func_1b302();
		if (front_collision())
			return;
		FARCALL(Func_2b17a);
		clamp_walk_vel();
		if (ld_and(wIsTurning))
			port_Func_1a14b();
	}

	void port_Func_1b302()
	{
		if (centre_collision())
			return;
		move_y(ld_bit(wJoypadDown, B_PAD_DOWN) ? 2 : 1);
	}

	void port_Func_1b322()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_SandWalking();
	}

	void port_Func_1b333()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_SetState_SandJumping();
			return;
		}
		uint8_t joy = rd8(wJoypadDown);
		bool right;
		if (joy & PAD_RIGHT) {
			right = true;
		} else if (joy & PAD_LEFT) {
			right = false;
		} else {
			port_SetState_SandIdling();
			return;
		}
		// (buggy: wDirection was already switched by the sand idling input
		// handling, so Sand Turning is never reached)
		if (facing_right() != right) {
			port_SetState_SandTurning();
			return;
		}
		wr8(wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
		if (front_collision())
			return;
		walk_with_velocity(right);
		clamp_walk_vel();
	}

	void port_HandleLadderInput()
	{
		uint8_t joy = rd8(wJoypadDown);
		if ((joy & (PAD_B | PAD_DOWN)) == (PAD_B | PAD_DOWN)) {
			port_SetState_LadderSliding();
			return;
		}
		if (joy & PAD_UP) {
			up_collision();
			if (ld_and(wJumpCollision))
				return;
			move_y(-1);
			if (up_floor_transition())
				return;
			if (ld_and(wLadderInteraction))
				return;
			FARCALL(Func_19b12);
			if (ld_and(wLadderInteraction))
				return;
			FARCALL(SetState_Idling);
			return;
		}
		if (joy & PAD_DOWN) {
			move_y(1);
			if ((rd8(wCameraConfigFlags) & CAM_SCROLLING_MASK) >= CAM_TRANSITIONS) {
				uint8_t floor = floor_for_y_pos();
				if (rd8(wFloor) > floor) {
					cpu_.c = floor;
					CALL(StartDownwardsFloorTransition);
					return;
				}
			}
			FARCALL(Func_19b12);
			if (rd8(wLadderInteraction)) {
				if (centre_collision())
					center_on_ladder();
				return;
			}
			// .no_more_ladder_interaction
			if (centre_collision())
				FARCALL(SetState_Idling);
			else
				FARCALL(StartFall);
			return;
		}
		port_SetState_LadderIdling();
	}

	void port_Func_1b480()
	{
		if (ld_bit(wJoypadDown, B_PAD_B) && ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			// .asm_1b4a7: the bottom collision a block to the side, result in b
			if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
				wr8(wCollisionBoxRight, 16);
				FARCALL(Func_19741);
			} else {
				wr8(wCollisionBoxLeft, static_cast<uint8_t>(-16));
				FARCALL(Func_197b1);
			}
			bool ok = !cpu_.b;
			wr8(wCollisionBoxLeft, static_cast<uint8_t>(-9));
			wr8(wCollisionBoxRight, 9);
			if (ok)
				port_SetState_GettingOffLadder();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_UP | PAD_DOWN)) {
			port_SetState_LadderClimbing();
			return;
		}
		uint8_t lo = rd8(wWarioStateCounter);
		wr8(wWarioStateCounter, static_cast<uint8_t>(lo + 1));
		uint8_t hi = static_cast<uint8_t>(rd8(wWarioStateCounter + 1) + (lo == 0xff));
		wr8(wWarioStateCounter + 1, hi);
		if (hi < 0x02)
			return;
		port_SetState_LadderScratching();
	}
};

} // namespace

} // namespace wl3::level

// Every caller reads no register after these (the asm liveness check):
// they clobber them all.
using wl3::level::WarioStates2;

PORT_EX(UpdateWarioStates_Group2, 0, ALL_REGISTERS) { WarioStates2(gb).port_UpdateWarioStates_Group2(); }
PORT_EX(SetState_Slipping, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_Slipping(); }
PORT_COVERED(UpdateState_Slipping);
PORT_EX(SetState_CrouchSlipping, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_CrouchSlipping(); }
PORT_COVERED(Func_19e89);
PORT_COVERED(UpdateState_CrouchSlipping);
PORT_EX(SetState_DraggedDown, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_DraggedDown(); }
PORT_COVERED(UpdateState_DraggedDown);
PORT_COVERED(UpdateState_Teleporting);
PORT_COVERED(PlayTeleportingSFX);
PORT_COVERED(UpdateState_WaterTeleporting);
PORT_EX(SetState_SandFalling, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_SandFalling(); }
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
PORT_EX(SetState_LadderClimbing, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_LadderClimbing(); }
PORT_COVERED(UpdateState_LadderClimbing);
PORT_COVERED(SetState_LadderIdling);
PORT_COVERED(UpdateState_LadderIdling);
PORT_COVERED(UpdateState_LadderShakeSlipping);
PORT_COVERED(SetState_GettingOffLadder);
PORT_COVERED(UpdateState_GettingOffLadder);
PORT_COVERED(SetState_LadderSliding);
PORT_COVERED(UpdateState_LadderSliding);
PORT_EX(SetState_GrabSlipping, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_GrabSlipping(); }
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
PORT_EX(SetState_Sleeping, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_Sleeping(); }
PORT_COVERED(UpdateState_Sleeping);
PORT_COVERED(SetState_LadderScratching);
PORT_COVERED(UpdateState_LadderScratching);
PORT_EX(SetState_FenceShakeSliding, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_FenceShakeSliding(); }
PORT_COVERED(UpdateState_FenceShakeSliding);
PORT_EX(SetState_FenceMovingVertical, 0, ALL_REGISTERS) { WarioStates2(gb).port_SetState_FenceMovingVertical(); }
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
