// The temple shaking (bank56.asm, _HandleTempleShakeAndRocks): the screen
// shakes left and right, rocks fall from the ceiling, and others bounce
// along the floor. Each rock has a 4-byte struct (wTempleRock1..10: action,
// counter, size, respawn time) and a scene object. See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

static_assert(wTempleRock1Counter == wTempleRock1 + 1 && wTempleRock1Size == wTempleRock1 + 2
	&& wTempleRock1RespawnTime == wTempleRock1 + 3);
static_assert(wSceneObj12State == wSceneObj12 + 6);

constexpr uint16_t ROCK_COUNTER = 1;
constexpr uint16_t ROCK_SIZE = 2;
constexpr uint16_t ROCK_RESPAWN_TIME = 3;
constexpr uint16_t SCENE_OBJ_STATE = 6;

// the rocks' actions
enum Action : uint8_t {
	IDLE, // StubTempleRockFunc
	SPAWN,
	UPDATE,
	RESET,
};

constexpr uint8_t SMALL_ROCK_STATE = 0x09; // below TEMPLE_ROCK_SIZE_LARGE

// temple_rock_fall: rock, its scene object, whether it can be large, its
// two x coords (by rDIV's bit 3) and the y it falls to
struct FallingRock {
	uint16_t rock, obj;
	bool can_be_large;
	uint8_t x1, x2, max_y;
};
constexpr FallingRock FALLING[6] = {
	{wTempleRock1, wSceneObj12, true, 32, 48, 96},
	{wTempleRock2, wSceneObj13, true, 112, 128, 96},
	{wTempleRock3, wSceneObj14, false, 40, 56, 96},
	{wTempleRock4, wSceneObj15, false, 104, 120, 96},
	{wTempleRock5, wSceneObj1, false, 16, 64, 160},
	{wTempleRock6, wSceneObj2, false, 96, 144, 160},
};

// temple_rock_bounce: rock, its scene object, fast or not, left or right,
// its two x coords
struct BouncingRock {
	uint16_t rock, obj;
	bool fast, left;
	uint8_t x1, x2;
};
constexpr BouncingRock BOUNCING[4] = {
	{wTempleRock7, wSceneObj8, false, true, 32, 40},
	{wTempleRock8, wSceneObj9, false, false, 120, 128},
	{wTempleRock9, wSceneObj10, true, true, 24, 32},
	{wTempleRock10, wSceneObj11, true, false, 128, 136},
};
constexpr uint8_t BOUNCE_Y = 0x60;

// GetRockYVel.y_vel_table, by the counter (capped at $2c, back to $28)
constexpr uint8_t Y_VELOCITIES[0x2c] = {
	1, 0, 1, 0, 1, 0, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 2, 1, 2, 1, 2, 1,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3,
};

// HandleTempleScreenShake's steps, from w2d896 = 1: wSCX's move, every
// other frame (0: .Init or .Next), whether it starts the sound, whether it
// goes back 7 steps after
struct ShakeStep {
	int8_t dx;
	bool sfx, repeat;
};
constexpr ShakeStep SHAKE_STEPS[20] = {
	{0, false, false}, // .Init
	{0, false, false}, // .Next
	{-1, true, false}, // .SlowScrollLeftWithSFX
	{1, false, false}, {-1, false, false}, {1, false, false}, {-1, false, false},
	{1, false, false}, {-1, false, false},
	{1, false, true}, // .SlowScrollRightAndReset
	{0, false, false}, // .Next
	{-1, false, false},
	{2, true, false}, // .FastScrollRightWithSFX
	{-2, false, false}, {2, false, false}, {-2, false, false}, {2, false, false},
	{-2, false, false}, {2, false, false},
	{-2, false, true}, // .FastScrollLeftAndReset
};
constexpr uint8_t SHAKE_INIT = 0;

} // namespace

void TempleRocks::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// _HandleTempleShakeAndRocks
void TempleRocks::update()
{
	shake();
	for (const auto &r : FALLING)
		fall(r.rock, r.obj, r.can_be_large, r.x1, r.x2, r.max_y);
	for (const auto &r : BOUNCING)
		bounce(r.rock, r.obj, r.fast, r.left, r.x1, r.x2);
}

// HandleTempleScreenShake: wSCX one or two pixels left and right, every
// other frame; w2d896 is the step (0 for none), w2d897 its frames
void TempleRocks::shake()
{
	uint8_t step = wram_.unk_2d896();
	if (!step)
		return;
	Byte frames = wram_.unk_2d897();
	++frames;
	step = static_cast<uint8_t>(step - 1);
	if (step >= 20) {
		unrealistic();
		return;
	}
	if (step == SHAKE_INIT) {
		frames = 0;
		wram_.unk_2d896() = 0;
		wram_.scx() = 0;
		wram_.scy() = 0;
		return;
	}
	const ShakeStep &s = SHAKE_STEPS[step];
	if (!s.dx) {
		next_shake_step();
		return;
	}
	if (s.sfx)
		play_sfx(mem_, SFX_01F);
	if (frames < 2)
		return;
	wram_.scx() += static_cast<uint8_t>(s.dx);
	if (s.repeat) {
		frames = 0;
		wram_.unk_2d896() -= 7;
		return;
	}
	next_shake_step();
}

// HandleTempleScreenShake.Next
void TempleRocks::next_shake_step()
{
	wram_.unk_2d897() = 0;
	++wram_.unk_2d896();
}

// temple_rock_fall
void TempleRocks::fall(uint16_t rock, uint16_t obj, bool can_be_large, uint8_t x1, uint8_t x2, uint8_t max_y)
{
	Byte counter = mem_[static_cast<uint16_t>(rock + ROCK_COUNTER)];
	++counter;
	switch (mem_[rock]) {
	case IDLE:
		return;
	case SPAWN: {
		uint8_t seed = mem_[rDIV];
		spawn(obj, 0, seed, x1, x2);
		set_size(rock, obj, seed, can_be_large);
		next_action(rock);
		return;
	}
	case UPDATE: {
		Byte y = mem_[obj];
		y += y_velocity(counter);
		if (y < max_y)
			return;
		mem_[static_cast<uint16_t>(obj + SCENE_OBJ_STATE)] = 0;
		bool large = mem_[static_cast<uint16_t>(rock + ROCK_SIZE)] == TEMPLE_ROCK_SIZE_LARGE;
		uint8_t mask = large ? 0x1f : 0x3f;
		mem_[static_cast<uint16_t>(rock + ROCK_RESPAWN_TIME)] = static_cast<uint8_t>(mem_[rDIV] & mask);
		play_sfx(mem_, SFX_TEMPLE_ROCK); // PlayFallingRockSFX
		next_action(rock);
		return;
	}
	case RESET: // Func_1580c8: after its respawn time, again (if it has a size)
		if (counter < mem_[static_cast<uint16_t>(rock + ROCK_RESPAWN_TIME)])
			return;
		if (mem_[static_cast<uint16_t>(rock + ROCK_SIZE)]) {
			mem_[rock] = SPAWN;
			return;
		}
		counter = 0;
		mem_[rock] = IDLE;
		return;
	default:
		unrealistic();
		return;
	}
}

// temple_rock_bounce
void TempleRocks::bounce(uint16_t rock, uint16_t obj, bool fast, bool left, uint8_t x1, uint8_t x2)
{
	switch (mem_[rock]) {
	case IDLE:
		return;
	case SPAWN: {
		uint8_t seed = mem_[rDIV];
		spawn(obj, BOUNCE_Y, seed, x1, x2);
		set_size(rock, obj, seed, false);
		play_sfx(mem_, SFX_TEMPLE_ROCK);
		next_action(rock);
		return;
	}
	case UPDATE:
		// fast: twice a frame, along Data_1588de; slow: Data_158897
		for (int i = 0; i < (fast ? 2 : 1); i++) {
			if (!move(obj, fast ? ADDR_Data_1588de : ADDR_Data_158897, left)) {
				next_action(rock);
				return;
			}
		}
		return;
	case RESET:
		mem_[rock] = mem_[static_cast<uint16_t>(rock + ROCK_SIZE)] ? SPAWN : IDLE;
		mem_[static_cast<uint16_t>(obj + SCENE_OBJ_STATE)] = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// SpawnRock: at y, and x1 or x2 by the seed's bit 3
void TempleRocks::spawn(uint16_t obj, uint8_t y, uint8_t seed, uint8_t x1, uint8_t x2)
{
	mem_[obj] = y;
	mem_[static_cast<uint16_t>(obj + 1)] = (seed & (1 << 3)) ? x2 : x1;
}

// SetTempleRockSize_WithLarge / _WithoutLarge: the object's state by the
// rock's size, plus 0 to 2 (or 0 to 1) from the seed
void TempleRocks::set_size(uint16_t rock, uint16_t obj, uint8_t seed, bool can_be_large)
{
	uint8_t size = mem_[static_cast<uint16_t>(rock + ROCK_SIZE)];
	uint8_t state = SMALL_ROCK_STATE;
	if (size >= TEMPLE_ROCK_SIZE_LARGE) {
		uint8_t extra = can_be_large ? static_cast<uint8_t>(seed & 3) : static_cast<uint8_t>(seed & 1);
		if (extra == 3)
			extra = 2;
		state = static_cast<uint8_t>(size + extra);
	}
	gb_->cpu.a = state;
	SET_HL(gb_, static_cast<uint16_t>(obj + SCENE_OBJ_STATE));
	routines_.call(ADDR_SetSceneObjState);
}

// .Next: the next action, from frame 0
void TempleRocks::next_action(uint16_t rock)
{
	mem_[static_cast<uint16_t>(rock + ROCK_COUNTER)] = 0;
	++mem_[rock];
}

// GetRockYVel
uint8_t TempleRocks::y_velocity(Byte counter)
{
	uint8_t frames = counter;
	if (frames >= 0x2c) {
		frames = 0x28;
		counter = frames;
	}
	return Y_VELOCITIES[frames];
}

// ApplyOWMovement(_Mirrored): a step along the movement data; false at its
// end (the asm's z)
bool TempleRocks::move(uint16_t obj, uint16_t data, bool left)
{
	SET_DE(gb_, data);
	SET_HL(gb_, obj);
	routines_.call(left ? ADDR_ApplyOWMovement : ADDR_ApplyOWMovement_Mirrored);
	return !(gb_->cpu.f & FLAG_Z);
}

} // namespace wl3::overworld

// Only farcalled, by HandleTempleShakeAndRocks, whose callers reload every
// register they use.
PORT_EX(_HandleTempleShakeAndRocks, 0, ALL_REGISTERS)
{
	wl3::overworld::TempleRocks(gb).update();
}

PORT_COVERED(HandleTempleScreenShake);
PORT_COVERED(Func_1580c8);
PORT_COVERED(StubTempleRockFunc);
PORT_COVERED(SetTempleRockSize_WithLarge);
PORT_COVERED(SetTempleRockSize_WithoutLarge);
PORT_COVERED(SpawnRock_Top);
PORT_COVERED(SpawnRock);
PORT_COVERED(SpawnRock_Bottom);
PORT_COVERED(PlayFallingRockSFX);
PORT_COVERED(HandleTempleRock1Fall);
PORT_COVERED(HandleTempleRock2Fall);
PORT_COVERED(HandleTempleRock3Fall);
PORT_COVERED(HandleTempleRock4Fall);
PORT_COVERED(HandleTempleRock5Fall);
PORT_COVERED(HandleTempleRock6Fall);
PORT_COVERED(GetRockYVel);
PORT_COVERED(HandleTempleRock7Bounce);
PORT_COVERED(HandleTempleRock8Bounce);
PORT_COVERED(HandleTempleRock9Bounce);
PORT_COVERED(HandleTempleRock10Bounce);
PORT_COVERED(BounceTempleRockRight);
PORT_COVERED(BounceTempleRockLeft);
