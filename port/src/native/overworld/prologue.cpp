// The prologue (bank2b.asm, Prologue). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

void Prologue::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void Prologue::wario_state(uint8_t state)
{
	gb_->cpu.a = state;
	routines_.call(ADDR_SetSceneWarioState);
}

void Prologue::set_state(uint16_t state, uint8_t value)
{
	gb_->cpu.a = value;
	SET_HL(gb_, state);
	routines_.call(ADDR_SetSceneObjState);
}

void Prologue::far_copy(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count)
{
	SET_HL(gb_, src);
	SET_DE(gb_, dest);
	gb_->cpu.c = count;
	gb_->cpu.b = bank;
	routines_.call(ADDR_CopyFarBytes);
}

// Prologue_Next
void Prologue::next_step()
{
	wram_.unk_2d014() = 0;
	++wram_.unk_2d013();
}

// Prologue_Delay: after frames (counted in w2d014)
void Prologue::wait(uint8_t frames)
{
	if (wram_.unk_2d014() >= frames)
		next_step();
}

// Prologue.Func_ae009: the next fade's palettes (PrologueFade, by w2d880)
void Prologue::set_fade(uint8_t fade)
{
	mem_[w2d880] = fade;
	wram_.pal_config1_total_steps() = 0;
	mem_[wPalConfig2TotalSteps] = 0;
	next_step();
}

// Prologue.Func_ae067: the music box on its pedestal (scene object 3)
void Prologue::place_music_box()
{
	mem_[wSceneObj3YCoord] = 195;
	mem_[wSceneObj3XCoord] = 120;
	set_state(wSceneObj3State, 0x09);
}

// Prologue.Func_ae0ab: the BG palettes from pals, and the configs for
// them and the first object palette
void Prologue::load_pals(uint16_t pals)
{
	far_copy(BANK_Pals_86a0d, pals, wTempPals1, 8 * PAL_SIZE);
	SET_DE(gb_, ADDR_Data_ac9f5);
	routines_.call(ADDR_Func_ac9e9);
	SET_DE(gb_, ADDR_Data_ac9ff);
	routines_.call(ADDR_Func_ac9e4);
	next_step();
}

// Prologue.configure_hdma_transfer: a block of the temple's, by HDMA
void Prologue::hdma(uint8_t vram_bank, uint16_t src, uint8_t dest_hi, uint8_t mode)
{
	mem_[wHDMADestVRAMBank] = vram_bank;
	mem_[wHDMABank] = BANK_TempleTilemap;
	mem_[wHDMA + 0] = static_cast<uint8_t>(src >> 8);
	mem_[wHDMA + 1] = static_cast<uint8_t>(src);
	mem_[wHDMA + 2] = dest_hi;
	mem_[wHDMA + 3] = 0;
	mem_[wHDMA + 4] = mode;
	next_step();
}

// Prologue: a frame of the prologue (PrologueFunc's cutscene)
void Prologue::update()
{
	step();
	routines_.call(ADDR_UpdatePrologueSceneObjects);
	routines_.call(ADDR_UpdateSceneWario);
	routines_.call(ADDR_Func_ac3fd);
}

// Prologue.PlayCutscene: the plane, the cave, the music box, the temple
void Prologue::step()
{
	switch (wram_.unk_2d013()) {
	case 0: next_step(); return;
	case 1: wait(60); return; // .Wait60
	case 2: set_fade(0x01); return; // .SetFadeInFromBlack
	case 3: case 8: case 10: case 17: routines_.call(ADDR_PrologueFade); return;
	case 4: fall_from_plane(); return; // Prologue_WarioFallFromPlane
	case 5: { // .ScrollRight40Pixels
		Byte scx = wram_.scx();
		++scx;
		if (scx == 40)
			next_step();
		return;
	}
	case 6: walk_to_cave(); return; // Prologue_WarioWalkToCaveEntrance
	case 7: case 16: set_fade(0x00); return; // .SetFadeToBlack
	case 9: // .PrepareCaveScene1: the cave, Wario coming in from the right
		wram_.scy() = 112;
		wram_.scx() = 0;
		mem_[wSceneWarioYCoord] = 208;
		mem_[wSceneWarioXCoord] = 240;
		wario_state(SCENEWARIO_NONE);
		place_music_box();
		mem_[wSceneObj1] = 187;
		mem_[wSceneObj2YCoord] = 187;
		mem_[wSceneObj1 + 1] = 120;
		mem_[wSceneObj2XCoord] = 120;
		mem_[w2d8b0] = 0;
		set_state(wSceneObj1State, 0x00);
		play_music2(mem_, MUSIC_MUSIC_BOX_CAVE);
		set_fade(0x06);
		return;
	case 11: enter_cave(); return; // Prologue_WarioEnterCave
	case 12: // .ShowMusicBoxCloseUp: on BG map 1
		wram_.scy() = 0;
		wram_.scx() = 0;
		mem_[wLCDCFlagsToFlip] = LCDC_BG_MAP;
		mem_[wSceneObj8] = 80;
		mem_[wSceneObj8 + 1] = 80;
		set_state(wSceneObj8State, 0x0d);
		set_state(wSceneObj3State, 0x00);
		far_copy(BANK_Pals_86a8d, ADDR_Pals_86a8d, wTempOBPals, PAL_SIZE);
		load_pals(ADDR_Pals_86a0d);
		return;
	case 13: music_box_close_up(); return; // Func_ae1de
	case 14: // .PrepareCaveScene2: back in the cave, Wario holding it
		wram_.scy() = 112;
		wram_.scx() = 0;
		mem_[wLCDCFlagsToFlip] = LCDC_BG_MAP;
		wario_state(SCENEWARIO_HOLD_MUSIC_BOX);
		place_music_box();
		set_state(wSceneObj1State, 0x0e);
		set_state(wSceneObj8State, 0x00);
		routines_.copy(wTempPals2, wTempOBPals, PAL_SIZE);
		load_pals(ADDR_Pals_869cd);
		return;
	case 15: sucked_into_music_box(); return; // Prologue_WarioSuckedIntoMusicBox
	// the temple, by HDMA
	case 18: hdma(0, ADDR_TempleTilemap, (v0BGMap0 >> 8) - 0x80, 18 * 2 - 1); return;
	case 19: hdma(1, ADDR_TempleAttrmap, (v0BGMap0 >> 8) - 0x80, 18 * 2 - 1); return;
	case 20: hdma(0, ADDR_TempleGfx, (v0Tiles2 >> 8) - 0x80, 0x80 - 1); return;
	case 21: // .PrepareTempleScene: Wario asleep
		wram_.scy() = 0;
		wram_.scx() = 0;
		mem_[wLCDCFlagsToFlip] = LCDC_BG_MAP;
		mem_[wSceneWarioYCoord] = 96;
		mem_[wSceneWarioXCoord] = 64;
		wario_state(SCENEWARIO_SLEEP);
		play_sfx(mem_, SFX_SNORE);
		set_fade(0x05);
		return;
	case 22: // .Func_ae15b: a fade step every 8 frames
		if (!(wram_.unk_2d014() & 0x07))
			routines_.call(ADDR_PrologueFade);
		return;
	case 23: wake_up(); return; // Func_ae55c
	case 24: set_fade(0x02); return; // .Func_ae164
	case 25: { // .Func_ae169: every 8 frames, then every other
		uint8_t mask = wram_.pal_config1_total_steps() >= 0x08 ? 0x01 : 0x07;
		if (!(wram_.unk_2d014() & mask))
			routines_.call(ADDR_PrologueFade);
		return;
	}
	case 26: bow(); return; // Func_ae641
	case 27: routines_.call(ADDR_InitTextPrinting); return;
	case 28: routines_.call(ADDR_Func_aca9e); return;
	case 29: wait(30); return; // .Wait30
	case 30: routines_.call(ADDR_LoadCurTextPageBufferPage); return;
	case 31: routines_.call(ADDR_Func_acacd); return;
	case 32: routines_.call(ADDR_Func_acae7); return;
	case 33: // .Func_ae181
		routines_.call(ADDR_Func_acaf3);
		if (mem_[wCurTextBufferPage] == 0x02) // .Func_ae188
			wario_state(SCENEWARIO_LOOK_AROUND);
		return;
	case 34: // .Func_ae196
		wario_state(SCENEWARIO_NOD_ONCE);
		next_step();
		return;
	case 35: // .Func_ae19e: the text arrow once he looks back
		if (mem_[wSceneWarioState] != SCENEWARIO_LOOK_BACK)
			return;
		set_state(wSceneObj1State, 0x01);
		next_step();
		return;
	case 36: // .Func_ae1af
		if (!(wram_.joypad_pressed() & (1 << B_PAD_A)))
			return;
		set_state(wSceneObj1State, 0x00);
		routines_.call(ADDR_ClearTextbox);
		next_step();
		return;
	case 37: walk_off(); return; // Func_ae668
	case 38: ++wram_.sub_state(); return; // Prologue_Finish
	default:
		unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// The sub-scenes, their sub-steps in w2d890 (frames in w2d891)

// Func_ae1ce
void Prologue::next_sub()
{
	mem_[w2d891] = 0;
	++mem_[w2d890];
}

// Func_ae1c9: after frames
void Prologue::wait_sub(uint8_t frames)
{
	if (mem_[w2d891] >= frames)
		next_sub();
}

// Func_ae1d5: the sub-scene done, the next step
void Prologue::end_sub()
{
	mem_[w2d890] = 0;
	mem_[w2d891] = 0;
	next_step();
}

// ApplySceneWarioMovementRight: Wario's x after the step (even)
uint8_t Prologue::walk_right()
{
	routines_.call(ADDR_ApplySceneWarioMovementRight);
	return gb_->cpu.a;
}

// Func_ae1de: the music box's close-up: two sparkles
void Prologue::music_box_close_up()
{
	++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: wait_sub(20); return; // .Wait20
	case 1: // .Func_ae1f5
		mem_[wSceneObj4] = 56;
		mem_[wSceneObj4 + 1] = 56;
		set_state(wSceneObj4State, 0x0e);
		play_sfx(mem_, SFX_104);
		next_sub();
		return;
	case 2: wait_sub(10); return; // .Wait10
	case 3: // .Func_ae215
		mem_[wSceneObj5] = 48;
		mem_[wSceneObj5 + 1] = 120;
		set_state(wSceneObj5State, 0x0e);
		next_sub();
		return;
	case 4: wait_sub(90); return; // .Wait90
	case 5: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Prologue_WarioEnterCave: Wario walks in, is surprised by the music
// box's glow, walks up and takes it
void Prologue::enter_cave()
{
	music_box_glow();
	++mem_[w2d891];
	auto stop_at = [this](uint8_t x) { // .wario_idle_right
		mem_[wSceneWarioXCoord] = x;
		wario_state(SCENEWARIO_IDLE_RIGHT);
		next_sub();
	};
	switch (mem_[w2d890]) {
	case 0: case 6: // .WarioWalkRight
		wario_state(SCENEWARIO_WALK_RIGHT);
		next_sub();
		return;
	case 1: { // .WarioMoveRightTo64, the glow on the way (.Func_ae26c)
		uint8_t x = walk_right();
		if (x == 50)
			mem_[w2d8b0] = 0x01;
		else if (x == 64)
			stop_at(x);
		return;
	}
	case 2: // .Func_ae272
		if (!mem_[w2d8b0])
			next_sub();
		return;
	case 3: // .ShowEmote
		if (mem_[w2d891] < 0x16)
			return;
		mem_[wCurSceneObj] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] - 24);
		mem_[wCurSceneObj + 1] = static_cast<uint8_t>(mem_[wSceneWarioXCoord] + 16);
		set_state(wCurSceneObjState, 0x11);
		play_sfx(mem_, SFX_106);
		next_sub();
		return;
	case 4: // .WaitEmoteAnimation
		if (!mem_[wCurSceneObjState])
			next_sub();
		return;
	case 5: wait_sub(26); return; // .Wait26
	case 7: // .WarioMoveRightTo102
		if (uint8_t x = walk_right(); x == 102)
			stop_at(x);
		return;
	case 8: { // .Func_ae2b8: the glow again
		uint8_t frames = mem_[w2d891];
		if (frames == 0x14)
			mem_[w2d8b0] = 0x03;
		else if (frames >= 0x3c)
			next_sub();
		return;
	}
	case 9: // .WarioLookMusicBox
		wario_state(SCENEWARIO_LOOK_MUSIC_BOX);
		next_sub();
		return;
	case 10: // .WarioHoldMusicBox (Func_ae1c2)
		if (mem_[wSceneWarioState] == SCENEWARIO_HOLD_MUSIC_BOX)
			next_sub();
		return;
	case 11: wait_sub(60); return; // .Wait60
	case 12: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Func_ae2dd: the music box's glow (scene object 1) by w2d8b0: once, or
// (from 3) twice
void Prologue::music_box_glow()
{
	Byte glow = mem_[w2d8b0];
	auto next = [&] { // Func_ae2f5
		mem_[w2d8b1] = 0;
		++glow;
	};
	switch (glow) {
	case 0: return; // Func_ae2fb
	case 1: case 3: // Func_ae2ed
		set_state(wSceneObj1State, 0x0e);
		next();
		return;
	case 2: // Func_ae303
		if (mem_[wSceneObj1State])
			return;
		glow = 0;
		mem_[w2d8b1] = 0;
		return;
	case 4: // Func_ae2fc
		if (!mem_[wSceneObj1State])
			next();
		return;
	case 5: // Func_ae30e: again after 20 frames
		if (++mem_[w2d8b1] >= 0x14)
			glow -= 2;
		return;
	default:
		unrealistic();
		return;
	}
}

// Func_ae523: Wario flickering through the palettes (w2d150 counting them,
// a step every 2 frames), until 0x80 stops it
void Prologue::wario_flicker()
{
	static constexpr uint16_t PALETTES = ADDR_Func_ae523 + 0x28; // .data_ae54b
	Byte flicker = mem_[w2d150];
	if (!flicker)
		return;
	if (flicker == 0x80) {
		mem_[wSceneWarioAttributes] = 0;
		flicker = 0;
		return;
	}
	Byte frames = mem_[w2d151];
	++frames;
	if (frames < 0x02)
		return;
	frames = 0;
	uint8_t palette = flicker;
	++flicker;
	if (flicker >= 0x09)
		flicker = 0x01;
	mem_[wSceneWarioAttributes] = mem_[static_cast<uint16_t>(PALETTES + palette)];
}

// Prologue_WarioSuckedIntoMusicBox: Wario picks up the music box, it
// rises, glows and pulls him in, then drops back on its pedestal
void Prologue::sucked_into_music_box()
{
	static constexpr uint16_t FALL = ADDR_Prologue_WarioSuckedIntoMusicBox + 0x1b5; // .data_ae4d1
	static constexpr uint16_t BOUNCE = ADDR_Prologue_WarioSuckedIntoMusicBox + 0x1ed; // .data_ae509

	wario_flicker();
	uint8_t frames = ++mem_[w2d891];
	auto bump = [this] { play_sfx(mem_, SFX_BUMP); }; // .PlayBumpSFX
	switch (mem_[w2d890]) {
	case 0: // .Func_ae352: the glow over
		if (!mem_[wSceneObj1State])
			next_sub();
		return;
	case 1: // .Func_ae35a
		if (frames < 0x14)
			return;
		stop_music2(mem_);
		next_sub();
		return;
	case 2: wait_sub(30); return; // .Wait30
	case 3: // .WarioPickupMusicBox
		wario_state(SCENEWARIO_PICKUP_MUSIC_BOX);
		next_sub();
		return;
	case 4: // .Func_ae37c
		if (frames >= 0x08)
			return;
		set_state(wSceneObj3State, 0x0a);
		next_sub();
		return;
	case 5: // .WarioShortHopBack
		if (frames == 0x01) { // .hop_up
			--mem_[wSceneWario];
			--mem_[wSceneWario + 1];
		} else if (frames == 0x05) { // .hop_down
			++mem_[wSceneWario];
			--mem_[wSceneWario + 1];
		} else if (frames >= 0x30) {
			next_sub();
		}
		return;
	case 6: { // .MusicBoxRise: a pixel every 8 frames, its glow at 180
		if (frames & 0x07)
			return;
		uint8_t y = --mem_[wSceneObj3YCoord];
		mem_[wSceneObj2YCoord] = static_cast<uint8_t>(y - 8);
		if (y == 180) {
			set_state(wSceneObj2State, 0x0c);
			mem_[w2d150] = 0x01;
		} else if (y == 165) {
			next_sub();
		}
		return;
	}
	case 7: case 20: wait_sub(120); return; // .Wait120_1, .Wait120
	case 8: // .Func_ae3d8
		set_state(wSceneObj2State, 0x00);
		next_sub();
		return;
	case 9: // .Func_ae3e2: the sparkle over Wario
		mem_[wCurSceneObj] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] - 8);
		mem_[wCurSceneObj + 1] = mem_[wSceneWarioXCoord];
		set_state(wCurSceneObjState, 0x0e);
		next_sub();
		return;
	case 10: // .Func_ae3fa
		if (mem_[wCurSceneObjState])
			return;
		mem_[w2d150] = 0x80;
		next_sub();
		return;
	case 11: { // .Func_ae407: Wario turned into a light, OB palette 0
		// all (28, 28, 28)
		mem_[wCurSceneObj] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] - 16);
		mem_[wCurSceneObj + 1] = static_cast<uint8_t>(mem_[wSceneWarioXCoord] + 2);
		set_state(wCurSceneObjState, 0x15);
		constexpr uint16_t GREY = 28 | 28 << 5 | 28 << 10;
		for (uint16_t i = 0; i < 4; i++) {
			mem_[wTempOBPals + i * 2] = static_cast<uint8_t>(GREY);
			mem_[wTempOBPals + i * 2 + 1] = static_cast<uint8_t>(GREY >> 8);
		}
		SET_DE(gb_, ADDR_Data_ac9ff);
		routines_.call(ADDR_Func_ac9e4);
		next_sub();
		return;
	}
	case 12: // .Func_ae435
		if (mem_[wCurSceneObjState] != 0x16)
			return;
		wario_state(SCENEWARIO_NONE);
		next_sub();
		return;
	case 13: wait_sub(60); return; // .Wait60
	case 14: { // .Func_ae447: the light flies into the box
		Byte y = mem_[wCurSceneObj];
		Byte x = mem_[wCurSceneObj + 1];
		if (y >= 181) {
			if (frames & 0x03)
				return;
			if (frames & 0x04)
				++x;
			--y;
			return;
		}
		++x; // .above_181
		y -= 2;
		if (y == 168)
			set_state(wCurSceneObjState, 0x17);
		else if (y == 156)
			next_sub();
		return;
	}
	case 15: // .Func_ae476
		set_state(wSceneObj3State, 0x09);
		set_state(wCurSceneObjState, 0x00);
		next_sub();
		return;
	case 16: // .Func_ae488: the box shakes, every other frame
		if (frames & 1)
			return;
		if (frames >> 1 == 0x01) { // .IncreaseX
			++mem_[wSceneObj3XCoord];
		} else if (frames >> 1 == 0x02) { // .DecreaseX
			mem_[wSceneObj3XCoord] -= 2;
		} else {
			++mem_[wSceneObj3XCoord];
			next_sub();
		}
		return;
	case 17: wait_sub(40); return; // .Wait40
	case 18: { // .Func_ae4a9: and falls back to 195
		Byte counter = mem_[w2d891];
		if (counter >= 0x14) // .Func_ae4c0
			counter = 0x10;
		Byte y = mem_[wSceneObj3YCoord];
		y += mem_[static_cast<uint16_t>(FALL + counter)];
		if (y < 195)
			return;
		y = 195;
		bump();
		next_sub();
		return;
	}
	case 19: { // .Func_ae4e5: a bounce (.Func_ae4fe: offsets, 0x80 last)
		uint16_t entry = static_cast<uint16_t>(BOUNCE + frames);
		uint8_t dy = mem_[entry];
		uint8_t last = mem_[static_cast<uint16_t>(entry + 1)];
		if (!dy)
			return;
		Byte y = mem_[wSceneObj3YCoord];
		y += dy;
		if (y == 195)
			bump();
		if (last == 0x80)
			next_sub();
		return;
	}
	case 21: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Func_ae1c2: once Wario's in the state
void Prologue::wait_wario(uint8_t state)
{
	if (mem_[wSceneWarioState] == state)
		next_sub();
}

uint8_t Prologue::walk_left()
{
	routines_.call(ADDR_ApplySceneWarioMovementLeft);
	return gb_->cpu.a;
}

// Func_ae55c: Wario wakes in the temple, gets up, walks over and hears
// the temple's music
void Prologue::wake_up()
{
	uint8_t frames = ++mem_[w2d891];
	auto after = [&](uint8_t wait, uint8_t state) {
		if (frames < wait)
			return false;
		wario_state(state);
		next_sub();
		return true;
	};
	switch (mem_[w2d890]) {
	case 0: // Func_ae585: the fade done
		if (mem_[w2d14e] == 0xff)
			next_sub();
		return;
	case 1: after(0x4b, SCENEWARIO_WAKE_UP); return; // Func_ae58d
	case 2: // Func_ae599
		if (frames < 0x14)
			return;
		play_sfx(mem_, SFX_GRAB);
		next_sub();
		return;
	case 3: wait_wario(SCENEWARIO_SIT_IDLE); return; // Func_ae5a8
	case 4: // Func_ae5b1: the sweat drop
		if (frames < 0x05)
			return;
		mem_[wCurSceneObjYCoord] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] - 1);
		mem_[wCurSceneObjXCoord] = static_cast<uint8_t>(mem_[wSceneWarioXCoord] + 9);
		set_state(wCurSceneObjState, 0x14);
		next_sub();
		return;
	case 5: after(0x8c, SCENEWARIO_SIT_STILL); return; // Func_ae5d0
	case 6: // Func_ae5dc
		if (after(0x10, SCENEWARIO_IDLE_RIGHT))
			play_sfx(mem_, SFX_12A);
		return;
	case 7: wait_sub(60); return; // Func_ae5f0
	case 8: // Func_ae5f5
		wario_state(SCENEWARIO_WALK_RIGHT);
		next_sub();
		return;
	case 9: // Func_ae5fd
		if (uint8_t x = walk_right(); x == 0x50) {
			mem_[wSceneWarioXCoord] = x;
			wario_state(SCENEWARIO_IDLE_RIGHT);
			next_sub();
		}
		return;
	case 10: after(0x1e, SCENEWARIO_TURN_BACK_RIGHT); return; // Func_ae60c
	case 11: wait_wario(SCENEWARIO_LOOK_BACK); return; // Func_ae618
	case 12: // Func_ae61d
		if (frames < 0x1e)
			return;
		play_music2(mem_, MUSIC_THE_TEMPLE);
		next_sub();
		return;
	case 13: after(0x1e, SCENEWARIO_LOOK_AROUND); return; // Func_ae630
	case 14: wait_sub(30); return; // Func_ae63c
	case 15: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Func_ae641: Wario looks at the hidden figure, jumps and bows
void Prologue::bow()
{
	++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: wait_wario(SCENEWARIO_LOOK_FORWARD); return; // Func_ae652
	case 1: // Func_ae657
		wario_state(SCENEWARIO_JUMP_AND_BOW);
		mem_[wSceneWarioUnk7] = 0;
		next_sub();
		return;
	case 2: wait_wario(SCENEWARIO_LOOK_BACK); return; // Func_ae663
	case 3: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Func_ae668: Wario snaps his fingers and walks off left
void Prologue::walk_off()
{
	uint8_t frames = ++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: // Func_ae683
		wario_state(SCENEWARIO_SNAP_FINGERS);
		next_sub();
		return;
	case 1: // Func_ae68b
		if (mem_[wSceneWarioFrame] != 0x0f)
			return;
		play_sfx(mem_, SFX_106);
		next_sub();
		return;
	case 2: wait_wario(SCENEWARIO_LOOK_BACK); return; // Func_ae69c
	case 3: case 8: wait_sub(30); return; // Func_ae6a1, Func_ae6ce
	case 4: // Func_ae6a6
		wario_state(SCENEWARIO_TURN_BACK_TO_LEFT);
		next_sub();
		return;
	case 5: wait_wario(SCENEWARIO_IDLE_LEFT); return; // Func_ae6ae
	case 6: // Func_ae6b3
		if (frames < 0x04)
			return;
		wario_state(SCENEWARIO_WALK_LEFT);
		next_sub();
		return;
	case 7: // Func_ae6c1: off the screen
		if (walk_left() != 0xf0)
			return;
		wario_state(SCENEWARIO_NONE);
		next_sub();
		return;
	case 9: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Prologue_WarioFallFromPlane: Wario falls burning from the plane (its
// parts flying, Func_ae8bb), crashes, bounces and gets up
void Prologue::fall_from_plane()
{
	plane_parts(); // Func_ae8bb
	++mem_[w2d891];
	switch (mem_[w2d890]) {
	case 0: wait_sub(74); return; // .Wait74
	case 1: // .ShowWarioFall
		mem_[wSceneWarioYCoord] = 60;
		mem_[wSceneWarioXCoord] = 240;
		wario_state(SCENEWARIO_ON_FIRE);
		play_sfx(mem_, SFX_THROW);
		mem_[w2d894] = 0;
		mem_[w2d892] = 0;
		next_sub();
		return;
	case 2: // .WarioBounce
		bounce();
		if (mem_[w2d892] == 0x80)
			next_sub();
		return;
	case 3: wait_sub(60); return; // .Wait60
	case 4: // .WarioTurnRight
		wario_state(SCENEWARIO_TURN_BACK_RIGHT);
		next_sub();
		return;
	case 5: wait_wario(SCENEWARIO_LOOK_BACK); return; // .WarioLookBack
	case 6: wait_sub(38); return; // .Wait38
	case 7: // .WarioLookAround
		wario_state(SCENEWARIO_LOOK_AROUND_THEN_TURN);
		next_sub();
		return;
	case 8: wait_wario(SCENEWARIO_IDLE_RIGHT); return; // .WarioIdleRight
	case 9: wait_sub(46); return; // .Wait46
	case 10: end_sub(); return;
	default: unrealistic(); return;
	}
}

// Prologue_WarioFallFromPlane.DoBounce: his flight (Data_aed06, every
// frame of 4 but the first) and landings (w2d892's steps)
void Prologue::bounce()
{
	if (mem_[w2d894] && (mem_[w2d891] & 0x03)) {
		SET_DE(gb_, ADDR_Data_aed06);
		SET_HL(gb_, wSceneWario);
		routines_.call(ADDR_ApplyOWMovement);
		if (gb_->cpu.f & FLAG_Z)
			mem_[w2d894] = 0;
	}
	Byte action = mem_[w2d892];
	auto next = [&] { // .asm_ae768
		++action;
		mem_[w2d893] = 0;
	};
	auto land = [&](uint8_t state, bool stars) { // on the ground, at 96
		if (mem_[wSceneWarioYCoord] != 96)
			return;
		wario_state(state);
		play_sfx(mem_, SFX_BUMP); // .PlayBumpSfx
		if (stars) { // .asm_ae77f
			mem_[wCurSceneObj] = mem_[wSceneWarioYCoord];
			mem_[wCurSceneObj + 1] = mem_[wSceneWarioXCoord];
			set_state(wCurSceneObjState, 0x10);
		}
		next();
	};
	switch (action) {
	case 0: // .asm_ae763
		mem_[w2d894] = 0x01;
		next();
		return;
	case 1: land(SCENEWARIO_STUNNED_RIGHT, true); return; // .asm_ae771
	case 2: case 5: // .asm_ae794
		if (++mem_[w2d893] >= 0x04)
			next();
		return;
	case 3: // .asm_ae79e
		wario_state(SCENEWARIO_CRASH_TUMBLE_END);
		next();
		return;
	case 4: land(SCENEWARIO_CRASH_LAND_RIGHT, true); return; // .asm_ae7a5
	case 6: land(SCENEWARIO_CRASH_LAND_RIGHT, false); return; // .asm_ae7b5
	case 7: // .asm_ae7c5
		if (mem_[wSceneWarioState] == SCENEWARIO_STUNNED_RIGHT)
			next();
		return;
	case 8: // .asm_ae7d4
		if (++mem_[w2d893] < 0x3e)
			return;
		wario_state(SCENEWARIO_IDLE_RIGHT);
		play_sfx(mem_, SFX_12A);
		next();
		return;
	case 9: action = 0x80; return; // .asm_ae7f2
	default: return;
	}
}

// Prologue_WarioWalkToCaveEntrance: Wario walks right, sees the cave and
// walks in
void Prologue::walk_to_cave()
{
	++mem_[w2d891];
	auto walk_to = [this](uint8_t x, uint8_t state) {
		if (walk_right() != x)
			return;
		mem_[wSceneWarioXCoord] = x;
		wario_state(state);
		next_sub();
	};
	switch (mem_[w2d890]) {
	case 0: case 3: wait_sub(60); return; // .Wait60_1, .Wait60_2
	case 1: case 9: // .WarioWalkRight
		wario_state(SCENEWARIO_WALK_RIGHT);
		next_sub();
		return;
	case 2: walk_to(120, SCENEWARIO_IDLE_RIGHT); return; // .WarioMoveRightTo120
	case 4: // .WarioLookUp
		wario_state(SCENEWARIO_LOOK_UP);
		next_sub();
		return;
	case 5: wait_wario(SCENEWARIO_IDLE_RIGHT); return; // .WarioIdleRight
	case 6: // .ShowEmote
		mem_[wCurSceneObj] = static_cast<uint8_t>(mem_[wSceneWarioYCoord] - 24);
		mem_[wCurSceneObj + 1] = static_cast<uint8_t>(mem_[wSceneWarioXCoord] + 16);
		set_state(wCurSceneObjState, 0x11);
		play_sfx(mem_, SFX_106);
		next_sub();
		return;
	case 7: // .WaitEmoteAnimation
		if (!mem_[wCurSceneObjState])
			next_sub();
		return;
	case 8: case 11: wait_sub(40); return; // .Wait40_1, .Wait40_2
	case 10: walk_to(208, SCENEWARIO_NONE); return; // .WarioMoveRightTo208
	case 12: end_sub(); return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The plane's parts (Func_ae8bb): four falling from the sky one after
// another, each a scene object (some with a shadow) running a script

namespace {

enum class PartOp : uint8_t {
	PLACE, // at (y, x) in state, the shadow by shadow
	FALL, // along data until the ground (y 0x62), the shadow under it
	ROLL, // along data to its end
	WAIT, // frames
	STATE, // to state
	END,
};

enum Shadow : uint8_t { NO_SHADOW, HIDE_SHADOW, SHOW_SHADOW, FOLLOW };

struct PartStep {
	PartOp op;
	uint8_t y, x, state; // PLACE (state: STATE's too); WAIT: y the frames
	uint16_t data;
	Shadow shadow;
};

constexpr PartStep place(uint8_t y, uint8_t x, uint8_t state, Shadow shadow = NO_SHADOW)
{
	return {PartOp::PLACE, y, x, state, 0, shadow};
}
constexpr PartStep fall(uint16_t data, Shadow shadow = NO_SHADOW) { return {PartOp::FALL, 0, 0, 0, data, shadow}; }
constexpr PartStep roll(uint16_t data) { return {PartOp::ROLL, 0, 0, 0, data, NO_SHADOW}; }
constexpr PartStep wait_for(uint8_t frames) { return {PartOp::WAIT, frames, 0, 0, 0, NO_SHADOW}; }
constexpr PartStep to_state(uint8_t state) { return {PartOp::STATE, 0, 0, state, 0, NO_SHADOW}; }
constexpr PartStep END_PART = {PartOp::END, 0, 0, 0, 0, NO_SHADOW};

// Func_ae924
constexpr PartStep PART1[] = {
	place(46, 240, 0x06), fall(ADDR_Data_aee09), roll(ADDR_Data_aee09), wait_for(0x08),
	place(77, 224, 0x05), fall(ADDR_Data_aedba), roll(ADDR_Data_aedba), wait_for(0x28),
	place(62, 240, 0x04), fall(ADDR_Data_aed6d), roll(ADDR_Data_aed6d), wait_for(0x08), END_PART,
};
// Func_ae9ee
constexpr PartStep PART2[] = {
	place(62, 240, 0x02, HIDE_SHADOW), fall(ADDR_Data_aed6d, FOLLOW), roll(ADDR_Data_aed6d), wait_for(0x04),
	to_state(0x12), wait_for(0x28),
	place(46, 224, 0x08, SHOW_SHADOW), fall(ADDR_Data_aee09, FOLLOW), roll(ADDR_Data_aee09), wait_for(0x08),
	place(46, 244, 0x04, SHOW_SHADOW), fall(ADDR_Data_aee09), roll(ADDR_Data_aee09), wait_for(0x08), END_PART,
};
// Func_aeb0d
constexpr PartStep PART3[] = {
	place(62, 224, 0x03, SHOW_SHADOW), fall(ADDR_Data_aed6d, FOLLOW), roll(ADDR_Data_aed6d), wait_for(0x08),
	place(46, 240, 0x07), fall(ADDR_Data_aee09), roll(ADDR_Data_aee09), wait_for(0x04),
	to_state(0x13), wait_for(0x28),
	place(77, 248, 0x05), fall(ADDR_Data_aedba), roll(ADDR_Data_aedba), wait_for(0x08), END_PART,
};
// Func_aec1f
constexpr PartStep PART4[] = {
	place(77, 240, 0x07), fall(ADDR_Data_aedba), roll(ADDR_Data_aedba), wait_for(0x04),
	to_state(0x13), wait_for(0x28),
	place(62, 240, 0x08, SHOW_SHADOW), fall(ADDR_Data_aed6d, FOLLOW), roll(ADDR_Data_aed6d), wait_for(0x08),
	END_PART,
};

struct Part {
	uint16_t counter; // its action the byte before
	uint16_t obj, shadow;
	const PartStep *steps;
	uint8_t count;
};

constexpr Part PARTS[] = {
	{wPlanePart1Counter, wSceneObj3, 0, PART1, sizeof(PART1) / sizeof(PART1[0])},
	{wPlanePart2Counter, wSceneObj4, wSceneObj1, PART2, sizeof(PART2) / sizeof(PART2[0])},
	{wPlanePart3Counter, wSceneObj5, wSceneObj2, PART3, sizeof(PART3) / sizeof(PART3[0])},
	{wPlanePart4Counter, wSceneObj9, wSceneObj8, PART4, sizeof(PART4) / sizeof(PART4[0])},
};

} // namespace

// Func_ae915: the counter's action's next step
void Prologue::next_action(uint16_t counter)
{
	mem_[counter] = 0;
	++mem_[static_cast<uint16_t>(counter - 1)];
}

// Func_ae8bb: the parts, each starting 8 frames after the last
void Prologue::plane_parts()
{
	for (int i = 0; i < 4; i++)
		plane_part(i);
	Byte frames = mem_[w2d8a1];
	++frames;
	uint8_t part = mem_[w2d8a0];
	if (part == 4) // .Func_ae8d8: all started
		return;
	if (part > 4) {
		unrealistic();
		return;
	}
	if (frames < 0x08)
		return;
	next_action(PARTS[part].counter);
	next_action(w2d8a1); // .asm_ae912
}

// Func_ae924, Func_ae9ee, Func_aeb0d, Func_aec1f: a part's script
void Prologue::plane_part(int index)
{
	const Part &part = PARTS[index];
	uint16_t counter = part.counter;
	Byte frames = mem_[counter];
	++frames;
	Byte action = mem_[static_cast<uint16_t>(counter - 1)];
	if (!action)
		return;
	uint8_t i = action - 1;
	if (i >= part.count) {
		unrealistic();
		return;
	}
	const PartStep &step = part.steps[i];
	uint16_t obj = part.obj;
	auto move = [&] { // ApplyOWMovement: whether still moving
		SET_DE(gb_, step.data);
		SET_HL(gb_, obj);
		routines_.call(ADDR_ApplyOWMovement);
		return !(gb_->cpu.f & FLAG_Z);
	};
	auto follow = [&] { // the shadow under it
		mem_[part.shadow] = mem_[obj];
		mem_[part.shadow + 1] = mem_[obj + 1];
	};
	switch (step.op) {
	case PartOp::PLACE: // SetSceneObjCoordAndState
		mem_[obj] = step.y;
		mem_[obj + 1] = step.x;
		set_state(static_cast<uint16_t>(obj + 6), step.state);
		if (step.shadow == HIDE_SHADOW) {
			set_state(static_cast<uint16_t>(part.shadow + 6), 0x00);
		} else if (step.shadow == SHOW_SHADOW) {
			follow();
			set_state(static_cast<uint16_t>(part.shadow + 6), 0x0f);
		}
		next_action(counter);
		return;
	case PartOp::FALL:
		move();
		if (step.shadow == FOLLOW)
			follow();
		if (mem_[obj] != 0x62)
			return;
		if (step.shadow == FOLLOW)
			set_state(static_cast<uint16_t>(part.shadow + 6), 0x10);
		play_sfx(mem_, SFX_006);
		++action;
		return;
	case PartOp::ROLL:
		if (move())
			return;
		play_sfx(mem_, SFX_006);
		next_action(counter);
		return;
	case PartOp::WAIT:
		if (frames >= step.y)
			next_action(counter);
		return;
	case PartOp::STATE:
		set_state(static_cast<uint16_t>(obj + 6), step.state);
		next_action(counter);
		return;
	case PartOp::END:
		action = 0;
		set_state(static_cast<uint16_t>(obj + 6), 0x00);
		return;
	}
}

} // namespace wl3::overworld

// Farcalled by PrologueFunc (bank27's C, which passes bc, de and hl on: as
// the asm, they're left by the last routine, Func_ac3fd).
PORT_EX(Prologue, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).update(); }
PORT_EX(Func_ae1de, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).music_box_close_up(); }
PORT_EX(Prologue_WarioEnterCave, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).enter_cave(); }
PORT_EX(Prologue_WarioSuckedIntoMusicBox, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).sucked_into_music_box(); }
PORT_EX(Func_ae55c, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).wake_up(); }
PORT_EX(Func_ae641, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).bow(); }
PORT_EX(Func_ae668, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).walk_off(); }
PORT_EX(Prologue_WarioFallFromPlane, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).fall_from_plane(); }
PORT_EX(Prologue_WarioWalkToCaveEntrance, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).walk_to_cave(); }
PORT_EX(Func_ae8bb, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).plane_parts(); }
PORT_EX(Func_ae523, 0, ALL_REGISTERS) { wl3::overworld::Prologue(gb).wario_flicker(); }

// Only Prologue_WarioEnterCave runs these (the glow).
PORT_COVERED(Func_ae2dd);
PORT_COVERED(Func_ae2ed);
PORT_COVERED(Func_ae2f5);
PORT_COVERED(Func_ae2fb);
PORT_COVERED(Func_ae2fc);
PORT_COVERED(Func_ae303);
PORT_COVERED(Func_ae30e);

// Only Func_ae55c, Func_ae641 and Func_ae668 run these (their steps).
PORT_COVERED(Func_ae585);
PORT_COVERED(Func_ae58d);
PORT_COVERED(Func_ae599);
PORT_COVERED(Func_ae5a8);
PORT_COVERED(Func_ae5b1);
PORT_COVERED(Func_ae5d0);
PORT_COVERED(Func_ae5dc);
PORT_COVERED(Func_ae5f0);
PORT_COVERED(Func_ae5f5);
PORT_COVERED(Func_ae5fd);
PORT_COVERED(Func_ae60c);
PORT_COVERED(Func_ae618);
PORT_COVERED(Func_ae61d);
PORT_COVERED(Func_ae630);
PORT_COVERED(Func_ae63c);
PORT_COVERED(Func_ae652);
PORT_COVERED(Func_ae657);
PORT_COVERED(Func_ae663);
PORT_COVERED(Func_ae683);
PORT_COVERED(Func_ae68b);
PORT_COVERED(Func_ae69c);
PORT_COVERED(Func_ae6a1);
PORT_COVERED(Func_ae6a6);
PORT_COVERED(Func_ae6ae);
PORT_COVERED(Func_ae6b3);
PORT_COVERED(Func_ae6c1);
PORT_COVERED(Func_ae6ce);

// Only Func_ae8bb runs these (the plane's parts).
PORT_COVERED(Func_ae915);
PORT_COVERED(SetSceneObjCoordAndState);
PORT_COVERED(Func_ae924);
PORT_COVERED(Func_ae9ee);
PORT_COVERED(Func_aeb0d);
PORT_COVERED(Func_aec1f);
PORT_COVERED(Func_ae9d0);
PORT_COVERED(Func_ae9da);
PORT_COVERED(Func_ae9e4);
