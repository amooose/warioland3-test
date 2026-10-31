// Doll Boy (doll_boy.asm), the boss on a stack of three barrels throwing
// hammers of growing range: barrels flash in turn, and attacking a
// flashing one drops him; on the floor he walks the platforms, and can be
// stunned and knocked over. And the barrels, his hammers and the hammer
// platforms (with their spawner). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t DB(uint16_t offset) { return static_cast<uint16_t>(ADDR_DollBoyFunc + offset); }

constexpr uint16_t INIT = DB(0x34); // .Init
constexpr uint16_t BARREL_PHASE = DB(0x65); // .BarrelPhase
constexpr uint16_t INIT_WALKING_PHASE = DB(0x1ab); // .InitWalkingPhase
constexpr uint16_t WALKING_PHASE = DB(0x2c4); // .WalkingPhase
constexpr uint16_t HIT_WARIO = DB(0x485); // .HitWario
constexpr uint16_t WAIT_THEN_DO_NOTHING = DB(0x4ca); // .WaitThenDoNothing
constexpr uint16_t DO_NOTHING = DB(0x4e9); // .DoNothing
constexpr uint16_t DEFEATED = DB(0x4ef); // .Defeated

constexpr uint16_t BARREL_START_DELAY = ADDR_DollBoyBarrelFunc + 0x2c; // .StartDelay
constexpr uint16_t BARREL_MAIN = ADDR_DollBoyBarrelFunc + 0x40; // .Main
constexpr uint16_t HAMMER_INIT = ADDR_DollBoyHammerFunc + 0x0e; // .Init
constexpr uint16_t HAMMER_UPDATE = ADDR_DollBoyHammerFunc + 0x2a; // .Update
constexpr uint16_t HAMMER_HIT_LEFT = ADDR_DollBoyHammerFunc + 0x92; // .HitLeft
constexpr uint16_t HAMMER_HIT_RIGHT = ADDR_DollBoyHammerFunc + 0x97; // .HitRight
constexpr uint16_t SPAWNER_SPAWN = ADDR_HammerPlatformSpawnerFunc + 0x16; // .SpawnHammerPlatform
constexpr uint16_t SPAWNER_DO_NOTHING = ADDR_HammerPlatformSpawnerFunc + 0x2a; // .DoNothing
constexpr uint16_t PLATFORM_MOVE_DOWN = ADDR_HammerPlatformFunc + 0x13; // .MoveDown
constexpr uint16_t PLATFORM_MOVE_UP = ADDR_HammerPlatformFunc + 0x48; // .MoveUp

// wDollBoyActiveBarrels flags
constexpr uint8_t DOLLBOY_BARREL1 = 1 << 0; // top barrel
constexpr uint8_t DOLLBOY_BARREL2 = 1 << 1; // middle barrel
constexpr uint8_t DOLLBOY_BARREL3 = 1 << 2; // bottom barrel
constexpr uint8_t DOLLBOY_DOLL = 1 << 3;

// wDollBoyHammerRange
constexpr uint8_t HAMMER_SHORT = 1;
constexpr uint8_t HAMMER_MID_SHORT = 2;
constexpr uint8_t HAMMER_MID_LONG = 3;
constexpr uint8_t HAMMER_LONG = 4;

constexpr uint8_t BARREL_FLASH_DELAY = 70; // before a barrel starts flashing
constexpr uint8_t BARREL_FLASH_DURATION = 240;
constexpr uint8_t HAMMER_PLATFORM_DURATION = 140; // going up or down
constexpr uint8_t DOLLBOY_THROW_HAMMER_ODDS = 250; // out of 256
constexpr uint8_t DOLLBOY_START_DELAY = 40;
constexpr uint8_t DOLLBOY_THROW_HAMMER_DELAY = 56;
constexpr uint8_t DOLLBOY_WALKING_PHASE_DELAY = 80;
constexpr uint8_t DOLLBOY_JUMP_DELAY = 20;
constexpr uint8_t DOLLBOY_TURN_DELAY = 32;
constexpr uint8_t DOLLBOY_AFTER_HIT_DELAY = 60; // after hitting Wario with the hammer
constexpr uint8_t DOLLBOY_STUN_DURATION = 120;
constexpr uint8_t DOLLBOY_DEFEAT_DURATION = 43; // falling off screen

class DollBoy : public Object {
public:
	using Object::Object;

	void start(); // DollBoyFunc
	void init(); // .Init
	void barrel_phase(); // .BarrelPhase
	void init_walking_phase(); // .InitWalkingPhase
	void walking_phase(); // .WalkingPhase
	void hit_wario(); // .HitWario
	void wait_then_do_nothing(); // .WaitThenDoNothing
	void do_nothing() { no_actions(); } // .DoNothing
	void defeated(); // .Defeated
	void update_remaining_parts(); // UpdateDollBoyRemainingParts
	void ground_shake(); // DollBoyGroundShake

private:
	void copy_pals(uint16_t pals);
	void after_fall();
	void fall(uint8_t frame, uint8_t shake, uint8_t end, uint16_t movement);
	void fall_wait_1_barrel() { fall(18, 26, 30, ADDR_Data_60b80); }
	void fall_immediately() { fall(2, 10, 14, ADDR_Data_60880); }
	void set_stunned();
	void update_stunned();
	void jump_platform(uint16_t movement);
	void show_stars(uint16_t frameset, uint16_t params);
	void walk(); // .handle_walk_movement
	void switch_direction();
};

class DollBoyHelper : public Object {
public:
	using Object::Object;

	void barrel(uint8_t part); // DollBoyBarrelFunc
	void barrel_start_delay(); // .StartDelay
	void barrel_main(); // .Main
	void hammer(); // DollBoyHammerFunc
	void hammer_init(); // .Init
	void hammer_update(); // .Update
	void hammer_hit(uint16_t move); // .HitLeft / .HitRight
	void platform(); // HammerPlatformFunc
	void platform_down(); // .MoveDown
	void platform_up(); // .MoveUp
	void spawner(); // HammerPlatformSpawnerFunc
	void spawner_spawn(); // .SpawnHammerPlatform
	void spawner_do_nothing(); // .DoNothing

private:
	void barrel_fall(uint8_t shake, uint8_t end, uint16_t movement);
	void end_flashing();
};

} // namespace

void DollBoy::start()
{
	mem_[wIsBossBattle] = TRUE;
	flags() &= ~OBJFLAG_INVISIBLE;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() = (substate() & OBJSUBFLAGS_MASK) | 0x5 | OBJSUBFLAG_HDIR;
	set_update(INIT);
	mem_[wCurObjCollBoxRight] = 11;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-12);
	mem_[wCurObjCollBoxBottom] = 0;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-28);
	set_frameset(ADDR_Frameset_69632);
	state_duration() = DOLLBOY_START_DELAY;
}

// on screen a while: the battle starts
void DollBoy::init()
{
	if (!(flags() & (1 << OBJFLAG_ON_SCREEN_F)))
		return;
	if (--state_duration())
		return;
	set_update(BARREL_PHASE);
	mem_[wDollBoyNoFlashingBarrel] = TRUE;
	var1() = 0;
	mem_[wDollBoyFallImmediatelyMiddleBarrel] = 0;
	mem_[wDollBoyHammerRange] = 0;
	mem_[wDollBoyActiveBarrels] = DOLLBOY_BARREL1 | DOLLBOY_BARREL2 | DOLLBOY_BARREL3;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
}

void DollBoy::copy_pals(uint16_t pals)
{
	SET_HL(gb_, pals);
	SET_DE(gb_, wTempPals2 + 4 * PAL_SIZE);
	gb_->cpu.c = 4;
	gb_->cpu.b = 2;
	routines_.call(ADDR_CopyAndApplyOBPals);
}

void DollBoy::after_fall()
{
	set_frameset(ADDR_Frameset_69632);
	var1() = 0;
}

// down onto the next barrel (var3 the movement's index): the frameset at
// frame, the ground shaking at shake, landed at end
void DollBoy::fall(uint8_t frame, uint8_t shake, uint8_t end, uint16_t movement)
{
	uint8_t at = var3();
	if (at == frame) {
		set_frameset(ADDR_Frameset_69646);
	} else if (at == shake) {
		ground_shake();
	} else if (at == end) {
		after_fall();
		return;
	}
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjYMovement);
}

// throws hammers; falls when a barrel under him is attacked (var1 8)
void DollBoy::barrel_phase()
{
	switch (var1()) {
	case 0x00:
		if (mem_[wTransformation] == TRANSFORMATION_BOUNCY_WARIO) {
			set_update(DO_NOTHING);
			mem_[wBossState] = 0;
			return;
		}
		if (substate() == (0x2 | OBJSUBFLAG_HDIR)) { // on the floor
			set_update(INIT_WALKING_PHASE);
			set_interaction(OBJ_INTERACTION_0B);
			state_duration() = DOLLBOY_WALKING_PHASE_DELAY;
			return;
		}
		update_remaining_parts();
		if (var1() == 0x08)
			return;
		if (mem_[rDIV] < DOLLBOY_THROW_HAMMER_ODDS)
			return;
		set_frameset(ADDR_Frameset_6963b);
		state_duration() = DOLLBOY_THROW_HAMMER_DELAY;
		var1() = 0x01;
		return;
	case 0x01: { // the hammer out partway, each longer than the last
		update_remaining_parts();
		if (var1() == 0x08)
			return;
		if (!--state_duration()) {
			after_fall();
			return;
		}
		if (state_duration() != 0x13)
			return;
		Byte range = mem_[wDollBoyHammerRange];
		if (range == HAMMER_LONG)
			range = 0;
		++range;
		play_sfx(SFX_0A2);
		SET_BC(gb_, ADDR_ObjParams_DollBoyHammer);
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	}
	case 0x08:
		set_frameset(ADDR_Frameset_69632);
		var1() = 0x09;
		var3() = 0;
		break;
	case 0x09:
		break;
	default:
		return;
	}
	// waits on the barrels still under the one attacked
	uint8_t height = substate() & 0x0f;
	uint8_t barrels = mem_[wDollBoyActiveBarrels];
	if (height == 0x4) {
		if (barrels == (DOLLBOY_BARREL1 | DOLLBOY_BARREL2)) { // the bottom barrel
			mem_[wDollBoyFallImmediatelyMiddleBarrel] = TRUE;
			fall(34, 42, 46, ADDR_Data_60ba0);
		} else if (barrels == (DOLLBOY_BARREL1 | DOLLBOY_BARREL3)) { // the middle one
			fall_wait_1_barrel();
		} else {
			fall_immediately();
		}
		return;
	}
	if (height != 0x3) {
		fall_immediately();
		return;
	}
	if (barrels == DOLLBOY_BARREL1)
		fall_wait_1_barrel();
	else if (barrels == DOLLBOY_BARREL3 || mem_[wDollBoyFallImmediatelyMiddleBarrel])
		fall_immediately();
	else
		fall_wait_1_barrel();
}

void DollBoy::init_walking_phase()
{
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_6964d);
	set_update(WALKING_PHASE);
	state() = 0;
	var1() = 0;
	copy_pals(ADDR_DollBoyGreenPals);
}

void DollBoy::set_stunned()
{
	play_sfx(SFX_0A4);
	state() = 0x5c;
	bool right = facing_right();
	var3() = 0;
	set_frameset(right ? ADDR_Frameset_69675 : ADDR_Frameset_6967a);
	state_duration() = DOLLBOY_STUN_DURATION;
	var1() = 0xff;
	copy_pals(ADDR_DollBoyRedPals);
}

// red, until it's over (green again near the end); then faces Wario
void DollBoy::update_stunned()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		SET_BC(gb_, ADDR_FallingYVel_Light);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	if (--state_duration()) {
		if (state_duration() == 10)
			copy_pals(ADDR_DollBoyGreenPals);
		return;
	}
	state() = 0;
	bool right = facing_right();
	bool left_of_wario = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	uint16_t frameset;
	uint8_t duration;
	if (!right) {
		frameset = left_of_wario ? ADDR_Frameset_6966a : ADDR_Frameset_69656;
		duration = left_of_wario ? DOLLBOY_TURN_DELAY : 0;
	} else {
		frameset = left_of_wario ? ADDR_Frameset_6964d : ADDR_Frameset_6965f;
		duration = left_of_wario ? 0 : DOLLBOY_TURN_DELAY;
	}
	set_frameset(frameset);
	state_duration() = duration;
	var1() = 0;
	var3() = 0;
}

// to the platform below or above
void DollBoy::jump_platform(uint16_t movement)
{
	SET_BC(gb_, movement);
	if (state_duration()) {
		--state_duration();
		return;
	}
	routines_.call(ADDR_ApplyObjYMovement);
	if (var3() == 0x0a)
		ground_shake();
	if (--var1())
		return;
	var1() = 0x3c;
	state() = 0;
}

// his hammer hit Wario
void DollBoy::show_stars(uint16_t frameset, uint16_t params)
{
	set_update(HIT_WARIO);
	var3() = 0;
	mem_[wBossState] = 0;
	stop_sfx(mem_);
	set_frameset(frameset);
	state_duration() = 1;
	SET_BC(gb_, params);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// a step, bouncy as the foot comes down; turns at walls
void DollBoy::walk()
{
	if (mem_[wCurObjFramesetOffset] == 0x06 && frame_duration() == 0x01)
		play_sfx(SFX_0A2);
	bool right = facing_right();
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = static_cast<uint16_t>(right ? x() + 11 : x() - 12);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		set_frameset(right ? ADDR_Frameset_6965f : ADDR_Frameset_6966a);
		state_duration() = DOLLBOY_TURN_DELAY;
		var3() = 0;
		return;
	}
	if (mem_[wCurObjFramesetOffset] < 0x06)
		return;
	set_interaction(OBJ_INTERACTION_BOUNCY);
	routines_.call(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
}

// turning (a hop, the ground shaking), or walking; jumps to Wario's
// platform from the middle of the room after a while
void DollBoy::switch_direction()
{
	if (state_duration()) {
		if (!--state_duration()) {
			set_frameset(facing_right() ? ADDR_Frameset_6964d : ADDR_Frameset_69656);
			return;
		}
		if (var3() == 0x17) {
			substate() ^= OBJSUBFLAG_HDIR;
			ground_shake();
		}
		SET_BC(gb_, ADDR_Data_60c00);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	if (var1()) {
		--var1();
		walk();
		return;
	}
	uint8_t x_screen = screen_x();
	uint8_t wario_y = mem_[wWarioScreenYPos];
	uint8_t own_y = screen_y();
	if (x_screen >= 0x78 || x_screen < 0x18 || wario_y < 0x28 || own_y >= 0xa0) {
		walk();
		return;
	}
	uint8_t dy = static_cast<uint8_t>(own_y - wario_y);
	uint8_t s;
	if (own_y < wario_y) {
		if (dy >= static_cast<uint8_t>(-0x1f)) {
			walk();
			return;
		}
		s = 0x5a; // to the platform below
	} else {
		if (dy < 0x20) {
			walk();
			return;
		}
		s = 0x5b; // to the platform above
	}
	state() = s;
	var3() = 0;
	var1() = 0x1e;
	state_duration() = DOLLBOY_JUMP_DELAY;
}

// on the floor: walks, jumps between platforms; bumped, hit, stunned and
// knocked over
void DollBoy::walking_phase()
{
	set_interaction(OBJ_INTERACTION_01);
	uint8_t s = state();
	switch (s) {
	case 0x00:
		switch_direction();
		return;
	case 0x5a:
		jump_platform(ADDR_Data_60890);
		return;
	case 0x5b:
		jump_platform(ADDR_Data_608a0);
		return;
	case 0x5c:
		update_stunned();
		return;
	case 0x02:
	case 0x03:
		state() = s == 0x02 ? 0x32 : 0x33;
		state_duration() = 12;
		var2() = 0x02;
		return;
	case 0x32:
		routines_.call(ADDR_ObjState_BumpLeft);
		return;
	case 0x33:
		routines_.call(ADDR_ObjState_BumpRight);
		return;
	case 0x0b:
		show_stars(ADDR_Frameset_696b9, ADDR_ObjParams_DollBoyHammerStarsLeft);
		return;
	case 0x0c:
		show_stars(ADDR_Frameset_696b6, ADDR_ObjParams_DollBoyHammerStarsRight);
		return;
	default:
		break;
	}
	s &= 0xfe;
	if (s == 0x08) {
		set_stunned();
		return;
	}
	bool stunned = var1() == 0xff;
	if (s != 0x04) {
		if (stunned) {
			set_stunned();
			return;
		}
		state_duration() = 1;
		update_stunned();
		return;
	}
	if (!stunned) {
		set_stunned();
		return;
	}
	// attacked while stunned: beaten
	state() = 0x48;
	substate() &= OBJSUBFLAGS_MASK;
	var3() = 0;
	state_duration() = DOLLBOY_DEFEAT_DURATION;
	set_update(DEFEATED);
	flags() |= OBJFLAG_PRIORITY;
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x05;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
	stop_music2(mem_);
	set_frameset(ADDR_Frameset_6967f);
}

void DollBoy::hit_wario()
{
	no_actions();
	if (state_duration()) {
		--state_duration();
		play_sfx(SFX_049);
	}
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		SET_BC(gb_, ADDR_FallingYVel_Light);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	set_update(WAIT_THEN_DO_NOTHING);
	state_duration() = DOLLBOY_AFTER_HIT_DELAY;
}

void DollBoy::wait_then_do_nothing()
{
	if (--state_duration())
		return;
	set_update(DO_NOTHING);
	set_frameset(facing_right() ? ADDR_Frameset_69695 : ADDR_Frameset_6969a);
}

// off the bottom of the screen: the boss is beaten
void DollBoy::defeated()
{
	no_actions();
	if (screen_y() >= 0xd0) {
		flags() = 0;
		mem_[wBossState] = BOSS_DEFEATED;
		routines_.call(ADDR_UpdateLevelMusic);
		mem_[wIsBossBattle] = 0;
		ground_shake();
		return;
	}
	if (state_duration()) {
		if (state_duration() == 27) {
			stop_sfx(mem_);
			copy_pals(ADDR_DollBoyRedPals);
		}
		if (!--state_duration())
			play_sfx(SFX_0A3);
	}
	SET_BC(gb_, ADDR_Data_60bd0);
	routines_.call(ADDR_ApplyObjYMovement);
}

// a barrel attacked from under him: its bit out of the parts left (the id's
// low nibble), and down a barrel
void DollBoy::update_remaining_parts()
{
	if (substate() == 1)
		return;
	uint8_t part = mem_[wCurObjId] & 0x0f;
	uint8_t parts = mem_[wDollBoyRemainingParts];
	if (!(parts & part))
		return;
	mem_[wDollBoyRemainingParts] = static_cast<uint8_t>(~part & parts);
	var1() = 0x08;
	--substate();
}

void DollBoy::ground_shake()
{
	play_sfx(SFX_061);
	gb_->cpu.b = 24;
	routines_.call(ADDR_DoGroundShake);
}

void DollBoyHelper::barrel(uint8_t part)
{
	substate() = part;
	flags() &= ~OBJFLAG_INVISIBLE;
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	set_update(BARREL_START_DELAY);
	set_frameset(ADDR_Frameset_6969f);
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = 0;
	var1() = 0;
}

// until a barrel may flash
void DollBoyHelper::barrel_start_delay()
{
	frame_duration() = 2;
	if (!mem_[wDollBoyNoFlashingBarrel])
		return;
	set_update(BARREL_MAIN);
}

void DollBoyHelper::barrel_fall(uint8_t shake, uint8_t end, uint16_t movement)
{
	uint8_t at = var3();
	if (at == shake) {
		DollBoy(gb_).ground_shake();
	} else if (at == end) {
		var1() = 0;
		return;
	}
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjYMovement);
}

void DollBoyHelper::end_flashing()
{
	set_frameset(ADDR_Frameset_6969f);
	set_update(BARREL_MAIN);
	mem_[wDollBoyNoFlashingBarrel] = TRUE;
}

// one barrel at a time flashes (when it's the last, or not level with
// Wario); attacked then, it's knocked out from under Doll Boy. Falls when
// one under it goes (var1 8)
void DollBoyHelper::barrel_main()
{
	switch (var1()) {
	case 0x00: {
		DollBoy(gb_).update_remaining_parts();
		if (var1() == 0x08)
			return;
		if ((mem_[wCurObjId] & 0x03) != (mem_[wGlobalCounter] & 0x03))
			return;
		if (!mem_[wDollBoyNoFlashingBarrel])
			return;
		uint8_t barrels = mem_[wDollBoyActiveBarrels];
		if (barrels != DOLLBOY_BARREL1 && barrels != DOLLBOY_BARREL2 && barrels != DOLLBOY_BARREL3) {
			uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
			if (dy < 8 || dy >= static_cast<uint8_t>(-8))
				return;
		}
		mem_[wDollBoyNoFlashingBarrel] = 0;
		state_duration() = static_cast<uint8_t>(BARREL_FLASH_DELAY + (mem_[rDIV] & 0x03));
		var1() = 0x01;
		return;
	}
	case 0x01:
		if (--state_duration())
			return;
		set_frameset(ADDR_Frameset_696a8);
		state_duration() = BARREL_FLASH_DURATION;
		var1() = 0x02;
		state() = 0;
		return;
	case 0x02: {
		if (mem_[wTransformation] == TRANSFORMATION_BOUNCY_WARIO) {
			state_duration() = 0;
			var1() = 0;
			end_flashing();
			return;
		}
		if ((flags() & (1 << OBJFLAG_ON_SCREEN_F)) && !(mem_[wGlobalCounter] & 0x07))
			play_sfx(SFX_06A);
		if (state() == 0x05) { // attacked
			substate() &= OBJSUBFLAGS_MASK;
			var3() = 0;
			var2() = 0x01;
			set_frameset(ADDR_Frameset_6969f);
			set_update(ADDR_Func_3317);
			play_sfx(SFX_017);
			uint8_t id = mem_[wCurObjId];
			uint8_t knocked, remaining;
			if (id == DOLL_BOY_BARREL1) {
				knocked = DOLLBOY_BARREL1;
				remaining = DOLLBOY_DOLL;
			} else if (id == 0x42) {
				knocked = DOLLBOY_BARREL2;
				remaining = DOLLBOY_BARREL1 | DOLLBOY_DOLL;
			} else {
				knocked = DOLLBOY_BARREL3;
				remaining = DOLLBOY_BARREL1 | DOLLBOY_BARREL2 | DOLLBOY_DOLL;
			}
			mem_[wDollBoyActiveBarrels] &= static_cast<uint8_t>(0xff ^ knocked);
			mem_[wDollBoyRemainingParts] = remaining;
			mem_[wDollBoyNoFlashingBarrel] = TRUE;
			return;
		}
		if (--state_duration())
			return;
		var1() = 0;
		end_flashing();
		return;
	}
	case 0x08:
		var1() = 0x09;
		var3() = 0;
		break;
	case 0x09:
		break;
	default:
		return;
	}
	if (mem_[wCurObjId] == DOLL_BOY_BARREL1 && mem_[wDollBoyActiveBarrels] == (DOLLBOY_BARREL1 | DOLLBOY_BARREL2))
		barrel_fall(0x1a, 0x1e, ADDR_Data_60b80);
	else
		barrel_fall(0x0a, 0x0e, ADDR_Data_60880);
}

void DollBoyHelper::hammer()
{
	set_update(HAMMER_INIT);
	flags() |= OBJFLAG_NO_COLLISION;
}

// thrown from below Wario, it flies straight right
void DollBoyHelper::hammer_init()
{
	set_update(HAMMER_UPDATE);
	uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
	if (dy < 0xec)
		return;
	++var1();
}

// an arc by the hammer's range; hit, it falls away
void DollBoyHelper::hammer_update()
{
	if (state()) {
		bool left = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		set_update(left ? HAMMER_HIT_LEFT : HAMMER_HIT_RIGHT);
		var3() = 0;
		play_sfx(SFX_049);
		SET_BC(gb_, ADDR_ObjParams_StarsCentre);
		routines_.call(ADDR_CreateObjectFromCurObjPos);
		return;
	}
	if (var1()) {
		routines_.call(ADDR_MoveObjectRight_Fast);
		return;
	}
	SET_BC(gb_, ADDR_Data_60860);
	routines_.call(ADDR_ApplyObjYMovement);
	uint16_t movement;
	switch (static_cast<uint8_t>(mem_[wDollBoyHammerRange])) {
	case HAMMER_SHORT: movement = ADDR_Data_60ae0; break;
	case HAMMER_MID_SHORT: movement = ADDR_Data_60b00; break;
	case HAMMER_MID_LONG: movement = ADDR_Data_60b30; break;
	default: movement = ADDR_Data_60b60; break;
	}
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjXMovement);
}

void DollBoyHelper::hammer_hit(uint16_t move)
{
	routines_.call(move);
	no_actions();
	SET_BC(gb_, ADDR_Data_60040);
	routines_.call(ADDR_ApplyObjYMovement);
}

void DollBoyHelper::platform()
{
	set_update(PLATFORM_MOVE_DOWN);
	var2() = mem_[wCurObjYPos + 0];
	var3() = mem_[wCurObjYPos + 1];
	platform_down();
}

// down (carrying Wario) until hit or its time's up
void DollBoyHelper::platform_down()
{
	one_way_platform();
	if (state() || !--state_duration()) {
		set_update(PLATFORM_MOVE_UP);
		state() = 0;
		return;
	}
	if (!(flags() & OBJFLAG_STEPPED)) {
		routines_.call(ADDR_MoveObjectDown_Slow);
		return;
	}
	flags() &= ~OBJFLAG_STEPPED;
	if (!(mem_[wGlobalCounter] & 1))
		return;
	gb_->cpu.b = 1;
	routines_.call(ADDR_MoveWarioDown);
	routines_.call(ADDR_HandleDownwardsFloorTransition);
	move_down();
}

// back up to where it started (var2, var3)
void DollBoyHelper::platform_up()
{
	one_way_platform();
	if (mem_[wCurObjYPos + 0] == var2() && mem_[wCurObjYPos + 1] == var3()) {
		set_update(PLATFORM_MOVE_DOWN);
		state() = 0;
		state_duration() = HAMMER_PLATFORM_DURATION;
		return;
	}
	if (!(flags() & OBJFLAG_STEPPED)) {
		routines_.call(ADDR_MoveObjectUp_Slow);
		return;
	}
	flags() &= ~OBJFLAG_STEPPED;
	if (!(mem_[wGlobalCounter] & 1))
		return;
	gb_->cpu.b = 1;
	routines_.call(ADDR_MoveWarioUp);
	routines_.call(ADDR_HandleUpwardsFloorTransition);
	move_up();
}

void DollBoyHelper::spawner()
{
	set_update(SPAWNER_SPAWN);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	state_duration() = 150;
	spawner_do_nothing();
}

// a hammer platform after 150 frames
void DollBoyHelper::spawner_spawn()
{
	if (!--state_duration()) {
		set_update(SPAWNER_DO_NOTHING);
		SET_BC(gb_, ADDR_ObjParams_HammerPlatform);
		routines_.call(ADDR_CreateObjectAtRelativePos);
	}
	spawner_do_nothing();
}

void DollBoyHelper::spawner_do_nothing()
{
	frame_duration() = 2;
	no_actions();
}

} // namespace wl3::level

using wl3::level::DollBoy;
using wl3::level::DollBoyHelper;

PORT_EX(DollBoyFunc, 0, ALL_REGISTERS) { DollBoy(gb).start(); }
PORT_LOCAL_EX(DollBoyFunc, Init, 0, ALL_REGISTERS) { DollBoy(gb).init(); }
PORT_LOCAL_EX(DollBoyFunc, BarrelPhase, 0, ALL_REGISTERS) { DollBoy(gb).barrel_phase(); }
PORT_LOCAL_EX(DollBoyFunc, InitWalkingPhase, 0, ALL_REGISTERS) { DollBoy(gb).init_walking_phase(); }
PORT_LOCAL_EX(DollBoyFunc, WalkingPhase, 0, ALL_REGISTERS) { DollBoy(gb).walking_phase(); }
PORT_LOCAL_EX(DollBoyFunc, HitWario, 0, ALL_REGISTERS) { DollBoy(gb).hit_wario(); }
PORT_LOCAL_EX(DollBoyFunc, WaitThenDoNothing, 0, ALL_REGISTERS) { DollBoy(gb).wait_then_do_nothing(); }
PORT_LOCAL_EX(DollBoyFunc, DoNothing, 0, ALL_REGISTERS) { DollBoy(gb).do_nothing(); }
PORT_LOCAL_EX(DollBoyFunc, Defeated, 0, ALL_REGISTERS) { DollBoy(gb).defeated(); }
PORT_EX(UpdateDollBoyRemainingParts, 0, ALL_REGISTERS) { DollBoy(gb).update_remaining_parts(); }
PORT_EX(DollBoyGroundShake, 0, ALL_REGISTERS) { DollBoy(gb).ground_shake(); }

PORT_EX(DollBoyBarrelFunc, 0, ALL_REGISTERS) { DollBoyHelper(gb).barrel(gb->cpu.a); }
PORT_EX(DollBoyBarrel1Func, 0, ALL_REGISTERS) { DollBoyHelper(gb).barrel(0x03); }
PORT_EX(DollBoyBarrel2Func, 0, ALL_REGISTERS) { DollBoyHelper(gb).barrel(0x02); }
PORT_EX(DollBoyBarrel3Func, 0, ALL_REGISTERS) { DollBoyHelper(gb).barrel(0x01); }
PORT_LOCAL_EX(DollBoyBarrelFunc, StartDelay, 0, ALL_REGISTERS) { DollBoyHelper(gb).barrel_start_delay(); }
PORT_LOCAL_EX(DollBoyBarrelFunc, Main, 0, ALL_REGISTERS) { DollBoyHelper(gb).barrel_main(); }
PORT_EX(DollBoyHammerFunc, 0, ALL_REGISTERS) { DollBoyHelper(gb).hammer(); }
PORT_LOCAL_EX(DollBoyHammerFunc, Init, 0, ALL_REGISTERS) { DollBoyHelper(gb).hammer_init(); }
PORT_LOCAL_EX(DollBoyHammerFunc, Update, 0, ALL_REGISTERS) { DollBoyHelper(gb).hammer_update(); }
PORT_LOCAL_EX(DollBoyHammerFunc, HitLeft, 0, ALL_REGISTERS) { DollBoyHelper(gb).hammer_hit(ADDR_MoveObjectLeft); }
PORT_LOCAL_EX(DollBoyHammerFunc, HitRight, 0, ALL_REGISTERS) { DollBoyHelper(gb).hammer_hit(ADDR_MoveObjectRight); }
PORT_EX(HammerPlatformFunc, 0, ALL_REGISTERS) { DollBoyHelper(gb).platform(); }
PORT_LOCAL_EX(HammerPlatformFunc, MoveDown, 0, ALL_REGISTERS) { DollBoyHelper(gb).platform_down(); }
PORT_LOCAL_EX(HammerPlatformFunc, MoveUp, 0, ALL_REGISTERS) { DollBoyHelper(gb).platform_up(); }
PORT_EX(HammerPlatformSpawnerFunc, 0, ALL_REGISTERS) { DollBoyHelper(gb).spawner(); }
PORT_LOCAL_EX(HammerPlatformSpawnerFunc, SpawnHammerPlatform, 0, ALL_REGISTERS) { DollBoyHelper(gb).spawner_spawn(); }
PORT_LOCAL_EX(HammerPlatformSpawnerFunc, DoNothing, 0, ALL_REGISTERS) { DollBoyHelper(gb).spawner_do_nothing(); }
