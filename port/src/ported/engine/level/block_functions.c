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

PORT_JUMP(BlockFunc_AirCurrent)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	if (!coll_bit(gb, COLLISION_UNK7_F))
		GOTO(FREE);
	if (ld_and(gb, wInvincibleCounter))
		GOTO(FREE);
	st(gb, wIsInAirCurrent, TRUE);
	if (ld_and(gb, wTransformation))
		GOTO(FREE);
	st(gb, wTransformation, TRANSFORMATION_FAN);
	st(gb, wTouchState, TOUCH_BUMP);
	st(gb, wStingTouchState, TOUCH_VULNERABLE);
	st(gb, wca94, 0x01);
	FARCALL(SetState_FanStart);
	JUMP_TO(FREE);
}

static void slippery(GB *gb, uint16_t next)
{
	if (!wario_flag(gb))
		GOTO(next);
	if (!ld_mask(gb, wCollisionPoints, COLLISION_DOWN))
		GOTO(next);
	st(gb, wIsOnSlipperyGround, TRUE);
	JUMP_TO(next);
}

PORT_JUMP(BlockFunc_SlipperySolid) { slippery(gb, SOLID); }
PORT_JUMP(BlockFunc_SlipperyFloor) { slippery(gb, FLOOR); }

PORT_JUMP(BlockFunc_CloudPlatform1)
{
	gb->cpu.a = rd8(gb, wRoomPalCycleIndex);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (!ZF) {
		alu_cp(gb, gb->cpu.a, 0x04);
		if (!CF)
			GOTO(FREE);
	}
	JUMP_TO(FLOOR);
}

PORT_JUMP(BlockFunc_CloudPlatform2)
{
	ld_cp(gb, wRoomPalCycleIndex, 0x03);
	JUMP_TO(CF ? FREE : FLOOR);
}

PORT_JUMP(BlockFunc_SmallGap)
{
	if (!wario_flag(gb))
		GOTO(FREE);
	ld_cp(gb, wTransformation, TRANSFORMATION_FLAT_WARIO);
	JUMP_TO(ZF ? FREE : SOLID);
}

// --- switches and breakable blocks ---

// Toggles the switch block Wario hit (and with it every switchable block),
// then acts solid.
PORT_JUMP(BlockFunc_Switch)
{
	if (!wario_flag(gb))
		GOTO(SOLID);
	gb->cpu.a = alu_and(gb, rd8(gb, wCollisionPoints), COLLISION_UNK3 | COLLISION_UNK5);
	if (gb->cpu.f & FLAG_Z)
		GOTO(SOLID);
	uint16_t hl = rd8(gb, wBlockPtr) << 8 | rd8(gb, wBlockPtr + 1);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wSwitchState, gb->cpu.a);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, 0x08);
	wr8(gb, hl, gb->cpu.a);
	CALL(Func_e31);
	gb->cpu.a = alu_xor(gb, rd8(gb, wSwitchStateUpdated), 1);
	wr8(gb, wSwitchStateUpdated, gb->cpu.a);
	play_sfx(gb, SFX_SWITCH);
	gb->cpu.b = 16;
	CALL(DoGroundShake);
	GOTO(SOLID);
}

// BreakBlock (a coin 1 time in 4, unless on cooldown) and
// BreakBlockWithColourCoin: debris at the block in hPos and an object
// (params) created there, with hPos and wCurObj kept as they were.
static void break_block(GB *gb, bool chance, uint16_t params)
{
	// temporarily store hPos
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hYPosHiBackup, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hYPosLoBackup, gb->cpu.a);
	gb->cpu.a = rd8(gb, hXPosHi);
	wr8(gb, hXPosHiBackup, gb->cpu.a);
	gb->cpu.a = rd8(gb, hXPosLo);
	wr8(gb, hXPosLoBackup, gb->cpu.a);

	gb->cpu.b = PARTICLE_DEBRIS;
	FARCALL(CreateParticleInBlock);
	bool spawn = true;
	if (chance) {
		gb->cpu.a = alu_and(gb, rd8(gb, rDIV), 0x03);
		if (!(gb->cpu.f & FLAG_Z)) {
			spawn = false;
		} else {
			gb->cpu.a = rd8(gb, wCoinCooldown);
			alu_and(gb, gb->cpu.a, gb->cpu.a);
			spawn = gb->cpu.f & FLAG_Z;
		}
	}
	if (spawn) {
		// push_wram BANK("Level Objects WRAM")
		gb->cpu.a = rd8(gb, rWBK);
		push_af(gb);
		gb->cpu.a = 0x01;
		wr8(gb, rWBK, 0x01);
		if (chance) {
			gb->cpu.a = COIN_COOLDOWN;
			wr8(gb, wCoinCooldown, COIN_COOLDOWN);
		}

		// backup CurObj
		SET_HL(gb, wCurObjUnk01);
		SET_DE(gb, hCurObjBackup);
		gb->cpu.b = OBJ_ID - OBJ_UNK_01;
		CALL(CopyHLToDE);

		uint16_t hl = wCurObjUnk01;
		xor_a(gb);
		wr8(gb, hl++, 0);
		wr8(gb, hl++, 0);
		gb->cpu.a = rd8(gb, hYPosLo);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = rd8(gb, hXPosLo);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = rd8(gb, hXPosHi);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		SET_BC(gb, params);
		FARCALL(_CreateObjectFromCurObjPos);

		// restore CurObj
		SET_HL(gb, hCurObjBackup);
		SET_DE(gb, wCurObjUnk01);
		gb->cpu.b = OBJ_ID - OBJ_UNK_01;
		CALL(CopyHLToDE);
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
	}

	// restore hPos
	gb->cpu.a = rd8(gb, hYPosHiBackup);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosLoBackup);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hXPosHiBackup);
	wr8(gb, hXPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hXPosLoBackup);
	wr8(gb, hXPosLo, gb->cpu.a);
}

PORT(BreakBlock) { break_block(gb, true, ADDR_ObjParams_Coin); }
PORT(BreakBlockWithColourCoin) { break_block(gb, false, ADDR_ObjParams_ColourCoin); }

// wcedb / wcedc / wcedf = the SRAM bank, pointer and value of the block at
// wBlockPtr
static void save_block(GB *gb)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	wr8(gb, wcedb, gb->cpu.a);
	uint16_t hl = wBlockPtr;
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wcedc + 0, gb->cpu.a);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wcedc + 1, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	wr8(gb, wcedf, gb->cpu.a);
}

// The block at wBlockPtr = b (keeping bit 7), updated on screen with func
// (Func_e31 or Func_e2b)
static void replace_block(GB *gb, bool e2b)
{
	uint16_t hl = rd8(gb, wBlockPtr) << 8 | rd8(gb, wBlockPtr + 1);
	SET_HL(gb, hl);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), 0x80), gb->cpu.b);
	wr8(gb, hl, gb->cpu.a);
	gb_push16(gb, hl);
	if (e2b)
		CALL(Func_e2b);
	else
		CALL(Func_e31);
	SET_HL(gb, gb_pop16(gb));
}

// replace_block with Func_e31, then BreakBlock
static void replace_and_break(GB *gb)
{
	replace_block(gb, false);
	gb_call(gb, ADDR_BreakBlock); // same bank: whatever is mapped
}

// After breaking a block: free when rolling or fat, else solid unless
// strong enough (garlic, or the super jump slam overalls for a smash attack)
static void free_if_strong(GB *gb)
{
	if (ld_and(gb, wIsRolling))
		GOTO(FREE);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FAT_WARIO);
	if (gb->cpu.f & FLAG_Z)
		GOTO(FREE);
	bool smash = ld_and(gb, wIsSmashAttacking);
	gb->cpu.a = rd8(gb, wPowerUpLevel);
	alu_cp(gb, gb->cpu.a, smash ? SUPER_JUMP_SLAM_OVERALLS : GARLIC);
	GOTO((gb->cpu.f & FLAG_C) ? SOLID : FREE);
}

PORT_JUMP(Func_18f32)
{
	save_block(gb);
	replace_and_break(gb);
	GOTO(FREE);
}

PORT_JUMP(Func_19246)
{
	save_block(gb);
	replace_and_break(gb);
	GOTO(FREE);
}

PORT_JUMP(Func_19481)
{
	replace_and_break(gb);
	GOTO(FREE);
}

PORT_JUMP(Func_18df1)
{
	replace_and_break(gb);
	GOTO(ld_and(gb, wIsRolling) ? FREE : SOLID);
}

PORT_JUMP(Func_19086)
{
	save_block(gb);
	gb->cpu.a = TRUE;
	wr8(gb, wIsFloorTransition, TRUE);
	replace_and_break(gb);
	free_if_strong(gb);
}

PORT_JUMP(Func_19423)
{
	replace_block(gb, true);
	gb->cpu.a = rd8(gb, wCurObjFlags);
	if (alu_bit(gb, gb->cpu.a, OBJFLAG_ON_SCREEN_F))
		gb_call(gb, ADDR_BreakBlock); // same bank: whatever is mapped
	else
		play_sfx(gb, SFX_BLOCK_BREAK);
	GOTO(FREE);
}

PORT_JUMP(DoughnutBlockCollision)
{
	if (!wario_flag(gb))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FAT_WARIO);
	if (!(gb->cpu.f & FLAG_Z))
		GOTO(SOLID);
	gb->cpu.a = alu_and(gb, rd8(gb, wCollisionPoints), COLLISION_UNK1 | COLLISION_UNK6);
	if (gb->cpu.f & FLAG_Z)
		GOTO(SOLID);
	replace_and_break(gb);
	GOTO(SOLID);
}

// The robo mouse breaks (Func_19423) or passes through cracked blocks.
// Returns whether it jumped.
static bool robo_mouse(GB *gb)
{
	gb->cpu.a = rd8(gb, wBlockFuncSpecialCollision);
	alu_cp(gb, gb->cpu.a, COLLISION_ROBO_MOUSE_BREAK);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_19423);
		return true;
	}
	alu_cp(gb, gb->cpu.a, COLLISION_ROBO_MOUSE_FREE);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(FREE);
		return true;
	}
	return false;
}

// CrackedBlockCollision / CrackedBlockWithColourCoinCollision: b = the block
// that replaces it. Broken by an attack, a roll, a head smash (with the
// helmet) or a ground pound, through break_func.
static void cracked_block(GB *gb, uint16_t break_func)
{
	if (!ld_and(gb, wBlockFuncBreakFlag))
		GOTO(SOLID);
	bool brk;
	if (coll_bit(gb, COLLISION_UNK4_F)) {
		brk = ld_and(gb, wAttackCounter) || ld_and(gb, wIsRolling);
	} else if (coll_bit(gb, COLLISION_UNK5_F) || coll_bit(gb, COLLISION_UNK3_F)) {
		gb->cpu.a = rd8(gb, wPowerUpLevel);
		alu_cp(gb, gb->cpu.a, HEAD_SMASH_HELMET);
		if (gb->cpu.f & FLAG_C)
			GOTO(SOLID);
		brk = ld_and(gb, wJumpVelTable) || ld_and(gb, wSwimVelIndex);
	} else {
		gb->cpu.a = alu_and(gb, gb->cpu.a, COLLISION_UNK1 | COLLISION_UNK6);
		brk = !(gb->cpu.f & FLAG_Z);
	}
	if (!brk)
		GOTO(SOLID);
	replace_block(gb, false);
	gb_call(gb, break_func); // same bank: whatever is mapped
	free_if_strong(gb);
}

PORT_JUMP(CrackedBlockWithColourCoinCollision) { cracked_block(gb, ADDR_BreakBlockWithColourCoin); }

PORT_JUMP(CrackedBlockCollision)
{
	if (robo_mouse(gb))
		return;
	cracked_block(gb, ADDR_BreakBlock);
}

// Breaks with garlic and an attack, a roll, a head smash (with the helmet)
// or a ground pound (with the super jump slam overalls), or as Fat Wario
// with a ground pound
PORT_JUMP(NonCrackedBlockCollision)
{
	if (robo_mouse(gb))
		return;
	if (!ld_and(gb, wBlockFuncBreakFlag))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FAT_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_and(gb, rd8(gb, wCollisionPoints), COLLISION_UNK1 | COLLISION_UNK6);
		GOTO((gb->cpu.f & FLAG_Z) ? SOLID : ADDR_Func_18df1);
	}
	gb->cpu.a = rd8(gb, wPowerUpLevel);
	alu_cp(gb, gb->cpu.a, GARLIC);
	if (gb->cpu.f & FLAG_C)
		GOTO(SOLID);
	bool brk;
	if (coll_bit(gb, COLLISION_UNK4_F)) {
		brk = ld_and(gb, wAttackCounter) || ld_and(gb, wIsRolling);
	} else if (coll_bit(gb, COLLISION_UNK5_F) || coll_bit(gb, COLLISION_UNK3_F)) {
		gb->cpu.a = rd8(gb, wPowerUpLevel);
		alu_cp(gb, gb->cpu.a, HEAD_SMASH_HELMET);
		if (gb->cpu.f & FLAG_C)
			GOTO(SOLID);
		brk = ld_and(gb, wJumpVelTable) || ld_and(gb, wSwimVelIndex);
	} else {
		gb->cpu.a = alu_and(gb, gb->cpu.a, COLLISION_UNK1 | COLLISION_UNK6);
		if (gb->cpu.f & FLAG_Z)
			GOTO(SOLID);
		gb->cpu.a = rd8(gb, wPowerUpLevel);
		alu_cp(gb, gb->cpu.a, SUPER_JUMP_SLAM_OVERALLS);
		brk = !(gb->cpu.f & FLAG_C);
	}
	GOTO(brk ? ADDR_Func_18df1 : SOLID);
}

// Broken by a thrown object
PORT_JUMP(EnemyBlockCollision)
{
	gb->cpu.a = rd8(gb, wBlockFuncSpecialCollision);
	alu_cp(gb, gb->cpu.a, COLLISION_THROWN_OBJ);
	GOTO((gb->cpu.f & FLAG_Z) ? ADDR_Func_19423 : SOLID);
}

// Burnt by Hot Wario once engulfed in flames
PORT_JUMP(BonfireBlockCollision)
{
	if (!wario_flag(gb))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_HOT_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wTransformationProgress);
		alu_cp(gb, gb->cpu.a, 2);
		if (!(gb->cpu.f & FLAG_C))
			GOTO(ADDR_Func_19481); // engulfed in flames
	}
	GOTO(SOLID);
}

// ld b, block / jr CrackedBlockWithColourCoinCollision
static void cracked_colour_coin(GB *gb, uint8_t block)
{
	gb->cpu.b = block;
	JUMP_TO(ADDR_CrackedBlockWithColourCoinCollision);
}

PORT_JUMP(BlockFunc_CrackedBlockWithColourCoin_78) { cracked_colour_coin(gb, 0x78); }
PORT_JUMP(BlockFunc_CrackedBlockWithColourCoin_79) { cracked_colour_coin(gb, 0x79); }
PORT_JUMP(BlockFunc_CrackedBlockWithColourCoin_7a) { cracked_colour_coin(gb, 0x7a); }
PORT_JUMP(BlockFunc_CrackedBlockWithColourCoin_7b) { cracked_colour_coin(gb, 0x7b); }
PORT_JUMP(BlockFunc_CrackedBlockWithColourCoin_7c) { cracked_colour_coin(gb, 0x7c); }
PORT_JUMP(Func_18a0f) { cracked_colour_coin(gb, 0x7d); }
PORT_JUMP(BlockFunc_CrackedBlockWithColourCoin_7e) { cracked_colour_coin(gb, 0x7e); }
PORT_JUMP(Func_18a17) { cracked_colour_coin(gb, 0x7f); } // fallthrough

// --- multi-blocks: big blocks broken as a whole ---
// wMultiBlockParam bits 0-2 = the part already handled, bits 3-7 = a block
// of the same group was broken this frame

// ld b, block / jr target
static void ldb_jump(GB *gb, uint8_t block, uint16_t target)
{
	gb->cpu.b = block;
	JUMP_TO(target);
}

// ld a, n / ld [wMultiBlockParam], a
static void multi_set(GB *gb, uint8_t n)
{
	gb->cpu.a = n;
	wr8(gb, wMultiBlockParam, n);
}

// or n / ld [wMultiBlockParam], a
static void multi_or(GB *gb, uint8_t n)
{
	gb->cpu.a = alu_or(gb, gb->cpu.a, n);
	wr8(gb, wMultiBlockParam, gb->cpu.a);
}

// The common start: low bits set -> jump to low; high bits set -> or n and
// jump to high. Returns whether it jumped.
static bool multi_start(GB *gb, uint16_t low, uint8_t n, uint16_t high)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wMultiBlockParam), 0x07);
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP_TO(low);
		return true;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wMultiBlockParam), 0xf8);
	if (!(gb->cpu.f & FLAG_Z)) {
		multi_or(gb, n);
		JUMP_TO(high);
		return true;
	}
	return false;
}

enum { PROGRESS_ANY, PROGRESS_SNOWBALL, PROGRESS_BIG_SNOWBALL };

// Broken by Wario in the given transformation (as a snowball or a big
// snowball for the snowman). n = 3 falls through into the or.
static void transformation_multi(GB *gb, uint8_t n, uint8_t transformation, int progress)
{
	if (multi_start(gb, FREE, n, ADDR_Func_18f32))
		return;
	if (!wario_flag(gb))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, transformation);
	if (!(gb->cpu.f & FLAG_Z))
		GOTO(SOLID);
	if (progress != PROGRESS_ANY) {
		gb->cpu.a = rd8(gb, wTransformationProgress);
		if (progress == PROGRESS_SNOWBALL) {
			alu_cp(gb, gb->cpu.a, 1);
			if (gb->cpu.f & FLAG_C)
				GOTO(SOLID);
		} else {
			alu_cp(gb, gb->cpu.a, 3);
			if (!(gb->cpu.f & FLAG_Z))
				GOTO(SOLID);
		}
	}
	multi_set(gb, n);
	if (n == 3)
		multi_or(gb, 3);
	GOTO(ADDR_Func_18f32);
}

PORT_JUMP(Func_18aa8) { transformation_multi(gb, 1, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_SNOWBALL); }
PORT_JUMP(Func_18aec) { transformation_multi(gb, 3, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_SNOWBALL); }
PORT_JUMP(Func_18b2d) { transformation_multi(gb, 2, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_SNOWBALL); }
PORT_JUMP(Func_18b71) { transformation_multi(gb, 4, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_SNOWBALL); }
PORT_JUMP(Func_18bb5) { transformation_multi(gb, 1, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_BIG_SNOWBALL); }
PORT_JUMP(Func_18bf9) { transformation_multi(gb, 3, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_BIG_SNOWBALL); }
PORT_JUMP(Func_18c3a) { transformation_multi(gb, 2, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_BIG_SNOWBALL); }
PORT_JUMP(Func_18c7e) { transformation_multi(gb, 4, TRANSFORMATION_SNOWMAN_WARIO, PROGRESS_BIG_SNOWBALL); }
PORT_JUMP(Func_19522) { transformation_multi(gb, 1, TRANSFORMATION_BALL_O_STRING_WARIO, PROGRESS_ANY); }
PORT_JUMP(Func_19560) { transformation_multi(gb, 3, TRANSFORMATION_BALL_O_STRING_WARIO, PROGRESS_ANY); }
PORT_JUMP(Func_1959b) { transformation_multi(gb, 2, TRANSFORMATION_BALL_O_STRING_WARIO, PROGRESS_ANY); }
PORT_JUMP(YarnBlockCollision) { transformation_multi(gb, 4, TRANSFORMATION_BALL_O_STRING_WARIO, PROGRESS_ANY); }

// The robo mouse: passes through (free) or breaks it (n, then broken).
// Returns whether it jumped.
static bool multi_robo_mouse(GB *gb, uint8_t n, uint16_t broken)
{
	gb->cpu.a = rd8(gb, wBlockFuncSpecialCollision);
	alu_cp(gb, gb->cpu.a, COLLISION_ROBO_MOUSE_FREE);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(FREE);
		return true;
	}
	alu_cp(gb, gb->cpu.a, COLLISION_ROBO_MOUSE_BREAK);
	if (gb->cpu.f & FLAG_Z) {
		multi_set(gb, n);
		JUMP_TO(broken);
		return true;
	}
	return false;
}

// Cracked multi-block: broken by an attack, a roll, a head smash (with the
// helmet) or a ground pound
static void cracked_multi(GB *gb, uint8_t n)
{
	if (multi_start(gb, FREE, n, ADDR_Func_18f32))
		return;
	if (multi_robo_mouse(gb, n, ADDR_Func_18f32))
		return;
	if (!ld_and(gb, wBlockFuncBreakFlag))
		GOTO(SOLID);
	bool brk;
	if (coll_bit(gb, COLLISION_UNK4_F)) {
		brk = ld_and(gb, wAttackCounter) || ld_and(gb, wIsRolling);
	} else if (coll_bit(gb, COLLISION_UNK5_F) || coll_bit(gb, COLLISION_UNK3_F)) {
		gb->cpu.a = rd8(gb, wPowerUpLevel);
		alu_cp(gb, gb->cpu.a, HEAD_SMASH_HELMET);
		if (gb->cpu.f & FLAG_C)
			GOTO(SOLID);
		brk = ld_and(gb, wJumpVelTable) || ld_and(gb, wSwimVelIndex);
	} else {
		gb->cpu.a = alu_and(gb, gb->cpu.a, COLLISION_UNK1 | COLLISION_UNK6);
		brk = !(gb->cpu.f & FLAG_Z);
	}
	if (!brk)
		GOTO(SOLID);
	multi_set(gb, n);
	GOTO(ADDR_Func_19086);
}

PORT_JUMP(Func_18e2c) { cracked_multi(gb, 1); }
PORT_JUMP(Func_18ec0) { cracked_multi(gb, 3); }
PORT_JUMP(Func_18f7d) { cracked_multi(gb, 2); }
PORT_JUMP(Func_19011) { cracked_multi(gb, 4); }

// Non-cracked multi-block: needs garlic (and the super jump slam overalls
// for a roll or a ground pound), or Fat Wario's ground pound
static void noncracked_multi(GB *gb, uint8_t n)
{
	if (multi_start(gb, SOLID, n, ADDR_Func_19246))
		return;
	if (multi_robo_mouse(gb, n, ADDR_Func_19246))
		return;
	if (!ld_and(gb, wBlockFuncBreakFlag))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_FAT_WARIO);
	bool overalls; // needs the super jump slam overalls
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_and(gb, rd8(gb, wCollisionPoints), COLLISION_UNK1 | COLLISION_UNK6);
		if (gb->cpu.f & FLAG_Z)
			GOTO(SOLID);
		overalls = false;
	} else {
		gb->cpu.a = rd8(gb, wPowerUpLevel);
		alu_cp(gb, gb->cpu.a, GARLIC);
		if (gb->cpu.f & FLAG_C)
			GOTO(SOLID);
		if (coll_bit(gb, COLLISION_UNK4_F)) {
			if (ld_and(gb, wAttackCounter))
				overalls = false;
			else if (ld_and(gb, wIsRolling))
				overalls = true;
			else
				GOTO(SOLID);
		} else if (coll_bit(gb, COLLISION_UNK5_F) || coll_bit(gb, COLLISION_UNK3_F)) {
			gb->cpu.a = rd8(gb, wPowerUpLevel);
			alu_cp(gb, gb->cpu.a, HEAD_SMASH_HELMET);
			if (gb->cpu.f & FLAG_C)
				GOTO(SOLID);
			if (!ld_and(gb, wJumpVelTable) && !ld_and(gb, wSwimVelIndex))
				GOTO(SOLID);
			overalls = false;
		} else {
			gb->cpu.a = alu_and(gb, gb->cpu.a, COLLISION_UNK1 | COLLISION_UNK6);
			if (gb->cpu.f & FLAG_Z)
				GOTO(SOLID);
			overalls = true;
		}
	}
	if (overalls) {
		gb->cpu.a = rd8(gb, wPowerUpLevel);
		alu_cp(gb, gb->cpu.a, SUPER_JUMP_SLAM_OVERALLS);
		if (gb->cpu.f & FLAG_C)
			GOTO(SOLID);
	}
	multi_set(gb, n);
	GOTO(ADDR_Func_193dc);
}

PORT_JUMP(Func_190fe) { noncracked_multi(gb, 1); }
PORT_JUMP(Func_191b3) { noncracked_multi(gb, 3); }
PORT_JUMP(Func_19291) { noncracked_multi(gb, 2); }
PORT_JUMP(Func_19346) { noncracked_multi(gb, 4); }

PORT_JUMP(Func_193dc)
{
	save_block(gb);
	gb->cpu.a = TRUE;
	wr8(gb, wIsFloorTransition, TRUE);
	GOTO(ADDR_Func_18df1);
}

// Broken by Ball-o'-String Wario
PORT_JUMP(Func_19502)
{
	if (!wario_flag(gb))
		GOTO(SOLID);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BALL_O_STRING_WARIO);
	GOTO((gb->cpu.f & FLAG_Z) ? ADDR_Func_19481 : SOLID);
}

// --- ld b, block / jr <collision> entries ---
PORT_JUMP(Func_18a9a) { ldb_jump(gb, 0x79, ADDR_Func_18aa8); }
PORT_JUMP(Func_18a9e) { ldb_jump(gb, 0x7a, ADDR_Func_18aa8); }
PORT_JUMP(Func_18aa2) { ldb_jump(gb, 0x7b, ADDR_Func_18aa8); }
PORT_JUMP(BlockFunc_18aa6) { ldb_jump(gb, 0x78, ADDR_Func_18aa8); }
PORT_JUMP(Func_18ade) { ldb_jump(gb, 0x79, ADDR_Func_18aec); }
PORT_JUMP(Func_18ae2) { ldb_jump(gb, 0x7a, ADDR_Func_18aec); }
PORT_JUMP(Func_18ae6) { ldb_jump(gb, 0x7b, ADDR_Func_18aec); }
PORT_JUMP(BlockFunc_18aea) { ldb_jump(gb, 0x78, ADDR_Func_18aec); }
PORT_JUMP(BlockFunc_18b23) { ldb_jump(gb, 0x7a, ADDR_Func_18b2d); }
PORT_JUMP(BlockFunc_18b2b) { ldb_jump(gb, 0x78, ADDR_Func_18b2d); }
PORT_JUMP(BlockFunc_18b67) { ldb_jump(gb, 0x7a, ADDR_Func_18b71); }
PORT_JUMP(BlockFunc_18b6f) { ldb_jump(gb, 0x78, ADDR_Func_18b71); }
PORT_JUMP(BlockFunc_18bb3) { ldb_jump(gb, 0x78, ADDR_Func_18bb5); }
PORT_JUMP(BlockFunc_18bf7) { ldb_jump(gb, 0x78, ADDR_Func_18bf9); }
PORT_JUMP(BlockFunc_18c30) { ldb_jump(gb, 0x7a, ADDR_Func_18c3a); }
PORT_JUMP(BlockFunc_18c38) { ldb_jump(gb, 0x78, ADDR_Func_18c3a); }
PORT_JUMP(BlockFunc_18c74) { ldb_jump(gb, 0x7a, ADDR_Func_18c7e); }
PORT_JUMP(BlockFunc_18c7c) { ldb_jump(gb, 0x78, ADDR_Func_18c7e); }
PORT_JUMP(BlockFunc_CrackedBlock_78) { ldb_jump(gb, 0x78, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_79) { ldb_jump(gb, 0x79, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_7a) { ldb_jump(gb, 0x7a, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_7b) { ldb_jump(gb, 0x7b, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_7c) { ldb_jump(gb, 0x7c, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_7d) { ldb_jump(gb, 0x7d, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_7e) { ldb_jump(gb, 0x7e, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_CrackedBlock_7f) { ldb_jump(gb, 0x7f, ADDR_CrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_78) { ldb_jump(gb, 0x78, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_79) { ldb_jump(gb, 0x79, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_7a) { ldb_jump(gb, 0x7a, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_7b) { ldb_jump(gb, 0x7b, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_7c) { ldb_jump(gb, 0x7c, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_7d) { ldb_jump(gb, 0x7d, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_7e) { ldb_jump(gb, 0x7e, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_NonCrackedBlock_7f) { ldb_jump(gb, 0x7f, ADDR_NonCrackedBlockCollision); }
PORT_JUMP(BlockFunc_18e0e) { ldb_jump(gb, 0x79, ADDR_Func_18e2c); }
PORT_JUMP(BlockFunc_18e1a) { ldb_jump(gb, 0x7c, ADDR_Func_18e2c); }
PORT_JUMP(BlockFunc_18e26) { ldb_jump(gb, 0x7f, ADDR_Func_18e2c); }
PORT_JUMP(BlockFunc_18e2a) { ldb_jump(gb, 0x78, ADDR_Func_18e2c); }
PORT_JUMP(BlockFunc_18ea2) { ldb_jump(gb, 0x79, ADDR_Func_18ec0); }
PORT_JUMP(BlockFunc_18eae) { ldb_jump(gb, 0x7c, ADDR_Func_18ec0); }
PORT_JUMP(BlockFunc_18ebe) { ldb_jump(gb, 0x78, ADDR_Func_18ec0); }
PORT_JUMP(BlockFunc_18f5f) { ldb_jump(gb, 0x79, ADDR_Func_18f7d); }
PORT_JUMP(BlockFunc_18f6b) { ldb_jump(gb, 0x7c, ADDR_Func_18f7d); }
PORT_JUMP(BlockFunc_18f7b) { ldb_jump(gb, 0x78, ADDR_Func_18f7d); }
PORT_JUMP(BlockFunc_18ff3) { ldb_jump(gb, 0x79, ADDR_Func_19011); }
PORT_JUMP(BlockFunc_18fff) { ldb_jump(gb, 0x7c, ADDR_Func_19011); }
PORT_JUMP(BlockFunc_19007) { ldb_jump(gb, 0x7e, ADDR_Func_19011); }
PORT_JUMP(BlockFunc_1900f) { ldb_jump(gb, 0x78, ADDR_Func_19011); }
PORT_JUMP(BlockFunc_190e4) { ldb_jump(gb, 0x7a, ADDR_Func_190fe); }
PORT_JUMP(BlockFunc_190e8) { ldb_jump(gb, 0x7b, ADDR_Func_190fe); }
PORT_JUMP(BlockFunc_190fc) { ldb_jump(gb, 0x78, ADDR_Func_190fe); }
PORT_JUMP(BlockFunc_19199) { ldb_jump(gb, 0x7a, ADDR_Func_191b3); }
PORT_JUMP(BlockFunc_1919d) { ldb_jump(gb, 0x7b, ADDR_Func_191b3); }
PORT_JUMP(BlockFunc_191b1) { ldb_jump(gb, 0x78, ADDR_Func_191b3); }
PORT_JUMP(BlockFunc_19277) { ldb_jump(gb, 0x7a, ADDR_Func_19291); }
PORT_JUMP(BlockFunc_1927b) { ldb_jump(gb, 0x7b, ADDR_Func_19291); }
PORT_JUMP(BlockFunc_1928f) { ldb_jump(gb, 0x78, ADDR_Func_19291); }
PORT_JUMP(BlockFunc_1932c) { ldb_jump(gb, 0x7a, ADDR_Func_19346); }
PORT_JUMP(BlockFunc_19330) { ldb_jump(gb, 0x7b, ADDR_Func_19346); }
PORT_JUMP(BlockFunc_19344) { ldb_jump(gb, 0x78, ADDR_Func_19346); }
PORT_JUMP(BlockFunc_EnemyBlock_78) { ldb_jump(gb, 0x78, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_79) { ldb_jump(gb, 0x79, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_7a) { ldb_jump(gb, 0x7a, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_7b) { ldb_jump(gb, 0x7b, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_7c) { ldb_jump(gb, 0x7c, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_7d) { ldb_jump(gb, 0x7d, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_7e) { ldb_jump(gb, 0x7e, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_EnemyBlock_7f) { ldb_jump(gb, 0x7f, ADDR_EnemyBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_78) { ldb_jump(gb, 0x78, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_79) { ldb_jump(gb, 0x79, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_7a) { ldb_jump(gb, 0x7a, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_7b) { ldb_jump(gb, 0x7b, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_7c) { ldb_jump(gb, 0x7c, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_7d) { ldb_jump(gb, 0x7d, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_BonfireBlock_7e) { ldb_jump(gb, 0x7e, ADDR_BonfireBlockCollision); }
PORT_JUMP(BlockFunc_DoughnutBlock_78) { ldb_jump(gb, 0x78, ADDR_DoughnutBlockCollision); }
PORT_JUMP(BlockFunc_DoughnutBlock_79) { ldb_jump(gb, 0x79, ADDR_DoughnutBlockCollision); }
PORT_JUMP(BlockFunc_DoughnutBlock_7a) { ldb_jump(gb, 0x7a, ADDR_DoughnutBlockCollision); }
PORT_JUMP(BlockFunc_DoughnutBlock_7b) { ldb_jump(gb, 0x7b, ADDR_DoughnutBlockCollision); }
PORT_JUMP(BlockFunc_19518) { ldb_jump(gb, 0x7a, ADDR_Func_19522); }
PORT_JUMP(BlockFunc_19520) { ldb_jump(gb, 0x78, ADDR_Func_19522); }
PORT_JUMP(BlockFunc_19552) { ldb_jump(gb, 0x79, ADDR_Func_19560); }
PORT_JUMP(BlockFunc_19556) { ldb_jump(gb, 0x7a, ADDR_Func_19560); }
PORT_JUMP(BlockFunc_1955e) { ldb_jump(gb, 0x78, ADDR_Func_19560); }
PORT_JUMP(BlockFunc_1958d) { ldb_jump(gb, 0x79, ADDR_Func_1959b); }
PORT_JUMP(BlockFunc_19599) { ldb_jump(gb, 0x78, ADDR_Func_1959b); }
PORT_JUMP(BlockFunc_YarnBlock_79) { ldb_jump(gb, 0x79, ADDR_YarnBlockCollision); }
PORT_JUMP(BlockFunc_YarnBlock_78) { ldb_jump(gb, 0x78, ADDR_YarnBlockCollision); }
