// The snake (snake.asm): a snake in a pot that rises up (a platform,
// carrying Wario), turns to face him, breathes fire, and sinks back. Its
// mode is the substate's low nibble; var2/var3 hold its start. See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t INIT = ADDR_SnakeFunc + 0x14; // .Init
constexpr uint16_t UPDATE = ADDR_SnakeFunc + 0x53; // .Update
constexpr uint16_t FIRE_UPDATE = ADDR_SnakeFireFunc + 0x13; // .Update
constexpr uint8_t SNAKE_ID = 0x2d;

class Snake : public Object {
public:
	using Object::Object;

	void start(); // SnakeFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint8_t mode);
	void waiting(); // .Func_49943 (modes 0, 1)
	void rising(); // .Func_49957
	void looking(); // .Func_49984
	void opening(); // .Func_499b8
	void breathing(); // .Func_499f8
	void closing(); // .Func_49a57
	void closed(); // .Func_49a7c
	void set_sinking(uint16_t zero_addr); // .Func_49a85: clears [hl], substate in h's page
	void sinking(); // .Func_49aa1
	void face_wario(); // .Func_49af0
	void check_on_screen(); // .Func_49b76
	void fire(); // SnakeFireFunc
	void fire_update(); // .Update

private:
	uint8_t mode() const { return substate() & 0x0f; }
	void next_mode() { ++substate(); }
	void set_mode(uint8_t m) { substate() = (substate() & 0xf0) | m; }
	void carry_wario(bool up); // Wario on it: half speed, with him
	void set_close(); // .asm_49a5f
	void breath_sfx(); // SFX_08A at the animation's 10th frameset step
};

} // namespace

void Snake::start()
{
	set_update(INIT);
	var2() = mem_[wCurObjYPos + 0]; // its start
	var3() = mem_[wCurObjYPos + 1];
}

void Snake::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_6881d);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_68804);
	}
	state_duration() = 0x78;
	var1() = 0;
	mem_[wCurObjCollBoxRight] = 0xff;
	mem_[wCurObjCollBoxLeft] = 0xf1;
	mem_[wCurObjCollBoxBottom] = 0x0f;
}

void Snake::update()
{
	one_way_platform();
	mem_[wCurObjCollBoxBottom] = 0x0f;
	face_wario();
	uint8_t s = state();
	if (s) {
		state() = 0;
		flags() &= ~OBJFLAG_NO_COLLISION;
		if (s == 0x0a) {
			play_sfx(SFX_VANISH);
			set_sinking(wCurObjVar1);
			return;
		}
		if ((s & 0xfe) == 0x04 || (s & 0xfe) == 0x28) {
			set_sinking(wCurObjVar1);
			return;
		}
	}
	uint8_t m = mode();
	if (m > 8) {
		unrealistic();
		return;
	}
	run(m);
}

void Snake::run(uint8_t m)
{
	switch (m) {
	case 0: case 1: waiting(); return;
	case 2: rising(); return;
	case 3: looking(); return;
	case 4: opening(); return;
	case 5: breathing(); return;
	case 6: closing(); return;
	case 7: closed(); return;
	case 8: sinking(); return;
	}
}

void Snake::waiting()
{
	if (--state_duration())
		return;
	state_duration() = 0x3e;
	next_mode();
	play_sfx(SFX_11A);
}

void Snake::carry_wario(bool up)
{
	if (!(flags() & OBJFLAG_STEPPED)) {
		routines_.call(up ? ADDR_MoveObjectUp_Slow : ADDR_MoveObjectDown_Slow);
		return;
	}
	flags() &= ~OBJFLAG_STEPPED;
	mem_[wCurObjCollBoxBottom] = 0;
	if (!(mem_[wGlobalCounter] & 1))
		return;
	gb_->cpu.b = 1;
	routines_.call(up ? ADDR_MoveWarioUp : ADDR_MoveWarioDown);
	routines_.call(up ? ADDR_HandleUpwardsFloorTransition : ADDR_HandleDownwardsFloorTransition);
	up ? move_up() : move_down();
}

void Snake::rising()
{
	check_on_screen();
	if (!--state_duration()) {
		state_duration() = 0x53;
		next_mode();
		return;
	}
	carry_wario(true);
}

// waits, then opens its mouth at the right frame of its look
void Snake::looking()
{
	if (state_duration()) {
		--state_duration();
		return;
	}
	if (var1()) {
		next_mode();
		state_duration() = 0x31;
		return;
	}
	bool right = facing_right();
	if (frame() != (right ? 0x0e : 0x05))
		return;
	next_mode();
	set_frameset(right ? ADDR_Frameset_688c6 : ADDR_Frameset_68852);
	state_duration() = 0x31;
}

void Snake::breath_sfx()
{
	if (mem_[wCurObjFramesetOffset] == 0x0a && !frame_duration())
		play_sfx(SFX_08A);
}

void Snake::opening()
{
	if (state_duration()) {
		--state_duration();
		breath_sfx();
		return;
	}
	if (var1())
		return;
	bool right = facing_right();
	if (frame() != (right ? 0x45 : 0x22))
		return;
	next_mode();
	set_frameset(right ? ADDR_Frameset_688e3 : ADDR_Frameset_6886f);
	state_duration() = 0x31;
	var1() = 0x02;
}

// breathes fire (if an object slot's free)
void Snake::breathing()
{
	var1() = 0x02;
	if (--state_duration()) {
		breath_sfx();
		return;
	}
	for (uint16_t obj = wObj1; ; obj = static_cast<uint16_t>(obj + OBJ_STRUCT_LENGTH)) {
		if (!(mem_[obj] & OBJFLAG_ACTIVE))
			break;
		if (obj == wObj8) {
			set_close();
			return;
		}
	}
	state_duration() = 0x93;
	play_sfx(SFX_089);
	next_mode();
	bool right = facing_right();
	set_frameset(right ? ADDR_Frameset_68900 : ADDR_Frameset_6888c);
	SET_BC(gb_, right ? ADDR_ObjParams_SnakeFireRight : ADDR_ObjParams_SnakeFireLeft);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void Snake::closing()
{
	var1() = 0x02;
	if (!--state_duration())
		set_close();
}

void Snake::set_close()
{
	state_duration() = 0x31;
	set_mode(7);
	set_frameset(facing_right() ? ADDR_Frameset_6891d : ADDR_Frameset_688a9);
}

void Snake::closed()
{
	var1() = 0x02;
	if (!--state_duration())
		set_sinking(wCurObjVar1);
}

void Snake::set_sinking(uint16_t zero_addr)
{
	mem_[zero_addr] = 0;
	Byte sub = mem_[static_cast<uint16_t>((zero_addr & 0xff00) | OBJ_SUBSTATE)];
	uint8_t value = static_cast<uint8_t>((sub & 0xf0) | 8);
	sub = value;
	set_frameset((value & 0x80) ? ADDR_Frameset_6881d : ADDR_Frameset_68804);
	state_duration() = 0x01;
}

// back down to its start (hissing), then waits
void Snake::sinking()
{
	check_on_screen();
	uint16_t start = static_cast<uint16_t>(var3() << 8 | var2());
	if (y() == start) {
		state_duration() = 0x78;
		set_mode(1);
		return;
	}
	if (state_duration()) {
		--state_duration();
		play_sfx(SFX_08B);
	}
	carry_wario(false);
}

// turns to Wario (var1 frames to settle) when he's near its height
void Snake::face_wario()
{
	if (var1()) {
		if (--var1())
			return;
		bool open = mode() == 4;
		if (facing_right())
			set_frameset(open ? ADDR_Frameset_688c6 : ADDR_Frameset_6881d);
		else
			set_frameset(open ? ADDR_Frameset_68852 : ADDR_Frameset_68804);
		return;
	}
	uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
	if (dy >= 0x28 && dy < 0xe8)
		return;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	bool wario_right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
	if (wario_right == facing_right())
		return;
	bool open = mode() == 4;
	if (wario_right) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(open ? ADDR_Frameset_68844 : ADDR_Frameset_68836);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(open ? ADDR_Frameset_6884b : ADDR_Frameset_6883d);
	}
	var1() = 0x14;
}

// state $48 (no collision with Wario) near the screen's top or bottom
void Snake::check_on_screen()
{
	uint8_t sy = screen_y();
	state() = (sy < 0x10 || (sy >= 0xa0 && sy < 0xc0)) ? 0x48 : 0;
}

void Snake::fire()
{
	no_actions();
	flags() |= OBJFLAG_INVISIBLE;
	set_update(FIRE_UPDATE);
}

// lasts while its snake (the active, lasting object of the same
// OBJ_UNK_01/02) is breathing (mode 6)
void Snake::fire_update()
{
	uint8_t unk01 = mem_[wCurObjUnk01], unk02 = mem_[wCurObjUnk02];
	for (int n = 0; n < NUM_OBJECTS; n++) {
		uint16_t obj = static_cast<uint16_t>(wObj1 + n * OBJ_STRUCT_LENGTH);
		if (mem_[static_cast<uint16_t>(obj + OBJ_UNK_02)] != unk02)
			continue;
		if (mem_[static_cast<uint16_t>(obj + OBJ_UNK_01)] != unk01)
			continue;
		if ((mem_[obj] & (OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT)) != OBJFLAG_ACTIVE)
			continue;
		if (mem_[static_cast<uint16_t>(obj + OBJ_ID)] != SNAKE_ID)
			continue;
		if ((mem_[static_cast<uint16_t>(obj + OBJ_SUBSTATE)] & 0x0f) == 6)
			return;
		break;
	}
	flags() = 0;
}

} // namespace wl3::level

using Snake = wl3::level::Snake;

PORT_EX(SnakeFunc, 0, ALL_REGISTERS) { Snake(gb).start(); }
PORT_LOCAL_EX(SnakeFunc, Init, 0, ALL_REGISTERS) { Snake(gb).init(); }
PORT_LOCAL_EX(SnakeFunc, Update, 0, ALL_REGISTERS) { Snake(gb).update(); }
PORT_LOCAL_EX(SnakeFunc, Func_49943, 0, ALL_REGISTERS) { Snake(gb).waiting(); }
PORT_LOCAL_EX(SnakeFunc, Func_49957, 0, ALL_REGISTERS) { Snake(gb).rising(); }
PORT_LOCAL_EX(SnakeFunc, Func_49984, 0, ALL_REGISTERS) { Snake(gb).looking(); }
PORT_LOCAL_EX(SnakeFunc, Func_499b8, 0, ALL_REGISTERS) { Snake(gb).opening(); }
PORT_LOCAL_EX(SnakeFunc, Func_499f8, 0, ALL_REGISTERS) { Snake(gb).breathing(); }
PORT_LOCAL_EX(SnakeFunc, Func_49a57, 0, ALL_REGISTERS) { Snake(gb).closing(); }
PORT_LOCAL_EX(SnakeFunc, Func_49a7c, 0, ALL_REGISTERS) { Snake(gb).closed(); }
PORT_LOCAL_EX(SnakeFunc, Func_49a85, 0, ALL_REGISTERS) { Snake(gb).set_sinking(REG_HL(gb)); }
PORT_LOCAL_EX(SnakeFunc, Func_49aa1, 0, ALL_REGISTERS) { Snake(gb).sinking(); }
PORT_LOCAL_EX(SnakeFunc, Func_49af0, 0, ALL_REGISTERS) { Snake(gb).face_wario(); }
PORT_LOCAL_EX(SnakeFunc, Func_49b76, 0, ALL_REGISTERS) { Snake(gb).check_on_screen(); }
PORT_EX(SnakeFireFunc, 0, ALL_REGISTERS) { Snake(gb).fire(); }
PORT_LOCAL_EX(SnakeFireFunc, Update, 0, ALL_REGISTERS) { Snake(gb).fire_update(); }
