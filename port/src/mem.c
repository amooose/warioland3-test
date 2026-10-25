// Memory map: MBC5 cartridge, banked WRAM/VRAM, I/O registers.
#include "gb.h"

#include <string.h>

static const uint8_t *rom_bank_ptr(GB *gb, int bank)
{
	bank %= gb->rom_banks;
	return gb->rom + (size_t)bank * 0x4000;
}

static int wram_bank(GB *gb)
{
	int b = gb->io[0x70] & 7;
	return b ? b : 1;
}

uint8_t gb_read(GB *gb, uint16_t addr)
{
	switch (addr >> 12) {
	case 0x0: case 0x1: case 0x2: case 0x3:
		return gb->rom[addr];
	case 0x4: case 0x5: case 0x6: case 0x7:
		return rom_bank_ptr(gb, gb->rom_bank)[addr - 0x4000];
	case 0x8: case 0x9:
		return gb->vram[gb->io[0x4f] & 1][addr - 0x8000];
	case 0xa: case 0xb:
		if (!gb->sram_enabled)
			return 0xff;
		return gb->sram[gb->sram_bank % GB_SRAM_BANKS][addr - 0xa000];
	case 0xc:
		return gb->wram[0][addr - 0xc000];
	case 0xd:
		return gb->wram[wram_bank(gb)][addr - 0xd000];
	case 0xe:
		return gb->wram[0][addr - 0xe000];
	default:
		if (addr < 0xfe00)
			return gb->wram[wram_bank(gb)][addr - 0xf000];
		if (addr < 0xfea0)
			return gb->oam[addr - 0xfe00];
		if (addr < 0xff00)
			return 0x00;
		if (addr < 0xff80)
			return gb_io_read(gb, (uint8_t)(addr - 0xff00));
		if (addr < 0xffff)
			return gb->hram[addr - 0xff80];
		return gb->ie;
	}
}

void gb_write(GB *gb, uint16_t addr, uint8_t val)
{
	switch (addr >> 12) {
	case 0x0: case 0x1:
		gb->sram_enabled = (val & 0x0f) == 0x0a;
		return;
	case 0x2:
		gb->rom_bank = (gb->rom_bank & 0x100) | val;
		return;
	case 0x3:
		gb->rom_bank = (gb->rom_bank & 0xff) | ((val & 1) << 8);
		return;
	case 0x4: case 0x5:
		gb->sram_bank = val & 0x0f;
		return;
	case 0x6: case 0x7:
		return;
	case 0x8: case 0x9:
		gb->vram[gb->io[0x4f] & 1][addr - 0x8000] = val;
		return;
	case 0xa: case 0xb:
		if (gb->sram_enabled) {
			gb->sram[gb->sram_bank % GB_SRAM_BANKS][addr - 0xa000] = val;
			gb->sram_dirty = true;
		}
		return;
	case 0xc:
		gb->wram[0][addr - 0xc000] = val;
		return;
	case 0xd:
		gb->wram[wram_bank(gb)][addr - 0xd000] = val;
		return;
	case 0xe:
		gb->wram[0][addr - 0xe000] = val;
		return;
	default:
		if (addr < 0xfe00)
			gb->wram[wram_bank(gb)][addr - 0xf000] = val;
		else if (addr < 0xfea0)
			gb->oam[addr - 0xfe00] = val;
		else if (addr < 0xff00)
			;
		else if (addr < 0xff80)
			gb_io_write(gb, (uint8_t)(addr - 0xff00), val);
		else if (addr < 0xffff)
			gb->hram[addr - 0xff80] = val;
		else
			gb->ie = val;
		return;
	}
}

uint16_t gb_read16(GB *gb, uint16_t addr)
{
	return (uint16_t)(gb_read(gb, addr) | gb_read(gb, (uint16_t)(addr + 1)) << 8);
}

void gb_write16(GB *gb, uint16_t addr, uint16_t val)
{
	gb_write(gb, addr, (uint8_t)val);
	gb_write(gb, (uint16_t)(addr + 1), (uint8_t)(val >> 8));
}

void gb_push16(GB *gb, uint16_t val)
{
	gb->cpu.sp -= 2;
	gb_write16(gb, gb->cpu.sp, val);
}

uint16_t gb_pop16(GB *gb)
{
	uint16_t v = gb_read16(gb, gb->cpu.sp);
	gb->cpu.sp += 2;
	return v;
}

static uint8_t joypad_read(GB *gb)
{
	uint8_t sel = gb->io[0x00] & 0x30;
	uint8_t res = 0x0f;
	if (!(sel & 0x10))
		res &= (uint8_t)~(gb->joypad & 0x0f);
	if (!(sel & 0x20))
		res &= (uint8_t)~((gb->joypad >> 4) & 0x0f);
	return 0xc0 | sel | res;
}

uint8_t gb_io_read(GB *gb, uint8_t reg)
{
	switch (reg) {
	case 0x00:
		return joypad_read(gb);
	case 0x04:
		return (uint8_t)(gb->div_counter >> 8);
	case 0x0f:
		return gb->io[0x0f] | 0xe0;
	case 0x41: {
		uint8_t stat = (gb->io[0x41] & 0x78) | 0x80;
		if (gb->io[0x40] & 0x80) {
			stat |= (uint8_t)gb->ppu.mode;
			if (gb->io[0x44] == gb->io[0x45])
				stat |= 0x04;
		}
		return stat;
	}
	case 0x4d:
		return (gb->cpu.double_speed ? 0x80 : 0) | (gb->cpu.speed_switch_armed ? 1 : 0) | 0x7e;
	case 0x4f:
		return 0xfe | (gb->io[0x4f] & 1);
	case 0x55:
		return gb->ppu.hdma_active ? (gb->io[0x55] & 0x7f) : 0xff;
	case 0x69:
		return gb->bg_pal[gb->io[0x68] & 0x3f];
	case 0x6b:
		return gb->obj_pal[gb->io[0x6a] & 0x3f];
	case 0x70:
		return 0xf8 | (gb->io[0x70] & 7);
	default:
		if (reg >= 0x10 && reg < 0x40)
			return apu_read(gb, reg);
		return gb->io[reg];
	}
}

static void oam_dma(GB *gb, uint8_t val)
{
	uint16_t src = (uint16_t)(val << 8);
	for (int i = 0; i < 0xa0; i++)
		gb->oam[i] = gb_read(gb, (uint16_t)(src + i));
}

void gb_io_write(GB *gb, uint8_t reg, uint8_t val)
{
	if (reg >= 0x10 && reg < 0x40) {
		apu_write(gb, reg, val);
		return;
	}
	switch (reg) {
	case 0x00:
		gb->io[0x00] = val & 0x30;
		return;
	case 0x04:
		gb->div_counter = 0;
		return;
	case 0x0f:
		gb->io[0x0f] = val & 0x1f;
		return;
	case 0x40:
		if ((gb->io[0x40] & 0x80) && !(val & 0x80))
			ppu_lcd_off(gb);
		gb->io[0x40] = val;
		return;
	case 0x41:
		gb->io[0x41] = (gb->io[0x41] & 0x07) | (val & 0x78);
		return;
	case 0x44:
		return; // LY is read-only
	case 0x46:
		oam_dma(gb, val);
		return;
	case 0x4d:
		gb->cpu.speed_switch_armed = val & 1;
		return;
	case 0x4f:
		gb->io[0x4f] = val & 1;
		return;
	case 0x55:
		ppu_hdma_start(gb, val);
		return;
	case 0x68:
	case 0x6a:
		gb->io[reg] = val & 0xbf;
		return;
	case 0x69:
	case 0x6b: {
		uint8_t idx_reg = (uint8_t)(reg - 1);
		uint8_t *pal = reg == 0x69 ? gb->bg_pal : gb->obj_pal;
		uint8_t idx = gb->io[idx_reg];
		pal[idx & 0x3f] = val;
		if (idx & 0x80)
			gb->io[idx_reg] = 0x80 | ((idx + 1) & 0x3f);
		return;
	}
	case 0x70:
		gb->io[0x70] = val & 7;
		return;
	default:
		gb->io[reg] = val;
		return;
	}
}
