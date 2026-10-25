// Port of src/home/backup_vram.asm
#include "ported.h"

// push_wram: ldh a, [rWBK] / push af / ld a, bank / ldh [rWBK], a
static void push_wram(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = bank;
	wr8(gb, rWBK, bank);
}

// pop_wram: pop af / ldh [rWBK], a
static void pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

static void copy_bc(GB *gb, uint16_t src, uint16_t dst, uint16_t n)
{
	SET_HL(gb, src);
	SET_DE(gb, dst);
	SET_BC(gb, n);
	CALL(CopyHLToDE_BC);
}

// xor a / ldh [rVBK], a
static void vram0(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, rVBK, 0);
}

PORT(SaveBackupVRAM)
{
	push_wram(gb, BANK_w3d500);
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, w3d500, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, w3d501, gb->cpu.a);
	SET_HL(gb, wTempPals1);
	SET_DE(gb, w3d280);
	gb->cpu.b = 16 * PAL_SIZE;
	CALL(CopyHLToDE);
	pop_wram(gb);

	vram0(gb);
	push_wram(gb, BANK_w4d000);
	copy_bc(gb, v0Tiles0, w4d000, v0Tiles2 - v0Tiles0);
	pop_wram(gb);

	push_wram(gb, BANK_w5d000);
	copy_bc(gb, v0Tiles2, w5d000, v0End - v0Tiles2);
	pop_wram(gb);

	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	push_wram(gb, BANK_w6d000);
	copy_bc(gb, v1Tiles0, w6d000, v1Tiles2 - v1Tiles0);
	pop_wram(gb);

	push_wram(gb, BANK_w7d000);
	copy_bc(gb, v1Tiles2, w7d000, v1End - v1Tiles2);
	pop_wram(gb);

	vram0(gb);
}

PORT(LoadBackupVRAM)
{
	push_wram(gb, 0x03);
	gb->cpu.a = rd8(gb, w3d500);
	wr8(gb, wSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, w3d501);
	wr8(gb, wSCX, gb->cpu.a);
	SET_HL(gb, w3d280);
	SET_DE(gb, wTempPals1);
	gb->cpu.b = 16 * PAL_SIZE;
	CALL(CopyHLToDE);
	pop_wram(gb);
	vram0(gb);

	push_wram(gb, BANK_w4d000);
	copy_bc(gb, w4d000, v0Tiles0, 0x1000);
	pop_wram(gb);

	push_wram(gb, BANK_w5d000);
	copy_bc(gb, w5d000, v0Tiles2, 0x1000);
	pop_wram(gb);

	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	push_wram(gb, BANK_w6d000);
	copy_bc(gb, w6d000, v1Tiles0, 0x1000);
	pop_wram(gb);

	push_wram(gb, BANK_w7d000);
	copy_bc(gb, w7d000, v1Tiles2, 0x1000);
	pop_wram(gb);
	vram0(gb);
}
