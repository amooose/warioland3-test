// Port of src/home/wario_mechanics1.asm
#include "ported.h"

// Moves Wario by the next entry of his current jump velocity table:
// negative entries move him up, others down.
PORT(ApplyJumpVelocity)
{
	static const uint16_t tables[] = {
		ADDR_JumpVelTable_KnockBack,      // JUMP_VEL_KNOCK_BACK
		ADDR_JumpVelTable_Normal,         // JUMP_VEL_NORMAL
		ADDR_JumpVelTable_HighJump,       // JUMP_VEL_HIGH_JUMP
		ADDR_JumpVelTable_BouncyJump,     // JUMP_VEL_BOUNCY_JUMP
		ADDR_JumpVelTable_BouncyHighJump, // JUMP_VEL_BOUNCY_HIGH_JUMP
	};

	// a chain of dec a / jr z
	uint8_t a = rd8(gb, wJumpVelTable);
	int table = -1;
	for (int i = 0; i < 5; i++) {
		a = alu_dec(gb, a);
		if (!a) {
			table = i;
			break;
		}
	}
	gb->cpu.a = a;
	if (table < 0)
		return;

	gb->cpu.e = rd8(gb, wJumpVelIndex);
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, tables[table], gb->cpu.e);
	uint8_t vel = rd8(gb, hl);
	if (alu_bit(gb, vel, 7)) {
		// rising: cpl / inc a
		gb->cpu.f |= FLAG_N | FLAG_H;
		gb->cpu.b = alu_inc(gb, (uint8_t)~vel);
		gb->cpu.a = gb->cpu.b;
		CALL(MoveWarioUp);
		SET_HL(gb, wJumpVelIndex);
		wr8(gb, wJumpVelIndex, alu_inc(gb, rd8(gb, wJumpVelIndex)));
		return;
	}

	// falling
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wDoFullJump, FALSE);
	gb->cpu.b = vel;
	CALL(MoveWarioDown);
	SET_HL(gb, wJumpVelIndex);
	gb->cpu.a = alu_inc(gb, rd8(gb, wJumpVelIndex));
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, MAX_JUMP_VEL_INDEX);
	if (!(gb->cpu.f & FLAG_C))
		wr8(gb, wJumpVelIndex, MAX_JUMP_VEL_INDEX);
}

// output: b = walk velocity; advances wWalkVelIndex
PORT(ApplyWalkVelocity)
{
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_WalkVelTable, REG_DE(gb));
	gb->cpu.b = rd8(gb, hl);
	SET_HL(gb, wWalkVelIndex);
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	alu_cp(gb, gb->cpu.a, 0x1b);
	if (gb->cpu.f & FLAG_Z) {
		// .cap
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfc);
		wr8(gb, wWalkVelIndex, gb->cpu.a);
		return;
	}
	wr8(gb, wWalkVelIndex, alu_inc(gb, gb->cpu.a));
}

// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp CAM_TRANSITIONS:
// true (carry) if the camera doesn't do floor transitions
static bool no_floor_transitions(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	return gb->cpu.f & FLAG_C;
}

// call GetFloorForYPos / ld a, [wFloor] / sub c
static void sub_floor_for_y_pos(GB *gb)
{
	CALL(GetFloorForYPos);
	gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
}

PORT_JUMP(TriggerDownwardsFloorTransition)
{
	if (no_floor_transitions(gb)) {
		RET();
		return;
	}
	sub_floor_for_y_pos(gb);
	if (gb->cpu.f & (FLAG_Z | FLAG_C)) {
		RET();
		return;
	}
	JUMP(StartDownwardsFloorTransition);
}

static void trigger_floor_transition(GB *gb)
{
	if (no_floor_transitions(gb))
		return;
	sub_floor_for_y_pos(gb);
	if (gb->cpu.f & FLAG_Z)
		return;
	if (gb->cpu.f & FLAG_C)
		CALL(StartUpwardsFloorTransition);
	else
		CALL(StartDownwardsFloorTransition);
}

PORT(TriggerFloorTransition_SkipUpdateYPos)
{
	trigger_floor_transition(gb);
}

PORT(TriggerFloorTransition)
{
	// update_pos_y
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, wWarioYPos, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi + 1);
	wr8(gb, wWarioYPos + 1, gb->cpu.a);
	SET_HL(gb, hYPosHi + 1);
	SET_DE(gb, wWarioYPos + 1);
	trigger_floor_transition(gb); // fallthrough
}

PORT_JUMP(ApplySlopedWalkVelocity)
{
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x04;
		wr8(gb, wWalkVelIndex, 0x04);
	}
	JUMP(ApplyWalkVelocity);
}

PORT_JUMP(ApplyWalkVelocity_Right)
{
	gb->cpu.a = rd8(gb, wDirection);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wWalkVelIndex, 0);
		JUMP(ApplyWalkVelocity);
		return;
	}
	gb->cpu.a = rd8(gb, wWarioSlopeInteraction);
	if (!alu_bit(gb, gb->cpu.a, RIGHT_SLOPE_F)) {
		JUMP(ApplyWalkVelocity);
		return;
	}
	JUMP(ApplySlopedWalkVelocity); // fallthrough
}

PORT_JUMP(ApplyWalkVelocity_Left)
{
	gb->cpu.a = rd8(gb, wDirection);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wWalkVelIndex, 0);
		JUMP(ApplyWalkVelocity);
		return;
	}
	gb->cpu.a = rd8(gb, wWarioSlopeInteraction);
	if (!alu_bit(gb, gb->cpu.a, LEFT_SLOPE_F)) {
		JUMP(ApplyWalkVelocity);
		return;
	}
	JUMP(ApplySlopedWalkVelocity);
}

PORT_JUMP(ResetLevelMusicWarioPalsAndState)
{
	CALL(UpdateLevelMusic);
	SET_HL(gb, ADDR_WarioDefaultPal);
	CALL(SetWarioPal);
	gb->cpu.a = rd8(gb, wJumpVelTable);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		FARCALL(SetState_Idling);
	else
		FARCALL(StartFall); // .fall
	JUMP(PlayRecoverySFX);
}

PORT_JUMP(RecoverFromTransformation)
{
	CALL(ClearTransformationValues);
	gb->cpu.a = 0x10;
	wr8(gb, wInvincibleCounter, 0x10);
	JUMP(ResetLevelMusicWarioPalsAndState);
}

PORT_JUMP(RecoverFromTransformation_WithoutInvincibility)
{
	CALL(ClearTransformationValues);
	JUMP(ResetLevelMusicWarioPalsAndState); // fallthrough
}

PORT_JUMP(InvalidWarioStateReset) { JUMP(Init); }
