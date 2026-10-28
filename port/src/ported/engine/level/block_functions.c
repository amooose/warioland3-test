// Port of src/engine/level/block_functions.asm
#include "wario_states.h"

// Jumps between block functions use JUMP_TO: like the asm, they run whatever
// bank is mapped.

// processes the block function of wBlockPtrBank:wBlockPtr
PORT_JUMP(ProcessBlock)
{
	wr8(gb, wIsInWaterOrSand, FALSE);
	gb->cpu.a = rd8(gb, wBlockPtrBank);
	sramswitch(gb, gb->cpu.a);

	// the block at wBlockPtr (stored big-endian) indexes the table of block
	// functions, which the level loads into wBlockFunctionTable
	uint16_t block_ptr = (uint16_t)(rd8(gb, wBlockPtr + 0) << 8 | rd8(gb, wBlockPtr + 1));
	uint8_t offset = alu_add(gb, rd8(gb, block_ptr), rd8(gb, block_ptr));
	gb->cpu.d = 0;
	gb->cpu.e = offset;
	uint16_t entry = alu_add16(gb, wBlockFunctionTable, offset);
	uint16_t func = rd16(gb, entry);
	gb->cpu.a = (uint8_t)func;
	SET_HL(gb, func);

	// jp hl: the block function returns to our caller
	JUMP_TO(func);
}

// output:
// - a = $00
PORT(BlockFunc_Free)
{
	sramswitch(gb, 0); // SRAM0
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);
	wr8(gb, wUnused_c18d, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// output:
// - a = $00 or $01
PORT(BlockFunc_Solid)
{
	sramswitch(gb, 0); // SRAM0
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);

	// Wario standing on a slope keeps his y position; everything else is
	// snapped to the top of the block.
	if (alu_and(gb, rd8(gb, wBlockFuncWarioFlag), 0xff) &&
		alu_and(gb, rd8(gb, wWarioSlopeInteraction), 0xff) &&
		alu_bit(gb, rd8(gb, wCollisionPoints), COLLISION_UNK4_F)) {
		wr8(gb, wUnused_c18d, 0);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}

	// .snap_y_pos
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = 1;
	wr8(gb, wUnused_c18d, 1);
}

PORT_JUMP(BlockFunc_Water)
{
	gb->cpu.a = TRUE;
	wr8(gb, wIsInWaterOrSand, TRUE);
	gb->cpu.a = rd8(gb, wBlockFuncWarioFlag);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_BlockFunc_Free);
		return;
	}
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (!alu_bit(gb, gb->cpu.a, COLLISION_UNK7_F)) {
		JUMP_TO(ADDR_BlockFunc_Free);
		return;
	}
	gb->cpu.a = SUBMERSIBLE_WATER;
	wr8(gb, wWaterInteraction, SUBMERSIBLE_WATER);
	JUMP_TO(ADDR_BlockFunc_Free);
}

// Sets wWaterCurrent if Wario's in the current, then it's water.
static void block_func_current(GB *gb, uint8_t current)
{
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (alu_bit(gb, gb->cpu.a, COLLISION_UNK7_F)) {
		gb->cpu.a = current;
		wr8(gb, wWaterCurrent, current);
	}
	JUMP_TO(ADDR_BlockFunc_Water);
}

PORT_JUMP(BlockFunc_RightCurrent) { block_func_current(gb, CURRENT_RIGHT); }
PORT_JUMP(BlockFunc_UpCurrent) { block_func_current(gb, CURRENT_UP); }
PORT_JUMP(BlockFunc_LeftCurrent) { block_func_current(gb, CURRENT_LEFT); }
PORT_JUMP(BlockFunc_DownCurrent) { block_func_current(gb, CURRENT_DOWN); }

PORT_JUMP(BlockFunc_Floor)
{
	gb->cpu.a = TRUE;
	wr8(gb, wc1ca, TRUE);
	gb->cpu.a = rd8(gb, wCollisionPoints);
	bool solid = alu_bit(gb, gb->cpu.a, COLLISION_DOWN_F) || alu_bit(gb, gb->cpu.a, COLLISION_UNK2_F);
	if (!solid) {
		gb->cpu.a = alu_and(gb, gb->cpu.a,
			COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7);
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP_TO(ADDR_BlockFunc_Free);
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0x0f);
		alu_cp(gb, gb->cpu.a, 0x05);
		if (!(gb->cpu.f & FLAG_C)) {
			JUMP_TO(ADDR_BlockFunc_Free);
			return;
		}
	}
	// .asm_18228
	gb->cpu.a = rd8(gb, wBlockFuncWarioFlag);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_BlockFunc_Solid);
		return;
	}
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, WST_ZOMBIE_LANDING);
	if (!(gb->cpu.f & FLAG_Z))
		alu_cp(gb, gb->cpu.a, WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_BlockFunc_Free);
		return;
	}
	JUMP_TO(ADDR_BlockFunc_Solid);
}

// output:
// - a = $00
PORT(Func_1808c)
{
	sramswitch(gb, 0); // SRAM0
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);
	wr8(gb, wUnused_c18d, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT_JUMP(Func_182ec)
{
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (!alu_bit(gb, gb->cpu.a, COLLISION_UNK7_F)) {
		JUMP_TO(ADDR_BlockFunc_Floor);
		return;
	}
	gb->cpu.a = LADDER_UNDERNEATH;
	wr8(gb, wLadderInteraction, gb->cpu.a);
	JUMP_TO(ADDR_Func_1808c);
}

// ld a, HIGH(ptr) / ld [addr + 0], a / ld a, LOW(ptr) / ld [addr + 1], a
static void st_ptr_be(GB *gb, uint16_t addr, uint16_t ptr)
{
	wr8(gb, addr + 0, (uint8_t)(ptr >> 8));
	gb->cpu.a = (uint8_t)ptr;
	wr8(gb, addr + 1, gb->cpu.a);
}

PORT_JUMP(EnterDoor)
{
	// centre on the door
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0), 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
	play_sfx(gb, SFX_0E6);

	gb->cpu.a = WST_ENTERING_DOOR;
	wr8(gb, wWarioState, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	static const uint16_t zero[] = {wSFXLoopCounter, wWarioStateCounter, wGrabState, wAttackCounter,
		wJumpVelIndex, wJumpVelTable, wIsCrouching, wFrameDuration, wAnimationFrame};
	for (unsigned i = 0; i < sizeof(zero) / sizeof(zero[0]); i++)
		wr8(gb, zero[i], 0);

	wr8(gb, wDMASourceBank, BANK_WarioIdleGfx);
	st_ptr_be(gb, wDMASourcePtr, ADDR_WarioIdleGfx);
	CALL(LoadWarioGfx);
	wr8(gb, wWarioOAMBank, BANK_OAM_14a82);
	st_ptr_be(gb, wWarioOAMPtr, ADDR_OAM_14a82);
	gb->cpu.a = rd8(gb, wDirection);
	bool right = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	st_ptr_be(gb, wWarioFramesetPtr, right ? ADDR_Frameset_14d10 : ADDR_Frameset_14d0b);
	FARCALL_BANK(BANK_OAM_14000, UpdateWarioAnimation); // BANK("Wario OAM 1")
	JUMP_TO(ADDR_BlockFunc_Free);
}

// --- more block functions ---

#define GOTO(addr) do { JUMP_TO(addr); return; } while (0)
#define FREE ADDR_BlockFunc_Free
#define SOLID ADDR_BlockFunc_Solid
#define WATER ADDR_BlockFunc_Water
#define FLOOR ADDR_BlockFunc_Floor

// ld a, [wBlockFuncWarioFlag] / and a; true if Wario
static bool wario_flag(GB *gb)
{
	return ld_and(gb, wBlockFuncWarioFlag);
}

// ld a, [wCollisionPoints] / bit n, a
static bool coll_bit(GB *gb, int n)
{
	return ld_bit(gb, wCollisionPoints, n);
}

// The SRAM0 / clear flags preamble with the given a for wUnused_c18d
static void block_func_c18d(GB *gb, uint8_t v)
{
	sramswitch(gb, 0); // SRAM0
	xor_a(gb);
	wr8(gb, wBlockFuncBreakFlag, FALSE);
	wr8(gb, wBlockFuncSpecialCollision, 0);
	gb->cpu.a = v;
	wr8(gb, wUnused_c18d, v);
}

PORT(Func_18064) { block_func_c18d(gb, 0x10); }
PORT(Func_18078) { block_func_c18d(gb, 0x01); }

// Func_180b5 / Func_1813f: a slope one block up, solid to an air attack
static void slope_above(GB *gb, uint16_t collision)
{
	if (wario_flag(gb)) {
		ld_cp(gb, wWarioState, WST_ATTACKING_AIRBORNE);
		if (ZF)
			GOTO(SOLID);
	}
	if (!ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		GOTO(FREE);
	gb->cpu.a = alu_sub(gb, rd8(gb, hYPosLo), 0x10);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hYPosHi), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	JUMP_TO(collision);
}

PORT_JUMP(BlockFunc_180b5) { slope_above(gb, ADDR_RightSlopeCollision); }
PORT_JUMP(BlockFunc_1813f) { slope_above(gb, ADDR_LeftSlopeCollision); }

// BlockFunc_1809f / BlockFunc_18129: a slope one block down
static void slope_below(GB *gb, uint16_t collision)
{
	if (!coll_bit(gb, COLLISION_DOWN_F))
		GOTO(FREE);
	gb->cpu.a = alu_add(gb, rd8(gb, hYPosLo), 0x10);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hYPosHi), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	JUMP_TO(collision);
}

PORT_JUMP(BlockFunc_1809f) { slope_below(gb, ADDR_RightSlopeCollision); }
PORT_JUMP(BlockFunc_18129) { slope_below(gb, ADDR_LeftSlopeCollision); }

// sets the slope interaction of Wario or the object
static void set_slope_interaction(GB *gb, uint8_t slope)
{
	st(gb, wario_flag(gb) ? wWarioSlopeInteraction : wObjSlopeInteraction, ON_SLOPE | slope);
}

// hYPos = the top of the block + c
static void hypos_top_plus_c(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0xf0);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hYPosHi), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
}

PORT_JUMP(RightSlopeCollision)
{
	gb->cpu.c = alu_and(gb, rd8(gb, hXPosLo), 0x0f);
	gb->cpu.a = alu_sub(gb, 0x10, gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	hypos_top_plus_c(gb);
	if (ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		set_slope_interaction(gb, RIGHT_SLOPE);
	JUMP_TO(ADDR_Func_18064);
}

PORT_JUMP(LeftSlopeCollision)
{
	if (ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1)) {
		gb->cpu.a = alu_and(gb, rd8(gb, hXPosLo), 0x0f);
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		gb->cpu.c = gb->cpu.a;
		hypos_top_plus_c(gb);
		set_slope_interaction(gb, LEFT_SLOPE);
	}
	JUMP_TO(ADDR_Func_18064);
}

PORT_JUMP(BlockFunc_RightSlope)
{
	if (!ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK1_F))
		GOTO(ADDR_RightSlopeCollision);
	gb->cpu.b = alu_and(gb, rd8(gb, hYPosLo), 0x0f);
	gb->cpu.a = alu_and(gb, rd8(gb, hXPosLo), 0x0f);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x10);
	JUMP_TO(CF ? FREE : ADDR_RightSlopeCollision);
}

PORT_JUMP(BlockFunc_LeftSlope)
{
	if (!ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		GOTO(FREE);
	if (!alu_bit(gb, gb->cpu.a, COLLISION_UNK1_F))
		GOTO(ADDR_LeftSlopeCollision);
	gb->cpu.b = alu_and(gb, rd8(gb, hYPosLo), 0x0f);
	gb->cpu.a = alu_and(gb, rd8(gb, hXPosLo), 0x0f);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	JUMP_TO(CF ? ADDR_LeftSlopeCollision : FREE);
}

PORT_JUMP(BlockFunc_WaterSurface)
{
	if (coll_bit(gb, COLLISION_UNK1_F))
		GOTO(FLOOR);
	if (ld_mask(gb, hYPosLo, 0x0e))
		GOTO(WATER);
	if (!ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK2))
		GOTO(WATER);
	st(gb, wIsInWaterOrSand, TRUE);
	JUMP_TO(SOLID);
}

PORT_JUMP(BlockFunc_18262)
{
	if (wario_flag(gb))
		GOTO(FREE);
	st(gb, wc1ca, TRUE);
	gb->cpu.a = rd8(gb, wCollisionPoints);
	if (alu_bit(gb, gb->cpu.a, COLLISION_DOWN_F))
		GOTO(SOLID);
	if (alu_bit(gb, gb->cpu.a, COLLISION_UNK2_F))
		GOTO(SOLID);
	gb->cpu.a = alu_and(gb, gb->cpu.a, COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7);
	if (gb->cpu.a)
		GOTO(FREE);
	gb->cpu.a = alu_and(gb, rd8(gb, hYPosLo), 0x0f);
	alu_cp(gb, gb->cpu.a, 0x05);
	JUMP_TO(CF ? SOLID : FREE);
}

PORT_JUMP(BlockFunc_Ladder)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(FREE);
	st(gb, wLadderInteraction, LADDER_COLLISION);
	JUMP_TO(ADDR_Func_1808c);
}

PORT_JUMP(BlockFunc_182dc)
{
	JUMP_TO(wario_flag(gb) ? ADDR_Func_182ec : SOLID);
}

PORT_JUMP(BlockFunc_LadderTop)
{
	JUMP_TO(wario_flag(gb) ? ADDR_Func_182ec : FLOOR);
}

static void ladder_transition(GB *gb, uint8_t param)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(FREE);
	st(gb, wLadderInteraction, LADDER_COLLISION);
	st(gb, wRoomTransitionParam, param);
	JUMP_TO(FREE);
}

PORT_JUMP(BlockFunc_182fe) { ladder_transition(gb, ROOMTRANSITION_5 | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_JUMP(Func_1831a) { ladder_transition(gb, ROOMTRANSITION_5); }

static void spike(GB *gb, uint16_t next)
{
	if (!wario_flag(gb))
		GOTO(next);
	if (ld_and(gb, wInvincibleCounter))
		GOTO(next);
	ld_cp(gb, wca94, 0x02);
	if (ZF)
		GOTO(next);
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, WST_STUNG);
	if (ZF)
		GOTO(next);
	alu_cp(gb, gb->cpu.a, WST_STUNG_RECOVERY);
	if (ZF)
		GOTO(next);
	// switch direction
	st(gb, wDirection, facing_right(gb) ? DIRECTION_LEFT : DIRECTION_RIGHT);
	if (!ld_and(gb, wca94)) {
		FARCALL(SetState_Stung);
		GOTO(next);
	}
	alu_cp(gb, gb->cpu.a, 0x01);
	if (ZF)
		FARCALL(Func_206eb);
	JUMP_TO(next);
}

PORT_JUMP(BlockFunc_NonSolidSpike) { spike(gb, FREE); }
PORT_JUMP(BlockFunc_SolidSpike) { spike(gb, SOLID); }

PORT_JUMP(BlockFunc_WaterSpike)
{
	if (!wario_flag(gb))
		GOTO(WATER);
	if (ld_and(gb, wInvincibleCounter))
		GOTO(WATER);
	ld_cp(gb, wWarioState, WST_WATER_STUNG);
	if (!ZF)
		FARCALL(SetState_WaterStung);
	JUMP_TO(WATER);
}

static void enter_pipe(GB *gb, bool down)
{
	st(gb, wRoomTransitionParam, ROOMTRANSITION_PIPE | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY);
	play_sfx(gb, SFX_PIPE_ENTER);
	st(gb, wWarioState, down ? WST_PIPE_GOING_DOWN : WST_PIPE_GOING_UP);
	static const uint16_t zero[] = {wFrameDuration, wAnimationFrame, wWarioStateCounter,
		wWarioStateCycles, wGrabState, wAttackCounter, wJumpVelIndex, wJumpVelTable,
		wIsCrouching, 0};
	clear(gb, zero);
	if (down)
		wr8(gb, wIsSmashAttacking, 0);
	load_gfx(gb, BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	set_oam(gb, BANK_OAM_14a82, ADDR_OAM_14a82);
	set_frameset(gb, ADDR_Frameset_14cf6);
	anim(gb, OAM1);
	JUMP_TO(SOLID);
}

PORT_JUMP(EnterPipeDown) { enter_pipe(gb, true); }
PORT_JUMP(EnterPipeUp) { enter_pipe(gb, false); }

// The pipe entrances: Wario in state1 or state2, touching the pipe on the
// collision side, holding the button
static void pipe(GB *gb, uint8_t state1, uint8_t state2, uint8_t coll, int button, bool left)
{
	if (!wario_flag(gb))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, state1);
	if (!ZF) {
		alu_cp(gb, gb->cpu.a, state2);
		if (!ZF)
			GOTO(SOLID);
	}
	if (!ld_mask(gb, wCollisionPoints, coll))
		GOTO(SOLID);
	if (!ld_bit(gb, wJoypadDown, button))
		GOTO(SOLID);
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0);
	gb->cpu.a = alu_add(gb, gb->cpu.a, left ? 0x18 : 0x08);
	wr8(gb, wWarioXPos + 1, gb->cpu.a);
	if (left) {
		gb->cpu.a = alu_adc(gb, rd8(gb, wWarioXPos + 0), 0x00);
		wr8(gb, wWarioXPos + 0, gb->cpu.a);
	}
	enter_pipe(gb, button == B_PAD_DOWN);
}

PORT_JUMP(BlockFunc_DownPipe_Left) { pipe(gb, WST_IDLING, WST_GRAB_IDLING, COLLISION_DOWN, B_PAD_DOWN, true); }
PORT_JUMP(BlockFunc_DownPipe_Right) { pipe(gb, WST_IDLING, WST_GRAB_IDLING, COLLISION_DOWN, B_PAD_DOWN, false); }
PORT_JUMP(BlockFunc_UpPipe_Left) { pipe(gb, WST_AIRBORNE, WST_GRAB_AIRBORNE, COLLISION_UNK5, B_PAD_UP, true); }
PORT_JUMP(BlockFunc_UpPipe_Right) { pipe(gb, WST_AIRBORNE, WST_GRAB_AIRBORNE, COLLISION_UNK5, B_PAD_UP, false); }

// A door: Wario in front of it pressing up while idling (or slipping)
static void door(GB *gb, bool slipping, uint8_t param)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(FREE);
	if (!ld_bit(gb, wJoypadPressed, B_PAD_UP))
		GOTO(FREE);
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, WST_IDLING);
	if (!ZF && slipping)
		alu_cp(gb, gb->cpu.a, WST_SLIPPING);
	if (!ZF) {
		alu_cp(gb, gb->cpu.a, WST_GRAB_IDLING);
		if (!ZF)
			GOTO(FREE);
	}
	st(gb, wRoomTransitionParam, param);
	JUMP_TO(ADDR_EnterDoor);
}

PORT_JUMP(BlockFunc_Door) { door(gb, true, ROOMTRANSITION_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); }
PORT_JUMP(BlockFunc_MinigameDoor) { door(gb, false, ROOMTRANSITION_GOLF | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); }
PORT_JUMP(Func_18624) { door(gb, false, ROOMTRANSITION_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_DELAY); }
PORT_JUMP(Func_1864e) { door(gb, false, ROOMTRANSITION_GOLF | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_DELAY); }

// An underwater door (water: Wario is in submersible water there)
static void underwater_door(GB *gb, bool water, uint8_t param)
{
	uint16_t next = water ? WATER : FREE;
	if (!wario_flag(gb))
		GOTO(next);
	ld_cp(gb, wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
	if (!ZF && alu_and(gb, gb->cpu.a, gb->cpu.a))
		GOTO(next);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(next);
	if (water)
		st(gb, wWaterInteraction, SUBMERSIBLE_WATER);
	if (ld_bit(gb, wJoypadPressed, B_PAD_UP))
		st(gb, wRoomTransitionParam, param);
	JUMP_TO(FREE);
}

PORT_JUMP(BlockFunc_UnderwaterDoor) { underwater_door(gb, true, ROOMTRANSITION_UNDERWATER_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_JUMP(BlockFunc_18727) { underwater_door(gb, true, ROOMTRANSITION_UNDERWATER_DOOR | ROOMTRANSITIONF_DOOR); }
PORT_JUMP(Func_18755) { underwater_door(gb, false, ROOMTRANSITION_8 | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); }

PORT_JUMP(BlockFunc_TreasureDoor)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!ld_bit(gb, wJoypadPressed, B_PAD_UP))
		GOTO(FREE);
	if (ld_and(gb, wTransformation))
		GOTO(FREE);
	gb->cpu.a = rd8(gb, wWarioState);
	alu_cp(gb, gb->cpu.a, WST_IDLING);
	if (!ZF)
		alu_cp(gb, gb->cpu.a, WST_SLIPPING);
	if (!ZF) {
		alu_cp(gb, gb->cpu.a, WST_GRAB_IDLING);
		if (!ZF)
			GOTO(FREE);
	}
	xor_a(gb);
	wr8(gb, wGrabState, 0);
	st(gb, wLevelEndScreen, LVLEND_NO_TREASURE);
	JUMP_TO(FREE);
}

static void transition_block(GB *gb, uint16_t next, uint8_t param)
{
	if (!wario_flag(gb))
		GOTO(next);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(next);
	st(gb, wRoomTransitionParam, param);
	JUMP_TO(next);
}

PORT_JUMP(BlockFunc_187b0) { transition_block(gb, FREE, ROOMTRANSITION_4 | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_JUMP(Func_187c7) { transition_block(gb, FREE, ROOMTRANSITION_4); }
PORT_JUMP(BlockFunc_187de) { transition_block(gb, WATER, ROOMTRANSITION_4 | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_JUMP(Func_187f5) { transition_block(gb, WATER, ROOMTRANSITION_4); }

PORT_JUMP(BlockFunc_WalkthroughPipe)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	ld_cp(gb, wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
	if (!ZF)
		GOTO(FREE);
	CALL(RecoverFromTransformation_WithoutInvincibility);
	JUMP_TO(FREE);
}

PORT_JUMP(BlockFunc_LightSource)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_ZOMBIE_WARIO);
	if (ZF) {
		FARCALL(SetState_ZombieRecovering);
		GOTO(FREE);
	}
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_VAMPIRE_WARIO);
	if (!ZF)
		GOTO(FREE);
	CALL(RecoverFromTransformation_WithoutInvincibility);
	JUMP_TO(FREE);
}

// Sand / non-submersible water: in it, and Wario's in it if he's touching it
static void in_block(GB *gb, uint16_t addr, uint8_t v)
{
	st(gb, wIsInWaterOrSand, TRUE);
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(FREE);
	st(gb, addr, v);
	JUMP_TO(FREE);
}

PORT_JUMP(BlockFunc_Sand) { in_block(gb, wIsInSand, TRUE); }
PORT_JUMP(BlockFunc_NonSubmersibleWater) { in_block(gb, wWaterInteraction, NON_SUBMERSIBLE_WATER); }

PORT_JUMP(BlockFunc_18868)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	ld_cp(gb, wTransformation, TRANSFORMATION_VAMPIRE_WARIO);
	if (!ZF)
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(SOLID);
	if (!ld_mask(gb, wWarioXPos + 1, 0x08)) {
		gb->cpu.b = ld(gb, wCollisionBoxRight);
		gb->cpu.a = alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		wr8(gb, wWarioXPos + 1, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioXPos + 0), 0x00);
	} else {
		gb->cpu.b = ld(gb, wCollisionBoxLeft);
		gb->cpu.a = alu_sub(gb, 0x10, gb->cpu.b);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_and(gb, rd8(gb, wWarioXPos + 1), 0xf0);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
		wr8(gb, wWarioXPos + 1, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, wWarioXPos + 0), 0x00);
	}
	wr8(gb, wWarioXPos + 0, gb->cpu.a);
	JUMP_TO(SOLID);
}

PORT_JUMP(BlockFunc_188ba) { JUMP_TO(wario_flag(gb) ? FREE : SOLID); }
PORT_JUMP(BlockFunc_189f1) { JUMP_TO(wario_flag(gb) ? FLOOR : FREE); }

PORT_JUMP(BlockFunc_Fire)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (ld_mask(gb, wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1 | COLLISION_UNK2 | COLLISION_UNK6))
		GOTO(FREE);
	if (ld_and(gb, wInvincibleCounter))
		GOTO(FREE);
	if (ld_bit(gb, wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		GOTO(FREE);
	st(gb, wTransformation, TRANSFORMATION_HOT_WARIO);
	st(gb, wTransformationProgress, 1);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_BUMP);
	st(gb, wca94, 0x02);
	st(gb, wTransformationDuration + 0, (uint8_t)(HOT_WARIO_DURATION >> 8));
	st(gb, wTransformationDuration + 1, (uint8_t)HOT_WARIO_DURATION);
	CALL(UpdateLevelMusic);
	FARCALL(SetState_OnFire_ResetStateCounter);
	RET();
}

PORT_JUMP(BlockFunc_Fence)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(FREE);
	st(gb, wIsNearFence, TRUE);
	JUMP_TO(ADDR_Func_1808c);
}

