// Port of src/engine/title/start_menu_selection.asm
#include "ported.h"

// .update_obj: a = new selection, bc = frameset
static void update_obj(GB *gb)
{
	wr8(gb, wStartMenuSelection, gb->cpu.a);
	uint16_t hl = wMenuObj1FramesetOffset;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	CALL(UpdateObjAnim);
}

static void play_scrolling_sfx(GB *gb)
{
	play_sfx(gb, SFX_0E2);
}

static void handle_input(GB *gb)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		play_sfx(gb, SFX_SELECTION);
		SET_HL(gb, wStartMenuSelection);
		gb->cpu.a = alu_and(gb, rd8(gb, wStartMenuSelection), 0x0f);
		alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA_NO);
		if (gb->cpu.f & FLAG_Z) {
			SET_BC(gb, ADDR_Frameset_6cf2);
			gb->cpu.a = 0x23;
			update_obj(gb);
			return;
		}
		alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA_YES);
		if (gb->cpu.f & FLAG_Z) {
			SET_BC(gb, ADDR_Frameset_6d05);
			gb->cpu.a = 0x24;
			update_obj(gb);
			return;
		}
		wr8(gb, wStartMenuSelection, rd8(gb, wStartMenuSelection) | STARTMENUF_ANIM2);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wMenuObj1FramesetOffset, 0);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		gb->cpu.a = rd8(gb, wStartMenuSelection);
		alu_cp(gb, gb->cpu.a, STARTMENU_CONTINUE);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.a = STARTMENU_CLEAR_DATA | STARTMENUF_SCROLLING;
		wr8(gb, wStartMenuSelection, gb->cpu.a);
		play_scrolling_sfx(gb);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_UP)) {
		gb->cpu.a = rd8(gb, wStartMenuSelection);
		alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.a = STARTMENU_CONTINUE | STARTMENUF_SCROLLING;
		wr8(gb, wStartMenuSelection, gb->cpu.a);
		play_scrolling_sfx(gb);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		gb->cpu.a = rd8(gb, wStartMenuSelection);
		alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA_YES);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		play_scrolling_sfx(gb);
		SET_BC(gb, ADDR_Frameset_6cec);
		gb->cpu.a = STARTMENU_CLEAR_DATA_NO;
		update_obj(gb);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		gb->cpu.a = rd8(gb, wStartMenuSelection);
		alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA_NO);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		play_scrolling_sfx(gb);
		SET_BC(gb, ADDR_Frameset_6cef);
		gb->cpu.a = STARTMENU_CLEAR_DATA_YES;
		update_obj(gb);
		return;
	}
}

static void move_vertical(GB *gb)
{
	gb->cpu.a = rd8(gb, wStartMenuSelection);
	alu_cp(gb, gb->cpu.a, STARTMENU_CLEAR_DATA | STARTMENUF_SCROLLING);
	SET_HL(gb, wMenuObj1YCoord);
	if (!(gb->cpu.f & FLAG_Z)) {
		// move down
		wr8(gb, wMenuObj1YCoord, alu_inc(gb, rd8(gb, wMenuObj1YCoord)));
		gb->cpu.a = rd8(gb, wMenuObj1YCoord);
		alu_cp(gb, gb->cpu.a, 130);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.a = 134;
	} else {
		wr8(gb, wMenuObj1YCoord, alu_dec(gb, rd8(gb, wMenuObj1YCoord)));
		gb->cpu.a = rd8(gb, wMenuObj1YCoord);
		alu_cp(gb, gb->cpu.a, 116);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.a = 118;
	}
	wr8(gb, wMenuObj2YCoord, gb->cpu.a);
	SET_HL(gb, wStartMenuSelection);
	wr8(gb, wStartMenuSelection, rd8(gb, wStartMenuSelection) & ~STARTMENUF_SCROLLING);
}

static void set_select_flag(GB *gb);

static void update_anim1(GB *gb)
{
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	set_select_flag(gb);
}

// .set_select_flag
static void set_select_flag(GB *gb)
{
	SET_HL(gb, wStartMenuSelection);
	wr8(gb, wStartMenuSelection, rd8(gb, wStartMenuSelection) | STARTMENUF_SELECT);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMenuObj1FramesetOffset, 0);
}

PORT(HandleStartMenuSelection)
{
	gb->cpu.a = rd8(gb, wStartMenuSelection);
	if (alu_bit(gb, gb->cpu.a, STARTMENUF_SCROLLING_F)) {
		move_vertical(gb);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, STARTMENUF_ANIM1_F)) {
		update_anim1(gb);
		return;
	}
	if (!alu_bit(gb, gb->cpu.a, STARTMENUF_ANIM2_F)) {
		handle_input(gb);
		return;
	}

	// .UpdateAnim2
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x01);
	if (gb->cpu.a)
		return;
	SET_HL(gb, wMenuObj1FramesetOffset);
	gb->cpu.a = rd8(gb, wMenuObj1FramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_Z) {
		set_select_flag(gb);
		return;
	}
	wr8(gb, wMenuObj1FramesetOffset, alu_inc(gb, rd8(gb, wMenuObj1FramesetOffset)));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
	if (!gb->cpu.a) {
		gb->cpu.a = 0x03;
	} else {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.a) {
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (!gb->cpu.a) {
				gb->cpu.a = 0x01;
				goto write;
			}
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
write:
	gb->cpu.l = alu_dec(gb, (uint8_t)wMenuObj1FramesetOffset); // dec l
	wr8(gb, REG_HL(gb), gb->cpu.a);
}
