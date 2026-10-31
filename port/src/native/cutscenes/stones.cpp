// Cutscene13Func, the foot stone, and Cutscene15Func / Cutscene49Func, the
// snake doors. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

// InitCutscene13
void Cutscenes::init_foot_stone()
{
	load_pals1(BANK_Pals_b8040, ADDR_Pals_b8040);
	load_pals2(BANK_Pals_b8e00, ADDR_Pals_b8e00);
	wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
	routines_.call(ADDR_LoadCutscenes4Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps05);
}

// Cutscene13Func: the stone rises and falls, its shadow down, and the screen
// shakes
void Cutscenes::foot_stone()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d9a1
		place(wSceneObj2, 176, 72, 0x01);
		mem_[wSceneObj3YCoord] = mem_[wSceneObj2YCoord];
		mem_[wSceneObj3XCoord] = mem_[wSceneObj2XCoord];
		mem_[wSceneObj1YCoord] = 1;
		advance();
		break;
	case 2: { // .Func_9d9c5: up, with the shadow
		Byte y = mem_[wSceneObj2YCoord];
		--y;
		mem_[wSceneObj3YCoord] = y;
		if (y == 0x86) {
			play_sfx(mem_, SFX_112); // Func_9ce1f
			break;
		}
		if (y == 0x60)
			advance();
		break;
	}
	case 3: { // .Func_9d9d8: on up, to y $40
		Byte y = mem_[wSceneObj2YCoord];
		--y;
		if (y >= 0x40)
			break;
		y = 0x40;
		advance();
		break;
	}
	case 4: // .Func_9d9e5: the shadow down on odd frames, off the screen
		if (!(frame() & 1))
			break;
		if (++mem_[wSceneObj3YCoord] == SCREEN_HEIGHT_PX)
			advance();
		break;
	case 5: // .Func_9d9f4
		wait(30);
		break;
	case 6: { // .Func_9d9f9: the stone falls, 3 pixels a frame
		Byte y = mem_[wSceneObj2YCoord];
		y += 3;
		if (y < 0xb0)
			break;
		play_sfx(mem_, SFX_061);
		advance();
		break;
	}
	case 7:
		shake_screen();
		break;
	case 8:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f367
	draw_shadow(wSceneObj3, wCurSceneObj, 0x01, 0x00); // Func_9fb4c
	animate_all({wSceneObj2State}, ADDR_Func_9f367 + 0x24, ADDR_OAM_bdc9c);
	draw_shadow(wSceneObj3, wCurSceneObjDuration, 0x01, 0x01); // Func_9fb6c
}

// InitCutscene15, InitCutscene49: the door's halves in the window and the
// BG, which scroll apart
void Cutscenes::init_snake_door(uint16_t pals1, uint16_t pals2)
{
	static_assert(BANK_Pals_b81c0 == BANK_Pals_b8180 && BANK_Pals_b8e80 == BANK_Pals_b8e40);
	load_pals1(BANK_Pals_b81c0, pals1);
	load_pals2(BANK_Pals_b8e80, pals2);
	wram_.lcdc_flags_to_flip() = LCDC_WINDOW; // .asm_9c367
	mem_[rWY] = 0x28;
	wram_.cutscene_wy() = 0x28;
	mem_[rWX] = 80 + WX_OFS;
	wram_.cutscene_wx() = 80 + WX_OFS;
	routines_.call(ADDR_LoadCutscenes2Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps14);
	routines_.call(ADDR_LoadCutsceneBGMaps13);
	// Func_9da0e
	set_state(wSceneObj4, 0x05);
	mem_[wSceneObj4] = 96;
	mem_[wSceneObj4 + 1] = 80;
	set_state(wSceneObj5, 0x06);
	mem_[wSceneObj5] = 96;
	mem_[wSceneObj5 + 1] = 80;
	draw_snake_door();
}

// Cutscene15Func / Cutscene49Func: the snake drops in, bites the lock,
// and the door slides open
void Cutscenes::snake_door()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9da4d
		wait(20);
		break;
	case 2: // .Func_9da52
		set_state(wSceneObj2, 0x01);
		mem_[wSceneObj2YCoord] = 0;
		mem_[wSceneObj2XCoord] = 80;
		advance();
		break;
	case 3: // .Func_9da65: down on even frames
		if (frame() & 1)
			break;
		if (++mem_[wSceneObj2YCoord] != 0x45)
			break;
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	case 4: // .Func_9da7c
		if (frame() < 0x14)
			break;
		set_state(wSceneObj2, 0x04);
		advance();
		break;
	case 5: // .Func_9da8b: the bite (its sound at frame 3), until state 1
		if (mem_[wSceneObj2State] != 0x01) {
			if (mem_[wSceneObj2Frame] == 0x03 && !mem_[wSceneObj2Duration])
				play_sfx(mem_, SFX_12C);
			break;
		}
		set_state(wSceneObj2, 0x02);
		set_state(wSceneObj3, 0x03);
		mem_[wSceneObj3YCoord] = mem_[wSceneObj2YCoord];
		mem_[wSceneObj3XCoord] = mem_[wSceneObj2XCoord];
		advance();
		break;
	case 6: // .Func_9dac4
		wait(40);
		break;
	case 7: { // .Func_9dac9: apart a pixel every 8 frames, rumbling
		Byte rumble = mem_[wSceneObj4Unk7]; // .Func_9daf0
		++rumble;
		if (rumble >= 0x11) {
			rumble = 0;
			play_sfx(mem_, SFX_01F);
		}
		if (frame() & 0x07)
			break;
		++wram_.cutscene_wx();
		++mem_[wSceneObj5XCoord];
		++mem_[wSceneObj3XCoord];
		++wram_.scx();
		--mem_[wSceneObj4XCoord];
		if (--mem_[wSceneObj2XCoord] == 0x40)
			advance();
		break;
	}
	case 8:
		end(40);
		break;
	default:
		unrealistic();
		return;
	}
	draw_snake_door();
}

// Func_9f38d: wSceneObj2 to 5; state 4 goes back to 1
void Cutscenes::draw_snake_door()
{
	for (uint16_t state : {wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State}) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(ADDR_Func_9f38d + 0x34, s));
			if (mem_[state] == 0x04 && next_state_when_done(obj, 0x01))
				continue;
			draw(obj, ADDR_OAM_bdce6);
			break;
		}
	}
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene13);
PORT_COVERED(Cutscene13Func);
PORT_COVERED(Func_9f367);
PORT_COVERED(InitCutscene15);
PORT_COVERED(InitCutscene49);
PORT_COVERED(Func_9da0e);
PORT_COVERED(Cutscene15Func);
PORT_COVERED(Cutscene49Func);
PORT_COVERED(Func_9f38d);
