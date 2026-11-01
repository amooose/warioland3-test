// The level camera (src/engine/level/camera.asm): scrolling it after Wario
// within the room's limits, the edges it reaches, and Wario's position on
// the screen.
//
// Its callers (the level's frame and room starts, still C) may read the
// registers after these, so each entry point leaves the registers and
// flags the original does.
#include "../flags.hpp"
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
	explicit Camera(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void scroll_right(); // ScrollCameraRight
	void scroll_left(); // ScrollCameraLeft
	void scroll_down(); // ScrollCameraDown
	void scroll_up(); // ScrollCameraUp
	void set_wario_screen_pos(); // SetWarioScreenPos
	void level_scroll_vertical(); // LevelScroll_Vertical
	void level_scroll_horizontal(); // LevelScroll_Horizontal

private:
	// ld a, [wCameraConfigFlags] / bit n, a: a and the flags
	bool config_bit(int n)
	{
		cpu_.a = mem_[wCameraConfigFlags];
		cpu_.f = flags::bit(cpu_.f, cpu_.a, n);
		return cpu_.a & (1 << n);
	}
	// ld a, [addr] / cp v
	void ld_cp(uint16_t addr, uint8_t v)
	{
		cpu_.a = mem_[addr];
		cpu_.f = flags::cp(cpu_.a, v);
	}
	bool zf() const { return cpu_.f & FLAG_Z; }
	bool cf() const { return cpu_.f & FLAG_C; }
	uint8_t spacing();
	bool transitions();
	uint8_t lower_y_scroll();
	void scroll_and_flag(void (Camera::*scroll)(), uint16_t edge, int flag_bit, uint16_t delta);
	bool count_up_was_zero(uint16_t counter);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// the camera's spacing on Wario's side (ld a, [wDirection] / and a ...)
uint8_t Camera::spacing()
{
	uint8_t dir = mem_[wDirection];
	cpu_.f = flags::and_(dir);
	cpu_.a = dir ? mem_[wCamRightSpacing] : mem_[wCamLeftSpacing];
	return cpu_.a;
}

// b = the x offset
void Camera::scroll_right()
{
	mem_[wScrollRightEdge] = 0;
	uint8_t right = static_cast<uint8_t>(mem_[wRoomRightXLimit] - 1);
	cpu_.c = right;
	unsigned lo = mem_[wCameraSCX + 1] + cpu_.b;
	mem_[wCameraSCX + 1] = static_cast<uint8_t>(lo);
	mem_[wCameraSCX + 0] = static_cast<uint8_t>(mem_[wCameraSCX + 0] + (lo > 0xff));
	cpu_.b = config_bit(CAM_BORDER_RIGHT_F) ? SCROLL_RIGHT_MAX_BORDER : SCROLL_RIGHT_MAX;
	ld_cp(wCameraSCX + 0, right);
	if (!cf()) {
		ld_cp(wCameraSCX + 1, cpu_.b);
		if (!cf()) { // at the most it scrolls
			mem_[wCameraSCX + 1] = cpu_.b;
			mem_[wScrollRightEdge] = 1;
			cpu_.a = 1;
			return;
		}
	}
	// .not_on_limit: Wario at the room's left: the camera follows him back
	cpu_.c = mem_[wRoomLeftXLimit];
	ld_cp(wWarioXPos + 0, cpu_.c);
	if (!zf())
		return;
	uint8_t space = spacing();
	cpu_.f = flags::sub(space, 8);
	cpu_.c = static_cast<uint8_t>(space - 8);
	if (config_bit(CAM_BORDER_LEFT_F)) {
		cpu_.f = flags::add(cpu_.c, CAMERA_BORDER_LEFT_W);
		cpu_.c = static_cast<uint8_t>(cpu_.c + CAMERA_BORDER_LEFT_W);
	}
	ld_cp(wWarioXPos + 1, cpu_.c);
	if (!cf())
		return; // nothing to do
	uint8_t scroll = config_bit(CAM_BORDER_LEFT_F) ? CAMERA_BORDER_LEFT_W : 0;
	mem_[wCameraSCX + 1] = scroll;
	mem_[wScrollRightEdge] = 2;
	cpu_.a = 2;
}

// b = the x offset
void Camera::scroll_left()
{
	mem_[wScrollLeftEdge] = 0;
	uint8_t left = mem_[wRoomLeftXLimit];
	cpu_.c = left;
	uint8_t old_lo = mem_[wCameraSCX + 1];
	uint8_t lo = static_cast<uint8_t>(old_lo - cpu_.b);
	mem_[wCameraSCX + 1] = lo;
	uint8_t hi = static_cast<uint8_t>(mem_[wCameraSCX + 0] - (old_lo < cpu_.b));
	mem_[wCameraSCX + 0] = hi;

	// 16 pixels on
	unsigned ahead = lo + 0x10;
	cpu_.l = static_cast<uint8_t>(ahead);
	uint8_t ahead_hi = static_cast<uint8_t>(hi + (ahead > 0xff));
	cpu_.a = ahead_hi;
	cpu_.f = flags::cp(ahead_hi, left);
	if (zf()) {
		cpu_.b = config_bit(CAM_BORDER_LEFT_F) ? CAMERA_BORDER_LEFT_W + 0x10 : 0x10;
		cpu_.a = cpu_.l;
		cpu_.f = flags::cp(cpu_.l, cpu_.b);
		if (cf()) { // at the room's left
			mem_[wCameraSCX + 0] = left;
			cpu_.f = flags::sub(cpu_.b, 0x10);
			mem_[wCameraSCX + 1] = static_cast<uint8_t>(cpu_.b - 0x10);
			mem_[wScrollLeftEdge] = 1;
			cpu_.a = 1;
			return;
		}
	}
	// .not_on_limit: Wario at the room's right: the camera follows him back
	cpu_.f = flags::dec(cpu_.f, mem_[wRoomRightXLimit]);
	cpu_.c = static_cast<uint8_t>(mem_[wRoomRightXLimit] - 1);
	ld_cp(wWarioXPos + 0, cpu_.c);
	if (!zf())
		return;
	uint8_t from_right = static_cast<uint8_t>(SCREEN_WIDTH_PX + 8 - spacing());
	cpu_.f = flags::sub(0, from_right); // xor a / sub c
	cpu_.c = static_cast<uint8_t>(-from_right);
	if (config_bit(CAM_BORDER_RIGHT_F)) {
		cpu_.f = flags::sub(cpu_.c, CAMERA_BORDER_RIGHT_W);
		cpu_.c = static_cast<uint8_t>(cpu_.c - CAMERA_BORDER_RIGHT_W);
	}
	ld_cp(wWarioXPos + 1, cpu_.c);
	if (cf())
		return; // nothing to do
	mem_[wScrollLeftEdge] = 2;
	uint8_t right = mem_[wRoomRightXLimit];
	cpu_.f = flags::dec(cpu_.f, right);
	mem_[wCameraSCX + 0] = static_cast<uint8_t>(right - 1);
	cpu_.a = config_bit(CAM_BORDER_RIGHT_F) ? SCROLL_RIGHT_MAX_BORDER : SCROLL_RIGHT_MAX;
	mem_[wCameraSCX + 1] = cpu_.a;
}

// in CAM_TRANSITIONS (and CAM_SCROLLING_MASK / cp CAM_TRANSITIONS: nc)
bool Camera::transitions()
{
	cpu_.a = mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK;
	cpu_.f = flags::cp(cpu_.a, CAM_TRANSITIONS);
	return !cf();
}

// the lowest y scroll, the bottom block row's
uint8_t Camera::lower_y_scroll()
{
	return config_bit(CAM_BORDER_DOWN_F) ? SCROLL_DOWN_MAX_BORDER : SCROLL_DOWN_MAX;
}

// b = the y offset
void Camera::scroll_down()
{
	uint8_t lower = mem_[wRoomLowerYLimit];
	cpu_.c = static_cast<uint8_t>(lower - 1);
	unsigned lo = mem_[wCameraSCY + 1] + cpu_.b;
	mem_[wCameraSCY + 1] = static_cast<uint8_t>(lo);
	uint8_t hi = static_cast<uint8_t>(mem_[wCameraSCY + 0] + (lo > 0xff));
	mem_[wCameraSCY + 0] = hi;
	cpu_.a = hi;
	cpu_.f = flags::cp(hi, cpu_.c);
	if (!cf()) {
		cpu_.b = lower_y_scroll();
		ld_cp(wCameraSCY + 1, cpu_.b);
		if (!cf()) { // at the most it scrolls
			mem_[wCameraSCY + 1] = cpu_.b;
			mem_[wScrollDownEdge] = 1;
			cpu_.a = 1;
			return;
		}
	}
	// .not_on_limit: Wario at the room's top: the camera follows him back
	if (transitions())
		return;
	cpu_.c = mem_[wRoomUpperYLimit];
	ld_cp(wWarioYPos + 0, cpu_.c);
	if (!cf()) {
		if (!zf())
			return;
		uint8_t prev = mem_[wPrevWarioScreenYPos];
		cpu_.f = flags::sub(prev, 0x10);
		cpu_.c = static_cast<uint8_t>(prev - 0x10);
		if (config_bit(CAM_BORDER_UP_F)) {
			cpu_.f = flags::add(cpu_.c, CAMERA_BORDER_UPPER_H);
			cpu_.c = static_cast<uint8_t>(cpu_.c + CAMERA_BORDER_UPPER_H);
		}
		ld_cp(wWarioYPos + 1, cpu_.c);
		if (!cf())
			return; // nothing to do
	}
	mem_[wCameraSCY + 1] = config_bit(CAM_BORDER_UP_F) ? CAMERA_BORDER_UPPER_H : 0;
	mem_[wCameraSCY + 0] = mem_[wRoomUpperYLimit];
	mem_[wScrollDownEdge] = 2;
	cpu_.a = 2;
}

// b = the y offset
void Camera::scroll_up()
{
	uint8_t upper = mem_[wRoomUpperYLimit];
	cpu_.c = upper;
	uint8_t old_lo = mem_[wCameraSCY + 1];
	uint8_t lo = static_cast<uint8_t>(old_lo - cpu_.b);
	mem_[wCameraSCY + 1] = lo;
	uint8_t hi = static_cast<uint8_t>(mem_[wCameraSCY + 0] - (old_lo < cpu_.b));
	mem_[wCameraSCY + 0] = hi;

	cpu_.b = config_bit(CAM_BORDER_UP_F) ? CAMERA_BORDER_UPPER_H + 0x10 : 0x10;
	unsigned ahead = lo + 0x10;
	cpu_.l = static_cast<uint8_t>(ahead);
	uint8_t ahead_hi = static_cast<uint8_t>(hi + (ahead > 0xff));
	cpu_.a = ahead_hi;
	cpu_.f = flags::cp(ahead_hi, upper);
	bool upper_edge = cf();
	if (!upper_edge && zf()) {
		cpu_.a = cpu_.l;
		cpu_.f = flags::cp(cpu_.l, cpu_.b);
		upper_edge = cf();
	}
	if (upper_edge) { // at the room's top
		mem_[wCameraSCY + 0] = upper;
		cpu_.f = flags::sub(cpu_.b, 0x10);
		mem_[wCameraSCY + 1] = static_cast<uint8_t>(cpu_.b - 0x10);
		mem_[wScrollUpEdge] = 1;
		cpu_.a = 1;
		return;
	}
	// .not_on_limit: Wario at the room's bottom: the camera follows him back
	if (transitions())
		return;
	uint8_t lower = mem_[wRoomLowerYLimit];
	cpu_.f = flags::dec(cpu_.f, lower);
	cpu_.c = static_cast<uint8_t>(lower - 1);
	ld_cp(wWarioYPos + 0, cpu_.c);
	if (!zf())
		return;
	uint8_t from_bottom = static_cast<uint8_t>(SCREEN_HEIGHT_PX + 0x10 - mem_[wPrevWarioScreenYPos]);
	cpu_.f = flags::sub(0, from_bottom); // xor a / sub c
	cpu_.c = static_cast<uint8_t>(-from_bottom);
	if (config_bit(CAM_BORDER_DOWN_F)) {
		cpu_.f = flags::sub(cpu_.c, CAMERA_BORDER_LOWER_H);
		cpu_.c = static_cast<uint8_t>(cpu_.c - CAMERA_BORDER_LOWER_H);
	}
	ld_cp(wWarioYPos + 1, cpu_.c);
	if (cf())
		return;
	mem_[wScrollUpEdge] = 2;
	cpu_.a = lower_y_scroll();
	mem_[wCameraSCY + 1] = cpu_.a;
}

// Wario's position on the screen (the Hidden Figure's battle scrolls by
// wc08a / wc08c and the shake)
void Camera::set_wario_screen_pos()
{
	ld_cp(wCameraConfigFlags, HIDDEN_FIGURE_CAMCONFIG);
	bool hidden_figure = zf();
	if (hidden_figure) {
		cpu_.c = mem_[wSCYShake];
		cpu_.b = static_cast<uint8_t>(mem_[wc08a] + cpu_.c);
	} else {
		cpu_.b = mem_[wSCY];
	}
	mem_[wWarioScreenYPos] = static_cast<uint8_t>(mem_[wWarioYPos + 1] + OAM_Y_OFS - cpu_.b);
	cpu_.b = mem_[hidden_figure ? wc08c : wSCX];
	uint8_t x = static_cast<uint8_t>(mem_[wWarioXPos + 1] + OAM_X_OFS);
	cpu_.f = flags::sub(x, cpu_.b);
	cpu_.a = static_cast<uint8_t>(x - cpu_.b);
	mem_[wWarioScreenXPos] = cpu_.a;
}

// scrolls; with no edge reached, the edge's row or column is to be drawn;
// then the delta's cleared (xor a)
void Camera::scroll_and_flag(void (Camera::*scroll)(), uint16_t edge, int flag_bit, uint16_t delta)
{
	(this->*scroll)();
	if (!mem_[edge]) {
		SET_HL(gb_, wLevelEdgeUpdateFlags);
		mem_[wLevelEdgeUpdateFlags] |= 1 << flag_bit;
	}
	mem_[delta] = 0;
	cpu_.a = 0;
	cpu_.f = FLAG_Z;
}

// ld a, [counter] / inc a / ld [counter], a / dec a: true if it was 0
bool Camera::count_up_was_zero(uint16_t counter)
{
	uint8_t was = mem_[counter];
	mem_[counter] = static_cast<uint8_t>(was + 1);
	cpu_.f = flags::dec(flags::inc(cpu_.f, was), static_cast<uint8_t>(was + 1));
	cpu_.a = was;
	return !was;
}

// scrolls up or down by wCameraYDelta, or towards Wario
void Camera::level_scroll_vertical()
{
	uint8_t delta = mem_[wCameraYDelta];
	cpu_.a = delta;
	cpu_.b = delta;
	cpu_.f = flags::and_(delta);
	auto down = [&] { scroll_and_flag(&Camera::scroll_down, wScrollDownEdge, LEVEL_EDGE_UPDATE_DOWN_F, wCameraYDelta); };
	auto up = [&] { scroll_and_flag(&Camera::scroll_up, wScrollUpEdge, LEVEL_EDGE_UPDATE_UP_F, wCameraYDelta); };
	if (delta) {
		mem_[wc0d4] = 0;
		SET_HL(gb_, wWarioScreenYPos);
		uint8_t prev = mem_[wPrevWarioScreenYPos], now = mem_[wWarioScreenYPos];
		if (!(delta & 0x80)) {
			cpu_.a = prev;
			cpu_.f = flags::cp(prev, now);
			if (cf()) {
				cpu_.f = flags::inc(cpu_.f, cpu_.b);
				cpu_.b++;
			}
			down();
		} else {
			cpu_.b = static_cast<uint8_t>(-delta); // cpl / inc a
			cpu_.a = prev;
			cpu_.f = flags::cp(prev, now);
			if (!(cpu_.f & (FLAG_C | FLAG_Z))) {
				cpu_.f = flags::inc(cpu_.f, cpu_.b);
				cpu_.b++;
			}
			up();
		}
		return;
	}

	// .no_y_offset
	cpu_.a = mem_[wWaterInteraction];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return; // in water
	cpu_.a = mem_[wJumpVelIndex];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return; // jumping
	uint8_t state = mem_[wWarioState];
	cpu_.a = state;
	cpu_.f = flags::cp(state, WST_LADDER_SCRATCHING);
	if (zf())
		return;
	cpu_.f = flags::cp(state, WST_LADDER_CLIMBING);
	if (!cf()) {
		cpu_.f = flags::cp(state, WST_LADDER_SLIDING + 1);
		if (cf())
			return;
	}
	if (count_up_was_zero(wc0d4))
		return;

	SET_HL(gb_, wWarioScreenYPos);
	uint8_t prev = mem_[wPrevWarioScreenYPos], now = mem_[wWarioScreenYPos];
	uint8_t distance = static_cast<uint8_t>(prev - now);
	cpu_.f = flags::sub(prev, now);
	if (cf())
		distance = static_cast<uint8_t>(-distance);
	cpu_.a = distance;
	cpu_.b = distance;
	cpu_.a = mem_[wWarioSlopeInteraction];
	cpu_.f = flags::and_(cpu_.a);
	if (!cpu_.a) {
		cpu_.a = cpu_.b;
		cpu_.f = flags::cp(cpu_.b, 8);
		if (cf())
			return; // less than 8 pixels
		cpu_.b = 7;
	}
	// .on_slope
	cpu_.a = prev;
	cpu_.f = flags::cp(prev, now);
	if (cf()) {
		down();
		return;
	}
	if (zf())
		return;
	up();
}

// scrolls right or left by wCameraXDelta, or towards Wario's spacing
void Camera::level_scroll_horizontal()
{
	uint8_t delta = mem_[wCameraXDelta];
	cpu_.a = delta;
	cpu_.b = delta;
	cpu_.f = flags::and_(delta);
	auto right = [&] { scroll_and_flag(&Camera::scroll_right, wScrollRightEdge, LEVEL_EDGE_UPDATE_RIGHT_F, wCameraXDelta); };
	auto left = [&] { scroll_and_flag(&Camera::scroll_left, wScrollLeftEdge, LEVEL_EDGE_UPDATE_LEFT_F, wCameraXDelta); };
	if (delta) {
		mem_[wc0d5] = 0;
		bool going_left = delta & 0x80;
		if (going_left)
			cpu_.b = static_cast<uint8_t>(-delta);
		SET_HL(gb_, wWarioScreenXPos);
		uint8_t space = spacing();
		cpu_.f = flags::cp(space, mem_[wWarioScreenXPos]);
		if (!zf()) {
			// past the spacing: faster towards it, slower away
			bool faster = cf() ? !going_left : going_left;
			if (faster) {
				cpu_.f = flags::inc(cpu_.f, cpu_.b);
				cpu_.b++;
			} else {
				cpu_.f = flags::dec(cpu_.f, cpu_.b);
				cpu_.b--;
			}
		}
		if (going_left)
			left();
		else
			right();
		return;
	}

	// .no_x_offset
	ld_cp(wTransformation, TRANSFORMATION_FLAT_WARIO);
	if (zf()) {
		cpu_.a = mem_[wJumpVelTable];
		cpu_.f = flags::and_(cpu_.a);
		if (cpu_.a)
			return; // jumping
	}
	if (count_up_was_zero(wc0d5))
		return;
	cpu_.b = 1;
	SET_HL(gb_, wWarioScreenXPos);
	uint8_t space = spacing();
	cpu_.f = flags::cp(space, mem_[wWarioScreenXPos]);
	if (cf()) {
		right();
		return;
	}
	if (zf())
		return;
	left();
}

} // namespace

} // namespace wl3::level

using wl3::level::Camera;

// b = the offset: a = the edge reached (or the last value read), with the
// flags, b and c as the original leaves them
PORT(ScrollCameraRight) { Camera(gb).scroll_right(); }
PORT(ScrollCameraLeft) { Camera(gb).scroll_left(); }
PORT(ScrollCameraDown) { Camera(gb).scroll_down(); }
PORT(ScrollCameraUp) { Camera(gb).scroll_up(); }
PORT(SetWarioScreenPos) { Camera(gb).set_wario_screen_pos(); }
PORT(LevelScroll_Vertical) { Camera(gb).level_scroll_vertical(); }
PORT(LevelScroll_Horizontal) { Camera(gb).level_scroll_horizontal(); }
