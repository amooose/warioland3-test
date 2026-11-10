// The water drops (water_drop.asm): each falls from where it started
// until it lands (or meets water), splashes, and rises back unseen to
// fall again. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_WaterDropFunc + 0x1f; // .Func_49d83

constexpr uint8_t FALLING = 0x00;
constexpr uint8_t SPLASHING = 0x5a;
constexpr uint8_t RISING = 0x5b;

class WaterDrop : public Object {
public:
	using Object::Object;

	void init(); // WaterDropFunc
	void update(); // .Func_49d83

private:
	void reset(); // .asm_49ddd: a new drop, its start in var1 and var2
	void fall(); // .asm_49df7
	void splash(); // .asm_49d92
	void splashing(); // .asm_49da6
	void rising(); // .asm_49dbd
};

} // namespace

void WaterDrop::init()
{
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_.cur_obj_coll_box_right() = 0x04;
	mem_.cur_obj_coll_box_left() = 0xfa;
	mem_.cur_obj_coll_box_bottom() = 0xff;
	flags() |= OBJFLAG_NO_COLLISION;
	reset();
}

void WaterDrop::reset()
{
	set_frameset(ADDR_Frameset_69761);
	state_duration() = 0x41;
	var1() = mem_.cur_obj_y().lo();
	var2() = mem_.cur_obj_y().hi();
	var3() = 0;
	state() = FALLING;
	flags() &= ~OBJFLAG_INVISIBLE;
}

void WaterDrop::update()
{
	switch (state()) {
	case FALLING: fall(); return;
	case SPLASHING: splashing(); return;
	case RISING: rising(); return;
	default: splash(); return; // hit by something
	}
}

// forming for a while, then down until it lands
void WaterDrop::fall()
{
	if (state_duration()) {
		if (!--state_duration())
			set_frameset(ADDR_Frameset_69772);
		return;
	}
	mem_.y_pos_lo() = mem_.cur_obj_y().lo();
	mem_.y_pos_hi() = mem_.cur_obj_y().hi();
	mem_.x_pos_lo() = mem_.cur_obj_x().lo();
	mem_.x_pos_hi() = mem_.cur_obj_x().hi();
	if (!hpos_block_unk1() && !mem_.is_in_water_or_sand()) {
		apply_y_movement(ADDR_FallingYVel_Light);
		return;
	}
	mem_.cur_obj_y().lo() = mem_.y_pos_lo();
	mem_.cur_obj_y().hi() = mem_.y_pos_hi();
	splash();
}

void WaterDrop::splash()
{
	state() = SPLASHING;
	set_frameset(ADDR_Frameset_69775);
	state_duration() = 0x18;
	play_sfx(SFX_0A1);
	splashing();
}

void WaterDrop::splashing()
{
	no_actions();
	if (--state_duration())
		return;
	flags() |= OBJFLAG_INVISIBLE;
	state() = RISING;
	state_duration() = 0x3c;
	rising();
}

// back up to its start (and a wait there)
void WaterDrop::rising()
{
	no_actions();
	uint8_t wait = state_duration();
	if (wait)
		--state_duration();
	if (mem_.cur_obj_y().lo() != var1() || mem_.cur_obj_y().hi() != var2()) {
		move_up();
		return;
	}
	if (!wait)
		reset();
}

} // namespace wl3::level

using WaterDrop = wl3::level::WaterDrop;

PORT_EX(WaterDropFunc, 0, ALL_REGISTERS) { WaterDrop(gb).init(); }
PORT_LOCAL_EX(WaterDropFunc, Func_49d83, 0, ALL_REGISTERS) { WaterDrop(gb).update(); }
