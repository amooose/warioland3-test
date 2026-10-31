// Wormwould (wormwould.asm), the boss worm in the sand: it comes up on the
// side away from Wario, shoots rolling rocks high or low, and burrows again
// (moving to the other side when Wario's near); smashed three times it's
// beaten. And the rolling rocks and the palm trees (with their spawner).
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t WW(uint16_t rom) { return static_cast<uint16_t>(ADDR_WormwouldFunc + (rom - 0x4020)); }

constexpr uint16_t INIT = WW(0x4044); // .Init
constexpr uint16_t DO_NOTHING = WW(0x4103); // .DoNothing
constexpr uint16_t MAIN = WW(0x41b5); // .Main
constexpr uint16_t UNDERGROUND = WW(0x4335); // .Underground
constexpr uint16_t DEFEATED_RAISE = WW(0x435f); // .DefeatedRaise
constexpr uint16_t DEFEATED_WAIT = WW(0x4387); // .DefeatedWait
constexpr uint16_t DEFEATED_FALL = WW(0x43d4); // .DefeatedFall

constexpr uint16_t ROLLING_ROCK_HIT = ADDR_RollingRockFunc + 0x2a; // .Hit
constexpr uint16_t PALM_TREE_SPAWN_TREES = ADDR_PalmTreeSpawnerFunc + 0x16; // .SpawnTrees
constexpr uint16_t PALM_TREE_UPDATE = ADDR_PalmTreeFunc + 0x16; // .Update

constexpr uint8_t WORMWOULD_IDLE_DURATION = 43;
constexpr uint8_t WORMWOULD_SHOOT_ROCK_DURATION = 69;
constexpr uint8_t WORMWOULD_BURROW_DELAY = 26;
constexpr uint8_t WORMWOULD_HURT_DURATION = 50;
constexpr uint8_t WORMWOULD_UNDERGROUND_DURATION = 30;

class Wormwould : public Object {
public:
	using Object::Object;

	void start(); // WormwouldFunc
	void init(); // .Init
	void do_nothing() { no_actions(); } // .DoNothing
	void main(); // .Main
	void underground(); // .Underground
	void defeated_raise(); // .DefeatedRaise
	void defeated_wait(); // .DefeatedWait
	void defeated_fall(); // .DefeatedFall

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	void leave();
	void change_side(); // .check_change_side
	void set_burrowing();
	void start_shoot();
	void set_idle();
	void spawn_rock(uint16_t left, uint16_t right);
};

class WormwouldHelper : public Object {
public:
	using Object::Object;

	void rock_rolling(uint16_t then, uint16_t move); // RollingRockFunc_Left / _Right
	void rock(uint16_t page); // RollingRockFunc
	void palm_tree_spawner(); // PalmTreeSpawnerFunc
	void palm_tree(); // PalmTreeFunc
	void palm_tree_update(); // .Update
};

} // namespace

void Wormwould::start()
{
	set_update(INIT);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() = (substate() & OBJSUBFLAGS_MASK) | (3 + 1) | OBJSUBFLAG_UNK_4 | OBJSUBFLAG_HDIR;
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = 30;
	set_frameset(ADDR_Frameset_696ce);
}

// once Wario's on its floor: down into the sand, and the battle starts
void Wormwould::init()
{
	no_actions();
	if (mem_[wCurObjYPos + 0] != mem_[wWarioYPos + 1])
		return;
	var2() = 0x20;
	move_down_by_var2();
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = TRUE;
	change_side();
}

void Wormwould::leave()
{
	set_update(DO_NOTHING);
	mem_[wBossState] = NONE;
	routines_.call(ADDR_UpdateLevelMusic);
}

// comes up facing Wario, moved to the other side ($70 pixels over) when
// he's behind it or close
void Wormwould::change_side()
{
	bool left_side = facing_right();
	bool change = false;
	if (flags() & (1 << OBJFLAG_ON_SCREEN_F)) {
		uint8_t own = static_cast<uint8_t>(screen_x() + 0x2a);
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		uint8_t ahead = left_side ? static_cast<uint8_t>(wario - own) : static_cast<uint8_t>(own - wario);
		change = (left_side ? wario < own : own < wario) || ahead < 0x38;
	}
	uint16_t frameset;
	if (!change) {
		frameset = left_side ? ADDR_Frameset_696ce : ADDR_Frameset_6970a;
	} else if (left_side) {
		substate() &= ~OBJSUBFLAG_HDIR;
		frameset = ADDR_Frameset_6970a;
		uint16_t x_pos = static_cast<uint16_t>(x() + 0x70);
		mem_[wCurObjXPos + 0] = static_cast<uint8_t>(x_pos);
		mem_[wCurObjXPos + 1] = static_cast<uint8_t>(x_pos >> 8);
	} else {
		substate() |= OBJSUBFLAG_HDIR;
		frameset = ADDR_Frameset_696ce;
		uint16_t x_pos = static_cast<uint16_t>(x() - 0x70);
		mem_[wCurObjXPos + 0] = static_cast<uint8_t>(x_pos);
		mem_[wCurObjXPos + 1] = static_cast<uint8_t>(x_pos >> 8);
	}
	set_frameset(frameset);
	set_update(MAIN);
	flags() &= ~OBJFLAG_INVISIBLE;
	state_duration() = 0;
	var1() = mem_[wCurObjYPos + 0];
	var2() = mem_[wCurObjYPos + 1];
	var3() = 0x41;
	state() = 0x2d;
	set_interaction(OBJ_INTERACTION_WORMWOULD);
	play_sfx(SFX_11A);
}

void Wormwould::set_burrowing()
{
	state() = 0x5c;
	set_frameset(facing(ADDR_Frameset_69734, ADDR_Frameset_69701));
}

void Wormwould::start_shoot()
{
	set_frameset(facing(ADDR_Frameset_69713, ADDR_Frameset_696d7));
	state_duration() = WORMWOULD_SHOOT_ROCK_DURATION;
	state() = 0x5a;
}

void Wormwould::set_idle()
{
	state() = 0;
	state_duration() = WORMWOULD_IDLE_DURATION;
}

void Wormwould::spawn_rock(uint16_t left, uint16_t right)
{
	play_sfx(SFX_TEMPLE_ROCK);
	SET_BC(gb_, facing_right() ? right : left);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// rises ($2d), shoots ($5a high, $5b low), idles (0), burrows ($5c);
// attacked, it burrows; smashed, it's hurt
void Wormwould::main()
{
	uint8_t s = state();
	switch (s) {
	case 0x00:
		if (static_cast<uint8_t>(screen_y() + 0x20) < 0x20) {
			leave();
			return;
		}
		if (--state_duration())
			return;
		start_shoot();
		return;
	case 0x34: // after an attack
		if (state_duration() == 16)
			play_sfx(SFX_0A6);
		if (--state_duration())
			return;
		set_burrowing();
		return;
	case 0x5a: // a rock high
		--state_duration();
		if (state_duration() == WORMWOULD_SHOOT_ROCK_DURATION - 48) {
			spawn_rock(ADDR_ObjParams_HighRollingRockLeft, ADDR_ObjParams_HighRollingRockRight);
			return;
		}
		if (state_duration())
			return;
		set_frameset(facing(ADDR_Frameset_6970a, ADDR_Frameset_696ce));
		set_idle();
		return;
	case 0x2d: // rising; the low rock early in the fight
		if ((substate() & 0x0f) < 3 && var3() == 0x1e) {
			set_frameset(facing(ADDR_Frameset_696bc, ADDR_Frameset_696c5));
			state_duration() = 34;
			state() = 0x5b;
			return;
		}
		if (--var3()) {
			move_up();
			return;
		}
		if ((substate() & 0x0f) < 3 + 1)
			start_shoot();
		else
			set_idle();
		return;
	case 0x5b: // a rock low
		--state_duration();
		if (state_duration() == 0x0a) {
			spawn_rock(ADDR_ObjParams_LowRollingRockLeft, ADDR_ObjParams_LowRollingRockRight);
			return;
		}
		if (state_duration())
			return;
		state_duration() = 1;
		set_burrowing();
		return;
	case 0x5c: // down to where it rose from (slowly at first)
		if ((substate() & 0x0f) >= 3 + 1 && !state_duration() && !(mem_[wGlobalCounter] & 1))
			return;
		if (mem_[wCurObjYPos + 0] != var1() || mem_[wCurObjYPos + 1] != var2()) {
			move_down();
			return;
		}
		if ((substate() & 0x0f) == 1) {
			var3() = 0x21;
			set_update(DEFEATED_RAISE);
			SET_HL(gb_, wCurObjUnk02);
			gb_->cpu.e = 0x05;
			routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
			play_sfx(SFX_0A7);
			return;
		}
		state_duration() = WORMWOULD_UNDERGROUND_DURATION;
		set_update(UNDERGROUND);
		flags() |= OBJFLAG_INVISIBLE;
		return;
	default:
		break;
	}
	s &= 0xfe;
	if (s == OBJSTATE_ATTACKED_LEFT_START) {
		state() = 0x34;
		set_frameset(facing(ADDR_Frameset_6971c, ADDR_Frameset_696e0));
		state_duration() = WORMWOULD_BURROW_DELAY;
		return;
	}
	if (s == 0x08) {
		state_duration() = 1;
		set_burrowing();
		return;
	}
	if (s == OBJSTATE_VANISH_TOUCH) { // smashed
		state() = 0x3a;
		set_frameset(facing(ADDR_Frameset_69721, ADDR_Frameset_696e5));
		state_duration() = WORMWOULD_HURT_DURATION;
		set_interaction(OBJ_INTERACTION_0B);
		mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-16);
		SET_HL(gb_, wCurObjCollBoxTop);
		routines_.call(ADDR_DecrementRemainingBossHits);
		var3() = 0;
		play_sfx(SFX_VANISH);
		return;
	}
	if (s == 0x3a) { // hurt
		SET_BC(gb_, ADDR_Data_60dd0);
		routines_.call(ADDR_ApplyObjYMovement);
		if (state_duration() == 30)
			play_sfx(SFX_0A7);
		if (--state_duration())
			return;
		state() = 0x5c;
		if ((substate() & 0x0f) == 1) {
			stop_music2(mem_);
			return;
		}
		mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-26);
		set_frameset(facing(ADDR_Frameset_69726, ADDR_Frameset_696ea));
		return;
	}
	state() = 0;
}

void Wormwould::underground()
{
	no_actions();
	if (screen_y() < 0x3a) {
		leave();
		return;
	}
	if (--state_duration())
		return;
	flags() &= ~OBJFLAG_INVISIBLE;
	change_side();
}

void Wormwould::defeated_raise()
{
	no_actions();
	if (--var3()) {
		routines_.call(ADDR_MoveObjectUp_Fast);
		return;
	}
	set_frameset(facing(ADDR_Frameset_6972f, ADDR_Frameset_696f3));
	state_duration() = 30;
	set_update(DEFEATED_WAIT);
}

void Wormwould::defeated_wait()
{
	no_actions();
	if (!state_duration()) {
		set_update(DEFEATED_FALL);
		play_sfx(SFX_0A3);
		defeated_fall();
		return;
	}
	if (--state_duration())
		return;
	if (static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a))
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
	substate() &= ~OBJSUBFLAG_UNK_4;
	substate() &= 0x80;
	var3() = 0;
	flags() |= OBJFLAG_PRIORITY;
	mem_[wBossState] = BOSS_DEFEATED;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = 0;
}

void Wormwould::defeated_fall()
{
	no_actions();
	SET_BC(gb_, ADDR_Data_60180);
	routines_.call(ADDR_ApplyObjYMovement);
	routines_.call(facing_right() ? ADDR_MoveObjectLeft : ADDR_MoveObjectRight);
}

// rolls for its duration, then on (then: .MoveLeft / .MoveRight)
void WormwouldHelper::rock_rolling(uint16_t then, uint16_t move)
{
	no_actions();
	if (!--state_duration())
		set_update(then);
	routines_.call(move);
	rock(static_cast<uint16_t>(gb_->cpu.h << 8));
}

// along the floor (high rocks: var1); hit, it's knocked away
void WormwouldHelper::rock(uint16_t page)
{
	SET_BC(gb_, var1() ? ADDR_Data_60900 : ADDR_Data_608b0);
	if (!state()) {
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	var3() = 0;
	mem_[page | (OBJ_UPDATE_FUNCTION + 1)] = static_cast<uint8_t>(ROLLING_ROCK_HIT >> 8);
	mem_[page | OBJ_UPDATE_FUNCTION] = static_cast<uint8_t>(ROLLING_ROCK_HIT);
	play_sfx(SFX_016);
}

void WormwouldHelper::palm_tree_spawner()
{
	set_update(PALM_TREE_SPAWN_TREES);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	state_duration() = 240;
	frame_duration() = 2;
	no_actions();
}

void WormwouldHelper::palm_tree()
{
	set_update(PALM_TREE_UPDATE);
	SET_HL(gb_, ADDR_PalmTreePals);
	SET_DE(gb_, wTempPals2 + PAL_SIZE * 7);
	gb_->cpu.c = 7;
	gb_->cpu.b = 1;
	routines_.call(ADDR_CopyAndApplyOBPals);
}

// rises while var1 counts down, then (var2) opens
void WormwouldHelper::palm_tree_update()
{
	one_way_platform();
	if (var1()) {
		--var1();
		move_up();
		no_actions();
	} else if (var2()) {
		--var2();
		no_actions();
		return;
	}
	frame_duration() = 10;
}

} // namespace wl3::level

using wl3::level::Wormwould;
using wl3::level::WormwouldHelper;

PORT_EX(WormwouldFunc, 0, ALL_REGISTERS) { Wormwould(gb).start(); }
PORT_LOCAL_EX(WormwouldFunc, Init, 0, ALL_REGISTERS) { Wormwould(gb).init(); }
PORT_LOCAL_EX(WormwouldFunc, DoNothing, 0, ALL_REGISTERS) { Wormwould(gb).do_nothing(); }
PORT_LOCAL_EX(WormwouldFunc, Main, 0, ALL_REGISTERS) { Wormwould(gb).main(); }
PORT_LOCAL_EX(WormwouldFunc, Underground, 0, ALL_REGISTERS) { Wormwould(gb).underground(); }
PORT_LOCAL_EX(WormwouldFunc, DefeatedRaise, 0, ALL_REGISTERS) { Wormwould(gb).defeated_raise(); }
PORT_LOCAL_EX(WormwouldFunc, DefeatedWait, 0, ALL_REGISTERS) { Wormwould(gb).defeated_wait(); }
PORT_LOCAL_EX(WormwouldFunc, DefeatedFall, 0, ALL_REGISTERS) { Wormwould(gb).defeated_fall(); }

PORT_EX(RollingRockFunc_Left, 0, ALL_REGISTERS) { WormwouldHelper(gb).rock_rolling(ADDR_RollingRockFunc_Left + 0x13, ADDR_MoveObjectLeft_Fast); }
PORT_EX(RollingRockFunc_Right, 0, ALL_REGISTERS) { WormwouldHelper(gb).rock_rolling(ADDR_RollingRockFunc_Right + 0x13, ADDR_MoveObjectRight_Fast); }
PORT_EX(RollingRockFunc, 0, ALL_REGISTERS) { WormwouldHelper(gb).rock(gb->cpu.h << 8); }
PORT_EX(PalmTreeSpawnerFunc, 0, ALL_REGISTERS) { WormwouldHelper(gb).palm_tree_spawner(); }
PORT_EX(PalmTreeFunc, 0, ALL_REGISTERS) { WormwouldHelper(gb).palm_tree(); }
PORT_LOCAL_EX(PalmTreeFunc, Update, 0, ALL_REGISTERS) { WormwouldHelper(gb).palm_tree_update(); }
