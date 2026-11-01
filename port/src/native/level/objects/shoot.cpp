// Shoot (shoot.asm): the football boss. Shoot jumps about the pitch;
// stomped on he curls into a ball Wario can kick into the net, and as
// Ball o' String Wario he kicks Wario into Wario's own net, where GK
// Tortoise the goalkeeper ducks or catches him. Three goals each way; the
// goal counters show them. And GK Tortoise's platform, and the routines
// that move Wario along with a kick. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SH(uint16_t offset) { return static_cast<uint16_t>(ADDR_ShootFunc + offset); }
constexpr uint16_t GK(uint16_t offset) { return static_cast<uint16_t>(ADDR_GKTortoiseFunc + offset); }

constexpr uint16_t START_WAIT = SH(0x55); // .StartWait
constexpr uint16_t TAUNT = SH(0x64); // .Taunt
constexpr uint16_t MAIN = SH(0x106); // .Main
constexpr uint16_t STANDING = SH(0x163); // .Standing
constexpr uint16_t JUMPING_UP = SH(0x18b); // .JumpingUp
constexpr uint16_t JUMPING_DOWN = SH(0x1a2); // .JumpingDown
constexpr uint16_t BETWEEN_JUMPS = SH(0x292); // .BetweenJumps
constexpr uint16_t TURNING = SH(0x29a); // .Turning
constexpr uint16_t PREKICK_JUMP = SH(0x2ac); // .PreKickJump
constexpr uint16_t PREKICK_ANIMATION = SH(0x2e0); // .PreKickAnimation
constexpr uint16_t KICKING = SH(0x309); // .Kicking
constexpr uint16_t SCORE_GOAL = SH(0x340); // .ScoreGoal
constexpr uint16_t WON = SH(0x375); // .Won
constexpr uint16_t BALL_FORM = SH(0x40b); // .BallForm
constexpr uint16_t B_56AF0 = SH(0x4df); // .Func_56af0
constexpr uint16_t B_56B20 = SH(0x50f); // .Func_56b20
constexpr uint16_t B_56B2A = SH(0x519); // .Func_56b2a
constexpr uint16_t B_56B47 = SH(0x536); // .Func_56b47
constexpr uint16_t B_56B96 = SH(0x585); // .Func_56b96
constexpr uint16_t B_56BAE = SH(0x59d); // .Func_56bae
constexpr uint16_t B_56BB8 = SH(0x5a7); // .Func_56bb8
constexpr uint16_t B_56BD1 = SH(0x5c0); // .Func_56bd1
constexpr uint16_t B_56BDB = SH(0x5ca); // .Func_56bdb
constexpr uint16_t B_56BE3 = SH(0x5d2); // .Func_56be3
constexpr uint16_t B_56BF1 = SH(0x5e0); // .Func_56bf1
constexpr uint16_t B_56BF8 = SH(0x5e7); // .Func_56bf8
constexpr uint16_t B_56BFF = SH(0x5ee); // .Func_56bff
constexpr uint16_t B_56C0D = SH(0x5fc); // .Func_56c0d
constexpr uint16_t B_56C1B = SH(0x60a); // .Func_56c1b
constexpr uint16_t B_56C3D = SH(0x62c); // .Func_56c3d
constexpr uint16_t B_56C49 = SH(0x638); // .Func_56c49
constexpr uint16_t B_56D1C = SH(0x70b); // .Func_56d1c
constexpr uint16_t DEFEAT_RISE = SH(0x777); // .DefeatRise
constexpr uint16_t DEFEAT_SUSPEND = SH(0x79e); // .DefeatSuspendAndFall

constexpr uint16_t GK_MAIN = GK(0x42); // .Main
constexpr uint16_t GK_DEFEATED = GK(0x73); // .Func_56edd
constexpr uint16_t GK_DUCKING = GK(0xa2); // .Ducking
constexpr uint16_t GK_56F27 = GK(0xbd); // .Func_56f27
constexpr uint16_t GK_56F2E = GK(0xc4); // .Func_56f2e
constexpr uint16_t GK_56FEF = GK(0x185); // .Func_56fef
constexpr uint16_t GK_57021 = GK(0x1b7); // .Func_57021
constexpr uint16_t GK_57039 = GK(0x1cf); // .Func_57039
constexpr uint16_t GK_5707B = GK(0x211); // .Func_5707b
constexpr uint16_t GK_5710F = GK(0x2a5); // .Func_5710f
constexpr uint16_t GK_57198 = GK(0x32e); // .Func_57198

constexpr uint16_t PLATFORM_MOVE_LEFT = ADDR_GKTortoisePlatformFunc + 0x35; // .MoveLeft
constexpr uint16_t PLATFORM_MOVE_UP_AND_DOWN = ADDR_GKTortoisePlatformFunc + 0x58; // .MoveUpAndDown

constexpr uint16_t SHOOT_UPDATE_SCORE = ADDR_ShootGoalCounterFunc + 0x20; // .UpdateScore
constexpr uint16_t WARIO_UPDATE_SCORE = ADDR_WarioGoalCounterFunc + 0x20; // .UpdateScore

// wShootBallState
constexpr uint8_t SHOOTBALL_WARIO_KICKED = 5;
constexpr uint8_t SHOOTBALL_WARIO_GOAL = 6;

// Shoot's var1
constexpr uint8_t SHOOTPHASE_STANDING = 0;
constexpr uint8_t SHOOTPHASE_JUMPING_UP = 1;
constexpr uint8_t SHOOTPHASE_JUMPING_DOWN = 2;
constexpr uint8_t SHOOTPHASE_BETWEEN_JUMPS = 3;
constexpr uint8_t SHOOTPHASE_TURNING = 4;
constexpr uint8_t SHOOTPHASE_PREKICK_JUMP = 5;
constexpr uint8_t SHOOTPHASE_PREKICK_ANIMATION = 6;
constexpr uint8_t SHOOTPHASE_KICKING = 7;
constexpr uint8_t SHOOTPHASE_SCORE_GOAL = 8;
constexpr uint8_t SHOOTPHASE_WON = 9;

// GK Tortoise's var1
constexpr uint8_t GKTORTOISEPHASE_0 = 0;
constexpr uint8_t GKTORTOISEPHASE_1 = 1;
constexpr uint8_t GKTORTOISEPHASE_5 = 5;
constexpr uint8_t GKTORTOISEPHASE_DUCKING = 7;
constexpr uint8_t GKTORTOISEPHASE_8 = 8;
constexpr uint8_t GKTORTOISEPHASE_9 = 9;

constexpr uint8_t NUM_SHOOT_GOALS_NEEDED = 3;
constexpr uint8_t NUM_WARIO_GOALS_NEEDED = 3;

constexpr uint8_t SHOOT_TAUNT_DURATION = 80;
constexpr uint8_t SHOOT_IDLE_DURATION = 27;
constexpr uint8_t SHOOT_JUMP_UP_DURATION = 22;
constexpr uint8_t SHOOT_TURN_AROUND_DURATION = 42;
constexpr uint8_t SHOOT_DEFEAT_RISE_DURATION = 28;
constexpr uint8_t SHOOT_DEFEAT_SUSPENDED_DURATION = 70;

// .HandleNormalState's jumptable, by phase
constexpr uint16_t PHASES[] = {
	STANDING, JUMPING_UP, JUMPING_DOWN, BETWEEN_JUMPS, TURNING,
	PREKICK_JUMP, PREKICK_ANIMATION, KICKING, SCORE_GOAL, WON,
};
constexpr uint8_t NUM_PHASES = sizeof(PHASES) / sizeof(PHASES[0]);

// .BallForm's jumptable, by state
constexpr uint16_t BALL_HANDLERS[] = {
	B_56B20, B_56B2A, B_56BAE, B_56BB8, B_56C1B, B_56C3D, B_56B20, B_56B20, // $00
	B_56BAE, B_56BB8, B_56B96, B_56BAE, B_56BB8, B_56B20, B_56B20, B_56B20,
	B_56B20, B_56BD1, B_56BD1, B_56B20, B_56BDB, B_56BE3, B_56BF1, B_56BF8, // $10
	B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	B_56AF0, B_56AF0, B_56B20, B_56B20, B_56B20, B_56D1C, B_56B20, B_56B20,
	ADDR_ObjState_Fall, B_56B47, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, B_56C49, B_56C49, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20,
	B_56B20, ADDR_ObjState_StandingFall, ADDR_Func_3371, B_56B20, ADDR_Func_32cc, ADDR_Func_32db, B_56BFF, B_56C0D, // $40
	B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20, B_56B20,
};
constexpr uint8_t NUM_BALL_HANDLERS = sizeof(BALL_HANDLERS) / sizeof(BALL_HANDLERS[0]);

// .Main's jumptable, by phase
constexpr uint16_t GK_PHASES[] = {
	GK_56F27, GK_56F2E, GK_56FEF, GK_57021, GK_5707B,
	GK_57039, GK_57198, GK_DUCKING, GK_DEFEATED, GK_5710F,
};
constexpr uint8_t NUM_GK_PHASES = sizeof(GK_PHASES) / sizeof(GK_PHASES[0]);

class Shoot : public Object {
public:
	using Object::Object;

	void start(); // ShootFunc
	void start_wait(); // .StartWait
	void taunt(); // .Taunt
	void main(); // .Main
	void run(uint16_t handler);
	void move_wario_ball_after_kick(); // .MoveWarioBallAfterKick
	void prekick_x_movement(); // .DoPreKickingXMovement
	void move_left_or_right(); // .MoveLeftOrRight
	void ball_form(); // .BallForm
	void run_ball(uint16_t handler);
	void ball_move(); // .Func_56cdb

	// DoPreKickingJumpYMovement, ApplyYOffsetOnKickedWarioBall, Func_5725b
	void prekick_y_movement(uint16_t offsets);
	void kicked_ball_y_offset(uint16_t offsets);
	void move_wario_up_by_neg(uint8_t b);

	void goal_counter(uint16_t update); // ShootGoalCounterFunc, WarioGoalCounterFunc
	void update_score(uint16_t goals, bool shoot); // .UpdateScore

private:
	void set_standing(); // .set_standing
	void set_jumping_up(uint16_t frameset); // .set_jumping_up
	void out_of_ball(bool vdir); // .asm_566b7 / .asm_566bd
	void interacted_with_ball_wario(); // .interacted_with_ball_wario
	void stepped_on(); // .stepped_on
	void still_more_goals_needed(); // .still_more_goals_needed
	bool ground_below(uint8_t offset);
	void return_to_regular_form(); // .return_to_regular_form
	void ball_56b47(); // .Func_56b47
	void ball_56bc0(uint8_t s); // .asm_56bc0
	void ball_56bdb(uint8_t s);
	void ball_56c25(uint8_t s, uint16_t frameset); // .asm_56c25
	void set_defeated(); // .set_defeated
	uint8_t kick_offset(uint16_t offsets);
};

class GKTortoise : public Object {
public:
	using Object::Object;

	void start(); // GKTortoiseFunc
	void main(); // .Main
	void run(uint16_t handler);
	void platform(); // GKTortoisePlatformFunc
	void platform_move_left(); // .MoveLeft
	void platform_move_up_and_down(); // .MoveUpAndDown

private:
	void set_ducking(); // .set_ducking
	void catch_ball(); // .asm_56fd2
	uint16_t shoot_pos(); // .GetShootPosPtr
	void stop_shoot(); // .Func_57170
	void shoot_to(int dy, int dx);
	void wario_to(int dy, int dx);
	void stand_back(); // .asm_56f7d
	void reset(); // .asm_57009
	void phase_56f2e(); // .Func_56f2e
	bool frame_ended(uint8_t &offset);
};

} // namespace

// --- Shoot ---

void Shoot::start()
{
	mem_[wIsBossBattle] = TRUE;
	stop_music2(mem_);
	mem_[wShootCanTurnAround] = 0;
	mem_[wShootBallState] = 0;
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(START_WAIT);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() |= 1 << OBJSUBFLAG_UNK_4_F;
	substate() = (substate() & OBJSUBFLAGS_MASK) | 0x5;
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-8);
	set_frameset(ADDR_Frameset_69f7c);
	state_duration() = SHOOT_TAUNT_DURATION;
	// 32 pixels up and 96 pixels right
	var2() = 32;
	move_up_by_var2();
	var2() = 96;
	move_right_by_var2();
}

void Shoot::start_wait()
{
	if (!(flags() & (1 << OBJFLAG_ON_SCREEN_F)))
		return;
	set_update(TAUNT);
}

void Shoot::set_standing()
{
	bool right = facing_right();
	if (right)
		substate() |= OBJSUBFLAG_VDIR;
	else
		substate() &= ~OBJSUBFLAG_VDIR;
	set_frameset(right ? ADDR_Frameset_6a03a : ADDR_Frameset_6a022);
	state_duration() = SHOOT_IDLE_DURATION;
	var1() = SHOOTPHASE_STANDING;
	var2() = 1; // the jump's x speed
}

void Shoot::set_jumping_up(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = SHOOT_JUMP_UP_DURATION;
	var1() = SHOOTPHASE_JUMPING_UP;
	var3() = 0;
	play_sfx(SFX_0C8);
}

void Shoot::taunt()
{
	// a sound every 16 frames
	if (!(mem_[wGlobalCounter] & 0x0f))
		play_sfx(SFX_0C6);
	if (--state_duration())
		return;
	set_update(MAIN);
	set_interaction(OBJ_INTERACTION_SHOOT);
	mem_[wShootBallState] = 0x01;
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
	set_standing();
}

// jumps out of the ball, vdir its direction
void Shoot::out_of_ball(bool vdir)
{
	state() = 0;
	if (vdir)
		substate() |= OBJSUBFLAG_VDIR;
	else
		substate() &= ~OBJSUBFLAG_VDIR;
	uint16_t frameset = facing_right() ? ADDR_Frameset_6a03a + 2 : ADDR_Frameset_6a022 + 2;
	var2() = 2; // out of the ball's x speed
	set_jumping_up(frameset);
}

// grabbed Ball o' String Wario: jumps up with him, to kick him
void Shoot::interacted_with_ball_wario()
{
	state() = 0;
	var3() = 0;
	var2() = 16;
	move_up_by_var2();
	set_frameset(ADDR_Frameset_69ffd);
	var1() = SHOOTPHASE_PREKICK_JUMP;
	var2() = 1; // the pre-kick x speed
	// Wario 16 pixels below
	uint8_t lo = mem_[wCurObjYPos + 0];
	mem_[wWarioYPos + 1] = static_cast<uint8_t>(lo + 16);
	mem_[wWarioYPos + 0] = static_cast<uint8_t>(mem_[wWarioYPos + 0] + (lo + 16 > 0xff));
	mem_[wWarioXPos + 1] = mem_[wCurObjXPos + 0];
	play_sfx(SFX_FAT_WALK);
	substate() |= OBJSUBFLAG_HDIR;
	// bug: meant to compare the x position with 123, it compares a, the
	// low byte of SFX_FAT_WALK, so it always sets the direction right
	if ((SFX_FAT_WALK & 0xff) < 123)
		substate() |= OBJSUBFLAG_VDIR;
	else
		substate() &= ~OBJSUBFLAG_VDIR;
}

// stomped on: curls into a ball
void Shoot::stepped_on()
{
	set_frameset(ADDR_Frameset_69f5d);
	set_update(BALL_FORM);
	interaction() = OBJ_INTERACTION_01;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-18);
	mem_[wCurObjCollBoxBottom] = 0;
	ball_form();
}

void Shoot::main()
{
	uint8_t s = state();
	if (!s) {
		uint8_t phase = var1();
		if (phase >= NUM_PHASES) {
			unrealistic();
			return;
		}
		run(PHASES[phase]);
		return;
	}
	switch (s) {
	case 0x0e:
		play_sfx(SFX_0C9);
		out_of_ball(!facing_right());
		return;
	case 0x04:
		out_of_ball(true);
		return;
	case 0x05:
		out_of_ball(false);
		return;
	case OBJSTATE_SPECIAL_3:
		interacted_with_ball_wario();
		return;
	}
	s &= 0xfe;
	if (s == OBJSTATE_WOBBLE_LEFT_START || s == OBJSTATE_VANISH_TOUCH) {
		stepped_on();
		return;
	}
	state() = 0;
}

// turns to face Wario (or when it may), else jumps again
void Shoot::still_more_goals_needed()
{
	bool turn = false;
	uint8_t wario_x = mem_[wWarioXPos + 1];
	if (wario_x < 192) {
		bool c = mem_[wCurObjXPos + 0] < wario_x;
		turn = facing_right() ? !c : c;
	}
	if (!turn)
		turn = mem_[wShootCanTurnAround];
	bool right = facing_right();
	if (turn) {
		set_frameset(right ? ADDR_Frameset_69f40 : ADDR_Frameset_69f35);
		state_duration() = SHOOT_TURN_AROUND_DURATION;
		var1() = SHOOTPHASE_TURNING;
		mem_[wShootCanTurnAround] = 0;
		return;
	}
	set_frameset(right ? ADDR_Frameset_6a04f : ADDR_Frameset_6a037);
	state_duration() = mem_[wWarioGoals] ? 2 : 25; // the jumping delay
	var1() = SHOOTPHASE_BETWEEN_JUMPS;
}

// the ground offset pixels below (Func_352b, x as it was)
bool Shoot::ground_below(uint8_t offset)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + offset);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	routines_.call(ADDR_Func_352b);
	return gb_->cpu.a;
}

void Shoot::run(uint16_t handler)
{
	switch (handler) {
	case STANDING:
		if (--state_duration())
			return;
		set_jumping_up(facing_right() ? ADDR_Frameset_6a043 : ADDR_Frameset_6a02b);
		return;
	case JUMPING_UP:
		if (--state_duration()) {
			SET_BC(gb_, ADDR_Data_603d0);
			routines_.call(ADDR_ApplyObjYMovement);
			move_left_or_right();
			return;
		}
		var1() = SHOOTPHASE_JUMPING_DOWN;
		var3() = 0;
		return;
	case JUMPING_DOWN: {
		SET_BC(gb_, ADDR_Data_603e0);
		routines_.call(ADDR_ApplyObjYMovement);
		move_left_or_right();
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (gb_->cpu.a) { // grounded
			mem_[wCurObjYPos + 0] = mem_[hYPosLo];
			mem_[wCurObjYPos + 1] = mem_[hYPosHi];
			still_more_goals_needed();
			return;
		}
		// lands soon: the landing frames
		uint16_t frameset;
		if (ground_below(16))
			frameset = facing_right() ? ADDR_Frameset_6a04c : ADDR_Frameset_6a034;
		else if (ground_below(32))
			frameset = facing_right() ? ADDR_Frameset_6a049 : ADDR_Frameset_6a031;
		else if (ground_below(48))
			frameset = facing_right() ? ADDR_Frameset_6a046 : ADDR_Frameset_6a02e;
		else
			return;
		set_frameset(frameset);
		return;
	}
	case BETWEEN_JUMPS:
		if (--state_duration())
			return;
		set_standing();
		return;
	case TURNING:
		if (!--state_duration()) {
			set_standing();
			return;
		}
		if (state_duration() == SHOOT_TURN_AROUND_DURATION / 2)
			substate() ^= OBJSUBFLAG_HDIR;
		return;
	case PREKICK_JUMP:
		prekick_x_movement();
		prekick_y_movement(ADDR_Data_54000);
		if (mem_[wCurObjYPos + 0] < 224)
			return;
		set_frameset(ADDR_Frameset_6a000);
		var1() = SHOOTPHASE_PREKICK_ANIMATION;
		var2() = 16;
		move_down_by_var2();
		// Shoot and Wario at y = 240, Wario at Shoot's x + 8
		mem_[wCurObjYPos + 0] = 240;
		mem_[wWarioYPos + 1] = 240;
		mem_[wWarioYPos + 0] = mem_[wCurObjYPos + 1];
		mem_[wWarioXPos + 1] = static_cast<uint8_t>(mem_[wCurObjXPos + 0] + 8);
		return;
	case PREKICK_ANIMATION: {
		if (frame_duration())
			return;
		uint8_t offset = mem_[wCurObjFramesetOffset];
		if (offset == 6) { // Wario 8 pixels right
			mem_[wWarioXPos + 1] = static_cast<uint8_t>(mem_[wWarioXPos + 1] + 8);
			play_sfx(SFX_HANG_GRAB);
			return;
		}
		if (offset != 10)
			return;
		var1() = SHOOTPHASE_KICKING;
		set_frameset(ADDR_Frameset_69f54);
		return;
	}
	case KICKING: {
		if (mem_[wShootBallState] == SHOOTBALL_WARIO_KICKED)
			move_wario_ball_after_kick();
		if (frame_duration())
			return;
		uint8_t offset = mem_[wCurObjFramesetOffset];
		if (offset == 4) { // kicks Wario
			mem_[wShootBallState] = SHOOTBALL_WARIO_KICKED;
			var3() = 0;
			mem_[wAutoMoveState] = 0x2 | (1 << 7);
			play_sfx(SFX_034);
			return;
		}
		if (offset != 8)
			return;
		var1() = SHOOTPHASE_SCORE_GOAL;
		set_frameset(ADDR_Frameset_6a00b);
		return;
	}
	case SCORE_GOAL: {
		if (mem_[wShootBallState] != SHOOTBALL_WARIO_GOAL) {
			move_wario_ball_after_kick();
			return;
		}
		// b: the goals now (with 3 already, whatever b held)
		uint8_t goals = gb_->cpu.b;
		if (mem_[wShootGoals] != NUM_SHOOT_GOALS_NEEDED)
			goals = ++mem_[wShootGoals];
		mem_[wAutoMoveState] = 0x1 | (1 << 7);
		play_sfx(SFX_0C7);
		mem_[wShootCanTurnAround] = TRUE;
		substate() &= ~OBJSUBFLAG_VDIR;
		if (goals != NUM_SHOOT_GOALS_NEEDED) {
			still_more_goals_needed();
			return;
		}
		var1() = SHOOTPHASE_WON;
		return;
	}
	case WON:
		no_actions();
		return;
	default:
		unrealistic();
		return;
	}
}

void Shoot::move_wario_ball_after_kick()
{
	if (mem_[wWarioXPos + 1] < 237) { // Wario right
		kicked_ball_y_offset(ADDR_Data_54010);
		gb_->cpu.b = 3;
		routines_.call(ADDR_MoveWarioRight);
		return;
	}
	// in the net: Wario down to the ground
	if (mem_[wWarioYPos + 1] < 240) {
		gb_->cpu.b = 3;
		routines_.call(ADDR_MoveWarioDown);
		return;
	}
	mem_[wShootBallState] = SHOOTBALL_WARIO_GOAL;
}

void Shoot::prekick_x_movement()
{
	gb_->cpu.b = var2();
	if (substate() & OBJSUBFLAG_VDIR) {
		move_right_by_var2();
		routines_.call(ADDR_MoveWarioRight);
		return;
	}
	move_left_by_var2();
	routines_.call(ADDR_MoveWarioLeft);
}

// moves by var2 along its vertical direction, turning at the pitch's ends
void Shoot::move_left_or_right()
{
	uint8_t x_pos = mem_[wCurObjXPos + 0];
	if (substate() & OBJSUBFLAG_VDIR) {
		if (x_pos < 219) {
			move_right_by_var2();
			return;
		}
		play_sfx(SFX_08D);
		substate() &= ~OBJSUBFLAG_VDIR;
		if (!facing_right())
			return;
	} else {
		if (x_pos >= 27) {
			move_left_by_var2();
			return;
		}
		play_sfx(SFX_08D);
		substate() |= OBJSUBFLAG_VDIR;
		if (facing_right())
			return;
	}
	mem_[wShootCanTurnAround] = TRUE;
}

// the next of the offsets ($80-terminated: stays on the last)
uint8_t Shoot::kick_offset(uint16_t offsets)
{
	uint8_t c = static_cast<uint8_t>(var3() + offsets);
	uint16_t page = offsets & 0xff00;
	uint8_t a = mem_[page | c];
	if (a == 0x80)
		return mem_[page | static_cast<uint8_t>(c - 1)];
	++var3();
	return a;
}

// moves Shoot and Wario by the next offset
void Shoot::prekick_y_movement(uint16_t offsets)
{
	uint8_t a = kick_offset(offsets);
	y() = static_cast<uint16_t>(y() + static_cast<int8_t>(a));
	if (a >= 0x80) {
		move_wario_up_by_neg(a);
		return;
	}
	gb_->cpu.b = a;
	routines_.call(ADDR_MoveWarioDown);
	routines_.call(ADDR_HandleDownwardsFloorTransition);
}

void Shoot::kicked_ball_y_offset(uint16_t offsets)
{
	gb_->cpu.b = kick_offset(offsets);
	routines_.call(ADDR_MoveWarioDown);
	routines_.call(ADDR_HandleDownwardsFloorTransition);
}

void Shoot::move_wario_up_by_neg(uint8_t b)
{
	gb_->cpu.b = static_cast<uint8_t>(-b);
	routines_.call(ADDR_MoveWarioUp);
	routines_.call(ADDR_HandleUpwardsFloorTransition);
}

// --- Shoot, ball form ---

void Shoot::ball_form()
{
	// can't be grabbed in the net
	if (mem_[wCurObjXPos + 0] >= 219)
		flags() &= ~OBJFLAG_GRABBED;
	else
		flags() |= OBJFLAG_GRABBED;
	uint8_t s = state();
	if (s >= NUM_BALL_HANDLERS) {
		unrealistic();
		return;
	}
	run_ball(BALL_HANDLERS[s]);
}

void Shoot::return_to_regular_form()
{
	flags() &= ~OBJFLAG_GRABBED;
	set_update(MAIN);
	state() = 0x05;
	substate() &= ~(1 << 7);
	interaction() = OBJ_INTERACTION_SHOOT | HEAVY_OBJ;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-32);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-8);
}

// a ball for a while (var1), wobbling at the end
void Shoot::ball_56b47()
{
	if (var3() == 0x22)
		play_sfx(SFX_075);
	if (!--var1()) {
		return_to_regular_form();
		return;
	}
	uint8_t left = var1();
	bool wobble = false;
	if (left == 0x12) {
		set_frameset(ADDR_Frameset_69f93);
		play_sfx(SFX_0B7);
	} else if (left > 0x12) {
		if (left == 0x64) {
			set_frameset(ADDR_Frameset_69f66);
			wobble = true;
		} else if (left < 0x64) {
			wobble = true;
		}
	}
	if (wobble && !(mem_[wGlobalCounter] & 0x0f))
		play_sfx(SFX_06A);
	SET_BC(gb_, ADDR_Data_60e40);
	routines_.call(ADDR_ApplyObjYMovement_Loop);
}

void Shoot::ball_56bc0(uint8_t s)
{
	mem_[wCurObjAction] = (1 << NO_ACTIONS_F) | 15;
	state() = s;
	set_frameset(ADDR_Frameset_69f5d);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
}

void Shoot::ball_56bdb(uint8_t s)
{
	state() = s;
	var2() = 0x02;
	var3() = 0;
}

void Shoot::ball_56c25(uint8_t s, uint16_t frameset)
{
	state() = s;
	var3() = 0;
	var2() = mem_[wCurObjYPos + 0] < 216 ? 0x04 : 0x02;
	set_frameset(frameset);
}

void Shoot::set_defeated()
{
	set_frameset(ADDR_Frameset_69f8a);
	state_duration() = SHOOT_DEFEAT_RISE_DURATION;
	set_update(DEFEAT_RISE);
	state() = 0x48;
	substate() &= OBJSUBFLAG_UNINITIALISED | OBJSUBFLAG_VDIR | OBJSUBFLAG_HDIR;
	var3() = 0;
	flags() |= OBJFLAG_PRIORITY;
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x09;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
	play_sfx(SFX_0A9);
}

// kicked: rolls along by var2, turning at the pitch's ends
void Shoot::ball_move()
{
	uint8_t x_pos = mem_[wCurObjXPos + 0];
	if (substate() & OBJSUBFLAG_VDIR) {
		if (mem_[wCurObjYPos + 0] >= 208) { // in the net
			if (x_pos < 243)
				move_right_by_var2();
			return;
		}
		if (x_pos < 219) {
			move_right_by_var2();
			return;
		}
		substate() &= ~OBJSUBFLAG_VDIR;
	} else {
		if (x_pos >= 27) {
			move_left_by_var2();
			return;
		}
		substate() |= OBJSUBFLAG_VDIR;
	}
	mem_[wShootCanTurnAround] = TRUE;
	play_sfx(SFX_01A);
}

void Shoot::run_ball(uint16_t handler)
{
	switch (handler) {
	case B_56AF0:
		state() = 0x2d;
		flags() &= ~OBJFLAG_GRABBED;
		set_frameset(ADDR_Frameset_69f5d);
		return;
	case B_56B20:
		state() = 0x30;
		var3() = 0;
		return;
	case B_56B2A:
		state() = 0x31;
		var3() = 0;
		// the ball's duration ($100, or 160 for the last goal)
		var1() = mem_[wWarioGoals] < 2 ? 0 : 160;
		set_frameset(ADDR_Frameset_69f5d);
		ball_56b47();
		return;
	case B_56B47:
		ball_56b47();
		return;
	case B_56B96: {
		play_sfx(SFX_014);
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		ball_56bc0(static_cast<uint8_t>(screen_x() + 0x2a) >= wario ? 0x38 : 0x39);
		return;
	}
	case B_56BAE:
		ball_56bc0(0x38);
		return;
	case B_56BB8:
		ball_56bc0(0x39);
		return;
	case B_56BD1:
		state() = 0x41;
		var3() = 0;
		return;
	case B_56BDB:
		ball_56bdb(0x44);
		return;
	case B_56BE3:
		ball_56bdb(0x45);
		return;
	case B_56BF1:
		state() = 0x46;
		return;
	case B_56BF8:
		state() = 0x47;
		return;
	case B_56BFF:
	case B_56C0D:
		if (var2()) {
			routines_.call(handler == B_56BFF ? ADDR_Func_32ae : ADDR_Func_32bd);
			return;
		}
		state() = 0x0f;
		return;
	case B_56C1B:
		substate() |= OBJSUBFLAG_VDIR;
		ball_56c25(0x34, ADDR_Frameset_69f9f);
		return;
	case B_56C3D:
		substate() &= ~OBJSUBFLAG_VDIR;
		ball_56c25(0x35, ADDR_Frameset_69fa4);
		return;
	case B_56C49: { // kicked
		no_actions();
		if (mem_[wShootCanTurnAround]) {
			mem_[wShootCanTurnAround] = FALSE;
			set_frameset(ADDR_Frameset_69f5d);
		}
		ball_move();
		SET_BC(gb_, var2() == 0x04 ? ADDR_Data_60e70 : ADDR_Data_60e20);
		routines_.call(ADDR_ApplyObjYMovement);
		if (mem_[wCurObjYPos + 0] < 240)
			return;
		mem_[wCurObjYPos + 0] = 240;
		if (mem_[wShootBallState] == 0x04) {
			if (mem_[wWarioGoals] == NUM_WARIO_GOALS_NEEDED) {
				set_defeated();
				return;
			}
			mem_[wShootBallState] = 0x01;
			return_to_regular_form();
			return;
		}
		if (!(substate() & OBJSUBFLAG_VDIR)) {
			state() = 0x09;
			return;
		}
		if (mem_[wCurObjXPos + 0] < 219) {
			state() = 0x08;
			return;
		}
		// in the net: a goal
		state() = 0x2d;
		mem_[wShootBallState] = 0x02;
		play_sfx(SFX_0C7);
		if (mem_[wWarioGoals] != NUM_WARIO_GOALS_NEEDED) {
			if (++mem_[wWarioGoals] == NUM_WARIO_GOALS_NEEDED)
				stop_music2(mem_);
		}
		set_frameset(ADDR_Frameset_69f9a);
		return;
	}
	case B_56D1C:
		no_actions();
		flags() &= ~OBJFLAG_GRABBED;
		if (mem_[wShootBallState] != 0x04)
			return;
		set_frameset(ADDR_Frameset_69fa4);
		state() = 0x35;
		substate() &= ~OBJSUBFLAG_VDIR;
		var3() = 0;
		var2() = mem_[wWarioGoals] == NUM_WARIO_GOALS_NEEDED ? 0x01 : 0x03;
		return;
	case DEFEAT_RISE:
		no_actions();
		if (--state_duration()) {
			SET_BC(gb_, ADDR_Data_60e90);
			routines_.call(ADDR_ApplyObjYMovement);
			return;
		}
		var3() = 0;
		set_frameset(ADDR_Frameset_69f6f);
		state_duration() = SHOOT_DEFEAT_SUSPENDED_DURATION;
		set_update(DEFEAT_SUSPEND);
		return;
	case DEFEAT_SUSPEND:
		no_actions();
		if (!state_duration()) { // falls
			SET_BC(gb_, ADDR_Data_60ea0);
			routines_.call(ADDR_ApplyObjYMovement);
			return;
		}
		if (--state_duration())
			return;
		play_sfx(SFX_0A3);
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

// --- Goal counters ---

void Shoot::goal_counter(uint16_t update)
{
	set_update(update);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	flags() |= OBJFLAG_NO_COLLISION;
	frame() = 0x3c;
	var2() = 0x58; // $58 pixels up
	move_up_by_var2();
}

void Shoot::update_score(uint16_t goals, bool shoot)
{
	no_actions();
	frame_duration() = 0x7f;
	switch (mem_[goals]) {
	case 1:
		flags() &= ~OBJFLAG_INVISIBLE;
		return;
	case 2:
		frame() = 0x3d;
		return;
	case 3:
		frame() = 0x3e;
		if (shoot)
			mem_[wBossState] = 0;
		return;
	}
}

// --- GK Tortoise ---

void GKTortoise::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(GK_MAIN);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() |= 1 << OBJSUBFLAG_UNK_4_F;
	mem_[wCurObjCollBoxRight] = 5;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-6);
	set_frameset(ADDR_Frameset_69fed);
	var1() = GKTORTOISEPHASE_0;
	// 64 pixels right and 16 pixels down
	var2() = 64;
	move_right_by_var2();
	var2() = 16;
	move_down_by_var2();
}

void GKTortoise::set_ducking()
{
	set_frameset(ADDR_Frameset_6a017);
	var1() = GKTORTOISEPHASE_DUCKING;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-10);
}

void GKTortoise::main()
{
	if (mem_[wTransformation])
		no_actions();
	// ducks when Shoot kicks Wario towards the net
	if (mem_[wShootBallState] == SHOOTBALL_WARIO_KICKED && var1() != GKTORTOISEPHASE_DUCKING) {
		set_ducking();
		return;
	}
	uint8_t phase = var1();
	if (phase >= NUM_GK_PHASES) {
		unrealistic();
		return;
	}
	run(GK_PHASES[phase]);
}

void GKTortoise::catch_ball()
{
	mem_[wShootBallState] = 0x07;
	set_frameset(ADDR_Frameset_69fba);
	var1() = 0x06;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-20);
	play_sfx(SFX_FAT_WALK);
}

// the first active Shoot's y position; with none, where the scan stopped
// (the last object's id, or its flags + 1 if that's an inactive Shoot)
uint16_t GKTortoise::shoot_pos()
{
	uint16_t page = wObj1 & 0xff00;
	for (uint8_t b = wObj1 & 0xff;; b += OBJ_STRUCT_LENGTH) {
		uint16_t obj = page | b;
		bool is_shoot = mem_[obj + OBJ_ID] == SHOOT;
		if (is_shoot && (mem_[obj + OBJ_FLAGS] & OBJFLAG_ACTIVE))
			return obj + OBJ_Y_POS;
		if (b == (wObj8 & 0xff))
			return is_shoot ? obj + OBJ_FLAGS + 1 : obj + OBJ_ID;
	}
}

// stops the first active Shoot's actions for 8 frames
void GKTortoise::stop_shoot()
{
	uint16_t page = wObj1 & 0xff00;
	for (uint8_t b = wObj1 & 0xff;; b += OBJ_STRUCT_LENGTH) {
		uint16_t obj = page | b;
		if (mem_[obj + OBJ_ID] == SHOOT && (mem_[obj + OBJ_FLAGS] & OBJFLAG_ACTIVE)) {
			mem_[obj + OBJ_ACTION] = (1 << NO_ACTIONS_F) | 8;
			return;
		}
		if (b == (wObj8 & 0xff))
			return;
	}
}

// Shoot's position: GK Tortoise's, offset
void GKTortoise::shoot_to(int dy, int dx)
{
	uint16_t at = shoot_pos();
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[at + 0] = static_cast<uint8_t>(y_pos);
	mem_[at + 1] = static_cast<uint8_t>(y_pos >> 8);
	mem_[at + 2] = static_cast<uint8_t>(x_pos);
	mem_[at + 3] = static_cast<uint8_t>(x_pos >> 8);
}

// Wario's position: GK Tortoise's, offset
void GKTortoise::wario_to(int dy, int dx)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[wWarioYPos + 1] = static_cast<uint8_t>(y_pos);
	mem_[wWarioYPos + 0] = static_cast<uint8_t>(y_pos >> 8);
	mem_[wWarioXPos + 1] = static_cast<uint8_t>(x_pos);
	mem_[wWarioXPos + 0] = static_cast<uint8_t>(x_pos >> 8);
}

void GKTortoise::stand_back()
{
	state() = 0;
	set_frameset(ADDR_Frameset_69fcf);
	var1() = 0x02;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-10);
	flags() |= OBJFLAG_NO_COLLISION;
}

void GKTortoise::reset()
{
	set_frameset(ADDR_Frameset_69fa9);
	var1() = GKTORTOISEPHASE_1;
	state() = 0;
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-20);
	flags() &= ~OBJFLAG_NO_COLLISION;
}

void GKTortoise::phase_56f2e()
{
	if (mem_[wShootBallState] == 0x02) {
		mem_[wShootBallState] = 0x03;
		set_frameset(ADDR_Frameset_69fba);
		var1() = 0x04;
		return;
	}
	mem_[wCurObjAction] = 0;
	uint8_t s = state();
	if (!s)
		return;
	if (s == 0x2f) { // grabs Wario
		state() = 0;
		set_frameset(ADDR_Frameset_69ff2);
		var1() = GKTORTOISEPHASE_5;
		play_sfx(SFX_FAT_WALK);
		wario_to(-16, -11);
		return;
	}
	if ((s & 0xfe) != 0x08) {
		stand_back();
		return;
	}
	// grabs Shoot
	state() = 0;
	set_frameset(ADDR_Frameset_69ff2);
	var1() = GKTORTOISEPHASE_9;
	mem_[wShootBallState] = 0x03;
	play_sfx(SFX_FAT_WALK);
	stop_shoot();
	shoot_to(-16, -11);
}

// at the end of a frame: offset is the frameset offset
bool GKTortoise::frame_ended(uint8_t &offset)
{
	if (frame_duration())
		return false;
	offset = mem_[wCurObjFramesetOffset];
	return true;
}

void GKTortoise::run(uint16_t handler)
{
	uint8_t offset;
	switch (handler) {
	case GK_DEFEATED:
		no_actions();
		if (--state_duration())
			return;
		flags() = 0;
		SET_HL(gb_, wCurObjUnk02);
		gb_->cpu.e = 0x08;
		routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
		mem_[wBossState] = BOSS_DEFEATED;
		routines_.call(ADDR_UpdateLevelMusic);
		mem_[wIsBossBattle] = 0;
		return;
	case GK_DUCKING:
		offset = mem_[wCurObjFramesetOffset];
		if (offset == 10) {
			if (frame_duration())
				return;
			catch_ball();
			return;
		}
		if (offset < 6)
			return;
		// a delay if Wario's still not in the net
		if (mem_[wShootBallState] == SHOOTBALL_WARIO_GOAL)
			return;
		frame_duration() = 16;
		return;
	case GK_56F27:
		if (mem_[wShootBallState]) {
			reset();
			return;
		}
		phase_56f2e();
		return;
	case GK_56F2E:
		phase_56f2e();
		return;
	case GK_56FEF:
		if (mem_[wShootBallState] != 0x02 && (state() & 0xfe)) {
			stand_back();
			return;
		}
		if (mem_[wCurObjFramesetOffset] != 0x0c || frame_duration())
			return;
		reset();
		return;
	case GK_57021:
		flags() &= ~(1 << 3);
		if (state() & 0xfe) {
			stand_back();
			return;
		}
		if (mem_[wCurObjFramesetOffset] != 0x06 || frame_duration())
			return;
		reset();
		return;
	case GK_57039: // throws Wario
		if (!frame_ended(offset))
			return;
		switch (offset) {
		case 2:
			wario_to(-22, 0);
			return;
		case 4:
			wario_to(-16, -11);
			return;
		case 6:
			mem_[wAutoMoveState] = 0;
			mem_[wCurObjAction] = (1 << NO_ACTIONS_F) | 15;
			play_sfx(SFX_067);
			return;
		case 10:
			reset();
			return;
		}
		return;
	case GK_5707B: // throws Shoot back
		flags() |= OBJFLAG_NO_COLLISION;
		if (!frame_ended(offset))
			return;
		switch (offset) {
		case 0x06:
			play_sfx(SFX_FAT_WALK);
			shoot_to(0, 0x0c);
			return;
		case 0x08:
			shoot_to(-0x16, 0);
			return;
		case 0x0e:
			shoot_to(-0x10, -0x0b);
			return;
		case 0x10:
			mem_[wShootBallState] = 0x04;
			play_sfx(SFX_067);
			return;
		case 0x14:
			if (mem_[wWarioGoals] != NUM_WARIO_GOALS_NEEDED) {
				reset();
				return;
			}
			set_frameset(ADDR_Frameset_6a00e);
			state_duration() = 180;
			var1() = GKTORTOISEPHASE_8;
			mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-10);
			return;
		}
		return;
	case GK_5710F:
		if (!frame_ended(offset))
			return;
		switch (offset) {
		case 2:
			shoot_to(-22, 0);
			return;
		case 4:
			shoot_to(-16, -11);
			return;
		case 6:
			mem_[wShootBallState] = 0x04;
			play_sfx(SFX_067);
			return;
		case 10:
			reset();
			return;
		}
		return;
	case GK_57198:
		if (!frame_ended(offset))
			return;
		switch (offset) {
		case 0x06:
			wario_to(0, 0x0c);
			return;
		case 0x08:
			wario_to(-0x16, 0);
			return;
		case 0x0e:
			wario_to(-0x10, -0x0b);
			return;
		case 0x10:
			mem_[wShootBallState] = 0x01;
			mem_[wAutoMoveState] = 0;
			play_sfx(SFX_067);
			return;
		case 0x14:
			reset();
			return;
		}
		return;
	default:
		unrealistic();
		return;
	}
}

// --- GK Tortoise platform ---

void GKTortoise::platform()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(PLATFORM_MOVE_LEFT);
	substate() = (substate() & ~(1 << OBJSUBFLAG_UNINITIALISED_F)) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 5;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-6);
	set_frameset(ADDR_Frameset_69fdc);
	state_duration() = 32;
	var1() = 60;
	// 64 pixels right and 16 pixels down
	var2() = 64;
	move_right_by_var2();
	var2() = 16;
	move_down_by_var2();
}

void GKTortoise::platform_move_left()
{
	one_way_platform();
	if (state_duration()) {
		--state_duration();
		no_actions();
		move_left();
		return;
	}
	if (--var1())
		return;
	state_duration() = 176;
	set_update(PLATFORM_MOVE_UP_AND_DOWN);
}

// var1: moving down
void GKTortoise::platform_move_up_and_down()
{
	one_way_platform();
	if (var1()) {
		if (++state_duration() == 176) {
			var1() = 0;
			state() = 0;
			return;
		}
		if (flags() & OBJFLAG_STEPPED) {
			flags() &= ~OBJFLAG_STEPPED;
			gb_->cpu.b = 1;
			routines_.call(ADDR_MoveWarioDown);
			routines_.call(ADDR_HandleDownwardsFloorTransition);
		}
		move_down();
		return;
	}
	if (!--state_duration()) {
		var1() = 0x01;
		return;
	}
	if (flags() & OBJFLAG_STEPPED) {
		flags() &= ~OBJFLAG_STEPPED;
		gb_->cpu.b = 1;
		routines_.call(ADDR_MoveWarioUp);
		routines_.call(ADDR_HandleUpwardsFloorTransition);
	}
	move_up();
}

} // namespace wl3::level

using wl3::level::GKTortoise;
using wl3::level::Shoot;

PORT_EX(ShootFunc, 0, ALL_REGISTERS) { Shoot(gb).start(); }
PORT_LOCAL_EX(ShootFunc, StartWait, 0, ALL_REGISTERS) { Shoot(gb).start_wait(); }
PORT_LOCAL_EX(ShootFunc, Taunt, 0, ALL_REGISTERS) { Shoot(gb).taunt(); }
PORT_LOCAL_EX(ShootFunc, Main, 0, ALL_REGISTERS) { Shoot(gb).main(); }
PORT_LOCAL_EX(ShootFunc, Standing, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::STANDING); }
PORT_LOCAL_EX(ShootFunc, JumpingUp, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::JUMPING_UP); }
PORT_LOCAL_EX(ShootFunc, JumpingDown, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::JUMPING_DOWN); }
PORT_LOCAL_EX(ShootFunc, BetweenJumps, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::BETWEEN_JUMPS); }
PORT_LOCAL_EX(ShootFunc, Turning, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(ShootFunc, PreKickJump, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::PREKICK_JUMP); }
PORT_LOCAL_EX(ShootFunc, PreKickAnimation, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::PREKICK_ANIMATION); }
PORT_LOCAL_EX(ShootFunc, Kicking, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::KICKING); }
PORT_LOCAL_EX(ShootFunc, ScoreGoal, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::SCORE_GOAL); }
PORT_LOCAL_EX(ShootFunc, Won, 0, ALL_REGISTERS) { Shoot(gb).run(wl3::level::WON); }
PORT_LOCAL_EX(ShootFunc, MoveWarioBallAfterKick, 0, ALL_REGISTERS) { Shoot(gb).move_wario_ball_after_kick(); }
PORT_LOCAL_EX(ShootFunc, DoPreKickingXMovement, 0, ALL_REGISTERS) { Shoot(gb).prekick_x_movement(); }
PORT_LOCAL_EX(ShootFunc, MoveLeftOrRight, 0, ALL_REGISTERS) { Shoot(gb).move_left_or_right(); }
PORT_LOCAL_EX(ShootFunc, BallForm, 0, ALL_REGISTERS) { Shoot(gb).ball_form(); }
PORT_LOCAL_EX(ShootFunc, Func_56af0, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56AF0); }
PORT_LOCAL_EX(ShootFunc, Func_56b20, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56B20); }
PORT_LOCAL_EX(ShootFunc, Func_56b2a, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56B2A); }
PORT_LOCAL_EX(ShootFunc, Func_56b47, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56B47); }
PORT_LOCAL_EX(ShootFunc, Func_56b96, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56B96); }
PORT_LOCAL_EX(ShootFunc, Func_56bae, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BAE); }
PORT_LOCAL_EX(ShootFunc, Func_56bb8, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BB8); }
PORT_LOCAL_EX(ShootFunc, Func_56bd1, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BD1); }
PORT_LOCAL_EX(ShootFunc, Func_56bdb, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BDB); }
PORT_LOCAL_EX(ShootFunc, Func_56be3, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BE3); }
PORT_LOCAL_EX(ShootFunc, Func_56bf1, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BF1); }
PORT_LOCAL_EX(ShootFunc, Func_56bf8, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BF8); }
PORT_LOCAL_EX(ShootFunc, Func_56bff, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56BFF); }
PORT_LOCAL_EX(ShootFunc, Func_56c0d, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56C0D); }
PORT_LOCAL_EX(ShootFunc, Func_56c1b, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56C1B); }
PORT_LOCAL_EX(ShootFunc, Func_56c3d, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56C3D); }
PORT_LOCAL_EX(ShootFunc, Func_56c49, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56C49); }
PORT_LOCAL_EX(ShootFunc, Func_56cdb, 0, ALL_REGISTERS) { Shoot(gb).ball_move(); }
PORT_LOCAL_EX(ShootFunc, Func_56d1c, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::B_56D1C); }
PORT_LOCAL_EX(ShootFunc, DefeatRise, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::DEFEAT_RISE); }
PORT_LOCAL_EX(ShootFunc, DefeatSuspendAndFall, 0, ALL_REGISTERS) { Shoot(gb).run_ball(wl3::level::DEFEAT_SUSPEND); }
PORT_EX(DoPreKickingJumpYMovement, 0, ALL_REGISTERS) { Shoot(gb).prekick_y_movement(REG_BC(gb)); }
PORT_EX(ApplyYOffsetOnKickedWarioBall, 0, ALL_REGISTERS) { Shoot(gb).kicked_ball_y_offset(REG_BC(gb)); }
PORT_EX(Func_5725b, 0, ALL_REGISTERS) { Shoot(gb).move_wario_up_by_neg(gb->cpu.b); }
PORT_EX(ShootGoalCounterFunc, 0, ALL_REGISTERS) { Shoot(gb).goal_counter(wl3::level::SHOOT_UPDATE_SCORE); }
PORT_EX(WarioGoalCounterFunc, 0, ALL_REGISTERS) { Shoot(gb).goal_counter(wl3::level::WARIO_UPDATE_SCORE); }
PORT_LOCAL_EX(ShootGoalCounterFunc, UpdateScore, 0, ALL_REGISTERS) { Shoot(gb).update_score(wShootGoals, true); }
PORT_LOCAL_EX(WarioGoalCounterFunc, UpdateScore, 0, ALL_REGISTERS) { Shoot(gb).update_score(wWarioGoals, false); }

PORT_EX(GKTortoiseFunc, 0, ALL_REGISTERS) { GKTortoise(gb).start(); }
PORT_LOCAL_EX(GKTortoiseFunc, Main, 0, ALL_REGISTERS) { GKTortoise(gb).main(); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_56edd, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_DEFEATED); }
PORT_LOCAL_EX(GKTortoiseFunc, Ducking, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_DUCKING); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_56f27, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_56F27); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_56f2e, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_56F2E); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_56fef, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_56FEF); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_57021, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_57021); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_57039, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_57039); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_5707b, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_5707B); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_5710f, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_5710F); }
PORT_LOCAL_EX(GKTortoiseFunc, Func_57198, 0, ALL_REGISTERS) { GKTortoise(gb).run(wl3::level::GK_57198); }
PORT_EX(GKTortoisePlatformFunc, 0, ALL_REGISTERS) { GKTortoise(gb).platform(); }
PORT_LOCAL_EX(GKTortoisePlatformFunc, MoveLeft, 0, ALL_REGISTERS) { GKTortoise(gb).platform_move_left(); }
PORT_LOCAL_EX(GKTortoisePlatformFunc, MoveUpAndDown, 0, ALL_REGISTERS) { GKTortoise(gb).platform_move_up_and_down(); }
