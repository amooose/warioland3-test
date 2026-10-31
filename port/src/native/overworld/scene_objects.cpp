// The map's scene objects (bank2d.asm, Func_b4a3d and Func_b4aa9): each
// side's objects run a script by their state, an animation and a sprite,
// some going on to a next state when their animation ends. The objects
// before Wario (Func_b4a3d: 1 to 4) and after (Func_b4aa9) are drawn
// around him. See overworld.hpp.
#include "overworld.hpp"

#include <iterator>

namespace wl3::overworld {

namespace {

constexpr uint8_t NO_NEXT = 0xff; // no next state
constexpr uint8_t OW_OBJ_BANK = 0x2a;

enum class Special : uint8_t {
	None,
	Palette, // Func_b4e8b: the palette (attributes) by day or night
	NextObj8, // Func_b4ea9: when wSceneObj7 is gone, wSceneObj8 in state 8
	BgPal5, // Func_b4f58: BG palette 5, lit or not by wSceneObj11
	SoundAtEnd, // Func_b4dc0: a sound when the animation ends
};

struct ObjAnim {
	uint16_t frameset = 0, oam = 0;
	uint8_t next = NO_NEXT;
	bool scroll = true;
	Special special = Special::None;
};

// Func_b4fab: by state
constexpr ObjAnim NORTH_SCRIPT[] = {
	{}, // 0x00
	{ADDR_Frameset_a85cd, ADDR_OAM_a8000}, // 0x01 Func_b5004
	{ADDR_Frameset_a8590, ADDR_OAM_a8000, 0x00}, // 0x02 Func_b5009
	{ADDR_Frameset_a85d0, ADDR_OAM_a8000}, // 0x03 Func_b500e
	{ADDR_Frameset_a85d3, ADDR_OAM_a8000, 0x00}, // 0x04 Func_b5013
	{ADDR_Frameset_a862a, ADDR_OAM_a8000}, // 0x05 Func_b5018
	{ADDR_Frameset_a8653, ADDR_OAM_a8000, 0x00}, // 0x06 Func_b501d
	{ADDR_Frameset_a862d, ADDR_OAM_a8000}, // 0x07 Func_b5022
	{ADDR_Frameset_a8639, ADDR_OAM_a8000, 0x09}, // 0x08 Func_b5027
	{ADDR_Frameset_a8636, ADDR_OAM_a8000}, // 0x09 Func_b5031
	{ADDR_Frameset_a8646, ADDR_OAM_a8000, 0x0b}, // 0x0a Func_b5036
	{ADDR_Frameset_a866e, ADDR_OAM_a8000}, // 0x0b Func_b5040
	{ADDR_Frameset_a867f, ADDR_OAM_a8000, 0x0e}, // 0x0c Func_b5045
	{ADDR_Frameset_a86da, ADDR_OAM_a8000, 0x0e}, // 0x0d Func_b504f
	{ADDR_Frameset_a872f, ADDR_OAM_a8000}, // 0x0e Func_b5059
	{ADDR_Frameset_a8732, ADDR_OAM_a8000}, // 0x0f Func_b505e
	{ADDR_Frameset_a8750, ADDR_OAM_a8000}, // 0x10 Func_b5063
	{ADDR_Frameset_a8761, ADDR_OAM_a8000}, // 0x11 Func_b5068
	{ADDR_Frameset_a879a, ADDR_OAM_a8000}, // 0x12 Func_b506d
	{}, // 0x13
	{ADDR_Frameset_a8764, ADDR_OAM_a8000}, // 0x14 Func_b5072
	{ADDR_Frameset_a876f, ADDR_OAM_a8000}, // 0x15 Func_b5077
	{ADDR_Frameset_a8782, ADDR_OAM_a8000, 0x18}, // 0x16 Func_b5093
	{ADDR_Frameset_a8793, ADDR_OAM_a8000, 0x19}, // 0x17 Func_b509d
	{ADDR_Frameset_a877a, ADDR_OAM_a8000}, // 0x18 Func_b50a7
	{}, // 0x19
	{ADDR_Frameset_a877d, ADDR_OAM_a8000}, // 0x1a Func_b50ac
	{}, // 0x1b
	{ADDR_Frameset_a879f, ADDR_OAM_a8000, 0x00}, // 0x1c Func_b50b1
	{ADDR_Frameset_a873d, ADDR_OAM_a8000, 0x0e}, // 0x1d Func_b50b6
	{ADDR_Frameset_a87d2, ADDR_OAM_a8000, 0x20}, // 0x1e Func_b50c0
	{ADDR_Frameset_a87d9, ADDR_OAM_a8000}, // 0x1f Func_b50ca
	{ADDR_Frameset_a87dc, ADDR_OAM_a8000}, // 0x20 Func_b50cf
	{ADDR_Frameset_a87df, ADDR_OAM_a8000}, // 0x21 Func_b50d4
	{ADDR_Frameset_aa5f5, ADDR_OAM_aa580}, // 0x22 Func_b50d9
	{ADDR_Frameset_aa5f8, ADDR_OAM_aa580}, // 0x23 Func_b50de
	{ADDR_Frameset_aa252, ADDR_OAM_aa0cf}, // 0x24 Func_b50ea
	{ADDR_Frameset_aa0a3, ADDR_OAM_a9fe2}, // 0x25 Func_b50f6
	{ADDR_Frameset_aa0c1, ADDR_OAM_a9fe2, 0x27}, // 0x26 Func_b50fb
	{ADDR_Frameset_aa09d, ADDR_OAM_a9fe2}, // 0x27 Func_b5105
	{ADDR_Frameset_aa0bc, ADDR_OAM_a9fe2, 0x25}, // 0x28 Func_b510a
	{ADDR_Frameset_a87e8, ADDR_OAM_a8000}, // 0x29 Func_b512a
	{ADDR_Frameset_a87f1, ADDR_OAM_a8000}, // 0x2a Func_b5130
	{ADDR_Frameset_a883b, ADDR_OAM_a87fa, 0x00, false}, // 0x2b Func_b5136
};

// Func_b4e04: by state
constexpr ObjAnim WEST_SCRIPT[] = {
	{}, // 0x00
	{ADDR_Frameset_a8de9, ADDR_OAM_a884a, 0x00}, // 0x01 Func_b4e77
	{ADDR_Frameset_a8de2, ADDR_OAM_a884a}, // 0x02 Func_b4e7c
	{ADDR_Frameset_a8dd1, ADDR_OAM_a884a}, // 0x03 Func_b4e81
	{ADDR_Frameset_a8e1e, ADDR_OAM_a884a, 0x00}, // 0x04 Func_b4e86
	{ADDR_Frameset_a8e29, ADDR_OAM_a884a, NO_NEXT, true, Special::Palette}, // 0x05 Func_b4e8b
	{ADDR_Frameset_a8e2c, ADDR_OAM_a884a}, // 0x06 Func_b4ea4
	{ADDR_Frameset_a8e5d, ADDR_OAM_a884a, 0x00, true, Special::NextObj8}, // 0x07 Func_b4ea9
	{ADDR_Frameset_a8e6c, ADDR_OAM_a884a}, // 0x08 Func_b4ebd
	{ADDR_Frameset_a8e29, ADDR_OAM_a884a, NO_NEXT, true, Special::Palette}, // 0x09 Func_b4e8b
	{ADDR_Frameset_a8df8, ADDR_OAM_a884a}, // 0x0a Func_b4ec2
	{ADDR_Frameset_a8e04, ADDR_OAM_a884a, 0x0c}, // 0x0b Func_b4ec8
	{ADDR_Frameset_a8e01, ADDR_OAM_a884a}, // 0x0c Func_b4ed2
	{ADDR_Frameset_a8e11, ADDR_OAM_a884a, 0x0e}, // 0x0d Func_b4ed8
	{ADDR_Frameset_a8f17, ADDR_OAM_a884a}, // 0x0e Func_b4ee3
	{ADDR_Frameset_a9090, ADDR_OAM_a8f57, 0x11}, // 0x0f Func_b4ee9
	{ADDR_Frameset_a90eb, ADDR_OAM_a8f57, 0x11}, // 0x10 Func_b4ef4
	{ADDR_Frameset_a9140, ADDR_OAM_a8f57}, // 0x11 Func_b4eff
	{ADDR_Frameset_a9143, ADDR_OAM_a8f57}, // 0x12 Func_b4f05
	{ADDR_Frameset_a914e, ADDR_OAM_a8f57, 0x11}, // 0x13 Func_b4f0b
	{ADDR_Frameset_a8e9d, ADDR_OAM_a884a, 0x00}, // 0x14 Func_b4f16
	{ADDR_Frameset_a8eac, ADDR_OAM_a884a, 0x00}, // 0x15 Func_b4f1c
	{ADDR_Frameset_a8eb7, ADDR_OAM_a884a, 0x00}, // 0x16 Func_b4f22
	{ADDR_Frameset_a8ec2, ADDR_OAM_a884a}, // 0x17 Func_b4f28
	{ADDR_Frameset_a8ec7, ADDR_OAM_a884a}, // 0x18 Func_b4f2e
	{ADDR_Frameset_a8eca, ADDR_OAM_a884a}, // 0x19 Func_b4f34
	{ADDR_Frameset_a8eca, ADDR_OAM_a884a, 0x00}, // 0x1a Func_b4f3a
	{ADDR_Frameset_a8ed3, ADDR_OAM_a884a}, // 0x1b Func_b4f40
	{ADDR_Frameset_a8eda, ADDR_OAM_a884a}, // 0x1c Func_b4f46
	{ADDR_Frameset_a8ee1, ADDR_OAM_a884a}, // 0x1d Func_b4f4c
	{ADDR_Frameset_a8ee8, ADDR_OAM_a884a, NO_NEXT, true, Special::BgPal5}, // 0x1e Func_b4f52
	{ADDR_Frameset_a8ee8, ADDR_OAM_a884a, 0x00, true, Special::BgPal5}, // 0x1f Func_b4f82
	{ADDR_Frameset_a8f28, ADDR_OAM_a884a}, // 0x20 Func_b4f8a
	{ADDR_Frameset_a8f31, ADDR_OAM_a884a}, // 0x21 Func_b4f90
	{ADDR_Frameset_a8f47, ADDR_OAM_a884a}, // 0x22 Func_b4f96
	{ADDR_Frameset_a8f3a, ADDR_OAM_a884a}, // 0x23 Func_b4f9c
};

// Func_b4ccb: by state
constexpr ObjAnim SOUTH_SCRIPT[] = {
	{}, // 0x00
	{ADDR_Frameset_a96c0, ADDR_OAM_a9161}, // 0x01 Func_b4d0e
	{ADDR_Frameset_a968d, ADDR_OAM_a9161, 0x01}, // 0x02 Func_b4d14
	{ADDR_Frameset_a967d, ADDR_OAM_a9161}, // 0x03 Func_b4d1f
	{ADDR_Frameset_a9680, ADDR_OAM_a9161}, // 0x04 Func_b4d25
	{ADDR_Frameset_a9674, ADDR_OAM_a9161}, // 0x05 Func_b4d2b
	{ADDR_Frameset_a9680, ADDR_OAM_a9161, 0x03}, // 0x06 Func_b4d31
	{ADDR_Frameset_a96af, ADDR_OAM_a9161}, // 0x07 Func_b4d3c
	{ADDR_Frameset_a96b2, ADDR_OAM_a9161, 0x09}, // 0x08 Func_b4d41
	{ADDR_Frameset_a96bd, ADDR_OAM_a9161}, // 0x09 Func_b4d4b
	{ADDR_Frameset_a96d1, ADDR_OAM_a9161}, // 0x0a Func_b4d50
	{ADDR_Frameset_a969a, ADDR_OAM_a9161}, // 0x0b Func_b4d55
	{ADDR_Frameset_a96a3, ADDR_OAM_a9161}, // 0x0c Func_b4d5a
	{ADDR_Frameset_a96a6, ADDR_OAM_a9161}, // 0x0d Func_b4d5f
	{ADDR_Frameset_a96a9, ADDR_OAM_a9161}, // 0x0e Func_b4d64
	{ADDR_Frameset_a96ac, ADDR_OAM_a9161}, // 0x0f Func_b4d69
	{ADDR_Frameset_a96d8, ADDR_OAM_a9161}, // 0x10 Func_b4d6e
	{ADDR_Frameset_a96e1, ADDR_OAM_a9161, 0x00}, // 0x11 Func_b4d73
	{ADDR_Frameset_a96ea, ADDR_OAM_a9161, 0x00}, // 0x12 Func_b4d78
	{ADDR_Frameset_a96ef, ADDR_OAM_a9161, 0x15}, // 0x13 Func_b4d7d
	{ADDR_Frameset_a974a, ADDR_OAM_a9161, 0x15}, // 0x14 Func_b4d87
	{ADDR_Frameset_a979f, ADDR_OAM_a9161}, // 0x15 Func_b4d91
	{ADDR_Frameset_a97a2, ADDR_OAM_a9161}, // 0x16 Func_b4d96
	{ADDR_Frameset_a97ad, ADDR_OAM_a9161, 0x00}, // 0x17 Func_b4d9b
	{ADDR_Frameset_a97e1, ADDR_OAM_a9161}, // 0x18 Func_b4db6
	{ADDR_Frameset_a97ec, ADDR_OAM_a9161}, // 0x19 Func_b4dbb
	{ADDR_Frameset_a97d4, ADDR_OAM_a9161, NO_NEXT, true, Special::SoundAtEnd}, // 0x1a Func_b4dc0
	{ADDR_Frameset_a97c5, ADDR_OAM_a9161, 0x00}, // 0x1b Func_b4ddd
	{ADDR_Frameset_a97ef, ADDR_OAM_a9161, 0x00}, // 0x1c Func_b4de2
	{ADDR_Frameset_a97b2, ADDR_OAM_a9161, 0x15}, // 0x1d Func_b4de7
	{ADDR_Frameset_a97fe, ADDR_OAM_a9161}, // 0x1e Func_b4df1
	{}, // 0x1f
	{ADDR_Frameset_a9803, ADDR_OAM_a9161}, // 0x20 Func_b4df6
};

// Func_b4ba2: by state
constexpr ObjAnim EAST_SCRIPT[] = {
	{}, // 0x00
	{ADDR_Frameset_a9e61, ADDR_OAM_a980c}, // 0x01 Func_b4be9
	{ADDR_Frameset_a9e72, ADDR_OAM_a980c}, // 0x02 Func_b4bee
	{ADDR_Frameset_a9e75, ADDR_OAM_a980c}, // 0x03 Func_b4bf3
	{ADDR_Frameset_a9e94, ADDR_OAM_a980c}, // 0x04 Func_b4bf8
	{ADDR_Frameset_a9e7a, ADDR_OAM_a980c, 0x00}, // 0x05 Func_b4bfd
	{ADDR_Frameset_a9e8c, ADDR_OAM_a980c}, // 0x06 Func_b4c02
	{ADDR_Frameset_a9e91, ADDR_OAM_a980c}, // 0x07 Func_b4c07
	{ADDR_Frameset_a9fd0, ADDR_OAM_a980c, 0x00}, // 0x08 Func_b4c0c
	{ADDR_Frameset_a9ed3, ADDR_OAM_a980c}, // 0x09 Func_b4c16
	{ADDR_Frameset_a9eea, ADDR_OAM_a980c, 0x0b}, // 0x0a Func_b4c1b
	{ADDR_Frameset_a9ef1, ADDR_OAM_a980c}, // 0x0b Func_b4c25
	{ADDR_Frameset_a9ee3, ADDR_OAM_a980c}, // 0x0c Func_b4c2a
	{ADDR_Frameset_a9ef6, ADDR_OAM_a980c, 0x0e}, // 0x0d Func_b4c2f
	{ADDR_Frameset_a9efd, ADDR_OAM_a980c}, // 0x0e Func_b4c39
	{ADDR_Frameset_a9ee0, ADDR_OAM_a980c}, // 0x0f Func_b4c3e
	{ADDR_Frameset_a9f02, ADDR_OAM_a980c}, // 0x10 Func_b4c43
	{ADDR_Frameset_a9f0d, ADDR_OAM_a980c}, // 0x11 Func_b4c48
	{ADDR_Frameset_a9ecd, ADDR_OAM_a980c}, // 0x12 Func_b4c4d
	{ADDR_Frameset_a9ed0, ADDR_OAM_a980c}, // 0x13 Func_b4c52
	{ADDR_Frameset_a9eaa, ADDR_OAM_a980c}, // 0x14 Func_b4c6d
	{ADDR_Frameset_a9eb3, ADDR_OAM_a980c}, // 0x15 Func_b4c72
	{ADDR_Frameset_a9ebc, ADDR_OAM_a980c}, // 0x16 Func_b4c77
	{ADDR_Frameset_a9ec5, ADDR_OAM_a980c}, // 0x17 Func_b4c7c
	{ADDR_Frameset_a9ec8, ADDR_OAM_a980c, 0x00}, // 0x18 Func_b4c81
	{ADDR_Frameset_a9f36, ADDR_OAM_a980c}, // 0x19 Func_b4c86
	{ADDR_Frameset_a9f47, ADDR_OAM_a980c, 0x1b}, // 0x1a Func_b4c8b
	{ADDR_Frameset_a9f76, ADDR_OAM_a980c, 0x19}, // 0x1b Func_b4c95
	{ADDR_Frameset_a9fa7, ADDR_OAM_a980c}, // 0x1c Func_b4c9f
	{ADDR_Frameset_a9faa, ADDR_OAM_a980c, 0x1c}, // 0x1d Func_b4ca4
	{ADDR_Frameset_a9fc9, ADDR_OAM_a980c}, // 0x1e Func_b4cae
	{ADDR_Frameset_a9fbb, ADDR_OAM_a980c}, // 0x1f Func_b4cb3
	{ADDR_Frameset_a9fc2, ADDR_OAM_a980c}, // 0x20 Func_b4cb8
	{ADDR_Frameset_a9fdf, ADDR_OAM_a980c}, // 0x21 Func_b4cbd
	{ADDR_Frameset_a9e99, ADDR_OAM_a980c, 0x00}, // 0x22 Func_b4c11
};

// the objects each side runs, before and after Wario
constexpr uint16_t BEFORE[] = {wSceneObj1State, wSceneObj2State, wSceneObj3State, wSceneObj4State};
constexpr uint16_t NORTH_AFTER[] = {wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
	wSceneObj9State, wSceneObj10State, wSceneObj13State, wSceneObj14State, wSceneObj15State};
constexpr uint16_t WEST_AFTER[] = {wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
	wSceneObj10State, wSceneObj11State, wSceneObj12State, wSceneObj13State, wSceneObj14State, wSceneObj15State};
constexpr uint16_t SOUTH_AFTER[] = {wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
	wSceneObj9State, wSceneObj10State, wSceneObj14State, wSceneObj15State};
constexpr uint16_t EAST_AFTER[] = {wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
	wSceneObj9State, wSceneObj10State, wSceneObj11State, wSceneObj12State, wSceneObj13State, wSceneObj14State,
	wSceneObj15State};

struct Script {
	const ObjAnim *anims;
	uint8_t size;
};
template <size_t N>
constexpr Script script(const ObjAnim (&a)[N])
{
	return {a, static_cast<uint8_t>(N)};
}
constexpr Script SCRIPTS[4] = {script(NORTH_SCRIPT), script(WEST_SCRIPT), script(SOUTH_SCRIPT), script(EAST_SCRIPT)};

constexpr uint16_t BG_PAL5_LIT = ADDR_Func_b4f58 + 0x22; // Func_b4f58.pal

} // namespace

void SceneObjects::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// Func_b4a3d
void SceneObjects::update_before()
{
	uint8_t side = wram_.cur_map_side();
	if (side >= 4) {
		unrealistic();
		return;
	}
	for (uint16_t state : BEFORE)
		run(side, state);
}

// Func_b4aa9
void SceneObjects::update_after()
{
	uint8_t side = wram_.cur_map_side();
	auto all = [&](const uint16_t *states, size_t n) {
		for (size_t i = 0; i < n; i++)
			run(side, states[i]);
	};
	switch (side) {
	case NORTH: all(NORTH_AFTER, std::size(NORTH_AFTER)); return;
	case WEST: all(WEST_AFTER, std::size(WEST_AFTER)); return;
	case SOUTH: all(SOUTH_AFTER, std::size(SOUTH_AFTER)); return;
	case EAST: all(EAST_AFTER, std::size(EAST_AFTER)); return;
	default: unrealistic(); return;
	}
}

// Func_b4fa2 and the like: the object whose state is at state, by its
// side's script
void SceneObjects::run(uint8_t side, uint16_t state)
{
	// wCurSceneObjPtr: the object (ld a, c / and $f8)
	wram_.cur_scene_obj_ptr().word() = static_cast<uint16_t>(state & 0xfff8);
	run_state(side, state, mem_[state]);
}

// Func_b4fab and the like: the script's entry for state s. An entry that
// goes on to a next state runs that one at once (the asm's jumptable again);
// a special's own code comes after all that.
void SceneObjects::run_state(uint8_t side, uint16_t state, uint8_t s)
{
	const Script &script = SCRIPTS[side];
	uint16_t obj = static_cast<uint16_t>(state & 0xfff8);
	for (;;) {
		if (s >= script.size) {
			unrealistic();
			return;
		}
		const ObjAnim &anim = script.anims[s];
		if (!anim.frameset) // Func_b4000
			return;
		// UpdateCurOWObjAnimation
		SET_DE(gb_, anim.frameset);
		SET_HL(gb_, static_cast<uint16_t>(obj | 1 << 2));
		gb_->cpu.b = OW_OBJ_BANK;
		routines_.call(ADDR_UpdateOWAnimation);
		bool finished = wram_.ow_animation_finished() == 0xff;
		if (anim.special == Special::Palette) {
			// pop hl / set 0, l / res 2, l: from the state to the attributes
			uint16_t attr = static_cast<uint16_t>((state | 1) & ~(1 << 2));
			mem_[attr] = wram_.unk_2d011() ? 0x06 : 0x00;
		}
		if (anim.special == Special::SoundAtEnd && finished)
			play_sfx(mem_, SFX_12F); // Func_b4dd4
		if (anim.next != NO_NEXT && finished) {
			// Func_b4a0d: the next state, from its first frame
			uint16_t duration = static_cast<uint16_t>(obj | 1 << 2);
			mem_[duration] = 0;
			mem_[static_cast<uint16_t>(duration + 1)] = 0;
			mem_[static_cast<uint16_t>(duration + 2)] = anim.next;
			if (anim.special == Special::None) {
				s = anim.next;
				continue;
			}
			run_state(side, state, anim.next); // inside the special's call
			after(static_cast<uint8_t>(anim.special));
			return;
		}
		// LoadCurSceneObjSprite(WithScroll)
		SET_HL(gb_, wram_.cur_scene_obj_ptr().word());
		SET_DE(gb_, anim.oam);
		gb_->cpu.b = OW_OBJ_BANK;
		routines_.call(anim.scroll ? ADDR_LoadOWSpriteWithScroll : ADDR_LoadOWSprite);
		after(static_cast<uint8_t>(anim.special));
		return;
	}
}

// the specials' code after the entry
void SceneObjects::after(uint8_t special)
{
	switch (static_cast<Special>(special)) {
	case Special::NextObj8: // Func_b4ea9
		if (mem_[wSceneObj7State])
			return;
		mem_[wSceneObj8Duration] = 0;
		mem_[wSceneObj8State] = 0x08;
		return;
	case Special::BgPal5: { // Func_b4f58: BG palette 5 lit while wSceneObj11 shows
		uint16_t pal = wTempPals1 + 5 * PAL_SIZE;
		if (mem_[wSceneObj11Frame] != 0x57 && mem_[wSceneObj11State])
			pal = BG_PAL5_LIT;
		routines_.copy(pal, wTempBGPals + 5 * PAL_SIZE, PAL_SIZE);
		wram_.pal_config1() = static_cast<uint8_t>(rBGPI);
		return;
	}
	default:
		return;
	}
}

} // namespace wl3::overworld

// Only farcalled (MapWalk, Overworld, Scenes: C++, which reload what they
// use).
PORT_EX(Func_b4a37, 0, ALL_REGISTERS)
{
	wl3::overworld::SceneObjects objects(gb);
	objects.update_before();
	objects.update_after();
}

PORT_EX(Func_b4a3d, 0, ALL_REGISTERS)
{
	wl3::overworld::SceneObjects(gb).update_before();
}

PORT_EX(Func_b4aa9, 0, ALL_REGISTERS)
{
	wl3::overworld::SceneObjects(gb).update_after();
}

// The scripts' routines (bank2d.asm, Func_b4b99 to _DoOWFunction) and their
// helpers, all in the tables above.
PORT_COVERED(Func_b4b99);
PORT_COVERED(Func_b4ba2);
PORT_COVERED(Func_b4be9);
PORT_COVERED(Func_b4bee);
PORT_COVERED(Func_b4bf3);
PORT_COVERED(Func_b4bf8);
PORT_COVERED(Func_b4bfd);
PORT_COVERED(Func_b4c02);
PORT_COVERED(Func_b4c07);
PORT_COVERED(Func_b4c0c);
PORT_COVERED(Func_b4c11);
PORT_COVERED(Func_b4c16);
PORT_COVERED(Func_b4c1b);
PORT_COVERED(Func_b4c25);
PORT_COVERED(Func_b4c2a);
PORT_COVERED(Func_b4c2f);
PORT_COVERED(Func_b4c39);
PORT_COVERED(Func_b4c3e);
PORT_COVERED(Func_b4c43);
PORT_COVERED(Func_b4c48);
PORT_COVERED(Func_b4c4d);
PORT_COVERED(Func_b4c52);
PORT_COVERED(Func_b4c57);
PORT_COVERED(Func_b4c5a);
PORT_COVERED(Func_b4c60);
PORT_COVERED(Func_b4c65);
PORT_COVERED(Func_b4c6d);
PORT_COVERED(Func_b4c72);
PORT_COVERED(Func_b4c77);
PORT_COVERED(Func_b4c7c);
PORT_COVERED(Func_b4c81);
PORT_COVERED(Func_b4c86);
PORT_COVERED(Func_b4c8b);
PORT_COVERED(Func_b4c95);
PORT_COVERED(Func_b4c9f);
PORT_COVERED(Func_b4ca4);
PORT_COVERED(Func_b4cae);
PORT_COVERED(Func_b4cb3);
PORT_COVERED(Func_b4cb8);
PORT_COVERED(Func_b4cbd);
PORT_COVERED(Func_b4cc2);
PORT_COVERED(Func_b4ccb);
PORT_COVERED(Func_b4d0e);
PORT_COVERED(Func_b4d14);
PORT_COVERED(Func_b4d1f);
PORT_COVERED(Func_b4d25);
PORT_COVERED(Func_b4d2b);
PORT_COVERED(Func_b4d31);
PORT_COVERED(Func_b4d3c);
PORT_COVERED(Func_b4d41);
PORT_COVERED(Func_b4d4b);
PORT_COVERED(Func_b4d50);
PORT_COVERED(Func_b4d55);
PORT_COVERED(Func_b4d5a);
PORT_COVERED(Func_b4d5f);
PORT_COVERED(Func_b4d64);
PORT_COVERED(Func_b4d69);
PORT_COVERED(Func_b4d6e);
PORT_COVERED(Func_b4d73);
PORT_COVERED(Func_b4d78);
PORT_COVERED(Func_b4d7d);
PORT_COVERED(Func_b4d87);
PORT_COVERED(Func_b4d91);
PORT_COVERED(Func_b4d96);
PORT_COVERED(Func_b4d9b);
PORT_COVERED(Func_b4da0);
PORT_COVERED(Func_b4da3);
PORT_COVERED(Func_b4da9);
PORT_COVERED(Func_b4dae);
PORT_COVERED(Func_b4db6);
PORT_COVERED(Func_b4dbb);
PORT_COVERED(Func_b4dc0);
PORT_COVERED(Func_b4dd4);
PORT_COVERED(Func_b4ddd);
PORT_COVERED(Func_b4de2);
PORT_COVERED(Func_b4de7);
PORT_COVERED(Func_b4df1);
PORT_COVERED(Func_b4df6);
PORT_COVERED(Func_b4dfb);
PORT_COVERED(Func_b4e04);
PORT_COVERED(Func_b4e4d);
PORT_COVERED(Func_b4e50);
PORT_COVERED(Func_b4e56);
PORT_COVERED(Func_b4e5b);
PORT_COVERED(Func_b4e62);
PORT_COVERED(Func_b4e65);
PORT_COVERED(Func_b4e6b);
PORT_COVERED(Func_b4e70);
PORT_COVERED(Func_b4e77);
PORT_COVERED(Func_b4e7c);
PORT_COVERED(Func_b4e81);
PORT_COVERED(Func_b4e86);
PORT_COVERED(Func_b4e8b);
PORT_COVERED(Func_b4ea4);
PORT_COVERED(Func_b4ea9);
PORT_COVERED(Func_b4ebd);
PORT_COVERED(Func_b4ec2);
PORT_COVERED(Func_b4ec8);
PORT_COVERED(Func_b4ed2);
PORT_COVERED(Func_b4ed8);
PORT_COVERED(Func_b4ee3);
PORT_COVERED(Func_b4ee9);
PORT_COVERED(Func_b4ef4);
PORT_COVERED(Func_b4eff);
PORT_COVERED(Func_b4f05);
PORT_COVERED(Func_b4f0b);
PORT_COVERED(Func_b4f16);
PORT_COVERED(Func_b4f1c);
PORT_COVERED(Func_b4f22);
PORT_COVERED(Func_b4f28);
PORT_COVERED(Func_b4f2e);
PORT_COVERED(Func_b4f34);
PORT_COVERED(Func_b4f3a);
PORT_COVERED(Func_b4f40);
PORT_COVERED(Func_b4f46);
PORT_COVERED(Func_b4f4c);
PORT_COVERED(Func_b4f52);
PORT_COVERED(Func_b4f58);
PORT_COVERED(Func_b4f82);
PORT_COVERED(Func_b4f8a);
PORT_COVERED(Func_b4f90);
PORT_COVERED(Func_b4f96);
PORT_COVERED(Func_b4f9c);
PORT_COVERED(Func_b4fa2);
PORT_COVERED(Func_b4fab);
PORT_COVERED(Func_b5004);
PORT_COVERED(Func_b5009);
PORT_COVERED(Func_b500e);
PORT_COVERED(Func_b5013);
PORT_COVERED(Func_b5018);
PORT_COVERED(Func_b501d);
PORT_COVERED(Func_b5022);
PORT_COVERED(Func_b5027);
PORT_COVERED(Func_b5031);
PORT_COVERED(Func_b5036);
PORT_COVERED(Func_b5040);
PORT_COVERED(Func_b5045);
PORT_COVERED(Func_b504f);
PORT_COVERED(Func_b5059);
PORT_COVERED(Func_b505e);
PORT_COVERED(Func_b5063);
PORT_COVERED(Func_b5068);
PORT_COVERED(Func_b506d);
PORT_COVERED(Func_b5072);
PORT_COVERED(Func_b5077);
PORT_COVERED(Func_b507c);
PORT_COVERED(Func_b507f);
PORT_COVERED(Func_b5085);
PORT_COVERED(Func_b508a);
PORT_COVERED(Func_b5093);
PORT_COVERED(Func_b509d);
PORT_COVERED(Func_b50a7);
PORT_COVERED(Func_b50ac);
PORT_COVERED(Func_b50b1);
PORT_COVERED(Func_b50b6);
PORT_COVERED(Func_b50c0);
PORT_COVERED(Func_b50ca);
PORT_COVERED(Func_b50cf);
PORT_COVERED(Func_b50d4);
PORT_COVERED(Func_b50d9);
PORT_COVERED(Func_b50de);
PORT_COVERED(Func_b50e1);
PORT_COVERED(Func_b50ea);
PORT_COVERED(Func_b50f6);
PORT_COVERED(Func_b50fb);
PORT_COVERED(Func_b5105);
PORT_COVERED(Func_b510a);
PORT_COVERED(Func_b5114);
PORT_COVERED(Func_b512a);
PORT_COVERED(Func_b5130);
PORT_COVERED(Func_b5136);
PORT_COVERED(UpdateCurOWObjAnimation);
PORT_COVERED(Func_b4a0d);
PORT_COVERED(LoadCurSceneObjSpriteWithScroll);
PORT_COVERED(LoadCurSceneObjSprite);
