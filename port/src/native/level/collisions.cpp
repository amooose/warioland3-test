// Wario's collisions with blocks (src/engine/level/collisions.asm); see
// collisions.hpp. The points around his collision box go to hPos, are
// looked up in the level map (GetBlockPtr) and handed to ProcessBlock,
// which runs the block's function.
//
// The block functions are whatever the level's block function table
// points at. In the game their pushes and pops balance and they come back
// in this bank: only a fuzzed table makes ProcessBlock pop past its caller
// or return with another bank mapped (no recording does), which the
// emulated call (gb_call) and process_block flag unrealistic under test.
// The block functions are still register-level and see the registers the
// asm leaves them: b and c (hl, de and a they load themselves), so those
// two are set where the asm sets them, and after a check the CPU holds the
// registers of its last ProcessBlock, as in the asm.
#include "collisions.hpp"

#include "../flags.hpp"
#include "../routines.hpp"

#include <initializer_list>

namespace wl3::level {

namespace {

class Collisions {
public:
	explicit Collisions(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	uint8_t block(uint8_t points); // Func_19b51, Func_19b7b, ...: the block at hPos
	uint8_t process_block(); // Func_19b5c, Func_19bce: ProcessBlock / ld b, a
	bool process_adjacent(bool right); // _ProcessBlockOnLeft, _ProcessBlockOnRight
	uint8_t at_y_offset(uint8_t dy); // Func_19acd, Func_19ae4, Func_19afb, Func_19b12, Func_19b25
	uint8_t below(); // Func_198c7
	uint8_t centre(); // CheckCentreCollision
	uint8_t front(); // CheckFrontCollision
	uint8_t bottom(bool right); // CheckBottomRightCollision, CheckBottomLeftCollision
	uint8_t up(); // CheckUpCollision
	uint8_t airborne(); // CheckAirborneCollision
	uint8_t sides(); // Func_19a77
	template <typename F> uint8_t as_wario(F check);

	// CheckCentreCollision's result is the steppable object (get_pos_y ran)
	bool on_steppable_ = false;

	void set_break_flag() { mem_[wBlockFuncBreakFlag] = TRUE; }

private:
	uint16_t pos(uint16_t addr) const { return static_cast<uint16_t>(mem_[addr] << 8 | mem_[addr + 1]); }
	void set_pos(uint16_t addr, unsigned v)
	{
		mem_[addr] = static_cast<uint8_t>(v >> 8);
		mem_[addr + 1] = static_cast<uint8_t>(v);
	}
	void set_hpos(int dy, int dx);
	uint8_t edge_dx(bool right, uint8_t in);
	void at_edge(bool right);
	uint8_t reset_and_process(); // Func_19b3a
	uint8_t up_row(); // Func_19bd3
	uint8_t break_up(); // Func_19823
	uint8_t slide_under(); // Func_19832
	uint8_t above(); // Func_19942
	void break_if_smash_attacking();
	void break_unless_puffy();

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// hPos = Wario's position + (dy, dx)
void Collisions::set_hpos(int dy, int dx)
{
	set_pos(hYPosHi, pos(wWarioYPos) + dy);
	set_pos(hXPosHi, pos(wWarioXPos) + dx);
}

// how far the collision box's right (or left) edge is, less `in`, as the
// asm's 8-bit offset
uint8_t Collisions::edge_dx(bool right, uint8_t in)
{
	return right ? static_cast<uint8_t>(mem_[wCollisionBoxRight] - in)
		     : static_cast<uint8_t>(-mem_[wCollisionBoxLeft] - in);
}

// hPos = the collision box's right (or left) edge, 3 pixels in, at Wario's y
void Collisions::at_edge(bool right)
{
	uint8_t dx = edge_dx(right, 3);
	set_hpos(0, right ? dx : -dx);
}

uint8_t Collisions::process_block()
{
	routines_.call(ADDR_ProcessBlock);
	if (gb_->test.active && gb_->rom_bank != BANK_ProcessBlock)
		gb_->test.unrealistic = true; // the asm would go on in that bank
	cpu_.b = cpu_.a;
	return cpu_.a;
}

uint8_t Collisions::block(uint8_t points)
{
	SET_HL(gb_, hPos);
	routines_.call(ADDR_GetBlockPtr);
	mem_[wCollisionPoints] = points;
	return process_block();
}

// the block left (or right) of wBlockPtr, unless that wraps its low byte
// around (to $ff, or $00): false
bool Collisions::process_adjacent(bool right)
{
	Byte lo = mem_[wBlockPtr + 1];
	if (right ? ++lo == 0x00 : --lo == 0xff)
		return false;
	process_block();
	return true;
}

// clears the interactions the blocks set, then the block at hPos
uint8_t Collisions::reset_and_process()
{
	for (uint16_t addr : {wWaterInteraction, wLadderInteraction, wRoomTransitionParam, wIsInSand, wIsNearFence,
		     wIsInAirCurrent})
		mem_[addr] = 0;
	return block(COLLISION_UNK7);
}

uint8_t Collisions::at_y_offset(uint8_t dy)
{
	set_hpos(-dy, 0);
	return reset_and_process();
}

// resets wWarioSlopeInteraction; the block under Wario
uint8_t Collisions::below()
{
	mem_[wWarioSlopeInteraction] = 0;
	set_hpos(0, 0);
	return block(COLLISION_DOWN);
}

// the checks in the centre, then on the left and right edges; then the
// steppable object Wario stands on, with hYPos = his y
uint8_t Collisions::centre()
{
	mem_[wUnused_ced3] = 1;
	if (uint8_t r = below())
		return r;
	mem_[wUnused_ced3] = 0;
	for (bool right : {false, true}) {
		at_edge(right);
		if (uint8_t r = block(COLLISION_UNK2))
			return r;
	}
	uint8_t steppable = mem_[wIsOnSteppableObject];
	if (!steppable)
		return 0;
	set_pos(hYPosHi, pos(wWarioYPos)); // get_pos_y
	on_steppable_ = true;
	return steppable;
}

// the block a row up from wBlockPtr (in the SRAM bank before, past the top)
uint8_t Collisions::up_row()
{
	Byte hi = mem_[wBlockPtr];
	if (--hi == 0xa0 - 1) {
		--mem_[wBlockPtrBank];
		hi = 0xc0 - 1;
	}
	return process_block();
}

// breaks the block a row up
uint8_t Collisions::break_up()
{
	set_break_flag();
	up_row();
	mem_[wUnused_c18d] = 1;
	return 1;
}

// a gap a block high ahead: crouch or slide under it
uint8_t Collisions::slide_under()
{
	if (!mem_[wIsCrouching]) {
		uint8_t row = static_cast<uint8_t>(mem_[wWarioYPos + 1] + mem_[wCollisionBoxTop] + 2) & 0xf0;
		if (mem_[wc0dc] != row) {
			set_break_flag();
			cpu_.b = row;
			uint8_t r = up_row();
			if (!(r & 0x0f))
				return r;
			for (uint16_t addr : {wJumpVelTable, wAttackCounter, wWaterInteraction, wIsCrouching,
				     wIsOnSteppableObject, wTransformation, wIsInSand})
				if (mem_[addr])
					return r;
			uint8_t pad = mem_[wJoypadDown];
			bool right = (pad & PAD_RIGHT) || (!(pad & PAD_LEFT) && mem_[wDirection]);
			// register-level callees: b is MoveWario*'s argument
			cpu_.b = 2;
			routines_.call(right ? ADDR_MoveWarioRight : ADDR_MoveWarioLeft);
			routines_.farcall(BANK_CrouchOrSlide, ADDR_CrouchOrSlide);
		}
	}
	mem_[wUnused_c18d] = 0;
	return 0;
}

// the block at the collision box's bottom right (or left) corner, a pixel
// in and 7 pixels up (1 when flat); a gap above it is slid under, and an
// attack breaks the block a row up
uint8_t Collisions::bottom(bool right)
{
	bool flat = mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO;
	uint8_t dx = edge_dx(right, 1);
	uint8_t dy = static_cast<uint8_t>(-mem_[wCollisionBoxBottom] + (flat ? 1 : 7));
	cpu_.c = dy;
	set_hpos(-dy, right ? dx : -dx);
	mem_[wc0dc] = mem_[hYPosLo] & 0xf0;
	uint8_t r = block(COLLISION_UNK4);
	if (!(r & 0x0f))
		return slide_under();
	if (mem_[wAttackCounter])
		return break_up();
	return r;
}

// the side Wario's facing
uint8_t Collisions::front()
{
	set_break_flag();
	return bottom(mem_[wDirection]);
}

void Collisions::break_unless_puffy()
{
	if (mem_[wTransformation] != TRANSFORMATION_PUFFY_WARIO)
		set_break_flag(); // can break the blocks above
}

// the block 2 pixels above the collision box
uint8_t Collisions::above()
{
	break_unless_puffy();
	uint8_t dy = static_cast<uint8_t>(-mem_[wCollisionBoxTop] + 2);
	cpu_.c = dy;
	set_hpos(-dy, 0);
	return block(COLLISION_UNK5);
}

// above the centre, then above the left and right edges, 3 pixels in
uint8_t Collisions::up()
{
	mem_[wJumpCollision] = TRUE;
	if (uint8_t r = above())
		return r;
	mem_[wJumpCollision] = 0;
	for (bool right : {false, true}) {
		break_unless_puffy();
		uint8_t dx = edge_dx(right, 3);
		uint8_t dy = static_cast<uint8_t>(-mem_[wCollisionBoxTop] + 2);
		cpu_.c = dy;
		set_hpos(-dy, right ? dx : -dx);
		if (uint8_t r = block(COLLISION_UNK3))
			return r;
	}
	return 0;
}

void Collisions::break_if_smash_attacking()
{
	if (mem_[wIsSmashAttacking])
		set_break_flag();
}

// in the air: the block under the centre, then the left and right edges
// (each followed by the steppable object)
uint8_t Collisions::airborne()
{
	mem_[wWarioSlopeInteraction] = 0;
	break_if_smash_attacking();
	set_hpos(0, 0);
	if (uint8_t r = block(COLLISION_UNK1))
		return r;
	for (bool right : {false, true}) {
		break_if_smash_attacking();
		at_edge(right);
		if (uint8_t r = block(COLLISION_UNK6))
			return r;
		if (uint8_t steppable = mem_[wIsOnSteppableObject])
			return steppable;
	}
	return 0;
}

// the blocks at the box's left and right edges; wc1ca is set when the right
// one's clear (the left one is skipped over when its block set wc1ca)
uint8_t Collisions::sides()
{
	mem_[wc1ca] = FALSE;
	at_edge(false);
	if (uint8_t r = reset_and_process(); r && !mem_[wc1ca])
		return r;
	mem_[wc1ca] = FALSE;
	at_edge(true);
	uint8_t r = reset_and_process();
	if (!r)
		mem_[wc1ca] = TRUE;
	return r;
}

// with wBlockFuncWarioFlag set: the block functions act for Wario
template <typename F> uint8_t Collisions::as_wario(F check)
{
	mem_[wBlockFuncWarioFlag] = TRUE;
	uint8_t r = check();
	mem_[wBlockFuncWarioFlag] = FALSE;
	return r;
}

} // namespace

uint8_t check_centre_collision(GB *gb) { return Collisions(gb).centre(); }
uint8_t check_front_collision(GB *gb) { return Collisions(gb).front(); }
uint8_t check_up_collision(GB *gb) { return Collisions(gb).up(); }
uint8_t check_airborne_collision(GB *gb) { return Collisions(gb).airborne(); }
uint8_t check_block_below(GB *gb) { return Collisions(gb).block(COLLISION_DOWN); }
uint8_t check_func_19741(GB *gb) { Collisions c(gb); c.set_break_flag(); return c.bottom(true); }
uint8_t check_func_197b1(GB *gb) { Collisions c(gb); c.set_break_flag(); return c.bottom(false); }
uint8_t check_func_198c7(GB *gb) { return Collisions(gb).below(); }
uint8_t check_func_19a77(GB *gb) { return Collisions(gb).sides(); }
uint8_t check_func_19acd(GB *gb) { return Collisions(gb).at_y_offset(0x0c); }
uint8_t check_func_19ae4(GB *gb) { return Collisions(gb).at_y_offset(0x1a); }
uint8_t check_func_19afb(GB *gb) { return Collisions(gb).at_y_offset(0x18); }
uint8_t check_func_19b12(GB *gb) { return Collisions(gb).at_y_offset(0); }
uint8_t check_func_19b25(GB *gb) { return Collisions(gb).at_y_offset(4); }

void asm_centre_collision(GB *gb)
{
	Collisions c(gb);
	gb->cpu.b = c.centre();
}
void asm_front_collision(GB *gb) { gb->cpu.b = check_front_collision(gb); }
void asm_up_collision(GB *gb) { gb->cpu.b = check_up_collision(gb); }
void asm_airborne_collision(GB *gb) { gb->cpu.b = check_airborne_collision(gb); }

} // namespace wl3::level

using wl3::level::Collisions;

// The result goes to b. Nothing reads c, d, e, h or l after a block check
// (poisoning them passed the full fuzz tier; Octohon, which writes 0 through
// the hl, works out that value itself). a and
// f are clobbered: every caller farcalls or homecalls these (the Wario
// states, the objects, home's object mechanics), and hCallFunc / homecall
// pop af after the call. (Never a macro around PORT_EX: tools/gen.c finds the
// hooks by their text.)

namespace {

void result(GB *gb, uint8_t b) { gb->cpu.b = b; }

} // namespace

PORT_EX(Func_19b51, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).block(COLLISION_DOWN); }
PORT_EX(Func_19b7b, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).block(COLLISION_UNK1); }
PORT_EX(Func_19b8b, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).block(COLLISION_UNK2); }
PORT_EX(Func_19b9b, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).block(COLLISION_UNK3); }
PORT_EX(Func_19bc3, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).block(COLLISION_UNK4); }
PORT_EX(_ProcessBlockOnLeft, 0, ALL_REGISTERS & ~R_B)
{
	Collisions(gb).process_adjacent(false);
}
PORT_EX(_ProcessBlockOnRight, 0, ALL_REGISTERS & ~R_B)
{
	Collisions(gb).process_adjacent(true);
}
// unreferenced: the block left (or right) of wBlockPtr, with no check for
// its low byte wrapping around
PORT_EX(Func_19bab, 0, ALL_REGISTERS & ~R_B)
{
	--wl3::Memory(gb)[wBlockPtr + 1];
	Collisions(gb).process_block();
}
PORT_EX(Func_19bb7, 0, ALL_REGISTERS & ~R_B)
{
	++wl3::Memory(gb)[wBlockPtr + 1];
	Collisions(gb).process_block();
}
PORT_EX(Func_19acd, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).at_y_offset(0x0c); }
PORT_EX(Func_19ae4, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).at_y_offset(0x1a); }
PORT_EX(Func_19afb, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).at_y_offset(0x18); }
PORT_EX(Func_19b12, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).at_y_offset(0); }
PORT_EX(Func_19b25, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).at_y_offset(4); }
PORT_EX(Func_198c7, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).below(); }
PORT_EX(CheckCentreCollision, 0, ALL_REGISTERS & ~R_B) { wl3::level::asm_centre_collision(gb); }
PORT_EX(CheckFrontCollision, 0, ALL_REGISTERS & ~R_B) { wl3::level::asm_front_collision(gb); }
PORT_EX(Func_19741, 0, ALL_REGISTERS & ~R_B)
{
	Collisions c(gb);
	c.set_break_flag();
	result(gb, c.bottom(true));
}
PORT_EX(Func_197b1, 0, ALL_REGISTERS & ~R_B)
{
	Collisions c(gb);
	c.set_break_flag();
	result(gb, c.bottom(false));
}
PORT_EX(Func_19706, 0, ALL_REGISTERS & ~R_B)
{
	Collisions c(gb);
	result(gb, c.as_wario([&] {
		c.set_break_flag();
		return c.bottom(true);
	}));
}
PORT_EX(Func_19713, 0, ALL_REGISTERS & ~R_B)
{
	Collisions c(gb);
	result(gb, c.as_wario([&] {
		c.set_break_flag();
		return c.bottom(false);
	}));
}
PORT_EX(CheckUpCollision, 0, ALL_REGISTERS & ~R_B) { wl3::level::asm_up_collision(gb); }
PORT_EX(CheckAirborneCollision, 0, ALL_REGISTERS & ~R_B) { wl3::level::asm_airborne_collision(gb); }
// ld a, b / and a / jp nz, TriggerFloorTransition / jp TriggerDownwardsFloorTransition
PORT_EX(Func_19720, HOOK_MANUAL_RET, ALL_REGISTERS & ~(R_A | R_F | R_B))
{
	Collisions c(gb);
	uint8_t b = c.as_wario([&] { return c.airborne(); });
	result(gb, b);
	gb->cpu.a = b;
	gb->cpu.f = wl3::flags::and_(b);
	JUMP_TO(b ? ADDR_TriggerFloorTransition : ADDR_TriggerDownwardsFloorTransition);
}
PORT_EX(Func_19a77, 0, ALL_REGISTERS & ~R_B) { Collisions(gb).sides(); }

// The routines only this file calls, now plain members.
PORT_COVERED(Func_19b3a);
PORT_COVERED(Func_19b5c);
PORT_COVERED(Func_19bce);
PORT_COVERED(DecrementBlockPtr);
PORT_COVERED(IncrementBlockPtr);
PORT_COVERED(Func_19bd3);
PORT_COVERED(Func_19beb);
PORT_COVERED(Func_19bfb);
PORT_COVERED(Func_19c0b);
PORT_COVERED(Func_19823);
PORT_COVERED(Func_19832);
PORT_COVERED(CheckBottomRightCollision);
PORT_COVERED(CheckBottomLeftCollision);
PORT_COVERED(Func_19942);
