// The block functions (src/engine/level/block_functions.asm): what each
// kind of block does when Wario (or an object) touches it at a collision
// point: solid, free, water and currents, slopes, ladders, spikes, doors and
// pipes, fire, fences, switches, and the blocks that break (alone, or as a
// multi-block).
//
// ProcessBlock jumps to the block's function from the level's table (jp
// hl), and the functions jump on to each other (the asm's jp / jr, by
// address: they run whatever bank is mapped). Each keeps the original's
// registers and flags: the collision code (and through it the Wario
// states, still C) reads a and b after them.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

#include <initializer_list>

namespace wl3::level {

namespace {

constexpr uint16_t FREE = ADDR_BlockFunc_Free;
constexpr uint16_t SOLID = ADDR_BlockFunc_Solid;
constexpr uint16_t WATER = ADDR_BlockFunc_Water;
constexpr uint16_t FLOOR = ADDR_BlockFunc_Floor;
constexpr uint8_t OAM1 = BANK_OAM_14000; // BANK("Wario OAM 1")

// a jump on (jp / jr): pc = addr, and out of the function
#define GOTO(addr) \
	do { \
		cpu_.pc = (addr); \
		return; \
	} while (0)

enum class Progress { ANY, SNOWBALL, BIG_SNOWBALL };

class BlockFuncs {
public:
	explicit BlockFuncs(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void process_block(); // ProcessBlock
	void free(); // BlockFunc_Free
	void solid(); // BlockFunc_Solid
	void water(); // BlockFunc_Water
	void block_func_current(uint8_t current);
	void floor(); // BlockFunc_Floor
	void free_1808c(); // Func_1808c
	void ladder_or_floor(); // Func_182ec
	void enter_door(); // EnterDoor
	void block_func_c18d(uint8_t v); // Func_18064, Func_18078
	void slope_above(uint16_t collision);
	void slope_below(uint16_t collision);
	void right_slope_collision(); // RightSlopeCollision
	void left_slope_collision(); // LeftSlopeCollision
	void right_slope(); // BlockFunc_RightSlope
	void left_slope(); // BlockFunc_LeftSlope
	void water_surface(); // BlockFunc_WaterSurface
	void object_floor(); // BlockFunc_18262
	void ladder(); // BlockFunc_Ladder
	void wario_ladder_else(uint16_t other); // BlockFunc_182dc, BlockFunc_LadderTop
	void ladder_transition(uint8_t param);
	void spike(uint16_t next);
	void water_spike(); // BlockFunc_WaterSpike
	void enter_pipe(bool down);
	void pipe(uint8_t state1, uint8_t state2, uint8_t coll, int button, bool left);
	void door(bool slipping, uint8_t param);
	void underwater_door(bool water, uint8_t param);
	void treasure_door(); // BlockFunc_TreasureDoor
	void transition_block(uint16_t next, uint8_t param);
	void walkthrough_pipe(); // BlockFunc_WalkthroughPipe
	void light_source(); // BlockFunc_LightSource
	void in_block(uint16_t addr, uint8_t v);
	void vampire_block(); // BlockFunc_18868
	void wario_or_else(uint16_t wario, uint16_t other); // BlockFunc_188ba, BlockFunc_189f1
	void fire(); // BlockFunc_Fire
	void fence(); // BlockFunc_Fence
	void air_current(); // BlockFunc_AirCurrent
	void slippery(uint16_t next);
	void cloud_platform_1(); // BlockFunc_CloudPlatform1
	void cloud_platform_2(); // BlockFunc_CloudPlatform2
	void small_gap(); // BlockFunc_SmallGap
	void switch_block(); // BlockFunc_Switch
	void break_block(bool chance, uint16_t params); // BreakBlock, BreakBlockWithColourCoin
	void saved_break(bool floor_transition, uint16_t next); // Func_18f32, Func_19246, Func_193dc
	void break_free(); // Func_19481
	void break_free_if_rolling(); // Func_18df1
	void break_multi(); // Func_19086
	void break_by_object(); // Func_19423
	void doughnut_block(); // DoughnutBlockCollision
	void cracked_block(uint16_t break_func);
	void cracked(); // CrackedBlockCollision
	void non_cracked(); // NonCrackedBlockCollision
	void enemy_block(); // EnemyBlockCollision
	void bonfire_block(); // BonfireBlockCollision
	void cracked_colour_coin(uint8_t block);
	void ldb_jump(uint8_t block, uint16_t target);
	void transformation_multi(uint8_t n, uint8_t transformation, Progress progress);
	void cracked_multi(uint8_t n);
	void noncracked_multi(uint8_t n);
	void ball_o_string_block(); // Func_19502

private:
	bool zf() const { return cpu_.f & FLAG_Z; }
	bool cf() const { return cpu_.f & FLAG_C; }
	void xor_a()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
	}
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void push_af() { gb_push16(gb_, REG_AF(gb_)); }
	void pop_af()
	{
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
	}
	// ld a, v / ld [addr], a
	void st(uint16_t addr, uint8_t v)
	{
		cpu_.a = v;
		mem_[addr] = v;
	}
	// ld a, [addr] / and a: true if nz
	bool ld_and(uint16_t addr)
	{
		cpu_.a = mem_[addr];
		cpu_.f = flags::and_(cpu_.a);
		return cpu_.a;
	}
	// ld a, [addr] / bit n, a
	bool ld_bit(uint16_t addr, int n)
	{
		cpu_.a = mem_[addr];
		return bit_a(n);
	}
	// bit n, a
	bool bit_a(int n)
	{
		cpu_.f = flags::bit(cpu_.f, cpu_.a, n);
		return cpu_.a & (1 << n);
	}
	// ld a, [addr] / and mask: true if nz
	bool ld_mask(uint16_t addr, uint8_t mask)
	{
		cpu_.a = mem_[addr] & mask;
		cpu_.f = flags::and_(cpu_.a);
		return cpu_.a;
	}
	// ld a, [addr] / cp v
	void ld_cp(uint16_t addr, uint8_t v)
	{
		cpu_.a = mem_[addr];
		cp_a(v);
	}
	void cp_a(uint8_t v) { cpu_.f = flags::cp(cpu_.a, v); }
	void and_a(uint8_t mask)
	{
		cpu_.a &= mask;
		cpu_.f = flags::and_(cpu_.a);
	}
	bool wario_flag() { return ld_and(wBlockFuncWarioFlag); }
	bool coll_bit(int n) { return ld_bit(wCollisionPoints, n); }
	// the play_sfx macro: a ends as the SFX's low byte
	void sfx(uint16_t id)
	{
		play_sfx(mem_, id);
		cpu_.a = static_cast<uint8_t>(id);
	}
	// ld a, HIGH(ptr) / ld [addr + 0], a / ld a, LOW(ptr) / ld [addr + 1], a
	void st_ptr(uint16_t addr, uint16_t ptr)
	{
		mem_[addr + 0] = static_cast<uint8_t>(ptr >> 8);
		st(addr + 1, static_cast<uint8_t>(ptr));
	}
	void clear(std::initializer_list<uint16_t> addrs)
	{
		xor_a();
		for (uint16_t addr : addrs)
			mem_[addr] = 0;
	}
	void set_slope_interaction(uint8_t slope);
	void hypos_top_plus_c();
	void save_block();
	void replace_block(bool e2b);
	void replace_and_break();
	void free_if_strong();
	bool robo_mouse();
	bool breaks(bool head_smash_reads_a);
	void multi_or(uint8_t n);
	bool multi_start(uint16_t low, uint8_t n, uint16_t high);
	bool multi_robo_mouse(uint8_t n, uint16_t broken);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// the function of the block at wBlockPtrBank:wBlockPtr: jp hl, so it
// returns to the caller
void BlockFuncs::process_block()
{
	mem_[wIsInWaterOrSand] = FALSE;
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	// the block (at wBlockPtr, stored big-endian) indexes the level's
	// wBlockFunctionTable
	uint16_t at = static_cast<uint16_t>(mem_[wBlockPtr + 0] << 8 | mem_[wBlockPtr + 1]);
	uint8_t block = mem_[at];
	uint8_t offset = static_cast<uint8_t>(block * 2);
	cpu_.f = flags::add(block, block);
	cpu_.d = 0;
	cpu_.e = offset;
	unsigned sum = wBlockFunctionTable + offset;
	cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((wBlockFunctionTable & 0xfff) + offset > 0xfff ? FLAG_H : 0)
		| (sum > 0xffff ? FLAG_C : 0));
	uint16_t func = Word(gb_, static_cast<uint16_t>(sum));
	cpu_.a = static_cast<uint8_t>(func);
	SET_HL(gb_, func);
	cpu_.pc = func;
	// The levels' tables (data/levels/function_tables*.asm) hold only
	// this file's block functions: a fuzzed table pointing anywhere else
	// (another routine's label) isn't comparable.
	if (gb_->test.active && (func < ADDR_ProcessBlock || func > ADDR_BreakBlockWithColourCoin
		|| !sym_is_rom_label(BANK_ProcessBlock, func)))
		gb_->test.unrealistic = true;
}

// a = 0: nothing there
void BlockFuncs::free()
{
	sram_switch(0); // SRAM0
	mem_[wBlockFuncBreakFlag] = FALSE;
	mem_[wBlockFuncSpecialCollision] = 0;
	mem_[wUnused_c18d] = 0;
	xor_a();
}

// a = 1 (snapped to the block's top), or 0 for Wario on a slope
void BlockFuncs::solid()
{
	sram_switch(0); // SRAM0
	mem_[wBlockFuncBreakFlag] = FALSE;
	mem_[wBlockFuncSpecialCollision] = 0;
	bool keep = mem_[wBlockFuncWarioFlag] && mem_[wWarioSlopeInteraction];
	if (keep) {
		uint8_t points = mem_[wCollisionPoints];
		keep = points & (1 << COLLISION_UNK4_F);
	}
	if (keep) {
		mem_[wUnused_c18d] = 0;
		xor_a();
		return;
	}
	// .snap_y_pos
	cpu_.a = mem_[hYPosLo] & 0xf0;
	cpu_.f = flags::and_(cpu_.a);
	mem_[hYPosLo] = cpu_.a;
	cpu_.a = 1;
	mem_[wUnused_c18d] = 1;
}

void BlockFuncs::water()
{
	st(wIsInWaterOrSand, TRUE);
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	st(wWaterInteraction, SUBMERSIBLE_WATER);
	GOTO(FREE);
}

// the current, if Wario's in it; then it's water
void BlockFuncs::block_func_current(uint8_t current)
{
	if (coll_bit(COLLISION_UNK7_F))
		st(wWaterCurrent, current);
	GOTO(WATER);
}

void BlockFuncs::floor()
{
	st(wc1ca, TRUE);
	cpu_.a = mem_[wCollisionPoints];
	bool solid = bit_a(COLLISION_DOWN_F) || bit_a(COLLISION_UNK2_F);
	if (!solid) {
		and_a(COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7);
		if (!zf())
			GOTO(FREE);
		cpu_.a = mem_[hYPosLo] & 0x0f;
		cp_a(0x05);
		if (!cf())
			GOTO(FREE);
	}
	// .asm_18228
	if (!wario_flag())
		GOTO(SOLID);
	ld_cp(wWarioState, WST_ZOMBIE_LANDING);
	if (!zf())
		cp_a(WST_ZOMBIE_SLIPPING_THROUGH_FLOOR);
	GOTO(zf() ? FREE : SOLID);
}

// a = 0, like BlockFunc_Free
void BlockFuncs::free_1808c()
{
	sram_switch(0); // SRAM0
	mem_[wBlockFuncBreakFlag] = FALSE;
	mem_[wBlockFuncSpecialCollision] = 0;
	mem_[wUnused_c18d] = 0;
	xor_a();
}

void BlockFuncs::ladder_or_floor()
{
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FLOOR);
	st(wLadderInteraction, LADDER_UNDERNEATH);
	GOTO(ADDR_Func_1808c);
}

void BlockFuncs::enter_door()
{
	// centred on the door
	uint8_t x = mem_[wWarioXPos + 1] & 0xf0;
	cpu_.f = flags::add(x, 0x08);
	cpu_.a = static_cast<uint8_t>(x + 0x08);
	mem_[wWarioXPos + 1] = cpu_.a;
	sfx(SFX_0E6);
	st(wWarioState, WST_ENTERING_DOOR);
	xor_a();
	for (uint16_t addr : {wSFXLoopCounter, wWarioStateCounter, wGrabState, wAttackCounter, wJumpVelIndex,
		     wJumpVelTable, wIsCrouching, wFrameDuration, wAnimationFrame})
		mem_[addr] = 0;
	mem_[wDMASourceBank] = BANK_WarioIdleGfx;
	st_ptr(wDMASourcePtr, ADDR_WarioIdleGfx);
	routines_.call(ADDR_LoadWarioGfx);
	mem_[wWarioOAMBank] = BANK_OAM_14a82;
	st_ptr(wWarioOAMPtr, ADDR_OAM_14a82);
	bool right = ld_and(wDirection);
	st_ptr(wWarioFramesetPtr, right ? ADDR_Frameset_14d10 : ADDR_Frameset_14d0b);
	routines_.farcall(OAM1, ADDR_UpdateWarioAnimation);
	GOTO(FREE);
}

// the SRAM0 / clear flags start, with v in wUnused_c18d (and a)
void BlockFuncs::block_func_c18d(uint8_t v)
{
	sram_switch(0); // SRAM0
	xor_a();
	mem_[wBlockFuncBreakFlag] = FALSE;
	mem_[wBlockFuncSpecialCollision] = 0;
	st(wUnused_c18d, v);
}

// a slope a block up: solid to an air attack
void BlockFuncs::slope_above(uint16_t collision)
{
	if (wario_flag()) {
		ld_cp(wWarioState, WST_ATTACKING_AIRBORNE);
		if (zf())
			GOTO(SOLID);
	}
	if (!ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		GOTO(FREE);
	uint8_t lo = mem_[hYPosLo];
	mem_[hYPosLo] = static_cast<uint8_t>(lo - 0x10);
	bool borrow = lo < 0x10;
	uint8_t hi = mem_[hYPosHi];
	cpu_.f = flags::sub(hi, 0, borrow);
	cpu_.a = static_cast<uint8_t>(hi - borrow);
	mem_[hYPosHi] = cpu_.a;
	GOTO(collision);
}

// a slope a block down
void BlockFuncs::slope_below(uint16_t collision)
{
	if (!coll_bit(COLLISION_DOWN_F))
		GOTO(FREE);
	unsigned lo = mem_[hYPosLo] + 0x10;
	mem_[hYPosLo] = static_cast<uint8_t>(lo);
	uint8_t hi = mem_[hYPosHi];
	cpu_.f = flags::add(hi, 0, lo > 0xff);
	cpu_.a = static_cast<uint8_t>(hi + (lo > 0xff));
	mem_[hYPosHi] = cpu_.a;
	GOTO(collision);
}

// Wario's or the object's slope interaction
void BlockFuncs::set_slope_interaction(uint8_t slope)
{
	st(wario_flag() ? wWarioSlopeInteraction : wObjSlopeInteraction, ON_SLOPE | slope);
}

// hYPos = the block's top + c
void BlockFuncs::hypos_top_plus_c()
{
	uint8_t top = mem_[hYPosLo] & 0xf0;
	unsigned lo = top + cpu_.c;
	mem_[hYPosLo] = static_cast<uint8_t>(lo);
	uint8_t hi = mem_[hYPosHi];
	cpu_.f = flags::add(hi, 0, lo > 0xff);
	cpu_.a = static_cast<uint8_t>(hi + (lo > 0xff));
	mem_[hYPosHi] = cpu_.a;
}

void BlockFuncs::right_slope_collision()
{
	uint8_t x = mem_[hXPosLo] & 0x0f;
	cpu_.c = static_cast<uint8_t>(0x10 - x);
	hypos_top_plus_c();
	if (ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		set_slope_interaction(RIGHT_SLOPE);
	GOTO(ADDR_Func_18064);
}

void BlockFuncs::left_slope_collision()
{
	if (ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1)) {
		cpu_.a = static_cast<uint8_t>((mem_[hXPosLo] & 0x0f) + 2);
		cpu_.c = cpu_.a;
		hypos_top_plus_c();
		set_slope_interaction(LEFT_SLOPE);
	}
	GOTO(ADDR_Func_18064);
}

void BlockFuncs::right_slope()
{
	if (!ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK1_F))
		GOTO(ADDR_RightSlopeCollision);
	cpu_.b = mem_[hYPosLo] & 0x0f;
	uint8_t x = mem_[hXPosLo] & 0x0f;
	cpu_.a = static_cast<uint8_t>(x + cpu_.b);
	cp_a(0x10);
	GOTO(cf() ? FREE : ADDR_RightSlopeCollision);
}

void BlockFuncs::left_slope()
{
	if (!ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1))
		GOTO(FREE);
	if (!bit_a(COLLISION_UNK1_F))
		GOTO(ADDR_LeftSlopeCollision);
	cpu_.b = mem_[hYPosLo] & 0x0f;
	cpu_.a = mem_[hXPosLo] & 0x0f;
	cp_a(cpu_.b);
	GOTO(cf() ? ADDR_LeftSlopeCollision : FREE);
}

void BlockFuncs::water_surface()
{
	if (coll_bit(COLLISION_UNK1_F))
		GOTO(FLOOR);
	if (ld_mask(hYPosLo, 0x0e))
		GOTO(WATER);
	if (!ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK2))
		GOTO(WATER);
	st(wIsInWaterOrSand, TRUE);
	GOTO(SOLID);
}

// a floor to objects, free to Wario
void BlockFuncs::object_floor()
{
	if (wario_flag())
		GOTO(FREE);
	st(wc1ca, TRUE);
	cpu_.a = mem_[wCollisionPoints];
	if (bit_a(COLLISION_DOWN_F))
		GOTO(SOLID);
	if (bit_a(COLLISION_UNK2_F))
		GOTO(SOLID);
	and_a(COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7);
	if (cpu_.a)
		GOTO(FREE);
	cpu_.a = mem_[hYPosLo] & 0x0f;
	cp_a(0x05);
	GOTO(cf() ? SOLID : FREE);
}

void BlockFuncs::ladder()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	st(wLadderInteraction, LADDER_COLLISION);
	GOTO(ADDR_Func_1808c);
}

void BlockFuncs::wario_ladder_else(uint16_t other)
{
	GOTO(wario_flag() ? ADDR_Func_182ec : other);
}

void BlockFuncs::ladder_transition(uint8_t param)
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	st(wLadderInteraction, LADDER_COLLISION);
	st(wRoomTransitionParam, param);
	GOTO(FREE);
}

void BlockFuncs::spike(uint16_t next)
{
	if (!wario_flag())
		GOTO(next);
	if (ld_and(wInvincibleCounter))
		GOTO(next);
	ld_cp(wca94, 0x02);
	if (zf())
		GOTO(next);
	cpu_.a = mem_[wWarioState];
	cp_a(WST_STUNG);
	if (zf())
		GOTO(next);
	cp_a(WST_STUNG_RECOVERY);
	if (zf())
		GOTO(next);
	// turned around
	st(wDirection, ld_and(wDirection) ? DIRECTION_LEFT : DIRECTION_RIGHT);
	if (!ld_and(wca94)) {
		routines_.farcall(BANK_SetState_Stung, ADDR_SetState_Stung);
		GOTO(next);
	}
	cp_a(0x01);
	if (zf())
		routines_.farcall(BANK_Func_206eb, ADDR_Func_206eb);
	GOTO(next);
}

void BlockFuncs::water_spike()
{
	if (!wario_flag())
		GOTO(WATER);
	if (ld_and(wInvincibleCounter))
		GOTO(WATER);
	ld_cp(wWarioState, WST_WATER_STUNG);
	if (!zf())
		routines_.farcall(BANK_SetState_WaterStung, ADDR_SetState_WaterStung);
	GOTO(WATER);
}

void BlockFuncs::enter_pipe(bool down)
{
	st(wRoomTransitionParam, ROOMTRANSITION_PIPE | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY);
	sfx(SFX_PIPE_ENTER);
	st(wWarioState, down ? WST_PIPE_GOING_DOWN : WST_PIPE_GOING_UP);
	clear({wFrameDuration, wAnimationFrame, wWarioStateCounter, wWarioStateCycles, wGrabState, wAttackCounter,
		wJumpVelIndex, wJumpVelTable, wIsCrouching});
	if (down)
		mem_[wIsSmashAttacking] = 0;
	mem_[wDMASourceBank] = BANK_WarioIdleGfx;
	st_ptr(wDMASourcePtr, ADDR_WarioIdleGfx);
	routines_.call(ADDR_LoadWarioGfx);
	mem_[wWarioOAMBank] = BANK_OAM_14a82;
	st_ptr(wWarioOAMPtr, ADDR_OAM_14a82);
	st_ptr(wWarioFramesetPtr, ADDR_Frameset_14cf6);
	routines_.farcall(OAM1, ADDR_UpdateWarioAnimation);
	GOTO(SOLID);
}

// a pipe's entrance: Wario in state1 or state2, touching it on the
// collision side, holding the button
void BlockFuncs::pipe(uint8_t state1, uint8_t state2, uint8_t coll, int button, bool left)
{
	if (!wario_flag())
		GOTO(SOLID);
	cpu_.a = mem_[wWarioState];
	cp_a(state1);
	if (!zf()) {
		cp_a(state2);
		if (!zf())
			GOTO(SOLID);
	}
	if (!ld_mask(wCollisionPoints, coll))
		GOTO(SOLID);
	if (!ld_bit(wJoypadDown, button))
		GOTO(SOLID);
	uint8_t x = mem_[wWarioXPos + 1] & 0xf0;
	uint8_t add = left ? 0x18 : 0x08;
	cpu_.f = flags::add(x, add);
	cpu_.a = static_cast<uint8_t>(x + add);
	mem_[wWarioXPos + 1] = cpu_.a;
	if (left) {
		bool carry = x + add > 0xff;
		uint8_t hi = mem_[wWarioXPos + 0];
		cpu_.f = flags::add(hi, 0, carry);
		cpu_.a = static_cast<uint8_t>(hi + carry);
		mem_[wWarioXPos + 0] = cpu_.a;
	}
	enter_pipe(button == B_PAD_DOWN);
}

// a door: Wario in front of it pressing up while idling (or slipping)
void BlockFuncs::door(bool slipping, uint8_t param)
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	if (!ld_bit(wJoypadPressed, B_PAD_UP))
		GOTO(FREE);
	cpu_.a = mem_[wWarioState];
	cp_a(WST_IDLING);
	if (!zf() && slipping)
		cp_a(WST_SLIPPING);
	if (!zf()) {
		cp_a(WST_GRAB_IDLING);
		if (!zf())
			GOTO(FREE);
	}
	st(wRoomTransitionParam, param);
	GOTO(ADDR_EnterDoor);
}

// an underwater door (water: in submersible water there)
void BlockFuncs::underwater_door(bool water, uint8_t param)
{
	uint16_t next = water ? WATER : FREE;
	if (!wario_flag())
		GOTO(next);
	ld_cp(wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
	if (!zf()) {
		cpu_.f = flags::and_(cpu_.a);
		if (cpu_.a)
			GOTO(next);
	}
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(next);
	if (water)
		st(wWaterInteraction, SUBMERSIBLE_WATER);
	if (ld_bit(wJoypadPressed, B_PAD_UP))
		st(wRoomTransitionParam, param);
	GOTO(FREE);
}

void BlockFuncs::treasure_door()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!ld_bit(wJoypadPressed, B_PAD_UP))
		GOTO(FREE);
	if (ld_and(wTransformation))
		GOTO(FREE);
	cpu_.a = mem_[wWarioState];
	cp_a(WST_IDLING);
	if (!zf())
		cp_a(WST_SLIPPING);
	if (!zf()) {
		cp_a(WST_GRAB_IDLING);
		if (!zf())
			GOTO(FREE);
	}
	xor_a();
	mem_[wGrabState] = 0;
	st(wLevelEndScreen, LVLEND_NO_TREASURE);
	GOTO(FREE);
}

void BlockFuncs::transition_block(uint16_t next, uint8_t param)
{
	if (!wario_flag())
		GOTO(next);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(next);
	st(wRoomTransitionParam, param);
	GOTO(next);
}

void BlockFuncs::walkthrough_pipe()
{
	if (!wario_flag())
		GOTO(FREE);
	ld_cp(wTransformation, TRANSFORMATION_INVISIBLE_WARIO);
	if (!zf())
		GOTO(FREE);
	routines_.call(ADDR_RecoverFromTransformation_WithoutInvincibility);
	GOTO(FREE);
}

void BlockFuncs::light_source()
{
	if (!wario_flag())
		GOTO(FREE);
	cpu_.a = mem_[wTransformation];
	cp_a(TRANSFORMATION_ZOMBIE_WARIO);
	if (zf()) {
		routines_.farcall(BANK_SetState_ZombieRecovering, ADDR_SetState_ZombieRecovering);
		GOTO(FREE);
	}
	cp_a(TRANSFORMATION_VAMPIRE_WARIO);
	if (!zf())
		GOTO(FREE);
	routines_.call(ADDR_RecoverFromTransformation_WithoutInvincibility);
	GOTO(FREE);
}

// sand / non-submersible water: in it, and Wario's in it if he touches it
void BlockFuncs::in_block(uint16_t addr, uint8_t v)
{
	st(wIsInWaterOrSand, TRUE);
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	st(addr, v);
	GOTO(FREE);
}

// solid to Vampire Wario, who's pushed out of it
void BlockFuncs::vampire_block()
{
	if (!wario_flag())
		GOTO(FREE);
	ld_cp(wTransformation, TRANSFORMATION_VAMPIRE_WARIO);
	if (!zf())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(SOLID);
	if (!ld_mask(wWarioXPos + 1, 0x08)) {
		cpu_.a = mem_[wCollisionBoxRight];
		cpu_.b = cpu_.a;
		uint8_t x = mem_[wWarioXPos + 1] & 0xf0;
		mem_[wWarioXPos + 1] = static_cast<uint8_t>(x - cpu_.b);
		bool borrow = x < cpu_.b;
		uint8_t hi = mem_[wWarioXPos + 0];
		cpu_.f = flags::sub(hi, 0, borrow);
		cpu_.a = static_cast<uint8_t>(hi - borrow);
	} else {
		cpu_.a = mem_[wCollisionBoxLeft];
		cpu_.b = static_cast<uint8_t>(0x10 - cpu_.a);
		uint8_t x = mem_[wWarioXPos + 1] & 0xf0;
		unsigned lo = x + cpu_.b;
		mem_[wWarioXPos + 1] = static_cast<uint8_t>(lo);
		uint8_t hi = mem_[wWarioXPos + 0];
		cpu_.f = flags::add(hi, 0, lo > 0xff);
		cpu_.a = static_cast<uint8_t>(hi + (lo > 0xff));
	}
	mem_[wWarioXPos + 0] = cpu_.a;
	GOTO(SOLID);
}

void BlockFuncs::wario_or_else(uint16_t wario, uint16_t other)
{
	GOTO(wario_flag() ? wario : other);
}

// Hot Wario, unless invincible, already transformed for good, or only
// touching it with the bottom points
void BlockFuncs::fire()
{
	if (!wario_flag())
		GOTO(FREE);
	if (ld_mask(wCollisionPoints, COLLISION_DOWN | COLLISION_UNK1 | COLLISION_UNK2 | COLLISION_UNK6))
		GOTO(FREE);
	if (ld_and(wInvincibleCounter))
		GOTO(FREE);
	if (ld_bit(wTransformation, TRANSFORMATIONF_PERSISTENT_F))
		GOTO(FREE);
	st(wTransformation, TRANSFORMATION_HOT_WARIO);
	st(wTransformationProgress, 1);
	st(wTouchState, TOUCH_BUMP);
	st(wStingTouchState, TOUCH_BUMP);
	st(wca94, 0x02);
	st(wTransformationDuration + 0, static_cast<uint8_t>(HOT_WARIO_DURATION >> 8));
	st(wTransformationDuration + 1, static_cast<uint8_t>(HOT_WARIO_DURATION));
	routines_.call(ADDR_UpdateLevelMusic);
	routines_.farcall(BANK_SetState_OnFire_ResetStateCounter, ADDR_SetState_OnFire_ResetStateCounter);
	cpu_.pc = gb_pop16(gb_); // ret
}

void BlockFuncs::fence()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	st(wIsNearFence, TRUE);
	GOTO(ADDR_Func_1808c);
}

void BlockFuncs::air_current()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	if (ld_and(wInvincibleCounter))
		GOTO(FREE);
	st(wIsInAirCurrent, TRUE);
	if (ld_and(wTransformation))
		GOTO(FREE);
	st(wTransformation, TRANSFORMATION_FAN);
	st(wTouchState, TOUCH_BUMP);
	st(wStingTouchState, TOUCH_VULNERABLE);
	st(wca94, 0x01);
	routines_.farcall(BANK_SetState_FanStart, ADDR_SetState_FanStart);
	GOTO(FREE);
}

void BlockFuncs::slippery(uint16_t next)
{
	if (!wario_flag())
		GOTO(next);
	if (!ld_mask(wCollisionPoints, COLLISION_DOWN))
		GOTO(next);
	st(wIsOnSlipperyGround, TRUE);
	GOTO(next);
}

void BlockFuncs::cloud_platform_1()
{
	cpu_.a = mem_[wRoomPalCycleIndex];
	cp_a(0x07);
	if (!zf()) {
		cp_a(0x04);
		if (!cf())
			GOTO(FREE);
	}
	GOTO(FLOOR);
}

void BlockFuncs::cloud_platform_2()
{
	ld_cp(wRoomPalCycleIndex, 0x03);
	GOTO(cf() ? FREE : FLOOR);
}

void BlockFuncs::small_gap()
{
	if (!wario_flag())
		GOTO(FREE);
	ld_cp(wTransformation, TRANSFORMATION_FLAT_WARIO);
	GOTO(zf() ? FREE : SOLID);
}

// toggles the switch Wario hit (and with it every switchable block), then
// it's solid
void BlockFuncs::switch_block()
{
	if (!wario_flag())
		GOTO(SOLID);
	cpu_.a = mem_[wCollisionPoints] & (COLLISION_UNK3 | COLLISION_UNK5);
	cpu_.f = flags::and_(cpu_.a);
	if (zf())
		GOTO(SOLID);
	uint16_t at = static_cast<uint16_t>(mem_[wBlockPtr] << 8 | mem_[wBlockPtr + 1]);
	SET_HL(gb_, at);
	uint8_t block = mem_[at];
	mem_[wSwitchState] = block;
	cpu_.a = block ^ 0x08;
	mem_[at] = cpu_.a;
	routines_.call(ADDR_Func_e31);
	cpu_.a = mem_[wSwitchStateUpdated] ^ 1;
	cpu_.f = flags::xor_(cpu_.a);
	mem_[wSwitchStateUpdated] = cpu_.a;
	sfx(SFX_SWITCH);
	cpu_.b = 16;
	routines_.call(ADDR_DoGroundShake);
	GOTO(SOLID);
}

// debris at the block in hPos and an object (params) made there (a coin 1
// time in 4 for BreakBlock, unless on cooldown), keeping hPos and wCurObj
void BlockFuncs::break_block(bool chance, uint16_t params)
{
	mem_[hYPosHiBackup] = mem_[hYPosHi];
	mem_[hYPosLoBackup] = mem_[hYPosLo];
	mem_[hXPosHiBackup] = mem_[hXPosHi];
	cpu_.a = mem_[hXPosLo];
	mem_[hXPosLoBackup] = cpu_.a;

	cpu_.b = PARTICLE_DEBRIS;
	routines_.farcall(BANK_CreateParticleInBlock, ADDR_CreateParticleInBlock);
	bool spawn = true;
	if (chance) {
		cpu_.a = mem_[rDIV] & 0x03;
		cpu_.f = flags::and_(cpu_.a);
		if (cpu_.a)
			spawn = false;
		else
			spawn = !ld_and(wCoinCooldown);
	}
	if (spawn) {
		cpu_.a = mem_[rWBK]; // push_wram BANK("Level Objects WRAM")
		push_af();
		cpu_.a = 0x01;
		mem_[rWBK] = 0x01;
		if (chance)
			st(wCoinCooldown, COIN_COOLDOWN);

		// wCurObj kept
		SET_HL(gb_, wCurObjUnk01);
		SET_DE(gb_, hCurObjBackup);
		cpu_.b = OBJ_ID - OBJ_UNK_01;
		routines_.call(ADDR_CopyHLToDE);

		xor_a();
		mem_[wCurObjUnk01 + 0] = 0;
		mem_[wCurObjUnk01 + 1] = 0;
		mem_[wCurObjUnk01 + 2] = mem_[hYPosLo];
		mem_[wCurObjUnk01 + 3] = mem_[hYPosHi];
		mem_[wCurObjUnk01 + 4] = mem_[hXPosLo];
		cpu_.a = mem_[hXPosHi];
		mem_[wCurObjUnk01 + 5] = cpu_.a;
		SET_HL(gb_, wCurObjUnk01 + 6);
		SET_BC(gb_, params);
		routines_.farcall(BANK__CreateObjectFromCurObjPos, ADDR__CreateObjectFromCurObjPos);

		SET_HL(gb_, hCurObjBackup);
		SET_DE(gb_, wCurObjUnk01);
		cpu_.b = OBJ_ID - OBJ_UNK_01;
		routines_.call(ADDR_CopyHLToDE);
		pop_af();
		mem_[rWBK] = cpu_.a;
	}

	mem_[hYPosHi] = mem_[hYPosHiBackup];
	mem_[hYPosLo] = mem_[hYPosLoBackup];
	mem_[hXPosHi] = mem_[hXPosHiBackup];
	cpu_.a = mem_[hXPosLoBackup];
	mem_[hXPosLo] = cpu_.a;
}

// wcedb / wcedc / wcedf = the SRAM bank, pointer and value of the block at
// wBlockPtr
void BlockFuncs::save_block()
{
	mem_[wcedb] = mem_[wSRAMBank];
	cpu_.d = mem_[wBlockPtr + 0];
	mem_[wcedc + 0] = cpu_.d;
	cpu_.e = mem_[wBlockPtr + 1];
	mem_[wcedc + 1] = cpu_.e;
	SET_HL(gb_, wBlockPtr + 1);
	cpu_.a = mem_[REG_DE(gb_)];
	mem_[wcedf] = cpu_.a;
}

// the block at wBlockPtr = b (keeping bit 7), redrawn with Func_e31 (or
// Func_e2b)
void BlockFuncs::replace_block(bool e2b)
{
	uint16_t at = static_cast<uint16_t>(mem_[wBlockPtr] << 8 | mem_[wBlockPtr + 1]);
	SET_HL(gb_, at);
	cpu_.a = (mem_[at] & 0x80) | cpu_.b;
	cpu_.f = flags::or_(cpu_.a);
	mem_[at] = cpu_.a;
	gb_push16(gb_, at);
	routines_.call(e2b ? ADDR_Func_e2b : ADDR_Func_e31);
	SET_HL(gb_, gb_pop16(gb_));
}

void BlockFuncs::replace_and_break()
{
	replace_block(false);
	routines_.call(ADDR_BreakBlock);
}

// after breaking: free when rolling or fat, else solid unless strong enough
// (garlic, or the super jump slam overalls for a ground pound)
void BlockFuncs::free_if_strong()
{
	if (ld_and(wIsRolling))
		GOTO(FREE);
	ld_cp(wTransformation, TRANSFORMATION_FAT_WARIO);
	if (zf())
		GOTO(FREE);
	bool smash = ld_and(wIsSmashAttacking);
	ld_cp(wPowerUpLevel, smash ? SUPER_JUMP_SLAM_OVERALLS : GARLIC);
	GOTO(cf() ? SOLID : FREE);
}

// Func_18f32 / Func_19246 / Func_193dc: saves the block, then breaks it
void BlockFuncs::saved_break(bool floor_transition, uint16_t next)
{
	save_block();
	if (floor_transition) {
		st(wIsFloorTransition, TRUE);
		GOTO(next);
	}
	replace_and_break();
	GOTO(next);
}

void BlockFuncs::break_free()
{
	replace_and_break();
	GOTO(FREE);
}

void BlockFuncs::break_free_if_rolling()
{
	replace_and_break();
	GOTO(ld_and(wIsRolling) ? FREE : SOLID);
}

void BlockFuncs::break_multi()
{
	save_block();
	st(wIsFloorTransition, TRUE);
	replace_and_break();
	free_if_strong();
}

// broken by an object: debris if it's on screen, else just the sound
void BlockFuncs::break_by_object()
{
	replace_block(true);
	cpu_.a = mem_[wCurObjFlags];
	if (bit_a(OBJFLAG_ON_SCREEN_F))
		routines_.call(ADDR_BreakBlock);
	else
		sfx(SFX_BLOCK_BREAK);
	GOTO(FREE);
}

void BlockFuncs::doughnut_block()
{
	if (!wario_flag())
		GOTO(SOLID);
	ld_cp(wTransformation, TRANSFORMATION_FAT_WARIO);
	if (!zf())
		GOTO(SOLID);
	if (!ld_mask(wCollisionPoints, COLLISION_UNK1 | COLLISION_UNK6))
		GOTO(SOLID);
	replace_and_break();
	GOTO(SOLID);
}

// the robo mouse breaks (Func_19423) or passes through: true if it jumped
bool BlockFuncs::robo_mouse()
{
	cpu_.a = mem_[wBlockFuncSpecialCollision];
	cp_a(COLLISION_ROBO_MOUSE_BREAK);
	if (zf()) {
		cpu_.pc = ADDR_Func_19423;
		return true;
	}
	cp_a(COLLISION_ROBO_MOUSE_FREE);
	if (zf()) {
		cpu_.pc = FREE;
		return true;
	}
	return false;
}

// b = the block that replaces it: broken by an attack or a roll (from the
// side), a head smash with the helmet (above), or a ground pound
void BlockFuncs::cracked_block(uint16_t break_func)
{
	if (!ld_and(wBlockFuncBreakFlag))
		GOTO(SOLID);
	bool brk;
	if (coll_bit(COLLISION_UNK4_F)) {
		brk = ld_and(wAttackCounter) || ld_and(wIsRolling);
	} else if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F)) {
		ld_cp(wPowerUpLevel, HEAD_SMASH_HELMET);
		if (cf())
			GOTO(SOLID);
		brk = ld_and(wJumpVelTable) || ld_and(wSwimVelIndex);
	} else {
		and_a(COLLISION_UNK1 | COLLISION_UNK6);
		brk = !zf();
	}
	if (!brk)
		GOTO(SOLID);
	replace_block(false);
	routines_.call(break_func);
	free_if_strong();
}

void BlockFuncs::cracked()
{
	if (robo_mouse())
		return;
	cracked_block(ADDR_BreakBlock);
}

// broken with garlic and an attack, a roll, a head smash with the helmet
// or a ground pound with the super jump slam overalls; or by Fat Wario's
// ground pound
void BlockFuncs::non_cracked()
{
	if (robo_mouse())
		return;
	if (!ld_and(wBlockFuncBreakFlag))
		GOTO(SOLID);
	ld_cp(wTransformation, TRANSFORMATION_FAT_WARIO);
	if (zf()) {
		bool pound = ld_mask(wCollisionPoints, COLLISION_UNK1 | COLLISION_UNK6);
		GOTO(pound ? ADDR_Func_18df1 : SOLID);
	}
	ld_cp(wPowerUpLevel, GARLIC);
	if (cf())
		GOTO(SOLID);
	bool brk;
	if (coll_bit(COLLISION_UNK4_F)) {
		brk = ld_and(wAttackCounter) || ld_and(wIsRolling);
	} else if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F)) {
		ld_cp(wPowerUpLevel, HEAD_SMASH_HELMET);
		if (cf())
			GOTO(SOLID);
		brk = ld_and(wJumpVelTable) || ld_and(wSwimVelIndex);
	} else {
		and_a(COLLISION_UNK1 | COLLISION_UNK6);
		if (zf())
			GOTO(SOLID);
		ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		brk = !cf();
	}
	GOTO(brk ? ADDR_Func_18df1 : SOLID);
}

// broken by a thrown object
void BlockFuncs::enemy_block()
{
	ld_cp(wBlockFuncSpecialCollision, COLLISION_THROWN_OBJ);
	GOTO(zf() ? ADDR_Func_19423 : SOLID);
}

// burnt by Hot Wario once he's engulfed in flames
void BlockFuncs::bonfire_block()
{
	if (!wario_flag())
		GOTO(SOLID);
	ld_cp(wTransformation, TRANSFORMATION_HOT_WARIO);
	if (zf()) {
		ld_cp(wTransformationProgress, 2);
		if (!cf())
			GOTO(ADDR_Func_19481);
	}
	GOTO(SOLID);
}

// ld b, block / jr CrackedBlockWithColourCoinCollision
void BlockFuncs::cracked_colour_coin(uint8_t block)
{
	cpu_.b = block;
	GOTO(ADDR_CrackedBlockWithColourCoinCollision);
}

// ld b, block / jr target
void BlockFuncs::ldb_jump(uint8_t block, uint16_t target)
{
	cpu_.b = block;
	GOTO(target);
}

// or n / ld [wMultiBlockParam], a
void BlockFuncs::multi_or(uint8_t n)
{
	cpu_.a |= n;
	cpu_.f = flags::or_(cpu_.a);
	mem_[wMultiBlockParam] = cpu_.a;
}

// wMultiBlockParam: bits 0-2 the part already handled (-> low), bits 3-7 a
// block of the same group broken this frame (or n, -> high); true if it
// jumped
bool BlockFuncs::multi_start(uint16_t low, uint8_t n, uint16_t high)
{
	if (ld_mask(wMultiBlockParam, 0x07)) {
		cpu_.pc = low;
		return true;
	}
	if (ld_mask(wMultiBlockParam, 0xf8)) {
		multi_or(n);
		cpu_.pc = high;
		return true;
	}
	return false;
}

// broken by Wario in the transformation (a snowball, or the big one, for
// the snowman); n = 3 goes on into the or
void BlockFuncs::transformation_multi(uint8_t n, uint8_t transformation, Progress progress)
{
	if (multi_start(FREE, n, ADDR_Func_18f32))
		return;
	if (!wario_flag())
		GOTO(SOLID);
	ld_cp(wTransformation, transformation);
	if (!zf())
		GOTO(SOLID);
	if (progress != Progress::ANY) {
		cpu_.a = mem_[wTransformationProgress];
		if (progress == Progress::SNOWBALL) {
			cp_a(1);
			if (cf())
				GOTO(SOLID);
		} else {
			cp_a(3);
			if (!zf())
				GOTO(SOLID);
		}
	}
	st(wMultiBlockParam, n);
	if (n == 3)
		multi_or(3);
	GOTO(ADDR_Func_18f32);
}

// the robo mouse passes through (free) or breaks it (n, then broken): true
// if it jumped
bool BlockFuncs::multi_robo_mouse(uint8_t n, uint16_t broken)
{
	cpu_.a = mem_[wBlockFuncSpecialCollision];
	cp_a(COLLISION_ROBO_MOUSE_FREE);
	if (zf()) {
		cpu_.pc = FREE;
		return true;
	}
	cp_a(COLLISION_ROBO_MOUSE_BREAK);
	if (zf()) {
		st(wMultiBlockParam, n);
		cpu_.pc = broken;
		return true;
	}
	return false;
}

// a cracked multi-block
void BlockFuncs::cracked_multi(uint8_t n)
{
	if (multi_start(FREE, n, ADDR_Func_18f32))
		return;
	if (multi_robo_mouse(n, ADDR_Func_18f32))
		return;
	if (!ld_and(wBlockFuncBreakFlag))
		GOTO(SOLID);
	bool brk;
	if (coll_bit(COLLISION_UNK4_F)) {
		brk = ld_and(wAttackCounter) || ld_and(wIsRolling);
	} else if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F)) {
		ld_cp(wPowerUpLevel, HEAD_SMASH_HELMET);
		if (cf())
			GOTO(SOLID);
		brk = ld_and(wJumpVelTable) || ld_and(wSwimVelIndex);
	} else {
		and_a(COLLISION_UNK1 | COLLISION_UNK6);
		brk = !zf();
	}
	if (!brk)
		GOTO(SOLID);
	st(wMultiBlockParam, n);
	GOTO(ADDR_Func_19086);
}

// a non-cracked multi-block: garlic (and the super jump slam overalls for a
// ground pound), or Fat Wario's ground pound
void BlockFuncs::noncracked_multi(uint8_t n)
{
	if (multi_start(SOLID, n, ADDR_Func_19246))
		return;
	if (multi_robo_mouse(n, ADDR_Func_19246))
		return;
	if (!ld_and(wBlockFuncBreakFlag))
		GOTO(SOLID);
	ld_cp(wTransformation, TRANSFORMATION_FAT_WARIO);
	bool overalls; // needs the super jump slam overalls
	if (zf()) {
		if (!ld_mask(wCollisionPoints, COLLISION_UNK1 | COLLISION_UNK6))
			GOTO(SOLID);
		overalls = false;
	} else {
		ld_cp(wPowerUpLevel, GARLIC);
		if (cf())
			GOTO(SOLID);
		if (coll_bit(COLLISION_UNK4_F)) {
			if (ld_and(wAttackCounter))
				overalls = false;
			else if (ld_and(wIsRolling))
				overalls = n == 1; // only Func_190fe checks them when rolling
			else
				GOTO(SOLID);
		} else if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F)) {
			ld_cp(wPowerUpLevel, HEAD_SMASH_HELMET);
			if (cf())
				GOTO(SOLID);
			if (!ld_and(wJumpVelTable) && !ld_and(wSwimVelIndex))
				GOTO(SOLID);
			overalls = false;
		} else {
			and_a(COLLISION_UNK1 | COLLISION_UNK6);
			if (zf())
				GOTO(SOLID);
			overalls = true;
		}
	}
	if (overalls) {
		ld_cp(wPowerUpLevel, SUPER_JUMP_SLAM_OVERALLS);
		if (cf())
			GOTO(SOLID);
	}
	st(wMultiBlockParam, n);
	GOTO(ADDR_Func_193dc);
}

// broken by Ball o' String Wario
void BlockFuncs::ball_o_string_block()
{
	if (!wario_flag())
		GOTO(SOLID);
	ld_cp(wTransformation, TRANSFORMATION_BALL_O_STRING_WARIO);
	GOTO(zf() ? ADDR_Func_19481 : SOLID);
}

#undef GOTO

} // namespace

} // namespace wl3::level

using wl3::level::BlockFuncs;
using wl3::level::Progress;

// Each ends where the asm does (HOOK_MANUAL_RET): a jump on to another
// block function, or a ret.
PORT_EX(ProcessBlock, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).process_block(); }
PORT(BlockFunc_Free) { BlockFuncs(gb).free(); }
PORT(BlockFunc_Solid) { BlockFuncs(gb).solid(); }
PORT_EX(BlockFunc_Water, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).water(); }
PORT_EX(BlockFunc_RightCurrent, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).block_func_current(CURRENT_RIGHT); }
PORT_EX(BlockFunc_UpCurrent, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).block_func_current(CURRENT_UP); }
PORT_EX(BlockFunc_LeftCurrent, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).block_func_current(CURRENT_LEFT); }
PORT_EX(BlockFunc_DownCurrent, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).block_func_current(CURRENT_DOWN); }
PORT_EX(BlockFunc_Floor, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).floor(); }
PORT(Func_1808c) { BlockFuncs(gb).free_1808c(); }
PORT_EX(Func_182ec, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ladder_or_floor(); }
PORT_EX(EnterDoor, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).enter_door(); }
PORT(Func_18064) { BlockFuncs(gb).block_func_c18d(0x10); }
PORT(Func_18078) { BlockFuncs(gb).block_func_c18d(0x01); }
PORT_EX(BlockFunc_180b5, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).slope_above(ADDR_RightSlopeCollision); }
PORT_EX(BlockFunc_1813f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).slope_above(ADDR_LeftSlopeCollision); }
PORT_EX(BlockFunc_1809f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).slope_below(ADDR_RightSlopeCollision); }
PORT_EX(BlockFunc_18129, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).slope_below(ADDR_LeftSlopeCollision); }
PORT_EX(RightSlopeCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).right_slope_collision(); }
PORT_EX(LeftSlopeCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).left_slope_collision(); }
PORT_EX(BlockFunc_RightSlope, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).right_slope(); }
PORT_EX(BlockFunc_LeftSlope, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).left_slope(); }
PORT_EX(BlockFunc_WaterSurface, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).water_surface(); }
PORT_EX(BlockFunc_18262, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).object_floor(); }
PORT_EX(BlockFunc_Ladder, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ladder(); }
PORT_EX(BlockFunc_182dc, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).wario_ladder_else(wl3::level::SOLID); }
PORT_EX(BlockFunc_LadderTop, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).wario_ladder_else(wl3::level::FLOOR); }
PORT_EX(BlockFunc_182fe, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ladder_transition(ROOMTRANSITION_5 | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_EX(Func_1831a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ladder_transition(ROOMTRANSITION_5); }
PORT_EX(BlockFunc_NonSolidSpike, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).spike(wl3::level::FREE); }
PORT_EX(BlockFunc_SolidSpike, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).spike(wl3::level::SOLID); }
PORT_EX(BlockFunc_WaterSpike, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).water_spike(); }
PORT_EX(EnterPipeDown, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).enter_pipe(true); }
PORT_EX(EnterPipeUp, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).enter_pipe(false); }
PORT_EX(BlockFunc_DownPipe_Left, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).pipe(WST_IDLING, WST_GRAB_IDLING, COLLISION_DOWN, B_PAD_DOWN, true); }
PORT_EX(BlockFunc_DownPipe_Right, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).pipe(WST_IDLING, WST_GRAB_IDLING, COLLISION_DOWN, B_PAD_DOWN, false); }
PORT_EX(BlockFunc_UpPipe_Left, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).pipe(WST_AIRBORNE, WST_GRAB_AIRBORNE, COLLISION_UNK5, B_PAD_UP, true); }
PORT_EX(BlockFunc_UpPipe_Right, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).pipe(WST_AIRBORNE, WST_GRAB_AIRBORNE, COLLISION_UNK5, B_PAD_UP, false); }
PORT_EX(BlockFunc_Door, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).door(true, ROOMTRANSITION_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); }
PORT_EX(BlockFunc_MinigameDoor, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).door(false, ROOMTRANSITION_GOLF | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); }
PORT_EX(Func_18624, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).door(false, ROOMTRANSITION_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_DELAY); }
PORT_EX(Func_1864e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).door(false, ROOMTRANSITION_GOLF | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_DELAY); }
PORT_EX(BlockFunc_UnderwaterDoor, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).underwater_door(true, ROOMTRANSITION_UNDERWATER_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_EX(BlockFunc_18727, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).underwater_door(true, ROOMTRANSITION_UNDERWATER_DOOR | ROOMTRANSITIONF_DOOR); }
PORT_EX(Func_18755, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).underwater_door(false, ROOMTRANSITION_8 | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); }
PORT_EX(BlockFunc_TreasureDoor, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).treasure_door(); }
PORT_EX(BlockFunc_187b0, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transition_block(wl3::level::FREE, ROOMTRANSITION_4 | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_EX(Func_187c7, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transition_block(wl3::level::FREE, ROOMTRANSITION_4); }
PORT_EX(BlockFunc_187de, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transition_block(wl3::level::WATER, ROOMTRANSITION_4 | ROOMTRANSITIONF_RELOAD_OBJECTS); }
PORT_EX(Func_187f5, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transition_block(wl3::level::WATER, ROOMTRANSITION_4); }
PORT_EX(BlockFunc_WalkthroughPipe, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).walkthrough_pipe(); }
PORT_EX(BlockFunc_LightSource, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).light_source(); }
PORT_EX(BlockFunc_Sand, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).in_block(wIsInSand, TRUE); }
PORT_EX(BlockFunc_NonSubmersibleWater, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).in_block(wWaterInteraction, NON_SUBMERSIBLE_WATER); }
PORT_EX(BlockFunc_18868, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).vampire_block(); }
PORT_EX(BlockFunc_188ba, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).wario_or_else(wl3::level::FREE, wl3::level::SOLID); }
PORT_EX(BlockFunc_189f1, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).wario_or_else(wl3::level::FLOOR, wl3::level::FREE); }
PORT_EX(BlockFunc_Fire, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).fire(); }
PORT_EX(BlockFunc_Fence, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).fence(); }
PORT_EX(BlockFunc_AirCurrent, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).air_current(); }
PORT_EX(BlockFunc_SlipperySolid, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).slippery(wl3::level::SOLID); }
PORT_EX(BlockFunc_SlipperyFloor, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).slippery(wl3::level::FLOOR); }
PORT_EX(BlockFunc_CloudPlatform1, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cloud_platform_1(); }
PORT_EX(BlockFunc_CloudPlatform2, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cloud_platform_2(); }
PORT_EX(BlockFunc_SmallGap, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).small_gap(); }
PORT_EX(BlockFunc_Switch, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).switch_block(); }
PORT(BreakBlock) { BlockFuncs(gb).break_block(true, ADDR_ObjParams_Coin); }
PORT(BreakBlockWithColourCoin) { BlockFuncs(gb).break_block(false, ADDR_ObjParams_ColourCoin); }
PORT_EX(Func_18f32, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).saved_break(false, wl3::level::FREE); }
PORT_EX(Func_19246, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).saved_break(false, wl3::level::FREE); }
PORT_EX(Func_19481, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).break_free(); }
PORT_EX(Func_18df1, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).break_free_if_rolling(); }
PORT_EX(Func_19086, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).break_multi(); }
PORT_EX(Func_19423, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).break_by_object(); }
PORT_EX(DoughnutBlockCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).doughnut_block(); }
PORT_EX(CrackedBlockWithColourCoinCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_block(ADDR_BreakBlockWithColourCoin); }
PORT_EX(CrackedBlockCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked(); }
PORT_EX(NonCrackedBlockCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).non_cracked(); }
PORT_EX(EnemyBlockCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).enemy_block(); }
PORT_EX(BonfireBlockCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).bonfire_block(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x78); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x79); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x7a); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x7b); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x7c); }
PORT_EX(Func_18a0f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x7d); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x7e); }
PORT_EX(Func_18a17, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_colour_coin(0x7f); }
PORT_EX(Func_18aa8, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(1, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); }
PORT_EX(Func_18aec, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(3, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); }
PORT_EX(Func_18b2d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(2, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); }
PORT_EX(Func_18b71, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(4, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); }
PORT_EX(Func_18bb5, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(1, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); }
PORT_EX(Func_18bf9, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(3, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); }
PORT_EX(Func_18c3a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(2, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); }
PORT_EX(Func_18c7e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(4, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); }
PORT_EX(Func_19522, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(1, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); }
PORT_EX(Func_19560, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(3, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); }
PORT_EX(Func_1959b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(2, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); }
PORT_EX(YarnBlockCollision, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).transformation_multi(4, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); }
PORT_EX(Func_18e2c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_multi(1); }
PORT_EX(Func_18ec0, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_multi(3); }
PORT_EX(Func_18f7d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_multi(2); }
PORT_EX(Func_19011, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).cracked_multi(4); }
PORT_EX(Func_190fe, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).noncracked_multi(1); }
PORT_EX(Func_191b3, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).noncracked_multi(3); }
PORT_EX(Func_19291, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).noncracked_multi(2); }
PORT_EX(Func_19346, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).noncracked_multi(4); }
PORT_EX(Func_193dc, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).saved_break(true, ADDR_Func_18df1); }
PORT_EX(Func_19502, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ball_o_string_block(); }
PORT_EX(Func_18a9a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18aa8); }
PORT_EX(Func_18a9e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18aa8); }
PORT_EX(Func_18aa2, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18aa8); }
PORT_EX(BlockFunc_18aa6, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18aa8); }
PORT_EX(Func_18ade, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18aec); }
PORT_EX(Func_18ae2, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18aec); }
PORT_EX(Func_18ae6, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18aec); }
PORT_EX(BlockFunc_18aea, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18aec); }
PORT_EX(BlockFunc_18b23, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18b2d); }
PORT_EX(BlockFunc_18b2b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18b2d); }
PORT_EX(BlockFunc_18b67, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18b71); }
PORT_EX(BlockFunc_18b6f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18b71); }
PORT_EX(BlockFunc_18bb3, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18bb5); }
PORT_EX(BlockFunc_18bf7, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18bf9); }
PORT_EX(BlockFunc_18c30, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18c3a); }
PORT_EX(BlockFunc_18c38, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18c3a); }
PORT_EX(BlockFunc_18c74, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18c7e); }
PORT_EX(BlockFunc_18c7c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18c7e); }
PORT_EX(BlockFunc_CrackedBlock_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_7a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_7c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_7d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_7e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_CrackedBlock_7f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_CrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_7a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_7c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_7d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_7e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_NonCrackedBlock_7f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_NonCrackedBlockCollision); }
PORT_EX(BlockFunc_18e0e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18e2c); }
PORT_EX(BlockFunc_18e1a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_18e2c); }
PORT_EX(BlockFunc_18e26, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_18e2c); }
PORT_EX(BlockFunc_18e2a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18e2c); }
PORT_EX(BlockFunc_18ea2, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18ec0); }
PORT_EX(BlockFunc_18eae, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_18ec0); }
PORT_EX(BlockFunc_18ebe, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18ec0); }
PORT_EX(BlockFunc_18f5f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18f7d); }
PORT_EX(BlockFunc_18f6b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_18f7d); }
PORT_EX(BlockFunc_18f7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_18f7d); }
PORT_EX(BlockFunc_18ff3, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_19011); }
PORT_EX(BlockFunc_18fff, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_19011); }
PORT_EX(BlockFunc_19007, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_19011); }
PORT_EX(BlockFunc_1900f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_19011); }
PORT_EX(BlockFunc_190e4, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_190fe); }
PORT_EX(BlockFunc_190e8, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_190fe); }
PORT_EX(BlockFunc_190fc, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_190fe); }
PORT_EX(BlockFunc_19199, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_191b3); }
PORT_EX(BlockFunc_1919d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_191b3); }
PORT_EX(BlockFunc_191b1, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_191b3); }
PORT_EX(BlockFunc_19277, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_19291); }
PORT_EX(BlockFunc_1927b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_19291); }
PORT_EX(BlockFunc_1928f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_19291); }
PORT_EX(BlockFunc_1932c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_19346); }
PORT_EX(BlockFunc_19330, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_19346); }
PORT_EX(BlockFunc_19344, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_19346); }
PORT_EX(BlockFunc_EnemyBlock_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_7a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_7c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_7d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_7e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_EnemyBlock_7f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_EnemyBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_7a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_7c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_7d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_BonfireBlock_7e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_BonfireBlockCollision); }
PORT_EX(BlockFunc_DoughnutBlock_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_DoughnutBlockCollision); }
PORT_EX(BlockFunc_DoughnutBlock_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_DoughnutBlockCollision); }
PORT_EX(BlockFunc_DoughnutBlock_7a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_DoughnutBlockCollision); }
PORT_EX(BlockFunc_DoughnutBlock_7b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_DoughnutBlockCollision); }
PORT_EX(BlockFunc_19518, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_19522); }
PORT_EX(BlockFunc_19520, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_19522); }
PORT_EX(BlockFunc_19552, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_19560); }
PORT_EX(BlockFunc_19556, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_19560); }
PORT_EX(BlockFunc_1955e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_19560); }
PORT_EX(BlockFunc_1958d, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_1959b); }
PORT_EX(BlockFunc_19599, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_1959b); }
PORT_EX(BlockFunc_YarnBlock_79, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_YarnBlockCollision); }
PORT_EX(BlockFunc_YarnBlock_78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_YarnBlockCollision); }
PORT_EX(Func_19467, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_BonfireBlockCollision); }
PORT_EX(Func_194a7, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_DoughnutBlockCollision); }
PORT_EX(Func_194ab, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_DoughnutBlockCollision); }
PORT_EX(Func_194af, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_DoughnutBlockCollision); }
PORT_EX(Func_194b3, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_DoughnutBlockCollision); }
PORT_EX(Func_194e4, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x78, ADDR_Func_19502); }
PORT_EX(Func_194e8, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_19502); }
PORT_EX(Func_194ec, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_19502); }
PORT_EX(Func_194f0, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_19502); }
PORT_EX(Func_194f4, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_19502); }
PORT_EX(Func_194f8, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_19502); }
PORT_EX(Func_194fc, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_19502); }
PORT_EX(Func_19500, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_19502); }
PORT_EX(Func_195cf, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_YarnBlockCollision); }
PORT_EX(Func_195d3, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_YarnBlockCollision); }
PORT_EX(Func_19514, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_19522); }
PORT_EX(Func_1951c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_19522); }
PORT_EX(Func_1955a, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_19560); }
PORT_EX(Func_19591, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_1959b); }
PORT_EX(Func_19595, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_1959b); }
PORT_EX(Func_19328, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_19346); }
PORT_EX(Func_19334, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_19346); }
PORT_EX(Func_19338, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_19346); }
PORT_EX(Func_1933c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_19346); }
PORT_EX(Func_19340, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_19346); }
PORT_EX(Func_19273, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_19291); }
PORT_EX(Func_1927f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_19291); }
PORT_EX(Func_19283, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_19291); }
PORT_EX(Func_19287, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_19291); }
PORT_EX(Func_1928b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_19291); }
PORT_EX(Func_19195, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_191b3); }
PORT_EX(Func_191a1, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_191b3); }
PORT_EX(Func_191a5, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_191b3); }
PORT_EX(Func_191a9, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_191b3); }
PORT_EX(Func_191ad, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_191b3); }
PORT_EX(Func_18b1f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18b2d); }
PORT_EX(Func_18b27, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18b2d); }
PORT_EX(Func_18b63, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18b71); }
PORT_EX(Func_18b6b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18b71); }
PORT_EX(Func_18ba7, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18bb5); }
PORT_EX(Func_18bab, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18bb5); }
PORT_EX(Func_18baf, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18bb5); }
PORT_EX(Func_18beb, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18bf9); }
PORT_EX(Func_18bef, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18bf9); }
PORT_EX(Func_18bf3, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18bf9); }
PORT_EX(Func_18c2c, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18c3a); }
PORT_EX(Func_18c34, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18c3a); }
PORT_EX(Func_18c70, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_18c7e); }
PORT_EX(Func_18c78, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18c7e); }
PORT_EX(Func_18e12, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18e2c); }
PORT_EX(Func_18e16, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18e2c); }
PORT_EX(Func_18e1e, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_18e2c); }
PORT_EX(Func_18e22, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_18e2c); }
PORT_EX(Func_18ea6, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18ec0); }
PORT_EX(Func_18eaa, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18ec0); }
PORT_EX(Func_18eb2, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_18ec0); }
PORT_EX(Func_18eb6, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_18ec0); }
PORT_EX(Func_18eba, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_18ec0); }
PORT_EX(Func_18f63, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_18f7d); }
PORT_EX(Func_18f67, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_18f7d); }
PORT_EX(Func_18f6f, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_18f7d); }
PORT_EX(Func_18f73, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_18f7d); }
PORT_EX(Func_18f77, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_18f7d); }
PORT_EX(Func_18ff7, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7a, ADDR_Func_19011); }
PORT_EX(Func_18ffb, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7b, ADDR_Func_19011); }
PORT_EX(Func_19003, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_19011); }
PORT_EX(Func_1900b, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_19011); }
PORT_EX(Func_190e0, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x79, ADDR_Func_190fe); }
PORT_EX(Func_190ec, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7c, ADDR_Func_190fe); }
PORT_EX(Func_190f0, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7d, ADDR_Func_190fe); }
PORT_EX(Func_190f4, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7e, ADDR_Func_190fe); }
PORT_EX(Func_190f8, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).ldb_jump(0x7f, ADDR_Func_190fe); }
