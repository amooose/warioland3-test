// Particles (src/engine/level/particles.asm): the 4 particle slots (block
// debris, dust, splashes, bubbles, the owl's feathers, the magic sparkles
// and so on): creating one, and each frame running each slot's particle
// function, which animates it and draws its sprite.
//
// The routines pass the particle in bc and leave registers their callers
// see (UpdateParticles returns with the last function's), so each keeps
// the original's registers and flags.
#include "frame.hpp"
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "object.hpp"

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

const ParticleFunc *particle_func(uint8_t id);

class Particles {
public:
	explicit Particles(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void update(); // UpdateParticles
	void run(const ParticleFunc &f, uint16_t particle);
	// UpdateParticleAnimation: at = the particle's frame duration (then its
	// frameset offset and frame); returns at + 2
	uint16_t update_animation(uint16_t at, uint16_t frameset);
	void animate(uint16_t anim, uint16_t at); // ParticleAnim_*, at = the frame duration
	void load_sprite(uint16_t oam); // LoadParticleSprite

private:
	uint16_t by_direction(uint16_t left, uint16_t right) const { return mem_[wDirection] ? right : left; }
	void anim(uint16_t frameset, uint16_t y_dest, uint16_t oam, uint16_t at);
	void deinit(uint16_t particle);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// runs the function of every slot in use (UpdateParticles' jumptable, by
// the particle's id)
void Particles::update()
{
	if (!mem_[wNumParticles])
		return;
	for (int n = 0; n < MAX_NUM_PARTICLES; n++) {
		uint16_t slot = static_cast<uint16_t>(wParticle0 + n * (wParticle1 - wParticle0));
		uint8_t id = mem_[slot];
		if (!id)
			continue;
		if (id >= NUM_PARTICLES) {
			if (gb_->test.active)
				gb_->test.unrealistic = true;
			return;
		}
		if (const ParticleFunc *f = particle_func(id))
			run(*f, slot);
	}
}

// the slot cleared, one particle fewer
void Particles::deinit(uint16_t particle)
{
	mem_[particle] = 0;
	mem_[wNumParticles] = static_cast<uint8_t>(mem_[wNumParticles] - 1);
}

// starts the particle (or moves it along), animates it, and ends it when
// it's done
void Particles::run(const ParticleFunc &f, uint16_t particle)
{
	uint8_t state = mem_[wWarioState];
	if ((f.pre == Pre::BOUNCY && state != WST_BOUNCY_AIRBORNE) || (f.pre == Pre::MAGIC && state == WST_MAGIC_STOPPING)) {
		deinit(particle);
		return;
	}
	// the particle's fields (the asm steps through them with inc c, in its
	// page)
	auto field = [particle](int n) { return static_cast<uint16_t>((particle & 0xff00) | static_cast<uint8_t>(particle + n)); };
	uint16_t init = field(PARTICLE_STRUCT_INITIALISED);
	if (!mem_[init]) {
		mem_[init] = 1;
		mem_[field(PARTICLE_STRUCT_DURATION)] = 0;
		mem_[field(PARTICLE_STRUCT_DURATION + 1)] = 0; // PARTICLE_STRUCT_FRAMESET_OFFSET
		if (f.sfx)
			play_sfx(mem_, f.sfx);
	} else {
		switch (f.init) {
		case Init::IN_PLACE:
			break;
		case Init::WARIO_POS:
			for (int i = 0; i < 4; i++)
				mem_[hYPosHi + i] = mem_[wWarioPos + i];
			for (int i = 0; i < 4; i++)
				mem_[field(2 + i)] = mem_[hYPosHi + i];
			break;
		case Init::SLIDE_RIGHT:
		case Init::SLIDE_LEFT: {
			// 12 pixels behind Wario, and 12 up
			bool right = f.init == Init::SLIDE_RIGHT;
			uint16_t x = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
			x = static_cast<uint16_t>(right ? x - 0x0c : x + 0x0c);
			uint16_t y = static_cast<uint16_t>((mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]) - 0x0c);
			mem_[hYPosHi] = static_cast<uint8_t>(y >> 8);
			mem_[hYPosLo] = static_cast<uint8_t>(y);
			mem_[hXPosHi] = static_cast<uint8_t>(x >> 8);
			mem_[hXPosLo] = static_cast<uint8_t>(x);
			for (int i = 0; i < 4; i++)
				mem_[field(2 + i)] = mem_[hYPosHi + i];
			break;
		}
		case Init::DRIFT_UP: { // drifts up 2 pixels
			uint16_t y = static_cast<uint16_t>((mem_[field(2)] << 8 | mem_[field(3)]) - 2);
			mem_[field(3)] = static_cast<uint8_t>(y);
			mem_[field(2)] = static_cast<uint8_t>(y >> 8);
			break;
		}
		}
	}
	animate(f.anim, field(PARTICLE_STRUCT_DURATION));
	switch (f.end) {
	case End::NEVER:
		return;
	case End::ANIM:
		if (!mem_[wCurParticleAnimEnded])
			return;
		break;
	case End::SLIDING:
		if (mem_[wWarioState] == WST_SLIDING)
			return;
		break;
	case End::OFF_SCREEN: // until off the bottom of the screen
		if (mem_[wCurSpriteYCoord] < 0xc0)
			return;
		break;
	}
	deinit(particle);
}

uint16_t Particles::update_animation(uint16_t at, uint16_t frameset)
{
	mem_[wCurParticleAnimEnded] = FALSE;
	uint8_t duration = mem_[at];
	mem_[at] = static_cast<uint8_t>(duration - 1);
	if (duration) // .no_new_frame
		return static_cast<uint16_t>(at + 2);

	uint16_t entry = static_cast<uint16_t>(frameset + mem_[static_cast<uint16_t>(at + 1)]);
	uint8_t frame = mem_[entry];
	if (frame == 0xff) { // .reset_anim
		mem_[static_cast<uint16_t>(at + 1)] = 0; // frameset offset
		mem_[at] = 0; // duration
		mem_[wCurParticleAnimEnded] = TRUE;
		return static_cast<uint16_t>(at + 2);
	}
	mem_[static_cast<uint16_t>(at + 2)] = frame;
	// the offset is read again after the frame's written (they can alias)
	mem_[static_cast<uint16_t>(at + 1)] = static_cast<uint8_t>(mem_[static_cast<uint16_t>(at + 1)] + 2);
	mem_[at] = mem_[static_cast<uint16_t>(entry + 1)]; // duration
	return static_cast<uint16_t>(at + 2);
}

// animates the particle (at its frame duration) with the frameset and
// draws it with the OAM data
void Particles::anim(uint16_t frameset, uint16_t y_dest, uint16_t oam, uint16_t at)
{
	uint16_t hl = update_animation(at, frameset);
	// ld a, [hld] (the frame) / dec l / dec l / ld a, [hld] (x) / dec l /
	// ld a, [hld] (y): hld is 16-bit, dec l 8-bit
	auto dec_l = [](uint16_t v) { return static_cast<uint16_t>((v & 0xff00) | static_cast<uint8_t>(v - 1)); };
	mem_[wCurParticleFrame] = mem_[hl--];
	hl = dec_l(dec_l(hl));
	mem_[wCurParticleXCoord] = mem_[hl--];
	hl = dec_l(hl);
	mem_[y_dest] = mem_[hl];
	mem_[wCurParticleAttributes] = 0;
	load_sprite(oam);
}

void Particles::animate(uint16_t anim_func, uint16_t at)
{
	switch (anim_func) {
	case ADDR_ParticleAnim_Bubbles: anim(by_direction(ADDR_Frameset_d502, ADDR_Frameset_d50b), wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	case ADDR_ParticleAnim_Magic1: anim(ADDR_Frameset_d8e7, wCurParticle, ADDR_OAM_d80d, at); break;
	case ADDR_ParticleAnim_Magic2: anim(ADDR_Frameset_d8f4, wCurParticle, ADDR_OAM_d80d, at); break;
	case ADDR_ParticleAnim_MagicStop: anim(ADDR_Frameset_d901, wCurParticle, ADDR_OAM_d80d, at); break;
	case ADDR_ParticleAnim_Owl: anim(ADDR_Frameset_d7b4, wCurParticleYCoord, ADDR_OAM_d763, at); break;
	case ADDR_ParticleAnim_TreasureGlow: anim(ADDR_Frameset_d75a, wCurParticleYCoord, ADDR_OAM_d716, at); break;
	case ADDR_ParticleAnim_BubbleBurst: anim(ADDR_Frameset_d622, wCurParticleYCoord, ADDR_OAM_d5ae, at); break;
	case ADDR_ParticleAnim_BouncyJump: anim(ADDR_Frameset_d591, wCurParticleYCoord, ADDR_OAM_d532, at); break;
	case ADDR_ParticleAnim_Snow: anim(ADDR_Frameset_d806, wCurParticle, ADDR_OAM_d7bd, at); break;
	case ADDR_ParticleAnim_Ice: anim(by_direction(ADDR_Frameset_d6fd, ADDR_Frameset_d6e4), wCurParticleYCoord, ADDR_OAM_d62b, at); break;
	case ADDR_ParticleAnim_Dust: anim(ADDR_Frameset_d4c5, wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	case ADDR_ParticleAnim_Debris: anim(ADDR_Frameset_d49a, wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	case ADDR_ParticleAnim_Stars: anim(ADDR_Frameset_d525, wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	case ADDR_ParticleAnim_SmokeLarge: anim(ADDR_Frameset_d4f3, wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	case ADDR_ParticleAnim_SmokeSmall: anim(ADDR_Frameset_d4ea, wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	case ADDR_ParticleAnim_Splash: anim(ADDR_Frameset_d4ba, wCurParticleYCoord, ADDR_OAM_d20e, at); break;
	}
}

// the particle's sprite to virtual OAM, relative to the camera (LoadSprite
// takes the OAM data in hl)
void Particles::load_sprite(uint16_t oam)
{
	uint16_t scy = wSCY, scx = wSCX;
	if (mem_[wCameraConfigFlags] == (CAM_XSCROLL2 | CAM_TRANSITIONS | CAM_BORDER_RIGHT | CAM_BORDER_LEFT)) {
		scy = wCameraSCY + 1;
		scx = wCameraSCX + 1;
	}
	mem_[wCurSpriteYCoord] = static_cast<uint8_t>(mem_[wCurParticleYCoord] + OAM_Y_OFS - mem_[scy]);
	mem_[wCurSpriteXCoord] = static_cast<uint8_t>(mem_[wCurParticleXCoord] + OAM_X_OFS - mem_[scx]);
	mem_[wCurSpriteFrame] = mem_[wCurParticleFrame];
	mem_[wCurSpriteAttributes] = mem_[wCurParticleAttributes];
	home::load_sprite(gb_, oam);
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

// UpdateParticles' jumptable, by id (PARTICLE_NONE's function does nothing)
const ParticleFunc *particle_func(uint8_t id)
{
	static const ParticleFunc *const funcs[] = {nullptr, &DEBRIS, &STARS, &SPLASH, &DUST, &SLIDE_DUST_RIGHT,
		&SLIDE_DUST_LEFT, &SMOKE_LARGE, &SMOKE_SMALL, &BUBBLES, &BOUNCY_JUMP, &BUBBLE_BURST, &ICE,
		&TREASURE_GLOW, &OWL_PARTICLE, &SNOW, &MAGIC_1, &MAGIC_2, &MAGIC_STOP};
	return id < sizeof funcs / sizeof funcs[0] ? funcs[id] : nullptr;
}

} // namespace

uint16_t create_particle(GB *gb, uint8_t id)
{
	Memory mem(gb);
	uint8_t index = mem[wCurParticleIndex];
	uint8_t offset = static_cast<uint8_t>(index * 16);
	mem[wCurParticleIndex] = (index + 1) & 0x03;
	uint16_t slot = static_cast<uint16_t>(wParticles + offset);
	if (!mem[slot])
		++mem[wNumParticles]; // a free slot: one more particle
	mem[slot] = id; // PARTICLE_STRUCT_ID
	uint16_t at = static_cast<uint16_t>((slot & 0xff00) | static_cast<uint8_t>(slot + 1)); // inc l
	mem[at++] = FALSE; // PARTICLE_STRUCT_INITIALISED
	mem[at++] = mem[hYPosHi];
	mem[at++] = mem[hYPosLo];
	mem[at++] = mem[hXPosHi];
	mem[at++] = mem[hXPosLo];
	return at;
}

void block_to_hpos(GB *gb, uint16_t block)
{
	Memory mem(gb);
	uint8_t row = static_cast<uint8_t>((block >> 8) - 0xa0), column = static_cast<uint8_t>(block);
	uint8_t y_hi = static_cast<uint8_t>((mem[wBlockPtrBank] - 1) * 2 + (row >> 4));
	unsigned y_lo = ((row & 0x0f) << 4) + 0x10;
	mem[hYPosLo] = static_cast<uint8_t>(y_lo);
	mem[hYPosHi] = static_cast<uint8_t>(y_hi + (y_lo > 0xff));
	unsigned x_lo = ((column & 0x0f) << 4) + 0x08;
	mem[hXPosLo] = static_cast<uint8_t>(x_lo);
	mem[hXPosHi] = static_cast<uint8_t>((column >> 4) + (x_lo > 0xff));
}

void create_particle_in_block(GB *gb, uint16_t block, uint8_t id)
{
	block_to_hpos(gb, block);
	create_particle(gb, id);
}

} // namespace wl3::level

using namespace wl3::level;

// b = the PARTICLE_* constant
PORT_EX(CreateParticle, 0, ALL_REGISTERS) { create_particle(gb, gb->cpu.b); }
// hl = a block in the level map: hYPos / hXPos = its position
PORT_EX(Func_ce3e, 0, ALL_REGISTERS) { block_to_hpos(gb, REG_HL(gb)); }

// b = the PARTICLE_* constant, hl = a block: a particle there
PORT_EX(CreateParticleInBlock, 0, ALL_REGISTERS) { create_particle_in_block(gb, REG_HL(gb), gb->cpu.b); }

// The particles' update, plain: UpdateParticles runs the functions and
// animations itself. Their only callers are each other and the level's
// frame and room start (plain C++), and UpdateParticles' own loop reloads
// hl for each slot, so the hooks clobber every register.
PORT_EX(UpdateParticles, 0, ALL_REGISTERS) { wl3::level::update_particles(gb); }
// hl = the frame duration, de = the frameset
PORT_EX(UpdateParticleAnimation, 0, ALL_REGISTERS) { Particles(gb).update_animation(REG_HL(gb), REG_DE(gb)); }
// hl = the OAM data
PORT_EX(LoadParticleSprite, 0, ALL_REGISTERS) { Particles(gb).load_sprite(REG_HL(gb)); }

// bc = the particle
PORT_EX(ParticleFunc_None, 0, ALL_REGISTERS) {}
PORT_EX(ParticleFunc_Debris, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::DEBRIS, REG_BC(gb)); }
PORT_EX(ParticleFunc_Stars, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::STARS, REG_BC(gb)); }
PORT_EX(ParticleFunc_Splash, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::SPLASH, REG_BC(gb)); }
PORT_EX(ParticleFunc_Dust, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::DUST, REG_BC(gb)); }
PORT_EX(ParticleFunc_SlideDustRight, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::SLIDE_DUST_RIGHT, REG_BC(gb)); }
PORT_EX(ParticleFunc_SlideDustLeft, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::SLIDE_DUST_LEFT, REG_BC(gb)); }
PORT_EX(ParticleFunc_SmokeLarge, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::SMOKE_LARGE, REG_BC(gb)); }
PORT_EX(ParticleFunc_SmokeSmall, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::SMOKE_SMALL, REG_BC(gb)); }
PORT_EX(ParticleFunc_Bubbles, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::BUBBLES, REG_BC(gb)); }
PORT_EX(ParticleFunc_BouncyJump, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::BOUNCY_JUMP, REG_BC(gb)); }
PORT_EX(ParticleFunc_BubbleBurst, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::BUBBLE_BURST, REG_BC(gb)); }
PORT_EX(ParticleFunc_Ice, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::ICE, REG_BC(gb)); }
PORT_EX(ParticleFunc_TreasureGlow, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::TREASURE_GLOW, REG_BC(gb)); }
PORT_EX(ParticleFunc_Owl, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::OWL_PARTICLE, REG_BC(gb)); }
PORT_EX(ParticleFunc_Snow, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::SNOW, REG_BC(gb)); }
PORT_EX(ParticleFunc_Magic1, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::MAGIC_1, REG_BC(gb)); }
PORT_EX(ParticleFunc_Magic2, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::MAGIC_2, REG_BC(gb)); }
PORT_EX(ParticleFunc_MagicStop, 0, ALL_REGISTERS) { Particles(gb).run(wl3::level::MAGIC_STOP, REG_BC(gb)); }

// bc = the particle's frame duration
PORT_EX(ParticleAnim_Bubbles, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Bubbles, REG_BC(gb)); }
PORT_EX(ParticleAnim_Magic1, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Magic1, REG_BC(gb)); }
PORT_EX(ParticleAnim_Magic2, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Magic2, REG_BC(gb)); }
PORT_EX(ParticleAnim_MagicStop, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_MagicStop, REG_BC(gb)); }
PORT_EX(ParticleAnim_Owl, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Owl, REG_BC(gb)); }
PORT_EX(ParticleAnim_TreasureGlow, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_TreasureGlow, REG_BC(gb)); }
PORT_EX(ParticleAnim_BubbleBurst, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_BubbleBurst, REG_BC(gb)); }
PORT_EX(ParticleAnim_BouncyJump, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_BouncyJump, REG_BC(gb)); }
PORT_EX(ParticleAnim_Snow, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Snow, REG_BC(gb)); }
PORT_EX(ParticleAnim_Ice, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Ice, REG_BC(gb)); }
PORT_EX(ParticleAnim_Dust, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Dust, REG_BC(gb)); }
PORT_EX(ParticleAnim_Debris, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Debris, REG_BC(gb)); }
PORT_EX(ParticleAnim_Stars, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Stars, REG_BC(gb)); }
PORT_EX(ParticleAnim_SmokeLarge, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_SmokeLarge, REG_BC(gb)); }
PORT_EX(ParticleAnim_SmokeSmall, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_SmokeSmall, REG_BC(gb)); }
PORT_EX(ParticleAnim_Splash, 0, ALL_REGISTERS) { Particles(gb).animate(ADDR_ParticleAnim_Splash, REG_BC(gb)); }

namespace wl3::level {
void update_particles(GB *gb) { Particles(gb).update(); }
} // namespace wl3::level
