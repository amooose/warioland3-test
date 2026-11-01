// home/load_level (src/home/load_level.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadLevel : public AsmModule {
public:
	using AsmModule::AsmModule;

	#define ADDR_RLO_OnlyHighBit (ADDR_ReloadLevelObjects + 0x38) // .DecompressLevelBlockMap_OnlyHighBit
	#define ADDR_RLO_Decompress (ADDR_ReloadLevelObjects + 0x71)  // .Decompress

	void port_InvalidLevelData()
	{
		JUMP(Init);
	}

	// ld d, $00 / ld a, [wLevel] / add a / ld e, a / rl d / ld hl, LevelBlockAndObjectMaps /
	// add hl, de / ld a, [wROMBank] / push af / ld a, BANK(LevelBlockAndObjectMaps) /
	// bankswitch / ld a, [hli] / ld h, [hl] / ld l, a / ld a, h / inc a
	// Returns whether the pointer is valid (z clear).
	bool level_maps_pointer()
	{
		cpu_.d = 0;
		cpu_.a = rd8(wLevel);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = alu_.rl(cpu_.d);
		uint16_t hl = alu_.add16(ADDR_LevelBlockAndObjectMaps, REG_DE(gb_));
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_LevelBlockAndObjectMaps;
		bankswitch(cpu_.a);
		cpu_.a = rd8(hl++);
		cpu_.h = rd8(hl);
		cpu_.l = cpu_.a;
		cpu_.a = alu_.inc(cpu_.h);
		return !(cpu_.f & FLAG_Z);
	}

	// ld a, [hli] / ld [wCompressedLevelBlockMapPtr + 1], a / ld a, [hli] /
	// ld [wCompressedLevelBlockMapPtr + 0], a / ld a, [hl] / ld [wTempBank], a
	void read_map_header()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(wCompressedLevelBlockMapPtr + 1, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(wCompressedLevelBlockMapPtr + 0, cpu_.a);
		cpu_.a = rd8(hl);
		wr8(wTempBank, cpu_.a);
		SET_HL(gb_, hl);
	}

	// loads data related to the level block and object map
	// and stores them in SRAM
	void port_LoadLevelBlockMapAndObjects()
	{
		if (!level_maps_pointer()) {
			JUMP(InvalidLevelData); // null
			return;
		}
		read_map_header();
		gb_push16(gb_, REG_HL(gb_));
		CALL(DecompressLevelBlockMap);
		SET_HL(gb_, gb_pop16(gb_));

		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(wTempBank, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(wCompressedLevelBlockMapPtr + 1, cpu_.a);
		cpu_.a = rd8(hl);
		wr8(wCompressedLevelBlockMapPtr + 0, cpu_.a);
		SET_HL(gb_, hl);
		pop_af();
		bankswitch(cpu_.a);
		gb_push16(gb_, REG_HL(gb_));
		cpu_.a = alu_.and_(rd8(wceef), 0x3c);
		if (cpu_.f & FLAG_Z)
			CALL(DecompressLevelObjectMap);
		SET_HL(gb_, gb_pop16(gb_));
		RET();
	}

	void port_ReloadLevelObjects()
	{
		if (!level_maps_pointer()) {
			JUMP(InvalidLevelData); // null
			return;
		}
		read_map_header();
		pop_af();
		bankswitch(cpu_.a);
		gb_call(gb_, ADDR_RLO_OnlyHighBit);
		RET();
	}

	// almost identical to DecompressLevelBlockMap
	// but instead only copies over the highest bit
	void port_ReloadLevelObjects__DecompressLevelBlockMap_OnlyHighBit()
	{
		cpu_.a = alu_.and_(rd8(wceef), 0x3c);
		if (!(cpu_.f & FLAG_Z))
			return;
		cpu_.a = rd8(wSRAMBank);
		push_af();
		cpu_.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
		sramswitch(cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		cpu_.a = rd8(wCompressedLevelBlockMapPtr + 0);
		cpu_.d = cpu_.a;
		cpu_.a = rd8(wCompressedLevelBlockMapPtr + 1);
		cpu_.e = cpu_.a;
		gb_call(gb_, ADDR_RLO_Decompress);
		pop_af();
		bankswitch(cpu_.a);
		pop_af();
		sramswitch(cpu_.a);
	}

	// ld l, LOW(STARTOF(SRAM)) / inc h / ld a, h / cp $c0 / jr nz, ... /
	// ld h, HIGH(STARTOF(SRAM)) / ld a, [wSRAMBank] / inc a / sramswitch
	void next_row()
	{
		cpu_.l = 0x00;
		cpu_.h = alu_.inc(cpu_.h);
		cpu_.a = cpu_.h;
		alu_.cp(cpu_.a, 0xc0); // check if already outside SRAM
		if (!(cpu_.f & FLAG_Z))
			return;
		cpu_.h = 0xa0;
		cpu_.a = alu_.inc(rd8(wSRAMBank));
		sramswitch(cpu_.a);
	}

	// ld a, [wRepeatByte] or [de] & $80 / or [hl] / ld [hli], a / ld a, l / cp c
	bool or_into_map(uint8_t v)
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.a = alu_.or_(v, rd8(hl));
		wr8(hl++, cpu_.a);
		SET_HL(gb_, hl);
		cpu_.a = cpu_.l;
		alu_.cp(cpu_.a, cpu_.c);
		return cpu_.f & FLAG_Z;
	}

	void port_ReloadLevelObjects__Decompress()
	{
		cpu_.c = LEVEL_WIDTH;
		SET_HL(gb_, sLevelBlockObjectMap);
		for (;;) {
			// .loop_data
			cpu_.a = rd8(REG_DE(gb_));
			cpu_.a = alu_.and_(cpu_.a, cpu_.a);
			if (cpu_.f & FLAG_Z)
				return; // done
			if (!alu_.bit(cpu_.a, 7)) {
				cpu_.b = cpu_.a; // num rows
				SET_DE(gb_, REG_DE(gb_) + 1);
				cpu_.a = alu_.and_(rd8(REG_DE(gb_)), 0x80);
				wr8(wRepeatByte, cpu_.a);
				SET_DE(gb_, REG_DE(gb_) + 1);
				do {
					// .asm_f61
					if (or_into_map(rd8(wRepeatByte)))
						next_row(); // .next_row_1
					// .next_repeat
					cpu_.b = alu_.dec(cpu_.b);
				} while (!(cpu_.f & FLAG_Z));
			} else {
				// .asm_f85
				cpu_.a = alu_.and_(cpu_.a, 0x7f);
				cpu_.b = cpu_.a;
				SET_DE(gb_, REG_DE(gb_) + 1);
				do {
					// .loop_copy
					cpu_.a = alu_.and_(rd8(REG_DE(gb_)), 0x80);
					uint16_t hl = REG_HL(gb_);
					cpu_.a = alu_.or_(cpu_.a, rd8(hl));
					wr8(hl++, cpu_.a);
					SET_HL(gb_, hl);
					SET_DE(gb_, REG_DE(gb_) + 1);
					cpu_.a = cpu_.l;
					alu_.cp(cpu_.a, cpu_.c);
					if (cpu_.f & FLAG_Z)
						next_row(); // .asm_f98
					// .asm_f93
					cpu_.b = alu_.dec(cpu_.b);
				} while (!(cpu_.f & FLAG_Z));
			}
		}
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadLevel;

PORT_EX(InvalidLevelData, HOOK_MANUAL_RET, 0) { HomeLoadLevel(gb).port_InvalidLevelData(); }
PORT_EX(LoadLevelBlockMapAndObjects, HOOK_MANUAL_RET, 0) { HomeLoadLevel(gb).port_LoadLevelBlockMapAndObjects(); }
PORT_EX(ReloadLevelObjects, HOOK_MANUAL_RET, 0) { HomeLoadLevel(gb).port_ReloadLevelObjects(); }
PORT_LOCAL_EX(ReloadLevelObjects, DecompressLevelBlockMap_OnlyHighBit, 0, 0) { HomeLoadLevel(gb).port_ReloadLevelObjects__DecompressLevelBlockMap_OnlyHighBit(); }
PORT_LOCAL_EX(ReloadLevelObjects, Decompress, 0, 0) { HomeLoadLevel(gb).port_ReloadLevelObjects__Decompress(); }
