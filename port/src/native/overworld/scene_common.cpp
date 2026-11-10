// bank2b's helpers shared by its scenes: the text box, scene Wario's
// walking, the scenes' RAM and palette configs. See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint8_t NO_CHAR = 0x7f; // '@'
constexpr uint8_t SPACE = 0x7e; // ' '
constexpr uint8_t ARROW = 0x5d; // '▼'
constexpr uint8_t TEXT_DONE = 0x80; // wCurTextLine once the page is printed
constexpr uint16_t SCENE_RAM = 0xd800; // WRAM2's second half
constexpr uint16_t SCENE_RAM_SIZE = 0x0800;
constexpr uint16_t VBLANK_FUNC = ADDR_VBlank_PrologueEpilogue + 0x0c; // VBlank_PrologueEpilogue.Func
constexpr uint8_t VBLANK_FUNC_SIZE = 0x7e;

} // namespace

void SceneCommon::fill(uint16_t addr, uint16_t size, uint8_t value)
{
	for (uint16_t i = 0; i < size; i++)
		mem_[static_cast<uint16_t>(addr + i)] = value;
}

void SceneCommon::set_state(uint16_t state, uint8_t value)
{
	home::set_scene_obj_state(gb_, state, value);
}

// Func_aca6d
void SceneCommon::next_step()
{
	mem_.unk_2d014() = 0;
	++mem_.unk_2d013();
}

// ApplySceneWarioMovementRight: 1 pixel, and another every other frame
// or while right is held. ApplySceneWarioMovementLeft: the same leftwards.
uint8_t SceneCommon::walk(bool right)
{
	uint8_t pad = static_cast<uint8_t>(mem_.joypad_down() << 4 | mem_.joypad_down() >> 4);
	if (!right)
		pad >>= 1; // d-left
	int8_t step = right ? 1 : -1;
	Byte x = mem_.scene_wario_x_coord();
	x = static_cast<uint8_t>(x + step);
	if ((mem_.global_counter() | pad) & 1)
		x = static_cast<uint8_t>(x + step);
	return x & 0xfe;
}

void SceneCommon::clear_scene_ram() { fill(SCENE_RAM, SCENE_RAM_SIZE, 0); }
void SceneCommon::clear_scene_objs() { fill(wSceneWario, wSceneObjsEnd - wSceneWario, 0); }
void SceneCommon::set_vblank() { routines_.copy(VBLANK_FUNC, wVBlankFunc, VBLANK_FUNC_SIZE); }

// Func_ac9e9 (wPalConfig1), Func_ac9e4 (wPalConfig2): the config from data
void SceneCommon::set_pal_config(uint16_t config, uint16_t data)
{
	for (uint16_t i = 0; i < 5; i++)
		mem_[static_cast<uint16_t>(config + i)] = mem_[static_cast<uint16_t>(data + i)];
}

void SceneCommon::copy_temp_pals()
{
	routines_.copy(wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
}

void SceneCommon::white_pals(uint16_t pals)
{
	for (uint16_t i = 0; i < 8 * PAL_SIZE; i += 2)
		mem_.word(static_cast<uint16_t>(pals + i)) = 0x7fff; // PALRGB_WHITE
}

void SceneCommon::fade()
{
	routines_.farcall(BANK_Func_84e43, ADDR_Func_84e43);
	if (!mem_.pal_config1_total_steps())
		next_step();
}

void SceneCommon::prologue_fade() { routines_.farcall(BANK__PrologueFade, ADDR__PrologueFade); }

void SceneCommon::temple_shake()
{
	routines_.farcall(BANK__HandleTempleShakeAndRocks, ADDR__HandleTempleShakeAndRocks);
}

// The text

void SceneCommon::fill_cleared_text_buffer() { fill(wClearedTextBuffer, 8 * TEXT_LINE_LENGTH, NO_CHAR); }

uint16_t SceneCommon::buffer_page(uint8_t page)
{
	uint8_t swapped = static_cast<uint8_t>(page << 4 | page >> 4); // as the asm, for any page
	return static_cast<uint16_t>(wTextBuffer + (swapped << 2));
}

// InitTextPrinting: the header line, and the arrow object
void SceneCommon::init_text()
{
	fill(wText, 4 * TEXT_LINE_LENGTH, NO_CHAR); // ClearText
	routines_.copy(wTextBuffer, wTextLine1, TEXT_LINE_LENGTH);
	fill(wTextPrintInfo, wTextPrintInfoEnd - wTextPrintInfo, 0);
	mem_[wSceneObj1 + 0] = 0x8e;
	mem_[wSceneObj1 + 1] = 0x98;
	set_state(wSceneObj1State, 0);
	next_step();
}

// LoadCurTextPageBufferPage: the page's 3 lines below the header
void SceneCommon::load_page()
{
	routines_.copy(buffer_page(mem_.cur_text_buffer_page()), wTextLine2, 3 * TEXT_LINE_LENGTH);
	mem_.cur_text_line() = 0;
	next_step();
}

// Func_acaf3: the next page; one starting with '▼' ends the text, else the
// arrow shows and the scene goes back 3 steps to print it
bool SceneCommon::next_page()
{
	uint8_t page = ++mem_.cur_text_buffer_page();
	if (mem_[buffer_page(page)] == ARROW) {
		next_step();
		return true;
	}
	set_state(wSceneObj1State, 0x01);
	mem_.unk_2d013() = static_cast<uint8_t>(mem_.unk_2d013() - 3);
	mem_.unk_2d014() = 0;
	return false;
}

void SceneCommon::print_page()
{
	print_with_header();
	if (mem_.cur_text_line() != TEXT_DONE)
		return;
	++mem_.cur_text_buffer_page();
	next_step();
}

void SceneCommon::print_page_without_header()
{
	print_without_header();
	if (mem_.cur_text_line() == TEXT_DONE)
		next_step();
}

// Func_acacd: past the first page, waits for A (and hides the arrow)
void SceneCommon::wait_for_a()
{
	if (mem_.cur_text_buffer_page() != 1) {
		if (!(mem_.joypad_pressed() & PAD_A))
			return;
		set_state(wSceneObj1State, 0);
	}
	clear_textbox_except_header();
	next_step();
}

// PrintTextWithHeader (lines 1 to 4), PrintTextWithoutHeader (2 to 4):
// wCurTextLine 0 is a pause before the first line
void SceneCommon::print_lines(bool header)
{
	++mem_.text_delay_counter();
	uint8_t line = mem_.cur_text_line();
	if (!line) { // DelayTextThenGoToNextLine
		if (mem_.text_delay_counter() >= 10)
			next_line();
		return;
	}
	uint8_t lines = header ? 4 : 3;
	if (line > lines) {
		mem_.cur_text_line() = TEXT_DONE;
		return;
	}
	print(header ? line - 1 : line);
}

void SceneCommon::next_line()
{
	mem_.cur_text_line_pos() = 0;
	mem_.text_delay_counter() = 0;
	++mem_.cur_text_line();
}

// PrintText: the line's next character (skipping blanks) to
// wPendingCharDest, every 4 frames (2 with A or down held)
void SceneCommon::print(uint8_t line)
{
	if (mem_[wPendingCharDest + 0])
		return;
	uint8_t delay = (mem_.joypad_down() & (PAD_A | PAD_DOWN)) ? 2 : 4;
	if (mem_.text_delay_counter() < delay)
		return;
	mem_.text_delay_counter() = 0;
	mem_.cur_text_buffer_line() = line;
	uint8_t row = static_cast<uint8_t>(line << 4 | line >> 4); // * TEXT_LINE_LENGTH
	for (;;) {
		uint8_t pos = mem_.cur_text_line_pos();
		uint8_t c = mem_[static_cast<uint16_t>(wText + pos + row)];
		if (c != NO_CHAR) {
			mem_.pending_char_tile() = c;
			mem_.pending_char_attr() = 0x1 | BG_BANK1;
			uint16_t dest = mem_.bg_map_to_print_text() ? 0x9c00 + 14 * 0x20 + 2 : 0x9800 + 14 * 0x20 + 2;
			if (!line) // the header starts 1 tile to the left
				dest = static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest - 1));
			dest = static_cast<uint16_t>(dest + pos + static_cast<uint8_t>(row << 1));
			mem_.word(wPendingCharDest) = dest;
			if (c != SPACE)
				play_sfx(mem_, SFX_130);
			if (++mem_.cur_text_line_pos() == TEXT_LINE_LENGTH)
				next_line();
			return;
		}
		if (++mem_.cur_text_line_pos() == TEXT_LINE_LENGTH) {
			next_line();
			return;
		}
	}
}

// ClearTextboxAtCoord: an HDMA of wClearedTextBuffer to the BG map's
// tiles from dest (its low byte)
void SceneCommon::clear_textbox_at(uint8_t dest, uint8_t tiles)
{
	uint8_t dest_hi = mem_.bg_map_to_print_text() ? ((v0BGMap1 + 0x100) >> 8) - 0x80 : ((v0BGMap0 + 0x100) >> 8) - 0x80;
	mem_[wHDMA + 0] = wClearedTextBuffer >> 8;
	mem_[wHDMA + 1] = wClearedTextBuffer & 0xff;
	mem_[wHDMA + 2] = dest_hi;
	mem_[wHDMA + 3] = dest;
	mem_[wHDMA + 4] = tiles;
	mem_.hdma_bank() = BANK_ClearTextboxAtCoord;
	mem_.hdma_dest_vram_bank() = 0;
	mem_[wPendingCharDest + 0] = 0;
}

// LoadNextObjectiveText: the header, and the next treasure's level's
// name and description (from the text in wTextBuffer, by side and level)
void SceneCommon::load_next_objective()
{
	routines_.copy(wTextBuffer, wTextLine1, TEXT_LINE_LENGTH);
	mem_.is_next_objective_the_temple() = 0;
	Events::SideLevel at{};
	routines_.in_bank(BANK_GetMapSideAndLevelForNextTreasure, ADDR_GetMapSideAndLevelForNextTreasure,
		[&] { at = Events(gb_).next_treasure_side_level(); });
	uint8_t side = at.side, level = at.level;
	uint16_t offset;
	switch (side) {
	case 1: offset = 0x120; break; // west
	case 2: offset = 0x200; break; // south
	case 0: offset = 0x20; break; // north
	default: offset = 0x2e0; break; // east (anything else: debug_nop, then the temple's check)
	}
	if ((side == 0 || side > 3) && !level)
		mem_.is_next_objective_the_temple() = TRUE;
	uint8_t swapped = static_cast<uint8_t>(level << 4 | level >> 4);
	uint8_t row = static_cast<uint8_t>(swapped << 1 | swapped >> 7); // * $20
	uint16_t text = static_cast<uint16_t>(wTextBuffer + offset + row);
	if (mem_.language()) {
		routines_.copy(text, wTextLine3, 2 * TEXT_LINE_LENGTH);
		routines_.copy(wTextBuffer + TEXT_LINE_LENGTH, wTextLine2, TEXT_LINE_LENGTH);
		return;
	}
	routines_.copy(text, wTextLine2, TEXT_LINE_LENGTH);
	routines_.copy(wTextBuffer + TEXT_LINE_LENGTH, wTextLine3, TEXT_LINE_LENGTH);
	fill(wTextLine4, TEXT_LINE_LENGTH, NO_CHAR);
}

} // namespace wl3::overworld

using wl3::overworld::SceneCommon;

// Each scene's C++ calls these directly; the hooks are for the fuzzer (and
// _InitPrologue's C, which reads no register after them)
PORT_EX(ApplySceneWarioMovementRight, 0, ALL_REGISTERS) { SceneCommon(gb).walk(true); }
PORT_EX(ApplySceneWarioMovementLeft, 0, ALL_REGISTERS) { SceneCommon(gb).walk(false); }
PORT_EX(ClearGeneralSceneRAM, 0, ALL_REGISTERS) { SceneCommon(gb).clear_scene_ram(); }
PORT_EX(ClearSceneObjsRAM, 0, ALL_REGISTERS) { SceneCommon(gb).clear_scene_objs(); }
PORT_EX(VBlank_PrologueEpilogue, 0, ALL_REGISTERS) { SceneCommon(gb).set_vblank(); }
PORT_EX(Func_ac9e4, 0, ALL_REGISTERS) { SceneCommon(gb).set_pal_config(wPalConfig2, REG_DE(gb)); }
PORT_EX(Func_ac9e9, 0, ALL_REGISTERS) { SceneCommon(gb).set_pal_config(wPalConfig1, REG_DE(gb)); }
PORT_EX(Func_aca09, 0, ALL_REGISTERS) { SceneCommon(gb).copy_temp_pals(); }
PORT_EX(Func_aca20, 0, ALL_REGISTERS) { SceneCommon(gb).white_pals(wTempPals1); } // unreferenced
PORT_EX(Func_aca25, 0, ALL_REGISTERS) { SceneCommon(gb).white_pals(wTempPals2); }
PORT_EX(Func_aca28, 0, ALL_REGISTERS) { SceneCommon(gb).white_pals(REG_HL(gb)); }
PORT_EX(Func_ac9ca, 0, ALL_REGISTERS) { SceneCommon(gb).fade(); }
PORT_EX(PrologueFade, 0, ALL_REGISTERS) { SceneCommon(gb).prologue_fade(); }
PORT_EX(HandleTempleShakeAndRocks, 0, ALL_REGISTERS) { SceneCommon(gb).temple_shake(); }
PORT_EX(FillClearedTextBuffer, 0, ALL_REGISTERS) { SceneCommon(gb).fill_cleared_text_buffer(); }
PORT_EX(InitTextPrinting, 0, ALL_REGISTERS) { SceneCommon(gb).init_text(); }
PORT_EX(LoadCurTextPageBufferPage, 0, ALL_REGISTERS) { SceneCommon(gb).load_page(); }
PORT_EX(Func_acaf3, 0, ALL_REGISTERS) { SceneCommon(gb).next_page(); }
PORT_EX(Func_aca9e, 0, ALL_REGISTERS) { SceneCommon(gb).print_page(); }
PORT_EX(Func_acae7, 0, ALL_REGISTERS) { SceneCommon(gb).print_page_without_header(); }
PORT_EX(Func_acacd, 0, ALL_REGISTERS) { SceneCommon(gb).wait_for_a(); }
PORT_EX(PrintTextWithHeader, 0, ALL_REGISTERS) { SceneCommon(gb).print_with_header(); }
PORT_EX(PrintTextWithoutHeader, 0, ALL_REGISTERS) { SceneCommon(gb).print_without_header(); }
PORT_EX(ClearTextbox, 0, ALL_REGISTERS) { SceneCommon(gb).clear_textbox(); }
PORT_EX(ClearTextboxExceptHeader, 0, ALL_REGISTERS) { SceneCommon(gb).clear_textbox_except_header(); }
PORT_EX(LoadNextObjectiveText, 0, ALL_REGISTERS) { SceneCommon(gb).load_next_objective(); }

// inside SceneCommon
PORT_COVERED(Func_aca6d);
PORT_COVERED(Func_ac9ec);
PORT_COVERED(ClearText);
PORT_COVERED(ClearTextboxAtCoord);
PORT_COVERED(DelayTextThenGoToNextLine);
PORT_COVERED(GoToNextLine);
PORT_COVERED(PrintText);
PORT_COVERED(PrintText_Line1);
PORT_COVERED(PrintText_Line2);
PORT_COVERED(PrintText_Line3);
PORT_COVERED(PrintText_Line4);
