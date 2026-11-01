// home/wario_mechanics1 (src/home/wario_mechanics1.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeWarioMechanics1 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Moves Wario by the next entry of his current jump velocity table:
	// negative entries move him up, others down.
	void port_ApplyJumpVelocity()
	{
		static const uint16_t tables[] = {
			ADDR_JumpVelTable_KnockBack,      // JUMP_VEL_KNOCK_BACK
			ADDR_JumpVelTable_Normal,         // JUMP_VEL_NORMAL
			ADDR_JumpVelTable_HighJump,       // JUMP_VEL_HIGH_JUMP
			ADDR_JumpVelTable_BouncyJump,     // JUMP_VEL_BOUNCY_JUMP
			ADDR_JumpVelTable_BouncyHighJump, // JUMP_VEL_BOUNCY_HIGH_JUMP
		};

		// a chain of dec a / jr z
		uint8_t a = rd8(wJumpVelTable);
		int table = -1;
		for (int i = 0; i < 5; i++) {
			a = alu_.dec(a);
			if (!a) {
				table = i;
				break;
			}
		}
		cpu_.a = a;
		if (table < 0)
			return;

		cpu_.e = rd8(wJumpVelIndex);
		cpu_.d = 0;
		uint16_t hl = alu_.add16(tables[table], cpu_.e);
		uint8_t vel = rd8(hl);
		if (alu_.bit(vel, 7)) {
			// rising: cpl / inc a
			cpu_.f |= FLAG_N | FLAG_H;
			cpu_.b = alu_.inc((uint8_t)~vel);
			cpu_.a = cpu_.b;
			CALL(MoveWarioUp);
			SET_HL(gb_, wJumpVelIndex);
			wr8(wJumpVelIndex, alu_.inc(rd8(wJumpVelIndex)));
			return;
		}

		// falling
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		wr8(wDoFullJump, FALSE);
		cpu_.b = vel;
		CALL(MoveWarioDown);
		SET_HL(gb_, wJumpVelIndex);
		cpu_.a = alu_.inc(rd8(wJumpVelIndex));
		wr8(wJumpVelIndex, cpu_.a);
		alu_.cp(cpu_.a, MAX_JUMP_VEL_INDEX);
		if (!(cpu_.f & FLAG_C))
			wr8(wJumpVelIndex, MAX_JUMP_VEL_INDEX);
	}

	// output: b = walk velocity; advances wWalkVelIndex
	void port_ApplyWalkVelocity()
	{
		cpu_.a = rd8(wWalkVelIndex);
		cpu_.e = cpu_.a;
		cpu_.d = 0x00;
		uint16_t hl = alu_.add16(ADDR_WalkVelTable, REG_DE(gb_));
		cpu_.b = rd8(hl);
		SET_HL(gb_, wWalkVelIndex);
		cpu_.a = rd8(wWalkVelIndex);
		alu_.cp(cpu_.a, 0x1b);
		if (cpu_.f & FLAG_Z) {
			// .cap
			cpu_.a = alu_.and_(cpu_.a, 0xfc);
			wr8(wWalkVelIndex, cpu_.a);
			return;
		}
		wr8(wWalkVelIndex, alu_.inc(cpu_.a));
	}

	// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp CAM_TRANSITIONS:
	// true (carry) if the camera doesn't do floor transitions
	bool no_floor_transitions()
	{
		cpu_.a = alu_.and_(rd8(wCameraConfigFlags), CAM_SCROLLING_MASK);
		alu_.cp(cpu_.a, CAM_TRANSITIONS);
		return cpu_.f & FLAG_C;
	}

	// call GetFloorForYPos / ld a, [wFloor] / sub c
	void sub_floor_for_y_pos()
	{
		CALL(GetFloorForYPos);
		cpu_.a = alu_.sub(rd8(wFloor), cpu_.c);
	}

	void port_TriggerDownwardsFloorTransition()
	{
		if (no_floor_transitions()) {
			RET();
			return;
		}
		sub_floor_for_y_pos();
		if (cpu_.f & (FLAG_Z | FLAG_C)) {
			RET();
			return;
		}
		JUMP(StartDownwardsFloorTransition);
	}

	void trigger_floor_transition()
	{
		if (no_floor_transitions())
			return;
		sub_floor_for_y_pos();
		if (cpu_.f & FLAG_Z)
			return;
		if (cpu_.f & FLAG_C)
			CALL(StartUpwardsFloorTransition);
		else
			CALL(StartDownwardsFloorTransition);
	}

	void port_TriggerFloorTransition_SkipUpdateYPos()
	{
		trigger_floor_transition();
	}

	void port_TriggerFloorTransition()
	{
		// update_pos_y
		cpu_.a = rd8(hYPosHi);
		wr8(wWarioYPos, cpu_.a);
		cpu_.a = rd8(hYPosHi + 1);
		wr8(wWarioYPos + 1, cpu_.a);
		SET_HL(gb_, hYPosHi + 1);
		SET_DE(gb_, wWarioYPos + 1);
		trigger_floor_transition(); // fallthrough
	}

	void port_ApplySlopedWalkVelocity()
	{
		cpu_.a = rd8(wWalkVelIndex);
		alu_.cp(cpu_.a, 0x08);
		if (!(cpu_.f & FLAG_C)) {
			cpu_.a = 0x04;
			wr8(wWalkVelIndex, 0x04);
		}
		JUMP(ApplyWalkVelocity);
	}

	void port_ApplyWalkVelocity_Right()
	{
		cpu_.a = rd8(wDirection);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.a = 0;
			cpu_.f = FLAG_Z;
			wr8(wWalkVelIndex, 0);
			JUMP(ApplyWalkVelocity);
			return;
		}
		cpu_.a = rd8(wWarioSlopeInteraction);
		if (!alu_.bit(cpu_.a, RIGHT_SLOPE_F)) {
			JUMP(ApplyWalkVelocity);
			return;
		}
		JUMP(ApplySlopedWalkVelocity); // fallthrough
	}

	void port_ApplyWalkVelocity_Left()
	{
		cpu_.a = rd8(wDirection);
		if (alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.a = 0;
			cpu_.f = FLAG_Z;
			wr8(wWalkVelIndex, 0);
			JUMP(ApplyWalkVelocity);
			return;
		}
		cpu_.a = rd8(wWarioSlopeInteraction);
		if (!alu_.bit(cpu_.a, LEFT_SLOPE_F)) {
			JUMP(ApplyWalkVelocity);
			return;
		}
		JUMP(ApplySlopedWalkVelocity);
	}

	void port_ResetLevelMusicWarioPalsAndState()
	{
		CALL(UpdateLevelMusic);
		SET_HL(gb_, ADDR_WarioDefaultPal);
		CALL(SetWarioPal);
		cpu_.a = rd8(wJumpVelTable);
		alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.f & FLAG_Z)
			FARCALL(SetState_Idling);
		else
			FARCALL(StartFall); // .fall
		JUMP(PlayRecoverySFX);
	}

	void port_RecoverFromTransformation()
	{
		CALL(ClearTransformationValues);
		cpu_.a = 0x10;
		wr8(wInvincibleCounter, 0x10);
		JUMP(ResetLevelMusicWarioPalsAndState);
	}

	void port_RecoverFromTransformation_WithoutInvincibility()
	{
		CALL(ClearTransformationValues);
		JUMP(ResetLevelMusicWarioPalsAndState); // fallthrough
	}

	void port_InvalidWarioStateReset() { JUMP(Init); }
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeWarioMechanics1;

PORT_EX(ApplyJumpVelocity, 0, 0) { HomeWarioMechanics1(gb).port_ApplyJumpVelocity(); }
PORT_EX(ApplyWalkVelocity, 0, 0) { HomeWarioMechanics1(gb).port_ApplyWalkVelocity(); }
PORT_EX(TriggerDownwardsFloorTransition, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_TriggerDownwardsFloorTransition(); }
PORT_EX(TriggerFloorTransition_SkipUpdateYPos, 0, 0) { HomeWarioMechanics1(gb).port_TriggerFloorTransition_SkipUpdateYPos(); }
PORT_EX(TriggerFloorTransition, 0, 0) { HomeWarioMechanics1(gb).port_TriggerFloorTransition(); }
PORT_EX(ApplySlopedWalkVelocity, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_ApplySlopedWalkVelocity(); }
PORT_EX(ApplyWalkVelocity_Right, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_ApplyWalkVelocity_Right(); }
PORT_EX(ApplyWalkVelocity_Left, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_ApplyWalkVelocity_Left(); }
PORT_EX(ResetLevelMusicWarioPalsAndState, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_ResetLevelMusicWarioPalsAndState(); }
PORT_EX(RecoverFromTransformation, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_RecoverFromTransformation(); }
PORT_EX(RecoverFromTransformation_WithoutInvincibility, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_RecoverFromTransformation_WithoutInvincibility(); }
PORT_EX(InvalidWarioStateReset, HOOK_MANUAL_RET, 0) { HomeWarioMechanics1(gb).port_InvalidWarioStateReset(); }
