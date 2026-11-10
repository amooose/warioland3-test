// Wario's states, part 1 (src/engine/level/wario_states_1.asm): normal
// Wario on land and in water (idling, walking, jumping, attacking,
// crouching, sliding and rolling, swimming, grabbing and throwing, the
// bumps and stings), and the frame's state dispatch, as plain C++ over
// WarioStateBase. Its hooks are the labels called from other files.
#include "frame.hpp"
#include "wario_state_base.hpp"

namespace wl3::level {

namespace {

class WarioStates1 : public WarioStateBase {
public:
	explicit WarioStates1(GB *gb) : WarioStateBase(gb, BANK_UpdateWarioStates_Group1) {} // its ROM data from its bank

	void port_Func_1e6ea()
	{
		reset_anim();
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		frameset_by_dir(ADDR_Frameset_149b4, ADDR_Frameset_149c5);
		anim(OAM1);
	}

	void port_SetState_Turning()
	{
		mem_.wario_state() = WST_TURNING;
		clear({wAttackCounter, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelIndex,
			wJumpVelTable});
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14a82, ADDR_OAM_14a82);
		set_frameset(flip_dir() ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
		anim(OAM1);
	}

	void port_SetState_Attacking()
	{
		mem_.attack_counter() = MAX_ATTACK_COUNTER;
		mem_.sfx_loop_counter() = 0;
		mem_.wario_state() = WST_ATTACKING;
		collision_box(-1, -27, -9, 9);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable,
			wIsCrouching});
		set_oam(BANK_OAM_14d1b, ADDR_OAM_14d1b);
		load_gfx(BANK_WarioAttackGfx, ADDR_WarioAttackGfx);
		// with the garlic power-up, Wario's attack is stronger
		if (mem_.power_up_level() < GARLIC)
			frameset_by_dir(ADDR_Frameset_151e4, ADDR_Frameset_151fd);
		else
			frameset_by_dir(ADDR_Frameset_1521c, ADDR_Frameset_1522d);
		anim(OAM1);
	}

	void move_up_if_on_steppable_object()
	{
		if (ld_and(wIsOnSteppableObject))
			move_y(-2);
	}

	void port_SetState_Walking()
	{
		mem_.wario_state() = WST_WALKING;
		uint8_t pad = mem_.joypad_down();
		if (pad & PAD_RIGHT)
			mem_.direction() = DIRECTION_RIGHT;
		else if (pad & PAD_LEFT)
			mem_.direction() = DIRECTION_LEFT;
		clear({wSFXLoopCounter, wJumpVelIndex, wJumpVelTable, wIsSmashAttacking, wWarioStateCounter,
			wWarioStateCycles});
		port_Func_1e6ea(); // fallthrough
	}

	void port_SetState_Airborne()
	{
		mem_.wario_slope_interaction() = 0;
		mem_.wario_state() = WST_AIRBORNE;
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wIsCrouching, wAttackCounter});
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f94, ADDR_Frameset_15f97);
		anim(OAM1);
		move_up_if_on_steppable_object();
	}

	void port_SetState_Diving()
	{
		mem_.wario_state() = WST_DIVING;
		mem_.collision_box_bottom() = static_cast<uint8_t>(-1);
		mem_.collision_box_left() = static_cast<uint8_t>(-9);
		mem_.collision_box_right() = 9;
		clear({wJumpVelIndex, wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking, wAttackCounter,
			wIsRolling, wSwimVelIndex, wIsCrouching, wc0e0});
		set_pal(ADDR_WarioDefaultPal);
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
	}

	void port_SetState_Submerged()
	{
		mem_.wario_state() = WST_SUBMERGED;
		clear({wWarioStateCounter, wWarioStateCycles, wSwimVelIndex, wc0e0, wSwimmingDirectionInput});
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		bool can_swim = mem_.power_up_level() >= SWIMMING_FLIPPERS && mem_.water_interaction() != NON_SUBMERSIBLE_WATER;
		if (can_swim)
			collision_box(-1, -15, -9, 9);
		// else .cannot_swim_underwater
		reset_anim();
		if (can_swim)
			frameset_by_dir(ADDR_Frameset_15576, ADDR_Frameset_1558b);
		else
			frameset_by_dir(ADDR_Frameset_155a0, ADDR_Frameset_155a3);
		anim(OAM1);
	}

	void port_SetState_WaterSurfaceIdling()
	{
		mem_.wario_state() = WST_WATER_SURFACE_IDLING;
		collision_box(-1, -27, -9, 9);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wSwimVelIndex, wc0e0,
			wSwimmingDirectionInput});
		if (!ld_and(wWaterInteraction))
			mem_.water_interaction() = SUBMERSIBLE_WATER;
		frameset_by_dir(ADDR_Frameset_15554, ADDR_Frameset_1555b);
		anim(OAM1);
	}

	// hPos = Wario's position + (dy, dx), the slide dust's position
	void get_pos_with_offset(int dy, int dx)
	{
		uint16_t x = static_cast<uint16_t>(mem_.wario_x() + dx);
		mem_.x_pos_lo() = static_cast<uint8_t>(x);
		wr8(hXPosLo - 1, static_cast<uint8_t>(x >> 8));
		uint16_t y = static_cast<uint16_t>(mem_.wario_y() + dy);
		wr8(hXPosLo - 2, static_cast<uint8_t>(y));
		wr8(hXPosLo - 3, static_cast<uint8_t>(y >> 8));
	}

	void port_SetState_Sliding()
	{
		mem_.wario_state() = WST_SLIDING;
		clear({wSFXLoopCounter, wIsCrouching, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles,
			wWalkVelIndex, wGrabState});
		mem_.is_rolling() = TRUE;
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_1644a, ADDR_OAM_1644a);
		if (!ld_bit(wWarioSlopeInteraction, LEFT_SLOPE_F)) {
			get_pos_with_offset(-0x0c, 0x0c);
			Routines(gb_).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, PARTICLE_SLIDE_DUST_LEFT); });
			mem_.direction() = DIRECTION_LEFT;
			set_frameset(ADDR_Frameset_16708);
		} else {
			get_pos_with_offset(-0x0c, -0x0c);
			Routines(gb_).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, PARTICLE_SLIDE_DUST_RIGHT); });
			mem_.direction() = DIRECTION_RIGHT;
			set_frameset(ADDR_Frameset_1671d);
		}
		anim(OAM1);
	}

	void port_SetState_RollingAirborne()
	{
		mem_.wario_slope_interaction() = 0;
		mem_.grab_state() = 0;
		mem_.jump_vel_table() = JUMP_VEL_KNOCK_BACK;
		mem_.wario_state() = WST_ROLLING_AIRBORNE;
		mem_.is_rolling() = 0x02;
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_1644a, ADDR_OAM_1644a);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_166e6, ADDR_Frameset_166f7);
		anim(OAM1);
	}

	void port_SetState_CrouchWalking()
	{
		collision_box(-1, -15, -9, 9);
		mem_.wario_state() = WST_CROUCH_WALKING;
		mem_.is_crouching() = TRUE;
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wAttackCounter,
			wGrabState});
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_frameset(mem_.direction() == DIRECTION_RIGHT ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
		anim(OAM1);
	}

	void port_SetState_CrouchAirborne()
	{
		mem_.wario_state() = WST_CROUCH_AIRBORNE;
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter});
		collision_box(-1, -15, -9, 9);
		mem_.jump_vel_table() = JUMP_VEL_NORMAL;
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		frameset_by_dir(ADDR_Frameset_14a38, ADDR_Frameset_14a3b);
		anim(OAM1);
		move_up_if_on_steppable_object();
	}

	// flashes Wario's palette while he's invisible
	void port_Func_1f6c2()
	{
		uint8_t t = mem_.global_counter() & 0x07;
		if (!t)
			set_pal(ADDR_WarioInvisiblePal);
		else if (t == 3)
			set_pal(ADDR_WarioDefaultPal);
	}

	void port_HandleWaterSurfaceFloatingMovement()
	{
		if (mem_.global_counter() & 0x03)
			return;
		uint8_t t = static_cast<uint8_t>(mem_.water_surface_floating_counter() + 1);
		mem_.water_surface_floating_counter() = t;
		if (t < 7) {
			home::move_wario_y(gb_, 1, false); // MoveWarioDown_WithoutCameraOffset
			return;
		}
		if (t >= 12)
			mem_.water_surface_floating_counter() = 0;
		home::move_wario_y(gb_, -1, false); // MoveWarioUp_WithoutCameraOffset
	}

	// the jump: higher with the high jump boots and up held
	uint8_t jump_table(uint8_t normal, uint8_t high) const
	{
		return mem_.power_up_level() >= HIGH_JUMP_BOOTS && ld_bit(wJoypadDown, B_PAD_UP) ? high : normal;
	}

	void port_StartJump()
	{
		mem_.jump_vel_table() = jump_table(JUMP_VEL_NORMAL, JUMP_VEL_HIGH_JUMP);
		port_SetState_Airborne(); // fallthrough
	}

	void port_SetState_Walking_ResetWalkVel()
	{
		mem_.walk_vel_index() = 0;
		port_SetState_Walking(); // fallthrough
	}

	void port_CrouchOrSlide()
	{
		if (ld_and(wWarioSlopeInteraction)) {
			port_SetState_Sliding();
			return;
		}
		collision_box(-1, -15, -9, 9);
		mem_.wario_state() = WST_CROUCHING;
		clear({wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter, wWalkVelIndex, wJumpVelTable,
			wJumpVelIndex});
		mem_.is_crouching() = TRUE;
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_CrouchWalking();
			return;
		}
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		reset_anim();
		set_frameset(mem_.direction() == DIRECTION_RIGHT ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
	}

	void port_StartCrouchFall()
	{
		mem_.walk_vel_index() = 0;
		mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		port_SetState_CrouchAirborne();
	}

	void port_StartCrouchJump()
	{
		play_sfx(SFX_JUMP);
		mem_.jump_vel_index() = 0;
		port_SetState_CrouchAirborne(); // fallthrough
	}

	void port_StartFall_SkipResetJumpUpwards()
	{
		mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		port_StartJump(); // jr
	}

	void port_StartJump_FromInput()
	{
		play_sfx(SFX_JUMP);
		mem_.jump_vel_index() = 0;
		mem_.is_smash_attacking() = 0;
		mem_.jumping_upwards() = TRUE;
		port_StartJump(); // fallthrough
	}

	void port_DiveFromSurface_SkipSplash()
	{
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_155a0, ADDR_Frameset_155a3);
		anim(OAM1);
		port_SetState_Diving(); // jr
	}

	void port_SurfaceFromUnderwater()
	{
		mem_.sfx_loop_counter() = 0;
		mem_.water_surface_floating_counter() = 0;
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		set_oam(BANK_OAM_15254, ADDR_OAM_15254);
		port_SetState_WaterSurfaceIdling(); // fallthrough
	}

	void port_StartFall()
	{
		mem_.jumping_upwards() = FALSE;
		port_StartFall_SkipResetJumpUpwards(); // fallthrough
	}

	void port_StartJump_FromWaterSurface()
	{
		play_sfx(SFX_SPLASH);
		mem_.jump_vel_index() = 0x01;
		mem_.jumping_upwards() = TRUE;
		mem_.do_full_jump() = TRUE;
		port_StartJump(); // jr
	}

	void port_StartJump_FromSand()
	{
		mem_.jump_vel_index() = 0;
		mem_.jumping_upwards() = TRUE;
		mem_.do_full_jump() = TRUE;
		mem_.jump_vel_table() = jump_table(JUMP_VEL_KNOCK_BACK, JUMP_VEL_NORMAL);
		port_SetState_Airborne(); // jr
	}

	// a particle at Wario's position, on the block row
	void particle_on_row(uint8_t id)
	{
		get_pos();
		mem_.y_pos_lo() = mem_.y_pos_lo() & 0xf0;
		particle(id);
	}

	void port_DiveFromSurface()
	{
		mem_.grab_state() = 0;
		particle_on_row(PARTICLE_SPLASH);
		play_sfx(SFX_SPLASH);
		port_DiveFromSurface_SkipSplash(); // fallthrough
	}

	void port_SetState_TryingSubmerge()
	{
		mem_.wario_state() = WST_TRYING_SUBMERGE;
		mem_.wario_state_counter() = 0;
		mem_.wario_state_cycles() = 0;
		reset_anim();
		frameset_by_dir(ADDR_Frameset_155b0, ADDR_Frameset_155b7);
		anim(OAM1);
	}

	void port_UpdateState_TryingSubmerge()
	{
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			port_SetState_WaterSurfaceIdling();
			return;
		}
		anim(OAM1);
	}

	void port_Func_1e1e3()
	{
		mem_.jump_vel_index() = 0;
		port_SetState_RollingAirborne(); // jr
	}

	void port_HandleInput_Idling()
	{
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_B) {
			port_SetState_Attacking();
			return;
		}
		if (pressed & PAD_A) {
			port_StartJump_FromInput();
			return;
		}
		uint8_t pad = mem_.joypad_down();
		bool right;
		if (pad & PAD_RIGHT)
			right = true;
		else if (pad & PAD_LEFT)
			right = false;
		else
			return;
		// walk if already facing that way, else turn
		if (facing_right() == right)
			port_SetState_Walking_ResetWalkVel();
		else
			port_SetState_Turning();
	}

	void port_Func_1e174()
	{
		mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		port_SetState_RollingAirborne(); // fallthrough
	}

	// de = the swim velocity's index
	void port_Func_1f863(uint16_t index)
	{
		move_y(-rd8(static_cast<uint16_t>(ADDR_SwimStraightVelTable + index)));
	}

	void port_SetState_Throwing()
	{
		mem_.wario_state_counter() = 0;
		mem_.wario_state_cycles() = 0;
		mem_.wario_state() = WST_THROWING;
	}

	// either water current byte has one of `mask`
	bool water_current_mask(uint8_t mask) const
	{
		return ((mem_.cur_water_current() | rd8(wCurWaterCurrent - 1)) & mask) != 0;
	}

	void port_Func_1f3a7()
	{
		if (centre_collision()) {
			update_pos_y();
			mem_.unk_c0e1() = PAD_DOWN;
			return;
		}
		if (!water_current_mask(CURRENT_UP | CURRENT_DOWN))
			move_y(1);
	}

	void port_SetState_SmashAttacking()
	{
		mem_.is_smash_attacking() = 0x81;
		mem_.wario_state() = WST_SMASH_ATTACKING;
		clear({wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable});
		collision_box(-1, -27, -9, 9);
	}

	void port_SetState_LadderShakeStunned()
	{
		mem_.wario_state() = WST_LADDER_SHAKE_SLIDING;
		play_sfx(SFX_SLIDE);
		clear({wSFXLoopCounter, wWarioStateCounter, wWarioStateCycles, wIsGettingOffLadder});
		mem_.jump_vel_table() = 1;
		reset_anim();
		set_frameset(ADDR_Frameset_15948);
		anim(OAM1);
	}

	void port_StartDive()
	{
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15569, ADDR_Frameset_15573);
		anim(OAM1);
		port_SetState_Diving(); // fallthrough
	}

	void port_UpdateState_Idling()
	{
		uint8_t hit = centre_collision();
		if (!still_in(WST_IDLING))
			return;
		if (!hit) {
			port_StartFall();
			return;
		}
		update_pos_y();
		if (!still_in(WST_IDLING))
			return;

		if (ld_bit(wJoypadDown, B_PAD_DOWN)) {
			collision(BANK_Func_19b12, ADDR_Func_19b12, check_func_19b12);
			if (!still_in(WST_IDLING))
				return;
			if (mem_.ladder_interaction() != LADDER_UNDERNEATH) {
				port_CrouchOrSlide();
				return;
			}
			FARCALL(SetState_LadderClimbing);
			return;
		}

		anim(OAM1);
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (room_transition()) {
			home::trigger_room_transition(gb_);
			return;
		}
		if (ld_and(wLadderInteraction) && ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_LadderClimbing);
			return;
		}
		if (ld_and(wIsNearFence) && ld_bit(wJoypadDown, B_PAD_UP)) {
			FARCALL(SetState_FenceMovingVertical);
			return;
		}
		if (!still_in(WST_IDLING))
			return;
		port_HandleInput_Idling();
		if (!still_in(WST_IDLING) || ld_and(wIsOnSteppableObject))
			return;
		// idle long enough and he falls asleep (wWarioStateCycles counts the
		// counter's carries)
		uint8_t lo = mem_.wario_state_counter();
		mem_.wario_state_counter() = static_cast<uint8_t>(lo + 1);
		uint8_t hi = static_cast<uint8_t>(mem_.wario_state_cycles() + (lo == 0xff));
		mem_.wario_state_cycles() = hi;
		if (hi >= NUM_SLEEPING_IDLE_CYCLES)
			FARCALL(SetState_Sleeping);
	}

	void port_SetState_ThrowingAirborne()
	{
		mem_.wario_state() = WST_THROWING_AIRBORNE;
		mem_.grab_state() = (mem_.grab_state() & GRAB_FLAGS_MASK) | GRAB_THROW;
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
		frameset_by_dir(ADDR_Frameset_15fae, ADDR_Frameset_15fb3);
		anim(OAM1);
	}

	void port_SetState_WaterSurfaceMoving()
	{
		mem_.wario_state() = WST_WATER_SURFACE_MOVING;
		collision_box(-1, -27, -9, 9);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wSwimVelIndex});
		frameset_by_dir(ADDR_Frameset_15554, ADDR_Frameset_1555b);
		anim(OAM1);
	}

	void port_SetState_Rolling()
	{
		clear({wSFXLoopCounter, wJumpVelIndex, wJumpVelTable});
		mem_.wario_state() = WST_ROLLING;
		collision_box(-1, -14, -9, 9);
		mem_.is_rolling() = 0x02;
		reset_anim();
		frameset_by_dir(ADDR_Frameset_166e6, ADDR_Frameset_166f7);
		anim(OAM1);
	}

	void port_SetState_SwimKnockBack()
	{
		play_sfx(SFX_BUMP);
		mem_.wario_state() = WST_SWIM_KNOCK_BACK;
		clear({wWarioStateCounter, wWarioStateCycles, wSwimVelIndex, wSwimmingDirectionInput, wc0e0});
		load_gfx(BANK_WarioSwimGfx, ADDR_WarioSwimGfx);
		reset_anim();
		frameset_by_dir(ADDR_Frameset_15573, ADDR_Frameset_15569);
		anim(OAM1);
	}

	// the grab state's flags kept, with a new state
	void grab_state(uint8_t state) { mem_.grab_state() = static_cast<uint8_t>((mem_.grab_state() & GRAB_FLAGS_MASK) + state); }

	void port_SetState_ThrowCharging()
	{
		play_sfx(SFX_02C);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
		mem_.wario_state() = WST_THROW_CHARGING;
		grab_state(GRAB_CHARGE_THROW);
		load_gfx(BANK_WarioThrowGfx, ADDR_WarioThrowGfx);
		set_oam(BANK_OAM_1606a, ADDR_OAM_1606a);
		frameset_by_dir(ADDR_Frameset_1642c, ADDR_Frameset_1642f);
		anim(OAM1);
	}

	void port_SetState_CrouchSliding()
	{
		mem_.sfx_loop_counter() = 0;
		mem_.attack_counter() = 0;
		play_sfx(SFX_SLIDE);
		mem_.wario_state() = WST_CROUCH_SLIDING;
		mem_.is_crouching() = TRUE;
		collision_box(-1, -15, -9, 9);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		set_frameset(mem_.direction() == DIRECTION_RIGHT ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
	}

	void port_UpdateState_PickedUp()
	{
		bool right = facing_right();
		// the d-pad away from the facing, or A or B, wiggles
		bool wiggle = ld_mask(wJoypadPressed, PAD_A | PAD_B | (right ? PAD_LEFT : PAD_RIGHT));
		if (!wiggle) {
			anim(OAM1);
			if (count_down_to_zero(wPickedUpFrameCounter)) {
				mem_.picked_up_frame_counter() = MAX_PICKEB_PAD_UPRAME_COUNTER;
				mem_.picked_up_wiggle_counter() = NUM_WIGGLES_TO_ESCAPE;
			}
			return;
		}
		mem_.picked_up_frame_counter() = MAX_PICKEB_PAD_UPRAME_COUNTER;
		mem_.direction() = right ? DIRECTION_LEFT : DIRECTION_RIGHT;
		set_frameset(right ? ADDR_Frameset_15f70 : ADDR_Frameset_15f7f);
		reset_anim();
		anim(OAM1);
		mem_.picked_up_frame_counter() = MAX_PICKEB_PAD_UPRAME_COUNTER;
		if (!count_down_to_zero(wPickedUpWiggleCounter))
			return;
		mem_.auto_move_state() = 0;
		port_StartFall();
	}

	// dec [addr]: true if it reached 0
	bool count_down_to_zero(uint16_t addr)
	{
		uint8_t v = static_cast<uint8_t>(rd8(addr) - 1);
		wr8(addr, v);
		return !v;
	}

	void port_SetState_UnderwaterThrusting()
	{
		mem_.wario_state() = WST_UNDERWATER_THRUSTING;
		collision_box(-1, -15, -9, 9);
		clear({wSFXLoopCounter, wSwimVelIndex, wWarioStateCycles, wFrameDuration, wAnimationFrame});
		uint8_t in = mem_.swimming_direction_input();
		bool right;
		if (!in) {
			right = mem_.direction() == DIRECTION_RIGHT;
			mem_.swimming_direction_input() = right ? PAD_RIGHT : PAD_LEFT;
		} else if (in & PAD_LEFT) {
			right = false;
		} else if (in & PAD_RIGHT) {
			right = true;
		} else {
			right = facing_right();
		}
		mem_.direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
		bool gloves = mem_.power_up_level() >= PRINCE_FROGS_GLOVES;
		if (right)
			set_frameset(gloves ? ADDR_Frameset_1556c : ADDR_Frameset_155c5);
		else
			set_frameset(gloves ? ADDR_Frameset_15562 : ADDR_Frameset_155be);
		anim(OAM1);
	}

	void port_StartUpwardsUnderwaterThrusting()
	{
		mem_.swimming_direction_input() = PAD_UP;
		port_SetState_UnderwaterThrusting();
	}

	void port_SetState_SwimKnockBack_SwitchDirection()
	{
		flip_dir();
		port_SetState_SwimKnockBack(); // fallthrough
	}

	void port_StartUnderwaterThrusting()
	{
		mem_.swimming_direction_input() = mem_.joypad_down() & PAD_CTRL_PAD;
		port_SetState_UnderwaterThrusting(); // fallthrough
	}

	void port_Func_1de3f()
	{
		reset_anim();
		frameset_by_dir(ADDR_Frameset_163fe, ADDR_Frameset_1641e);
		anim(OAM1);
		grab_state(GRAB_THROW);
		port_SetState_Throwing(); // fallthrough
	}

	// airborne gfx (the OAM bank stays), then a frameset by direction and the
	// animation
	void airborne_gfx(uint16_t left, uint16_t right)
	{
		reset_anim();
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		wr8(wWarioOAMPtr + 0, static_cast<uint8_t>(ADDR_OAM_15955 >> 8));
		wr8(wWarioOAMPtr + 1, static_cast<uint8_t>(ADDR_OAM_15955));
		frameset_by_dir(left, right);
		anim(OAM1);
	}

	void port_DoGroundPound()
	{
		play_sfx(SFX_GROUND_POUND);
		airborne_gfx(ADDR_Frameset_15ff2, ADDR_Frameset_16009);
		port_SetState_SmashAttacking(); // fallthrough
	}

	// the ground shakes for $20 frames, unless it already does
	void land_shake()
	{
		if (!ld_and(wGroundShakeCounter)) {
			mem_.ground_shake_counter() = 32;
			mem_.is_wario_ground_shaking() = TRUE;
		}
	}

	void port_DoWarioGroundShake()
	{
		land_shake();
		if (!ld_and(wFloorTransitionDir))
			play_sfx(SFX_GROUND_SHAKE);
		airborne_gfx(ADDR_Frameset_15fb8, ADDR_Frameset_15fc5);
		port_SetState_SmashAttacking();
	}

	void port_HandleGroundShake()
	{
		if (ld_and(wLevelEndScreen))
			return;
		uint8_t counter = mem_.ground_shake_counter();
		if (!counter) {
			mem_.is_wario_ground_shaking() = 0;
			return;
		}
		// in its first frames, the shake stuns Wario if he's on the ground
		bool stun = false;
		if (counter >= 0x10) {
			stun = true;
			for (uint16_t a : {wIsGettingOffLadder, wIsWarioGroundShaking, wJumpVelTable, wWaterInteraction,
					 wTransformation, wRoomTransitionParam, wIsIntangible, wIsOnSteppableObject})
				if (rd8(a))
					stun = false;
			if (still_in(WST_GROUND_SHAKE_STUNNED))
				stun = false;
		}
		if (!stun) {
			uint8_t left = static_cast<uint8_t>(mem_.ground_shake_counter() - 1);
			mem_.ground_shake_counter() = left;
			mem_.scy_shake() = (left & 0x02) ? 2 : 0;
			return;
		}
		// .wario_shake_stun
		uint8_t state = mem_.wario_state();
		if (state == WST_LADDER_CLIMBING || state == WST_LADDER_IDLING || state == WST_LADDER_SCRATCHING) {
			port_SetState_LadderShakeStunned();
			return;
		}
		mem_.wario_state() = WST_GROUND_SHAKE_STUNNED;
		clear({wJumpVelIndex, wJumpVelTable, wAttackCounter, wIsRolling, wGrabState});
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
		if (!ld_and(wIsCrouching)) {
			mem_.collision_box_top() = static_cast<uint8_t>(-27);
			mem_.jump_vel_index() = 10;
			mem_.jump_vel_table() = JUMP_VEL_KNOCK_BACK;
		}
		mem_.collision_box_bottom() = static_cast<uint8_t>(-1);
		mem_.collision_box_left() = static_cast<uint8_t>(-9);
		mem_.collision_box_right() = 9;
		clear({wWarioStateCounter, wWarioStateCycles, wFrameDuration, wAnimationFrame});
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
		anim(OAM1);
	}

	void port_SetState_Idling()
	{
		mem_.walk_vel_index() = 0;
		mem_.wario_state() = WST_IDLING;
		clear({wFrameDuration, wAnimationFrame, wJumpVelIndex, wJumpVelTable, wWarioStateCounter, wWarioStateCycles,
			wIsSmashAttacking, wIsCrouching, wAttackCounter, wGrabState});
		coll_box_blrt();
		uint8_t hit = up_collision();
		if (!still_in(WST_IDLING))
			return;
		if (hit) {
			port_CrouchOrSlide();
			return;
		}
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	// farcall CheckFrontCollision / ld a, b / and $0f / ret nz
	bool front_blocked() { return front_collision() & 0x0f; }

	void port_WalkIfPossible_Right()
	{
		if (!front_blocked())
			walk_with_velocity(true);
	}

	void port_WalkIfPossible_Left()
	{
		if (!front_blocked())
			walk_with_velocity(false);
	}

	void port_Func_1ec19()
	{
		mem_.jump_vel_table() = JUMP_VEL_KNOCK_BACK;
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		uint8_t hit = up_collision();
		if (!still_in(WST_CROUCHING) && !still_in(WST_CROUCH_WALKING))
			return;
		if (!hit) {
			port_StartCrouchJump();
			return;
		}
		mem_.jump_vel_table() = 0;
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
	}

	void port_Func_1ec4b()
	{
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		if (!up_collision()) {
			port_SetState_Attacking();
			return;
		}
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
	}

	// farcall Func_19b12 / ld a, [wWaterInteraction] / cp NON_SUBMERSIBLE_WATER
	bool non_submersible()
	{
		collision(BANK_Func_19b12, ADDR_Func_19b12, check_func_19b12);
		return mem_.water_interaction() == NON_SUBMERSIBLE_WATER;
	}

	void port_HandleWaterSurfaceInput()
	{
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_A) {
			port_StartJump_FromWaterSurface();
			return;
		}
		if ((pressed & PAD_B) && mem_.power_up_level() >= SWIMMING_FLIPPERS) {
			if (ld_bit(wJoypadDown, B_PAD_UP))
				return;
			if (non_submersible()) {
				port_SetState_TryingSubmerge();
				return;
			}
			move_y(3);
			port_StartUnderwaterThrusting();
			return;
		}
		// .no_thrust
		if (!ld_bit(wJoypadDown, B_PAD_DOWN))
			return;
		if (mem_.power_up_level() < SWIMMING_FLIPPERS || non_submersible()) {
			port_SetState_TryingSubmerge();
			return;
		}
		port_SetState_Submerged();
	}

	void port_HandleBumpMovement()
	{
		if (mem_.jump_vel_index() >= 28) {
			uint8_t pad = mem_.joypad_down() & (PAD_RIGHT | PAD_LEFT);
			if (pad) {
				// the d-pad stops the bump (backing off a wall behind)
				if (pad & PAD_RIGHT) {
					mem_.direction() = DIRECTION_RIGHT;
					if (!side_collision(false))
						move_x(-1);
				} else {
					mem_.direction() = DIRECTION_LEFT;
					if (!side_collision(true))
						move_x(1);
				}
				mem_.walk_vel_index() = 0;
				port_StartFall();
				return;
			}
		}
		// .no_control
		bool right = mem_.direction() == DIRECTION_RIGHT;
		move_x(right ? 1 : -1);
		if (side_collision(right))
			move_x(right ? -1 : 1);
		// .handle_vertical
		jump_vel_move();
	}

	void port_SetState_Bumping()
	{
		mem_.wario_state() = WST_BUMPING;
		flip_dir();
		mem_.jump_vel_table() = JUMP_VEL_KNOCK_BACK;
		collision_box(-1, -27, -9, 9);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wAttackCounter});
		if (!ld_and(wIsRolling)) {
			frameset_by_dir(ADDR_Frameset_15219, ADDR_Frameset_15216);
			anim(OAM1);
			return;
		}
		mem_.is_rolling() = 0;
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		// (the asm calls a bare ret on the way, which only leaves its return
		// address below sp)
		set_frameset(facing_right() ? ADDR_Frameset_15f70 : ADDR_Frameset_15f7f);
		anim(OAM1);
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		if (up_collision())
			mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
	}

	void port_HandleDivingHorizontalMovement()
	{
		if (mem_.power_up_level() < SWIMMING_FLIPPERS)
			return;
		uint8_t pad = mem_.joypad_down();
		bool right;
		if (pad & PAD_RIGHT)
			right = true;
		else if (pad & PAD_LEFT)
			right = false;
		else
			return;
		mem_.direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
		if (front_collision())
			return;
		move_x(right ? 1 : -1);
	}

	// a landing with a smash attack: a ground pound, or with the super jump
	// slam overalls a ground shake; true if it was one
	bool smash_landing()
	{
		if (!mem_.is_smash_attacking())
			return false;
		if (mem_.power_up_level() < SUPER_JUMP_SLAM_OVERALLS)
			port_DoGroundPound();
		else
			port_DoWarioGroundShake();
		return true;
	}

	void port_DoHardLand()
	{
		home::trigger_floor_transition(gb_, false);
		clear({wJumpVelIndex, wJumpVelTable, wFrameDuration, wAnimationFrame, wWalkVelIndex});
		if (smash_landing())
			return;
		if (ld_mask(wJoypadDown, PAD_DOWN)) {
			port_CrouchOrSlide();
			return;
		}
		play_sfx(SFX_LAND);
		mem_.wario_state() = WST_LANDING;
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15f9a, ADDR_Frameset_15fa1);
		anim(OAM1);
	}

	void port_HandleInvincibility()
	{
		uint8_t counter = mem_.invincible_counter();
		if (!counter)
			return;
		if (counter >= 0x16) {
			// .fast
			if (mem_.global_counter() & 0b1)
				return;
			counter++;
			mem_.invincible_counter() = counter;
			if (counter >= 0x28) {
				mem_.invincible_counter() = 0;
				mem_.invisible_frame() = 0;
				if (!ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT | PAD_DOWN))
					up_collision();
				return;
			}
		} else if (counter >= 0x10) {
			// .slow
			if (mem_.global_counter() & 0b11)
				return;
			mem_.invincible_counter() = static_cast<uint8_t>(counter + 1);
		} else {
			if (still_in(WST_STUNG) || still_in(WST_STUNG_RECOVERY))
				return;
			mem_.invincible_counter() = 0x10;
			if (rd8(wWarioPalsPtr + 0) == static_cast<uint8_t>(ADDR_WarioInvisiblePal >> 8) &&
				rd8(wWarioPalsPtr + 1) == static_cast<uint8_t>(ADDR_WarioInvisiblePal))
				set_pal(ADDR_WarioDefaultPal);
			return;
		}
		// .alternate_invisibility
		mem_.invisible_frame() = mem_.invisible_frame() ^ 0x1;
	}

	void port_DoSoftLand()
	{
		home::trigger_floor_transition(gb_, false);
		mem_.jump_vel_index() = 0;
		mem_.jump_vel_table() = 0;
		if (smash_landing())
			return;
		if (ld_mask(wJoypadDown, PAD_DOWN)) {
			port_CrouchOrSlide();
			return;
		}
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		if (up_collision()) {
			port_CrouchOrSlide();
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_Walking();
		else
			port_SetState_Idling();
	}

	void port_WalkRight()
	{
		mem_.is_turning() = 0;
		if (!facing_right()) {
			mem_.direction() = DIRECTION_RIGHT;
			mem_.is_turning() = TRUE;
		}
		port_WalkIfPossible_Right();
	}

	void port_WalkLeft()
	{
		mem_.is_turning() = 0;
		if (facing_right()) {
			mem_.direction() = DIRECTION_LEFT;
			mem_.is_turning() = TRUE;
		}
		port_WalkIfPossible_Left();
	}

	void port_DoBumpOrJumpingBump()
	{
		play_sfx(SFX_BUMP);
		mem_.jump_vel_index() = ld_and(wJumpVelTable) ? FALLING_JUMP_VEL_INDEX : 10;
		port_SetState_Bumping();
	}

	void port_DoJumpingBump_NoSFX()
	{
		mem_.jump_vel_index() = 10;
		port_SetState_Bumping(); // fallthrough
	}

	void port_UpdateState_Turning()
	{
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_A) {
			port_StartJump_FromInput();
			return;
		}
		if (pressed & PAD_B) {
			port_SetState_Attacking();
			return;
		}
		if (!anim_and_ended(OAM1))
			return;
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_Walking_ResetWalkVel();
		else
			port_SetState_Idling();
	}

	// crouched: up out of the crouch unless down is held or a ceiling is low
	// (true if he stood up, or left `state`)
	bool stand_up_unless_low(uint8_t state)
	{
		if (ld_bit(wJoypadDown, B_PAD_DOWN))
			return false;
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		uint8_t hit = up_collision();
		if (!still_in(state))
			return true;
		if (!hit) {
			port_SetState_Idling();
			return true;
		}
		return false;
	}

	void port_Func_1eb46()
	{
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_A) {
			port_Func_1ec19();
			return;
		}
		if (pressed & PAD_B) {
			port_Func_1ec4b();
			return;
		}
		if (stand_up_unless_low(WST_CROUCHING))
			return;
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
		uint8_t pad = mem_.joypad_down() & (PAD_RIGHT | PAD_LEFT);
		if (!pad)
			return;
		mem_.direction() = (pad & PAD_RIGHT) ? DIRECTION_RIGHT : DIRECTION_LEFT;
		port_SetState_CrouchWalking();
	}

	void port_UpdateState_Diving()
	{
		uint8_t hit = centre_collision();
		if (!still_in(WST_DIVING))
			return;
		if (!hit) {
			port_HandleDivingHorizontalMovement();
			uint8_t count = inc_counter();
			if (count >= 14) {
				port_SetState_Submerged();
				return;
			}
			move_y(count < 10 ? 2 : 1);
			if (!centre_collision())
				return;
		}
		// .submerge
		update_pos_y();
		port_SetState_Submerged(); // fallthrough
	}

	void port_Func_1f825()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			mem_.is_rolling() = 0;
			port_StartJump_FromInput();
			return;
		}
		bool right = facing_right();
		if (ld_bit(wJoypadPressed, right ? B_PAD_LEFT : B_PAD_RIGHT)) {
			mem_.is_rolling() = 0;
			port_SetState_Idling();
			return;
		}
		if (right)
			port_WalkIfPossible_Right();
		else
			port_WalkIfPossible_Left();
		cap_walk(0x08, 0x04);
	}

	void port_UpdateState_StungRecovery()
	{
		port_Func_1f6c2();
		anim(OAM1);
		if (inc_counter() < 0x14)
			return;
		flip_dir();
		mem_.invincible_counter() = 0x10;
		set_pal(ADDR_WarioDefaultPal);
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		uint8_t hit = up_collision();
		if (!still_in(WST_STUNG_RECOVERY))
			return;
		if (!hit && !ld_bit(wJoypadDown, B_PAD_DOWN)) {
			port_SetState_Idling();
			return;
		}
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
		port_CrouchOrSlide();
	}

	// the shared tail of UpdateState_PipeGoingDown/Up
	void pipe_counter(bool up)
	{
		uint8_t count = inc_counter();
		if (count == 0x20) {
			home::clear_transformation_values(gb_);
			mem_.room_transition_param() = mem_.room_transition_param() & ~(1 << ROOMTRANSITIONF_DELAY_F);
			home::trigger_room_transition(gb_);
			return;
		}
		if (count == 0x21) {
			play_sfx(SFX_PIPE_EXIT);
			return;
		}
		if (count < 0x40)
			return;
		mem_.wario_x().lo() = static_cast<uint8_t>(mem_.wario_x().lo() - 0x08);
		mem_.is_intangible() = 0;
		if (up)
			home::update_floor(gb_);
		port_SetState_Idling();
	}

	void port_UpdateState_PipeGoingDown()
	{
		mem_.is_intangible() = TRUE;
		anim(OAM1);
		move_y(1);
		pipe_counter(false);
	}

	void port_UpdateState_PipeGoingUp()
	{
		mem_.is_intangible() = TRUE;
		anim(OAM1);
		move_y(-1);
		pipe_counter(true);
	}

	// A held (and not still rising from the jump before) or just pressed
	bool jump_input() const
	{
		return (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A)) || ld_bit(wJoypadPressed, B_PAD_A);
	}

	void port_UpdateState_Landing()
	{
		if (ld_mask(wJoypadPressed, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_Walking_ResetWalkVel();
			return;
		}
		if (!anim_and_ended(OAM1))
			return;
		if (!jump_input()) {
			if (ld_bit(wJoypadDown, B_PAD_DOWN))
				port_CrouchOrSlide();
			else
				port_SetState_Idling();
			return;
		}
		port_StartJump_FromInput();
		if (mem_.power_up_level() >= HIGH_JUMP_BOOTS)
			mem_.jump_vel_table() = JUMP_VEL_HIGH_JUMP;
	}

	void port_HandleInput_Walking()
	{
		if (!ld_and(wJumpingUpwards) && ld_bit(wJoypadDown, B_PAD_A)) {
			port_StartJump_FromInput();
			return;
		}
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_B) {
			port_SetState_Attacking();
			return;
		}
		if (pressed & PAD_A) {
			port_StartJump_FromInput();
			return;
		}
		uint8_t pad = mem_.joypad_down();
		if (pad & PAD_DOWN) {
			if (!on_slippery_ground())
				port_CrouchOrSlide();
			else
				FARCALL(SetState_CrouchSlipping);
			return;
		}
		bool right;
		if (pad & PAD_RIGHT) {
			right = true;
		} else if (pad & PAD_LEFT) {
			right = false;
		} else {
			if (!on_slippery_ground())
				port_SetState_Idling();
			else
				FARCALL(SetState_Slipping);
			return;
		}
		if (facing_right() != right) {
			port_SetState_Turning();
			return;
		}
		mem_.direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
		if (front_collision())
			return;
		walk_with_velocity(right);
		cap_walk(0x10, 0x0c);
	}

	void port_Func_1ec6c()
	{
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_A) {
			port_Func_1ec19();
			return;
		}
		if (pressed & PAD_B) {
			port_Func_1ec4b();
			return;
		}
		if (stand_up_unless_low(WST_CROUCH_WALKING))
			return;
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
		uint8_t pad = mem_.joypad_down();
		bool right;
		if (pad & PAD_RIGHT) {
			right = true;
		} else if (pad & PAD_LEFT) {
			right = false;
		} else {
			port_CrouchOrSlide();
			return;
		}
		if (facing_right() != right) {
			mem_.direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
			reset_anim();
			set_frameset(right ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
		}
		if (front_blocked())
			return;
		walk_with_velocity(right);
		cap_walk(0x04, 0x00);
	}

	void port_SetState_GrabIdling()
	{
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		clear({wJumpVelIndex, wJumpVelTable, wIsSmashAttacking, wIsCrouching, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles});
		grab_state(GRAB_IDLE);
		mem_.wario_state() = WST_GRAB_IDLING;
		coll_box_btlr();
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		frameset_by_dir(ADDR_Frameset_14a20, ADDR_Frameset_14a23);
		anim(OAM1);
	}

	void port_DoBumpWithBigBackOffset()
	{
		move_by_dir(-3);
		port_DoBumpOrJumpingBump();
	}

	void port_DoBumpWithSmallBackOffset()
	{
		move_by_dir(-2);
		port_DoBumpOrJumpingBump();
	}

	void port_DoJumpingBump()
	{
		play_sfx(SFX_BUMP);
		port_DoJumpingBump_NoSFX(); // fallthrough
	}

	void port_UpdateState_Crouching()
	{
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (!still_in(WST_CROUCHING))
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		port_Func_1eb46();
		if (!still_in(WST_CROUCHING))
			return;
		if (!centre_collision())
			port_StartCrouchFall();
	}

	// the room transition, water and sand checks of UpdateState_Bumping/Stung
	// and the like; true if one changed the state
	bool bump_stung_start()
	{
		if (room_transition_check())
			return true;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return true;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return true;
		}
		return false;
	}

	void port_UpdateState_Bumping()
	{
		if (bump_stung_start())
			return;
		if (!still_in(WST_BUMPING))
			return;
		port_HandleBumpMovement();
		if (!still_in(WST_BUMPING))
			return;
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			uint8_t hit = up_collision();
			if (still_in(WST_BUMPING) && hit)
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		update_pos_y();
		flip_dir();
		if (still_in(WST_BUMPING))
			port_DoSoftLand();
	}

	void port_UpdateState_Stung()
	{
		if (bump_stung_start())
			return;
		port_Func_1f6c2();
		anim(OAM1);
		if (!front_collision())
			move_by_dir(mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX ? 2 : 1);
		else
			move_by_dir(-1);
		if (!ld_and(wJumpVelTable)) {
			uint8_t index = static_cast<uint8_t>(mem_.jump_vel_index() + 1);
			mem_.jump_vel_index() = index;
			if (index < MAX_JUMP_VEL_INDEX) {
				if (!(centre_collision() & 0x0f)) {
					mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
					mem_.jump_vel_table() = JUMP_VEL_KNOCK_BACK;
				}
				return;
			}
		} else {
			// the knock back: rising until a ceiling, then falling
			while (jump_vel_move()) {
				if (!up_collision())
					return;
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			}
			if (!airborne_collision()) {
				home::handle_downwards_floor_transition(gb_);
				return;
			}
			home::trigger_floor_transition(gb_);
			// three bounces
			uint8_t bounce = static_cast<uint8_t>(mem_.jump_vel_table() + 1);
			mem_.jump_vel_table() = bounce;
			if (bounce == 2) {
				mem_.jump_vel_index() = 0x0a;
				return;
			}
			if (bounce == 3) {
				mem_.jump_vel_index() = 0x0e;
				return;
			}
		}
		mem_.wario_state_counter() = 0;
		mem_.wario_state_cycles() = 0;
		mem_.wario_state() = WST_STUNG_RECOVERY;
	}

	void port_UpdateState_EnemyBumping()
	{
		if (bump_stung_start())
			return;
		if (!still_in(WST_ENEMY_BUMPING))
			return;
		anim(OAM1);
		port_HandleBumpMovement();
		if (!still_in(WST_ENEMY_BUMPING))
			return;
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			if (up_collision())
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		flip_dir();
		update_pos_y();
		port_DoSoftLand();
	}

	void port_UpdateState_EnteringDoor()
	{
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		switch (mem_.wario_state_counter()) {
		case 1:
			if (anim_ended()) {
				inc_counter();
				mem_.room_transition_param() = mem_.room_transition_param() & ~(1 << ROOMTRANSITIONF_DELAY_F);
				home::trigger_room_transition(gb_);
			}
			return;
		case 2:
			inc_counter();
			return;
		case 3:
			if (!anim_ended())
				return;
			if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
				port_SetState_Walking_ResetWalkVel();
			else
				port_SetState_Idling();
			return;
		}
		if (!anim_ended())
			return;
		reset_anim();
		set_frameset(ADDR_Frameset_14d15);
		anim(OAM1);
		inc_counter();
	}

	// a water current's or the d-pad's direction
	enum Dir { RIGHT, LEFT, UP, DOWN };
	// the collision check that way, the result in b
	uint8_t dir_collision(Dir d)
	{
		switch (d) {
		case RIGHT: return side_collision(true);
		case LEFT: return side_collision(false);
		case UP: return up_collision();
		default: return centre_collision();
		}
	}
	// MoveWarioRight / Left / Up / Down
	void dir_move(Dir d, int n)
	{
		switch (d) {
		case RIGHT: move_x(n); break;
		case LEFT: move_x(-n); break;
		case UP: move_y(-n); break;
		default: move_y(n); break;
		}
	}

	// A water current pushes Wario `step` its way (d, the pad bit `with`),
	// unless he's against something that way, swims with it already, or
	// against it with the prince frog's gloves. True to stop the current.
	bool water_current_push(Dir d, int with, int against, int step)
	{
		if (ld_bit(wc0e1, with))
			return true;
		if (dir_collision(d))
			return true;
		uint8_t in = mem_.swimming_direction_input();
		if (in >> with & 1)
			return false;
		if ((in >> against & 1) && mem_.power_up_level() >= PRINCE_FROGS_GLOVES)
			return false;
		dir_move(d, step);
		return false;
	}

	void port_ApplyLastWaterCurrentMovement()
	{
		uint8_t last = mem_.last_water_current();
		if (!last)
			return;
		bool stop;
		if (last >> CURRENT_RIGHT_F & 1)
			stop = water_current_push(RIGHT, B_PAD_RIGHT, B_PAD_LEFT, 1);
		else if (last >> CURRENT_LEFT_F & 1)
			stop = water_current_push(LEFT, B_PAD_LEFT, B_PAD_RIGHT, 1);
		else if (last >> CURRENT_UP_F & 1)
			stop = water_current_push(UP, B_PAD_UP, B_PAD_DOWN, 1);
		else if (last >> CURRENT_DOWN_F & 1)
			stop = water_current_push(DOWN, B_PAD_DOWN, B_PAD_UP, 1);
		else
			return;
		if (!stop) {
			// it lingers for the frames in the high nibble
			uint8_t v = static_cast<uint8_t>(mem_.last_water_current() - (1 << 4));
			mem_.last_water_current() = v;
			if (v & 0xf0)
				return;
		}
		mem_.last_water_current() = 0;
	}

	void port_HandleInput_Airborne()
	{
		if (!ld_bit(wJoypadDown, B_PAD_A)) {
			mem_.jumping_upwards() = 0;
			if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX && !ld_and(wDoFullJump))
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		}
		uint16_t table;
		switch (mem_.jump_vel_table()) {
		case 1: table = ADDR_JumpVelTable_KnockBack; break;
		case 2: table = ADDR_JumpVelTable_Normal; break;
		default: table = ADDR_JumpVelTable_HighJump; break;
		}
		if (!(jump_vel(table) & 0x80))
			mem_.do_full_jump() = 0;
		jump_vel_move(table);
		uint8_t pad = mem_.joypad_down();
		if (pad & PAD_RIGHT)
			port_WalkRight();
		else if (pad & PAD_LEFT)
			port_WalkLeft();
		else
			return;
		home::cap_walk_vel(gb_); // Func_1762
	}

	void port_UpdateState_SmashAttacking()
	{
		anim(OAM1);
		if (inc_counter() < 0x1b)
			return;
		mem_.is_smash_attacking() = 0;
		port_SetState_Idling();
	}

	// the shared body of UpdateState_Throwing/ThrowingAirborne
	void throwing(void (WarioStates1::*done)())
	{
		if (!ld_and(wGrabState)) {
			(this->*done)();
			return;
		}
		if (!anim_and_ended(OAM1))
			return;
		mem_.grab_state() = 0;
		(this->*done)();
	}

	void port_UpdateState_Throwing() { throwing(&WarioStates1::port_SetState_Idling); }
	void port_UpdateState_ThrowingAirborne() { throwing(&WarioStates1::port_StartFall); }

	void port_Func_1dd78()
	{
		mem_.grab_state() = 0;
		port_StartJump_FromInput();
	}

	void port_UpdateState_SwimKnockBack()
	{
		if (!front_collision()) {
			uint8_t count = inc_counter();
			if (count < 0x04) {
				move_by_dir(2);
				return;
			}
			if (count < 0x08) {
				move_by_dir(1);
				return;
			}
		}
		flip_dir();
		port_SetState_Submerged();
	}

	// xor a / ld [wIsIntangible], a / ld a, $10 / ld [wInvincibleCounter], a / jp
	void water_stung_end(void (WarioStates1::*to)())
	{
		mem_.is_intangible() = 0;
		mem_.invincible_counter() = 0x10;
		(this->*to)();
	}

	void port_UpdateState_WaterStung()
	{
		anim(OAM1);
		uint8_t count = inc_counter();
		if (up_collision()) {
			if (count < 0x78)
				return;
			mem_.unk_c0e1() = PAD_UP;
			water_stung_end(&WarioStates1::port_SetState_Submerged);
			return;
		}
		collision(BANK_Func_19acd, ADDR_Func_19acd, check_func_19acd);
		if (!mem_.water_interaction()) {
			if (count >= 0x78)
				water_stung_end(&WarioStates1::port_SurfaceFromUnderwater);
			return;
		}
		if (count >= 0x78) {
			water_stung_end(&WarioStates1::port_SetState_Submerged);
			return;
		}
		if (mem_.global_counter() & 1)
			return;
		move_y(-1);
		if (!mem_.water_current())
			return;
		mem_.water_current() = 0;
		water_stung_end(&WarioStates1::port_SetState_Submerged);
	}

	void port_Func_1f310()
	{
		if (front_collision()) {
			mem_.unk_c0e1() = facing_right() ? PAD_RIGHT : PAD_LEFT;
			return;
		}
		bool right = facing_right();
		if (!water_current_mask(CURRENT_RIGHT | CURRENT_LEFT))
			move_x(right ? 1 : -1);
	}

	void port_Func_1f3d9()
	{
		if (up_collision())
			return;
		collision(BANK_Func_19acd, ADDR_Func_19acd, check_func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		if (!water_current_mask(CURRENT_UP | CURRENT_DOWN))
			move_y(-1);
	}

	// ApplyWaterCurrentMovement.ChangeCurrentDirection
	void change_current_direction()
	{
		if (!ld_and(wLastWaterCurrent))
			mem_.last_water_current() = static_cast<uint8_t>(mem_.cur_water_current() + (15 << 4));
	}

	// one direction of ApplyWaterCurrentMovement
	void water_current(int bit, uint8_t others, Dir d, int with, int against)
	{
		if (mem_.cur_water_current() & others)
			change_current_direction();
		mem_.cur_water_current() = mem_.water_current();
		mem_.water_current() = static_cast<uint8_t>(mem_.water_current() & ~(1 << bit));
		water_current_push(d, with, against, 2);
	}

	void port_ApplyWaterCurrentMovement()
	{
		port_ApplyLastWaterCurrentMovement();
		uint8_t current = mem_.water_current();
		if (current >> CURRENT_RIGHT_F & 1) {
			water_current(CURRENT_RIGHT_F, CURRENT_LEFT | CURRENT_UP | CURRENT_DOWN, RIGHT, B_PAD_RIGHT, B_PAD_LEFT);
		} else if (current >> CURRENT_LEFT_F & 1) {
			water_current(CURRENT_LEFT_F, CURRENT_RIGHT | CURRENT_UP | CURRENT_DOWN, LEFT, B_PAD_LEFT, B_PAD_RIGHT);
		} else if (current >> CURRENT_UP_F & 1) {
			water_current(CURRENT_UP_F, CURRENT_RIGHT | CURRENT_LEFT | CURRENT_DOWN, UP, B_PAD_UP, B_PAD_DOWN);
		} else if (current >> CURRENT_DOWN_F & 1) {
			water_current(CURRENT_DOWN_F, CURRENT_RIGHT | CURRENT_LEFT | CURRENT_UP, DOWN, B_PAD_DOWN, B_PAD_UP);
		} else {
			if (mem_.cur_water_current())
				change_current_direction();
			mem_.cur_water_current() = 0;
		}
	}

	void port_HandleAttackingAirborneMovement()
	{
		home::apply_jump_velocity(gb_);
		FARCALL(HandleWalk);
		cap_walk(0x14, 0x10);
	}

	// update_pos: Wario's position from hPos
	void update_pos()
	{
		for (int i = 0; i < 4; i++)
			wr8(wWarioPos + i, rd8(hPos + i));
	}

	void port_UpdateState_Walking()
	{
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		if (anim_ended())
			particle_at_wario(PARTICLE_DUST);
		port_HandleInput_Walking();
		if (!still_in(WST_WALKING))
			return;
		if (room_transition_check())
			return;
		if (!still_in(WST_WALKING))
			return;
		uint8_t hit = centre_collision();
		if (!still_in(WST_WALKING))
			return;
		if (!hit) {
			port_StartFall();
			return;
		}
		update_pos_y();
	}

	// the framesets of UpdateState_Airborne and UpdateState_GrabAirborne
	struct AirborneFramesets {
		uint16_t left, right; // not smash attacking
		uint16_t smash_left, smash_right, super_smash_left, super_smash_right;
	};

	// the smash attack checks after HandleInput_Airborne, up to the
	// UpdateWarioAnimation call
	void airborne_smash_attack(const AirborneFramesets &f)
	{
		enum { CONTINUE, SET_SMASH, NO_SMASH } next = CONTINUE;
		if (mem_.power_up_level() >= LEAD_OVERALLS) {
			if (ld_and(wIsSmashAttacking)) {
				// .check_still_smash_attacking
				if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
					mem_.is_smash_attacking() = FALSE;
					next = NO_SMASH;
				}
			} else if (mem_.jump_vel_index() >= FALLING_JUMP_VEL_INDEX && ld_bit(wJoypadDown, B_PAD_DOWN)) {
				mem_.is_smash_attacking() = TRUE;
				next = SET_SMASH;
			}
		}
		if (next == CONTINUE && ld_and(wIsTurning))
			next = ld_and(wIsSmashAttacking) ? SET_SMASH : NO_SMASH;
		if (next == SET_SMASH) {
			reset_anim();
			if (mem_.power_up_level() < SUPER_JUMP_SLAM_OVERALLS)
				frameset_by_dir(f.smash_left, f.smash_right);
			else
				frameset_by_dir(f.super_smash_left, f.super_smash_right);
		} else if (next == NO_SMASH) {
			reset_anim();
			frameset_by_dir(f.left, f.right);
		}
		anim(OAM1);
	}

	void port_UpdateState_Airborne()
	{
		if (room_transition_check())
			return;
		if (mem_.jump_vel_index() >= FALLING_JUMP_VEL_INDEX) {
			// falling
			if (ld_and(wWaterInteraction)) {
				port_DiveFromSurface();
				return;
			}
			if (ld_and(wIsInSand)) {
				FARCALL(SetState_SandFalling);
				return;
			}
			if (ld_and(wLadderInteraction) && ld_bit(wJoypadDown, B_PAD_UP)) {
				FARCALL(SetState_LadderClimbing);
				return;
			}
			if (!still_in(WST_AIRBORNE))
				return;
			collision(BANK_Func_19afb, ADDR_Func_19afb, check_func_19afb);
			if (ld_and(wIsNearFence) && ld_bit(wJoypadDown, B_PAD_UP)) {
				FARCALL(SetState_FenceMovingVertical);
				return;
			}
		}
		// .handle_input
		if (!still_in(WST_AIRBORNE))
			return;
		port_HandleInput_Airborne();
		if (!still_in(WST_AIRBORNE))
			return;

		airborne_smash_attack({ADDR_Frameset_15f94, ADDR_Frameset_15f97, ADDR_Frameset_15f8e, ADDR_Frameset_15f91,
			ADDR_Frameset_1604e, ADDR_Frameset_16055});
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			if (!up_collision() || !still_in(WST_AIRBORNE))
				return;
			if (ld_and(wJumpCollision)) {
				port_StartFall_SkipResetJumpUpwards();
				return;
			}
			center_on_ladder();
			return;
		}
		// .falling
		uint8_t hit = airborne_collision();
		if (!still_in(WST_AIRBORNE))
			return;
		if (!hit) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		update_pos_y();
		particle_at_wario(PARTICLE_DUST);
		if (mem_.jump_vel_index() >= MAX_JUMP_VEL_INDEX && !ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_DoHardLand();
		else
			port_DoSoftLand();
	}

	void port_UpdateState_CrouchWalking()
	{
		sfx_loop(21, SFX_CROUCH_WALK);
		anim(OAM1);
		if (room_transition_check())
			return;
		if (!still_in(WST_CROUCH_WALKING))
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		port_Func_1ec6c();
		if (!still_in(WST_CROUCH_WALKING))
			return;
		if (!centre_collision()) {
			port_StartCrouchFall();
			return;
		}
		update_pos_y();
		if (ld_and(wWarioSlopeInteraction))
			port_SetState_Sliding();
	}

	void port_UpdateState_CrouchAirborne()
	{
		if (room_transition_check())
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return;
		}
		if (!still_in(WST_CROUCH_AIRBORNE))
			return;
		if (!ld_bit(wJoypadDown, B_PAD_DOWN)) {
			mem_.collision_box_top() = static_cast<uint8_t>(-27);
			uint8_t hit = up_collision();
			if (!still_in(WST_CROUCH_AIRBORNE))
				return;
			if (!hit) {
				port_StartJump();
				return;
			}
		}
		// .asm_1d583
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
		port_HandleInput_Airborne();
		if (!still_in(WST_CROUCH_AIRBORNE))
			return;
		if (ld_and(wIsTurning)) {
			reset_anim();
			frameset_by_dir(ADDR_Frameset_14a38, ADDR_Frameset_14a3b);
			anim(OAM1);
		}
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			// rising
			if (up_collision() && still_in(WST_CROUCH_AIRBORNE))
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		// .falling
		if (!airborne_collision()) {
			if (still_in(WST_CROUCH_AIRBORNE))
				home::handle_downwards_floor_transition(gb_);
			return;
		}
		update_pos_y();
		if (!still_in(WST_CROUCH_AIRBORNE))
			return;
		port_DoSoftLand();
		if (ld_bit(wJoypadDown, B_PAD_DOWN))
			port_CrouchOrSlide();
	}

	void port_UpdateState_Sliding()
	{
		sfx_loop(12, SFX_ZIP_LINE_SLIDE);
		anim(OAM1);
		port_Func_1f825();
		if (!still_in(WST_SLIDING))
			return;
		// the ground below, by Func_198c7 (the result in b)
		if (!collision(BANK_Func_198c7, ADDR_Func_198c7, check_func_198c7)) {
			port_Func_1e174();
			return;
		}
		update_pos();
		if (front_blocked()) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
		if (anim_ended())
			port_SetState_Rolling(); // fallthrough
	}

	// move by direction with the walk velocity, then cap it at $18 (to $14)
	void roll_move()
	{
		walk_with_velocity(facing_right());
		cap_walk(0x18, 0x14);
	}

	// on a slope down the other way (rolling against it)
	bool against_slope() const
	{
		uint8_t slope = mem_.wario_slope_interaction();
		return slope && (static_cast<uint8_t>((slope & 0x0f) - 1) ^ mem_.direction());
	}

	void port_UpdateState_Rolling()
	{
		if (room_transition_check())
			return;
		sfx_loop(12, SFX_ROLL);
		anim(OAM1);
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			// .asm_1e145
			centre_collision();
			update_pos_y();
			if (against_slope())
				port_DoBumpWithSmallBackOffset();
			else
				port_Func_1e1e3();
			return;
		}
		roll_move();
		if (front_blocked()) {
			port_DoBumpWithBigBackOffset();
			return;
		}
		if (!centre_collision()) {
			port_Func_1e174();
			return;
		}
		update_pos_y();
		if (against_slope())
			port_DoBumpWithSmallBackOffset();
	}

	void port_UpdateState_RollingAirborne()
	{
		if (bump_stung_start())
			return;
		anim(OAM1);
		roll_move();
		if (front_blocked()) {
			port_DoBumpWithBigBackOffset();
			return;
		}
		jump_vel_move(ADDR_JumpVelTable_HighJump);
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			if (up_collision())
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		port_SetState_Rolling();
	}

	void port_UpdateState_PickingUp()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1dd78();
			return;
		}
		uint8_t grab = mem_.grab_state();
		if (!grab) {
			port_SetState_Idling();
			return;
		}
		if ((grab & 0x0f) == 0x02 || anim_and_ended(OAM1))
			port_SetState_GrabIdling();
	}

	void port_UpdateState_ThrowCharging()
	{
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		uint8_t pad = mem_.joypad_down();
		if (!(pad & PAD_B)) {
			port_Func_1de3f();
			return;
		}
		if (pad & PAD_A) {
			port_Func_1dd78();
			return;
		}
		if (pad & PAD_DOWN) {
			port_CrouchOrSlide();
			return;
		}
		uint8_t frames = ld_bit(wGrabState, GRAB_HEAVY_F) ? THROW_CHARGE_FRAMES_HEAVY : THROW_CHARGE_FRAMES_LIGHT;
		if (inc_counter() < frames)
			return;
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
		mem_.wario_state() = WST_THROW_FULLY_CHARGED;
		grab_state(GRAB_THROW_FULLY_CHARGED);
		frameset_by_dir(ADDR_Frameset_16432, ADDR_Frameset_16435);
		anim(OAM1);
	}

	void port_UpdateState_ThrowFullyCharged()
	{
		sfx_loop(30, SFX_FULLY_CHARGED_THROW);
		uint8_t grab = mem_.grab_state();
		if (!grab) {
			port_SetState_Idling();
			return;
		}
		if ((grab & GRAB_STATE_MASK) != GRAB_HOLD_FULL_CHARGE && first_anim_ended()) {
			grab_state(GRAB_HOLD_FULL_CHARGE);
			reset_anim();
			mem_.wario_state_counter() = 1;
			frameset_by_dir(ADDR_Frameset_16438, ADDR_Frameset_16441);
		}
		anim(OAM1);
		uint8_t pad = mem_.joypad_down();
		if (pad & PAD_B) {
			if (pad & PAD_A)
				port_Func_1dd78();
			else if (pad & PAD_DOWN)
				port_CrouchOrSlide();
			return;
		}
		reset_anim();
		frameset_by_dir(ADDR_Frameset_16403, ADDR_Frameset_16423);
		anim(OAM1);
		grab_state(GRAB_THROW | (1 << GRAB_FULL_CHARGE_F));
		port_SetState_Throwing();
	}

	// the state counter at 0 and the animation ended
	bool first_anim_ended() const { return !ld_and(wWarioStateCounter) && anim_ended(); }

	void port_UpdateState_GrabSmashAttacking()
	{
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		anim(OAM1);
		if (inc_counter() < 0x1b)
			return;
		mem_.is_smash_attacking() = FALSE;
		port_SetState_GrabIdling();
	}

	void port_UpdateState_GroundShakeStunned()
	{
		anim(OAM1);
		if (!mem_.ground_shake_counter()) {
			// .no_more_ground_shake
			mem_.collision_box_top() = static_cast<uint8_t>(-27);
			if (up_collision())
				port_CrouchOrSlide();
			else if (!centre_collision())
				port_StartFall();
			else
				port_SetState_Idling();
			return;
		}
		if (!ld_and(wJumpVelTable))
			return;
		home::apply_jump_velocity(gb_);
		FARCALL(HandleWalk);
		home::cap_walk_vel(gb_); // Func_1762
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			if (up_collision())
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		if (!centre_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		mem_.jump_vel_table() = 0;
		mem_.jump_vel_index() = 0;
	}

	// UpdateState_CrouchSliding.end_slide
	void crouch_slide_end()
	{
		if (stand_up_unless_low(WST_CROUCH_SLIDING))
			return;
		// .crouching
		mem_.collision_box_top() = static_cast<uint8_t>(-15);
		port_CrouchOrSlide();
	}

	void port_UpdateState_CrouchSliding()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_CROUCH_SLIDING))
			return;
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			// .try_jump
			mem_.collision_box_top() = static_cast<uint8_t>(-27);
			uint8_t blocked = up_collision();
			if (!still_in(WST_CROUCH_SLIDING))
				return;
			mem_.collision_box_top() = static_cast<uint8_t>(-15);
			if (!blocked)
				port_StartCrouchJump();
			return;
		}
		uint8_t hit = centre_collision();
		if (!still_in(WST_CROUCH_SLIDING))
			return;
		if (!hit) {
			port_StartCrouchFall();
			return;
		}
		if (ld_and(wWarioSlopeInteraction)) {
			update_pos_y();
			port_SetState_Sliding();
			return;
		}
		hit = front_collision();
		if (!still_in(WST_CROUCH_SLIDING))
			return;
		if (hit) {
			crouch_slide_end();
			return;
		}
		// if Wario has Garlic power-up, then slide further
		uint8_t count = inc_counter();
		bool garlic = mem_.power_up_level() >= GARLIC;
		int step;
		if (count < (garlic ? 0x1e : 0x14)) {
			step = 2; // .slide_fast
		} else if (count < (garlic ? 0x36 : 0x24)) {
			step = 1; // .slide_slow
		} else if (count < (garlic ? 0x3c : 0x28)) {
			// .slide_slower
			if (!(mem_.global_counter() & 0x1))
				return;
			step = 1;
		} else {
			if (count == (garlic ? 0x3c : 0x28))
				crouch_slide_end();
			return;
		}
		move_by_dir(step);
	}

	void port_UpdateState_GrabAirborne()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_AIRBORNE))
			return;
		if (ld_and(wWaterInteraction)) {
			port_DiveFromSurface();
			return;
		}
		if (!ld_and(wGrabState)) {
			port_StartFall();
			return;
		}
		if (ld_and(wIsInSand)) {
			FARCALL(SetState_SandFalling);
			return;
		}
		if (ld_bit(wJoypadPressed, B_PAD_B)) {
			port_SetState_ThrowingAirborne();
			return;
		}
		port_HandleInput_Airborne();
		if (!still_in(WST_GRAB_AIRBORNE))
			return;
		airborne_smash_attack({ADDR_Frameset_15fa8, ADDR_Frameset_15fab, ADDR_Frameset_15fec, ADDR_Frameset_15fef,
			ADDR_Frameset_1605c, ADDR_Frameset_16063});

		if (ld_bit(wGrabState, GRAB_HEAVY_F))
			cap_walk(0x08, 0x04);
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			if (up_collision())
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		uint8_t hit = airborne_collision();
		if (!still_in(WST_GRAB_AIRBORNE))
			return;
		if (!hit) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (!mem_.floor_transition_dir())
			play_sfx(SFX_LAND);
		clear({wJumpVelIndex, wJumpVelTable, wWalkVelIndex});
		if (!ld_and(wIsSmashAttacking)) {
			port_SetState_GrabIdling();
			return;
		}
		if (mem_.power_up_level() < SUPER_JUMP_SLAM_OVERALLS) {
			// .asm_1dc25
			play_sfx(SFX_GROUND_POUND);
			reset_anim();
			frameset_by_dir(ADDR_Frameset_16020, ADDR_Frameset_16037);
			anim(OAM1);
		} else {
			// .continue
			land_shake();
			if (!mem_.floor_transition_dir())
				play_sfx(SFX_GROUND_SHAKE);
			reset_anim();
			frameset_by_dir(ADDR_Frameset_15fd2, ADDR_Frameset_15fdf);
			anim(OAM1);
		}
		// .asm_1dc5f
		mem_.is_smash_attacking() = 0x81;
		mem_.wario_state() = WST_GRAB_SMASH_ATTACKING;
		clear({wWarioStateCounter, wWarioStateCycles, wJumpVelIndex, wJumpVelTable});
		collision_box(-1, -27, -9, 9);
	}

	// swimming sideways: onto the ground, then Func_1f310's move while facing
	// `right`, else turning with the frameset
	void swim_sideways(bool right, uint16_t turn_frameset)
	{
		if (centre_collision())
			update_pos_y();
		if (still_in(WST_WATER_STUNG))
			return;
		if (facing_right() == right) {
			port_Func_1f310();
			return;
		}
		mem_.direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
		reset_anim();
		set_frameset(turn_frameset);
		anim(OAM1);
	}

	void port_Func_1f357() { swim_sideways(false, ADDR_Frameset_15576); }

	// the swim velocity, by wSwimVelIndex, from the `alt` table when
	// wSwimmingDirectionInput has `mask`, else SwimStraightVelTable
	uint8_t swim_vel(uint8_t mask, uint16_t alt) const
	{
		uint16_t table = ld_mask(wSwimmingDirectionInput, mask) ? alt : ADDR_SwimStraightVelTable;
		return rd8(static_cast<uint16_t>(table + mem_.swim_vel_index()));
	}

	void set_c0e1(int bit) { mem_.unk_c0e1() = static_cast<uint8_t>(mem_.unk_c0e1() | (1 << bit)); }

	// HandleUnderwaterThrustingInput.up
	void thrust_up()
	{
		if (up_collision()) {
			set_c0e1(B_PAD_UP);
			port_StartDive();
			return;
		}
		collision(BANK_Func_19acd, ADDR_Func_19acd, check_func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		move_y(-swim_vel(PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894));
	}

	// HandleUnderwaterThrustingInput.down
	void thrust_down()
	{
		if (centre_collision()) {
			update_pos_y();
			set_c0e1(B_PAD_DOWN);
			return;
		}
		move_y(swim_vel(PAD_RIGHT | PAD_LEFT, ADDR_Data_1f894));
	}

	// HandleUnderwaterThrustingInput.right / .left
	void thrust_horizontal(bool right)
	{
		if (centre_collision())
			update_pos_y();
		if (still_in(WST_WATER_STUNG))
			return;
		if (side_collision(right)) {
			port_SetState_SwimKnockBack_SwitchDirection();
			return;
		}
		uint8_t v = swim_vel(PAD_UP | PAD_DOWN, ADDR_SwimDiagonalVelTable);
		move_x(right ? v : -v);
		uint8_t in = mem_.swimming_direction_input();
		if (in & PAD_UP) {
			thrust_up();
			return;
		}
		if (in & PAD_DOWN) {
			thrust_down();
			return;
		}
		// .handle_diagonal_movement
		uint8_t pad = mem_.joypad_down() & (PAD_UP | PAD_DOWN);
		if (!pad)
			return;
		if (pad & PAD_DOWN) {
			// .diagonal_down
			if (!centre_collision()) {
				move_y(1);
				return;
			}
			update_pos_y();
			set_c0e1(B_PAD_DOWN);
			return;
		}
		// diagonal up
		if (up_collision()) {
			set_c0e1(B_PAD_UP);
			return;
		}
		collision(BANK_Func_19acd, ADDR_Func_19acd, check_func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		move_y(-1);
	}

	void port_HandleUnderwaterThrustingInput()
	{
		uint8_t index = static_cast<uint8_t>(mem_.swim_vel_index() + 1);
		mem_.swim_vel_index() = index;
		if (index >= 40) {
			mem_.swim_vel_index() = 0;
			port_SetState_Submerged();
			return;
		}
		if (!ld_mask(wJoypadDown, PAD_A | PAD_B)) {
			port_SetState_Submerged();
			return;
		}
		uint8_t in = mem_.swimming_direction_input();
		if (in & PAD_RIGHT)
			thrust_horizontal(true);
		else if (in & PAD_LEFT)
			thrust_horizontal(false);
		else if (in & PAD_UP)
			thrust_up();
		else if (in & PAD_DOWN)
			thrust_down();
	}

	void port_UpdateState_AttackingAirborne()
	{
		if (bump_stung_start())
			return;
		if (!still_in(WST_ATTACKING_AIRBORNE))
			return;
		sfx_loop(6, SFX_ATTACK);
		anim(OAM1);
		port_HandleAttackingAirborneMovement();
		if (!still_in(WST_ATTACKING_AIRBORNE))
			return;
		uint8_t hit = front_collision();
		if (!still_in(WST_ATTACKING_AIRBORNE))
			return;
		if (hit & 0x0f) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
		uint8_t attack = static_cast<uint8_t>(mem_.attack_counter() - 1);
		if (attack)
			mem_.attack_counter() = attack;
		if (ld_bit(wJoypadPressed, facing_right() ? B_PAD_LEFT : B_PAD_RIGHT)) {
			mem_.attack_counter() = 0;
			port_StartFall();
			return;
		}
		if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX) {
			if (!up_collision() || !still_in(WST_ATTACKING_AIRBORNE))
				return;
			if (mem_.jump_vel_index() < FALLING_JUMP_VEL_INDEX)
				mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
			return;
		}
		if (!airborne_collision()) {
			home::handle_downwards_floor_transition(gb_);
			return;
		}
		home::trigger_floor_transition(gb_);
		if (still_in(WST_ATTACKING_AIRBORNE))
			port_SetState_Idling();
	}

	void port_UpdateState_WaterSurfaceIdling()
	{
		mem_.unk_c0e1() = 0;
		collision(BANK_Func_19b25, ADDR_Func_19b25, check_func_19b25);
		if (!still_in(WST_WATER_SURFACE_IDLING))
			return;
		if (room_transition()) {
			home::trigger_room_transition(gb_);
			return;
		}
		sfx_loop(28, SFX_WATER_SURFACE);
		anim(OAM1);
		port_HandleWaterSurfaceFloatingMovement();
		port_HandleWaterSurfaceInput();
		if (!still_in(WST_WATER_SURFACE_IDLING))
			return;
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
			port_SetState_WaterSurfaceMoving();
			return;
		}
		port_ApplyWaterCurrentMovement();
	}

	// UpdateState_WaterSurfaceMoving.HandleDirectionalInput
	void water_surface_directional_input()
	{
		uint8_t pad = mem_.joypad_down();
		bool right;
		if (pad & PAD_RIGHT) {
			right = true;
		} else if (pad & PAD_LEFT) {
			right = false;
		} else {
			port_SetState_WaterSurfaceIdling(); // jp from a called routine
			return;
		}
		if (facing_right() != right) {
			mem_.direction() = right ? DIRECTION_RIGHT : DIRECTION_LEFT;
			reset_anim();
			set_frameset(right ? ADDR_Frameset_1555b : ADDR_Frameset_15554);
			anim(OAM1);
			return;
		}
		if (front_collision())
			return;
		// current overrides player input
		if (water_current_mask(CURRENT_RIGHT | CURRENT_LEFT))
			return;
		walk_with_velocity(right);
		// .done_moving
		cap_walk(0x04, 0x00);
	}

	void port_UpdateState_WaterSurfaceMoving()
	{
		mem_.unk_c0e1() = 0;
		if (room_transition_check())
			return;
		sfx_loop(28, SFX_WATER_SURFACE);
		anim(OAM1);
		port_HandleWaterSurfaceFloatingMovement();
		port_ApplyWaterCurrentMovement();
		water_surface_directional_input();
		port_HandleWaterSurfaceInput();
	}

	void port_UpdateState_UnderwaterThrusting()
	{
		mem_.unk_c0e1() = 0;
		if (room_transition_check())
			return;
		if (!still_in(WST_UNDERWATER_THRUSTING))
			return;
		sfx_loop(15, SFX_00F);
		anim(OAM1);
		port_HandleUnderwaterThrustingInput();
		if (still_in(WST_UNDERWATER_THRUSTING))
			port_ApplyWaterCurrentMovement();
	}

	void port_SetState_GrabAirborne()
	{
		mem_.wario_state() = WST_GRAB_AIRBORNE;
		collision_box(-1, -27, -9, 9);
		clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles});
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		frameset_by_dir(ADDR_Frameset_15fa8, ADDR_Frameset_15fab);
		anim(OAM1);
		if (ld_and(wIsOnSteppableObject))
			port_UpdateState_GrabAirborne();
	}

	void port_Func_1f24c()
	{
		sfx_loop(37, SFX_00E);
		if (ld_and(wWarioStateCycles)) {
			clear({wWarioStateCycles, wFrameDuration, wAnimationFrame});
			frameset_by_dir(ADDR_Frameset_15576, ADDR_Frameset_1558b);
		}
		anim(OAM1);
		uint8_t pad = mem_.joypad_down();
		if (!(mem_.global_counter() & 0x1)) {
			if (pad & PAD_UP)
				port_Func_1f3d9();
			else if (pad & PAD_DOWN)
				port_Func_1f3a7();
			return;
		}
		if (pad & PAD_RIGHT)
			swim_sideways(true, ADDR_Frameset_1558b); // .asm_1f2c0
		else if (pad & PAD_LEFT)
			port_Func_1f357();
	}

	void port_SetState_AttackingAirborne()
	{
		mem_.wario_slope_interaction() = 0;
		mem_.wario_state() = WST_ATTACKING_AIRBORNE;
		reset_anim();
		mem_.jump_vel_table() = mem_.power_up_level() >= HIGH_JUMP_BOOTS ? JUMP_VEL_NORMAL + 1 : JUMP_VEL_NORMAL;
		if (mem_.power_up_level() < GARLIC)
			frameset_by_dir(ADDR_Frameset_15216, ADDR_Frameset_15219);
		else
			frameset_by_dir(ADDR_Frameset_1523e, ADDR_Frameset_15249);
		anim(OAM1);
		if (ld_and(wIsOnSteppableObject))
			port_UpdateState_AttackingAirborne();
	}

	void port_Func_1edd3()
	{
		grab_state(GRAB_IDLE);
		mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		mem_.jump_vel_table() = JUMP_VEL_HIGH_JUMP;
		port_SetState_GrabAirborne();
	}

	void port_Func_1ede9()
	{
		play_sfx(SFX_JUMP);
		mem_.jump_vel_index() = 0;
		mem_.is_smash_attacking() = 0;
		mem_.jump_vel_table() = jump_table(JUMP_VEL_NORMAL, JUMP_VEL_HIGH_JUMP);
		port_SetState_GrabAirborne(); // fallthrough
	}

	void port_Func_1f1a9()
	{
		bool skip = true;
		if (mem_.water_interaction() != NON_SUBMERSIBLE_WATER && mem_.power_up_level() >= SWIMMING_FLIPPERS) {
			uint8_t pressed = mem_.joypad_pressed();
			if (pressed & PAD_B) {
				port_StartUnderwaterThrusting();
				return;
			}
			if (pressed & PAD_A) {
				port_StartUpwardsUnderwaterThrusting();
				return;
			}
			if (ld_mask(wJoypadDown, PAD_CTRL_PAD)) {
				port_Func_1f24c();
				return;
			}
			skip = ld_and(wWarioStateCycles);
		}
		if (!skip) {
			reset_anim();
			frameset_by_dir(ADDR_Frameset_15569, ADDR_Frameset_15573);
		}
		// .asm_1f1f5
		anim(OAM1);
		mem_.wario_state_cycles() = 0x01;
		if (up_collision()) {
			mem_.unk_c0e1() = PAD_UP;
			return;
		}
		collision(BANK_Func_19acd, ADDR_Func_19acd, check_func_19acd);
		if (!ld_and(wWaterInteraction)) {
			port_SurfaceFromUnderwater();
			return;
		}
		if ((mem_.global_counter() & 0x1) || water_current_mask(CURRENT_UP | CURRENT_DOWN))
			return;
		move_y(-1);
	}

	void port_StartFallingAirborneAttack()
	{
		mem_.jump_vel_index() = FALLING_JUMP_VEL_INDEX;
		port_SetState_AttackingAirborne();
	}

	void port_StartJumpingAirborneAttack()
	{
		mem_.jump_vel_index() = 0;
		mem_.jumping_upwards() = TRUE;
		port_SetState_AttackingAirborne(); // fallthrough
	}

	void port_UpdateState_Submerged()
	{
		mem_.unk_c0e1() = 0;
		if (room_transition_check())
			return;
		if (!still_in(WST_SUBMERGED))
			return;
		port_ApplyWaterCurrentMovement();
		port_Func_1f1a9();
	}

	void port_Func_1ee88()
	{
		if (ld_bit(wJoypadPressed, B_PAD_A)) {
			port_Func_1ede9();
			return;
		}
		uint8_t pad = mem_.joypad_down();
		if (pad & PAD_B) {
			port_SetState_ThrowCharging();
			return;
		}
		if (pad & PAD_DOWN) {
			port_CrouchOrSlide();
			return;
		}
		if (!(pad & (PAD_RIGHT | PAD_LEFT)))
			return;
		mem_.wario_state() = WST_GRAB_WALKING;
		clear({wSFXLoopCounter, wFrameDuration, wAnimationFrame});
		collision_box(-1, -27, -9, 9);
		if (ld_bit(wJoypadDown, B_PAD_RIGHT)) {
			mem_.direction() = DIRECTION_RIGHT;
			set_frameset(ADDR_Frameset_14a0f);
		} else {
			mem_.direction() = DIRECTION_LEFT;
			set_frameset(ADDR_Frameset_149fe);
		}
		anim(OAM1);
	}

	void port_Func_1eefc()
	{
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_B) {
			port_SetState_ThrowCharging();
			return;
		}
		if (pressed & PAD_A) {
			port_Func_1ede9();
			return;
		}
		if (pressed & PAD_DOWN) {
			// .asm_1efaf
			if (!on_slippery_ground())
				port_CrouchOrSlide();
			else
				FARCALL(SetState_CrouchSlipping);
			return;
		}
		uint8_t pad = mem_.joypad_down();
		bool right;
		if (pad & PAD_RIGHT) {
			right = true;
		} else if (pad & PAD_LEFT) {
			right = false;
		} else {
			if (on_slippery_ground())
				FARCALL(SetState_GrabSlipping);
			else
				port_SetState_GrabIdling();
			return;
		}
		if (facing_right() != right) {
			port_SetState_GrabIdling();
			return;
		}
		if (front_blocked())
			return;
		walk_with_velocity(right);
		// uphill: .asm_1efc5, else .asm_1ef8d
		bool uphill = false;
		uint8_t slope = mem_.wario_slope_interaction();
		if (slope) {
			bool right_slope = slope >> RIGHT_SLOPE_F & 1;
			uphill = right ? right_slope : !right_slope;
		}
		bool heavy = ld_bit(wGrabState, GRAB_HEAVY_F);
		if (uphill)
			cap_walk(heavy ? 0x04 : 0x08, heavy ? 0x00 : 0x04);
		else
			cap_walk(heavy ? 0x08 : 0x10, heavy ? 0x04 : 0x0c);
	}

	void port_HandleInput_Attacking()
	{
		if (ld_bit(wJoypadDown, facing_right() ? B_PAD_LEFT : B_PAD_RIGHT)) {
			port_SetState_Turning();
			return;
		}
		// .check_pressed
		uint8_t pressed = mem_.joypad_pressed();
		if (pressed & PAD_A)
			port_StartJumpingAirborneAttack();
		else if (pressed & PAD_DOWN)
			port_SetState_CrouchSliding();
	}

	void port_UpdateState_GrabIdling()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_IDLING))
			return;
		if (!ld_and(wGrabState)) {
			port_SetState_Idling();
			return;
		}
		port_Func_1ee88();
		if (!still_in(WST_GRAB_IDLING))
			return;
		uint8_t hit = centre_collision();
		if (!still_in(WST_GRAB_IDLING))
			return;
		if (!hit) {
			port_Func_1edd3();
			return;
		}
		update_pos_y();
	}

	void port_UpdateState_GrabWalking()
	{
		if (room_transition_check())
			return;
		if (!still_in(WST_GRAB_WALKING))
			return;
		if (!ld_and(wGrabState)) {
			port_SetState_Walking_ResetWalkVel();
			return;
		}
		sfx_loop(36, SFX_WALK);
		anim(OAM1);
		port_Func_1eefc();
		if (!still_in(WST_GRAB_WALKING))
			return;
		if (!centre_collision()) {
			port_Func_1edd3();
			return;
		}
		update_pos_y();
	}

	void port_UpdateState_Attacking()
	{
		sfx_loop(6, mem_.power_up_level() < GARLIC ? SFX_ATTACK : SFX_ATTACK_POWERED_UP);
		anim(OAM1);
		port_HandleInput_Attacking();
		if (!still_in(WST_ATTACKING))
			return;
		if (room_transition_check())
			return;
		if (!still_in(WST_ATTACKING))
			return;
		uint8_t hit = front_collision();
		if (!still_in(WST_ATTACKING))
			return;
		if (hit & 0x0f) {
			port_DoBumpWithSmallBackOffset();
			return;
		}
		if (!centre_collision()) {
			port_StartFallingAirborneAttack();
			return;
		}
		if (!still_in(WST_ATTACKING))
			return;
		if (mem_.attack_counter() < CHARGE_ATTACK_COUNTER) {
			walk_with_velocity(facing_right());
			cap_walk(0x14, 0x10);
		}
		// .check_collision
		centre_collision();
		update_pos_y();
		if (!still_in(WST_ATTACKING))
			return;
		if (!count_down_to_zero(wAttackCounter))
			return;
		if (on_slippery_ground()) {
			FARCALL(SetState_Slipping);
			return;
		}
		if (ld_mask(wJoypadDown, PAD_RIGHT | PAD_LEFT))
			port_SetState_Walking_ResetWalkVel();
		else
			port_SetState_Idling();
	}

	// Dispatch on the state (a, wWarioState as UpdateWarioStates jumps
	// here), as the asm's jumptable
	void port_UpdateWarioStates_Group1(uint8_t state)
	{
		switch (state) {
		case 0x00: port_UpdateState_Idling(); return;
		case 0x01: port_UpdateState_Walking(); return;
		case 0x02: port_UpdateState_Turning(); return;
		case 0x04: port_UpdateState_Airborne(); return;
		case 0x05: port_UpdateState_Landing(); return;
		case 0x06: port_UpdateState_CrouchSliding(); return;
		case 0x07: port_UpdateState_Attacking(); return;
		case 0x08: port_UpdateState_AttackingAirborne(); return;
		case 0x09: port_UpdateState_Bumping(); return;
		case 0x0a: port_UpdateState_Diving(); return;
		case 0x0b: port_UpdateState_Submerged(); return;
		case 0x0c: port_UpdateState_WaterSurfaceIdling(); return;
		case 0x0d: port_UpdateState_WaterSurfaceMoving(); return;
		case 0x0e: port_UpdateState_UnderwaterThrusting(); return;
		case 0x0f: port_UpdateState_SwimKnockBack(); return;
		case 0x10: port_UpdateState_WaterStung(); return;
		case 0x11: port_UpdateState_TryingSubmerge(); return;
		case 0x12: port_UpdateState_Crouching(); return;
		case 0x13: port_UpdateState_CrouchWalking(); return;
		case 0x14: port_UpdateState_CrouchAirborne(); return;
		case 0x15: port_UpdateState_Stung(); return;
		case 0x16: port_UpdateState_StungRecovery(); return;
		case 0x17: port_UpdateState_PipeGoingDown(); return;
		case 0x18: port_UpdateState_PipeGoingUp(); return;
		case 0x19: port_UpdateState_EnemyBumping(); return;
		case 0x1a: port_UpdateState_SmashAttacking(); return;
		case 0x1e: port_UpdateState_PickingUp(); return;
		case 0x1f: port_UpdateState_GrabIdling(); return;
		case 0x20: port_UpdateState_GrabWalking(); return;
		case 0x21: port_UpdateState_GrabAirborne(); return;
		case 0x22: port_UpdateState_ThrowCharging(); return;
		case 0x23: port_UpdateState_ThrowFullyCharged(); return;
		case 0x24: port_UpdateState_Throwing(); return;
		case 0x25: port_UpdateState_ThrowingAirborne(); return;
		case 0x26: port_UpdateState_GrabSmashAttacking(); return;
		case 0x27: port_UpdateState_Sliding(); return;
		case 0x28: port_UpdateState_Rolling(); return;
		case 0x29: port_UpdateState_RollingAirborne(); return;
		case 0x2a: port_UpdateState_PickedUp(); return;
		case 0x2b: port_UpdateState_GroundShakeStunned(); return;
		case 0x2c: port_UpdateState_EnteringDoor(); return;
		case 0x03: case 0x1b: case 0x1c: case 0x1d: case 0x2d: case 0x2e: case 0x2f: case 0x30: case 0x31:
		case 0x32: case 0x33:
			CALL(InvalidWarioStateReset);
			return;
		}
		// past the table: the asm jumps through whatever follows
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}

	void port_UpdateWarioStates()
	{
		uint8_t cooldown = mem_.coin_cooldown();
		if (cooldown)
			mem_.coin_cooldown() = static_cast<uint8_t>(cooldown - 1);
		port_HandleGroundShake();
		port_HandleInvincibility();

		uint8_t state = mem_.wario_state();
		if (state < WARIO_STATES_GROUP_1)
			port_UpdateWarioStates_Group1(state);
		else if (state < WARIO_STATES_GROUP_2)
			routines_.in_bank(BANK_UpdateWarioStates_Group2, ADDR_UpdateWarioStates_Group2,
				[&] { update_wario_states_group2(gb_); });
		else if (state < WARIO_STATES_GROUP_3)
			routines_.in_bank(BANK_UpdateWarioStates_Group3, ADDR_UpdateWarioStates_Group3,
				[&] { update_wario_states_group3(gb_); });
		else
			routines_.in_bank(BANK_UpdateWarioStates_Group4, ADDR_UpdateWarioStates_Group4,
				[&] { update_wario_states_group4(gb_); });
	}
};

// CrouchOrSlide, SetState_Idling and StartFall as the asm leaves the
// registers: they're reached from inside the collision checks (Func_19832
// farcalls CrouchOrSlide; BlockFunc_WalkthroughPipe and
// BlockFunc_LightSource the other two, through RecoverFromTransformation),
// whose register-level callers see what they leave (ZipLineFunc reads c
// after Func_19706), and the liveness script can't follow them all
class WarioStates1Exact : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_CrouchOrSlide()
	{
		if (ld_and(wWarioSlopeInteraction)) {
			port_SetState_Sliding();
			return;
		}
		collision_box(-1, -15, -9, 9);
		mem_.wario_state() = WST_CROUCHING;
		static const uint16_t zero[] = {wWarioStateCounter, wWarioStateCycles, wGrabState,
			wAttackCounter, wWalkVelIndex, wJumpVelTable, wJumpVelIndex, 0};
		clear(zero);
		mem_.is_crouching() = TRUE;
		if (mem_.joypad_down() & (PAD_RIGHT | PAD_LEFT)) {
			port_SetState_CrouchWalking();
			return;
		}
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		ld_cp(wDirection, DIRECTION_RIGHT);
		set_frameset(ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
		anim(OAM1);
	}

	void get_pos_with_offset(int dy, int dx)
	{
		auto x = static_cast<uint16_t>(mem_.wario_x() + dx);
		auto y = static_cast<uint16_t>(mem_.wario_y() + dy);
		mem_.x_pos_lo() = static_cast<uint8_t>(x);
		wr8(hXPosLo - 1, static_cast<uint8_t>(x >> 8));
		wr8(hXPosLo - 2, static_cast<uint8_t>(y));
		wr8(hXPosLo - 3, static_cast<uint8_t>(y >> 8));
	}

	void port_SetState_Sliding()
	{
		mem_.wario_state() = WST_SLIDING;
		static const uint16_t zero[] = {wSFXLoopCounter, wIsCrouching, wFrameDuration,
			wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wWalkVelIndex, wGrabState, 0};
		clear(zero);
		mem_.is_rolling() = TRUE;
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioSlideGfx, ADDR_WarioSlideGfx);
		set_oam(BANK_OAM_1644a, ADDR_OAM_1644a);
		if (!ld_bit(wWarioSlopeInteraction, LEFT_SLOPE_F)) {
			get_pos_with_offset(-0x0c, 0x0c);
			Routines(gb_).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, PARTICLE_SLIDE_DUST_LEFT); });
			st(wDirection, DIRECTION_LEFT);
			set_frameset(ADDR_Frameset_16708);
		} else {
			get_pos_with_offset(-0x0c, -0x0c);
			Routines(gb_).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { create_particle(gb_, PARTICLE_SLIDE_DUST_RIGHT); });
			st(wDirection, DIRECTION_RIGHT);
			set_frameset(ADDR_Frameset_1671d);
		}
		anim(OAM1);
	}

	void port_SetState_CrouchWalking()
	{
		collision_box(-1, -15, -9, 9);
		mem_.wario_state() = WST_CROUCH_WALKING;
		mem_.is_crouching() = TRUE;
		static const uint16_t zero[] = {wSFXLoopCounter, wFrameDuration, wAnimationFrame,
			wWarioStateCounter, wWarioStateCycles, wAttackCounter, wGrabState, 0};
		clear(zero);
		set_oam(BANK_OAM_1426c, ADDR_OAM_1426c);
		load_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
		ld_cp(wDirection, DIRECTION_RIGHT);
		set_frameset(ZF ? ADDR_Frameset_14a2f : ADDR_Frameset_14a26);
		anim(OAM1);
	}
	void port_SetState_Idling()
	{
		xor_a();
		mem_.walk_vel_index() = 0;
		mem_.wario_state() = WST_IDLING;
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wJumpVelIndex,
			wJumpVelTable, wWarioStateCounter, wWarioStateCycles, wIsSmashAttacking,
			wIsCrouching, wAttackCounter, wGrabState, 0};
		clear(zero);
		coll_box_blrt();
		run_up_collision();
		ld_cp(wWarioState, WST_IDLING);
		if (!ZF) {
			return;
		}
		if (b_and()) {
			port_CrouchOrSlide();
			return;
		}
		load_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
		set_oam(BANK_OAM_14000, ADDR_OAM_14000);
		frameset_by_dir(ADDR_Frameset_14252, ADDR_Frameset_1425f);
		anim(OAM1);
	}

	void port_StartFall()
	{
		xor_a();
		mem_.jumping_upwards() = FALSE;
		port_StartFall_SkipResetJumpUpwards(); // fallthrough
	}

	void port_StartFall_SkipResetJumpUpwards()
	{
		st(wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
		port_StartJump(); // jr
	}

	void port_StartJump()
	{
		bool high = mem_.power_up_level() >= HIGH_JUMP_BOOTS && (mem_.joypad_down() & (1 << B_PAD_UP));
		mem_.jump_vel_table() = high ? JUMP_VEL_HIGH_JUMP : JUMP_VEL_NORMAL;
		port_SetState_Airborne(); // fallthrough
	}

	void port_SetState_Airborne()
	{
		xor_a();
		mem_.wario_slope_interaction() = 0;
		st(wWarioState, WST_AIRBORNE);
		static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
			wWarioStateCycles, wIsCrouching, wAttackCounter, 0};
		clear(zero);
		collision_box(-1, -27, -9, 9);
		load_gfx(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
		set_oam(BANK_OAM_15955, ADDR_OAM_15955);
		set_frameset(facing_right() ? ADDR_Frameset_15f97 : ADDR_Frameset_15f94);
		anim(OAM1);
		move_up_if_on_steppable_object();
	}

	void move_up_if_on_steppable_object()
	{
		if (!ld_and(wIsOnSteppableObject))
			return;
		home::move_wario_y(gb_, -2);
	}
};

} // namespace

} // namespace wl3::level

// Every caller of these reads no register after them (the asm liveness
// check): they clobber them all, but for the three of WarioStates1Exact.
using wl3::level::WarioStates1;

PORT_EX(Func_1e6ea, 0, ALL_REGISTERS) { WarioStates1(gb).port_Func_1e6ea(); }
PORT_COVERED(SetState_Turning);
PORT_COVERED(SetState_Attacking);
PORT_COVERED(SetState_Walking);
PORT_COVERED(SetState_Airborne);
PORT_COVERED(SetState_Diving);
PORT_EX(SetState_Submerged, 0, ALL_REGISTERS) { WarioStates1(gb).port_SetState_Submerged(); }
PORT_COVERED(SetState_WaterSurfaceIdling);
PORT_COVERED(SetState_Sliding);
PORT_COVERED(SetState_RollingAirborne);
PORT_COVERED(SetState_CrouchWalking);
PORT_COVERED(SetState_CrouchAirborne);
PORT_COVERED(Func_1f6c2);
PORT_COVERED(HandleWaterSurfaceFloatingMovement);
PORT_COVERED(StartJump);
PORT_COVERED(SetState_Walking_ResetWalkVel);
PORT_EX(CrouchOrSlide, 0, ALL_REGISTERS) { wl3::level::WarioStates1Exact(gb).port_CrouchOrSlide(); }
PORT_EX(StartCrouchFall, 0, ALL_REGISTERS) { WarioStates1(gb).port_StartCrouchFall(); }
PORT_EX(StartCrouchJump, 0, ALL_REGISTERS) { WarioStates1(gb).port_StartCrouchJump(); }
PORT_COVERED(StartFall_SkipResetJumpUpwards);
PORT_EX(StartJump_FromInput, 0, ALL_REGISTERS) { WarioStates1(gb).port_StartJump_FromInput(); }
PORT_EX(DiveFromSurface_SkipSplash, 0, ALL_REGISTERS) { WarioStates1(gb).port_DiveFromSurface_SkipSplash(); }
PORT_COVERED(SurfaceFromUnderwater);
PORT_EX(StartFall, 0, ALL_REGISTERS) { wl3::level::WarioStates1Exact(gb).port_StartFall(); }
PORT_COVERED(StartJump_FromWaterSurface);
PORT_EX(StartJump_FromSand, 0, ALL_REGISTERS) { WarioStates1(gb).port_StartJump_FromSand(); }
PORT_COVERED(DiveFromSurface);
PORT_COVERED(SetState_TryingSubmerge);
PORT_COVERED(UpdateState_TryingSubmerge);
PORT_COVERED(Func_1e1e3);
PORT_COVERED(HandleInput_Idling);
PORT_EX(Func_1e174, 0, ALL_REGISTERS) { WarioStates1(gb).port_Func_1e174(); }
PORT_EX(Func_1f863, 0, ALL_REGISTERS) { WarioStates1(gb).port_Func_1f863(REG_DE(gb)); } // de = the index
PORT_COVERED(SetState_Throwing);
PORT_COVERED(Func_1f3a7);
PORT_COVERED(SetState_SmashAttacking);
PORT_COVERED(SetState_LadderShakeStunned);
PORT_EX(StartDive, 0, ALL_REGISTERS) { WarioStates1(gb).port_StartDive(); }
PORT_COVERED(UpdateState_Idling);
PORT_COVERED(SetState_ThrowingAirborne);
PORT_COVERED(SetState_WaterSurfaceMoving);
PORT_COVERED(SetState_Rolling);
PORT_EX(SetState_SwimKnockBack, 0, ALL_REGISTERS) { WarioStates1(gb).port_SetState_SwimKnockBack(); }
PORT_COVERED(SetState_ThrowCharging);
PORT_COVERED(SetState_CrouchSliding);
PORT_COVERED(UpdateState_PickedUp);
PORT_COVERED(SetState_UnderwaterThrusting);
PORT_COVERED(StartUpwardsUnderwaterThrusting);
PORT_COVERED(SetState_SwimKnockBack_SwitchDirection);
PORT_COVERED(StartUnderwaterThrusting);
PORT_COVERED(Func_1de3f);
PORT_EX(DoGroundPound, 0, ALL_REGISTERS) { WarioStates1(gb).port_DoGroundPound(); }
PORT_EX(DoWarioGroundShake, 0, ALL_REGISTERS) { WarioStates1(gb).port_DoWarioGroundShake(); }
PORT_COVERED(HandleGroundShake);
PORT_EX(SetState_Idling, 0, ALL_REGISTERS) { wl3::level::WarioStates1Exact(gb).port_SetState_Idling(); }
PORT_EX(WalkIfPossible_Right, 0, ALL_REGISTERS) { WarioStates1(gb).port_WalkIfPossible_Right(); }
PORT_EX(WalkIfPossible_Left, 0, ALL_REGISTERS) { WarioStates1(gb).port_WalkIfPossible_Left(); }
PORT_COVERED(Func_1ec19);
PORT_COVERED(Func_1ec4b);
PORT_COVERED(HandleWaterSurfaceInput);
PORT_COVERED(HandleBumpMovement);
PORT_COVERED(SetState_Bumping);
PORT_COVERED(HandleDivingHorizontalMovement);
PORT_COVERED(DoHardLand);
PORT_COVERED(HandleInvincibility);
PORT_COVERED(DoSoftLand);
PORT_EX(WalkRight, 0, ALL_REGISTERS) { WarioStates1(gb).port_WalkRight(); }
PORT_EX(WalkLeft, 0, ALL_REGISTERS) { WarioStates1(gb).port_WalkLeft(); }
PORT_COVERED(DoBumpOrJumpingBump);
PORT_EX(DoJumpingBump_NoSFX, 0, ALL_REGISTERS) { WarioStates1(gb).port_DoJumpingBump_NoSFX(); }
PORT_COVERED(UpdateState_Turning);
PORT_COVERED(Func_1eb46);
PORT_COVERED(UpdateState_Diving);
PORT_COVERED(Func_1f825);
PORT_COVERED(UpdateState_StungRecovery);
PORT_COVERED(UpdateState_PipeGoingDown);
PORT_COVERED(UpdateState_PipeGoingUp);
PORT_COVERED(UpdateState_Landing);
PORT_COVERED(HandleInput_Walking);
PORT_COVERED(Func_1ec6c);
PORT_EX(SetState_GrabIdling, 0, ALL_REGISTERS) { WarioStates1(gb).port_SetState_GrabIdling(); }
PORT_COVERED(DoBumpWithBigBackOffset);
PORT_COVERED(DoBumpWithSmallBackOffset);
PORT_EX(DoJumpingBump, 0, ALL_REGISTERS) { WarioStates1(gb).port_DoJumpingBump(); }
PORT_COVERED(UpdateState_Crouching);
PORT_COVERED(UpdateState_Bumping);
PORT_COVERED(UpdateState_Stung);
PORT_COVERED(UpdateState_EnemyBumping);
PORT_COVERED(UpdateState_EnteringDoor);
PORT_COVERED(ApplyLastWaterCurrentMovement);
PORT_COVERED(HandleInput_Airborne);
PORT_COVERED(UpdateState_SmashAttacking);
PORT_COVERED(UpdateState_Throwing);
PORT_COVERED(UpdateState_ThrowingAirborne);
PORT_COVERED(Func_1dd78);
PORT_COVERED(UpdateState_SwimKnockBack);
PORT_COVERED(UpdateState_WaterStung);
PORT_COVERED(Func_1f310);
PORT_COVERED(Func_1f3d9);
PORT_COVERED(ApplyWaterCurrentMovement);
PORT_COVERED(HandleAttackingAirborneMovement);
PORT_COVERED(UpdateState_Walking);
PORT_COVERED(UpdateState_Airborne);
PORT_COVERED(UpdateState_CrouchWalking);
PORT_COVERED(UpdateState_CrouchAirborne);
PORT_COVERED(UpdateState_Sliding);
PORT_COVERED(UpdateState_Rolling);
PORT_COVERED(UpdateState_RollingAirborne);
PORT_COVERED(UpdateState_PickingUp);
PORT_COVERED(UpdateState_ThrowCharging);
PORT_COVERED(UpdateState_ThrowFullyCharged);
PORT_COVERED(UpdateState_GrabSmashAttacking);
PORT_COVERED(UpdateState_GroundShakeStunned);
PORT_COVERED(UpdateState_CrouchSliding);
PORT_COVERED(UpdateState_GrabAirborne);
PORT_COVERED(Func_1f357);
PORT_COVERED(HandleUnderwaterThrustingInput);
PORT_COVERED(UpdateState_AttackingAirborne);
PORT_COVERED(UpdateState_WaterSurfaceIdling);
PORT_COVERED(UpdateState_WaterSurfaceMoving);
PORT_COVERED(UpdateState_UnderwaterThrusting);
PORT_COVERED(SetState_GrabAirborne);
PORT_COVERED(Func_1f24c);
PORT_COVERED(SetState_AttackingAirborne);
PORT_EX(Func_1edd3, 0, ALL_REGISTERS) { WarioStates1(gb).port_Func_1edd3(); }
PORT_EX(Func_1ede9, 0, ALL_REGISTERS) { WarioStates1(gb).port_Func_1ede9(); }
PORT_COVERED(Func_1f1a9);
PORT_COVERED(StartFallingAirborneAttack);
PORT_COVERED(StartJumpingAirborneAttack);
PORT_COVERED(UpdateState_Submerged);
PORT_COVERED(Func_1ee88);
PORT_COVERED(Func_1eefc);
PORT_COVERED(HandleInput_Attacking);
PORT_COVERED(UpdateState_GrabIdling);
PORT_COVERED(UpdateState_GrabWalking);
PORT_COVERED(UpdateState_Attacking);
PORT_EX(UpdateWarioStates_Group1, 0, ALL_REGISTERS) { WarioStates1(gb).port_UpdateWarioStates_Group1(gb->cpu.a); }
// update_level farcalls it and reloads every register it uses after
PORT_EX(UpdateWarioStates, 0, ALL_REGISTERS) { wl3::level::update_wario_states(gb); }

namespace wl3::level {
void update_wario_states(GB *gb) { WarioStates1(gb).port_UpdateWarioStates(); }
} // namespace wl3::level
