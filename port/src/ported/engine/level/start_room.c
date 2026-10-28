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
