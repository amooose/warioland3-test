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
