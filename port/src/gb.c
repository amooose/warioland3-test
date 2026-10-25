// Machine setup, timing and save data.
#include "gb.h"
#include "hooks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *read_file(const char *path, size_t *size)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *buf = malloc((size_t)n);
	if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) {
		free(buf);
		buf = NULL;
	}
	fclose(f);
	*size = (size_t)n;
	return buf;
}

bool gb_init(GB *gb, const char *rom_path)
{
	memset(gb, 0, sizeof(*gb));
	gb->rom = read_file(rom_path, &gb->rom_size);
	if (!gb->rom) {
		fprintf(stderr, "can't read ROM %s\n", rom_path);
		return false;
	}
	if (gb->rom_size < 0x8000 || gb->rom_size % 0x4000) {
		fprintf(stderr, "%s doesn't look like a ROM\n", rom_path);
		return false;
	}
	gb->rom_banks = (int)(gb->rom_size / 0x4000);
	gb->ppu.framebuffer = calloc(GB_SCREEN_W * GB_SCREEN_H, sizeof(uint32_t));
	gb->apu.buffer = calloc(AUDIO_BUFFER_FRAMES * 2, sizeof(int16_t));
	gb_reset(gb);
	return true;
}

void gb_free(GB *gb)
{
	free(gb->rom);
	free(gb->ppu.framebuffer);
	free(gb->apu.buffer);
	gb->rom = NULL;
}

// State after the CGB boot ROM hands over to the cartridge.
void gb_reset(GB *gb)
{
	memset(&gb->cpu, 0, sizeof(gb->cpu));
	uint32_t *framebuffer = gb->ppu.framebuffer;
	memset(&gb->ppu, 0, sizeof(gb->ppu));
	gb->ppu.framebuffer = framebuffer;
	memset(gb->vram, 0, sizeof(gb->vram));
	memset(gb->wram, 0, sizeof(gb->wram));
	memset(gb->oam, 0, sizeof(gb->oam));
	memset(gb->io, 0, sizeof(gb->io));
	memset(gb->hram, 0, sizeof(gb->hram));
	memset(gb->bg_pal, 0xff, sizeof(gb->bg_pal));
	memset(gb->obj_pal, 0xff, sizeof(gb->obj_pal));
	apu_reset(gb);

	// A = $11 is how the game detects a Game Boy Color.
	gb->cpu.a = 0x11;
	gb->cpu.f = 0x80;
	gb->cpu.b = 0x00;
	gb->cpu.c = 0x00;
	gb->cpu.d = 0xff;
	gb->cpu.e = 0x56;
	gb->cpu.h = 0x00;
	gb->cpu.l = 0x0d;
	gb->cpu.sp = 0xfffe;
	gb->cpu.pc = 0x0100;

	gb->rom_bank = 1;
	gb->sram_bank = 0;
	gb->sram_enabled = false;
	gb->ie = 0;
	gb->io[0x00] = 0x30;
	gb->io[0x40] = 0x91;
	gb->io[0x47] = 0xfc;
	gb->io[0x55] = 0xff;
	apu_write(gb, 0x26, 0x80);
	apu_write(gb, 0x24, 0x77);
	apu_write(gb, 0x25, 0xf3);
	gb->ppu.mode = 1;
}

static void timer_tick(GB *gb, int cycles)
{
	static const int tac_bits[4] = {9, 3, 5, 7};
	uint8_t tac = gb->io[0x07];
	for (int i = 0; i < cycles; i += 4) {
		uint16_t old = gb->div_counter;
		gb->div_counter += 4;
		if (!(tac & 4))
			continue;
		int bit = tac_bits[tac & 3];
		if ((old >> bit & 1) && !(gb->div_counter >> bit & 1)) {
			if (++gb->io[0x05] == 0) {
				gb->io[0x05] = gb->io[0x06];
				gb->io[0x0f] |= INT_TIMER;
			}
		}
	}
}

void gb_tick(GB *gb, int cycles)
{
	gb->cycles += (uint64_t)cycles;
	timer_tick(gb, cycles);
	// The PPU and APU run in real time, so in double speed mode they see
	// half as many cycles as the CPU executes.
	int dots = gb->cpu.double_speed ? cycles / 2 : cycles;
	ppu_tick(gb, dots);
	apu_tick(gb, dots);
}

void gb_run_frame(GB *gb)
{
	gb->ppu.frame_ready = false;
	while (!gb->ppu.frame_ready)
		gb_tick(gb, cpu_step(gb));
}

bool gb_load_sram(GB *gb, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return false;
	size_t n = fread(gb->sram, 1, sizeof(gb->sram), f);
	fclose(f);
	return n > 0;
}

bool gb_save_sram(GB *gb, const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	// Same layout as emulator saves, so they're interchangeable.
	size_t n = fwrite(gb->sram, 1, sizeof(gb->sram), f);
	fclose(f);
	gb->sram_dirty = false;
	return n == sizeof(gb->sram);
}
