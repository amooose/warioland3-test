// The north side's cutscenes. See cutscenes.hpp.
#include "cutscenes.hpp"

#include <initializer_list>

namespace wl3::cutscenes {

namespace {

constexpr uint8_t OAM2_BANK = BANK_Frameset_bc1f3; // "Overworld OAM 2"
constexpr uint16_t FUNC_9F0E0_FRAMESETS = ADDR_Func_9f0e0 + 0x30; // Func_9f0e0.framesets

} // namespace

// InitCutscene02, InitCutscene04
void Cutscenes::init_north(uint16_t pals2, uint8_t pals2_bank, uint16_t gfx, uint16_t bg_maps1, uint16_t bg_maps2)
{
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	load_pals2(pals2_bank, pals2);
	routines_.call(gfx);
	routines_.call(bg_maps1);
	routines_.call(bg_maps2);
}

// Cutscene02Func: an axe swings in, speed lines, and the tree is cut
void Cutscenes::cut_tree()
{
	switch (wram_.unk_2d013()) {
	case 0: // StartCutsceneDelay
		wait(30);
		break;
	case 1: // .ShowAxe
		place(wSceneObj2, SCREEN_HEIGHT_PX, 0x6c, 0x01);
		mem_[wSceneObj1YCoord] = 3; // its shadow (draw_shadow)
		advance();
		break;
	case 2: // .RaiseAxe: up and left a pixel a frame
		rise(wSceneObj2, 0x86, 0x60, 0xff);
		break;
	case 3: // .Wait30
		wait(30);
		break;
	case 4: // .ShowSpeedLines
		wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
		advance();
		break;
	case 5: // .Wait60
		wait(60);
		break;
	case 6: // .HideAxe
		wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
		set_state(wSceneObj2, 0);
		mem_[wSceneObj1YCoord] = 0;
		advance();
		break;
	case 7: // .Wait16
		wait(16);
		break;
	case 8: // .ShowCut
		place(wSceneObj2, 96, 66, 0x02);
		play_sfx(mem_, SFX_101);
		advance();
		break;
	case 9: // .WaitCutAnimation
		if (!mem_[wSceneObj2State])
			advance();
		break;
	case 10:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_cut_tree();
}

// OWCutTree: the axe (state 1) or the cut (state 2, gone when it ends),
// with the shadow
void Cutscenes::draw_cut_tree()
{
	uint16_t obj;
	for (;;) {
		draw_shadow(wSceneObj2, wCurSceneObj, 0x03, 0x05); // Func_9fb97
		uint8_t state = mem_[wSceneObj2State];
		if (!state)
			return;
		if (state == 1) {
			obj = animate(wSceneObj2State, ADDR_Frameset_bc1f3, OAM2_BANK);
			break;
		}
		if (state != 2) { // debug_nop
			unrealistic();
			return;
		}
		obj = animate(wSceneObj2State, ADDR_Frameset_bc1e8, OAM2_BANK);
		if (!next_state_when_done(obj, 0))
			break;
	}
	draw(obj, ADDR_OAM_bc122);
}

// Cutscene03Func: the north gate opens
void Cutscenes::open_north_gate()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9cf36
		place(wSceneObj2, 59, 40, 0x01);
		place(wSceneObj3, 59, 120, 0x02);
		advance();
		break;
	case 2: // .Func_9cf59: the two close in, every other frame
		if (frame() & 1)
			break;
		--mem_[wSceneObj3XCoord];
		++mem_[wSceneObj2XCoord];
		if (mem_[wSceneObj2XCoord] == 79)
			advance();
		break;
	case 3: // .Wait30_1
	case 5: // .Wait30_2
	case 12: // .Wait30_3
	case 16: // .Wait30_4
		wait(30);
		break;
	case 4: // .Func_9cf71
		set_state(wSceneObj2, 0x03);
		mem_[wSceneObj2XCoord] = 80;
		mem_[wSceneObj3State] = 0;
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 6: // .Func_9cf92
		place(wSceneObj3, 59, 80, 0x05);
		play_sfx(mem_, SFX_106);
		advance();
		break;
	case 7: // .Func_9cfad
		if (!mem_[wSceneObj3State])
			advance();
		break;
	case 8: // .Wait60_1
	case 10: // .Wait60_2
		wait(60);
		break;
	case 9: // .Func_9cfba: down, every other frame
		if (!(frame() & 1))
			break;
		++mem_[wSceneObj2YCoord];
		if (mem_[wSceneObj2YCoord] == 0x54)
			advance();
		break;
	case 11: // .Func_9cfce
		set_state(wSceneObj2, 0x04);
		++mem_[wSceneObj2XCoord];
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 13: // .Func_9cfea
		mem_[wSceneObj3YCoord] = mem_[wSceneObj2YCoord];
		mem_[wSceneObj3XCoord] = mem_[wSceneObj2XCoord];
		set_state(wSceneObj3, 0x06);
		advance();
		break;
	case 14: // .Func_9d001
		if (mem_[wSceneObj3State] == 0x07)
			advance();
		break;
	case 15: // .Func_9d00a
		play_sfx(mem_, SFX_10C);
		advance();
		break;
	case 17:
		fade_in();
		break;
	case 18:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f0e0();
}

// Func_9f0e0: wSceneObj3 and wSceneObj2; states 5 and 6 end in 0 and 7
void Cutscenes::draw_func_9f0e0()
{
	for (uint16_t state : {wSceneObj3State, wSceneObj2State}) {
		uint16_t obj = 0;
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			obj = animate(state, frameset(FUNC_9F0E0_FRAMESETS, s));
			uint8_t now = mem_[state];
			if (now != 5 && now != 6) {
				draw(obj, ADDR_OAM_bd1fd);
				break;
			}
			if (!next_state_when_done(obj, now == 5 ? 0x00 : 0x07)) {
				draw(obj, ADDR_OAM_bd1fd);
				break;
			}
		}
	}
}

} // namespace wl3::cutscenes

PORT_COVERED(Cutscene02Func);
PORT_COVERED(OWCutTree);
PORT_COVERED(Cutscene03Func);
PORT_COVERED(Func_9f0e0);

namespace wl3::cutscenes {

// Cutscene04Func: the tornado is blown away
void Cutscenes::expel_tornado()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9db1b
		if (frame() < 0x14)
			break;
		place(wSceneObj2, 128, 80, 0x02);
		advance();
		break;
	case 2: // .Func_9db32: up
		rise(wSceneObj2, 0x70, 0x4a);
		break;
	case 3: // .Func_9db46
		if (frame() < 0x32)
			break;
		place(wSceneObj3, 128, 80, 0x03);
		advance();
		break;
	case 4: { // .Func_9db5d
		Byte y = mem_[wSceneObj3YCoord];
		--y;
		if (y != 0x4a)
			break;
		play_sfx(mem_, SFX_113);
		advance();
		break;
	}
	case 5: // .Func_9db71
		set_state(wSceneObj2, 0x01);
		mem_[wSceneObj3State] = 0;
		wram_.collection_row() = 0x08;
		advance();
		break;
	case 6:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	animate_all({wSceneObj3State, wSceneObj2State}, ADDR_Func_9f3cd + 0x1b, ADDR_OAM_bddcf); // Func_9f3cd
}

} // namespace wl3::cutscenes

PORT_COVERED(Cutscene04Func);
PORT_COVERED(Func_9f3cd);

namespace wl3::cutscenes {

// InitCutscene06
void Cutscenes::init_rainstorm()
{
	routines_.call(ADDR_LoadCutscenes3Gfx);
	decompress(BANK_Cutscene08Attrmap, ADDR_Cutscene08Attrmap, v1BGMap0, true);
	decompress(BANK_Cutscene08Tilemap, ADDR_Cutscene08Tilemap, v0BGMap0);
	load_pals1(BANK_Pals_b8080, ADDR_Pals_b8080);
	load_pals2(BANK_Pals_b8900, ADDR_Pals_b8900);
	place(wSceneObj2, 72, 80, 0x01); // Func_9d01a
	draw_func_9f11e();
}

// Cutscene06Func: a cloud bobs, rises, and rains
void Cutscenes::rainstorm()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: { // .Func_9d04b: bob a pixel every 4 frames, until frame $20
		uint8_t f = frame();
		if (f == 1) {
			play_sfx(mem_, SFX_102);
			break;
		}
		if (f & 0x03)
			break;
		mem_[wSceneObj2YCoord] += (f & (1 << 2)) ? 0xff : 0x01;
		if (f >= 0x20)
			advance();
		break;
	}
	case 2: // .Func_9d071
	case 4:
		wait(30);
		break;
	case 3: { // .Func_9d076: up, faster and faster, to y $20
		uint8_t f = frame();
		if (f == 1) {
			play_sfx(mem_, SFX_TEMPLE_ROCK);
			break;
		}
		uint8_t dy = f < 0x08 ? 0xff : f < 0x10 ? 0xfe : 0xfd;
		Byte y = mem_[wSceneObj2YCoord];
		y += dy;
		if (y < 0x20)
			advance();
		break;
	}
	case 5: // .Func_9d09c
		place(wSceneObj2, 64, 80, 0x02);
		advance();
		break;
	case 6: // .Func_9d0af
		if (mem_[wSceneObj2State] != 0x03)
			break;
		wram_.colour_fade_diff() = 0;
		advance();
		break;
	case 7: // .Func_9d0bc
		wait(100);
		break;
	case 8: { // .Func_9d0c1: the rain's sound every 14 frames, and the cloud
		// up a pixel every 4
		Byte rain = wram_.colour_fade_diff(); // .Func_9d0d3
		++rain;
		if (rain >= 0x0e) {
			rain = 0;
			play_sfx(mem_, SFX_023);
		}
		if (frame() & 0x03)
			break;
		Byte y = mem_[wSceneObj2YCoord];
		--y;
		if (y == 0x28)
			advance();
		break;
	}
	case 9:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f11e();
}

// Func_9f11e: wSceneObj2; state 2 ends in 3
void Cutscenes::draw_func_9f11e()
{
	uint16_t obj;
	for (;;) {
		uint8_t s = mem_[wSceneObj2State];
		if (!s)
			return;
		obj = animate(wSceneObj2State, frameset(ADDR_Func_9f11e + 0x22, s));
		if (mem_[wSceneObj2State] != 0x02 || !next_state_when_done(obj, 0x03))
			break;
	}
	draw(obj, ADDR_OAM_bd377);
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene06);
PORT_COVERED(Func_9d01a);
PORT_COVERED(Cutscene06Func);
PORT_COVERED(Func_9f11e);

namespace wl3::cutscenes {

// InitCutscene07, Func_9d158
void Cutscenes::init_fix_elevator()
{
	routines_.call(ADDR_LoadCutscenes1Gfx_Vram0);
	decompress(BANK_Cutscene06Attrmap, ADDR_Cutscene06Attrmap, v1BGMap0, true);
	decompress(BANK_Cutscene06Tilemap, ADDR_Cutscene06Tilemap, v0BGMap0);
	load_pals1(BANK_Pals_b8000, ADDR_Pals_b8000);
	load_pals2(BANK_Pals_b89c0, ADDR_Pals_b89c0);
	place(wSceneObj2, 88, 84, 0x09);
	place(wSceneObj3, 67, 64, 0x03);
	place(wSceneObj4, 88, 84, 0x0a);
	draw_func_9f174();
}

// Cutscene07Func: the elevator's gears are fixed, and it goes up
void Cutscenes::fix_elevator()
{
	uint8_t step = wram_.unk_2d013();
	switch (step) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d1c5
		if (frame() < 0x14)
			break;
		place(wSceneObj5, 90, 56, 0x01);
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 2: // .Func_9d1e4
	case 15: // .Func_9d234
	case 20: // .Func_9d264
		wait(50);
		break;
	case 3: // .Func_9d1e9
		set_state(wSceneObj5, 0x02);
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 4: // .Func_9d1fc
		wait(30);
		break;
	case 5: // .Func_9d201
		set_state(wSceneObj3, 0x04);
		advance();
		break;
	case 6: // .Func_9d20c
	case 17: // .Func_9d244
		wait(40);
		break;
	case 7: // .Func_9d211
		set_state(wSceneObj2, 0x06);
		advance();
		break;
	case 8: // .Func_9d21c: the wheel's frame back to 0, four times
	case 10:
	case 12:
	case 14:
		if (mem_[wSceneObj2Frame])
			break;
		play_sfx(mem_, SFX_109);
		advance();
		break;
	case 9: // .Func_9d22c: and away from it
	case 11:
	case 13:
		if (mem_[wSceneObj2Frame])
			advance();
		break;
	case 16: // .Func_9d239
		set_state(wSceneObj3, 0x05);
		advance();
		break;
	case 18: // .Func_9d249
		set_state(wSceneObj4, 0x08);
		play_sfx(mem_, SFX_035);
		advance();
		break;
	case 19: // .Func_9d25c
		if (!mem_[wSceneObj4State])
			advance();
		break;
	case 21: // .Func_9d269
		play_sfx(mem_, SFX_10B);
		advance();
		break;
	case 22: { // .Func_9d274: up to y $18
		Byte y = mem_[wSceneObj2YCoord];
		--y;
		if (y < 0x18)
			advance();
		break;
	}
	case 23:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f174();
}

// Func_9f174: wSceneObj4, 2, 3 and 5; states 6 and 8 end in 7 and 0, state 5
// clicks when its animation ends
void Cutscenes::draw_func_9f174()
{
	for (uint16_t state : {wSceneObj4State, wSceneObj2State, wSceneObj3State, wSceneObj5State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f174 + 0x50, s));
			uint8_t now = mem_[state];
			if ((now == 0x06 || now == 0x08) && next_state_when_done(obj, now == 0x06 ? 0x07 : 0x00))
				continue;
			if (now == 0x05 && wram_.ow_animation_finished() == 0xff) // .Func_9f1b5
				play_sfx(mem_, SFX_10A);
			draw(obj, ADDR_OAM_bd46f);
			break;
		}
	}
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene07);
PORT_COVERED(Func_9d158);
PORT_COVERED(Cutscene07Func);
PORT_COVERED(Func_9f174);

namespace wl3::cutscenes {

// InitCutscene0a, Func_9d0e6
void Cutscenes::init_raise_tower()
{
	load_pals1(BANK_Pals_b8040, ADDR_Pals_b8040);
	load_pals2(BANK_Pals_b8a00, ADDR_Pals_b8a00);
	routines_.call(ADDR_LoadCutscenes1Gfx_Vram0);
	routines_.call(ADDR_LoadCutsceneBGMaps03);
	mem_[wSceneObj1YCoord] = 3; // the shadows (draw_shadow)
	place(wSceneObj2, 130, 113, 0x01);
	place(wSceneObj3, 130, 47, 0x02);
	draw_func_9f146();
}

// Cutscene0aFunc: the tower's two halves rise
void Cutscenes::raise_tower()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d126: every other frame
		if (!(frame() & 1))
			break;
		if (raise_tower_step() == 0x63)
			advance();
		break;
	case 2: // .Func_9d140
		wait(50);
		break;
	case 3: // .Func_9d145
		raise_tower_step();
		play_sfx(mem_, SFX_106);
		advance();
		break;
	case 4: // .Func_9d153
		wait(30);
		break;
	case 5:
		fade_in();
		break;
	case 6:
		end(20);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f146();
}

// .Func_9d133: both up a pixel, apart a pixel; their new y
uint8_t Cutscenes::raise_tower_step()
{
	uint8_t y = static_cast<uint8_t>(mem_[wSceneObj2YCoord] - 1);
	mem_[wSceneObj2YCoord] = y;
	--mem_[wSceneObj2XCoord];
	mem_[wSceneObj3YCoord] = y;
	++mem_[wSceneObj3XCoord];
	return y;
}

// Func_9f146: wSceneObj2 and 3, and their shadows
void Cutscenes::draw_func_9f146()
{
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f146 + 0x2a, ADDR_OAM_bd453);
	draw_shadow(wSceneObj2, wCurSceneObj, 0x03, 0x05); // Func_9fb97
	draw_shadow(wSceneObj3, wCurSceneObjDuration, 0x03, 0x06); // Func_9fba0
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene0a);
PORT_COVERED(Func_9d0e6);
PORT_COVERED(Cutscene0aFunc);
PORT_COVERED(Func_9f146);

namespace wl3::cutscenes {

// InitCutscene0c
void Cutscenes::init_blow_mist()
{
	load_pals1(BANK_Pals_b8040, ADDR_Pals_b8040);
	load_pals2(BANK_Pals_b8d40, ADDR_Pals_b8d40);
	routines_.call(ADDR_LoadCutscenes4Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps03);
}

// Cutscene0cFunc: the bellows rise
void Cutscenes::blow_mist()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d840
		place(wSceneObj2, 164, 120, 0x01);
		mem_[wSceneObj1YCoord] = 3;
		advance();
		break;
	case 2: // .Func_9d858: up and left
		rise(wSceneObj2, 0x8a, 0x64, 0xff);
		break;
	case 3:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f323
	draw_shadow(wSceneObj2, wCurSceneObj, 0x03, 0x05); // Func_9fb97
	animate_all({wSceneObj2State}, ADDR_Func_9f323 + 0x1b, ADDR_OAM_bdabc);
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene0c);
PORT_COVERED(Cutscene0cFunc);
PORT_COVERED(Func_9f323);

namespace wl3::cutscenes {

// InitCutscene0d
void Cutscenes::init_summon_snakes()
{
	load_pals1(BANK_Pals_b8040, ADDR_Pals_b8040);
	load_pals2(BANK_Pals_b8dc0, ADDR_Pals_b8dc0);
	routines_.call(ADDR_LoadCutscenes4Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps01);
	routines_.call(ADDR_LoadCutsceneBGMaps02);
}

// Cutscene0dFunc: the flute rises and plays; the snakes come and go
void Cutscenes::summon_snakes()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d8d9
		place(wSceneObj2, 156, 72, 0x01);
		advance();
		break;
	case 2: // .Func_9d8ec
		rise(wSceneObj2, 0x7a, 0x54);
		break;
	case 3: // .Func_9d8fc
		wait(20);
		break;
	case 4: // .Func_9d901
		wram_.collection_row() = 0x08;
		advance();
		break;
	case 5: // .Func_9d909
		wait(60);
		break;
	case 6: // .Func_9d90e
		place(wSceneObj3, 88, 64, 0x02);
		play_sfx(mem_, SFX_117);
		advance();
		break;
	case 7: // .Func_9d929: wSceneObj3 left on odd frames, to x $1c, then
	case 9: // wSceneObj4 takes over
		if (!(frame() & 1))
			break;
		if (--mem_[wSceneObj3XCoord] != 0x1c)
			break;
		mem_[wSceneObj3State] = 0;
		place(wSceneObj4, 80, 60, 0x03);
		advance();
		break;
	case 8: // .Func_9d94c: wSceneObj4 left on even frames, to x $18
		if (!snake_left())
			break;
		mem_[wSceneObj4State] = 0;
		place(wSceneObj3, 88, 64, 0x02);
		advance();
		break;
	case 10: // .Func_9d973
		if (!snake_left())
			break;
		mem_[wSceneObj4State] = 0;
		advance();
		break;
	case 11: // .Func_9d980
		wait(80);
		break;
	case 12:
		end(20);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f340
	animate_all({wSceneObj3State, wSceneObj4State, wSceneObj2State}, ADDR_Func_9f340 + 0x21, ADDR_OAM_bdb06);
}

// .Func_9d967: on even frames, wSceneObj4 left a pixel; whether it got to x $18
bool Cutscenes::snake_left()
{
	if (frame() & 1)
		return false;
	return --mem_[wSceneObj4XCoord] == 0x18;
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene0d);
PORT_COVERED(Cutscene0dFunc);
PORT_COVERED(Func_9f340);
PORT_COVERED(InitCutscene02);
PORT_COVERED(InitCutscene03);
PORT_COVERED(InitCutscene04);
