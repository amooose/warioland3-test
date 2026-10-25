// CGB picture processing unit: a scanline renderer.
//
// Each line is drawn in one go at the end of mode 3, using the register values
// at that moment. Mid-line raster tricks aren't supported, but per-line
// effects (scroll changes in the STAT/HBlank interrupt) work.
#include "gb.h"

#include <string.h>

#define LCDC (gb->io[0x40])
#define STAT (gb->io[0x41])
#define SCY (gb->io[0x42])
#define SCX (gb->io[0x43])
#define LY (gb->io[0x44])
#define LYC (gb->io[0x45])
#define WY (gb->io[0x4a])
#define WX (gb->io[0x4b])

enum { MODE_HBLANK, MODE_VBLANK, MODE_OAM, MODE_DRAW };

static uint32_t cgb_color(const uint8_t *pal, int palette, int color)
{
	int i = palette * 8 + color * 2;
	uint16_t c = (uint16_t)(pal[i] | pal[i + 1] << 8);
	uint32_t r = c & 0x1f, g = (c >> 5) & 0x1f, b = (c >> 10) & 0x1f;
	r = r << 3 | r >> 2;
	g = g << 3 | g >> 2;
	b = b << 3 | b >> 2;
	return 0xff000000u | r << 16 | g << 8 | b;
}

static int tile_pixel(GB *gb, int bank, uint16_t tile_addr, int x, int y)
{
	const uint8_t *t = &gb->vram[bank][tile_addr - 0x8000 + y * 2];
	int bit = 7 - x;
	return ((t[0] >> bit) & 1) | (((t[1] >> bit) & 1) << 1);
}

static uint16_t bg_tile_addr(GB *gb, uint8_t tile)
{
	if (LCDC & 0x10)
		return (uint16_t)(0x8000 + tile * 16);
	return (uint16_t)(0x9000 + (int8_t)tile * 16);
}

static void render_bg_pixel(GB *gb, int sx, uint16_t map, int px, int py, uint32_t *out)
{
	uint16_t map_off = (uint16_t)(map - 0x8000 + (py / 8) * 32 + (px / 8));
	uint8_t tile = gb->vram[0][map_off];
	uint8_t attr = gb->vram[1][map_off];
	int tx = px & 7, ty = py & 7;
	if (attr & 0x20)
		tx = 7 - tx;
	if (attr & 0x40)
		ty = 7 - ty;
	int color = tile_pixel(gb, (attr >> 3) & 1, bg_tile_addr(gb, tile), tx, ty);
	gb->ppu.line_bg_color[sx] = (uint8_t)color;
	gb->ppu.line_bg_attr[sx] = attr;
	out[sx] = cgb_color(gb->bg_pal, attr & 7, color);
}

static void render_line(GB *gb)
{
	int y = LY;
	uint32_t *out = &gb->ppu.framebuffer[y * GB_SCREEN_W];

	// Background and window. In CGB mode they can't be turned off; LCDC
	// bit 0 instead strips their priority over sprites.
	uint16_t bg_map = (LCDC & 0x08) ? 0x9c00 : 0x9800;
	uint16_t win_map = (LCDC & 0x40) ? 0x9c00 : 0x9800;
	int wx = WX - 7;
	bool window = (LCDC & 0x20) && y >= WY && WX <= 166;
	for (int x = 0; x < GB_SCREEN_W; x++) {
		if (window && x >= wx)
			render_bg_pixel(gb, x, win_map, x - wx, gb->ppu.window_line, out);
		else
			render_bg_pixel(gb, x, bg_map, (x + SCX) & 0xff, (y + SCY) & 0xff, out);
	}
	if (window && wx < GB_SCREEN_W)
		gb->ppu.window_line++;

	if (!(LCDC & 0x02))
		return;

	// Sprites: up to 10 per line, chosen and prioritised by OAM order.
	int height = (LCDC & 0x04) ? 16 : 8;
	int selected[10];
	int count = 0;
	for (int i = 0; i < 40 && count < 10; i++) {
		int sy = gb->oam[i * 4] - 16;
		if (y >= sy && y < sy + height)
			selected[count++] = i;
	}
	// Draw lowest priority first so earlier sprites end up on top.
	for (int s = count - 1; s >= 0; s--) {
		const uint8_t *o = &gb->oam[selected[s] * 4];
		int sy = o[0] - 16, sx = o[1] - 8;
		uint8_t tile = o[2], attr = o[3];
		int ty = y - sy;
		if (attr & 0x40)
			ty = height - 1 - ty;
		if (height == 16)
			tile &= 0xfe;
		uint16_t addr = (uint16_t)(0x8000 + tile * 16);
		for (int px = 0; px < 8; px++) {
			int x = sx + px;
			if (x < 0 || x >= GB_SCREEN_W)
				continue;
			int tx = (attr & 0x20) ? 7 - px : px;
			int color = tile_pixel(gb, (attr >> 3) & 1, addr, tx, ty);
			if (!color)
				continue;
			int bg_color = gb->ppu.line_bg_color[x];
			if ((LCDC & 0x01) && bg_color &&
				((attr & 0x80) || (gb->ppu.line_bg_attr[x] & 0x80)))
				continue;
			out[x] = cgb_color(gb->obj_pal, attr & 7, color);
		}
	}
}

static void hdma_copy_block(GB *gb)
{
	uint16_t src = (uint16_t)((gb->io[0x51] << 8 | gb->io[0x52]) & 0xfff0);
	uint16_t dst = (uint16_t)(((gb->io[0x53] << 8 | gb->io[0x54]) & 0x1ff0) | 0x8000);
	for (int i = 0; i < 16; i++)
		gb->vram[gb->io[0x4f] & 1][(dst + i) & 0x1fff] = gb_read(gb, (uint16_t)(src + i));
	src += 16;
	dst += 16;
	gb->io[0x51] = (uint8_t)(src >> 8);
	gb->io[0x52] = (uint8_t)src;
	gb->io[0x53] = (uint8_t)(dst >> 8);
	gb->io[0x54] = (uint8_t)dst;
}

void ppu_hdma_start(GB *gb, uint8_t val)
{
	if (gb->ppu.hdma_active && !(val & 0x80)) {
		// Writing bit 7 = 0 during an HBlank transfer cancels it.
		gb->ppu.hdma_active = false;
		gb->io[0x55] = 0x80 | (val & 0x7f);
		return;
	}
	gb->io[0x55] = val & 0x7f;
	if (val & 0x80) {
		gb->ppu.hdma_active = true;
		return;
	}
	// General purpose DMA: everything at once.
	int blocks = (val & 0x7f) + 1;
	for (int i = 0; i < blocks; i++)
		hdma_copy_block(gb);
	gb->io[0x55] = 0xff;
}

static void hblank_hdma(GB *gb)
{
	if (!gb->ppu.hdma_active)
		return;
	hdma_copy_block(gb);
	if (gb->io[0x55] == 0) {
		gb->ppu.hdma_active = false;
		gb->io[0x55] = 0xff;
	} else {
		gb->io[0x55]--;
	}
}

static void update_stat_irq(GB *gb)
{
	bool line = false;
	if ((STAT & 0x40) && LY == LYC)
		line = true;
	if ((STAT & 0x08) && gb->ppu.mode == MODE_HBLANK)
		line = true;
	if ((STAT & 0x10) && gb->ppu.mode == MODE_VBLANK)
		line = true;
	if ((STAT & 0x20) && gb->ppu.mode == MODE_OAM)
		line = true;
	if (line && !gb->ppu.stat_line)
		gb->io[0x0f] |= INT_STAT;
	gb->ppu.stat_line = line;
}

static void frame_done(GB *gb)
{
	gb->frame_count++;
	gb->ppu.frame_ready = true;
	if (gb->frame_cb)
		gb->frame_cb(gb);
}

void ppu_lcd_off(GB *gb)
{
	LY = 0;
	gb->ppu.lx = 0;
	gb->ppu.mode = MODE_HBLANK;
	gb->ppu.window_line = 0;
	gb->lcd_off_dots = 0;
}

static void set_mode(GB *gb, int mode)
{
	gb->ppu.mode = mode;
	if (mode == MODE_HBLANK) {
		render_line(gb);
		hblank_hdma(gb);
	}
}

void ppu_tick(GB *gb, int dots)
{
	if (!(LCDC & 0x80)) {
		// Keep presenting frames while the screen is off so the frontend
		// stays responsive; the last image stays on screen.
		gb->lcd_off_dots += dots;
		while (gb->lcd_off_dots >= GB_CYCLES_PER_FRAME) {
			gb->lcd_off_dots -= GB_CYCLES_PER_FRAME;
			frame_done(gb);
		}
		return;
	}

	while (dots > 0) {
		// Advance to the next mode boundary or until we run out of dots.
		int next;
		if (LY >= 144)
			next = GB_CYCLES_PER_LINE;
		else if (gb->ppu.lx < 80)
			next = 80;
		else if (gb->ppu.lx < 252)
			next = 252;
		else
			next = GB_CYCLES_PER_LINE;
		int step = next - gb->ppu.lx;
		if (step > dots)
			step = dots;
		gb->ppu.lx += step;
		dots -= step;

		if (gb->ppu.lx == 80 && LY < 144) {
			set_mode(gb, MODE_DRAW);
		} else if (gb->ppu.lx == 252 && LY < 144) {
			set_mode(gb, MODE_HBLANK);
		} else if (gb->ppu.lx == GB_CYCLES_PER_LINE) {
			gb->ppu.lx = 0;
			LY++;
			if (LY == 144) {
				set_mode(gb, MODE_VBLANK);
				gb->io[0x0f] |= INT_VBLANK;
				update_stat_irq(gb);
				frame_done(gb);
			} else if (LY == GB_LINES_PER_FRAME) {
				LY = 0;
				gb->ppu.window_line = 0;
				set_mode(gb, MODE_OAM);
			} else if (LY < 144) {
				set_mode(gb, MODE_OAM);
			}
		}
		update_stat_irq(gb);
	}
}
