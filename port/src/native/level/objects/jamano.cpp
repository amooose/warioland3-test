// Jamano (jamano.asm), the boss flying back and forth swinging a hammer,
// with four skulls circling: each skull Wario knocks into him shortens the
// hammer (the range, wDollBoyHammerRange, and the palettes with it) and
// lost skulls respawn; hammerless, he bounces round the room, then loses his
// hat. And the hat (flying off), the hat platform, the skull spawner and
// the skulls. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t J(uint16_t offset) { return static_cast<uint16_t>(ADDR_JamanoFunc + offset); }
constexpr uint16_t SK(uint16_t offset) { return static_cast<uint16_t>(ADDR_SkullFunc + offset); }

constexpr uint16_t REVEAL = J(0x46); // .Func_54d7d
constexpr uint16_t STARTING = J(0x73); // .Func_54daa
constexpr uint16_t IDLE = J(0xd3); // .Func_54e0a
constexpr uint16_t FIGHTING = J(0xd9); // .Func_54e10
constexpr uint16_t HAMMERLESS = J(0x266); // .Func_54f9d
constexpr uint16_t STOPPING = J(0x344); // .Func_5507b
constexpr uint16_t STOPPED = J(0x360); // .Func_55097
constexpr uint16_t HAT_LOSS = J(0x37c); // .Func_550b3
constexpr uint16_t HATLESS = J(0x3a4); // .Func_550db
constexpr uint16_t FALLING = J(0x3c8); // .Func_550ff

constexpr uint16_t HAT_FLYING = ADDR_JamanoHatFunc + 0x1b; // .Func_55125
constexpr uint16_t PLATFORM_UPDATE = ADDR_HatPlatformFunc + 0x39; // .Func_55194
constexpr uint16_t SPAWNER_UPDATE = ADDR_SkullSpawnerFunc + 0x47; // .Func_5522c

constexpr uint16_t SKULL_WAITING = SK(0x12); // .Func_5529f
constexpr uint16_t SKULL_DELAY = SK(0x2d); // .Func_552ba
constexpr uint16_t SKULL_FLYING_OUT = SK(0x47); // .Func_552d4
constexpr uint16_t SKULL_AT_EDGE = SK(0x8f); // .Func_5531c
constexpr uint16_t SKULL_CIRCLING = SK(0xba); // .Func_55347
constexpr uint16_t SKULL_KNOCKED_RIGHT = SK(0x109); // .Func_55396
constexpr uint16_t SKULL_KNOCKED_LEFT = SK(0x113); // .Func_553a0

class Jamano : public Object {
public:
	using Object::Object;

	void start(); // JamanoFunc
	void reveal(); // .Func_54d7d
	void starting(); // .Func_54daa
	void idle() { no_actions(); } // .Func_54e0a
	void fighting(); // .Func_54e10
	void face_y(); // .Func_54ee4
	void face_wario(); // .Func_54f55
	void hammerless(); // .Func_54f9d
	bool step(uint16_t then, uint16_t frameset); // .Func_5507b / .Func_55097
	void hat_loss(); // .Func_550b3
	void hatless(); // .Func_550db
	void falling(); // .Func_550ff
	void hammer_frame(uint16_t flip_addr, uint8_t e); // Func_554bc
	void hammer_range(); // Func_55407
	void find_jamano(); // Func_55407.Func_5548b

private:
	uint8_t wall_ahead(bool right);
	uint8_t ground_below();
	void bob(); // .asm_54efb
	void range_changed(bool more, bool frame, uint16_t pals);
};

class JamanoHat : public Object {
public:
	using Object::Object;

	void start(); // JamanoHatFunc
	void flying(); // .Func_55125
};

class HatPlatform : public Object {
public:
	using Object::Object;

	void start(); // HatPlatformFunc
	void update(); // .Func_55194

private:
	void move(bool down);
};

class Skull : public Object {
public:
	using Object::Object;

	void spawner_start(); // SkullSpawnerFunc
	void spawner_update(); // .Func_5522c
	void start(); // SkullFunc
	void waiting(); // .Func_5529f
	void delay(); // .Func_552ba
	void flying_out(); // .Func_552d4
	void at_edge(); // .Func_5531c
	void circling(); // .Func_55347
	void knocked(uint16_t routine); // .Func_55396 / .Func_553a0
	void flicker(); // .Func_553e8

private:
	void fall(); // .asm_553a8
};

bool fight_on(Memory mem) { return mem[w1d149] || mem[wDollBoyHammerRange]; }

} // namespace

void Jamano::start()
{
	mem_[wIsBossBattle] = TRUE;
	stop_music2(mem_);
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(REVEAL);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x03;
	mem_[wCurObjCollBoxLeft] = 0xfc;
	mem_[wCurObjCollBoxBottom] = 0x06;
	set_frameset(ADDR_Frameset_69ce9);
	state_duration() = 0xff;
	var2() = 0x08;
	move_left_by_var2();
	var2() = 0x04;
	move_up_by_var2();
}

void Jamano::reveal()
{
	no_actions();
	if (state_duration() == 0xc0)
		play_music2(mem_, MUSIC_BOSS_REVEAL);
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_69cdb);
	state_duration() = 0xff;
	set_update(STARTING);
}

void Jamano::starting()
{
	no_actions();
	if (state_duration() == 0x30)
		stop_music2(mem_);
	if (--state_duration())
		return;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	flags() |= 1 << 7;
	set_update(FIGHTING);
	state_duration() = 0;
	var1() = 0;
	var2() = 0;
	var3() = 0;
	mem_[w1d149] = 0;
	bool right = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (right)
		substate() |= 1 << 7;
	else
		substate() &= ~(1 << 7);
	state() = right ? 0x5b : 0x5a;
	set_frameset(right ? ADDR_Frameset_69ca1 : ADDR_Frameset_69c1c);
}

// flies back and forth (state $5a left, $5b right), turning at walls and
// when Wario's passed, bobbing
void Jamano::fighting()
{
	if (!(mem_[wGlobalCounter] & 1)) {
		flags() &= ~(1 << 4);
		return;
	}
	flags() |= 1 << 4;
	if (!mem_[wDollBoyHammerRange]) {
		set_update(HAMMERLESS);
		flags() &= ~OBJFLAG_INVISIBLE;
		set_frameset(ADDR_Frameset_69ce0);
		state_duration() = 0xa0;
		stop_music2(mem_);
		return;
	}
	uint8_t s = state();
	bool left = s == 0x5a;
	if (!left && s != 0x5b) {
		uint16_t frameset = (substate() & (1 << 7)) ? ADDR_Frameset_69c7a : ADDR_Frameset_69c92;
		flags() &= ~OBJFLAG_INVISIBLE;
		set_update(IDLE);
		stop_music2(mem_);
		set_frameset(frameset);
		return;
	}
	face_wario();
	bool turn = false;
	if (var2() == 1) {
		bool hdir = substate() & OBJSUBFLAG_HDIR;
		turn = left ? hdir : !hdir;
	}
	if (!turn)
		turn = wall_ahead(!left);
	if (turn) {
		state() = left ? 0x5b : 0x5a;
	} else {
		SET_BC(gb_, left ? ADDR_Data_60c80 : ADDR_Data_60c20);
		routines_.call(ADDR_ApplyObjXMovement_Loop);
	}
	bob();
}

uint8_t Jamano::wall_ahead(bool right)
{
	uint16_t x_pos = static_cast<uint16_t>(x() + (right ? 0x08 : -0x08));
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	return gb_->cpu.a;
}

uint8_t Jamano::ground_below()
{
	uint16_t y_pos = static_cast<uint16_t>(y() + 0x0a);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	return gb_->cpu.a;
}

// up to the top of the room, then down to the floor
void Jamano::bob()
{
	if (var3() == 1)
		face_y();
	uint8_t y_screen = screen_y();
	bool up = true;
	if (!(substate() & (1 << 6))) {
		if (y_screen < 0x28) {
			substate() |= 1 << 6;
			var3() = 0;
			up = false;
		}
	} else {
		bool flip = y_screen >= 0x98 || (y_screen >= 0x7c && y_screen < 0x80 && ground_below());
		if (flip) {
			substate() &= ~(1 << 6);
			var3() = 0;
		} else {
			up = false;
		}
	}
	SET_BC(gb_, up ? ADDR_Data_60c80 : ADDR_Data_60c20);
	routines_.call(ADDR_ApplyObjYMovement_Loop);
}

void Jamano::face_y()
{
	if (static_cast<uint8_t>(screen_y() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a))
		substate() |= 1 << 6;
	else
		substate() &= ~(1 << 6);
}

// turns, swinging the hammer round
void Jamano::face_wario()
{
	if (state_duration()) {
		if (--state_duration())
			return;
		hammer_frame(wCurObjSubState, OBJ_FRAMESET_PTR + 1);
		frame_duration() = 0;
		mem_[wCurObjFramesetOffset] = 0;
		return;
	}
	bool right = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (right == static_cast<bool>(substate() & (1 << 7)))
		return;
	if (right)
		substate() |= 1 << 7;
	else
		substate() &= ~(1 << 7);
	set_frameset(right ? ADDR_Frameset_69c92 : ADDR_Frameset_69c7a);
	state_duration() = 0x14;
	play_sfx(SFX_0BC);
}

// bounces round the room until he's back in the middle
void Jamano::hammerless()
{
	no_actions();
	if (state_duration()) {
		--state_duration();
	} else if (screen_x() == 0x58) {
		set_update(STOPPING);
		set_frameset(ADDR_Frameset_69c40);
		state_duration() = 0x28;
		play_sfx(SFX_0BD);
		return;
	}
	uint8_t s = state();
	bool left = s == 0x5a;
	if (!left && s != 0x5b)
		return;
	if (wall_ahead(!left)) {
		play_sfx(SFX_BUMP);
		state() = left ? 0x5b : 0x5a;
	} else {
		routines_.call(left ? ADDR_MoveObjectLeft : ADDR_MoveObjectRight);
	}
	uint8_t y_screen = screen_y();
	if (!(substate() & (1 << 6))) {
		if (y_screen >= 0x30) {
			move_up();
			return;
		}
		substate() |= 1 << 6;
		play_sfx(SFX_BUMP);
		move_down();
		return;
	}
	if (y_screen < 0x7c || !ground_below()) {
		move_down();
		return;
	}
	substate() &= ~OBJSUBFLAG_VDIR;
	play_sfx(SFX_BUMP);
	move_up();
}

bool Jamano::step(uint16_t then, uint16_t frameset)
{
	no_actions();
	if (--state_duration())
		return false;
	set_update(then);
	set_frameset(frameset);
	state_duration() = 0x28;
	return true;
}

// the hat flies off
void Jamano::hat_loss()
{
	if (!step(HATLESS, ADDR_Frameset_69c4f))
		return;
	var2() = 0x0f;
	move_up_by_var2();
	SET_BC(gb_, ADDR_ObjParams_JamanoHat);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void Jamano::hatless()
{
	no_actions();
	if (--state_duration())
		return;
	set_update(FALLING);
	substate() &= 0xf0;
	var3() = 0;
	var2() = 0;
	play_sfx(SFX_0A3);
}

void Jamano::falling()
{
	no_actions();
	SET_BC(gb_, ADDR_Data_60d20);
	routines_.call(ADDR_ApplyObjYMovement);
}

// the hammer's frameset by the range, flipped by bit 7 at flip_addr,
// written to e (high byte) of its page and the byte before
void Jamano::hammer_frame(uint16_t flip_addr, uint8_t e)
{
	static const uint8_t LOWS[4][2] = {
		{0x37, 0xbc}, // 1
		{0x2e, 0xb3}, // 2
		{0x25, 0xaa}, // 3
		{0x1c, 0xa1}, // 4
	};
	uint8_t range = mem_[wDollBoyHammerRange];
	if (range < 1 || range > 4)
		return;
	bool flip = mem_[flip_addr] & (1 << 7);
	uint16_t page = flip_addr & 0xff00;
	uint16_t at = static_cast<uint16_t>(page | e);
	mem_[at] = 0x5c;
	mem_[static_cast<uint16_t>(at - 1)] = LOWS[range - 1][flip];
}

// the active Jamano's hammer, unless it's turning
void Jamano::find_jamano()
{
	if (mem_[w1d149])
		return;
	for (uint8_t obj = 0;; obj = static_cast<uint8_t>(obj + OBJ_STRUCT_LENGTH)) {
		uint16_t base = static_cast<uint16_t>((wObj1 & 0xff00) | obj);
		if (mem_[base + OBJ_ID] == JAMANO && (mem_[base + OBJ_FLAGS] & 1)) {
			if (mem_[base + OBJ_STATE_DURATION])
				return;
			hammer_frame(static_cast<uint16_t>(base + OBJ_SUBSTATE), static_cast<uint8_t>(obj + OBJ_FRAMESET_PTR + 1));
			return;
		}
		if (obj == static_cast<uint8_t>(wObj8))
			return;
	}
}

void Jamano::range_changed(bool more, bool frame, uint16_t pals)
{
	Byte range = mem_[wDollBoyHammerRange];
	if (more)
		++range;
	else
		--range;
	if (frame)
		find_jamano();
	gb_->cpu.c = 0;
	gb_->cpu.b = 8;
	if (!pals) {
		routines_.call(ADDR_BlackOutBGPals);
		return;
	}
	SET_HL(gb_, pals);
	routines_.call(ADDR_SetBGPals);
}

// the hammer by the skulls out (w1d147): longer as they go, shorter as
// they're knocked back
void Jamano::hammer_range()
{
	uint8_t skulls = mem_[w1d147];
	switch (static_cast<uint8_t>(mem_[wDollBoyHammerRange])) {
	case 0:
		if (skulls == 0x01)
			range_changed(true, true, ADDR_Pals_55551);
		return;
	case 1:
		if (skulls == 0x02)
			range_changed(true, true, ADDR_Pals_55591);
		else if (!skulls)
			range_changed(false, false, ADDR_Pals_55511);
		return;
	case 2:
		if (skulls == 0x03)
			range_changed(true, true, ADDR_Pals_555d1);
		else if (skulls == 0x01)
			range_changed(false, true, ADDR_Pals_55551);
		return;
	case 3:
		if (skulls == 0x04)
			range_changed(true, true, 0);
		else if (skulls == 0x02)
			range_changed(false, true, ADDR_Pals_55591);
		return;
	default:
		if (skulls == 0x03)
			range_changed(false, true, ADDR_Pals_555d1);
		return;
	}
}

void JamanoHat::start()
{
	no_actions();
	if (--state_duration())
		return;
	state_duration() = 0x3c;
	var2() = 0x03;
	set_update(HAT_FLYING);
}

// off the screen, then the boss is beaten
void JamanoHat::flying()
{
	no_actions();
	if (flags() & (1 << OBJFLAG_ON_SCREEN_F)) {
		move_up_by_var2();
		return;
	}
	if (--state_duration())
		return;
	flags() = 0;
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x06;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
	mem_[wBossState] = BOSS_DEFEATED;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = 0;
}

void HatPlatform::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_update(PLATFORM_UPDATE);
	mem_[wCurObjCollBoxRight] = 0x05;
	mem_[wCurObjCollBoxLeft] = 0xfa;
	mem_[wCurObjCollBoxBottom] = 0x00;
	set_frameset(ADDR_Frameset_69cef);
	state_duration() = 0;
	state() = 0;
	var1() = 1;
	var2() = 0x08;
	move_left_by_var2();
	var2() = 0x5a;
	move_up_by_var2();
}

// sinks while stood on (var1 1), then rises back
void HatPlatform::update()
{
	one_way_platform();
	if (!(mem_[wGlobalCounter] & 1))
		return;
	if (var1()) {
		if (++state_duration() == 0x30 || state()) {
			var1() = 0;
			state() = 0;
			return;
		}
		move(true);
		return;
	}
	if (!--state_duration()) {
		var1() = 0x01;
		return;
	}
	move(false);
}

void HatPlatform::move(bool down)
{
	if (flags() & OBJFLAG_STEPPED) {
		flags() &= ~OBJFLAG_STEPPED;
		gb_->cpu.b = 0x01;
		if (down) {
			routines_.call(ADDR_MoveWarioDown);
			routines_.call(ADDR_HandleDownwardsFloorTransition);
		} else {
			routines_.call(ADDR_MoveWarioUp);
			routines_.call(ADDR_HandleUpwardsFloorTransition);
		}
	}
	if (down)
		move_down();
	else
		move_up();
}

void Skull::spawner_start()
{
	set_update(SPAWNER_UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	var2() = 0x08;
	move_left_by_var2();
	var2() = 0x14;
	move_up_by_var2();
	for (uint16_t params : {ADDR_ObjParams_Skull1, ADDR_ObjParams_Skull2, ADDR_ObjParams_Skull3, ADDR_ObjParams_Skull4}) {
		SET_BC(gb_, params);
		routines_.call(ADDR_CreateObjectAtRelativePos);
	}
	mem_[wJamanoActiveSkulls] = 0x0f; // JAMANO_SKULL1 | JAMANO_SKULL2 | JAMANO_SKULL3 | JAMANO_SKULL4
	mem_[w1d147] = 0;
	mem_[wDollBoyHammerRange] = 0;
	mem_[w1d149] = 0x01;
}

// respawns lost skulls (one a frame) while the fight's on
void Skull::spawner_update()
{
	static const uint16_t PARAMS[4] = {
		ADDR_ObjParams_Skull1, ADDR_ObjParams_Skull2, ADDR_ObjParams_Skull3, ADDR_ObjParams_Skull4,
	};
	no_actions();
	frame_duration() = 0x02;
	if (!fight_on(mem_)) {
		flags() = 0;
		SET_HL(gb_, wCurObjUnk02);
		gb_->cpu.e = 0x07;
		routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
		return;
	}
	routines_.call(ADDR_Func_55407);
	Byte skulls = mem_[wJamanoActiveSkulls];
	for (int i = 0; i < 4; i++) {
		if (skulls & (1 << i))
			continue;
		skulls |= 1 << i;
		SET_BC(gb_, PARAMS[i]);
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	}
}

void Skull::flicker()
{
	if (!fight_on(mem_)) {
		flags() = 0;
		return;
	}
	if (mem_[wGlobalCounter] & 1)
		flags() &= ~(1 << 4);
	else
		flags() |= 1 << 4;
}

void Skull::start()
{
	flicker();
	no_actions();
	set_update(SKULL_WAITING);
}

void Skull::waiting()
{
	flicker();
	no_actions();
	frame_duration() = 0x02;
	if (--state_duration())
		return;
	set_update(SKULL_DELAY);
}

void Skull::delay()
{
	flicker();
	no_actions();
	if (--var1())
		return;
	set_update(SKULL_FLYING_OUT);
	flags() |= OBJFLAG_PRIORITY;
}

// out to the screen edge on its side (skulls 3 and 4 right), bobbing
void Skull::flying_out()
{
	flicker();
	no_actions();
	uint8_t id = mem_[wCurObjId];
	bool right = id >= SKULL3;
	if (screen_x() == (right ? SCREEN_WIDTH_PX : 0x10)) {
		set_update(SKULL_AT_EDGE);
		state_duration() = 0x27;
		++mem_[w1d147];
		return;
	}
	routines_.call(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
	SET_BC(gb_, (id & 1) ? ADDR_Data_60d00 : ADDR_Data_60ce0);
	routines_.call(ADDR_ApplyObjYMovement);
}

void Skull::at_edge()
{
	flicker();
	no_actions();
	if (state_duration()) {
		--state_duration();
		return;
	}
	if (mem_[wCurObjFramesetOffset] != 0x02)
		return;
	flags() &= ~((1 << 7) | (1 << 4));
	set_update(SKULL_CIRCLING);
	set_frameset(ADDR_Frameset_69cd6);
}

// until Wario knocks it (states 4 and 5)
void Skull::circling()
{
	if (mem_[w1d149]) {
		no_actions();
		flicker();
		return;
	}
	flags() &= ~OBJFLAG_INVISIBLE;
	uint16_t then;
	if (state() == 0x04)
		then = SKULL_KNOCKED_LEFT;
	else if (state() == 0x05)
		then = SKULL_KNOCKED_RIGHT;
	else
		return;
	set_update(then);
	var3() = 0;
	var2() = 1;
	set_frameset(ADDR_Frameset_69cec);
	state_duration() = 0xff;
	no_actions();
	play_sfx(SFX_017);
	--mem_[w1d147];
}

void Skull::knocked(uint16_t routine)
{
	if (flags() & (1 << OBJFLAG_ON_SCREEN_F)) {
		routines_.call(routine);
		return;
	}
	fall();
}

// off the screen: frees its slot to respawn
void Skull::fall()
{
	no_actions();
	if (state_duration() == 0xf7)
		play_sfx(SFX_0BF);
	if (--state_duration())
		return;
	flags() = 0;
	uint8_t id = mem_[wCurObjId];
	uint8_t mask = id == SKULL1 ? 0xff ^ (1 << 0)
		: id == SKULL2 ? 0xff ^ (1 << 1)
		: id == SKULL3 ? 0xff ^ (1 << 2)
		: 0xff ^ (1 << 3);
	mem_[wJamanoActiveSkulls] &= mask;
}

} // namespace wl3::level

using wl3::level::Jamano;
using wl3::level::JamanoHat;
using wl3::level::HatPlatform;
using wl3::level::Skull;

PORT_EX(JamanoFunc, 0, ALL_REGISTERS) { Jamano(gb).start(); }
PORT_LOCAL_EX(JamanoFunc, Func_54d7d, 0, ALL_REGISTERS) { Jamano(gb).reveal(); }
PORT_LOCAL_EX(JamanoFunc, Func_54daa, 0, ALL_REGISTERS) { Jamano(gb).starting(); }
PORT_LOCAL_EX(JamanoFunc, Func_54e0a, 0, ALL_REGISTERS) { Jamano(gb).idle(); }
PORT_LOCAL_EX(JamanoFunc, Func_54e10, 0, ALL_REGISTERS) { Jamano(gb).fighting(); }
PORT_LOCAL_EX(JamanoFunc, Func_54ee4, 0, ALL_REGISTERS) { Jamano(gb).face_y(); }
PORT_LOCAL_EX(JamanoFunc, Func_54f55, 0, ALL_REGISTERS) { Jamano(gb).face_wario(); }
PORT_LOCAL_EX(JamanoFunc, Func_54f9d, 0, ALL_REGISTERS) { Jamano(gb).hammerless(); }
PORT_LOCAL_EX(JamanoFunc, Func_5507b, 0, ALL_REGISTERS) { Jamano(gb).step(wl3::level::STOPPED, ADDR_Frameset_69c45); }
PORT_LOCAL_EX(JamanoFunc, Func_55097, 0, ALL_REGISTERS) { Jamano(gb).step(wl3::level::HAT_LOSS, ADDR_Frameset_69c4a); }
PORT_LOCAL_EX(JamanoFunc, Func_550b3, 0, ALL_REGISTERS) { Jamano(gb).hat_loss(); }
PORT_LOCAL_EX(JamanoFunc, Func_550db, 0, ALL_REGISTERS) { Jamano(gb).hatless(); }
PORT_LOCAL_EX(JamanoFunc, Func_550ff, 0, ALL_REGISTERS) { Jamano(gb).falling(); }
PORT_EX(Func_554bc, 0, ALL_REGISTERS) { Jamano(gb).hammer_frame(REG_HL(gb), gb->cpu.e); }
PORT_EX(Func_55407, 0, ALL_REGISTERS) { Jamano(gb).hammer_range(); }
PORT_LOCAL_EX(Func_55407, Func_5548b, 0, ALL_REGISTERS) { Jamano(gb).find_jamano(); }

PORT_EX(JamanoHatFunc, 0, ALL_REGISTERS) { JamanoHat(gb).start(); }
PORT_LOCAL_EX(JamanoHatFunc, Func_55125, 0, ALL_REGISTERS) { JamanoHat(gb).flying(); }
PORT_EX(HatPlatformFunc, 0, ALL_REGISTERS) { HatPlatform(gb).start(); }
PORT_LOCAL_EX(HatPlatformFunc, Func_55194, 0, ALL_REGISTERS) { HatPlatform(gb).update(); }

PORT_EX(SkullSpawnerFunc, 0, ALL_REGISTERS) { Skull(gb).spawner_start(); }
PORT_LOCAL_EX(SkullSpawnerFunc, Func_5522c, 0, ALL_REGISTERS) { Skull(gb).spawner_update(); }
PORT_EX(SkullFunc, 0, ALL_REGISTERS) { Skull(gb).start(); }
PORT_LOCAL_EX(SkullFunc, Func_5529f, 0, ALL_REGISTERS) { Skull(gb).waiting(); }
PORT_LOCAL_EX(SkullFunc, Func_552ba, 0, ALL_REGISTERS) { Skull(gb).delay(); }
PORT_LOCAL_EX(SkullFunc, Func_552d4, 0, ALL_REGISTERS) { Skull(gb).flying_out(); }
PORT_LOCAL_EX(SkullFunc, Func_5531c, 0, ALL_REGISTERS) { Skull(gb).at_edge(); }
PORT_LOCAL_EX(SkullFunc, Func_55347, 0, ALL_REGISTERS) { Skull(gb).circling(); }
PORT_LOCAL_EX(SkullFunc, Func_55396, 0, ALL_REGISTERS) { Skull(gb).knocked(ADDR_Func_3317); }
PORT_LOCAL_EX(SkullFunc, Func_553a0, 0, ALL_REGISTERS) { Skull(gb).knocked(ADDR_Func_3326); }
PORT_LOCAL_EX(SkullFunc, Func_553e8, 0, ALL_REGISTERS) { Skull(gb).flicker(); }
