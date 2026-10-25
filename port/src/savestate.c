#include "savestate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC "WL3STATE"
// Bump whenever the layout below changes.
#define VERSION 1

// One function describes the layout and is used for both directions, so
// saving and loading can't drift apart.
typedef struct {
	bool loading;
	uint8_t *buf;
	size_t len, cap;
	bool error;
} Ser;

static void bytes(Ser *s, void *p, size_t n)
{
	if (s->loading) {
		if (s->len + n > s->cap) {
			s->error = true;
			return;
		}
		memcpy(p, s->buf + s->len, n);
	} else {
		if (s->len + n > s->cap) {
			while (s->len + n > s->cap)
				s->cap = s->cap ? s->cap * 2 : 1 << 16;
			s->buf = realloc(s->buf, s->cap);
		}
		memcpy(s->buf + s->len, p, n);
	}
	s->len += n;
}

// Integers are stored little-endian, whatever the host.
static void u64(Ser *s, uint64_t *v)
{
	uint8_t b[8];
	for (int i = 0; i < 8; i++)
		b[i] = (uint8_t)(*v >> (8 * i));
	bytes(s, b, 8);
	if (s->loading) {
		*v = 0;
		for (int i = 0; i < 8; i++)
			*v |= (uint64_t)b[i] << (8 * i);
	}
}

// For any integer or bool field.
#define SYNC_INT(s, field) do { \
	uint64_t tmp_ = (uint64_t)(int64_t)(field); \
	u64(s, &tmp_); \
	if ((s)->loading) \
		(field) = (int64_t)tmp_; \
} while (0)

static void u32(Ser *s, uint32_t *v)
{
	uint8_t b[4] = {(uint8_t)*v, (uint8_t)(*v >> 8), (uint8_t)(*v >> 16), (uint8_t)(*v >> 24)};
	bytes(s, b, 4);
	if (s->loading)
		*v = (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static void sync_double(Ser *s, double *d)
{
	uint64_t bits;
	memcpy(&bits, d, 8);
	u64(s, &bits);
	if (s->loading)
		memcpy(d, &bits, 8);
}

static void sync_channel(Ser *s, APUChannel *c)
{
	SYNC_INT(s, c->on);
	SYNC_INT(s, c->dac);
	SYNC_INT(s, c->length);
	SYNC_INT(s, c->length_enable);
	SYNC_INT(s, c->volume);
	SYNC_INT(s, c->env_period);
	SYNC_INT(s, c->env_timer);
	SYNC_INT(s, c->env_up);
	SYNC_INT(s, c->freq);
	SYNC_INT(s, c->timer);
	SYNC_INT(s, c->duty_pos);
	SYNC_INT(s, c->duty);
	SYNC_INT(s, c->sweep_period);
	SYNC_INT(s, c->sweep_timer);
	SYNC_INT(s, c->sweep_shift);
	SYNC_INT(s, c->sweep_down);
	SYNC_INT(s, c->sweep_enabled);
	SYNC_INT(s, c->sweep_shadow);
	SYNC_INT(s, c->wave_pos);
	SYNC_INT(s, c->lfsr);
}

static void sync_gb(Ser *s, GB *gb)
{
	CPU *c = &gb->cpu;
	SYNC_INT(s, c->a);
	SYNC_INT(s, c->f);
	SYNC_INT(s, c->b);
	SYNC_INT(s, c->c);
	SYNC_INT(s, c->d);
	SYNC_INT(s, c->e);
	SYNC_INT(s, c->h);
	SYNC_INT(s, c->l);
	SYNC_INT(s, c->sp);
	SYNC_INT(s, c->pc);
	SYNC_INT(s, c->ime);
	SYNC_INT(s, c->ime_delay);
	SYNC_INT(s, c->halted);
	SYNC_INT(s, c->stopped);
	SYNC_INT(s, c->double_speed);
	SYNC_INT(s, c->speed_switch_armed);

	PPU *p = &gb->ppu;
	SYNC_INT(s, p->lx);
	SYNC_INT(s, p->mode);
	SYNC_INT(s, p->stat_line);
	SYNC_INT(s, p->hdma_active);
	SYNC_INT(s, p->window_line);
	for (int i = 0; i < GB_SCREEN_W * GB_SCREEN_H; i++)
		u32(s, &p->framebuffer[i]);

	APU *a = &gb->apu;
	for (int i = 0; i < 4; i++)
		sync_channel(s, &a->ch[i]);
	SYNC_INT(s, a->power);
	SYNC_INT(s, a->frame_seq_timer);
	SYNC_INT(s, a->frame_seq_step);
	sync_double(s, &a->sample_acc);

	SYNC_INT(s, gb->rom_bank);
	SYNC_INT(s, gb->sram_bank);
	SYNC_INT(s, gb->sram_enabled);
	bytes(s, gb->vram, sizeof(gb->vram));
	bytes(s, gb->wram, sizeof(gb->wram));
	bytes(s, gb->sram, sizeof(gb->sram));
	bytes(s, gb->oam, sizeof(gb->oam));
	bytes(s, gb->io, sizeof(gb->io));
	bytes(s, gb->hram, sizeof(gb->hram));
	SYNC_INT(s, gb->ie);
	bytes(s, gb->bg_pal, sizeof(gb->bg_pal));
	bytes(s, gb->obj_pal, sizeof(gb->obj_pal));
	SYNC_INT(s, gb->div_counter);
	SYNC_INT(s, gb->joypad);
	SYNC_INT(s, gb->cycles);
	SYNC_INT(s, gb->lcd_off_dots);
	SYNC_INT(s, gb->frame_count);
}

static uint32_t rom_crc(const GB *gb)
{
	uint32_t c = 0xffffffffu;
	for (size_t i = 0; i < gb->rom_size; i++) {
		c ^= gb->rom[i];
		for (int k = 0; k < 8; k++)
			c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
	}
	return c ^ 0xffffffffu;
}

uint8_t *savestate_save(GB *gb, size_t *len)
{
	Ser s = {0};
	bytes(&s, MAGIC, 8);
	uint64_t version = VERSION, crc = rom_crc(gb);
	u64(&s, &version);
	u64(&s, &crc);
	sync_gb(&s, gb);
	*len = s.len;
	return s.buf;
}

bool savestate_load(GB *gb, const uint8_t *data, size_t len)
{
	Ser s = {.loading = true, .buf = (uint8_t *)data, .cap = len};
	char magic[8];
	uint64_t version = 0, crc = 0;
	bytes(&s, magic, 8);
	u64(&s, &version);
	u64(&s, &crc);
	if (s.error || memcmp(magic, MAGIC, 8)) {
		fprintf(stderr, "savestate: not a save state\n");
		return false;
	}
	if (version != VERSION) {
		fprintf(stderr, "savestate: made by a different version of the port (%llu, expected %d)\n",
			(unsigned long long)version, VERSION);
		return false;
	}
	if (crc != rom_crc(gb)) {
		fprintf(stderr, "savestate: made with a different build of the ROM\n");
		return false;
	}

	// Load into a copy so a truncated file can't leave a half-loaded state.
	GB *tmp = malloc(sizeof(GB));
	*tmp = *gb;
	uint32_t *framebuffer = malloc(GB_SCREEN_W * GB_SCREEN_H * sizeof(uint32_t));
	tmp->ppu.framebuffer = framebuffer;
	sync_gb(&s, tmp);
	bool ok = !s.error && s.len == len;
	if (ok) {
		memcpy(gb->ppu.framebuffer, framebuffer, GB_SCREEN_W * GB_SCREEN_H * sizeof(uint32_t));
		tmp->ppu.framebuffer = gb->ppu.framebuffer;
		tmp->ppu.frame_ready = false;
		tmp->apu.buffer_len = 0;
		tmp->sram_dirty = false;
		*gb = *tmp;
	} else {
		fprintf(stderr, "savestate: file is truncated or corrupt\n");
	}
	free(framebuffer);
	free(tmp);
	return ok;
}

bool savestate_save_file(GB *gb, const char *path)
{
	size_t len;
	uint8_t *data = savestate_save(gb, &len);
	FILE *f = fopen(path, "wb");
	bool ok = f && fwrite(data, 1, len, f) == len;
	if (f && fclose(f))
		ok = false;
	free(data);
	return ok;
}

bool savestate_load_file(GB *gb, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "savestate: can't read %s\n", path);
		return false;
	}
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *data = malloc((size_t)n);
	bool ok = fread(data, 1, (size_t)n, f) == (size_t)n && savestate_load(gb, data, (size_t)n);
	fclose(f);
	free(data);
	return ok;
}
