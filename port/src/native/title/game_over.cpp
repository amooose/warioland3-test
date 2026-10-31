// The game over screen; see title.hpp.
#include "title.hpp"

namespace wl3::title {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;

static_assert(wGameOverWarioVar == wGameOverWario + 8);
static_assert(wGameOverWarioAnimationEnded == wGameOverWario + 11);

} // namespace

void GameOver::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: update(); break;
	case 4: wake_up(); break;
	case 5: routines_.call(ADDR_FastFadeToWhite); break;
	case 6: routines_.call(ADDR_Init); break; // GameOverReset
	case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void GameOver::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	load_pals();
	load_gfx();
	load_tilemap();
	routines_.call(ADDR_VBlank_GameOver);

	ram(rSCX) = 0;
	wram_.scx() = 0;
	ram(rSCY) = 0x60;
	wram_.scy() = 0x60;

	MenuObject w = wario();
	w.init(0x80, 0x50, ADDR_Frameset_aaccb);
	w.var() = 0;
	wram_.sfx_timer() = 0;
	w.animate(BANK_Frameset_aaccb);
	SET_HL(gb_, wGameOverWario);
	routines_.call(ADDR_LoadGameOverSprite);

	MenuObject text = object(wMenuObj1);
	text.init(0x18, 0x18, ADDR_Frameset_dbbf5);
	text.animate();
	text.load_sprite(ADDR_OAM_dbbd2);

	ram(rLCDC) = LCDC_DEFAULT;
	wram_.timer() = 64;
	++wram_.sub_state();
}

void GameOver::update()
{
	MenuObject w = wario();
	Byte timer = wram_.timer();
	Byte scy = wram_.scy();
	switch (w.var()) {
	case 1: // the screen scrolls up to the text
		scy -= 3;
		if (scy)
			break;
		play_sfx(mem_, SFX_061);
		play_music(mem_, MUSIC_GOLF_GAME_OVER);
		timer = 6;
		++w.var();
		break;
	case 2: // and bounces
		if (wram_.global_counter() & 0x03)
			return;
		--timer;
		if (timer & 0x01) {
			scy += 2;
			break;
		}
		scy -= 2;
		if (timer)
			break;
		play_music(mem_, MUSIC_GAME_OVER);
		++w.var();
		break;
	case 3: // until A wakes Wario
		if (!wram_.joypad_pressed().bit(B_PAD_A))
			break;
		play_sfx(mem_, SFX_SELECTION);
		w.set_frameset(ADDR_Frameset_aace2);
		++wram_.sub_state();
		break;
	default:
		if (--timer)
			break;
		play_sfx(mem_, SFX_134);
		++w.var();
		break;
	}
	update_sprites(BANK_Frameset_aace2, true);
}

void GameOver::wake_up()
{
	if (update_sprites(BANK_Frameset_aaccb, false))
		++wram_.sub_state();
}

bool GameOver::update_sprites(uint8_t bank, bool snore)
{
	MenuObject w = wario();
	bool ended = w.animate(bank);
	w.animation_ended() = ended;
	if (snore && ended)
		play_sfx(mem_, SFX_SNORE);
	SET_HL(gb_, wGameOverWario);
	routines_.call(ADDR_LoadGameOverSprite);
	MenuObject text = object(wMenuObj1);
	text.animate();
	text.load_sprite(ADDR_OAM_dbbd2);
	routines_.clear_unused_virtual_oam();
	return ended;
}

void GameOver::load_pals()
{
	routines_.load_pals_to_temp_pals1(ADDR_Pals_db509);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_db549);
	routines_.copy_short(ADDR_Pals_86ad5, wTempPals2, 1 * 8, BANK_Pals_86ad5);
}

void GameOver::load_gfx()
{
	ram(rVBK) = 0;
	routines_.decompress(ADDR_GameOverGfx, v0Tiles0);
	routines_.copy_bc(ADDR_WarioSleepGfx, v0Tiles0, 0x80 * TILE_SIZE, BANK_WarioSleepGfx);
}

void GameOver::load_tilemap()
{
	ram(rVBK) = VRAM1;
	routines_.decompress(ADDR_GameOverAttrmap, v0BGMap0);
	ram(rVBK) = 0;
	routines_.decompress(ADDR_GameOverTilemap, v0BGMap0);
}

// The hidden figure behind the final battle. Its registers end as the last
// Decompress leaves them, as the original's do.
void GameOver::load_hidden_figure_gfx()
{
	routines_.load_pals_to_temp_pals1(ADDR_Pals_db000);
	routines_.copy_short(ADDR_Pals_db040, wTempPals2 + 3 * 8, 5 * 8);

	ram(rVBK) = VRAM1;
	routines_.copy_bc(ADDR_HiddenFigure2Gfx, v1Tiles0, 0x180 * TILE_SIZE);
	ram(rVBK) = 0;
	routines_.copy_bc(ADDR_HiddenFigure1Gfx, v0Tiles0, 0x180 * TILE_SIZE);

	ram(rVBK) = VRAM1;
	routines_.decompress(ADDR_HiddenFigure2Map, v1BGMap0);
	routines_.decompress(ADDR_HiddenFigure4Map, v1BGMap1);
	ram(rVBK) = 0;
	routines_.decompress(ADDR_HiddenFigure1Map, v0BGMap0);
	routines_.decompress(ADDR_HiddenFigure3Map, v0BGMap1);
}

} // namespace wl3::title

using wl3::title::GameOver;

// Called from GameOverStateTable, whose callers use no register after it
// (see src/ported/engine/game_over/common.c).
PORT_EX(_GameOverStateTable, 0, ALL_REGISTERS)
{
	GameOver(gb).run_state();
}

PORT(LoadHiddenFigureGfx)
{
	GameOver(gb).load_hidden_figure_gfx();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitGameOverScreen);
PORT_COVERED(UpdateGameOverScreen);
PORT_COVERED(Func_db3e4);
PORT_COVERED(GameOverReset);
PORT_COVERED(LoadGameOverPals);
PORT_COVERED(LoadGameOverGfx);
PORT_COVERED(Func_db4cf);
PORT_COVERED(Func_db4e9);
