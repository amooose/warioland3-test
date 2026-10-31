// Cutscenes $30 to $3d. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

// InitCutscene30, Func_9ded4
void Cutscenes::init_raise_ice_blocks()
{
	gb_->cpu.b = BANK_LevelMainTiles7;
	SET_HL(gb_, ADDR_LevelMainTiles7);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_Func_9c977);
	routines_.call(ADDR_LoadCutsceneBGMaps26);
	routines_.call(ADDR_ClearTempPals_Bank27);
	place(wSceneObj2, 88, 98, 0x03);
	place(wSceneObj3, 88, 70, 0x01);
	mem_[wSceneObj4YCoord] = 72;
	mem_[wSceneObj4XCoord] = 82;
	mem_[wSceneObj5YCoord] = 68;
	mem_[wSceneObj5XCoord] = 70;
	mem_[wSceneObj6YCoord] = 60;
	mem_[wSceneObj6XCoord] = 52;
	draw_func_9f5a0();
}

// Cutscene30Func: fade in; something is struck three times, then rises,
// letting go of three bubbles that float up
void Cutscenes::raise_ice_blocks()
{
	// .Func_9dfb6, .Func_9dfa6, .Func_9dfae: the bubbles up on even frames,
	// gone at y $20
	for (uint16_t obj : {wSceneObj4, wSceneObj5, wSceneObj6}) {
		uint16_t state = static_cast<uint16_t>(obj + 6);
		if (!mem_[state] || (wram_.unk_2d014() & 1))
			continue;
		if (--mem_[obj] == 0x20)
			mem_[state] = 0;
	}
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9df3c
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b8500, ADDR_Pals_b8500);
		load_pals2(BANK_Pals_b8500, ADDR_Pals_b8500);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9df52
		wait(10);
		break;
	case 4: // .Func_9df57
	case 6:
	case 8:
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 5: // .Func_9df62
	case 7:
	case 9:
		if (mem_[wSceneObj2State] == 0x03)
			advance();
		break;
	case 10: // .Func_9df6b
		wait(30);
		break;
	case 11: { // .Func_9df70: up a pixel every 8 frames, letting the bubbles go
		if (wram_.unk_2d014() & 0x07)
			break;
		uint8_t y = --mem_[wSceneObj3YCoord];
		uint16_t bubble = y == 0x48 ? wSceneObj4State : y == 0x44 ? wSceneObj5State : y == 0x3c ? wSceneObj6State : 0;
		if (bubble) {
			mem_[bubble] = 0x04;
			play_sfx(mem_, SFX_00F);
			break;
		}
		if (y == 0x20)
			advance();
		break;
	}
	case 12:
		end(20);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f5a0();
}

// Func_9f5a0: wSceneObj4, 6, 2, 3 and 5; state 2 (on to 3) thuds at
// wSceneObj2's frame 3, once each time (wSceneObj2Unk7 remembers)
void Cutscenes::draw_func_9f5a0()
{
	for (uint16_t state : {wSceneObj4State, wSceneObj6State, wSceneObj2State, wSceneObj3State, wSceneObj5State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f5a0 + 0x3d, s));
			if (mem_[state] == 0x02) {
				// .Func_9f5e5
				uint8_t frame = mem_[wSceneObj2Frame];
				Byte hit = mem_[wSceneObj2Unk7];
				if (!hit) {
					if (frame == 0x03) {
						play_sfx(mem_, SFX_0BA);
						hit = 1;
					}
				} else if (frame != 0x03) {
					hit = 0;
				}
				if (next_state_when_done(obj, 0x03))
					continue;
			}
			draw(obj, ADDR_OAM_be807);
			break;
		}
	}
}

// InitCutscene37
void Cutscenes::init_torch_forest()
{
	load_pals1(BANK_Pals_b8240, ADDR_Pals_b8240);
	load_pals2(BANK_Pals_b8f40, ADDR_Pals_b8f40);
	routines_.call(ADDR_LoadCutscenes4Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps12);
}

// Cutscene37Func: the torch rises, burning
void Cutscenes::torch_forest()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9de1b
		place(wSceneObj2, 192, 50, 0x01);
		mem_[wSceneObj1YCoord] = 3;
		advance();
		break;
	case 2: // .Func_9de33
		wait(40);
		break;
	case 3: // .Func_9de38
		rise(wSceneObj2, 0x8e, 0x68);
		break;
	case 4:
		end(100);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f508
	draw_shadow(wSceneObj2, wCurSceneObj, 0x03, 0x05); // Func_9fb97
	uint8_t state = mem_[wSceneObj2State];
	if (!state)
		return;
	uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9f508 + 0x1f, state));
	if (mem_[wSceneObj2State] == 0x01)
		loop_sound(wSceneObj2Unk7, 0x0e, SFX_01E); // .Func_9f52e
	draw(obj, ADDR_OAM_be2d9);
}

// InitCutscene38
void Cutscenes::init_reveal_warped_void()
{
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	load_pals2(BANK_Pals_b8f80, ADDR_Pals_b8f80);
	routines_.call(ADDR_Func_9c931);
	routines_.call(ADDR_LoadCutsceneBGMaps03);
}

// Cutscene38Func: the switch rises and is pressed
void Cutscenes::reveal_warped_void()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9de68
		if (frame() < 0x1e)
			break;
		place(wSceneObj2, 152, 72, 0x01);
		mem_[wSceneObj1YCoord] = 1;
		play_sfx(mem_, SFX_112); // Func_9ce1f
		advance();
		break;
	case 2: // .Func_9de87
		if (--mem_[wSceneObj2YCoord] == 0x78)
			advance();
		break;
	case 3: // .Func_9de92
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 4: // .Func_9de9d
		if (mem_[wSceneObj2Frame] != 0x01)
			break;
		play_sfx(mem_, SFX_SWITCH);
		advance();
		break;
	case 5: // .Func_9deae
		if (mem_[wSceneObj2Frame] != 0x05)
			break;
		play_sfx(mem_, SFX_106);
		advance();
		break;
	case 6: // .Func_9debf
		if (mem_[wSceneObj2State] == 0x03)
			advance();
		break;
	case 7: // .Func_9dec8
		wait(120);
		break;
	case 8:
		fade_in();
		break;
	case 9: // .Func_9decd
		mem_[wSceneObj2State] = 0;
		advance();
		break;
	case 10:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f547: state 2 on to 3, which hums
	draw_shadow(wSceneObj2, wCurSceneObj, 0x01, 0x00); // Func_9fb4c
	for (;;) {
		uint8_t s = mem_[wSceneObj2State];
		if (!s)
			break;
		uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9f547 + 0x35, s));
		uint8_t now = mem_[wSceneObj2State];
		if (now == 0x02 && next_state_when_done(obj, 0x03))
			continue;
		if (now == 0x03)
			loop_sound(wSceneObj2Unk7, 0x10, SFX_040); // .Func_9f587
		draw(obj, ADDR_OAM_be3ce);
		break;
	}
	draw_shadow(wSceneObj2, wCurSceneObjDuration, 0x01, 0x01); // Func_9fb6c
}

// InitCutscene39
void Cutscenes::init_raise_pipe()
{
	load_pals1(BANK_Pals_b8380, ADDR_Pals_b8380);
	load_pals2(BANK_Pals_b8fc0, ADDR_Pals_b8fc0);
	routines_.call(ADDR_LoadCutsceneBGMaps15);
}

// Cutscene39Func: the lever drops in and is pulled
void Cutscenes::raise_pipe()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9dfe3
		place(wSceneObj2, 0x18, 0x54, 0x01); // Func_9ccf9
		advance();
		break;
	case 2: // .Func_9dff1
		if (++mem_[wSceneObj2YCoord] != 0x50)
			break;
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 3: // .Func_9e004
		if (frame() < 0x2d)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 4: // .Func_9e013
		wait(100);
		break;
	case 5:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f607: a sound when state 2's animation ends
	uint8_t state = mem_[wSceneObj2State];
	if (!state)
		return;
	uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9f607 + 0x19, state));
	if (mem_[wSceneObj2State] == 0x02 && wram_.ow_animation_finished() == 0xff) // .Func_9f629
		play_sfx(mem_, SFX_085);
	draw(obj, ADDR_OAM_be8fe);
}

// InitCutscene3a, Func_9e018
void Cutscenes::init_remove_warps()
{
	routines_.call(ADDR_ClearTempPals_Bank27);
	routines_.call(ADDR_LoadCutscenes9Gfx_Vram1);
	gb_->cpu.b = BANK_LevelMainTiles30;
	SET_HL(gb_, ADDR_LevelMainTiles30);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps37);
	routines_.call(ADDR_LoadCutsceneBGMaps38);
	place(wSceneObj2, 0x54, 0x46, 0x01); // Func_9ccf9
	static constexpr uint16_t WARPS[4] = {wSceneObj4, wSceneObj5, wSceneObj6, wSceneObj7};
	for (int i = 0; i < 4; i++) {
		mem_[WARPS[i]] = 80;
		mem_[static_cast<uint16_t>(WARPS[i] + 1)] = static_cast<uint8_t>(56 + 16 * i);
	}
	draw_func_9f638();
}

// Cutscene3aFunc: fade in; the switch is pressed, and the four warps
// vanish one by one
void Cutscenes::remove_warps()
{
	if (mem_[wSceneObj7State] == 0x03) // .Func_9e153
		loop_sound(wSceneObj7Unk7, 0x1b, SFX_080);
	static constexpr uint16_t WARPS[4] = {wSceneObj4, wSceneObj5, wSceneObj6, wSceneObj7};
	uint8_t step = wram_.unk_2d013();
	if (step >= 8 && step < 20) {
		// .Func_9e108 and the like: each warp at its frame $0a goes (state 4)
		// and is waited for
		uint16_t warp = WARPS[(step - 8) / 3];
		switch ((step - 8) % 3) {
		case 0:
			if (mem_[static_cast<uint16_t>(warp + 2)] == 0x0a) // its Frame
				advance();
			break;
		case 1:
			set_state(warp, 0x04);
			advance();
			break;
		case 2:
			if (!mem_[static_cast<uint16_t>(warp + 6)])
				advance();
			break;
		}
		draw_func_9f638();
		return;
	}
	switch (step) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e07d
		routines_.call(ADDR_Func_9cc87);
		load_pals1(BANK_Pals_b8780, ADDR_Pals_b8780);
		load_pals2(BANK_Pals_b8780, ADDR_Pals_b8780);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9e093
		wait(45);
		break;
	case 4: { // .Func_9e098: the switch down and right, on even frames
		if (frame() & 1)
			break;
		Byte y = mem_[wSceneObj2YCoord];
		y -= 2;
		++mem_[wSceneObj2XCoord];
		if (y == 0x50) {
			play_sfx(mem_, SFX_SWITCH);
			break;
		}
		if (y == 0x4c)
			advance();
		break;
	}
	case 5: // .Func_9e0b6
		if (frame() < 0x2d)
			break;
		place(wSceneObj3, 0x3d, 0x52, 0x02); // Func_9ccf9
		advance();
		break;
	case 6: // .Func_9e0c8
		if (frame() < 0x8c)
			break;
		wram_.collection_row() = 0x08;
		mem_[wSceneObj3State] = 0;
		mem_[wSceneObj2State] = 0;
		stop_sfx();
		for (uint16_t warp : WARPS)
			set_state(warp, 0x03);
		advance();
		break;
	case 7: // .Func_9e103
		wait(80);
		break;
	case 20:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f638();
}

// Func_9f638: wSceneObj2 to 7; state 4 ends in 0; state 2 hums
// (wSceneObj3Unk7 counts, whichever object)
void Cutscenes::draw_func_9f638()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State, wSceneObj6State,
		     wSceneObj7State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f638 + 0x44, s));
			uint8_t now = mem_[state];
			if (now == 0x04 && next_state_when_done(obj, 0x00))
				continue;
			if (now == 0x02)
				loop_sound(wSceneObj3Unk7, 0x48, SFX_10E); // .Func_9f689
			draw(obj, ADDR_OAM_bea06);
			break;
		}
	}
}

// InitCutscene3c
void Cutscenes::init_demons_blood()
{
	load_pals1(BANK_Pals_b84c0, ADDR_Pals_b84c0);
	load_pals2(BANK_Pals_b84c0, ADDR_Pals_b84c0);
	routines_.call(ADDR_Func_9c977);
	gb_->cpu.b = BANK_LevelMainTiles31;
	SET_HL(gb_, ADDR_LevelMainTiles31);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps23);
	routines_.call(ADDR_SetBGPriorityOnTopRows_Vram1);
}

// Cutscene3cFunc: the bottle comes down, tips, a drop falls and splashes,
// and something appears
void Cutscenes::demons_blood()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e18f
		place(wSceneObj2, 0x20, 0x42, 0x01);
		place(wSceneObj3, 0x3e, 0x50, 0x00);
		advance();
		break;
	case 2: { // .Func_9e1a8: down on odd frames
		if (!(frame() & 1))
			break;
		uint8_t y = ++mem_[wSceneObj2YCoord];
		if (y == 0x27) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x3a)
			advance();
		break;
	}
	case 3: // .Func_9e1bc
		if (frame() < 0x3c)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 4: // .Func_9e1cb
		if (mem_[wSceneObj2State] == 0x03)
			advance();
		break;
	case 5: { // .Func_9e1d4: the drop falls by .data_9e205's speeds, to y $58
		Byte f = wram_.unk_2d014(); // .Func_9e1f4
		if (f >= 0x2c)
			f = 0x28;
		uint8_t dy = mem_[static_cast<uint16_t>(ADDR_Cutscene3cFunc + 0x94 + f)];
		Byte y = mem_[wSceneObj3YCoord];
		y += dy;
		if (y < 0x58)
			break;
		y = 0x58;
		set_state(wSceneObj3, 0x05);
		play_sfx(mem_, SFX_0A1);
		advance();
		break;
	}
	case 6: // .Func_9e231
		if (!mem_[wSceneObj3State])
			advance();
		break;
	case 7: // .Func_9e239
		if (frame() < 0x3c)
			break;
		place(wSceneObj4, 0x58, 0x50, 0x06);
		play_sfx(mem_, SFX_074);
		advance();
		break;
	case 8: // .Func_9e253
		if (mem_[wSceneObj4State] == 0x07)
			advance();
		break;
	case 9:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f6a2();
}

// Func_9f6a2: wSceneObj4, 2 and 3. State 2 ends in 3, and starts the drop
// (wSceneObj3 in state 4); states 5 and 6 end in 0 and 7.
void Cutscenes::draw_func_9f6a2()
{
	for (uint16_t state : {wSceneObj4State, wSceneObj2State, wSceneObj3State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f6a2 + 0x48, s));
			uint8_t now = mem_[state];
			bool again = false;
			if (now == 0x02) {
				again = next_state_when_done(obj, 0x03);
				if (again)
					mem_[wSceneObj3State] = 0x04;
			} else if (now == 0x05 || now == 0x06) {
				again = next_state_when_done(obj, now == 0x05 ? 0x00 : 0x07);
			}
			if (again)
				continue;
			draw(obj, ADDR_OAM_beafc);
			break;
		}
	}
}

// InitCutscene3d
void Cutscenes::init_paragoom_cage()
{
	load_pals1(BANK_Pals_b8700, ADDR_Pals_b8700);
	load_pals2(BANK_Pals_b9000, ADDR_Pals_b9000);
	routines_.call(ADDR_LoadCutscenes9Gfx_Vram0);
	decompress(BANK_ParaGoomGfx, ADDR_ParaGoomGfx, v1Tiles0 + 0x40 * TILE_SIZE, true);
	mem_[rVBK] = 1;
	gb_->cpu.b = BANK_LevelMainTiles30;
	SET_HL(gb_, ADDR_LevelMainTiles30);
	routines_.call(ADDR_LoadFarTiles);
	mem_[rVBK] = 0;
	routines_.call(ADDR_LoadCutsceneBGMaps34);
	routines_.call(ADDR_LoadCutsceneBGMaps35);
}

// Cutscene3dFunc: the key goes in twice, the cage opens, the Paragoom
// flies off
void Cutscenes::paragoom_cage()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9e376
		place(wSceneObj2, 0x6c, 0x78, 0x01);
		advance();
		break;
	case 2: // .Func_9e384: up and left, to x $44
	case 6:
		--mem_[wSceneObj2YCoord];
		if (--mem_[wSceneObj2XCoord] == 0x44)
			advance();
		break;
	case 3: // .Func_9e391: turn
	case 7:
		if (frame() < 0x1e)
			break;
		set_state(wSceneObj2, static_cast<uint8_t>(mem_[wSceneObj2State] + 1));
		play_sfx(mem_, SFX_0C4);
		advance();
		break;
	case 4: // .Func_9e3a8
	case 8:
		if (!mem_[wSceneObj2State])
			advance();
		break;
	case 5: // .Func_9e3b0
		if (frame() < 0x0a)
			break;
		place(wSceneObj2, 0x84, 0x78, 0x03);
		advance();
		break;
	case 9: // .Func_9e3c2
		if (frame() < 0x3c)
			break;
		wram_.collection_row() = 0x08;
		place(wSceneObj4, 0x50, 0x50, 0x05);
		place(wSceneObj3, 0x40, 0x50, 0x01);
		advance();
		break;
	case 10: // .Func_9e3e4
		if (frame() < 0x5a)
			break;
		set_state(wSceneObj4, 0x06);
		advance();
		break;
	case 11: // .Func_9e3f3
		if (frame() < 0x04)
			break;
		play_sfx(mem_, SFX_07A);
		advance();
		break;
	case 12: // .Func_9e402
		if (mem_[wSceneObj4State] == 0x07)
			advance();
		break;
	case 13: // .Func_9e40b
		if (frame() < 0x08)
			break;
		set_state(wSceneObj3, 0x02);
		advance();
		break;
	case 14: // .Func_9e41a
		if (frame() < 0x14)
			break;
		play_sfx(mem_, SFX_0AC);
		advance();
		break;
	case 15: // .Func_9e429: down a pixel every 4 frames
		if (frame() & 0x03)
			break;
		if (++mem_[wSceneObj3YCoord] == 0x80)
			advance();
		break;
	case 16:
		end(20);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f7a2: the Paragoom (bank $1a's animations, bank $60's sprites)
	uint8_t state = mem_[wSceneObj3State];
	if (state)
		draw(animate(wSceneObj3State, frameset(ADDR_Func_9f7a2 + 0x15, state), 0x1a), ADDR_OAM_180b43, 0x60);
	// Func_9f75c: states 2 and 4 end in 0, state 6 in 7
	animate_all({wSceneObj2State, wSceneObj4State}, ADDR_Func_9f75c + 0x38, ADDR_OAM_bed42,
		{{0x02, 0x00}, {0x04, 0x00}, {0x06, 0x07}});
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene30);
PORT_COVERED(Func_9ded4);
PORT_COVERED(Cutscene30Func);
PORT_COVERED(Func_9f5a0);
PORT_COVERED(InitCutscene37);
PORT_COVERED(Cutscene37Func);
PORT_COVERED(Func_9f508);
PORT_COVERED(InitCutscene38);
PORT_COVERED(Cutscene38Func);
PORT_COVERED(Func_9f547);
PORT_COVERED(InitCutscene39);
PORT_COVERED(Cutscene39Func);
PORT_COVERED(Func_9f607);
PORT_COVERED(InitCutscene3a);
PORT_COVERED(Func_9e018);
PORT_COVERED(Cutscene3aFunc);
PORT_COVERED(Func_9f638);
PORT_COVERED(InitCutscene3c);
PORT_COVERED(Cutscene3cFunc);
PORT_COVERED(Func_9f6a2);
PORT_COVERED(InitCutscene3d);
PORT_COVERED(Cutscene3dFunc);
PORT_COVERED(Func_9f7a2);
PORT_COVERED(Func_9f75c);
