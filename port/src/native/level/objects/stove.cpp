// The stove (stove.asm): a heavy block Wario can push along the floor;
// it falls off edges (shaking the ground as it lands) and lights Wario
// on fire unless he's already transformed. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_StoveFunc + 0x6f; // .Update

class Stove : public Object {
public:
	using Object::Object;

	void init(); // StoveFunc
	void update(); // .Update

private:
	void set_fall(); // .set_fall
	void fall(); // .Fall
	void set_land(); // .set_land
	void land(); // .Land
	void pushed(bool right); // .State34, .AttackedRightStart
	void start_push(uint8_t state); // .AttackedLeftStart, .State05
	bool ground_at(uint16_t x_pos); // Func_35a3 at x, at its height
};

} // namespace

void Stove::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_update(UPDATE);
	mem_.cur_obj_coll_box_right() = 10;
	mem_.cur_obj_coll_box_left() = static_cast<uint8_t>(-9);
	mem_.cur_obj_coll_box_bottom() = static_cast<uint8_t>(-9);
	set_frameset(ADDR_Frameset_69615);
	var1() = 0;
}

void Stove::update()
{
	switch (state()) {
	case OBJSTATE_FALL: fall(); return;
	case 0: set_fall(); return;
	case OBJSTATE_LAND: land(); return;
	case OBJSTATE_ATTACKED_LEFT: pushed(true); return;
	case OBJSTATE_ATTACKED_LEFT_START: start_push(OBJSTATE_ATTACKED_LEFT); return;
	case OBJSTATE_ATTACKED_RIGHT: pushed(false); return;
	case OBJSTATE_ATTACKED_RIGHT_START: start_push(OBJSTATE_ATTACKED_RIGHT); return;
	case OBJSTATE_VANISH_TOUCH: // .VanishTouch: Wario's set on fire
		if (mem_.transformation()) {
			vanish();
			return;
		}
		state() = OBJSTATE_3A;
		set_frameset(ADDR_Frameset_69625);
		state_duration() = 44;
		return;
	case OBJSTATE_3A:
		if (!--state_duration())
			set_land();
		return;
	default: set_fall(); return;
	}
}

void Stove::set_fall()
{
	state() = OBJSTATE_FALL;
	var3() = 0;
	set_frameset(ADDR_Frameset_69615);
}

// shaking the ground on landing, unless it's just been placed
void Stove::fall()
{
	apply_y_movement(ADDR_Data_605e0);
	mem_.y_pos_lo() = mem_.cur_obj_y().lo();
	mem_.y_pos_hi() = mem_.cur_obj_y().hi();
	mem_.x_pos_lo() = mem_.cur_obj_x().lo();
	mem_.x_pos_hi() = mem_.cur_obj_x().hi();
	if (!hpos_block_unk1())
		return;
	mem_.cur_obj_y().lo() = mem_.y_pos_lo();
	mem_.cur_obj_y().hi() = mem_.y_pos_hi();
	state() = OBJSTATE_LAND;
	if (!var1())
		return;
	play_sfx(SFX_063);
	ground_shake(24);
}

void Stove::set_land()
{
	state() = OBJSTATE_LAND;
	set_frameset(ADDR_Frameset_69615);
}

void Stove::start_push(uint8_t s)
{
	state() = s;
	var2() = 0x02;
	set_frameset(ADDR_Frameset_69618);
}

// pushed var2 pixels a frame, slowing every 8 frames, until it stops or
// meets a wall
void Stove::pushed(bool right)
{
	if (!(mem_.global_counter() & 0b111)) {
		play_sfx(SFX_078);
		if (!--var2()) {
			set_land();
			return;
		}
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 10) : static_cast<uint16_t>(x() - 9);
	mem_.y_pos_lo() = static_cast<uint8_t>(y_pos);
	mem_.y_pos_hi() = static_cast<uint8_t>(y_pos >> 8);
	mem_.x_pos_lo() = static_cast<uint8_t>(x_pos);
	mem_.x_pos_hi() = static_cast<uint8_t>(x_pos >> 8);
	if (hpos_block_unk4() & 0x0f) {
		set_land();
		return;
	}
	right ? move_right_by_var2() : move_left_by_var2();
}

bool Stove::ground_at(uint16_t x_pos)
{
	mem_.x_pos_lo() = static_cast<uint8_t>(x_pos);
	mem_.x_pos_hi() = static_cast<uint8_t>(x_pos >> 8);
	return hpos_block_unk2();
}

// on the floor; with nothing under either side, it'll fall (and shake
// the ground when it lands)
void Stove::land()
{
	mem_.y_pos_lo() = mem_.cur_obj_y().lo();
	mem_.y_pos_hi() = mem_.cur_obj_y().hi();
	mem_.x_pos_lo() = mem_.cur_obj_x().lo();
	mem_.x_pos_hi() = mem_.cur_obj_x().hi();
	bool on_floor = hpos_block_down() == 0x10;
	mem_.cur_obj_y().lo() = mem_.y_pos_lo();
	mem_.cur_obj_y().hi() = mem_.y_pos_hi();
	if (on_floor)
		return;
	if (ground_at(static_cast<uint16_t>(x() - 7)))
		return;
	if (ground_at(static_cast<uint16_t>(x() + 8)))
		return;
	state() = OBJSTATE_00;
	var1() = 1;
}

} // namespace wl3::level

using Stove = wl3::level::Stove;

PORT_EX(StoveFunc, 0, ALL_REGISTERS) { Stove(gb).init(); }
PORT_LOCAL_EX(StoveFunc, Update, 0, ALL_REGISTERS) { Stove(gb).update(); }
