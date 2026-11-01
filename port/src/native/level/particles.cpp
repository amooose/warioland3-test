// Particles (src/engine/level/particles.asm): the 4 particle slots (block
// debris, dust, splashes, bubbles, the owl's feathers, the magic sparkles
// and so on): creating one, and each frame running each slot's particle
// function, which animates it and draws its sprite.
//
// The routines pass the particle in bc and leave registers their callers
// see (UpdateParticles returns with the last function's), so each keeps
// the original's registers and flags.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t FUNC_TABLE = ADDR_UpdateParticles + 0x32; // .DoParticleFunc's table

// what a particle function does before, at the start and after its
// animation
enum class Pre { ALWAYS, BOUNCY, MAGIC };
enum class Init { IN_PLACE, WARIO_POS, SLIDE_RIGHT, SLIDE_LEFT, DRIFT_UP };
enum class End { ANIM, NEVER, SLIDING, OFF_SCREEN };

struct ParticleFunc {
	Pre pre;
	Init init;
	uint16_t anim;
	End end;
	uint16_t sfx;
};

class Particles {
public:
	explicit Particles(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void create(); // CreateParticle
	void block_pos(); // Func_ce3e
	void update(); // UpdateParticles
	void run(const ParticleFunc &f);
	void update_animation(); // UpdateParticleAnimation
	void anim(uint16_t frameset, uint16_t y_dest, uint16_t oam); // ParticleAnim_*
	uint16_t by_direction(uint16_t left, uint16_t right);
	void load_sprite(); // LoadParticleSprite

private:
	void inc_c(int n = 1);
	void deinit();
	void hpos_to_particle();
	void ld_cp(uint16_t addr, uint8_t v);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// b = the PARTICLE_* constant: a particle at hYPos / hXPos, in the next
// of the 4 slots
void Particles::create()
{
	uint8_t index = mem_[wCurParticleIndex];
	uint8_t offset = static_cast<uint8_t>(index * 16);
	SET_DE(gb_, offset);
	mem_[wCurParticleIndex] = (index + 1) & 0x03;
	uint16_t slot = static_cast<uint16_t>(wParticles + offset);
	if (!mem_[slot])
		++mem_[wNumParticles]; // a free slot: one more particle
	mem_[slot] = cpu_.b; // PARTICLE_STRUCT_ID
	uint16_t at = static_cast<uint16_t>((slot & 0xff00) | static_cast<uint8_t>(slot + 1)); // inc l
	mem_[at++] = FALSE; // PARTICLE_STRUCT_INITIALISED
	mem_[at++] = mem_[hYPosHi];
	mem_[at++] = mem_[hYPosLo];
	mem_[at++] = mem_[hXPosHi];
	cpu_.a = mem_[hXPosLo];
	mem_[at++] = cpu_.a;
	SET_HL(gb_, at);
	cpu_.f = FLAG_Z; // xor a
}

// hl = a block in the level map: hYPos / hXPos = its position
void Particles::block_pos()
{
	uint8_t h = static_cast<uint8_t>(cpu_.h - 0xa0);
	cpu_.h = h;
	cpu_.d = h >> 4;
	uint8_t y_hi = static_cast<uint8_t>((mem_[wBlockPtrBank] - 1) * 2 + cpu_.d);
	unsigned y_lo = ((h & 0x0f) << 4) + 0x10;
	mem_[hYPosLo] = static_cast<uint8_t>(y_lo);
	mem_[hYPosHi] = static_cast<uint8_t>(y_hi + (y_lo > 0xff));
	uint8_t x_hi = cpu_.l >> 4;
	unsigned x_lo = ((cpu_.l & 0x0f) << 4) + 0x08;
	bool carry = x_lo > 0xff;
	mem_[hXPosLo] = static_cast<uint8_t>(x_lo);
	cpu_.a = static_cast<uint8_t>(x_hi + carry);
	cpu_.f = flags::add(x_hi, 0, carry);
	mem_[hXPosHi] = cpu_.a;
}

// runs the function of every slot in use
void Particles::update()
{
	uint8_t count = mem_[wNumParticles];
	cpu_.a = count;
	cpu_.f = flags::and_(count);
	if (!count)
		return;
	for (int n = 0; n < MAX_NUM_PARTICLES; n++) {
		uint16_t slot = static_cast<uint16_t>(wParticle0 + n * (wParticle1 - wParticle0));
		SET_HL(gb_, slot);
		uint8_t id = mem_[slot];
		cpu_.a = id;
		cpu_.f = flags::and_(id);
		if (!id)
			continue;
		// .DoParticleFunc: bc = the particle, then the jumptable's
		// registers: de = 2 * id, hl and a the function's address
		SET_BC(gb_, slot);
		if (id >= NUM_PARTICLES) {
			if (gb_->test.active)
				gb_->test.unrealistic = true;
			return;
		}
		uint8_t twice = static_cast<uint8_t>(id * 2);
		SET_DE(gb_, twice);
		unsigned sum = FUNC_TABLE + twice;
		cpu_.f = static_cast<uint8_t>(flags::z(twice) | ((FUNC_TABLE & 0xfff) + twice > 0xfff ? FLAG_H : 0)
			| (sum > 0xffff ? FLAG_C : 0));
		uint16_t func = Word(gb_, static_cast<uint16_t>(sum));
		SET_HL(gb_, func);
		cpu_.a = static_cast<uint8_t>(func);
		routines_.call(func);
	}
}

void Particles::inc_c(int n)
{
	while (n--) {
		cpu_.f = flags::inc(cpu_.f, cpu_.c);
		cpu_.c++;
	}
}

// ld a, [addr] / cp v
void Particles::ld_cp(uint16_t addr, uint8_t v)
{
	cpu_.a = mem_[addr];
	cpu_.f = flags::cp(cpu_.a, v);
}

// xor a / ld [bc], a / ld hl, wNumParticles / dec [hl]
void Particles::deinit()
{
	mem_[REG_BC(gb_)] = 0;
	cpu_.a = 0;
	SET_HL(gb_, wNumParticles);
	Byte count = mem_[wNumParticles];
	cpu_.f = flags::dec(FLAG_Z, count);
	--count;
}

// inc c / ldh a, [hPos + i] / ld [bc], a for the 4 bytes of hPos, inc c
void Particles::hpos_to_particle()
{
	for (int i = 0; i < 4; i++) {
		inc_c();
		cpu_.a = mem_[hYPosHi + i];
		mem_[REG_BC(gb_)] = cpu_.a;
	}
	inc_c();
}

// bc = the particle: starts it (or moves it along), animates it, and ends
// it when it's done
void Particles::run(const ParticleFunc &f)
{
	if (f.pre != Pre::ALWAYS) {
		ld_cp(wWarioState, f.pre == Pre::BOUNCY ? WST_BOUNCY_AIRBORNE : WST_MAGIC_STOPPING);
		bool equal = cpu_.f & FLAG_Z;
		if (f.pre == Pre::BOUNCY ? !equal : equal) {
			deinit();
			return;
		}
	}
	uint16_t particle = REG_BC(gb_);
	inc_c();
	cpu_.a = mem_[REG_BC(gb_)]; // PARTICLE_STRUCT_INITIALISED
	cpu_.f = flags::and_(cpu_.a);
	if (!cpu_.a) {
		cpu_.f = flags::inc(cpu_.f, 0);
		cpu_.a = 1;
		mem_[REG_BC(gb_)] = 1;
		inc_c(5);
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		mem_[REG_BC(gb_)] = 0; // PARTICLE_STRUCT_DURATION
		inc_c();
		mem_[REG_BC(gb_)] = 0; // PARTICLE_STRUCT_FRAMESET_OFFSET
		if (f.sfx) {
			play_sfx(mem_, f.sfx);
			cpu_.a = static_cast<uint8_t>(f.sfx);
		}
		cpu_.f = flags::dec(cpu_.f, cpu_.c);
		cpu_.c--;
	} else {
		switch (f.init) {
		case Init::IN_PLACE:
			inc_c(5);
			break;
		case Init::WARIO_POS:
			for (int i = 0; i < 4; i++)
				mem_[hYPosHi + i] = mem_[wWarioPos + i];
			cpu_.a = mem_[wWarioPos + 3];
			SET_HL(gb_, wWarioPos + 3);
			SET_DE(gb_, hYPosHi + 3);
			hpos_to_particle();
			break;
		case Init::SLIDE_RIGHT:
		case Init::SLIDE_LEFT: {
			// 12 pixels behind Wario, and 12 up
			bool right = f.init == Init::SLIDE_RIGHT;
			uint16_t x = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
			x = static_cast<uint16_t>(right ? x - 0x0c : x + 0x0c);
			mem_[hXPosLo] = static_cast<uint8_t>(x);
			mem_[hXPosHi] = static_cast<uint8_t>(x >> 8);
			uint16_t y = static_cast<uint16_t>((mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]) - 0x0c);
			mem_[hYPosLo] = static_cast<uint8_t>(y);
			mem_[hYPosHi] = static_cast<uint8_t>(y >> 8);
			cpu_.a = static_cast<uint8_t>(y >> 8);
			SET_HL(gb_, wWarioXPos + 1 - 3); // after three ld a, [hld]
			SET_DE(gb_, hXPosLo - 3);
			hpos_to_particle();
			break;
		}
		case Init::DRIFT_UP: { // drifts up 2 pixels
			inc_c(2);
			uint16_t lo = REG_BC(gb_);
			uint8_t y_lo = mem_[lo];
			mem_[lo] = static_cast<uint8_t>(y_lo - 2);
			cpu_.f = flags::dec(cpu_.f, cpu_.c);
			cpu_.c--;
			uint16_t hi = REG_BC(gb_);
			cpu_.a = static_cast<uint8_t>(mem_[hi] - (y_lo < 2));
			mem_[hi] = cpu_.a;
			inc_c(4);
			break;
		}
		}
	}
	routines_.call(f.anim);
	SET_BC(gb_, particle);
	switch (f.end) {
	case End::NEVER:
		return;
	case End::ANIM:
		cpu_.a = mem_[wCurParticleAnimEnded];
		cpu_.f = flags::and_(cpu_.a);
		if (!cpu_.a)
			return;
		break;
	case End::SLIDING:
		ld_cp(wWarioState, WST_SLIDING);
		if (cpu_.f & FLAG_Z)
			return;
		break;
	case End::OFF_SCREEN: // until off the bottom of the screen
		ld_cp(wCurSpriteYCoord, 0xc0);
		if (cpu_.f & FLAG_C)
			return;
		break;
	}
	deinit();
}

// hl = the particle's frame duration, then its frameset offset and frame;
// de = its frameset
void Particles::update_animation()
{
	mem_[wCurParticleAnimEnded] = FALSE;
	uint16_t at = REG_HL(gb_);
	uint8_t duration = mem_[at];
	cpu_.a = static_cast<uint8_t>(duration - 1);
	cpu_.f = flags::sub(duration, 1);
	mem_[at] = cpu_.a;
	SET_HL(gb_, at + 2);
	if (duration) // .no_new_frame
		return;

	uint8_t offset = mem_[at + 1];
	unsigned entry = offset + cpu_.e;
	cpu_.c = static_cast<uint8_t>(entry);
	cpu_.b = static_cast<uint8_t>(cpu_.d + (entry > 0xff));
	uint8_t frame = mem_[REG_BC(gb_)];
	if (frame == 0xff) { // .reset_anim
		mem_[at + 1] = 0; // frameset offset
		mem_[at] = 0; // duration
		mem_[wCurParticleAnimEnded] = TRUE;
		cpu_.a = TRUE;
		cpu_.f = FLAG_Z;
		return;
	}
	mem_[at + 2] = frame;
	// the offset is read again after the frame's written (they can alias)
	uint8_t again = mem_[at + 1];
	cpu_.f = flags::add(again, 2);
	mem_[at + 1] = static_cast<uint8_t>(again + 2);
	SET_BC(gb_, REG_BC(gb_) + 1);
	cpu_.a = mem_[REG_BC(gb_)];
	mem_[at] = cpu_.a; // duration
}

// the frameset for Wario's direction (and a's flags)
uint16_t Particles::by_direction(uint16_t left, uint16_t right)
{
	cpu_.a = mem_[wDirection];
	cpu_.f = flags::and_(cpu_.a);
	return cpu_.a ? right : left;
}

// bc = the particle's frame duration: animates it with the frameset and
// draws it with the OAM data
void Particles::anim(uint16_t frameset, uint16_t y_dest, uint16_t oam)
{
	SET_HL(gb_, REG_BC(gb_));
	SET_DE(gb_, frameset);
	update_animation();
	// ld a, [hld] (the frame) / dec l / dec l / ld a, [hld] (x) / dec l /
	// ld a, [hld] (y): hld is 16-bit, dec l 8-bit
	auto dec_l = [](uint16_t hl) { return static_cast<uint16_t>((hl & 0xff00) | static_cast<uint8_t>(hl - 1)); };
	uint16_t hl = REG_HL(gb_);
	mem_[wCurParticleFrame] = mem_[hl--];
	hl = dec_l(dec_l(hl));
	mem_[wCurParticleXCoord] = mem_[hl--];
	hl = dec_l(hl);
	mem_[y_dest] = mem_[hl];
	mem_[wCurParticleAttributes] = 0;
	cpu_.a = 0;
	cpu_.f = FLAG_Z;
	SET_HL(gb_, oam);
	load_sprite();
}

// the particle's sprite to virtual OAM, relative to the camera; hl = the
// OAM data
void Particles::load_sprite()
{
	uint16_t scy = wSCY, scx = wSCX;
	if (mem_[wCameraConfigFlags] == (CAM_XSCROLL2 | CAM_TRANSITIONS | CAM_BORDER_RIGHT | CAM_BORDER_LEFT)) {
		scy = wCameraSCY + 1;
		scx = wCameraSCX + 1;
	}
	uint8_t y = static_cast<uint8_t>(mem_[wCurParticleYCoord] + OAM_Y_OFS);
	mem_[wCurSpriteYCoord] = static_cast<uint8_t>(y - mem_[scy]);
	cpu_.b = mem_[scx];
	uint8_t x = static_cast<uint8_t>(mem_[wCurParticleXCoord] + OAM_X_OFS);
	cpu_.f = flags::sub(x, cpu_.b);
	mem_[wCurSpriteXCoord] = static_cast<uint8_t>(x - cpu_.b);
	mem_[wCurSpriteFrame] = mem_[wCurParticleFrame];
	cpu_.a = mem_[wCurParticleAttributes];
	mem_[wCurSpriteAttributes] = cpu_.a;
	routines_.call(ADDR_LoadSprite);
}

constexpr ParticleFunc DEBRIS{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Debris, End::ANIM, SFX_BLOCK_BREAK};
constexpr ParticleFunc STARS{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Stars, End::ANIM, 0};
constexpr ParticleFunc SPLASH{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Splash, End::ANIM, 0};
constexpr ParticleFunc DUST{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Dust, End::ANIM, 0};
constexpr ParticleFunc SLIDE_DUST_RIGHT{Pre::ALWAYS, Init::SLIDE_RIGHT, ADDR_ParticleAnim_Dust, End::SLIDING, 0};
constexpr ParticleFunc SLIDE_DUST_LEFT{Pre::ALWAYS, Init::SLIDE_LEFT, ADDR_ParticleAnim_Dust, End::SLIDING, 0};
constexpr ParticleFunc SMOKE_LARGE{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_SmokeLarge, End::ANIM, 0};
constexpr ParticleFunc SMOKE_SMALL{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_SmokeSmall, End::ANIM, 0};
constexpr ParticleFunc BUBBLES{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Bubbles, End::ANIM, 0};
constexpr ParticleFunc BOUNCY_JUMP{Pre::BOUNCY, Init::WARIO_POS, ADDR_ParticleAnim_BouncyJump, End::ANIM, 0};
constexpr ParticleFunc BUBBLE_BURST{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_BubbleBurst, End::ANIM, 0};
constexpr ParticleFunc ICE{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Ice, End::ANIM, 0};
constexpr ParticleFunc TREASURE_GLOW{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_TreasureGlow, End::NEVER, 0};
constexpr ParticleFunc OWL_PARTICLE{Pre::ALWAYS, Init::DRIFT_UP, ADDR_ParticleAnim_Owl, End::OFF_SCREEN, 0};
constexpr ParticleFunc SNOW{Pre::ALWAYS, Init::IN_PLACE, ADDR_ParticleAnim_Snow, End::ANIM, 0};
constexpr ParticleFunc MAGIC_1{Pre::MAGIC, Init::WARIO_POS, ADDR_ParticleAnim_Magic1, End::NEVER, 0};
constexpr ParticleFunc MAGIC_2{Pre::MAGIC, Init::WARIO_POS, ADDR_ParticleAnim_Magic2, End::NEVER, 0};
constexpr ParticleFunc MAGIC_STOP{Pre::ALWAYS, Init::WARIO_POS, ADDR_ParticleAnim_MagicStop, End::ANIM, 0};

} // namespace

} // namespace wl3::level

using wl3::level::Particles;

PORT(CreateParticle) { Particles(gb).create(); }
PORT(Func_ce3e) { Particles(gb).block_pos(); }

// b = the PARTICLE_* constant, hl = a block: a particle there
PORT(CreateParticleInBlock)
{
	Particles particles(gb);
	particles.block_pos();
	particles.create();
}

PORT(UpdateParticles) { Particles(gb).update(); }
PORT(UpdateParticleAnimation) { Particles(gb).update_animation(); }
PORT(LoadParticleSprite) { Particles(gb).load_sprite(); }

PORT(ParticleFunc_None) {}
PORT(ParticleFunc_Debris) { Particles(gb).run(wl3::level::DEBRIS); }
PORT(ParticleFunc_Stars) { Particles(gb).run(wl3::level::STARS); }
PORT(ParticleFunc_Splash) { Particles(gb).run(wl3::level::SPLASH); }
PORT(ParticleFunc_Dust) { Particles(gb).run(wl3::level::DUST); }
PORT(ParticleFunc_SlideDustRight) { Particles(gb).run(wl3::level::SLIDE_DUST_RIGHT); }
PORT(ParticleFunc_SlideDustLeft) { Particles(gb).run(wl3::level::SLIDE_DUST_LEFT); }
PORT(ParticleFunc_SmokeLarge) { Particles(gb).run(wl3::level::SMOKE_LARGE); }
PORT(ParticleFunc_SmokeSmall) { Particles(gb).run(wl3::level::SMOKE_SMALL); }
PORT(ParticleFunc_Bubbles) { Particles(gb).run(wl3::level::BUBBLES); }
PORT(ParticleFunc_BouncyJump) { Particles(gb).run(wl3::level::BOUNCY_JUMP); }
PORT(ParticleFunc_BubbleBurst) { Particles(gb).run(wl3::level::BUBBLE_BURST); }
PORT(ParticleFunc_Ice) { Particles(gb).run(wl3::level::ICE); }
PORT(ParticleFunc_TreasureGlow) { Particles(gb).run(wl3::level::TREASURE_GLOW); }
PORT(ParticleFunc_Owl) { Particles(gb).run(wl3::level::OWL_PARTICLE); }
PORT(ParticleFunc_Snow) { Particles(gb).run(wl3::level::SNOW); }
PORT(ParticleFunc_Magic1) { Particles(gb).run(wl3::level::MAGIC_1); }
PORT(ParticleFunc_Magic2) { Particles(gb).run(wl3::level::MAGIC_2); }
PORT(ParticleFunc_MagicStop) { Particles(gb).run(wl3::level::MAGIC_STOP); }

// bc = the particle's frame duration; they end in LoadParticleSprite's
// LoadSprite
PORT(ParticleAnim_Bubbles)
{
	Particles p(gb);
	p.anim(p.by_direction(ADDR_Frameset_d502, ADDR_Frameset_d50b), wCurParticleYCoord, ADDR_OAM_d20e);
}
PORT(ParticleAnim_Magic1) { Particles(gb).anim(ADDR_Frameset_d8e7, wCurParticle, ADDR_OAM_d80d); }
PORT(ParticleAnim_Magic2) { Particles(gb).anim(ADDR_Frameset_d8f4, wCurParticle, ADDR_OAM_d80d); }
PORT(ParticleAnim_MagicStop) { Particles(gb).anim(ADDR_Frameset_d901, wCurParticle, ADDR_OAM_d80d); }
PORT(ParticleAnim_Owl) { Particles(gb).anim(ADDR_Frameset_d7b4, wCurParticleYCoord, ADDR_OAM_d763); }
PORT(ParticleAnim_TreasureGlow) { Particles(gb).anim(ADDR_Frameset_d75a, wCurParticleYCoord, ADDR_OAM_d716); }
PORT(ParticleAnim_BubbleBurst) { Particles(gb).anim(ADDR_Frameset_d622, wCurParticleYCoord, ADDR_OAM_d5ae); }
PORT(ParticleAnim_BouncyJump) { Particles(gb).anim(ADDR_Frameset_d591, wCurParticleYCoord, ADDR_OAM_d532); }
PORT(ParticleAnim_Snow) { Particles(gb).anim(ADDR_Frameset_d806, wCurParticle, ADDR_OAM_d7bd); }
PORT(ParticleAnim_Ice)
{
	Particles p(gb);
	p.anim(p.by_direction(ADDR_Frameset_d6fd, ADDR_Frameset_d6e4), wCurParticleYCoord, ADDR_OAM_d62b);
}
PORT(ParticleAnim_Dust) { Particles(gb).anim(ADDR_Frameset_d4c5, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT(ParticleAnim_Debris) { Particles(gb).anim(ADDR_Frameset_d49a, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT(ParticleAnim_Stars) { Particles(gb).anim(ADDR_Frameset_d525, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT(ParticleAnim_SmokeLarge) { Particles(gb).anim(ADDR_Frameset_d4f3, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT(ParticleAnim_SmokeSmall) { Particles(gb).anim(ADDR_Frameset_d4ea, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT(ParticleAnim_Splash) { Particles(gb).anim(ADDR_Frameset_d4ba, wCurParticleYCoord, ADDR_OAM_d20e); }
