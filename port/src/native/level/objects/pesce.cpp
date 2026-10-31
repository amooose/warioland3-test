// Pesce (pesce.asm), the boss fish swimming the pool's floor: he swallows
// Wario when he catches him, and chases the cheese a fly drops; knocked
// three times (w1d147) he's beaten. Wario rides dragonflies over the pool.
// And the strong water current, the dragonfly and fly spawners, the
// dragonflies, the fly and its cheese. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t DS(uint16_t offset) { return static_cast<uint16_t>(ADDR_DragonflySpawnerFunc + offset); }
constexpr uint16_t D1(uint16_t offset) { return static_cast<uint16_t>(ADDR_Dragonfly1Func + offset); }
constexpr uint16_t D2(uint16_t offset) { return static_cast<uint16_t>(ADDR_Dragonfly2Func + offset); }
constexpr uint16_t FS(uint16_t offset) { return static_cast<uint16_t>(ADDR_FlySpawnerFunc + offset); }
constexpr uint16_t PE(uint16_t rom) { return static_cast<uint16_t>(ADDR_PesceFunc + (rom - 0x5d2e)); }

constexpr uint16_t CURRENT_UPDATE = ADDR_StrongWaterCurrentFunc + 0x34; // .Func_51d2d
constexpr uint16_t PESCE_UPDATE = PE(0x5e28); // .Func_51e28
constexpr uint16_t PESCE_BEATEN = PE(0x61d0); // .Func_521d0
constexpr uint16_t FLY_UPDATE = ADDR_FlyFunc + 0x20; // .Func_52533

// an active object with that id: its address, else 0
uint16_t find_active(Memory mem, uint8_t id)
{
	for (uint8_t obj = static_cast<uint8_t>(wObj1);; obj = static_cast<uint8_t>(obj + OBJ_STRUCT_LENGTH)) {
		uint16_t base = static_cast<uint16_t>((wObj1 & 0xff00) | obj);
		if (mem[base + OBJ_ID] == id && (mem[base + OBJ_FLAGS] & 1))
			return base;
		if (obj == static_cast<uint8_t>(wObj8))
			return 0;
	}
}

class Pesce : public Object {
public:
	using Object::Object;

	void start(); // PesceFunc
	void update(); // .Func_51e28
	void knocked(uint16_t at); // .asm_51d76: at, the state
	void swallow(uint16_t at); // .asm_51dcb: at, the state
	void spit_wait(uint16_t page); // .asm_51e5d
	void spit(uint16_t page); // .asm_51e6e
	void sink_wario(uint16_t page); // .asm_51e97
	void rising(); // .asm_51efc
	void swim(); // .asm_51f46
	void chase(); // .asm_51f8a
	void turn(uint16_t page); // .asm_52068
	void lunge(); // .asm_520c5
	void bite(); // .asm_52108
	void swim_frameset(); // .asm_52155
	void beaten(); // .Func_521d0

private:
	uint16_t pick(int n, const uint16_t (*framesets)[2], bool right);
	void hpos(uint16_t page, int dy, int dx);
};

class PesceHelper : public Object {
public:
	using Object::Object;

	void current(); // StrongWaterCurrentFunc
	void dragonfly_spawner(); // DragonflySpawnerFunc
	void dragonfly_wait(); // .Func_52282
	void dragonfly_spawning(); // .Func_5229a
	void dragonfly_done(); // .Func_522d9
	void dragonfly1() { set_update(D1(0x0a)); } // Dragonfly1Func
	void dragonfly1_down(); // .Func_52321
	void dragonfly1_flying(); // .Func_5233b
	void dragonfly1_off(); // .Func_523b1
	void dragonfly2(); // Dragonfly2Func
	void dragonfly2_wait(); // .Func_523f2
	void dragonfly2_flying(); // .Func_5240f
	void fly_spawner(); // FlySpawnerFunc
	void fly_wait(); // .Func_52475
	void fly_spawning(); // .Func_52497
	void fly_done(); // .Func_524eb
	void fly(); // FlyFunc
	void fly_update(); // .Func_52533
	void cheese(); // CheeseFunc

private:
	void spawner_frame();
	void spawner_done(uint8_t e);
	void move_wario(uint16_t flags_addr, uint16_t routine, uint16_t then);
};

} // namespace

void Pesce::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(PESCE_UPDATE);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 0x0f;
	mem_[wCurObjCollBoxLeft] = 0xf0;
	mem_[wCurObjCollBoxBottom] = 0xfe;
	set_frameset(ADDR_Frameset_6990a);
	mem_[w1d147] = 0;
	mem_[wDollBoyHammerRange] = 0;
	state_duration() = 0;
	var1() = 0;
	var2() = 0x0a;
	var3() = 0;
	move_down_by_var2();
	var2() = 0x3c;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = TRUE;
}

// the phase (var1), after a knock (states 8 and 9) or Wario caught ($2a)
void Pesce::update()
{
	flags() &= ~OBJFLAG_GRABBED;
	uint8_t s = state();
	if (s) {
		if (s == 0x2a) {
			swallow(wCurObjState);
			return;
		}
		if ((s & 0xfe) == 0x08) {
			knocked(wCurObjState);
			return;
		}
		state() = 0;
	}
	switch (var1()) {
	case 0: chase(); return;
	case 1: rising(); return;
	case 2: spit(wCurObjState & 0xff00); return;
	case 3: sink_wario(wCurObjState & 0xff00); return;
	case 4: no_actions(); return;
	case 5: spit_wait(wCurObjState & 0xff00); return;
	default: return;
	}
}

// the collision box higher at each knock; the third beats him
void Pesce::knocked(uint16_t at)
{
	mem_[at] = 0;
	state_duration() = 0;
	play_sfx(SFX_0BB);
	uint8_t knocks = ++mem_[w1d147];
	uint8_t top;
	if (knocks == 0x01) {
		top = 0xef;
	} else if (knocks == 0x02) {
		top = 0xeb;
	} else if (knocks == 0x03) {
		mem_[wCurObjCollBoxTop] = 0xe6;
		set_update(PESCE_BEATEN);
		set_interaction(OBJ_INTERACTION_51);
		stop_music2(mem_);
		var1() = 0;
		swim_frameset();
		return;
	} else {
		mem_[w1d147] = 0;
		top = 0xf2;
	}
	mem_[wCurObjCollBoxTop] = top;
	var1() = 0;
	swim_frameset();
}

// Wario in his mouth: carried to the left of him
void Pesce::swallow(uint16_t at)
{
	stop_music2(mem_);
	play_sfx(SFX_FAT_WALK);
	mem_[at] = 0;
	uint16_t page = at & 0xff00;
	uint16_t y_pos = static_cast<uint16_t>(mem_[page | OBJ_Y_POS] | mem_[page | (OBJ_Y_POS + 1)] << 8);
	y_pos = static_cast<uint16_t>(y_pos + 0x04);
	mem_[wWarioYPos + 1] = static_cast<uint8_t>(y_pos);
	mem_[wWarioYPos + 0] = static_cast<uint8_t>(y_pos >> 8);
	uint16_t x_pos = static_cast<uint16_t>(mem_[page | OBJ_X_POS] | mem_[page | (OBJ_X_POS + 1)] << 8);
	if (facing_right()) {
		mem_[wWarioXPos + 1] = static_cast<uint8_t>(x_pos);
		mem_[wWarioXPos + 0] = static_cast<uint8_t>(x_pos >> 8);
		set_frameset(ADDR_Frameset_69a15);
		state_duration() = 0x0d;
		var1() = 0x02;
		mem_[wIsTurning] = TRUE;
		return;
	}
	x_pos = static_cast<uint16_t>(x_pos - 0x14);
	mem_[wWarioXPos + 1] = static_cast<uint8_t>(x_pos);
	mem_[wWarioXPos + 0] = static_cast<uint8_t>(x_pos >> 8);
	set_frameset(ADDR_Frameset_6992d);
	state_duration() = 0x3c;
	var1() = 0x03;
}

void Pesce::spit_wait(uint16_t page)
{
	mem_[page | OBJ_FLAGS] |= OBJFLAG_GRABBED;
	if (dec_at(page | OBJ_STATE_DURATION)) {
		swim();
		return;
	}
	var1() = 0;
	swim_frameset();
}

// turned round: Wario at his left
void Pesce::spit(uint16_t page)
{
	if (dec_at(page | OBJ_STATE_DURATION))
		return;
	uint16_t x_pos = static_cast<uint16_t>(mem_[page | OBJ_X_POS] | mem_[page | (OBJ_X_POS + 1)] << 8);
	x_pos = static_cast<uint16_t>(x_pos - 0x14);
	mem_[wWarioXPos + 1] = static_cast<uint8_t>(x_pos);
	mem_[wWarioXPos + 0] = static_cast<uint8_t>(x_pos >> 8);
	set_frameset(ADDR_Frameset_6992d);
	state_duration() = 0x3c;
	var1() = 0x03;
	substate() &= ~OBJSUBFLAG_HDIR;
}

// down to the floor with Wario, then lets him go
void Pesce::sink_wario(uint16_t page)
{
	if (!(mem_[wGlobalCounter] & 0x5f))
		play_sfx(SFX_WATER_SURFACE);
	Byte duration = mem_[page | OBJ_STATE_DURATION];
	if (duration) {
		--duration;
		return;
	}
	hpos(page, 0x06, 0);
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		if (mem_[wGlobalCounter] & 0x03)
			return;
		gb_->cpu.b = 0x01;
		routines_.call(ADDR_MoveWarioDown);
		move_down();
		return;
	}
	mem_[wAutoMoveState] = 0;
	play_sfx(SFX_0B9);
	var1() = 0x04;
	var2() = 0x04;
	move_up_by_var2();
	set_frameset(facing_right() ? ADDR_Frameset_69942 : ADDR_Frameset_6990a);
}

void Pesce::hpos(uint16_t page, int dy, int dx)
{
	uint16_t y_pos = static_cast<uint16_t>((mem_[page | OBJ_Y_POS] | mem_[page | (OBJ_Y_POS + 1)] << 8) + dy);
	uint16_t x_pos = static_cast<uint16_t>((mem_[page | OBJ_X_POS] | mem_[page | (OBJ_X_POS + 1)] << 8) + dx);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
}

// up towards Wario's height
void Pesce::rising()
{
	if (mem_[wCurObjFramesetOffset] == 0x02 && frame_duration() == 0x01)
		play_sfx(SFX_0AE);
	hpos(wCurObjYPos & 0xff00, -0x18, 0);
	routines_.call(ADDR_Func_358b);
	if (mem_[wIsInWaterOrSand] && screen_y() >= mem_[wWarioScreenYPos] && !(mem_[wGlobalCounter] & 0x03)) {
		++var3();
		move_up();
	}
	swim();
}

void Pesce::swim()
{
	bool right = facing_right();
	hpos(wCurObjYPos & 0xff00, -0x04, right ? 0x0f : -0x10);
	routines_.call(ADDR_Func_3513);
	if (!gb_->cpu.a) {
		routines_.call(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
		return;
	}
	if (--var2())
		return;
	var2() = 0x3c;
	var1() = 0;
	turn(wCurObjSubState & 0xff00);
}

// the floor: lunges at Wario in the water ahead, bites at the cheese ahead,
// turns at walls
void Pesce::chase()
{
	if (state_duration()) {
		if (--state_duration())
			return;
		swim_frameset();
		return;
	}
	bool right;
	if (var3()) {
		if (!(mem_[wGlobalCounter] & 1)) {
			--var3();
			move_down();
		}
		right = facing_right();
	} else {
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		uint8_t own = static_cast<uint8_t>(screen_x() + 0x2a);
		right = facing_right();
		if (mem_[wWaterInteraction] && (right ? own < wario : own >= wario)) {
			lunge();
			return;
		}
	}
	uint16_t cheese = 0;
	for (uint8_t obj = static_cast<uint8_t>(wObj1);; obj = static_cast<uint8_t>(obj + OBJ_STRUCT_LENGTH)) {
		uint16_t base = static_cast<uint16_t>((wObj1 & 0xff00) | obj);
		if (mem_[base + OBJ_ID] >= CHEESE && (mem_[base + OBJ_FLAGS] & 1)) {
			cheese = base;
			break;
		}
		if (obj == static_cast<uint8_t>(wObj8))
			break;
	}
	if (cheese) {
		uint8_t c_y = static_cast<uint8_t>(mem_[cheese + OBJ_SCREEN_Y_POS] + 0x2a);
		uint8_t c_x = static_cast<uint8_t>(mem_[cheese + OBJ_SCREEN_X_POS] + 0x2a);
		uint8_t own_y = static_cast<uint8_t>(screen_y() + 0x2a);
		if (own_y >= c_y && static_cast<uint8_t>(own_y - c_y) < 0x18) {
			uint8_t dx = right ? static_cast<uint8_t>(c_x - static_cast<uint8_t>(screen_x() + 0x4a))
				: static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x0a) - c_x);
			if (dx < 0x30) {
				bite();
				return;
			}
		}
	}
	hpos(wCurObjYPos & 0xff00, -0x04, right ? 0x0f : -0x10);
	routines_.call(ADDR_Func_3513);
	if (!gb_->cpu.a) {
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
		return;
	}
	if (--var2())
		return;
	var2() = 0x3c;
	turn(wCurObjSubState & 0xff00);
}

// the framesets by the knocks (w1d147 1 to n - 1, else 0, reset), and by
// direction
uint16_t Pesce::pick(int n, const uint16_t (*framesets)[2], bool right)
{
	uint8_t knocks = mem_[w1d147];
	int level = knocks >= 1 && knocks < n ? knocks : 0;
	if (!level)
		mem_[w1d147] = 0;
	return framesets[level][right];
}

void Pesce::turn(uint16_t page)
{
	static const uint16_t FRAMESETS[4][2] = {
		{ADDR_Frameset_69981, ADDR_Frameset_6997a},
		{ADDR_Frameset_6998f, ADDR_Frameset_69988},
		{ADDR_Frameset_6999d, ADDR_Frameset_69996},
		{ADDR_Frameset_699ab, ADDR_Frameset_699a4},
	};
	static const uint8_t DURATIONS[4] = {0x17, 0x1b, 0x1f, 0x25};
	uint8_t sub = mem_[page | OBJ_SUBSTATE] ^ OBJSUBFLAG_HDIR;
	mem_[page | OBJ_SUBSTATE] = sub;
	uint8_t knocks = mem_[w1d147];
	uint8_t duration = DURATIONS[knocks >= 1 && knocks <= 3 ? knocks : 0];
	set_frameset(pick(4, FRAMESETS, sub & 0x80));
	state_duration() = duration;
}

void Pesce::lunge()
{
	static const uint16_t FRAMESETS[3][2] = {
		{ADDR_Frameset_69911, ADDR_Frameset_69949},
		{ADDR_Frameset_69934, ADDR_Frameset_6996c},
		{ADDR_Frameset_6993b, ADDR_Frameset_69973},
	};
	var1() = 0x01;
	set_frameset(pick(3, FRAMESETS, facing_right()));
}

void Pesce::bite()
{
	static const uint16_t FRAMESETS[3][2] = {
		{ADDR_Frameset_699b2, ADDR_Frameset_699c1},
		{ADDR_Frameset_699b7, ADDR_Frameset_699c6},
		{ADDR_Frameset_699bc, ADDR_Frameset_699cb},
	};
	play_sfx(SFX_0AD);
	set_frameset(pick(3, FRAMESETS, facing_right()));
	state_duration() = 0x3c;
	var1() = 0x05;
}

void Pesce::swim_frameset()
{
	static const uint16_t FRAMESETS[4][2] = {
		{ADDR_Frameset_6990a, ADDR_Frameset_69942},
		{ADDR_Frameset_69918, ADDR_Frameset_69950},
		{ADDR_Frameset_6991f, ADDR_Frameset_69957},
		{ADDR_Frameset_69926, ADDR_Frameset_6995e},
	};
	set_frameset(pick(4, FRAMESETS, facing_right()));
}

// beaten: sinks, then swims off left
void Pesce::beaten()
{
	switch (var1()) {
	case 0:
		hpos(wCurObjYPos & 0xff00, 0x06, 0);
		routines_.call(ADDR_Func_352b);
		if (!gb_->cpu.a) {
			routines_.call(ADDR_MoveObjectDown_Slow);
			return;
		}
		if (facing_right()) {
			var1() = 0x02;
			return;
		}
		set_frameset(ADDR_Frameset_699a4);
		state_duration() = 0x25;
		var1() = 0x01;
		return;
	case 1:
		if (--state_duration())
			return;
		var1() = 0x02;
		set_frameset(ADDR_Frameset_6995e);
		return;
	case 2:
		hpos(wCurObjYPos & 0xff00, -0x30, 0);
		routines_.call(ADDR_Func_3513);
		if (!gb_->cpu.a) {
			routines_.call(ADDR_MoveObjectLeft_Slow);
			return;
		}
		set_interaction(OBJ_INTERACTION_51);
		var1() = 0x03;
		SET_HL(gb_, wCurObjUnk02);
		gb_->cpu.e = 0x07;
		routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
		mem_[wDollBoyHammerRange] = 0x01;
		return;
	case 3:
		flags() = 0;
		return;
	default:
		return;
	}
}

void PesceHelper::current()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(CURRENT_UPDATE);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 0x0f;
	mem_[wCurObjCollBoxLeft] = 0xf0;
	mem_[wCurObjCollBoxBottom] = 0xfe;
	set_frameset(ADDR_Frameset_69965);
	var2() = 0x2a;
	move_down_by_var2();
	var2() = 0x69;
	move_left_by_var2();
}

void PesceHelper::spawner_frame()
{
	no_actions();
	frame_duration() = 0x02;
}

// the end of the boss battle: e, the index of the object, for Func_ba42
void PesceHelper::spawner_done(uint8_t e)
{
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = e;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
}

void PesceHelper::dragonfly_spawner()
{
	set_update(DS(0x17));
	flags() |= OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	state_duration() = 0x3c;
}

void PesceHelper::dragonfly_wait()
{
	spawner_frame();
	if (--state_duration())
		return;
	set_update(DS(0x2f));
}

// a dragonfly whenever there's none, until Pesce's beaten
void PesceHelper::dragonfly_spawning()
{
	spawner_frame();
	if (mem_[w1d147] == 0x03) {
		set_update(DS(0x6e));
		return;
	}
	if (find_active(mem_, DRAGONFLY1))
		return;
	SET_BC(gb_, ADDR_ObjParams_Dragonfly1);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// once Pesce's swum off: the boss is beaten
void PesceHelper::dragonfly_done()
{
	spawner_frame();
	if (!mem_[wDollBoyHammerRange])
		return;
	flags() = 0;
	spawner_done(0x08);
	mem_[wBossState] = BOSS_DEFEATED;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = 0;
	mem_[wSwitchStateUpdated] = 0x01;
	mem_[wSwitchState] = 0x60;
}

void PesceHelper::dragonfly1_down()
{
	no_actions();
	if (--state_duration()) {
		routines_.call(ADDR_MoveObjectDown_Fast);
		return;
	}
	state_duration() = 0x70;
	var1() = 0;
	set_update(D1(0x24));
}

// back and forth over the pool, carrying Wario
void PesceHelper::dragonfly1_flying()
{
	one_way_platform();
	if (var1()) {
		if (--var1())
			return;
		set_frameset(facing_right() ? ADDR_Frameset_699d5 : ADDR_Frameset_699d0);
		return;
	}
	if (mem_[w1d147] == 0x03) {
		set_update(D1(0x9a));
		no_actions();
		return;
	}
	if (!--state_duration()) {
		state_duration() = 0x70;
		var1() = 0x11;
		uint8_t sub = substate() ^= OBJSUBFLAG_HDIR;
		set_frameset((sub & 0x80) ? ADDR_Frameset_699da : ADDR_Frameset_699df);
		return;
	}
	bool right = facing_right();
	routines_.call(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
	uint16_t flags_addr = static_cast<uint16_t>((gb_->cpu.h << 8) | OBJ_FLAGS);
	if (!(mem_[flags_addr] & OBJFLAG_STEPPED))
		return;
	mem_[flags_addr] &= ~OBJFLAG_STEPPED;
	gb_->cpu.b = 0x01;
	routines_.call(right ? ADDR_MoveWarioRight : ADDR_MoveWarioLeft);
}

void PesceHelper::dragonfly1_off()
{
	no_actions();
	routines_.call(facing_right() ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
}

void PesceHelper::dragonfly2()
{
	set_frameset(ADDR_Frameset_699d0);
	state_duration() = 0x28;
	set_update(D2(0x32));
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xf9;
	var2() = 0x40;
	move_left_by_var2();
	var2() = 0x50;
	move_up_by_var2();
}

void PesceHelper::dragonfly2_wait()
{
	no_actions();
	if (--state_duration())
		return;
	state_duration() = 0x50;
	var1() = 0;
	set_update(D2(0x4f));
	flags() &= ~OBJFLAG_INVISIBLE;
}

void PesceHelper::move_wario(uint16_t flags_addr, uint16_t routine, uint16_t then)
{
	if (!(mem_[flags_addr] & OBJFLAG_STEPPED))
		return;
	mem_[flags_addr] &= ~OBJFLAG_STEPPED;
	gb_->cpu.b = 0x01;
	routines_.call(routine);
	routines_.call(then);
}

// up and down (var1 1 up), carrying Wario
void PesceHelper::dragonfly2_flying()
{
	one_way_platform();
	if (!(mem_[wGlobalCounter] & 1))
		return;
	uint8_t dir = var1();
	if (dir > 1)
		return;
	bool up = dir;
	if (!--state_duration()) {
		state_duration() = 0x50;
		var1() = up ? 0 : 1;
		return;
	}
	routines_.call(up ? ADDR_MoveObjectUp : ADDR_MoveObjectDown);
	uint16_t flags_addr = static_cast<uint16_t>((gb_->cpu.h << 8) | OBJ_FLAGS);
	if (up)
		move_wario(flags_addr, ADDR_MoveWarioUp, ADDR_HandleUpwardsFloorTransition);
	else
		move_wario(flags_addr, ADDR_MoveWarioDown, ADDR_HandleDownwardsFloorTransition);
}

void PesceHelper::fly_spawner()
{
	set_update(FS(0x17));
	flags() |= OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	state_duration() = 0xc8;
}

void PesceHelper::fly_wait()
{
	spawner_frame();
	if (screen_y() >= 0xa0)
		return;
	if (--state_duration())
		return;
	set_update(FS(0x39));
	mem_[wDollBoyActiveBarrels] = 0;
}

// a fly whenever the last's gone, until Pesce's beaten
void PesceHelper::fly_spawning()
{
	spawner_frame();
	if (mem_[w1d147] == 0x03) {
		set_update(FS(0x8d));
		return;
	}
	if (mem_[wDollBoyActiveBarrels] || mem_[wTransformation])
		return;
	if (mem_[wAutoMoveState]) {
		flags() = 0;
		return;
	}
	if (find_active(mem_, FLY))
		return;
	SET_BC(gb_, ADDR_ObjParams_Fly);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void PesceHelper::fly_done()
{
	spawner_frame();
	if (!mem_[wDollBoyHammerRange])
		return;
	spawner_done(0x09);
	flags() = 0;
}

// with its cheese (wDollBoyActiveBarrels: 3 coming, 1 flying, 2 dropped)
void PesceHelper::fly()
{
	set_update(FLY_UPDATE);
	flags() |= OBJFLAG_NO_COLLISION;
	mem_[wDollBoyActiveBarrels] = 0x03;
	play_sfx(SFX_0BE);
	SET_BC(gb_, ADDR_ObjParams_Cheese);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

void PesceHelper::fly_update()
{
	switch (static_cast<uint8_t>(state())) {
	case 0x5a: // down
		no_actions();
		if (--state_duration()) {
			move_down();
			return;
		}
		state_duration() = 0x70;
		state() = 0x5b;
		mem_[wDollBoyActiveBarrels] = 0x01;
		return;
	case 0x5b: // left, bobbing
		SET_BC(gb_, ADDR_Data_604c0);
		routines_.call(ADDR_ApplyObjYMovement_Loop);
		if (mem_[wDollBoyActiveBarrels] && !(mem_[wGlobalCounter] & 1))
			return;
		move_left();
		if (--state_duration())
			return;
		state() = 0x5c;
		return;
	case 0x5c: // up and off
		no_actions();
		if (flags() & (1 << OBJFLAG_ON_SCREEN_F)) {
			move_up();
			return;
		}
		flags() = 0;
		mem_[wDollBoyActiveBarrels] = 0;
		return;
	default: { // hit: drops the cheese
		bool left = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		set_update(left ? ADDR_Func_33f8 : ADDR_Func_3407);
		var3() = 0;
		var2() = 0x01;
		if (mem_[wDollBoyActiveBarrels])
			mem_[wDollBoyActiveBarrels] = 0x02;
		return;
	}
	}
}

// under its fly, or dropped: down through the water
void PesceHelper::cheese()
{
	no_actions();
	uint8_t barrels = mem_[wDollBoyActiveBarrels];
	if (state() || !barrels) {
		if (state())
			mem_[wDollBoyActiveBarrels] = 0;
		flags() = 0;
		return;
	}
	if (barrels == 0x02) {
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (mem_[wIsInWaterOrSand]) {
			no_actions();
			if (!(flags() & (1 << OBJFLAG_ON_SCREEN_F))) {
				mem_[wDollBoyActiveBarrels] = 0;
				flags() = 0;
				return;
			}
			routines_.call(ADDR_MoveObjectDown_Slow);
			if (--state_duration())
				return;
			state_duration() = 0x18;
			gb_->cpu.b = PARTICLE_BUBBLES;
			routines_.farcall(BANK_CreateParticle, ADDR_CreateParticle);
			return;
		}
		if (flags() & (1 << OBJFLAG_ON_SCREEN_F)) {
			routines_.call(ADDR_MoveObjectDown_Fast);
			return;
		}
		mem_[wDollBoyActiveBarrels] = 0;
		flags() = 0;
		return;
	}
	if (barrels == 0x03)
		no_actions();
	uint16_t fly_obj = find_active(mem_, FLY);
	if (!fly_obj)
		return;
	uint16_t y_pos = static_cast<uint16_t>((mem_[fly_obj + OBJ_Y_POS] | mem_[fly_obj + OBJ_Y_POS + 1] << 8) + 0x0b);
	mem_[wCurObjYPos + 0] = static_cast<uint8_t>(y_pos);
	mem_[wCurObjYPos + 1] = static_cast<uint8_t>(y_pos >> 8);
	mem_[wCurObjXPos + 0] = mem_[fly_obj + OBJ_X_POS];
	mem_[wCurObjXPos + 1] = mem_[fly_obj + OBJ_X_POS + 1];
}

} // namespace wl3::level

using wl3::level::Pesce;
using wl3::level::PesceHelper;

PORT_EX(PesceFunc, 0, ALL_REGISTERS) { Pesce(gb).start(); }
PORT_LOCAL_EX(PesceFunc, Func_51e28, 0, ALL_REGISTERS) { Pesce(gb).update(); }
PORT_LOCAL_EX(PesceFunc, asm_51d76, 0, ALL_REGISTERS) { Pesce(gb).knocked(REG_HL(gb)); }
PORT_LOCAL_EX(PesceFunc, asm_51dcb, 0, ALL_REGISTERS) { Pesce(gb).swallow(REG_HL(gb)); }
PORT_LOCAL_EX(PesceFunc, asm_51e5d, 0, ALL_REGISTERS) { Pesce(gb).spit_wait(gb->cpu.h << 8); }
PORT_LOCAL_EX(PesceFunc, asm_51e6e, 0, ALL_REGISTERS) { Pesce(gb).spit(gb->cpu.h << 8); }
PORT_LOCAL_EX(PesceFunc, asm_51e97, 0, ALL_REGISTERS) { Pesce(gb).sink_wario(gb->cpu.h << 8); }
PORT_LOCAL_EX(PesceFunc, asm_51efc, 0, ALL_REGISTERS) { Pesce(gb).rising(); }
PORT_LOCAL_EX(PesceFunc, asm_51f46, 0, ALL_REGISTERS) { Pesce(gb).swim(); }
PORT_LOCAL_EX(PesceFunc, asm_51f8a, 0, ALL_REGISTERS) { Pesce(gb).chase(); }
PORT_LOCAL_EX(PesceFunc, asm_52068, 0, ALL_REGISTERS) { Pesce(gb).turn(gb->cpu.h << 8); }
PORT_LOCAL_EX(PesceFunc, asm_520c5, 0, ALL_REGISTERS) { Pesce(gb).lunge(); }
PORT_LOCAL_EX(PesceFunc, asm_52108, 0, ALL_REGISTERS) { Pesce(gb).bite(); }
PORT_LOCAL_EX(PesceFunc, asm_52155, 0, ALL_REGISTERS) { Pesce(gb).swim_frameset(); }
PORT_LOCAL_EX(PesceFunc, Func_521d0, 0, ALL_REGISTERS) { Pesce(gb).beaten(); }

PORT_EX(StrongWaterCurrentFunc, 0, ALL_REGISTERS) { PesceHelper(gb).current(); }
PORT_EX(DragonflySpawnerFunc, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly_spawner(); }
PORT_LOCAL_EX(DragonflySpawnerFunc, Func_52282, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly_wait(); }
PORT_LOCAL_EX(DragonflySpawnerFunc, Func_5229a, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly_spawning(); }
PORT_LOCAL_EX(DragonflySpawnerFunc, Func_522d9, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly_done(); }
PORT_EX(Dragonfly1Func, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly1(); }
PORT_LOCAL_EX(Dragonfly1Func, Func_52321, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly1_down(); }
PORT_LOCAL_EX(Dragonfly1Func, Func_5233b, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly1_flying(); }
PORT_LOCAL_EX(Dragonfly1Func, Func_523b1, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly1_off(); }
PORT_EX(Dragonfly2Func, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly2(); }
PORT_LOCAL_EX(Dragonfly2Func, Func_523f2, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly2_wait(); }
PORT_LOCAL_EX(Dragonfly2Func, Func_5240f, 0, ALL_REGISTERS) { PesceHelper(gb).dragonfly2_flying(); }
PORT_EX(FlySpawnerFunc, 0, ALL_REGISTERS) { PesceHelper(gb).fly_spawner(); }
PORT_LOCAL_EX(FlySpawnerFunc, Func_52475, 0, ALL_REGISTERS) { PesceHelper(gb).fly_wait(); }
PORT_LOCAL_EX(FlySpawnerFunc, Func_52497, 0, ALL_REGISTERS) { PesceHelper(gb).fly_spawning(); }
PORT_LOCAL_EX(FlySpawnerFunc, Func_524eb, 0, ALL_REGISTERS) { PesceHelper(gb).fly_done(); }
PORT_EX(FlyFunc, 0, ALL_REGISTERS) { PesceHelper(gb).fly(); }
PORT_LOCAL_EX(FlyFunc, Func_52533, 0, ALL_REGISTERS) { PesceHelper(gb).fly_update(); }
PORT_EX(CheeseFunc, 0, ALL_REGISTERS) { PesceHelper(gb).cheese(); }
