// home/wario_mechanics1 (src/home/wario_mechanics1.asm) as plain C++; see
// wario.hpp. The callers are C++ (the Wario states, the object
// interactions), which read no register after the velocity helpers: those
// hooks clobber them all but b, the walk velocity.
#include "wario.hpp"

#include "../flags.hpp"
#include "../routines.hpp"
#include "floor_transition.hpp"
#include "memory.hpp"
#include "pals.hpp"
#include "system.hpp"

namespace wl3::home {

void apply_jump_velocity(GB *gb)
{
	static const uint16_t tables[] = {
		ADDR_JumpVelTable_KnockBack,      // JUMP_VEL_KNOCK_BACK
		ADDR_JumpVelTable_Normal,         // JUMP_VEL_NORMAL
		ADDR_JumpVelTable_HighJump,       // JUMP_VEL_HIGH_JUMP
		ADDR_JumpVelTable_BouncyJump,     // JUMP_VEL_BOUNCY_JUMP
		ADDR_JumpVelTable_BouncyHighJump, // JUMP_VEL_BOUNCY_HIGH_JUMP
	};
	Memory mem(gb);
	uint8_t table = mem[wJumpVelTable];
	if (table < JUMP_VEL_KNOCK_BACK || table > JUMP_VEL_BOUNCY_HIGH_JUMP)
		return;
	uint8_t index = mem[wJumpVelIndex];
	auto vel = static_cast<int8_t>(mem[static_cast<uint16_t>(tables[table - 1] + index)]);
	if (vel < 0) { // rising
		move_wario_y(gb, vel);
		mem[wJumpVelIndex] = static_cast<uint8_t>(index + 1);
		return;
	}
	mem[wDoFullJump] = FALSE;
	move_wario_y(gb, vel);
	index = static_cast<uint8_t>(index + 1);
	mem[wJumpVelIndex] = index < MAX_JUMP_VEL_INDEX ? index : MAX_JUMP_VEL_INDEX;
}

namespace {

// ApplyWalkVelocity: from $1b, wWalkVelIndex goes back to $18
uint8_t walk_velocity(Memory mem)
{
	uint8_t index = mem[wWalkVelIndex];
	uint8_t vel = mem[static_cast<uint16_t>(ADDR_WalkVelTable + index)];
	mem[wWalkVelIndex] = index == 0x1b ? (index & 0xfc) : static_cast<uint8_t>(index + 1);
	return vel;
}

// ApplySlopedWalkVelocity: walking up a slope, back to 4 from 8
uint8_t sloped_walk_velocity(Memory mem)
{
	if (mem[wWalkVelIndex] >= 0x08)
		mem[wWalkVelIndex] = 0x04;
	return walk_velocity(mem);
}

} // namespace

uint8_t apply_walk_velocity(GB *gb, bool right)
{
	Memory mem(gb);
	bool facing_right = mem[wDirection] != DIRECTION_LEFT;
	if (facing_right != right) { // turning: from a standstill
		mem[wWalkVelIndex] = 0;
		return walk_velocity(mem);
	}
	if (mem[wWarioSlopeInteraction] >> (right ? RIGHT_SLOPE_F : LEFT_SLOPE_F) & 1)
		return sloped_walk_velocity(mem);
	return walk_velocity(mem);
}

void trigger_floor_transition(GB *gb, bool update_y)
{
	Memory mem(gb);
	if (update_y) { // update_pos_y
		mem[wWarioYPos + 0] = mem[hYPosHi];
		mem[wWarioYPos + 1] = mem[hYPosHi + 1];
	}
	handle_floor_transition(gb);
}

static void reset_level_music_wario_pals_and_state(GB *gb)
{
	Memory mem(gb);
	update_level_music(gb);
	set_wario_pal(gb, ADDR_WarioDefaultPal);
	if (mem[wJumpVelTable])
		Routines(gb).farcall(BANK_StartFall, ADDR_StartFall);
	else
		Routines(gb).farcall(BANK_SetState_Idling, ADDR_SetState_Idling);
	play_sfx(mem, SFX_RECOVERY); // PlayRecoverySFX
}

void recover_from_transformation(GB *gb, bool invincible)
{
	Memory mem(gb);
	clear_transformation_values(gb);
	if (invincible)
		mem[wInvincibleCounter] = 0x10;
	reset_level_music_wario_pals_and_state(gb);
}

// callable by the hooks below
static void reset_level_music_wario_pals(GB *gb) { reset_level_music_wario_pals_and_state(gb); }

} // namespace wl3::home

using namespace wl3::home;

PORT_EX(ApplyJumpVelocity, 0, ALL_REGISTERS) { apply_jump_velocity(gb); }

PORT_EX(ApplyWalkVelocity, 0, ALL_REGISTERS & ~R_B) { gb->cpu.b = walk_velocity(wl3::Memory(gb)); }
PORT_EX(ApplySlopedWalkVelocity, 0, ALL_REGISTERS & ~R_B) { gb->cpu.b = sloped_walk_velocity(wl3::Memory(gb)); }
PORT_EX(ApplyWalkVelocity_Right, 0, ALL_REGISTERS & ~R_B) { gb->cpu.b = apply_walk_velocity(gb, true); }
PORT_EX(ApplyWalkVelocity_Left, 0, ALL_REGISTERS & ~R_B) { gb->cpu.b = apply_walk_velocity(gb, false); }

PORT_EX(TriggerDownwardsFloorTransition, 0, ALL_REGISTERS) { handle_downwards_floor_transition(gb); }
PORT_EX(TriggerFloorTransition_SkipUpdateYPos, 0, ALL_REGISTERS) { trigger_floor_transition(gb, false); }
PORT_EX(TriggerFloorTransition, 0, ALL_REGISTERS) { trigger_floor_transition(gb, true); }

PORT_EX(ResetLevelMusicWarioPalsAndState, 0, ALL_REGISTERS) { reset_level_music_wario_pals(gb); }
PORT_EX(RecoverFromTransformation, 0, ALL_REGISTERS) { recover_from_transformation(gb, true); }
PORT_EX(RecoverFromTransformation_WithoutInvincibility, 0, ALL_REGISTERS) { recover_from_transformation(gb, false); }
PORT_EX(InvalidWarioStateReset, HOOK_MANUAL_RET, 0) { JUMP_TO(ADDR_Init); }
