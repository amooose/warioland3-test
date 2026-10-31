// Wario in the overworld's scenes (bank2b.asm, UpdateSceneWario and
// SetSceneWarioState). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

enum class Extra : uint8_t {
	PLAIN,
	WALK, // Func_ac4fa: the footsteps
	PANIC_RUN, // Func_ac515: and the panicked ones
	LOOK_AROUND, // .asm_ac65f: a sound at frames 0 and 4
	LOOK_UP, // the surprise's sounds
	SLEEP, // the snoring, each time round
	JUMP_AND_BOW, // .Func_ac7b7: the landing, at frame 7
	PANIC_JUMP, // .Func_ac7ff: the landing, at the end
};

constexpr uint8_t STAY = 0xff; // no state to go to at the animation's end

struct Animation {
	uint16_t frameset;
	uint8_t next; // the state once it's over, or STAY
	Extra extra;
};

// UpdateSceneWario.DoAnimation's table, by state
constexpr Animation ANIMATIONS[] = {
	{0, STAY, Extra::PLAIN}, // SCENEWARIO_NONE
	{ADDR_Frameset_14d18, STAY, Extra::PLAIN}, // .LookFront
	{ADDR_Frameset_14d15, STAY, Extra::PLAIN}, // .LookBack
	{ADDR_Frameset_14cbe, SCENEWARIO_IDLE_LEFT, Extra::PLAIN}, // .TurnFrontLeft
	{ADDR_Frameset_14cc5, SCENEWARIO_IDLE_RIGHT, Extra::PLAIN}, // .TurnFrontRight
	{ADDR_Frameset_14d0b, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .TurnBackLeft
	{ADDR_Frameset_14d10, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .TurnBackRight
	{ADDR_Frameset_149b4, STAY, Extra::WALK}, // .WalkLeft
	{ADDR_Frameset_149c5, STAY, Extra::WALK}, // .WalkRight
	{ADDR_Frameset_149fe, STAY, Extra::WALK}, // .CarryWalk
	{ADDR_Frameset_14a20, STAY, Extra::PLAIN}, // .CarryIdle
	{ADDR_Frameset_163ec, SCENEWARIO_CARRY_IDLE, Extra::PLAIN}, // .PickUp
	{ADDR_Frameset_14252, STAY, Extra::PLAIN}, // .IdleLeft
	{ADDR_Frameset_1425f, STAY, Extra::PLAIN}, // .IdleRight
	{ADDR_Frameset_d4de4, STAY, Extra::PLAIN}, // .Nod
	{ADDR_Frameset_d4de4, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .NodOnce
	{ADDR_Frameset_aa6a5, SCENEWARIO_LOOK_TURN_FORWARD, Extra::LOOK_AROUND}, // .LookAround
	{ADDR_Frameset_aa6c2, SCENEWARIO_LOOK_FORWARD, Extra::PLAIN}, // .LookTurnForward
	{ADDR_Frameset_aa6c7, STAY, Extra::PLAIN}, // .LookForward
	{ADDR_Frameset_aa6a5, SCENEWARIO_TURN_LEFT_END, Extra::LOOK_AROUND}, // .LookAroundThenTurn
	{ADDR_Frameset_aa6ca, SCENEWARIO_TURN_RIGHT_END, Extra::LOOK_AROUND}, // .LookTurnRight
	{ADDR_Frameset_aa8ec, STAY, Extra::PLAIN}, // .OnFire
	{ADDR_Frameset_aa8f5, SCENEWARIO_STUNNED_RIGHT, Extra::PLAIN}, // .CrashLandRight
	{ADDR_Frameset_aa902, STAY, Extra::PLAIN}, // .CrashTumble
	{ADDR_Frameset_aa902, SCENEWARIO_STUNNED_RIGHT, Extra::PLAIN}, // .CrashTumbleEnd
	{ADDR_Frameset_aa90d, STAY, Extra::PLAIN}, // .StunnedRight
	{ADDR_Frameset_aa913, SCENEWARIO_IDLE_RIGHT, Extra::LOOK_UP}, // .LookUp
	{ADDR_Frameset_aa924, SCENEWARIO_STUNNED_LEFT, Extra::PLAIN}, // .CrashLandLeft
	{ADDR_Frameset_aa931, STAY, Extra::PLAIN}, // .StunnedLeft
	{ADDR_Frameset_aab27, SCENEWARIO_HOLD_MUSIC_BOX, Extra::PLAIN}, // .LookMusicBox
	{ADDR_Frameset_aab32, SCENEWARIO_LOOK_UP_SURPRISED, Extra::PLAIN}, // .PickUpMusicBox
	{ADDR_Frameset_aab3f, STAY, Extra::PLAIN}, // .HoldMusicBox
	{ADDR_Frameset_aab42, STAY, Extra::PLAIN}, // .LookUpSurprised
	{ADDR_Frameset_aab45, SCENEWARIO_CARRY_IDLE, Extra::PLAIN}, // .CarrySnapFingers
	{ADDR_Frameset_aaccb, STAY, Extra::SLEEP}, // .Sleep
	{ADDR_Frameset_aace2, SCENEWARIO_SIT_IDLE, Extra::PLAIN}, // .WakeUp
	{ADDR_Frameset_aacf7, STAY, Extra::PLAIN}, // .SitStill
	{ADDR_Frameset_aacfa, STAY, Extra::PLAIN}, // .SitIdle
	{ADDR_Frameset_ab03c, SCENEWARIO_LOOK_BACK, Extra::JUMP_AND_BOW}, // .JumpAndBow
	{ADDR_Frameset_ab06b, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .SnapFingers
	{ADDR_Frameset_ab08c, SCENEWARIO_SHOW_MUSIC_BOXES_2, Extra::PLAIN}, // .ShowMusicBoxes1
	{ADDR_Frameset_ab098, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .ShowMusicBoxes5
	{ADDR_Frameset_ab09d, SCENEWARIO_LOOK_BACK, Extra::PANIC_JUMP}, // .PanicJump
	{ADDR_Frameset_ab09d, STAY, Extra::PLAIN}, // .Panic
	{ADDR_Frameset_ab0ac, STAY, Extra::PLAIN}, // .Bow
	{ADDR_Frameset_ab14d, SCENEWARIO_SHOW_MUSIC_BOXES_3, Extra::PLAIN}, // .ShowMusicBoxes2
	{ADDR_Frameset_ab160, STAY, Extra::PLAIN}, // .ShowMusicBoxes3
	{ADDR_Frameset_ab163, SCENEWARIO_SHOW_MUSIC_BOXES_5, Extra::PLAIN}, // .ShowMusicBoxes4
	{ADDR_Frameset_1fc439, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .TurnFrontToBackRight
	{ADDR_Frameset_1fc446, SCENEWARIO_LOOK_BACK, Extra::PLAIN}, // .TurnFrontToBackLeft
	{ADDR_Frameset_1fc453, SCENEWARIO_IDLE_RIGHT, Extra::PLAIN}, // .TurnBackToRight
	{ADDR_Frameset_1fc45a, SCENEWARIO_IDLE_LEFT, Extra::PLAIN}, // .TurnBackToLeft
	{ADDR_Frameset_1fc461, SCENEWARIO_LOOK_FRONT, Extra::PLAIN}, // .TurnRightToFront
	{ADDR_Frameset_1fc46a, SCENEWARIO_LOOK_FRONT, Extra::PLAIN}, // .TurnLeftToFront
	{ADDR_Frameset_1fc473, SCENEWARIO_IDLE_LEFT, Extra::PLAIN}, // .TurnFrontToLeft
	{ADDR_Frameset_1fc47c, SCENEWARIO_IDLE_RIGHT, Extra::PLAIN}, // .TurnFrontToRight
	{ADDR_Frameset_1fc485, SCENEWARIO_IDLE_RIGHT, Extra::PLAIN}, // .TurnLeftEnd
	{ADDR_Frameset_1fc488, SCENEWARIO_IDLE_LEFT, Extra::PLAIN}, // .TurnRightEnd
	{ADDR_Frameset_16b5a, STAY, Extra::PANIC_RUN}, // .PanicRunLeft
	{ADDR_Frameset_16b63, STAY, Extra::PANIC_RUN}, // .PanicRunRight
	{ADDR_Frameset_1fc316, STAY, Extra::PLAIN}, // .IdleFront
};
static_assert(sizeof(ANIMATIONS) / sizeof(ANIMATIONS[0]) == NUM_SCENEWARIO_STATES);

// SetSceneWarioState's groups' first states (SCENEWARIO_GROUP_*)
constexpr uint8_t GROUPS[] = {
	SCENEWARIO_GROUP_00, SCENEWARIO_GROUP_01, SCENEWARIO_GROUP_02, SCENEWARIO_GROUP_03,
	SCENEWARIO_GROUP_04, SCENEWARIO_GROUP_05, SCENEWARIO_GROUP_06, SCENEWARIO_GROUP_07,
	SCENEWARIO_GROUP_08, SCENEWARIO_GROUP_19, SCENEWARIO_GROUP_10, SCENEWARIO_GROUP_11,
	SCENEWARIO_GROUP_12, SCENEWARIO_GROUP_13,
};

constexpr uint16_t SPRITE_POINTERS = ADDR_SetSceneWarioState + 0x8e; // .WarioSpritePointers

} // namespace

void SceneWario::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// SetSceneWarioState: the state; a new group of states loads its sprites'
// OAM and (by HDMA) tiles. The registers as the asm leaves them: C calls it.
void SceneWario::set_state(uint8_t state)
{
	mem_[wSceneWarioState] = state;
	uint8_t group = 0;
	while (group < 13 && state >= GROUPS[group])
		++group;
	gb_->cpu.b = group;
	if (mem_[wSceneWarioStateGroup] != group) { // .LoadGfx
		mem_[wSceneWarioStateGroup] = group;
		gb_->cpu.c = static_cast<uint8_t>(group * 2);
		uint16_t entry = static_cast<uint16_t>(SPRITE_POINTERS + static_cast<uint8_t>(group * 6));
		mem_[wSceneWarioOAMPtr] = mem_[entry]; // dab: the sprites
		mem_[wSceneWarioOAMPtr + 1] = mem_[entry + 1];
		mem_[wSceneWarioOAMBank] = mem_[entry + 2];
		mem_[wHDMABank] = mem_[entry + 3]; // dba: the tiles
		uint8_t lo = mem_[entry + 4];
		mem_[wHDMA + 0] = mem_[entry + 5];
		mem_[wHDMA + 1] = lo;
		mem_[wHDMA + 2] = 0;
		mem_[wHDMA + 3] = 0;
		mem_[wHDMA + 4] = 0x80 - 1; // $80 dma_tiles
		mem_[wHDMADestVRAMBank] = 0;
		gb_->cpu.b = lo;
		SET_DE(gb_, static_cast<uint16_t>(entry + 5));
	}
	mem_[wSceneWarioDuration] = 0;
	mem_[wSceneWarioDuration + 1] = 0;
	mem_[w2d14c] = 0;
	SET_HL(gb_, static_cast<uint16_t>(wSceneWarioDuration + 1));
	gb_->cpu.a = 0;
	gb_->cpu.f = FLAG_Z;
}

// UpdateSceneWario: his animation, and his sprite (with the scroll, or
// without)
void SceneWario::update()
{
	animate();
	if (!mem_[wSceneWarioState]) {
		// as the asm leaves them (InitTempleScene, in C, passes them on):
		// JumpTable's to .None, then and a
		SET_DE(gb_, 0x0000);
		SET_HL(gb_, ADDR_UpdateSceneWario + 0x9d); // .None
		gb_->cpu.a = 0;
		gb_->cpu.f = FLAG_Z | FLAG_H;
		return;
	}
	SET_DE(gb_, mem_.word(wSceneWarioOAMPtr));
	gb_->cpu.b = mem_[wSceneWarioOAMBank];
	SET_HL(gb_, wSceneWario);
	routines_.call(mem_[wSceneWarioIgnoreScroll] ? ADDR_LoadOWSprite : ADDR_LoadOWSpriteWithScroll);
}

// UpdateSceneWario.DoAnimation: a frame of the state's animation, on to
// the next state at its end (and that one's first frame)
void SceneWario::animate()
{
	uint8_t state = mem_[wSceneWarioState];
	if (state >= NUM_SCENEWARIO_STATES) {
		unrealistic();
		return;
	}
	if (!state) // .None
		return;
	const Animation &anim = ANIMATIONS[state];
	// .UpdateAnimation
	gb_->cpu.b = mem_[wSceneWarioOAMBank];
	SET_HL(gb_, wSceneWarioDuration);
	SET_DE(gb_, anim.frameset);
	routines_.call(ADDR_UpdateOWAnimation);
	bool finished = mem_[wOWAnimationFinished] == 0xff;

	switch (anim.extra) {
	case Extra::WALK: footsteps(); return;
	case Extra::PANIC_RUN: panic_steps(); return;
	case Extra::SLEEP: // the snoring
		mem_[w2d14e] = mem_[wOWAnimationFinished];
		if (finished)
			play_sfx(mem_, SFX_SNORE);
		return;
	case Extra::JUMP_AND_BOW: // .Func_ac7b7
		if (mem_[wSceneWarioFrame] == 0x07 && !mem_[wSceneWarioUnk7]) {
			mem_[wSceneWarioUnk7] = 0x01;
			play_sfx(mem_, SFX_LAND);
		}
		break;
	case Extra::PANIC_JUMP: // .Func_ac7ff
		if (finished)
			play_sfx(mem_, SFX_LAND);
		break;
	default:
		break;
	}
	if (anim.next != STAY && finished) { // .SetStateIfFinished
		set_state(anim.next);
		animate();
	}
	if (anim.extra == Extra::LOOK_AROUND)
		look_around_sounds();
	else if (anim.extra == Extra::LOOK_UP)
		look_up_sounds();
}

// Func_ac4fa: a footstep every 0x24 frames
void SceneWario::footsteps()
{
	Byte steps = mem_[w2d14c];
	if (!steps)
		play_sfx(mem_, SFX_12A);
	++steps;
	if (steps >= 0x24)
		steps = 0;
}

// Func_ac515: and a panicked one every 8
void SceneWario::panic_steps()
{
	Byte steps = mem_[w2d14c];
	++steps;
	if (steps < 0x08)
		return;
	steps = 0;
	play_sfx(mem_, SFX_137);
}

// UpdateSceneWario.asm_ac65f: looking around, a sound as he turns (frames
// 0 and 4), once each (w2d14c)
void SceneWario::look_around_sounds()
{
	if (mem_[wSceneWarioState] == SCENEWARIO_LOOK_BACK)
		return;
	uint8_t frame = mem_[wSceneWarioFrame];
	bool turning = frame == 0x00 || frame == 0x04;
	Byte played = mem_[w2d14c];
	if (played == 0) {
		if (turning) {
			play_sfx(mem_, SFX_0F1);
			played = 0x01;
		}
	} else if (played == 1 && !turning) {
		played = 0;
	}
}

// UpdateSceneWario.LookUp's: frame 0x0c's sound and 0x0d's, once each
void SceneWario::look_up_sounds()
{
	if (mem_[wSceneWarioState] == SCENEWARIO_IDLE_RIGHT)
		return;
	uint8_t frame = mem_[wSceneWarioFrame];
	Byte played = mem_[w2d14c];
	switch (played) {
	case 0:
		if (frame == 0x0c) {
			play_sfx(mem_, SFX_011);
			played = 0x02;
		} else if (frame == 0x0d) {
			play_sfx(mem_, SFX_0F2);
			played = 0x01;
		}
		return;
	case 1:
		if (frame != 0x0d)
			played = 0;
		return;
	case 2:
		if (frame != 0x0c)
			played = 0;
		return;
	default:
		return;
	}
}

} // namespace wl3::overworld

// UpdateSceneWario: called by the scenes (C++), nothing reads its
// registers. SetSceneWarioState: still called by C, so it leaves them as
// the asm does.
PORT_EX(UpdateSceneWario, 0, ALL_REGISTERS) { wl3::overworld::SceneWario(gb).update(); }
PORT_EX(SetSceneWarioState, 0, 0) { wl3::overworld::SceneWario(gb).set_state(gb->cpu.a); }

// Only UpdateSceneWario runs these.
PORT_COVERED(Func_ac4fa);
PORT_COVERED(Func_ac515);
