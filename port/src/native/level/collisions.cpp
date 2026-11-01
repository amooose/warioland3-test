// Wario's collisions with blocks (src/engine/level/collisions.asm): the
// points around his collision box, looked up in the level map and handed
// to ProcessBlock, which runs the block's function.
//
// A block function is whatever the level's block function table points
// at: it sees the asm's return address, may pop past it (returning to the
// caller's caller) and may come back with another ROM bank mapped, after
// which the asm returns into whatever that bank holds. So these keep the
// original's calls, return addresses, registers and flags, and stop where
// the asm would (see HANDOFF.md, "the collision tails"). Calls within this
// bank go by address: like the asm, they run whatever bank is mapped.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

#include <initializer_list>

namespace wl3::level {

namespace {

constexpr uint16_t AIRBORNE_RET_FUNC_19A53 = ADDR_CheckAirborneCollision + 0x03;
constexpr uint16_t AIRBORNE_RET_LEFT = ADDR_CheckAirborneCollision + 0x2f;
constexpr uint16_t AIRBORNE_RET_RIGHT = ADDR_CheckAirborneCollision + 0x61;
constexpr uint16_t AIRBORNE_RET_CENTRE = ADDR_CheckAirborneCollision + 0x8d;

class Collisions {
public:
	explicit Collisions(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void reset_and_process(); // Func_19b3a
	void process_down(); // Func_19b51
	void decrement_block_ptr(); // DecrementBlockPtr
	void increment_block_ptr(); // IncrementBlockPtr
	void process_left(); // _ProcessBlockOnLeft
	void process_right(); // _ProcessBlockOnRight
	void process_at_pos(uint16_t routine, uint8_t points); // Func_19b7b, ...
	void process_unk4(); // Func_19bc3
	void process_up_row(); // Func_19bd3
	void at_y_offset(uint8_t offset); // Func_19acd, Func_19ae4, Func_19afb, Func_19b25
	void at_centre(); // Func_19b12
	void below(); // Func_198c7
	void airborne(); // CheckAirborneCollision
	void process_block(uint16_t ret); // Func_19b5c, Func_19bce
	void centre(); // CheckCentreCollision
	void break_up(); // Func_19823
	void bottom(bool right); // CheckBottomRightCollision, CheckBottomLeftCollision
	void front(); // CheckFrontCollision
	bool as_wario(uint16_t routine, uint16_t ret);
	void airborne_as_wario(); // Func_19720
	void slide_under(); // Func_19832
	void above(); // Func_19942
	void up(); // CheckUpCollision
	void sides(); // Func_19a77
	void adjacent(bool next, uint16_t ret); // Func_19bab, Func_19bb7

	void ret() { cpu_.pc = gb_pop16(gb_); }
	void set_break_flag()
	{
		cpu_.a = TRUE;
		mem_[wBlockFuncBreakFlag] = TRUE;
	}

private:
	bool zf() const { return cpu_.f & FLAG_Z; }
	void xor_a()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
	}
	void and_a() { cpu_.f = flags::and_(cpu_.a); }
	// ld a, [addr] / and a: true if nz
	bool ld_and(uint16_t addr)
	{
		cpu_.a = mem_[addr];
		and_a();
		return cpu_.a;
	}
	bool call_in_bank(uint16_t addr, uint16_t ret);
	void get_block_at_pos(uint8_t points);
	void get_pos();
	void get_pos_with_y_offset(uint8_t offset);
	void get_pos_at_box_edge(bool left);
	void break_if_smash_attacking();
	void break_unless_puffy();
	uint8_t neg(uint16_t addr);
	uint8_t cy(uint16_t box, uint8_t plus);
	void hpos_from_wario(bool add_x, uint8_t cx, uint16_t box, uint8_t plus);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// A call of ProcessBlock, then the caller's ld b, a / ret: a block
// function that pops past the call stops it where it left off, and one
// that comes back with another bank mapped sends the asm into that bank
// at ret
void Collisions::process_block(uint16_t ret_addr)
{
	if (!gb_call_ret_unwind(gb_, ADDR_ProcessBlock, ret_addr))
		return;
	if (gb_->rom_bank != BANK_ProcessBlock) {
		cpu_.pc = ret_addr;
		return;
	}
	cpu_.b = cpu_.a; // ld b, a
	ret();
}

// A call that reaches ProcessBlock: false when it came back with another
// bank mapped (pc at ret: the asm runs from there)
bool Collisions::call_in_bank(uint16_t addr, uint16_t ret_addr)
{
	routines_.call_ret(addr, ret_addr);
	if (gb_->rom_bank == BANK_ProcessBlock)
		return true;
	cpu_.pc = ret_addr;
	return false;
}

// ld hl, hPos / call GetBlockPtr / ld a, points / ld [wCollisionPoints], a
void Collisions::get_block_at_pos(uint8_t points)
{
	SET_HL(gb_, hPos);
	routines_.call(ADDR_GetBlockPtr);
	cpu_.a = points;
	mem_[wCollisionPoints] = points;
}

// the block at hPos with a set of collision points: the call of
// ProcessBlock is $0b bytes in
void Collisions::process_at_pos(uint16_t routine, uint8_t points)
{
	get_block_at_pos(points);
	process_block(static_cast<uint16_t>(routine + 0x0b + 3));
}

// clears the interactions the blocks set, then the block at hPos
void Collisions::reset_and_process()
{
	xor_a();
	for (uint16_t addr : {wWaterInteraction, wLadderInteraction, wRoomTransitionParam, wIsInSand, wIsNearFence,
		     wIsInAirCurrent})
		mem_[addr] = 0;
	// call Func_19c0b / ret
	uint16_t ret_addr = ADDR_Func_19b3a + 0x13 + 3;
	if (!gb_call_ret_unwind(gb_, ADDR_Func_19c0b, ret_addr))
		return;
	if (gb_->rom_bank != BANK_ProcessBlock) {
		cpu_.pc = ret_addr;
		return;
	}
	ret();
}

void Collisions::process_down()
{
	get_block_at_pos(COLLISION_DOWN);
	process_block(ADDR_Func_19b5c + 3); // falls through
}

void Collisions::decrement_block_ptr()
{
	SET_HL(gb_, wBlockPtr + 1);
	Byte lo = mem_[wBlockPtr + 1];
	cpu_.f = flags::dec(cpu_.f, lo);
	--lo;
}

void Collisions::increment_block_ptr()
{
	SET_HL(gb_, wBlockPtr + 1);
	Byte lo = mem_[wBlockPtr + 1];
	cpu_.f = flags::inc(cpu_.f, lo);
	++lo;
}

// the block to the left, unless it's $ff
void Collisions::process_left()
{
	decrement_block_ptr();
	uint8_t block = mem_[REG_HL(gb_)];
	cpu_.f = flags::inc(cpu_.f, block);
	cpu_.a = static_cast<uint8_t>(block + 1);
	if (zf()) {
		ret();
		return;
	}
	cpu_.pc = ADDR_Func_19b5c;
}

// the block to the right, unless it's $00
void Collisions::process_right()
{
	increment_block_ptr();
	cpu_.a = mem_[REG_HL(gb_)];
	and_a();
	if (zf()) {
		ret();
		return;
	}
	cpu_.pc = ADDR_Func_19b5c;
}

void Collisions::process_unk4()
{
	get_block_at_pos(COLLISION_UNK4);
	process_block(ADDR_Func_19bce + 3); // falls through
}

// the block a row up
void Collisions::process_up_row()
{
	uint8_t old = mem_[wBlockPtr];
	uint8_t hi = static_cast<uint8_t>(old - 1);
	mem_[wBlockPtr] = hi;
	SET_HL(gb_, wBlockPtr);
	cpu_.a = hi;
	cpu_.f = flags::cp(hi, 0xa0 - 1);
	if (zf()) { // into the SRAM bank before
		uint8_t bank = mem_[wBlockPtrBank];
		cpu_.f = flags::dec(cpu_.f, bank);
		mem_[wBlockPtrBank] = static_cast<uint8_t>(bank - 1);
		cpu_.a = 0xc0 - 1;
		mem_[wBlockPtr] = cpu_.a;
	}
	cpu_.a = mem_[wBlockPtr];
	cpu_.l = mem_[wBlockPtr + 1];
	cpu_.h = cpu_.a;
	cpu_.pc = ADDR_Func_19bce;
}

// hPos = wWarioPos with y - offset, as the asm copies it: from wWarioXPos
// + 1 down
void Collisions::get_pos_with_y_offset(uint8_t offset)
{
	mem_[hXPosLo] = mem_[wWarioXPos + 1];
	mem_[hXPosLo - 1] = mem_[wWarioXPos];
	uint8_t lo = mem_[wWarioYPos + 1];
	mem_[hXPosLo - 2] = static_cast<uint8_t>(lo - offset);
	bool borrow = lo < offset;
	uint8_t hi = mem_[wWarioYPos];
	cpu_.f = flags::sub(hi, 0, borrow);
	cpu_.a = static_cast<uint8_t>(hi - borrow);
	mem_[hXPosLo - 3] = cpu_.a;
	SET_HL(gb_, wWarioYPos);
	SET_DE(gb_, hXPosLo - 3);
}

void Collisions::at_y_offset(uint8_t offset)
{
	get_pos_with_y_offset(offset);
	cpu_.pc = ADDR_Func_19b3a;
}

// get_pos: hPos = wWarioPos
void Collisions::get_pos()
{
	for (int i = 0; i < 4; i++)
		mem_[hPos + i] = mem_[wWarioPos + i];
	cpu_.a = mem_[wWarioPos + 3];
	SET_HL(gb_, wWarioPos + 3);
	SET_DE(gb_, hPos + 3);
}

// collision in the centre
void Collisions::at_centre()
{
	get_pos();
	cpu_.pc = ADDR_Func_19b3a;
}

// hPos = wWarioPos with x at the collision box's left or right edge, 3
// pixels in, as the asm copies it: from wWarioXPos + 1 down
void Collisions::get_pos_at_box_edge(bool left)
{
	uint8_t x_lo = mem_[wWarioXPos + 1], x_hi = mem_[wWarioXPos];
	if (left) {
		cpu_.c = static_cast<uint8_t>(static_cast<uint8_t>(-mem_[wCollisionBoxLeft]) - 3);
		mem_[hXPosLo] = static_cast<uint8_t>(x_lo - cpu_.c);
		bool borrow = x_lo < cpu_.c;
		cpu_.f = flags::sub(x_hi, 0, borrow);
		cpu_.a = static_cast<uint8_t>(x_hi - borrow);
	} else {
		cpu_.c = static_cast<uint8_t>(mem_[wCollisionBoxRight] - 3);
		unsigned sum = x_lo + cpu_.c;
		mem_[hXPosLo] = static_cast<uint8_t>(sum);
		cpu_.f = flags::add(x_hi, 0, sum > 0xff);
		cpu_.a = static_cast<uint8_t>(x_hi + (sum > 0xff));
	}
	mem_[hXPosLo - 1] = cpu_.a;
	mem_[hXPosLo - 2] = mem_[wWarioYPos + 1];
	cpu_.a = mem_[wWarioYPos];
	mem_[hXPosLo - 3] = cpu_.a;
	SET_HL(gb_, wWarioYPos);
	SET_DE(gb_, hXPosLo - 3);
}

// resets wWarioSlopeInteraction; the block below Wario
void Collisions::below()
{
	xor_a();
	mem_[wWarioSlopeInteraction] = 0;
	get_pos();
	if (call_in_bank(ADDR_Func_19b51, ADDR_Func_198c7 + 0x18))
		ret();
}

// ld a, [wIsSmashAttacking] / and a / jr z / ld a, TRUE / ld [wBlockFuncBreakFlag], a
void Collisions::break_if_smash_attacking()
{
	if (!ld_and(wIsSmashAttacking))
		return;
	set_break_flag();
}

// in the air: the blocks under the centre, then the left and right edges
void Collisions::airborne()
{
	// call .Func_19a53, with its return address on the stack as in the asm
	gb_push16(gb_, AIRBORNE_RET_FUNC_19A53);
	xor_a();
	mem_[wWarioSlopeInteraction] = 0;
	break_if_smash_attacking();
	get_pos();
	if (!call_in_bank(ADDR_Func_19b7b, AIRBORNE_RET_CENTRE))
		return;
	gb_pop16(gb_);
	and_a();
	if (!zf()) {
		ret();
		return;
	}

	for (bool left : {true, false}) {
		break_if_smash_attacking();
		get_pos_at_box_edge(left);
		if (!call_in_bank(ADDR_Func_19bfb, left ? AIRBORNE_RET_LEFT : AIRBORNE_RET_RIGHT))
			return;
		and_a();
		if (!zf()) {
			ret();
			return;
		}
		if (ld_and(wIsOnSteppableObject)) {
			cpu_.b = cpu_.a;
			ret();
			return;
		}
	}
	ret();
}

// the checks in the centre, then on the left and right edges
void Collisions::centre()
{
	cpu_.a = 0x01;
	mem_[wUnused_ced3] = 0x01;
	if (!call_in_bank(ADDR_Func_198c7, ADDR_CheckCentreCollision + 0x08))
		return;
	and_a();
	if (!zf()) {
		ret();
		return;
	}
	xor_a();
	mem_[wUnused_ced3] = 0;

	for (bool left : {true, false}) {
		get_pos_at_box_edge(left);
		if (!call_in_bank(ADDR_Func_19b8b, ADDR_CheckCentreCollision + (left ? 0x2d : 0x4c)))
			return;
		and_a();
		if (!zf()) {
			ret();
			return;
		}
	}

	if (!ld_and(wIsOnSteppableObject)) {
		ret();
		return;
	}
	cpu_.b = cpu_.a;
	// get_pos_y
	mem_[hYPosHi] = mem_[wWarioYPos];
	cpu_.a = mem_[wWarioYPos + 1];
	mem_[hYPosHi + 1] = cpu_.a;
	SET_HL(gb_, wWarioYPos + 1);
	SET_DE(gb_, hYPosHi + 1);
	cpu_.a = 0x01;
	ret();
}

// breaks the block a row up
void Collisions::break_up()
{
	set_break_flag();
	if (!call_in_bank(ADDR_Func_19bd3, ADDR_Func_19823 + 0x08))
		return;
	cpu_.a = 0x01;
	mem_[wUnused_c18d] = 0x01;
	cpu_.b = 0x01;
	ret();
}

// ld a, [addr] / cpl / inc a
uint8_t Collisions::neg(uint16_t addr)
{
	uint8_t v = static_cast<uint8_t>(~mem_[addr]);
	cpu_.f = flags::inc(static_cast<uint8_t>(FLAG_N | FLAG_H | (cpu_.f & (FLAG_Z | FLAG_C))), v);
	return static_cast<uint8_t>(v + 1);
}

// -[box] + plus (the y the point is above Wario's)
uint8_t Collisions::cy(uint16_t box, uint8_t plus)
{
	uint8_t v = neg(box);
	cpu_.f = flags::add(v, plus);
	return static_cast<uint8_t>(v + plus);
}

// hPos = Wario's position with x + cx (or - cx) and y - cy(box, plus), as
// the asm computes it: from wWarioXPos + 1 down, with cy between the halves
void Collisions::hpos_from_wario(bool add_x, uint8_t cx, uint16_t box, uint8_t plus)
{
	cpu_.c = cx;
	uint8_t x_lo = mem_[wWarioXPos + 1], x_hi = mem_[wWarioXPos];
	if (add_x) {
		unsigned sum = x_lo + cx;
		mem_[hXPosLo] = static_cast<uint8_t>(sum);
		cpu_.a = static_cast<uint8_t>(x_hi + (sum > 0xff));
	} else {
		mem_[hXPosLo] = static_cast<uint8_t>(x_lo - cx);
		cpu_.a = static_cast<uint8_t>(x_hi - (x_lo < cx));
	}
	mem_[hXPosLo - 1] = cpu_.a;
	cpu_.c = cy(box, plus);
	uint8_t y_lo = mem_[wWarioYPos + 1];
	mem_[hXPosLo - 2] = static_cast<uint8_t>(y_lo - cpu_.c);
	bool borrow = y_lo < cpu_.c;
	uint8_t y_hi = mem_[wWarioYPos];
	cpu_.f = flags::sub(y_hi, 0, borrow);
	cpu_.a = static_cast<uint8_t>(y_hi - borrow);
	mem_[hXPosLo - 3] = cpu_.a;
	SET_HL(gb_, wWarioYPos);
	SET_DE(gb_, hXPosLo - 3);
}

// the block at the collision box's bottom right or left corner
void Collisions::bottom(bool right)
{
	uint16_t routine = right ? ADDR_CheckBottomRightCollision : ADDR_CheckBottomLeftCollision;
	cpu_.a = mem_[wTransformation];
	cpu_.f = flags::cp(cpu_.a, TRANSFORMATION_FLAT_WARIO);
	bool flat = zf();
	uint8_t edge = right ? static_cast<uint8_t>(mem_[wCollisionBoxRight]) : neg(wCollisionBoxLeft);
	cpu_.f = flags::sub(edge, 1);
	uint8_t cx = static_cast<uint8_t>(edge - 1);
	hpos_from_wario(right, cx, wCollisionBoxBottom, flat ? 1 : 7);
	cpu_.a = mem_[hYPosLo] & 0xf0;
	and_a();
	mem_[wc0dc] = cpu_.a;
	if (!call_in_bank(ADDR_Func_19bc3, static_cast<uint16_t>(routine + (right ? 0x5d : 0x61))))
		return;
	cpu_.a &= 0x0f;
	and_a();
	if (zf()) {
		cpu_.pc = ADDR_Func_19832;
		return;
	}
	if (ld_and(wAttackCounter)) {
		cpu_.pc = ADDR_Func_19823;
		return;
	}
	cpu_.a = cpu_.b;
	ret();
}

// the side Wario's facing
void Collisions::front()
{
	set_break_flag();
	cpu_.a = mem_[wDirection];
	and_a();
	bottom(cpu_.a);
}

// ld a, TRUE / ld [wBlockFuncWarioFlag], a / call routine / xor a /
// ld [wBlockFuncWarioFlag], a: false when the call came back in another bank
bool Collisions::as_wario(uint16_t routine, uint16_t ret_addr)
{
	cpu_.a = TRUE;
	mem_[wBlockFuncWarioFlag] = TRUE;
	if (!call_in_bank(routine, ret_addr))
		return false;
	xor_a();
	mem_[wBlockFuncWarioFlag] = FALSE;
	return true;
}

void Collisions::airborne_as_wario()
{
	if (!as_wario(ADDR_CheckAirborneCollision, ADDR_Func_19720 + 0x08))
		return;
	cpu_.a = cpu_.b;
	and_a();
	cpu_.pc = cpu_.a ? ADDR_TriggerFloorTransition : ADDR_TriggerDownwardsFloorTransition;
}

// a gap a block high ahead: crouch or slide under it
void Collisions::slide_under()
{
	bool skip = ld_and(wIsCrouching);
	if (!skip) {
		cpu_.b = mem_[wCollisionBoxTop];
		uint8_t y = static_cast<uint8_t>(mem_[wWarioYPos + 1] + cpu_.b);
		cpu_.f = flags::add(y, 2);
		cpu_.a = static_cast<uint8_t>((y + 2) & 0xf0);
		cpu_.b = cpu_.a;
		cpu_.a = mem_[wc0dc];
		cpu_.f = flags::cp(cpu_.a, cpu_.b);
		skip = zf();
	}
	if (!skip) {
		set_break_flag();
		if (!call_in_bank(ADDR_Func_19bd3, ADDR_Func_19832 + 0x23))
			return;
		cpu_.a &= 0x0f;
		and_a();
		if (zf()) {
			ret();
			return;
		}
		for (uint16_t addr : {wJumpVelTable, wAttackCounter, wWaterInteraction, wIsCrouching, wIsOnSteppableObject,
			     wTransformation, wIsInSand}) {
			if (ld_and(addr)) {
				ret();
				return;
			}
		}
		cpu_.a = mem_[wJoypadDown];
		bool right;
		cpu_.f = flags::bit(cpu_.f, cpu_.a, B_PAD_RIGHT);
		if (cpu_.a & (1 << B_PAD_RIGHT)) {
			right = true;
		} else {
			cpu_.f = flags::bit(cpu_.f, cpu_.a, B_PAD_LEFT);
			right = (cpu_.a & (1 << B_PAD_LEFT)) ? false : ld_and(wDirection);
		}
		cpu_.b = 2;
		routines_.call(right ? ADDR_MoveWarioRight : ADDR_MoveWarioLeft);
		routines_.farcall_ret(BANK_CrouchOrSlide, ADDR_CrouchOrSlide, ADDR_Func_19832 + (right ? 0x8c : 0x76));
	}
	// .asm_198c1
	xor_a();
	mem_[wUnused_c18d] = 0;
	cpu_.b = 0;
	ret();
}

void Collisions::break_unless_puffy()
{
	cpu_.a = mem_[wTransformation];
	cpu_.f = flags::cp(cpu_.a, TRANSFORMATION_PUFFY_WARIO);
	if (zf())
		return;
	set_break_flag(); // can break the blocks above
}

// the block 2 pixels above the collision box
void Collisions::above()
{
	break_unless_puffy();
	cpu_.c = cy(wCollisionBoxTop, 2);
	mem_[hXPosLo] = mem_[wWarioXPos + 1];
	cpu_.a = mem_[wWarioXPos];
	mem_[hXPosLo - 1] = cpu_.a;
	uint8_t y_lo = mem_[wWarioYPos + 1];
	mem_[hXPosLo - 2] = static_cast<uint8_t>(y_lo - cpu_.c);
	bool borrow = y_lo < cpu_.c;
	uint8_t y_hi = mem_[wWarioYPos];
	cpu_.f = flags::sub(y_hi, 0, borrow);
	cpu_.a = static_cast<uint8_t>(y_hi - borrow);
	mem_[hXPosLo - 3] = cpu_.a;
	SET_HL(gb_, wWarioYPos);
	SET_DE(gb_, hXPosLo - 3);
	if (call_in_bank(ADDR_Func_19beb, ADDR_Func_19942 + 0x2b))
		ret();
}

// above the centre, then above the left and right edges, 3 pixels in
void Collisions::up()
{
	cpu_.a = TRUE;
	mem_[wJumpCollision] = TRUE;
	if (!call_in_bank(ADDR_Func_19942, ADDR_CheckUpCollision + 0x08))
		return;
	and_a();
	if (!zf()) {
		ret();
		return;
	}
	xor_a();
	mem_[wJumpCollision] = 0;
	break_unless_puffy();
	uint8_t left = neg(wCollisionBoxLeft);
	cpu_.f = flags::sub(left, 3);
	hpos_from_wario(false, static_cast<uint8_t>(left - 3), wCollisionBoxTop, 2);
	if (!call_in_bank(ADDR_Func_19b9b, ADDR_CheckUpCollision + 0x44))
		return;
	and_a();
	if (!zf()) {
		ret();
		return;
	}
	break_unless_puffy();
	uint8_t right = mem_[wCollisionBoxRight];
	cpu_.f = flags::sub(right, 3);
	hpos_from_wario(true, static_cast<uint8_t>(right - 3), wCollisionBoxTop, 2);
	if (call_in_bank(ADDR_Func_19b9b, ADDR_CheckUpCollision + 0x7a))
		ret();
}

// the blocks at the box's left and right edges; wc1ca when only the left
// one's clear... as the asm leaves it
void Collisions::sides()
{
	xor_a();
	mem_[wc1ca] = FALSE;
	get_pos_at_box_edge(true);
	if (!call_in_bank(ADDR_Func_19b3a, ADDR_Func_19a77 + 0x23))
		return;
	and_a();
	if (cpu_.a && !ld_and(wc1ca)) {
		ret();
		return;
	}
	xor_a();
	mem_[wc1ca] = FALSE;
	get_pos_at_box_edge(false);
	if (!call_in_bank(ADDR_Func_19b3a, ADDR_Func_19a77 + 0x4c))
		return;
	and_a();
	if (!cpu_.a) {
		cpu_.a = TRUE;
		mem_[wc1ca] = TRUE;
	}
	ret();
}

// the block before (or after) wBlockPtr
void Collisions::adjacent(bool next, uint16_t ret_addr)
{
	Byte lo = mem_[wBlockPtr + 1];
	cpu_.f = next ? flags::inc(cpu_.f, lo) : flags::dec(cpu_.f, lo);
	lo = static_cast<uint8_t>(next ? lo + 1 : lo - 1);
	cpu_.a = lo;
	cpu_.h = mem_[wBlockPtr];
	cpu_.l = cpu_.a;
	process_block(ret_addr);
}

} // namespace

} // namespace wl3::level

using wl3::level::Collisions;

// Every one ends where the asm would (HOOK_MANUAL_RET): a ret, a jump on,
// or wherever a block function left the stack and the bank. (Never a
// macro around PORT_EX: tools/gen.c finds the hooks by their text.)

PORT_EX(Func_19b3a, HOOK_MANUAL_RET, 0) { Collisions(gb).reset_and_process(); }
PORT_EX(Func_19b51, HOOK_MANUAL_RET, 0) { Collisions(gb).process_down(); }
PORT(DecrementBlockPtr) { Collisions(gb).decrement_block_ptr(); }
PORT(IncrementBlockPtr) { Collisions(gb).increment_block_ptr(); }
PORT_EX(_ProcessBlockOnLeft, HOOK_MANUAL_RET, 0) { Collisions(gb).process_left(); }
PORT_EX(_ProcessBlockOnRight, HOOK_MANUAL_RET, 0) { Collisions(gb).process_right(); }
PORT_EX(Func_19b7b, HOOK_MANUAL_RET, 0) { Collisions(gb).process_at_pos(ADDR_Func_19b7b, COLLISION_UNK1); }
PORT_EX(Func_19b8b, HOOK_MANUAL_RET, 0) { Collisions(gb).process_at_pos(ADDR_Func_19b8b, COLLISION_UNK2); }
PORT_EX(Func_19b9b, HOOK_MANUAL_RET, 0) { Collisions(gb).process_at_pos(ADDR_Func_19b9b, COLLISION_UNK3); }
PORT_EX(Func_19bc3, HOOK_MANUAL_RET, 0) { Collisions(gb).process_unk4(); }
PORT_EX(Func_19bd3, HOOK_MANUAL_RET, 0) { Collisions(gb).process_up_row(); }
PORT_EX(Func_19beb, HOOK_MANUAL_RET, 0) { Collisions(gb).process_at_pos(ADDR_Func_19beb, COLLISION_UNK5); }
PORT_EX(Func_19bfb, HOOK_MANUAL_RET, 0) { Collisions(gb).process_at_pos(ADDR_Func_19bfb, COLLISION_UNK6); }
PORT_EX(Func_19c0b, HOOK_MANUAL_RET, 0) { Collisions(gb).process_at_pos(ADDR_Func_19c0b, COLLISION_UNK7); }
PORT_EX(Func_19acd, HOOK_MANUAL_RET, 0) { Collisions(gb).at_y_offset(0x0c); }
PORT_EX(Func_19ae4, HOOK_MANUAL_RET, 0) { Collisions(gb).at_y_offset(0x1a); }
PORT_EX(Func_19afb, HOOK_MANUAL_RET, 0) { Collisions(gb).at_y_offset(0x18); }
PORT_EX(Func_19b12, HOOK_MANUAL_RET, 0) { Collisions(gb).at_centre(); }
PORT_EX(Func_19b25, HOOK_MANUAL_RET, 0) { Collisions(gb).at_y_offset(4); }
PORT_EX(Func_198c7, HOOK_MANUAL_RET, 0) { Collisions(gb).below(); }
PORT_EX(CheckAirborneCollision, HOOK_MANUAL_RET, 0) { Collisions(gb).airborne(); }
PORT_EX(Func_19b5c, HOOK_MANUAL_RET, 0) { Collisions(gb).process_block(ADDR_Func_19b5c + 3); }
PORT_EX(CheckCentreCollision, HOOK_MANUAL_RET, 0) { Collisions(gb).centre(); }
PORT_EX(Func_19bce, HOOK_MANUAL_RET, 0) { Collisions(gb).process_block(ADDR_Func_19bce + 3); }
PORT_EX(Func_19823, HOOK_MANUAL_RET, 0) { Collisions(gb).break_up(); }
PORT_EX(CheckBottomRightCollision, HOOK_MANUAL_RET, 0) { Collisions(gb).bottom(true); }
PORT_EX(CheckBottomLeftCollision, HOOK_MANUAL_RET, 0) { Collisions(gb).bottom(false); }
PORT_EX(Func_19741, HOOK_MANUAL_RET, 0)
{
	Collisions c(gb);
	c.set_break_flag();
	c.bottom(true);
}
PORT_EX(Func_197b1, HOOK_MANUAL_RET, 0)
{
	Collisions c(gb);
	c.set_break_flag();
	c.bottom(false);
}
PORT_EX(CheckFrontCollision, HOOK_MANUAL_RET, 0) { Collisions(gb).front(); }
PORT_EX(Func_19706, HOOK_MANUAL_RET, 0)
{
	Collisions c(gb);
	if (c.as_wario(ADDR_Func_19741, ADDR_Func_19706 + 0x08))
		c.ret();
}
PORT_EX(Func_19713, HOOK_MANUAL_RET, 0)
{
	Collisions c(gb);
	if (c.as_wario(ADDR_Func_197b1, ADDR_Func_19713 + 0x08))
		c.ret();
}
PORT_EX(Func_19720, HOOK_MANUAL_RET, 0) { Collisions(gb).airborne_as_wario(); }
PORT_EX(Func_19832, HOOK_MANUAL_RET, 0) { Collisions(gb).slide_under(); }
PORT_EX(Func_19942, HOOK_MANUAL_RET, 0) { Collisions(gb).above(); }
PORT_EX(CheckUpCollision, HOOK_MANUAL_RET, 0) { Collisions(gb).up(); }
PORT_EX(Func_19a77, HOOK_MANUAL_RET, 0) { Collisions(gb).sides(); }
PORT_EX(Func_19bab, HOOK_MANUAL_RET, 0) { Collisions(gb).adjacent(false, ADDR_Func_19bab + 0x0a); }
PORT_EX(Func_19bb7, HOOK_MANUAL_RET, 0) { Collisions(gb).adjacent(true, ADDR_Func_19bb7 + 0x0a); }
