// overworld/bank26 (src/engine/overworld/bank26.asm): the level's
// treasures for the map's bottom bar and the levels (their IDs, palettes
// and tiles), and the treasure collection screen's V-Blank function.
// Plain C++ over memory, but for CheckLevelMusicalCoinFlag (a in, z out)
// and the V-Blank function (a for hTransferVirtualOAM).
#include "../asm_module.hpp"
#include "../home/memory.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank26 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// and the levels call (the collection screen is C++, src/native/overworld)

	void clear_treasure_data()
	{
		home::fill(gb_, wTileBuffer, 0, 16 * TILE_SIZE);
		home::fill(gb_, wLevelTreasureIDs, 0, 0x10);
	}

	// the level's 4 treasure IDs (inc e: within the page)
	void load_level_treasures()
	{
		Memory mem(gb_);
		uint16_t src = static_cast<uint16_t>(ADDR_LevelTreasureIDs + static_cast<uint8_t>(mem[wOWLevel] * 4));
		uint8_t at = static_cast<uint8_t>(wLevelTreasureIDs);
		for (int i = 0; i < NUM_LEVEL_TREASURES; i++)
			mem[static_cast<uint16_t>((wLevelTreasureIDs & 0xff00) | at++)] = mem[src++];
	}

	void load_treasure_pals()
	{
		static const uint16_t ids[] = {wLevelGreyTreasureID, wLevelRedTreasureID,
			wLevelGreenTreasureID, wLevelBlueTreasureID};
		static const uint16_t pals[] = {wLevelGreyTreasurePal, wLevelRedTreasurePal,
			wLevelGreenTreasurePal, wLevelBlueTreasurePal};
		Memory mem(gb_);
		for (int i = 0; i < 4; i++)
			mem[pals[i]] = mem[static_cast<uint16_t>(ADDR_TreasureOBPals + mem[ids[i]])];
	}

	// .CopyGfx: the treasure's 4 tiles from TreasureGfx to dest, returning
	// the last byte copied. The offset is id * $40 only for an id under $80:
	// rla / rla / rl b put bit 7 back in bit 0 and bit 6 in b's.
	uint8_t copy_treasure_gfx(uint8_t id, uint16_t &dest)
	{
		uint8_t rotated = static_cast<uint8_t>(id << 2 | id >> 7);
		uint16_t offset = static_cast<uint16_t>((id & 0x40) << 6 | rotated << 4);
		uint8_t last = home::copy(gb_, static_cast<uint16_t>(ADDR_TreasureGfx + offset), dest, 4 * TILE_SIZE);
		dest = static_cast<uint16_t>(dest + 4 * TILE_SIZE);
		return last;
	}

	void load_treasure_tiles()
	{
		Memory mem(gb_);
		uint16_t dest = wTileBuffer;
		copy_treasure_gfx(mem[wLevelGreyTreasureID], dest);
		copy_treasure_gfx(mem[wLevelRedTreasureID], dest);
		copy_treasure_gfx(mem[wLevelGreenTreasureID], dest);
		uint8_t last = copy_treasure_gfx(mem[wLevelBlueTreasureID], dest);
		copy_treasure_gfx(last, dest); // bug: falls through into .CopyGfx once more
	}

	void fill_bottom_bar_treasure_ids()
	{
		home::copy(gb_, wLevelTreasureIDs, wBottomBarTreasureIDs, NUM_LEVEL_TREASURES);
	}

	// with the level's musical coin: the bottom bar's flag, and the compass
	// reset
	void func_9aaf4()
	{
		Memory mem(gb_);
		cpu_.a = mem[wOWLevel]; // CheckLevelMusicalCoinFlag takes a, returns z
		CALL(CheckLevelMusicalCoinFlag);
		if (cpu_.f & FLAG_Z)
			return;
		mem[wBottomBarFlags] |= 1 << 6;
		mem[wCompassObjState + 0] = 0;
		mem[wCompassObjState + 1] = 0;
	}

	// for each treasure not yet collected in the level: TREASURE_NONE
	// (inc e: within the page)
	void write_uncollected_bottom_bar_treasure_ids()
	{
		Memory mem(gb_);
		uint8_t collected = mem[wKeyAndTreasureFlags] >> 4;
		uint8_t at = static_cast<uint8_t>(wLevelTreasureIDs);
		for (int i = 0; i < NUM_LEVEL_TREASURES; i++, at++) {
			if (!(collected >> i & 1))
				mem[static_cast<uint16_t>((wLevelTreasureIDs & 0xff00) | at)] = 0;
		}
	}

	// the level's treasures, their palettes and tiles (to v0Tiles1 tile $10),
	// in WRAM2
	void load_level_treasure_data()
	{
		Memory mem(gb_);
		uint8_t wbk = mem[rWBK]; // push_wram BANK("WRAM2")
		mem[rWBK] = 2;
		clear_treasure_data();
		load_level_treasures();
		load_treasure_pals();
		load_treasure_tiles();
		mem[rVBK] = 0; // VRAM0
		home::copy(gb_, wTileBuffer, v0Tiles1 + 0x10 * TILE_SIZE, 0x100);
		mem[rWBK] = wbk; // pop_wram
	}

	static bool is_level(uint8_t level) { return level && level != 0x80; }

	void func_9ab1c()
	{
		if (is_level(Memory(gb_)[wOWLevel]))
			FARCALL(GetOWLevelCollectedTreasures);
	}

	void get_level_time_attack_score()
	{
		Memory mem(gb_);
		uint8_t level = mem[wOWLevel];
		if (!is_level(level))
			return;
		uint16_t src = static_cast<uint16_t>(wLevelTimeAttackScores + static_cast<uint8_t>((level - 1) * 2));
		mem[w2d0ee] = mem[src];
		mem[static_cast<uint16_t>((w2d0ee & 0xff00) | static_cast<uint8_t>(w2d0ee + 1))] = mem[src + 1]; // inc e
	}

	// on a level of the map: its treasures (or its time attack score), and
	// their tiles' HDMA to VRAM
	void func_9aab5()
	{
		Memory mem(gb_);
		if (mem[wMapSideLevelID] >= OW_EXITS)
			return;
		clear_treasure_data();
		func_9aaf4();
		func_9ab1c();
		if (mem[wGameModeFlags] & (1 << MODE_TIME_ATTACK_F)) {
			get_level_time_attack_score();
			return;
		}
		load_level_treasures();
		fill_bottom_bar_treasure_ids();
		write_uncollected_bottom_bar_treasure_ids();
		load_treasure_pals();
		load_treasure_tiles();
		mem[wHDMASourceHi] = static_cast<uint8_t>(wTileBuffer >> 8);
		mem[wHDMASourceLo] = static_cast<uint8_t>(wTileBuffer);
		mem[wHDMADestHi] = 0x09;
		mem[wHDMADestLo] = 0;
		mem[wHDMAMode] = 0x10 - 1; // $10 dma_tiles
	}

	// VBlank_TreasureCollection.Func, as copied to wVBlankFunc (see ram_code.c)
	void port_VBlank_TreasureCollection__Func()
	{
		cpu_.a = 0x02;
		wr8(rWBK, cpu_.a);
		wr8(rSCY, rd8(wSCY));
		wr8(rSCX, rd8(wSCX));
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank26;

// The callers (LoadLevelTreasureData's and Func_9aab5's, C) read no
// registers after, so the hooks clobber them all; LoadLevelTreasureData
// leaves pop_wram's a and f (the WRAM bank, the entry flags).
PORT_EX(ClearTreasureData, 0, ALL_REGISTERS) { OverworldBank26(gb).clear_treasure_data(); }
PORT_EX(LoadLevelTreasures, 0, ALL_REGISTERS) { OverworldBank26(gb).load_level_treasures(); }
PORT_EX(LoadTreasurePals, 0, ALL_REGISTERS) { OverworldBank26(gb).load_treasure_pals(); }
PORT_EX(LoadTreasureTiles, 0, ALL_REGISTERS) { OverworldBank26(gb).load_treasure_tiles(); }
PORT_EX(FillBottomBarTreasureIDs, 0, ALL_REGISTERS) { OverworldBank26(gb).fill_bottom_bar_treasure_ids(); }
PORT_EX(Func_9aaf4, 0, ALL_REGISTERS) { OverworldBank26(gb).func_9aaf4(); }
PORT_EX(WriteUncollectedBottomBarTreasureIDs, 0, ALL_REGISTERS) { OverworldBank26(gb).write_uncollected_bottom_bar_treasure_ids(); }
PORT_EX(LoadLevelTreasureData, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	uint8_t wbk = wl3::Memory(gb)[rWBK];
	OverworldBank26(gb).load_level_treasure_data();
	gb->cpu.a = wbk;
}
PORT_EX(Func_9ab1c, 0, ALL_REGISTERS) { OverworldBank26(gb).func_9ab1c(); }
PORT_EX(GetLevelTimeAttackScore, 0, ALL_REGISTERS) { OverworldBank26(gb).get_level_time_attack_score(); }
PORT_EX(Func_9aab5, 0, ALL_REGISTERS) { OverworldBank26(gb).func_9aab5(); }
PORT_LOCAL_EX(VBlank_TreasureCollection, Func, HOOK_MANUAL_RET, 0) { OverworldBank26(gb).port_VBlank_TreasureCollection__Func(); }
