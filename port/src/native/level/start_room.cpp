// Starting a room (src/engine/level/start_room.asm): from the level's start
// or a transition, loading it, placing Wario at its spawn point, setting
// the camera, drawing the whole visible BG map, spawning its objects; and
// processing a multi-block between frames.
//
// They call much that's still C with its registers, and the room drawing's
// queues read the stack (the asm's return addresses, see HANDOFF.md), so
// these keep the original's registers, flags and return addresses.
#include "room_gfx.hpp"
#include "frame.hpp"
#include "../flags.hpp"
#include "../home/sprites.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "object.hpp"
#include "../home/object_pos.hpp"
#include "../home/floor_transition.hpp"
#include "../home/level_map.hpp"

#include <initializer_list>

namespace wl3::level {

void queue_block_tiles_along_row(GB *gb, uint16_t block, bool odd); // scroll.cpp
void queue_block_attributes_along_row(GB *gb, uint16_t block, bool odd);

namespace {

constexpr uint16_t PROCESS_BLOCKS = ADDR_ProcessMultiBlock + 0x63; // .ProcessBlocks
constexpr uint8_t SRAM1 = BANK_sLevelBlockObjectMap;
constexpr uint8_t GFX_RAM = BANK_w3d280;
constexpr uint8_t OAM1 = BANK_OAM_14000; // BANK("Wario OAM 1")

class RoomStart {
public:
	explicit RoomStart(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void from_transition(); // StartRoom_FromTransition
	void from_level_start(); // StartRoom_FromLevelStart
	void spawn_objects(); // Func_896f
	void set_initial_camera_scroll(); // SetInitialCameraScroll
	void apply_initial_camera_scroll(); // ApplyInitialCameraScroll
	void set_wario_position_to_spawn(); // SetWarioPositionToSpawn
	void draw_room(); // DrawRoom_FromStart
	void process_multi_block(); // ProcessMultiBlock
	void process_blocks(); // ProcessMultiBlock.ProcessBlocks

private:
	bool zf() const { return cpu_.f & FLAG_Z; }
	bool cf() const { return cpu_.f & FLAG_C; }
	void sram_switch(uint8_t bank)
	{
		mem_.sram_bank() = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void xor_a()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
	}
	// ld a, v / ld [addr], a
	void st(uint16_t addr, uint8_t v)
	{
		cpu_.a = v;
		mem_[addr] = v;
	}
	void clear(std::initializer_list<uint16_t> addrs)
	{
		xor_a();
		for (uint16_t addr : addrs)
			mem_[addr] = 0;
	}
	// push_wram / pop_wram: WRAM bank, then back
	void push_wram(uint8_t bank)
	{
		saved_wbk_[saved_++] = mem_[rWBK];
		mem_[rWBK] = bank;
	}
	void pop_wram() { mem_[rWBK] = saved_wbk_[--saved_]; }
	// ld a, [wceef] / and %00111100: from a saved level
	bool from_saved_level()
	{
		cpu_.a = mem_.unk_ceef() & 0x3c;
		cpu_.f = flags::and_(cpu_.a);
		return cpu_.a;
	}
	// ld a, [wCameraConfigFlags] / bit n, a
	bool cam_bit(int n)
	{
		cpu_.a = mem_.camera_config_flags();
		cpu_.f = flags::bit(cpu_.f, cpu_.a, n);
		return cpu_.a & (1 << n);
	}
	// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp v
	void cam_cp(uint8_t v)
	{
		cpu_.a = mem_.camera_config_flags() & CAM_SCROLLING_MASK;
		cpu_.f = flags::cp(cpu_.a, v);
	}
	void ld_cp(uint16_t addr, uint8_t v)
	{
		cpu_.a = mem_[addr];
		cpu_.f = flags::cp(cpu_.a, v);
	}
	void add_hl(uint16_t v)
	{
		uint16_t hl = REG_HL(gb_);
		unsigned sum = hl + v;
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((hl & 0xfff) + (v & 0xfff) > 0xfff ? FLAG_H : 0)
			| (sum > 0xffff ? FLAG_C : 0));
		SET_HL(gb_, sum);
	}
	void spawn_if_in_range(uint8_t index, home::ObjMapBlock block);
	home::ObjMapBlock obj_map_at_hpos();
	void next_obj_map_row(home::ObjMapBlock &block);
	uint16_t scroll_minus(uint16_t scroll, uint16_t pos, uint8_t offset, int border);
	bool cap_low(uint16_t scroll, uint16_t v, uint16_t limit, uint16_t limit_flag);
	bool cap_high(uint16_t scroll, uint8_t span, uint8_t wide_span, int border, uint16_t limit, uint16_t limit_flag);
	void set_scroll(uint16_t scroll, uint16_t reg, uint16_t wreg, uint8_t v, bool store);
	// big-endian words (positions, scrolls)
	uint16_t be(uint16_t addr) const { return static_cast<uint16_t>(mem_[addr] << 8 | mem_[addr + 1]); }
	void set_be(uint16_t addr, uint16_t v)
	{
		mem_[addr] = static_cast<uint8_t>(v >> 8);
		mem_[addr + 1] = static_cast<uint8_t>(v);
	}
	bool cam_border(int bit) const { return mem_.camera_config_flags() & (1 << bit); }
	void draw_room_column(uint16_t queue);
	void queue_row_tiles_and_attributes();
	void call_in_sram1(uint16_t addr);
	void farcall_obj_wram(uint8_t bank, uint16_t addr);

	uint8_t saved_wbk_[4] = {};
	int saved_ = 0;
	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// spawns the object (index) at the object map block if the block's in the
// spawn range (wc0c4-wc0c7: rows and columns, the rows less $20 past the
// first SRAM bank)
void RoomStart::spawn_if_in_range(uint8_t index, home::ObjMapBlock block)
{
	if (index >= 0x0f)
		return; // Wario's spawn block
	if (static_cast<uint8_t>(mem_.unk_c0c7() - 1) < block.col || mem_.unk_c0c6() > block.col)
		return;
	uint8_t sub = mem_.unk_ccec() == 1 ? 0 : 0x20;
	if (static_cast<uint8_t>(mem_.unk_c0c4() - sub - 1) < block.row || static_cast<uint8_t>(mem_.unk_c0c5() - sub) > block.row)
		return;

	// .spawn
	home::obj_map_block_to_hpos(gb_, block);
	uint16_t at = static_cast<uint16_t>(block.row << 8 | home::level_col(block.col, block.parity));
	push_wram(BANK_wObjects); // BANK("Level Objects WRAM")
	routines_.in_bank(BANK_SpawnObject, ADDR_SpawnObject, [&] { spawn_object(gb_, index, at); });
	pop_wram();
}

// spawns the objects in the 16x16 blocks of the object map around
// wBlockXPos / wObjectSpawnBlockY
void RoomStart::spawn_objects()
{
	for (int i = 0; i < 4; i++)
		mem_[hXPosLo - i] = mem_[wBlockXPos + 1 - i];
	home::ObjMapBlock block = obj_map_at_hpos();
	mem_.unused_object_parity() = 1;
	block.parity = 1;
	for (int rows = 0x10; rows; rows--) {
		for (int pairs = 16 / 2; pairs; pairs--) {
			uint16_t at = static_cast<uint16_t>(block.row << 8 | block.col);
			if (uint8_t index = mem_[at] >> 4)
				spawn_if_in_range(index, block);
			block.parity ^= 1;
			if (uint8_t index = mem_[at] & 0x0f)
				spawn_if_in_range(index, block);
			block.col++;
			block.parity ^= 1;
		}
		next_obj_map_row(block);
	}
}

// the object map block at hPos (Func_cc0), its row's SRAM bank (in wccec)
// switched in: $20 rows a bank
home::ObjMapBlock RoomStart::obj_map_at_hpos()
{
	home::ObjMapBlock block = home::obj_map_block(gb_, hYPosHi);
	uint8_t bank = 0x01;
	uint8_t row = mem_.object_spawn_block_y();
	for (int i = 0; i < 2 && row >= 0xc0; i++) {
		bank++;
		row = static_cast<uint8_t>(row - 0x20);
	}
	mem_.unk_ccec() = bank;
	sram_switch(bank);
	block.row = row;
	return block;
}

// back 8 bytes, a row down (on into the next SRAM bank past its end)
void RoomStart::next_obj_map_row(home::ObjMapBlock &block)
{
	block.col = static_cast<uint8_t>(block.col - 0x08);
	if (++block.row == 0xc0) {
		block.row = sLevelBlockObjectMap >> 8;
		mem_.unk_ccec() = static_cast<uint8_t>(mem_.unk_ccec() + 1);
		sram_switch(mem_.unk_ccec());
	}
}

// scroll = pos - offset (big-endian); returns that, less $20 more with the
// border bit (the asm's hl)
uint16_t RoomStart::scroll_minus(uint16_t scroll, uint16_t pos, uint8_t offset, int border)
{
	uint16_t v = static_cast<uint16_t>(be(pos) - offset);
	set_be(scroll, v);
	return cam_border(border) ? static_cast<uint16_t>(v - 0x20) : v;
}

// caps the scroll (at v) to the low limit: true (and the scroll's high
// byte at it) if it's below, or past the top of the map
bool RoomStart::cap_low(uint16_t scroll, uint16_t v, uint16_t limit, uint16_t limit_flag)
{
	uint8_t lim = mem_[limit], hi = static_cast<uint8_t>(v >> 8);
	if (!((hi & 0x80) || hi < lim || (hi == lim && !(v & 0xff))))
		return false;
	mem_[scroll + 0] = lim;
	mem_[limit_flag] = 1;
	return true;
}

// caps scroll + span (wide_span with the border) to the high limit: true
// if it's at it
bool RoomStart::cap_high(uint16_t scroll, uint8_t span, uint8_t wide_span, int border, uint16_t limit, uint16_t limit_flag)
{
	uint16_t v = static_cast<uint16_t>(be(scroll) + (cam_border(border) ? wide_span : span));
	uint8_t lim = mem_[limit];
	if ((v >> 8) < lim)
		return false;
	mem_[scroll + 0] = static_cast<uint8_t>(lim - 1);
	mem_[limit_flag] = 0xff;
	return true;
}

// the camera scroll for a new room or the level's start
void RoomStart::set_initial_camera_scroll()
{
	mem_.camera_room_x_scroll_limit() = 0;
	uint8_t mode = mem_.camera_config_flags() & CAM_SCROLLING_MASK;
	if (mode == (CAM_XSCROLL2 | CAM_TRANSITIONS) || mode == CAM_YSCROLL) { // the x scroll at Wario's
		mem_[wCameraSCX + 0] = mem_.wario_x().hi();
		mem_[wCameraSCX + 1] = 0x30;
	} else {
		uint8_t spacing = static_cast<uint8_t>(mem_[mem_.direction() ? wCamRightSpacing : wCamLeftSpacing] - 8);
		uint16_t v = scroll_minus(wCameraSCX, wWarioXPos, spacing, CAM_BORDER_LEFT_F);
		if (!cap_low(wCameraSCX, v, wRoomLeftXLimit, wCameraRoomXScrollLimit))
			cap_high(wCameraSCX, sLevelBlockObjectMap >> 8, 0xc0, CAM_BORDER_RIGHT_F, wRoomRightXLimit,
				wCameraRoomXScrollLimit);
	}

	// .check_if_saved_level: no y scroll to handle
	if ((mem_.unk_ceef() & 0b00111100) && mode >= CAM_TRANSITIONS)
		return;

	// .handle_cam_y_scroll
	mem_.camera_room_y_scroll_limit() = 0;
	uint16_t v = scroll_minus(wCameraSCY, wWarioYPos, 0x60, CAM_BORDER_UP_F);
	if (cap_low(wCameraSCY, v, wRoomUpperYLimit, wCameraRoomYScrollLimit))
		return;
	if (cap_high(wCameraSCY, 0x90, 0xb0, CAM_BORDER_DOWN_F, wRoomLowerYLimit, wCameraRoomYScrollLimit))
		return;

	// .check_transitions: the floor Wario's on
	if (mode < CAM_TRANSITIONS)
		return;
	if (mem_.wario_y().lo() >= 0x80) {
		mem_[wCameraSCY + 1] = 0x68;
		return;
	}
	uint8_t upper = mem_.room_upper_y_limit();
	uint8_t hi = static_cast<uint8_t>(mem_.wario_y().hi() - 1);
	mem_[wCameraSCY + 0] = hi;
	if (hi < upper) { // .cap_up_side
		mem_[wCameraSCY + 0] = upper;
		mem_.camera_room_y_scroll_limit() = 1;
		return;
	}
	if (!(hi & 0x80)) {
		mem_[wCameraSCY + 1] = 0xe8;
		return;
	}
	mem_[wCameraSCY + 0] = 0;
	mem_.camera_room_y_scroll_limit() = 1;
}

// the scroll's low byte to rSC* and wSC* (and to [scroll + 1], if store)
void RoomStart::set_scroll(uint16_t scroll, uint16_t reg, uint16_t wreg, uint8_t v, bool store)
{
	if (store)
		mem_[scroll + 1] = v;
	mem_[reg] = v;
	mem_[wreg] = v;
}

// the camera scroll from SetInitialCameraScroll to rSCY / rSCX, and
// wBlockPos at the BG map's top left
void RoomStart::apply_initial_camera_scroll()
{
	uint16_t scy = be(wCameraSCY);
	uint8_t limit = mem_.camera_room_y_scroll_limit();
	if (limit == 1) { // .on_upper_edge
		uint8_t v = cam_border(CAM_BORDER_UP_F) ? 0x20 : 0x00;
		set_scroll(wCameraSCY, rSCY, wSCY, v, true);
		set_be(wBlockYPos, static_cast<uint16_t>((scy & 0xff00) | v));
	} else if (limit == 0xff) { // .on_lower_edge
		uint8_t v = cam_border(CAM_BORDER_DOWN_F) ? 0x48 : 0x68;
		set_scroll(wCameraSCY, rSCY, wSCY, v, true);
		set_be(wBlockYPos, static_cast<uint16_t>((scy & 0xff00) | static_cast<uint8_t>(v - 0x20)));
	} else {
		set_scroll(wCameraSCY, rSCY, wSCY, static_cast<uint8_t>(scy), false);
		set_be(wBlockYPos, static_cast<uint16_t>(scy - 0x18));
	}

	// .cam_x_scroll
	uint16_t scx = be(wCameraSCX);
	limit = mem_.camera_room_x_scroll_limit();
	if (limit == 1) { // .on_left_edge
		uint8_t v = cam_border(CAM_BORDER_LEFT_F) ? 0x20 : 0x00;
		set_scroll(wCameraSCX, rSCX, wSCX, v, true);
		set_be(wBlockXPos, static_cast<uint16_t>((scx & 0xff00) | v));
		return;
	}
	if (limit == 0xff) { // .on_right_edge
		uint8_t v = cam_border(CAM_BORDER_RIGHT_F) ? 0x40 : 0x60;
		set_scroll(wCameraSCX, rSCX, wSCX, v, true);
		set_be(wBlockXPos, static_cast<uint16_t>((scx & 0xff00) | static_cast<uint8_t>(v - 0x28)));
		return;
	}
	set_scroll(wCameraSCX, rSCX, wSCX, static_cast<uint8_t>(scx), false);
	uint16_t x = static_cast<uint16_t>(scx - 0x10);
	set_be(wBlockXPos, (x & 0x8000) ? 0 : x);
}

// hPos (and wWarioPos) at the block of wWarioSpawnPos, then exactly at the
// spawn block ($f) found in the object map there
void RoomStart::set_wario_position_to_spawn()
{
	uint8_t spawn = mem_.wario_spawn_pos();
	mem_.x_pos_lo() = 0;
	mem_.x_pos_hi() = spawn & 0x0f;
	mem_.y_pos_lo() = 0;
	mem_.y_pos_hi() = spawn >> 4;
	home::ObjMapBlock block = obj_map_at_hpos();
	// the spawn block's nybble ($f), on from the parity Func_cc0 gave
	for (;; next_obj_map_row(block)) {
		bool found = false;
		for (int pairs = 16 / 2; pairs && !found; pairs--) {
			uint16_t at = static_cast<uint16_t>(block.row << 8 | block.col);
			if ((mem_[at] >> 4) >= 0x0f) {
				found = true;
				break;
			}
			block.parity ^= 1;
			if ((mem_[at] & 0x0f) >= 0x0f) {
				found = true;
				break;
			}
			block.col++;
			block.parity ^= 1;
		}
		if (found)
			break;
	}

	// .found_spawn_block
	home::obj_map_block_to_hpos(gb_, block);
	for (int i = 0; i < 4; i++) // update_pos
		mem_[wWarioPos + i] = mem_[hPos + i];
	if ((mem_.camera_config_flags() & CAM_SCROLLING_MASK) < CAM_TRANSITIONS || (mem_.unk_ceef() & 0x3c))
		return;
	home::update_floor(gb_);
}

// the queue down a BG map column (wrapping from v0BGMap1 back to v0BGMap0),
// TILEMAP_HEIGHT tiles: ld a, [hli] / ld [de], a
void RoomStart::draw_room_column(uint16_t queue)
{
	uint16_t dest = be(wBGPtr);
	for (int i = 0; i < TILEMAP_HEIGHT; i++) {
		mem_[dest] = mem_[queue++];
		dest = static_cast<uint16_t>(dest + TILEMAP_WIDTH);
		if ((dest >> 8) == (v0BGMap1 >> 8))
			dest = static_cast<uint16_t>((v0BGMap0 & 0xff00) | (dest & 0xff));
	}
}

// DrawRoom_FromStart.QueueRowTilesAndAttributes: the BG map addresses down
// the column, and the tiles and attributes of its blocks
void RoomStart::queue_row_tiles_and_attributes()
{
	// the BG map addresses down the column (from GetBlockBGPtr's hl; the
	// queue's index wraps in its page, as the asm's inc e)
	uint16_t bg = level::block_bg_ptr(gb_);
	uint8_t at = static_cast<uint8_t>(wBGMapAddressQueue + mem_.bg_map_address_queue_size());
	for (int i = 0; i < TILEMAP_HEIGHT; i++) {
		mem_[static_cast<uint16_t>((wBGMapAddressQueue & 0xff00) | at++)] = static_cast<uint8_t>(bg >> 8);
		mem_[static_cast<uint16_t>((wBGMapAddressQueue & 0xff00) | at++)] = static_cast<uint8_t>(bg);
		bg = static_cast<uint16_t>((bg + TILEMAP_WIDTH) & 0xfbff); // wrapping in the BG map
	}
	mem_.bg_map_address_queue_size() = static_cast<uint8_t>(mem_.bg_map_address_queue_size() + 2 * TILEMAP_HEIGHT);

	// the tiles and attributes of the column's blocks, from its block pointer
	uint16_t block = home::block_ptr(gb_, wBlockPos).ptr;
	sram_switch(mem_.block_ptr_bank());
	bool odd = mem_.block_x().lo() & 0x08;
	routines_.in_bank(BANK_QueueBlockTilesAlongRow_OddColumn,
		odd ? ADDR_QueueBlockTilesAlongRow_OddColumn : ADDR_QueueBlockTilesAlongRow_EvenColumn,
		[&] { queue_block_tiles_along_row(gb_, block, odd); });
	sram_switch(mem_.block_ptr_bank());
	routines_.in_bank(BANK_QueueBlockTilesAlongRow_OddColumn,
		odd ? ADDR_QueueBlockAttributesAlongRow_OddColumn : ADDR_QueueBlockAttributesAlongRow_EvenColumn,
		[&] { queue_block_attributes_along_row(gb_, block, odd); });
	mem_.bg_map_tile_queue_size() = static_cast<uint8_t>(mem_.bg_map_tile_queue_size() + TILEMAP_WIDTH);
}

// draws all of the room that the BG map shows, column by column
void RoomStart::draw_room()
{
	for (int i = 0; i < 4; i++)
		mem_[wBackupBlockPos + i] = mem_[wBlockPos + i];
	mem_.draw_room_width_loop_var() = TILEMAP_WIDTH;
	do {
		queue_row_tiles_and_attributes();
		draw_room_column(wBGMapTileVRAM0Queue);
		mem_[rVBK] = 1; // BANK("VRAM1")
		draw_room_column(wBGMapTileVRAM1Queue);
		mem_[rVBK] = 0;
		mem_.bg_map_address_queue_size() = 0;
		mem_.bg_map_tile_queue_size() = 0;
		set_be(wBlockXPos, static_cast<uint16_t>(be(wBlockXPos) + 0x08));
		mem_.draw_room_width_loop_var() = static_cast<uint8_t>(mem_.draw_room_width_loop_var() - 1);
	} while (mem_.draw_room_width_loop_var());
	for (int i = 0; i < 4; i++)
		mem_[wBlockYPos + i] = mem_[wBackupBlockPos + i];
}

// push_wram BANK("Level Objects WRAM") / farcall / pop_wram
void RoomStart::farcall_obj_wram(uint8_t bank, uint16_t addr)
{
	push_wram(0x01);
	routines_.farcall(bank, addr);
	pop_wram();
}

// a multi-block's blocks, then a frame's drawing
void RoomStart::process_multi_block()
{
	routines_.call(PROCESS_BLOCKS);
	routines_.in_bank(BANK_UpdateParticles, ADDR_UpdateParticles, [&] { level::update_particles(gb_); });
	farcall_obj_wram(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority);
	home::draw_wario(gb_);
	farcall_obj_wram(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority);
	home::clear_unused_virtual_oam(gb_);
	uint8_t sram = mem_.sram_bank();
	sram_switch(SRAM1);
	cpu_.ime = false; // di
	level::set_tile_queue_processing_functions(gb_);
	cpu_.ime = true; // ei
	cpu_.ime_delay = 0;
	sram_switch(sram);
}

// the block next to wcedb:wcedc in the direction in wMultiBlockParam's low
// 3 bits, until ProcessBlock leaves none
void RoomStart::process_blocks()
{
	uint8_t param = static_cast<uint8_t>((mem_.multi_block_param() & 0x0f) + 0x10);
	mem_.multi_block_param() = param;
	mem_.unk_cede() = param & 0x08;

	for (;;) {
		// the next block from wcedb:wcedc (the map's rows run on into the
		// next SRAM bank)
		uint8_t bank = mem_.unk_cedb(), row = mem_[wcedc + 0], column = mem_[wcedc + 1];
		switch (mem_.multi_block_param() & 0x07) {
		case 1: // the block on the right
			column++;
			break;
		case 2: // the block on the left
			column--;
			break;
		case 3: // the block below
			if (--row == 0x9f) { // HIGH(STARTOF(SRAM)) - 1
				row = 0xbf; // HIGH(STARTOF(SRAM) + SIZEOF(SRAM)) - 1
				bank--;
			}
			break;
		default: // the block above
			if (++row == 0xc0) { // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
				row = sLevelBlockObjectMap >> 8;
				bank++;
			}
			break;
		}
		mem_.block_ptr_bank() = bank;
		mem_.block_pointer().hi() = row;
		mem_.block_pointer().lo() = column;

		// .process_block
		mem_.multi_block_param() = mem_.multi_block_param() & (0xff ^ 0x07);
		routines_.farcall(BANK_ProcessBlock, ADDR_ProcessBlock);
		if (!(mem_.multi_block_param() & 0x07))
			break;
		level::queue_changed_block(gb_);
	}

	// .break
	mem_.multi_block_param() = 0;
	mem_.is_floor_transition() = 0;
	mem_.sub_state() = mem_.pending_sub_state();
}

// ld a, [wSRAMBank] / push af / ld a, BANK("SRAM1") / sramswitch /
// call addr / pop af / sramswitch
void RoomStart::call_in_sram1(uint16_t addr)
{
	uint8_t sram = mem_.sram_bank();
	sram_switch(SRAM1);
	routines_.call(addr);
	sram_switch(sram);
}

// a room from a transition (or the golf minigame)
void RoomStart::from_transition()
{
	if ((mem_.room_transition_param() & ROOMTRANSITION_MASK) == ROOMTRANSITION_GOLF) { // the golf minigame
		push_wram(0x03);
		mem_.temp_animated_tiles_frame_duration() = mem_.animated_tiles_frame_duration();
		mem_.temp_animated_tiles_group() = mem_.animated_tiles_gfx();
		pop_wram();
		mem_.pending_sub_state() = mem_.sub_state();
		mem_.state() = ST_GOLF;
		mem_.sub_state() = 0;
		return;
	}

	// .not_golf
	routines_.call(ADDR_DisableLCD);
	routines_.farcall(BANK_DespawnAllObjects, ADDR_DespawnAllObjects);
	push_wram(BANK_wObjects); // BANK("Level Objects WRAM")
	routines_.farcall(BANK_InitObjectVars_FromTransition, ADDR_InitObjectVars_FromTransition);
	pop_wram();

	sram_switch(0x00);
	if ((mem_.room_transition_param() & ROOMTRANSITION_MASK) == ROOMTRANSITION_TELEPORT) {
		mem_.wario_y().hi() = 0x01;
		mem_.wario_x().hi() = 0;
	}
	// .not_teleport: up a block
	set_be(wWarioYPos, static_cast<uint16_t>(be(wWarioYPos) - 0x10));
	uint8_t y_hi = mem_.wario_y().hi();
	mem_.unused_ca6c() = static_cast<uint8_t>(mem_.wario_x().hi() | (y_hi << 4 | y_hi >> 4));

	// the next room's ID from Wario's position
	mem_.level_room_id() = static_cast<uint8_t>(mem_[wWarioPos + 2] + 10 * mem_.wario_pos()); // GetNextLevelRoomID

	clear({wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta, wCameraXDelta, wScrollUpEdge,
		wScrollDownEdge, wGroundShakeCounter, wSCYShake});
	mem_.temp_level_room_id() = mem_.level_room_id();

	push_wram(GFX_RAM);
	routines_.call(ADDR_LoadRoom);
	pop_wram();

	home::fill_bg_map0(gb_, 0x7f);
	routines_.call(ADDR_ClearVirtualOAM);
	routines_.call(ADDR_ClearParticles);
	routines_.call_in(BANK_SetWarioPositionToSpawn, ADDR_SetWarioPositionToSpawn);
	routines_.call_in(BANK_SetInitialCameraScroll, ADDR_SetInitialCameraScroll);
	routines_.call_in(BANK_ApplyInitialCameraScroll, ADDR_ApplyInitialCameraScroll);

	if (mem_.room_transition_param() & (1 << ROOMTRANSITIONF_RELOAD_OBJECTS_F))
		routines_.call(ADDR_ReloadLevelObjects);
	// .no_object_reloading
	mem_.room_transition_param() = 0;

	routines_.call_in(BANK_UpdateSwitchableBlocks, ADDR_UpdateSwitchableBlocks);
	call_in_sram1(ADDR_DrawRoom_FromStart);
	call_in_sram1(ADDR_Func_896f);
	level::set_wario_screen_pos(gb_);

	if (mem_.wario_state() == WST_ENTERING_DOOR) {
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
		mem_[wWarioFramesetPtr + 0] = ADDR_Frameset_14d18 >> 8;
		mem_[wWarioFramesetPtr + 1] = static_cast<uint8_t>(ADDR_Frameset_14d18);
		home::farcall(gb_, OAM1, ADDR_UpdateWarioAnimation, [&] { home::update_wario_animation_regs(gb_, OAM1); });
	}

	// .not_entering_door
	home::update_level_music(gb_);
	mem_.block_func_wario_flag() = FALSE;
	mem_.is_floor_transition() = 0;
	push_wram(BANK_wObjects);
	routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { level::update_objects(gb_); });
	routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { level::update_objects(gb_); });
	pop_wram();
	push_wram(BANK_wObjects);
	routines_.in_bank(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, [&] { level::draw_objects_no_priority(gb_); });
	pop_wram();
	home::draw_wario(gb_);
	push_wram(BANK_wObjects);
	routines_.in_bank(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, [&] { level::draw_objects_with_priority(gb_); });
	pop_wram();
	mem_.is_intangible() = 0;
	mem_.sub_state() = 0x02;
	mem_[rLCDC] = LCDC_DEFAULT;
}

// the level's first room (or the saved level's)
void RoomStart::from_level_start()
{
	mem_.is_boss_battle() = FALSE;

	if (!from_saved_level()) {
		clear({wLevelRoomID, wUnused_ca6c, wTempLevelRoomID});
		clear({wLevelEndScreen, wNumMusicalCoins, wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta,
			wCameraXDelta, wScrollUpEdge, wScrollDownEdge, wIsIntangible, wInvincibleCounter,
			wIsMinigameCleared, wIsRolling, wIsGettingOffLadder, wWarioAttributes, wSwitchStateUpdated,
			wLevelTime + 0, wLevelTime + 1, wLevelTime + 2, wBossState, wSwimVelIndex,
			wWaterSurfaceFloatingCounter});
		routines_.call(ADDR_ClearTransformationValues);
		// the 0 ClearTransformationValues leaves in a
		for (uint16_t addr : {wInvisibleFrame, wCameraRoomYScrollLimit, wIsOnSteppableObject, wGroundShakeCounter,
			     wSCYShake, wSwimmingDirectionInput, wAnimatedTilesFrameDuration, wRoomPalCycleDuration,
			     wRoomAnimatedTilesEnabled, wRoomAnimatedPalsEnabled, wWaterCurrent, wLastWaterCurrent,
			     wCurWaterCurrent})
			mem_[addr] = 0;

		// the level's variables and its objects'
		push_wram(BANK_wObjects);
		routines_.farcall(BANK_InitObjectVars_FromLevelStart, ADDR_InitObjectVars_FromLevelStart);
		pop_wram();
	} else {
		// .skip_init_vars_and_objects
		mem_.level_room_id() = mem_.temp_level_room_id();
		home::load_wario_gfx(gb_);
	}

	// .load_blocks_and_objects
	routines_.call(ADDR_LoadLevelBlockMapAndObjects);
	push_wram(GFX_RAM);
	routines_.call(ADDR_LoadRoom);
	pop_wram();

	if (!from_saved_level()) {
		routines_.call_in(BANK_SetWarioPositionToSpawn, ADDR_SetWarioPositionToSpawn);
		mem_.direction() = DIRECTION_RIGHT;
		routines_.farcall(BANK_SetState_Idling, ADDR_SetState_Idling);
		set_be(wWarioPalsPtr, ADDR_WarioDefaultPal);
		// CopyHLToDE_Short takes hl, de and b
		routines_.in_bank(BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, [&] { home::copy_short(gb_, ROM(gb_, WarioDefaultPal), wTempPals2, 2 * PAL_SIZE); });
	} else {
		// .asm_883a
		routines_.in_bank(BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, [&] { home::copy_short(gb_, RomPtr(gb_, BANK_WarioDefaultPal, be(wWarioPalsPtr)), wTempPals2, 2 * PAL_SIZE); });

		home::copy(gb_, wcaa1, wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
	}

	// .asm_8861
	routines_.in_bank(BANK_Pals_d1fe, ADDR_CopyHLToDE_Short, [&] { home::copy_short(gb_, ROM(gb_, Pals_d1fe), wTempPals2 + 2 * PAL_SIZE, 1 * PAL_SIZE); });

	routines_.call_in(BANK_SetInitialCameraScroll, ADDR_SetInitialCameraScroll);
	routines_.call_in(BANK_ApplyInitialCameraScroll, ADDR_ApplyInitialCameraScroll);
	routines_.call_in(BANK_VBlank_Level, ADDR_VBlank_Level);
	level::set_tile_queue_processing_functions(gb_);
	call_in_sram1(ADDR_DrawRoom_FromStart);
	if (!from_saved_level())
		call_in_sram1(ADDR_Func_896f);

	// .asm_88b7
	if (mem_.level() == THE_TEMPLE) {
		uint8_t scy = mem_[wCameraSCY + 1];
		for (uint16_t addr : std::initializer_list<uint16_t>{wc08a, wHiddenFigureSCYShake, wSCY, rSCY})
			mem_[addr] = scy;
		uint8_t scx = mem_[wCameraSCX + 1];
		for (uint16_t addr : std::initializer_list<uint16_t>{wc08c, wc08b, wSCX, rSCX})
			mem_[addr] = scx;
	}

	// .not_the_temple
	level::set_wario_screen_pos(gb_);
	mem_.block_func_wario_flag() = FALSE;

	if (!from_saved_level()) {
		mem_.is_floor_transition() = FALSE;
		push_wram(BANK_wObjects);
		// the objects updated twice
		routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { level::update_objects(gb_); });
		routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { level::update_objects(gb_); });
		pop_wram();
	} else {
		// .asm_8917
		mem_.is_floor_transition() = TRUE;
		push_wram(BANK_wObjects);
		routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { level::update_objects(gb_); });
		pop_wram();
	}

	// .draw_objs
	mem_.is_floor_transition() = FALSE;
	push_wram(BANK_wObjects);
	routines_.in_bank(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, [&] { level::draw_objects_no_priority(gb_); });
	pop_wram();
	home::draw_wario(gb_);
	push_wram(BANK_wObjects);
	routines_.in_bank(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, [&] { level::draw_objects_with_priority(gb_); });
	pop_wram();
}

} // namespace

} // namespace wl3::level

using wl3::level::RoomStart;

// These three end on UpdateObjects / DrawObjects, which leave b to l as
// they please; only _LevelStateTable reaches them (InitLevel, for the
// second, then calls UpdateLevelMusic, which reads no register), and
// LevelStateTable sets a and f itself after the state: nothing reads any
// register they leave (live.py, entry_reads2.py / entry_reads_af.py).
PORT_EX(StartRoom_FromTransition, 0, ALL_REGISTERS) { RoomStart(gb).from_transition(); }
PORT_EX(StartRoom_FromLevelStart, 0, ALL_REGISTERS) { RoomStart(gb).from_level_start(); }
// called from StartRoom_* (in SRAM1), which go on to SetWarioScreenPos: it
// and they read no register this leaves (live.py, entry_reads2.py)
PORT_EX(Func_896f, 0, ALL_REGISTERS) { RoomStart(gb).spawn_objects(); }
// called only from StartRoom_*, which read no register these leave (live.py)
PORT_EX(SetInitialCameraScroll, 0, ALL_REGISTERS) { RoomStart(gb).set_initial_camera_scroll(); }
PORT_EX(ApplyInitialCameraScroll, 0, ALL_REGISTERS) { RoomStart(gb).apply_initial_camera_scroll(); }
// no caller reads a register this leaves (live.py, all 11 sites)
PORT_EX(SetWarioPositionToSpawn, 0, ALL_REGISTERS) { RoomStart(gb).set_wario_position_to_spawn(); }
// called only from StartRoom_*, which read no register it leaves (live.py)
PORT_EX(DrawRoom_FromStart, 0, ALL_REGISTERS) { RoomStart(gb).draw_room(); }
PORT_EX(ProcessMultiBlock, 0, ALL_REGISTERS) { RoomStart(gb).process_multi_block(); }
// ProcessMultiBlock calls it, then UpdateParticles, which reads no register
PORT_LOCAL_EX(ProcessMultiBlock, ProcessBlocks, 0, ALL_REGISTERS) { RoomStart(gb).process_blocks(); }
