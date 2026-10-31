// The temple, inside (bank2b.asm, _InitTempleScene and _UpdateTempleScene).
// See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

void TempleScene::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void TempleScene::wario_state(uint8_t state)
{
	gb_->cpu.a = state;
	routines_.call(ADDR_SetSceneWarioState);
}

void TempleScene::set_state(uint16_t state, uint8_t value)
{
	gb_->cpu.a = value;
	SET_HL(gb_, state);
	routines_.call(ADDR_SetSceneObjState);
}

// The decompress_* macros
void TempleScene::decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1)
{
	if (vram1)
		mem_[rVBK] = 1;
	wram_.temp_bank() = bank;
	SET_HL(gb_, src);
	SET_BC(gb_, dest);
	routines_.farcall(bank, ADDR_Decompress);
	if (vram1)
		mem_[rVBK] = 0;
}

void TempleScene::load_far(uint16_t routine, uint8_t bank, uint16_t src)
{
	SET_HL(gb_, src);
	gb_->cpu.b = bank;
	routines_.call(routine);
}

// _InitTempleScene: the temple, and the hidden figure's words: the next
// objective's, or with the gold music box (once the game's cleared, in
// time attack) his reveal
void TempleScene::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	stop_music2(mem_);
	mem_[rWBK] = 0x02; // BANK("WRAM2")
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	mem_[rSCY] = 0;
	wram_.scy() = 0;
	SceneCommon(gb_).set_vblank();
	SceneCommon(gb_).clear_scene_ram();
	SceneCommon(gb_).clear_scene_objs();
	load_far(ADDR_LoadFarPalsToTempPals1, BANK_Pals_86a4d, ADDR_Pals_86a4d);
	decompress(BANK_FontGfx, ADDR_FontGfx, v1Tiles0, true);
	load_far(ADDR_LoadFarTiles, BANK_TempleGfx, ADDR_TempleGfx);
	load_far(ADDR_LoadFarBGMap, BANK_TempleTilemap, ADDR_TempleTilemap);
	mem_[rVBK] = 1;
	load_far(ADDR_LoadFarBGMap, BANK_TempleAttrmap, ADDR_TempleAttrmap);
	mem_[rVBK] = 0;
	SceneCommon(gb_).fill_cleared_text_buffer();

	gb_->cpu.a = GOLD_MUSIC_BOX;
	routines_.call(ADDR_IsTreasureCollected);
	bool reveal = false;
	if (!(gb_->cpu.f & FLAG_Z)) {
		uint8_t flags = wram_.game_mode_flags();
		reveal = !(flags & (1 << MODE_GAME_CLEARED_F)) || (flags & (1 << MODE_TIME_ATTACK_F));
	}
	if (reveal) {
		wram_.temple_scene() = TEMPLE_SCENE_HIDDEN_FIGURE_REVEAL;
		decompress(BANK_TempleMusicBoxSceneGfx, ADDR_TempleMusicBoxSceneGfx, v1Tiles0, true);
		decompress(BANK_HiddenFigureCutsceneTilemap, ADDR_HiddenFigureCutsceneTilemap, v0BGMap1, false);
		decompress(BANK_HiddenFigureCutsceneAttrmap, ADDR_HiddenFigureCutsceneAttrmap, v1BGMap1, true);
		decompress(BANK_TextJP_HiddenFigureReplenishPower,
			wram_.language() ? ADDR_TextEN_HiddenFigureReplenishPower : ADDR_TextJP_HiddenFigureReplenishPower,
			wTextBuffer, false);
	} else {
		wram_.temple_scene() = TEMPLE_SCENE_ENTERING;
		decompress(BANK_TextJP_HiddenFigureHeadFor,
			wram_.language() ? ADDR_TextEN_HiddenFigureHeadFor : ADDR_TextJP_HiddenFigureHeadFor,
			wTextBuffer, false);
		SceneCommon(gb_).load_next_objective();
	}

	// Wario, off the screen to the left
	mem_[wSceneWarioYCoord] = 0x60;
	mem_[wSceneWarioXCoord] = 0xe8;
	mem_[wSceneWarioStateGroup] = SCENEWARIO_GROUP_NONE;
	wario_state(SCENEWARIO_NONE);
	routines_.call(ADDR_UpdateSceneWario);
	wram_.unk_2d013() = 0;
	wram_.unk_2d014() = 0;
	uint8_t lcdc = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
	mem_[rLCDC] = static_cast<uint8_t>(lcdc ^ mem_[wLCDCFlagsToFlip]);
	mem_[wLCDCFlagsToFlip] = 0;
	++wram_.sub_state();
	SET_HL(gb_, wSubState); // as the asm leaves it (InitTempleScene, in C, passes it on)
}

// _UpdateTempleScene: a frame in the temple
void TempleScene::update()
{
	step(); // .UpdateScene
	routines_.call(ADDR_Func_ac2b2);
	routines_.call(ADDR_UpdateSceneWario);
	routines_.call(ADDR_UpdateTempleSceneObjs);
	routines_.clear_unused_virtual_oam();
}

// .NextState
void TempleScene::next_state()
{
	mem_[wTempleSceneCounter] = 0;
	++mem_[w2da04];
}

// _UpdateTempleScene.UpdateScene: Wario walks in, listens to the hidden
// figure (B or left leaves), and walks out
void TempleScene::step()
{
	uint8_t counter = ++mem_[wTempleSceneCounter];
	uint8_t state = mem_[w2da04];
	switch (wram_.temple_scene()) {
	case TEMPLE_SCENE_ENTERING: // .Entering
		switch (state) {
		case 0: // .start
			wario_state(SCENEWARIO_WALK_RIGHT);
			next_state();
			return;
		case 1: // .walk_right
			if (SceneCommon(gb_).walk(true) != 0x50)
				return;
			mem_[wSceneWarioXCoord] = 0x50;
			wario_state(SCENEWARIO_IDLE_RIGHT);
			next_state();
			return;
		case 2: // .turn
			if (counter < 8)
				return;
			wario_state(SCENEWARIO_TURN_BACK_RIGHT);
			next_state();
			return;
		case 3: // .look_back
			if (mem_[wSceneWarioState] == SCENEWARIO_LOOK_BACK)
				next_state();
			return;
		case 4: // .nod: then the words
			if (counter < 50)
				return;
			wario_state(SCENEWARIO_NOD);
			mem_[wTempleSceneCounter] = 0;
			mem_[w2da04] = 0;
			mem_[wCurTextLine] = 0;
			mem_[wTextDelayCounter] = 0;
			wram_.temple_scene() = TEMPLE_SCENE_TALKING;
			return;
		default:
			return;
		}
	case TEMPLE_SCENE_TALKING: // .TalkToHiddenFigure
		temple_music();
		if (!mem_[wIsNextObjectiveTheTemple])
			SceneCommon(gb_).print_with_header();
		if (!(wram_.joypad_pressed() & (PAD_B | PAD_LEFT)))
			return;
		mem_[wTempleSceneCounter] = 0; // .start_exit
		mem_[w2da04] = 0;
		SceneCommon(gb_).clear_textbox();
		wram_.temple_scene() = TEMPLE_SCENE_EXITING;
		return;
	case TEMPLE_SCENE_EXITING: // .Exiting
		switch (state) {
		case 0: // .turn_left
			wario_state(SCENEWARIO_TURN_BACK_TO_LEFT);
			next_state();
			return;
		case 1: // .idle_left
			if (mem_[wSceneWarioState] == SCENEWARIO_IDLE_LEFT)
				next_state();
			return;
		case 2: // .start_walk
			if (counter < 4)
				return;
			wario_state(SCENEWARIO_WALK_LEFT);
			next_state();
			return;
		case 3: // .walk_left (B: straight out)
			if (wram_.joypad_pressed() & (1 << B_PAD_B)) {
				leave();
				return;
			}
			if (SceneCommon(gb_).walk(false) != 0xf0)
				return;
			wario_state(SCENEWARIO_NONE);
			next_state();
			return;
		case 4: // .outside
			if (counter >= 0x0a)
				leave();
			return;
		default:
			return;
		}
	case TEMPLE_SCENE_HIDDEN_FIGURE_REVEAL:
		reveal(); // HiddenFigureRevealScene
		return;
	default:
		unrealistic();
		return;
	}
}

// _UpdateTempleScene.exit: back to the map
void TempleScene::leave()
{
	mem_[hSFXID + 0] = 0xff; // stop_sfx
	mem_[hSFXID + 1] = 0x00;
	mem_[w2d14c] = 0x01;
	wram_.sub_state() = SST_OVERWORLD_08;
}

// _UpdateTempleScene.DelayAndPlayTempleMusic: the temple's music, after
// 90 frames
void TempleScene::temple_music()
{
	if (mem_[wTempleMusicIsPlaying])
		return;
	Byte delay = mem_[wTempleMusicDelayCounter];
	++delay;
	if (delay < 90)
		return;
	delay = 0;
	play_music2(mem_, MUSIC_THE_TEMPLE);
	mem_[wTempleMusicIsPlaying] = TRUE;
}

// ---------------------------------------------------------------------------
// The hidden figure's reveal

// Func_ad016
void TempleScene::next_step()
{
	wram_.unk_2d014() = 0;
	++wram_.unk_2d013();
}

// Func_ad011: after frames
void TempleScene::wait(uint8_t frames)
{
	if (wram_.unk_2d014() >= frames)
		next_step();
}

// SetFightAgainstAHiddenFigure: to the level
void TempleScene::fight()
{
	stop_music2(mem_);
	mem_[hSFXID + 0] = 0xff; // stop_sfx
	mem_[hSFXID + 1] = 0x00;
	wram_.sub_state() = 0;
	++wram_.state();
}

// The HDMA of the hidden figure's tiles (.SetHDMA)
void TempleScene::hdma(uint8_t vram_bank, uint16_t src, uint8_t dest_hi)
{
	mem_[wHDMADestVRAMBank] = vram_bank;
	mem_[wHDMABank] = BANK_HiddenFigure1Gfx;
	mem_[wHDMA + 0] = static_cast<uint8_t>(src >> 8);
	mem_[wHDMA + 1] = static_cast<uint8_t>(src);
	mem_[wHDMA + 2] = dest_hi;
	mem_[wHDMA + 3] = 0;
	mem_[wHDMA + 4] = 0x80 - 1;
	panic_then_next();
}

// .Func_ad16a
void TempleScene::panic_then_next()
{
	wario_panic(); // HandleTempleWarioPanic
	next_step();
}

// HiddenFigureRevealScene: with all the music boxes, the temple shakes,
// the hidden figure shows himself, laughs and talks, and the fight (B
// skips to it once fought before)
void TempleScene::reveal()
{
	if (wram_.unk_2d013() >= 0x02 && (wram_.game_mode_flags() & (1 << MODE_FOUGHT_A_HIDDEN_FIGURE_F))
		&& (wram_.joypad_pressed() & (1 << B_PAD_B))) {
		fight();
		return;
	}
	left_hand(); // DoHiddenFigureLeftHandMovement
	right_hand(); // DoHiddenFigureRightHandMovement
	uint8_t frames = ++wram_.unk_2d014();
	auto panic = [this] { wario_panic(); }; // HandleTempleWarioPanic
	auto shake = [this] { SceneCommon(gb_).temple_shake(); };
	auto fade = [this](uint8_t mode) { // the next PrologueFade
		mem_[w2d880] = mode;
		wram_.pal_config1_total_steps() = 0;
		mem_[wPalConfig2TotalSteps] = 0;
	};
	switch (wram_.unk_2d013()) {
	case 0: next_step(); return;
	case 1: enter_with_music_boxes(); return; // HandleEnterTempleWithAllMusicBoxesScene
	case 2: wait(40); return; // .Wait1
	case 3: SceneCommon(gb_).init_text(); return;
	case 4: case 26: SceneCommon(gb_).print_page(); return;
	case 5: wait(30); return; // .Wait2
	case 6: case 28: case 33: SceneCommon(gb_).load_page(); return;
	case 7: SceneCommon(gb_).wait_for_a(); return;
	case 8: SceneCommon(gb_).print_page_without_header(); return;
	case 9: SceneCommon(gb_).next_page(); return; // .Func_ad0bd
	case 10: case 39: // .WaitThenShowTextArrow, .Func_ad308
		if (frames < 30)
			return;
		set_state(wSceneObj1State, 0x01);
		next_step();
		return;
	case 11: // .WaitAButtonThenClearTextbox
		if (!(wram_.joypad_pressed() & (1 << B_PAD_A)))
			return;
		set_state(wSceneObj1State, 0x00);
		SceneCommon(gb_).clear_textbox();
		next_step();
		return;
	case 12: show_music_boxes(); return; // Func_ad445
	case 13: shake_scene(); return; // HandleTempleShakeScene
	case 14: // .Func_ad0e4
		shake();
		fade(0x03);
		panic_then_next();
		return;
	case 15: // .Func_ad0f6: a fade step every 16 frames
		panic();
		shake();
		if (!(wram_.unk_2d014() & 0x0f))
			SceneCommon(gb_).prologue_fade();
		return;
	case 16: // .InitShakeAndRocks
		mem_[w2d896] = 0x01;
		shake();
		for (uint16_t rock = 0; rock < NUM_TEMPLE_ROCKS; rock++)
			mem_[static_cast<uint16_t>(wTempleRock1Size + rock * TEMPLE_ROCK_STRUCT_LENGTH)] = 0;
		panic_then_next();
		return;
	case 17: { // .Func_ad12f: once the rocks are all down, the other map
		panic();
		shake();
		uint8_t sum = 0;
		for (uint16_t rock = 0; rock < NUM_TEMPLE_ROCKS; rock++)
			sum = static_cast<uint8_t>(sum + mem_[static_cast<uint16_t>(wTempleRocks + rock * TEMPLE_ROCK_STRUCT_LENGTH)]);
		if (sum)
			return;
		mem_[wLCDCFlagsToFlip] = LCDC_BG_MAP;
		next_step();
		return;
	}
	case 18: hdma(0, ADDR_HiddenFigure1Gfx + 0x80 * TILE_SIZE, (v0Tiles1 >> 8) - 0x80); return;
	case 19: hdma(0, ADDR_HiddenFigure1Gfx + 0x100 * TILE_SIZE, (v0Tiles2 >> 8) - 0x80); return;
	case 20: hdma(1, ADDR_HiddenFigure2Gfx, (v1Tiles0 >> 8) - 0x80); return;
	case 21: // .Func_ad195: the hidden figure and his hands
		panic();
		if (frames < 30)
			return;
		set_state(wSceneObj7State, 0x0d);
		set_state(wSceneObj3State, 0x11);
		set_state(wSceneObj4State, 0x12);
		mem_[wSceneObj7] = 0x38;
		mem_[wSceneObj7 + 1] = 0x50;
		mem_[wSceneObj3] = 0x58;
		mem_[wSceneObj3 + 1] = 0x20;
		mem_[wSceneObj4] = 0x58;
		mem_[wSceneObj4 + 1] = 0x80;
		mem_[w2d8da] = 0x01;
		mem_[w2d8dc] = 0x01;
		fade(0x04);
		next_step();
		return;
	case 22: { // .Func_ad1e3: a fade step every 8 frames, then every 4
		panic();
		uint8_t mask = wram_.pal_config1_total_steps() >= 0x14 ? 0x03 : 0x07;
		if (!(wram_.unk_2d014() & mask))
			SceneCommon(gb_).prologue_fade();
		return;
	}
	case 23: figure_appears(); return; // HandleHiddenFigureReveal
	case 24: wait(40); return; // .Wait3
	case 25: // .InitTextPrintOnBGMap1
		SceneCommon(gb_).init_text();
		mem_[wBGMapToPrintText] = 0x01;
		return;
	case 27: // .Func_ad207
		if (frames < 30)
			return;
		mem_[wCurTextBufferPage] = 4;
		next_step();
		return;
	case 29: // .StartHiddenFigureMouthMovement
		mem_[wSceneObj7State] = 0x10;
		next_step();
		return;
	case 30: // .PlayHiddenFigureLaughSFX: once the mouth is opening
		if (mem_[wSceneObj7State] != 0x0e)
			return;
		play_sfx(mem_, SFX_HIDDEN_FIGURE_LAUGH);
		next_step();
		return;
	case 31: // .PrintTextUntilNextPage
		SceneCommon(gb_).print_without_header();
		if (mem_[wCurTextLine] == 0x80)
			next_step();
		return;
	case 32: // .Func_ad23a
		if (frames == 40) { // .finish_mouth_movement
			mem_[wSceneObj7State] = 0x0f;
			return;
		}
		if (frames < 70)
			return;
		++mem_[wCurTextBufferPage];
		set_state(wSceneObj1State, 0x01);
		next_step();
		return;
	case 34: // .Func_ad259
		if (!(wram_.joypad_pressed() & (1 << B_PAD_A)))
			return;
		set_state(wSceneObj1State, 0x00);
		SceneCommon(gb_).clear_textbox_except_header();
		mem_[wSceneObj7State] = 0x10;
		next_step(); // Func_aca6d
		return;
	case 35: // .Func_ad271
		if (mem_[wSceneObj7State] == 0x0e)
			next_step(); // Func_aca6d
		return;
	case 36: // .Func_ad27a
		SceneCommon(gb_).print_without_header();
		close_mouth();
		if (mem_[wCurTextLine] == 0x80)
			next_step(); // Func_af135
		return;
	case 37: // .Func_ad2f4: the next page once the mouth is closed (back
		// one more step than Func_acaf3 goes)
		if (mem_[wSceneObj7State] != 0x0d)
			return;
		if (!SceneCommon(gb_).next_page())
			--mem_[w2d013];
		return;
	case 38: // .Func_ad300
		wario_state(SCENEWARIO_PANIC_JUMP);
		next_step();
		return;
	case 40: // .Func_ad319
		if (!(wram_.joypad_pressed() & (1 << B_PAD_A)))
			return;
		mem_[wSceneObj1State] = 0;
		SceneCommon(gb_).clear_textbox();
		next_step();
		return;
	case 41: fight(); return;
	default: unrealistic(); return;
	}
}

// .HandleHiddenFigureMouthClosing: the mouth closes at a set character of
// line 2 (by page and language)
void TempleScene::close_mouth()
{
	if (!mem_[wPendingCharDest] || mem_[wCurTextBufferLine] == 1)
		return;
	static constexpr uint8_t JAPANESE_POS[] = {1, 8, 8, 12, 1}; // pages 5 to 9
	static constexpr uint8_t ENGLISH_POS[] = {10, 12, 5, 11, 12};
	uint8_t page = mem_[wCurTextBufferPage];
	if (page < 5 || page > 9)
		return;
	uint8_t pos = (wram_.language() ? ENGLISH_POS : JAPANESE_POS)[page - 5];
	if (mem_[wCurTextLinePos] != pos || mem_[wCurTextBufferLine] != 2) // .check_pos
		return;
	mem_[wSceneObj7State] = 0x0f;
}

// ---------------------------------------------------------------------------
// The reveal's sub-scenes, their steps in w2d890 (frames in w2d891)

// Func_ad335
void TempleScene::next_sub()
{
	mem_[w2d891] = 0;
	++mem_[w2d890];
}

// Func_ad330: after frames
void TempleScene::wait_sub(uint8_t frames)
{
	if (mem_[w2d891] >= frames)
		next_sub();
}

// Func_ad329: once Wario is in the state
void TempleScene::wait_wario(uint8_t state)
{
	if (mem_[wSceneWarioState] == state)
		next_sub();
}

// Func_ad33c: the sub-scene done, the next step
void TempleScene::end_sub()
{
	mem_[w2d890] = 0;
	mem_[w2d891] = 0;
	next_step();
}

// HandleEnterTempleWithAllMusicBoxesScene: Wario walks in; in time attack
// he bows, and A goes on (B or left leaves)
void TempleScene::enter_with_music_boxes()
{
	uint8_t frames = ++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: wait_sub(8); return; // .Wait1
	case 1: // .WalkRight
		wario_state(SCENEWARIO_WALK_RIGHT);
		next_sub();
		return;
	case 2: // .MoveRight
		if (SceneCommon(gb_).walk(true) != 0x50)
			return;
		mem_[wSceneWarioXCoord] = 0x50;
		wario_state(SCENEWARIO_IDLE_RIGHT);
		next_sub();
		return;
	case 3: wait_sub(30); return; // .Wait2
	case 4: // .TurnToBack
		wario_state(SCENEWARIO_TURN_BACK_RIGHT);
		next_sub();
		return;
	case 5: wait_wario(SCENEWARIO_LOOK_BACK); return; // .LookBack
	case 6: // .CheckTimeAttack
		if (wram_.game_mode_flags() & (1 << MODE_TIME_ATTACK_F))
			next_sub();
		else
			end_sub();
		return;
	case 7: // .Bow, with the text arrow
		if (frames < 20)
			return;
		wario_state(SCENEWARIO_BOW);
		mem_[wSceneObj1] = 0x8e;
		mem_[wSceneObj1 + 1] = 0x98;
		set_state(wSceneObj1State, 0x01);
		next_sub();
		return;
	case 8: { // .WaitInput
		uint8_t pressed = wram_.joypad_pressed();
		if (pressed & (1 << B_PAD_A)) { // .a_btn
			wario_state(SCENEWARIO_LOOK_BACK);
			mem_[wSceneObj1State] = 0;
			end_sub();
		} else if (pressed & (PAD_B | PAD_LEFT)) {
			mem_[wSceneObj1State] = 0;
			wram_.temple_scene() = TEMPLE_SCENE_EXITING;
			mem_[wTempleSceneCounter] = 0;
		}
		return;
	}
	default: unrealistic(); return;
	}
}

// Func_ad445: Wario holds up the music boxes, they play (HandleTempleMusicBoxes
// by w2d8a0) and go
void TempleScene::show_music_boxes()
{
	music_boxes(); // HandleTempleMusicBoxes
	uint8_t frames = ++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: wait_sub(60); return; // .Wait1
	case 1: // .ShowMusicBoxes
		wario_state(SCENEWARIO_SHOW_MUSIC_BOXES_1);
		next_sub();
		return;
	case 2: wait_wario(SCENEWARIO_SHOW_MUSIC_BOXES_3); return; // .WaitUntilRaiseHands
	case 3: // .SpawnMusicBoxes
		if (frames < 16)
			return;
		mem_[w2d8a0] = 0x01;
		next_sub();
		return;
	case 4: case 12: // .WaitMusicBoxSequenceFinish
		if (!mem_[w2d8a0])
			next_sub();
		return;
	case 5: // .LowerHands
		if (frames < 60)
			return;
		wario_state(SCENEWARIO_SHOW_MUSIC_BOXES_4);
		next_sub();
		return;
	case 6: wait_wario(SCENEWARIO_LOOK_BACK); return; // .WaitUntilLookBack
	case 7: case 8: case 9: case 10: wait_sub(240); return; // .Wait2
	case 11: // .DespawnMusicBoxes
		mem_[w2d8a0] = 0x08;
		next_sub();
		return;
	case 13: end_sub(); return;
	default: unrealistic(); return;
	}
}

// HandleHiddenFigureReveal: the music, Wario stops his panic run, panics
// and looks back
void TempleScene::figure_appears()
{
	wario_panic(); // HandleTempleWarioPanic
	uint8_t frames = ++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: // .PlayHiddenFigureRevealMusic
		play_music2(mem_, MUSIC_HIDDEN_FIGURE_REVEAL);
		next_sub();
		return;
	case 1: // .StopPanicRun
		mem_[w2d892] = 0x03;
		next_sub();
		return;
	case 2: // .WaitForPanicRunStop
		if (!mem_[w2d894])
			next_sub();
		return;
	case 3: // .SetWarioPanic
		if (frames < 20)
			return;
		wario_state(SCENEWARIO_PANIC);
		next_sub();
		return;
	case 4: // .SetWarioPanicJump
		if (frames < 40)
			return;
		mem_[wSceneWarioState] = SCENEWARIO_PANIC_JUMP;
		next_sub();
		return;
	case 5: wait_wario(SCENEWARIO_LOOK_BACK); return; // .WaitUntilLookBack
	case 6: end_sub(); return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The music boxes, over Wario's hands

namespace {

struct MusicBox {
	uint16_t action, counter; // wTempleMusicBoxN, its counter
	uint16_t obj; // the scene object
	uint8_t state;
};

constexpr MusicBox MUSIC_BOXES[] = { // yellow, blue, green, red, gold
	{wTempleMusicBox1, wTempleMusicBox1Counter, wSceneObj8, 0x03},
	{wTempleMusicBox2, wTempleMusicBox2Counter, wSceneObj9, 0x04},
	{wTempleMusicBox3, wTempleMusicBox3Counter, wSceneObj10, 0x05},
	{wTempleMusicBox4, wTempleMusicBox4Counter, wSceneObj11, 0x06},
	{wTempleMusicBox5, wTempleMusicBox5Counter, wSceneObj12, 0x07},
};

} // namespace

// HandleTempleMusicBox1Hover to HandleTempleMusicBox5Hover: a music box
// comes out of Wario's hands (InitTempleSceneMusicBox), rises up-left
// (MoveMusicBoxUpLeft; the last one starts the music) and circles
void TempleScene::music_box_hover(int index)
{
	const MusicBox &box = MUSIC_BOXES[index];
	auto next = [&] { // Func_ad557
		mem_[box.counter] = 0;
		++mem_[box.action];
	};
	switch (mem_[box.action]) {
	case 0: return;
	case 1: // .Init
		mem_[box.obj] = 0x50;
		mem_[box.obj + 1] = 0x4e;
		set_state(static_cast<uint16_t>(box.obj + 6), box.state);
		next();
		return;
	case 2: { // .MoveUpLeft, to 0x2c
		--mem_[box.obj + 1];
		Byte y = mem_[box.obj];
		y -= 2;
		if (y != 0x2c)
			return;
		if (index == 4)
			play_music2(mem_, MUSIC_MUSIC_BOXES);
		next();
		return;
	}
	case 3: // .MoveCircle
		SET_HL(gb_, box.obj);
		SET_DE(gb_, ADDR_Data_ad654);
		routines_.call(ADDR_ApplyOWMovement);
		return;
	default: // InvalidMusicBoxAction
		unrealistic();
		return;
	}
}

// HandleTempleMusicBoxes: the boxes, out one by one every 16 frames (by
// w2d8a0 from 1), and away once each has circled (from 8)
void TempleScene::music_boxes()
{
	for (int i = 0; i < 5; i++)
		music_box_hover(i);
	Byte step = mem_[w2d8a0];
	Byte frames = mem_[w2d8a1];
	++frames;
	auto next = [&] { // .Next
		frames = 0;
		++step;
	};
	uint8_t s = step;
	if (!s)
		return;
	if (s >= 1 && s <= 5) { // .InitYellowMusicBox and the like
		if (frames < 16)
			return;
		mem_[MUSIC_BOXES[s - 1].action] = 0x01;
		next();
		return;
	}
	if (s >= 8 && s <= 12) { // .DeinitYellowMusicBox and the like: once it
		// has circled 16 times
		const MusicBox &box = MUSIC_BOXES[s - 8];
		if (mem_[box.obj + 7] != 16)
			return;
		mem_[box.obj + 6] = 0;
		mem_[box.action] = 0;
		next();
		return;
	}
	if (s == 7) {
		next();
		return;
	}
	if (s == 6 || s == 13) { // .Reset
		step = 0;
		frames = 0;
		return;
	}
	unrealistic();
}

// HandleTempleShakeScene: the temple shakes twice: small rocks fall, Wario
// is startled and looks around; then big ones, and he panics
void TempleScene::shake_scene()
{
	static constexpr uint8_t SMALL_WAITS[] = {16, 8, 20, 8, 16, 8};
	static constexpr uint8_t LARGE_WAITS[] = {20, 4, 10, 4, 10, 10, 30, 8, 10, 8};

	wario_panic(); // HandleTempleWarioPanic
	SceneCommon(gb_).temple_shake();
	uint8_t frames = ++mem_[w2d891];
	uint8_t s = mem_[w2d890];
	auto rock = [](int i) { return static_cast<uint16_t>(wTempleRocks + i * TEMPLE_ROCK_STRUCT_LENGTH); };
	auto drop = [&](int i, uint8_t size) { // .InitTempleRock
		mem_[rock(i)] = 0x01; // action
		mem_[rock(i) + 1] = 0; // counter
		mem_[rock(i) + 2] = size;
		next_sub();
	};
	if (s >= 1 && s <= 6) { // .InitSmallRock1 to .InitSmallRock6
		if (frames >= SMALL_WAITS[s - 1])
			drop(s - 1, 0x01);
		return;
	}
	if (s >= 15 && s <= 24) { // .InitLargeRock1 to .InitLargeRock10
		int i = s - 15;
		if (frames < LARGE_WAITS[i])
			return;
		if (i >= 6) // the music boxes' objects, for the last four
			mem_[static_cast<uint16_t>(wSceneObj8Unk7 + (i - 6) * 8)] = 0;
		drop(i, 0x02);
		return;
	}
	switch (s) {
	case 0: // .StartShake
		if (frames < 90)
			return;
		mem_[w2d896] = 0x02;
		next_sub();
		return;
	case 7: // .ShowExclamationMark
		if (frames < 60)
			return;
		mem_[wCurSceneObj] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] - 0x1c);
		mem_[wCurSceneObj + 1] = mem_[wSceneWarioXCoord];
		set_state(wCurSceneObjState, 0x02);
		play_sfx(mem_, SFX_106);
		next_sub();
		return;
	case 8: // .WaitExclamationMark
		if (!mem_[wCurSceneObjState])
			next_sub();
		return;
	case 9: wait_sub(45); return; // .Wait
	case 10: // .LookAround
		wario_state(SCENEWARIO_LOOK_AROUND);
		next_sub();
		return;
	case 11: wait_wario(SCENEWARIO_LOOK_FORWARD); return; // .WaitUntilLookForward
	case 12: // .StopShakeAndDeinitRocks
		mem_[w2d896] = 0x01;
		for (int i = 0; i < 6; i++)
			mem_[rock(i) + 2] = 0;
		next_sub();
		return;
	case 13: { // .WaitUntilRocksDeinit
		uint8_t sum = 0;
		for (int i = 0; i < NUM_FALLING_TEMPLE_ROCKS; i++)
			sum = static_cast<uint8_t>(sum + mem_[rock(i)]);
		if (!sum)
			next_sub();
		return;
	}
	case 14: // .StartBigShake
		mem_[w2d896] = 0x0b;
		next_sub();
		return;
	case 25: // .PanicJump
		mem_[wSceneWarioIgnoreScroll] = TRUE;
		wario_state(SCENEWARIO_PANIC_JUMP);
		next_sub();
		return;
	case 26: wait_wario(SCENEWARIO_LOOK_BACK); return; // .LookBack
	case 27: // .StopShake
		if (frames < 8)
			return;
		mem_[w2d894] = 0x01;
		next_sub();
		return;
	case 28: end_sub(); return;
	default: unrealistic(); return;
	}
}

// HandleTempleWarioPanic: while w2d894 is 1, Wario runs right and left in
// a panic (back and forth until told to stop, w2d892 3), then to the
// centre, turns and looks back
void TempleScene::wario_panic()
{
	if (mem_[w2d894] != 0x01)
		return;
	uint8_t frames = ++mem_[w2d893];
	Byte step = mem_[w2d892];
	auto next = [&] { // .Next
		++step;
		mem_[w2d893] = 0;
	};
	auto run_to = [&](bool right, uint8_t x, uint8_t state) { // whether there
		if (SceneCommon(gb_).walk(right) != x)
			return false;
		mem_[wSceneWarioXCoord] = x;
		wario_state(state);
		return true;
	};
	switch (step) {
	case 0: // .PanicRunRight
		wario_state(SCENEWARIO_PANIC_RUN_RIGHT);
		next();
		return;
	case 1: case 4: // .MoveRight
		if (run_to(true, 0x80, SCENEWARIO_PANIC_RUN_LEFT))
			next();
		return;
	case 2: // .MoveLeft, then back
		if (run_to(false, 0x20, SCENEWARIO_PANIC_RUN_RIGHT)) {
			--step;
			mem_[w2d893] = 0;
		}
		return;
	case 3: // .PanicRunUntilMovingRight
		if (mem_[wSceneWarioState] == SCENEWARIO_PANIC_RUN_RIGHT) { // .moving_right
			next();
			if (run_to(true, 0x80, SCENEWARIO_PANIC_RUN_LEFT))
				next();
			return;
		}
		if (run_to(false, 0x20, SCENEWARIO_PANIC_RUN_RIGHT))
			next();
		return;
	case 5: // .MoveLeftToCentre
		if (run_to(false, 0x50, SCENEWARIO_IDLE_LEFT))
			next();
		return;
	case 6: // .TurnBack
		if (frames < 20)
			return;
		wario_state(SCENEWARIO_TURN_BACK_LEFT);
		next();
		return;
	case 7: // .WaitUntilLookBackThenFinish
		if (mem_[wSceneWarioState] != SCENEWARIO_LOOK_BACK)
			return;
		mem_[w2d894] = 0;
		mem_[w2d893] = 0;
		step = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// DoHiddenFigureLeftHandMovement: the hidden figure's left hand (scene
// object 3) waving
void TempleScene::left_hand()
{
	if (!mem_[w2d8da])
		return;
	SET_HL(gb_, wSceneObj3);
	SET_DE(gb_, ADDR_Data_ad939);
	routines_.call(ADDR_ApplyOWMovement);
}

// DoHiddenFigureRightHandMovement: and the right one (4), mirrored
void TempleScene::right_hand()
{
	if (!mem_[w2d8dc])
		return;
	SET_HL(gb_, wSceneObj4);
	SET_DE(gb_, ADDR_Data_ad939);
	routines_.call(ADDR_ApplyOWMovement_Mirrored);
}

} // namespace wl3::overworld

// Farcalled by the map's sub-state table.
PORT_EX(_InitTempleScene, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).init(); }
PORT_EX(_UpdateTempleScene, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).update(); }
PORT_EX(HandleEnterTempleWithAllMusicBoxesScene, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).enter_with_music_boxes(); }
PORT_EX(Func_ad445, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).show_music_boxes(); }
PORT_EX(HandleHiddenFigureReveal, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).figure_appears(); }
PORT_EX(HandleTempleMusicBoxes, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).music_boxes(); }
PORT_EX(HandleTempleShakeScene, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).shake_scene(); }
PORT_EX(HandleTempleWarioPanic, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).wario_panic(); }
PORT_EX(DoHiddenFigureLeftHandMovement, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).left_hand(); }
PORT_EX(DoHiddenFigureRightHandMovement, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).right_hand(); }
PORT_EX(HiddenFigureRevealScene, 0, ALL_REGISTERS) { wl3::overworld::TempleScene(gb).reveal(); }

// Only the reveal runs this.
PORT_COVERED(SetFightAgainstAHiddenFigure);
PORT_COVERED(HandleTempleMusicBox1Hover);
PORT_COVERED(HandleTempleMusicBox2Hover);
PORT_COVERED(HandleTempleMusicBox3Hover);
PORT_COVERED(HandleTempleMusicBox4Hover);
PORT_COVERED(HandleTempleMusicBox5Hover);
PORT_COVERED(InitTempleSceneMusicBox);
PORT_COVERED(MoveMusicBoxUpLeft);
PORT_COVERED(Func_ad557);
PORT_COVERED(InvalidMusicBoxAction);
