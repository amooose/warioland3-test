// Cutscenes $43 to $4f. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

// InitCutscene43
void Cutscenes::init_place_propeller()
{
	load_pals1(BANK_Pals_b8440, ADDR_Pals_b8440);
	load_pals2(BANK_Pals_b8440, ADDR_Pals_b8440);
	gb_->cpu.b = BANK_LevelMainTiles36;
	SET_HL(gb_, ADDR_LevelMainTiles36);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps20);
}

// Cutscene43Func: the propeller drops in and spins
void Cutscenes::place_propeller()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e272
		place(wSceneObj2, 0x20, 0x50, 0x01);
		advance();
		break;
	case 2: // .Func_9e280: down on even frames
		if (frame() & 1)
			break;
		if (++mem_[wSceneObj2YCoord] != 0x50)
			break;
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 3: // .Func_9e297
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 4: // .Func_9e2a6
		if (frame() < 0x2d)
			break;
		place(wSceneObj3, 0x48, 0x50, 0x03);
		advance();
		break;
	case 5:
		end(120);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f6f8: state 2 whirs (wSceneObj2Unk7)
	for (uint16_t state : {wSceneObj2State, wSceneObj3State}) {
		uint8_t s = mem_[state];
		if (!s)
			continue;
		uint16_t obj = animate(state, frameset(ADDR_Func_9f6f8 + 0x1f, s));
		if (mem_[state] == 0x02)
			loop_sound(wSceneObj2Unk7, 0x0c, SFX_07D); // .Func_9f722
		draw(obj, ADDR_OAM_bebef);
	}
}

// InitCutscene44
void Cutscenes::init_feed_octohon()
{
	wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	load_pals2(BANK_Pals_b90c0, ADDR_Pals_b90c0);
	routines_.call(ADDR_Func_9c931);
	decompress(BANK_OctohonGfx, ADDR_OctohonGfx, v1Tiles1 + 0x40 * TILE_SIZE, true);
	routines_.call(ADDR_LoadCutsceneBGMaps01);
	routines_.call(ADDR_LoadCutsceneBGMaps04);
	routines_.call(ADDR_SetBGPriorityOnTopRows_BGMap1);
	routines_.call(ADDR_SetBGPriorityOnTopRows_Vram0);
	routines_.call(ADDR_Func_9cc4f);
}

// Cutscene44Func: the food drops in; the Octohon swims up to it, eats,
// and swims off; the screen shakes
void Cutscenes::feed_octohon()
{
	// .Func_9ebbc: the food sinks a pixel every 4 frames (wGlobalCounter)
	auto sink = [&] {
		if (!(wram_.global_counter() & 0x03))
			++mem_[wSceneObj2YCoord];
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9eaf7
		place(wSceneObj2, 0x20, 0x4c, 0x01);
		advance();
		break;
	case 2: // .Func_9eb05: down on even frames, out of sight
		if (frame() & 1)
			break;
		if (++mem_[wSceneObj2YCoord] != 0xa8)
			break;
		mem_[wSceneObj2State] = 0;
		advance();
		break;
	case 3: // .Func_9eb18
		if (frame() < 0x1e)
			break;
		wram_.collection_row() = 0x08;
		advance();
		break;
	case 4: // .Func_9eb24
		place(wSceneObj2, 0x20, 0x50, 0x02);
		place(wSceneObj3, 0x48, 0x20, 0x01);
		advance();
		break;
	case 5: // .Func_9eb3d
		if (frame() == 0x50)
			advance();
		sink();
		break;
	case 6: // .Func_9eb47
		sink();
		if (octohon_step() != 0x48)
			break;
		play_sfx(mem_, SFX_FAT_WALK);
		advance();
		break;
	case 7: // .Func_9eb5c
		if (frame() < 0x02)
			break;
		mem_[wSceneObj3XCoord] -= 2;
		advance();
		break;
	case 8: // .Func_9eb68
		if (frame() < 0x06)
			break;
		mem_[wSceneObj2State] = 0;
		advance();
		break;
	case 9: // .Func_9eb73
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj3, 0x02);
		play_sfx(mem_, SFX_11A);
		advance();
		break;
	case 10: // .Func_9eb8a
		if (mem_[wSceneObj3State] != 0x03)
			break;
		mem_[wSceneObj3Unk7] = 0;
		mem_[wSceneObj4YCoord] = 0;
		advance();
		break;
	case 11: // .Func_9eb9a
		wait(60);
		break;
	case 12: // .Func_9eb9f
		if (octohon_step() != 0x88)
			break;
		mem_[wSceneObj3State] = 0;
		advance();
		break;
	case 13: // .Func_9ebad
		if (frame() < 0x2d)
			break;
		play_sfx(mem_, SFX_08D);
		advance();
		break;
	case 14:
		shake_screen();
		break;
	case 15:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9fab4: the food, bubbling in state 1
	uint8_t state = mem_[wSceneObj2State];
	if (state) {
		uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9fab4 + 0x19, state));
		if (mem_[wSceneObj2State] == 0x01)
			periodic_sound(wSceneObj2Unk7, 0x30, SFX_08E); // .Func_9fad6
		draw(obj, ADDR_OAM_bf847);
	}
	// Func_9faea: the Octohon (bank $1a's animations, bank $62's sprites);
	// state 2 ends in 3; states 1 and 3 make sounds every $28 frames
	// (counted in wSceneObj4YCoord)
	for (;;) {
		uint8_t s = mem_[wSceneObj3State];
		if (!s)
			break;
		uint16_t obj = animate(wSceneObj3State, frameset(ADDR_Func_9faea + 0x2a, s), 0x1a);
		uint8_t now = mem_[wSceneObj3State];
		if (now == 0x01)
			periodic_sound(wSceneObj4YCoord, 0x28, SFX_08F); // .Func_9fb1f
		else if (now == 0x03)
			periodic_sound(wSceneObj4YCoord, 0x28, SFX_083); // .Func_9fb38
		else if (now == 0x02 && next_state_when_done(obj, 0x03))
			continue;
		draw(obj, ADDR_OAM_18a895, 0x62);
		break;
	}
}

// .Func_9ebc7: the Octohon right by .data_9ebe6's steps (by wSceneObj3Unk7,
// round every $30 frames); its new x
uint8_t Cutscenes::octohon_step()
{
	Byte i = mem_[wSceneObj3Unk7]; // .Func_9ebd3
	uint8_t index = i;
	++i;
	if (index >= 0x30) {
		i = 0;
		index = 0;
		++i;
	}
	uint8_t dx = mem_[static_cast<uint16_t>(ADDR_Cutscene44Func + 0x11c + index)];
	Byte x = mem_[wSceneObj3XCoord];
	x += dx;
	return x;
}

// InitCutscene4a, Func_9e438
void Cutscenes::init_sky_door()
{
	load_pals1(BANK_Pals_b83c0, ADDR_Pals_b83c0);
	load_pals2(BANK_Pals_b9040, ADDR_Pals_b9040);
	routines_.call(ADDR_Func_9c937);
	routines_.call(ADDR_LoadCutsceneBGMaps17);
	place(wSceneObj2, 0x40, 0xb0, 0x02);
	place(wSceneObj3, 0x44, 0x50, 0x01);
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f7bb + 0x1b, ADDR_OAM_bee6e);
}

// Cutscene4aFunc: the key comes in from the right and turns in the lock
void Cutscenes::sky_door()
{
	auto at_frame = [&](uint8_t f, uint16_t sfx) {
		if (mem_[wSceneObj2Frame] != f)
			return;
		if (sfx)
			play_sfx(mem_, sfx);
		advance();
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e469
		if (--mem_[wSceneObj2XCoord] == 0x56)
			advance();
		break;
	case 2: // .Func_9e474
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj2, 0x03);
		advance();
		break;
	case 3: // .Func_9e483
		at_frame(0x04, SFX_CLICK);
		break;
	case 4: // .Func_9e494
		at_frame(0x06, SFX_132);
		break;
	case 5: // .Func_9e4a5
		at_frame(0x07, 0);
		break;
	case 6:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f7bb
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f7bb + 0x1b, ADDR_OAM_bee6e);
}

// InitCutscene4c, Func_9e4ae
void Cutscenes::init_extinguish_fire()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	routines_.call(ADDR_LoadCutscenes8Gfx_Vram0);
	decompress(BANK_FireGfx, ADDR_FireGfx, v1Tiles1, true);
	routines_.call(ADDR_LoadLevelMainTiles29_Vram1);
	routines_.call(ADDR_LoadCutsceneBGMaps32);
	place(wSceneObj2, 0x80, 0x40, 0x01);
	place(wSceneObj3, 0x50, 0x5c, 0x01);
	mem_[wSceneObj3Attributes] |= 1 << 1;
	draw_extinguish_fire();
}

// Cutscene4cFunc: fade in; the extinguisher rises and sprays; the fire
// dies down
void Cutscenes::extinguish_fire()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e4f0
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b86c0, ADDR_Pals_b86c0);
		load_pals2(BANK_Pals_b86c0, ADDR_Pals_b86c0);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e506
		wait(40);
		break;
	case 4: // .Func_9e50b
		rise(wSceneObj2, 0x7e, 0x58);
		break;
	case 5: // .Func_9e51b
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 6: // .Func_9e52a: cp $03, but advances anyway
		advance();
		break;
	case 7: // .Func_9e532
		if (frame() < 0x64)
			break;
		set_state(wSceneObj3, 0x02);
		advance();
		break;
	case 8: // .Func_9e541
		if (frame() < 0x3c)
			break;
		mem_[wSceneObj3State] = 0;
		advance();
		break;
	case 9:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	draw_extinguish_fire();
}

void Cutscenes::draw_extinguish_fire()
{
	// Func_9f7dc: the extinguisher; state 2 ends in 3, which hisses
	for (;;) {
		uint8_t s = mem_[wSceneObj2State];
		if (!s)
			break;
		uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9f7dc + 0x26, s));
		uint8_t now = mem_[wSceneObj2State];
		if (now == 0x02 && next_state_when_done(obj, 0x03))
			continue;
		if (now == 0x03)
			periodic_sound(wSceneObj2Unk7, 0x1a, SFX_076); // .Func_9f80d
		draw(obj, ADDR_OAM_befd5);
		break;
	}
	// Func_9f821: the fire (bank $1a's animations, bank $62's sprites),
	// crackling in state 1
	uint8_t state = mem_[wSceneObj3State];
	if (!state)
		return;
	uint16_t obj = animate(wSceneObj3State, frameset(ADDR_Func_9f821 + 0x19, state), 0x1a);
	if (mem_[wSceneObj3State] == 0x01)
		periodic_sound(wSceneObj3Unk7, 0x0e, SFX_01E); // .Func_9f843
	draw(obj, ADDR_OAM_18aba0, 0x62);
}

// InitCutscene4e
void Cutscenes::init_place_brick()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	routines_.call(ADDR_LoadCutscenes1Gfx_Vram1);
	gb_->cpu.b = BANK_LevelMainTiles27;
	SET_HL(gb_, ADDR_LevelMainTiles27);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps30);
}

// Cutscene4eFunc: fade in; the brick rises into place
void Cutscenes::place_brick()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e562
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b8640, ADDR_Pals_b8640);
		load_pals2(BANK_Pals_b8640, ADDR_Pals_b8640);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e578
		if (frame() < 0x14)
			break;
		place(wSceneObj2, 0x78, 0x50, 0x01);
		advance();
		break;
	case 4: // .Func_9e58a: up on even frames
		if (frame() & 1)
			break;
		if (--mem_[wSceneObj2YCoord] != 0x50)
			break;
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 5:
		end(80);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f857: one frameset
	if (mem_[wSceneObj2State])
		draw(animate(wSceneObj2State, ADDR_Frameset_bf154), ADDR_OAM_bf131);
}

// InitCutscene4f, Func_9e5a1
void Cutscenes::init_spray_iron_wall()
{
	load_pals1(BANK_Pals_b8480, ADDR_Pals_b8480);
	load_pals2(BANK_Pals_b8480, ADDR_Pals_b8480);
	gb_->cpu.b = BANK_LevelMainTiles23;
	SET_HL(gb_, ADDR_LevelMainTiles23);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps22);
	place(wSceneObj2, 0x20, 0x4c, 0x01);
	place(wSceneObj3, 0x50, 0x60, 0x04);
	draw_func_9f86a();
}

// Cutscene4fFunc: the spray can comes down, sprays up and down twice, and
// the wall rusts away
void Cutscenes::spray_iron_wall()
{
	// .Func_9e60a / .Func_9e61f: down to y $5c / up to y $44, a pixel every
	// 4 frames
	auto move = [&](bool down) {
		if (frame() & 0x03)
			return;
		Byte y = mem_[wSceneObj2YCoord];
		if (down)
			++y;
		else
			--y;
		if (y == (down ? 0x5c : 0x44))
			advance();
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: { // .Func_9e5e2: down on even frames
		if (frame() & 1)
			break;
		uint8_t y = ++mem_[wSceneObj2YCoord];
		if (y == 0x31) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x44)
			advance();
		break;
	}
	case 2: // .Func_9e5f6
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 3: // .Func_9e605
		wait(45);
		break;
	case 4: // .Func_9e60a
	case 8:
		move(true);
		break;
	case 5: // .Func_9e61a
	case 7:
	case 9:
		wait(8);
		break;
	case 6: // .Func_9e61f
	case 10:
		move(false);
		break;
	case 11: // .Func_9e62e
		if (frame() < 0x1e)
			break;
		set_state(wSceneObj2, 0x01);
		advance();
		break;
	case 12: // .Func_9e63d
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj3, 0x03);
		advance();
		break;
	case 13: // .Func_9e64c
		if (mem_[wSceneObj3State] == 0x05)
			advance();
		break;
	case 14:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f86a();
}

// Func_9f86a: wSceneObj2 and 3. State 2 hisses (wSceneObj2Unk7); state 3
// crumbles (wSceneObj3Unk7) and ends in 5.
void Cutscenes::draw_func_9f86a()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f86a + 0x2f, s));
			uint8_t now = mem_[state];
			if (now == 0x02) {
				loop_sound(wSceneObj2Unk7, 0x1a, SFX_076); // .Func_9f8a8
			} else if (now == 0x03) {
				loop_sound(wSceneObj3Unk7, 0x0a, SFX_0C4); // .Func_9f8c1
				if (next_state_when_done(obj, 0x05))
					continue;
			}
			draw(obj, ADDR_OAM_bf157);
			break;
		}
	}
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene43);
PORT_COVERED(Cutscene43Func);
PORT_COVERED(Func_9f6f8);
PORT_COVERED(InitCutscene44);
PORT_COVERED(Cutscene44Func);
PORT_COVERED(Func_9fab4);
PORT_COVERED(Func_9faea);
PORT_COVERED(InitCutscene4a);
PORT_COVERED(Func_9e438);
PORT_COVERED(Cutscene4aFunc);
PORT_COVERED(Func_9f7bb);
PORT_COVERED(InitCutscene4c);
PORT_COVERED(Func_9e4ae);
PORT_COVERED(Cutscene4cFunc);
PORT_COVERED(Func_9f7dc);
PORT_COVERED(Func_9f821);
PORT_COVERED(InitCutscene4e);
PORT_COVERED(Cutscene4eFunc);
PORT_COVERED(Func_9f857);
PORT_COVERED(InitCutscene4f);
PORT_COVERED(Func_9e5a1);
PORT_COVERED(Cutscene4fFunc);
PORT_COVERED(Func_9f86a);
