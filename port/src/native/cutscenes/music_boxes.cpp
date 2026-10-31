// The music boxes' cutscenes (Cutscene08Func, 10, 17, 1d and 25): the box
// rises, opens and plays its tune. See cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

namespace {

static_assert(wSceneObj2Duration == wSceneObj2State - 2);

constexpr uint16_t MUSIC_BOX_FRAMESETS = ADDR_Func_9fc1d + 0x27; // Func_9fc1d.framesets

} // namespace

// InitCutscene08, 10, 17, 1d and 25 (Func_9c25f): the box's palettes
void Cutscenes::init_music_box(uint16_t pals)
{
	load_pals2(BANK_Pals_b8ac0, pals);
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	wram_.lcdc_flags_to_flip() = LCDC_BG_MAP;
	routines_.call(ADDR_LoadCutscenes2Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps01);
	routines_.call(ADDR_LoadCutsceneBGMaps04);
}

// Func_9d6f0: box is the box's first state (1, 3, 5, 7, 9)
void Cutscenes::music_box(uint8_t box)
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d70c
		place(wSceneObj2, 176, 74, box);
		mem_[wSceneObj1YCoord] = 1; // its shadows (draw_shadow)
		advance();
		break;
	case 2: // .Func_9d723
		rise(wSceneObj2, 0x88, 0x62);
		break;
	case 3: // .Func_9d733
		wait(30);
		break;
	case 4: // .Func_9d738: open
		set_state(wSceneObj2, static_cast<uint8_t>(mem_[wSceneObj2State] + 1));
		advance();
		break;
	case 5: // .Func_9d744
		wait(50);
		break;
	case 6: { // .Func_9d749: its tune
		advance();
		static constexpr uint16_t TUNES[5] = {SFX_11E, SFX_11F, SFX_120, SFX_121, SFX_122};
		uint8_t closed = static_cast<uint8_t>(mem_[wSceneObj2State] - 1);
		if (closed < 10 && (closed & 1))
			play_sfx(mem_, TUNES[closed / 2]);
		break;
	}
	case 7: // .Func_9d792
		wait(254);
		break;
	case 8:
		end(80);
		break;
	default:
		unrealistic();
		return;
	}
	draw_music_box();
}

// Func_9fc1d: the box, between its two shadow sprites
void Cutscenes::draw_music_box()
{
	draw_shadow(wSceneObj2, wCurSceneObj, 0x01, 0x00); // Func_9fb4c
	uint8_t state = mem_[wSceneObj2State]; // .Func_9fc2c
	if (state)
		draw(animate(wSceneObj2State, frameset(MUSIC_BOX_FRAMESETS, state)), ADDR_OAM_bc1f6);
	draw_shadow(wSceneObj2, wCurSceneObjDuration, 0x01, 0x01); // Func_9fb6c
}

// InitCutscene09
void Cutscenes::init_plant_seeds()
{
	load_pals1(BANK_Pals_b8200, ADDR_Pals_b8200);
	load_pals2(BANK_Pals_b8a80, ADDR_Pals_b8a80);
	routines_.call(ADDR_LoadCutscenes2Gfx);
	routines_.call(ADDR_LoadCutsceneBGMaps03);
}

// Cutscene09Func: the seeds' pot rises
void Cutscenes::plant_seeds()
{
	switch (wram_.unk_2d013()) {
	case 0:
		wait(30);
		break;
	case 1: // .Func_9d6b6
		place(wSceneObj2, 176, 74, 0x01);
		mem_[wSceneObj1YCoord] = 1;
		advance();
		break;
	case 2: // .Func_9d6ce
		rise(wSceneObj2, 0x88, 0x62);
		break;
	case 3:
		end(60);
		break;
	default:
		unrealistic();
		return;
	}
	draw_func_9fbfa();
}

// Func_9fbfa
void Cutscenes::draw_func_9fbfa()
{
	draw_shadow(wSceneObj2, wCurSceneObj, 0x01, 0x00); // Func_9fb4c
	if (mem_[wSceneObj2State]) // .Func_9fc09
		draw(animate(wSceneObj2State, ADDR_Frameset_bd1b4), ADDR_OAM_bcfb3);
	draw_shadow(wSceneObj2, wCurSceneObjDuration, 0x01, 0x01); // Func_9fb6c
}

} // namespace wl3::cutscenes

PORT_COVERED(InitCutscene08);
PORT_COVERED(InitCutscene10);
PORT_COVERED(InitCutscene17);
PORT_COVERED(InitCutscene1d);
PORT_COVERED(InitCutscene25);
PORT_COVERED(Func_9c25f);
PORT_COVERED(Cutscene08Func);
PORT_COVERED(Cutscene10Func);
PORT_COVERED(Cutscene17Func);
PORT_COVERED(Cutscene1dFunc);
PORT_COVERED(Cutscene25Func);
PORT_COVERED(Func_9d6f0);
PORT_COVERED(Func_9fc1d);
PORT_COVERED(InitCutscene09);
PORT_COVERED(Cutscene09Func);
PORT_COVERED(Func_9fbfa);
