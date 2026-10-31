// Cutscenes $1e to $27. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

// InitCutscene1e, Func_9d536
void Cutscenes::init_explode_bombs()
{
	load_pals1(BANK_Pals_b80c0, ADDR_Pals_b80c0);
	load_pals2(BANK_Pals_b8cc0, ADDR_Pals_b8cc0);
	routines_.call(ADDR_LoadCutscenes3Gfx);
	decompress(BANK_Cutscene09Attrmap, ADDR_Cutscene09Attrmap, v1BGMap0, true);
	decompress(BANK_Cutscene09Tilemap, ADDR_Cutscene09Tilemap, v0BGMap0);
	place(wSceneObj2, 83, 80, 0x04);
	place(wSceneObj3, 54, 80, 0x02);
	place(wSceneObj4, 83, 80, 0x03);
	animate_all({wSceneObj2State, wSceneObj4State, wSceneObj3State}, ADDR_Func_9f2a2 + 0x21, ADDR_OAM_bd971);
}

// Cutscene1eFunc: the switch is pressed, and the bombs go off
void Cutscenes::explode_bombs()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d589
		wait(15);
		break;
	case 2: // .Func_9d58e
		if (++mem_[wSceneObj3YCoord] != 0x3b)
			break;
		play_sfx(mem_, SFX_SWITCH);
		advance();
		break;
	case 3: // .Func_9d5a1
		wait(60);
		break;
	case 4: // .Func_9d5a6
		set_state(wSceneObj2, 0x01);
		play_sfx(mem_, SFX_10E);
		advance();
		break;
	case 5: // .Func_9d5b9
		wait(70);
		break;
	case 6: // .Func_9d5be
		if (mem_[wSceneObj2Frame] == 0x03)
			advance();
		break;
	case 7: // .Func_9d5c7
		set_state(wSceneObj2, 0x05);
		advance();
		break;
	case 8: // .Func_9d5d2
		wait(30);
		break;
	case 9:
		fade_in();
		break;
	case 10:
		finish();
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f2a2
	animate_all({wSceneObj2State, wSceneObj4State, wSceneObj3State}, ADDR_Func_9f2a2 + 0x21, ADDR_OAM_bd971);
}

// InitCutscene1f, Func_9dc6a
void Cutscenes::init_leaves_fall()
{
	load_pals1(BANK_Pals_b8280, ADDR_Pals_b8280);
	load_pals2(BANK_Pals_b8f00, ADDR_Pals_b8f00);
	routines_.call(ADDR_Func_9c931);
	routines_.call(ADDR_LoadCutsceneBGMaps16);
	routines_.call(ADDR_SetBGPriorityOnTopRows_Vram0);
	routines_.call(ADDR_Func_9cc4f);
	place(wSceneObj3, 96, 112, 0x04);
	draw_func_9f423();
}

// Cutscene1fFunc: a leaf drifts down, and the creature walks off
void Cutscenes::leaves_fall()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9dc99
		place(wSceneObj2, 36, 42, 0x01);
		advance();
		break;
	case 2: { // .Func_9dcac: down and right, on odd frames
		if (!(frame() & 1))
			break;
		++mem_[wSceneObj2YCoord];
		uint8_t x = ++mem_[wSceneObj2XCoord];
		if (x == 0x2e) {
			play_sfx(mem_, SFX_11D);
			break;
		}
		if (x == 0x4a)
			advance();
		break;
	}
	case 3: // .Func_9dcca
		if (frame() < 0x46)
			break;
		mem_[wSceneObj2State] = 0;
		advance();
		break;
	case 4: // .Func_9dcd5
		if (frame() < 0x1e)
			break;
		set_state(wSceneObj3, 0x02);
		advance();
		break;
	case 5: // .Func_9dce4
		if (mem_[wSceneObj3Frame] != 0x0a)
			break;
		play_sfx(mem_, SFX_FAT_WALK);
		advance();
		break;
	case 6: // .Func_9dcf5
		if (mem_[wSceneObj3State] == 0x03)
			advance();
		break;
	case 7: // .Func_9dcfe
		wait(40);
		break;
	case 8:
		end(120);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f423();
}

// Func_9f423: wSceneObj2 and 3. In state 2 (on to 3), wSceneObj3 steps at
// frames 7 and 8 in turn; in state 3, a sound every $1a frames (both kept
// in wSceneObj3Unk7, whichever object is drawn).
void Cutscenes::draw_func_9f423()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f423 + 0x2f, s));
			uint8_t now = mem_[state];
			if (now == 0x02) {
				// .Func_9f45a
				uint8_t frame = mem_[wSceneObj3Frame];
				Byte foot = mem_[wSceneObj3Unk7];
				if (frame == (foot ? 0x08 : 0x07)) {
					foot = static_cast<uint8_t>(foot ^ 1);
					play_sfx(mem_, SFX_FAT_WALK);
				}
				if (next_state_when_done(obj, 0x03))
					continue;
			} else if (now == 0x03) {
				loop_sound(wSceneObj3Unk7, 0x1a, SFX_076); // .Func_9f483
			}
			draw(obj, ADDR_OAM_bdfa5);
			break;
		}
	}
}

// InitCutscene22
void Cutscenes::init_treasure_map()
{
	routines_.call(ADDR_LoadCutscenes3Gfx);
	decompress(BANK_Cutscene10Attrmap, ADDR_Cutscene10Attrmap, v1BGMap0, true);
	decompress(BANK_Cutscene10Tilemap, ADDR_Cutscene10Tilemap, v0BGMap0);
	routines_.call(ADDR_ClearTempPals_Bank27);
}

// Cutscene22Func: fade in from black, and the map is opened
void Cutscenes::treasure_map()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d5f1: the palettes to fade to, from black
		copy_far(BANK_Pals_b8100, ADDR_Pals_b8100, wTempPals1, 8 * PAL_SIZE);
		copy_far(BANK_Pals_b8d00, ADDR_Pals_b8d00, wTempPals2, 8 * PAL_SIZE);
		routines_.call(ADDR_Func_9cc87);
		advance();
		break;
	case 2: // Func_9cdbd
		routines_.farcall(BANK_Func_84fff, ADDR_Func_84fff);
		break;
	case 3: // .Func_9d611
		wait(45);
		break;
	case 4: // .Func_9d616
		place(wSceneObj2, 76, 83, 0x01);
		play_sfx(mem_, SFX_103);
		advance();
		break;
	case 5: // .Func_9d631
		if (mem_[wSceneObj2Frame] != 0x05)
			break;
		play_sfx(mem_, SFX_107);
		advance();
		break;
	case 6: // .Func_9d642
		if (mem_[wSceneObj2State] == 0x02)
			advance();
		break;
	case 7:
		end(20);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f2cd
	animate_all({wSceneObj2State}, ADDR_Func_9f2cd + 0x22, ADDR_OAM_bda01, {{0x01, 0x02}});
}

// InitCutscene24, Func_9db85
void Cutscenes::init_open_tree_mouth()
{
	load_pals1(BANK_Pals_b82c0, ADDR_Pals_b82c0);
	load_pals2(BANK_Pals_b82c0, ADDR_Pals_b82c0);
	routines_.call(ADDR_LoadCutscenes8Gfx_Vram0);
	routines_.call(ADDR_LoadCutsceneBGMaps28);
	mem_[rVBK] = 1;
	gb_->cpu.b = BANK_SpecialTiles0;
	SET_HL(gb_, ADDR_SpecialTiles0);
	routines_.call(ADDR_LoadFarTiles);
	mem_[rVBK] = 0;
	place(wSceneObj2, 104, 80, 0x01);
	draw_func_9f3ee();
}

// Cutscene24Func: something runs past, and the tree's mouth opens and
// sinks
void Cutscenes::open_tree_mouth()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9dbb2
		if (frame() < 0x28)
			break;
		place(wSceneObj3, 70, 16, 0x02);
		advance();
		break;
	case 2: { // .Func_9dbc9: right, off the screen
		uint8_t x = ++mem_[wSceneObj3XCoord];
		if (x == 0x30) {
			play_sfx(mem_, SFX_128);
			break;
		}
		if (x != SCREEN_WIDTH_PX)
			break;
		mem_[wSceneObj3State] = 0;
		advance();
		break;
	}
	case 3: // .Func_9dbe5
		if (frame() < 0x46)
			break;
		++mem_[wSceneObj2YCoord];
		set_state(wSceneObj2, 0x03);
		advance();
		break;
	case 4: // .Func_9dbf8
		wait(40);
		break;
	case 5: { // .Func_9dbfd: down by .data_9dc1f's speeds (by frame, capped at $40)
		Byte f = wram_.unk_2d014(); // .Func_9dc0e
		if (f >= 0x44)
			f = 0x40;
		uint8_t dy = mem_[static_cast<uint16_t>(ADDR_Cutscene24Func + 0x87 + f)];
		Byte y = mem_[wSceneObj2YCoord];
		y += dy;
		if (y >= 0x88)
			advance();
		break;
	}
	case 6: // .Func_9dc63
		mem_[wSceneObj2State] = 0;
		advance();
		break;
	case 7:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f3ee();
}

// Func_9f3ee: wSceneObj2 and 3; a sound when state 3's animation ends
void Cutscenes::draw_func_9f3ee()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State}) {
		uint8_t s = mem_[state];
		if (!s)
			continue;
		uint16_t obj = animate(state, frameset(ADDR_Func_9f3ee + 0x20, s));
		if (mem_[state] == 0x03 && wram_.ow_animation_finished() == 0xff) // .Func_9f414
			play_sfx(mem_, SFX_131);
		draw(obj, ADDR_OAM_bde69);
	}
}

// InitCutscene27, Func_9d64b
void Cutscenes::init_cart_wheels()
{
	gb_->cpu.b = BANK_LevelMainTiles14;
	SET_HL(gb_, ADDR_LevelMainTiles14);
	routines_.call(ADDR_LoadFarTiles);
	mem_[rVBK] = 1;
	gb_->cpu.b = BANK_SpecialTiles1;
	SET_HL(gb_, ADDR_SpecialTiles1);
	routines_.call(ADDR_LoadFarTiles);
	mem_[rVBK] = 0;
	decompress(BANK_Cutscene11Attrmap, ADDR_Cutscene11Attrmap, v1BGMap0, true);
	decompress(BANK_Cutscene11Tilemap, ADDR_Cutscene11Tilemap, v0BGMap0);
	copy_far(BANK_Pals_cc5c0, ADDR_Pals_cc5c0, wTempPals1, 8 * PAL_SIZE);
	decompress(BANK_CartGfx, ADDR_CartGfx, v0Tiles1, true);
	routines_.copy(ADDR_InitCutscene27 + 0x91, wTempPals2 + 6 * PAL_SIZE, PAL_SIZE); // .palette
	place(wSceneObj2, 86, 88, 0x01);
	draw_cart();
}

// Cutscene27Func: the wheels go on, click by click
void Cutscenes::cart_wheels()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d674
		set_state(wSceneObj2, 0x02);
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 2: // .Func_9d687
		wait(50);
		break;
	case 3: // .Func_9d68c
		set_state(wSceneObj2, 0x03);
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 4: // .Func_9d69f
		wait(180);
		break;
	case 5:
		finish();
		break;
	default:
		unrealistic();
		return;
	}
	draw_cart();
}

// Func_9f2f3: the cart (bank $1a's animations, bank $62's sprites); a sound
// when state 3's animation ends
void Cutscenes::draw_cart()
{
	uint8_t state = mem_[wSceneObj2State];
	if (!state)
		return;
	uint16_t obj = animate(wSceneObj2State, frameset(ADDR_Func_9f2f3 + 0x2a, state), 0x1a);
	if (mem_[wSceneObj2State] == 0x03 && wram_.ow_animation_finished() == 0xff) // .Func_9f30e
		play_sfx(mem_, SFX_06D);
	draw(obj, ADDR_OAM_188279, 0x62);
}

// InitCutscene21
void Cutscenes::init_make_wire()
{
	load_pals1(BANK_Pals_b8300, ADDR_Pals_b8300);
	load_pals2(BANK_Pals_b8300, ADDR_Pals_b8300);
	routines_.call(ADDR_Func_9c977);
	gb_->cpu.b = BANK_LevelMainTiles22;
	SET_HL(gb_, ADDR_LevelMainTiles22);
	routines_.call(ADDR_LoadFarTiles);
	routines_.call(ADDR_LoadCutsceneBGMaps31);
}

// Cutscene21Func: the tool goes up, along the three spools right and back
// left (each winding up in turn), and away
void Cutscenes::make_wire()
{
	// .Func_9dd3f: up a pixel every 4 frames; .Func_9dd4a / .Func_9dd55:
	// right / left a pixel on odd frames; each returns the new coord
	auto up = [&] {
		Byte y = mem_[wSceneObj2YCoord];
		if (!(frame() & 0x03))
			--y;
		return static_cast<uint8_t>(y);
	};
	auto across = [&](uint8_t dx) {
		Byte x = mem_[wSceneObj2XCoord];
		if (frame() & 1)
			x += dx;
		return static_cast<uint8_t>(x);
	};
	auto advance_at = [&](uint8_t at, uint8_t want) {
		if (at == want)
			advance();
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9dd60
		mem_[wSceneObj3YCoord] = 72;
		mem_[wSceneObj3XCoord] = 72;
		mem_[wSceneObj4YCoord] = 72;
		mem_[wSceneObj4XCoord] = 88;
		mem_[wSceneObj5YCoord] = 72;
		mem_[wSceneObj5XCoord] = 104;
		mem_[wSceneObj2YCoord] = 128;
		mem_[wSceneObj2XCoord] = 72;
		[[fallthrough]];
	case 21: // .Func_9dd80
		set_state(wSceneObj2, 0x01);
		advance();
		break;
	case 2: // .Func_9dd8b
		if (up() != 0x48)
			break;
		set_state(wSceneObj2, 0x02);
		advance();
		break;
	case 3: // .Func_9dd9c
	case 6:
	case 9:
	case 13:
	case 16:
	case 19:
		wait(60);
		break;
	case 4: // .Func_9dda1
		mem_[wSceneObj3State] = 0x03;
		advance();
		break;
	case 5: // .Func_9dda9
		advance_at(across(1), 0x58);
		break;
	case 7: // .Func_9ddb3
		mem_[wSceneObj4State] = 0x03;
		advance();
		break;
	case 8: // .Func_9ddbb
		advance_at(across(1), 0x68);
		break;
	case 10: // .Func_9ddc2
		mem_[wSceneObj5State] = 0x03;
		advance();
		break;
	case 11: // .Func_9ddca
		if (across(1) != 0x88)
			break;
		mem_[wSceneObj2YCoord] = 0x38;
		advance();
		break;
	case 12: // .Func_9ddd8
		advance_at(across(0xff), 0x68);
		break;
	case 14: // .Func_9dddf
		mem_[wSceneObj5State] = 0x04;
		advance();
		break;
	case 15: // .Func_9dde3
		advance_at(across(0xff), 0x58);
		break;
	case 17: // .Func_9ddea
		mem_[wSceneObj4State] = 0x04;
		advance();
		break;
	case 18: // .Func_9ddee
		advance_at(across(0xff), 0x48);
		break;
	case 20: // .Func_9ddf5
		mem_[wSceneObj3State] = 0x04;
		advance();
		break;
	case 22: // .Func_9ddf9
		advance_at(up(), 0x20);
		break;
	case 23: // .Func_9de00
		mem_[wSceneObj2State] = 0;
		advance();
		break;
	case 24:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f49c();
}

// Func_9f49c: wSceneObj2 to 5; states 1 and 2 make wSceneObj2Unk7 click
// every 12 frames, state 2 also wSceneObj3Unk7 every 16 (whichever
// object is drawn)
void Cutscenes::draw_func_9f49c()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State}) {
		uint8_t s = mem_[state];
		if (!s)
			continue;
		uint16_t obj = animate(state, frameset(ADDR_Func_9f49c + 0x2f, s));
		uint8_t now = mem_[state];
		if (now == 0x01 || now == 0x02)
			periodic_sound(wSceneObj2Unk7, 0x0c, SFX_133); // .Func_9f4d8
		if (now == 0x02)
			periodic_sound(wSceneObj3Unk7, 0x10, SFX_132); // .Func_9f4f4
		draw(obj, ADDR_OAM_be206);
	}
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene1e);
PORT_COVERED(Func_9d536);
PORT_COVERED(Cutscene1eFunc);
PORT_COVERED(Func_9f2a2);
PORT_COVERED(InitCutscene1f);
PORT_COVERED(Func_9dc6a);
PORT_COVERED(Cutscene1fFunc);
PORT_COVERED(Func_9f423);
PORT_COVERED(InitCutscene22);
PORT_COVERED(Cutscene22Func);
PORT_COVERED(Func_9f2cd);
PORT_COVERED(InitCutscene24);
PORT_COVERED(Func_9db85);
PORT_COVERED(Cutscene24Func);
PORT_COVERED(Func_9f3ee);
PORT_COVERED(InitCutscene27);
PORT_COVERED(Func_9d64b);
PORT_COVERED(Cutscene27Func);
PORT_COVERED(Func_9f2f3);
PORT_COVERED(InitCutscene21);
PORT_COVERED(Cutscene21Func);
PORT_COVERED(Func_9f49c);
