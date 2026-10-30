// Port of src/engine/level/start_room.asm
#include "wario_states.h"

// c = 1 / ld a, [addr] / the SRAM bank (c) and page (a) of an object map
// row
static void obj_map_bank(GB *gb, uint16_t addr)
{
	gb->cpu.c = 0x01;
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	if (CF)
		return;
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
	alu_cp(gb, gb->cpu.a, 0xc0);
	if (CF)
		return;
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
}

// ld a, b / xor $01 / ld b, a
static void flip_b(GB *gb)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.b, 0x01);
	gb->cpu.b = gb->cpu.a;
}

// a = object index: spawns it if its block (hl) is in the spawn range
static void spawn_if_in_range(GB *gb)
{
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (!CF)
		return; // is Wario spawn block
	gb->cpu.c = gb->cpu.a; // object index
	gb->cpu.a = alu_dec(gb, rd8(gb, wc0c7));
	alu_cp(gb, gb->cpu.a, gb->cpu.l);
	if (CF)
		return;
	gb->cpu.a = rd8(gb, wc0c6);
	alu_cp(gb, gb->cpu.a, gb->cpu.l);
	if (!ZF && !CF)
		return;
	gb->cpu.a = alu_dec(gb, rd8(gb, wccec));
	uint8_t sub = ZF ? 0 : 0x20;
	gb->cpu.a = rd8(gb, wc0c4);
	if (sub)
		gb->cpu.a = alu_sub(gb, gb->cpu.a, sub);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.h);
	if (CF)
		return;
	gb->cpu.a = rd8(gb, wc0c5);
	if (sub)
		gb->cpu.a = alu_sub(gb, gb->cpu.a, sub);
	alu_cp(gb, gb->cpu.a, gb->cpu.h);
	if (!ZF && !CF)
		return;

	// .spawn
	gb_push16(gb, REG_DE(gb));
	gb_push16(gb, REG_BC(gb));
	CALL(Func_cf8);
	gb_push16(gb, REG_HL(gb));
	CALL(Func_d81);
	gb->cpu.d = gb->cpu.h;
	gb->cpu.e = gb->cpu.l;
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wObjects; // BANK("Level Objects WRAM")
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(SpawnObject);
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
	SET_HL(gb, gb_pop16(gb));
	SET_BC(gb, gb_pop16(gb));
	SET_DE(gb, gb_pop16(gb));
}

// Spawns the objects in the 16x16 blocks of the object map around
// wBlockXPos / wObjectSpawnBlockY.
PORT(Func_896f)
{
	// load block X pos to hPos
	uint16_t de = wBlockXPos + 1, hl = hXPosLo;
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl, gb->cpu.a);
		if (i < 3) {
			hl--;
			de--;
		}
	}
	SET_HL(gb, hl);
	SET_DE(gb, de);
	CALL(Func_cc0);

	obj_map_bank(gb, wObjectSpawnBlockY);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wccec, gb->cpu.a);
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = 1;
	wr8(gb, wUnused_ObjectParity, 1);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.d = 0x10;
	do {
		gb->cpu.e = 16 / 2;
		do {
			gb->cpu.a = alu_swap(gb, alu_and(gb, rd8(gb, REG_HL(gb)), 0xf0));
			if (!ZF)
				spawn_if_in_range(gb);
			flip_b(gb);
			gb->cpu.a = alu_and(gb, rd8(gb, REG_HL(gb)), 0x0f);
			if (!ZF)
				spawn_if_in_range(gb);
			gb->cpu.l = alu_inc(gb, gb->cpu.l);
			flip_b(gb);
			gb->cpu.e = alu_dec(gb, gb->cpu.e);
		} while (gb->cpu.e);
		gb->cpu.a = alu_sub(gb, gb->cpu.l, 0x08);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.h = alu_inc(gb, gb->cpu.h);
		gb->cpu.a = gb->cpu.h;
		alu_cp(gb, gb->cpu.a, 0xc0);
		if (ZF) {
			gb->cpu.h = (uint8_t)(sLevelBlockObjectMap >> 8);
			gb->cpu.a = alu_inc(gb, rd8(gb, wccec));
			wr8(gb, wccec, gb->cpu.a);
			sramswitch(gb, gb->cpu.a);
		}
		gb->cpu.d = alu_dec(gb, gb->cpu.d);
	} while (gb->cpu.d);
}

// ld a, [wCameraConfigFlags] / bit n, a
static bool cam_bit(GB *gb, int n)
{
	return ld_bit(gb, wCameraConfigFlags, n);
}

// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp v
static void cam_cp(GB *gb, uint8_t v)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, v);
}

// hl = scroll - offset (16-bit, big-endian in scroll), stored back; with
// de = -$20 added if the border bit is set
static void scroll_minus(GB *gb, uint16_t scroll, uint16_t pos, uint8_t offset, int border)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, pos + 1), offset);
	wr8(gb, scroll + 1, gb->cpu.a);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = alu_sbc(gb, rd8(gb, pos + 0), 0);
	wr8(gb, scroll + 0, gb->cpu.a);
	gb->cpu.h = gb->cpu.a;
	if (cam_bit(gb, border)) {
		SET_DE(gb, (uint16_t)-0x20);
		SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_DE(gb)));
	}
}

// Caps hl against the low limit: true (and the scroll is capped) if it's
// below it
static bool cap_low(GB *gb, uint16_t scroll, uint16_t limit, uint16_t limit_flag)
{
	gb->cpu.b = rd8(gb, limit);
	gb->cpu.a = gb->cpu.h;
	bool cap = alu_bit(gb, gb->cpu.a, 7);
	if (!cap) {
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (CF)
			cap = true;
		else if (ZF) {
			gb->cpu.a = gb->cpu.l;
			cap = !alu_and(gb, gb->cpu.a, gb->cpu.a);
		}
	}
	if (cap) {
		gb->cpu.a = gb->cpu.b;
		wr8(gb, scroll + 0, gb->cpu.a);
		st(gb, limit_flag, 1);
	}
	return cap;
}

// Caps hl = scroll + de (e = span, or wide_span with the border) against
// the high limit; false if it's below it
static bool cap_high(GB *gb, uint16_t scroll, uint16_t de, uint8_t wide_span, int border,
	uint16_t limit, uint16_t limit_flag)
{
	gb->cpu.a = rd8(gb, scroll + 0);
	gb->cpu.l = rd8(gb, scroll + 1);
	gb->cpu.h = gb->cpu.a;
	SET_DE(gb, de);
	if (cam_bit(gb, border))
		gb->cpu.e = wide_span;
	SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_DE(gb)));
	gb->cpu.b = rd8(gb, limit);
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (CF)
		return false;
	gb->cpu.a = alu_dec(gb, gb->cpu.b);
	wr8(gb, scroll + 0, gb->cpu.a);
	st(gb, limit_flag, (uint8_t)-1);
	return true;
}

// sets the initial camera scroll values
// when entering a new room or level starts
PORT(SetInitialCameraScroll)
{
	xor_a(gb);
	wr8(gb, wCameraRoomXScrollLimit, 0);
	cam_cp(gb, CAM_XSCROLL2 | CAM_TRANSITIONS);
	if (!ZF)
		alu_cp(gb, gb->cpu.a, CAM_YSCROLL);
	if (ZF) {
		// snap camera X scroll to Wario's x position
		gb->cpu.a = rd8(gb, wWarioXPos + 0);
		wr8(gb, wCameraSCX + 0, gb->cpu.a);
		st(gb, wCameraSCX + 1, 0x30);
	} else {
		uint16_t spacing = facing_right(gb) ? wCamRightSpacing : wCamLeftSpacing;
		gb->cpu.a = alu_sub(gb, rd8(gb, spacing), 8);
		gb->cpu.b = gb->cpu.a;
		scroll_minus(gb, wCameraSCX, wWarioXPos, gb->cpu.b, CAM_BORDER_LEFT_F);
		if (!cap_low(gb, wCameraSCX, wRoomLeftXLimit, wCameraRoomXScrollLimit))
			cap_high(gb, wCameraSCX, (uint16_t)(sLevelBlockObjectMap >> 8), 0xc0,
				CAM_BORDER_RIGHT_F, wRoomRightXLimit, wCameraRoomXScrollLimit);
	}

	// .check_if_saved_level
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0b00111100);
	if (!ZF) {
		cam_cp(gb, CAM_TRANSITIONS);
		if (!CF)
			return; // no need to handle Y scroll
	}

	// .handle_cam_y_scroll
	xor_a(gb);
	wr8(gb, wCameraRoomYScrollLimit, 0);
	scroll_minus(gb, wCameraSCY, wWarioYPos, 0x60, CAM_BORDER_UP_F);
	if (cap_low(gb, wCameraSCY, wRoomUpperYLimit, wCameraRoomYScrollLimit))
		return;
	if (cap_high(gb, wCameraSCY, 0x90, 0xb0, CAM_BORDER_DOWN_F, wRoomLowerYLimit,
		wCameraRoomYScrollLimit))
		return;

	// .check_transitions
	cam_cp(gb, CAM_TRANSITIONS);
	if (CF)
		return; // not CAM_TRANSITIONS
	ld_cp(gb, wWarioYPos + 1, 0x80);
	if (!CF) {
		st(gb, wCameraSCY + 1, 0x68);
		return;
	}
	gb->cpu.b = rd8(gb, wRoomUpperYLimit);
	gb->cpu.a = alu_dec(gb, rd8(gb, wWarioYPos + 0));
	wr8(gb, wCameraSCY + 0, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (CF) {
		// .cap_up_side
		gb->cpu.a = gb->cpu.b;
		wr8(gb, wCameraSCY + 0, gb->cpu.a);
		st(gb, wCameraRoomYScrollLimit, 1);
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, 7)) {
		st(gb, wCameraSCY + 1, 0xe8);
		return;
	}
	xor_a(gb);
	wr8(gb, wCameraSCY + 0, 0);
	st(gb, wCameraRoomYScrollLimit, 1);
}

// ld a, [wCameraConfigFlags] / bit n, a / ld a, v1 / jr z / ld a, v2
static uint8_t by_border(GB *gb, int bit, uint8_t clear, uint8_t set)
{
	return cam_bit(gb, bit) ? set : clear;
}

// a = the scroll register's value: [scroll + 1], rSC*, wSC*
static void set_scroll(GB *gb, uint16_t scroll, uint16_t reg, uint16_t wreg, bool store)
{
	if (store)
		wr8(gb, scroll + 1, gb->cpu.a);
	wr8(gb, reg, gb->cpu.a);
	wr8(gb, wreg, gb->cpu.a);
}

// applies the camera scroll set in SetInitialCameraScroll
// to the actual scroll registers rSCY and rSCX
PORT(ApplyInitialCameraScroll)
{
	gb->cpu.a = rd8(gb, wCameraSCY + 0);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCameraSCY + 1);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCameraRoomYScrollLimit);
	alu_cp(gb, gb->cpu.a, 1);
	if (ZF) {
		// .on_upper_edge
		gb->cpu.a = by_border(gb, CAM_BORDER_UP_F, 0x00, 0x20);
		set_scroll(gb, wCameraSCY, rSCY, wSCY, true);
		wr8(gb, wBlockYPos + 1, gb->cpu.a);
		gb->cpu.a = gb->cpu.h;
		wr8(gb, wBlockYPos + 0, gb->cpu.a);
	} else {
		alu_cp(gb, gb->cpu.a, (uint8_t)-1);
		if (ZF) {
			// .on_lower_edge
			gb->cpu.a = by_border(gb, CAM_BORDER_DOWN_F, 0x68, 0x48);
			set_scroll(gb, wCameraSCY, rSCY, wSCY, true);
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
			wr8(gb, wBlockYPos + 1, gb->cpu.a);
			gb->cpu.a = gb->cpu.h;
			wr8(gb, wBlockYPos + 0, gb->cpu.a);
		} else {
			gb->cpu.a = gb->cpu.l;
			set_scroll(gb, wCameraSCY, rSCY, wSCY, false);
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x18);
			wr8(gb, wBlockYPos + 1, gb->cpu.a);
			gb->cpu.a = alu_sbc(gb, gb->cpu.h, 0);
			wr8(gb, wBlockYPos + 0, gb->cpu.a);
		}
	}

	// .cam_x_scroll
	gb->cpu.a = rd8(gb, wCameraSCX + 0);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCameraSCX + 1);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCameraRoomXScrollLimit);
	alu_cp(gb, gb->cpu.a, 1);
	if (ZF) {
		// .on_left_edge
		gb->cpu.a = by_border(gb, CAM_BORDER_LEFT_F, 0x00, 0x20);
		set_scroll(gb, wCameraSCX, rSCX, wSCX, true);
		wr8(gb, wBlockXPos + 1, gb->cpu.a);
		gb->cpu.a = gb->cpu.h;
		wr8(gb, wBlockXPos + 0, gb->cpu.a);
		return;
	}
	alu_cp(gb, gb->cpu.a, (uint8_t)-1);
	if (ZF) {
		// .on_right_edge
		gb->cpu.a = by_border(gb, CAM_BORDER_RIGHT_F, 0x60, 0x40);
		set_scroll(gb, wCameraSCX, rSCX, wSCX, true);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x28);
		wr8(gb, wBlockXPos + 1, gb->cpu.a);
		gb->cpu.a = gb->cpu.h;
		wr8(gb, wBlockXPos + 0, gb->cpu.a);
		return;
	}
	gb->cpu.a = gb->cpu.l;
	set_scroll(gb, wCameraSCX, rSCX, wSCX, false);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x10);
	wr8(gb, wBlockXPos + 1, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, gb->cpu.h, 0x00);
	wr8(gb, wBlockXPos + 0, gb->cpu.a);
	if (!alu_bit(gb, gb->cpu.a, 7))
		return;
	xor_a(gb);
	wr8(gb, wBlockXPos + 1, 0);
	wr8(gb, wBlockXPos + 0, 0);
}

// Sets hPos (and wWarioPos) to the block of wWarioSpawnPos, then searches the
// object map for the spawn block ($f) to get the exact position.
PORT(SetWarioPositionToSpawn)
{
	xor_a(gb);
	wr8(gb, hXPosLo, 0);
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioSpawnPos), 0x0f);
	wr8(gb, hXPosHi, gb->cpu.a);
	xor_a(gb);
	wr8(gb, hYPosLo, 0);
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioSpawnPos), 0xf0);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	wr8(gb, hYPosHi, gb->cpu.a);
	SET_HL(gb, hYPosHi);
	CALL(Func_cc0);

	obj_map_bank(gb, wObjectSpawnBlockY);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wccec, gb->cpu.a);
	sramswitch(gb, gb->cpu.a);

	for (;;) {
		gb->cpu.e = 16 / 2;
		bool found = false;
		do {
			gb->cpu.a = alu_swap(gb, alu_and(gb, rd8(gb, REG_HL(gb)), 0xf0));
			alu_cp(gb, gb->cpu.a, 0x0f);
			if (!CF) {
				found = true;
				break;
			}
			flip_b(gb);
			gb->cpu.a = alu_and(gb, rd8(gb, REG_HL(gb)), 0x0f);
			alu_cp(gb, gb->cpu.a, 0x0f);
			if (!CF) {
				found = true;
				break;
			}
			gb->cpu.l = alu_inc(gb, gb->cpu.l);
			flip_b(gb);
			gb->cpu.e = alu_dec(gb, gb->cpu.e);
		} while (gb->cpu.e);
		if (found)
			break;
		gb->cpu.a = alu_sub(gb, gb->cpu.l, 0x08);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.h = alu_inc(gb, gb->cpu.h);
		gb->cpu.a = gb->cpu.h;
		alu_cp(gb, gb->cpu.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		if (ZF) {
			gb->cpu.h = (uint8_t)(sLevelBlockObjectMap >> 8);
			gb->cpu.a = alu_inc(gb, rd8(gb, wccec));
			wr8(gb, wccec, gb->cpu.a);
			sramswitch(gb, gb->cpu.a);
		}
	}

	// .found_spawn_block
	gb->cpu.c = gb->cpu.a;
	CALL(Func_cf8);
	// update_pos
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hPos + i);
		wr8(gb, wWarioPos + i, gb->cpu.a);
	}
	SET_HL(gb, hPos + 3);
	SET_DE(gb, wWarioPos + 3);

	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	if (CF)
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0x3c);
	if (!ZF)
		return;
	CALL(UpdateFloor);
}

// ld a, [hli] / ld [de], a, down a column of the BG map (wrapping from
// v0BGMap1 back to v0BGMap0), b = TILEMAP_HEIGHT times
static void draw_room_col(GB *gb, uint16_t queue)
{
	gb->cpu.d = rd8(gb, wBGPtr + 0);
	gb->cpu.a = gb->cpu.d;
	gb->cpu.e = rd8(gb, wBGPtr + 1);
	gb->cpu.a = gb->cpu.e;
	SET_HL(gb, queue);
	gb->cpu.b = TILEMAP_HEIGHT;
	do {
		uint16_t hl = REG_HL(gb);
		gb->cpu.a = rd8(gb, hl);
		SET_HL(gb, hl + 1);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
		gb->cpu.d = gb->cpu.a;
		alu_cp(gb, gb->cpu.a, (uint8_t)(v0BGMap1 >> 8));
		if (ZF)
			gb->cpu.d = (uint8_t)(v0BGMap0 >> 8);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
}

// DrawRoom_FromStart.QueueRowTilesAndAttributes
static void queue_row_tiles_and_attributes(GB *gb)
{
	gb_call(gb, ADDR_GetBlockBGPtr);
	gb->cpu.a = rd8(gb, wBGMapAddressQueueSize);
	gb->cpu.b = gb->cpu.a;
	SET_DE(gb, wBGMapAddressQueue);
	gb->cpu.a = alu_add(gb, gb->cpu.e, gb->cpu.b);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.b = TILEMAP_HEIGHT;
	do {
		gb->cpu.a = gb->cpu.h;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = gb->cpu.l;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		SET_HL(gb, alu_add16(gb, REG_HL(gb), TILEMAP_WIDTH));
		gb->cpu.a = alu_and(gb, gb->cpu.h, 0xfb);
		gb->cpu.h = gb->cpu.a;
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);

	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapAddressQueueSize), 2 * TILEMAP_HEIGHT);
	wr8(gb, wBGMapAddressQueueSize, gb->cpu.a);

	SET_HL(gb, wBlockPos);
	CALL(GetBlockPtr);
	gb->cpu.a = rd8(gb, wBlockPtrBank);
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, wBlockXPos + 1), 0x08);
	uint16_t hl = REG_HL(gb);
	if (!ZF) {
		gb_push16(gb, hl);
		FARCALL(QueueBlockTilesAlongRow_OddColumn);
		SET_HL(gb, gb_pop16(gb));
		gb->cpu.a = rd8(gb, wBlockPtrBank);
		sramswitch(gb, gb->cpu.a);
		FARCALL(QueueBlockAttributesAlongRow_OddColumn);
	} else {
		gb_push16(gb, hl);
		FARCALL(QueueBlockTilesAlongRow_EvenColumn);
		SET_HL(gb, gb_pop16(gb));
		gb->cpu.a = rd8(gb, wBlockPtrBank);
		sramswitch(gb, gb->cpu.a);
		FARCALL(QueueBlockAttributesAlongRow_EvenColumn);
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapTileQueueSize), TILEMAP_WIDTH);
	wr8(gb, wBGMapTileQueueSize, gb->cpu.a);
}

// loads all room tiles that are visible in the BGMap, row by row
PORT(DrawRoom_FromStart)
{
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wBlockPos + i);
		wr8(gb, wBackupBlockPos + i, gb->cpu.a);
	}
	SET_HL(gb, wBlockPos + 3);
	SET_DE(gb, wBackupBlockPos + 3);

	gb->cpu.a = TILEMAP_WIDTH;
	wr8(gb, wDrawRoomWidthLoopVar, gb->cpu.a);
	do {
		queue_row_tiles_and_attributes(gb);
		draw_room_col(gb, wBGMapTileVRAM0Queue);
		gb->cpu.a = 1; // BANK("VRAM1")
		wr8(gb, rVBK, gb->cpu.a);
		draw_room_col(gb, wBGMapTileVRAM1Queue);
		xor_a(gb);
		wr8(gb, rVBK, 0);
		wr8(gb, wBGMapAddressQueueSize, 0);
		wr8(gb, wBGMapTileQueueSize, 0);
		gb->cpu.a = alu_add(gb, rd8(gb, wBlockXPos + 1), 0x08);
		wr8(gb, wBlockXPos + 1, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, wBlockXPos + 0), 0x00);
		wr8(gb, wBlockXPos + 0, gb->cpu.a);
		gb->cpu.a = alu_dec(gb, rd8(gb, wDrawRoomWidthLoopVar));
		wr8(gb, wDrawRoomWidthLoopVar, gb->cpu.a);
	} while (gb->cpu.a);

	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wBackupBlockPos + i);
		wr8(gb, wBlockYPos + i, gb->cpu.a);
	}
	SET_HL(gb, wBackupBlockPos + 3);
	SET_DE(gb, wBlockYPos + 3);
}

// --- multi-blocks ---

#define PMB_PROCESS_BLOCKS (ADDR_ProcessMultiBlock + 0x63) // .ProcessBlocks

// push_wram BANK("Level Objects WRAM") / farcall / pop_wram
static void farcall_obj_wram(GB *gb, int bank, uint16_t addr)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x01;
	wr8(gb, rWBK, 0x01);
	port_farcall(gb, bank, addr);
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// Processes the blocks of a multi-block, then does a normal frame's
// drawing.
PORT(ProcessMultiBlock)
{
	gb_call_ret(gb, PMB_PROCESS_BLOCKS, ADDR_ProcessMultiBlock + 0x03);
	FARCALL(UpdateParticles);
	farcall_obj_wram(gb, BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority);
	CALL(DrawWario);
	farcall_obj_wram(gb, BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority);
	CALL(ClearUnusedVirtualOAM);
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	sramswitch(gb, BANK_sLevelBlockObjectMap);
	gb->cpu.ime = false; // di
	gb_call(gb, ADDR_SetTileQueueProcessingFunctions); // same bank: whatever is mapped
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

// Processes the block next to wcedb:wcedc in each direction in
// wMultiBlockParam's low 3 bits, until ProcessBlock leaves none.
PORT_LOCAL(ProcessMultiBlock, ProcessBlocks)
{
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, wMultiBlockParam), 0x0f), 0x10);
	wr8(gb, wMultiBlockParam, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x08);
	wr8(gb, wcede, gb->cpu.a);

	for (;;) {
		gb->cpu.a = alu_and(gb, rd8(gb, wMultiBlockParam), 0x07);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		uint8_t dir = 0; // up
		if (gb->cpu.f & FLAG_Z) {
			dir = 1; // right
		} else {
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z) {
				dir = 2; // left
			} else {
				gb->cpu.a = alu_dec(gb, gb->cpu.a);
				if (gb->cpu.f & FLAG_Z)
					dir = 3; // down
			}
		}
		switch (dir) {
		case 0: // the block above
			gb->cpu.a = alu_inc(gb, rd8(gb, wcedc + 0));
			gb->cpu.h = gb->cpu.a;
			wr8(gb, wBlockPtr + 0, gb->cpu.a);
			alu_cp(gb, gb->cpu.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
			gb->cpu.a = rd8(gb, wcedb);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = (uint8_t)(sLevelBlockObjectMap >> 8);
				gb->cpu.h = gb->cpu.a;
				wr8(gb, wBlockPtr + 0, gb->cpu.a);
				gb->cpu.a = alu_inc(gb, rd8(gb, wcedb));
			}
			wr8(gb, wBlockPtrBank, gb->cpu.a);
			gb->cpu.a = rd8(gb, wcedc + 1);
			gb->cpu.l = gb->cpu.a;
			wr8(gb, wBlockPtr + 1, gb->cpu.a);
			break;
		case 1: // the block on the right
			gb->cpu.a = rd8(gb, wcedb);
			wr8(gb, wBlockPtrBank, gb->cpu.a);
			gb->cpu.a = rd8(gb, wcedc + 0);
			gb->cpu.h = gb->cpu.a;
			wr8(gb, wBlockPtr + 0, gb->cpu.a);
			gb->cpu.a = alu_inc(gb, rd8(gb, wcedc + 1));
			gb->cpu.l = gb->cpu.a;
			wr8(gb, wBlockPtr + 1, gb->cpu.a);
			break;
		case 2: // the block on the left
			gb->cpu.a = rd8(gb, wcedb);
			wr8(gb, wBlockPtrBank, gb->cpu.a);
			gb->cpu.a = alu_dec(gb, rd8(gb, wcedc + 1));
			gb->cpu.l = gb->cpu.a;
			wr8(gb, wBlockPtr + 1, gb->cpu.a);
			gb->cpu.a = rd8(gb, wcedc + 0);
			gb->cpu.h = gb->cpu.a;
			wr8(gb, wBlockPtr + 0, gb->cpu.a);
			break;
		default: // the block below
			gb->cpu.a = alu_dec(gb, rd8(gb, wcedc + 0));
			gb->cpu.h = gb->cpu.a;
			wr8(gb, wBlockPtr + 0, gb->cpu.a);
			alu_cp(gb, gb->cpu.a, 0x9f); // HIGH(STARTOF(SRAM)) - 1
			gb->cpu.a = rd8(gb, wcedb);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = 0xbf; // HIGH(STARTOF(SRAM) + SIZEOF(SRAM)) - 1
				gb->cpu.h = gb->cpu.a;
				wr8(gb, wBlockPtr + 0, gb->cpu.a);
				gb->cpu.a = alu_dec(gb, rd8(gb, wcedb));
			}
			wr8(gb, wBlockPtrBank, gb->cpu.a);
			gb->cpu.a = rd8(gb, wcedc + 1);
			gb->cpu.l = gb->cpu.a;
			wr8(gb, wBlockPtr + 1, gb->cpu.a);
			break;
		}

		// .process_block
		gb->cpu.a = alu_and(gb, rd8(gb, wMultiBlockParam), 0xff ^ 0x07);
		wr8(gb, wMultiBlockParam, gb->cpu.a);
		FARCALL_RET(ProcessBlock, ADDR_ProcessMultiBlock + 0x10d);
		gb->cpu.a = alu_and(gb, rd8(gb, wMultiBlockParam), 0x07);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb_call_ret(gb, ADDR_Func_bb85, ADDR_ProcessMultiBlock + 0x11a);
	}

	// .break
	xor_a(gb);
	wr8(gb, wMultiBlockParam, 0);
	wr8(gb, wIsFloorTransition, 0);
	gb->cpu.a = rd8(gb, wPendingSubState);
	wr8(gb, wSubState, gb->cpu.a);
}

#define SRT(off) ((uint16_t)(ADDR_StartRoom_FromTransition + (off)))

// push_wram bank
static void sr_push_wram(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = bank;
	wr8(gb, rWBK, gb->cpu.a);
}

// pop_wram
static void sr_pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// ld a, [wSRAMBank] / push af / ld a, BANK("SRAM1") / sramswitch /
// call addr / pop af / sramswitch
static void call_in_sram1(GB *gb, uint16_t addr, uint16_t ret)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
	sramswitch(gb, gb->cpu.a);
	gb_call_ret(gb, addr, ret);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

PORT(StartRoom_FromTransition)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wRoomTransitionParam), ROOMTRANSITION_MASK);
	alu_cp(gb, gb->cpu.a, ROOMTRANSITION_GOLF);
	if (ZF) {
		// enter Golf minigame
		sr_push_wram(gb, 0x03);
		gb->cpu.a = rd8(gb, wAnimatedTilesFrameDuration);
		wr8(gb, wTempAnimatedTilesFrameDuration, gb->cpu.a);
		gb->cpu.a = rd8(gb, wAnimatedTilesGfx);
		wr8(gb, wTempAnimatedTilesGroup, gb->cpu.a);
		sr_pop_wram(gb);

		gb->cpu.a = rd8(gb, wSubState);
		wr8(gb, wPendingSubState, gb->cpu.a);
		SET_HL(gb, wState);
		wr8(gb, wState, ST_GOLF);
		xor_a(gb);
		wr8(gb, wSubState, 0);
		return;
	}

	// .not_golf
	gb_call_ret(gb, ADDR_DisableLCD, SRT(0x32));
	FARCALL_RET(DespawnAllObjects, SRT(0x41));

	sr_push_wram(gb, BANK_wObjects); // BANK("Level Objects WRAM")
	FARCALL_RET(InitObjectVars_FromTransition, SRT(0x57));
	sr_pop_wram(gb);

	gb->cpu.a = 0x00;
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, wRoomTransitionParam), ROOMTRANSITION_MASK);
	alu_cp(gb, gb->cpu.a, ROOMTRANSITION_TELEPORT);
	if (ZF) {
		gb->cpu.a = 0x01;
		wr8(gb, wWarioYPos + 0, gb->cpu.a);
		xor_a(gb);
		wr8(gb, wWarioXPos + 0, 0);
	}
	// .not_teleport
	gb->cpu.a = alu_sub(gb, rd8(gb, wWarioYPos + 1), 0x10);
	wr8(gb, wWarioYPos + 1, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wWarioYPos + 0), 0x00);
	wr8(gb, wWarioYPos + 0, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.b = alu_swap(gb, gb->cpu.b);
	gb->cpu.a = alu_or(gb, rd8(gb, wWarioXPos + 0), gb->cpu.b);
	wr8(gb, wUnused_ca6c, gb->cpu.a);

	// get next room ID from Wario's position
	SET_HL(gb, wWarioPos);
	gb_call_ret(gb, ADDR_GetNextLevelRoomID, SRT(0x94));

	static const uint16_t zero[] = {wFloorTransitionDir, wFloorTransitionTimer, wCameraYDelta,
		wCameraXDelta, wScrollUpEdge, wScrollDownEdge, wGroundShakeCounter, wSCYShake, 0};
	clear(gb, zero);
	gb->cpu.a = rd8(gb, wLevelRoomID);
	wr8(gb, wTempLevelRoomID, gb->cpu.a);

	sr_push_wram(gb, BANK_w3d280); // BANK("GFX RAM")
	gb_call_ret(gb, ADDR_LoadRoom, SRT(0xbd));
	sr_pop_wram(gb);

	gb_call_ret(gb, ADDR_FillBGMap0_With7f, SRT(0xc3));
	gb_call_ret(gb, ADDR_ClearVirtualOAM, SRT(0xc6));
	gb_call_ret(gb, ADDR_ClearParticles, SRT(0xc9));
	gb_call_ret(gb, ADDR_SetWarioPositionToSpawn, SRT(0xcc));

	gb_call_ret(gb, ADDR_SetInitialCameraScroll, SRT(0xcf));
	gb_call_ret(gb, ADDR_ApplyInitialCameraScroll, SRT(0xd2));

	if (ld_bit(gb, wRoomTransitionParam, ROOMTRANSITIONF_RELOAD_OBJECTS_F))
		gb_call_ret(gb, ADDR_ReloadLevelObjects, SRT(0xdc));
	// .no_object_reloading
	xor_a(gb);
	wr8(gb, wRoomTransitionParam, 0);

	gb_call_ret(gb, ADDR_UpdateSwitchableBlocks, SRT(0xe3));

	call_in_sram1(gb, ADDR_DrawRoom_FromStart, SRT(0xf2));
	call_in_sram1(gb, ADDR_Func_896f, SRT(0x108));
	gb_call_ret(gb, ADDR_SetWarioScreenPos, SRT(0x112));

	ld_cp(gb, wWarioState, WST_ENTERING_DOOR);
	if (ZF) {
		xor_a(gb);
		wr8(gb, wFrameDuration, 0);
		wr8(gb, wAnimationFrame, 0);
		set_frameset(gb, ADDR_Frameset_14d18);
		port_farcall_ret(gb, OAM1, ADDR_UpdateWarioAnimation, SRT(0x139));
	}

	// .not_entering_door
	gb_call_ret(gb, ADDR_UpdateLevelMusic, SRT(0x13c));
	xor_a(gb); // FALSE
	wr8(gb, wBlockFuncWarioFlag, 0);
	wr8(gb, wIsFloorTransition, 0);
	sr_push_wram(gb, BANK_wObjects);
	FARCALL_RET(UpdateObjects, SRT(0x159));
	FARCALL_RET(UpdateObjects, SRT(0x168));
	sr_pop_wram(gb);
	sr_push_wram(gb, BANK_wObjects);
	FARCALL_RET(DrawObjects_NoPriority, SRT(0x181));
	sr_pop_wram(gb);
	gb_call_ret(gb, ADDR_DrawWario, SRT(0x187));
	sr_push_wram(gb, BANK_wObjects);
	FARCALL_RET(DrawObjects_WithPriority, SRT(0x19d));
	sr_pop_wram(gb);
	xor_a(gb);
	wr8(gb, wIsIntangible, 0);
	gb->cpu.a = 0x02;
	wr8(gb, wSubState, gb->cpu.a);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
}

#define SRL(off) ((uint16_t)(ADDR_StartRoom_FromLevelStart + (off)))

// ld a, [wceef] / and %00111100: true (nz) when from a saved level
static bool from_saved_level(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wceef), 0x3c);
	return !ZF;
}

PORT(StartRoom_FromLevelStart)
{
	xor_a(gb); // FALSE
	wr8(gb, wIsBossBattle, 0);

	// if from saved level, don't init start level vars
	if (!from_saved_level(gb)) {
		static const uint16_t zero1[] = {wLevelRoomID, wUnused_ca6c, wTempLevelRoomID, 0};
		clear(gb, zero1);
		static const uint16_t zero2[] = {wLevelEndScreen, wNumMusicalCoins, wFloorTransitionDir,
			wFloorTransitionTimer, wCameraYDelta, wCameraXDelta, wScrollUpEdge, wScrollDownEdge,
			wIsIntangible, wInvincibleCounter, wIsMinigameCleared, wIsRolling,
			wIsGettingOffLadder, wWarioAttributes, wSwitchStateUpdated, wLevelTime + 0,
			wLevelTime + 1, wLevelTime + 2, wBossState, wSwimVelIndex,
			wWaterSurfaceFloatingCounter, 0};
		clear(gb, zero2); // xor a ; unnecessary
		gb_call_ret(gb, ADDR_ClearTransformationValues, SRL(0x59));
		// assumes ClearTransformationValues returns with a = 0
		static const uint16_t after[] = {wInvisibleFrame, wCameraRoomYScrollLimit,
			wIsOnSteppableObject, wGroundShakeCounter, wSCYShake, wSwimmingDirectionInput,
			wAnimatedTilesFrameDuration, wRoomPalCycleDuration, wRoomAnimatedTilesEnabled,
			wRoomAnimatedPalsEnabled, wWaterCurrent, wLastWaterCurrent, wCurWaterCurrent, 0};
		for (const uint16_t *p = after; *p; p++)
			wr8(gb, *p, gb->cpu.a);

		// init variables from level start
		// as well as everything related to objects
		sr_push_wram(gb, BANK_wObjects);
		FARCALL_RET(InitObjectVars_FromLevelStart, SRL(0x96));
		sr_pop_wram(gb);
	} else {
		// .skip_init_vars_and_objects
		gb->cpu.a = rd8(gb, wTempLevelRoomID);
		wr8(gb, wLevelRoomID, gb->cpu.a);
		gb_call_ret(gb, ADDR_LoadWarioGfx, SRL(0xa4));
	}

	// .load_blocks_and_objects
	gb_call_ret(gb, ADDR_LoadLevelBlockMapAndObjects, SRL(0xa7));

	sr_push_wram(gb, BANK_w3d280); // BANK("GFX RAM")
	gb_call_ret(gb, ADDR_LoadRoom, SRL(0xb1));
	sr_pop_wram(gb);

	if (!from_saved_level(gb)) {
		gb_call_ret(gb, ADDR_SetWarioPositionToSpawn, SRL(0xbe));
		st(gb, wDirection, DIRECTION_RIGHT);
		FARCALL_RET(SetState_Idling, SRL(0xd2));
		SET_HL(gb, ADDR_WarioDefaultPal);
		gb->cpu.a = gb->cpu.h;
		wr8(gb, wWarioPalsPtr + 0, gb->cpu.a);
		gb->cpu.a = gb->cpu.l;
		wr8(gb, wWarioPalsPtr + 1, gb->cpu.a);
		SET_DE(gb, wTempPals2);
		gb->cpu.b = 2 * PAL_SIZE; // 2 palettes
		port_farcall_ret(gb, BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, SRL(0xf1));
	} else {
		// .asm_883a
		gb->cpu.a = rd8(gb, wWarioPalsPtr + 0);
		gb->cpu.h = gb->cpu.a;
		gb->cpu.a = rd8(gb, wWarioPalsPtr + 1);
		gb->cpu.l = gb->cpu.a;
		SET_DE(gb, wTempPals2);
		gb->cpu.b = 2 * PAL_SIZE; // 2 palettes
		port_farcall_ret(gb, BANK_WarioDefaultPal, ADDR_CopyHLToDE_Short, SRL(0x10f));

		SET_HL(gb, wcaa1);
		SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
		gb->cpu.b = 4 * PAL_SIZE; // 4 palettes
		gb_call_ret(gb, ADDR_CopyHLToDE, SRL(0x11a));
	}

	// .asm_8861
	SET_HL(gb, ADDR_Pals_d1fe);
	SET_DE(gb, wTempPals2 + 2 * PAL_SIZE);
	gb->cpu.b = 1 * PAL_SIZE; // 1 palettes
	port_farcall_ret(gb, BANK_Pals_d1fe, ADDR_CopyHLToDE_Short, SRL(0x131));

	gb_call_ret(gb, ADDR_SetInitialCameraScroll, SRL(0x134));
	gb_call_ret(gb, ADDR_ApplyInitialCameraScroll, SRL(0x137));

	gb_call_ret(gb, ADDR_VBlank_Level, SRL(0x13a));
	gb_call_ret(gb, ADDR_SetTileQueueProcessingFunctions, SRL(0x13d));

	call_in_sram1(gb, ADDR_DrawRoom_FromStart, SRL(0x14c));

	if (!from_saved_level(gb))
		call_in_sram1(gb, ADDR_Func_896f, SRL(0x169));

	// .asm_88b7
	gb->cpu.a = rd8(gb, wLevel);
	alu_cp(gb, gb->cpu.a, THE_TEMPLE);
	if (ZF) {
		xor_a(gb);
		gb->cpu.a = rd8(gb, wCameraSCY + 1);
		wr8(gb, wc08a, gb->cpu.a);
		wr8(gb, wHiddenFigureSCYShake, gb->cpu.a);
		wr8(gb, wSCY, gb->cpu.a);
		wr8(gb, rSCY, gb->cpu.a);
		gb->cpu.a = rd8(gb, wCameraSCX + 1);
		wr8(gb, wc08c, gb->cpu.a);
		wr8(gb, wc08b, gb->cpu.a);
		wr8(gb, wSCX, gb->cpu.a);
		wr8(gb, rSCX, gb->cpu.a);
	}

	// .not_the_temple
	gb_call_ret(gb, ADDR_SetWarioScreenPos, SRL(0x197));
	xor_a(gb); // FALSE
	wr8(gb, wBlockFuncWarioFlag, 0);

	if (!from_saved_level(gb)) {
		xor_a(gb); // FALSE
		wr8(gb, wIsFloorTransition, 0);
		sr_push_wram(gb, BANK_wObjects);
		// for some reason updates objects twice?
		FARCALL_RET(UpdateObjects, SRL(0x1bc));
		FARCALL_RET(UpdateObjects, SRL(0x1cb));
		sr_pop_wram(gb);
	} else {
		// .asm_8917
		st(gb, wIsFloorTransition, TRUE);
		sr_push_wram(gb, BANK_wObjects);
		FARCALL_RET(UpdateObjects, SRL(0x1eb));
		sr_pop_wram(gb);
	}

	// .draw_objs
	xor_a(gb); // FALSE
	wr8(gb, wIsFloorTransition, 0);

	sr_push_wram(gb, BANK_wObjects);
	FARCALL_RET(DrawObjects_NoPriority, SRL(0x208));
	sr_pop_wram(gb);

	gb_call_ret(gb, ADDR_DrawWario, SRL(0x20e));

	sr_push_wram(gb, BANK_wObjects);
	FARCALL_RET(DrawObjects_WithPriority, SRL(0x224));
	sr_pop_wram(gb);
}
