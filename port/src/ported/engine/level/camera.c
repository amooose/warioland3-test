// Port of src/engine/level/camera.asm
#include "ported.h"

#define CPU (&gb->cpu)

static bool flag(GB *gb, uint8_t f) { return gb->cpu.f & f; }

// ld a, [wDirection] / and a / jr nz, .dir_right / ld a, [wCamLeftSpacing] ...
static uint8_t cam_spacing(GB *gb)
{
	CPU->a = rd8(gb, wDirection);
	alu_and(gb, CPU->a, CPU->a);
	return flag(gb, FLAG_Z) ? rd8(gb, wCamLeftSpacing) : rd8(gb, wCamRightSpacing);
}

// b = offset for x scroll
PORT(ScrollCameraRight)
{
	CPU->f = FLAG_Z; // xor a
	wr8(gb, wScrollRightEdge, 0);
	CPU->c = alu_dec(gb, rd8(gb, wRoomRightXLimit));

	CPU->a = alu_add(gb, rd8(gb, wCameraSCX + 1), CPU->b);
	wr8(gb, wCameraSCX + 1, CPU->a);
	CPU->a = alu_adc(gb, rd8(gb, wCameraSCX + 0), 0);
	wr8(gb, wCameraSCX + 0, CPU->a);

	CPU->a = rd8(gb, wCameraConfigFlags);
	CPU->b = alu_bit(gb, CPU->a, CAM_BORDER_RIGHT_F)
		? TILEMAP_WIDTH_PX - (SCREEN_WIDTH_PX + CAMERA_BORDER_RIGHT_W)
		: TILEMAP_WIDTH_PX - SCREEN_WIDTH_PX;
	CPU->a = rd8(gb, wCameraSCX + 0);
	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_C)) {
		CPU->a = rd8(gb, wCameraSCX + 1);
		alu_cp(gb, CPU->a, CPU->b);
		if (!flag(gb, FLAG_C)) {
			// set x scroll to maximum allowed
			CPU->a = CPU->b;
			wr8(gb, wCameraSCX + 1, CPU->a);
			CPU->a = 1;
			wr8(gb, wScrollRightEdge, 1);
			return;
		}
	}

	// .not_on_limit
	CPU->c = rd8(gb, wRoomLeftXLimit);
	CPU->a = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_Z))
		return;
	CPU->a = alu_sub(gb, cam_spacing(gb), 8);
	CPU->c = CPU->a;
	CPU->a = rd8(gb, wCameraConfigFlags);
	if (alu_bit(gb, CPU->a, CAM_BORDER_LEFT_F)) {
		CPU->a = alu_add(gb, CPU->c, CAMERA_BORDER_LEFT_W);
		CPU->c = CPU->a;
	}
	CPU->a = rd8(gb, wWarioXPos + 1);
	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_C))
		return; // no adjustment needed
	CPU->a = rd8(gb, wCameraConfigFlags);
	CPU->a = alu_bit(gb, CPU->a, CAM_BORDER_LEFT_F) ? CAMERA_BORDER_LEFT_W : 0;
	wr8(gb, wCameraSCX + 1, CPU->a);
	CPU->a = 2;
	wr8(gb, wScrollRightEdge, 2);
}

// b = offset for x scroll
PORT(ScrollCameraLeft)
{
	CPU->f = FLAG_Z; // xor a
	wr8(gb, wScrollLeftEdge, 0);
	CPU->c = rd8(gb, wRoomLeftXLimit);

	CPU->a = alu_sub(gb, rd8(gb, wCameraSCX + 1), CPU->b);
	wr8(gb, wCameraSCX + 1, CPU->a);
	CPU->a = alu_sbc(gb, rd8(gb, wCameraSCX + 0), 0);
	wr8(gb, wCameraSCX + 0, CPU->a);

	CPU->a = alu_add(gb, rd8(gb, wCameraSCX + 1), 0x10);
	CPU->l = CPU->a;
	CPU->a = alu_adc(gb, rd8(gb, wCameraSCX + 0), 0);
	alu_cp(gb, CPU->a, CPU->c);
	if (flag(gb, FLAG_Z)) {
		CPU->a = rd8(gb, wCameraConfigFlags);
		CPU->b = alu_bit(gb, CPU->a, CAM_BORDER_LEFT_F) ? CAMERA_BORDER_LEFT_W + 0x10 : 0x10;
		CPU->a = CPU->l;
		alu_cp(gb, CPU->a, CPU->b);
		if (flag(gb, FLAG_C)) {
			CPU->a = CPU->c;
			wr8(gb, wCameraSCX + 0, CPU->a);
			CPU->a = alu_sub(gb, CPU->b, 0x10);
			wr8(gb, wCameraSCX + 1, CPU->a);
			CPU->a = 1;
			wr8(gb, wScrollLeftEdge, 1);
			return;
		}
	}

	// .not_on_limit
	CPU->c = alu_dec(gb, rd8(gb, wRoomRightXLimit));
	CPU->a = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_Z))
		return;
	CPU->c = cam_spacing(gb);
	CPU->a = alu_sub(gb, SCREEN_WIDTH_PX + 8, CPU->c);
	CPU->c = CPU->a;
	CPU->a = alu_sub(gb, 0, CPU->c); // xor a / sub c
	CPU->c = CPU->a;
	CPU->a = rd8(gb, wCameraConfigFlags);
	if (alu_bit(gb, CPU->a, CAM_BORDER_RIGHT_F)) {
		CPU->a = alu_sub(gb, CPU->c, CAMERA_BORDER_RIGHT_W);
		CPU->c = CPU->a;
	}
	CPU->a = rd8(gb, wWarioXPos + 1);
	alu_cp(gb, CPU->a, CPU->c);
	if (flag(gb, FLAG_C))
		return; // no adjustment needed
	CPU->a = 2;
	wr8(gb, wScrollLeftEdge, 2);
	CPU->a = alu_dec(gb, rd8(gb, wRoomRightXLimit));
	wr8(gb, wCameraSCX + 0, CPU->a);
	CPU->a = rd8(gb, wCameraConfigFlags);
	CPU->a = alu_bit(gb, CPU->a, CAM_BORDER_RIGHT_F)
		? TILEMAP_WIDTH_PX - (SCREEN_WIDTH_PX + CAMERA_BORDER_RIGHT_W)
		: TILEMAP_WIDTH_PX - SCREEN_WIDTH_PX;
	wr8(gb, wCameraSCX + 1, CPU->a);
}

// ld a, [wCameraConfigFlags] / and CAM_SCROLLING_MASK / cp CAM_TRANSITIONS
// returns whether the camera's in CAM_TRANSITIONS (ret nc)
static bool cam_transitions(GB *gb)
{
	CPU->a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, CPU->a, CAM_TRANSITIONS);
	return !flag(gb, FLAG_C);
}

// the lowest y scroll (in the bottom block row)
static uint8_t lower_y_scroll(GB *gb)
{
	CPU->a = rd8(gb, wCameraConfigFlags);
	return alu_bit(gb, CPU->a, CAM_BORDER_DOWN_F)
		? TILEMAP_HEIGHT_PX - (SCREEN_HEIGHT_PX + CAMERA_BORDER_LOWER_H + 8)
		: TILEMAP_HEIGHT_PX - (SCREEN_HEIGHT_PX + 8);
}

// b = offset for y scroll
PORT(ScrollCameraDown)
{
	CPU->c = alu_dec(gb, rd8(gb, wRoomLowerYLimit));

	CPU->a = alu_add(gb, rd8(gb, wCameraSCY + 1), CPU->b);
	wr8(gb, wCameraSCY + 1, CPU->a);
	CPU->a = alu_adc(gb, rd8(gb, wCameraSCY + 0), 0);
	wr8(gb, wCameraSCY + 0, CPU->a);

	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_C)) {
		CPU->b = lower_y_scroll(gb);
		CPU->a = rd8(gb, wCameraSCY + 1);
		alu_cp(gb, CPU->a, CPU->b);
		if (!flag(gb, FLAG_C)) {
			// set y scroll to maximum allowed
			CPU->a = CPU->b;
			wr8(gb, wCameraSCY + 1, CPU->a);
			CPU->a = 1;
			wr8(gb, wScrollDownEdge, 1);
			return;
		}
	}

	// .not_on_limit
	if (cam_transitions(gb))
		return;
	CPU->c = rd8(gb, wRoomUpperYLimit);
	CPU->a = rd8(gb, wWarioYPos + 0);
	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_C)) {
		if (!flag(gb, FLAG_Z))
			return;
		CPU->a = alu_sub(gb, rd8(gb, wPrevWarioScreenYPos), 0x10);
		CPU->c = CPU->a;
		CPU->a = rd8(gb, wCameraConfigFlags);
		if (alu_bit(gb, CPU->a, CAM_BORDER_UP_F)) {
			CPU->a = alu_add(gb, CPU->c, CAMERA_BORDER_UPPER_H);
			CPU->c = CPU->a;
		}
		CPU->a = rd8(gb, wWarioYPos + 1);
		alu_cp(gb, CPU->a, CPU->c);
		if (!flag(gb, FLAG_C))
			return; // no adjustment needed
	}
	// .asm_b836
	CPU->a = rd8(gb, wCameraConfigFlags);
	CPU->a = alu_bit(gb, CPU->a, CAM_BORDER_UP_F) ? CAMERA_BORDER_UPPER_H : 0;
	wr8(gb, wCameraSCY + 1, CPU->a);
	CPU->a = rd8(gb, wRoomUpperYLimit);
	wr8(gb, wCameraSCY + 0, CPU->a);
	CPU->a = 2;
	wr8(gb, wScrollDownEdge, 2);
}

// b = offset for y scroll
PORT(ScrollCameraUp)
{
	CPU->c = rd8(gb, wRoomUpperYLimit);

	CPU->a = alu_sub(gb, rd8(gb, wCameraSCY + 1), CPU->b);
	wr8(gb, wCameraSCY + 1, CPU->a);
	CPU->a = alu_sbc(gb, rd8(gb, wCameraSCY + 0), 0);
	wr8(gb, wCameraSCY + 0, CPU->a);

	CPU->a = rd8(gb, wCameraConfigFlags);
	CPU->b = alu_bit(gb, CPU->a, CAM_BORDER_UP_F) ? CAMERA_BORDER_UPPER_H + 0x10 : 0x10;
	CPU->a = alu_add(gb, rd8(gb, wCameraSCY + 1), 0x10);
	CPU->l = CPU->a;
	CPU->a = alu_adc(gb, rd8(gb, wCameraSCY + 0), 0);
	alu_cp(gb, CPU->a, CPU->c);
	bool upper_edge = flag(gb, FLAG_C);
	if (!upper_edge && flag(gb, FLAG_Z)) {
		CPU->a = CPU->l;
		alu_cp(gb, CPU->a, CPU->b);
		upper_edge = flag(gb, FLAG_C);
	}
	if (upper_edge) {
		CPU->a = CPU->c;
		wr8(gb, wCameraSCY + 0, CPU->a);
		CPU->a = alu_sub(gb, CPU->b, 0x10);
		wr8(gb, wCameraSCY + 1, CPU->a);
		CPU->a = 1;
		wr8(gb, wScrollUpEdge, 1);
		return;
	}

	// .not_on_limit
	if (cam_transitions(gb))
		return;
	CPU->c = alu_dec(gb, rd8(gb, wRoomLowerYLimit));
	CPU->a = rd8(gb, wWarioYPos + 0);
	alu_cp(gb, CPU->a, CPU->c);
	if (!flag(gb, FLAG_Z))
		return;
	CPU->c = rd8(gb, wPrevWarioScreenYPos);
	CPU->a = alu_sub(gb, SCREEN_HEIGHT_PX + 0x10, CPU->c);
	CPU->c = CPU->a;
	CPU->a = alu_sub(gb, 0, CPU->c); // xor a / sub c
	CPU->c = CPU->a;
	CPU->a = rd8(gb, wCameraConfigFlags);
	if (alu_bit(gb, CPU->a, CAM_BORDER_DOWN_F)) {
		CPU->a = alu_sub(gb, CPU->c, CAMERA_BORDER_LOWER_H);
		CPU->c = CPU->a;
	}
	CPU->a = rd8(gb, wWarioYPos + 1);
	alu_cp(gb, CPU->a, CPU->c);
	if (flag(gb, FLAG_C))
		return;
	CPU->a = 2;
	wr8(gb, wScrollUpEdge, 2);
	CPU->a = lower_y_scroll(gb);
	wr8(gb, wCameraSCY + 1, CPU->a);
}

PORT(SetWarioScreenPos)
{
	CPU->a = rd8(gb, wCameraConfigFlags);
	alu_cp(gb, CPU->a, HIDDEN_FIGURE_CAMCONFIG);
	bool hidden_figure = flag(gb, FLAG_Z);
	if (hidden_figure) {
		CPU->c = rd8(gb, wSCYShake);
		CPU->a = alu_add(gb, rd8(gb, wc08a), CPU->c);
		CPU->b = CPU->a;
	} else {
		CPU->b = rd8(gb, wSCY);
	}
	CPU->a = alu_sub(gb, alu_add(gb, rd8(gb, wWarioYPos + 1), OAM_Y_OFS), CPU->b);
	wr8(gb, wWarioScreenYPos, CPU->a);
	CPU->b = rd8(gb, hidden_figure ? wc08c : wSCX);
	CPU->a = alu_sub(gb, alu_add(gb, rd8(gb, wWarioXPos + 1), OAM_X_OFS), CPU->b);
	wr8(gb, wWarioScreenXPos, CPU->a);
}

// call ScrollCamera*; if the edge wasn't reached, set the edge update flag;
// then xor a / ld [delta], a
static void scroll_and_flag(GB *gb, uint16_t scroll, uint16_t edge, int flag_bit, uint16_t delta)
{
	gb_call(gb, scroll);
	CPU->a = rd8(gb, edge);
	if (!alu_and(gb, CPU->a, CPU->a)) {
		SET_HL(gb, wLevelEdgeUpdateFlags);
		wr8(gb, wLevelEdgeUpdateFlags, (uint8_t)(rd8(gb, wLevelEdgeUpdateFlags) | (1 << flag_bit)));
	}
	CPU->a = 0;
	CPU->f = FLAG_Z; // xor a
	wr8(gb, delta, 0);
}

static void scroll_down(GB *gb)
{
	scroll_and_flag(gb, ADDR_ScrollCameraDown, wScrollDownEdge, LEVEL_EDGE_UPDATE_DOWN_F, wCameraYDelta);
}

static void scroll_up(GB *gb)
{
	scroll_and_flag(gb, ADDR_ScrollCameraUp, wScrollUpEdge, LEVEL_EDGE_UPDATE_UP_F, wCameraYDelta);
}

static void scroll_right(GB *gb)
{
	scroll_and_flag(gb, ADDR_ScrollCameraRight, wScrollRightEdge, LEVEL_EDGE_UPDATE_RIGHT_F, wCameraXDelta);
}

static void scroll_left(GB *gb)
{
	scroll_and_flag(gb, ADDR_ScrollCameraLeft, wScrollLeftEdge, LEVEL_EDGE_UPDATE_LEFT_F, wCameraXDelta);
}

// ld a, [counter] / inc a / ld [counter], a / dec a; true if it was 0
static bool count_up_was_zero(GB *gb, uint16_t counter)
{
	CPU->a = alu_inc(gb, rd8(gb, counter));
	wr8(gb, counter, CPU->a);
	CPU->a = alu_dec(gb, CPU->a);
	return flag(gb, FLAG_Z);
}

PORT(LevelScroll_Vertical)
{
	CPU->a = rd8(gb, wCameraYDelta);
	CPU->b = CPU->a;
	if (alu_and(gb, CPU->a, CPU->a)) {
		CPU->a = 0;
		CPU->f = FLAG_Z; // xor a
		wr8(gb, wc0d4, 0);
		SET_HL(gb, wWarioScreenYPos);
		if (!alu_bit(gb, CPU->b, 7)) {
			CPU->a = rd8(gb, wPrevWarioScreenYPos);
			alu_cp(gb, CPU->a, rd8(gb, wWarioScreenYPos));
			if (flag(gb, FLAG_C))
				CPU->b = alu_inc(gb, CPU->b);
			scroll_down(gb);
		} else {
			CPU->a = alu_inc(gb, alu_cpl(gb, CPU->b));
			CPU->b = CPU->a;
			CPU->a = rd8(gb, wPrevWarioScreenYPos);
			alu_cp(gb, CPU->a, rd8(gb, wWarioScreenYPos));
			if (!flag(gb, FLAG_C | FLAG_Z))
				CPU->b = alu_inc(gb, CPU->b);
			scroll_up(gb);
		}
		return;
	}

	// .no_y_offset
	CPU->a = rd8(gb, wWaterInteraction);
	if (alu_and(gb, CPU->a, CPU->a))
		return; // is in water
	CPU->a = rd8(gb, wJumpVelIndex);
	if (alu_and(gb, CPU->a, CPU->a))
		return; // is jumping
	CPU->a = rd8(gb, wWarioState);
	alu_cp(gb, CPU->a, WST_LADDER_SCRATCHING);
	if (flag(gb, FLAG_Z))
		return;
	alu_cp(gb, CPU->a, WST_LADDER_CLIMBING);
	if (!flag(gb, FLAG_C)) {
		alu_cp(gb, CPU->a, WST_LADDER_SLIDING + 1);
		if (flag(gb, FLAG_C))
			return;
	}
	if (count_up_was_zero(gb, wc0d4))
		return;

	SET_HL(gb, wWarioScreenYPos);
	CPU->a = alu_sub(gb, rd8(gb, wPrevWarioScreenYPos), rd8(gb, wWarioScreenYPos));
	if (flag(gb, FLAG_C))
		CPU->a = alu_inc(gb, alu_cpl(gb, CPU->a));
	CPU->b = CPU->a;
	CPU->a = rd8(gb, wWarioSlopeInteraction);
	if (!alu_and(gb, CPU->a, CPU->a)) {
		CPU->a = CPU->b;
		alu_cp(gb, CPU->a, 8);
		if (flag(gb, FLAG_C))
			return; // offset < 8
		CPU->b = 7;
	}
	// .on_slope
	CPU->a = rd8(gb, wPrevWarioScreenYPos);
	alu_cp(gb, CPU->a, rd8(gb, wWarioScreenYPos));
	if (flag(gb, FLAG_C)) {
		scroll_down(gb);
		return;
	}
	if (flag(gb, FLAG_Z))
		return;
	scroll_up(gb);
}

PORT(LevelScroll_Horizontal)
{
	CPU->a = rd8(gb, wCameraXDelta);
	CPU->b = CPU->a;
	if (alu_and(gb, CPU->a, CPU->a)) {
		CPU->a = 0;
		CPU->f = FLAG_Z; // xor a
		wr8(gb, wc0d5, 0);
		bool left = alu_bit(gb, CPU->b, 7);
		if (left) {
			CPU->a = alu_inc(gb, alu_cpl(gb, CPU->b));
			CPU->b = CPU->a;
		}
		SET_HL(gb, wWarioScreenXPos);
		CPU->a = cam_spacing(gb);
		alu_cp(gb, CPU->a, rd8(gb, wWarioScreenXPos));
		if (!flag(gb, FLAG_Z)) {
			// x pos > cam spacing: towards it, else away
			bool inc = flag(gb, FLAG_C) ? !left : left;
			CPU->b = inc ? alu_inc(gb, CPU->b) : alu_dec(gb, CPU->b);
		}
		if (left)
			scroll_left(gb);
		else
			scroll_right(gb);
		return;
	}

	// .no_x_offset
	CPU->a = rd8(gb, wTransformation);
	alu_cp(gb, CPU->a, TRANSFORMATION_FLAT_WARIO);
	if (flag(gb, FLAG_Z)) {
		CPU->a = rd8(gb, wJumpVelTable);
		if (alu_and(gb, CPU->a, CPU->a))
			return; // is jumping
	}
	if (count_up_was_zero(gb, wc0d5))
		return;
	CPU->b = 1;
	SET_HL(gb, wWarioScreenXPos);
	CPU->a = cam_spacing(gb);
	alu_cp(gb, CPU->a, rd8(gb, wWarioScreenXPos));
	if (flag(gb, FLAG_C)) {
		scroll_right(gb);
		return;
	}
	if (flag(gb, FLAG_Z))
		return;
	scroll_left(gb);
}
