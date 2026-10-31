// Cutscene14Func: the volcano erupts. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

namespace {

// .Func_9ee5b's three rocks bob along .data_9ee99 (Data_9ef82 later),
// each from its own pointer, kept in wPalFade1's bytes
constexpr uint16_t BOB_DATA = ADDR_Cutscene14Func + 0x22d; // .data_9ee99
constexpr uint8_t BOB_END = 0x80;
static_assert(wPalFade1Colour1RedSign == wPalFade1 + 2 && wPalFade1Colour1RedUnk1 == wPalFade1 + 4);

constexpr uint16_t VOLCANO_FRAMESETS = ADDR_Func_9fba9 + 0x45; // Func_9fba9.framesets

} // namespace

// InitCutscene14
void Cutscenes::init_volcano()
{
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	load_pals2(BANK_Pals_b8a40, ADDR_Pals_b8a40);
	wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
	routines_.call(ADDR_LoadCutscenes2Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps01);
	routines_.call(ADDR_LoadCutsceneBGMaps05);
}

// Cutscene14Func: the item rises with three rocks bobbing around it, and
// sinks; the volcano rumbles, erupts and the eruption rises
void Cutscenes::volcano()
{
	auto set_bob_pointers = [&](std::initializer_list<uint8_t> bytes) {
		uint16_t at = wPalFade1;
		for (uint8_t b : bytes)
			mem_[at++] = b;
	};
	// .Func_9edce: a rumble every 10 frames, deeper from its frame 9
	auto rumble = [&] {
		Byte c = wram_.colour_fade_diff();
		++c;
		if (c < 0x0a)
			return;
		c = 0;
		play_sfx(mem_, mem_[wSceneObj5Frame] < 0x09 ? SFX_110 : SFX_111);
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9ec9b
		place(wSceneObj2, 120, 82, 0x01);
		place(wSceneObj3, 145, 68, 0x02);
		place(wSceneObj4, 145, 96, 0x03);
		mem_[wCurSceneObj] = 0xbc; // the shadow (Func_9fb87)
		mem_[wCurSceneObj + 1] = 0x50;
		mem_[wSceneObj1YCoord] = 2;
		set_bob_pointers({0x99, 0x6e, 0xe7, 0x6e, 0x33, 0x6f});
		advance();
		break;
	case 2: { // .Func_9ecef: all up, on even frames
		bob_rocks();
		if (wram_.global_counter() & 1)
			break;
		--mem_[wSceneObj2YCoord];
		--mem_[wSceneObj3YCoord];
		--mem_[wSceneObj4YCoord];
		uint8_t y = --mem_[wCurSceneObj];
		if (y == 0x87) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x74)
			advance();
		break;
	}
	case 3: // .Func_9ed14
		bob_rocks();
		wait(60);
		break;
	case 4: // .Func_9ed1c: all down, every 4 frames
		bob_rocks();
		if (wram_.global_counter() & 0x03)
			break;
		++mem_[wSceneObj2YCoord];
		++mem_[wSceneObj3YCoord];
		++mem_[wSceneObj4YCoord];
		if (++mem_[wCurSceneObj] == 0x82)
			advance();
		break;
	case 5: // .Func_9ed3c: the shadow on down
		bob_rocks();
		if (wram_.global_counter() & 0x03)
			break;
		if (++mem_[wCurSceneObj] != 0x92)
			break;
		mem_[wSceneObj1YCoord] = 0;
		advance();
		break;
	case 6: // .Func_9ed54: at the bob's end, the next paths
		if (!bob_rocks())
			break;
		set_bob_pointers({0x82, 0x6f, 0xe3, 0x6f, 0x44, 0x70});
		advance();
		break;
	case 7: // .Func_9ed70
		if (bob_rocks())
			advance();
		break;
	case 8: // .Func_9ed77
		wait(30);
		break;
	case 9: // .Func_9ed7c: the volcano
		place(wSceneObj5, 76, 80, 0x04);
		mem_[wSceneObj2State] = 0;
		mem_[wSceneObj3State] = 0;
		mem_[wSceneObj4State] = 0;
		wram_.colour_fade_diff() = 0;
		advance();
		break;
	case 10: { // .Func_9ed9c: rumbling (.Func_9eda4)
		Byte c = wram_.colour_fade_diff();
		++c;
		if (c >= 0x0a) {
			c = 0;
			play_sfx(mem_, SFX_10F);
		}
		wait(120);
		break;
	}
	case 11: // .Func_9edb7
		set_state(wSceneObj5, 0x05);
		advance();
		break;
	case 12: // .Func_9edc2
		rumble();
		if (mem_[wSceneObj5State] == 0x06)
			advance();
		break;
	case 13: // .Func_9edf1
		rumble();
		wait(90);
		break;
	case 14: { // .Func_9edf9: the eruption down, faster and faster
		rumble();
		uint8_t f = wram_.unk_2d014();
		uint8_t dy = f < 0x08 ? 1 : f < 0x10 ? 2 : 3;
		Byte y = mem_[wSceneObj5];
		y += dy;
		if (y >= 0x70)
			advance();
		break;
	}
	case 15: // .Func_9ee17
		rumble();
		if (wram_.colour_fade_diff() == 0x09)
			advance();
		break;
	case 16: { // .Func_9ee23: the last rumbles, and the fade
		Byte c = wram_.colour_fade_diff(); // .Func_9ee2a
		++c;
		if (c == 0x0a)
			play_sfx(mem_, SFX_114);
		else if (c == 0x14)
			play_sfx(mem_, SFX_115);
		else if (c == 0x1e)
			play_sfx(mem_, SFX_116);
		fade_in();
		break;
	}
	case 17: // .Func_9ee57
		finish();
		break;
	default:
		unrealistic();
		return;
	}
	draw_volcano();
}

// .Func_9ee5b: wSceneObj4, 3 and 2 each a step along their path
// (.Func_9ee77); whether wSceneObj2's path came round (the asm's carry)
bool Cutscenes::bob_rocks()
{
	auto step = [&](uint16_t ptr, uint16_t obj) {
		uint16_t hi = static_cast<uint16_t>((ptr & 0xff00) | static_cast<uint8_t>(ptr + 1)); // inc c
		uint16_t de = static_cast<uint16_t>(mem_[ptr] | mem_[hi] << 8);
		mem_[obj] += mem_[de];
		mem_[static_cast<uint16_t>(obj + 1)] += mem_[static_cast<uint16_t>(de + 1)];
		de = static_cast<uint16_t>(de + 2);
		bool round = mem_[de] == BOB_END;
		if (round)
			de = BOB_DATA;
		mem_[ptr] = static_cast<uint8_t>(de); // .Func_9ee92
		mem_[hi] = static_cast<uint8_t>(de >> 8);
		return round;
	};
	step(wPalFade1Colour1RedUnk1, wSceneObj4);
	step(wPalFade1Colour1RedSign, wSceneObj3);
	return step(wPalFade1, wSceneObj2);
}

// Func_9fba9: the shadow, then wSceneObj4, 2, 3 and 5; state 5 (the volcano
// erupting) ends in 6
void Cutscenes::draw_volcano()
{
	if (mem_[wSceneObj1YCoord] == 0x02) { // Func_9fb87
		mem_[wCurSceneObjFrame] = 0x04;
		mem_[wCurSceneObjAttributes] = 0;
		draw(static_cast<uint16_t>(wCurSceneObjAttributes & 0xfffc), ADDR_OAM_bc000); // Func_9fb61
	}
	animate_all({wSceneObj4State, wSceneObj2State, wSceneObj3State, wSceneObj5State}, VOLCANO_FRAMESETS,
		ADDR_OAM_bcfb3, {{0x05, 0x06}});
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene14);
PORT_COVERED(Cutscene14Func);
PORT_COVERED(Func_9fba9);
PORT_COVERED(Func_9fb87);
