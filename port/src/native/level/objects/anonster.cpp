// Anonster (anonster.asm): the spider boss. It walks along the ceiling,
// dropping silk strands that float down and settle (Wario can throw them
// at it), drops to the ground, makes waves and rushes; hit from behind it
// climbs back up. And its silk strands, its waves, and the silk platforms
// left when it's beaten. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t A(uint16_t offset) { return static_cast<uint16_t>(ADDR_AnonsterFunc + offset); }
constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_AnonsterSilkFunc + offset); }

constexpr uint16_t ENTER = A(0x24); // .Func_50024
constexpr uint16_t HIDDEN = A(0x5e); // .Func_5005e
constexpr uint16_t WALK = A(0x7f); // .Func_5007f
constexpr uint16_t SILK_TIME = A(0x16f); // .Func_5016f
constexpr uint16_t DROP_SILK1 = A(0x1c4); // .Func_501c4
constexpr uint16_t DROP_SILK2 = A(0x200); // .Func_50200
constexpr uint16_t DROP_SILK3 = A(0x23d); // .Func_5023d
constexpr uint16_t CLIMB_START = A(0x282); // .Func_50282
constexpr uint16_t CLIMB = A(0x293); // .Func_50293
constexpr uint16_t RUN_TO_WARIO = A(0x2d1); // .Func_502d1
constexpr uint16_t DROP = A(0x34c); // .Func_5034c
constexpr uint16_t LAND = A(0x397); // .Func_50397
constexpr uint16_t GROUND = A(0x41d); // .Func_5041d
constexpr uint16_t HURT = A(0x5be); // .Func_505be
constexpr uint16_t HURT2 = A(0x5f4); // .Func_505f4
constexpr uint16_t HURT3 = A(0x610); // .Func_50610
constexpr uint16_t CLIMB_BACK = A(0x635); // .Func_50635
constexpr uint16_t TANGLED = A(0x650); // .Func_50650
constexpr uint16_t TANGLED2 = A(0x66c); // .Func_5066c
constexpr uint16_t DEFEATED = A(0x689); // .Func_50689
constexpr uint16_t FALL_AWAY = A(0x705); // .Func_50705

constexpr uint16_t PLATFORMS_CREATE3 = ADDR_SilkPlatformsFunc + 0x16; // .CreatePlatform3
constexpr uint16_t PLATFORMS_CREATE2 = ADDR_SilkPlatformsFunc + 0x1e; // .CreatePlatform2
constexpr uint16_t PLATFORMS_CREATE1 = ADDR_SilkPlatformsFunc + 0x26; // .CreatePlatform1
constexpr uint16_t PLATFORMS_WAIT = ADDR_SilkPlatformsFunc + 0x47; // .Wait

constexpr uint16_t FLOAT_LEFT = L(0x0); // .FloatLeft
constexpr uint16_t FLOAT_RIGHT = L(0x3f); // .FloatRight
constexpr uint16_t FALL_OFF = L(0x11f); // .Func_50898
constexpr uint16_t VANISHING = L(0x145); // .Func_508be
constexpr uint16_t DESTROY = L(0x14f); // .destroy
constexpr uint16_t LANDED = L(0x154); // .Func_508cd
constexpr uint16_t STATE_44 = L(0x21c); // .Func_50995
constexpr uint16_t STATE_45 = L(0x224); // .Func_5099d
constexpr uint16_t RESET = L(0x232); // .Func_509ab
constexpr uint16_t LYING = L(0x242); // .Func_509bb
constexpr uint16_t KNOCKED_59 = L(0x264); // .Func_509dd
constexpr uint16_t KNOCKED_58 = L(0x276); // .Func_509ef
constexpr uint16_t KNOCKED_34 = L(0x288); // .Func_50a01
constexpr uint16_t KNOCKED_35 = L(0x292); // .Func_50a0b
constexpr uint16_t THROWN_36 = L(0x2a5); // .Func_50a1e
constexpr uint16_t THROWN_37 = L(0x2b8); // .Func_50a31
constexpr uint16_t GRAB_49 = L(0x2cb); // .Func_50a44
constexpr uint16_t HELD_49 = L(0x2d5); // .Func_50a4e
constexpr uint16_t GRAB_48 = L(0x2e5); // .Func_50a5e
constexpr uint16_t HELD_48 = L(0x2ef); // .Func_50a68
constexpr uint16_t STATE_42 = L(0x300); // .Func_50a79

// .Func_508cd's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, FALL_OFF, FALL_OFF, FALL_OFF, KNOCKED_34, KNOCKED_35, THROWN_36, THROWN_37, // $00
	FALL_OFF, FALL_OFF, ADDR_VanishObject2, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, RESET,
	FALL_OFF, FALL_OFF, STATE_42, FALL_OFF, STATE_44, STATE_45, FALL_OFF, FALL_OFF, // $10
	GRAB_48, GRAB_49, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF,
	FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF,
	ADDR_ObjState_Fall, FALL_OFF, FALL_OFF, FALL_OFF, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, FALL_OFF, LYING,
	FALL_OFF, FALL_OFF, ADDR_Func_3371, FALL_OFF, ADDR_Func_32cc, ADDR_Func_32db, FALL_OFF, FALL_OFF, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Anonster : public Object {
public:
	using Object::Object;

	void start(); // AnonsterFunc
	void enter(); // .Func_50024
	void hidden(); // .Func_5005e
	void walk(); // .Func_5007f
	void silk_time(); // .Func_5016f
	void drop_silk(uint16_t left, uint16_t right, uint8_t last, uint16_t next); // .Func_501c4, ...
	void climb_start(); // .Func_50282
	void climb(); // .Func_50293
	void run_to_wario(); // .Func_502d1
	void drop(); // .Func_5034c
	void land(); // .Func_50397
	void ground(); // .Func_5041d
	void hurt(); // .Func_505be
	void hurt2(); // .Func_505f4
	void hurt3(); // .Func_50610
	void climb_back(); // .Func_50635
	void tangled(); // .Func_50650
	void tangled2(); // .Func_5066c
	void defeated(); // .Func_50689
	void fall_away(); // .Func_50705
	void update_pal(); // UpdateAnonsterPal

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool left_of_wario() const;
	void face_wario();
	void hide(); // .asm_50031
	void walk_frameset(); // .asm_5018e
	void walk_ceiling(); // .asm_5017f
	void back_to_walking(); // .asm_50266
	bool wall_ahead(uint8_t right_offset, uint8_t left_offset);
	void ground_box(); // .asm_50388
	void stand(); // .asm_5043b
	void tangle(); // .asm_504b1
	void rush(); // .asm_504fe
	bool count_down();
	void hit(); // .asm_5054b
};

class AnonsterHelper : public Object {
public:
	using Object::Object;

	void platforms(); // SilkPlatformsFunc
	void create_platform(uint16_t params, uint16_t next); // .asm_50746
	void platforms_wait(); // .Wait
	void platform(); // SilkPlatformFunc
	void silk_float(bool right); // AnonsterSilkFunc (.FloatLeft), .FloatRight
	void run(uint16_t handler);
	void wave(); // AnonsteWaveFunc

private:
	void destroy() { flags() = 0; }
	void fall_off(); // .Func_50898
	void drift(); // .asm_507f7
	void sink(); // .asm_50855
	void turn_at_random(); // .asm_50849
	void knocked(uint8_t s); // .asm_50a13
	void knocked_low(uint8_t s);
	void thrown(uint8_t s, uint8_t var2_, uint16_t routine);
	void grab(bool right);
	void hold(bool right);
};

} // namespace

// carry if the object is left of Wario
bool Anonster::left_of_wario() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void Anonster::face_wario()
{
	if (left_of_wario())
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
}

void Anonster::start()
{
	mem_[wIsBossBattle] = TRUE;
	var2() = 27;
	move_up_by_var2();
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() = (substate() & 0xf0) | 0x04;
	state_duration() = 0x28;
	set_update(ENTER);
	enter();
}

void Anonster::enter()
{
	if (--state_duration())
		return;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	hide();
}

// up in the ceiling
void Anonster::hide()
{
	set_frameset(facing(ADDR_Frameset_693c4, ADDR_Frameset_693d5));
	state_duration() = 0x50;
	set_update(HIDDEN);
	set_coll_box(0xe3, 0xfc, 0xf7, 0x08);
	interaction() &= ~(1 << 7);
}

void Anonster::hidden()
{
	no_actions();
	if (--state_duration())
		return;
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(WALK);
	state() = 0;
	var3() = 0;
	var2() = 0;
	state_duration() = 0;
}

void Anonster::walk_frameset()
{
	set_frameset(facing(ADDR_Frameset_693f9, ADDR_Frameset_69424));
	state_duration() = 0x20;
}

void Anonster::walk_ceiling()
{
	SET_BC(gb_, ADDR_Data_60790);
	routines_.call(ADDR_ApplyObjYMovement_Loop);
}

// a wall ahead (Func_3513) at the given x offsets
bool Anonster::wall_ahead(uint8_t right_offset, uint8_t left_offset)
{
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = facing_right() ? static_cast<uint16_t>(x() + right_offset) : static_cast<uint16_t>(x() - left_offset);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	return gb_->cpu.a;
}

// walks along the ceiling, turning at walls; after a while (var3 $4f
// twice, with no transient objects about) it's time to drop silk; at
// state $28/$29 it climbs
void Anonster::walk()
{
	no_actions();
	if ((state() & 0xfe) == 0x28) {
		set_frameset(facing(ADDR_Frameset_6943c, ADDR_Frameset_69441));
		state_duration() = 0;
		set_update(CLIMB_START);
		return;
	}
	flags() &= ~OBJFLAG_NO_COLLISION;
	walk_ceiling();
	if (state_duration()) {
		if (--state_duration())
			return;
		set_frameset(facing(ADDR_Frameset_693c4, ADDR_Frameset_693d5));
		return;
	}
	if (var3() == 0x4f && ++var2() >= 2) {
		routines_.farcall(BANK__CountActiveTransientObjects, ADDR__CountActiveTransientObjects);
		if (!gb_->cpu.e) {
			var2() = 0;
			var3() = 0;
			set_update(SILK_TIME);
			face_wario();
			set_frameset(facing_right() ? ADDR_Frameset_69411 : ADDR_Frameset_693e6);
			return;
		}
	}
	if (var3() == 1)
		play_sfx(SFX_0B0);
	bool right = facing_right();
	if (!wall_ahead(0x10, 0x11)) {
		if (right)
			move_right();
		else
			move_left();
		return;
	}
	set_frameset(right ? ADDR_Frameset_69512 : ADDR_Frameset_69446);
	state_duration() = 0x0a;
	substate() ^= OBJSUBFLAG_HDIR;
}

void Anonster::back_to_walking()
{
	set_update(SILK_TIME);
	set_frameset(facing(ADDR_Frameset_69402, ADDR_Frameset_6942d));
	walk_ceiling();
}

// drops silk at var3 $1f, goes back to walking at $4f
void Anonster::silk_time()
{
	no_actions();
	if (var3() == 0x1f) {
		set_update(DROP_SILK1);
		walk_frameset();
		return;
	}
	if (var3() != 0x4f) {
		walk_ceiling();
		return;
	}
	set_update(WALK);
	bool right = facing_right();
	var3() = 0;
	set_frameset(right ? ADDR_Frameset_693d5 : ADDR_Frameset_693c4);
	state_duration() = 0;
}

// drops a silk strand at duration $18; then the next, or back to walking
// if it has `last` hits left (or after the last)
void Anonster::drop_silk(uint16_t left, uint16_t right, uint8_t last, uint16_t next)
{
	no_actions();
	if (!--state_duration()) {
		if (last && (substate() & 0x0f) != last) {
			set_update(next);
			walk_frameset();
			return;
		}
		back_to_walking();
		return;
	}
	if (state_duration() != 0x18)
		return;
	play_sfx(SFX_07E);
	SET_BC(gb_, facing(left, right));
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void Anonster::climb_start()
{
	set_update(CLIMB);
	play_sfx(SFX_0B1);
	climb();
}

// climbs to the top of the screen, then turns towards Wario
void Anonster::climb()
{
	no_actions();
	if (state_duration()) {
		if (--state_duration())
			return;
		var1() = 0x03;
		set_update(RUN_TO_WARIO);
		return;
	}
	if (screen_y() >= 0x08) {
		routines_.call(ADDR_MoveObjectUp_Slow);
		return;
	}
	state_duration() = 0x3c;
	face_wario();
}

void Anonster::ground_box()
{
	set_coll_box(0xf0, 0x00, 0xf0, 0x0f);
}

// runs along the ceiling to above Wario (turning at walls var1 times at
// most), then drops
void Anonster::run_to_wario()
{
	if ((mem_[wWarioScreenXPos] & 0xfe) != (screen_x() & 0xfe)) {
		bool right = facing_right();
		if (!wall_ahead(0x10, 0x11)) {
			routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
			return;
		}
		substate() ^= OBJSUBFLAG_HDIR;
		if (--var1())
			return;
	}
	flags() &= ~OBJFLAG_NO_COLLISION;
	var3() = 0;
	set_update(DROP);
	interaction() = (interaction() & HEAVY_OBJ) | OBJ_INTERACTION_48 | HEAVY_OBJ;
	set_coll_box(0xe8, 0xf8, 0xf4, 0x0b);
	set_frameset(ADDR_Frameset_6944d);
}

// drops to the ground
void Anonster::drop()
{
	bool landed = false;
	if (screen_y() >= 0x50) {
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		landed = gb_->cpu.a;
	}
	if (!landed) {
		SET_BC(gb_, ADDR_Data_607f0);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	set_update(LAND);
	set_frameset(ADDR_Frameset_69450);
	state_duration() = 0x20;
	ground_box();
}

// lands (shaking the ground), then turns to Wario
void Anonster::land()
{
	uint8_t d = --state_duration();
	if (d == 0x18) {
		play_sfx(SFX_061);
		gb_->cpu.b = 0x18;
		routines_.call(ADDR_DoGroundShake);
		return;
	}
	if (d)
		return;
	set_update(GROUND);
	state() = 0x5a;
	face_wario();
	set_frameset(facing_right() ? ADDR_Frameset_694c9 : ADDR_Frameset_694d0);
	state_duration() = 0x07;
	var1() = 0x02;
}

// stands, facing its direction
void Anonster::stand()
{
	bool right = facing_right();
	mem_[wCurObjCollBoxRight] = right ? 0x07 : 0x10;
	mem_[wCurObjCollBoxLeft] = right ? 0xef : 0xf8;
	mem_[wCurObjCollBoxBottom] = 0x00;
	mem_[wCurObjCollBoxTop] = 0xf0;
	set_interaction(OBJ_INTERACTION_48);
	set_frameset(right ? ADDR_Frameset_69496 : ADDR_Frameset_69485);
	state_duration() = 0;
	state() = 0;
}

// tangled up by Ball o' String Wario
void Anonster::tangle()
{
	set_frameset(facing(ADDR_Frameset_69554, ADDR_Frameset_6954d));
	state_duration() = 0x07;
	set_update(TANGLED);
	ground_box();
}

// rushes, until a wall
void Anonster::rush()
{
	uint16_t frameset;
	if (!facing_right()) {
		if (!wall_ahead(0, 0x14)) {
			move_left();
			return;
		}
		frameset = ADDR_Frameset_69573;
	} else {
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		uint16_t x_pos = static_cast<uint16_t>(x() + 0x17);
		mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
		mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
		routines_.call(ADDR_Func_3543);
		if (gb_->cpu.a) {
			move_right();
			return;
		}
		frameset = ADDR_Frameset_69568;
	}
	set_frameset(frameset);
	state_duration() = 0x0e;
}

// counts the state duration down to standing, turning around at 7; false
// if it had run out
bool Anonster::count_down()
{
	if (!state_duration())
		return false;
	uint8_t d = --state_duration();
	if (!d) {
		stand();
		return true;
	}
	if (d == 0x07)
		substate() ^= OBJSUBFLAG_HDIR;
	return true;
}

// hit: it's hurt only from behind
void Anonster::hit()
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	bool c = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
	if (facing_right() ? c : !c) {
		stand();
		return;
	}
	set_interaction(OBJ_INTERACTION_0B);
	play_sfx(SFX_VANISH);
	SET_HL(gb_, wCurObjId);
	routines_.call(ADDR_DecrementRemainingBossHits);
	set_frameset(facing(ADDR_Frameset_694a7, ADDR_Frameset_694b8));
	if ((substate() & 0x0f) == 1) { // defeated
		state_duration() = 0xac;
		substate() &= OBJSUBFLAG_HDIR;
		set_update(DEFEATED);
		stop_music2(mem_);
		return;
	}
	state_duration() = 0x56;
	flags() &= ~OBJFLAG_NO_COLLISION;
	set_update(HURT);
	update_pal();
}

// on the ground: makes waves, rushes at Wario, turns
void Anonster::ground()
{
	switch (state()) {
	case 0x00: {
		if (count_down())
			return;
		if (mem_[wGlobalCounter] & 1)
			return;
		bool c = screen_x() < mem_[wWarioScreenXPos];
		bool right = facing_right();
		if (right ? !c : c) {
			rush();
			return;
		}
		set_frameset(right ? ADDR_Frameset_69474 : ADDR_Frameset_69463);
		state_duration() = 0x5d;
		state() = 0x5b;
		return;
	}
	case 0x5b: // makes waves
		if (!--state_duration()) {
			if (!--var1()) {
				tangle();
				return;
			}
			state() = 0x5c;
			set_frameset(facing(ADDR_Frameset_69485, ADDR_Frameset_69496));
			return;
		}
		if (state_duration() != 0x2b)
			return;
		play_sfx(SFX_07E);
		SET_BC(gb_, facing(ADDR_ObjParams_AnonsterWaveLeft, ADDR_ObjParams_AnonsterWaveRight));
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	case 0x5c:
		if (mem_[wTransformation] == TRANSFORMATION_BALL_O_STRING_WARIO) {
			tangle();
			return;
		}
		if (count_down())
			return;
		if (frame_duration() >= 0x03)
			frame_duration() -= 0x03;
		if (!(mem_[wGlobalCounter] & 0x0f))
			play_sfx(SFX_0AF);
		rush();
		return;
	case 0x5a:
		if (--state_duration())
			return;
		stand();
		return;
	case 0x0a:
		hit();
		return;
	default:
		stand();
		return;
	}
}

void Anonster::hurt()
{
	no_actions();
	if (state_duration() == 0x4e)
		play_sfx(SFX_0B1);
	if (--state_duration())
		return;
	set_frameset(facing(ADDR_Frameset_6953c, ADDR_Frameset_69543));
	state_duration() = 0x0e;
	set_update(HURT2);
	ground_box();
}

void Anonster::hurt2()
{
	no_actions();
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_6951e);
	state_duration() = 0x33;
	set_update(HURT3);
}

void Anonster::hurt3()
{
	no_actions();
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_69519);
	var3() = 0;
	set_update(CLIMB_BACK);
	play_sfx(SFX_0B2);
}

// climbs back up, then hides in the ceiling again
void Anonster::climb_back()
{
	no_actions();
	if (screen_y() >= 0x10) {
		SET_BC(gb_, ADDR_Data_607e0);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	if (mem_[wTransformation] != TRANSFORMATION_BALL_O_STRING_WARIO)
		hide();
}

void Anonster::tangled()
{
	no_actions();
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_694e1);
	state_duration() = 0x2d;
	set_update(TANGLED2);
}

void Anonster::tangled2()
{
	no_actions();
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_6944d);
	var3() = 0;
	set_update(CLIMB_BACK);
}

// falls off the screen
void Anonster::fall_away()
{
	no_actions();
	SET_BC(gb_, ADDR_Data_60180);
	routines_.call(ADDR_ApplyObjYMovement);
	if (!facing_right())
		move_right();
	else
		move_left();
}

void Anonster::defeated()
{
	no_actions();
	if (!state_duration()) {
		set_update(FALL_AWAY);
		play_sfx(SFX_0A3);
		fall_away();
		return;
	}
	if (state_duration() == 0xa4)
		play_sfx(SFX_0B1);
	if (--state_duration())
		return;
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x05;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
	mem_[wBossState] = BOSS_DEFEATED;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = 0;
	set_frameset(facing(ADDR_Frameset_694d7, ADDR_Frameset_694dc));
	state() = 0x48;
	face_wario();
	var3() = 0;
	flags() |= OBJFLAG_PRIORITY;
}

// red when it has 2 hits left, green at 3
void Anonster::update_pal()
{
	uint8_t hits = substate() & 0x0f;
	if (hits == 0x03)
		SET_HL(gb_, ADDR_UpdateAnonsterPal + 0x1e); // .Pals_50ab4
	else if (hits == 0x02)
		SET_HL(gb_, ADDR_UpdateAnonsterPal + 0x26); // .Pals_50abc
	else
		return;
	SET_DE(gb_, wTempPals2 + 7 * PAL_SIZE);
	gb_->cpu.c = 7;
	gb_->cpu.b = 1;
	routines_.call(ADDR_CopyAndApplyOBPals);
}

// spawns the three silk platforms one after the other
void AnonsterHelper::platforms()
{
	set_update(PLATFORMS_CREATE1);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() |= 1 << OBJSUBFLAG_UNK_4_F;
	state_duration() = 0x28;
	create_platform(REG_BC(gb_), REG_DE(gb_));
}

void AnonsterHelper::create_platform(uint16_t params, uint16_t next)
{
	no_actions();
	frame_duration() = 2;
	if (--state_duration())
		return;
	state_duration() = 0x32;
	set_update(next);
	SET_BC(gb_, params);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void AnonsterHelper::platforms_wait()
{
	no_actions();
	frame_duration() = 2;
}

void AnonsterHelper::platform()
{
	one_way_platform();
	if (!state_duration())
		return;
	--state_duration();
	routines_.call(ADDR_MoveObjectDown_Slow);
}

void AnonsterHelper::wave()
{
	if (state())
		destroy();
}

// falls off: disappears after a while
void AnonsterHelper::fall_off()
{
	no_actions();
	set_update(VANISHING);
	set_frameset(ADDR_Frameset_6955b);
	state_duration() = 0x1d;
	flags() |= OBJFLAG_NO_COLLISION;
	flags() &= ~OBJFLAG_GRABBED;
	play_sfx(SFX_0A1);
}

void AnonsterHelper::turn_at_random()
{
	if (!(mem_[rDIV] & 1))
		substate() ^= OBJSUBFLAG_HDIR;
}

// hit, or sinks a pixel every 4 frames until it lands
void AnonsterHelper::sink()
{
	if (uint8_t s = state()) {
		if (s < 0x0b)
			fall_off();
		else
			destroy();
		return;
	}
	if (mem_[wGlobalCounter] & 0x03)
		return;
	move_down();
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	set_frameset(ADDR_Frameset_694f5);
	state_duration() = 0x64;
	set_update(LANDED);
	set_interaction(OBJ_INTERACTION_00);
}

// drifts with the wind, turning at walls and (at var2 $2e) at random
void AnonsterHelper::drift()
{
	if (screen_y() >= 0xb0) {
		destroy();
		return;
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	bool right = facing_right();
	SET_BC(gb_, right ? ADDR_Data_60800 : ADDR_Data_60830);
	routines_.call(ADDR_ApplyObjXMovement_Loop);
	// the x position through the page in h
	uint16_t at = static_cast<uint16_t>(gb_->cpu.h << 8 | OBJ_X_POS);
	uint16_t x_pos = static_cast<uint16_t>(mem_[at] | mem_[at + 1] << 8);
	x_pos = right ? static_cast<uint16_t>(x_pos - 5) : static_cast<uint16_t>(x_pos + 4);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		substate() ^= OBJSUBFLAG_HDIR;
	else if (var2() == 0x2e)
		turn_at_random();
	sink();
}

// floats sideways while the state duration lasts, turning at walls
void AnonsterHelper::silk_float(bool right)
{
	if (!state_duration()) {
		drift();
		return;
	}
	routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 4) : static_cast<uint16_t>(x() - 5);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		set_update(right ? FLOAT_LEFT : FLOAT_RIGHT);
		if (right)
			substate() &= ~OBJSUBFLAG_HDIR;
		else
			substate() |= OBJSUBFLAG_HDIR;
		return;
	}
	if (!--state_duration())
		turn_at_random();
	sink();
}

// knocked away
void AnonsterHelper::knocked(uint8_t s)
{
	state() = s;
	var3() = 0;
	var2() = 0x02;
	no_actions();
}

// knocked away, unless it's high on the screen
void AnonsterHelper::knocked_low(uint8_t s)
{
	if (screen_y() < 0x50) {
		routines_.call(ADDR_VanishObject2);
		return;
	}
	knocked(s);
}

void AnonsterHelper::thrown(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

// grabbed: turns with Wario
void AnonsterHelper::grab(bool right)
{
	state() = right ? 0x49 : 0x48;
	flags() |= OBJFLAG_GRABBED;
	hold(right);
}

void AnonsterHelper::hold(bool right)
{
	if (right ? mem_[wDirection] != 0 : mem_[wDirection] == 0) {
		routines_.call(right ? ADDR_Func_3353 : ADDR_Func_3362);
		return;
	}
	substate() ^= OBJSUBFLAG_HDIR;
	grab(!right);
}

void AnonsterHelper::run(uint16_t handler)
{
	switch (handler) {
	case LANDED: { // hit states, by state
		flags() &= ~OBJFLAG_INVISIBLE;
		uint8_t sy = screen_y();
		if (sy >= 0xb0 && sy < 0xc0) {
			fall_off();
			return;
		}
		uint8_t s = state();
		if (s >= NUM_HANDLERS) {
			unrealistic();
			return;
		}
		run(HANDLERS[s]);
		return;
	}
	case FALL_OFF:
		fall_off();
		return;
	case VANISHING:
		no_actions();
		if (!--state_duration())
			destroy();
		return;
	case DESTROY:
		destroy();
		return;
	case STATE_44:
	case STATE_45:
		state() = handler == STATE_44 ? 0x44 : 0x45;
		var2() = 0x02;
		var3() = 0;
		return;
	case RESET:
		state() = 0x3f;
		flags() &= ~OBJFLAG_GRABBED;
		state_duration() = 0x78;
		return;
	case LYING: // flickers at the end, then disappears
		if (!--state_duration()) {
			destroy();
			return;
		}
		if (state_duration() < 0x3c) {
			if ((mem_[wGlobalCounter] & 0x07) < 0x04)
				flags() |= OBJFLAG_INVISIBLE;
			else
				flags() &= ~OBJFLAG_INVISIBLE;
		}
		routines_.call(ADDR_Func_32ea);
		return;
	case KNOCKED_59:
		knocked_low(0x59);
		return;
	case KNOCKED_58:
		knocked_low(0x58);
		return;
	case KNOCKED_34:
		knocked(0x34);
		return;
	case KNOCKED_35:
		knocked(0x35);
		return;
	case THROWN_36:
		thrown(0x36, 0xc0, ADDR_Func_3335);
		return;
	case THROWN_37:
		thrown(0x37, 0xe0, ADDR_Func_3344);
		return;
	case GRAB_49:
		grab(true);
		return;
	case HELD_49:
		hold(true);
		return;
	case GRAB_48:
		grab(false);
		return;
	case HELD_48:
		hold(false);
		return;
	case STATE_42:
		mem_[wCurObjAction] = 31 | (1 << 7); // no_actions_for 31
		state() = 0x42;
		var3() = 0;
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using wl3::level::Anonster;
using wl3::level::AnonsterHelper;

PORT_EX(AnonsterFunc, 0, ALL_REGISTERS) { Anonster(gb).start(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50024, 0, ALL_REGISTERS) { Anonster(gb).enter(); }
PORT_LOCAL_EX(AnonsterFunc, Func_5005e, 0, ALL_REGISTERS) { Anonster(gb).hidden(); }
PORT_LOCAL_EX(AnonsterFunc, Func_5007f, 0, ALL_REGISTERS) { Anonster(gb).walk(); }
PORT_LOCAL_EX(AnonsterFunc, Func_5016f, 0, ALL_REGISTERS) { Anonster(gb).silk_time(); }
PORT_LOCAL_EX(AnonsterFunc, Func_501c4, 0, ALL_REGISTERS)
{
	Anonster(gb).drop_silk(ADDR_ObjParams_AnonsterSilk1Left, ADDR_ObjParams_AnonsterSilk1Right, 0x04, wl3::level::DROP_SILK2);
}
PORT_LOCAL_EX(AnonsterFunc, Func_50200, 0, ALL_REGISTERS)
{
	Anonster(gb).drop_silk(ADDR_ObjParams_AnonsterSilk2Left, ADDR_ObjParams_AnonsterSilk2Right, 0x03, wl3::level::DROP_SILK3);
}
PORT_LOCAL_EX(AnonsterFunc, Func_5023d, 0, ALL_REGISTERS)
{
	Anonster(gb).drop_silk(ADDR_ObjParams_AnonsterSilk3Left, ADDR_ObjParams_AnonsterSilk3Right, 0, 0);
}
PORT_LOCAL_EX(AnonsterFunc, Func_50282, 0, ALL_REGISTERS) { Anonster(gb).climb_start(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50293, 0, ALL_REGISTERS) { Anonster(gb).climb(); }
PORT_LOCAL_EX(AnonsterFunc, Func_502d1, 0, ALL_REGISTERS) { Anonster(gb).run_to_wario(); }
PORT_LOCAL_EX(AnonsterFunc, Func_5034c, 0, ALL_REGISTERS) { Anonster(gb).drop(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50397, 0, ALL_REGISTERS) { Anonster(gb).land(); }
PORT_LOCAL_EX(AnonsterFunc, Func_5041d, 0, ALL_REGISTERS) { Anonster(gb).ground(); }
PORT_LOCAL_EX(AnonsterFunc, Func_505be, 0, ALL_REGISTERS) { Anonster(gb).hurt(); }
PORT_LOCAL_EX(AnonsterFunc, Func_505f4, 0, ALL_REGISTERS) { Anonster(gb).hurt2(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50610, 0, ALL_REGISTERS) { Anonster(gb).hurt3(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50635, 0, ALL_REGISTERS) { Anonster(gb).climb_back(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50650, 0, ALL_REGISTERS) { Anonster(gb).tangled(); }
PORT_LOCAL_EX(AnonsterFunc, Func_5066c, 0, ALL_REGISTERS) { Anonster(gb).tangled2(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50689, 0, ALL_REGISTERS) { Anonster(gb).defeated(); }
PORT_LOCAL_EX(AnonsterFunc, Func_50705, 0, ALL_REGISTERS) { Anonster(gb).fall_away(); }
PORT_EX(UpdateAnonsterPal, 0, ALL_REGISTERS) { Anonster(gb).update_pal(); }

PORT_EX(SilkPlatformsFunc, 0, ALL_REGISTERS) { AnonsterHelper(gb).platforms(); }
PORT_LOCAL_EX(SilkPlatformsFunc, CreatePlatform3, 0, ALL_REGISTERS)
{
	AnonsterHelper(gb).create_platform(ADDR_ObjParams_SilkPlatform3, wl3::level::PLATFORMS_WAIT);
}
PORT_LOCAL_EX(SilkPlatformsFunc, CreatePlatform2, 0, ALL_REGISTERS)
{
	AnonsterHelper(gb).create_platform(ADDR_ObjParams_SilkPlatform2, wl3::level::PLATFORMS_CREATE3);
}
PORT_LOCAL_EX(SilkPlatformsFunc, CreatePlatform1, 0, ALL_REGISTERS)
{
	AnonsterHelper(gb).create_platform(ADDR_ObjParams_SilkPlatform1, wl3::level::PLATFORMS_CREATE2);
}
PORT_LOCAL_EX(SilkPlatformsFunc, Wait, 0, ALL_REGISTERS) { AnonsterHelper(gb).platforms_wait(); }
PORT_EX(SilkPlatformFunc, 0, ALL_REGISTERS) { AnonsterHelper(gb).platform(); }
PORT_EX(AnonsteWaveLFunc_Left, 0, ALL_REGISTERS)
{
	wl3::Routines(gb).call(ADDR_MoveObjectLeft_Fast);
	AnonsterHelper(gb).wave();
}
PORT_EX(AnonsterWaveFunc_Right, 0, ALL_REGISTERS)
{
	wl3::Routines(gb).call(ADDR_MoveObjectRight_Fast);
	AnonsterHelper(gb).wave();
}
PORT_EX(AnonsteWaveFunc, 0, ALL_REGISTERS) { AnonsterHelper(gb).wave(); }

PORT_EX(AnonsterSilkFunc, 0, ALL_REGISTERS) { AnonsterHelper(gb).silk_float(false); }
PORT_LOCAL_EX(AnonsterSilkFunc, FloatRight, 0, ALL_REGISTERS) { AnonsterHelper(gb).silk_float(true); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50898, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::FALL_OFF); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_508be, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::VANISHING); }
PORT_LOCAL_EX(AnonsterSilkFunc, destroy, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::DESTROY); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_508cd, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::LANDED); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50995, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_5099d, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_509ab, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_509bb, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::LYING); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_509dd, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_509ef, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a01, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::KNOCKED_34); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a0b, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::KNOCKED_35); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a1e, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::THROWN_36); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a31, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::THROWN_37); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a44, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a4e, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a5e, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a68, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(AnonsterSilkFunc, Func_50a79, 0, ALL_REGISTERS) { AnonsterHelper(gb).run(wl3::level::STATE_42); }
