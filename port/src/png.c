// Minimal PNG writer (uncompressed deflate) for screenshots.
#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc_table[256];

static void crc_init(void)
{
	for (uint32_t n = 0; n < 256; n++) {
		uint32_t c = n;
		for (int k = 0; k < 8; k++)
			c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
}

static uint32_t crc(uint32_t c, const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++)
		c = crc_table[(c ^ buf[i]) & 0xff] ^ (c >> 8);
	return c;
}

static void put32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len)
{
	uint8_t hdr[8];
	put32(hdr, len);
	memcpy(hdr + 4, type, 4);
	fwrite(hdr, 1, 8, f);
	if (len)
		fwrite(data, 1, len, f);
	uint32_t c = crc(0xffffffffu, (const uint8_t *)type, 4);
	c = crc(c, data, len) ^ 0xffffffffu;
	uint8_t tail[4];
	put32(tail, c);
	fwrite(tail, 1, 4, f);
}

bool png_write(const char *path, const uint32_t *argb, int w, int h)
{
	crc_init();
	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
	fwrite(sig, 1, 8, f);

	uint8_t ihdr[13];
	put32(ihdr, (uint32_t)w);
	put32(ihdr + 4, (uint32_t)h);
	ihdr[8] = 8;  // bit depth
	ihdr[9] = 2;  // RGB
	ihdr[10] = ihdr[11] = ihdr[12] = 0;
	chunk(f, "IHDR", ihdr, 13);

	// Raw scanlines with filter byte 0.
	size_t row = (size_t)w * 3 + 1;
	size_t raw_len = row * (size_t)h;
	uint8_t *raw = malloc(raw_len);
	for (int y = 0; y < h; y++) {
		uint8_t *r = raw + row * (size_t)y;
		r[0] = 0;
		for (int x = 0; x < w; x++) {
			uint32_t p = argb[y * w + x];
			r[1 + x * 3] = (uint8_t)(p >> 16);
			r[2 + x * 3] = (uint8_t)(p >> 8);
			r[3 + x * 3] = (uint8_t)p;
		}
	}

	// zlib stream made of stored deflate blocks.
	size_t nblocks = raw_len / 65535 + 1;
	size_t z_len = 2 + raw_len + nblocks * 5 + 4;
	uint8_t *z = malloc(z_len);
	size_t zp = 0;
	z[zp++] = 0x78;
	z[zp++] = 0x01;
	uint32_t a = 1, b = 0;
	for (size_t off = 0; off < raw_len || off == 0;) {
		size_t n = raw_len - off > 65535 ? 65535 : raw_len - off;
		bool last = off + n >= raw_len;
		z[zp++] = last ? 1 : 0;
		z[zp++] = (uint8_t)n;
		z[zp++] = (uint8_t)(n >> 8);
		z[zp++] = (uint8_t)~n;
		z[zp++] = (uint8_t)(~n >> 8);
		memcpy(z + zp, raw + off, n);
		for (size_t i = 0; i < n; i++) {
			a = (a + raw[off + i]) % 65521;
			b = (b + a) % 65521;
		}
		zp += n;
		off += n;
		if (last)
			break;
	}
	put32(z + zp, b << 16 | a);
	zp += 4;
	chunk(f, "IDAT", z, (uint32_t)zp);
	chunk(f, "IEND", NULL, 0);

	free(raw);
	free(z);
	return fclose(f) == 0;
}
