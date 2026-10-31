// Cutscene19Func, the castle appears, and Cutscene1bFunc, the sun is
// summoned (with the owl). See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

// InitCutscene19
void Cutscenes::init_reveal_castle()
{
	load_pals1(BANK_Pals_b8140, ADDR_Pals_b8140);
	load_pals2(BANK_Pals_b8940, ADDR_Pals_b8940);
	routines_.call(ADDR_LoadCutscenes4Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps12);
}

// Cutscene19Func: the item rises and is used; BG palette 2 lightens in
// four steps
void Cutscenes::reveal_castle()
{
	// .asm_9d302: BG palette 2 from pals
	auto set_pal2 = [&](uint16_t pals) {
		constexpr uint16_t PAL2 = wTempPals1 + 2 * PAL_SIZE;
		routines_.copy(pals, PAL2, PAL_SIZE);
		wram_.pal_config1_source_hi() = PAL2 >> 8;
		wram_.pal_config1_source_lo() = PAL2 & 0xff;
		wram_.pal_config1_index() = BGPI_AUTOINC | 2 * PAL_SIZE;
		wram_.pal_config1_number() = 1;
		wram_.pal_config1_register() = static_cast<uint8_t>(rBGPI);
		advance();
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d2a9
		place(wSceneObj2, SCREEN_HEIGHT_PX, 0x50, 0x01);
		advance();
		break;
	case 2: // .Func_9d2bc
		rise(wSceneObj2, 0x86, 0x60);
		break;
	case 3: // .Func_9d2cc
		wait(90);
		break;
	case 4: // .Func_9d2d1
		set_state(wSceneObj2, 0x02);
		play_sfx(mem_, SFX_073);
		advance();
		break;
	case 5: // .Func_9d2e4
		wait(30);
		break;
	case 6: // .Func_9d2e9
		if (mem_[wSceneObj2Frame] != 0x04)
			break;
		set_state(wSceneObj2, 0x03);
		advance();
		break;
	case 7: // .Func_9d2fa
		wait(60);
		break;
	case 8: // .Func_9d2ff
		set_pal2(ADDR_Pals_9fc8b);
		break;
	case 9: // .Func_9d326
	case 11:
	case 13:
		wait(8);
		break;
	case 10: // .Func_9d32b
		set_pal2(ADDR_Pals_9fc93);
		break;
	case 12: // .Func_9d330
		set_pal2(ADDR_Pals_9fc9b);
		break;
	case 14: // .Func_9d335
		set_pal2(ADDR_Pals_9fca3);
		break;
	case 15:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	// Func_9f1d8
	animate_all({wSceneObj2State}, ADDR_Func_9f1d8 + 0x15, ADDR_OAM_bd615);
}

// InitCutscene1b
void Cutscenes::init_summon_sun()
{
	routines_.call(ADDR_LoadCutscenes4Gfx);
	decompress(BANK_OwlGfx, ADDR_OwlGfx, v1Tiles0, true);
	mem_[rVBK] = 1;
	gb_->cpu.b = BANK_LevelMainTiles28;
	SET_HL(gb_, ADDR_LevelMainTiles28);
	routines_.call(ADDR_LoadFarTiles);
	mem_[rVBK] = 0;
	routines_.call(ADDR_LoadCutsceneBGMaps03);
	decompress(BANK_Cutscene33Attrmap, ADDR_Cutscene33Attrmap, v1BGMap1, true);
	decompress(BANK_Cutscene33Tilemap, ADDR_Cutscene33Tilemap, v0BGMap1);
	load_pals1(BANK_Pals_b8040, ADDR_Pals_b8040);
	load_pals2(BANK_Pals_b8c80, ADDR_Pals_b8c80);
	routines_.call(ADDR_SetBGPriorityOnTopRows_BGMap1);
}

// Cutscene1bFunc: two halves close in, the palettes fade and change, and
// the owl comes down
void Cutscenes::summon_sun()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d46c
		place(wSceneObj2, 50, 80, 0x01);
		place(wSceneObj3, 130, 80, 0x02);
		advance();
		break;
	case 2: { // .Func_9d48f: together, on odd frames
		if (!(frame() & 1))
			break;
		uint8_t y = ++mem_[wSceneObj2YCoord]; // .Func_9d4a4
		--mem_[wSceneObj3YCoord];
		if (y != 0x5a)
			break;
		play_sfx(mem_, SFX_CLICK);
		advance();
		break;
	}
	case 3: // .Func_9d4ae
		wait(90);
		break;
	case 4: // Func_9cd9d
		routines_.farcall(BANK_Func_850b9, ADDR_Func_850b9);
		break;
	case 5: // .Func_9d4b3
		wait(8);
		break;
	case 6: // Func_9cdad
		routines_.farcall(BANK_Func_850ff, ADDR_Func_850ff);
		break;
	case 7: // .Func_9d4b8
		wait(60);
		break;
	case 8: // .Func_9d4bd: the new palettes, BG and OB
		mem_[wSceneObj2State] = 0;
		mem_[wSceneObj3State] = 0;
		copy_far(BANK_Pals_b8340, ADDR_Pals_b8340, wTempPals1, 8 * PAL_SIZE);
		set_bg_pals_to_load();
		copy_far(BANK_Pals_b8340, ADDR_Pals_b8340, wTempPals2, 8 * PAL_SIZE);
		set_ob_pals_to_load();
		wram_.collection_row() = 0x08;
		advance();
		break;
	case 9: // .Func_9d4ec
		place(wSceneObj5, 32, 80, 0x02);
		advance();
		break;
	case 10: // .Func_9d4ff
		wait(20);
		break;
	case 11: { // .Func_9d504: the owl down, every 4 frames, then every 8
		Byte y = mem_[wSceneObj5];
		uint8_t mask = y < 0x44 ? 0x03 : 0x07;
		if (wram_.unk_2d014() & mask)
			break;
		++y;
		if (y == 0x4e)
			advance();
		break;
	}
	case 12: // .Func_9d51d
		if (frame() < 0x1e)
			break;
		set_state(wSceneObj5, 0x01);
		mem_[wSceneObj5] = mem_[wSceneObj5]; // add [hl] with a = 0
		advance();
		break;
	case 13: // .Func_9d531
		wait(100);
		break;
	case 14:
		end(120);
		break;
	default:
		unrealistic();
		return;
	}
	animate_all({wSceneObj2State, wSceneObj3State}, ADDR_Func_9f225 + 0x1b, ADDR_OAM_bd911); // Func_9f225
	draw_owl();
}

// Func_9f244: the owl (bank $1a's animations, bank $62's sprites): flapping
// (state 2) or perched (state 1, a hoot when its frame goes back to 1)
void Cutscenes::draw_owl()
{
	uint8_t state = mem_[wSceneObj5State];
	if (!state)
		return;
	uint16_t obj = animate(wSceneObj5State, frameset(ADDR_Func_9f244 + 0x1e, state), 0x1a); // Func_9f0b5
	uint8_t now = mem_[wSceneObj5State];
	if (now == 0x02) { // .Func_9f26b
		if (wram_.ow_animation_finished() == 0xff)
			play_sfx(mem_, SFX_FLAP);
	} else if (now == 0x01) { // .Func_9f27f: unk7 tracks frame 1
		uint8_t frame = mem_[static_cast<uint16_t>(obj | 2)];
		Byte seen = mem_[static_cast<uint16_t>(obj | 7)];
		if (!seen) {
			if (frame != 1)
				seen = 1;
		} else if (frame == 1) {
			seen = 0;
			play_sfx(mem_, SFX_011);
		}
	}
	draw(obj, ADDR_OAM_188e16, 0x62); // Func_9f0c1
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene19);
PORT_COVERED(Cutscene19Func);
PORT_COVERED(Func_9f1d8);
PORT_COVERED(InitCutscene1b);
PORT_COVERED(Cutscene1bFunc);
PORT_COVERED(Func_9f225);
PORT_COVERED(Func_9f244);
