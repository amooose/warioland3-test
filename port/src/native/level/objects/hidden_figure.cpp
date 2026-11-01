// Hidden Figure (hidden_figure.asm): the final boss. The body picks its
// attacks (shaking the screen, slamming and sweeping with its hands,
// grabbing Wario), its face shows where Wario is and is hit by the hands
// Wario throws at it, and the hands fight and can be stunned, carried and
// thrown. The body keeps the battle's state in w1d14b-w1d162. See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t HF(uint16_t offset) { return static_cast<uint16_t>(ADDR_HiddenFigureFunc + offset); }
constexpr uint16_t LH(uint16_t offset) { return static_cast<uint16_t>(ADDR_HiddenFigureLeftHandFunc + offset); }
constexpr uint16_t RH(uint16_t offset) { return static_cast<uint16_t>(ADDR_HiddenFigureRightHandFunc + offset); }
constexpr uint16_t FACE(uint16_t offset) { return static_cast<uint16_t>(ADDR_HiddenFigureFaceFunc + offset); }

constexpr uint16_t START = HF(0x4d); // .Func_4cb9e
constexpr uint16_t MAIN = HF(0x73); // .Func_4cbc4
constexpr uint16_t P_RELEASE = HF(0x97); // .Func_4cbe8
constexpr uint16_t P_WAIT = HF(0xa4); // .Func_4cbf5
constexpr uint16_t P_PICK = HF(0xb4); // .Func_4cc05
constexpr uint16_t P_SHAKE_START = HF(0xeb); // .Func_4cc3c
constexpr uint16_t P_SHAKE = HF(0x136); // .Func_4cc87
constexpr uint16_t P_LEFT_START = HF(0x191); // .Func_4cce2
constexpr uint16_t P_LEFT = HF(0x1a5); // .Func_4ccf6
constexpr uint16_t P_LEFT_WAIT = HF(0x1d7); // .Func_4cd28
constexpr uint16_t P_RIGHT_START = HF(0x1f4); // .Func_4cd45
constexpr uint16_t P_RIGHT = HF(0x208); // .Func_4cd59
constexpr uint16_t P_RIGHT_WAIT = HF(0x23a); // .Func_4cd8b
constexpr uint16_t DEFEATED = HF(0x26b); // .Func_4cdbc
constexpr uint16_t CRUMBLE = HF(0x2b0); // .Func_4ce01

constexpr uint16_t FACE_HURT = FACE(0xcc); // .Func_4cf13
constexpr uint16_t FACE_DEFEATED = FACE(0xf8); // .Func_4cf3f
constexpr uint16_t FACE_LAUGH = FACE(0x10e); // .Func_4cf55

constexpr uint16_t L_REST = LH(0x1c); // .Func_4d0c7
constexpr uint16_t L_STING = LH(0xb9); // .Func_4d164
constexpr uint16_t L_BACK = LH(0xf3); // .Func_4d19e
constexpr uint16_t L_SLAM = LH(0x106); // .Func_4d1b1
constexpr uint16_t L_SWEEP = LH(0x21d); // .Func_4d2c8
constexpr uint16_t L_HOLD = LH(0x2ce); // .Func_4d379
constexpr uint16_t L_DEFEATED = LH(0x2d4); // .Func_4d37f
constexpr uint16_t L_RISE = LH(0x2f0); // .Func_4d39b

constexpr uint16_t R_REST = RH(0x1c); // .Func_4d3c8
constexpr uint16_t R_STING = RH(0xb9); // .Func_4d465
constexpr uint16_t R_BACK = RH(0xf3); // .Func_4d49f
constexpr uint16_t R_SLAM = RH(0x106); // .Func_4d4b2
constexpr uint16_t R_SWEEP = RH(0x1c5); // .Func_4d571
constexpr uint16_t R_HOLD = RH(0x276); // .Func_4d622
constexpr uint16_t R_DEFEATED = RH(0x27c); // .Func_4d628
constexpr uint16_t R_SINK = RH(0x298); // .Func_4d644
constexpr uint16_t STUNNED = RH(0x2a9); // .Func_4d655
constexpr uint16_t H_4D73E = RH(0x392); // .Func_4d73e
constexpr uint16_t H_4D750 = RH(0x3a4); // .Func_4d750
constexpr uint16_t H_4D7DC = RH(0x430); // .Func_4d7dc
constexpr uint16_t H_4D7E6 = RH(0x43a); // .Func_4d7e6
constexpr uint16_t H_4D7FD = RH(0x451); // .Func_4d7fd
constexpr uint16_t H_4D824 = RH(0x478); // .Func_4d824
constexpr uint16_t H_4D83A = RH(0x48e); // .Func_4d83a
constexpr uint16_t H_4D844 = RH(0x498); // .Func_4d844
constexpr uint16_t H_4D859 = RH(0x4ad); // .Func_4d859
constexpr uint16_t H_4D861 = RH(0x4b5); // .Func_4d861
constexpr uint16_t H_4D86F = RH(0x4c3); // .Func_4d86f
constexpr uint16_t H_4D876 = RH(0x4ca); // .Func_4d876
constexpr uint16_t H_4D87D = RH(0x4d1); // .Func_4d87d
constexpr uint16_t H_4D88B = RH(0x4df); // .Func_4d88b
constexpr uint16_t H_4D899 = RH(0x4ed); // .Func_4d899
constexpr uint16_t H_4D8B1 = RH(0x505); // .Func_4d8b1
constexpr uint16_t H_4D8C9 = RH(0x51d); // .Func_4d8c9
constexpr uint16_t H_4D8D8 = RH(0x52c); // .Func_4d8d8
constexpr uint16_t H_4D8E1 = RH(0x535); // .Func_4d8e1
constexpr uint16_t H_4D8F0 = RH(0x544); // .Func_4d8f0

// .Func_4cbc4's jumptable, by var1
constexpr uint16_t PHASES[] = {
	P_WAIT, P_PICK, P_LEFT_START, P_LEFT, P_RIGHT_START, P_RIGHT, P_SHAKE_START, P_SHAKE,
	P_RELEASE, P_LEFT_WAIT, P_RIGHT_WAIT,
};
constexpr uint8_t NUM_PHASES = sizeof(PHASES) / sizeof(PHASES[0]);

// .Func_4d655's jumptable, by state
constexpr uint16_t HAND_HANDLERS[] = {
	H_4D7DC, H_4D7E6, H_4D83A, H_4D844, H_4D83A, H_4D844, H_4D899, H_4D8B1, // $00
	H_4D83A, H_4D844, H_4D824, H_4D83A, H_4D844, H_4D7DC, H_4D7DC, H_4D7DC,
	H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D859, H_4D861, H_4D86F, H_4D876, // $10
	H_4D8E1, H_4D8C9, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	H_4D844, H_4D83A, H_4D7DC, H_4D7DC, H_4D7DC, H_4D750, H_4D73E, H_4D7DC,
	ADDR_ObjState_Fall, H_4D7FD, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, H_4D7DC, H_4D7DC, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC,
	H_4D7DC, ADDR_ObjState_StandingFall, ADDR_Func_3371, H_4D7DC, ADDR_Func_32cc, ADDR_Func_32db, H_4D87D, H_4D88B, // $40
	H_4D8F0, H_4D8D8, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC, H_4D7DC,
};
constexpr uint8_t NUM_HAND_HANDLERS = sizeof(HAND_HANDLERS) / sizeof(HAND_HANDLERS[0]);

class HiddenFigure : public Object {
public:
	using Object::Object;

	void start(); // HiddenFigureFunc
	void wait_start(); // .Func_4cb9e
	void main(); // .Func_4cbc4
	void run(uint16_t handler);
	void defeated(); // .Func_4cdbc
	void crumble(); // .Func_4ce01

	void face(); // HiddenFigureFaceFunc
	void face_laugh_start(uint16_t page); // Func_4ce39, with h's page
	void face_hurt(); // .Func_4cf13
	void face_defeated(); // .Func_4cf3f
	void face_laugh(); // .Func_4cf55

	void hand_start(bool right); // HiddenFigureLeftHandFunc, HiddenFigureRightHandFunc
	void hand_rest(bool right); // .Func_4d0c7, .Func_4d3c8
	void hand_sting(bool right); // .Func_4d164, .Func_4d465
	void hand_back(bool right); // .Func_4d19e, .Func_4d49f
	void left_slam(); // .Func_4d1b1
	void right_slam(); // .Func_4d4b2
	void hand_sweep(bool right); // .Func_4d2c8, .Func_4d571
	void hand_hold() { no_actions(); } // .Func_4d379, .Func_4d622
	void hand_defeated(bool right); // .Func_4d37f, .Func_4d628
	void left_rise(); // .Func_4d39b
	void right_sink(); // .Func_4d644
	void stunned(); // .Func_4d655
	void run_hand(uint16_t handler);

	void flash_pals(); // Func_4cfe0
	void set_pals(); // Func_4d005
	// Func_4cf70, Func_4cf87, Func_4cf9e, Func_4cfbf: by the next of the
	// offsets at bc
	void shake_y(uint16_t offsets);
	void shake_x(uint16_t offsets);
	void move_y(uint16_t offsets);
	void move_x(uint16_t offsets);

private:
	void hold();
	uint8_t next_offset(Byte index, uint16_t offsets);
	void attack(uint8_t next, uint8_t which); // .Func_4cce2, .Func_4cd45
	void hand_attack(uint16_t data, uint16_t hand, uint8_t next, uint8_t which); // .Func_4ccf6, .Func_4cd59
	void wait_hand(uint16_t hand); // .Func_4cd28, .Func_4cd8b
	void face_by_x(uint16_t left, uint16_t middle, uint16_t right);
	void face_hurt_frameset(); // .asm_4ce9d
	void set_box(uint8_t type, uint8_t top, uint8_t bottom, uint8_t left, uint8_t right);
	void rest(bool right); // .asm_4d34c, .asm_4d5f5
	void bump(uint8_t s); // .Func_4d83a, .Func_4d844
	void release(); // .Func_4d7fd
	void carried(uint8_t s);
	void carry(bool right);
	void thrown(uint8_t s, uint8_t var2_, uint16_t routine);
};

} // namespace

// --- Hidden Figure ---

void HiddenFigure::hold()
{
	frame_duration() = 0x02;
	no_actions();
}

void HiddenFigure::start()
{
	mem_[wIsBossBattle] = TRUE;
	for (int i = 0; i < 7; i++)
		mem_[w1d14b + i] = 0;
	mem_[w1d15a] = 0x04;
	for (int i = 1; i <= 7; i++)
		mem_[w1d15a + i] = 0;
	mem_[w1d162] = 0x02;
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(START);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_frameset(ADDR_Frameset_6a06c);
	state_duration() = 20;
	SET_BC(gb_, ADDR_ObjParams_HiddenFigureFace);
	routines_.call(ADDR_CreateObjectAtRelativePos);
	SET_BC(gb_, ADDR_ObjParams_HiddenFigureLeftHand);
	routines_.call(ADDR_CreateObjectAtRelativePos);
	SET_BC(gb_, ADDR_ObjParams_HiddenFigureRightHand);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void HiddenFigure::wait_start()
{
	hold();
	if (--state_duration())
		return;
	state_duration() = 0x1e;
	var1() = 0x00;
	set_update(MAIN);
	mem_[wBossState] = BOSS_HIDDEN_FIGURE;
	routines_.call(ADDR_UpdateLevelMusic);
}

void HiddenFigure::main()
{
	hold();
	uint8_t phase = var1();
	if (phase >= NUM_PHASES) {
		unrealistic();
		return;
	}
	run(PHASES[phase]);
}

// the next of the offsets ($80-terminated: stays on the last), by index
uint8_t HiddenFigure::next_offset(Byte index, uint16_t offsets)
{
	uint8_t c = static_cast<uint8_t>(index + offsets);
	uint16_t page = offsets & 0xff00;
	uint8_t a = mem_[page | c];
	if (a == 0x80)
		return mem_[page | static_cast<uint8_t>(c - 1)];
	++index;
	return a;
}

void HiddenFigure::shake_y(uint16_t offsets)
{
	uint8_t a = next_offset(var3(), offsets);
	mem_[wHiddenFigureSCYShake] = static_cast<uint8_t>(mem_[wHiddenFigureSCYShake] + a);
}

void HiddenFigure::shake_x(uint16_t offsets)
{
	uint8_t a = next_offset(var2(), offsets);
	mem_[wc08b] = static_cast<uint8_t>(mem_[wc08b] + a);
}

void HiddenFigure::move_y(uint16_t offsets)
{
	uint8_t a = next_offset(var3(), offsets);
	y() = static_cast<uint16_t>(y() + static_cast<int8_t>(a));
}

void HiddenFigure::move_x(uint16_t offsets)
{
	uint8_t a = next_offset(var2(), offsets);
	x() = static_cast<uint16_t>(x() + static_cast<int8_t>(a));
}

// starts a hand attack
void HiddenFigure::attack(uint8_t next, uint8_t which)
{
	if (--state_duration())
		return;
	state_duration() = 0x80;
	var1() = next;
	var2() = 0;
	var3() = 0;
	mem_[w1d14b] = which;
}

// the hand's attack, shaking the screen; then waits for the hand, if it's
// stunned
void HiddenFigure::hand_attack(uint16_t data, uint16_t hand, uint8_t next, uint8_t which)
{
	shake_y(ADDR_Data_4c000);
	shake_x(data);
	if (--state_duration())
		return;
	mem_[w1d14b] = 0x00;
	if (!mem_[hand]) {
		state_duration() = 0x1e;
		var1() = 0x00;
		return;
	}
	state_duration() = 0x04;
	var1() = next;
	mem_[w1d14b] = which;
}

void HiddenFigure::wait_hand(uint16_t hand)
{
	if (!mem_[w1d15a]) { // beaten
		set_update(DEFEATED);
		state_duration() = 0x82;
		mem_[w1d14b] = 0x06;
		return;
	}
	if (mem_[hand])
		return;
	if (--state_duration())
		return;
	state_duration() = 0x1e;
	var1() = 0x06;
	mem_[w1d14b] = 0x00;
}

void HiddenFigure::run(uint16_t handler)
{
	switch (handler) {
	case P_RELEASE:
		if (!state_duration())
			return;
		if (--state_duration())
			return;
		mem_[wAutoMoveState] = 0;
		return;
	case P_WAIT:
		if (!mem_[w1d15e] && --state_duration())
			return;
		var1() = 0x01;
		return;
	case P_PICK: { // picks the next attack
		state_duration() = 0x01;
		uint8_t count = mem_[w1d160];
		uint8_t next;
		if (count >= 0x02 || mem_[w1d15e]) {
			mem_[w1d15e] = 0;
			mem_[w1d160] = 0;
			next = 0x06;
		} else {
			next = mem_[wWarioScreenXPos] < 0x58 ? 0x02 : 0x04;
			mem_[w1d160] = static_cast<uint8_t>(count + 1);
		}
		var1() = next;
		++mem_[w1d161];
		return;
	}
	case P_SHAKE_START: {
		if (mem_[w1d15c] || mem_[w1d15b])
			return;
		if (--state_duration())
			return;
		state_duration() = 0x80;
		var1() = 0x07;
		var2() = 0;
		var3() = 0;
		uint8_t which = 0x03;
		uint8_t lives = mem_[w1d15a];
		if (lives != 0x04) {
			bool random = true;
			uint8_t wario_x = mem_[wWarioScreenXPos];
			if (lives < 0x03 && wario_x >= 0x30 && wario_x < 0x80 && !(mem_[w1d161] & 1)) {
				state_duration() = 0x60;
				which = 0x08;
				random = false;
			}
			if (random && !(mem_[rDIV] & 1))
				which = 0x07;
		}
		mem_[w1d14b] = which;
		return;
	}
	case P_SHAKE: {
		Byte grab = mem_[w1d14e];
		if (grab) {
			if (grab != 0x80) {
				--grab;
				return;
			}
			state_duration() = 0xa0;
			var1() = 0x08;
			return;
		}
		uint8_t which = mem_[w1d14b];
		shake_y(which == 0x07 ? ADDR_Data_4c580 : which == 0x08 ? ADDR_Data_4c680 : ADDR_Data_4c180);
		if (--state_duration())
			return;
		state_duration() = 0x1e;
		var1() = 0x00;
		mem_[w1d14b] = 0x00;
		if (mem_[w1d15a] != 1)
			return;
		if (!--mem_[w1d162]) {
			mem_[w1d162] = 0x02;
			return;
		}
		var1() = 0x06;
		++mem_[w1d161];
		return;
	}
	case P_LEFT_START:
		attack(0x03, 0x01);
		return;
	case P_RIGHT_START:
		attack(0x05, 0x02);
		return;
	case P_LEFT:
		hand_attack(ADDR_Data_4c080, w1d14c, 0x09, 0x04);
		return;
	case P_RIGHT:
		hand_attack(ADDR_Data_4c100, w1d14d, 0x0a, 0x05);
		return;
	case P_LEFT_WAIT:
		wait_hand(w1d14c);
		return;
	case P_RIGHT_WAIT:
		wait_hand(w1d14d);
		return;
	default:
		unrealistic();
		return;
	}
}

void HiddenFigure::defeated()
{
	hold();
	flash_pals();
	if (mem_[w1d14c] || mem_[w1d14d])
		return;
	if (--state_duration()) {
		if (!(state_duration() & 0x3f))
			play_sfx(SFX_0CE);
		return;
	}
	state_duration() = 0x82;
	var1() = 0;
	set_update(CRUMBLE);
	mem_[w1d15f] = 0x01;
	play_sfx(SFX_0CF);
}

// crumbles, shaking, then ends the level
void HiddenFigure::crumble()
{
	hold();
	if (!(var1() & 0x07))
		play_sfx(SFX_01F);
	++var1();
	if (!state_duration())
		return;
	if (!--state_duration()) {
		mem_[wLevelEndScreen] = LVLEND_EPILOGUE;
		set_pals();
		return;
	}
	flash_pals();
	if (state_duration() & 0x03)
		return;
	--mem_[wHiddenFigureSCYShake];
}

// flashes when it's time
void HiddenFigure::flash_pals()
{
	uint8_t t = mem_[wGlobalCounter] & 0x0f;
	uint16_t pals;
	if (t) {
		if (t != 0x08)
			return;
		pals = ADDR_Pals_4d04b;
	} else {
		pals = mem_[w1d15a] < 0x02 ? ADDR_Pals_4d01b : ADDR_Pals_4d07b;
	}
	SET_HL(gb_, pals);
	gb_->cpu.c = 0;
	gb_->cpu.b = 6;
	routines_.call(ADDR_SetBGPals);
}

void HiddenFigure::set_pals()
{
	SET_HL(gb_, mem_[w1d15a] < 0x02 ? ADDR_Pals_4d01b : ADDR_Pals_4d07b);
	gb_->cpu.c = 0;
	gb_->cpu.b = 6;
	routines_.call(ADDR_SetBGPals);
}

// --- the face ---

// laughs (Wario grabbed); the update function goes to h's page
// bug: from the face, h is still w1d14e's page, so the laugh state never
// takes (docs/ASM_BUGS.md)
void HiddenFigure::face_laugh_start(uint16_t page)
{
	state_duration() = 0x3c;
	mem_[page | (OBJ_UPDATE_FUNCTION + 1)] = static_cast<uint8_t>(FACE_LAUGH >> 8);
	mem_[page | OBJ_UPDATE_FUNCTION] = static_cast<uint8_t>(FACE_LAUGH);
}

// the frameset by Wario's screen x: < $38, < $78, or further right
void HiddenFigure::face_by_x(uint16_t left, uint16_t middle, uint16_t right)
{
	uint8_t wario_x = mem_[wWarioScreenXPos];
	set_frameset(wario_x < 0x38 ? left : wario_x < 0x78 ? middle : right);
}

void HiddenFigure::face_hurt_frameset()
{
	face_by_x(ADDR_Frameset_6a092, ADDR_Frameset_6a08d, ADDR_Frameset_6a097);
}

void HiddenFigure::face()
{
	if (mem_[w1d14e] == 0x80) {
		face_laugh_start(w1d14e & 0xff00);
		return;
	}
	uint8_t s = state();
	if (!s) {
		if (mem_[wCurObjFramesetOffset] != 0x04 || frame_duration())
			return;
		if (mem_[w1d14c] || mem_[w1d14d] || mem_[w1d15a] == 1) {
			face_hurt_frameset();
			return;
		}
		face_by_x(ADDR_Frameset_6a07a, ADDR_Frameset_6a06c, ADDR_Frameset_6a07f);
		return;
	}
	if ((s & 0xfe) != 0x28) {
		state() = 0;
		flags() &= ~OBJFLAG_NO_COLLISION;
		return;
	}
	// hit by a hand
	state() = 0;
	set_frameset(ADDR_Frameset_6a084);
	state_duration() = 0x3c;
	set_update(FACE_HURT);
	mem_[w1d151] = 0x01;
	Byte lives = mem_[w1d15a];
	if (!lives)
		return;
	if (--lives) {
		if (lives == 1)
			play_music2(mem_, MUSIC_HIDDEN_FIGURE_BATTLE_2);
		return;
	}
	set_update(FACE_DEFEATED);
	flags() |= 1 << 3;
	mem_[w1d151] = 0;
	stop_music2(mem_);
	state_duration() = 0x10;
	mem_[wAutoMoveState] = 0x02;
}

void HiddenFigure::face_hurt()
{
	if (--state_duration()) {
		if (state_duration() == 0x2c)
			play_sfx(SFX_0CE);
		flash_pals();
		return;
	}
	mem_[w1d151] = 0;
	set_update(ADDR_HiddenFigureFaceFunc);
	flags() &= ~OBJFLAG_NO_COLLISION;
	set_pals();
	face_hurt_frameset();
}

void HiddenFigure::face_defeated()
{
	no_actions();
	if (!state_duration())
		return;
	if (--state_duration())
		return;
	play_sfx(SFX_0CE);
}

void HiddenFigure::face_laugh()
{
	no_actions();
	if (!state_duration())
		return;
	if (--state_duration())
		return;
	play_sfx(SFX_HIDDEN_FIGURE_LAUGH);
	set_frameset(ADDR_Frameset_6a071);
}

// --- the hands ---

void HiddenFigure::set_box(uint8_t type, uint8_t top, uint8_t bottom, uint8_t left, uint8_t right)
{
	set_interaction(type);
	set_coll_box(top, bottom, left, right);
}

// back to resting
void HiddenFigure::rest(bool right)
{
	set_update(right ? R_REST : L_REST);
	set_box(OBJ_INTERACTION_0B, 0xe8, 0xf8, 0xf8, 0x07);
	state() = 0;
	flags() &= ~OBJFLAG_NO_COLLISION;
	flags() &= ~(1 << 4);
	set_frameset(right ? ADDR_Frameset_6a131 : ADDR_Frameset_6a128);
}

// keeps its starting position (w1d152 / w1d156)
void HiddenFigure::hand_start(bool right)
{
	set_update(right ? R_REST : L_REST);
	uint16_t start = right ? w1d156 : w1d152;
	for (int i = 0; i < 4; i++)
		mem_[start + i] = mem_[wCurObjYPos + i];
}

// waits for the body's attack (w1d14b)
void HiddenFigure::hand_rest(bool right)
{
	no_actions();
	flags() &= ~OBJFLAG_NO_COLLISION;
	uint16_t update;
	uint16_t frameset = right ? ADDR_Frameset_6a0fe : ADDR_Frameset_6a0d4;
	uint8_t which = mem_[w1d14b];
	if (which == (right ? 0x02 : 0x01)) { // sweeps
		mem_[w1d151] = 0x01;
		update = right ? R_SWEEP : L_SWEEP;
		frameset = right ? ADDR_Frameset_6a145 : ADDR_Frameset_6a13a;
	} else if (which == 0x03 || which == 0x07 || which == 0x08) { // slams
		set_box(OBJ_INTERACTION_4F, 0xe0, 0x00, right ? 0xff : 0xf4, right ? 0x0b : 0x00);
		update = right ? R_SLAM : L_SLAM;
		frameset = right ? ADDR_Frameset_6a0c8 : ADDR_Frameset_6a0bc;
	} else if (which == (right ? 0x04 : 0x05)) { // stuck in the ground
		set_interaction(OBJ_INTERACTION_FULL_STING);
		mem_[right ? w1d15b : w1d15c] = 0x01;
		state_duration() = mem_[w1d15a] == 1 ? 0x1e : 0x3c;
		update = right ? R_STING : L_STING;
	} else if (which == 0x06) { // beaten
		flags() |= 1 << 3;
		update = right ? R_DEFEATED : L_DEFEATED;
	} else {
		if (!mem_[w1d151])
			return;
		update = right ? R_BACK : L_BACK;
	}
	set_update(update);
	var2() = 0;
	var3() = 0;
	// bug? (docs/ASM_BUGS.md): the right hand clears the substate where
	// the left clears the state (an ld [hl] for ld [hli])
	if (right)
		substate() = 0;
	else
		state() = 0;
	set_frameset(frameset);
}

// stuck in the ground for a while, then back up
void HiddenFigure::hand_sting(bool right)
{
	if (state_duration()) {
		--state_duration();
		if (mem_[w1d15a] && mem_[w1d14b] != 0x00)
			return;
	} else {
		move_y(ADDR_Data_4c540);
		if (var3() == 0x0a) {
			play_sfx(SFX_063);
			gb_->cpu.b = 0x18;
			routines_.call(ADDR_DoGroundShake);
			return;
		}
		if (var3() != 0x3f)
			return;
	}
	mem_[right ? w1d15b : w1d15c] = 0;
	rest(right);
}

void HiddenFigure::hand_back(bool right)
{
	move_y(ADDR_Data_4c500);
	if (var3() != 0x3a)
		return;
	mem_[w1d151] = 0;
	rest(right);
}

void HiddenFigure::left_slam()
{
	Byte grab = mem_[w1d14f];
	if (grab) {
		if (grab != 0x80) {
			--grab;
			return;
		}
		// grabbed Wario
		set_update(L_HOLD);
		uint16_t wario_y = static_cast<uint16_t>(y() - 0x01);
		uint16_t wario_x = static_cast<uint16_t>(x() + 0x06);
		mem_[wWarioYPos + 1] = static_cast<uint8_t>(wario_y);
		mem_[wWarioYPos + 0] = static_cast<uint8_t>(wario_y >> 8);
		mem_[wWarioXPos + 1] = static_cast<uint8_t>(wario_x);
		mem_[wWarioXPos + 0] = static_cast<uint8_t>(wario_x >> 8);
		flags() |= OBJFLAG_PRIORITY;
		set_frameset(ADDR_Frameset_6a0f2);
		stop_music2(mem_);
		return;
	}
	if (uint8_t s = state()) {
		uint8_t v = 0;
		if (s == 0x2f) { // grabs Wario
			play_sfx(SFX_0CD);
			v = 0x80;
		} else if ((s & 0xfe) == 0x04) {
			v = 0x0a;
		}
		state() = 0;
		if (v) {
			mem_[w1d14e] = v;
			mem_[w1d14f] = v;
			mem_[w1d150] = v;
			return;
		}
	}
	uint8_t hit, end;
	if (mem_[w1d14b] == 0x08) {
		move_x(ADDR_Data_4c700);
		move_y(ADDR_Data_4c800);
		hit = 0x4e;
		end = 0x5f;
	} else {
		move_x(ADDR_Data_4c380);
		move_y(mem_[w1d14b] == 0x07 ? ADDR_Data_4c600 : ADDR_Data_4c400);
		hit = 0x62;
		end = 0x7f;
	}
	uint8_t t = var2();
	if (t == 0x13) {
		set_frameset(ADDR_Frameset_6a0ef);
	} else if (t == hit) {
		set_interaction(OBJ_INTERACTION_4F);
		set_frameset(ADDR_Frameset_6a0c3);
	} else if (t == static_cast<uint8_t>(hit - 1)) {
		set_interaction(OBJ_INTERACTION_50);
		play_sfx(SFX_FAT_WALK);
	} else if (t == end) {
		rest(false);
	}
}

void HiddenFigure::right_slam()
{
	Byte grab = mem_[w1d150];
	if (grab) {
		if (grab != 0x80) {
			--grab;
			return;
		}
		set_update(R_HOLD);
		flags() |= OBJFLAG_PRIORITY;
		set_frameset(ADDR_Frameset_6a11c);
		return;
	}
	if (uint8_t s = state()) {
		state() = 0;
		if ((s & 0xfe) == 0x04) {
			mem_[w1d14e] = 0x0a;
			mem_[w1d14f] = 0x0a;
			mem_[w1d150] = 0x0a;
			return;
		}
	}
	if (mem_[w1d14b] == 0x08) {
		move_x(ADDR_Data_4c760);
		move_y(ADDR_Data_4c800);
		switch (var2()) {
		case 0x13:
			play_sfx(SFX_0D0);
			set_frameset(ADDR_Frameset_6a119);
			return;
		case 0x4e:
			set_frameset(ADDR_Frameset_6a0cf);
			return;
		case 0x5f:
			rest(true);
			return;
		}
		return;
	}
	move_x(ADDR_Data_4c480);
	move_y(mem_[w1d14b] == 0x07 ? ADDR_Data_4c600 : ADDR_Data_4c400);
	switch (var2()) {
	case 0x13:
		play_sfx(SFX_0D1);
		set_frameset(ADDR_Frameset_6a119);
		return;
	case 0x62:
		set_frameset(ADDR_Frameset_6a0cf);
		return;
	case 0x32:
		play_sfx(SFX_0D0);
		return;
	case 0x7f:
		rest(true);
		return;
	}
}

// sweeps across; hit, it's stunned
void HiddenFigure::hand_sweep(bool right)
{
	if (uint8_t s = state()) {
		if (s == 0x0a) {
			state() = 0x2e;
			flags() |= OBJFLAG_PRIORITY;
			set_update(STUNNED);
			set_box(OBJ_INTERACTION_00, 0xec, 0xfe, 0xf8, 0x07);
			var3() = 0;
			mem_[right ? w1d14d : w1d14c] = 1;
			state_duration() = 1;
			stop_sfx(mem_);
			set_frameset(right ? ADDR_Frameset_6a0b7 : ADDR_Frameset_6a0b2);
			return;
		}
		mem_[w1d15e] = 0x01;
		state() = 0;
	}
	move_x(right ? ADDR_Data_4c300 : ADDR_Data_4c200);
	move_y(ADDR_Data_4c280);
	switch (var2()) {
	case 0x10:
		set_interaction(OBJ_INTERACTION_48);
		return;
	case 0x3c:
		set_interaction(OBJ_INTERACTION_52);
		play_sfx(SFX_0CB);
		return;
	case 0x7f:
		rest(right);
		return;
	}
}

void HiddenFigure::hand_defeated(bool right)
{
	no_actions();
	if (!mem_[w1d15f])
		return;
	set_frameset(right ? ADDR_Frameset_6a0c8 : ADDR_Frameset_6a0bc);
	state_duration() = right ? 0x1e : 0x20;
	set_update(right ? R_SINK : L_RISE);
}

// rises every other frame while the state duration lasts
void HiddenFigure::left_rise()
{
	no_actions();
	uint8_t d = state_duration();
	if (!d)
		return;
	--state_duration();
	if (!(d & 1))
		move_up();
}

void HiddenFigure::right_sink()
{
	no_actions();
	uint8_t d = state_duration();
	if (!d)
		return;
	--state_duration();
	if (d & 1)
		move_down();
}

// stunned, or carried: by state
void HiddenFigure::stunned()
{
	mem_[wSCY] = static_cast<uint8_t>(mem_[wc08a] + mem_[wSCYShake]);
	mem_[wSCX] = mem_[wc08c];
	uint8_t s = state();
	if (s != 0x2d && !(mem_[wGlobalCounter] & 0x0f))
		play_sfx(SFX_06A);
	if (s >= NUM_HAND_HANDLERS) {
		unrealistic();
		return;
	}
	run_hand(HAND_HANDLERS[s]);
}

void HiddenFigure::bump(uint8_t s)
{
	mem_[wCurObjAction] = (1 << NO_ACTIONS_F) | 15;
	state() = s;
	state_duration() = 0x04;
	var2() = 0x02;
	var3() = 0;
}

void HiddenFigure::release()
{
	if (--state_duration())
		return;
	no_actions();
	flags() &= ~OBJFLAG_PRIORITY;
	mem_[w1d15d] = 0;
	state() = 0x2d;
	set_frameset(facing_right() ? ADDR_Frameset_6a0d4 : ADDR_Frameset_6a0fe);
}

void HiddenFigure::carried(uint8_t s)
{
	mem_[w1d15d] = 0x01;
	state() = s;
	flags() |= OBJFLAG_GRABBED;
}

// grabbed: turns with Wario
void HiddenFigure::carry(bool right)
{
	for (;;) {
		bool dir = mem_[wDirection];
		if (!right) {
			if (dir) {
				routines_.call(ADDR_Func_3353);
				return;
			}
			carried(0x48);
		} else {
			if (!dir) {
				routines_.call(ADDR_Func_3362);
				return;
			}
			carried(0x49);
		}
		right = !right;
	}
}

void HiddenFigure::thrown(uint8_t s, uint8_t var2_, uint16_t routine)
{
	mem_[w1d15d] = 0x01;
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void HiddenFigure::run_hand(uint16_t handler)
{
	switch (handler) {
	case H_4D73E:
		if (!state_duration()) {
			routines_.call(ADDR_ObjState_Fall);
			return;
		}
		--state_duration();
		play_sfx(SFX_VANISH);
		return;
	case H_4D750: { // back to its place
		no_actions();
		bool right = mem_[wCurObjId] == HIDDEN_FIGURE_RIGHT_HAND;
		uint16_t start = right ? w1d156 : w1d152;
		uint8_t e = mem_[start + 0], d = mem_[start + 1], c = mem_[start + 2], b = mem_[start + 3];
		if (mem_[wCurObjYPos + 0] == e && mem_[wCurObjYPos + 1] == d && mem_[wCurObjXPos + 0] == c && mem_[wCurObjXPos + 1] == b) {
			mem_[right ? w1d14d : w1d14c] = 0;
			rest(right);
			return;
		}
		// the x position through the page in h
		uint16_t page = wCurObjYPos & 0xff00;
		if (mem_[wCurObjYPos + 0] != e || mem_[wCurObjYPos + 1] != d) {
			SET_HL(gb_, wCurObjYPos + 1);
			move_up();
			page = static_cast<uint16_t>(gb_->cpu.h << 8);
		}
		uint8_t hi = mem_[page | (OBJ_X_POS + 1)], lo = mem_[page | OBJ_X_POS];
		if (hi != b || lo != c) {
			if (hi != b ? hi < b : lo < c)
				move_right();
			else
				move_left();
		}
		if (mem_[wGlobalCounter] & 0x03)
			flags() &= ~(1 << 4);
		else
			flags() |= 1 << 4;
		return;
	}
	case H_4D7DC:
		state() = 0x30;
		var3() = 0;
		return;
	case H_4D7E6:
		state() = 0x31;
		state_duration() = mem_[w1d15d] == 0x01 ? 0x01 : 0x46;
		flags() &= ~OBJFLAG_GRABBED;
		release();
		return;
	case H_4D7FD:
		release();
		return;
	case H_4D824: {
		play_sfx(SFX_VANISH);
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		bump(static_cast<uint8_t>(screen_x() + 0x2a) < wario ? 0x39 : 0x38);
		return;
	}
	case H_4D83A:
		bump(0x38);
		return;
	case H_4D844:
		bump(0x39);
		return;
	case H_4D859:
	case H_4D861:
		state() = handler == H_4D859 ? 0x44 : 0x45;
		var2() = 0x02;
		var3() = 0;
		return;
	case H_4D86F:
		state() = 0x46;
		return;
	case H_4D876:
		state() = 0x47;
		return;
	case H_4D87D:
	case H_4D88B:
		if (var2()) {
			routines_.call(handler == H_4D87D ? ADDR_Func_32ae : ADDR_Func_32bd);
			return;
		}
		state() = 0x0f;
		return;
	case H_4D899:
		thrown(0x36, 0xc0, ADDR_Func_3335);
		return;
	case H_4D8B1:
		thrown(0x37, 0xe0, ADDR_Func_3344);
		return;
	case H_4D8C9:
		carried(0x49);
		carry(false);
		return;
	case H_4D8D8:
		carry(false);
		return;
	case H_4D8E1:
		carried(0x48);
		carry(true);
		return;
	case H_4D8F0:
		carry(true);
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using wl3::level::HiddenFigure;

PORT_EX(HiddenFigureFunc, 0, ALL_REGISTERS) { HiddenFigure(gb).start(); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cb9e, 0, ALL_REGISTERS) { HiddenFigure(gb).wait_start(); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cbc4, 0, ALL_REGISTERS) { HiddenFigure(gb).main(); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cbe8, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_RELEASE); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cbf5, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_WAIT); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cc05, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_PICK); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cc3c, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_SHAKE_START); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cc87, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_SHAKE); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cce2, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_LEFT_START); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4ccf6, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_LEFT); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cd28, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_LEFT_WAIT); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cd45, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_RIGHT_START); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cd59, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_RIGHT); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cd8b, 0, ALL_REGISTERS) { HiddenFigure(gb).run(wl3::level::P_RIGHT_WAIT); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4cdbc, 0, ALL_REGISTERS) { HiddenFigure(gb).defeated(); }
PORT_LOCAL_EX(HiddenFigureFunc, Func_4ce01, 0, ALL_REGISTERS) { HiddenFigure(gb).crumble(); }
PORT_EX(Func_4ce39, 0, ALL_REGISTERS) { HiddenFigure(gb).face_laugh_start(static_cast<uint16_t>(gb->cpu.h << 8)); }
PORT_EX(Func_4cf70, 0, ALL_REGISTERS) { HiddenFigure(gb).shake_y(REG_BC(gb)); }
PORT_EX(Func_4cf87, 0, ALL_REGISTERS) { HiddenFigure(gb).shake_x(REG_BC(gb)); }
PORT_EX(Func_4cf9e, 0, ALL_REGISTERS) { HiddenFigure(gb).move_y(REG_BC(gb)); }
PORT_EX(Func_4cfbf, 0, ALL_REGISTERS) { HiddenFigure(gb).move_x(REG_BC(gb)); }
PORT_EX(Func_4cfe0, 0, ALL_REGISTERS) { HiddenFigure(gb).flash_pals(); }
PORT_EX(Func_4d005, 0, ALL_REGISTERS) { HiddenFigure(gb).set_pals(); }

PORT_EX(HiddenFigureFaceFunc, 0, ALL_REGISTERS) { HiddenFigure(gb).face(); }
PORT_LOCAL_EX(HiddenFigureFaceFunc, Func_4cf13, 0, ALL_REGISTERS) { HiddenFigure(gb).face_hurt(); }
PORT_LOCAL_EX(HiddenFigureFaceFunc, Func_4cf3f, 0, ALL_REGISTERS) { HiddenFigure(gb).face_defeated(); }
PORT_LOCAL_EX(HiddenFigureFaceFunc, Func_4cf55, 0, ALL_REGISTERS) { HiddenFigure(gb).face_laugh(); }

PORT_EX(HiddenFigureLeftHandFunc, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_start(false); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d0c7, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_rest(false); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d164, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_sting(false); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d19e, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_back(false); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d1b1, 0, ALL_REGISTERS) { HiddenFigure(gb).left_slam(); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d2c8, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_sweep(false); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d379, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_hold(); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d37f, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_defeated(false); }
PORT_LOCAL_EX(HiddenFigureLeftHandFunc, Func_4d39b, 0, ALL_REGISTERS) { HiddenFigure(gb).left_rise(); }

PORT_EX(HiddenFigureRightHandFunc, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_start(true); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d3c8, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_rest(true); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d465, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_sting(true); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d49f, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_back(true); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d4b2, 0, ALL_REGISTERS) { HiddenFigure(gb).right_slam(); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d571, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_sweep(true); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d622, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_hold(); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d628, 0, ALL_REGISTERS) { HiddenFigure(gb).hand_defeated(true); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d644, 0, ALL_REGISTERS) { HiddenFigure(gb).right_sink(); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d655, 0, ALL_REGISTERS) { HiddenFigure(gb).stunned(); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d73e, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D73E); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d750, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D750); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d7dc, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D7DC); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d7e6, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D7E6); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d7fd, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D7FD); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d824, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D824); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d83a, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D83A); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d844, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D844); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d859, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D859); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d861, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D861); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d86f, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D86F); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d876, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D876); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d87d, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D87D); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d88b, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D88B); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d899, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D899); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d8b1, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D8B1); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d8c9, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D8C9); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d8d8, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D8D8); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d8e1, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D8E1); }
PORT_LOCAL_EX(HiddenFigureRightHandFunc, Func_4d8f0, 0, ALL_REGISTERS) { HiddenFigure(gb).run_hand(wl3::level::H_4D8F0); }
