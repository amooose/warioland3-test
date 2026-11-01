// overworld/bank26 (src/engine/overworld/bank26.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank26 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// and the levels call (the collection screen is C++, src/native/overworld)

	// a = digit (low nibble): copies its 2 tiles from NumbersGfx to de
	void draw_number()
	{
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		cpu_.a = alu_.inc(cpu_.a);
		uint16_t hl = ADDR_NumbersGfx;
		SET_BC(gb_, 0x20);
		for (;;) {
			cpu_.a = alu_.dec(cpu_.a);
			if (!cpu_.a)
				break;
			hl = alu_.add16(hl, 0x20);
		}
		SET_HL(gb_, hl);
		cpu_.b = 0x20;
		CALL(CopyHLToDE);
	}

	void port_DrawCoinCount()
	{
		SET_DE(gb_, v0Tiles1 + 0xa * TILE_SIZE);
		cpu_.a = rd8(wNumCoins + 0);
		draw_number();
		cpu_.a = alu_.swap(rd8(wNumCoins + 1));
		draw_number();
		cpu_.a = rd8(wNumCoins + 1);
		draw_number();
	}

	void port_ClearTreasureData()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, wTileBuffer);
		SET_BC(gb_, 16 * TILE_SIZE);
		CALL(WriteAToHL_BCTimes);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, wLevelTreasureIDs);
		SET_BC(gb_, 0x10);
		CALL(WriteAToHL_BCTimes);
	}

	void port_LoadLevelTreasures()
	{
		SET_DE(gb_, wLevelTreasureIDs);
		cpu_.a = rd8(wOWLevel);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.c = cpu_.a;
		cpu_.b = 0;
		uint16_t hl = alu_.add16(ADDR_LevelTreasureIDs, cpu_.c);
		cpu_.b = NUM_LEVEL_TREASURES;
		do {
			cpu_.a = rd8(hl++);
			wr8(REG_DE(gb_), cpu_.a);
			cpu_.e++; // inc e
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}

	void port_LoadTreasurePals()
	{
		static const uint16_t ids[] = {wLevelGreyTreasureID, wLevelRedTreasureID,
			wLevelGreenTreasureID, wLevelBlueTreasureID};
		static const uint16_t pals[] = {wLevelGreyTreasurePal, wLevelRedTreasurePal,
			wLevelGreenTreasurePal, wLevelBlueTreasurePal};
		SET_DE(gb_, ADDR_TreasureOBPals);
		for (int i = 0; i < 4; i++) {
			cpu_.a = rd8(ids[i]);
			uint16_t hl = alu_.add16(cpu_.a, ADDR_TreasureOBPals);
			SET_HL(gb_, hl);
			cpu_.a = rd8(hl);
			wr8(pals[i], cpu_.a);
		}
	}

	// a = treasure ID: copies its 4 tiles from TreasureGfx to de
	void copy_treasure_gfx()
	{
		alu_.and_(cpu_.a, cpu_.a);
		cpu_.b = 0;
		cpu_.a = alu_.rla(cpu_.a);
		cpu_.a = alu_.rla(cpu_.a);
		cpu_.b = alu_.rl(cpu_.b);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.and_(cpu_.a, 0xf0);
		cpu_.a = alu_.or_(cpu_.a, cpu_.b);
		cpu_.a = alu_.swap(cpu_.a);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.and_(0x0f, cpu_.c);
		cpu_.a = alu_.swap(cpu_.a);
		cpu_.c = cpu_.a;
		SET_HL(gb_, alu_.add16(ADDR_TreasureGfx, REG_BC(gb_)));
		cpu_.b = 4 * TILE_SIZE;
		CALL(CopyHLToDE);
	}

	void port_LoadTreasureTiles()
	{
		cpu_.a = rd8(wLevelGreyTreasureID);
		SET_DE(gb_, wTileBuffer);
		copy_treasure_gfx();
		cpu_.a = rd8(wLevelRedTreasureID);
		copy_treasure_gfx();
		cpu_.a = rd8(wLevelGreenTreasureID);
		copy_treasure_gfx();
		cpu_.a = rd8(wLevelBlueTreasureID);
		copy_treasure_gfx();
		copy_treasure_gfx(); // falls through into .CopyGfx once more
	}

	void port_FillBottomBarTreasureIDs()
	{
		SET_DE(gb_, wBottomBarTreasureIDs);
		SET_HL(gb_, wLevelTreasureIDs);
		cpu_.b = NUM_LEVEL_TREASURES;
		CALL(CopyHLToDE);
	}

	// c = treasure ID
	// returns b = level
	void port_GetLevelThatContainsTreasure()
	{
		cpu_.b = 0xff;
		uint16_t hl = ADDR_LevelTreasureIDs;
		do {
			cpu_.b = alu_.inc(cpu_.b);
			cpu_.a = rd8(hl++);
			alu_.cp(cpu_.a, cpu_.c);
		} while (!(cpu_.f & FLAG_Z));
		SET_HL(gb_, hl);
		cpu_.b = alu_.srl(cpu_.b);
		cpu_.b = alu_.srl(cpu_.b);
	}

	void port_Func_9aaf4()
	{
		cpu_.a = rd8(wOWLevel);
		CALL(CheckLevelMusicalCoinFlag);
		if (cpu_.f & FLAG_Z)
			return;
		SET_HL(gb_, wBottomBarFlags);
		wr8(wBottomBarFlags, rd8(wBottomBarFlags) | (1 << 6));
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wCompassObjState, 0);
		wr8(wCompassObjState + 1, 0);
		SET_HL(gb_, wCompassObjState + 1);
	}

	void port_WriteUncollectedBottomBarTreasureIDs()
	{
		cpu_.a = alu_.swap(rd8(wKeyAndTreasureFlags));
		cpu_.a = alu_.and_(cpu_.a, 0x0f);
		cpu_.b = cpu_.a;
		cpu_.c = NUM_LEVEL_TREASURES;
		SET_DE(gb_, wLevelTreasureIDs);
		do {
			cpu_.b = alu_.srl(cpu_.b);
			if (!(cpu_.f & FLAG_C)) {
				cpu_.a = 0;
				cpu_.f = FLAG_Z; // xor a
				wr8(REG_DE(gb_), 0);
			}
			cpu_.e = alu_.inc(cpu_.e);
			cpu_.c = alu_.dec(cpu_.c);
		} while (cpu_.c);
	}





	void port_LoadLevelTreasureData()
	{
		// push_wram BANK("WRAM2")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 2;
		wr8(rWBK, cpu_.a);
		gb_call(gb_, ADDR_ClearTreasureData);
		gb_call(gb_, ADDR_LoadLevelTreasures);
		gb_call(gb_, ADDR_LoadTreasurePals);
		gb_call(gb_, ADDR_LoadTreasureTiles);
		cpu_.a = alu_.xor_(cpu_.a, cpu_.a); // VRAM0
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, wTileBuffer);
		SET_DE(gb_, v0Tiles1 + 0x10 * TILE_SIZE);
		cpu_.b = 0x00;
		CALL(CopyHLToDE);
		pop_af(); // pop_wram
		wr8(rWBK, cpu_.a);
	}

	void port_Func_9ab1c()
	{
		cpu_.a = rd8(wOWLevel);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.f & FLAG_Z)
			return;
		alu_.cp(cpu_.a, 0x80);
		if (cpu_.f & FLAG_Z)
			return;
		FARCALL(GetOWLevelCollectedTreasures);
	}

	void port_GetLevelTimeAttackScore()
	{
		cpu_.a = rd8(wOWLevel);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.f & FLAG_Z)
			return;
		alu_.cp(cpu_.a, 0x80);
		if (cpu_.f & FLAG_Z)
			return;
		cpu_.a = alu_.dec(cpu_.a);
		SET_DE(gb_, w2d0ee);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.c = cpu_.a;
		cpu_.b = 0x00;
		uint16_t hl = alu_.add16(wLevelTimeAttackScores, REG_BC(gb_));
		cpu_.a = rd8(hl++);
		wr8(REG_DE(gb_), cpu_.a);
		cpu_.e = alu_.inc(cpu_.e);
		cpu_.a = rd8(hl);
		wr8(REG_DE(gb_), cpu_.a);
		SET_HL(gb_, hl);
	}


	void port_Func_9aab5()
	{
		cpu_.a = rd8(wMapSideLevelID);
		alu_.cp(cpu_.a, OW_EXITS);
		if (!(cpu_.f & FLAG_C)) {
			RET();
			return;
		}
		gb_call(gb_, ADDR_ClearTreasureData);
		gb_call(gb_, ADDR_Func_9aaf4);
		gb_call(gb_, ADDR_Func_9ab1c);
		cpu_.a = rd8(wGameModeFlags);
		if (alu_.bit(cpu_.a, MODE_TIME_ATTACK_F)) {
			JUMP_TO(ADDR_GetLevelTimeAttackScore);
			return;
		}
		gb_call(gb_, ADDR_LoadLevelTreasures);
		gb_call(gb_, ADDR_FillBottomBarTreasureIDs);
		gb_call(gb_, ADDR_WriteUncollectedBottomBarTreasureIDs);
		gb_call(gb_, ADDR_LoadTreasurePals);
		gb_call(gb_, ADDR_LoadTreasureTiles);
		cpu_.a = (uint8_t)(wTileBuffer >> 8);
		wr8(wHDMASourceHi, cpu_.a);
		cpu_.a = (uint8_t)wTileBuffer;
		wr8(wHDMASourceLo, cpu_.a);
		cpu_.a = 0x09;
		wr8(wHDMADestHi, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wHDMADestLo, 0);
		cpu_.a = 0x10 - 1; // $10 dma_tiles
		wr8(wHDMAMode, cpu_.a);
		RET();
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

PORT_EX(DrawCoinCount, 0, 0) { OverworldBank26(gb).port_DrawCoinCount(); }
PORT_EX(ClearTreasureData, 0, 0) { OverworldBank26(gb).port_ClearTreasureData(); }
PORT_EX(LoadLevelTreasures, 0, 0) { OverworldBank26(gb).port_LoadLevelTreasures(); }
PORT_EX(LoadTreasurePals, 0, 0) { OverworldBank26(gb).port_LoadTreasurePals(); }
PORT_EX(LoadTreasureTiles, 0, 0) { OverworldBank26(gb).port_LoadTreasureTiles(); }
PORT_EX(FillBottomBarTreasureIDs, 0, 0) { OverworldBank26(gb).port_FillBottomBarTreasureIDs(); }
PORT_EX(GetLevelThatContainsTreasure, 0, 0) { OverworldBank26(gb).port_GetLevelThatContainsTreasure(); }
PORT_EX(Func_9aaf4, 0, 0) { OverworldBank26(gb).port_Func_9aaf4(); }
PORT_EX(WriteUncollectedBottomBarTreasureIDs, 0, 0) { OverworldBank26(gb).port_WriteUncollectedBottomBarTreasureIDs(); }
PORT_EX(LoadLevelTreasureData, 0, 0) { OverworldBank26(gb).port_LoadLevelTreasureData(); }
PORT_EX(Func_9ab1c, 0, 0) { OverworldBank26(gb).port_Func_9ab1c(); }
PORT_EX(GetLevelTimeAttackScore, 0, 0) { OverworldBank26(gb).port_GetLevelTimeAttackScore(); }
PORT_EX(Func_9aab5, HOOK_MANUAL_RET, 0) { OverworldBank26(gb).port_Func_9aab5(); }
PORT_LOCAL_EX(VBlank_TreasureCollection, Func, HOOK_MANUAL_RET, 0) { OverworldBank26(gb).port_VBlank_TreasureCollection__Func(); }
