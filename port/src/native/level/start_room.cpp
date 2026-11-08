// Starting a room (src/engine/level/start_room.asm): from the level's start
// or a transition, loading it, placing Wario at its spawn point, setting
// the camera, drawing the whole visible BG map, spawning its objects; and
// processing a multi-block between frames.
//
// They call much that's still C with its registers, and the room drawing's
// queues read the stack (the asm's return addresses, see HANDOFF.md), so
// these keep the original's registers, flags and return addresses.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "../home/object_pos.hpp"

#include <initializer_list>

namespace wl3::level {

namespace {

constexpr uint16_t SRT(uint16_t offset) { return static_cast<uint16_t>(ADDR_StartRoom_FromTransition + offset); }
constexpr uint16_t SRL(uint16_t offset) { return static_cast<uint16_t>(ADDR_StartRoom_FromLevelStart + offset); }
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
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void push_af() { gb_push16(gb_, REG_AF(gb_)); }
	void pop_af()
	{
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
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
	void push_wram(uint8_t bank)
	{
		cpu_.a = mem_[rWBK];
		push_af();
		cpu_.a = bank;
		mem_[rWBK] = bank;
	}
	void pop_wram()
	{
		pop_af();
		mem_[rWBK] = cpu_.a;
	}
	// ld a, [wceef] / and %00111100: from a saved level
	bool from_saved_level()
	{
		cpu_.a = mem_[wceef] & 0x3c;
		cpu_.f = flags::and_(cpu_.a);
		return cpu_.a;
	}
	// ld a, [wCameraConfigFlags] / bit n, a
	bool cam_bit(int n)
	{
		cpu_.a = mem_[wCameraConfigFlags];
		cpu_.f = flags::bit(cpu_.f, cpu_.a, n);
		return cpu_.a & (1 << n);
	}
	// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp v
	void cam_cp(uint8_t v)
	{
		cpu_.a = mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK;
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
	bool cam_border(int bit) const { return mem_[wCameraConfigFlags] & (1 << bit); }
	void draw_room_column(uint16_t queue);
	void queue_row_tiles_and_attributes();
	void call_in_sram1(uint16_t addr, uint16_t ret);
	void farcall_obj_wram(uint8_t bank, uint16_t addr);

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
	if (static_cast<uint8_t>(mem_[wc0c7] - 1) < block.col || mem_[wc0c6] > block.col)
		return;
	uint8_t sub = mem_[wccec] == 1 ? 0 : 0x20;
	if (static_cast<uint8_t>(mem_[wc0c4] - sub - 1) < block.row || static_cast<uint8_t>(mem_[wc0c5] - sub) > block.row)
		return;

	// .spawn: SpawnObject takes the index in c, the block in de
	home::obj_map_block_to_hpos(gb_, block);
	cpu_.c = index;
	cpu_.d = block.row;
	cpu_.e = home::level_col(block.col, block.parity);
	push_wram(BANK_wObjects); // BANK("Level Objects WRAM")
	routines_.farcall(BANK_SpawnObject, ADDR_SpawnObject);
	pop_wram();
}

// spawns the objects in the 16x16 blocks of the object map around
// wBlockXPos / wObjectSpawnBlockY
void RoomStart::spawn_objects()
{
	for (int i = 0; i < 4; i++)
		mem_[hXPosLo - i] = mem_[wBlockXPos + 1 - i];
	home::ObjMapBlock block = obj_map_at_hpos();
	mem_[wUnused_ObjectParity] = 1;
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
	uint8_t row = mem_[wObjectSpawnBlockY];
	for (int i = 0; i < 2 && row >= 0xc0; i++) {
		bank++;
		row = static_cast<uint8_t>(row - 0x20);
	}
	mem_[wccec] = bank;
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
		mem_[wccec] = static_cast<uint8_t>(mem_[wccec] + 1);
		sram_switch(mem_[wccec]);
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
	mem_[wCameraRoomXScrollLimit] = 0;
	uint8_t mode = mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK;
	if (mode == (CAM_XSCROLL2 | CAM_TRANSITIONS) || mode == CAM_YSCROLL) { // the x scroll at Wario's
		mem_[wCameraSCX + 0] = mem_[wWarioXPos + 0];
		mem_[wCameraSCX + 1] = 0x30;
	} else {
		uint8_t spacing = static_cast<uint8_t>(mem_[mem_[wDirection] ? wCamRightSpacing : wCamLeftSpacing] - 8);
		uint16_t v = scroll_minus(wCameraSCX, wWarioXPos, spacing, CAM_BORDER_LEFT_F);
		if (!cap_low(wCameraSCX, v, wRoomLeftXLimit, wCameraRoomXScrollLimit))
			cap_high(wCameraSCX, sLevelBlockObjectMap >> 8, 0xc0, CAM_BORDER_RIGHT_F, wRoomRightXLimit,
				wCameraRoomXScrollLimit);
	}

	// .check_if_saved_level: no y scroll to handle
	if ((mem_[wceef] & 0b00111100) && mode >= CAM_TRANSITIONS)
		return;

	// .handle_cam_y_scroll
	mem_[wCameraRoomYScrollLimit] = 0;
	uint16_t v = scroll_minus(wCameraSCY, wWarioYPos, 0x60, CAM_BORDER_UP_F);
	if (cap_low(wCameraSCY, v, wRoomUpperYLimit, wCameraRoomYScrollLimit))
		return;
	if (cap_high(wCameraSCY, 0x90, 0xb0, CAM_BORDER_DOWN_F, wRoomLowerYLimit, wCameraRoomYScrollLimit))
		return;

	// .check_transitions: the floor Wario's on
	if (mode < CAM_TRANSITIONS)
		return;
	if (mem_[wWarioYPos + 1] >= 0x80) {
		mem_[wCameraSCY + 1] = 0x68;
		return;
	}
	uint8_t upper = mem_[wRoomUpperYLimit];
	uint8_t hi = static_cast<uint8_t>(mem_[wWarioYPos + 0] - 1);
	mem_[wCameraSCY + 0] = hi;
	if (hi < upper) { // .cap_up_side
		mem_[wCameraSCY + 0] = upper;
		mem_[wCameraRoomYScrollLimit] = 1;
		return;
	}
	if (!(hi & 0x80)) {
		mem_[wCameraSCY + 1] = 0xe8;
		return;
	}
	mem_[wCameraSCY + 0] = 0;
	mem_[wCameraRoomYScrollLimit] = 1;
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
	uint8_t limit = mem_[wCameraRoomYScrollLimit];
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
	limit = mem_[wCameraRoomXScrollLimit];
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
	uint8_t spawn = mem_[wWarioSpawnPos];
	mem_[hXPosLo] = 0;
	mem_[hXPosHi] = spawn & 0x0f;
	mem_[hYPosLo] = 0;
	mem_[hYPosHi] = spawn >> 4;
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
	if ((mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK) < CAM_TRANSITIONS || (mem_[wceef] & 0x3c))
		return;
	routines_.call(ADDR_UpdateFloor);
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
	routines_.call(ADDR_GetBlockBGPtr);
	cpu_.b = mem_[wBGMapAddressQueueSize];
	SET_DE(gb_, wBGMapAddressQueue);
	cpu_.f = flags::add(cpu_.e, cpu_.b);
	cpu_.e = static_cast<uint8_t>(cpu_.e + cpu_.b);
	cpu_.b = TILEMAP_HEIGHT;
	do {
		cpu_.a = cpu_.h;
		mem_[REG_DE(gb_)] = cpu_.a;
		cpu_.f = flags::inc(cpu_.f, cpu_.e);
		cpu_.e++;
		cpu_.a = cpu_.l;
		mem_[REG_DE(gb_)] = cpu_.a;
		cpu_.f = flags::inc(cpu_.f, cpu_.e);
		cpu_.e++;
		add_hl(TILEMAP_WIDTH);
		cpu_.a = cpu_.h & 0xfb;
		cpu_.f = flags::and_(cpu_.a);
		cpu_.h = cpu_.a;
		cpu_.f = flags::dec(cpu_.f, cpu_.b);
	} while (--cpu_.b);

	uint8_t addresses = mem_[wBGMapAddressQueueSize];
	cpu_.f = flags::add(addresses, 2 * TILEMAP_HEIGHT);
	cpu_.a = static_cast<uint8_t>(addresses + 2 * TILEMAP_HEIGHT);
	mem_[wBGMapAddressQueueSize] = cpu_.a;

	SET_HL(gb_, wBlockPos);
	routines_.call(ADDR_GetBlockPtr);
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	cpu_.a = mem_[wBlockXPos + 1] & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	bool odd = cpu_.a;
	gb_push16(gb_, REG_HL(gb_));
	routines_.farcall(BANK_QueueBlockTilesAlongRow_OddColumn,
		odd ? ADDR_QueueBlockTilesAlongRow_OddColumn : ADDR_QueueBlockTilesAlongRow_EvenColumn);
	SET_HL(gb_, gb_pop16(gb_));
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	routines_.farcall(BANK_QueueBlockTilesAlongRow_OddColumn,
		odd ? ADDR_QueueBlockAttributesAlongRow_OddColumn : ADDR_QueueBlockAttributesAlongRow_EvenColumn);
	uint8_t tiles = mem_[wBGMapTileQueueSize];
	cpu_.f = flags::add(tiles, TILEMAP_WIDTH);
	cpu_.a = static_cast<uint8_t>(tiles + TILEMAP_WIDTH);
	mem_[wBGMapTileQueueSize] = cpu_.a;
}

// draws all of the room that the BG map shows, column by column
void RoomStart::draw_room()
{
	for (int i = 0; i < 4; i++)
		mem_[wBackupBlockPos + i] = mem_[wBlockPos + i];
	mem_[wDrawRoomWidthLoopVar] = TILEMAP_WIDTH;
	do {
		queue_row_tiles_and_attributes();
		draw_room_column(wBGMapTileVRAM0Queue);
		mem_[rVBK] = 1; // BANK("VRAM1")
		draw_room_column(wBGMapTileVRAM1Queue);
		mem_[rVBK] = 0;
		mem_[wBGMapAddressQueueSize] = 0;
		mem_[wBGMapTileQueueSize] = 0;
		set_be(wBlockXPos, static_cast<uint16_t>(be(wBlockXPos) + 0x08));
		mem_[wDrawRoomWidthLoopVar] = static_cast<uint8_t>(mem_[wDrawRoomWidthLoopVar] - 1);
	} while (mem_[wDrawRoomWidthLoopVar]);
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
	routines_.call_ret(PROCESS_BLOCKS, ADDR_ProcessMultiBlock + 0x03);
	routines_.farcall(BANK_UpdateParticles, ADDR_UpdateParticles);
	farcall_obj_wram(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority);
	routines_.call(ADDR_DrawWario);
	farcall_obj_wram(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority);
	routines_.call(ADDR_ClearUnusedVirtualOAM);
	cpu_.a = mem_[wSRAMBank];
	push_af();
	sram_switch(SRAM1);
	cpu_.ime = false; // di
	routines_.call(ADDR_SetTileQueueProcessingFunctions);
	cpu_.ime = true; // ei
	cpu_.ime_delay = 0;
	pop_af();
	sram_switch(cpu_.a);
}

// the block next to wcedb:wcedc in the direction in wMultiBlockParam's low
// 3 bits, until ProcessBlock leaves none
void RoomStart::process_blocks()
{
	uint8_t param = static_cast<uint8_t>((mem_[wMultiBlockParam] & 0x0f) + 0x10);
	mem_[wMultiBlockParam] = param;
	mem_[wcede] = param & 0x08;

	for (;;) {
		// the next block from wcedb:wcedc (the map's rows run on into the
		// next SRAM bank)
		uint8_t bank = mem_[wcedb], row = mem_[wcedc + 0], column = mem_[wcedc + 1];
		switch (mem_[wMultiBlockParam] & 0x07) {
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
		mem_[wBlockPtrBank] = bank;
		mem_[wBlockPtr + 0] = row;
		mem_[wBlockPtr + 1] = column;

		// .process_block
		mem_[wMultiBlockParam] = mem_[wMultiBlockParam] & (0xff ^ 0x07);
		routines_.farcall_ret(BANK_ProcessBlock, ADDR_ProcessBlock, ADDR_ProcessMultiBlock + 0x10d);
		if (!(mem_[wMultiBlockParam] & 0x07))
			break;
		routines_.call_ret(ADDR_Func_bb85, ADDR_ProcessMultiBlock + 0x11a);
	}

	// .break
	mem_[wMultiBlockParam] = 0;
	mem_[wIsFloorTransition] = 0;
	mem_[wSubState] = mem_[wPendingSubState];
}

// ld a, [wSRAMBank] / push af / ld a, BANK("SRAM1") / sramswitch /
// call addr / pop af / sramswitch
void RoomStart::call_in_sram1(uint16_t addr, uint16_t ret)
{
	cpu_.a = mem_[wSRAMBank];
	push_af();
	cpu_.a = SRAM1;
	sram_switch(SRAM1);
	routines_.call_ret(addr, ret);
	pop_af();
	sram_switch(cpu_.a);
}

// a room from a transition (or the golf minigame)
void RoomStart::from_transition()
{
	if ((mem_[wRoomTransitionParam] & ROOMTRANSITION_MASK) == ROOMTRANSITION_GOLF) { // the golf minigame
		push_wram(0x03);
		mem_[wTempAnimatedTilesFrameDuration] = mem_[wAnimatedTilesFrameDuration];
		mem_[wTempAnimatedTilesGroup] = mem_[wAnimatedTilesGfx];
		pop_wram();
		mem_[wPendingSubState] = mem_[wSubState];
		mem_[wState] = ST_GOLF;
		mem_[wSubState] = 0;
		return;
	}

	// .not_golf
	routines_.call_ret(ADDR_DisableLCD, SRT(0x32));
	routines_.farcall_ret(BANK_DespawnAllObjects, ADDR_DespawnAllObjects, SRT(0x41));
	push_wram(BANK_wObjects); // BANK("Level Objects WRAM")
	routines_.farcall_ret(BANK_InitObjectVars_FromTransition, ADDR_InitObjectVars_FromTransition, SRT(0x57));
	pop_wram();

	sram_switch(0x00);
	if ((mem_[wRoomTransitionParam] & ROOMTRANSITION_MASK) == ROOMTRANSITION_TELEPORT) {
		mem_[wWarioYPos + 0] = 0x01;
		mem_[wWarioXPos + 0] = 0;
	}
	// .not_teleport: up a block
	set_be(wWarioYPos, static_cast<uint16_t>(be(wWarioYPos) - 0x10));
	uint8_t y_hi = mem_[wWarioYPos + 0];
	mem_[wUnused_ca6c] = static_cast<uint8_t>(mem_[wWarioXPos + 0] | (y_hi << 4 | y_hi >> 4));

	// the next room's ID from Wario's position
	SET_HL(gb_, wWarioPos);
	routines_.call_ret(ADDR_GetNextLevelRoomID, SRT(0x94));

	clear({wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta, wCameraXDelta, wScrollUpEdge,
		wScrollDownEdge, wGroundShakeCounter, wSCYShake});
	mem_[wTempLevelRoomID] = mem_[wLevelRoomID];

	push_wram(GFX_RAM);
	routines_.call_ret(ADDR_LoadRoom, SRT(0xbd));
	pop_wram();

	routines_.call_ret(ADDR_FillBGMap0_With7f, SRT(0xc3));
	routines_.call_ret(ADDR_ClearVirtualOAM, SRT(0xc6));
	routines_.call_ret(ADDR_ClearParticles, SRT(0xc9));
	routines_.call_ret(ADDR_SetWarioPositionToSpawn, SRT(0xcc));
	routines_.call_ret(ADDR_SetInitialCameraScroll, SRT(0xcf));
	routines_.call_ret(ADDR_ApplyInitialCameraScroll, SRT(0xd2));

	if (mem_[wRoomTransitionParam] & (1 << ROOMTRANSITIONF_RELOAD_OBJECTS_F))
		routines_.call_ret(ADDR_ReloadLevelObjects, SRT(0xdc));
	// .no_object_reloading
	mem_[wRoomTransitionParam] = 0;

	routines_.call_ret(ADDR_UpdateSwitchableBlocks, SRT(0xe3));
	call_in_sram1(ADDR_DrawRoom_FromStart, SRT(0xf2));
	call_in_sram1(ADDR_Func_896f, SRT(0x108));
	routines_.call_ret(ADDR_SetWarioScreenPos, SRT(0x112));

	if (mem_[wWarioState] == WST_ENTERING_DOOR) {
		mem_[wFrameDuration] = 0;
		mem_[wAnimationFrame] = 0;
		mem_[wWarioFramesetPtr + 0] = ADDR_Frameset_14d18 >> 8;
		mem_[wWarioFramesetPtr + 1] = static_cast<uint8_t>(ADDR_Frameset_14d18);
		routines_.farcall_ret(OAM1, ADDR_UpdateWarioAnimation, SRT(0x139));
	}

	// .not_entering_door
	routines_.call_ret(ADDR_UpdateLevelMusic, SRT(0x13c));
	mem_[wBlockFuncWarioFlag] = FALSE;
	mem_[wIsFloorTransition] = 0;
	push_wram(BANK_wObjects);
	routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRT(0x159));
	routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRT(0x168));
	pop_wram();
	push_wram(BANK_wObjects);
	routines_.farcall_ret(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, SRT(0x181));
	pop_wram();
	routines_.call_ret(ADDR_DrawWario, SRT(0x187));
	push_wram(BANK_wObjects);
	routines_.farcall_ret(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, SRT(0x19d));
	pop_wram();
	mem_[wIsIntangible] = 0;
	mem_[wSubState] = 0x02;
	mem_[rLCDC] = LCDC_DEFAULT;
}

// the level's first room (or the saved level's)
void RoomStart::from_level_start()
{
	mem_[wIsBossBattle] = FALSE;

	if (!from_saved_level()) {
		clear({wLevelRoomID, wUnused_ca6c, wTempLevelRoomID});
		clear({wLevelEndScreen, wNumMusicalCoins, wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta,
			wCameraXDelta, wScrollUpEdge, wScrollDownEdge, wIsIntangible, wInvincibleCounter,
			wIsMinigameCleared, wIsRolling, wIsGettingOffLadder, wWarioAttributes, wSwitchStateUpdated,
			wLevelTime + 0, wLevelTime + 1, wLevelTime + 2, wBossState, wSwimVelIndex,
			wWaterSurfaceFloatingCounter});
		routines_.call_ret(ADDR_ClearTransformationValues, SRL(0x59));
		// the 0 ClearTransformationValues leaves in a
		for (uint16_t addr : {wInvisibleFrame, wCameraRoomYScrollLimit, wIsOnSteppableObject, wGroundShakeCounter,
			     wSCYShake, wSwimmingDirectionInput, wAnimatedTilesFrameDuration, wRoomPalCycleDuration,
			     wRoomAnimatedTilesEnabled, wRoomAnimatedPalsEnabled, wWaterCurrent, wLastWaterCurrent,
			     wCurWaterCurrent})
			mem_[addr] = 0;

		// the level's variables and its objects'
		push_wram(BANK_wObjects);
		routines_.farcall_ret(BANK_InitObjectVars_FromLevelStart, ADDR_InitObjectVars_FromLevelStart, SRL(0x96));
		pop_wram();
	} else {
		// .skip_init_vars_and_objects
		mem_[wLevelRoomID] = mem_[wTempLevelRoomID];
		routines_.call_ret(ADDR_LoadWarioGfx, SRL(0xa4));
	}

	// .load_blocks_and_objects
	routines_.call_ret(ADDR_LoadLevelBlockMapAndObjects, SRL(0xa7));
	push_wram(GFX_RAM);
	routines_.call_ret(ADDR_LoadRoom, SRL(0xb1));
	pop_wram();

	if (!from_saved_level()) {
		routines_.call_ret(ADDR_SetWarioPositionToSpawn, SRL(0xbe));
		mem_[wDirection] = DIRECTION_RIGHT;
		routines_.farcall_ret(BANK_SetState_Idling, ADDR_SetState_Idling, SRL(0xd2));
		set_be(wWarioPalsPtr, ADDR_WarioDefaultPal);
		// CopyHLToDE_Short takes hl, de and b
		SET_HL(gb_, ADDR_WarioDefaultPal);
		SET_DE(gb_, wTempPals2);
		cpu_.b = 2 * PAL_SIZE; // 2 palettes
		routines_.farcall_ret(BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, SRL(0xf1));
	} else {
		// .asm_883a
		SET_HL(gb_, be(wWarioPalsPtr));
		SET_DE(gb_, wTempPals2);
		cpu_.b = 2 * PAL_SIZE; // 2 palettes
		routines_.farcall_ret(BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, SRL(0x10f));

		SET_HL(gb_, wcaa1);
		SET_DE(gb_, wTempPals2 + 4 * PAL_SIZE);
		cpu_.b = 4 * PAL_SIZE; // 4 palettes
		routines_.call_ret(ADDR_CopyHLToDE, SRL(0x11a));
	}

	// .asm_8861
	SET_HL(gb_, ADDR_Pals_d1fe);
	SET_DE(gb_, wTempPals2 + 2 * PAL_SIZE);
	cpu_.b = 1 * PAL_SIZE; // 1 palette
	routines_.farcall_ret(BANK_Pals_d1fe, ADDR_CopyHLToDE_Short, SRL(0x131));

	routines_.call_ret(ADDR_SetInitialCameraScroll, SRL(0x134));
	routines_.call_ret(ADDR_ApplyInitialCameraScroll, SRL(0x137));
	routines_.call_ret(ADDR_VBlank_Level, SRL(0x13a));
	routines_.call_ret(ADDR_SetTileQueueProcessingFunctions, SRL(0x13d));
	call_in_sram1(ADDR_DrawRoom_FromStart, SRL(0x14c));
	if (!from_saved_level())
		call_in_sram1(ADDR_Func_896f, SRL(0x169));

	// .asm_88b7
	if (mem_[wLevel] == THE_TEMPLE) {
		uint8_t scy = mem_[wCameraSCY + 1];
		for (uint16_t addr : std::initializer_list<uint16_t>{wc08a, wHiddenFigureSCYShake, wSCY, rSCY})
			mem_[addr] = scy;
		uint8_t scx = mem_[wCameraSCX + 1];
		for (uint16_t addr : std::initializer_list<uint16_t>{wc08c, wc08b, wSCX, rSCX})
			mem_[addr] = scx;
	}

	// .not_the_temple
	routines_.call_ret(ADDR_SetWarioScreenPos, SRL(0x197));
	mem_[wBlockFuncWarioFlag] = FALSE;

	if (!from_saved_level()) {
		mem_[wIsFloorTransition] = FALSE;
		push_wram(BANK_wObjects);
		// the objects updated twice
		routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRL(0x1bc));
		routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRL(0x1cb));
		pop_wram();
	} else {
		// .asm_8917
		mem_[wIsFloorTransition] = TRUE;
		push_wram(BANK_wObjects);
		routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRL(0x1eb));
		pop_wram();
	}

	// .draw_objs
	mem_[wIsFloorTransition] = FALSE;
	push_wram(BANK_wObjects);
	routines_.farcall_ret(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, SRL(0x208));
	pop_wram();
	routines_.call_ret(ADDR_DrawWario, SRL(0x20e));
	push_wram(BANK_wObjects);
	routines_.farcall_ret(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, SRL(0x224));
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
