// The coins (coin.asm): spun out of a block or an enemy, a coin hops
// along (by one of six arcs), falls, slides and bounces off walls, then
// rests a while and flickers out. One coin in 64 is a colour coin. The
// frameset pointer's bytes count the spin's ticks and frames (it spins
// slower as it ages, and at half speed in water). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_CoinFunc + 0x2f; // .Update

// a hop: its state, frames, and y velocities (by var3, ended by $80)
struct Hop {
	uint8_t state;
	uint8_t duration;
	uint16_t velocities;
	bool right;
};

constexpr Hop HOPS[] = {
	{0x0a, 6, ADDR_Data_60300, false}, // .Move1
	{0x02, 4, ADDR_Data_602c0, false}, // .Move2
	{0x06, 5, ADDR_Data_602e0, false}, // .Move3
	{0x0b, 6, ADDR_Data_60300, true}, // .Move4
	{0x03, 4, ADDR_Data_602c0, true}, // .Move5
	{0x07, 5, ADDR_Data_602e0, true}, // .Move6
};

// the state table's hops: (hop, starting) by state 0 to $b
constexpr struct {
	uint8_t hop;
	bool start;
} HOP_STATES[] = {
	{1, true}, {4, true}, {1, false}, {4, false}, // $00: Move2Start, Move5Start, Move2, Move5
	{2, true}, {5, true}, {2, false}, {5, false}, // $04: Move3Start, Move6Start, Move3, Move6
	{0, true}, {3, true}, {0, false}, {3, false}, // $08: Move1Start, Move4Start, Move1, Move4
};

class Coin : public Object {
public:
	using Object::Object;

	void init(); // CoinFunc
	void colour_coin(); // .ColourCoin
	void update(); // .Update
	void run(uint8_t s); // the state table
	void hop(int index, bool start); // .Move1Start ... .Move6
	void fall(bool right, bool start); // .FallLeftStart ... .FallRight
	void slide(bool right); // .State10, .State11
	void rest(uint8_t next); // .State12, .State16
	void resting(); // .State13
	void flicker(); // .State17
	void bounce(bool start); // .State14, .State15
	void start_moving(); // .StartMovement
	void set_up(); // .Init

private:
	void spin();
	void velocity(uint16_t table); // y by the next velocity (saved in hffa0)
	void pos_to_hram(int dy, int dx);
	void in_water();
};

} // namespace

void Coin::init()
{
	if (static_cast<uint8_t>(mem_[wGlobalCounter] - 1) & 0x3f) {
		frame() = 0;
		set_update(UPDATE);
		flags() |= OBJFLAG_NO_COLLISION;
		update();
		return;
	}
	set_interaction(OBJ_INTERACTION_COLOUR_COIN);
	colour_coin();
}

void Coin::colour_coin()
{
	play_sfx(SFX_08E);
	frame() = 8;
	set_update(UPDATE);
	flags() |= OBJFLAG_NO_COLLISION;
	update();
}

void Coin::update()
{
	frame_duration() = 2;
	// the frameset offset is set in water: half speed
	if (!mem_[wCurObjFramesetOffset] || !(mem_[wGlobalCounter] & 1))
		spin();
	else
		return;
	uint8_t s = state();
	if (s > 0x1a) {
		unrealistic();
		return;
	}
	run(s);
}

// a frame every 1 to 4 ticks, slower with age (var1, up to 150)
void Coin::spin()
{
	uint8_t ticks;
	if (var1() >= 150) {
		ticks = 4;
	} else {
		uint8_t age = ++var1();
		ticks = age >= 100 ? 3 : age >= 50 ? 2 : 1;
	}
	Byte tick = mem_[wCurObjFramesetPtr + 0];
	Byte frames = mem_[wCurObjFramesetPtr + 1];
	if (--tick)
		return;
	tick = ticks;
	if (--frames) {
		++frame();
		return;
	}
	frames = 8;
	frame() = static_cast<uint8_t>(frame() - 7);
}

void Coin::run(uint8_t s)
{
	if (s < 0x0c) {
		hop(HOP_STATES[s].hop, HOP_STATES[s].start);
		return;
	}
	switch (s) {
	case 0x0c: fall(false, true); return;
	case 0x0d: fall(true, true); return;
	case 0x0e: fall(false, false); return;
	case 0x0f: fall(true, false); return;
	case 0x10: slide(false); return;
	case 0x11: slide(true); return;
	case 0x12: rest(0x13); return;
	case 0x13: resting(); return;
	case 0x14: bounce(true); return;
	case 0x15: bounce(false); return;
	case 0x16: rest(0x17); return;
	case 0x17: flicker(); return;
	case 0x18: flags() = 0; return;
	case 0x19: set_up(); return;
	case 0x1a: start_moving(); return;
	}
}

void Coin::set_up()
{
	mem_[wCurObjAction] = 15 | (1 << 7);
	state() = 0x1a;
	flags() |= OBJFLAG_NO_COLLISION;
}

// hops away from Wario
void Coin::start_moving()
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	state() = static_cast<uint8_t>(screen_x() + 0x2a) < wario ? 0x06 : 0x07;
}

void Coin::velocity(uint16_t table)
{
	uint8_t c = static_cast<uint8_t>((table & 0xff) + var3());
	uint8_t v = mem_[static_cast<uint16_t>((table & 0xff00) | c)];
	if (v == 0x80)
		v = mem_[static_cast<uint16_t>((table & 0xff00) | static_cast<uint8_t>(c - 1))];
	else
		++var3();
	mem_[hffa0] = v;
	y() = static_cast<uint16_t>(y() + static_cast<int8_t>(v));
}

void Coin::pos_to_hram(int dy, int dx)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
}

// an arc for its frames, then falling; a wall turns it back
void Coin::hop(int index, bool start)
{
	const Hop &h = HOPS[index];
	if (start) {
		state() = h.state;
		var3() = 0;
		state_duration() = h.duration;
	}
	velocity(h.velocities);
	h.right ? move_right_by_var2() : move_left_by_var2();
	pos_to_hram(-8, h.right ? 3 : -4);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a & 0x0f) {
		state() = h.right ? 0x0c : 0x0d;
		return;
	}
	if (!--state_duration())
		state() = h.right ? 0x0d : 0x0c;
}

void Coin::in_water()
{
	if (uint8_t water = mem_[wIsInWaterOrSand])
		mem_[wCurObjFramesetOffset] = water;
}

// until it lands, then hops again by how fast it fell (or slides)
void Coin::fall(bool right, bool start)
{
	if (start) {
		state() = right ? 0x0f : 0x0e;
		var3() = 0;
	}
	right ? move_right_by_var2() : move_left_by_var2();
	velocity(ADDR_FallingYVel_Light);
	pos_to_hram(0, 0);
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		in_water();
		pos_to_hram(-4, right ? 3 : -4);
		routines_.call(ADDR_Func_3513);
		if (gb_->cpu.a & 0x0f)
			state() = right ? 0x0c : 0x0d;
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	if (var2())
		--var2();
	uint8_t speed = mem_[hffa0];
	if (speed < 2)
		state() = right ? 0x11 : 0x10;
	else if (speed < 3)
		state() = right ? 0x01 : 0x00;
	else if (speed < 4)
		state() = right ? 0x05 : 0x04;
	else
		state() = right ? 0x09 : 0x08;
}

// var2 pixels a frame, slowing every 32 frames; turns at walls, falls
// off edges, rests once stopped
void Coin::slide(bool right)
{
	Byte speed = var2();
	if (!speed) {
		state() = 0x12;
		return;
	}
	if (!(mem_[wGlobalCounter] & 0x1f))
		--speed;
	pos_to_hram(-4, right ? 3 : -4);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a & 0x0f) {
		state() = right ? 0x10 : 0x11;
		return;
	}
	right ? move_right_by_var2() : move_left_by_var2();
	pos_to_hram(0, 0);
	routines_.call(ADDR_Func_3543);
	bool on_floor = gb_->cpu.a == 0x10;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	if (on_floor)
		return;
	for (int dx : {right ? 3 : -4, right ? -4 : 3}) {
		uint16_t x_pos = static_cast<uint16_t>(x() + dx);
		mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
		mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
		routines_.call(ADDR_Func_35a3);
		if (gb_->cpu.a)
			return;
	}
	state() = right ? 0x0d : 0x0c;
}

// 100 frames (50 in water)
void Coin::rest(uint8_t next)
{
	state() = next;
	state_duration() = mem_[wCurObjFramesetOffset] ? 50 : 100;
}

void Coin::resting()
{
	if (!--state_duration()) {
		state() = 0x16;
		return;
	}
	pos_to_hram(0, 0);
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a)
		state() = 0x14; // the floor's gone
}

// hidden every 4th frame, then gone
void Coin::flicker()
{
	Byte duration = state_duration();
	if (!duration) {
		flags() = 0;
		return;
	}
	if (--duration & 0b11)
		flags() &= ~OBJFLAG_INVISIBLE;
	else
		flags() = (flags() & ~OBJFLAG_ON_SCREEN) | OBJFLAG_INVISIBLE;
}

// falls (by the velocities at $4040, in whatever bank's mapped) until it
// lands, then rests
void Coin::bounce(bool start)
{
	if (start) {
		state() = 0x15;
		var3() = 0;
	}
	velocity(0x4040);
	pos_to_hram(0, 0);
	routines_.call(ADDR_Func_352b);
	if (gb_->cpu.a) {
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		state() = 0x12;
	}
	in_water();
}

} // namespace wl3::level

using Coin = wl3::level::Coin;

PORT_EX(CoinFunc, 0, ALL_REGISTERS) { Coin(gb).init(); }
PORT_LOCAL_EX(CoinFunc, ColourCoin, 0, ALL_REGISTERS) { Coin(gb).colour_coin(); }
PORT_LOCAL_EX(CoinFunc, Update, 0, ALL_REGISTERS) { Coin(gb).update(); }
PORT_LOCAL_EX(CoinFunc, Init, 0, ALL_REGISTERS) { Coin(gb).set_up(); }
PORT_LOCAL_EX(CoinFunc, StartMovement, 0, ALL_REGISTERS) { Coin(gb).start_moving(); }
PORT_LOCAL_EX(CoinFunc, Move1Start, 0, ALL_REGISTERS) { Coin(gb).hop(0, true); }
PORT_LOCAL_EX(CoinFunc, Move1, 0, ALL_REGISTERS) { Coin(gb).hop(0, false); }
PORT_LOCAL_EX(CoinFunc, Move2Start, 0, ALL_REGISTERS) { Coin(gb).hop(1, true); }
PORT_LOCAL_EX(CoinFunc, Move2, 0, ALL_REGISTERS) { Coin(gb).hop(1, false); }
PORT_LOCAL_EX(CoinFunc, Move3Start, 0, ALL_REGISTERS) { Coin(gb).hop(2, true); }
PORT_LOCAL_EX(CoinFunc, Move3, 0, ALL_REGISTERS) { Coin(gb).hop(2, false); }
PORT_LOCAL_EX(CoinFunc, Move4Start, 0, ALL_REGISTERS) { Coin(gb).hop(3, true); }
PORT_LOCAL_EX(CoinFunc, Move4, 0, ALL_REGISTERS) { Coin(gb).hop(3, false); }
PORT_LOCAL_EX(CoinFunc, Move5Start, 0, ALL_REGISTERS) { Coin(gb).hop(4, true); }
PORT_LOCAL_EX(CoinFunc, Move5, 0, ALL_REGISTERS) { Coin(gb).hop(4, false); }
PORT_LOCAL_EX(CoinFunc, Move6Start, 0, ALL_REGISTERS) { Coin(gb).hop(5, true); }
PORT_LOCAL_EX(CoinFunc, Move6, 0, ALL_REGISTERS) { Coin(gb).hop(5, false); }
PORT_LOCAL_EX(CoinFunc, FallLeftStart, 0, ALL_REGISTERS) { Coin(gb).fall(false, true); }
PORT_LOCAL_EX(CoinFunc, FallLeft, 0, ALL_REGISTERS) { Coin(gb).fall(false, false); }
PORT_LOCAL_EX(CoinFunc, FallRightStart, 0, ALL_REGISTERS) { Coin(gb).fall(true, true); }
PORT_LOCAL_EX(CoinFunc, FallRight, 0, ALL_REGISTERS) { Coin(gb).fall(true, false); }
PORT_LOCAL_EX(CoinFunc, State10, 0, ALL_REGISTERS) { Coin(gb).slide(false); }
PORT_LOCAL_EX(CoinFunc, State11, 0, ALL_REGISTERS) { Coin(gb).slide(true); }
PORT_LOCAL_EX(CoinFunc, State12, 0, ALL_REGISTERS) { Coin(gb).rest(0x13); }
PORT_LOCAL_EX(CoinFunc, State13, 0, ALL_REGISTERS) { Coin(gb).resting(); }
PORT_LOCAL_EX(CoinFunc, State14, 0, ALL_REGISTERS) { Coin(gb).bounce(true); }
PORT_LOCAL_EX(CoinFunc, State15, 0, ALL_REGISTERS) { Coin(gb).bounce(false); }
PORT_LOCAL_EX(CoinFunc, State16, 0, ALL_REGISTERS) { Coin(gb).rest(0x17); }
PORT_LOCAL_EX(CoinFunc, State17, 0, ALL_REGISTERS) { Coin(gb).flicker(); }
PORT_LOCAL_EX(CoinFunc, State18, 0, ALL_REGISTERS) { wl3::Memory mem(gb); mem[wCurObjFlags] = 0; }
