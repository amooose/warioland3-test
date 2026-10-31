// Cutscene0eFunc and Cutscene11Func, one script: an item rises, the sky
// changes, and a cloud (the freeze) or the lightning comes. And
// Cutscene18Func, drawn the same way. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

namespace {

constexpr uint16_t FUNC_9F1F3_FRAMESETS = ADDR_Func_9f1f3 + 0x2a; // Func_9f1f3.framesets
constexpr uint8_t LIGHTNING_STEPS = 0x09; // the script's second list, less 1

} // namespace

// InitCutscene0e, InitCutscene11
void Cutscenes::init_freeze_sea(uint16_t pals1, uint16_t pals2)
{
	static_assert(BANK_Pals_b8200 == BANK_Pals_b8040 && BANK_Pals_b8980 == BANK_Pals_b8c40);
	load_pals1(BANK_Pals_b8200, pals1);
	load_pals2(BANK_Pals_b8980, pals2);
	routines_.call(ADDR_LoadCutscenes1Gfx_Vram0);
	routines_.call(ADDR_LoadCutsceneBGMaps03);
	routines_.call(ADDR_LoadCutsceneBGMaps21);
}

void Cutscenes::freeze_sea()
{
	// .asm_9d3aa: the sky's palettes, from pals
	auto change_sky = [&](uint16_t pals) {
		SET_HL(gb_, pals);
		SET_DE(gb_, wTempPals1);
		gb_->cpu.c = 8 * PAL_SIZE;
		gb_->cpu.b = BANK_Pals_b8040;
		routines_.call(ADDR_CopyFarBytes);
		mem_[wSceneObj1YCoord] = 0;
		mem_[wSceneObj2State] = 0;
		wram_.collection_row() = 0x08;
		set_bg_pals_to_load();
		advance();
	};
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d36a: the lightning has its own steps
		if (wram_.event_with_cutscene() == EVENT_SUMMON_LIGHTNING)
			wram_.unk_2d013() = LIGHTNING_STEPS;
		advance();
		break;
	case 2: // .Func_9d37a
	case 10:
		place(wSceneObj2, 172, 88, 0x01);
		mem_[wSceneObj1YCoord] = 1;
		advance();
		break;
	case 3: // .Func_9d392
	case 11:
		rise(wSceneObj2, 0x92, 0x6c);
		break;
	case 4: // .Func_9d3a2
	case 12:
		wait(60);
		break;
	case 5: // .Func_9d3a7
		change_sky(ADDR_Pals_b8040);
		break;
	case 6: // .Func_9d3dc
	case 14:
	case 15: // .Func_9d40e
		wait(30);
		break;
	case 7: // .Func_9d3e1
		place(wSceneObj2, 176, 50, 0x02);
		mem_[wSceneObj1YCoord] = 3;
		advance();
		break;
	case 8: { // .Func_9d3f9: up 2 pixels a frame
		Byte y = mem_[wSceneObj2YCoord];
		y -= 2;
		if (y == 0x74) {
			play_sfx(mem_, SFX_113); // .asm_9d438
			break;
		}
		if (y == 0x68)
			advance();
		break;
	}
	case 9:
	case 18:
		end(60);
		break;
	case 13: // .Func_9d409
		change_sky(ADDR_Pals_b8200);
		break;
	case 16: // .Func_9d413
		place(wSceneObj2, 164, 240, 0x03);
		advance();
		break;
	case 17: { // .Func_9d426: up and right 2 pixels a frame
		Byte y = mem_[wSceneObj2YCoord];
		y -= 2;
		mem_[wSceneObj2XCoord] += 2;
		if (y == 0x70) {
			play_sfx(mem_, SFX_113);
			break;
		}
		if (y == 0x64)
			advance();
		break;
	}
	default:
		unrealistic();
		return;
	}
	draw_func_9f1f3();
}

// Func_9f1f3: wSceneObj2 with its shadows
void Cutscenes::draw_func_9f1f3()
{
	draw_shadow(wSceneObj2, wCurSceneObjDuration, 0x01, 0x02); // Func_9fb75
	draw_shadow(wSceneObj2, wCurSceneObj, 0x03, 0x05); // Func_9fb97
	animate_all({wSceneObj2State}, FUNC_9F1F3_FRAMESETS, ADDR_OAM_bd7a1);
	draw_shadow(wSceneObj2, wCurSceneObjDuration, 0x01, 0x03); // Func_9fb7e
}

// InitCutscene18
void Cutscenes::init_purify_water()
{
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	load_pals2(BANK_Pals_b8d80, ADDR_Pals_b8d80);
	routines_.call(ADDR_LoadCutscenes1Gfx_Vram0);
	routines_.call(ADDR_LoadCutsceneBGMaps01);
	routines_.call(ADDR_LoadCutsceneBGMaps02);
}

// Cutscene18Func: the item rises
void Cutscenes::purify_water()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d87f
		place(wSceneObj2, 164, 112, 0x04);
		mem_[wSceneObj1YCoord] = 3;
		advance();
		break;
	case 2: // .Func_9d897: up and left
		rise(wSceneObj2, 0x8a, 0x64, 0xff);
		break;
	case 3: // .Func_9d8a8
		wait(20);
		break;
	case 4: // .Func_9d8ad
		wram_.collection_row() = 0x08;
		advance();
		break;
	case 5:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9f1f3();
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene0e);
PORT_COVERED(InitCutscene11);
PORT_COVERED(Cutscene0eFunc);
PORT_COVERED(Cutscene11Func);
PORT_COVERED(InitCutscene18);
PORT_COVERED(Cutscene18Func);
PORT_COVERED(Func_9f1f3);
