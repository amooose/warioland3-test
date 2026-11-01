// home/load_room (src/home/load_room.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadRoom : public AsmModule {
public:
	using AsmModule::AsmModule;

	// ld a, [index] / add a / ld e, a / ld d, $00 / ld hl, table / add hl, de /
	// ld a, [hli] / ld h, [hl] / ld l, a
	void get_pointer(uint16_t index, uint16_t table)
	{
		cpu_.a = rd8(index);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(table, REG_DE(gb_));
		cpu_.a = rd8(hl++);
		cpu_.h = rd8(hl);
		cpu_.l = cpu_.a;
	}

	// copies bc bytes from hl to de with the bank in bank_addr mapped
	void far_copy(uint16_t bank_addr, uint16_t de, uint16_t bc)
	{
		cpu_.a = rd8(bank_addr);
		wr8(wTempBank, cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		SET_DE(gb_, de);
		SET_BC(gb_, bc);
		CALL(CopyHLToDE_BC);
		pop_af();
		bankswitch(cpu_.a);
	}

	void port_Func_298d()
	{
		get_pointer(wRoomBlockFunctionTable, ADDR_RoomBlockTilesPointers);
		far_copy(wRoomBlockFunctionTableBank, wRoomBlockTiles, 4 * NUM_ROOM_BLOCK_TYPES);
	}

	void port_LoadRoomTileMap()
	{
		get_pointer(wRoomTileMap, ADDR_RoomBlockAttributePointers);
		cpu_.a = rd8(wc0d0);
		wr8(wTempBank, cpu_.a);
		SET_BC(gb_, wRoomBlockAttributes);
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);
	}

	void port_LoadRoomMainTiles()
	{
		get_pointer(wRoomMainTiles, ADDR_LevelMainTilesPointers);
		far_copy(wRoomMainTilesBank, v0Tiles2, 0x800);
	}

	void port_LoadRoomSpecialTiles()
	{
		cpu_.a = 1;
		wr8(rVBK, 1);
		get_pointer(wRoomSpecialTiles, ADDR_SpecialTilesPointers);
		far_copy(wRoomSpecialTilesBank, v0Tiles2, 0x800);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	void port_LoadRoomPalettes()
	{
		get_pointer(wRoomPalettes, ADDR_RoomPals);
		cpu_.a = rd8(wPaletteBank);
		wr8(wTempBank, cpu_.a);
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(LoadPalsToTempPals1);
	}

	void wait_next_hblank()
	{
		while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
		while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
	}

	void port_UpdateRoomAnimatedPals()
	{
		// only animate if at least 8 scanlines from V-Blank
		cpu_.a = rd8(rLY);
		alu_.cp(cpu_.a, LY_VBLANK - 8);
		if (!(cpu_.f & FLAG_C))
			return;
		cpu_.a = rd8(wRoomPalCycleDuration);
		alu_.and_(cpu_.a, cpu_.a);
		if (!cpu_.a)
			return;
		cpu_.b = cpu_.a;
		cpu_.a = alu_.inc(rd8(wRoomPalCycleCounter));
		wr8(wRoomPalCycleCounter, cpu_.a);
		alu_.cp(cpu_.a, cpu_.b);
		if (cpu_.f & FLAG_C)
			return;

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wRoomPalCycleCounter, 0);
		cpu_.h = rd8(wRoomPalCyclePtr + 0);
		cpu_.l = rd8(wRoomPalCyclePtr + 1);

		cpu_.a = alu_.inc(rd8(wRoomPalCycleIndex));
		alu_.cp(cpu_.a, ROOM_PALCYCLE_LENGTH);
		if (!(cpu_.f & FLAG_C)) {
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
		}
		wr8(wRoomPalCycleIndex, cpu_.a);

		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(REG_HL(gb_), REG_DE(gb_));
		SET_HL(gb_, hl);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_RoomPalCycles;
		bankswitch(cpu_.a);
		cpu_.d = 0;
		cpu_.a = rd8(hl);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *2
		cpu_.e = cpu_.a;
		cpu_.d = alu_.rl(cpu_.d);
		hl = alu_.add16(ADDR_RoomPals, REG_DE(gb_));
		cpu_.a = rd8(hl++);
		cpu_.h = rd8(hl);
		cpu_.l = cpu_.a;
		pop_af();
		bankswitch(cpu_.a);

		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = 0x33; // BANK("Level Palettes")
		bankswitch(cpu_.a);
		cpu_.a = BGPI_AUTOINC | 0;
		wr8(rBGPI, cpu_.a);
		cpu_.b = 5;
		cpu_.c = (uint8_t)rBGPD;
		hl = REG_HL(gb_);
		do {
			wait_next_hblank();
			for (int i = 0; i < 12; i++) {
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);

		wait_next_hblank();
		for (int i = 0; i < 3; i++) {
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		}
		cpu_.a = rd8(hl);
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		SET_HL(gb_, hl);
		pop_af();
		bankswitch(cpu_.a);
	}

	#define BANK_BLOCK_FUNCTION_TABLES_2 0x50 // BANK("Block Function Tables 2")

	void port_LoadBlockFunctionTable()
	{
		cpu_.a = rd8(wRoomBlockFunctionTable);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		uint16_t hl = alu_.add16(ADDR_BlockFunctionTablePointers, REG_DE(gb_));
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_BlockFunctionTablePointers;
		bankswitch(cpu_.a);
		cpu_.a = rd8(hl++);
		cpu_.h = rd8(hl);
		cpu_.l = cpu_.a;
		cpu_.a = cpu_.h;
		alu_.cp(cpu_.a, 0xff);
		if (cpu_.f & FLAG_Z) {
			JUMP(Init); // .reset
			return;
		}
		cpu_.a = rd8(wRoomBlockFunctionTable);
		alu_.cp(cpu_.a, NUM_BLOCK_FUNCTION_TABLES_GROUP_1 - 1);
		bool group_2 = !(cpu_.f & FLAG_C);
		if (group_2) {
			cpu_.a = rd8(wROMBank);
			push_af();
			cpu_.a = BANK_BLOCK_FUNCTION_TABLES_2;
			bankswitch(cpu_.a);
		}
		SET_DE(gb_, wBlockFunctionTable);
		cpu_.b = 0;
		CALL(CopyHLToDE);
		pop_af();
		bankswitch(cpu_.a);
		if (group_2) {
			pop_af();
			bankswitch(cpu_.a);
		}
		RET();
	}

	// ld a, [hl] / swap a / and $0f (or without the swap)
	uint8_t nibble(uint16_t hl, bool high)
	{
		cpu_.a = rd8(hl);
		if (high)
			cpu_.a = alu_.swap(cpu_.a);
		return cpu_.a = alu_.and_(cpu_.a, 0x0f);
	}

	// ld a, [limit] / add a (shift times) / add base, then $ff if that made 0
	void room_limit(uint16_t limit, int shift, uint8_t base, bool nonzero, uint16_t dest)
	{
		cpu_.a = rd8(limit);
		for (int i = 0; i < shift; i++)
			cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, base);
		if (nonzero && (cpu_.f & FLAG_Z))
			cpu_.a = 0xff;
		wr8(dest, cpu_.a);
	}

	void port_LoadRoom()
	{
		cpu_.a = 0x70;
		wr8(wPrevWarioScreenYPos, cpu_.a);
		cpu_.a = 0x58;
		wr8(wCamRightSpacing, cpu_.a);
		cpu_.a = alu_.sub(cpu_.a, 0x58);
		cpu_.a = alu_.cpl(cpu_.a);
		cpu_.a = alu_.inc(cpu_.a);
		cpu_.a = alu_.add(cpu_.a, 0x58);
		wr8(wCamLeftSpacing, cpu_.a);

		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_LevelHeaders;
		bankswitch(cpu_.a);
		cpu_.a = rd8(wLevel);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		cpu_.d = alu_.rl(cpu_.d);
		uint16_t hl = alu_.add16(ADDR_LevelHeaders, REG_DE(gb_));
		cpu_.a = rd8(hl++);
		cpu_.h = rd8(hl);
		cpu_.l = cpu_.a;
		cpu_.a = cpu_.h;
		alu_.cp(cpu_.a, 0xff);
		if (cpu_.f & FLAG_Z) {
			JUMP(Init); // debug_assert_not $ff
			return;
		}
		pop_af();
		bankswitch(cpu_.a);

		cpu_.a = rd8(wLevel);
		alu_.cp(cpu_.a, THE_GRASSLANDS_NIGHT_1);
		uint8_t bank = (cpu_.f & FLAG_C) ? 0x30 : 0x31;
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = bank;
		bankswitch(cpu_.a);

		// .got_bank
		cpu_.a = rd8(wLevelRoomID);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		hl = alu_.add16(REG_HL(gb_), REG_DE(gb_));
		cpu_.a = rd8(hl++);
		cpu_.e = cpu_.a;
		cpu_.h = rd8(hl);
		cpu_.l = cpu_.e;
		cpu_.a = cpu_.h;
		alu_.cp(cpu_.a, 0xff);
		if (cpu_.f & FLAG_Z) {
			JUMP(Init); // debug_assert_not $ff
			return;
		}
		hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(wWarioSpawnPos, cpu_.a);
		wr8(wRoomUpperYLimit, nibble(hl, true));
		wr8(wRoomLowerYLimit, nibble(hl++, false));
		wr8(wRoomLeftXLimit, nibble(hl, true));
		wr8(wRoomRightXLimit, nibble(hl++, false));
		cpu_.a = rd8(hl++);
		wr8(wCameraConfigFlags, cpu_.a);

		cpu_.a = rd8(hl++);
		wr8(wObjectGroup, cpu_.a);
		SET_HL(gb_, hl);
		gb_push16(gb_, REG_HL(gb_));
		cpu_.a = rd8(rWBK); // push_wram
		push_af();
		cpu_.a = 0x01; // BANK("Level Objects WRAM")
		wr8(rWBK, cpu_.a);
		FARCALL(LoadObjectGroupData);
		pop_af(); // pop_wram
		wr8(rWBK, cpu_.a);
		SET_HL(gb_, gb_pop16(gb_));

		hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(wAnimatedTilesGroup, cpu_.a);
		SET_HL(gb_, hl);
		gb_push16(gb_, REG_HL(gb_));
		FARCALL(InitRoomAnimatedTiles);
		SET_HL(gb_, gb_pop16(gb_));

		hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(wRoomPalCycle, cpu_.a);
		SET_HL(gb_, hl);
		gb_push16(gb_, REG_HL(gb_));
		FARCALL(InitRoomAnimatedPals);
		SET_HL(gb_, gb_pop16(gb_));

		cpu_.a = rd8(REG_HL(gb_));
		wr8(wRoom, cpu_.a);
		FARCALL(LoadRoomGfx);
		pop_af();
		bankswitch(cpu_.a);

		room_limit(wRoomLowerYLimit, 4, 0xa0, true, wc0c4);
		room_limit(wRoomUpperYLimit, 4, 0xa0, false, wc0c5);
		room_limit(wRoomRightXLimit, 3, 0xb0, true, wc0c7);
		room_limit(wRoomLeftXLimit, 3, 0xb0, false, wc0c6);
		RET();
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadRoom;

PORT_EX(Func_298d, 0, 0) { HomeLoadRoom(gb).port_Func_298d(); }
PORT_EX(LoadRoomTileMap, 0, 0) { HomeLoadRoom(gb).port_LoadRoomTileMap(); }
PORT_EX(LoadRoomMainTiles, 0, 0) { HomeLoadRoom(gb).port_LoadRoomMainTiles(); }
PORT_EX(LoadRoomSpecialTiles, 0, 0) { HomeLoadRoom(gb).port_LoadRoomSpecialTiles(); }
PORT_EX(LoadRoomPalettes, 0, 0) { HomeLoadRoom(gb).port_LoadRoomPalettes(); }
PORT_EX(UpdateRoomAnimatedPals, 0, 0) { HomeLoadRoom(gb).port_UpdateRoomAnimatedPals(); }
PORT_EX(LoadBlockFunctionTable, HOOK_MANUAL_RET, 0) { HomeLoadRoom(gb).port_LoadBlockFunctionTable(); }
PORT_EX(LoadRoom, HOOK_MANUAL_RET, 0) { HomeLoadRoom(gb).port_LoadRoom(); }
