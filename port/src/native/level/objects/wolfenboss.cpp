// Wolfenboss (wolfenboss.asm): a whisp rises out of the floor into smoke
// and the boss comes out of it, flying back and forth over Wario and
// attacking with magic spirals and igaguri (chestnuts that hatch kuri);
// a kuri thrown back up at him hurts him. And the magic spiral, the
// platform he leaves (opening when he's beaten), the kuri and the igaguri.
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t WB(uint16_t offset) { return static_cast<uint16_t>(ADDR_WolfenbossFunc + offset); }

constexpr uint16_t WHISP_WAITING = WB(0x3f); // .WhispWaiting
constexpr uint16_t WHISP_RISING = WB(0x76); // .WhispRising
constexpr uint16_t SMOKE_ENTRANCE = WB(0x9c); // .SmokeEntrance
constexpr uint16_t EMERGE_FROM_SMOKE = WB(0xd2); // .EmergeFromSmoke
constexpr uint16_t START_WAIT = WB(0xf3); // .StartWait
constexpr uint16_t SMOKE_EXIT = WB(0x111); // .SmokeExit
constexpr uint16_t DISAPPEAR = WB(0x120); // .Disappear
constexpr uint16_t MAIN = WB(0x165); // .Main
constexpr uint16_t DEFEAT_FALL = WB(0x35b); // .DefeatFall

constexpr uint16_t SPIRAL_GROW = ADDR_MagicSpiralFunc + 0x15; // .Grow
constexpr uint16_t SPIRAL_MOVE = ADDR_MagicSpiralFunc + 0x40; // .Move
constexpr uint16_t SPIRAL_DISAPPEAR = ADDR_MagicSpiralFunc + 0xb8; // .Disappear
constexpr uint16_t PLATFORM_WAIT_OPEN = ADDR_WolfenbossPlatformFunc + 0x27; // .WaitOpen
constexpr uint16_t PLATFORM_OPEN = ADDR_WolfenbossPlatformFunc + 0x41; // .Open
constexpr uint16_t PLATFORM_DISAPPEAR = ADDR_WolfenbossPlatformFunc + 0x56; // .Disappear
constexpr uint16_t KURI_LANDED = ADDR_KuriFunc + 0x52; // .Landed
constexpr uint16_t KURI_LAUNCHED = ADDR_KuriFunc + 0xe0; // .Launched
constexpr uint16_t IGAGURI_FALL = ADDR_IgaguriFunc + 0x63; // .Fall
constexpr uint16_t IGAGURI_BOUNCE = ADDR_IgaguriFunc + 0xf1; // .Bounce
constexpr uint16_t IGAGURI_HATCH = ADDR_IgaguriFunc + 0x149; // .Hatch

constexpr uint8_t SPIRAL_ATTACK_DURATION = 103;
constexpr uint8_t IGAGURI_ATTACK_DURATION = 72;

class Wolfenboss : public Object {
public:
	using Object::Object;

	void start(); // WolfenbossFunc
	void whisp_waiting(); // .WhispWaiting
	void whisp_rising(); // .WhispRising
	void smoke_entrance(); // .SmokeEntrance
	void emerge_from_smoke(); // .EmergeFromSmoke
	void start_wait(); // .StartWait
	void smoke_exit(); // .SmokeExit
	void disappear(); // .Disappear
	void main(); // .Main
	void defeat_fall(); // .DefeatFall

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	void start_movement();
	void start_attack(uint16_t frameset);
	void start_turning(uint16_t frameset);
	void wait_then_start_movement(uint16_t page);
	void do_movement();
	void defeated();
};

class WolfenbossHelper : public Object {
public:
	using Object::Object;

	void spiral(); // MagicSpiralFunc
	void spiral_grow(); // .Grow
	void spiral_move(); // .Move
	void spiral_disappear(); // .Disappear
	void platform(); // WolfenbossPlatformFunc
	void platform_wait_open(); // .WaitOpen
	void platform_open(); // .Open
	void platform_disappear(); // .Disappear
	void platform_done(); // Func_56326
	void kuri(); // KuriFunc
	void kuri_landed(); // .Landed
	void kuri_launched(); // .Launched
	void igaguri(); // IgaguriFunc
	void igaguri_fall(); // .Fall
	void igaguri_bounce(); // .Bounce
	void igaguri_hatch(); // .Hatch

private:
	void spiral_reflect(uint16_t substate_addr);
	void spiral_vanish();
	void kuri_vanish();
	void igaguri_move_x();
	void igaguri_skip_bounce();
	bool landed();
	void ground_shake();
};

} // namespace

void Wolfenboss::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(WHISP_WAITING);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() = (substate() & OBJSUBFLAGS_MASK) | (3 + 1);
	mem_[wCurObjCollBoxRight] = 2;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-3);
	mem_[wCurObjCollBoxBottom] = 1;
	set_frameset(ADDR_Frameset_69e97);
	state_duration() = 20;
	var2() = 0;
	mem_[wWolfenbossNextAttack] = 0;
	mem_[wWolfenbossIgaguriOrKuriActive] = 0;
	mem_[wWolfenbossRemainingHits] = 3 + 1;
	mem_[wWolfenbossConsecutiveSpirals] = 2;
}

void Wolfenboss::whisp_waiting()
{
	no_actions();
	if (!(flags() & (1 << OBJFLAG_ON_SCREEN_F)))
		return;
	if (--state_duration()) {
		SET_BC(gb_, ADDR_Data_60650);
		routines_.call(ADDR_ApplyObjXMovement_Loop);
		return;
	}
	set_update(WHISP_RISING);
	set_frameset(ADDR_Frameset_69ea0);
	state_duration() = 64;
	mem_[wIsBossBattle] = TRUE;
	stop_music2(mem_);
}

void Wolfenboss::whisp_rising()
{
	no_actions();
	if (--state_duration()) {
		move_up();
		return;
	}
	set_update(SMOKE_ENTRANCE);
	set_frameset(ADDR_Frameset_69ea9);
	state_duration() = 19;
	play_sfx(SFX_0AA);
}

void Wolfenboss::smoke_entrance()
{
	no_actions();
	if (--state_duration())
		return;
	set_update(EMERGE_FROM_SMOKE);
	mem_[wCurObjFramesetPtr + 0] = static_cast<uint8_t>(ADDR_Frameset_69e0e);
	mem_[wCurObjFramesetPtr + 1] = static_cast<uint8_t>(ADDR_Frameset_69e0e >> 8);
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = 2;
	set_coll_box(static_cast<uint8_t>(-8), 12, static_cast<uint8_t>(-8), 7);
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
}

void Wolfenboss::emerge_from_smoke()
{
	no_actions();
	if (frame()) {
		routines_.call(ADDR_MoveObjectUp_Slow);
		return;
	}
	set_frameset(ADDR_Frameset_69dcb);
	state_duration() = 40;
	var1() = 0;
	var2() = 0;
	set_update(START_WAIT);
}

void Wolfenboss::start_wait()
{
	no_actions();
	if (--state_duration())
		return;
	var1() = 0;
	var3() = 0;
	set_update(MAIN);
	set_frameset(ADDR_Frameset_69dec);
}

void Wolfenboss::smoke_exit()
{
	no_actions();
	if (--state_duration())
		return;
	flags() = 0;
}

void Wolfenboss::disappear()
{
	no_actions();
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_69ea9);
	state_duration() = 14;
	set_update(SMOKE_EXIT);
	play_sfx(SFX_0AA);
}

void Wolfenboss::start_movement()
{
	var1() = 0;
	set_frameset(facing_right() ? ADDR_Frameset_69de7 : ADDR_Frameset_69dec);
}

// a spiral, or an igaguri when there are none about and he's thrown his
// first two spirals
void Wolfenboss::start_attack(uint16_t frameset)
{
	set_frameset(frameset);
	bool spiral;
	uint8_t spirals = mem_[wWolfenbossConsecutiveSpirals];
	if (spirals) {
		mem_[wWolfenbossConsecutiveSpirals] = static_cast<uint8_t>(spirals - 1);
		spiral = true;
	} else {
		spiral = mem_[wWolfenbossIgaguriOrKuriActive];
	}
	if (!spiral)
		mem_[wWolfenbossIgaguriOrKuriActive] = TRUE;
	state_duration() = spiral ? SPIRAL_ATTACK_DURATION : IGAGURI_ATTACK_DURATION;
	var1() = 0x01;
	mem_[wWolfenbossNextAttack] = spiral ? 0x00 : 0x01;
}

void Wolfenboss::start_turning(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = 27;
	var1() = 0x03;
}

void Wolfenboss::defeated()
{
	set_update(DEFEAT_FALL);
	substate() &= OBJSUBFLAG_HDIR;
	var3() = 0;
	flags() |= 1 << OBJFLAG_PRIORITY_F;
	stop_music2(mem_);
	set_frameset(ADDR_Frameset_69e05);
}

void Wolfenboss::wait_then_start_movement(uint16_t page)
{
	if (dec_at(page | OBJ_STATE_DURATION))
		return;
	start_movement();
}

// to the wall, attacking when he's passed over Wario (at a global counter
// of 0)
void Wolfenboss::do_movement()
{
	if (!(mem_[wGlobalCounter] & 0x1f))
		play_sfx(SFX_0C3);
	SET_BC(gb_, ADDR_Data_604c0);
	routines_.call(ADDR_ApplyObjYMovement_Loop);
	bool right = facing_right();
	if (!mem_[wGlobalCounter]) {
		bool passed = right ? mem_[wWarioScreenXPos] >= screen_x() : screen_x() >= mem_[wWarioScreenXPos];
		if (passed) {
			start_attack(right ? ADDR_Frameset_69df6 : ADDR_Frameset_69df1);
			return;
		}
		var2() = 0x01;
	} else if (screen_x() != (right ? 0x98 : 0x18)) {
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
		return;
	}
	start_turning(right ? ADDR_Frameset_69dde : ADDR_Frameset_69dd5);
}

// the fight: hurt (state 8 or 9), then the move (var1)
void Wolfenboss::main()
{
	no_actions();
	if (mem_[wTransformation]) {
		set_frameset(facing_right() ? ADDR_Frameset_69e1f : ADDR_Frameset_69e0e);
		state_duration() = 70; // WOLFENBOSS_DISAPPEAR_DELAY
		set_update(DISAPPEAR);
		mem_[wBossState] = 0;
		return;
	}
	uint8_t s = state() & 0xfe;
	if (s) {
		if (s == 0x08) {
			state() = 0x38;
			set_frameset(facing(ADDR_Frameset_69e85, ADDR_Frameset_69e8e));
			state_duration() = 60; // WOLFENBOSS_HURT_DURATION
			if ((substate() & 0x0f) == 1)
				return;
			--substate();
			--mem_[wWolfenbossRemainingHits];
			return;
		}
		if (s == 0x38) {
			if (!--state_duration()) {
				state_duration() = 40; // WOLFENBOSS_AFTER_HURT_DELAY
				var1() = 0x04;
				flags() &= ~OBJFLAG_NO_COLLISION;
				state() = 0;
				set_frameset(facing_right() ? ADDR_Frameset_69e1f : ADDR_Frameset_69e0e);
				return;
			}
			uint8_t duration = state_duration();
			if (duration < 41)
				routines_.call(ADDR_MoveObjectDown_Slow);
			else if (duration == 41)
				play_sfx(SFX_0A9);
			return;
		}
		state() = 0;
	}
	switch (var1()) {
	case 0:
		do_movement();
		return;
	case 1: { // the attack thrown 16 frames in
		uint8_t next = mem_[wWolfenbossNextAttack];
		uint8_t at = next ? IGAGURI_ATTACK_DURATION - 16 : SPIRAL_ATTACK_DURATION - 16;
		if (state_duration() == at) {
			--state_duration();
			bool igaguri = next == 1;
			if (facing_right())
				SET_BC(gb_, igaguri ? ADDR_ObjParams_IgaguriRight : ADDR_ObjParams_MagicSpiralRight);
			else
				SET_BC(gb_, igaguri ? ADDR_ObjParams_IgaguriLeft : ADDR_ObjParams_MagicSpiralLeft);
			routines_.call(ADDR_CreateObjectAtRelativePos);
			return;
		}
		if (--state_duration())
			return;
		state_duration() = 19; // WOLFENBOSS_AFTER_ATTACK_DELAY
		var1() = 0x02;
		set_frameset(facing_right() ? ADDR_Frameset_69e00 : ADDR_Frameset_69dfb);
		return;
	}
	case 2:
		wait_then_start_movement(wCurObjStateDuration & 0xff00);
		return;
	case 3: // turning, then the attack he turned back for
		SET_BC(gb_, ADDR_Data_604c0);
		routines_.call(ADDR_ApplyObjYMovement_Loop);
		if (--state_duration())
			return;
		substate() ^= OBJSUBFLAG_HDIR;
		if (!var2()) {
			start_movement();
			return;
		}
		var2() = 0;
		start_attack(facing_right() ? ADDR_Frameset_69df6 : ADDR_Frameset_69df1);
		return;
	case 4: { // after hurt: rises; beaten on the last hit
		SET_HL(gb_, wCurObjVar1); // kept on the frames MoveObjectUp_Slow skips
		routines_.call(ADDR_MoveObjectUp_Slow);
		uint16_t page = static_cast<uint16_t>(gb_->cpu.h << 8);
		if (mem_[page | OBJ_FRAMESET_OFFSET] == 0x4 && !mem_[page | OBJ_FRAME_DURATION] && (substate() & 0x0f) == 1) {
			defeated();
			return;
		}
		wait_then_start_movement(page);
		return;
	}
	default:
		return;
	}
}

// then the platform opens
void Wolfenboss::defeat_fall()
{
	no_actions();
	if (screen_y() < 0xc0) {
		if (var3() == 0x2b)
			play_sfx(SFX_0A3);
		SET_BC(gb_, ADDR_Data_60da0);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	mem_[wBossState] = BOSS_DEFEATED;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = 0;
	routines_.call(ADDR_Func_56326);
	for (uint8_t obj = static_cast<uint8_t>(wObj1);; obj = static_cast<uint8_t>(obj + OBJ_STRUCT_LENGTH)) {
		uint16_t base = static_cast<uint16_t>((wObj1 & 0xff00) | obj);
		if (mem_[base + OBJ_ID] == WOLFENBOSS_PLATFORM && (mem_[base + OBJ_FLAGS] & 1)) {
			mem_[base + OBJ_STATE] = 0x2f;
			return;
		}
		if (obj == static_cast<uint8_t>(wObj8))
			return;
	}
}

void WolfenbossHelper::spiral()
{
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(SPIRAL_GROW);
	play_sfx(SFX_0C2);
	spiral_grow();
}

void WolfenbossHelper::spiral_grow()
{
	no_actions();
	if (!--state_duration()) {
		state_duration() = 8;
		set_update(SPIRAL_MOVE);
		return;
	}
	uint8_t duration = state_duration();
	if (duration == 69)
		set_frameset(ADDR_Frameset_69e39);
	else if (duration == 38)
		set_frameset(ADDR_Frameset_69e42);
}

void WolfenbossHelper::spiral_vanish()
{
	set_frameset(ADDR_Frameset_69eb2);
	state_duration() = 27;
	set_update(SPIRAL_DISAPPEAR);
	no_actions();
}

// a bounce off the screen's edges, counted down in the substate
void WolfenbossHelper::spiral_reflect(uint16_t substate_addr)
{
	if (!(dec_at(substate_addr) & 0x0f))
		spiral_vanish();
}

// diagonally, bouncing off the screen's edges
void WolfenbossHelper::spiral_move()
{
	if (state_duration() && !--state_duration())
		play_sfx(SFX_07C);
	if (mem_[wTransformation]) {
		spiral_vanish();
		return;
	}
	if (mem_[wWolfenbossRemainingHits] == 1) {
		flags() = 0;
		return;
	}
	uint16_t page = wCurObjSubState & 0xff00;
	if (!(substate() & OBJSUBFLAG_VDIR)) {
		if (screen_y() < 24) {
			substate() |= OBJSUBFLAG_VDIR;
			spiral_reflect(wCurObjSubState);
		} else {
			move_up();
			page = static_cast<uint16_t>(gb_->cpu.h << 8);
		}
	} else {
		if (screen_y() >= SCREEN_HEIGHT_PX) {
			substate() &= ~OBJSUBFLAG_VDIR;
			spiral_reflect(wCurObjSubState);
		} else {
			move_down();
			page = static_cast<uint16_t>(gb_->cpu.h << 8);
		}
	}
	Byte sub = mem_[page | OBJ_SUBSTATE];
	if (!(sub & OBJSUBFLAG_HDIR)) {
		if (screen_x() >= 16) {
			move_left();
			return;
		}
		sub |= OBJSUBFLAG_HDIR;
	} else {
		if (screen_x() < SCREEN_WIDTH_PX) {
			move_right();
			return;
		}
		sub &= ~OBJSUBFLAG_HDIR;
	}
	spiral_reflect(sub.addr());
}

void WolfenbossHelper::spiral_disappear()
{
	no_actions();
	if (--state_duration())
		return;
	flags() = 0;
}

void WolfenbossHelper::platform()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(PLATFORM_WAIT_OPEN);
	set_frameset(ADDR_Frameset_69e72);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = 8;
}

// until the boss is beaten (state $2f)
void WolfenbossHelper::platform_wait_open()
{
	frame_duration() = 2;
	if (state() != 0x2f)
		return;
	set_update(PLATFORM_OPEN);
	state_duration() = 80;
}

void WolfenbossHelper::platform_open()
{
	if (--state_duration())
		return;
	var2() = 0x02;
	var3() = 0;
	set_update(PLATFORM_DISAPPEAR);
}

void WolfenbossHelper::platform_disappear()
{
	if (flags() & (1 << OBJFLAG_ON_SCREEN_F)) {
		routines_.call(ADDR_Func_3326);
		return;
	}
	platform_done();
}

void WolfenbossHelper::platform_done()
{
	flags() = 0;
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x07;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
}

// out of the igaguri, down to the floor, then walking away from Wario
void WolfenbossHelper::kuri()
{
	no_actions();
	SET_BC(gb_, ADDR_Data_60d60);
	routines_.call(ADDR_ApplyObjYMovement);
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	set_update(KURI_LANDED);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a))
		substate() &= ~OBJSUBFLAG_HDIR;
	else
		substate() |= OBJSUBFLAG_HDIR;
}

void WolfenbossHelper::kuri_vanish()
{
	--mem_[wWolfenbossIgaguriOrKuriActive];
	routines_.call(ADDR_VanishObject2);
}

// back and forth over the floor; bumped, it turns; attacked, it's
// launched up
void WolfenbossHelper::kuri_landed()
{
	if (mem_[wTransformation])
		no_actions();
	uint8_t s = state();
	if (s) {
		bool turn;
		if (s == OBJSTATE_BUMP_LEFT_START) {
			turn = !(substate() & OBJSUBFLAG_HDIR);
		} else if (s == OBJSTATE_BUMP_RIGHT_START) {
			turn = substate() & OBJSUBFLAG_HDIR;
		} else if (s == OBJSTATE_VANISH_TOUCH) {
			kuri_vanish();
			return;
		} else if (s == OBJSTATE_ATTACKED_LEFT_START || s == OBJSTATE_ATTACKED_RIGHT_START) {
			if (s == OBJSTATE_ATTACKED_LEFT_START)
				substate() |= OBJSUBFLAG_HDIR;
			else
				substate() &= ~OBJSUBFLAG_HDIR;
			state() = 0;
			flags() |= OBJFLAG_GRABBED;
			var2() = 3;
			set_update(KURI_LAUNCHED);
			set_frameset(ADDR_Frameset_69e60);
			return;
		} else {
			turn = true;
		}
		if (turn)
			substate() ^= OBJSUBFLAG_HDIR;
		state() = 0;
	}
	if (!(flags() & (1 << OBJFLAG_ON_SCREEN_F))) {
		flags() = 0;
		return;
	}
	if (!mem_[wTransformation] && !(mem_[wGlobalCounter] & 0x07))
		play_sfx(SFX_06A);
	if (!facing_right()) {
		if (screen_x() >= 0x10) {
			move_left();
			return;
		}
		substate() |= OBJSUBFLAG_HDIR;
	} else {
		if (screen_x() < SCREEN_WIDTH_PX) {
			move_right();
			return;
		}
		substate() &= ~OBJSUBFLAG_HDIR;
	}
}

// up at the boss, bouncing off the screen's sides
void WolfenbossHelper::kuri_launched()
{
	no_actions();
	if ((state() & 0xfe) == 0x28) {
		kuri_vanish();
		return;
	}
	routines_.call(ADDR_MoveObjectUp_Fast);
	uint16_t flags_addr = static_cast<uint16_t>((gb_->cpu.h << 8) | OBJ_FLAGS);
	if (!(mem_[flags_addr] & (1 << OBJFLAG_ON_SCREEN_F))) {
		mem_[flags_addr] = 0;
		--mem_[wWolfenbossIgaguriOrKuriActive];
		return;
	}
	if (!facing_right()) {
		if (screen_x() >= 0x10) {
			move_left_by_var2();
			return;
		}
		substate() |= OBJSUBFLAG_HDIR;
	} else {
		if (screen_x() < SCREEN_WIDTH_PX) {
			move_right_by_var2();
			return;
		}
		substate() &= ~OBJSUBFLAG_HDIR;
	}
	if (screen_y() < 0x11)
		return;
	play_sfx(SFX_01A);
}

void WolfenbossHelper::igaguri()
{
	if (!(mem_[wGlobalCounter] & 0x07))
		play_sfx(SFX_06B);
	no_actions();
	if (--state_duration()) {
		uint8_t duration = state_duration();
		if (duration == 43)
			set_frameset(ADDR_Frameset_69e80);
		else if (duration == 24)
			set_frameset(ADDR_Frameset_69e4b);
		return;
	}
	set_update(IGAGURI_FALL);
	set_frameset(ADDR_Frameset_69e4b);
	state_duration() = 40;
	var1() = 0x03;
	if (mem_[wWolfenbossRemainingHits] >= 2 + 1) {
		play_sfx(SFX_101);
		var2() = 0x02;
	} else {
		play_sfx(SFX_067);
		var2() = 0x01;
	}
	var3() = 0;
}

void WolfenbossHelper::igaguri_move_x()
{
	if (!facing_right()) {
		if (screen_x() >= 0x10) {
			move_left_by_var2();
			return;
		}
		substate() |= OBJSUBFLAG_HDIR;
	} else {
		if (screen_x() < SCREEN_WIDTH_PX) {
			move_right_by_var2();
			return;
		}
		substate() &= ~OBJSUBFLAG_HDIR;
	}
}

void WolfenbossHelper::igaguri_skip_bounce()
{
	set_update(IGAGURI_HATCH);
	var1() = 0;
}

// on the floor: hYPos into the position
bool WolfenbossHelper::landed()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return false;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	return true;
}

void WolfenbossHelper::ground_shake()
{
	play_sfx(SFX_061);
	gb_->cpu.b = 24;
	routines_.call(ADDR_DoGroundShake);
}

// straight down early in the fight, else in an arc towards Wario, then
// bouncing
void WolfenbossHelper::igaguri_fall()
{
	if (mem_[wTransformation])
		no_actions();
	frame_duration() = 0x02;
	if (mem_[wWolfenbossRemainingHits] >= 2 + 1) {
		mem_[wCurObjYPos + 0] += 0x4;
		if (mem_[wCurObjYPos + 0] < 0x4)
			++mem_[wCurObjYPos + 1];
	} else {
		SET_BC(gb_, ADDR_Data_60d80);
		routines_.call(ADDR_ApplyObjYMovement);
	}
	if (!landed()) {
		igaguri_move_x();
		return;
	}
	ground_shake();
	if (mem_[wWolfenbossRemainingHits] >= 2 + 1) {
		igaguri_skip_bounce();
		return;
	}
	if (static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a))
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
	var3() = 0;
	set_update(IGAGURI_BOUNCE);
}

void WolfenbossHelper::igaguri_bounce()
{
	if (mem_[wTransformation])
		no_actions();
	frame_duration() = 0x2;
	SET_BC(gb_, ADDR_Data_60e00);
	routines_.call(ADDR_ApplyObjYMovement);
	if (!landed()) {
		igaguri_move_x();
		return;
	}
	if (!--var1()) {
		igaguri_skip_bounce();
		return;
	}
	var3() = 0;
	ground_shake();
}

// cracks open, and a kuri comes out
void WolfenbossHelper::igaguri_hatch()
{
	if (mem_[wTransformation])
		no_actions();
	switch (var1()) {
	case 0:
		frame_duration() = 0x2;
		if (--state_duration())
			return;
		state_duration() = 60;
		++var1();
		return;
	case 1:
		if (!(mem_[wGlobalCounter] & 0x1f))
			play_sfx(SFX_0C5);
		if (--state_duration())
			return;
		state_duration() = 14;
		++var1();
		set_frameset(ADDR_Frameset_69e59);
		return;
	case 2:
		if (!--state_duration()) {
			flags() = 0;
			return;
		}
		if (state_duration() != 5)
			return;
		play_sfx(SFX_0C4);
		SET_BC(gb_, ADDR_ObjParams_Kuri);
		routines_.call(ADDR_CreateObjectFromCurObjPos);
		return;
	default:
		return;
	}
}

} // namespace wl3::level

using wl3::level::Wolfenboss;
using wl3::level::WolfenbossHelper;

PORT_EX(WolfenbossFunc, 0, ALL_REGISTERS) { Wolfenboss(gb).start(); }
PORT_LOCAL_EX(WolfenbossFunc, WhispWaiting, 0, ALL_REGISTERS) { Wolfenboss(gb).whisp_waiting(); }
PORT_LOCAL_EX(WolfenbossFunc, WhispRising, 0, ALL_REGISTERS) { Wolfenboss(gb).whisp_rising(); }
PORT_LOCAL_EX(WolfenbossFunc, SmokeEntrance, 0, ALL_REGISTERS) { Wolfenboss(gb).smoke_entrance(); }
PORT_LOCAL_EX(WolfenbossFunc, EmergeFromSmoke, 0, ALL_REGISTERS) { Wolfenboss(gb).emerge_from_smoke(); }
PORT_LOCAL_EX(WolfenbossFunc, StartWait, 0, ALL_REGISTERS) { Wolfenboss(gb).start_wait(); }
PORT_LOCAL_EX(WolfenbossFunc, SmokeExit, 0, ALL_REGISTERS) { Wolfenboss(gb).smoke_exit(); }
PORT_LOCAL_EX(WolfenbossFunc, Disappear, 0, ALL_REGISTERS) { Wolfenboss(gb).disappear(); }
PORT_LOCAL_EX(WolfenbossFunc, Main, 0, ALL_REGISTERS) { Wolfenboss(gb).main(); }
PORT_LOCAL_EX(WolfenbossFunc, DefeatFall, 0, ALL_REGISTERS) { Wolfenboss(gb).defeat_fall(); }

PORT_EX(MagicSpiralFunc, 0, ALL_REGISTERS) { WolfenbossHelper(gb).spiral(); }
PORT_LOCAL_EX(MagicSpiralFunc, Grow, 0, ALL_REGISTERS) { WolfenbossHelper(gb).spiral_grow(); }
PORT_LOCAL_EX(MagicSpiralFunc, Move, 0, ALL_REGISTERS) { WolfenbossHelper(gb).spiral_move(); }
PORT_LOCAL_EX(MagicSpiralFunc, Disappear, 0, ALL_REGISTERS) { WolfenbossHelper(gb).spiral_disappear(); }
PORT_EX(WolfenbossPlatformFunc, 0, ALL_REGISTERS) { WolfenbossHelper(gb).platform(); }
PORT_LOCAL_EX(WolfenbossPlatformFunc, WaitOpen, 0, ALL_REGISTERS) { WolfenbossHelper(gb).platform_wait_open(); }
PORT_LOCAL_EX(WolfenbossPlatformFunc, Open, 0, ALL_REGISTERS) { WolfenbossHelper(gb).platform_open(); }
PORT_LOCAL_EX(WolfenbossPlatformFunc, Disappear, 0, ALL_REGISTERS) { WolfenbossHelper(gb).platform_disappear(); }
PORT_EX(Func_56326, 0, ALL_REGISTERS) { WolfenbossHelper(gb).platform_done(); }
PORT_EX(KuriFunc, 0, ALL_REGISTERS) { WolfenbossHelper(gb).kuri(); }
PORT_LOCAL_EX(KuriFunc, Landed, 0, ALL_REGISTERS) { WolfenbossHelper(gb).kuri_landed(); }
PORT_LOCAL_EX(KuriFunc, Launched, 0, ALL_REGISTERS) { WolfenbossHelper(gb).kuri_launched(); }
PORT_EX(IgaguriFunc, 0, ALL_REGISTERS) { WolfenbossHelper(gb).igaguri(); }
PORT_LOCAL_EX(IgaguriFunc, Fall, 0, ALL_REGISTERS) { WolfenbossHelper(gb).igaguri_fall(); }
PORT_LOCAL_EX(IgaguriFunc, Bounce, 0, ALL_REGISTERS) { WolfenbossHelper(gb).igaguri_bounce(); }
PORT_LOCAL_EX(IgaguriFunc, Hatch, 0, ALL_REGISTERS) { WolfenbossHelper(gb).igaguri_hatch(); }
