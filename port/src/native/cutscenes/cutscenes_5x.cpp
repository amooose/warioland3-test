// Cutscenes $51 to $58. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

// InitCutscene51, Func_9e655
void Cutscenes::init_mix_chemicals()
{
	load_pals1(BANK_Pals_b8400, ADDR_Pals_b8400);
	load_pals2(BANK_Pals_b8400, ADDR_Pals_b8400);
	routines_.call(ADDR_Func_9c931);
	routines_.call(ADDR_LoadCutsceneBGMaps18);
	routines_.call(ADDR_LoadCutsceneBGMaps19);
	routines_.call(ADDR_SetBGPriorityOnTopRows_Vram0);
	place(wSceneObj2, 0x40, 0x58, 0x02);
	place(wSceneObj3, 0x58, 0x58, 0x01);
	draw_func_9f8da();
}

// Cutscene51Func: the flask comes down and pours; the mix sinks, bubbling
void Cutscenes::mix_chemicals()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: { // .Func_9e688: down on even frames
		if (frame() & 1)
			break;
		uint8_t y = ++mem_[wSceneObj2YCoord];
		if (y == 0x45) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x58)
			advance();
		break;
	}
	case 2: // .Func_9e69c
		set_state(wSceneObj2, 0x03);
		advance();
		break;
	case 3: // .Func_9e6a7
		if (mem_[wSceneObj2Frame] != 0x04)
			break;
		play_sfx(mem_, SFX_0A1);
		advance();
		break;
	case 4: // .Func_9e6b8
		if (!mem_[wSceneObj2State])
			advance();
		break;
	case 5: // .Func_9e6c0
		wait(45);
		break;
	case 6: // .Func_9e6c5: down a pixel every 4 frames, bubbling
		loop_sound(wSceneObj3Unk7, 0x10, SFX_01F); // .Func_9e6d7
		if (frame() & 0x03)
			break;
		if (++mem_[wSceneObj3YCoord] == 0x78)
			advance();
		break;
	case 7:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f8da();
}

// Func_9f8da: wSceneObj2 and 3. State 3 (pouring) ends in 0; at its frame 7
// the BG map flips, once (wSceneObj2Unk7).
void Cutscenes::draw_func_9f8da()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State}) {
		uint8_t s = mem_[state];
		if (!s)
			continue;
		uint16_t obj = animate(state, frameset(ADDR_Func_9f8da + 0x3f, s));
		if (mem_[state] != 0x03) {
			draw(obj, ADDR_OAM_bf2ab);
			continue;
		}
		// .Func_9f912: when it ends, state 0 (and nothing to draw)
		if (!next_state_when_done(obj, 0x00))
			draw(obj, ADDR_OAM_bf2ab);
		Byte flipped = mem_[wSceneObj2Unk7];
		if (flipped || mem_[wSceneObj2Frame] != 0x07)
			continue;
		wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
		flipped = 0x01;
	}
}

// InitCutscene54, Func_9e2b8
void Cutscenes::init_cut_balloon()
{
	routines_.call(ADDR_LoadCutscenes9Gfx_Vram1);
	gb_->cpu.b = BANK_LevelMainTiles24;
	SET_HL(gb_, ADDR_LevelMainTiles24);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps36);
	routines_.call(ADDR_ClearTempPals_Bank27);
	place(wSceneObj2, 0x58, 0x50, 0x01);
	place(wSceneObj3, 0x70, 0x50, 0x02);
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f73b + 0x1b, ADDR_OAM_becdc);
}

// Cutscene54Func: fade in; the scissors rise and cut; the balloon floats off
void Cutscenes::cut_balloon()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e2ed
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b8740, ADDR_Pals_b8740);
		load_pals2(BANK_Pals_b8740, ADDR_Pals_b8740);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e303
		wait(40);
		break;
	case 4: { // .Func_9e308: up on even frames
		if (frame() & 1)
			break;
		uint8_t y = --mem_[wSceneObj3YCoord];
		if (y == 0x6b) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x58)
			advance();
		break;
	}
	case 5: // .Func_9e31c
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj3, 0x03);
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 6: // .Func_9e333
		wait(20);
		break;
	case 7: // .Func_9e338: up, 3 frames out of 4
		if (!(frame() & 0x03))
			break;
		if (--mem_[wSceneObj2YCoord] == 0x20)
			advance();
		break;
	case 8:
		end(20);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f73b
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f73b + 0x1b, ADDR_OAM_becdc);
}

// InitCutscene56, Func_9e6ef
void Cutscenes::init_reveal_dark_room()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	gb_->cpu.b = BANK_LevelMainTiles11;
	SET_HL(gb_, ADDR_LevelMainTiles11);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_Func_9c97d);
	routines_.call(ADDR_LoadCutsceneBGMaps24);
	routines_.call(ADDR_LoadCutsceneBGMaps25);
	place(wSceneObj2, 0x56, 0x50, 0x01);
	draw_func_9f91f();
}

// Cutscene56Func: fade in; the lamp lights, and the room brightens in two
// fades
void Cutscenes::reveal_dark_room()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e71f
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b9080, ADDR_Pals_b9080);
		load_pals2(BANK_Pals_b9080, ADDR_Pals_b9080);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e735
		if (frame() < 0x2d)
			break;
		place(wSceneObj2, 0x56, 0x50, 0x01);
		advance();
		break;
	case 4: // .Func_9e747
		if (mem_[wSceneObj2State] == 0x02)
			advance();
		break;
	case 5: // .Func_9e750
		if (mem_[wSceneObj2State] == 0x01)
			advance();
		break;
	case 6: // .Func_9e759
		if (frame() < 0x14)
			break;
		mem_[wSceneObj2State] = 0;
		wram_.collection_row() = 0x08;
		copy_far(BANK_Pals_b8800, ADDR_Pals_b8800, wTempPals1, 8 * PAL_SIZE);
		set_bg_pals_to_load();
		advance();
		break;
	case 7: // .Func_9e779
		next_fade(BANK_Pals_b8840, ADDR_Pals_b8840);
		break;
	case 8: // .Func_9e787: the fade, on even frames
		if (!(frame() & 1))
			routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 9: // .Func_9e78e
		next_fade(BANK_Pals_b8840, ADDR_Pals_b8880);
		break;
	case 10: // .Func_9e793: every 4 frames
		if (!(frame() & 0x03))
			routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 11:
		finish();
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f91f();
}

// Func_9f91f: the lamp flickers between states 1 and 2, with sounds at its
// frames 2 (and 4 or 5), once each (wSceneObj2Unk7)
void Cutscenes::draw_func_9f91f()
{
	for (;;) {
		uint8_t s = mem_[wSceneObj2State];
		if (!s)
			return;
		uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9f91f + 0x30, s));
		uint8_t now = mem_[wSceneObj2State];
		if (now == 0x01 || now == 0x02) {
			// .Func_9f953
			uint8_t frame = mem_[wSceneObj2Frame];
			Byte heard = mem_[wSceneObj2Unk7];
			bool key = frame == 0x02 || frame == 0x04 || frame == 0x05;
			if (heard) {
				if (!key)
					heard = static_cast<uint8_t>(heard ^ 1);
			} else if (key) {
				play_sfx(mem_, frame == 0x02 ? SFX_011 : SFX_0F1);
				heard = static_cast<uint8_t>(heard ^ 1);
			}
			if (next_state_when_done(obj, now == 0x01 ? 0x02 : 0x01))
				continue;
		}
		draw(obj, ADDR_OAM_bf460);
		return;
	}
}

// InitCutscene57, Func_9e79a
void Cutscenes::init_summon_moon()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	routines_.call(ADDR_Func_9c937);
	routines_.call(ADDR_LoadCutsceneBGMaps29);
	routines_.call(ADDR_SetBGPriorityOnTopRows_Vram0);
	place(wSceneObj3, 0x4e, 0x54, 0x01);
	place(wSceneObj2, 0x10, 0x48, 0x03);
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f991 + 0x28, ADDR_OAM_bf611, {{0x04, 0x03}});
}

// Cutscene57Func: fade in; the item drops in and is used; night falls in
// three fades
void Cutscenes::summon_moon()
{
	auto fade_on_even_frames = [&] { // .Func_9e844
		if (!(frame() & 1))
			routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e7d9
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b8540, ADDR_Pals_b8540);
		load_pals2(BANK_Pals_b8540, ADDR_Pals_b8540);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: { // .Func_9e7ef: down
		uint8_t y = ++mem_[wSceneObj2YCoord];
		if (y == 0x2c) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x52)
			advance();
		break;
	}
	case 4: // .Func_9e7ff
		if (frame() < 0x32)
			break;
		set_state(wSceneObj2, 0x04);
		advance();
		break;
	case 5: // .Func_9e80e
		if (mem_[wSceneObj2State] != 0x03)
			break;
		set_state(wSceneObj3, 0x02);
		play_sfx(mem_, SFX_105);
		advance();
		break;
	case 6: // .Func_9e827
		if (frame() < 0x30)
			break;
		set_state(wSceneObj3, 0x01);
		advance();
		break;
	case 7: // .Func_9e836
		next_fade(BANK_Pals_b8580, ADDR_Pals_b8580);
		break;
	case 8: // .Func_9e844
	case 10:
	case 12:
		fade_on_even_frames();
		break;
	case 9: // .Func_9e84b
		next_fade(BANK_Pals_b8580, ADDR_Pals_b85c0);
		break;
	case 11: // .Func_9e850
		next_fade(BANK_Pals_b8580, ADDR_Pals_b8600);
		break;
	case 13:
		finish();
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f991: state 4 ends in 3
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f991 + 0x28, ADDR_OAM_bf611, {{0x04, 0x03}});
}

// Func_9ea9e, Func_9eab4: while wSceneObj12's y (x), wSceneObj10 (11)
// follows Data_9ec16 (Data_9ec41); at its end, both cleared with what
// ApplyOWMovement leaves in a
void Cutscenes::move_debris()
{
	static constexpr struct {
		uint16_t flag, obj, path;
	} DEBRIS[2] = {
		{wSceneObj12YCoord, wSceneObj10, ADDR_Data_9ec16},
		{wSceneObj12XCoord, wSceneObj11, ADDR_Data_9ec41},
	};
	for (const auto &d : DEBRIS) {
		if (!mem_[d.flag])
			continue;
		SET_DE(gb_, d.path);
		SET_HL(gb_, d.obj);
		routines_.call(ADDR_ApplyOWMovement);
		if (!(gb_->cpu.f & FLAG_Z))
			continue;
		uint8_t a = gb_->cpu.a;
		mem_[d.flag] = a;
		mem_[static_cast<uint16_t>(d.obj + 6)] = a;
	}
}

// Func_9f9f5: wSceneObj3 (bank $1a's animations, bank $63's sprites),
// rumbling in states 1 and 2
void Cutscenes::draw_func_9f9f5()
{
	uint8_t state = mem_[wSceneObj3State];
	if (!state)
		return;
	uint16_t obj = animate(wSceneObj3State, frameset(ADDR_Func_9f9f5 + 0x1d, state), 0x1a);
	uint8_t now = mem_[wSceneObj3State];
	if (now == 0x01)
		periodic_sound(wSceneObj3Unk7, 0x1b, SFX_135); // .Func_9fa1b
	else if (now == 0x02)
		periodic_sound(wSceneObj3Unk7, 0x1b, SFX_080); // .Func_9fa34
	draw(obj, ADDR_OAM_18c806, 0x63); // Func_9f0cb
}

// InitCutscene58, Func_9e855
void Cutscenes::init_crater_hole()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	routines_.call(ADDR_LoadCutscenes9Gfx_Vram0);
	decompress(BANK_WallCrackGfx, ADDR_WallCrackGfx, v1Tiles1, true);
	routines_.call(ADDR_LoadLevelMainTiles29_Vram1);
	routines_.call(ADDR_LoadCutsceneBGMaps39);
	routines_.call(ADDR_LoadCutsceneBGMaps040);
	place(wSceneObj2, SCREEN_HEIGHT_PX, 0x48, 0x01);
	place(wSceneObj3, 0x48, 0x58, 0x01);
	animate_all({wSceneObj11State, wSceneObj10State, wSceneObj2State}, ADDR_Func_9f9c1 + 0x2e, ADDR_OAM_bf66d,
		{{0x02, 0x01}}); // Func_9f9c1
	draw_func_9f9f5();
}

// Cutscene58Func: fade in; the hammer rises and strikes twice, the debris
// flies, and the wall cracks open
void Cutscenes::crater_hole()
{
	move_debris();
	// .asm_9e8dc: the debris flies off
	auto debris = [&] {
		place(wSceneObj10, 0x48, 0x54, 0x03);
		place(wSceneObj11, 0x44, 0x58, 0x03);
		mem_[wSceneObj12YCoord] = 0x01;
		mem_[wSceneObj12XCoord] = 0x01;
		play_sfx(mem_, SFX_132);
		advance();
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e89c
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b87c0, ADDR_Pals_b87c0);
		load_pals2(BANK_Pals_b87c0, ADDR_Pals_b87c0);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e8b2
	case 5:
		wait(30);
		break;
	case 4: // .Func_9e8b7
		rise(wSceneObj2, 0x76, 0x50);
		break;
	case 6: // .Func_9e8c7
	case 8:
		if (frame() < 0x1a)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 7: // .Func_9e8d6
		if (mem_[wSceneObj2State] == 0x01)
			debris();
		break;
	case 9: // .Func_9e905
		if (mem_[wSceneObj2State] != 0x01)
			break;
		wram_.collection_row() = 0x08;
		set_state(wSceneObj3, 0x02);
		debris();
		break;
	case 10:
		end(120);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f9c1: state 2 ends in 1
	animate_all({wSceneObj11State, wSceneObj10State, wSceneObj2State}, ADDR_Func_9f9c1 + 0x2e, ADDR_OAM_bf66d,
		{{0x02, 0x01}});
	draw_func_9f9f5();
}

// InitCutscene55, Func_9e91a: three blocks, each with its drill hole
void Cutscenes::init_drill_holes()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	routines_.call(ADDR_LoadLevelMainTiles29_Vram0);
	routines_.call(ADDR_LoadCutscenes7Gfx_Vram1);
	routines_.call(ADDR_LoadCutsceneBGMaps27);
	place(wSceneObj3, 0x58, 0x40, 0x01);
	place(wSceneObj4, 0x58, 0x50, 0x01);
	place(wSceneObj5, 0x58, 0x60, 0x01);
	place(wSceneObj6, 0x58, 0x40, 0x04);
	place(wSceneObj7, 0x58, 0x50, 0x04);
	place(wSceneObj8, 0x58, 0x60, 0x04);
	draw_func_9fa48();
}

// Cutscene55Func: fade in; the drill comes in and drills the three
// blocks in turn, the debris flying
void Cutscenes::drill_holes()
{
	move_debris();
	// .Func_9e9fa: the debris flies off, dx right of the first block
	auto debris = [&](uint8_t dx) {
		place(wSceneObj10, 0x4c, 0x40, 0x03);
		place(wSceneObj11, 0x4c, 0x48, 0x03);
		mem_[wSceneObj12YCoord] = 0x01;
		mem_[wSceneObj12XCoord] = 0x01;
		advance();
		if (!dx)
			return;
		mem_[wSceneObj10XCoord] += dx; // .asm_9ea52
		mem_[wSceneObj11XCoord] += dx;
	};
	// a pixel every 4 frames
	auto every_4 = [&](uint16_t coord, uint8_t d, uint8_t to) {
		if (frame() & 0x03)
			return false;
		Byte c = mem_[coord];
		c += d;
		if (c != to)
			return false;
		return true;
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e9a3
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b8680, ADDR_Pals_b8680);
		load_pals2(BANK_Pals_b8680, ADDR_Pals_b8680);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e9b9
		if (frame() < 0x28)
			break;
		place(wSceneObj2, 0x46, 0x90, 0x02);
		advance();
		break;
	case 4: // .Func_9e9cb: left on even frames
		if (frame() & 1)
			break;
		if (--mem_[wSceneObj2XCoord] == 0x44)
			advance();
		break;
	case 5: // .Func_9e9da
	case 11:
	case 17:
		wait(45);
		break;
	case 6: // .Func_9e9df: down into the block
	case 12:
	case 18:
		if (!every_4(wSceneObj2YCoord, 0x01, 0x4e))
			break;
		play_sfx(mem_, SFX_SWITCH);
		advance();
		break;
	case 7: // .Func_9e9f6
		mem_[wSceneObj3State] = 0;
		debris(0);
		break;
	case 8: // .Func_9ea1b
		if (frame() < 0x14)
			break;
		set_state(wSceneObj6, 0x05);
		advance();
		break;
	case 9: // .Func_9ea2a: back up
	case 15:
	case 21:
		if (every_4(wSceneObj2YCoord, 0xff, 0x46))
			advance();
		break;
	case 10: // .Func_9ea39: on to the next block
		if (every_4(wSceneObj2XCoord, 0x01, 0x54))
			advance();
		break;
	case 13: // .Func_9ea49
		mem_[wSceneObj4State] = 0;
		debris(0x10);
		break;
	case 14: // .Func_9ea61
		if (frame() < 0x14)
			break;
		set_state(wSceneObj7, 0x05);
		advance();
		break;
	case 16: // .Func_9ea70
		if (every_4(wSceneObj2XCoord, 0x01, 0x64))
			advance();
		break;
	case 19: // .Func_9ea74
		mem_[wSceneObj5State] = 0;
		debris(0x20);
		break;
	case 20: // .Func_9ea7f
		if (frame() < 0x14)
			break;
		set_state(wSceneObj8, 0x05);
		advance();
		break;
	case 22: // .Func_9ea8e
		wait(30);
		break;
	case 23: // .Func_9ea93: off to the right
		if (++mem_[wSceneObj2XCoord] == 0x90)
			advance();
		break;
	case 24:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9fa48();
}

// Func_9fa48: the debris, the drill (state 2 whirs: wSceneObj2Unk7) and
// the blocks
void Cutscenes::draw_func_9fa48()
{
	for (uint16_t state : {wSceneObj10State, wSceneObj11State, wSceneObj2State, wSceneObj3State, wSceneObj4State,
		     wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State}) {
		uint8_t s = mem_[state];
		if (!s)
			continue;
		uint16_t obj = animate(state, frameset(ADDR_Func_9fa48 + 0x49, s));
		if (mem_[state] == 0x02)
			periodic_sound(wSceneObj2Unk7, 0x0c, SFX_PLANE1); // .Func_9faa0
		draw(obj, ADDR_OAM_bf6f0);
	}
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene51);
PORT_COVERED(Func_9e655);
PORT_COVERED(Cutscene51Func);
PORT_COVERED(Func_9f8da);
PORT_COVERED(InitCutscene54);
PORT_COVERED(Func_9e2b8);
PORT_COVERED(Cutscene54Func);
PORT_COVERED(Func_9f73b);
PORT_COVERED(InitCutscene56);
PORT_COVERED(Func_9e6ef);
PORT_COVERED(Cutscene56Func);
PORT_COVERED(Func_9f91f);
PORT_COVERED(InitCutscene57);
PORT_COVERED(Func_9e79a);
PORT_COVERED(Cutscene57Func);
PORT_COVERED(Func_9f991);
PORT_COVERED(InitCutscene58);
PORT_COVERED(Func_9e855);
PORT_COVERED(Cutscene58Func);
PORT_COVERED(Func_9f9c1);
PORT_COVERED(InitCutscene55);
PORT_COVERED(Func_9e91a);
PORT_COVERED(Cutscene55Func);
PORT_COVERED(Func_9fa48);
PORT_COVERED(Func_9ea9e);
PORT_COVERED(Func_9eab4);
PORT_COVERED(Func_9f9f5);
