// The scenes' objects in bank2b (the epilogue's, the temple's and the
// prologue's), each a state machine over a table of animations. See
// overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

// The framesets' and sprites' banks
constexpr uint8_t CREDITS_OAM = BANK_OAM_15e44a; // BANK("Credits OAM")
constexpr uint8_t OBJECT_FRAMESETS = 0x1a; // BANK("Object Framesets")
constexpr uint8_t TITLE_GFX = 0x01; // BANK("Title Gfx")
constexpr uint8_t OVERWORLD_OAM_1 = 0x2a; // BANK("Overworld OAM 1")

constexpr uint8_t STAY = 0xff;

enum class Special : uint8_t {
	PLAIN,
	SFX_138_EVERY_30, // Func_ac0b6.PlaySFXEvery30Frames
	SFX_08E_EVERY_48, // Func_ac0b6.Func_ac1e9
	SFX_11A_EVERY_40, // Func_ac0b6.Func_ac207: on the first frame of each 40
	CARRY, // Func_ac0b6.Func_ac1c3: following Wario's carrying
	SFX_0F3_AT_END, // Func_ac409.Func_ac479
	SFX_0F4_AT_END, // Func_ac409.Func_ac493
};

struct ObjAnim {
	uint16_t frameset;
	uint8_t anim_bank; // UpdateOWAnimation's
	uint16_t oam;
	uint8_t oam_bank;
	bool scroll; // LoadOWSpriteWithScroll, or LoadOWSprite
	uint8_t next; // the state at the animation's end (and a frame of it), or STAY
	Special special;
};

constexpr ObjAnim NO_ANIM = {0, 0, 0, 0, false, STAY, Special::PLAIN};
// Func_ac056: the same in each
constexpr ObjAnim FUNC_AC056 = {ADDR_Frameset_76ca, TITLE_GFX, ADDR_OAM_75c3, BANK_OAM_75c3, false, STAY, Special::PLAIN};

constexpr ObjAnim credits(uint16_t frameset, uint8_t next = STAY, Special special = Special::PLAIN)
{
	return {frameset, CREDITS_OAM, ADDR_OAM_15e97d, CREDITS_OAM, true, next, special};
}

// Func_ac0b6: the epilogue's objects (and the prologue's text arrow)
constexpr ObjAnim EPILOGUE_ANIMS[] = {
	NO_ANIM, FUNC_AC056,
	credits(ADDR_Frameset_15ed91), credits(ADDR_Frameset_15ed94), credits(ADDR_Frameset_15ed97),
	credits(ADDR_Frameset_15ed9a), credits(ADDR_Frameset_15ed9d, 0x0a), credits(ADDR_Frameset_15edb3),
	credits(ADDR_Frameset_15eda6, STAY, Special::SFX_138_EVERY_30),
	credits(ADDR_Frameset_15edc0), credits(ADDR_Frameset_15edc3), credits(ADDR_Frameset_15edc6, 0x0a),
	credits(ADDR_Frameset_15ede9, 0x0b), credits(ADDR_Frameset_15edf2, 0x0a), credits(ADDR_Frameset_15ee0f, 0x0a),
	credits(ADDR_Frameset_15ee1e, 0x17), credits(ADDR_Frameset_15ee3b, 0x0a), credits(ADDR_Frameset_15ee44),
	credits(ADDR_Frameset_15ee57), credits(ADDR_Frameset_15ee64, STAY, Special::CARRY),
	credits(ADDR_Frameset_15ee67), credits(ADDR_Frameset_15ee70),
	credits(ADDR_Frameset_15ee79, STAY, Special::SFX_08E_EVERY_48),
	credits(ADDR_Frameset_15ee82, STAY, Special::SFX_11A_EVERY_40),
	{ADDR_Frameset_681d2, OBJECT_FRAMESETS, ADDR_OAM_18007b, BANK_OAM_18007b, true, STAY, Special::PLAIN},
	{ADDR_Frameset_68b12, OBJECT_FRAMESETS, ADDR_OAM_18422f, BANK_OAM_18422f, true, STAY, Special::PLAIN},
	{ADDR_Frameset_68562, OBJECT_FRAMESETS, ADDR_OAM_180ea7, BANK_OAM_180ea7, true, STAY, Special::PLAIN},
	credits(ADDR_Frameset_15ee87), credits(ADDR_Frameset_15ee8a),
};

constexpr ObjAnim temple(uint16_t frameset, bool scroll = true)
{
	return {frameset, CREDITS_OAM, ADDR_OAM_15e813, CREDITS_OAM, scroll, STAY, Special::PLAIN};
}
constexpr ObjAnim temple_big(uint16_t frameset, uint8_t next = STAY)
{
	return {frameset, CREDITS_OAM, ADDR_OAM_18c88a, BANK_OAM_18c88a, true, next, Special::PLAIN};
}

// UpdateTempleSceneObjAnimation: the temple's objects
constexpr ObjAnim TEMPLE_ANIMS[] = {
	NO_ANIM, FUNC_AC056,
	{ADDR_Frameset_15e796, CREDITS_OAM, ADDR_OAM_1644a, CREDITS_OAM, true, 0x00, Special::PLAIN},
	temple(ADDR_Frameset_15e89c), temple(ADDR_Frameset_15e89f), temple(ADDR_Frameset_15e8a2),
	temple(ADDR_Frameset_15e8a5), temple(ADDR_Frameset_15e8a8), temple(ADDR_Frameset_15e88d),
	temple(ADDR_Frameset_15e896, false), temple(ADDR_Frameset_15e893, false), temple(ADDR_Frameset_15e890, false),
	temple(ADDR_Frameset_15e899, false),
	temple_big(ADDR_Frameset_15e8ab), temple_big(ADDR_Frameset_15e8b0),
	temple_big(ADDR_Frameset_15e8b0, 0x0d), temple_big(ADDR_Frameset_15e8ab, 0x0e),
	temple_big(ADDR_Frameset_15e932), temple_big(ADDR_Frameset_15e95c),
};

constexpr ObjAnim prologue(uint16_t frameset, uint8_t next = STAY, Special special = Special::PLAIN)
{
	return {frameset, CREDITS_OAM, ADDR_OAM_15e44a, CREDITS_OAM, true, next, special};
}

// Func_ac409: the prologue's objects
constexpr ObjAnim PROLOGUE_ANIMS[] = {
	NO_ANIM, FUNC_AC056,
	prologue(ADDR_Frameset_15e741), prologue(ADDR_Frameset_15e748), prologue(ADDR_Frameset_15e74b),
	prologue(ADDR_Frameset_15e774), prologue(ADDR_Frameset_15e77d), prologue(ADDR_Frameset_15e780),
	prologue(ADDR_Frameset_15e793), prologue(ADDR_Frameset_15e7a3),
	prologue(ADDR_Frameset_15e7a6, STAY, Special::SFX_0F3_AT_END), prologue(ADDR_Frameset_15e7b7),
	prologue(ADDR_Frameset_15e7c4, STAY, Special::SFX_0F4_AT_END), prologue(ADDR_Frameset_15e7ed),
	prologue(ADDR_Frameset_15e7b7, 0x00), prologue(ADDR_Frameset_15e783), prologue(ADDR_Frameset_15e78a, 0x00),
	prologue(ADDR_Frameset_15e796, 0x00), prologue(ADDR_Frameset_15e7f0), prologue(ADDR_Frameset_15e7f5),
	{ADDR_Frameset_aad0b, OVERWORLD_OAM_1, ADDR_OAM_aab5c, BANK_OAM_aab5c, true, 0x00, Special::PLAIN},
	prologue(ADDR_Frameset_15e7fa, 0x16), prologue(ADDR_Frameset_15e801), prologue(ADDR_Frameset_15e80a),
};

struct Machine {
	const ObjAnim *anims;
	uint8_t count;
	uint16_t none; // its state 0's ret (JumpTable's target)
};

constexpr Machine EPILOGUE_MACHINE = {EPILOGUE_ANIMS, sizeof(EPILOGUE_ANIMS) / sizeof(EPILOGUE_ANIMS[0]), ADDR_Func_ac0b6 + 0x66};
constexpr Machine TEMPLE_MACHINE = {TEMPLE_ANIMS, sizeof(TEMPLE_ANIMS) / sizeof(TEMPLE_ANIMS[0]),
	ADDR_UpdateTempleSceneObjAnimation + 0x52};
constexpr Machine PROLOGUE_MACHINE = {PROLOGUE_ANIMS, sizeof(PROLOGUE_ANIMS) / sizeof(PROLOGUE_ANIMS[0]), ADDR_Func_ac409 + 0x47};

} // namespace

void SceneSprites::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void SceneSprites::run(const void *machine_ptr, uint16_t state_addr)
{
	const Machine &machine = *static_cast<const Machine *>(machine_ptr);
	uint8_t state = mem_[state_addr];
	if (state >= machine.count) {
		unrealistic();
		return;
	}
	SET_BC(gb_, state_addr);
	if (!state) { // the ret, as JumpTable leaves the registers
		SET_DE(gb_, 0x0000);
		SET_HL(gb_, machine.none);
		gb_->cpu.a = static_cast<uint8_t>(machine.none);
		gb_->cpu.f = FLAG_Z;
		return;
	}
	const ObjAnim &anim = machine.anims[state];
	// Func_ac000 and the like: the animation (hl the object's duration)
	SET_HL(gb_, static_cast<uint16_t>(state_addr & ~(1 << 1)));
	gb_->cpu.b = anim.anim_bank;
	SET_DE(gb_, anim.frameset);
	routines_.call(ADDR_UpdateOWAnimation);
	SET_BC(gb_, state_addr);
	gb_->cpu.a = mem_[state_addr];
	// as the asm finds them: the object by UpdateOWAnimation's hl, its
	// counter by inc c
	uint16_t obj = REG_HL(gb_);
	uint16_t unk7_addr = static_cast<uint16_t>((state_addr & 0xff00) | static_cast<uint8_t>(state_addr + 1));

	Byte unk7 = mem_[unk7_addr];
	switch (anim.special) {
	case Special::SFX_138_EVERY_30:
		++unk7;
		if (unk7 >= 30) {
			unk7 = 0;
			play_sfx(mem_, SFX_138);
		}
		break;
	case Special::SFX_08E_EVERY_48:
		++unk7;
		if (unk7 >= 0x30) {
			unk7 = 0;
			play_sfx(mem_, SFX_08E);
		}
		break;
	case Special::SFX_11A_EVERY_40:
		++unk7;
		if (unk7 == 0x01)
			play_sfx(mem_, SFX_11A);
		else if (unk7 >= 0x28)
			unk7 = 0;
		break;
	case Special::CARRY:
		carry(obj, unk7_addr);
		break;
	default:
		break;
	}

	if (anim.next != STAY && mem_[wOWAnimationFinished] == 0xff) { // SetObjStateIfAnimationIsFinished
		uint16_t duration = static_cast<uint16_t>(obj | 0x04); // set 2, l
		mem_[duration] = 0;
		mem_[static_cast<uint16_t>(duration + 1)] = 0; // frameset offset
		mem_[static_cast<uint16_t>(duration + 2)] = anim.next; // state
		run(machine_ptr, static_cast<uint16_t>(duration + 2));
		return;
	}
	// the sprite (hl still the object, from UpdateOWAnimation)
	SET_DE(gb_, anim.oam);
	gb_->cpu.b = anim.oam_bank;
	routines_.call(anim.scroll ? ADDR_LoadOWSpriteWithScroll : ADDR_LoadOWSprite);
	if (mem_[wOWAnimationFinished] == 0xff) {
		if (anim.special == Special::SFX_0F3_AT_END)
			play_sfx(mem_, SFX_0F3);
		else if (anim.special == Special::SFX_0F4_AT_END)
			play_sfx(mem_, SFX_0F4);
	}
}

// Func_ac0b6.Func_ac1c3: the carried object (unk7 its phase) bobs as
// Wario walks, and follows his hands as he picks it up
void SceneSprites::carry(uint16_t obj, uint16_t phase_addr)
{
	Byte phase = mem_[phase_addr];
	uint8_t frame = mem_[wSceneWarioFrame];
	switch (mem_[wSceneWarioState]) {
	case SCENEWARIO_CARRY_WALK: { // .asm_ac254
		if (phase > 1)
			return;
		uint8_t at = phase ? 26 : 25;
		if (frame != at)
			return;
		int8_t dy = phase ? 1 : -1;
		phase = phase ^ 1;
		mem_[obj] = static_cast<uint8_t>(mem_[obj] + dy);
		return;
	}
	case SCENEWARIO_PICKUP: { // .asm_ac275
		uint8_t p = phase;
		if (p == 0x80)
			return;
		static constexpr struct {
			uint8_t frame, dy, dx;
		} HANDS[] = {{0x00, 0xfc, 0xea}, {0x01, 0xf0, 0xec}, {0x02, 0xde, 0xf9}};
		uint8_t i = p < 2 ? p : 2;
		if (frame != HANDS[i].frame)
			return;
		phase = i == 2 ? 0x80 : static_cast<uint8_t>(p + 1);
		mem_[obj] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] + HANDS[i].dy);
		mem_[obj + 1] = static_cast<uint8_t>(mem_[wSceneWarioXCoord] + HANDS[i].dx);
		return;
	}
	default:
		return;
	}
}

// bc: a scene object's state (wCurSceneObj's to wSceneObj15's), else a
// fuzzed one no caller passes
bool SceneSprites::scene_obj(uint16_t state)
{
	if (state >= wCurSceneObjState && state < wSceneObjsEnd && (state & 7) == (wCurSceneObjState & 7))
		return true;
	unrealistic();
	return false;
}

void SceneSprites::epilogue_obj(uint16_t state)
{
	if (scene_obj(state))
		run(&EPILOGUE_MACHINE, state);
}

void SceneSprites::temple_obj(uint16_t state)
{
	if (scene_obj(state))
		run(&TEMPLE_MACHINE, state);
}

void SceneSprites::prologue_obj(uint16_t state)
{
	if (scene_obj(state))
		run(&PROLOGUE_MACHINE, state);
}

// Func_ac074 / Func_ac080: the epilogue's
void SceneSprites::func_ac074()
{
	for (uint16_t state : {wCurSceneObjState, wSceneObj1State})
		epilogue_obj(state);
}

void SceneSprites::func_ac080()
{
	for (uint16_t state : {wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
		wSceneObj9State, wSceneObj10State, wSceneObj11State, wSceneObj12State, wSceneObj13State})
		epilogue_obj(state);
}

// Func_ac2b2 / UpdateTempleSceneObjs: the temple's
void SceneSprites::func_ac2b2()
{
	for (uint16_t state : {wCurSceneObjState, wSceneObj1State, wSceneObj2State})
		temple_obj(state);
}

void SceneSprites::temple_objs()
{
	for (uint16_t state : {wSceneObj3State, wSceneObj4State, wSceneObj7State, wSceneObj8State,
		wSceneObj9State, wSceneObj10State, wSceneObj11State, wSceneObj12State, wSceneObj13State,
		wSceneObj14State, wSceneObj15State})
		temple_obj(state);
}

// UpdatePrologueSceneObjects / Func_ac3fd: the prologue's
void SceneSprites::prologue_objs()
{
	for (uint16_t state : {wCurSceneObjState, wSceneObj1State, wSceneObj2State, wSceneObj3State,
		wSceneObj4State, wSceneObj5State})
		prologue_obj(state);
}

void SceneSprites::func_ac3fd()
{
	for (uint16_t state : {wSceneObj8State, wSceneObj9State})
		prologue_obj(state);
}

} // namespace wl3::overworld

// Called by the scenes (C++): Func_ac3fd last in Prologue, whose C caller
// (PrologueFunc) passes the registers on, so run() leaves them as the asm.
PORT_EX(Func_ac074, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).func_ac074(); }
PORT_EX(Func_ac080, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).func_ac080(); }
PORT_EX(Func_ac2b2, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).func_ac2b2(); }
PORT_EX(UpdateTempleSceneObjs, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).temple_objs(); }
PORT_EX(UpdatePrologueSceneObjects, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).prologue_objs(); }
PORT_EX(Func_ac3fd, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).func_ac3fd(); }
PORT_EX(Func_ac0b6, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).epilogue_obj(REG_BC(gb)); }
PORT_EX(UpdateTempleSceneObjAnimation, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).temple_obj(REG_BC(gb)); }
PORT_EX(Func_ac409, 0, ALL_REGISTERS) { wl3::overworld::SceneSprites(gb).prologue_obj(REG_BC(gb)); }

// Only the machines above run these.
PORT_COVERED(Func_ac000);
PORT_COVERED(Func_ac00b);
PORT_COVERED(Func_ac016);
PORT_COVERED(Func_ac021);
PORT_COVERED(Func_ac02c);
PORT_COVERED(Func_ac037);
PORT_COVERED(Func_ac042);
PORT_COVERED(Func_ac047);
PORT_COVERED(Func_ac04c);
PORT_COVERED(Func_ac051);
PORT_COVERED(Func_ac056);
PORT_COVERED(SetObjStateIfAnimationIsFinished);
