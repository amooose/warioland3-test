// Togeba (togeba.asm): a spiked shell that pops out its spikes when hit.
// A ground pound's shake flips it over: it falls, sinks slowly (carrying
// Wario down if he stands on it) and rights itself on reaching a wall.
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t RESET = ADDR_TogebaFunc + 0x11; // .Func_48d9b
constexpr uint16_t IDLE = ADDR_TogebaFunc + 0x2d; // .Func_48db7
constexpr uint16_t FLIPPED = ADDR_TogebaFunc + 0x78; // .Func_48e02
constexpr uint16_t RIGHTING = ADDR_TogebaFunc + 0xb4; // .Func_48e3e
constexpr uint16_t SINKING = ADDR_TogebaFunc + 0xbc; // .Func_48e46
constexpr uint16_t RISING = ADDR_TogebaFunc + 0x142; // .Func_48ecc

class Togeba : public Object {
public:
	using Object::Object;

	void init(); // TogebaFunc
	void reset(); // .Func_48d9b
	void idle(); // .Func_48db7
	void flipped(); // .Func_48e02
	void right_itself(); // .Func_48e2b
	void righting(); // .Func_48e3e
	void sinking(); // .Func_48e46
	void rising(); // .Func_48ecc

private:
	bool wall(uint16_t x_pos);
	void sink();
};

} // namespace

void Togeba::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_.cur_obj_coll_box_right() = 0x0b;
	mem_.cur_obj_coll_box_left() = 0xf4;
	reset();
}

void Togeba::reset()
{
	set_frameset(ADDR_Frameset_69230);
	state_duration() = 0;
	state() = 0;
	set_update(IDLE);
	set_interaction(OBJ_INTERACTION_FULL_STING);
}

// spikes out for a while when hit; flipped by a strong ground shake
void Togeba::idle()
{
	if (mem_.ground_shake_counter() >= 0x10) {
		set_update(FLIPPED);
		set_frameset(ADDR_Frameset_69256);
		state_duration() = 0x30;
		set_interaction(OBJ_INTERACTION_0B);
		var3() = 0;
		return;
	}
	if (state_duration()) {
		if (!--state_duration())
			set_frameset(ADDR_Frameset_69230);
		return;
	}
	if (!state())
		return;
	state() = 0;
	set_frameset(ADDR_Frameset_6924b);
	state_duration() = 0x16;
	flags() &= ~OBJFLAG_NO_COLLISION;
}

// bounced up and down, then walkable
void Togeba::flipped()
{
	apply_y_movement(ADDR_Data_60760);
	if (--state_duration())
		return;
	set_update(SINKING);
	set_frameset(ADDR_Frameset_69263);
	var3() = 0;
	state() = 0;
	set_interaction(OBJ_INTERACTION_WALKABLE);
}

void Togeba::right_itself()
{
	set_update(RIGHTING);
	set_frameset(ADDR_Frameset_69275);
	state_duration() = 0x10;
}

void Togeba::righting()
{
	if (!--state_duration())
		reset();
}

bool Togeba::wall(uint16_t x_pos)
{
	mem_.x_pos_lo() = static_cast<uint8_t>(x_pos);
	mem_.x_pos_hi() = static_cast<uint8_t>(x_pos >> 8);
	return hpos_block_unk1();
}

// a pixel down every other frame, with Wario on it
void Togeba::sinking()
{
	mem_.y_pos_lo() = mem_.cur_obj_y().lo();
	mem_.y_pos_hi() = mem_.cur_obj_y().hi();
	if (wall(static_cast<uint16_t>(x() - 0x0c)) || wall(static_cast<uint16_t>(x() + 0x0b))) {
		right_itself();
		return;
	}
	if (state_duration() && !--state_duration())
		set_frameset(ADDR_Frameset_69263);
	uint8_t s = state();
	if (s == 0x0a) { // pounded
		state() = 0;
		set_frameset(ADDR_Frameset_6926c);
		state_duration() = 0x1b;
	} else if (s) { // hit from below: up a bit
		state() = 0;
		set_frameset(ADDR_Frameset_6927c);
		state_duration() = 0x0a;
		if (mem_.cur_obj_unk1d() & 0x80) {
			set_update(RISING);
			return;
		}
	}
	sink();
}

void Togeba::sink()
{
	if (!(mem_.global_counter() & 1))
		return;
	if (flags() & OBJFLAG_STEPPED) {
		flags() &= ~OBJFLAG_STEPPED;
		move_wario(ADDR_MoveWarioDown, 0x01);
		home::handle_downwards_floor_transition(gb_);
	}
	move_down();
}

void Togeba::rising()
{
	call_home(ADDR_MoveObjectUp_Fast);
	if (--state_duration())
		return;
	set_update(SINKING);
	set_frameset(ADDR_Frameset_69263);
}

} // namespace wl3::level

using Togeba = wl3::level::Togeba;

PORT_EX(TogebaFunc, 0, ALL_REGISTERS) { Togeba(gb).init(); }
PORT_LOCAL_EX(TogebaFunc, Func_48d9b, 0, ALL_REGISTERS) { Togeba(gb).reset(); }
PORT_LOCAL_EX(TogebaFunc, Func_48db7, 0, ALL_REGISTERS) { Togeba(gb).idle(); }
PORT_LOCAL_EX(TogebaFunc, Func_48e02, 0, ALL_REGISTERS) { Togeba(gb).flipped(); }
PORT_LOCAL_EX(TogebaFunc, Func_48e2b, 0, ALL_REGISTERS) { Togeba(gb).right_itself(); }
PORT_LOCAL_EX(TogebaFunc, Func_48e3e, 0, ALL_REGISTERS) { Togeba(gb).righting(); }
PORT_LOCAL_EX(TogebaFunc, Func_48e46, 0, ALL_REGISTERS) { Togeba(gb).sinking(); }
PORT_LOCAL_EX(TogebaFunc, Func_48ecc, 0, ALL_REGISTERS) { Togeba(gb).rising(); }
