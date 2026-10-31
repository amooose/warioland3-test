// Yellow Belly (yellow_belly.asm), the boss swimming the room's water:
// its body floats up and down and shoots arrows at Wario; hit, it drops a
// pump that Wario can pump to blow the water down and leave it stranded.
// And its head (riding the body), the arrows (carrying the pump off), the
// pump and the platform. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t YB(uint16_t rom) { return static_cast<uint16_t>(ADDR_YellowBellyBodyFunc + (rom - 0x5356)); }
constexpr uint16_t YA(uint16_t offset) { return static_cast<uint16_t>(ADDR_YellowBellyArrowFunc + offset); }

constexpr uint16_t FADE_1 = YB(0x539d); // .Func_5139d
constexpr uint16_t FADE_2 = YB(0x53bf); // .Func_513bf
constexpr uint16_t FADE_3 = YB(0x53d9); // .Func_513d9
constexpr uint16_t MUSIC_START = YB(0x53fb); // .Func_513fb
constexpr uint16_t RISING = YB(0x5419); // .Func_51419
constexpr uint16_t SHOOTING = YB(0x54a4); // .Func_514a4
constexpr uint16_t STRANDED = YB(0x5572); // .Func_51572
constexpr uint16_t TO_PUMP = YB(0x55dd); // .Func_515dd
constexpr uint16_t SWIMMING = YB(0x5652); // .Func_51652
constexpr uint16_t SINKING = YB(0x578c); // .Func_5178c
constexpr uint16_t HURT = YB(0x57ee); // .Func_517ee
constexpr uint16_t DEFEATED = YB(0x584a); // .Func_5184a

constexpr uint16_t HEAD_UPDATE = ADDR_YellowBellyHeadFunc + 0x0d; // .Func_51b7b
constexpr uint16_t PLATFORM_UPDATE = ADDR_YellowBellyPlatformFunc + 0x27; // .Func_51c3f
constexpr uint16_t PUMP_FALLING = ADDR_PumpFunc + 0x0f; // .Func_518c6
constexpr uint16_t PUMP_LANDED = ADDR_PumpFunc + 0x90; // .Func_51947

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

uint16_t word_at(Memory mem, uint16_t addr) { return static_cast<uint16_t>(mem[addr] | mem[static_cast<uint16_t>(addr + 1)] << 8); }

class YellowBelly : public Object {
public:
	using Object::Object;

	void start(); // YellowBellyBodyFunc
	void fade(uint16_t then, uint16_t sfx, uint16_t pals); // .Func_5139d / .Func_513bf / .Func_513d9
	void music_start(); // .Func_513fb
	void rising(); // .Func_51419
	void aim(uint16_t page); // .asm_51442
	void shooting(); // .Func_514a4
	void swim_on(); // .asm_514eb
	void stranded_start(uint16_t page); // .asm_5154e
	void stranded(); // .Func_51572
	void to_pump(); // .Func_515dd
	void swimming(); // .Func_51652
	void swim(); // .asm_5168e
	void sinking(); // .Func_5178c
	void hit(); // .asm_5179a
	void hurt(); // .Func_517ee
	void defeated(); // .Func_5184a

private:
	void copy_pals(uint16_t pals);
	void turn_frameset(uint16_t frameset);
	void frameset_62(uint16_t frameset);
	void flash(uint16_t pals); // .asm_51812 and the like: by the duration's low nibble
};

class YellowBellyHelper : public Object {
public:
	using Object::Object;

	void arrow(bool left); // YellowBellyArrowFunc / .Right
	void arrow_flying(bool left); // .Func_51a7d / .Func_51ad9
	void arrow_off(bool right); // .Func_51aaa / .Func_51b34
	void arrow_vanish(); // .Func_51b16
	void arrow_actions(); // .Func_51b4b
	void head(); // YellowBellyHeadFunc
	void head_update(); // .Func_51b7b
	void platform(); // YellowBellyPlatformFunc
	void platform_update(); // .Func_51c3f
	void platform_done(); // Func_51c62
	void platform_vanish(); // Func_51c77
	void pump(); // PumpFunc
	void pump_falling(); // .Func_518c6
	void pump_landed(); // .Func_51947

private:
	void pump_carried(); // .asm_51a1d
};

void stop_boss_music(Memory mem)
{
	stop_music2(mem);
	mem[wBossState] = 0;
}

} // namespace

void YellowBelly::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(FADE_1);
	set_frameset(ADDR_Frameset_69780);
	state_duration() = 0x3c;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	substate() = (substate() & 0xf0) | 0x04;
	mem_[wCurObjCollBoxRight] = 0x07;
	mem_[wCurObjCollBoxLeft] = 0xf8;
	mem_[wCurObjCollBoxBottom] = 0x0a;
	mem_[wDollBoyActiveBarrels] = 0;
	mem_[w1d147] = 0;
	mem_[wDollBoyHammerRange] = 0;
	mem_[wIsBossBattle] = TRUE;
	stop_music2(mem_);
}

void YellowBelly::copy_pals(uint16_t pals)
{
	SET_HL(gb_, pals);
	SET_DE(gb_, wTempPals2 + 4 * PAL_SIZE);
	gb_->cpu.c = 4;
	gb_->cpu.b = 3;
	routines_.call(ADDR_CopyAndApplyOBPals);
}

// the palettes fading in
void YellowBelly::fade(uint16_t then, uint16_t sfx, uint16_t pals)
{
	no_actions();
	if (--state_duration())
		return;
	state_duration() = 0x1e;
	set_update(then);
	if (sfx)
		play_sfx(sfx);
	copy_pals(pals);
}

void YellowBelly::music_start()
{
	no_actions();
	if (--state_duration())
		return;
	state_duration() = 0x20;
	set_update(RISING);
	mem_[wBossState] = BOSS_REGULAR;
	routines_.call(ADDR_UpdateLevelMusic);
}

void YellowBelly::rising()
{
	no_actions();
	if (--state_duration()) {
		routines_.call(ADDR_MoveObjectUp_Slow);
		return;
	}
	state_duration() = static_cast<uint8_t>((mem_[rDIV] & 0x01) + 0x03);
	var1() = 0;
	var2() = 0x01;
	var3() = 0;
	set_update(SWIMMING);
	SET_BC(gb_, ADDR_ObjParams_YellowBellyHead);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

void YellowBelly::turn_frameset(uint16_t frameset)
{
	set_frameset(frameset);
	var1() = 0x44;
	substate() ^= OBJSUBFLAG_VDIR | OBJSUBFLAG_HDIR;
}

void YellowBelly::frameset_62(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = 0x62;
}

// faces Wario to shoot (VDIR: facing right)
void YellowBelly::aim(uint16_t page)
{
	mem_[page | (OBJ_UPDATE_FUNCTION + 1)] = static_cast<uint8_t>(SHOOTING >> 8);
	mem_[page | OBJ_UPDATE_FUNCTION] = static_cast<uint8_t>(SHOOTING);
	bool vdir = mem_[page | OBJ_SUBSTATE] & OBJSUBFLAG_VDIR;
	if (mem_[wDollBoyHammerRange]) {
		var1() = 0;
		frameset_62(vdir ? ADDR_Frameset_697a1 : ADDR_Frameset_69792);
		return;
	}
	uint16_t own = word_at(mem_, page | OBJ_X_POS);
	uint16_t wario = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
	if (wario >= own) {
		if (vdir)
			frameset_62(ADDR_Frameset_697a1);
		else
			turn_frameset(ADDR_Frameset_697b0);
	} else {
		if (!vdir)
			frameset_62(ADDR_Frameset_69792);
		else
			turn_frameset(ADDR_Frameset_697bb);
	}
}

// the arrow shot partway through
void YellowBelly::shooting()
{
	if (mem_[w1d147]) {
		hit();
		return;
	}
	bool vdir = substate() & OBJSUBFLAG_VDIR;
	if (var1()) {
		if (--var1())
			return;
		frameset_62(vdir ? ADDR_Frameset_697a1 : ADDR_Frameset_69792);
		return;
	}
	if (!--state_duration()) {
		swim_on();
		return;
	}
	if (state_duration() != 0x2f)
		return;
	play_sfx(SFX_07C);
	SET_BC(gb_, vdir ? ADDR_ObjParams_YellowBellyArrowRight : ADDR_ObjParams_YellowBellyArrowLeft);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// back to swimming, or to the pump when it's down
void YellowBelly::swim_on()
{
	bool vdir;
	if (!mem_[wDollBoyHammerRange]) {
		set_update(SWIMMING);
		flags() |= 1 << OBJFLAG_PRIORITY_F;
		state_duration() = static_cast<uint8_t>((mem_[rDIV] & 0x01) + 0x03);
		var2() = 0x01;
		var3() = 0x01;
		state() = 0;
		vdir = substate() & OBJSUBFLAG_VDIR;
		set_frameset(vdir ? ADDR_Frameset_697eb : ADDR_Frameset_697e6);
		return;
	}
	set_update(TO_PUMP);
	state_duration() = 0x03;
	var2() = 0x02;
	var3() = 0x02;
	state() = 0;
	vdir = substate() & OBJSUBFLAG_VDIR;
	set_frameset(vdir ? ADDR_Frameset_69789 : ADDR_Frameset_69780);
}

// the water's gone: stranded
void YellowBelly::stranded_start(uint16_t page)
{
	stop_sfx(mem_);
	mem_[wDollBoyHammerRange] = 0x01;
	mem_[page | (OBJ_UPDATE_FUNCTION + 1)] = static_cast<uint8_t>(STRANDED >> 8);
	mem_[page | OBJ_UPDATE_FUNCTION] = static_cast<uint8_t>(STRANDED);
	mem_[page | OBJ_FLAGS] &= ~(1 << OBJFLAG_PRIORITY_F);
	var1() = 0xa6;
	set_frameset(ADDR_Frameset_697c6);
}

void YellowBelly::stranded()
{
	no_actions();
	uint8_t count = var1();
	if (count == 0xa6) {
		play_sfx(SFX_VANISH);
		count = var1();
	} else if (count == 0x88) {
		play_sfx(SFX_0A9);
		count = var1();
	} else if (count == 0x25) {
		play_sfx(SFX_0A5);
		SET_BC(gb_, ADDR_ObjParams_Pump);
		routines_.call(ADDR_CreateObjectFromCurObjPos);
		count = var1();
	}
	if (count < 0x92 && screen_y() >= 0x39)
		move_up();
	if (!--var1()) {
		swim_on();
		return;
	}
	count = var1();
	if (count < 0x37)
		return;
	if (count == 0x37 || !(count & 0x0f)) {
		copy_pals(ADDR_Pals_51cb1);
		return;
	}
	if ((count & 0x0f) == 0x08)
		copy_pals(ADDR_Pals_51cc9);
}

// swims to the pump, $38 pixels away on its side, and aims from there
void YellowBelly::to_pump()
{
	if (mem_[wTransformation] == TRANSFORMATION_PUFFY_WARIO) {
		mem_[wDollBoyHammerRange] = 0;
		swim_on();
		return;
	}
	if (mem_[w1d147]) {
		hit();
		return;
	}
	if (!mem_[wDollBoyHammerRange]) {
		swim_on();
		return;
	}
	uint16_t pump_obj = find_active(mem_, PUMP);
	if (!pump_obj)
		return;
	uint16_t pump_x = word_at(mem_, pump_obj + OBJ_X_POS) & 0xfffe;
	uint16_t at = word_at(mem_, wCurObjXPos) & 0xfffe;
	at = static_cast<uint16_t>(facing_right() ? at + 0x38 : at - 0x38);
	if (pump_x == at) {
		aim(wCurObjSubState & 0xff00);
		return;
	}
	var2() = 0x02;
	swim();
}

void YellowBelly::swimming()
{
	if (mem_[wDollBoyActiveBarrels]) {
		no_actions();
		state() = 0;
		return;
	}
	if (mem_[w1d147]) {
		hit();
		return;
	}
	if (!var1() && var3() == 1 && screen_y() >= mem_[wWarioScreenYPos]) {
		set_update(SINKING);
		return;
	}
	SET_BC(gb_, ADDR_Data_60950);
	routines_.call(ADDR_ApplyObjYMovement_Loop);
	swim();
}

// back and forth (bit 7) bobbing; hit, it turns away from Wario
void YellowBelly::swim()
{
	uint8_t s = state();
	if (s) {
		if (s == 0x0a) {
			stranded_start(wCurObjState & 0xff00);
			return;
		}
		state() = 0;
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		uint8_t own = static_cast<uint8_t>(screen_x() + 0x2a);
		if (substate() & 0x80 ? wario >= own : own >= wario)
			substate() ^= 0x80;
		var2() = 0x02;
	}
	if (var1()) {
		if (--var1())
			return;
		if (!(mem_[wGlobalCounter] & 1) && state_duration() >= 0x02)
			--state_duration();
		bool vdir = substate() & OBJSUBFLAG_VDIR;
		if (mem_[wDollBoyHammerRange] != 1) {
			set_frameset(vdir ? ADDR_Frameset_697eb : ADDR_Frameset_697e6);
		} else {
			play_sfx(SFX_101);
			set_frameset(vdir ? ADDR_Frameset_69789 : ADDR_Frameset_69780);
		}
		return;
	}
	if (var3() == 1 && mem_[wTransformation] != TRANSFORMATION_PUFFY_WARIO && screen_y() < mem_[wWarioScreenYPos]) {
		if (!--state_duration()) {
			aim(wCurObjSubState & 0xff00);
			return;
		}
		play_sfx(SFX_0A8);
	}
	bool right = substate() & 0x80;
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x04);
	uint16_t x_pos = static_cast<uint16_t>(right ? x() + 0x18 : x() - 0x18);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (!gb_->cpu.a) {
		if (right)
			move_right_by_var2();
		else
			move_left_by_var2();
		return;
	}
	bool vdir = substate() & OBJSUBFLAG_VDIR;
	if (right ? vdir : !vdir)
		turn_frameset(right ? ADDR_Frameset_697bb : ADDR_Frameset_697b0);
	else
		substate() ^= 0x80;
	var2() = 0x01;
}

void YellowBelly::sinking()
{
	no_actions();
	if (screen_x() >= 0xc8)
		return;
	move_down();
}

// hurt, or beaten on the last hit
void YellowBelly::hit()
{
	mem_[w1d147] = 0;
	set_update(HURT);
	SET_HL(gb_, wCurObjSubState);
	routines_.call(ADDR_DecrementRemainingBossHits);
	if ((substate() & 0x0f) != 1) {
		set_frameset(ADDR_Frameset_697d5);
		state_duration() = 0x64;
		var1() = 0x01;
		return;
	}
	set_update(DEFEATED);
	set_frameset(ADDR_Frameset_69845);
	state_duration() = 0x65;
	var1() = 0xff;
	set_interaction(OBJ_INTERACTION_0B);
	play_sfx(SFX_0A9);
	stop_music2(mem_);
}

// the palettes flashing by the duration
void YellowBelly::flash(uint16_t pals)
{
	uint8_t low = state_duration() & 0x0f;
	if (!low)
		copy_pals(ADDR_Pals_51cb1);
	else if (low == 0x08)
		copy_pals(pals);
}

void YellowBelly::hurt()
{
	no_actions();
	if (state_duration()) {
		if (!--state_duration()) {
			play_sfx(SFX_0B9);
			copy_pals(ADDR_Pals_51cb1);
			return;
		}
		flash(ADDR_Pals_51ce1);
		return;
	}
	if (mem_[wCurObjFramesetOffset] != 0x10 || frame_duration())
		return;
	var1() = 0;
	swim_on();
	uint8_t hits = substate() & 0x0f;
	uint16_t oam;
	if (hits == 0x03)
		oam = ADDR_OAM_191940;
	else if (hits == 0x02)
		oam = ADDR_OAM_191f04;
	else
		return;
	mem_[wCurObjOAMPtr + 0] = static_cast<uint8_t>(oam);
	mem_[wCurObjOAMPtr + 1] = static_cast<uint8_t>(oam >> 8);
}

// flashing, then gone, and the platform comes
void YellowBelly::defeated()
{
	if (state_duration()) {
		if (!--state_duration()) {
			copy_pals(ADDR_Pals_51cb1);
			return;
		}
		flash(ADDR_Pals_51ce1);
		return;
	}
	if (mem_[wCurObjFramesetOffset] == 0x14 && !frame_duration())
		play_sfx(SFX_0AA);
	if (mem_[wCurObjFramesetOffset] != 0x20 || frame_duration())
		return;
	flags() = 0;
	uint16_t platform = find_active(mem_, YELLOW_BELLY_PLATFORM);
	if (!platform)
		return;
	mem_[platform + OBJ_STATE] = 0x2f;
	YellowBellyHelper(gb_).platform_done();
}

void YellowBellyHelper::arrow(bool left)
{
	set_update(YA(left ? 0x18 : 0x74));
	if (mem_[wDollBoyHammerRange]) {
		flags() |= OBJFLAG_GRABBED;
		var2() = 0x02;
	}
	arrow_flying(left);
}

// harmless unless above Wario and flying at him
void YellowBellyHelper::arrow_actions()
{
	if (screen_y() < mem_[wWarioScreenYPos]) {
		bool before = screen_x() < mem_[wWarioScreenXPos];
		if (facing_right() ? before : !before)
			return;
	}
	no_actions();
}

// down at an angle; hit back, up and off
void YellowBellyHelper::arrow_flying(bool left)
{
	arrow_actions();
	uint8_t s = state();
	if (s == 0x28 || s == 0x29) {
		set_update(YA(left ? 0x45 : 0xcf));
		var2() = 0x04;
		return;
	}
	if (s) {
		state_duration() = 0x1e;
		set_update(YA(0xb1));
		stop_boss_music(mem_);
		return;
	}
	move_down_by_var2();
	if (!mem_[wDollBoyHammerRange]) {
		if (left)
			move_left_by_var2();
		else
			move_right_by_var2();
	} else {
		if (left)
			move_left();
		else
			move_right();
	}
}

void YellowBellyHelper::arrow_off(bool right)
{
	no_actions();
	if (screen_y() < 0x40) {
		flags() = 0;
		return;
	}
	move_up_by_var2();
	routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
}

void YellowBellyHelper::arrow_vanish()
{
	no_actions();
	if (--state_duration())
		return;
	flags() = 0;
}

void YellowBellyHelper::head()
{
	flags() |= OBJFLAG_INVISIBLE;
	set_update(HEAD_UPDATE);
	head_update();
}

// rides the body, 8 pixels to its side
void YellowBellyHelper::head_update()
{
	if (mem_[wTransformation] == TRANSFORMATION_PUFFY_WARIO)
		no_actions();
	if (state()) {
		state() = 0;
		state_duration() = 0x1e;
		mem_[wDollBoyActiveBarrels] = 0x01;
		stop_boss_music(mem_);
	}
	if (state_duration() && !--state_duration())
		mem_[wDollBoyActiveBarrels] = 0;
	uint16_t body = find_active(mem_, YELLOW_BELLY_BODY);
	if (!body)
		return;
	uint16_t y_pos = static_cast<uint16_t>(word_at(mem_, body + OBJ_Y_POS) + 0x10);
	mem_[wCurObjYPos + 0] = static_cast<uint8_t>(y_pos);
	mem_[wCurObjYPos + 1] = static_cast<uint8_t>(y_pos >> 8);
	uint16_t body_x = word_at(mem_, body + OBJ_X_POS);
	uint8_t body_var1 = mem_[body + OBJ_VAR_1];
	if (body_var1) {
		if (body_var1 == 0xff) {
			flags() = 0;
			return;
		}
		no_actions();
	}
	uint16_t x_pos = static_cast<uint16_t>((mem_[body + OBJ_SUBSTATE] & OBJSUBFLAG_VDIR) ? body_x + 0x08 : body_x - 0x08);
	mem_[wCurObjXPos + 0] = static_cast<uint8_t>(x_pos);
	mem_[wCurObjXPos + 1] = static_cast<uint8_t>(x_pos >> 8);
}

void YellowBellyHelper::platform()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | (1 << 3);
	set_update(PLATFORM_UPDATE);
	set_frameset(ADDR_Frameset_69893);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	mem_[wCurObjCollBoxRight] = 0x07;
	mem_[wCurObjCollBoxLeft] = 0xf8;
	mem_[wCurObjCollBoxBottom] = 0xf6;
}

// once the boss is beaten ($2f): the battle's over
void YellowBellyHelper::platform_update()
{
	if (state() != 0x2f)
		return;
	set_frameset(ADDR_Frameset_69896);
	state_duration() = 0x5a;
	set_update(ADDR_Func_51c77);
	mem_[wBossState] = BOSS_DEFEATED;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[wIsBossBattle] = 0;
	platform_done();
}

void YellowBellyHelper::platform_done()
{
	SET_HL(gb_, wCurObjUnk02);
	gb_->cpu.e = 0x07;
	routines_.farcall(BANK_Func_ba42, ADDR_Func_ba42);
}

void YellowBellyHelper::platform_vanish()
{
	if (--state_duration())
		return;
	flags() = 0;
}

void YellowBellyHelper::pump()
{
	set_update(PUMP_FALLING);
	var1() = mem_[wGlobalCounter];
	pump_falling();
}

// falls out of the boss, sliding a little (var1 bit 0: right)
void YellowBellyHelper::pump_falling()
{
	no_actions();
	SET_BC(gb_, ADDR_Data_609e0);
	routines_.call(ADDR_ApplyObjYMovement);
	if (state_duration()) {
		--state_duration();
	} else {
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (gb_->cpu.a) {
			mem_[wCurObjYPos + 0] = mem_[hYPosLo];
			mem_[wCurObjYPos + 1] = mem_[hYPosHi];
			set_frameset(ADDR_Frameset_6987f);
			set_update(PUMP_LANDED);
			state() = 0;
			flags() &= ~(1 << OBJFLAG_PRIORITY_F);
			return;
		}
	}
	bool right = var1() & 1;
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x04);
	uint16_t x_pos = static_cast<uint16_t>(right ? x() + 0x05 : x() - 0x06);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return;
	routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
}

// pushed about ($31); pumped ($0a), it blows the water away; an arrow
// carries it off
void YellowBellyHelper::pump_landed()
{
	uint8_t s = state();
	if (s == 0x31) {
		if (!--state_duration()) {
			state() = 0;
			return;
		}
		bool right = substate() & 0x80;
		uint16_t y_pos = static_cast<uint16_t>(y() - 0x04);
		uint16_t x_pos = static_cast<uint16_t>(right ? x() + 0x05 : x() - 0x06);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
		mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
		routines_.call(ADDR_Func_3513);
		bool blocked = gb_->cpu.a;
		if (blocked) {
			if (right)
				substate() &= ~(1 << 7);
			else
				substate() |= 1 << 7;
		}
		SET_BC(gb_, right != blocked ? ADDR_Data_60a10 : ADDR_Data_60a00);
		routines_.call(ADDR_ApplyObjXMovement);
		return;
	}
	if (s == 0x0a) {
		state() = 0x3a;
		mem_[w1d147] = 0x3a;
		mem_[wDollBoyHammerRange] = 0;
		set_frameset(ADDR_Frameset_69882);
		state_duration() = 0x1d;
		var3() = 0;
		play_sfx(SFX_0BA);
		return;
	}
	if (s == 0x3a) {
		no_actions();
		if (state_duration()) {
			--state_duration();
			return;
		}
		SET_BC(gb_, ADDR_Data_60040);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	if (s == 0x58) {
		pump_carried();
		return;
	}
	s &= 0xfe;
	if (!s)
		return;
	if (s == 0x28) {
		state() = 0x58;
		mem_[wDollBoyHammerRange] = 0;
		pump_carried();
		return;
	}
	state() = 0x31;
	if (static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a))
		substate() &= ~OBJSUBFLAG_HDIR;
	else
		substate() |= OBJSUBFLAG_HDIR;
	var2() = 0;
	state_duration() = 0x3c;
	flags() &= ~OBJFLAG_NO_COLLISION;
}

void YellowBellyHelper::pump_carried()
{
	no_actions();
	if (screen_y() < 0x48) {
		flags() = 0;
		return;
	}
	uint16_t arrow = find_active(mem_, YELLOW_BELLY_ARROW_LEFT);
	if (!arrow)
		return;
	uint16_t y_pos = static_cast<uint16_t>(word_at(mem_, arrow + OBJ_Y_POS) + 0x08);
	mem_[wCurObjYPos + 0] = static_cast<uint8_t>(y_pos);
	mem_[wCurObjYPos + 1] = static_cast<uint8_t>(y_pos >> 8);
	mem_[wCurObjXPos + 0] = mem_[arrow + OBJ_X_POS];
	mem_[wCurObjXPos + 1] = mem_[arrow + OBJ_X_POS + 1];
}

} // namespace wl3::level

using wl3::level::YellowBelly;
using wl3::level::YellowBellyHelper;

PORT_EX(YellowBellyBodyFunc, 0, ALL_REGISTERS) { YellowBelly(gb).start(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_5139d, 0, ALL_REGISTERS) { YellowBelly(gb).fade(wl3::level::FADE_2, SFX_0CA, ADDR_Pals_51c81); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_513bf, 0, ALL_REGISTERS) { YellowBelly(gb).fade(wl3::level::FADE_3, 0, ADDR_Pals_51c99); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_513d9, 0, ALL_REGISTERS) { YellowBelly(gb).fade(wl3::level::MUSIC_START, 0, ADDR_Pals_51cb1); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_513fb, 0, ALL_REGISTERS) { YellowBelly(gb).music_start(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_51419, 0, ALL_REGISTERS) { YellowBelly(gb).rising(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, asm_51442, 0, ALL_REGISTERS) { YellowBelly(gb).aim(gb->cpu.h << 8); }
PORT_LOCAL_EX(YellowBellyBodyFunc, asm_514eb, 0, ALL_REGISTERS) { YellowBelly(gb).swim_on(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, asm_5179a, 0, ALL_REGISTERS) { YellowBelly(gb).hit(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_514a4, 0, ALL_REGISTERS) { YellowBelly(gb).shooting(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, asm_5154e, 0, ALL_REGISTERS) { YellowBelly(gb).stranded_start(gb->cpu.h << 8); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_51572, 0, ALL_REGISTERS) { YellowBelly(gb).stranded(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, asm_5168e, 0, ALL_REGISTERS) { YellowBelly(gb).swim(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_515dd, 0, ALL_REGISTERS) { YellowBelly(gb).to_pump(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_51652, 0, ALL_REGISTERS) { YellowBelly(gb).swimming(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_5178c, 0, ALL_REGISTERS) { YellowBelly(gb).sinking(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_517ee, 0, ALL_REGISTERS) { YellowBelly(gb).hurt(); }
PORT_LOCAL_EX(YellowBellyBodyFunc, Func_5184a, 0, ALL_REGISTERS) { YellowBelly(gb).defeated(); }

PORT_EX(YellowBellyArrowFunc, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow(true); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Right, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow(false); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Func_51a7d, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow_flying(true); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Func_51ad9, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow_flying(false); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Func_51aaa, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow_off(true); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Func_51b34, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow_off(false); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Func_51b16, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow_vanish(); }
PORT_LOCAL_EX(YellowBellyArrowFunc, Func_51b4b, 0, ALL_REGISTERS) { YellowBellyHelper(gb).arrow_actions(); }
PORT_EX(YellowBellyHeadFunc, 0, ALL_REGISTERS) { YellowBellyHelper(gb).head(); }
PORT_LOCAL_EX(YellowBellyHeadFunc, Func_51b7b, 0, ALL_REGISTERS) { YellowBellyHelper(gb).head_update(); }
PORT_EX(YellowBellyPlatformFunc, 0, ALL_REGISTERS) { YellowBellyHelper(gb).platform(); }
PORT_LOCAL_EX(YellowBellyPlatformFunc, Func_51c3f, 0, ALL_REGISTERS) { YellowBellyHelper(gb).platform_update(); }
PORT_EX(Func_51c62, 0, ALL_REGISTERS) { YellowBellyHelper(gb).platform_done(); }
PORT_EX(Func_51c77, 0, ALL_REGISTERS) { YellowBellyHelper(gb).platform_vanish(); }
PORT_EX(PumpFunc, 0, ALL_REGISTERS) { YellowBellyHelper(gb).pump(); }
PORT_LOCAL_EX(PumpFunc, Func_518c6, 0, ALL_REGISTERS) { YellowBellyHelper(gb).pump_falling(); }
PORT_LOCAL_EX(PumpFunc, Func_51947, 0, ALL_REGISTERS) { YellowBellyHelper(gb).pump_landed(); }
