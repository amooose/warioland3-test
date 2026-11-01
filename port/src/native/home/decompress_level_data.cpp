// home/decompress_level_data (src/home/decompress_level_data.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeDecompressLevelData : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Turns a 2-byte coordinate 0xXWYZ into the block index 0xWY.
	uint8_t block_coord(uint8_t hi, uint8_t lo)
	{
		uint8_t low_nybble = alu_.and_((uint8_t)(lo >> 4 | lo << 4), 0x0f);
		uint8_t high = (uint8_t)(alu_.and_(hi, 0x0f) << 4);
		return alu_.add(high, low_nybble);
	}

	// hl = 2-byte y coordinate followed by 2-byte x coordinate
	// output: hl = wBlockPtr = pointer into the level's block map in SRAM,
	// wBlockPtrBank = its SRAM bank
	void port_GetBlockPtr()
	{
		uint16_t hl = REG_HL(gb_);

		// y position
		cpu_.d = rd8(hl++);
		uint8_t lo = rd8(hl++);
		cpu_.b = alu_.and_((uint8_t)(lo >> 4 | lo << 4), 0x0f);
		uint8_t row = alu_.add((uint8_t)(alu_.and_(cpu_.d, 0x0f) << 4), cpu_.b);

		// The block map spans several SRAM banks of $20 pages each.
		cpu_.b = BANK_sLevelBlockObjectMap;
		uint8_t page = alu_.add(row, (uint8_t)(sLevelBlockObjectMap >> 8));
		alu_.cp(page, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		if (!(cpu_.f & FLAG_C)) {
			cpu_.b = alu_.inc(cpu_.b);
			page = alu_.sub(page, 0x20);
			alu_.cp(page, 0xc0);
			if (!(cpu_.f & FLAG_C)) {
				cpu_.b = alu_.inc(cpu_.b);
				page = alu_.sub(page, 0x20);
			}
		}
		wr8(wBlockPtr + 0, page);
		wr8(wBlockPtrBank, cpu_.b);

		// x position
		cpu_.d = rd8(hl++);
		lo = rd8(hl);
		cpu_.a = block_coord(cpu_.d, lo);
		wr8(wBlockPtr + 1, cpu_.a);
		cpu_.l = cpu_.a;
		cpu_.a = rd8(wBlockPtr + 0);
		cpu_.h = cpu_.a;
	}

	// e reached the end of a row: go to the start of the next one, and to the
	// next SRAM bank after its end
	void next_row(uint8_t row_start)
	{
		cpu_.e = row_start;
		cpu_.d = alu_.inc(cpu_.d);
		cpu_.a = cpu_.d;
		alu_.cp(cpu_.a, 0xc0);
		if (cpu_.f & FLAG_Z) {
			cpu_.d = 0xa0; // HIGH(STARTOF(SRAM))
			cpu_.a = alu_.inc(rd8(wSRAMBank));
			sramswitch(cpu_.a);
		}
	}

	// writes a to [de] and advances de, wrapping rows at c
	void put_byte(bool objmap)
	{
		wr8(REG_DE(gb_), cpu_.a);
		if (!objmap)
			SET_DE(gb_, REG_DE(gb_) + 1);
		cpu_.a = cpu_.e;
		alu_.cp(cpu_.a, cpu_.c);
		if (cpu_.f & FLAG_Z)
			next_row(objmap ? 0xb0 : 0x00);
		else if (objmap)
			SET_DE(gb_, REG_DE(gb_) + 1);
	}

	// .Decompress: RLE data at hl into the level map in SRAM
	void decompress_map(bool objmap)
	{
		uint16_t hl = REG_HL(gb_);
		for (;;) {
			cpu_.a = rd8(hl++);
			alu_.and_(cpu_.a, cpu_.a);
			if (!cpu_.a)
				break;
			if (alu_.bit(cpu_.a, 7)) {
				cpu_.a = alu_.and_(cpu_.a, 0x7f);
				cpu_.b = cpu_.a;
				do {
					cpu_.a = rd8(hl++);
					SET_HL(gb_, hl);
					put_byte(objmap);
					hl = REG_HL(gb_);
					cpu_.b = alu_.dec(cpu_.b);
				} while (cpu_.b);
			} else {
				cpu_.b = cpu_.a;
				cpu_.a = rd8(hl++);
				wr8(wRepeatByte, cpu_.a);
				do {
					cpu_.a = rd8(wRepeatByte);
					SET_HL(gb_, hl);
					put_byte(objmap);
					hl = REG_HL(gb_);
					cpu_.b = alu_.dec(cpu_.b);
				} while (cpu_.b);
			}
		}
		SET_HL(gb_, hl);
	}

	void decompress_level_map(bool objmap)
	{
		cpu_.a = rd8(wSRAMBank);
		push_af();
		cpu_.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
		sramswitch(cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		cpu_.h = rd8(wCompressedLevelBlockMapPtr + 0);
		cpu_.a = rd8(wCompressedLevelBlockMapPtr + 1);
		cpu_.l = cpu_.a;
		if (objmap) {
			cpu_.c = (LEVEL_WIDTH >> 1) + 0xb0 - 1;
			SET_DE(gb_, sLevelBlockObjectMap + 0xb0);
		} else {
			cpu_.c = LEVEL_WIDTH;
			SET_DE(gb_, sLevelBlockObjectMap);
		}
		decompress_map(objmap);
		pop_af();
		bankswitch(cpu_.a);
		pop_af();
		sramswitch(cpu_.a);
	}

	void port_DecompressLevelBlockMap()
	{
		cpu_.a = alu_.and_(rd8(wceef), 0x3c);
		if (cpu_.a)
			return;
		decompress_level_map(false);
	}

	void port_DecompressLevelObjectMap()
	{
		decompress_level_map(true);
	}

	// hl = block pointer in SRAM
	// output: wBGPtr = its address in v0BGMap0
	void port_GetBlockPtrBGMapAddress()
	{
		cpu_.a = alu_.sub(cpu_.h, 0xa0); // HIGH(STARTOF(SRAM))
		cpu_.e = cpu_.a;
		cpu_.d = 0;
		for (int i = 0; i < 6; i++) {
			cpu_.e = alu_.sla(cpu_.e);
			cpu_.d = alu_.rl(cpu_.d);
		}
		cpu_.a = alu_.and_(cpu_.d, 0x03);
		cpu_.d = cpu_.a;
		cpu_.a = alu_.and_(cpu_.l, 0x0f);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.l = cpu_.a;
		cpu_.h = (uint8_t)(v0BGMap0 >> 8);
		uint16_t hl = alu_.add16(REG_HL(gb_), REG_DE(gb_));
		SET_HL(gb_, hl);
		cpu_.a = cpu_.h;
		wr8(wBGPtr + 0, cpu_.a);
		cpu_.a = cpu_.l;
		wr8(wBGPtr + 1, cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeDecompressLevelData;

PORT_EX(GetBlockPtr, 0, 0) { HomeDecompressLevelData(gb).port_GetBlockPtr(); }
PORT_EX(DecompressLevelBlockMap, 0, 0) { HomeDecompressLevelData(gb).port_DecompressLevelBlockMap(); }
PORT_EX(DecompressLevelObjectMap, 0, 0) { HomeDecompressLevelData(gb).port_DecompressLevelObjectMap(); }
PORT_EX(GetBlockPtrBGMapAddress, 0, 0) { HomeDecompressLevelData(gb).port_GetBlockPtrBGMapAddress(); }
