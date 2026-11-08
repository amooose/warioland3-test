// The level camera (src/engine/level/camera.asm): scrolling it after Wario
// within the room's limits, the edges it reaches, and Wario's position on
// the screen.
//
// Plain C++ over memory. The callers (update_level.cpp and start_room.cpp,
// with ProcessInteractions, Func_8ed9 and Func_bb85 after) read no
// register these leave (live.py over every call site, and the entry
// liveness of what follows), so the hooks clobber them all.
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::level {

namespace {

constexpr uint8_t SCROLL_RIGHT_MAX = TILEMAP_WIDTH_PX - SCREEN_WIDTH_PX;
constexpr uint8_t SCROLL_RIGHT_MAX_BORDER = TILEMAP_WIDTH_PX - (SCREEN_WIDTH_PX + CAMERA_BORDER_RIGHT_W);
constexpr uint8_t SCROLL_DOWN_MAX = TILEMAP_HEIGHT_PX - (SCREEN_HEIGHT_PX + 8);
constexpr uint8_t SCROLL_DOWN_MAX_BORDER = TILEMAP_HEIGHT_PX - (SCREEN_HEIGHT_PX + CAMERA_BORDER_LOWER_H + 8);

class Camera {
public:
	explicit Camera(GB *gb) : mem_(gb) {}

	void scroll_right(uint8_t by); // ScrollCameraRight
	void scroll_left(uint8_t by); // ScrollCameraLeft
	void scroll_down(uint8_t by); // ScrollCameraDown
	void scroll_up(uint8_t by); // ScrollCameraUp
	void set_wario_screen_pos(); // SetWarioScreenPos
	void level_scroll_vertical(); // LevelScroll_Vertical
	void level_scroll_horizontal(); // LevelScroll_Horizontal

private:
	bool config_bit(int n) const { return mem_[wCameraConfigFlags] & (1 << n); }
	// the camera's spacing on Wario's side
	uint8_t spacing() const { return mem_[mem_[wDirection] ? wCamRightSpacing : wCamLeftSpacing]; }
	bool transitions() const { return (mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK) >= CAM_TRANSITIONS; }
	// the lowest y scroll, the bottom block row's
	uint8_t lower_y_scroll() const { return config_bit(CAM_BORDER_DOWN_F) ? SCROLL_DOWN_MAX_BORDER : SCROLL_DOWN_MAX; }
	uint8_t right_x_scroll() const { return config_bit(CAM_BORDER_RIGHT_F) ? SCROLL_RIGHT_MAX_BORDER : SCROLL_RIGHT_MAX; }
	// the scroll (big-endian, high byte at addr) moved by v
	void add_scroll(uint16_t addr, int v)
	{
		uint16_t s = static_cast<uint16_t>((mem_[addr] << 8 | mem_[addr + 1]) + v);
		mem_[addr + 0] = static_cast<uint8_t>(s >> 8);
		mem_[addr + 1] = static_cast<uint8_t>(s);
	}
	// the low byte 16 pixels on from the scroll, and whether its high byte's
	// then at `limit` (the room's left or top), with the low byte under min,
	// or under the limit
	bool near_limit(uint16_t addr, uint8_t limit, uint8_t min) const
	{
		unsigned ahead = mem_[addr + 1] + 0x10;
		uint8_t hi = static_cast<uint8_t>(mem_[addr + 0] + (ahead > 0xff));
		return hi < limit || (hi == limit && static_cast<uint8_t>(ahead) < min);
	}
	void scroll_and_flag(void (Camera::*scroll)(uint8_t), uint8_t by, uint16_t edge, int flag_bit, uint16_t delta);
	bool count_up_was_zero(uint16_t counter);

	Memory mem_;
};

void Camera::scroll_right(uint8_t by)
{
	mem_[wScrollRightEdge] = 0;
	uint8_t right = static_cast<uint8_t>(mem_[wRoomRightXLimit] - 1);
	add_scroll(wCameraSCX, by);
	uint8_t max = right_x_scroll();
	if (mem_[wCameraSCX + 0] >= right && mem_[wCameraSCX + 1] >= max) { // at the most it scrolls
		mem_[wCameraSCX + 1] = max;
		mem_[wScrollRightEdge] = 1;
		return;
	}
	// .not_on_limit: Wario at the room's left: the camera follows him back
	if (mem_[wWarioXPos + 0] != mem_[wRoomLeftXLimit])
		return;
	uint8_t c = static_cast<uint8_t>(spacing() - 8);
	if (config_bit(CAM_BORDER_LEFT_F))
		c = static_cast<uint8_t>(c + CAMERA_BORDER_LEFT_W);
	if (mem_[wWarioXPos + 1] >= c)
		return; // nothing to do
	mem_[wCameraSCX + 1] = config_bit(CAM_BORDER_LEFT_F) ? CAMERA_BORDER_LEFT_W : 0;
	mem_[wScrollRightEdge] = 2;
}

void Camera::scroll_left(uint8_t by)
{
	mem_[wScrollLeftEdge] = 0;
	uint8_t left = mem_[wRoomLeftXLimit];
	add_scroll(wCameraSCX, -by);
	uint8_t min = config_bit(CAM_BORDER_LEFT_F) ? CAMERA_BORDER_LEFT_W + 0x10 : 0x10;
	// (16 pixels on, at the room's left: only the high byte's equality
	// counts, as the asm's jr nz)
	unsigned ahead = mem_[wCameraSCX + 1] + 0x10;
	uint8_t hi = static_cast<uint8_t>(mem_[wCameraSCX + 0] + (ahead > 0xff));
	if (hi == left && static_cast<uint8_t>(ahead) < min) { // at the room's left
		mem_[wCameraSCX + 0] = left;
		mem_[wCameraSCX + 1] = static_cast<uint8_t>(min - 0x10);
		mem_[wScrollLeftEdge] = 1;
		return;
	}
	// .not_on_limit: Wario at the room's right: the camera follows him back
	uint8_t right = static_cast<uint8_t>(mem_[wRoomRightXLimit] - 1);
	if (mem_[wWarioXPos + 0] != right)
		return;
	uint8_t c = static_cast<uint8_t>(-(SCREEN_WIDTH_PX + 8 - spacing()));
	if (config_bit(CAM_BORDER_RIGHT_F))
		c = static_cast<uint8_t>(c - CAMERA_BORDER_RIGHT_W);
	if (mem_[wWarioXPos + 1] < c)
		return; // nothing to do
	mem_[wScrollLeftEdge] = 2;
	mem_[wCameraSCX + 0] = right;
	mem_[wCameraSCX + 1] = right_x_scroll();
}

void Camera::scroll_down(uint8_t by)
{
	uint8_t lower = static_cast<uint8_t>(mem_[wRoomLowerYLimit] - 1);
	add_scroll(wCameraSCY, by);
	uint8_t max = lower_y_scroll();
	if (mem_[wCameraSCY + 0] >= lower && mem_[wCameraSCY + 1] >= max) { // at the most it scrolls
		mem_[wCameraSCY + 1] = max;
		mem_[wScrollDownEdge] = 1;
		return;
	}
	// .not_on_limit: Wario at the room's top: the camera follows him back
	if (transitions())
		return;
	uint8_t upper = mem_[wRoomUpperYLimit];
	if (mem_[wWarioYPos + 0] >= upper) {
		if (mem_[wWarioYPos + 0] != upper)
			return;
		uint8_t c = static_cast<uint8_t>(mem_[wPrevWarioScreenYPos] - 0x10);
		if (config_bit(CAM_BORDER_UP_F))
			c = static_cast<uint8_t>(c + CAMERA_BORDER_UPPER_H);
		if (mem_[wWarioYPos + 1] >= c)
			return; // nothing to do
	}
	mem_[wCameraSCY + 1] = config_bit(CAM_BORDER_UP_F) ? CAMERA_BORDER_UPPER_H : 0;
	mem_[wCameraSCY + 0] = upper;
	mem_[wScrollDownEdge] = 2;
}

void Camera::scroll_up(uint8_t by)
{
	uint8_t upper = mem_[wRoomUpperYLimit];
	add_scroll(wCameraSCY, -by);
	uint8_t min = config_bit(CAM_BORDER_UP_F) ? CAMERA_BORDER_UPPER_H + 0x10 : 0x10;
	if (near_limit(wCameraSCY, upper, min)) { // at the room's top
		mem_[wCameraSCY + 0] = upper;
		mem_[wCameraSCY + 1] = static_cast<uint8_t>(min - 0x10);
		mem_[wScrollUpEdge] = 1;
		return;
	}
	// .not_on_limit: Wario at the room's bottom: the camera follows him back
	if (transitions())
		return;
	if (mem_[wWarioYPos + 0] != static_cast<uint8_t>(mem_[wRoomLowerYLimit] - 1))
		return;
	uint8_t c = static_cast<uint8_t>(-(SCREEN_HEIGHT_PX + 0x10 - mem_[wPrevWarioScreenYPos]));
	if (config_bit(CAM_BORDER_DOWN_F))
		c = static_cast<uint8_t>(c - CAMERA_BORDER_LOWER_H);
	if (mem_[wWarioYPos + 1] < c)
		return;
	mem_[wScrollUpEdge] = 2;
	mem_[wCameraSCY + 1] = lower_y_scroll();
}

// Wario's position on the screen (the Hidden Figure's battle scrolls by
// wc08a / wc08c and the shake)
void Camera::set_wario_screen_pos()
{
	bool hidden_figure = mem_[wCameraConfigFlags] == HIDDEN_FIGURE_CAMCONFIG;
	uint8_t scy = hidden_figure ? static_cast<uint8_t>(mem_[wc08a] + mem_[wSCYShake]) : mem_[wSCY];
	mem_[wWarioScreenYPos] = static_cast<uint8_t>(mem_[wWarioYPos + 1] + OAM_Y_OFS - scy);
	uint8_t scx = mem_[hidden_figure ? wc08c : wSCX];
	mem_[wWarioScreenXPos] = static_cast<uint8_t>(mem_[wWarioXPos + 1] + OAM_X_OFS - scx);
}

// scrolls; with no edge reached, the edge's row or column is to be drawn;
// then the delta's cleared
void Camera::scroll_and_flag(void (Camera::*scroll)(uint8_t), uint8_t by, uint16_t edge, int flag_bit, uint16_t delta)
{
	(this->*scroll)(by);
	if (!mem_[edge])
		mem_[wLevelEdgeUpdateFlags] = mem_[wLevelEdgeUpdateFlags] | (1 << flag_bit);
	mem_[delta] = 0;
}

// counts up: true if it was 0
bool Camera::count_up_was_zero(uint16_t counter)
{
	uint8_t was = mem_[counter];
	mem_[counter] = static_cast<uint8_t>(was + 1);
	return !was;
}

// scrolls up or down by wCameraYDelta, or towards Wario
void Camera::level_scroll_vertical()
{
	auto down = [&](uint8_t by) { scroll_and_flag(&Camera::scroll_down, by, wScrollDownEdge, LEVEL_EDGE_UPDATE_DOWN_F, wCameraYDelta); };
	auto up = [&](uint8_t by) { scroll_and_flag(&Camera::scroll_up, by, wScrollUpEdge, LEVEL_EDGE_UPDATE_UP_F, wCameraYDelta); };
	uint8_t delta = mem_[wCameraYDelta];
	uint8_t prev = mem_[wPrevWarioScreenYPos], now = mem_[wWarioScreenYPos];
	if (delta) {
		// a pixel faster while Wario's moving further from the camera
		mem_[wc0d4] = 0;
		if (!(delta & 0x80))
			down(static_cast<uint8_t>(delta + (prev < now)));
		else
			up(static_cast<uint8_t>(-delta + (prev > now)));
		return;
	}

	// .no_y_offset: in water, jumping or on a ladder, it stays
	if (mem_[wWaterInteraction] || mem_[wJumpVelIndex])
		return;
	uint8_t state = mem_[wWarioState];
	if (state == WST_LADDER_SCRATCHING || (state >= WST_LADDER_CLIMBING && state <= WST_LADDER_SLIDING))
		return;
	if (count_up_was_zero(wc0d4))
		return;

	// towards Wario: 7 pixels at a time off slopes, from 8 away
	uint8_t distance = prev < now ? static_cast<uint8_t>(now - prev) : static_cast<uint8_t>(prev - now);
	if (!mem_[wWarioSlopeInteraction]) {
		if (distance < 8)
			return; // less than 8 pixels
		distance = 7;
	}
	// .on_slope
	if (prev < now)
		down(distance);
	else if (prev != now)
		up(distance);
}

// scrolls right or left by wCameraXDelta, or towards Wario's spacing
void Camera::level_scroll_horizontal()
{
	auto right = [&](uint8_t by) { scroll_and_flag(&Camera::scroll_right, by, wScrollRightEdge, LEVEL_EDGE_UPDATE_RIGHT_F, wCameraXDelta); };
	auto left = [&](uint8_t by) { scroll_and_flag(&Camera::scroll_left, by, wScrollLeftEdge, LEVEL_EDGE_UPDATE_LEFT_F, wCameraXDelta); };
	uint8_t delta = mem_[wCameraXDelta];
	uint8_t space = spacing(), now = mem_[wWarioScreenXPos];
	if (delta) {
		mem_[wc0d5] = 0;
		bool going_left = delta & 0x80;
		uint8_t by = going_left ? static_cast<uint8_t>(-delta) : delta;
		if (space != now) {
			// past the spacing: faster towards it, slower away
			bool faster = space < now ? !going_left : going_left;
			by = static_cast<uint8_t>(faster ? by + 1 : by - 1);
		}
		if (going_left)
			left(by);
		else
			right(by);
		return;
	}

	// .no_x_offset: towards the spacing a pixel at a time (not while Flat
	// Wario's jumping)
	if (mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO && mem_[wJumpVelTable])
		return; // jumping
	if (count_up_was_zero(wc0d5))
		return;
	if (space < now)
		right(1);
	else if (space != now)
		left(1);
}

} // namespace

} // namespace wl3::level

using wl3::level::Camera;

// ScrollCamera*: b = how far
PORT_EX(ScrollCameraRight, 0, ALL_REGISTERS) { Camera(gb).scroll_right(gb->cpu.b); }
PORT_EX(ScrollCameraLeft, 0, ALL_REGISTERS) { Camera(gb).scroll_left(gb->cpu.b); }
PORT_EX(ScrollCameraDown, 0, ALL_REGISTERS) { Camera(gb).scroll_down(gb->cpu.b); }
PORT_EX(ScrollCameraUp, 0, ALL_REGISTERS) { Camera(gb).scroll_up(gb->cpu.b); }
PORT_EX(SetWarioScreenPos, 0, ALL_REGISTERS) { Camera(gb).set_wario_screen_pos(); }
PORT_EX(LevelScroll_Vertical, 0, ALL_REGISTERS) { Camera(gb).level_scroll_vertical(); }
PORT_EX(LevelScroll_Horizontal, 0, ALL_REGISTERS) { Camera(gb).level_scroll_horizontal(); }
