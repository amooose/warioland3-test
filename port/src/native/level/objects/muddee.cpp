// Muddee (muddee.asm), the boss hiding under the turtle in the muddy pool:
// once Wario rides the turtle out and it dives, Muddee jumps out throwing
// stingers, then swims after him round the pool (a move table, by var1)
// until hit enough; then the turtle resurfaces. And the turtles: platforms
// that swim right, carrying Wario. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t M(uint16_t offset) { return static_cast<uint16_t>(ADDR_MuddeeFunc + offset); }
constexpr uint16_t T1(uint16_t offset) { return static_cast<uint16_t>(ADDR_Turtle1Func + offset); }
constexpr uint16_t TC(uint16_t offset) { return static_cast<uint16_t>(ADDR_TurtleCommonFunc + offset); }

constexpr uint16_t HIDDEN = M(0x3d); // .Func_5564e
constexpr uint16_t SURFACING = M(0x65); // .Func_55676
constexpr uint16_t LURKING = M(0x85); // .Func_55696
constexpr uint16_t LEAPING = M(0xdf); // .Func_556f0
constexpr uint16_t FIGHTING = M(0x100); // .Func_55711
constexpr uint16_t DEFEATED = M(0x596); // .Func_55ba7

constexpr uint16_t T1_WAITING = T1(0x33); // .Func_55c7d
constexpr uint16_t T1_SWIMMING = T1(0x47); // .Func_55c91
constexpr uint16_t T1_STOPPED = T1(0x93); // .Func_55cdd
constexpr uint16_t T1_DIVING = T1(0xaf); // .Func_55cf9
constexpr uint16_t T1_UNDER = T1(0xce); // .Func_55d18
constexpr uint16_t T1_RESURFACING = T1(0x102); // .Func_55d4c
constexpr uint16_t TC_SWIMMING = TC(0x14); // .Func_55d90
constexpr uint16_t TC_STOPPED = TC(0x64); // .Func_55de0

class Muddee : public Object {
public:
	using Object::Object;

	void start(); // MuddeeFunc
	void hidden(); // .Func_5564e
	void surfacing(); // .Func_55676
	void lurking(); // .Func_55696
	void leaping(); // .Func_556f0
	void fighting(); // .Func_55711
	void swim_up(); // .Func_55861
	void swim_down(); // .Func_558d0
	void swim_side(bool right); // .Func_55942 / .Func_5599c
	void stop(); // .Func_559f6
	void sink(); // .Func_55a0b
	void follow(); // .Func_55a2b
	void charge_start(); // .Func_55a82
	void charging(); // .Func_55ad9
	void defeated(); // .Func_55ba7
	void sfx_0c1(); // .Func_55bd8
	void swim_sfx(); // .Func_55be7
	void set_frameset_raw(uint16_t frameset, uint8_t offset, uint8_t duration); // .Func_55c11: and the state duration
	bool above_wario() const; // .Func_55c1e: c
	bool level_with_wario() const; // .Func_55c2b: z
	void flicker(); // .Func_55c3a

private:
	void hit(); // .asm_557b1, w1d147 2
	bool waited(uint16_t frameset);
	void pos_to_h(int dy, int dx);
	void move(uint16_t normal, uint16_t slow);
	bool wario_left() const;
	void next(uint8_t move, uint16_t frameset, uint8_t offset);
	void spawn_stingers();
	void charge(); // .asm_55aa3
};

class Turtle : public Object {
public:
	using Object::Object;

	void start1(); // Turtle1Func
	void start2(); // Turtle2Func
	void wait_stepped(uint16_t then); // .Func_55c7d / TurtleCommonFunc
	void swimming1(); // .Func_55c91
	void stopped1(); // .Func_55cdd
	void diving(); // .Func_55cf9
	void under(); // .Func_55d18
	void resurfacing(); // .Func_55d4c
	void swimming(); // .Func_55d90
	void stopped() { one_way_platform(); } // .Func_55de0
	void paddle_sfx(); // Func_55e33

private:
	void init_box();
	void carry_wario(uint16_t flags_addr);
};

} // namespace

void Muddee::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(HIDDEN);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	substate() = (substate() & OBJSUBFLAGS_MASK) | 0x05;
	mem_[wCurObjCollBoxRight] = 0x07;
	mem_[wCurObjCollBoxLeft] = 0xf8;
	mem_[wCurObjCollBoxBottom] = 0x07;
	set_frameset(ADDR_Frameset_69d73);
	state_duration() = 0x3c;
	var2() = 0x70;
	move_right_by_var2();
	var2() = 0x04;
	move_up_by_var2();
}

// waits for the turtle to dive
void Muddee::hidden()
{
	no_actions();
	flicker();
	if (mem_[wMuddeeAmbushState] != 1)
		return;
	mem_[wIsBossBattle] = TRUE;
	set_update(SURFACING);
	SET_HL(gb_, ADDR_Pals_55e46);
	SET_DE(gb_, wTempPals2 + PAL_SIZE * 6);
	gb_->cpu.c = 6;
	gb_->cpu.b = 1;
	routines_.call(ADDR_CopyAndApplyOBPals);
}

void Muddee::surfacing()
{
	no_actions();
	flicker();
	if (--state_duration())
		return;
	set_update(LURKING);
	set_frameset(ADDR_Frameset_69d85);
	var3() = 0;
}

// leaps out once Wario's in range
void Muddee::lurking()
{
	no_actions();
	flicker();
	if (mem_[wWarioXPos + 0] >= 0x09 && mem_[wWarioXPos + 1] >= 0x40)
		return;
	uint8_t dy = static_cast<uint8_t>(screen_y() - mem_[wWarioScreenYPos]);
	if (dy < 0x48 || dy >= 0x96)
		return;
	set_update(LEAPING);
	set_frameset(ADDR_Frameset_69cf2);
	var3() = 0;
	flags() &= ~OBJFLAG_INVISIBLE;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[w1d147] = 0;
	spawn_stingers();
}

void Muddee::leaping()
{
	no_actions();
	if (var3() != 0x0e) {
		SET_BC(gb_, ADDR_Data_60d30);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	set_update(FIGHTING);
	var1() = 0x01;
}

// the interaction by frame, then the current move, or being hit
void Muddee::fighting()
{
	no_actions();
	uint8_t f = frame();
	if (f < 0x10) {
		uint8_t type;
		if (f >= 0x0c) {
			type = 0x0b;
		} else if (f >= 0x09) {
			substate() &= ~(1 << 7);
			type = 0x46;
		} else if (f >= 0x06) {
			type = 0x47;
		} else if (f >= 0x03) {
			substate() |= 1 << 7;
			type = 0x46;
		} else {
			type = 0x48;
		}
		set_interaction(type);
	}
	uint8_t hit_state = mem_[w1d147];
	if (!hit_state) {
		switch (var1()) {
		case 0: swim_up(); return;
		case 1: swim_down(); return;
		case 2: swim_side(false); return;
		case 3: swim_side(true); return;
		case 4: stop(); return;
		case 5: sink(); return;
		case 6: follow(); return;
		case 7: charge_start(); return;
		case 8: charging(); return;
		default: unrealistic(); return;
		}
	}
	if (hit_state == 0x02) {
		hit();
		return;
	}
	play_sfx(SFX_VANISH);
	mem_[w1d147] = 0x02;
	uint8_t type = interaction();
	if (type == 0x48)
		set_frameset(ADDR_Frameset_69d48);
	else if (type == 0x47)
		set_frameset(ADDR_Frameset_69d4d);
	else
		set_frameset(facing_right() ? ADDR_Frameset_69d52 : ADDR_Frameset_69d57);
	state_duration() = 0x3c;
	var3() = 0;
	SET_HL(gb_, wCurObjVar1);
	routines_.call(ADDR_DecrementRemainingBossHits);
}

// hit: sinks to the water (the turtle resurfacing when it's the last hit)
void Muddee::hit()
{
	if (state_duration()) {
		if (state_duration() == 0x28)
			play_sfx(SFX_0C0);
		if (--state_duration())
			return;
		if ((substate() & 0x0f) != 0x02)
			return;
		set_frameset(ADDR_Frameset_69d6a);
		state_duration() = 0x28;
		var1() = 0x14;
		set_update(DEFEATED);
		substate() &= 0x80;
		if (static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a))
			substate() &= ~(1 << 7);
		else
			substate() |= 1 << 7;
		var3() = 0;
		flags() |= OBJFLAG_PRIORITY;
		SET_HL(gb_, wCurObjUnk02);
		gb_->cpu.e = 0x07;
		routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
		stop_music2(mem_);
		mem_[wMuddeeAmbushState] = 2;
		return;
	}
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!mem_[wIsInWaterOrSand]) {
		SET_BC(gb_, ADDR_Data_60d40);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	mem_[w1d147] = 0;
	set_frameset(ADDR_Frameset_69dc0);
	state_duration() = 0x2c;
	var1() = 0x04;
	play_sfx(SFX_083);
}

// the state duration's countdown before a move, the frameset set as it ends;
// true when it's run out
bool Muddee::waited(uint16_t frameset)
{
	if (!state_duration())
		return true;
	if (!--state_duration())
		set_frameset(frameset);
	return false;
}

void Muddee::pos_to_h(int dy, int dx)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
}

// slow in the first round (substate 5)
void Muddee::move(uint16_t normal, uint16_t slow)
{
	swim_sfx();
	routines_.call((substate() & 0x0f) == 0x05 ? slow : normal);
}

bool Muddee::wario_left() const
{
	uint8_t hi = mem_[wCurObjXPos + 1];
	uint8_t w_hi = mem_[wWarioXPos + 0];
	if (w_hi != hi)
		return w_hi < hi;
	return mem_[wWarioXPos + 1] < static_cast<uint8_t>(mem_[wCurObjXPos + 0]);
}

void Muddee::next(uint8_t move, uint16_t frameset, uint8_t offset)
{
	var1() = move;
	set_frameset_raw(frameset, offset, 0x7d);
}

void Muddee::spawn_stingers()
{
	play_sfx(SFX_0B2);
	SET_BC(gb_, ADDR_ObjParams_MuddeeStinger1);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
	SET_BC(gb_, ADDR_ObjParams_MuddeeStinger2);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

// up to Wario's height
void Muddee::swim_up()
{
	if (!waited(ADDR_Frameset_69d04))
		return;
	pos_to_h(-0x1c, 0);
	routines_.call(ADDR_Func_358b);
	if (!gb_->cpu.a && !level_with_wario()) {
		move(ADDR_MoveObjectUp, ADDR_MoveObjectUp_Slow);
		return;
	}
	if (wario_left())
		next(0x02, ADDR_Frameset_69d16, 0x0e);
	else
		next(0x03, ADDR_Frameset_69d2f, 0x0e);
}

// down to Wario's height
void Muddee::swim_down()
{
	if (!waited(ADDR_Frameset_69cf2))
		return;
	pos_to_h(0x30, 0);
	routines_.call(ADDR_Func_352b);
	if (!mem_[wIsInWaterOrSand] && !level_with_wario()) {
		move(ADDR_MoveObjectDown, ADDR_MoveObjectDown_Slow);
		return;
	}
	if (wario_left())
		next(0x02, ADDR_Frameset_69d2f, 0x02);
	else
		next(0x03, ADDR_Frameset_69d16, 0x02);
}

// to a wall, then up or down
void Muddee::swim_side(bool right)
{
	if (!waited(right ? ADDR_Frameset_69cfb : ADDR_Frameset_69d0d))
		return;
	pos_to_h(0, right ? 0x18 : -0x18);
	routines_.call(ADDR_Func_3513);
	if (!gb_->cpu.a) {
		if (right)
			move(ADDR_MoveObjectRight, ADDR_MoveObjectRight_Slow);
		else
			move(ADDR_MoveObjectLeft, ADDR_MoveObjectLeft_Slow);
		return;
	}
	if (above_wario())
		next(0x01, right ? ADDR_Frameset_69d2f : ADDR_Frameset_69d16, 0x14);
	else
		next(0x00, right ? ADDR_Frameset_69d16 : ADDR_Frameset_69d2f, 0x08);
}

void Muddee::stop()
{
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_69d85);
	var1() = 0x05;
	var2() = 0x08;
	move_down_by_var2();
}

void Muddee::sink()
{
	pos_to_h(0x12, 0);
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		move_down();
		return;
	}
	var1() = 0x06;
}

// along the floor under Wario, then more stingers
void Muddee::follow()
{
	uint8_t lo = mem_[wCurObjXPos + 0], hi = mem_[wCurObjXPos + 1];
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x40);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	uint8_t w_hi = mem_[wWarioXPos + 0], w_lo = mem_[wWarioXPos + 1];
	int dir = w_hi < hi ? -1 : w_hi > hi ? 1 : w_lo < lo ? -1 : w_lo > lo ? 1 : 0;
	if (!dir) {
		set_frameset(ADDR_Frameset_69d04);
		var1() = 0x07;
		var3() = 0;
		spawn_stingers();
		return;
	}
	uint16_t x_pos = static_cast<uint16_t>(x() + (dir > 0 ? 0x08 : -0x08));
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return;
	routines_.call(dir > 0 ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
}

void Muddee::charge_start()
{
	if (var3() != 0x0e) {
		SET_BC(gb_, ADDR_Data_60d30);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	if ((substate() & 0x0f) != 0x03) {
		var1() = 0;
		return;
	}
	state_duration() = 0;
	var1() = 0x08;
	charge();
}

void Muddee::charge()
{
	bool left = wario_left();
	var2() = left ? 0 : 1;
	var3() = 0;
	if (above_wario())
		substate() |= 1 << 6;
	else
		substate() &= ~(1 << 6);
	set_frameset(left ? ADDR_Frameset_69d8e : ADDR_Frameset_69da7);
}

// round the pool, bouncing off walls, floor and surface; three laps of
// frames, then a pause
void Muddee::charging()
{
	if (var3() != 0xff && !frame_duration() && mem_[wCurObjFramesetOffset] == 0x18 && ++var3() == 0x03) {
		var3() = 0xff;
		set_frameset(ADDR_Frameset_69cf2);
		state_duration() = 0x3c;
		return;
	}
	if (state_duration()) {
		if (--state_duration())
			return;
		if (var3() != 0xff)
			return;
		var3() = 0;
		charge();
		return;
	}
	sfx_0c1();
	bool right = var2() & 1;
	pos_to_h(0, right ? 0x18 : -0x18);
	routines_.call(ADDR_Func_3513);
	if (!gb_->cpu.a) {
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
	} else {
		var2() ^= 0x01;
		state_duration() = 0x28;
	}
	if (!(substate() & (1 << 6))) {
		pos_to_h(-0x1c, 0);
		routines_.call(ADDR_Func_358b);
		if (!gb_->cpu.a) {
			routines_.call(ADDR_MoveObjectUp_Slow);
			return;
		}
	} else {
		pos_to_h(0x30, 0);
		routines_.call(ADDR_Func_352b);
		if (!mem_[wIsInWaterOrSand]) {
			routines_.call(ADDR_MoveObjectDown_Slow);
			return;
		}
	}
	substate() ^= OBJSUBFLAG_VDIR;
	state_duration() = 0x28;
}

// floats off
void Muddee::defeated()
{
	no_actions();
	if (var1()) {
		--var1();
	} else if (state_duration()) {
		if (!--state_duration())
			play_sfx(SFX_0A3);
		return;
	}
	SET_BC(gb_, ADDR_Data_60180);
	routines_.call(ADDR_ApplyObjYMovement);
	routines_.call(facing_right() ? ADDR_MoveObjectLeft : ADDR_MoveObjectRight);
}

void Muddee::sfx_0c1()
{
	if (!(mem_[wGlobalCounter] & 0x0f))
		play_sfx(SFX_0C1);
}

// faster in the later rounds
void Muddee::swim_sfx()
{
	uint8_t round = substate() & 0x0f;
	uint8_t mask = round == 0x05 ? 0x1f : round == 0x04 ? 0x0f : 0x07;
	if (!(mem_[wGlobalCounter] & mask))
		play_sfx(SFX_085);
}

void Muddee::set_frameset_raw(uint16_t frameset, uint8_t offset, uint8_t duration)
{
	mem_[wCurObjFramesetPtr + 0] = static_cast<uint8_t>(frameset);
	mem_[wCurObjFramesetPtr + 1] = static_cast<uint8_t>(frameset >> 8);
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = offset;
	state_duration() = duration;
}

bool Muddee::above_wario() const
{
	return static_cast<uint8_t>(screen_y() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a);
}

bool Muddee::level_with_wario() const
{
	return (screen_y() & 0xf8) == (static_cast<uint8_t>(mem_[wWarioScreenYPos] - 0x10) & 0xf8);
}

void Muddee::flicker()
{
	if (mem_[wGlobalCounter] & 0x03)
		flags() &= ~(1 << 4);
	else
		flags() |= 1 << 4;
}

void Turtle::init_box()
{
	mem_[wCurObjCollBoxRight] = 0x0a;
	mem_[wCurObjCollBoxLeft] = 0xf5;
	mem_[wCurObjCollBoxBottom] = 0x04;
	set_frameset(ADDR_Frameset_69d5c);
}

// the turtle Muddee hides under
void Turtle::start1()
{
	mem_[wMuddeeAmbushState] = 0;
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(T1_WAITING);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	init_box();
	state_duration() = 0xa0;
	var2() = 0x10;
	move_up_by_var2();
}

// destroyed while a turtle 1 is about
void Turtle::start2()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(ADDR_TurtleCommonFunc);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	init_box();
	var2() = 0x10;
	move_up_by_var2();
	for (uint8_t obj = static_cast<uint8_t>(wObj1);; obj = static_cast<uint8_t>(obj + OBJ_STRUCT_LENGTH)) {
		uint16_t base = static_cast<uint16_t>((wObj1 & 0xff00) | obj);
		if (mem_[base + OBJ_ID] == TURTLE1 && (mem_[base + OBJ_FLAGS] & 1)) {
			flags() = 0;
			return;
		}
		if (obj == static_cast<uint8_t>(wObj8))
			return;
	}
}

// once Wario stands on it
void Turtle::wait_stepped(uint16_t then)
{
	one_way_platform();
	if (!(flags() & OBJFLAG_STEPPED))
		return;
	flags() &= ~OBJFLAG_STEPPED;
	set_update(then);
}

void Turtle::carry_wario(uint16_t flags_addr)
{
	if (!(mem_[flags_addr] & OBJFLAG_STEPPED))
		return;
	mem_[flags_addr] &= ~OBJFLAG_STEPPED;
	routines_.farcall(BANK_Func_19706, ADDR_Func_19706);
	if (gb_->cpu.b)
		return;
	gb_->cpu.b = 0x01;
	routines_.call(ADDR_MoveWarioRight);
}

// out across the pool, then stops
void Turtle::swimming1()
{
	one_way_platform();
	paddle_sfx();
	if (!(mem_[wGlobalCounter] & 1))
		return;
	if (--state_duration()) {
		move_right();
		carry_wario(static_cast<uint16_t>((gb_->cpu.h << 8) | OBJ_FLAGS));
		return;
	}
	set_update(T1_STOPPED);
	set_frameset(ADDR_Frameset_69d65);
	state_duration() = 0x3c;
	play_sfx(SFX_0CA);
}

void Turtle::stopped1()
{
	one_way_platform();
	if (--state_duration())
		return;
	state_duration() = 0x28;
	var1() = 0x40;
	set_update(T1_DIVING);
	mem_[wMuddeeAmbushState] = 1;
}

void Turtle::diving()
{
	one_way_platform();
	if (state_duration()) {
		--state_duration();
		return;
	}
	no_actions();
	if (--var1()) {
		move_down();
		return;
	}
	set_update(T1_UNDER);
}

// until Muddee's beaten
void Turtle::under()
{
	no_actions();
	if (mem_[wMuddeeAmbushState] != 2)
		return;
	set_frameset(ADDR_Frameset_69d5c);
	state_duration() = 0x3c;
	var1() = 0x40;
	set_update(T1_RESURFACING);
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x06;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
}

// then the boss fight's over, and it's a turtle like the others
void Turtle::resurfacing()
{
	one_way_platform();
	if (!(mem_[wGlobalCounter] & 1))
		return;
	if (state_duration()) {
		if (--state_duration())
			return;
		mem_[wBossState] = BOSS_DEFEATED;
		routines_.call(ADDR_UpdateLevelMusic);
		mem_[wIsBossBattle] = 0;
		return;
	}
	if (--var1()) {
		move_up();
		return;
	}
	set_update(ADDR_TurtleCommonFunc);
	flags() &= ~(1 << 5);
}

// right to a wall, carrying Wario
void Turtle::swimming()
{
	one_way_platform();
	paddle_sfx();
	if (!(mem_[wGlobalCounter] & 1))
		return;
	uint16_t x_pos = static_cast<uint16_t>(x() + 0x30);
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		set_update(TC_STOPPED);
		return;
	}
	move_right();
	carry_wario(wCurObjFlags);
}

void Turtle::paddle_sfx()
{
	if (!frame_duration() && mem_[wCurObjFramesetOffset] == 0x04)
		play_sfx(SFX_084);
}

} // namespace wl3::level

using wl3::level::Muddee;
using wl3::level::Turtle;

PORT_EX(MuddeeFunc, 0, ALL_REGISTERS) { Muddee(gb).start(); }
PORT_LOCAL_EX(MuddeeFunc, Func_5564e, 0, ALL_REGISTERS) { Muddee(gb).hidden(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55676, 0, ALL_REGISTERS) { Muddee(gb).surfacing(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55696, 0, ALL_REGISTERS) { Muddee(gb).lurking(); }
PORT_LOCAL_EX(MuddeeFunc, Func_556f0, 0, ALL_REGISTERS) { Muddee(gb).leaping(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55711, 0, ALL_REGISTERS) { Muddee(gb).fighting(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55861, 0, ALL_REGISTERS) { Muddee(gb).swim_up(); }
PORT_LOCAL_EX(MuddeeFunc, Func_558d0, 0, ALL_REGISTERS) { Muddee(gb).swim_down(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55942, 0, ALL_REGISTERS) { Muddee(gb).swim_side(false); }
PORT_LOCAL_EX(MuddeeFunc, Func_5599c, 0, ALL_REGISTERS) { Muddee(gb).swim_side(true); }
PORT_LOCAL_EX(MuddeeFunc, Func_559f6, 0, ALL_REGISTERS) { Muddee(gb).stop(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55a0b, 0, ALL_REGISTERS) { Muddee(gb).sink(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55a2b, 0, ALL_REGISTERS) { Muddee(gb).follow(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55a82, 0, ALL_REGISTERS) { Muddee(gb).charge_start(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55ad9, 0, ALL_REGISTERS) { Muddee(gb).charging(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55ba7, 0, ALL_REGISTERS) { Muddee(gb).defeated(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55bd8, 0, ALL_REGISTERS) { Muddee(gb).sfx_0c1(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55be7, 0, ALL_REGISTERS) { Muddee(gb).swim_sfx(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55c11, 0, ALL_REGISTERS) { Muddee(gb).set_frameset_raw(REG_DE(gb), gb->cpu.c, gb->cpu.b); }
PORT_LOCAL_EX(MuddeeFunc, Func_55c1e, 0, ALL_REGISTERS) { Muddee(gb).above_wario(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55c2b, 0, ALL_REGISTERS) { Muddee(gb).level_with_wario(); }
PORT_LOCAL_EX(MuddeeFunc, Func_55c3a, 0, ALL_REGISTERS) { Muddee(gb).flicker(); }

PORT_EX(Turtle1Func, 0, ALL_REGISTERS) { Turtle(gb).start1(); }
PORT_LOCAL_EX(Turtle1Func, Func_55c7d, 0, ALL_REGISTERS) { Turtle(gb).wait_stepped(wl3::level::T1_SWIMMING); }
PORT_LOCAL_EX(Turtle1Func, Func_55c91, 0, ALL_REGISTERS) { Turtle(gb).swimming1(); }
PORT_LOCAL_EX(Turtle1Func, Func_55cdd, 0, ALL_REGISTERS) { Turtle(gb).stopped1(); }
PORT_LOCAL_EX(Turtle1Func, Func_55cf9, 0, ALL_REGISTERS) { Turtle(gb).diving(); }
PORT_LOCAL_EX(Turtle1Func, Func_55d18, 0, ALL_REGISTERS) { Turtle(gb).under(); }
PORT_LOCAL_EX(Turtle1Func, Func_55d4c, 0, ALL_REGISTERS) { Turtle(gb).resurfacing(); }
PORT_EX(Turtle2Func, 0, ALL_REGISTERS) { Turtle(gb).start2(); }
PORT_EX(TurtleCommonFunc, 0, ALL_REGISTERS) { Turtle(gb).wait_stepped(wl3::level::TC_SWIMMING); }
PORT_LOCAL_EX(TurtleCommonFunc, Func_55d90, 0, ALL_REGISTERS) { Turtle(gb).swimming(); }
PORT_LOCAL_EX(TurtleCommonFunc, Func_55de0, 0, ALL_REGISTERS) { Turtle(gb).stopped(); }
PORT_EX(Func_55e33, 0, ALL_REGISTERS) { Turtle(gb).paddle_sfx(); }
