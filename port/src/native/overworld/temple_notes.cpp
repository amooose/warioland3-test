// bank56.asm's small scene object scripts: the music box's notes (Func_15848a,
// a cutscene of bank2b's) and the six objects of the prologue's map
// function (Func_15868e, bank2d's). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

static_assert(wSceneObj1State == wSceneObj1 + 6 && wSceneObj9XCoord == wSceneObj9 + 1);

constexpr uint16_t OBJ_X = 1;
constexpr uint16_t OBJ_STATE_OFFSET = 6;

// The notes' [action, counter] pairs. The asm's labels for the third and
// fifth are one byte off the wTempleMusicBox structs (wPlanePart2Counter,
// wTempleMusicBox5Counter): those are the bytes it uses.
constexpr uint16_t NOTE0 = w2d8a0;
constexpr uint16_t NOTE1 = wTempleMusicBox1;
constexpr uint16_t NOTE2 = wPlanePart2Counter;
constexpr uint16_t NOTE3 = wTempleMusicBox4;
constexpr uint16_t NOTE4 = wTempleMusicBox5Counter;
static_assert(wTempleMusicBox3 == NOTE2 + 1 && wPalFade2Colour3GreenUnk1 == NOTE4 + 1);

// Func_15848a.data_1 and .data_2: a note's rise by frame, to $80
constexpr uint16_t RISE_SLOW = ADDR_Func_15848a + 0x1cf;
constexpr uint16_t RISE_FAST = ADDR_Func_15848a + 0x1f3;
constexpr uint8_t RISE_END = 0x80;

constexpr uint8_t NOTE_STATE = 0x15;
constexpr uint8_t NOTE_FADE_STATE = 0x14;
constexpr uint8_t STAFF_STATE = 0x16;

// Func_15868e: the six objects' steps. Each object goes round them in its
// own order: appear at a spot (with state $29 or $2a, by rDIV), then move
// along a path, three times, then back to step 1.
enum class Kind : uint8_t { Idle, Appear, Move, Restart };
struct Step {
	Kind kind;
	uint8_t y, x; // APPEAR: where
	uint16_t path; // MOVE: the ApplyOWMovement data
};
constexpr Step IDLE = {Kind::Idle, 0, 0, 0};
constexpr Step AT_869 = {Kind::Appear, 0x30, 0x50, 0}; // .Func_158869
constexpr Step AT_87B = {Kind::Appear, 0x2e, 0x4e, 0}; // .Func_15887b
constexpr Step AT_882 = {Kind::Appear, 0x2e, 0x52, 0}; // .Func_158882
constexpr Step AT_889 = {Kind::Appear, 0x31, 0x4f, 0}; // .Func_158889
constexpr Step AT_890 = {Kind::Appear, 0x31, 0x51, 0}; // .Func_158890
constexpr Step PATH_897 = {Kind::Move, 0, 0, ADDR_Data_158897};
constexpr Step PATH_8DE = {Kind::Move, 0, 0, ADDR_Data_1588de};
constexpr Step PATH_925 = {Kind::Move, 0, 0, ADDR_Data_158925};
constexpr Step AGAIN = {Kind::Restart, 0, 0, 0};

struct PrologueObject {
	uint16_t step, obj;
	bool mirrored;
	Step steps[8];
};
// in the order Func_15868e runs them
constexpr PrologueObject PROLOGUE_OBJECTS[6] = {
	{w2d150, wSceneObj2, false, {IDLE, AT_87B, PATH_897, AT_869, PATH_8DE, AT_889, PATH_925, AGAIN}},
	{w2d151, wSceneObj3, true, {IDLE, AT_882, PATH_897, AT_869, PATH_8DE, AT_890, PATH_925, AGAIN}},
	{wd152, wSceneObj5, false, {IDLE, AT_869, PATH_8DE, AT_889, PATH_925, AT_87B, PATH_897, AGAIN}},
	{wd153, wSceneObj8, true, {IDLE, AT_869, PATH_8DE, AT_890, PATH_925, AT_882, PATH_897, AGAIN}},
	{wd154, wSceneObj9, false, {IDLE, AT_889, PATH_925, AT_87B, PATH_897, AT_869, PATH_8DE, AGAIN}},
	{wd155, wSceneObj10, true, {IDLE, AT_890, PATH_925, AT_882, PATH_897, AT_869, PATH_8DE, AGAIN}},
};
constexpr uint8_t PROLOGUE_OBJ_STATE = 0x29;

} // namespace

void TempleNotes::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void TempleNotes::set_scene_obj_state(uint16_t state, uint8_t value)
{
	SET_HL(gb_, state);
	gb_->cpu.a = value;
	routines_.call(ADDR_SetSceneObjState);
}

// ---------------------------------------------------------------------------
// Func_15848a: the notes

void TempleNotes::update_notes()
{
	staff(); // .Func_15849a
	note(NOTE1, wSceneObj5, false, 0xf8, 0xfd, RISE_SLOW, 0); // .Func_1584da
	note(NOTE2, wSceneObj6, true, 0xf8, 0x03, RISE_SLOW, 0); // .Func_158529
	note(NOTE3, wSceneObj7, false, 0xfa, 0xfc, RISE_FAST, 0xff); // .Func_15858c
	note(NOTE4, wSceneObj8, true, 0xf0, 0x06, RISE_FAST, 1); // .Func_1585e1
}

// .Func_15849a: wSceneObj1 rises from y $18 above wSceneObj9, a pixel every
// 4 frames, to $58
void TempleNotes::staff()
{
	Byte counter = mem_[NOTE0 + 1];
	++counter;
	Byte action = mem_[NOTE0];
	switch (action) {
	case 0:
		counter = 0;
		return;
	case 1:
		mem_[wSceneObj1] = 0x18;
		mem_[wSceneObj1 + OBJ_X] = mem_[wSceneObj9XCoord];
		set_scene_obj_state(wSceneObj1State, STAFF_STATE);
		counter = 0;
		++action;
		return;
	case 2: {
		if (counter & 0x03)
			return;
		Byte y = mem_[wSceneObj1];
		++y;
		if (y != 0x58)
			return;
		mem_[wSceneObj1State] = 0;
		action = 0;
		return;
	}
	default:
		unrealistic();
		return;
	}
}

// A note from wSceneObj1 (dy, dx from it), rising along a table, its
// x stepping by x_step on odd frames. The second and fourth wait first (10
// and 8 frames) and come round again while wSceneObj1 is shown.
void TempleNotes::note(uint16_t note, uint16_t obj, bool waits, uint8_t dy, uint8_t dx, uint16_t rise, uint8_t x_step)
{
	Byte counter = mem_[static_cast<uint16_t>(note + 1)];
	++counter;
	Byte action = mem_[note];
	auto next = [&] {
		counter = 0;
		++action;
	};
	uint8_t step = action;
	// the notes that wait have a step for it after the first
	if (waits && step >= 1) {
		if (step == 1) {
			if (counter < (note == NOTE2 ? 10 : 8))
				return;
			next();
			return;
		}
		if (step == 4) {
			action = mem_[wSceneObj1State] ? 2 : 0;
			return;
		}
		step = static_cast<uint8_t>(step - 1);
	}
	uint16_t state = static_cast<uint16_t>(obj + OBJ_STATE_OFFSET);
	switch (step) {
	case 0:
		if (!mem_[wSceneObj1State])
			return;
		next();
		return;
	case 1:
		mem_[obj] = static_cast<uint8_t>(mem_[wSceneObj1] + dy);
		mem_[static_cast<uint16_t>(obj + OBJ_X)] = static_cast<uint8_t>(mem_[wSceneObj1 + OBJ_X] + dx);
		set_scene_obj_state(state, NOTE_STATE);
		next();
		return;
	case 2: { // .Func_15864a / .Func_15866e: the rise
		uint8_t frame = counter;
		uint8_t by = mem_[static_cast<uint16_t>(rise + frame)];
		if (by == RISE_END) {
			mem_[state] = 0;
			if (waits)
				next();
			else
				action = 0;
			return;
		}
		Byte y = mem_[obj];
		y += by;
		if (x_step && (counter & 1)) {
			Byte x = mem_[static_cast<uint16_t>(obj + OBJ_X)];
			x += x_step;
		}
		if (counter == 3)
			mem_[state] = NOTE_FADE_STATE;
		return;
	}
	default:
		unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// Func_15868e: the prologue's six objects

void TempleNotes::update_prologue_objects()
{
	for (const auto &o : PROLOGUE_OBJECTS) {
		Byte step = mem_[o.step];
		if (step >= 8) {
			unrealistic();
			continue;
		}
		const Step &s = o.steps[static_cast<uint8_t>(step)];
		switch (s.kind) {
		case Kind::Idle:
			break;
		case Kind::Appear: { // .Func_158869 and the like
			mem_[o.obj] = s.y;
			mem_[static_cast<uint16_t>(o.obj + 1)] = s.x;
			// ld [hli], a / set 2, l: the state, 4 bytes on
			uint16_t state = static_cast<uint16_t>(o.obj + 2);
			state = static_cast<uint16_t>(state | 1 << 2);
			set_scene_obj_state(state, static_cast<uint8_t>((mem_[rDIV] & 1) + PROLOGUE_OBJ_STATE));
			++step;
			break;
		}
		case Kind::Move:
			SET_DE(gb_, s.path);
			SET_HL(gb_, o.obj);
			routines_.call(o.mirrored ? ADDR_ApplyOWMovement_Mirrored : ADDR_ApplyOWMovement);
			if (gb_->cpu.f & FLAG_Z)
				++step;
			break;
		case Kind::Restart:
			step = 1;
			break;
		}
	}
}

} // namespace wl3::overworld

// Only farcalled; their callers reload what they use after.
PORT_EX(Func_15848a, 0, ALL_REGISTERS)
{
	wl3::overworld::TempleNotes(gb).update_notes();
}

PORT_EX(Func_15868e, 0, ALL_REGISTERS)
{
	wl3::overworld::TempleNotes(gb).update_prologue_objects();
}
