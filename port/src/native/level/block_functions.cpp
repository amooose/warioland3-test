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

// a jump on (jp / jr) to the block function at addr, and out of this one
#define GOTO(addr) \
	do { \
		next_ = (addr); \
		return; \
	} while (0)

enum class Progress { ANY, SNOWBALL, BIG_SNOWBALL };

class BlockFuncs {
public:
	explicit BlockFuncs(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void process_block(); // ProcessBlock
	// The block function at func, by address: false if there's none there.
	// It leaves in next_ the one it jumps on to (0: it returned).
	bool dispatch(uint16_t func);
	// func and the ones it jumps on to, to the ret
	void run(uint16_t func);
	// A hook's end: on to next_ (the asm's jp), or the ret
	void jump_on();
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
	// big-endian words (hPos, wWarioXPos, wBlockPtr)
	uint16_t word_be(uint16_t addr) const { return static_cast<uint16_t>(mem_[addr] << 8 | mem_[addr + 1]); }
	void set_word_be(uint16_t addr, uint16_t v)
	{
		mem_[addr] = static_cast<uint8_t>(v >> 8);
		mem_[addr + 1] = static_cast<uint8_t>(v);
	}
	bool wario_flag() const { return mem_[wBlockFuncWarioFlag]; }
	bool coll_bit(int n) const { return mem_[wCollisionPoints] & (1 << n); }
	void sfx(uint16_t id) { play_sfx(mem_, id); }
	// ld a, HIGH(ptr) / ld [addr + 0], a / ld a, LOW(ptr) / ld [addr + 1], a
	void st_ptr(uint16_t addr, uint16_t ptr)
	{
		mem_[addr + 0] = static_cast<uint8_t>(ptr >> 8);
		st(addr + 1, static_cast<uint8_t>(ptr));
	}
	void clear(std::initializer_list<uint16_t> addrs)
	{
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
	bool breaks_cracked() const;
	bool breaks(bool head_smash_reads_a);
	void multi_or(uint8_t base, uint8_t n);
	bool multi_start(uint16_t low, uint8_t n, uint16_t high);
	bool multi_robo_mouse(uint8_t n, uint16_t broken);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
	uint16_t next_ = 0;
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
	// The levels' tables (data/levels/function_tables*.asm) hold only
	// this file's block functions: a fuzzed table pointing anywhere else
	// (another routine's label) isn't comparable.
	if (gb_->test.active && (func < ADDR_ProcessBlock || func > ADDR_BreakBlockWithColourCoin
		|| !sym_is_rom_label(BANK_ProcessBlock, func)))
		gb_->test.unrealistic = true;
	// jp hl: the block functions run here when their bank is mapped (the
	// asm's jp runs whatever is), and the last one's ret is ProcessBlock's
	if (gb_->rom_bank == BANK_ProcessBlock && dispatch(func)) {
		run(next_);
		cpu_.pc = gb_pop16(gb_);
		return;
	}
	cpu_.pc = func;
}

void BlockFuncs::run(uint16_t func)
{
	while (func) {
		dispatch(func);
		func = next_;
	}
}

void BlockFuncs::jump_on()
{
	cpu_.pc = next_ ? next_ : gb_pop16(gb_);
}

bool BlockFuncs::dispatch(uint16_t func)
{
	next_ = 0;
	switch (func) {
	case ADDR_BlockFunc_Free: free(); break;
	case ADDR_BlockFunc_Solid: solid(); break;
	case ADDR_BlockFunc_Water: water(); break;
	case ADDR_BlockFunc_RightCurrent: block_func_current(CURRENT_RIGHT); break;
	case ADDR_BlockFunc_UpCurrent: block_func_current(CURRENT_UP); break;
	case ADDR_BlockFunc_LeftCurrent: block_func_current(CURRENT_LEFT); break;
	case ADDR_BlockFunc_DownCurrent: block_func_current(CURRENT_DOWN); break;
	case ADDR_BlockFunc_Floor: floor(); break;
	case ADDR_Func_1808c: free_1808c(); break;
	case ADDR_Func_182ec: ladder_or_floor(); break;
	case ADDR_EnterDoor: enter_door(); break;
	case ADDR_Func_18064: block_func_c18d(0x10); break;
	case ADDR_Func_18078: block_func_c18d(0x01); break;
	case ADDR_BlockFunc_180b5: slope_above(ADDR_RightSlopeCollision); break;
	case ADDR_BlockFunc_1813f: slope_above(ADDR_LeftSlopeCollision); break;
	case ADDR_BlockFunc_1809f: slope_below(ADDR_RightSlopeCollision); break;
	case ADDR_BlockFunc_18129: slope_below(ADDR_LeftSlopeCollision); break;
	case ADDR_RightSlopeCollision: right_slope_collision(); break;
	case ADDR_LeftSlopeCollision: left_slope_collision(); break;
	case ADDR_BlockFunc_RightSlope: right_slope(); break;
	case ADDR_BlockFunc_LeftSlope: left_slope(); break;
	case ADDR_BlockFunc_WaterSurface: water_surface(); break;
	case ADDR_BlockFunc_18262: object_floor(); break;
	case ADDR_BlockFunc_Ladder: ladder(); break;
	case ADDR_BlockFunc_182dc: wario_ladder_else(SOLID); break;
	case ADDR_BlockFunc_LadderTop: wario_ladder_else(FLOOR); break;
	case ADDR_BlockFunc_182fe: ladder_transition(ROOMTRANSITION_5 | ROOMTRANSITIONF_RELOAD_OBJECTS); break;
	case ADDR_Func_1831a: ladder_transition(ROOMTRANSITION_5); break;
	case ADDR_BlockFunc_NonSolidSpike: spike(FREE); break;
	case ADDR_BlockFunc_SolidSpike: spike(SOLID); break;
	case ADDR_BlockFunc_WaterSpike: water_spike(); break;
	case ADDR_EnterPipeDown: enter_pipe(true); break;
	case ADDR_EnterPipeUp: enter_pipe(false); break;
	case ADDR_BlockFunc_DownPipe_Left: pipe(WST_IDLING, WST_GRAB_IDLING, COLLISION_DOWN, B_PAD_DOWN, true); break;
	case ADDR_BlockFunc_DownPipe_Right: pipe(WST_IDLING, WST_GRAB_IDLING, COLLISION_DOWN, B_PAD_DOWN, false); break;
	case ADDR_BlockFunc_UpPipe_Left: pipe(WST_AIRBORNE, WST_GRAB_AIRBORNE, COLLISION_UNK5, B_PAD_UP, true); break;
	case ADDR_BlockFunc_UpPipe_Right: pipe(WST_AIRBORNE, WST_GRAB_AIRBORNE, COLLISION_UNK5, B_PAD_UP, false); break;
	case ADDR_BlockFunc_Door: door(true, ROOMTRANSITION_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); break;
	case ADDR_BlockFunc_MinigameDoor: door(false, ROOMTRANSITION_GOLF | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); break;
	case ADDR_Func_18624: door(false, ROOMTRANSITION_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_DELAY); break;
	case ADDR_Func_1864e: door(false, ROOMTRANSITION_GOLF | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_DELAY); break;
	case ADDR_BlockFunc_UnderwaterDoor: underwater_door(true, ROOMTRANSITION_UNDERWATER_DOOR | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS); break;
	case ADDR_BlockFunc_18727: underwater_door(true, ROOMTRANSITION_UNDERWATER_DOOR | ROOMTRANSITIONF_DOOR); break;
	case ADDR_Func_18755: underwater_door(false, ROOMTRANSITION_8 | ROOMTRANSITIONF_DOOR | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY); break;
	case ADDR_BlockFunc_TreasureDoor: treasure_door(); break;
	case ADDR_BlockFunc_187b0: transition_block(FREE, ROOMTRANSITION_4 | ROOMTRANSITIONF_RELOAD_OBJECTS); break;
	case ADDR_Func_187c7: transition_block(FREE, ROOMTRANSITION_4); break;
	case ADDR_BlockFunc_187de: transition_block(WATER, ROOMTRANSITION_4 | ROOMTRANSITIONF_RELOAD_OBJECTS); break;
	case ADDR_Func_187f5: transition_block(WATER, ROOMTRANSITION_4); break;
	case ADDR_BlockFunc_WalkthroughPipe: walkthrough_pipe(); break;
	case ADDR_BlockFunc_LightSource: light_source(); break;
	case ADDR_BlockFunc_Sand: in_block(wIsInSand, TRUE); break;
	case ADDR_BlockFunc_NonSubmersibleWater: in_block(wWaterInteraction, NON_SUBMERSIBLE_WATER); break;
	case ADDR_BlockFunc_18868: vampire_block(); break;
	case ADDR_BlockFunc_188ba: wario_or_else(FREE, SOLID); break;
	case ADDR_BlockFunc_189f1: wario_or_else(FLOOR, FREE); break;
	case ADDR_BlockFunc_Fire: fire(); break;
	case ADDR_BlockFunc_Fence: fence(); break;
	case ADDR_BlockFunc_AirCurrent: air_current(); break;
	case ADDR_BlockFunc_SlipperySolid: slippery(SOLID); break;
	case ADDR_BlockFunc_SlipperyFloor: slippery(FLOOR); break;
	case ADDR_BlockFunc_CloudPlatform1: cloud_platform_1(); break;
	case ADDR_BlockFunc_CloudPlatform2: cloud_platform_2(); break;
	case ADDR_BlockFunc_SmallGap: small_gap(); break;
	case ADDR_BlockFunc_Switch: switch_block(); break;
	case ADDR_BreakBlock: break_block(true, ADDR_ObjParams_Coin); break;
	case ADDR_BreakBlockWithColourCoin: break_block(false, ADDR_ObjParams_ColourCoin); break;
	case ADDR_Func_18f32: saved_break(false, FREE); break;
	case ADDR_Func_19246: saved_break(false, FREE); break;
	case ADDR_Func_19481: break_free(); break;
	case ADDR_Func_18df1: break_free_if_rolling(); break;
	case ADDR_Func_19086: break_multi(); break;
	case ADDR_Func_19423: break_by_object(); break;
	case ADDR_DoughnutBlockCollision: doughnut_block(); break;
	case ADDR_CrackedBlockWithColourCoinCollision: cracked_block(ADDR_BreakBlockWithColourCoin); break;
	case ADDR_CrackedBlockCollision: cracked(); break;
	case ADDR_NonCrackedBlockCollision: non_cracked(); break;
	case ADDR_EnemyBlockCollision: enemy_block(); break;
	case ADDR_BonfireBlockCollision: bonfire_block(); break;
	case ADDR_BlockFunc_CrackedBlockWithColourCoin_78: cracked_colour_coin(0x78); break;
	case ADDR_BlockFunc_CrackedBlockWithColourCoin_79: cracked_colour_coin(0x79); break;
	case ADDR_BlockFunc_CrackedBlockWithColourCoin_7a: cracked_colour_coin(0x7a); break;
	case ADDR_BlockFunc_CrackedBlockWithColourCoin_7b: cracked_colour_coin(0x7b); break;
	case ADDR_BlockFunc_CrackedBlockWithColourCoin_7c: cracked_colour_coin(0x7c); break;
	case ADDR_Func_18a0f: cracked_colour_coin(0x7d); break;
	case ADDR_BlockFunc_CrackedBlockWithColourCoin_7e: cracked_colour_coin(0x7e); break;
	case ADDR_Func_18a17: cracked_colour_coin(0x7f); break;
	case ADDR_Func_18aa8: transformation_multi(1, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); break;
	case ADDR_Func_18aec: transformation_multi(3, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); break;
	case ADDR_Func_18b2d: transformation_multi(2, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); break;
	case ADDR_Func_18b71: transformation_multi(4, TRANSFORMATION_SNOWMAN_WARIO, Progress::SNOWBALL); break;
	case ADDR_Func_18bb5: transformation_multi(1, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); break;
	case ADDR_Func_18bf9: transformation_multi(3, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); break;
	case ADDR_Func_18c3a: transformation_multi(2, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); break;
	case ADDR_Func_18c7e: transformation_multi(4, TRANSFORMATION_SNOWMAN_WARIO, Progress::BIG_SNOWBALL); break;
	case ADDR_Func_19522: transformation_multi(1, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); break;
	case ADDR_Func_19560: transformation_multi(3, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); break;
	case ADDR_Func_1959b: transformation_multi(2, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); break;
	case ADDR_YarnBlockCollision: transformation_multi(4, TRANSFORMATION_BALL_O_STRING_WARIO, Progress::ANY); break;
	case ADDR_Func_18e2c: cracked_multi(1); break;
	case ADDR_Func_18ec0: cracked_multi(3); break;
	case ADDR_Func_18f7d: cracked_multi(2); break;
	case ADDR_Func_19011: cracked_multi(4); break;
	case ADDR_Func_190fe: noncracked_multi(1); break;
	case ADDR_Func_191b3: noncracked_multi(3); break;
	case ADDR_Func_19291: noncracked_multi(2); break;
	case ADDR_Func_19346: noncracked_multi(4); break;
	case ADDR_Func_193dc: saved_break(true, ADDR_Func_18df1); break;
	case ADDR_Func_19502: ball_o_string_block(); break;
	case ADDR_Func_18a9a: ldb_jump(0x79, ADDR_Func_18aa8); break;
	case ADDR_Func_18a9e: ldb_jump(0x7a, ADDR_Func_18aa8); break;
	case ADDR_Func_18aa2: ldb_jump(0x7b, ADDR_Func_18aa8); break;
	case ADDR_BlockFunc_18aa6: ldb_jump(0x78, ADDR_Func_18aa8); break;
	case ADDR_Func_18ade: ldb_jump(0x79, ADDR_Func_18aec); break;
	case ADDR_Func_18ae2: ldb_jump(0x7a, ADDR_Func_18aec); break;
	case ADDR_Func_18ae6: ldb_jump(0x7b, ADDR_Func_18aec); break;
	case ADDR_BlockFunc_18aea: ldb_jump(0x78, ADDR_Func_18aec); break;
	case ADDR_BlockFunc_18b23: ldb_jump(0x7a, ADDR_Func_18b2d); break;
	case ADDR_BlockFunc_18b2b: ldb_jump(0x78, ADDR_Func_18b2d); break;
	case ADDR_BlockFunc_18b67: ldb_jump(0x7a, ADDR_Func_18b71); break;
	case ADDR_BlockFunc_18b6f: ldb_jump(0x78, ADDR_Func_18b71); break;
	case ADDR_BlockFunc_18bb3: ldb_jump(0x78, ADDR_Func_18bb5); break;
	case ADDR_BlockFunc_18bf7: ldb_jump(0x78, ADDR_Func_18bf9); break;
	case ADDR_BlockFunc_18c30: ldb_jump(0x7a, ADDR_Func_18c3a); break;
	case ADDR_BlockFunc_18c38: ldb_jump(0x78, ADDR_Func_18c3a); break;
	case ADDR_BlockFunc_18c74: ldb_jump(0x7a, ADDR_Func_18c7e); break;
	case ADDR_BlockFunc_18c7c: ldb_jump(0x78, ADDR_Func_18c7e); break;
	case ADDR_BlockFunc_CrackedBlock_78: ldb_jump(0x78, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_79: ldb_jump(0x79, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_7a: ldb_jump(0x7a, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_7b: ldb_jump(0x7b, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_7c: ldb_jump(0x7c, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_7d: ldb_jump(0x7d, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_7e: ldb_jump(0x7e, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_CrackedBlock_7f: ldb_jump(0x7f, ADDR_CrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_78: ldb_jump(0x78, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_79: ldb_jump(0x79, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_7a: ldb_jump(0x7a, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_7b: ldb_jump(0x7b, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_7c: ldb_jump(0x7c, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_7d: ldb_jump(0x7d, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_7e: ldb_jump(0x7e, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_NonCrackedBlock_7f: ldb_jump(0x7f, ADDR_NonCrackedBlockCollision); break;
	case ADDR_BlockFunc_18e0e: ldb_jump(0x79, ADDR_Func_18e2c); break;
	case ADDR_BlockFunc_18e1a: ldb_jump(0x7c, ADDR_Func_18e2c); break;
	case ADDR_BlockFunc_18e26: ldb_jump(0x7f, ADDR_Func_18e2c); break;
	case ADDR_BlockFunc_18e2a: ldb_jump(0x78, ADDR_Func_18e2c); break;
	case ADDR_BlockFunc_18ea2: ldb_jump(0x79, ADDR_Func_18ec0); break;
	case ADDR_BlockFunc_18eae: ldb_jump(0x7c, ADDR_Func_18ec0); break;
	case ADDR_BlockFunc_18ebe: ldb_jump(0x78, ADDR_Func_18ec0); break;
	case ADDR_BlockFunc_18f5f: ldb_jump(0x79, ADDR_Func_18f7d); break;
	case ADDR_BlockFunc_18f6b: ldb_jump(0x7c, ADDR_Func_18f7d); break;
	case ADDR_BlockFunc_18f7b: ldb_jump(0x78, ADDR_Func_18f7d); break;
	case ADDR_BlockFunc_18ff3: ldb_jump(0x79, ADDR_Func_19011); break;
	case ADDR_BlockFunc_18fff: ldb_jump(0x7c, ADDR_Func_19011); break;
	case ADDR_BlockFunc_19007: ldb_jump(0x7e, ADDR_Func_19011); break;
	case ADDR_BlockFunc_1900f: ldb_jump(0x78, ADDR_Func_19011); break;
	case ADDR_BlockFunc_190e4: ldb_jump(0x7a, ADDR_Func_190fe); break;
	case ADDR_BlockFunc_190e8: ldb_jump(0x7b, ADDR_Func_190fe); break;
	case ADDR_BlockFunc_190fc: ldb_jump(0x78, ADDR_Func_190fe); break;
	case ADDR_BlockFunc_19199: ldb_jump(0x7a, ADDR_Func_191b3); break;
	case ADDR_BlockFunc_1919d: ldb_jump(0x7b, ADDR_Func_191b3); break;
	case ADDR_BlockFunc_191b1: ldb_jump(0x78, ADDR_Func_191b3); break;
	case ADDR_BlockFunc_19277: ldb_jump(0x7a, ADDR_Func_19291); break;
	case ADDR_BlockFunc_1927b: ldb_jump(0x7b, ADDR_Func_19291); break;
	case ADDR_BlockFunc_1928f: ldb_jump(0x78, ADDR_Func_19291); break;
	case ADDR_BlockFunc_1932c: ldb_jump(0x7a, ADDR_Func_19346); break;
	case ADDR_BlockFunc_19330: ldb_jump(0x7b, ADDR_Func_19346); break;
	case ADDR_BlockFunc_19344: ldb_jump(0x78, ADDR_Func_19346); break;
	case ADDR_BlockFunc_EnemyBlock_78: ldb_jump(0x78, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_79: ldb_jump(0x79, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_7a: ldb_jump(0x7a, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_7b: ldb_jump(0x7b, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_7c: ldb_jump(0x7c, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_7d: ldb_jump(0x7d, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_7e: ldb_jump(0x7e, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_EnemyBlock_7f: ldb_jump(0x7f, ADDR_EnemyBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_78: ldb_jump(0x78, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_79: ldb_jump(0x79, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_7a: ldb_jump(0x7a, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_7b: ldb_jump(0x7b, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_7c: ldb_jump(0x7c, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_7d: ldb_jump(0x7d, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_BonfireBlock_7e: ldb_jump(0x7e, ADDR_BonfireBlockCollision); break;
	case ADDR_BlockFunc_DoughnutBlock_78: ldb_jump(0x78, ADDR_DoughnutBlockCollision); break;
	case ADDR_BlockFunc_DoughnutBlock_79: ldb_jump(0x79, ADDR_DoughnutBlockCollision); break;
	case ADDR_BlockFunc_DoughnutBlock_7a: ldb_jump(0x7a, ADDR_DoughnutBlockCollision); break;
	case ADDR_BlockFunc_DoughnutBlock_7b: ldb_jump(0x7b, ADDR_DoughnutBlockCollision); break;
	case ADDR_BlockFunc_19518: ldb_jump(0x7a, ADDR_Func_19522); break;
	case ADDR_BlockFunc_19520: ldb_jump(0x78, ADDR_Func_19522); break;
	case ADDR_BlockFunc_19552: ldb_jump(0x79, ADDR_Func_19560); break;
	case ADDR_BlockFunc_19556: ldb_jump(0x7a, ADDR_Func_19560); break;
	case ADDR_BlockFunc_1955e: ldb_jump(0x78, ADDR_Func_19560); break;
	case ADDR_BlockFunc_1958d: ldb_jump(0x79, ADDR_Func_1959b); break;
	case ADDR_BlockFunc_19599: ldb_jump(0x78, ADDR_Func_1959b); break;
	case ADDR_BlockFunc_YarnBlock_79: ldb_jump(0x79, ADDR_YarnBlockCollision); break;
	case ADDR_BlockFunc_YarnBlock_78: ldb_jump(0x78, ADDR_YarnBlockCollision); break;
	case ADDR_Func_19467: ldb_jump(0x7f, ADDR_BonfireBlockCollision); break;
	case ADDR_Func_194a7: ldb_jump(0x7c, ADDR_DoughnutBlockCollision); break;
	case ADDR_Func_194ab: ldb_jump(0x7d, ADDR_DoughnutBlockCollision); break;
	case ADDR_Func_194af: ldb_jump(0x7e, ADDR_DoughnutBlockCollision); break;
	case ADDR_Func_194b3: ldb_jump(0x7f, ADDR_DoughnutBlockCollision); break;
	case ADDR_Func_194e4: ldb_jump(0x78, ADDR_Func_19502); break;
	case ADDR_Func_194e8: ldb_jump(0x79, ADDR_Func_19502); break;
	case ADDR_Func_194ec: ldb_jump(0x7a, ADDR_Func_19502); break;
	case ADDR_Func_194f0: ldb_jump(0x7b, ADDR_Func_19502); break;
	case ADDR_Func_194f4: ldb_jump(0x7c, ADDR_Func_19502); break;
	case ADDR_Func_194f8: ldb_jump(0x7d, ADDR_Func_19502); break;
	case ADDR_Func_194fc: ldb_jump(0x7e, ADDR_Func_19502); break;
	case ADDR_Func_19500: ldb_jump(0x7f, ADDR_Func_19502); break;
	case ADDR_Func_195cf: ldb_jump(0x7a, ADDR_YarnBlockCollision); break;
	case ADDR_Func_195d3: ldb_jump(0x7b, ADDR_YarnBlockCollision); break;
	case ADDR_Func_19514: ldb_jump(0x79, ADDR_Func_19522); break;
	case ADDR_Func_1951c: ldb_jump(0x7b, ADDR_Func_19522); break;
	case ADDR_Func_1955a: ldb_jump(0x7b, ADDR_Func_19560); break;
	case ADDR_Func_19591: ldb_jump(0x7a, ADDR_Func_1959b); break;
	case ADDR_Func_19595: ldb_jump(0x7b, ADDR_Func_1959b); break;
	case ADDR_Func_19328: ldb_jump(0x79, ADDR_Func_19346); break;
	case ADDR_Func_19334: ldb_jump(0x7c, ADDR_Func_19346); break;
	case ADDR_Func_19338: ldb_jump(0x7d, ADDR_Func_19346); break;
	case ADDR_Func_1933c: ldb_jump(0x7e, ADDR_Func_19346); break;
	case ADDR_Func_19340: ldb_jump(0x7f, ADDR_Func_19346); break;
	case ADDR_Func_19273: ldb_jump(0x79, ADDR_Func_19291); break;
	case ADDR_Func_1927f: ldb_jump(0x7c, ADDR_Func_19291); break;
	case ADDR_Func_19283: ldb_jump(0x7d, ADDR_Func_19291); break;
	case ADDR_Func_19287: ldb_jump(0x7e, ADDR_Func_19291); break;
	case ADDR_Func_1928b: ldb_jump(0x7f, ADDR_Func_19291); break;
	case ADDR_Func_19195: ldb_jump(0x79, ADDR_Func_191b3); break;
	case ADDR_Func_191a1: ldb_jump(0x7c, ADDR_Func_191b3); break;
	case ADDR_Func_191a5: ldb_jump(0x7d, ADDR_Func_191b3); break;
	case ADDR_Func_191a9: ldb_jump(0x7e, ADDR_Func_191b3); break;
	case ADDR_Func_191ad: ldb_jump(0x7f, ADDR_Func_191b3); break;
	case ADDR_Func_18b1f: ldb_jump(0x79, ADDR_Func_18b2d); break;
	case ADDR_Func_18b27: ldb_jump(0x7b, ADDR_Func_18b2d); break;
	case ADDR_Func_18b63: ldb_jump(0x79, ADDR_Func_18b71); break;
	case ADDR_Func_18b6b: ldb_jump(0x7b, ADDR_Func_18b71); break;
	case ADDR_Func_18ba7: ldb_jump(0x79, ADDR_Func_18bb5); break;
	case ADDR_Func_18bab: ldb_jump(0x7a, ADDR_Func_18bb5); break;
	case ADDR_Func_18baf: ldb_jump(0x7b, ADDR_Func_18bb5); break;
	case ADDR_Func_18beb: ldb_jump(0x79, ADDR_Func_18bf9); break;
	case ADDR_Func_18bef: ldb_jump(0x7a, ADDR_Func_18bf9); break;
	case ADDR_Func_18bf3: ldb_jump(0x7b, ADDR_Func_18bf9); break;
	case ADDR_Func_18c2c: ldb_jump(0x79, ADDR_Func_18c3a); break;
	case ADDR_Func_18c34: ldb_jump(0x7b, ADDR_Func_18c3a); break;
	case ADDR_Func_18c70: ldb_jump(0x79, ADDR_Func_18c7e); break;
	case ADDR_Func_18c78: ldb_jump(0x7b, ADDR_Func_18c7e); break;
	case ADDR_Func_18e12: ldb_jump(0x7a, ADDR_Func_18e2c); break;
	case ADDR_Func_18e16: ldb_jump(0x7b, ADDR_Func_18e2c); break;
	case ADDR_Func_18e1e: ldb_jump(0x7d, ADDR_Func_18e2c); break;
	case ADDR_Func_18e22: ldb_jump(0x7e, ADDR_Func_18e2c); break;
	case ADDR_Func_18ea6: ldb_jump(0x7a, ADDR_Func_18ec0); break;
	case ADDR_Func_18eaa: ldb_jump(0x7b, ADDR_Func_18ec0); break;
	case ADDR_Func_18eb2: ldb_jump(0x7d, ADDR_Func_18ec0); break;
	case ADDR_Func_18eb6: ldb_jump(0x7e, ADDR_Func_18ec0); break;
	case ADDR_Func_18eba: ldb_jump(0x7f, ADDR_Func_18ec0); break;
	case ADDR_Func_18f63: ldb_jump(0x7a, ADDR_Func_18f7d); break;
	case ADDR_Func_18f67: ldb_jump(0x7b, ADDR_Func_18f7d); break;
	case ADDR_Func_18f6f: ldb_jump(0x7d, ADDR_Func_18f7d); break;
	case ADDR_Func_18f73: ldb_jump(0x7e, ADDR_Func_18f7d); break;
	case ADDR_Func_18f77: ldb_jump(0x7f, ADDR_Func_18f7d); break;
	case ADDR_Func_18ff7: ldb_jump(0x7a, ADDR_Func_19011); break;
	case ADDR_Func_18ffb: ldb_jump(0x7b, ADDR_Func_19011); break;
	case ADDR_Func_19003: ldb_jump(0x7d, ADDR_Func_19011); break;
	case ADDR_Func_1900b: ldb_jump(0x7f, ADDR_Func_19011); break;
	case ADDR_Func_190e0: ldb_jump(0x79, ADDR_Func_190fe); break;
	case ADDR_Func_190ec: ldb_jump(0x7c, ADDR_Func_190fe); break;
	case ADDR_Func_190f0: ldb_jump(0x7d, ADDR_Func_190fe); break;
	case ADDR_Func_190f4: ldb_jump(0x7e, ADDR_Func_190fe); break;
	case ADDR_Func_190f8: ldb_jump(0x7f, ADDR_Func_190fe); break;
	default: return false;
	}
	return true;
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
	mem_[wIsInWaterOrSand] = TRUE;
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	mem_[wWaterInteraction] = SUBMERSIBLE_WATER;
	GOTO(FREE);
}

// the current, if Wario's in it; then it's water
void BlockFuncs::block_func_current(uint8_t current)
{
	if (coll_bit(COLLISION_UNK7_F))
		mem_[wWaterCurrent] = current;
	GOTO(WATER);
}

void BlockFuncs::floor()
{
	mem_[wc1ca] = TRUE;
	uint8_t points = mem_[wCollisionPoints];
	if (!(points & (COLLISION_DOWN | COLLISION_UNK2))) {
		if (points & (COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7))
			GOTO(FREE);
		if ((mem_[hYPosLo] & 0x0f) >= 0x05)
			GOTO(FREE);
	}
	// .asm_18228
	if (!wario_flag())
		GOTO(SOLID);
	uint8_t state = mem_[wWarioState];
	GOTO(state == WST_ZOMBIE_LANDING || state == WST_ZOMBIE_SLIPPING_THROUGH_FLOOR ? FREE : SOLID);
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
	mem_[wLadderInteraction] = LADDER_UNDERNEATH;
	GOTO(ADDR_Func_1808c);
}

void BlockFuncs::enter_door()
{
	// centred on the door
	mem_[wWarioXPos + 1] = static_cast<uint8_t>((mem_[wWarioXPos + 1] & 0xf0) + 0x08);
	sfx(SFX_0E6);
	mem_[wWarioState] = WST_ENTERING_DOOR;
	for (uint16_t addr : {wSFXLoopCounter, wWarioStateCounter, wGrabState, wAttackCounter, wJumpVelIndex,
		     wJumpVelTable, wIsCrouching, wFrameDuration, wAnimationFrame})
		mem_[addr] = 0;
	mem_[wDMASourceBank] = BANK_WarioIdleGfx;
	st_ptr(wDMASourcePtr, ADDR_WarioIdleGfx);
	routines_.call(ADDR_LoadWarioGfx);
	mem_[wWarioOAMBank] = BANK_OAM_14a82;
	st_ptr(wWarioOAMPtr, ADDR_OAM_14a82);
	bool right = mem_[wDirection];
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
	if (wario_flag() && mem_[wWarioState] == WST_ATTACKING_AIRBORNE)
		GOTO(SOLID);
	if (!(mem_[wCollisionPoints] & (COLLISION_DOWN | COLLISION_UNK1)))
		GOTO(FREE);
	set_word_be(hYPosHi, static_cast<uint16_t>(word_be(hYPosHi) - 0x10));
	GOTO(collision);
}

// a slope a block down
void BlockFuncs::slope_below(uint16_t collision)
{
	if (!coll_bit(COLLISION_DOWN_F))
		GOTO(FREE);
	set_word_be(hYPosHi, static_cast<uint16_t>(word_be(hYPosHi) + 0x10));
	GOTO(collision);
}

// Wario's or the object's slope interaction
void BlockFuncs::set_slope_interaction(uint8_t slope)
{
	mem_[wario_flag() ? wWarioSlopeInteraction : wObjSlopeInteraction] = ON_SLOPE | slope;
}

// hYPos = the block's top + c
void BlockFuncs::hypos_top_plus_c()
{
	set_word_be(hYPosHi, static_cast<uint16_t>((word_be(hYPosHi) & 0xfff0) + cpu_.c));
}

void BlockFuncs::right_slope_collision()
{
	uint8_t x = mem_[hXPosLo] & 0x0f;
	cpu_.c = static_cast<uint8_t>(0x10 - x);
	hypos_top_plus_c();
	if ((mem_[wCollisionPoints] & (COLLISION_DOWN | COLLISION_UNK1)))
		set_slope_interaction(RIGHT_SLOPE);
	GOTO(ADDR_Func_18064);
}

void BlockFuncs::left_slope_collision()
{
	if (mem_[wCollisionPoints] & (COLLISION_DOWN | COLLISION_UNK1)) {
		cpu_.c = static_cast<uint8_t>((mem_[hXPosLo] & 0x0f) + 2);
		hypos_top_plus_c();
		set_slope_interaction(LEFT_SLOPE);
	}
	GOTO(ADDR_Func_18064);
}

void BlockFuncs::right_slope()
{
	uint8_t points = mem_[wCollisionPoints];
	if (!(points & (COLLISION_DOWN | COLLISION_UNK1)))
		GOTO(FREE);
	if (!(points & COLLISION_UNK1))
		GOTO(ADDR_RightSlopeCollision);
	cpu_.b = mem_[hYPosLo] & 0x0f;
	GOTO((mem_[hXPosLo] & 0x0f) + cpu_.b < 0x10 ? FREE : ADDR_RightSlopeCollision);
}

void BlockFuncs::left_slope()
{
	uint8_t points = mem_[wCollisionPoints];
	if (!(points & (COLLISION_DOWN | COLLISION_UNK1)))
		GOTO(FREE);
	if (!(points & COLLISION_UNK1))
		GOTO(ADDR_LeftSlopeCollision);
	cpu_.b = mem_[hYPosLo] & 0x0f;
	GOTO((mem_[hXPosLo] & 0x0f) < cpu_.b ? ADDR_LeftSlopeCollision : FREE);
}

void BlockFuncs::water_surface()
{
	if (coll_bit(COLLISION_UNK1_F))
		GOTO(FLOOR);
	if ((mem_[hYPosLo] & (0x0e)))
		GOTO(WATER);
	if (!(mem_[wCollisionPoints] & (COLLISION_DOWN | COLLISION_UNK2)))
		GOTO(WATER);
	mem_[wIsInWaterOrSand] = TRUE;
	GOTO(SOLID);
}

// a floor to objects, free to Wario
void BlockFuncs::object_floor()
{
	if (wario_flag())
		GOTO(FREE);
	mem_[wc1ca] = TRUE;
	uint8_t points = mem_[wCollisionPoints];
	if (points & (COLLISION_DOWN | COLLISION_UNK2))
		GOTO(SOLID);
	if (points & (COLLISION_UNK3 | COLLISION_UNK4 | COLLISION_UNK5 | COLLISION_UNK7))
		GOTO(FREE);
	GOTO((mem_[hYPosLo] & 0x0f) < 0x05 ? SOLID : FREE);
}

void BlockFuncs::ladder()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	mem_[wLadderInteraction] = LADDER_COLLISION;
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
	mem_[wLadderInteraction] = LADDER_COLLISION;
	mem_[wRoomTransitionParam] = param;
	GOTO(FREE);
}

void BlockFuncs::spike(uint16_t next)
{
	if (!wario_flag() || mem_[wInvincibleCounter] || mem_[wca94] == 0x02)
		GOTO(next);
	uint8_t state = mem_[wWarioState];
	if (state == WST_STUNG || state == WST_STUNG_RECOVERY)
		GOTO(next);
	// turned around
	mem_[wDirection] = mem_[wDirection] ? DIRECTION_LEFT : DIRECTION_RIGHT;
	uint8_t touch = mem_[wca94];
	if (!touch)
		routines_.farcall(BANK_SetState_Stung, ADDR_SetState_Stung);
	else if (touch == 0x01)
		routines_.farcall(BANK_Func_206eb, ADDR_Func_206eb);
	GOTO(next);
}

void BlockFuncs::water_spike()
{
	if (!wario_flag() || mem_[wInvincibleCounter])
		GOTO(WATER);
	if (mem_[wWarioState] != WST_WATER_STUNG)
		routines_.farcall(BANK_SetState_WaterStung, ADDR_SetState_WaterStung);
	GOTO(WATER);
}

void BlockFuncs::enter_pipe(bool down)
{
	mem_[wRoomTransitionParam] = ROOMTRANSITION_PIPE | ROOMTRANSITIONF_RELOAD_OBJECTS | ROOMTRANSITIONF_DELAY;
	sfx(SFX_PIPE_ENTER);
	mem_[wWarioState] = down ? WST_PIPE_GOING_DOWN : WST_PIPE_GOING_UP;
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
	uint8_t state = mem_[wWarioState];
	if (state != state1 && state != state2)
		GOTO(SOLID);
	if (!(mem_[wCollisionPoints] & coll) || !(mem_[wJoypadDown] & (1 << button)))
		GOTO(SOLID);
	// centred on the pipe (the left one's x carrying into the high byte)
	unsigned lo = (mem_[wWarioXPos + 1] & 0xf0) + (left ? 0x18 : 0x08);
	mem_[wWarioXPos + 1] = static_cast<uint8_t>(lo);
	if (left)
		mem_[wWarioXPos + 0] = static_cast<uint8_t>(mem_[wWarioXPos + 0] + (lo > 0xff));
	enter_pipe(button == B_PAD_DOWN);
}

// a door: Wario in front of it pressing up while idling (or slipping)
void BlockFuncs::door(bool slipping, uint8_t param)
{
	if (!wario_flag() || !coll_bit(COLLISION_UNK7_F) || !(mem_[wJoypadPressed] & (1 << B_PAD_UP)))
		GOTO(FREE);
	uint8_t state = mem_[wWarioState];
	if (state != WST_IDLING && !(slipping && state == WST_SLIPPING) && state != WST_GRAB_IDLING)
		GOTO(FREE);
	mem_[wRoomTransitionParam] = param;
	GOTO(ADDR_EnterDoor);
}

// an underwater door (water: in submersible water there)
void BlockFuncs::underwater_door(bool water, uint8_t param)
{
	uint16_t next = water ? WATER : FREE;
	if (!wario_flag())
		GOTO(next);
	uint8_t transformation = mem_[wTransformation];
	if (transformation != TRANSFORMATION_INVISIBLE_WARIO && transformation)
		GOTO(next);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(next);
	if (water)
		mem_[wWaterInteraction] = SUBMERSIBLE_WATER;
	if (mem_[wJoypadPressed] & (1 << B_PAD_UP))
		mem_[wRoomTransitionParam] = param;
	GOTO(FREE);
}

void BlockFuncs::treasure_door()
{
	if (!wario_flag() || !(mem_[wJoypadPressed] & (1 << B_PAD_UP)) || mem_[wTransformation])
		GOTO(FREE);
	uint8_t state = mem_[wWarioState];
	if (state != WST_IDLING && state != WST_SLIPPING && state != WST_GRAB_IDLING)
		GOTO(FREE);
	mem_[wGrabState] = 0;
	mem_[wLevelEndScreen] = LVLEND_NO_TREASURE;
	GOTO(FREE);
}

void BlockFuncs::transition_block(uint16_t next, uint8_t param)
{
	if (!wario_flag())
		GOTO(next);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(next);
	mem_[wRoomTransitionParam] = param;
	GOTO(next);
}

void BlockFuncs::walkthrough_pipe()
{
	if (wario_flag() && mem_[wTransformation] == TRANSFORMATION_INVISIBLE_WARIO)
		routines_.call(ADDR_RecoverFromTransformation_WithoutInvincibility);
	GOTO(FREE);
}

void BlockFuncs::light_source()
{
	if (!wario_flag())
		GOTO(FREE);
	uint8_t transformation = mem_[wTransformation];
	if (transformation == TRANSFORMATION_ZOMBIE_WARIO)
		routines_.farcall(BANK_SetState_ZombieRecovering, ADDR_SetState_ZombieRecovering);
	else if (transformation == TRANSFORMATION_VAMPIRE_WARIO)
		routines_.call(ADDR_RecoverFromTransformation_WithoutInvincibility);
	GOTO(FREE);
}

// sand / non-submersible water: in it, and Wario's in it if he touches it
void BlockFuncs::in_block(uint16_t addr, uint8_t v)
{
	mem_[wIsInWaterOrSand] = TRUE;
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	mem_[addr] = v;
	GOTO(FREE);
}

// solid to Vampire Wario, who's pushed out of it
void BlockFuncs::vampire_block()
{
	if (!wario_flag() || mem_[wTransformation] != TRANSFORMATION_VAMPIRE_WARIO)
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(SOLID);
	// pushed out past the block's edge, by b
	uint16_t x = static_cast<uint16_t>(word_be(wWarioXPos) & 0xfff0);
	if (!(mem_[wWarioXPos + 1] & 0x08)) {
		cpu_.b = mem_[wCollisionBoxRight];
		x = static_cast<uint16_t>(x - cpu_.b);
	} else {
		cpu_.b = static_cast<uint8_t>(0x10 - mem_[wCollisionBoxLeft]);
		x = static_cast<uint16_t>(x + cpu_.b);
	}
	set_word_be(wWarioXPos, x);
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
	if (!ld_and(wBlockFuncWarioFlag)) // the flags this leaves reach the ret
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
	// ret
}

void BlockFuncs::fence()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	mem_[wIsNearFence] = TRUE;
	GOTO(ADDR_Func_1808c);
}

void BlockFuncs::air_current()
{
	if (!wario_flag())
		GOTO(FREE);
	if (!coll_bit(COLLISION_UNK7_F))
		GOTO(FREE);
	if (mem_[wInvincibleCounter])
		GOTO(FREE);
	mem_[wIsInAirCurrent] = TRUE;
	if (mem_[wTransformation])
		GOTO(FREE);
	mem_[wTransformation] = TRANSFORMATION_FAN;
	mem_[wTouchState] = TOUCH_BUMP;
	mem_[wStingTouchState] = TOUCH_VULNERABLE;
	mem_[wca94] = 0x01;
	routines_.farcall(BANK_SetState_FanStart, ADDR_SetState_FanStart);
	GOTO(FREE);
}

void BlockFuncs::slippery(uint16_t next)
{
	if (!wario_flag())
		GOTO(next);
	if (!(mem_[wCollisionPoints] & (COLLISION_DOWN)))
		GOTO(next);
	mem_[wIsOnSlipperyGround] = TRUE;
	GOTO(next);
}

void BlockFuncs::cloud_platform_1()
{
	uint8_t i = mem_[wRoomPalCycleIndex];
	GOTO(i == 0x07 || i < 0x04 ? FLOOR : FREE);
}

void BlockFuncs::cloud_platform_2()
{
	GOTO(mem_[wRoomPalCycleIndex] < 0x03 ? FREE : FLOOR);
}

void BlockFuncs::small_gap()
{
	if (!wario_flag())
		GOTO(FREE);
	GOTO(mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO ? FREE : SOLID);
}

// toggles the switch Wario hit (and with it every switchable block), then
// it's solid
void BlockFuncs::switch_block()
{
	if (!wario_flag() || !(mem_[wCollisionPoints] & (COLLISION_UNK3 | COLLISION_UNK5)))
		GOTO(SOLID);
	uint16_t at = word_be(wBlockPtr);
	SET_HL(gb_, at);
	uint8_t block = mem_[at];
	mem_[wSwitchState] = block;
	mem_[at] = block ^ 0x08;
	routines_.call(ADDR_Func_e31);
	mem_[wSwitchStateUpdated] = mem_[wSwitchStateUpdated] ^ 1;
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
	if (mem_[wIsRolling] || mem_[wTransformation] == TRANSFORMATION_FAT_WARIO)
		GOTO(FREE);
	uint8_t needs = mem_[wIsSmashAttacking] ? SUPER_JUMP_SLAM_OVERALLS : GARLIC;
	GOTO(mem_[wPowerUpLevel] < needs ? SOLID : FREE);
}

// Func_18f32 / Func_19246 / Func_193dc: saves the block, then breaks it
void BlockFuncs::saved_break(bool floor_transition, uint16_t next)
{
	save_block();
	if (floor_transition) {
		mem_[wIsFloorTransition] = TRUE;
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
	GOTO(mem_[wIsRolling] ? FREE : SOLID);
}

void BlockFuncs::break_multi()
{
	save_block();
	mem_[wIsFloorTransition] = TRUE;
	replace_and_break();
	free_if_strong();
}

// broken by an object: debris if it's on screen, else just the sound
void BlockFuncs::break_by_object()
{
	replace_block(true);
	if (mem_[wCurObjFlags] & (1 << OBJFLAG_ON_SCREEN_F))
		routines_.call(ADDR_BreakBlock);
	else
		sfx(SFX_BLOCK_BREAK);
	GOTO(FREE);
}

void BlockFuncs::doughnut_block()
{
	if (wario_flag() && mem_[wTransformation] == TRANSFORMATION_FAT_WARIO
		&& (mem_[wCollisionPoints] & (COLLISION_UNK1 | COLLISION_UNK6)))
		replace_and_break();
	GOTO(SOLID);
}

// the robo mouse breaks (Func_19423) or passes through: true if it jumped
bool BlockFuncs::robo_mouse()
{
	uint8_t special = mem_[wBlockFuncSpecialCollision];
	if (special == COLLISION_ROBO_MOUSE_BREAK) {
		next_ = ADDR_Func_19423;
		return true;
	}
	if (special == COLLISION_ROBO_MOUSE_FREE) {
		next_ = FREE;
		return true;
	}
	return false;
}

// a cracked block's break: by an attack or a roll (from the side), a head
// smash with the helmet (above), or a ground pound
bool BlockFuncs::breaks_cracked() const
{
	if (coll_bit(COLLISION_UNK4_F))
		return mem_[wAttackCounter] || mem_[wIsRolling];
	if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F))
		return mem_[wPowerUpLevel] >= HEAD_SMASH_HELMET && (mem_[wJumpVelTable] || mem_[wSwimVelIndex]);
	return mem_[wCollisionPoints] & (COLLISION_UNK1 | COLLISION_UNK6);
}

// b = the block that replaces it: broken by an attack or a roll (from the
// side), a head smash with the helmet (above), or a ground pound
void BlockFuncs::cracked_block(uint16_t break_func)
{
	if (!mem_[wBlockFuncBreakFlag] || !breaks_cracked())
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
	if (!mem_[wBlockFuncBreakFlag])
		GOTO(SOLID);
	if (mem_[wTransformation] == TRANSFORMATION_FAT_WARIO)
		GOTO(mem_[wCollisionPoints] & (COLLISION_UNK1 | COLLISION_UNK6) ? ADDR_Func_18df1 : SOLID);
	if (mem_[wPowerUpLevel] < GARLIC)
		GOTO(SOLID);
	bool brk;
	if (coll_bit(COLLISION_UNK4_F)) {
		brk = mem_[wAttackCounter] || mem_[wIsRolling];
	} else if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F)) {
		brk = mem_[wPowerUpLevel] >= HEAD_SMASH_HELMET && (mem_[wJumpVelTable] || mem_[wSwimVelIndex]);
	} else {
		brk = (mem_[wCollisionPoints] & (COLLISION_UNK1 | COLLISION_UNK6))
			&& mem_[wPowerUpLevel] >= SUPER_JUMP_SLAM_OVERALLS;
	}
	GOTO(brk ? ADDR_Func_18df1 : SOLID);
}

// broken by a thrown object
void BlockFuncs::enemy_block()
{
	GOTO(mem_[wBlockFuncSpecialCollision] == COLLISION_THROWN_OBJ ? ADDR_Func_19423 : SOLID);
}

// burnt by Hot Wario once he's engulfed in flames
void BlockFuncs::bonfire_block()
{
	if (wario_flag() && mem_[wTransformation] == TRANSFORMATION_HOT_WARIO && mem_[wTransformationProgress] >= 2)
		GOTO(ADDR_Func_19481);
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
void BlockFuncs::multi_or(uint8_t base, uint8_t n)
{
	mem_[wMultiBlockParam] = base | n;
}

// wMultiBlockParam: bits 0-2 the part already handled (-> low), bits 3-7 a
// block of the same group broken this frame (or n, -> high); true if it
// jumped
bool BlockFuncs::multi_start(uint16_t low, uint8_t n, uint16_t high)
{
	uint8_t param = mem_[wMultiBlockParam];
	if (param & 0x07) {
		next_ = low;
		return true;
	}
	if (param & 0xf8) {
		multi_or(param & 0xf8, n);
		next_ = high;
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
	if (!wario_flag() || mem_[wTransformation] != transformation)
		GOTO(SOLID);
	uint8_t done = mem_[wTransformationProgress];
	if (progress == Progress::SNOWBALL && done < 1)
		GOTO(SOLID);
	if (progress == Progress::BIG_SNOWBALL && done != 3)
		GOTO(SOLID);
	mem_[wMultiBlockParam] = n;
	if (n == 3)
		multi_or(n, 3);
	GOTO(ADDR_Func_18f32);
}

// the robo mouse passes through (free) or breaks it (n, then broken): true
// if it jumped
bool BlockFuncs::multi_robo_mouse(uint8_t n, uint16_t broken)
{
	uint8_t special = mem_[wBlockFuncSpecialCollision];
	if (special == COLLISION_ROBO_MOUSE_FREE) {
		next_ = FREE;
		return true;
	}
	if (special == COLLISION_ROBO_MOUSE_BREAK) {
		mem_[wMultiBlockParam] = n;
		next_ = broken;
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
	if (!mem_[wBlockFuncBreakFlag] || !breaks_cracked())
		GOTO(SOLID);
	mem_[wMultiBlockParam] = n;
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
	if (!mem_[wBlockFuncBreakFlag])
		GOTO(SOLID);
	bool overalls; // needs the super jump slam overalls
	if (mem_[wTransformation] == TRANSFORMATION_FAT_WARIO) {
		if (!(mem_[wCollisionPoints] & (COLLISION_UNK1 | COLLISION_UNK6)))
			GOTO(SOLID);
		overalls = false;
	} else {
		if (mem_[wPowerUpLevel] < GARLIC)
			GOTO(SOLID);
		if (coll_bit(COLLISION_UNK4_F)) {
			if (mem_[wAttackCounter])
				overalls = false;
			else if (mem_[wIsRolling])
				overalls = n == 1; // only Func_190fe checks them when rolling
			else
				GOTO(SOLID);
		} else if (coll_bit(COLLISION_UNK5_F) || coll_bit(COLLISION_UNK3_F)) {
			if (mem_[wPowerUpLevel] < HEAD_SMASH_HELMET || (!mem_[wJumpVelTable] && !mem_[wSwimVelIndex]))
				GOTO(SOLID);
			overalls = false;
		} else {
			if (!(mem_[wCollisionPoints] & (COLLISION_UNK1 | COLLISION_UNK6)))
				GOTO(SOLID);
			overalls = true;
		}
	}
	if (overalls && mem_[wPowerUpLevel] < SUPER_JUMP_SLAM_OVERALLS)
		GOTO(SOLID);
	mem_[wMultiBlockParam] = n;
	GOTO(ADDR_Func_193dc);
}

// broken by Ball o' String Wario
void BlockFuncs::ball_o_string_block()
{
	if (!wario_flag())
		GOTO(SOLID);
	GOTO(mem_[wTransformation] == TRANSFORMATION_BALL_O_STRING_WARIO ? ADDR_Func_19481 : SOLID);
}

#undef GOTO

} // namespace

} // namespace wl3::level

using wl3::level::BlockFuncs;
using wl3::level::Progress;

// Each ends where the asm does (HOOK_MANUAL_RET): a jump on to another
// block function, or a ret. At a jump a and f are dead: no block function
// reads them on entry (the asm's entry liveness, over all 269), and every
// chain ends in Free, Solid, Func_1808c, Func_18064 or Func_18078, which set
// them, or Fire's ret, which is exact. So the hooks that only jump on
// clobber them; the rest stay exact.
PORT_EX(ProcessBlock, HOOK_MANUAL_RET, 0) { BlockFuncs(gb).process_block(); }
PORT(BlockFunc_Free) { BlockFuncs(gb).free(); }
PORT(BlockFunc_Solid) { BlockFuncs(gb).solid(); }
PORT_EX(BlockFunc_Water, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Water); b.jump_on(); }
PORT_EX(BlockFunc_RightCurrent, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_RightCurrent); b.jump_on(); }
PORT_EX(BlockFunc_UpCurrent, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_UpCurrent); b.jump_on(); }
PORT_EX(BlockFunc_LeftCurrent, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_LeftCurrent); b.jump_on(); }
PORT_EX(BlockFunc_DownCurrent, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DownCurrent); b.jump_on(); }
PORT_EX(BlockFunc_Floor, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Floor); b.jump_on(); }
PORT(Func_1808c) { BlockFuncs(gb).free_1808c(); }
PORT_EX(Func_182ec, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_182ec); b.jump_on(); }
PORT_EX(EnterDoor, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_EnterDoor); b.jump_on(); }
PORT(Func_18064) { BlockFuncs(gb).block_func_c18d(0x10); }
PORT(Func_18078) { BlockFuncs(gb).block_func_c18d(0x01); }
PORT_EX(BlockFunc_180b5, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_180b5); b.jump_on(); }
PORT_EX(BlockFunc_1813f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1813f); b.jump_on(); }
PORT_EX(BlockFunc_1809f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1809f); b.jump_on(); }
PORT_EX(BlockFunc_18129, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18129); b.jump_on(); }
PORT_EX(RightSlopeCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_RightSlopeCollision); b.jump_on(); }
PORT_EX(LeftSlopeCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_LeftSlopeCollision); b.jump_on(); }
PORT_EX(BlockFunc_RightSlope, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_RightSlope); b.jump_on(); }
PORT_EX(BlockFunc_LeftSlope, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_LeftSlope); b.jump_on(); }
PORT_EX(BlockFunc_WaterSurface, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_WaterSurface); b.jump_on(); }
PORT_EX(BlockFunc_18262, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18262); b.jump_on(); }
PORT_EX(BlockFunc_Ladder, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Ladder); b.jump_on(); }
PORT_EX(BlockFunc_182dc, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_182dc); b.jump_on(); }
PORT_EX(BlockFunc_LadderTop, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_LadderTop); b.jump_on(); }
PORT_EX(BlockFunc_182fe, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_182fe); b.jump_on(); }
PORT_EX(Func_1831a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1831a); b.jump_on(); }
PORT_EX(BlockFunc_NonSolidSpike, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonSolidSpike); b.jump_on(); }
PORT_EX(BlockFunc_SolidSpike, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_SolidSpike); b.jump_on(); }
PORT_EX(BlockFunc_WaterSpike, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_WaterSpike); b.jump_on(); }
PORT_EX(EnterPipeDown, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_EnterPipeDown); b.jump_on(); }
PORT_EX(EnterPipeUp, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_EnterPipeUp); b.jump_on(); }
PORT_EX(BlockFunc_DownPipe_Left, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DownPipe_Left); b.jump_on(); }
PORT_EX(BlockFunc_DownPipe_Right, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DownPipe_Right); b.jump_on(); }
PORT_EX(BlockFunc_UpPipe_Left, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_UpPipe_Left); b.jump_on(); }
PORT_EX(BlockFunc_UpPipe_Right, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_UpPipe_Right); b.jump_on(); }
PORT_EX(BlockFunc_Door, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Door); b.jump_on(); }
PORT_EX(BlockFunc_MinigameDoor, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_MinigameDoor); b.jump_on(); }
PORT_EX(Func_18624, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18624); b.jump_on(); }
PORT_EX(Func_1864e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1864e); b.jump_on(); }
PORT_EX(BlockFunc_UnderwaterDoor, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_UnderwaterDoor); b.jump_on(); }
PORT_EX(BlockFunc_18727, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18727); b.jump_on(); }
PORT_EX(Func_18755, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18755); b.jump_on(); }
PORT_EX(BlockFunc_TreasureDoor, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_TreasureDoor); b.jump_on(); }
PORT_EX(BlockFunc_187b0, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_187b0); b.jump_on(); }
PORT_EX(Func_187c7, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_187c7); b.jump_on(); }
PORT_EX(BlockFunc_187de, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_187de); b.jump_on(); }
PORT_EX(Func_187f5, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_187f5); b.jump_on(); }
PORT_EX(BlockFunc_WalkthroughPipe, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_WalkthroughPipe); b.jump_on(); }
PORT_EX(BlockFunc_LightSource, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_LightSource); b.jump_on(); }
PORT_EX(BlockFunc_Sand, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Sand); b.jump_on(); }
PORT_EX(BlockFunc_NonSubmersibleWater, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonSubmersibleWater); b.jump_on(); }
PORT_EX(BlockFunc_18868, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18868); b.jump_on(); }
PORT_EX(BlockFunc_188ba, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_188ba); b.jump_on(); }
PORT_EX(BlockFunc_189f1, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_189f1); b.jump_on(); }
PORT_EX(BlockFunc_Fire, HOOK_MANUAL_RET, 0) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Fire); b.jump_on(); }
PORT_EX(BlockFunc_Fence, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Fence); b.jump_on(); }
PORT_EX(BlockFunc_AirCurrent, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_AirCurrent); b.jump_on(); }
PORT_EX(BlockFunc_SlipperySolid, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_SlipperySolid); b.jump_on(); }
PORT_EX(BlockFunc_SlipperyFloor, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_SlipperyFloor); b.jump_on(); }
PORT_EX(BlockFunc_CloudPlatform1, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CloudPlatform1); b.jump_on(); }
PORT_EX(BlockFunc_CloudPlatform2, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CloudPlatform2); b.jump_on(); }
PORT_EX(BlockFunc_SmallGap, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_SmallGap); b.jump_on(); }
PORT_EX(BlockFunc_Switch, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_Switch); b.jump_on(); }
PORT(BreakBlock) { BlockFuncs(gb).break_block(true, ADDR_ObjParams_Coin); }
PORT(BreakBlockWithColourCoin) { BlockFuncs(gb).break_block(false, ADDR_ObjParams_ColourCoin); }
PORT_EX(Func_18f32, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f32); b.jump_on(); }
PORT_EX(Func_19246, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19246); b.jump_on(); }
PORT_EX(Func_19481, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19481); b.jump_on(); }
PORT_EX(Func_18df1, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18df1); b.jump_on(); }
PORT_EX(Func_19086, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19086); b.jump_on(); }
PORT_EX(Func_19423, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19423); b.jump_on(); }
PORT_EX(DoughnutBlockCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_DoughnutBlockCollision); b.jump_on(); }
PORT_EX(CrackedBlockWithColourCoinCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_CrackedBlockWithColourCoinCollision); b.jump_on(); }
PORT_EX(CrackedBlockCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_CrackedBlockCollision); b.jump_on(); }
PORT_EX(NonCrackedBlockCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_NonCrackedBlockCollision); b.jump_on(); }
PORT_EX(EnemyBlockCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_EnemyBlockCollision); b.jump_on(); }
PORT_EX(BonfireBlockCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BonfireBlockCollision); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlockWithColourCoin_78); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlockWithColourCoin_79); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlockWithColourCoin_7a); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlockWithColourCoin_7b); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlockWithColourCoin_7c); b.jump_on(); }
PORT_EX(Func_18a0f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18a0f); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlockWithColourCoin_7e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlockWithColourCoin_7e); b.jump_on(); }
PORT_EX(Func_18a17, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18a17); b.jump_on(); }
PORT_EX(Func_18aa8, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18aa8); b.jump_on(); }
PORT_EX(Func_18aec, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18aec); b.jump_on(); }
PORT_EX(Func_18b2d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18b2d); b.jump_on(); }
PORT_EX(Func_18b71, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18b71); b.jump_on(); }
PORT_EX(Func_18bb5, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18bb5); b.jump_on(); }
PORT_EX(Func_18bf9, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18bf9); b.jump_on(); }
PORT_EX(Func_18c3a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18c3a); b.jump_on(); }
PORT_EX(Func_18c7e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18c7e); b.jump_on(); }
PORT_EX(Func_19522, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19522); b.jump_on(); }
PORT_EX(Func_19560, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19560); b.jump_on(); }
PORT_EX(Func_1959b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1959b); b.jump_on(); }
PORT_EX(YarnBlockCollision, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_YarnBlockCollision); b.jump_on(); }
PORT_EX(Func_18e2c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18e2c); b.jump_on(); }
PORT_EX(Func_18ec0, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ec0); b.jump_on(); }
PORT_EX(Func_18f7d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f7d); b.jump_on(); }
PORT_EX(Func_19011, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19011); b.jump_on(); }
PORT_EX(Func_190fe, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_190fe); b.jump_on(); }
PORT_EX(Func_191b3, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_191b3); b.jump_on(); }
PORT_EX(Func_19291, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19291); b.jump_on(); }
PORT_EX(Func_19346, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19346); b.jump_on(); }
PORT_EX(Func_193dc, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_193dc); b.jump_on(); }
PORT_EX(Func_19502, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19502); b.jump_on(); }
PORT_EX(Func_18a9a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18a9a); b.jump_on(); }
PORT_EX(Func_18a9e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18a9e); b.jump_on(); }
PORT_EX(Func_18aa2, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18aa2); b.jump_on(); }
PORT_EX(BlockFunc_18aa6, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18aa6); b.jump_on(); }
PORT_EX(Func_18ade, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ade); b.jump_on(); }
PORT_EX(Func_18ae2, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ae2); b.jump_on(); }
PORT_EX(Func_18ae6, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ae6); b.jump_on(); }
PORT_EX(BlockFunc_18aea, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18aea); b.jump_on(); }
PORT_EX(BlockFunc_18b23, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18b23); b.jump_on(); }
PORT_EX(BlockFunc_18b2b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18b2b); b.jump_on(); }
PORT_EX(BlockFunc_18b67, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18b67); b.jump_on(); }
PORT_EX(BlockFunc_18b6f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18b6f); b.jump_on(); }
PORT_EX(BlockFunc_18bb3, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18bb3); b.jump_on(); }
PORT_EX(BlockFunc_18bf7, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18bf7); b.jump_on(); }
PORT_EX(BlockFunc_18c30, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18c30); b.jump_on(); }
PORT_EX(BlockFunc_18c38, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18c38); b.jump_on(); }
PORT_EX(BlockFunc_18c74, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18c74); b.jump_on(); }
PORT_EX(BlockFunc_18c7c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18c7c); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_78); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_79); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_7a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_7a); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_7b); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_7c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_7c); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_7d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_7d); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_7e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_7e); b.jump_on(); }
PORT_EX(BlockFunc_CrackedBlock_7f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_CrackedBlock_7f); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_78); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_79); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_7a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_7a); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_7b); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_7c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_7c); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_7d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_7d); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_7e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_7e); b.jump_on(); }
PORT_EX(BlockFunc_NonCrackedBlock_7f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_NonCrackedBlock_7f); b.jump_on(); }
PORT_EX(BlockFunc_18e0e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18e0e); b.jump_on(); }
PORT_EX(BlockFunc_18e1a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18e1a); b.jump_on(); }
PORT_EX(BlockFunc_18e26, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18e26); b.jump_on(); }
PORT_EX(BlockFunc_18e2a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18e2a); b.jump_on(); }
PORT_EX(BlockFunc_18ea2, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18ea2); b.jump_on(); }
PORT_EX(BlockFunc_18eae, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18eae); b.jump_on(); }
PORT_EX(BlockFunc_18ebe, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18ebe); b.jump_on(); }
PORT_EX(BlockFunc_18f5f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18f5f); b.jump_on(); }
PORT_EX(BlockFunc_18f6b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18f6b); b.jump_on(); }
PORT_EX(BlockFunc_18f7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18f7b); b.jump_on(); }
PORT_EX(BlockFunc_18ff3, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18ff3); b.jump_on(); }
PORT_EX(BlockFunc_18fff, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_18fff); b.jump_on(); }
PORT_EX(BlockFunc_19007, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19007); b.jump_on(); }
PORT_EX(BlockFunc_1900f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1900f); b.jump_on(); }
PORT_EX(BlockFunc_190e4, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_190e4); b.jump_on(); }
PORT_EX(BlockFunc_190e8, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_190e8); b.jump_on(); }
PORT_EX(BlockFunc_190fc, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_190fc); b.jump_on(); }
PORT_EX(BlockFunc_19199, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19199); b.jump_on(); }
PORT_EX(BlockFunc_1919d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1919d); b.jump_on(); }
PORT_EX(BlockFunc_191b1, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_191b1); b.jump_on(); }
PORT_EX(BlockFunc_19277, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19277); b.jump_on(); }
PORT_EX(BlockFunc_1927b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1927b); b.jump_on(); }
PORT_EX(BlockFunc_1928f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1928f); b.jump_on(); }
PORT_EX(BlockFunc_1932c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1932c); b.jump_on(); }
PORT_EX(BlockFunc_19330, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19330); b.jump_on(); }
PORT_EX(BlockFunc_19344, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19344); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_78); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_79); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_7a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_7a); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_7b); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_7c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_7c); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_7d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_7d); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_7e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_7e); b.jump_on(); }
PORT_EX(BlockFunc_EnemyBlock_7f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_EnemyBlock_7f); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_78); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_79); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_7a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_7a); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_7b); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_7c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_7c); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_7d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_7d); b.jump_on(); }
PORT_EX(BlockFunc_BonfireBlock_7e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_BonfireBlock_7e); b.jump_on(); }
PORT_EX(BlockFunc_DoughnutBlock_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DoughnutBlock_78); b.jump_on(); }
PORT_EX(BlockFunc_DoughnutBlock_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DoughnutBlock_79); b.jump_on(); }
PORT_EX(BlockFunc_DoughnutBlock_7a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DoughnutBlock_7a); b.jump_on(); }
PORT_EX(BlockFunc_DoughnutBlock_7b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_DoughnutBlock_7b); b.jump_on(); }
PORT_EX(BlockFunc_19518, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19518); b.jump_on(); }
PORT_EX(BlockFunc_19520, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19520); b.jump_on(); }
PORT_EX(BlockFunc_19552, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19552); b.jump_on(); }
PORT_EX(BlockFunc_19556, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19556); b.jump_on(); }
PORT_EX(BlockFunc_1955e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1955e); b.jump_on(); }
PORT_EX(BlockFunc_1958d, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_1958d); b.jump_on(); }
PORT_EX(BlockFunc_19599, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_19599); b.jump_on(); }
PORT_EX(BlockFunc_YarnBlock_79, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_YarnBlock_79); b.jump_on(); }
PORT_EX(BlockFunc_YarnBlock_78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_BlockFunc_YarnBlock_78); b.jump_on(); }
PORT_EX(Func_19467, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19467); b.jump_on(); }
PORT_EX(Func_194a7, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194a7); b.jump_on(); }
PORT_EX(Func_194ab, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194ab); b.jump_on(); }
PORT_EX(Func_194af, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194af); b.jump_on(); }
PORT_EX(Func_194b3, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194b3); b.jump_on(); }
PORT_EX(Func_194e4, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194e4); b.jump_on(); }
PORT_EX(Func_194e8, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194e8); b.jump_on(); }
PORT_EX(Func_194ec, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194ec); b.jump_on(); }
PORT_EX(Func_194f0, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194f0); b.jump_on(); }
PORT_EX(Func_194f4, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194f4); b.jump_on(); }
PORT_EX(Func_194f8, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194f8); b.jump_on(); }
PORT_EX(Func_194fc, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_194fc); b.jump_on(); }
PORT_EX(Func_19500, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19500); b.jump_on(); }
PORT_EX(Func_195cf, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_195cf); b.jump_on(); }
PORT_EX(Func_195d3, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_195d3); b.jump_on(); }
PORT_EX(Func_19514, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19514); b.jump_on(); }
PORT_EX(Func_1951c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1951c); b.jump_on(); }
PORT_EX(Func_1955a, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1955a); b.jump_on(); }
PORT_EX(Func_19591, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19591); b.jump_on(); }
PORT_EX(Func_19595, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19595); b.jump_on(); }
PORT_EX(Func_19328, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19328); b.jump_on(); }
PORT_EX(Func_19334, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19334); b.jump_on(); }
PORT_EX(Func_19338, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19338); b.jump_on(); }
PORT_EX(Func_1933c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1933c); b.jump_on(); }
PORT_EX(Func_19340, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19340); b.jump_on(); }
PORT_EX(Func_19273, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19273); b.jump_on(); }
PORT_EX(Func_1927f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1927f); b.jump_on(); }
PORT_EX(Func_19283, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19283); b.jump_on(); }
PORT_EX(Func_19287, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19287); b.jump_on(); }
PORT_EX(Func_1928b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1928b); b.jump_on(); }
PORT_EX(Func_19195, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19195); b.jump_on(); }
PORT_EX(Func_191a1, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_191a1); b.jump_on(); }
PORT_EX(Func_191a5, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_191a5); b.jump_on(); }
PORT_EX(Func_191a9, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_191a9); b.jump_on(); }
PORT_EX(Func_191ad, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_191ad); b.jump_on(); }
PORT_EX(Func_18b1f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18b1f); b.jump_on(); }
PORT_EX(Func_18b27, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18b27); b.jump_on(); }
PORT_EX(Func_18b63, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18b63); b.jump_on(); }
PORT_EX(Func_18b6b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18b6b); b.jump_on(); }
PORT_EX(Func_18ba7, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ba7); b.jump_on(); }
PORT_EX(Func_18bab, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18bab); b.jump_on(); }
PORT_EX(Func_18baf, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18baf); b.jump_on(); }
PORT_EX(Func_18beb, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18beb); b.jump_on(); }
PORT_EX(Func_18bef, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18bef); b.jump_on(); }
PORT_EX(Func_18bf3, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18bf3); b.jump_on(); }
PORT_EX(Func_18c2c, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18c2c); b.jump_on(); }
PORT_EX(Func_18c34, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18c34); b.jump_on(); }
PORT_EX(Func_18c70, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18c70); b.jump_on(); }
PORT_EX(Func_18c78, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18c78); b.jump_on(); }
PORT_EX(Func_18e12, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18e12); b.jump_on(); }
PORT_EX(Func_18e16, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18e16); b.jump_on(); }
PORT_EX(Func_18e1e, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18e1e); b.jump_on(); }
PORT_EX(Func_18e22, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18e22); b.jump_on(); }
PORT_EX(Func_18ea6, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ea6); b.jump_on(); }
PORT_EX(Func_18eaa, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18eaa); b.jump_on(); }
PORT_EX(Func_18eb2, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18eb2); b.jump_on(); }
PORT_EX(Func_18eb6, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18eb6); b.jump_on(); }
PORT_EX(Func_18eba, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18eba); b.jump_on(); }
PORT_EX(Func_18f63, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f63); b.jump_on(); }
PORT_EX(Func_18f67, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f67); b.jump_on(); }
PORT_EX(Func_18f6f, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f6f); b.jump_on(); }
PORT_EX(Func_18f73, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f73); b.jump_on(); }
PORT_EX(Func_18f77, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18f77); b.jump_on(); }
PORT_EX(Func_18ff7, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ff7); b.jump_on(); }
PORT_EX(Func_18ffb, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_18ffb); b.jump_on(); }
PORT_EX(Func_19003, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_19003); b.jump_on(); }
PORT_EX(Func_1900b, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_1900b); b.jump_on(); }
PORT_EX(Func_190e0, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_190e0); b.jump_on(); }
PORT_EX(Func_190ec, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_190ec); b.jump_on(); }
PORT_EX(Func_190f0, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_190f0); b.jump_on(); }
PORT_EX(Func_190f4, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_190f4); b.jump_on(); }
PORT_EX(Func_190f8, HOOK_MANUAL_RET, R_A | R_F) { BlockFuncs b(gb); b.dispatch(ADDR_Func_190f8); b.jump_on(); }
