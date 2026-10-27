// Port of src/engine/overworld/init.asm
#include "ported.h"

// decompress_tiles0: ld a, BANK(gfx) / ld [wTempBank], a / ld hl, gfx /
// ld bc, dest / ld a, [wTempBank] / ldh [hCallFuncBank], a / hcall Decompress
static void decompress_tiles0(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	wr8(gb, wTempBank, bank);
	SET_HL(gb, gfx);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_tiles1: the same into VRAM bank 1, then back to bank 0
static void decompress_tiles1(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	wr8(gb, rVBK, BANK_v1Tiles0);
	decompress_tiles0(gb, bank, gfx, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// d:bc = tilemap, e:hl = attrmap
PORT(LoadBGMapsToWRAM)
{
	gb_push16(gb, REG_BC(gb)); // push bc
	gb->cpu.a = gb->cpu.d;
	wr8(gb, w2dfff, gb->cpu.a);
	wr8(gb, rVBK, BANK_v1Tiles0);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, wTempBank, gb->cpu.a);
	SET_BC(gb, wAttrmap);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, gb_pop16(gb)); // pop hl

	gb->cpu.a = rd8(gb, w2dfff);
	wr8(gb, wTempBank, gb->cpu.a);
	SET_BC(gb, w3d500);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

PORT(LoadOverworldCommonGfx)
{
	decompress_tiles1(gb, BANK_OverworldCommonGfx, ADDR_OverworldCommonGfx, v1Tiles0);
	decompress_tiles1(gb, BANK_OverworldNumbersGfx, ADDR_OverworldNumbersGfx, v1Tiles0 + 0x60 * 0x10);
}

PORT(LoadOverworld1Gfx) { decompress_tiles0(gb, BANK_Overworld1Gfx, ADDR_Overworld1Gfx, v0Tiles0); }
PORT(LoadOverworld2Gfx) { decompress_tiles0(gb, BANK_Overworld2Gfx, ADDR_Overworld2Gfx, v0Tiles0); }
PORT(LoadOverworld5Gfx) { decompress_tiles0(gb, BANK_Overworld5Gfx, ADDR_Overworld5Gfx, v0Tiles0); }
PORT(LoadOverworld6Gfx) { decompress_tiles0(gb, BANK_Overworld6Gfx, ADDR_Overworld6Gfx, v0Tiles0); }
PORT(LoadOverworldArrowsGfx) { decompress_tiles0(gb, BANK_OverworldArrowsGfx, ADDR_OverworldArrowsGfx, v0Tiles1); }

PORT(LoadOverworldGlowGfx)
{
	decompress_tiles1(gb, BANK_OverworldGlowGfx, ADDR_OverworldGlowGfx, v1Tiles0 + 0x20 * 0x10);
}

PORT(ClearOWWRAM)
{
	SET_HL(gb, 0xd060); // STARTOF("OW WRAM")
	SET_BC(gb, 0x07a0); // SIZEOF("OW WRAM")
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	CALL(WriteAToHL_BCTimes);
}

PORT(SetStoredTransitionParamAsReturnToMap)
{
	gb->cpu.a = rd8(gb, wOWBarsState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // is manual cutscene
	gb->cpu.a = TRANSITION_RETURN_TO_MAP;
	wr8(gb, wStoredTransitionParam, gb->cpu.a);
}
