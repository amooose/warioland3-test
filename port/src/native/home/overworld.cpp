// home/overworld (src/home/overworld.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeOverworld : public AsmModule {
public:
	using AsmModule::AsmModule;

	// returns TRUE in a and carry set if treasure
	// in input a has already been collected
	// if not collected returns z set
	// a = treasure constant
	void port_IsTreasureCollected()
	{
		uint8_t treasure = cpu_.a;
		uint8_t e = alu_.srl(alu_.srl(alu_.srl(treasure))); // / 8
		cpu_.d = 0;
		uint16_t hl = alu_.add16(wTreasuresCollected, e);

		// the treasure's bit within its byte
		e = 1;
		if (alu_.bit(treasure, 2))
			e = alu_.swap(e); // = $10
		uint8_t d = alu_.and_(treasure, 0x03);
		uint8_t a = e;
		if (d) {
			a = alu_.rla(a);
			d = alu_.dec(d);
			if (d) {
				a = alu_.rla(a);
				d = alu_.dec(d);
				if (d)
					a = alu_.rla(a);
			}
		}
		cpu_.e = e;
		cpu_.d = a;
		SET_HL(gb_, hl);
		cpu_.a = alu_.and_(a, rd8(hl));
		if (!cpu_.a)
			return; // not obtained
		cpu_.a = TRUE;
		cpu_.f = (uint8_t)((cpu_.f & FLAG_Z) | FLAG_C); // scf
	}

	// hl += a * 2
	void port_AddOffsetInPointerTable()
	{
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.a = alu_.adc(0, 0);
		cpu_.d = cpu_.a;
		SET_HL(gb_, alu_.add16(REG_HL(gb_), REG_DE(gb_)));
	}

	// ld a, [hli] / ld h, [hl] / ld l, a, with the second register pair
	// written to hi:lo
	void load_pointer(uint8_t *hi, uint8_t *lo)
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		*hi = rd8(hl);
		if (hi != &cpu_.h) {
			cpu_.h = (uint8_t)(hl >> 8);
			cpu_.l = (uint8_t)hl;
		}
		*lo = cpu_.a;
	}

	// outputs in hl the pointer corresponding to the index given in register a
	// from the pointer table given in hl
	void port_GetPointerFromTableHL()
	{
		CALL(AddOffsetInPointerTable);
		load_pointer(&cpu_.h, &cpu_.l);
	}

	// same as GetPointerFromTableHL but outputs pointer to de
	void port_GetPointerFromTableDE()
	{
		CALL(AddOffsetInPointerTable);
		load_pointer(&cpu_.d, &cpu_.e);
	}

	// outputs in hl the cth word contained in the ath table entry in the
	// pointer table in hl
	void port_GetCthWordFromAthTable()
	{
		CALL(AddOffsetInPointerTable);
		load_pointer(&cpu_.h, &cpu_.l);
		cpu_.a = cpu_.c;
		CALL(AddOffsetInPointerTable);
	}

	void port_GetByteFromPointerInHL()
	{
		load_pointer(&cpu_.h, &cpu_.l);
		cpu_.a = rd8(REG_HL(gb_));
	}

	// hl = y, x in pixels
	// output: hl = wOWTilemapPtr = their tile in wTilemap
	void port_GetOWCoordInTilemap()
	{
		uint16_t hl = REG_HL(gb_);
		uint8_t y = alu_.and_(rd8(hl++), 0xf8);
		uint8_t c = (uint8_t)(y << 2 | y >> 6); // rlca / rlca: y / 8 * 32
		cpu_.b = alu_.and_(c, 0x0f);
		cpu_.c = alu_.and_(c, 0xf0);
		uint8_t x = alu_.and_(rd8(hl), 0xf8);
		x = (uint8_t)(x << 1 | x >> 7); // rlca
		x = alu_.swap(x); // x / 8
		cpu_.a = alu_.add(x, cpu_.c);
		cpu_.l = cpu_.a;
		wr8(wOWTilemapPtr + 0, cpu_.a);
		cpu_.a = alu_.add(cpu_.b, (uint8_t)(wTilemap >> 8));
		cpu_.h = cpu_.a;
		wr8(wOWTilemapPtr + 1, cpu_.a);
	}

	// loads source address of tiles
	// with the given level's name for HDMA transfer
	// writes them to v0Tiles2 tile $50
	// b = map side constant (NORTH, ...)
	// d = level index within that map side
	void port_LoadLevelName()
	{
		cpu_.a = alu_.add(BANK_LevelNamesJPGfx, rd8(wLanguage));
		SET_HL(gb_, wLanguage);
		wr8(wHDMABank, cpu_.a);
		wr8(wHDMA, 0);
		cpu_.c = 0x00;
		cpu_.b = alu_.swap(cpu_.b); // *$10
		uint16_t hl = alu_.add16(ADDR_LevelNamesJPGfx, REG_BC(gb_)); // aka LevelNamesENGfx
		cpu_.d = alu_.add(cpu_.d, cpu_.d); // *2
		cpu_.e = 0x00;
		hl = alu_.add16(hl, REG_DE(gb_));
		SET_HL(gb_, hl);
		wr8(w2d0b5SourceHi, cpu_.h);
		wr8(w2d0b5SourceLo, cpu_.l);
		wr8(w2d0b5DestHi, 0x15);
		wr8(w2d0b5DestLo, 0);
		cpu_.f = FLAG_Z; // xor a
		cpu_.a = 32 - 1; // 32 dma_tiles
		wr8(w2d0b5Mode, cpu_.a);
	}

	// a = value to write to [bc]
	// if [hl] is $80, also clears [bc | 7]
	void port_Func_3c29()
	{
		wr8(REG_BC(gb_), cpu_.a);
		cpu_.a = rd8(REG_HL(gb_));
		alu_.cp(cpu_.a, 0x80);
		if (!(cpu_.f & FLAG_Z))
			return;
		cpu_.a = alu_.or_(0x07, cpu_.c);
		cpu_.c = cpu_.a;
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(REG_BC(gb_), 0);
	}

	// hl = struct, de = table of (add, ?) byte pairs
	// adds the next table entry (indexed by the counter at [hl | 7]) to [hl]
	void port_Func_3c35()
	{
		cpu_.c = cpu_.l;
		cpu_.b = cpu_.h;
		cpu_.a = alu_.or_(0x07, cpu_.l);
		cpu_.l = cpu_.a;
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl);
		wr8(hl, alu_.inc(rd8(hl)));
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.l = cpu_.a;
		cpu_.a = alu_.adc(0x00, 0x00);
		cpu_.h = cpu_.a;
		hl = alu_.add16(REG_HL(gb_), REG_DE(gb_));
		cpu_.a = rd8(hl++);
		cpu_.d = cpu_.a;
		cpu_.a = rd8(hl++);
		cpu_.e = cpu_.a;
		SET_HL(gb_, hl);
		cpu_.a = alu_.add(rd8(REG_BC(gb_)), cpu_.d);
		wr8(REG_BC(gb_), cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(REG_BC(gb_));
	}

	// starts the HDMA transfer whose parameters are in w2d0b5
	void port_Func_3c03()
	{
		cpu_.a = BANK_v1Tiles0; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		uint16_t hl = w2d0b5;
		cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
		for (int i = 0; i < 5; i++) {
			cpu_.a = rd8(hl++);
			if (i)
				cpu_.c = alu_.inc(cpu_.c);
			wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		}
		SET_HL(gb_, hl);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wHDMABank, 0);
	}

	// hl = tilemap, b = its bank
	void port_LoadFarBGMap()
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = cpu_.b;
		bankswitch(cpu_.a);
		SET_BC(gb_, 18 * TILEMAP_WIDTH);
		SET_DE(gb_, v0BGMap0);
		CALL(CopyHLToDE_BC);
		pop_af();
		bankswitch(cpu_.a);
	}

	// hl = pal config: register (cleared after reading), index, number of
	// 8-byte blocks, pointer to the colours (big-endian)
	void port_ApplyPalConfig()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.c = rd8(hl);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(hl++, 0);
		cpu_.a = rd8(hl++);
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(hl++);
		cpu_.b = cpu_.a;
		cpu_.a = rd8(hl++);
		cpu_.l = rd8(hl);
		cpu_.h = cpu_.a;
		hl = REG_HL(gb_);
		do {
			for (int i = 0; i < 8; i++) {
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}

	// same as ApplyOWMovement but the x offsets are mirrored
	// hl = OW obj
	// de = relative positions
	void port_ApplyOWMovement_Mirrored()
	{
		CALL(Func_3c35);
		cpu_.a = alu_.sub(cpu_.a, cpu_.e);
		JUMP(Func_3c29);
	}

	// hl = OW obj
	// de = relative positions
	// returns z if movement has finished
	void port_ApplyOWMovement()
	{
		CALL(Func_3c35);
		cpu_.a = alu_.add(cpu_.a, cpu_.e);
		JUMP(Func_3c29); // fallthrough
	}

	void port_Func_3c76()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, w2d0c0);
		SET_BC(gb_, 0x10);
		CALL(WriteAToHL_BCTimes);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeOverworld;

PORT_EX(IsTreasureCollected, 0, 0) { HomeOverworld(gb).port_IsTreasureCollected(); }
PORT_EX(AddOffsetInPointerTable, 0, 0) { HomeOverworld(gb).port_AddOffsetInPointerTable(); }
PORT_EX(GetPointerFromTableHL, 0, 0) { HomeOverworld(gb).port_GetPointerFromTableHL(); }
PORT_EX(GetPointerFromTableDE, 0, 0) { HomeOverworld(gb).port_GetPointerFromTableDE(); }
PORT_EX(GetCthWordFromAthTable, 0, 0) { HomeOverworld(gb).port_GetCthWordFromAthTable(); }
PORT_EX(GetByteFromPointerInHL, 0, 0) { HomeOverworld(gb).port_GetByteFromPointerInHL(); }
PORT_EX(GetOWCoordInTilemap, 0, 0) { HomeOverworld(gb).port_GetOWCoordInTilemap(); }
PORT_EX(LoadLevelName, 0, 0) { HomeOverworld(gb).port_LoadLevelName(); }
PORT_EX(Func_3c29, 0, 0) { HomeOverworld(gb).port_Func_3c29(); }
PORT_EX(Func_3c35, 0, 0) { HomeOverworld(gb).port_Func_3c35(); }
PORT_EX(Func_3c03, 0, 0) { HomeOverworld(gb).port_Func_3c03(); }
PORT_EX(LoadFarBGMap, 0, 0) { HomeOverworld(gb).port_LoadFarBGMap(); }
PORT_EX(ApplyPalConfig, 0, 0) { HomeOverworld(gb).port_ApplyPalConfig(); }
PORT_EX(ApplyOWMovement_Mirrored, HOOK_MANUAL_RET, 0) { HomeOverworld(gb).port_ApplyOWMovement_Mirrored(); }
PORT_EX(ApplyOWMovement, HOOK_MANUAL_RET, 0) { HomeOverworld(gb).port_ApplyOWMovement(); }
PORT_EX(Func_3c76, 0, 0) { HomeOverworld(gb).port_Func_3c76(); }
