// The Octohons (octohon.asm): octopuses swimming back and forth by the
// steps of Data_4a5a9 (var2 its index; $ee steps make a lap). The small
// one pauses at each end; the big one charges at Wario when he's level
// with it and close ahead, and is knocked back by a wall or a hit. See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SMALL_START = ADDR_SmallOctohonFunc + 0x1f; // .Func_4a4d4
constexpr uint16_t SMALL_SWIM = ADDR_SmallOctohonFunc + 0x5a; // .Func_4a50f
constexpr uint16_t BIG_START = ADDR_BigOctohonFunc + 0x1f; // .Func_4a6b9
constexpr uint16_t BIG_SWIM = ADDR_BigOctohonFunc + 0x8f; // .Func_4a729

constexpr uint8_t LAP = 0xee; // var2 at the end of a lap

class Octohon : public Object {
public:
	using Object::Object;

	void small(); // SmallOctohonFunc
	void small_start(); // .Func_4a4d4
	void small_swim(); // .Func_4a50f
	void big(); // BigOctohonFunc
	void big_start(); // .Func_4a6b9
	void big_swim(); // .Func_4a729

private:
	bool wario_to_right() const;
	void set_up(uint16_t update, uint8_t right, uint8_t left, uint8_t bottom);
	void small_lap(uint16_t frameset, bool right); // .asm_4a4f7
	void big_lap(uint16_t frameset); // .asm_4a6df
	void step(bool right); // a step of Data_4a5a9
	bool wall_at(int dx); // Func_3513, dx pixels along
	void knocked_back(uint16_t zero_addr, uint16_t frameset); // .asm_4a6eb, .asm_4a6f0
	void big_turn(); // .asm_4a860
	void big_swim_on(); // .asm_4a823
	void big_charge(); // .asm_4a78b
};

} // namespace

bool Octohon::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void Octohon::set_up(uint16_t update, uint8_t right, uint8_t left, uint8_t bottom)
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(update);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = right;
	mem_[wCurObjCollBoxLeft] = left;
	mem_[wCurObjCollBoxBottom] = bottom;
}

void Octohon::small() { set_up(SMALL_START, 0x00, 0x00, 0x00); }
void Octohon::big() { set_up(BIG_START, 0x08, 0xf7, 0x08); }

// var2 steps of Data_4a5a9 along (back to its start at the $80)
void Octohon::step(bool right)
{
	Byte index = var2();
	uint8_t speed = mem_[static_cast<uint16_t>(ADDR_Data_4a5a9 + index)];
	if (speed == 0x80) {
		index = 0;
		speed = mem_[ADDR_Data_4a5a9];
	}
	++index;
	x() = static_cast<uint16_t>(right ? x() + speed : x() - speed);
}

bool Octohon::wall_at(int dx)
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	return gb_->cpu.a;
}

// facing away from Wario (var1 the way it swims), at random flipped
void Octohon::small_start()
{
	if (wario_to_right())
		small_lap(ADDR_Frameset_69b38, 1);
	else
		small_lap(ADDR_Frameset_69b2f, 0);
	set_update(SMALL_SWIM);
}

void Octohon::small_lap(uint16_t frameset, bool right)
{
	state_duration() = 0xac;
	var1() = right;
	var2() = 0;
	if (mem_[rDIV] & 1)
		flags() ^= 0x80;
	set_frameset(frameset);
}

// a pause (the state duration), then a lap; then the other way
void Octohon::small_swim()
{
	mem_[wCurObjAction] = 0x81;
	if (var1() > 1)
		var1() = 0;
	bool right = var1();
	if (state_duration()) {
		if (!--state_duration())
			set_frameset(right ? ADDR_Frameset_69b4c : ADDR_Frameset_69b41);
	} else if (var2() == LAP) {
		small_lap(right ? ADDR_Frameset_69b2f : ADDR_Frameset_69b38, !right);
		return;
	}
	if (var2() && wall_at(0)) { // stuck: skips a step
		++var2();
		return;
	}
	step(right);
}

// facing Wario
void Octohon::big_start()
{
	if (wario_to_right()) {
		substate() |= OBJSUBFLAG_HDIR;
		set_update(BIG_SWIM);
		big_lap(ADDR_Frameset_69b60);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_update(BIG_SWIM);
		big_lap(ADDR_Frameset_69b57);
	}
}

void Octohon::big_lap(uint16_t frameset)
{
	state_duration() = 0xac;
	var1() = 0;
	var2() = 0;
	set_frameset(frameset);
}

// knocked back (by a hit, or a wall when charging); the asm clears the
// byte at hl first: the state, or wherever Func_3513 left hl
void Octohon::knocked_back(uint16_t zero_addr, uint16_t frameset)
{
	mem_[zero_addr] = 0;
	set_frameset(frameset);
	state_duration() = 0x43;
	var1() = 4;
	var2() = 0;
	set_interaction(OBJ_INTERACTION_06);
	play_sfx(SFX_08D);
}

void Octohon::big_turn()
{
	substate() ^= 0x80;
	big_lap(facing_right() ? ADDR_Frameset_69b60 : ADDR_Frameset_69b57);
}

// a step of its lap, unless at a wall
void Octohon::big_swim_on()
{
	bool right = facing_right();
	if (wall_at(right ? 6 : -6)) {
		++var2();
		return;
	}
	step(right);
}

void Octohon::big_charge()
{
	bool right = facing_right();
	if (!wall_at(right ? 6 : -6)) {
		routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
		return;
	}
	knocked_back(REG_HL(gb_), right ? ADDR_Frameset_69b74 : ADDR_Frameset_69b69);
}

void Octohon::big_swim()
{
	switch (state()) {
	case 0: break;
	case 0x0b: knocked_back(wCurObjState, ADDR_Frameset_69b69); return;
	case 0x0c: knocked_back(wCurObjState, ADDR_Frameset_69b74); return;
	case 0x0a: vanish(); return;
	default: // .asm_4a711: stunned
		state() = 0;
		set_frameset(facing_right() ? ADDR_Frameset_69b88 : ADDR_Frameset_69b7f);
		state_duration() = 0x3c;
		var1() = 3;
		return;
	}
	switch (var1()) {
	case 0: // swimming its lap; charges when Wario's level and close ahead
		if (!state_duration()) {
			if (var2() == LAP)
				big_turn();
			else
				big_swim_on();
			return;
		}
		if (!--state_duration()) {
			set_frameset(facing_right() ? ADDR_Frameset_69b74 : ADDR_Frameset_69b69);
			big_swim_on();
			return;
		}
		{
			uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
			if (dy < 0x14 || dy >= 0xec) {
				uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
				bool right = facing_right();
				if (right ? dx >= 0xc0 : dx < 0x40) {
					set_frameset(right ? ADDR_Frameset_69bce : ADDR_Frameset_69bc3);
					state_duration() = 0x24;
					var1() = 1;
					set_interaction(OBJ_INTERACTION_49);
					return;
				}
			}
		}
		big_swim_on();
		return;
	case 1: // winding up, then charging
		if (state_duration()) {
			--state_duration();
			return;
		}
		frame_duration() = 2;
		big_charge();
		return;
	case 3: // stunned
		if (--state_duration()) {
			big_charge();
			return;
		}
		big_lap(facing_right() ? ADDR_Frameset_69b60 : ADDR_Frameset_69b57);
		return;
	case 4: // knocked back
		SET_BC(gb_, ADDR_Data_60d50);
		routines_.call(ADDR_ApplyObjXMovement);
		[[fallthrough]];
	case 2:
		mem_[wCurObjAction] = 0x81;
		if (!--state_duration())
			big_turn();
		return;
	}
}

} // namespace wl3::level

using Octohon = wl3::level::Octohon;

PORT_EX(SmallOctohonFunc, 0, ALL_REGISTERS) { Octohon(gb).small(); }
PORT_LOCAL_EX(SmallOctohonFunc, Func_4a4d4, 0, ALL_REGISTERS) { Octohon(gb).small_start(); }
PORT_LOCAL_EX(SmallOctohonFunc, Func_4a50f, 0, ALL_REGISTERS) { Octohon(gb).small_swim(); }
PORT_EX(BigOctohonFunc, 0, ALL_REGISTERS) { Octohon(gb).big(); }
PORT_LOCAL_EX(BigOctohonFunc, Func_4a6b9, 0, ALL_REGISTERS) { Octohon(gb).big_start(); }
PORT_LOCAL_EX(BigOctohonFunc, Func_4a729, 0, ALL_REGISTERS) { Octohon(gb).big_swim(); }
