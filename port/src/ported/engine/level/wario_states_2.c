// Port of src/engine/level/wario_states_2.asm
#include "ported.h"

// Note: after a farcall, a = the caller's ROM bank and f = the flags from
// just before the farcall (hCallFunc pushes and pops af), so the flags have
// to be tracked exactly even on paths that end in a farcall.

// Jumps and calls within this bank (bank 6). They do the same as JUMP/CALL
// without the bank check: fuzzed inputs can make a callee switch banks
// through a stray write to the MBC, and then the asm jumps into the wrong
// bank just the same, which the check would abort on.
#define LJUMP(name) JUMP_TO(ADDR_##name)
#define LCALL(name) gb_call(gb, ADDR_##name)

#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)

#define OAM1 BANK_OAM_14000   // BANK("Wario OAM 1")
#define OAM2 BANK_OAM_1fc000  // BANK("Wario OAM 2")

// ld a, v / ld [addr], a
static void st(GB *gb, uint16_t addr, uint8_t v)
{
	gb->cpu.a = v;
	wr8(gb, addr, v);
}

// ld a, [addr]
static uint8_t ld(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	return gb->cpu.a;
}

static void xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

// ld a, [addr] / and a; true if nz
static bool ld_and(GB *gb, uint16_t addr)
{
	uint8_t v = ld(gb, addr);
	return alu_and(gb, v, v) != 0;
}

// ld a, b / and a; true if nz
static bool b_and(GB *gb)
{
	gb->cpu.a = gb->cpu.b;
	return alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
}

// ld a, [addr] / bit n, a
static bool ld_bit(GB *gb, uint16_t addr, int bit)
{
	return alu_bit(gb, ld(gb, addr), bit);
}

// ld a, [addr] / and mask; true if nz
static bool ld_mask(GB *gb, uint16_t addr, uint8_t mask)
{
	gb->cpu.a = alu_and(gb, rd8(gb, addr), mask);
	return gb->cpu.a != 0;
}

// ld a, [addr] / cp v
static void ld_cp(GB *gb, uint16_t addr, uint8_t v)
{
	alu_cp(gb, ld(gb, addr), v);
}

// wDMASourceBank/Ptr = gfx, call LoadWarioGfx
static void load_gfx(GB *gb, uint8_t bank, uint16_t ptr)
{
	wr8(gb, wDMASourceBank, bank);
	wr8(gb, wDMASourcePtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wDMASourcePtr + 1, (uint8_t)ptr);
	CALL(LoadWarioGfx);
}

static void set_oam(GB *gb, uint8_t bank, uint16_t ptr)
{
	wr8(gb, wWarioOAMBank, bank);
	wr8(gb, wWarioOAMPtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wWarioOAMPtr + 1, (uint8_t)ptr);
}

static void set_frameset(GB *gb, uint16_t ptr)
{
	wr8(gb, wWarioFramesetPtr + 0, (uint8_t)(ptr >> 8));
	st(gb, wWarioFramesetPtr + 1, (uint8_t)ptr);
}

// ld a, BANK("Wario OAM n") / ldh [hCallFuncBank], a / hcall UpdateWarioAnimation
static void anim(GB *gb, uint8_t bank)
{
	FARCALL_BANK(bank, UpdateWarioAnimation);
}

// bottom -1, left -9, right 9, top -27 (a = -27)
static void coll_box_blrt(GB *gb)
{
	wr8(gb, wCollisionBoxBottom, (uint8_t)-1);
	wr8(gb, wCollisionBoxLeft, (uint8_t)-9);
	wr8(gb, wCollisionBoxRight, 9);
	st(gb, wCollisionBoxTop, (uint8_t)-27);
}

// bottom -1, top -27, left -9, right 9 (a = 9)
static void coll_box_btlr(GB *gb)
{
	wr8(gb, wCollisionBoxBottom, (uint8_t)-1);
	wr8(gb, wCollisionBoxTop, (uint8_t)-27);
	wr8(gb, wCollisionBoxLeft, (uint8_t)-9);
	st(gb, wCollisionBoxRight, 9);
}

// update_pos_y
static void update_pos_y(GB *gb)
{
	wr8(gb, wWarioYPos + 0, rd8(gb, hYPosHi));
	gb->cpu.a = rd8(gb, hYPosHi + 1);
	wr8(gb, wWarioYPos + 1, gb->cpu.a);
	SET_HL(gb, hYPosHi + 1);
	SET_DE(gb, wWarioYPos + 1);
}

// get_pos
static void get_pos(GB *gb)
{
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wWarioPos + i);
		wr8(gb, hPos + i, gb->cpu.a);
	}
	SET_HL(gb, wWarioPos + 3);
	SET_DE(gb, hPos + 3);
}

// ld a, [wSFXLoopCounter] / sub 1 / ld [wSFXLoopCounter], a / jr nc, .skip
// ld a, reload / ld [wSFXLoopCounter], a / play_sfx sfx
static void sfx_loop(GB *gb, uint8_t reload, uint16_t sfx)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wSFXLoopCounter), 1);
	wr8(gb, wSFXLoopCounter, gb->cpu.a);
	if (!CF)
		return;
	st(gb, wSFXLoopCounter, reload);
	play_sfx(gb, sfx);
}

// ld a, [wDirection] / and a / jr nz: picks a frameset
static void frameset_by_dir(GB *gb, uint16_t left, uint16_t right)
{
	set_frameset(gb, ld_and(gb, wDirection) ? right : left);
}

// ld a, [wRoomTransitionParam] / and a; true if nz
static bool room_transition(GB *gb)
{
	return ld_and(gb, wRoomTransitionParam);
}

// ld a, [wWarioAnimationEnded] / and a; true if nz
static bool anim_ended(GB *gb)
{
	return ld_and(gb, wWarioAnimationEnded);
}

// The camera/floor check after moving down:
// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp CAM_TRANSITIONS
// jr c, .skip / call GetFloorForYPos / ld a, [wFloor] / sub c / jr z / jr c
// Returns true if it should transition down.
static bool down_floor_check(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	if (CF)
		return false;
	CALL(GetFloorForYPos);
	gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
	return !(gb->cpu.f & (FLAG_Z | FLAG_C));
}

// ld a, [wDirection] / and a / jr nz, .right / call MoveWarioLeft / ...
static void move_by_dir(GB *gb)
{
	if (ld_and(gb, wDirection))
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
}

// ld a, [wWarioXPos + 1] / and $f0 / add $08 / ld [wWarioXPos + 1], a
static void center_on_ladder(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
}

// ld hl, wWarioStateCounter / inc [hl] / ld a, [hl]
static uint8_t inc_counter(GB *gb)
{
	SET_HL(gb, wWarioStateCounter);
	gb->cpu.a = alu_inc(gb, rd8(gb, wWarioStateCounter));
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	return gb->cpu.a;
}

// The slipping movement shared by the slipping states, from
// ld hl, wWarioStateCounter / inc [hl] on. Returns true if it ends at the
// "stop slipping" label; otherwise the routine returns.
static bool slip_move(GB *gb, uint8_t t1, uint8_t t2, uint8_t t3, bool stop_at_or_above_t3)
{
	uint8_t v = inc_counter(gb);
	alu_cp(gb, v, t1);
	if (CF) {
		gb->cpu.b = 2;
		move_by_dir(gb);
		return false;
	}
	alu_cp(gb, v, t2);
	if (CF) {
		gb->cpu.b = 1;
		move_by_dir(gb);
		return false;
	}
	alu_cp(gb, v, t3);
	if (CF) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 1);
		if (ZF)
			return false;
		gb->cpu.b = 1;
		move_by_dir(gb);
		return false;
	}
	if (stop_at_or_above_t3 || ZF)
		return true;
	return false;
}

// Dispatch on wWarioState: ld a, [wWarioState] / sub $30 / rst JumpTable
PORT_JUMP(UpdateWarioStates_Group2)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioState), 0x30);
	// rst JumpTable: pushes the address of the table after the rst
	gb_push16(gb, ADDR_UpdateWarioStates_Group2 + 6);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl);
	hl = (uint16_t)(rd8(gb, hl + 1) << 8 | gb->cpu.a);
	SET_HL(gb, hl);
	JUMP_TO(hl);
}

PORT(SetState_Slipping)
{
	st(gb, wWarioState, WST_SLIPPING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wAttackCounter, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14000, ADDR_OAM_14000);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	ld(gb, wDirection);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_1425f : ADDR_Frameset_14252);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_Slipping)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}

	uint8_t cycles = ld(gb, wWarioStateCycles);
	alu_cp(gb, cycles, 0x04);
	if (!CF)
		goto anim_and_input;
	alu_cp(gb, cycles, 0x02);
	if (!CF)
		goto walk_turn;
	gb->cpu.b = cycles;
	if (!ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT))
		goto anim_and_input;
	gb->cpu.a = (uint8_t)(gb->cpu.a << 3 | gb->cpu.a >> 5); // rlca x3
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x01);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!ZF)
		goto anim_and_input;
	gb->cpu.a = alu_xor(gb, gb->cpu.a, 1);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 2);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	set_frameset(gb, ld_mask(gb, wWarioStateCycles, 0x01) ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
	goto reset_anim;

walk_turn:
	gb->cpu.b = cycles;
	if (!anim_ended(gb))
		goto anim_and_input;
	gb->cpu.a = alu_add(gb, gb->cpu.b, 0x02);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	set_frameset(gb, ld_mask(gb, wWarioStateCycles, 0x01) ? ADDR_Frameset_149c5 : ADDR_Frameset_149b4);

reset_anim:
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);

anim_and_input:
	anim(gb, OAM1);
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		FARCALL(StartJump_FromInput);
		RET();
		return;
	}
	if (ld_bit(gb, wJoypadDown, B_PAD_DOWN)) {
		LJUMP(Func_19e89);
		return;
	}

	FARCALL(CheckCentreCollision);
	ld_cp(gb, wWarioState, WST_SLIPPING);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		FARCALL(StartFall);
		RET();
		return;
	}

	update_pos_y(gb);
	FARCALL(CheckFrontCollision);
	ld_cp(gb, wWarioState, WST_SLIPPING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb))
		goto stop;
	CALL(IsOnSlipperyGround);
	if (ZF)
		goto stop;
	if (slip_move(gb, 0x08, 0x0f, 0x15, false))
		goto stop;
	RET();
	return;

stop:
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioStateCycles), 0x01);
	wr8(gb, wDirection, gb->cpu.a);
	FARCALL(SetState_Idling);
	RET();
}

PORT_JUMP(SetState_CrouchSlipping)
{
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	ld(gb, wDirection);
	wr8(gb, wWarioStateCycles, gb->cpu.a);
	LJUMP(Func_19e89);
}

PORT(Func_19e89)
{
	st(gb, wWarioState, WST_CROUCH_SLIPPING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wAttackCounter, 0);
	wr8(gb, wGrabState, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsCrouching, gb->cpu.a);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	ld_cp(gb, wWarioStateCycles, 0x01);
	set_frameset(gb, ZF ? ADDR_Frameset_14a3b : ADDR_Frameset_14a38);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_CrouchSlipping)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		FARCALL(StartCrouchJump);
		RET();
		return;
	}

	FARCALL(CheckCentreCollision);
	ld_cp(gb, wWarioState, WST_CROUCH_SLIPPING);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		FARCALL(StartCrouchFall);
		RET();
		return;
	}

	update_pos_y(gb);
	FARCALL(CheckFrontCollision);
	ld_cp(gb, wWarioState, WST_CROUCH_SLIPPING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb))
		goto stop;
	CALL(IsOnSlipperyGround);
	if (ZF)
		goto stop;
	if (slip_move(gb, 0x06, 0x0a, 0x0e, true))
		goto stop;
	RET();
	return;

stop:
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioStateCycles), 0x01);
	wr8(gb, wDirection, gb->cpu.a);
	FARCALL(CrouchOrSlide);
	RET();
}

PORT(SetState_DraggedDown)
{
	st(gb, wWarioState, WST_DRAGGED_DOWN);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wAttackCounter, 0);
	wr8(gb, wGrabState, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wIsSmashAttacking, 0);
	wr8(gb, wIsCrouching, 0);
	wr8(gb, wIsRolling, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wAutoMoveState, gb->cpu.a);
	coll_box_blrt(gb);
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_15f70, ADDR_Frameset_15f7f);
	anim(gb, OAM1);
}

PORT(UpdateState_DraggedDown)
{
	anim(gb, OAM1);
	if (ld_and(gb, wAutoMoveState))
		return;
	ld_cp(gb, wLevel, THE_TEMPLE);
	if (ZF) {
		st(gb, wLevelEndScreen, LVLEND_GAME_OVER);
		return;
	}
	FARCALL(StartFall);
}

static void teleporting(GB *gb, uint8_t sfx_frame, uint8_t next_state)
{
	st(gb, wIsIntangible, TRUE);
	anim(gb, OAM1);
	uint8_t v = inc_counter(gb);
	alu_cp(gb, v, sfx_frame);
	if (ZF) {
		// jr z, PlayTeleportingSFX
		play_sfx(gb, SFX_082);
		RET();
		return;
	}
	alu_cp(gb, v, 0xb4);
	if (CF) {
		RET();
		return;
	}
	wr8(gb, wWarioStateCounter, 0x00);
	st(gb, wWarioState, next_state);
	st(gb, wInvincibleCounter, 0x01);
	SET_HL(gb, wRoomTransitionParam);
	wr8(gb, wRoomTransitionParam,
		rd8(gb, wRoomTransitionParam) & (uint8_t)~(1 << ROOMTRANSITIONF_DELAY_F));
	JUMP(TriggerRoomTransition);
}

PORT_JUMP(UpdateState_Teleporting)
{
	teleporting(gb, 0x01, WST_STUNG);
}

PORT(PlayTeleportingSFX)
{
	play_sfx(gb, SFX_082);
}

PORT_JUMP(UpdateState_WaterTeleporting)
{
	teleporting(gb, 0x11, WST_WATER_STUNG);
}

// SetState_SandFalling stays in asm: it calls SetWarioPal, which waits for
// hblank (wait_not_hblank / wait_hblank), so it can't run in zero time.

PORT_JUMP(Func_1a12a)
{
	load_gfx(gb, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx);
	set_oam(gb, BANK_OAM_15955, ADDR_OAM_15955);
	LJUMP(Func_1a14b); // fallthrough
}

PORT(Func_1a14b)
{
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_15f94, ADDR_Frameset_15f97);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_SandFalling)
{
	LCALL(Func_1b2c0);
	ld_cp(gb, wWarioState, WST_SAND_FALLING);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	update_pos_y(gb);
	LJUMP(SetState_SandIdling);
}

PORT_JUMP(SetState_SandJumping)
{
	play_sfx(gb, SFX_JUMP);
	st(gb, wWarioState, WST_SAND_JUMPING);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	LJUMP(Func_1a12a);
}

// ld a, [wWalkVelIndex] / cp $04 / jr c / ld a, $00 / ld [wWalkVelIndex], a
static void clamp_walk_vel(GB *gb)
{
	ld_cp(gb, wWalkVelIndex, 0x04);
	if (!CF)
		st(gb, wWalkVelIndex, 0x00);
}

PORT_JUMP(UpdateState_SandJumping)
{
	FARCALL(Func_19b25);
	if (!ld_and(gb, wIsInSand)) {
		FARCALL(StartJump_FromSand);
		RET();
		return;
	}
	CALL(ApplyJumpVelocity);
	FARCALL(Func_2b17a);
	clamp_walk_vel(gb);
	if (ld_and(gb, wIsTurning))
		LCALL(Func_1a14b);

	if (!ld_bit(gb, wJoypadDown, B_PAD_A)) {
		LJUMP(SetState_SandFalling);
		return;
	}
	ld_cp(gb, wJumpVelIndex, FALLING_JUMP_VEL_INDEX);
	if (!CF) {
		LJUMP(SetState_SandFalling);
		return;
	}
	FARCALL(CheckUpCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	LJUMP(SetState_SandFalling);
}

PORT(SetState_SandIdling)
{
	st(gb, wWarioState, WST_SAND_IDLING);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14000, ADDR_OAM_14000);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_14252, ADDR_Frameset_1425f);
	anim(gb, OAM1);
}

PORT(UpdateState_SandIdling)
{
	LCALL(Func_1b322);
}

PORT(SetState_SandWalking)
{
	xor_a(gb);
	wr8(gb, wWalkVelIndex, 0);
	st(gb, wWarioState, WST_SAND_WALKING);
	if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT))
		st(gb, wDirection, DIRECTION_RIGHT);
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		st(gb, wDirection, DIRECTION_LEFT);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	FARCALL(Func_1e6ea);
}

PORT_JUMP(UpdateState_SandWalking)
{
	sfx_loop(gb, 0x24, SFX_WALK);
	anim(gb, OAM1);
	LCALL(Func_1b333);
	ld_cp(gb, wWarioState, WST_SAND_WALKING);
	if (!ZF) {
		RET();
		return;
	}
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	FARCALL(CheckCentreCollision);
	if (!b_and(gb)) {
		LJUMP(SetState_SandFalling);
		return;
	}
	RET();
}

PORT(SetState_SandTurning)
{
	st(gb, wWarioState, WST_SAND_TURNING);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), 1);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_14cc5 : ADDR_Frameset_14cbe);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_SandTurning)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_SandJumping);
		return;
	}
	anim(gb, OAM1);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT))
		LJUMP(SetState_SandWalking);
	else
		LJUMP(SetState_SandIdling);
}

PORT(SetState_LadderClimbing)
{
	st(gb, wWarioState, WST_LADDER_CLIMBING);
	center_on_ladder(gb);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsSmashAttacking, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wIsGettingOffLadder, 0);
	coll_box_blrt(gb);
	load_gfx(gb, BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
	set_oam(gb, BANK_OAM_155cc, ADDR_OAM_155cc);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, ADDR_Frameset_158d6);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_LadderClimbing)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	sfx_loop(gb, 32, SFX_CLIMB);
	anim(gb, OAM1);
	if (anim_ended(gb)) {
		get_pos(gb);
		gb->cpu.b = PARTICLE_DUST;
		FARCALL(CreateParticle);
	}
	LCALL(HandleLadderInput);
	RET();
}

PORT(SetState_LadderIdling)
{
	st(gb, wWarioState, WST_LADDER_IDLING);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsSmashAttacking, 0);
	wr8(gb, wIsGettingOffLadder, 0);
	coll_box_blrt(gb);
	load_gfx(gb, BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
	set_oam(gb, BANK_OAM_155cc, ADDR_OAM_155cc);
	frameset_by_dir(gb, ADDR_Frameset_15900, ADDR_Frameset_158fd);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_LadderIdling)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (!ld_and(gb, wLadderInteraction)) {
		FARCALL(StartFall);
		RET();
		return;
	}
	anim(gb, OAM1);
	LCALL(Func_1b480);
	RET();
}

PORT_JUMP(UpdateState_LadderShakeSlipping)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (!ld_and(gb, wLadderInteraction)) {
		FARCALL(StartFall);
		RET();
		return;
	}
	anim(gb, OAM1);
	gb->cpu.b = 0x02;
	CALL(MoveWarioDown);
	if (down_floor_check(gb))
		CALL(StartDownwardsFloorTransition);
	FARCALL(CheckAirborneCollision);
	if (b_and(gb)) {
		update_pos_y(gb);
		FARCALL(SetState_Idling);
		RET();
		return;
	}
	uint8_t v = inc_counter(gb);
	alu_cp(gb, v, 0x40);
	if (CF) {
		RET();
		return;
	}
	wr8(gb, wWarioStateCounter, 0x00);
	LJUMP(SetState_LadderIdling);
}

PORT(SetState_GettingOffLadder)
{
	st(gb, wWarioState, WST_GETTING_OFF_LADDER);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wIsGettingOffLadder, gb->cpu.a);
	wr8(gb, wIsIntangible, gb->cpu.a);
	if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT))
		st(gb, wDirection, DIRECTION_RIGHT);
	else
		st(gb, wDirection, DIRECTION_LEFT);
}

PORT(UpdateState_GettingOffLadder)
{
	sfx_loop(gb, 32, SFX_CLIMB);
	bool right = ld_and(gb, wDirection);
	gb->cpu.b = 0x01;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	FARCALL(Func_19b25);
	if (ld_and(gb, wLadderInteraction))
		return;
	xor_a(gb);
	wr8(gb, wIsGettingOffLadder, 0);
	wr8(gb, wIsIntangible, 0);
	FARCALL(StartFall);
}

static void set_state_sliding(GB *gb, uint8_t state)
{
	st(gb, wWarioState, state);
	play_sfx(gb, SFX_SLIDE);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsGettingOffLadder, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wJumpVelTable, gb->cpu.a);
	ld_cp(gb, wPowerUpLevel, LEAD_OVERALLS);
	if (!CF)
		st(gb, wIsSmashAttacking, TRUE);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, ADDR_Frameset_15945);
	anim(gb, OAM1);
}

PORT(SetState_LadderSliding)
{
	set_state_sliding(gb, WST_LADDER_SLIDING);
}

// The shared body of UpdateState_LadderSliding and UpdateState_FenceSliding.
static void update_sliding(GB *gb, bool fence)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (!ld_and(gb, fence ? wIsNearFence : wLadderInteraction)) {
		xor_a(gb);
		wr8(gb, wIsSmashAttacking, FALSE);
		FARCALL(StartFall);
		RET();
		return;
	}
	anim(gb, OAM1);
	if (ld_bit(gb, wJoypadDown, B_PAD_UP)) {
		if (fence)
			LJUMP(SetState_FenceMovingVertical);
		else
			LJUMP(SetState_LadderClimbing);
		return;
	}
	gb->cpu.b = 0x02;
	CALL(MoveWarioDown);
	if (down_floor_check(gb))
		CALL(StartDownwardsFloorTransition);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb)) {
		RET();
		return;
	}
	update_pos_y(gb);
	if (!ld_and(gb, wIsSmashAttacking)) {
		FARCALL(SetState_Idling);
		RET();
		return;
	}
	ld_cp(gb, wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
	if (CF)
		FARCALL(DoGroundPound);
	else
		FARCALL(DoWarioGroundShake);
	RET();
}

PORT_JUMP(UpdateState_LadderSliding)
{
	update_sliding(gb, false);
}

PORT(SetState_GrabSlipping)
{
	st(gb, wWarioState, WST_GRAB_SLIPPING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wAttackCounter, 0);
	load_gfx(gb, BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	set_oam(gb, BANK_OAM_1426c, ADDR_OAM_1426c);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_14a20, ADDR_Frameset_14a23);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_GrabSlipping)
{
	FARCALL(Func_19b25);
	if (room_transition(gb)) {
		JUMP(TriggerRoomTransition);
		return;
	}
	if (!ld_and(gb, wGrabState)) {
		FARCALL(SetState_Idling);
		RET();
		return;
	}
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		FARCALL(Func_1ede9);
		RET();
		return;
	}
	FARCALL(CheckCentreCollision);
	ld_cp(gb, wWarioState, WST_GRAB_SLIPPING);
	if (!ZF) {
		RET();
		return;
	}
	if (!b_and(gb)) {
		FARCALL(Func_1edd3);
		RET();
		return;
	}
	FARCALL(CheckFrontCollision);
	ld_cp(gb, wWarioState, WST_GRAB_SLIPPING);
	if (!ZF) {
		RET();
		return;
	}
	if (b_and(gb))
		goto stop;
	CALL(IsOnSlipperyGround);
	if (ZF)
		goto stop;
	if (slip_move(gb, 0x08, 0x0f, 0x15, false))
		goto stop;
	RET();
	return;

stop:
	FARCALL(SetState_GrabIdling);
	RET();
}

PORT_JUMP(UpdateState_GetTreasureStart)
{
	anim(gb, OAM1);
	if (ld_and(gb, wJumpVelTable)) {
		CALL(ApplyJumpVelocity);
		FARCALL(CheckAirborneCollision);
		if (!b_and(gb)) {
			RET();
			return;
		}
		wr8(gb, wWarioPos + 0, rd8(gb, hYPosHi));
		gb->cpu.a = rd8(gb, hYPosHi + 1);
		wr8(gb, wWarioPos + 1, gb->cpu.a);
		SET_HL(gb, hYPosHi + 1);
		SET_DE(gb, wWarioPos + 1);
		xor_a(gb);
		wr8(gb, wJumpVelTable, 0);
		wr8(gb, wJumpVelIndex, 0);
	}
	uint8_t v = inc_counter(gb);
	alu_cp(gb, v, 48);
	if (CF) {
		RET();
		return;
	}
	st(gb, wWarioStateCounter, 24);
	LJUMP(SetNextGetTreasureState); // fallthrough
}

PORT(SetNextGetTreasureState)
{
	SET_HL(gb, wWarioState);
	wr8(gb, wWarioState, alu_inc(gb, rd8(gb, wWarioState)));
	xor_a(gb);
	wr8(gb, wWarioStateCycles, 0);
	FARCALL(Func_1e6ea);
}

PORT(UpdateState_GetTreasureWalk)
{
	anim(gb, OAM1);
	bool right = ld_and(gb, wDirection);
	gb->cpu.b = 1;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);

	SET_HL(gb, wWarioStateCounter);
	uint8_t v = alu_dec(gb, rd8(gb, wWarioStateCounter));
	wr8(gb, wWarioStateCounter, v);
	if (v)
		return;
	SET_HL(gb, wWarioState);
	wr8(gb, wWarioState, alu_inc(gb, rd8(gb, wWarioState))); // WST_GET_TREASURE_TURN_FRONT_1
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
	frameset_by_dir(gb, ADDR_Frameset_1fc46a, ADDR_Frameset_1fc461);
	anim(gb, OAM2);
}

PORT_JUMP(UpdateState_GetTreasureTurnFront)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	ld_cp(gb, wWarioState, WST_GET_TREASURE_TURN_FRONT_2);
	if (ZF) {
		LJUMP(SetState_GetTreasureLookFront);
		return;
	}
	st(gb, wWarioState, WST_GET_TREASURE_LAUGH);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioClearGfx, ADDR_WarioClearGfx);
	set_oam(gb, BANK_OAM_1fc000, ADDR_OAM_1fc000);
	set_frameset(gb, ADDR_Frameset_1fc2ab);
	anim(gb, OAM2);
	RET();
}

PORT(UpdateState_GetTreasureLaugh)
{
	anim(gb, OAM2);
	if (!anim_ended(gb))
		return;
	st(gb, wWarioState, WST_GET_TREASURE_TURN_BACK);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
	frameset_by_dir(gb, ADDR_Frameset_1fc446, ADDR_Frameset_1fc439);
	anim(gb, OAM2);
}

PORT(UpdateState_GetTreasureTurnBack)
{
	anim(gb, OAM2);
	if (!anim_ended(gb))
		return;
	st(gb, wWarioState, WST_GET_TREASURE_OPEN);
	play_sfx(gb, SFX_03E);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioClearGfx, ADDR_WarioClearGfx);
	set_oam(gb, BANK_OAM_1fc000, ADDR_OAM_1fc000);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, ADDR_Frameset_1fc2c4);
	anim(gb, OAM2);
}

PORT(UpdateState_GetTreasureOpen)
{
	anim(gb, OAM2);
	if (!anim_ended(gb))
		return;
	st(gb, wWarioState, WST_GET_TREASURE_TURN_WALK);
	CALL(LoadCollectedTreasurePal_Level);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_1fc31b, ADDR_OAM_1fc31b);
	gb->cpu.a = alu_xor(gb, rd8(gb, wDirection), 1);
	wr8(gb, wDirection, gb->cpu.a);
	set_frameset(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Frameset_1fc453 : ADDR_Frameset_1fc45a);
	anim(gb, OAM2);
}

PORT_JUMP(UpdateState_GetTreasureTurnWalk)
{
	anim(gb, OAM2);
	if (!anim_ended(gb)) {
		RET();
		return;
	}
	st(gb, wWarioStateCounter, 32);
	xor_a(gb);
	wr8(gb, wAutoMoveState, 0);
	LJUMP(SetNextGetTreasureState);
}

PORT(SetState_GetTreasureLookFront)
{
	st(gb, wWarioState, WST_GET_TREASURE_LOOK_FRONT);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioClearGfx, ADDR_WarioClearGfx);
	set_oam(gb, BANK_OAM_1fc000, ADDR_OAM_1fc000);
	set_frameset(gb, ADDR_Frameset_1fc30d);
	anim(gb, OAM2);
}

PORT(UpdateState_GetTreasureLookFront)
{
	anim(gb, OAM2);
	if (!anim_ended(gb))
		return;
	st(gb, wWarioState, WST_GET_TREASURE_CLEAR);
	get_pos(gb);
	gb->cpu.b = PARTICLE_TREASURE_GLOW;
	FARCALL(CreateParticle);
	xor_a(gb);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
}

PORT(UpdateState_GetTreasureClear)
{
	SET_HL(gb, wWarioStateCounter);
	if (!ld_bit(gb, wJoypadDown, B_PAD_A)) {
		gb->cpu.a = alu_inc(gb, rd8(gb, wWarioStateCounter));
		wr8(gb, wWarioStateCounter, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 180);
		if (CF)
			return;
	}
	if (!ld_and(gb, wCurParticleAnimEnded))
		return;
	wr8(gb, wWarioStateCounter, 0);
	SET_HL(gb, wLevelEndScreen);
	wr8(gb, wLevelEndScreen, rd8(gb, wLevelEndScreen) & 0x7f);
}

PORT(SetState_Sleeping)
{
	st(gb, wWarioState, WST_SLEEPING);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14000, ADDR_OAM_14000);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_1421c, ADDR_Frameset_14225);
	anim(gb, OAM1);
}

PORT(UpdateState_Sleeping)
{
	anim(gb, OAM1);
	gb->cpu.a = alu_dec(gb, rd8(gb, wWarioStateCounter));
	if (ZF) {
		// .asm_1acd0
		if (anim_ended(gb))
			play_sfx(gb, SFX_SLEEP);
		if (!ld_and(gb, wJoypadDown))
			return;
		xor_a(gb);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		inc_counter(gb);
		frameset_by_dir(gb, ADDR_Frameset_14240, ADDR_Frameset_14249);
		anim(gb, OAM1);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (ZF) {
		// .asm_1ad0c
		if (!anim_ended(gb))
			return;
		FARCALL(SetState_Idling);
		return;
	}
	if (!anim_ended(gb))
		return;
	play_sfx(gb, SFX_SLEEP);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	inc_counter(gb);
	frameset_by_dir(gb, ADDR_Frameset_1422e, ADDR_Frameset_14237);
	anim(gb, OAM1);
}

PORT(SetState_LadderScratching)
{
	st(gb, wWarioState, WST_LADDER_SCRATCHING);
	coll_box_btlr(gb);
	xor_a(gb);
	wr8(gb, wJumpVelIndex, 0);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsGettingOffLadder, 0);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	frameset_by_dir(gb, ADDR_Frameset_15903, ADDR_Frameset_15924);
	anim(gb, OAM1);
}

PORT_JUMP(UpdateState_LadderScratching)
{
	anim(gb, OAM1);
	if (!ld_and(gb, wJoypadDown)) {
		if (!anim_ended(gb)) {
			RET();
			return;
		}
	}
	LJUMP(SetState_LadderIdling);
}

PORT(SetState_FenceShakeSliding)
{
	st(gb, wWarioState, WST_FENCE_SHAKE_SLIDING);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	st(gb, wAutoMoveState, 0x03);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	frameset_by_dir(gb, ADDR_Frameset_14d0b, ADDR_Frameset_14d10);
	anim(gb, OAM1);
}

PORT(UpdateState_FenceShakeSliding)
{
	anim(gb, OAM1);
	if (!ld_and(gb, wWarioStateCounter) && anim_ended(gb)) {
		xor_a(gb);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		set_frameset(gb, ADDR_Frameset_14d15);
		anim(gb, OAM1);
		inc_counter(gb);
	}
	if (!ld_and(gb, wJumpVelTable))
		return;
	CALL(ApplyJumpVelocity);
	FARCALL(CheckAirborneCollision);
	if (!b_and(gb))
		return;
	update_pos_y(gb);
	xor_a(gb);
	wr8(gb, wJumpVelTable, 0);
	wr8(gb, wJumpVelIndex, 0);
}

PORT(SetState_FenceMovingVertical)
{
	st(gb, wWarioState, WST_FENCE_MOVING_VERTICAL);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsSmashAttacking, 0);
	coll_box_blrt(gb);
	load_gfx(gb, BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
	set_oam(gb, BANK_OAM_155cc, ADDR_OAM_155cc);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	set_frameset(gb, ADDR_Frameset_158d6);
	anim(gb, OAM1);
}

PORT(UpdateState_FenceMovingVertical)
{
	sfx_loop(gb, 0x20, SFX_CLIMB);
	anim(gb, OAM1);
	if (anim_ended(gb)) {
		get_pos(gb);
		gb->cpu.b = PARTICLE_DUST;
		FARCALL(CreateParticle);
	}
	LCALL(Func_1b168);
}

PORT(SetState_FenceIdling)
{
	st(gb, wWarioState, WST_FENCE_IDLING);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsSmashAttacking, 0);
	coll_box_blrt(gb);
	load_gfx(gb, BANK_WarioLadderGfx, ADDR_WarioLadderGfx);
	set_oam(gb, BANK_OAM_155cc, ADDR_OAM_155cc);
	frameset_by_dir(gb, ADDR_Frameset_15900, ADDR_Frameset_158fd);
	anim(gb, OAM1);
}

PORT(UpdateState_FenceIdling)
{
	anim(gb, OAM1);
	LCALL(Func_1b21a);
}

PORT(SetState_FenceMovingHorizontal)
{
	st(gb, wWarioState, WST_FENCE_MOVING_HORIZONTAL);
	xor_a(gb);
	wr8(gb, wSFXLoopCounter, 0);
	wr8(gb, wWarioStateCounter, 0);
	wr8(gb, wWarioStateCycles, 0);
	wr8(gb, wIsSmashAttacking, 0);
	coll_box_blrt(gb);
	xor_a(gb);
	wr8(gb, wFrameDuration, 0);
	wr8(gb, wAnimationFrame, 0);
	bool right;
	if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT))
		right = true;
	else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT))
		right = false;
	else
		right = ld_and(gb, wDirection);
	set_frameset(gb, right ? ADDR_Frameset_158e7 : ADDR_Frameset_158f2);
	anim(gb, OAM1);
}

PORT(UpdateState_FenceMovingHorizontal)
{
	FARCALL(Func_19afb);
	if (!ld_and(gb, wIsNearFence)) {
		FARCALL(StartFall);
		return;
	}
	sfx_loop(gb, 0x20, SFX_CLIMB);
	anim(gb, OAM1);
	LCALL(Func_1b24f);
}

PORT(SetState_FenceSliding)
{
	set_state_sliding(gb, WST_FENCE_SLIDING);
}

PORT_JUMP(UpdateState_FenceSliding)
{
	update_sliding(gb, true);
}

// The jump input check at the start of Func_1b168, Func_1b21a and
// Func_1b24f. Returns true if it jumped (and returned).
static bool fence_jump_input(GB *gb)
{
	if (!ld_and(gb, wJumpingUpwards) && ld_bit(gb, wJoypadDown, B_PAD_A))
		goto jump;
	if (ld_bit(gb, wJoypadPressed, B_PAD_A))
		goto jump;
	return false;
jump:
	FARCALL(StartJump_FromInput);
	RET();
	return true;
}

PORT_JUMP(Func_1b168)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_B | PAD_DOWN);
	alu_cp(gb, gb->cpu.a, PAD_B | PAD_DOWN);
	if (ZF) {
		LJUMP(SetState_FenceSliding);
		return;
	}
	if (fence_jump_input(gb))
		return;

	if (ld_bit(gb, wJoypadDown, B_PAD_UP)) {
		FARCALL(Func_19ae4);
		if (!ld_and(gb, wIsNearFence)) {
			RET();
			return;
		}
		gb->cpu.b = 0x01;
		CALL(MoveWarioUp);
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		gb->cpu.b = 0x01;
		CALL(MoveWarioDown);
		FARCALL(CheckCentreCollision);
		if (b_and(gb)) {
			update_pos_y(gb);
			FARCALL(SetState_Idling);
			RET();
			return;
		}
		FARCALL(Func_19afb);
		if (ld_and(gb, wIsNearFence)) {
			RET();
			return;
		}
		FARCALL(StartFall);
		RET();
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, PAD_RIGHT | PAD_LEFT);
	if (gb->cpu.a)
		LJUMP(SetState_FenceMovingHorizontal);
	else
		LJUMP(SetState_FenceIdling);
}

PORT_JUMP(Func_1b21a)
{
	if (fence_jump_input(gb))
		return;
	if (ld_mask(gb, wJoypadDown, PAD_UP | PAD_DOWN)) {
		LJUMP(SetState_FenceMovingVertical);
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_FenceMovingHorizontal);
		return;
	}
	RET();
}

PORT_JUMP(Func_1b24f)
{
	if (fence_jump_input(gb))
		return;
	bool right;
	if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT)) {
		right = true;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		right = false;
	} else {
		gb->cpu.a = alu_and(gb, gb->cpu.a, PAD_UP | PAD_DOWN);
		if (gb->cpu.a)
			LJUMP(SetState_FenceMovingVertical);
		else
			LJUMP(SetState_FenceIdling);
		return;
	}
	st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
	FARCALL(CheckFrontCollision);
	if (b_and(gb)) {
		RET();
		return;
	}
	gb->cpu.b = 0x01;
	if (right)
		CALL(MoveWarioRight);
	else
		CALL(MoveWarioLeft);
	RET();
}

PORT_JUMP(Func_1b2c0)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_SandJumping);
		return;
	}
	LCALL(Func_1b302);
	FARCALL(CheckFrontCollision);
	if (b_and(gb)) {
		RET();
		return;
	}
	FARCALL(Func_2b17a);
	clamp_walk_vel(gb);
	if (ld_and(gb, wIsTurning))
		LCALL(Func_1a14b);
	RET();
}

PORT(Func_1b302)
{
	FARCALL(CheckCentreCollision);
	if (b_and(gb))
		return;
	gb->cpu.b = 0x01;
	if (ld_bit(gb, wJoypadDown, B_PAD_DOWN))
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
	CALL(MoveWarioDown);
}

PORT_JUMP(Func_1b322)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_SandJumping);
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		LJUMP(SetState_SandWalking);
		return;
	}
	RET();
}

PORT_JUMP(Func_1b333)
{
	if (ld_bit(gb, wJoypadPressed, B_PAD_A)) {
		LJUMP(SetState_SandJumping);
		return;
	}
	bool right;
	if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT)) {
		right = true;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		right = false;
	} else {
		LJUMP(SetState_SandIdling);
		return;
	}
	// (buggy: wDirection was already switched by the sand idling input
	// handling, so Sand Turning is never reached)
	if (ld_and(gb, wDirection) != right) {
		LJUMP(SetState_SandTurning);
		return;
	}
	st(gb, wDirection, right ? DIRECTION_RIGHT : DIRECTION_LEFT);
	FARCALL(CheckFrontCollision);
	if (b_and(gb)) {
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
	clamp_walk_vel(gb);
	RET();
}

PORT_JUMP(HandleLadderInput)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_B | PAD_DOWN);
	alu_cp(gb, gb->cpu.a, PAD_B | PAD_DOWN);
	if (ZF) {
		LJUMP(SetState_LadderSliding);
		return;
	}
	if (ld_bit(gb, wJoypadDown, B_PAD_UP)) {
		FARCALL(CheckUpCollision);
		if (ld_and(gb, wJumpCollision)) {
			RET();
			return;
		}
		gb->cpu.b = 1;
		CALL(MoveWarioUp);
		gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
		alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
		if (!CF) {
			CALL(GetFloorForYPos);
			gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
			if (CF) {
				JUMP(StartUpwardsFloorTransition);
				return;
			}
		}
		if (ld_and(gb, wLadderInteraction)) {
			RET();
			return;
		}
		FARCALL(Func_19b12);
		if (ld_and(gb, wLadderInteraction)) {
			RET();
			return;
		}
		goto idle;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		gb->cpu.b = 0x01;
		CALL(MoveWarioDown);
		if (down_floor_check(gb)) {
			JUMP(StartDownwardsFloorTransition);
			return;
		}
		FARCALL(Func_19b12);
		if (ld_and(gb, wLadderInteraction)) {
			FARCALL(CheckCentreCollision);
			if (b_and(gb))
				center_on_ladder(gb);
			RET();
			return;
		}
		// .no_more_ladder_interaction
		FARCALL(CheckCentreCollision);
		if (b_and(gb))
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

PORT_JUMP(Func_1b480)
{
	if (ld_bit(gb, wJoypadDown, B_PAD_B) && ld_mask(gb, wJoypadDown, PAD_RIGHT | PAD_LEFT)) {
		// .asm_1b4a7
		bool ok;
		if (ld_bit(gb, wJoypadDown, B_PAD_RIGHT)) {
			st(gb, wCollisionBoxRight, 16);
			FARCALL(Func_19741);
			ok = !b_and(gb);
		} else {
			st(gb, wCollisionBoxLeft, (uint8_t)-16);
			FARCALL(Func_197b1);
			ok = !b_and(gb);
		}
		wr8(gb, wCollisionBoxLeft, (uint8_t)-9);
		st(gb, wCollisionBoxRight, 9);
		if (ok)
			LJUMP(SetState_GettingOffLadder);
		else
			RET();
		return;
	}
	if (ld_mask(gb, wJoypadDown, PAD_UP | PAD_DOWN)) {
		LJUMP(SetState_LadderClimbing);
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioStateCounter), 1);
	wr8(gb, wWarioStateCounter, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, wWarioStateCounter + 1), 0);
	wr8(gb, wWarioStateCounter + 1, gb->cpu.a);
	SET_HL(gb, wWarioStateCounter + 1);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (CF) {
		RET();
		return;
	}
	LJUMP(SetState_LadderScratching);
}
