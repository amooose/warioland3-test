// The epilogue (bank2b.asm, _InitEpilogue and Func_af13c). See
// overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint16_t PALS_AF35E = ADDR_Func_af13c + 0x222; // .Pals_af35e: the gifts'

// the sub-steps' counter (w2d891, after the sub-step w2d890)
constexpr uint16_t SUB_STEP = w2d890;
constexpr uint16_t SUB_FRAMES = w2d891;

} // namespace

void Epilogue::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void Epilogue::wario_state(uint8_t state)
{
	SceneWario(gb_).set_state(state);
}

void Epilogue::set_state(uint16_t state, uint8_t value)
{
	home::set_scene_obj_state(gb_, state, value);
}

void Epilogue::far_pals(uint8_t bank, uint16_t src, uint16_t dest)
{
	home::copy_far_bytes(gb_, bank, src, dest, 8 * PAL_SIZE);
}

// The decompress_* macros
void Epilogue::decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1)
{
	if (vram1)
		mem_[rVBK] = 1;
	wram_.temp_bank() = bank;
	routines_.in_bank(bank, ADDR_Decompress, [&] { home::decompress(gb_, RomPtr(gb_, bank, src), dest); });
	if (vram1)
		mem_[rVBK] = 0;
}

// _InitEpilogue: the epilogue's graphics (the crowd, the old man and the
// enemies Wario turned back) and the old man's thanks
void Epilogue::init()
{
	routines_.disable_lcd();
	home::fill_bg_map0(gb_, 0x7f);
	routines_.clear_virtual_oam();
	stop_music2(mem_);
	mem_[rWBK] = 0x02;
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	mem_[rSCY] = 0;
	wram_.scy() = 0;
	SceneCommon(gb_).set_vblank();
	SceneCommon(gb_).clear_scene_ram();
	SceneCommon(gb_).clear_scene_objs();
	for (uint16_t pals : {wTempPals1, wTempPals2}) {
		home::fill(gb_, pals, 0, 8 * PAL_SIZE);
	}
	decompress(BANK_EpilogueGfx, ADDR_EpilogueGfx, v0Tiles0, false);
	decompress(BANK_FontGfx, ADDR_FontGfx, v1Tiles0, true);
	mem_[rVBK] = 1;
	routines_.copy_bc(v0Tiles1, wAttrmap, 0x50 * TILE_SIZE);
	home::copy_far_bytes(gb_, BANK_EpilogueStarGfx, ADDR_EpilogueStarGfx, v1Tiles1 + 0x40 * TILE_SIZE, 0x00);
	mem_[rVBK] = 0;
	decompress(BANK_SpearheadGfx, ADDR_SpearheadGfx, v1Tiles0, true);
	decompress(BANK_HammerBotGfx, ADDR_HammerBotGfx, v1Tiles0 + 0x40 * TILE_SIZE, true);
	decompress(BANK_DoughnuteerGfx, ADDR_DoughnuteerGfx, v1Tiles1, true);
	decompress(BANK_EpilogueTransformationAttrmap, ADDR_EpilogueTransformationAttrmap, v1BGMap0, true);
	decompress(BANK_EpilogueTransformationTilemap, ADDR_EpilogueTransformationTilemap, v0BGMap0, false);
	decompress(BANK_PrologueAttrmap, ADDR_PrologueAttrmap, v1BGMap1, true);
	decompress(BANK_PrologueTilemap, ADDR_PrologueTilemap, v0BGMap1, false);
	SceneCommon(gb_).fill_cleared_text_buffer();
	decompress(BANK_TextJP_OldManThankYou,
		wram_.language() ? ADDR_TextEN_OldManThankYou : ADDR_TextJP_OldManThankYou, wTextBuffer, false);
	wram_.unk_2d013() = 0;
	wram_.unk_2d014() = 0;
	wram_.pal_config1_total_steps() = 0;
	mem_.pal_config2_total_steps() = 0;
	uint8_t lcdc = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
	mem_[rLCDC] = static_cast<uint8_t>(lcdc ^ mem_.collection_row());
	mem_.collection_row() = 0;
	++wram_.sub_state();
}

// Func_af13c: a frame of the epilogue
void Epilogue::update()
{
	step();
	routines_.call(ADDR_Func_ac074);
	routines_.call(ADDR_UpdateSceneWario);
	routines_.call(ADDR_Func_ac080);
	routines_.clear_unused_virtual_oam();
}

// Func_af135
void Epilogue::next_step()
{
	wram_.unk_2d014() = 0;
	++wram_.unk_2d013();
}

// Func_af130: after frames
void Epilogue::wait(uint8_t frames)
{
	if (wram_.unk_2d014() >= frames)
		next_step();
}

// .asm_af40c
void Epilogue::next_sub()
{
	mem_[SUB_FRAMES] = 0;
	++mem_[SUB_STEP];
}

// .asm_af413: the sub-steps done, the next step
void Epilogue::end_sub()
{
	mem_[SUB_STEP] = 0;
	mem_[SUB_FRAMES] = 0;
	next_step();
}

// .asm_af400: once Wario's in the state
void Epilogue::wait_wario(uint8_t state)
{
	if (mem_.owui_obj1_state() == state)
		next_sub();
}

// .Func_af41c: the fade (Func_84e43) done
void Epilogue::wait_fade()
{
	routines_.farcall(BANK_Func_84e43, ADDR_Func_84e43);
	if (!wram_.pal_config1_total_steps())
		next_sub();
}

// The HDMA of a block (.asm_af1f2): in OldMan1Gfx's bank
void Epilogue::hdma(uint8_t bank, uint8_t vram_bank, uint16_t src, uint8_t dest_hi, uint8_t mode)
{
	mem_.hdma_dest_vram_bank() = vram_bank;
	mem_.hdma_bank() = bank;
	mem_[wHDMA + 0] = static_cast<uint8_t>(src >> 8);
	mem_[wHDMA + 1] = static_cast<uint8_t>(src);
	mem_[wHDMA + 2] = dest_hi;
	mem_[wHDMA + 3] = 0;
	mem_[wHDMA + 4] = mode;
	next_step();
}

// .Func_af1bd: the epilogue's palettes, from the current ones
void Epilogue::load_pals()
{
	SceneCommon(gb_).copy_temp_pals();
	far_pals(BANK_Pals_86add, ADDR_Pals_86add, wTempPals1);
	far_pals(BANK_Pals_86b1d, ADDR_Pals_86b1d, wTempPals2);
}

// .Func_af1db: the gifts' palettes
void Epilogue::load_gift_pals()
{
	routines_.copy(PALS_AF35E, wTempPals2 + 4 * PAL_SIZE, 3 * PAL_SIZE);
}

// Func_af13c.Func_af14c: the epilogue's steps
void Epilogue::step()
{
	crowd();
	++wram_.unk_2d014();
	switch (wram_.unk_2d013()) {
	case 0: next_step(); return;
	case 1: wait(0x3c); return; // .Func_af197
	case 2: // .Func_af19c: the old man
		mem_.scene_obj9() = 0x60;
		mem_[wSceneObj9 + 1] = 0x50;
		set_state(wSceneObj9State, 0x18);
		load_pals();
		load_gift_pals();
		wram_.pal_config1_total_steps() = 0;
		mem_.pal_config2_total_steps() = 0;
		next_step();
		return;
	case 3: case 13: case 27: SceneCommon(gb_).fade(); return;
	case 4: music_boxes(); return; // .Func_af7af
	case 5: case 23: case 29: // .Func_af2d0: to black
		mem_.unk_2d880() = 0;
		wram_.pal_config1_total_steps() = 0;
		mem_.pal_config2_total_steps() = 0;
		next_step();
		return;
	case 6: case 24: SceneCommon(gb_).prologue_fade(); return;
	// the crowd's graphics, by HDMA
	case 7: hdma(BANK_OldMan1Gfx, 0, ADDR_OldMan1Gfx, (v0Tiles1 >> 8) - 0x80, 0x40 - 1); return;
	case 8: hdma(BANK_OldMan1Gfx, 1, ADDR_OldMan2Gfx, (v1Tiles0 >> 8) - 0x80, 0x80 - 1); return;
	case 9: hdma(BANK_OldMan1Gfx, 0, ADDR_EpilogueCrowdTilemap, (v0BGMap0 >> 8) - 0x80, 18 * 2 - 1); return;
	case 10: hdma(BANK_OldMan1Gfx, 1, ADDR_EpilogueCrowdAttrmap, (v1BGMap0 >> 8) - 0x80, 18 * 2 - 1); return;
	case 11: hdma(BANK_OldMan1Gfx, 1, wAttrmap, (v1Tiles1 >> 8) - 0x80, 0x50 - 1); return;
	case 12: crowd_start(); return; // .Func_af23e
	case 14: transformation(); return; // .Func_af741
	case 15: SceneCommon(gb_).init_text(); return;
	case 16: SceneCommon(gb_).print_page(); return;
	case 17: wait(0x1e); return; // .Func_af2bc
	case 18: SceneCommon(gb_).load_page(); return;
	case 19: SceneCommon(gb_).wait_for_a(); return;
	case 20: // .Func_af2c1: the old man's thanks
		SceneCommon(gb_).print_without_header();
		old_man_talks();
		if (mem_.cur_text_line() == 0x80)
			next_step();
		return;
	case 21: SceneCommon(gb_).next_page(); return;
	case 22: leave_on_gift(); return; // .Func_af670
	case 25: // .Func_af2df
		hdma(BANK_PrologueBackgroundGfx, 0, ADDR_PrologueBackgroundGfx, (v0Tiles2 >> 8) - 0x80, 0x80 - 1);
		return;
	case 26: // .Func_af2f2: the castle
		mem_.scene_obj9() = 0x3c;
		mem_[wSceneObj9 + 1] = 0xb2;
		set_state(wSceneObj9State, 0x13);
		mem_.owui_obj1_y_coord() = 0x3c;
		mem_.owui_obj1_x_coord() = 0xc8;
		wario_state(0x1c);
		for (uint16_t state : {wSceneObj10State, wSceneObj11State, wSceneObj12State, wSceneObj13State})
			mem_[state] = 0;
		mem_.collection_row() = 0x08;
		SceneCommon(gb_).copy_temp_pals();
		far_pals(BANK_Pals_869cd, ADDR_Pals_869cd, wTempPals1);
		far_pals(BANK_Pals_86b1d, ADDR_Pals_86b1d, wTempPals2);
		wram_.pal_config1_total_steps() = 0;
		mem_.pal_config2_total_steps() = 0;
		next_step();
		return;
	case 28: thrown_out(); return; // .Func_af436
	case 30: // .Func_af34a: a fade step every 4 frames
		if (!(wram_.unk_2d014() & 0x03))
			SceneCommon(gb_).prologue_fade();
		return;
	case 31: // .Func_af353
		if (wram_.unk_2d014() >= 0x3c)
			++wram_.sub_state();
		return;
	default:
		unrealistic();
		return;
	}
}

// .Func_af8bc: the crowd's enemies walking right, while the first walks
void Epilogue::crowd()
{
	if (mem_.scene_obj10_state() != 0x1b)
		return;
	static constexpr struct {
		uint16_t obj;
		uint8_t state, mask;
	} WALKERS[] = {
		{wSceneObj10, 0x1b, 0x03}, // .Func_af8d0
		{wSceneObj11, 0x1c, 0x07}, // .Func_af8dc
		{wSceneObj12, 0x1b, 0x03}, // .Func_af8ef
		{wSceneObj13, 0x1c, 0x07}, // .Func_af902
	};
	for (const auto &w : WALKERS) {
		if (mem_[w.obj + 6] != w.state || (wram_.global_counter() & w.mask))
			continue;
		++mem_[w.obj + 1];
	}
}

// .Func_af23e: the crowd scene: Wario and four enemies
void Epilogue::crowd_start()
{
	mem_[rSCX] = SCREEN_WIDTH_PX;
	wram_.scx() = SCREEN_WIDTH_PX;
	mem_[rSCY] = 0;
	wram_.scy() = 0;
	mem_.owui_obj1_y_coord() = 0x60;
	mem_.owui_obj1_x_coord() = 0x5c;
	mem_.owui_obj2_y_coord() = 0xff;
	wario_state(0x0c);
	static constexpr struct {
		uint16_t obj;
		uint8_t y, x, state;
	} ENEMIES[] = {
		{wSceneObj10, 0x30, 0x00, 0x1b}, {wSceneObj11, 0x35, 0x20, 0x1c},
		{wSceneObj12, 0x32, 0x70, 0x1b}, {wSceneObj13, 0x38, 0xc0, 0x1c},
	};
	for (const auto &e : ENEMIES) { // .Func_af2ab
		mem_[e.obj] = e.y;
		mem_[e.obj + 1] = e.x;
		set_state(static_cast<uint16_t>(e.obj + 6), e.state);
	}
	far_pals(BANK_Pals_86add, ADDR_Pals_86add, wTempPals1);
	far_pals(BANK_Pals_86add, ADDR_Pals_86b1d, wTempPals2);
	wram_.pal_config1_total_steps() = 0;
	mem_.pal_config2_total_steps() = 0;
	mem_.unk_2d890() = 0;
	mem_.unk_2d891() = 0;
	next_step();
}

// .Func_af376: the old man's mouth and gestures by the text's page and
// line (by language), at each line's first character
void Epilogue::old_man_talks()
{
	if (!mem_.pending_char_dest() || mem_.cur_text_line_pos() != 1)
		return;
	uint8_t page = mem_.pal_fade7_colour3_red_unk1();
	uint8_t line = mem_.pal_fade7_colour3_red_sign();
	auto talk = [&] { // .asm_af3bf
		if (line == 1)
			set_state(wSceneObj9State, 0x0b);
	};
	auto gesture = [&](uint8_t state, uint8_t at_line) { // .asm_af3d0 / .asm_af3ea
		if (line == at_line)
			set_state(wSceneObj9State, state);
		else if (at_line != 1) // (on line 1's, only then)
			talk();
	};
	if (!wram_.language()) { // japanese
		switch (page) {
		case 0x01: gesture(0x10, 1); return;
		case 0x04: gesture(0x0c, 1); return;
		case 0x05: gesture(0x0d, 2); return;
		case 0x07: gesture(0x0e, 2); return;
		case 0x0a: gesture(0x10, 2); return;
		case 0x0b: gesture(0x0c, 3); return;
		}
	} else {
		switch (page) {
		case 0x01: gesture(0x10, 1); return;
		case 0x04: gesture(0x0c, 1); return;
		case 0x05: gesture(0x0d, 1); return;
		case 0x08: gesture(0x0e, 2); return;
		case 0x0b: gesture(0x10, 1); return;
		case 0x0c: gesture(0x0c, 1); return;
		}
	}
	talk();
}

// .Func_af7af: the music boxes' tune, each box's light (the attributes'
// palette) and the old man's gestures between
void Epilogue::music_boxes()
{
	auto boxes = [this](uint8_t attributes, bool started) { // .asm_af7ff
		for (uint16_t a : {wSceneObj5Attributes, wSceneObj6Attributes, wSceneObj7Attributes,
			wSceneObj8Attributes, wSceneObj1Attributes})
			mem_[a] = attributes;
		if (started) {
			mem_.unk_2d8a0() = 0x01;
			next_sub();
		}
	};
	auto turn = [this](uint8_t state, uint8_t dx) { // .Func_af82d and the like
		set_state(wSceneObj9State, state);
		mem_.scene_obj9_x_coord() -= dx;
		mem_.scene_obj1_state() = 0; // .asm_af839
		load_pals();
		next_sub();
	};
	auto face = [this](uint8_t state, uint8_t x) { // .Func_af843 / .Func_af871
		if (mem_[SUB_FRAMES] < 0x50)
			return;
		set_state(wSceneObj9State, state);
		mem_.scene_obj9_x_coord() = x;
		load_gift_pals();
		SceneCommon(gb_).set_pal_config(wPalConfig2, ADDR_Data_aca04);
		next_sub();
	};

	routines_.farcall(BANK_Func_15848a, ADDR_Func_15848a);
	uint8_t s = mem_[SUB_STEP];
	++mem_[SUB_FRAMES];
	switch (s) {
	case 0: // .Func_af7ee
		play_music2(mem_, MUSIC_EPILOGUE);
		next_sub();
		return;
	case 1: boxes(0x04, true); return; // .Func_af7fd
	case 2: case 8: case 14: // .Func_af816: once the light's down
		if (mem_.scene_obj1_y_coord() < 0x50)
			return;
		play_sfx(mem_, SFX_0EB);
		SceneCommon(gb_).copy_temp_pals();
		SceneCommon(gb_).white_pals(wTempPals2);
		next_sub();
		return;
	case 3: case 5: case 9: case 11: case 15: case 17: wait_fade(); return;
	case 4: turn(0x02, 1); return;
	case 6: face(0x19, 0x48); return;
	case 7: boxes(0x00, true); return; // .Func_af860
	case 10: turn(0x03, 1); return; // .Func_af863
	case 12: face(0x1a, 0x58); return;
	case 13: boxes(0x03, true); return; // .Func_af88e
	case 16: turn(0x04, 2); return; // .Func_af893
	case 18: // .Func_af8a2
		if (mem_[SUB_FRAMES] >= 0x50)
			next_sub();
		return;
	case 19: // .Func_af8a9
		boxes(0x00, false);
		end_sub();
		return;
	default:
		unrealistic();
		return;
	}
}

// .Func_af741: the transformation: the old man walks off right, the
// screen scrolls after him, and he turns back
void Epilogue::transformation()
{
	auto walk = [this] { // .Func_af7a7
		Byte x = mem_.scene_obj9_x_coord();
		++x;
		return static_cast<uint8_t>(x & 0xfe);
	};
	uint8_t s = mem_[SUB_STEP];
	uint8_t frames = ++mem_[SUB_FRAMES];
	switch (s) {
	case 0: // .Func_af755
		if (frames < 0x3c)
			return;
		set_state(wSceneObj9State, 0x08);
		mem_.scene_obj9() = 0x60;
		mem_[wSceneObj9 + 1] = 0x80;
		next_sub();
		return;
	case 1: // .Func_af76c
		if (walk() == 0xe0)
			next_sub();
		return;
	case 2: { // .Func_af775
		walk();
		Byte scx = wram_.scx();
		++scx;
		if (scx)
			return;
		set_state(wSceneObj9State, 0x09);
		next_sub();
		return;
	}
	case 3: // .Func_af788
		if (frames < 0x1e)
			return;
		set_state(wSceneObj9State, 0x06);
		next_sub();
		return;
	case 4: // .Func_af797
		if (mem_.scene_obj9_state() == 0x0a)
			next_sub();
		return;
	case 5: // .Func_af7a0
		if (frames >= 0x3c)
			end_sub();
		return;
	default:
		unrealistic();
		return;
	}
}

// .Func_af670: Wario takes his gift (A) and the old man flies off
void Epilogue::leave_on_gift()
{
	Prologue(gb_).wario_flicker(); // Func_ae523
	uint8_t s = mem_[SUB_STEP];
	uint8_t frames = ++mem_[SUB_FRAMES];
	switch (s) {
	case 0: // .Func_af68b: the text arrow
		if (frames < 0x1e)
			return;
		set_state(wSceneObj1State, 0x01);
		next_sub();
		return;
	case 1: // .Func_af69a
		if (!(wram_.joypad_pressed() & (1 << B_PAD_A)))
			return;
		set_state(wSceneObj1State, 0x00);
		stop_music2(mem_);
		SceneCommon(gb_).clear_textbox();
		set_state(wSceneObj9State, 0x0f);
		next_sub();
		return;
	case 2: // .Func_af6c1
		if (frames < 0x3c)
			return;
		mem_.unk_2d150() = 0x01;
		next_sub();
		return;
	case 3: // .Func_af6cd: the gift, over Wario
		if (frames < 0x28)
			return;
		mem_.unk_2d150() = 0x80;
		mem_.cur_scene_obj() = static_cast<uint8_t>(mem_.owui_obj1_y_coord() - 4);
		mem_[wCurSceneObj + 1] = mem_.owui_obj1_x_coord();
		set_state(wCurSceneObjState, 0x11);
		next_sub();
		return;
	case 4: // .Func_af6ee
		if (frames < 0x14)
			return;
		wario_state(0);
		next_sub();
		return;
	case 5: // .Func_af6f9
		if (frames >= 0x5a)
			next_sub();
		return;
	case 6: { // .Func_af700: the gift rises, slowly then fast
		Byte y = mem_.cur_scene_obj();
		if (y >= 0x50) {
			if (!(frames & 0x03))
				--y;
			return;
		}
		y -= 3;
		if (y < 0x18) {
			mem_.cur_scene_obj_state() = 0;
			mem_.unk_2d891() = 0; // Func_ae1ce: the next step
			++mem_.unk_2d890();
			return;
		}
		if (y >= 0x40 || mem_.cur_scene_obj_state() == 0x12)
			return;
		set_state(wCurSceneObjState, 0x12);
		set_state(wSceneObj9State, 0x0a);
		return;
	}
	case 7: // .Func_af73a
		if (frames >= 0x2d)
			end_sub();
		return;
	default:
		unrealistic();
		return;
	}
}

// .Func_af436: Wario is thrown out of the castle (twice, bouncing), takes
// the treasure and walks off
void Epilogue::thrown_out()
{
	bounce_treasure(); // .Func_af60d
	bounce_wario(); // .Func_af57b
	uint8_t s = mem_[SUB_STEP];
	uint8_t frames = ++mem_[SUB_FRAMES];
	auto after = [&](uint8_t wait, uint8_t state) {
		if (frames < wait)
			return;
		wario_state(state);
		next_sub();
	};
	auto treasure_at = [this](uint8_t dy, uint8_t dx) { // .asm_af50a
		mem_.scene_obj9() = static_cast<uint8_t>(mem_.owui_obj1_y_coord() + dy);
		mem_[wSceneObj9 + 1] = static_cast<uint8_t>(mem_.owui_obj1_x_coord() + dx);
	};
	switch (s) {
	case 0: // .Func_af466
		mem_.pal_fade2_colour2_green_unk1() = 0;
		mem_.unk_2d892() = 0;
		mem_.temple_rock5_action() = 0;
		mem_.pal_fade2_colour4_green_sign() = 0;
		next_sub();
		return;
	case 1: // .Func_af476: Wario thrown
		if (frames < 0x40)
			return;
		mem_.unk_2d892() = 0x01;
		play_sfx(mem_, SFX_THROW);
		next_sub();
		return;
	case 2: // .Func_af48a: and the treasure
		if (frames < 0x56)
			return;
		mem_.temple_rock5_action() = 0x01;
		play_sfx(mem_, SFX_THROW);
		next_sub();
		return;
	case 3: // .Func_af49e
		if (mem_.unk_2d892() == 0x80 && mem_.temple_rock5_action() == 0x80)
			next_sub();
		return;
	case 4: after(0x3c, 0x05); return; // .Func_af4ad
	case 5: wait_wario(0x02); return; // .Func_af4b9
	case 6: after(0x28, 0x14); return; // .Func_af4be
	case 7: wait_wario(0x0c); return; // .Func_af4ca
	case 8: // .Func_af4cf: he grabs it
		if (frames < 0x2d)
			return;
		wario_state(0x0b);
		mem_.scene_obj9_unk7() = 0;
		play_sfx(mem_, SFX_GRAB);
		next_sub();
		return;
	case 9: // .Func_af4e7
		if (mem_.scene_obj9_unk7() == 0x80 && mem_.owui_obj1_state() == 0x0a)
			next_sub();
		return;
	case 10: // .Func_af4f6: lifting it
		if (frames == 0x04) {
			treasure_at(0xdf, 0xfa);
		} else if (frames == 0x06) {
			treasure_at(0xe0, 0xfb);
		} else if (frames == 0x07) {
			treasure_at(0xe2, 0xfc);
			next_sub();
		}
		return;
	case 11: after(0x32, 0x21); return; // .Func_af528
	case 12: // .Func_af534
		if (mem_.owui_obj1_frame() != 0x0b)
			return;
		play_sfx(mem_, SFX_106);
		next_sub();
		return;
	case 13: wait_wario(0x0a); return; // .Func_af545
	case 14: // .Func_af54a
		if (frames < 0x32)
			return;
		wario_state(0x09);
		mem_.scene_obj9_unk7() = 0;
		next_sub();
		return;
	case 15: { // .Func_af55a: off left, carrying it
		--mem_.owui_obj1_x_coord(); // Func_aca63
		uint8_t x = mem_.owui_obj1_x_coord();
		mem_.scene_obj9_x_coord() = static_cast<uint8_t>(x - 4);
		if (x != 0xf0)
			return;
		mem_.owui_obj1_state() = 0;
		mem_.scene_obj9_state() = 0;
		next_sub();
		return;
	}
	case 16: // .Func_af574
		if (frames >= 0x3c)
			end_sub();
		return;
	default:
		unrealistic();
		return;
	}
}

// .Func_af57b: Wario's flight (Data_aed06, every frame of 4 but the first)
// and his bounces (w2d892's steps)
void Epilogue::bounce_wario()
{
	if (mem_.pal_fade2_colour2_green_unk1() && (mem_[SUB_FRAMES] & 0x03)) {
		if (home::apply_ow_movement(gb_, wOWUIObj1YCoord, ADDR_Data_aed06, true, mem_.code_bank()).finished)
			mem_.pal_fade2_colour2_green_unk1() = 0;
	}
	Byte action = mem_.unk_2d892();
	if (action == 0x80)
		return;
	auto next = [&] { // .asm_af5bc
		++action;
		mem_.unk_2d893() = 0;
	};
	switch (action) {
	case 0: return;
	case 1: // .Func_af5b3
		mem_.pal_fade2_colour2_green_unk1() = 0x01;
		mem_.scene_wario_unk7() = 0;
		next();
		return;
	case 2: case 4: case 6: // .Func_af5c5: landing
		if (mem_.owui_obj1_y_coord() != 0x60)
			return;
		wario_state(0x1b);
		play_sfx(mem_, SFX_BUMP);
		next();
		return;
	case 3: case 5: // .Func_af5d5
		if (++mem_.unk_2d893() >= 4)
			next();
		return;
	case 7: // .Func_af5df
		if (mem_.owui_obj1_state() == 0x1c)
			next();
		return;
	case 8: // .Func_af5e7
		if (++mem_.unk_2d893() < 0x42)
			return;
		wario_state(0x0c);
		play_sfx(mem_, SFX_12A);
		next();
		return;
	case 9: action = 0x80; return; // .Func_af5fe
	default: unrealistic(); return;
	}
}

// .Func_af60d: the treasure's flight and bounces
void Epilogue::bounce_treasure()
{
	if (mem_.pal_fade2_colour4_green_sign()) {
		if (home::apply_ow_movement(gb_, wSceneObj9, ADDR_Data_aed06, true, mem_.code_bank()).finished)
			mem_.pal_fade2_colour4_green_sign() = 0;
	}
	Byte action = mem_.temple_rock5_action();
	if (action == 0x80)
		return;
	auto next = [&] { // .asm_af643
		++action;
		mem_.temple_rock5_counter() = 0;
	};
	switch (action) {
	case 0: return;
	case 1: // .Func_af63a
		mem_.pal_fade2_colour4_green_sign() = 0x01;
		mem_.scene_obj9_unk7() = 0;
		next();
		return;
	case 2: case 4: case 6: // .Func_af64c
		if (mem_.scene_obj9_y_coord() != 0x60)
			return;
		play_sfx(mem_, SFX_BUMP);
		next();
		return;
	case 3: case 5: // .Func_af657
		if (++mem_.temple_rock5_counter() >= 4)
			next();
		return;
	case 7: action = 0x80; return; // .Func_af661
	default: unrealistic(); return;
	}
}

} // namespace wl3::overworld

// Farcalled by the map's sub-state table (C++).
PORT_EX(_InitEpilogue, 0, ALL_REGISTERS) { wl3::overworld::Epilogue(gb).init(); }
PORT_EX(Func_af13c, 0, ALL_REGISTERS) { wl3::overworld::Epilogue(gb).update(); }

PORT_COVERED(Func_ae1ce); // done inline
