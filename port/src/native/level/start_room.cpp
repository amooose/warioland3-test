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
	void flip_b()
	{
		cpu_.a = cpu_.b ^ 0x01;
		cpu_.f = flags::xor_(cpu_.a);
		cpu_.b = cpu_.a;
	}
	void add_hl(uint16_t v)
	{
		uint16_t hl = REG_HL(gb_);
		unsigned sum = hl + v;
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((hl & 0xfff) + (v & 0xfff) > 0xfff ? FLAG_H : 0)
			| (sum > 0xffff ? FLAG_C : 0));
		SET_HL(gb_, sum);
	}
	void obj_map_bank(uint16_t addr);
	void spawn_if_in_range();
	void scroll_minus(uint16_t scroll, uint16_t pos, uint8_t offset, int border);
	bool cap_low(uint16_t scroll, uint16_t limit, uint16_t limit_flag);
	bool cap_high(uint16_t scroll, uint16_t de, uint8_t wide_span, int border, uint16_t limit, uint16_t limit_flag);
	void set_scroll(uint16_t scroll, uint16_t reg, uint16_t wreg, bool store);
	void draw_room_column(uint16_t queue);
	void queue_row_tiles_and_attributes();
	void call_in_sram1(uint16_t addr, uint16_t ret);
	void farcall_obj_wram(uint8_t bank, uint16_t addr);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// c = 1 / ld a, [addr]: the SRAM bank (c) and page (a) of an object map row
void RoomStart::obj_map_bank(uint16_t addr)
{
	cpu_.c = 0x01;
	cpu_.a = mem_[addr];
	cpu_.f = flags::cp(cpu_.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	for (int i = 0; i < 2 && !cf(); i++) {
		cpu_.f = flags::inc(cpu_.f, cpu_.c);
		cpu_.c++;
		cpu_.f = flags::sub(cpu_.a, 0x20);
		cpu_.a = static_cast<uint8_t>(cpu_.a - 0x20);
		if (i == 0)
			cpu_.f = flags::cp(cpu_.a, 0xc0);
	}
}

// a = an object's index: spawns it if its block (hl) is in the spawn range
void RoomStart::spawn_if_in_range()
{
	cpu_.f = flags::cp(cpu_.a, 0x0f);
	if (!cf())
		return; // Wario's spawn block
	cpu_.c = cpu_.a; // the object's index
	uint8_t right = mem_[wc0c7];
	cpu_.f = flags::dec(cpu_.f, right);
	cpu_.a = static_cast<uint8_t>(right - 1);
	cpu_.f = flags::cp(cpu_.a, cpu_.l);
	if (cf())
		return;
	cpu_.a = mem_[wc0c6];
	cpu_.f = flags::cp(cpu_.a, cpu_.l);
	if (!zf() && !cf())
		return;
	uint8_t bank = mem_[wccec];
	cpu_.f = flags::dec(cpu_.f, bank);
	uint8_t sub = zf() ? 0 : 0x20;
	cpu_.a = mem_[wc0c4];
	if (sub) {
		cpu_.f = flags::sub(cpu_.a, sub);
		cpu_.a = static_cast<uint8_t>(cpu_.a - sub);
	}
	cpu_.f = flags::dec(cpu_.f, cpu_.a);
	cpu_.a--;
	cpu_.f = flags::cp(cpu_.a, cpu_.h);
	if (cf())
		return;
	cpu_.a = mem_[wc0c5];
	if (sub) {
		cpu_.f = flags::sub(cpu_.a, sub);
		cpu_.a = static_cast<uint8_t>(cpu_.a - sub);
	}
	cpu_.f = flags::cp(cpu_.a, cpu_.h);
	if (!zf() && !cf())
		return;

	// .spawn
	gb_push16(gb_, REG_DE(gb_));
	gb_push16(gb_, REG_BC(gb_));
	routines_.call(ADDR_Func_cf8);
	gb_push16(gb_, REG_HL(gb_));
	routines_.call(ADDR_Func_d81);
	cpu_.d = cpu_.h;
	cpu_.e = cpu_.l;
	push_wram(BANK_wObjects); // BANK("Level Objects WRAM")
	routines_.farcall(BANK_SpawnObject, ADDR_SpawnObject);
	pop_wram();
	SET_HL(gb_, gb_pop16(gb_));
	SET_BC(gb_, gb_pop16(gb_));
	SET_DE(gb_, gb_pop16(gb_));
}

// spawns the objects in the 16x16 blocks of the object map around
// wBlockXPos / wObjectSpawnBlockY
void RoomStart::spawn_objects()
{
	// the block position to hPos: ld a, [de] / ld [hld], a / dec de, from
	// the x position's low byte down
	for (int i = 0; i < 4; i++)
		mem_[hXPosLo - i] = mem_[wBlockXPos + 1 - i];
	cpu_.a = mem_[wBlockXPos + 1 - 3];
	SET_HL(gb_, hXPosLo - 3);
	SET_DE(gb_, wBlockXPos + 1 - 3);
	routines_.call(ADDR_Func_cc0);

	obj_map_bank(wObjectSpawnBlockY);
	cpu_.h = cpu_.a;
	cpu_.a = cpu_.c;
	mem_[wccec] = cpu_.c;
	sram_switch(cpu_.c);
	cpu_.a = 1;
	mem_[wUnused_ObjectParity] = 1;
	cpu_.b = 1;
	cpu_.d = 0x10;
	do {
		cpu_.e = 16 / 2;
		do {
			uint8_t pair = mem_[REG_HL(gb_)];
			cpu_.a = (pair & 0xf0) >> 4; // and $f0 / swap a
			cpu_.f = flags::z(cpu_.a);
			if (cpu_.a)
				spawn_if_in_range();
			flip_b();
			cpu_.a = mem_[REG_HL(gb_)] & 0x0f;
			cpu_.f = flags::and_(cpu_.a);
			if (cpu_.a)
				spawn_if_in_range();
			cpu_.f = flags::inc(cpu_.f, cpu_.l);
			cpu_.l++;
			flip_b();
			cpu_.f = flags::dec(cpu_.f, cpu_.e);
		} while (--cpu_.e);
		cpu_.f = flags::sub(cpu_.l, 0x08);
		cpu_.a = static_cast<uint8_t>(cpu_.l - 0x08);
		cpu_.l = cpu_.a;
		cpu_.f = flags::inc(cpu_.f, cpu_.h);
		cpu_.h++;
		cpu_.a = cpu_.h;
		cpu_.f = flags::cp(cpu_.h, 0xc0);
		if (zf()) {
			cpu_.h = sLevelBlockObjectMap >> 8;
			uint8_t bank = mem_[wccec];
			cpu_.f = flags::inc(cpu_.f, bank);
			cpu_.a = static_cast<uint8_t>(bank + 1);
			mem_[wccec] = cpu_.a;
			sram_switch(cpu_.a);
		}
		cpu_.f = flags::dec(cpu_.f, cpu_.d);
	} while (--cpu_.d);
}

// hl = pos - offset (big-endian), stored to scroll; - $20 more with the
// border bit
void RoomStart::scroll_minus(uint16_t scroll, uint16_t pos, uint8_t offset, int border)
{
	uint8_t lo = mem_[pos + 1];
	cpu_.f = flags::sub(lo, offset);
	cpu_.a = static_cast<uint8_t>(lo - offset);
	mem_[scroll + 1] = cpu_.a;
	cpu_.l = cpu_.a;
	bool borrow = lo < offset;
	uint8_t hi = mem_[pos + 0];
	cpu_.f = flags::sub(hi, 0, borrow);
	cpu_.a = static_cast<uint8_t>(hi - borrow);
	mem_[scroll + 0] = cpu_.a;
	cpu_.h = cpu_.a;
	if (cam_bit(border)) {
		SET_DE(gb_, static_cast<uint16_t>(-0x20));
		add_hl(static_cast<uint16_t>(-0x20));
	}
}

// caps hl to the low limit: true (and the scroll's at it) if it's below
bool RoomStart::cap_low(uint16_t scroll, uint16_t limit, uint16_t limit_flag)
{
	cpu_.b = mem_[limit];
	cpu_.a = cpu_.h;
	cpu_.f = flags::bit(cpu_.f, cpu_.a, 7);
	bool cap = cpu_.a & 0x80;
	if (!cap) {
		cpu_.f = flags::cp(cpu_.a, cpu_.b);
		if (cf()) {
			cap = true;
		} else if (zf()) {
			cpu_.a = cpu_.l;
			cpu_.f = flags::and_(cpu_.a);
			cap = !cpu_.a;
		}
	}
	if (cap) {
		cpu_.a = cpu_.b;
		mem_[scroll + 0] = cpu_.b;
		st(limit_flag, 1);
	}
	return cap;
}

// caps hl = scroll + de (e = wide_span with the border) to the high limit;
// true if it's at it
bool RoomStart::cap_high(uint16_t scroll, uint16_t de, uint8_t wide_span, int border, uint16_t limit, uint16_t limit_flag)
{
	cpu_.a = mem_[scroll + 0];
	cpu_.l = mem_[scroll + 1];
	cpu_.h = cpu_.a;
	SET_DE(gb_, de);
	if (cam_bit(border))
		cpu_.e = wide_span;
	add_hl(REG_DE(gb_));
	cpu_.b = mem_[limit];
	cpu_.a = cpu_.h;
	cpu_.f = flags::cp(cpu_.a, cpu_.b);
	if (cf())
		return false;
	cpu_.f = flags::dec(cpu_.f, cpu_.b);
	cpu_.a = static_cast<uint8_t>(cpu_.b - 1);
	mem_[scroll + 0] = cpu_.a;
	st(limit_flag, static_cast<uint8_t>(-1));
	return true;
}

// the camera scroll for a new room or the level's start
void RoomStart::set_initial_camera_scroll()
{
	xor_a();
	mem_[wCameraRoomXScrollLimit] = 0;
	cam_cp(CAM_XSCROLL2 | CAM_TRANSITIONS);
	if (!zf())
		cpu_.f = flags::cp(cpu_.a, CAM_YSCROLL);
	if (zf()) { // the x scroll at Wario's
		cpu_.a = mem_[wWarioXPos + 0];
		mem_[wCameraSCX + 0] = cpu_.a;
		st(wCameraSCX + 1, 0x30);
	} else {
		uint8_t dir = mem_[wDirection];
		cpu_.f = flags::and_(dir);
		uint8_t spacing = mem_[dir ? wCamRightSpacing : wCamLeftSpacing];
		cpu_.f = flags::sub(spacing, 8);
		cpu_.a = static_cast<uint8_t>(spacing - 8);
		cpu_.b = cpu_.a;
		scroll_minus(wCameraSCX, wWarioXPos, cpu_.b, CAM_BORDER_LEFT_F);
		if (!cap_low(wCameraSCX, wRoomLeftXLimit, wCameraRoomXScrollLimit))
			cap_high(wCameraSCX, sLevelBlockObjectMap >> 8, 0xc0, CAM_BORDER_RIGHT_F, wRoomRightXLimit,
				wCameraRoomXScrollLimit);
	}

	// .check_if_saved_level
	cpu_.a = mem_[wceef] & 0b00111100;
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a) {
		cam_cp(CAM_TRANSITIONS);
		if (!cf())
			return; // no y scroll to handle
	}

	// .handle_cam_y_scroll
	xor_a();
	mem_[wCameraRoomYScrollLimit] = 0;
	scroll_minus(wCameraSCY, wWarioYPos, 0x60, CAM_BORDER_UP_F);
	if (cap_low(wCameraSCY, wRoomUpperYLimit, wCameraRoomYScrollLimit))
		return;
	if (cap_high(wCameraSCY, 0x90, 0xb0, CAM_BORDER_DOWN_F, wRoomLowerYLimit, wCameraRoomYScrollLimit))
		return;

	// .check_transitions
	cam_cp(CAM_TRANSITIONS);
	if (cf())
		return; // not CAM_TRANSITIONS
	ld_cp(wWarioYPos + 1, 0x80);
	if (!cf()) {
		st(wCameraSCY + 1, 0x68);
		return;
	}
	cpu_.b = mem_[wRoomUpperYLimit];
	uint8_t y_hi = mem_[wWarioYPos + 0];
	cpu_.f = flags::dec(cpu_.f, y_hi);
	cpu_.a = static_cast<uint8_t>(y_hi - 1);
	mem_[wCameraSCY + 0] = cpu_.a;
	cpu_.f = flags::cp(cpu_.a, cpu_.b);
	if (cf()) { // .cap_up_side
		cpu_.a = cpu_.b;
		mem_[wCameraSCY + 0] = cpu_.b;
		st(wCameraRoomYScrollLimit, 1);
		return;
	}
	cpu_.f = flags::bit(cpu_.f, cpu_.a, 7);
	if (!(cpu_.a & 0x80)) {
		st(wCameraSCY + 1, 0xe8);
		return;
	}
	xor_a();
	mem_[wCameraSCY + 0] = 0;
	st(wCameraRoomYScrollLimit, 1);
}

// a = the scroll value: to [scroll + 1] (if store), rSC* and wSC*
void RoomStart::set_scroll(uint16_t scroll, uint16_t reg, uint16_t wreg, bool store)
{
	if (store)
		mem_[scroll + 1] = cpu_.a;
	mem_[reg] = cpu_.a;
	mem_[wreg] = cpu_.a;
}

// the camera scroll from SetInitialCameraScroll to rSCY / rSCX, and
// wBlockPos at the BG map's top left
void RoomStart::apply_initial_camera_scroll()
{
	cpu_.h = mem_[wCameraSCY + 0];
	cpu_.l = mem_[wCameraSCY + 1];
	cpu_.a = mem_[wCameraRoomYScrollLimit];
	cpu_.f = flags::cp(cpu_.a, 1);
	if (zf()) { // .on_upper_edge
		cpu_.a = cam_bit(CAM_BORDER_UP_F) ? 0x20 : 0x00;
		set_scroll(wCameraSCY, rSCY, wSCY, true);
		mem_[wBlockYPos + 1] = cpu_.a;
		cpu_.a = cpu_.h;
		mem_[wBlockYPos + 0] = cpu_.h;
	} else {
		cpu_.f = flags::cp(cpu_.a, static_cast<uint8_t>(-1));
		if (zf()) { // .on_lower_edge
			cpu_.a = cam_bit(CAM_BORDER_DOWN_F) ? 0x48 : 0x68;
			set_scroll(wCameraSCY, rSCY, wSCY, true);
			cpu_.f = flags::sub(cpu_.a, 0x20);
			cpu_.a = static_cast<uint8_t>(cpu_.a - 0x20);
			mem_[wBlockYPos + 1] = cpu_.a;
			cpu_.a = cpu_.h;
			mem_[wBlockYPos + 0] = cpu_.h;
		} else {
			cpu_.a = cpu_.l;
			set_scroll(wCameraSCY, rSCY, wSCY, false);
			cpu_.f = flags::sub(cpu_.a, 0x18);
			bool borrow = cpu_.a < 0x18;
			cpu_.a = static_cast<uint8_t>(cpu_.a - 0x18);
			mem_[wBlockYPos + 1] = cpu_.a;
			cpu_.f = flags::sub(cpu_.h, 0, borrow);
			cpu_.a = static_cast<uint8_t>(cpu_.h - borrow);
			mem_[wBlockYPos + 0] = cpu_.a;
		}
	}

	// .cam_x_scroll
	cpu_.h = mem_[wCameraSCX + 0];
	cpu_.l = mem_[wCameraSCX + 1];
	cpu_.a = mem_[wCameraRoomXScrollLimit];
	cpu_.f = flags::cp(cpu_.a, 1);
	if (zf()) { // .on_left_edge
		cpu_.a = cam_bit(CAM_BORDER_LEFT_F) ? 0x20 : 0x00;
		set_scroll(wCameraSCX, rSCX, wSCX, true);
		mem_[wBlockXPos + 1] = cpu_.a;
		cpu_.a = cpu_.h;
		mem_[wBlockXPos + 0] = cpu_.h;
		return;
	}
	cpu_.f = flags::cp(cpu_.a, static_cast<uint8_t>(-1));
	if (zf()) { // .on_right_edge
		cpu_.a = cam_bit(CAM_BORDER_RIGHT_F) ? 0x40 : 0x60;
		set_scroll(wCameraSCX, rSCX, wSCX, true);
		cpu_.f = flags::sub(cpu_.a, 0x28);
		cpu_.a = static_cast<uint8_t>(cpu_.a - 0x28);
		mem_[wBlockXPos + 1] = cpu_.a;
		cpu_.a = cpu_.h;
		mem_[wBlockXPos + 0] = cpu_.h;
		return;
	}
	cpu_.a = cpu_.l;
	set_scroll(wCameraSCX, rSCX, wSCX, false);
	cpu_.f = flags::sub(cpu_.a, 0x10);
	bool borrow = cpu_.a < 0x10;
	cpu_.a = static_cast<uint8_t>(cpu_.a - 0x10);
	mem_[wBlockXPos + 1] = cpu_.a;
	cpu_.f = flags::sub(cpu_.h, 0, borrow);
	cpu_.a = static_cast<uint8_t>(cpu_.h - borrow);
	mem_[wBlockXPos + 0] = cpu_.a;
	cpu_.f = flags::bit(cpu_.f, cpu_.a, 7);
	if (!(cpu_.a & 0x80))
		return;
	xor_a();
	mem_[wBlockXPos + 1] = 0;
	mem_[wBlockXPos + 0] = 0;
}

// hPos (and wWarioPos) at the block of wWarioSpawnPos, then exactly at the
// spawn block ($f) found in the object map there
void RoomStart::set_wario_position_to_spawn()
{
	xor_a();
	mem_[hXPosLo] = 0;
	cpu_.a = mem_[wWarioSpawnPos] & 0x0f;
	mem_[hXPosHi] = cpu_.a;
	xor_a();
	mem_[hYPosLo] = 0;
	uint8_t row = mem_[wWarioSpawnPos] & 0xf0;
	cpu_.a = static_cast<uint8_t>(row >> 4); // swap a
	cpu_.f = flags::z(cpu_.a);
	mem_[hYPosHi] = cpu_.a;
	SET_HL(gb_, hYPosHi);
	routines_.call(ADDR_Func_cc0);

	obj_map_bank(wObjectSpawnBlockY);
	cpu_.h = cpu_.a;
	cpu_.a = cpu_.c;
	mem_[wccec] = cpu_.c;
	sram_switch(cpu_.c);

	for (;;) {
		cpu_.e = 16 / 2;
		bool found = false;
		do {
			uint8_t pair = mem_[REG_HL(gb_)];
			cpu_.a = (pair & 0xf0) >> 4;
			cpu_.f = flags::cp(cpu_.a, 0x0f);
			if (!cf()) {
				found = true;
				break;
			}
			flip_b();
			cpu_.a = mem_[REG_HL(gb_)] & 0x0f;
			cpu_.f = flags::cp(cpu_.a, 0x0f);
			if (!cf()) {
				found = true;
				break;
			}
			cpu_.f = flags::inc(cpu_.f, cpu_.l);
			cpu_.l++;
			flip_b();
			cpu_.f = flags::dec(cpu_.f, cpu_.e);
		} while (--cpu_.e);
		if (found)
			break;
		cpu_.f = flags::sub(cpu_.l, 0x08);
		cpu_.a = static_cast<uint8_t>(cpu_.l - 0x08);
		cpu_.l = cpu_.a;
		cpu_.f = flags::inc(cpu_.f, cpu_.h);
		cpu_.h++;
		cpu_.a = cpu_.h;
		cpu_.f = flags::cp(cpu_.h, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		if (zf()) {
			cpu_.h = sLevelBlockObjectMap >> 8;
			uint8_t bank = mem_[wccec];
			cpu_.f = flags::inc(cpu_.f, bank);
			cpu_.a = static_cast<uint8_t>(bank + 1);
			mem_[wccec] = cpu_.a;
			sram_switch(cpu_.a);
		}
	}

	// .found_spawn_block
	cpu_.c = cpu_.a;
	routines_.call(ADDR_Func_cf8);
	for (int i = 0; i < 4; i++) // update_pos
		mem_[wWarioPos + i] = mem_[hPos + i];
	cpu_.a = mem_[hPos + 3];
	SET_HL(gb_, hPos + 3);
	SET_DE(gb_, wWarioPos + 3);

	cam_cp(CAM_TRANSITIONS);
	if (cf())
		return;
	cpu_.a = mem_[wceef] & 0x3c;
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return;
	routines_.call(ADDR_UpdateFloor);
}

// the queue down a BG map column (wrapping from v0BGMap1 back to v0BGMap0),
// TILEMAP_HEIGHT tiles: ld a, [hli] / ld [de], a
void RoomStart::draw_room_column(uint16_t queue)
{
	cpu_.d = mem_[wBGPtr + 0];
	cpu_.e = mem_[wBGPtr + 1];
	cpu_.a = cpu_.e;
	uint16_t src = queue;
	cpu_.b = TILEMAP_HEIGHT;
	do {
		cpu_.a = mem_[src++];
		mem_[REG_DE(gb_)] = cpu_.a;
		unsigned lo = cpu_.e + TILEMAP_WIDTH;
		cpu_.e = static_cast<uint8_t>(lo);
		cpu_.f = flags::add(cpu_.d, 0, lo > 0xff);
		cpu_.a = static_cast<uint8_t>(cpu_.d + (lo > 0xff));
		cpu_.d = cpu_.a;
		cpu_.f = flags::cp(cpu_.a, v0BGMap1 >> 8);
		if (zf())
			cpu_.d = v0BGMap0 >> 8;
		cpu_.f = flags::dec(cpu_.f, cpu_.b);
	} while (--cpu_.b);
	SET_HL(gb_, src);
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
	cpu_.a = mem_[wBlockPos + 3];
	SET_HL(gb_, wBlockPos + 3);
	SET_DE(gb_, wBackupBlockPos + 3);

	st(wDrawRoomWidthLoopVar, TILEMAP_WIDTH);
	do {
		queue_row_tiles_and_attributes();
		draw_room_column(wBGMapTileVRAM0Queue);
		st(rVBK, 1); // BANK("VRAM1")
		draw_room_column(wBGMapTileVRAM1Queue);
		xor_a();
		mem_[rVBK] = 0;
		mem_[wBGMapAddressQueueSize] = 0;
		mem_[wBGMapTileQueueSize] = 0;
		uint8_t lo = mem_[wBlockXPos + 1];
		bool carry = lo + 0x08 > 0xff;
		mem_[wBlockXPos + 1] = static_cast<uint8_t>(lo + 0x08);
		uint8_t hi = mem_[wBlockXPos + 0];
		mem_[wBlockXPos + 0] = static_cast<uint8_t>(hi + carry);
		uint8_t left = mem_[wDrawRoomWidthLoopVar];
		cpu_.f = flags::dec(flags::add(hi, 0, carry), left);
		cpu_.a = static_cast<uint8_t>(left - 1);
		mem_[wDrawRoomWidthLoopVar] = cpu_.a;
	} while (cpu_.a);

	for (int i = 0; i < 4; i++)
		mem_[wBlockYPos + i] = mem_[wBackupBlockPos + i];
	cpu_.a = mem_[wBackupBlockPos + 3];
	SET_HL(gb_, wBackupBlockPos + 3);
	SET_DE(gb_, wBlockYPos + 3);
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
	cpu_.f = flags::add(static_cast<uint8_t>(param - 0x10), 0x10);
	mem_[wMultiBlockParam] = param;
	cpu_.a = param & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	mem_[wcede] = cpu_.a;

	for (;;) {
		uint8_t dir = mem_[wMultiBlockParam] & 0x07;
		// dec a until z: 1 right, 2 left, 3 down, else up
		uint8_t a = dir;
		cpu_.f = flags::and_(a);
		int which = 0;
		for (int i = 1; i <= 3; i++) {
			cpu_.f = flags::dec(cpu_.f, a);
			a--;
			if (!a) {
				which = i;
				break;
			}
		}
		cpu_.a = a;
		switch (which) {
		case 0: { // the block above
			uint8_t row = mem_[wcedc + 0];
			cpu_.f = flags::inc(cpu_.f, row);
			cpu_.a = static_cast<uint8_t>(row + 1);
			cpu_.h = cpu_.a;
			mem_[wBlockPtr + 0] = cpu_.a;
			cpu_.f = flags::cp(cpu_.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
			cpu_.a = mem_[wcedb];
			if (zf()) {
				cpu_.h = sLevelBlockObjectMap >> 8;
				mem_[wBlockPtr + 0] = cpu_.h;
				uint8_t bank = mem_[wcedb];
				cpu_.f = flags::inc(cpu_.f, bank);
				cpu_.a = static_cast<uint8_t>(bank + 1);
			}
			mem_[wBlockPtrBank] = cpu_.a;
			cpu_.a = mem_[wcedc + 1];
			cpu_.l = cpu_.a;
			mem_[wBlockPtr + 1] = cpu_.a;
			break;
		}
		case 1: { // the block on the right
			mem_[wBlockPtrBank] = mem_[wcedb];
			cpu_.h = mem_[wcedc + 0];
			mem_[wBlockPtr + 0] = cpu_.h;
			uint8_t column = mem_[wcedc + 1];
			cpu_.f = flags::inc(cpu_.f, column);
			cpu_.a = static_cast<uint8_t>(column + 1);
			cpu_.l = cpu_.a;
			mem_[wBlockPtr + 1] = cpu_.a;
			break;
		}
		case 2: { // the block on the left
			mem_[wBlockPtrBank] = mem_[wcedb];
			uint8_t column = mem_[wcedc + 1];
			cpu_.f = flags::dec(cpu_.f, column);
			cpu_.l = static_cast<uint8_t>(column - 1);
			mem_[wBlockPtr + 1] = cpu_.l;
			cpu_.a = mem_[wcedc + 0];
			cpu_.h = cpu_.a;
			mem_[wBlockPtr + 0] = cpu_.a;
			break;
		}
		default: { // the block below
			uint8_t row = mem_[wcedc + 0];
			cpu_.f = flags::dec(cpu_.f, row);
			cpu_.a = static_cast<uint8_t>(row - 1);
			cpu_.h = cpu_.a;
			mem_[wBlockPtr + 0] = cpu_.a;
			cpu_.f = flags::cp(cpu_.a, 0x9f); // HIGH(STARTOF(SRAM)) - 1
			cpu_.a = mem_[wcedb];
			if (zf()) {
				cpu_.h = 0xbf; // HIGH(STARTOF(SRAM) + SIZEOF(SRAM)) - 1
				mem_[wBlockPtr + 0] = 0xbf;
				uint8_t bank = mem_[wcedb];
				cpu_.f = flags::dec(cpu_.f, bank);
				cpu_.a = static_cast<uint8_t>(bank - 1);
			}
			mem_[wBlockPtrBank] = cpu_.a;
			cpu_.a = mem_[wcedc + 1];
			cpu_.l = cpu_.a;
			mem_[wBlockPtr + 1] = cpu_.a;
			break;
		}
		}

		// .process_block
		cpu_.a = mem_[wMultiBlockParam] & (0xff ^ 0x07);
		cpu_.f = flags::and_(cpu_.a);
		mem_[wMultiBlockParam] = cpu_.a;
		routines_.farcall_ret(BANK_ProcessBlock, ADDR_ProcessBlock, ADDR_ProcessMultiBlock + 0x10d);
		cpu_.a = mem_[wMultiBlockParam] & 0x07;
		cpu_.f = flags::and_(cpu_.a);
		if (!cpu_.a)
			break;
		routines_.call_ret(ADDR_Func_bb85, ADDR_ProcessMultiBlock + 0x11a);
	}

	// .break
	xor_a();
	mem_[wMultiBlockParam] = 0;
	mem_[wIsFloorTransition] = 0;
	cpu_.a = mem_[wPendingSubState];
	mem_[wSubState] = cpu_.a;
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
	cpu_.a = mem_[wRoomTransitionParam] & ROOMTRANSITION_MASK;
	cpu_.f = flags::cp(cpu_.a, ROOMTRANSITION_GOLF);
	if (zf()) { // the golf minigame
		push_wram(0x03);
		cpu_.a = mem_[wAnimatedTilesFrameDuration];
		mem_[wTempAnimatedTilesFrameDuration] = cpu_.a;
		cpu_.a = mem_[wAnimatedTilesGfx];
		mem_[wTempAnimatedTilesGroup] = cpu_.a;
		pop_wram();
		cpu_.a = mem_[wSubState];
		mem_[wPendingSubState] = cpu_.a;
		SET_HL(gb_, wState);
		mem_[wState] = ST_GOLF;
		xor_a();
		mem_[wSubState] = 0;
		return;
	}

	// .not_golf
	routines_.call_ret(ADDR_DisableLCD, SRT(0x32));
	routines_.farcall_ret(BANK_DespawnAllObjects, ADDR_DespawnAllObjects, SRT(0x41));
	push_wram(BANK_wObjects); // BANK("Level Objects WRAM")
	routines_.farcall_ret(BANK_InitObjectVars_FromTransition, ADDR_InitObjectVars_FromTransition, SRT(0x57));
	pop_wram();

	cpu_.a = 0x00;
	sram_switch(0x00);
	cpu_.a = mem_[wRoomTransitionParam] & ROOMTRANSITION_MASK;
	cpu_.f = flags::cp(cpu_.a, ROOMTRANSITION_TELEPORT);
	if (zf()) {
		st(wWarioYPos + 0, 0x01);
		xor_a();
		mem_[wWarioXPos + 0] = 0;
	}
	// .not_teleport: up a block
	uint8_t y_lo = mem_[wWarioYPos + 1];
	cpu_.f = flags::sub(y_lo, 0x10);
	mem_[wWarioYPos + 1] = static_cast<uint8_t>(y_lo - 0x10);
	bool borrow = y_lo < 0x10;
	uint8_t y_hi = mem_[wWarioYPos + 0];
	cpu_.a = static_cast<uint8_t>(y_hi - borrow);
	mem_[wWarioYPos + 0] = cpu_.a;
	cpu_.b = static_cast<uint8_t>(cpu_.a << 4 | cpu_.a >> 4); // swap b
	cpu_.a = mem_[wWarioXPos + 0] | cpu_.b;
	cpu_.f = flags::or_(cpu_.a);
	mem_[wUnused_ca6c] = cpu_.a;

	// the next room's ID from Wario's position
	SET_HL(gb_, wWarioPos);
	routines_.call_ret(ADDR_GetNextLevelRoomID, SRT(0x94));

	clear({wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta, wCameraXDelta, wScrollUpEdge,
		wScrollDownEdge, wGroundShakeCounter, wSCYShake});
	cpu_.a = mem_[wLevelRoomID];
	mem_[wTempLevelRoomID] = cpu_.a;

	push_wram(GFX_RAM);
	routines_.call_ret(ADDR_LoadRoom, SRT(0xbd));
	pop_wram();

	routines_.call_ret(ADDR_FillBGMap0_With7f, SRT(0xc3));
	routines_.call_ret(ADDR_ClearVirtualOAM, SRT(0xc6));
	routines_.call_ret(ADDR_ClearParticles, SRT(0xc9));
	routines_.call_ret(ADDR_SetWarioPositionToSpawn, SRT(0xcc));
	routines_.call_ret(ADDR_SetInitialCameraScroll, SRT(0xcf));
	routines_.call_ret(ADDR_ApplyInitialCameraScroll, SRT(0xd2));

	cpu_.a = mem_[wRoomTransitionParam];
	cpu_.f = flags::bit(cpu_.f, cpu_.a, ROOMTRANSITIONF_RELOAD_OBJECTS_F);
	if (cpu_.a & (1 << ROOMTRANSITIONF_RELOAD_OBJECTS_F))
		routines_.call_ret(ADDR_ReloadLevelObjects, SRT(0xdc));
	// .no_object_reloading
	xor_a();
	mem_[wRoomTransitionParam] = 0;

	routines_.call_ret(ADDR_UpdateSwitchableBlocks, SRT(0xe3));
	call_in_sram1(ADDR_DrawRoom_FromStart, SRT(0xf2));
	call_in_sram1(ADDR_Func_896f, SRT(0x108));
	routines_.call_ret(ADDR_SetWarioScreenPos, SRT(0x112));

	ld_cp(wWarioState, WST_ENTERING_DOOR);
	if (zf()) {
		xor_a();
		mem_[wFrameDuration] = 0;
		mem_[wAnimationFrame] = 0;
		mem_[wWarioFramesetPtr + 0] = ADDR_Frameset_14d18 >> 8;
		st(wWarioFramesetPtr + 1, static_cast<uint8_t>(ADDR_Frameset_14d18));
		routines_.farcall_ret(OAM1, ADDR_UpdateWarioAnimation, SRT(0x139));
	}

	// .not_entering_door
	routines_.call_ret(ADDR_UpdateLevelMusic, SRT(0x13c));
	xor_a(); // FALSE
	mem_[wBlockFuncWarioFlag] = 0;
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
	xor_a();
	mem_[wIsIntangible] = 0;
	st(wSubState, 0x02);
	st(rLCDC, LCDC_DEFAULT);
}

// the level's first room (or the saved level's)
void RoomStart::from_level_start()
{
	xor_a(); // FALSE
	mem_[wIsBossBattle] = 0;

	if (!from_saved_level()) {
		clear({wLevelRoomID, wUnused_ca6c, wTempLevelRoomID});
		clear({wLevelEndScreen, wNumMusicalCoins, wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta,
			wCameraXDelta, wScrollUpEdge, wScrollDownEdge, wIsIntangible, wInvincibleCounter,
			wIsMinigameCleared, wIsRolling, wIsGettingOffLadder, wWarioAttributes, wSwitchStateUpdated,
			wLevelTime + 0, wLevelTime + 1, wLevelTime + 2, wBossState, wSwimVelIndex,
			wWaterSurfaceFloatingCounter});
		routines_.call_ret(ADDR_ClearTransformationValues, SRL(0x59));
		// with a as ClearTransformationValues leaves it (0)
		for (uint16_t addr : {wInvisibleFrame, wCameraRoomYScrollLimit, wIsOnSteppableObject, wGroundShakeCounter,
			     wSCYShake, wSwimmingDirectionInput, wAnimatedTilesFrameDuration, wRoomPalCycleDuration,
			     wRoomAnimatedTilesEnabled, wRoomAnimatedPalsEnabled, wWaterCurrent, wLastWaterCurrent,
			     wCurWaterCurrent})
			mem_[addr] = cpu_.a;

		// the level's variables and its objects'
		push_wram(BANK_wObjects);
		routines_.farcall_ret(BANK_InitObjectVars_FromLevelStart, ADDR_InitObjectVars_FromLevelStart, SRL(0x96));
		pop_wram();
	} else {
		// .skip_init_vars_and_objects
		cpu_.a = mem_[wTempLevelRoomID];
		mem_[wLevelRoomID] = cpu_.a;
		routines_.call_ret(ADDR_LoadWarioGfx, SRL(0xa4));
	}

	// .load_blocks_and_objects
	routines_.call_ret(ADDR_LoadLevelBlockMapAndObjects, SRL(0xa7));
	push_wram(GFX_RAM);
	routines_.call_ret(ADDR_LoadRoom, SRL(0xb1));
	pop_wram();

	if (!from_saved_level()) {
		routines_.call_ret(ADDR_SetWarioPositionToSpawn, SRL(0xbe));
		st(wDirection, DIRECTION_RIGHT);
		routines_.farcall_ret(BANK_SetState_Idling, ADDR_SetState_Idling, SRL(0xd2));
		SET_HL(gb_, ADDR_WarioDefaultPal);
		mem_[wWarioPalsPtr + 0] = cpu_.h;
		cpu_.a = cpu_.l;
		mem_[wWarioPalsPtr + 1] = cpu_.l;
		SET_DE(gb_, wTempPals2);
		cpu_.b = 2 * PAL_SIZE; // 2 palettes
		routines_.farcall_ret(BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, SRL(0xf1));
	} else {
		// .asm_883a
		cpu_.h = mem_[wWarioPalsPtr + 0];
		cpu_.a = mem_[wWarioPalsPtr + 1];
		cpu_.l = cpu_.a;
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
	ld_cp(wLevel, THE_TEMPLE);
	if (zf()) {
		xor_a();
		cpu_.a = mem_[wCameraSCY + 1];
		for (uint16_t addr : std::initializer_list<uint16_t>{wc08a, wHiddenFigureSCYShake, wSCY, rSCY})
			mem_[addr] = cpu_.a;
		cpu_.a = mem_[wCameraSCX + 1];
		for (uint16_t addr : std::initializer_list<uint16_t>{wc08c, wc08b, wSCX, rSCX})
			mem_[addr] = cpu_.a;
	}

	// .not_the_temple
	routines_.call_ret(ADDR_SetWarioScreenPos, SRL(0x197));
	xor_a(); // FALSE
	mem_[wBlockFuncWarioFlag] = 0;

	if (!from_saved_level()) {
		xor_a(); // FALSE
		mem_[wIsFloorTransition] = 0;
		push_wram(BANK_wObjects);
		// the objects updated twice
		routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRL(0x1bc));
		routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRL(0x1cb));
		pop_wram();
	} else {
		// .asm_8917
		st(wIsFloorTransition, TRUE);
		push_wram(BANK_wObjects);
		routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, SRL(0x1eb));
		pop_wram();
	}

	// .draw_objs
	xor_a(); // FALSE
	mem_[wIsFloorTransition] = 0;
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

PORT(StartRoom_FromTransition) { RoomStart(gb).from_transition(); }
PORT(StartRoom_FromLevelStart) { RoomStart(gb).from_level_start(); }
PORT(Func_896f) { RoomStart(gb).spawn_objects(); }
PORT(SetInitialCameraScroll) { RoomStart(gb).set_initial_camera_scroll(); }
PORT(ApplyInitialCameraScroll) { RoomStart(gb).apply_initial_camera_scroll(); }
PORT(SetWarioPositionToSpawn) { RoomStart(gb).set_wario_position_to_spawn(); }
PORT(DrawRoom_FromStart) { RoomStart(gb).draw_room(); }
PORT(ProcessMultiBlock) { RoomStart(gb).process_multi_block(); }
PORT_LOCAL_EX(ProcessMultiBlock, ProcessBlocks, 0, 0) { RoomStart(gb).process_blocks(); }
