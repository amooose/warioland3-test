// The zombie (zombie.asm): moves about unseen underground, rises when
// Wario's near, walks the floor and throws its head at him, then sinks
// back. Its mode on the surface is the substate's low nibble. See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t INIT = ADDR_ZombieFunc + 0x1f; // .Init
constexpr uint16_t UNDERGROUND = ADDR_ZombieFunc + 0x4f; // .OnUnderground
constexpr uint16_t START_RISE = ADDR_ZombieFunc + 0x127; // .StartRise
constexpr uint16_t RISING = ADDR_ZombieFunc + 0x16f; // .Rising
constexpr uint16_t ON_SURFACE = ADDR_ZombieFunc + 0x1a9; // .OnSurface
constexpr uint16_t HEADLESS = ADDR_ZombieFunc + 0x371; // .Headless
constexpr uint16_t HEAD_FLICKER = ADDR_ZombieHeadFunc + 0x2e; // .Flicker

enum : uint8_t {
	WALKING = 1, // .Func_45ce2 (0 too)
	TURNING = 2, // .Func_45ddc
	THROWING = 3, // .ThrowHead
};

class Zombie : public Object {
public:
	using Object::Object;

	void start(); // ZombieFunc
	void init(); // .Init
	void underground(); // .OnUnderground
	void start_rise(); // .StartRise
	void rising(); // .Rising
	void on_surface(); // .OnSurface
	void walking(); // .Func_45ce2
	void turning(); // .Func_45ddc
	void throwing(); // .ThrowHead
	void headless(); // .Headless
	// .asm_45c83: knocked away from Wario; fields through h's page
	void knocked(uint16_t page);
	void head(bool right); // ZombieHeadFunc_Left, ZombieHeadFunc_Right
	void head_fly(uint16_t movement); // ZombieHeadFunc
	void head_flicker(); // .Flicker

private:
	bool wario_to_right() const;
	void set_mode(uint8_t m) { substate() = (substate() & OBJSUBFLAGS_MASK) | m; }
	void set_walking(); // .asm_45cca
	void set_turn(uint16_t frameset); // .asm_45dcc
	void walk_on(); // .asm_45d32
	bool blocked(bool right); // a block edge without floor, or a wall
	void flicker(); // invisible every other frame
};

} // namespace

bool Zombie::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void Zombie::start()
{
	no_actions();
	set_update(INIT);
	mem_[wCurObjCollBoxRight] = 3;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-4);
	flags() |= OBJFLAG_NO_COLLISION;
	state_duration() = 0;
}

void Zombie::init()
{
	no_actions();
	set_update(UNDERGROUND);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	if (wario_to_right()) {
		substate() |= OBJSUBFLAG_HDIR;
		frame() = 0x18;
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		frame() = 0x17;
	}
}

// the edge of its block with nothing beyond (ProcessBlockOn*), or a wall
bool Zombie::blocked(bool right)
{
	uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
	if (right ? in_block + 3 >= 16 : in_block < 4) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f))
			return true;
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 3) : static_cast<uint16_t>(x() - 4);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	return gb_->cpu.a;
}

// creeps about underground, turning at walls and edges; rises when
// Wario's near (but not too near)
void Zombie::underground()
{
	no_actions();
	frame_duration() = 2;
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		flags() = 0;
		return;
	}
	bool right = facing_right();
	if (blocked(right)) {
		substate() ^= OBJSUBFLAG_HDIR;
		return;
	}
	right ? move_right() : move_left();
	if (state_duration()) {
		--state_duration();
		return;
	}
	uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
	if (dy >= 24 && dy < static_cast<uint8_t>(-24))
		return;
	uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
	if (dx >= static_cast<uint8_t>(-32) || dx < 32)
		return;
	if (dx < static_cast<uint8_t>(-64) && dx >= 64)
		return;
	set_update(START_RISE);
	state_duration() = 20;
}

void Zombie::start_rise()
{
	no_actions();
	frame_duration() = 2;
	if (--state_duration())
		return;
	if (wario_to_right()) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_68d9c);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_68d93);
	}
	state_duration() = 20;
	flags() &= ~(OBJFLAG_INVISIBLE | OBJFLAG_NO_COLLISION);
	set_update(RISING);
	play_sfx(SFX_074);
}

// its collision box grows as it rises
void Zombie::rising()
{
	if (mem_[wTransformation] == TRANSFORMATION_ZOMBIE_WARIO)
		no_actions();
	uint8_t time = --state_duration();
	if (time == 12) {
		mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-12);
		return;
	}
	if (time == 6) {
		mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-18);
		return;
	}
	if (time)
		return;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-24);
	set_update(ON_SURFACE);
	state() = 0;
	set_walking();
}

void Zombie::set_walking()
{
	set_mode(WALKING);
	set_frameset(facing_right() ? ADDR_Frameset_68d8a : ADDR_Frameset_68d81);
}

// hit: knocked away (unless Wario's a zombie himself, it vanishes when
// pounded)
void Zombie::on_surface()
{
	uint8_t s = state();
	if (s) {
		bool zombie = mem_[wTransformation] == TRANSFORMATION_ZOMBIE_WARIO;
		if (!zombie && s == 0x0a) {
			vanish();
			return;
		}
		if (!zombie || s == 0x0a) {
			set_frameset((substate() & 0x0f) == THROWING ? ADDR_Frameset_68d1f : ADDR_Frameset_68d1a);
			knocked(wCurObjSubState & 0xff00);
			return;
		}
		state() = 0;
	}
	uint8_t m = substate() & 0x0f;
	if (m > THROWING) {
		unrealistic();
		return;
	}
	switch (m) {
	case 0: case WALKING: walking(); return;
	case TURNING: turning(); return;
	case THROWING: throwing(); return;
	}
}

void Zombie::knocked(uint16_t page)
{
	Byte sub = mem_[static_cast<uint16_t>(page | OBJ_SUBSTATE)];
	sub = sub & OBJSUBFLAGS_MASK;
	mem_[static_cast<uint16_t>(page | OBJ_VAR_3)] = 0;
	mem_[static_cast<uint16_t>(page | OBJ_VAR_2)] = 0x02;
	play_sfx(SFX_017);
	uint16_t routine = wario_to_right() ? ADDR_Func_33f8 : ADDR_Func_3407;
	// high byte first, as the asm (h may point at the MBC)
	mem_[static_cast<uint16_t>(page | (OBJ_UPDATE_FUNCTION + 1))] = routine >> 8;
	mem_[static_cast<uint16_t>(page | OBJ_UPDATE_FUNCTION)] = routine & 0xff;
}

void Zombie::set_turn(uint16_t frameset)
{
	set_mode(TURNING);
	set_frameset(frameset);
	state_duration() = 32;
}

// throws its head when Wario's within 48 pixels ahead (turning first if
// he's behind)
void Zombie::walking()
{
	uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
	if (dy < 32 || dy >= static_cast<uint8_t>(-32)) {
		uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
		bool right = facing_right();
		uint16_t frameset = 0;
		if (dx >= static_cast<uint8_t>(-48)) { // Wario's close to its right
			if (!right) {
				set_turn(ADDR_Frameset_68d24);
				return;
			}
			frameset = ADDR_Frameset_68dac;
		} else if (dx < 48) { // to its left
			if (right) {
				set_turn(ADDR_Frameset_68d2b);
				return;
			}
			frameset = ADDR_Frameset_68da5;
		}
		if (frameset) {
			set_mode(THROWING);
			set_frameset(frameset);
			state_duration() = 50;
			play_sfx(SFX_064);
			return;
		}
	}
	walk_on();
}

// fast along the floor; off an edge, it goes headless (and under)
void Zombie::walk_on()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		state_duration() = 0x30;
		set_update(HEADLESS);
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	bool right = facing_right();
	if (blocked(right)) {
		set_turn(right ? ADDR_Frameset_68d2b : ADDR_Frameset_68d24);
		return;
	}
	routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
}

void Zombie::turning()
{
	if (state_duration() == 20) {
		--state_duration();
		substate() ^= OBJSUBFLAG_HDIR;
		return;
	}
	if (--state_duration())
		routines_.call(ADDR_Func_3308);
	else
		set_walking();
}

void Zombie::throwing()
{
	uint8_t time = --state_duration();
	if (time == 16) {
		SET_BC(gb_, facing_right() ? ADDR_ObjParams_ZombieHeadRight : ADDR_ObjParams_ZombieHeadLeft);
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	}
	if (time)
		return;
	state_duration() = 48;
	set_update(HEADLESS);
	flags() |= OBJFLAG_NO_COLLISION;
}

void Zombie::flicker()
{
	if (!(mem_[wGlobalCounter] & 1))
		flags() ^= OBJFLAG_INVISIBLE;
}

void Zombie::headless()
{
	no_actions();
	if (--state_duration()) {
		flicker();
		return;
	}
	flags() |= OBJFLAG_INVISIBLE;
	state() = 0;
	state_duration() = 100;
	set_update(UNDERGROUND);
}

// after a while it turns back; gone off screen
void Zombie::head(bool right)
{
	if (!state_duration()) {
		if (!(flags() & OBJFLAG_ON_SCREEN)) {
			flags() = 0;
			return;
		}
	} else if (!--state_duration()) {
		substate() ^= OBJSUBFLAG_HDIR;
		set_frameset(right ? ADDR_Frameset_68dd5 : ADDR_Frameset_68dda);
	}
	head_fly(right ? ADDR_Data_60560 : ADDR_Data_605a0);
}

void Zombie::head_fly(uint16_t movement)
{
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjXMovement);
	uint8_t s = state();
	if (!s)
		return;
	if (s < 0x0b || s >= 0x0f) {
		knocked(wCurObjSubState & 0xff00);
		return;
	}
	set_frameset(facing_right() ? ADDR_Frameset_68dda : ADDR_Frameset_68dd5);
	state_duration() = 40;
	set_update(HEAD_FLICKER);
}

void Zombie::head_flicker()
{
	no_actions();
	if (--state_duration())
		flicker();
	else
		flags() = 0;
}

} // namespace wl3::level

using Zombie = wl3::level::Zombie;

PORT_EX(ZombieFunc, 0, ALL_REGISTERS) { Zombie(gb).start(); }
PORT_LOCAL_EX(ZombieFunc, Init, 0, ALL_REGISTERS) { Zombie(gb).init(); }
PORT_LOCAL_EX(ZombieFunc, OnUnderground, 0, ALL_REGISTERS) { Zombie(gb).underground(); }
PORT_LOCAL_EX(ZombieFunc, StartRise, 0, ALL_REGISTERS) { Zombie(gb).start_rise(); }
PORT_LOCAL_EX(ZombieFunc, Rising, 0, ALL_REGISTERS) { Zombie(gb).rising(); }
PORT_LOCAL_EX(ZombieFunc, asm_45c83, 0, ALL_REGISTERS) { Zombie(gb).knocked(gb->cpu.h << 8); }
PORT_LOCAL_EX(ZombieFunc, OnSurface, 0, ALL_REGISTERS) { Zombie(gb).on_surface(); }
PORT_LOCAL_EX(ZombieFunc, Func_45ce2, 0, ALL_REGISTERS) { Zombie(gb).walking(); }
PORT_LOCAL_EX(ZombieFunc, Func_45ddc, 0, ALL_REGISTERS) { Zombie(gb).turning(); }
PORT_LOCAL_EX(ZombieFunc, ThrowHead, 0, ALL_REGISTERS) { Zombie(gb).throwing(); }
PORT_LOCAL_EX(ZombieFunc, Headless, 0, ALL_REGISTERS) { Zombie(gb).headless(); }
PORT_EX(ZombieHeadFunc_Left, 0, ALL_REGISTERS) { Zombie(gb).head(false); }
PORT_EX(ZombieHeadFunc_Right, 0, ALL_REGISTERS) { Zombie(gb).head(true); }
PORT_EX(ZombieHeadFunc, 0, ALL_REGISTERS) { Zombie(gb).head_fly(REG_BC(gb)); }
PORT_LOCAL_EX(ZombieHeadFunc, Flicker, 0, ALL_REGISTERS) { Zombie(gb).head_flicker(); }
