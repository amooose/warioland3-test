// The sun and the moon (sun_moon.asm): they follow Wario about, keeping
// 32 pixels off him (faster off screen), turn to face him, and when he's
// close and none of his buttons is pressed for a while, they shoot a
// flame (the sun) or a star (the moon). The two are the same code over
// different framesets. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

struct Kind {
	uint16_t routine; // SunFunc, MoonFunc
	uint16_t right, left; // facing
	uint16_t attack_right, attack_left;
	uint16_t turn_right, turn_left;
	uint8_t turn_time;
	uint16_t sfx;
	uint16_t shot_right, shot_left;
	uint16_t shot_routine; // SunFlameFunc, MoonStarFunc
	uint16_t shot_gone;
};

constexpr Kind SUN_KIND = {
	ADDR_SunFunc, ADDR_Frameset_68f6b, ADDR_Frameset_68f74, ADDR_Frameset_68f8f, ADDR_Frameset_68f9e,
	ADDR_Frameset_68fc0, ADDR_Frameset_68fb7, 35, SFX_068,
	ADDR_ObjParams_SunFlameRight, ADDR_ObjParams_SunFlameLeft, ADDR_SunFlameFunc, ADDR_Frameset_68fc9,
};
constexpr Kind MOON_KIND = {
	ADDR_MoonFunc, ADDR_Frameset_68fd0, ADDR_Frameset_68fd9, ADDR_Frameset_68ff4, ADDR_Frameset_69003,
	ADDR_Frameset_69036, ADDR_Frameset_6902b, 40, SFX_06E,
	ADDR_ObjParams_MoonStarRight, ADDR_ObjParams_MoonStarLeft, ADDR_MoonStarFunc, ADDR_Frameset_69024,
};

constexpr uint16_t CHECK_DIRECTION = 0x24; // .CheckDirection
constexpr uint16_t UPDATE = 0x58; // .Func_472a6, .Func_475c7
constexpr uint16_t SHOT_UPDATE = 0x20; // .Update

// var1: what it's doing
enum : uint8_t {
	FOLLOWING = 0, // .Func_4731e
	TURN_RIGHT = 1, // .Func_47378
	TURN_LEFT = 2, // .Func_4739c
	TURNING_RIGHT = 3, // .Func_47388
	TURNING_LEFT = 4, // .Func_473ac
	SHOOTING = 5, // .Func_472bb
};

class SunMoon : public Object {
public:
	SunMoon(GB *gb, const Kind &kind) : Object(gb), k_(kind) {}

	void init(); // SunFunc
	void check_direction(); // .CheckDirection
	void update(); // .Func_472a6
	void run(uint8_t action);
	void follow(); // SunMoon_GeneralMovement
	void follow_x(); // SunMoon_XMovement
	void move_x(bool right); // SunMoon_MoveRight, SunMoon_MoveLeft
	void move_down_(); // SunMoon_MoveDown
	void move_up_(); // SunMoon_MoveUp
	void shot(uint16_t movement); // SunFlameFunc, MoonStarFunc
	void shot_gone(); // .Update

private:
	uint16_t wario_x() const { return static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]); }
	void hpos(int dy, int dx);
	void set_following(uint16_t frameset);
	void following();
	void shooting();
	void turn(uint16_t frameset, uint8_t next);
	void turning(uint16_t frameset);

	const Kind &k_;
};

} // namespace

void SunMoon::init()
{
	set_frameset(k_.right);
	set_update(static_cast<uint16_t>(k_.routine + CHECK_DIRECTION));
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-4);
	flags() |= OBJFLAG_NO_COLLISION;
}

void SunMoon::check_direction()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(static_cast<uint16_t>(k_.routine + UPDATE));
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	bool right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
	if (right)
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
	set_frameset(right ? k_.right : k_.left);
	state_duration() = 50;
	var1() = FOLLOWING;
	var2() = 1;
}

void SunMoon::update()
{
	no_actions();
	uint8_t action = var1();
	if (action > SHOOTING) {
		unrealistic();
		return;
	}
	run(action);
}

void SunMoon::run(uint8_t action)
{
	switch (action) {
	case FOLLOWING: following(); return;
	case TURN_RIGHT: turn(k_.turn_right, TURNING_RIGHT); return;
	case TURN_LEFT: turn(k_.turn_left, TURNING_LEFT); return;
	case TURNING_RIGHT: turning(k_.right); return;
	case TURNING_LEFT: turning(k_.left); return;
	case SHOOTING: shooting(); return;
	}
}

void SunMoon::hpos(int dy, int dx)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
}

void SunMoon::set_following(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = 50;
	var1() = FOLLOWING;
}

// shoots when Wario's near, after 50 frames of no buttons
void SunMoon::following()
{
	follow();
	if (mem_[wJoypadPressed] & (0xff ^ (PAD_SELECT | PAD_START)))
		state_duration() = 50;
	if (state_duration()) {
		--state_duration();
		return;
	}
	if (!(flags() & OBJFLAG_ON_SCREEN))
		return;
	uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
	if (dx >= 64 && dx < static_cast<uint8_t>(-64))
		return;
	uint8_t below = static_cast<uint8_t>(static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a) - static_cast<uint8_t>(screen_y() + 0x2a));
	if (below >= 43)
		return;
	state_duration() = 82;
	var1() = SHOOTING;
	var3() = 0;
	set_frameset(facing_right() ? k_.attack_right : k_.attack_left);
}

void SunMoon::turn(uint16_t frameset, uint8_t next)
{
	follow();
	set_frameset(frameset);
	state_duration() = k_.turn_time;
	var1() = next;
}

void SunMoon::turning(uint16_t frameset)
{
	follow();
	if (!--state_duration())
		set_following(frameset);
}

// keeps level with Wario (bobbing, unless on the floor), shooting at
// 52 frames left
void SunMoon::shooting()
{
	follow_x();
	hpos(0, 0);
	mem_[wc1ca] = 0;
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a || mem_[wc1ca]) {
		SET_BC(gb_, ADDR_Data_60690);
		routines_.call(ADDR_ApplyObjYMovement);
	}
	if (!--state_duration()) {
		set_following(facing_right() ? k_.right : k_.left);
		return;
	}
	if (state_duration() != 52)
		return;
	play_sfx(k_.sfx);
	SET_BC(gb_, facing_right() ? k_.shot_right : k_.shot_left);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// 32 pixels off Wario (var2 pixels a frame: 2 off screen), turning to
// him; and towards his height
void SunMoon::follow()
{
	if (!(flags() & OBJFLAG_ON_SCREEN))
		var2() = 2;
	uint16_t wario = wario_x(), obj = x();
	if (wario >= obj) {
		if (wario < static_cast<uint16_t>(obj + 32))
			var2() = 1;
		else
			move_x(true);
		if (!facing_right()) {
			substate() |= OBJSUBFLAG_HDIR;
			var1() = TURN_RIGHT;
		}
	} else {
		if (wario >= static_cast<uint16_t>(obj - 32))
			var2() = 1;
		else
			move_x(false);
		if (facing_right()) {
			substate() &= ~OBJSUBFLAG_HDIR;
			var1() = TURN_LEFT;
		}
	}
	uint16_t level = static_cast<uint16_t>(y() + 0x2a);
	uint16_t wario_y = static_cast<uint16_t>(mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]);
	if (wario_y < level)
		move_up_();
	else if (wario_y > level)
		move_down_();
}

void SunMoon::follow_x()
{
	var2() = (flags() & OBJFLAG_ON_SCREEN) ? 1 : 2;
	uint16_t wario = wario_x(), obj = x();
	if (wario >= obj) {
		if (wario >= static_cast<uint16_t>(obj + 32))
			move_x(true);
	} else if (wario < static_cast<uint16_t>(obj - 32)) {
		move_x(false);
	}
}

// unless a wall's in the way, 12 pixels ahead (at two heights)
void SunMoon::move_x(bool right)
{
	hpos(-6, right ? 12 : -12);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return;
	uint16_t y_pos = static_cast<uint16_t>(y() - 20);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return;
	right ? move_right_by_var2() : move_left_by_var2();
}

void SunMoon::move_down_()
{
	hpos(0, 0);
	mem_[wc1ca] = 0;
	routines_.call(ADDR_Func_352b);
	if (gb_->cpu.a && !mem_[wc1ca])
		return;
	move_down_by_var2();
}

void SunMoon::move_up_()
{
	hpos(-24, 0);
	routines_.call(ADDR_Func_358b);
	if (gb_->cpu.a)
		return;
	move_up_by_var2();
}

// flies along movement until it hits something
void SunMoon::shot(uint16_t movement)
{
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjXMovement);
	flags() |= OBJFLAG_NO_COLLISION;
	if (!state())
		return;
	set_update(static_cast<uint16_t>(k_.shot_routine + SHOT_UPDATE));
	set_frameset(k_.shot_gone);
	state_duration() = 14;
}

void SunMoon::shot_gone()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using wl3::level::SunMoon;

PORT_EX(SunFunc, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).init(); }
PORT_LOCAL_EX(SunFunc, CheckDirection, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).check_direction(); }
PORT_LOCAL_EX(SunFunc, Func_472a6, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).update(); }
PORT_LOCAL_EX(SunFunc, Func_4731e, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).run(0); }
PORT_LOCAL_EX(SunFunc, Func_47378, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).run(1); }
PORT_LOCAL_EX(SunFunc, Func_4739c, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).run(2); }
PORT_LOCAL_EX(SunFunc, Func_47388, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).run(3); }
PORT_LOCAL_EX(SunFunc, Func_473ac, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).run(4); }
PORT_LOCAL_EX(SunFunc, Func_472bb, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).run(5); }
PORT_EX(MoonFunc, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).init(); }
PORT_LOCAL_EX(MoonFunc, CheckDirection, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).check_direction(); }
PORT_LOCAL_EX(MoonFunc, Func_475c7, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).update(); }
PORT_LOCAL_EX(MoonFunc, Func_4763f, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).run(0); }
PORT_LOCAL_EX(MoonFunc, Func_47699, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).run(1); }
PORT_LOCAL_EX(MoonFunc, Func_476bd, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).run(2); }
PORT_LOCAL_EX(MoonFunc, Func_476a9, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).run(3); }
PORT_LOCAL_EX(MoonFunc, Func_476cd, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).run(4); }
PORT_LOCAL_EX(MoonFunc, Func_475dc, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).run(5); }
// the movers are shared: the kind is unused there
PORT_EX(SunMoon_GeneralMovement, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).follow(); }
PORT_EX(SunMoon_XMovement, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).follow_x(); }
PORT_EX(SunMoon_MoveRight, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).move_x(true); }
PORT_EX(SunMoon_MoveLeft, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).move_x(false); }
PORT_EX(SunMoon_MoveDown, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).move_down_(); }
PORT_EX(SunMoon_MoveUp, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).move_up_(); }
PORT_EX(SunFlameFunc_Left, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).shot(ADDR_Data_606e0); }
PORT_EX(SunFlameFunc_Right, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).shot(ADDR_Data_606f0); }
PORT_EX(SunFlameFunc, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).shot(REG_BC(gb)); }
PORT_LOCAL_EX(SunFlameFunc, Update, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::SUN_KIND).shot_gone(); }
PORT_EX(MoonStarFunc_Left, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).shot(ADDR_Data_606e0); }
PORT_EX(MoonStarFunc_Right, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).shot(ADDR_Data_606f0); }
PORT_EX(MoonStarFunc, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).shot(REG_BC(gb)); }
PORT_LOCAL_EX(MoonStarFunc, Update, 0, ALL_REGISTERS) { SunMoon(gb, wl3::level::MOON_KIND).shot_gone(); }
