// Scowler (scowler.asm), the boss in the flooded room: it walks the floor
// with two tentacles, shoots ink when level with Wario, and sinks into the
// water to come up under him; hit from below while it's up it's hurt, and
// the palettes flash (Func_54c07). And its tentacles, the ink, the
// floating rings (with their spawner). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SC(uint16_t rom) { return static_cast<uint16_t>(ADDR_ScowlerFunc + (rom - 0x44e5)); }

constexpr uint16_t START = SC(0x4520); // .Func_54520
constexpr uint16_t LEAVING = SC(0x4542); // .Func_54542
constexpr uint16_t WALKING = SC(0x458a); // .Func_5458a
constexpr uint16_t SINKING = SC(0x46e0); // .Func_546e0
constexpr uint16_t UNDERWATER = SC(0x4770); // .Func_54770
constexpr uint16_t HURT = SC(0x4806); // .Func_54806
constexpr uint16_t HURT_SINKING = SC(0x4853); // .Func_54853
constexpr uint16_t RISING = SC(0x487d); // .Func_5487d
constexpr uint16_t DEFEATED_RISING = SC(0x48c5); // .Func_548c5
constexpr uint16_t DEFEATED_SINKING = SC(0x490b); // .Func_5490b
constexpr uint16_t DEFEATED_GONE = SC(0x4931); // .Func_54931

constexpr uint16_t RING_SPAWN = ADDR_FloatingRingSpawnerFunc + 0x16; // .Func_54997
constexpr uint16_t RING_RISING = ADDR_FloatingRingFunc + 0x11; // .Func_549c7
constexpr uint16_t RING_WAITING = ADDR_FloatingRingFunc + 0x38; // .Func_549ee
constexpr uint16_t RING_UP = ADDR_FloatingRingFunc + 0x4c; // .Func_54a02
constexpr uint16_t RING_DOWN = ADDR_FloatingRingFunc + 0x77; // .Func_54a2d
constexpr uint16_t TENTACLE_UPDATE = ADDR_ScrowlerTentacleFunc + 0x27; // .Func_54a97
constexpr uint16_t INK_VANISH = ADDR_InkFunc + 0x72; // .Func_54bf8

class Scowler : public Object {
public:
	using Object::Object;

	void start(); // ScowlerFunc
	void begin(); // .Func_54520
	void leaving(); // .Func_54542
	void walking(); // .Func_5458a
	void down(bool tail); // .Func_5469d
	void up(bool tail); // .Func_546bd
	void sinking(); // .Func_546e0
	void underwater(); // .Func_54770
	void hurt(); // .Func_54806
	void hurt_sinking(); // .Func_54853
	void rising(); // .Func_5487d
	void defeated_rising(); // .Func_548c5
	void defeated_sinking(); // .Func_5490b
	void defeated_gone(); // .Func_54931
	void flashes(); // Func_54c07
	void ob_pals(uint16_t pals); // WriteScowlerOBPals
	void bg_pals(uint16_t pals); // WriteScowlerBGPals

private:
	void hpos(int dy);
	void reset_var3(); // var3 from the substate's hits
	bool walk(); // false: moved
	void stand(); // .asm_54895
	void surface(); // .asm_547e9
};

class ScowlerHelper : public Object {
public:
	using Object::Object;

	void ring_spawner(); // FloatingRingSpawnerFunc
	void ring(); // FloatingRingFunc
	void ring_rising(); // .Func_549c7
	void ring_waiting(); // .Func_549ee
	void ring_up(); // .Func_54a02
	void ring_down(); // .Func_54a2d
	void tentacle(); // ScrowlerTentacleFunc
	void tentacle_update(); // .Func_54a97
	void ink(bool right); // InkFunc / .Right
	void ink_vanish(); // .Func_54bf8

private:
	void tentacle_follow(uint16_t body_y, uint16_t body_x, bool other_side);
	void tentacle_interaction(uint8_t type, uint8_t top, uint8_t bottom);
};

} // namespace

void Scowler::hpos(int dy)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
}

// and $0f / swap a / add a / add $48
void Scowler::reset_var3()
{
	var3() = static_cast<uint8_t>(((substate() & 0x0f) << 5) + 0x48);
}

void Scowler::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(START);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() = (substate() & 0xf0) | 0x04;
	reset_var3();
	mem_[wCurObjCollBoxRight] = 0x08;
	mem_[wCurObjCollBoxLeft] = 0xf7;
	mem_[wCurObjCollBoxBottom] = 0xf8;
	set_frameset(ADDR_Frameset_69a5f);
	state_duration() = 0;
	var1() = 0;
	var2() = 0;
	mem_[wDollBoyActiveBarrels] = 0;
	mem_[w1d147] = 0;
}

void Scowler::begin()
{
	mem_[wIsBossBattle] = TRUE;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	set_update(WALKING);
	SET_BC(gb_, ADDR_ObjParams_ScrowlerTentacleRight);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
	SET_BC(gb_, ADDR_ObjParams_ScrowlerTentacleLeft);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

// the room left: Wario held where it is
void Scowler::leaving()
{
	if (!state_duration())
		return;
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_69b03);
	mem_[wWarioYPos + 1] = mem_[wCurObjYPos + 0];
	mem_[wWarioYPos + 0] = mem_[wCurObjYPos + 1];
	mem_[wWarioXPos + 1] = mem_[wCurObjXPos + 0];
	mem_[wWarioXPos + 0] = mem_[wCurObjXPos + 1];
}

bool Scowler::walk()
{
	bool right = facing_right();
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x04);
	uint16_t x_pos = static_cast<uint16_t>(right ? x() + 0x10 : x() - 0x11);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return true;
	routines_.call(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
	return false;
}

// down to the floor
void Scowler::down(bool tail)
{
	(void)tail;
	hpos(0x05);
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		move_down();
		return;
	}
	substate() &= ~OBJSUBFLAG_VDIR;
}

// up to the water's surface
void Scowler::up(bool tail)
{
	(void)tail;
	hpos(-0x2c);
	routines_.call(ADDR_Func_358b);
	if (mem_[wIsInWaterOrSand]) {
		move_up();
		return;
	}
	substate() |= OBJSUBFLAG_VDIR;
}

void Scowler::stand()
{
	reset_var3();
	var1() = 0;
	state() = 0;
	if (static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a)) {
		substate() |= 1 << 7;
		set_frameset(ADDR_Frameset_69a68);
		return;
	}
	substate() &= ~(1 << 7);
	set_frameset(ADDR_Frameset_69a5f);
}

// walks the floor (turning at walls); shoots ink when Wario's above at
// times; hit, it sinks
void Scowler::walking()
{
	flashes();
	uint8_t s = state();
	if (s == 0x2a) {
		state() = 0x5a;
		play_sfx(SFX_040);
		stop_music2(mem_);
		return;
	}
	if (mem_[wRoomTransitionParam]) {
		set_update(LEAVING);
		state_duration() = 0x1e;
		return;
	}
	if (!s) {
		if (var1()) {
			if (--var1()) { // to Wario's height
				uint8_t wario = mem_[wWarioScreenYPos] & 0xfe;
				uint8_t own = screen_y() & 0xfe;
				if (own == wario)
					return;
				if (own >= wario)
					up(true);
				else
					down(true);
				return;
			}
			set_frameset(facing_right() ? ADDR_Frameset_69a68 : ADDR_Frameset_69a5f);
			return;
		}
		if (!mem_[w1d147] && !--var3()) {
			reset_var3();
			if (screen_y() >= mem_[wWarioScreenYPos]) {
				state() = 0x2f;
				set_frameset(ADDR_Frameset_69a7a);
				state_duration() = 0x48;
				var1() = 0xfe;
				return;
			}
		}
		if (!walk())
			return;
		set_frameset(ADDR_Frameset_69a71);
		var1() = 0x2b;
		substate() ^= OBJSUBFLAG_HDIR;
		return;
	}
	if (s == 0x2f) { // the ink out partway
		if (!--state_duration()) {
			stand();
			return;
		}
		if (state_duration() != 0x20)
			return;
		play_sfx(SFX_07E);
		SET_BC(gb_, facing_right() ? ADDR_ObjParams_InkRight : ADDR_ObjParams_InkLeft);
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	}
	if ((s & 0xfe) != 0x08)
		return;
	set_frameset(ADDR_Frameset_69a95);
	state_duration() = 0x44;
	var1() = 0xfd;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_07);
	set_update(SINKING);
	play_sfx(SFX_0B3);
}

void Scowler::sinking()
{
	flashes();
	hpos(0x05);
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		SET_BC(gb_, ADDR_Data_60a60);
		routines_.call(ADDR_ApplyObjYMovement);
	}
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_69ab3);
	state_duration() = 0xff;
	var1() = 0xfc;
	var2() = 0x02;
	set_interaction(OBJ_INTERACTION_06);
	set_update(UNDERWATER);
	state() = 0;
	substate() |= 1 << 6;
	substate() &= ~(1 << 6);
}

void Scowler::surface()
{
	set_frameset(ADDR_Frameset_69aa8);
	state_duration() = 0x28;
	var1() = 0xfe;
	set_interaction(OBJ_INTERACTION_07);
	set_update(RISING);
}

// under the water, up and down after Wario; hit from below, it's hurt
void Scowler::underwater()
{
	flashes();
	uint8_t s = state();
	if (s) {
		if ((s & 0xfe) == 0x08) {
			if (screen_y() < mem_[wWarioScreenYPos]) {
				play_sfx(SFX_0B4);
				set_frameset(ADDR_Frameset_69abc);
				state_duration() = 0x1e;
				set_update(HURT);
				set_interaction(OBJ_INTERACTION_07);
				SET_HL(gb_, wCurObjInteractionType - 1);
				routines_.call(ADDR_DecrementRemainingBossHits);
				return;
			}
			state() = 0;
			substate() |= 1 << 6;
			var2() = 0x02;
		} else {
			state() = 0;
		}
	}
	if (!(mem_[wGlobalCounter] & 0x07))
		play_sfx(SFX_0AB);
	if (!--state_duration() && !--var2()) {
		surface();
		return;
	}
	if (substate() & OBJSUBFLAG_VDIR)
		down(false);
	else
		up(false);
	if (!walk())
		return;
	substate() ^= OBJSUBFLAG_HDIR;
}

// hurt: beaten on the last hit
void Scowler::hurt()
{
	flashes();
	if (--state_duration())
		return;
	var3() = 0;
	if ((substate() & 0x0f) == 1) {
		substate() &= 0x80;
		set_update(DEFEATED_RISING);
		set_frameset(ADDR_Frameset_69ac3);
		var1() = 0xff;
		var2() = 0x14;
		move_up_by_var2();
		stop_music2(mem_);
		return;
	}
	set_update(HURT_SINKING);
	set_frameset(ADDR_Frameset_69b08);
	state_duration() = 0x78;
}

void Scowler::hurt_sinking()
{
	flashes();
	if (!--state_duration()) {
		surface();
		return;
	}
	hpos(-0x2c);
	routines_.call(ADDR_Func_358b);
	if (!mem_[wIsInWaterOrSand])
		return;
	SET_BC(gb_, ADDR_Data_60a70);
	routines_.call(ADDR_ApplyObjYMovement);
}

void Scowler::rising()
{
	flashes();
	if (--state_duration())
		return;
	set_interaction(OBJ_INTERACTION_SCOWLER);
	set_update(WALKING);
	stand();
}

void Scowler::defeated_rising()
{
	no_actions();
	flashes();
	hpos(-0x18);
	routines_.call(ADDR_Func_358b);
	if (mem_[wIsInWaterOrSand]) {
		SET_BC(gb_, ADDR_Data_60a70);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	state_duration() = 0x0f;
	var2() = 0;
	flags() |= 1 << OBJFLAG_PRIORITY_F;
	set_update(DEFEATED_SINKING);
	play_sfx(SFX_0AC);
}

void Scowler::defeated_sinking()
{
	no_actions();
	flashes();
	routines_.call(ADDR_MoveObjectDown_Slow);
	SET_BC(gb_, ADDR_Data_60a80);
	routines_.call(ADDR_ApplyObjXMovement);
	if (--state_duration())
		return;
	state_duration() = 0xb4;
	var2() = 0;
	set_update(DEFEATED_GONE);
}

void Scowler::defeated_gone()
{
	no_actions();
	flashes();
	if (state_duration() && !--state_duration()) {
		SET_HL(gb_, wCurObjUnk02);
		gb_->cpu.e = 0x05;
		routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
		mem_[wBossState] = BOSS_DEFEATED;
		routines_.call(ADDR_UpdateLevelMusic);
		mem_[wIsBossBattle] = 0;
		if (mem_[w1d147]) {
			mem_[w1d147] = 0;
			ob_pals(ADDR_Pals_54c87);
			bg_pals(ADDR_Pals_54cb7);
		}
	}
	routines_.call(ADDR_MoveObjectDown_Slow);
	SET_BC(gb_, ADDR_Data_60a90);
	routines_.call(ADDR_ApplyObjXMovement_Loop);
}

// the palettes' flashes after a hit, timed by w1d147 (paused while
// wDollBoyActiveBarrels counts down)
void Scowler::flashes()
{
	uint8_t t = mem_[w1d147];
	if (!t)
		return;
	if (mem_[wDollBoyActiveBarrels]) {
		--mem_[wDollBoyActiveBarrels];
		return;
	}
	++mem_[w1d147];
	if (t == 0x02 || t == 0x3d) {
		play_sfx(t == 0x02 ? SFX_0B5 : SFX_0B7);
		ob_pals(ADDR_Pals_54c9f);
		bg_pals(ADDR_Pals_54cf7);
		return;
	}
	if (t == 0x3c) {
		play_sfx(SFX_0B6);
		mem_[wDollBoyActiveBarrels] = 0xff;
		gb_->cpu.c = 5;
		gb_->cpu.b = 3;
		routines_.call(ADDR_BlackOutOBPals);
		gb_->cpu.c = 0;
		gb_->cpu.b = 8;
		routines_.call(ADDR_BlackOutBGPals);
		return;
	}
	if (t != 0x78)
		return;
	play_sfx(SFX_0B8);
	mem_[w1d147] = 0;
	ob_pals(ADDR_Pals_54c87);
	bg_pals(ADDR_Pals_54cb7);
}

void Scowler::ob_pals(uint16_t pals)
{
	SET_HL(gb_, pals);
	gb_->cpu.c = 5;
	gb_->cpu.b = 3;
	routines_.call(ADDR_SetOBPals);
}

void Scowler::bg_pals(uint16_t pals)
{
	SET_HL(gb_, pals);
	gb_->cpu.c = 0;
	gb_->cpu.b = 8;
	routines_.call(ADDR_SetBGPals);
}

// a floating ring after $40 frames
void ScowlerHelper::ring_spawner()
{
	set_update(RING_SPAWN);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	state_duration() = 0x40;
	frame_duration() = 0x02;
	no_actions();
}

void ScowlerHelper::ring()
{
	no_actions();
	set_update(RING_RISING);
	move_left_by_var2();
}

// up out of the water, then a platform
void ScowlerHelper::ring_rising()
{
	no_actions();
	if (mem_[wGlobalCounter] & 1)
		return;
	if (--state_duration()) {
		move_up();
		return;
	}
	state_duration() = 0x3c;
	set_update(RING_WAITING);
	var2() = mem_[wCurObjYPos + 0];
	var3() = mem_[wCurObjYPos + 1];
}

void ScowlerHelper::ring_waiting()
{
	one_way_platform();
	if (!(flags() & OBJFLAG_STEPPED))
		return;
	flags() &= ~OBJFLAG_STEPPED;
	set_update(RING_UP);
}

// up with Wario a while, then back down
void ScowlerHelper::ring_up()
{
	one_way_platform();
	if (!(mem_[wGlobalCounter] & 1))
		return;
	if (!--state_duration()) {
		set_update(RING_DOWN);
		return;
	}
	if (!(flags() & OBJFLAG_STEPPED)) {
		move_up();
		return;
	}
	flags() &= ~OBJFLAG_STEPPED;
	gb_->cpu.b = 0x01;
	routines_.call(ADDR_MoveWarioUp);
	routines_.call(ADDR_HandleUpwardsFloorTransition);
	move_up();
}

void ScowlerHelper::ring_down()
{
	one_way_platform();
	if (mem_[wCurObjYPos + 0] == var2() && mem_[wCurObjYPos + 1] == var3()) {
		if (!(flags() & OBJFLAG_STEPPED))
			return;
		flags() &= ~OBJFLAG_STEPPED;
		set_update(RING_UP);
		state_duration() = 0x3c;
		return;
	}
	if (!(flags() & OBJFLAG_STEPPED)) {
		routines_.call(ADDR_MoveObjectDown_Slow);
		return;
	}
	flags() &= ~(1 << 5);
	if (!(mem_[wGlobalCounter] & 1))
		return;
	gb_->cpu.b = 0x01;
	routines_.call(ADDR_MoveWarioDown);
	routines_.call(ADDR_HandleDownwardsFloorTransition);
	move_down();
}

void ScowlerHelper::tentacle()
{
	flags() |= 1 << 4;
	set_update(TENTACLE_UPDATE);
}

// beside the body: y = body y - dy, x = body x + dx
void ScowlerHelper::tentacle_follow(uint16_t body_y, uint16_t body_x, bool other_side)
{
	bool right = mem_[wCurObjId] == SCROWLER_TENTACLE_RIGHT;
	int dy, dx;
	if (!other_side) {
		dy = right ? 0x04 : 0x0b;
		dx = right ? -0x17 : 0x0f;
	} else {
		dy = right ? 0x0b : 0x04;
		dx = right ? -0x10 : 0x16;
	}
	uint16_t y_pos = static_cast<uint16_t>(body_y - dy);
	uint16_t x_pos = static_cast<uint16_t>(body_x + dx);
	mem_[wCurObjYPos + 0] = static_cast<uint8_t>(y_pos);
	mem_[wCurObjYPos + 1] = static_cast<uint8_t>(y_pos >> 8);
	mem_[wCurObjXPos + 0] = static_cast<uint8_t>(x_pos);
	mem_[wCurObjXPos + 1] = static_cast<uint8_t>(x_pos >> 8);
}

void ScowlerHelper::tentacle_interaction(uint8_t type, uint8_t top, uint8_t bottom)
{
	no_actions();
	set_interaction(type);
	mem_[wCurObjCollBoxTop] = top;
	mem_[wCurObjCollBoxBottom] = bottom;
}

// with the body (var1: its phase)
void ScowlerHelper::tentacle_update()
{
	if (state() == 0x2a) {
		state() = 0x5a;
		play_sfx(SFX_040);
		stop_music2(mem_);
		return;
	}
	if (mem_[wRoomTransitionParam])
		return;
	state() = 0;
	uint16_t body = 0;
	for (uint8_t obj = static_cast<uint8_t>(wObj1);; obj = static_cast<uint8_t>(obj + OBJ_STRUCT_LENGTH)) {
		uint16_t base = static_cast<uint16_t>((wObj1 & 0xff00) | obj);
		if (mem_[base + OBJ_ID] == SCOWLER && (mem_[base + OBJ_FLAGS] & 1)) {
			body = base;
			break;
		}
		if (obj == static_cast<uint8_t>(wObj8))
			return;
	}
	uint16_t body_y = static_cast<uint16_t>(mem_[body + OBJ_Y_POS] | mem_[body + OBJ_Y_POS + 1] << 8);
	uint16_t body_x = static_cast<uint16_t>(mem_[body + OBJ_X_POS] | mem_[body + OBJ_X_POS + 1] << 8);
	bool right = mem_[wCurObjId] == SCROWLER_TENTACLE_RIGHT;
	switch (static_cast<uint8_t>(mem_[body + OBJ_VAR_1])) {
	case 0x00:
		tentacle_follow(body_y, body_x, mem_[body + OBJ_SUBSTATE] & (1 << 7));
		return;
	case 0xfd:
		tentacle_interaction(OBJ_INTERACTION_43, 0xfd, 0x02);
		return;
	case 0xfe:
		tentacle_interaction(OBJ_INTERACTION_WATER_TELEPORTING, 0xf6, 0xfe);
		return;
	case 0xff:
		flags() = 0;
		return;
	default: // $fc too
		tentacle_follow(body_y, body_x, right);
		return;
	}
}

// drifts, then towards Wario, up through the water; gone when hit (and
// the palettes flash) or out of it
void ScowlerHelper::ink(bool right)
{
	if (state_duration()) {
		--state_duration();
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
	} else {
		bool left_of_wario = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		routines_.call(left_of_wario ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
	}
	if (state()) {
		mem_[w1d147] = 0x01;
	} else {
		uint16_t y_pos = static_cast<uint16_t>(y() - 0x10);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_358b);
		if (mem_[wIsInWaterOrSand]) {
			if (mem_[wGlobalCounter] & 0x03)
				return;
			move_up();
			return;
		}
	}
	set_frameset(ADDR_Frameset_69af5);
	state_duration() = 0x19;
	set_update(INK_VANISH);
}

void ScowlerHelper::ink_vanish()
{
	no_actions();
	if (--state_duration())
		return;
	flags() = 0;
}

} // namespace wl3::level

using wl3::level::Scowler;
using wl3::level::ScowlerHelper;

PORT_EX(ScowlerFunc, 0, ALL_REGISTERS) { Scowler(gb).start(); }
PORT_LOCAL_EX(ScowlerFunc, Func_54520, 0, ALL_REGISTERS) { Scowler(gb).begin(); }
PORT_LOCAL_EX(ScowlerFunc, Func_54542, 0, ALL_REGISTERS) { Scowler(gb).leaving(); }
PORT_LOCAL_EX(ScowlerFunc, Func_5469d, 0, ALL_REGISTERS) { Scowler(gb).down(true); }
PORT_LOCAL_EX(ScowlerFunc, Func_546bd, 0, ALL_REGISTERS) { Scowler(gb).up(true); }
PORT_LOCAL_EX(ScowlerFunc, Func_5458a, 0, ALL_REGISTERS) { Scowler(gb).walking(); }
PORT_LOCAL_EX(ScowlerFunc, Func_546e0, 0, ALL_REGISTERS) { Scowler(gb).sinking(); }
PORT_LOCAL_EX(ScowlerFunc, Func_54770, 0, ALL_REGISTERS) { Scowler(gb).underwater(); }
PORT_LOCAL_EX(ScowlerFunc, Func_54806, 0, ALL_REGISTERS) { Scowler(gb).hurt(); }
PORT_LOCAL_EX(ScowlerFunc, Func_54853, 0, ALL_REGISTERS) { Scowler(gb).hurt_sinking(); }
PORT_LOCAL_EX(ScowlerFunc, Func_5487d, 0, ALL_REGISTERS) { Scowler(gb).rising(); }
PORT_LOCAL_EX(ScowlerFunc, Func_548c5, 0, ALL_REGISTERS) { Scowler(gb).defeated_rising(); }
PORT_LOCAL_EX(ScowlerFunc, Func_5490b, 0, ALL_REGISTERS) { Scowler(gb).defeated_sinking(); }
PORT_LOCAL_EX(ScowlerFunc, Func_54931, 0, ALL_REGISTERS) { Scowler(gb).defeated_gone(); }
PORT_EX(Func_54c07, 0, ALL_REGISTERS) { Scowler(gb).flashes(); }
PORT_EX(WriteScowlerOBPals, 0, ALL_REGISTERS) { Scowler(gb).ob_pals(REG_HL(gb)); }
PORT_EX(WriteScowlerBGPals, 0, ALL_REGISTERS) { Scowler(gb).bg_pals(REG_HL(gb)); }

PORT_EX(FloatingRingSpawnerFunc, 0, ALL_REGISTERS) { ScowlerHelper(gb).ring_spawner(); }
PORT_EX(FloatingRingFunc, 0, ALL_REGISTERS) { ScowlerHelper(gb).ring(); }
PORT_LOCAL_EX(FloatingRingFunc, Func_549c7, 0, ALL_REGISTERS) { ScowlerHelper(gb).ring_rising(); }
PORT_LOCAL_EX(FloatingRingFunc, Func_549ee, 0, ALL_REGISTERS) { ScowlerHelper(gb).ring_waiting(); }
PORT_LOCAL_EX(FloatingRingFunc, Func_54a02, 0, ALL_REGISTERS) { ScowlerHelper(gb).ring_up(); }
PORT_LOCAL_EX(FloatingRingFunc, Func_54a2d, 0, ALL_REGISTERS) { ScowlerHelper(gb).ring_down(); }
PORT_EX(ScrowlerTentacleFunc, 0, ALL_REGISTERS) { ScowlerHelper(gb).tentacle(); }
PORT_LOCAL_EX(ScrowlerTentacleFunc, Func_54a97, 0, ALL_REGISTERS) { ScowlerHelper(gb).tentacle_update(); }
PORT_EX(InkFunc, 0, ALL_REGISTERS) { ScowlerHelper(gb).ink(false); }
PORT_LOCAL_EX(InkFunc, Right, 0, ALL_REGISTERS) { ScowlerHelper(gb).ink(true); }
PORT_LOCAL_EX(InkFunc, Func_54bf8, 0, ALL_REGISTERS) { ScowlerHelper(gb).ink_vanish(); }
