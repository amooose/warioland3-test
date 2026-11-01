// pause/save/checksum (src/engine/pause/save/checksum.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::pause {

namespace {

class PauseSaveChecksum : public AsmModule {
public:
	using AsmModule::AsmModule;

	// de += [hl++]
	void add_byte(uint16_t *hl)
	{
		cpu_.a = alu_.add(rd8((*hl)++), cpu_.e);
		cpu_.e = cpu_.a;
		cpu_.a = alu_.adc(cpu_.d, 0);
		cpu_.d = cpu_.a;
	}

	// sums b bytes starting at hl
	// result is added in de
	void port_CalculateChecksum()
	{
		uint16_t hl = REG_HL(gb_);
		do {
			add_byte(&hl);
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}

	// same as CalculateChecksum but for bc bytes; buggy, after the first byte
	// it jumps into CalculateChecksum
	// The jr runs whatever bank is mapped, so it's a JUMP_TO, not a JUMP.
	void port_CalculateChecksumLong()
	{
		uint16_t hl = REG_HL(gb_);
		add_byte(&hl);
		SET_HL(gb_, hl);
		SET_BC(gb_, REG_BC(gb_) - 1);
		cpu_.a = alu_.or_(cpu_.b, cpu_.c);
		if (!(cpu_.f & FLAG_Z)) {
			JUMP_TO(ADDR_CalculateChecksum);
			return;
		}
		RET();
	}

	// increments 4-byte counter in wSaveCounter
	void port_IncrementSaveCounter()
	{
		cpu_.a = alu_.add(rd8(wSaveCounter + 3), 1);
		wr8(wSaveCounter + 3, cpu_.a);
		for (int i = 2; i >= 0; i--) {
			cpu_.a = alu_.adc(rd8((uint16_t)(wSaveCounter + i)), 0);
			wr8((uint16_t)(wSaveCounter + i), cpu_.a);
		}
	}

	// SIZEOF("Level Objects WRAM"), SIZEOF("Progress WRAM") + SIZEOF("Level WRAM")
	#define SIZEOF_LEVEL_OBJECTS_WRAM 0x14a
	#define SIZEOF_PROGRESS_LEVEL_WRAM 0xca

	// push_wram bank
	void push_wram(uint8_t bank)
	{
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = bank;
		wr8(rWBK, bank);
	}

	void pop_wram()
	{
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	void port_Func_1f0b5b()
	{
		SET_DE(gb_, 0);
		cpu_.b = 0xca;
		gb_call(gb_, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
		push_wram(0x02);
		cpu_.b = 0x11;
		gb_call(gb_, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
		pop_wram();
		push_wram(BANK_wObjects);
		SET_BC(gb_, SIZEOF_LEVEL_OBJECTS_WRAM);
		gb_call(gb_, ADDR_CalculateChecksumLong); // in-bank: runs whatever is mapped
		pop_wram();
	}

	void port_CalculateWRAMDataChecksum()
	{
		SET_DE(gb_, 0);
		SET_HL(gb_, 0xca00); // STARTOF("Progress WRAM")
		cpu_.b = SIZEOF_PROGRESS_LEVEL_WRAM;
		gb_call(gb_, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
		push_wram(BANK_wTreasuresCollected);
		SET_HL(gb_, wTreasuresCollected);
		cpu_.b = (uint8_t)((wOWLevel - wTreasuresCollected) + 2);
		gb_call(gb_, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
		pop_wram();
		push_wram(BANK_wObjects);
		SET_HL(gb_, wObjects);
		SET_BC(gb_, SIZEOF_LEVEL_OBJECTS_WRAM);
		gb_call(gb_, ADDR_CalculateChecksumLong); // in-bank: runs whatever is mapped
		pop_wram();
	}

	// de += sum of the bc bytes at hl (dec c / jr nz / dec b / jr nz)
	void sum_bc()
	{
		uint16_t hl = REG_HL(gb_);
		do {
			do {
				add_byte(&hl);
				cpu_.c = alu_.dec(cpu_.c);
			} while (cpu_.c);
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}

	// checksum of the whole of sram bank 1 (or 3), then half of sram bank 2
	void backup_sram_checksum(uint8_t bank, uint16_t start, uint16_t sram2)
	{
		cpu_.a = rd8(wSRAMBank);
		push_af();
		cpu_.a = bank;
		sramswitch(bank);
		SET_HL(gb_, start);
		SET_DE(gb_, 0);
		SET_BC(gb_, 0x2000); // SIZEOF(SRAM)
		sum_bc();
		pop_af();
		sramswitch(cpu_.a);
		cpu_.a = rd8(wSRAMBank);
		push_af();

		cpu_.a = BANK_s2a000;
		sramswitch(BANK_s2a000);
		SET_HL(gb_, sram2);
		SET_BC(gb_, 0x1000);
		sum_bc();
		pop_af();
		sramswitch(cpu_.a);
	}

	// calculates checksum for SRAM1 and first half of SRAM2
	// outputs result in de
	void port_CalculateBackupSRAMChecksum1()
	{
		backup_sram_checksum(BANK_sLevelBlockObjectMap, sLevelBlockObjectMap, s2a000);
	}

	// calculates checksum for SRAM3 and second half of SRAM2
	// outputs result in de
	void port_CalculateBackupSRAMChecksum2()
	{
		backup_sram_checksum(BANK_s3a000, s3a000, s2b000);
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PauseSaveChecksum;

PORT_EX(CalculateChecksum, 0, 0) { PauseSaveChecksum(gb).port_CalculateChecksum(); }
PORT_EX(CalculateChecksumLong, HOOK_MANUAL_RET, 0) { PauseSaveChecksum(gb).port_CalculateChecksumLong(); }
PORT_EX(IncrementSaveCounter, 0, 0) { PauseSaveChecksum(gb).port_IncrementSaveCounter(); }
PORT_EX(Func_1f0b5b, 0, 0) { PauseSaveChecksum(gb).port_Func_1f0b5b(); }
PORT_EX(CalculateWRAMDataChecksum, 0, 0) { PauseSaveChecksum(gb).port_CalculateWRAMDataChecksum(); }
PORT_EX(CalculateBackupSRAMChecksum1, 0, 0) { PauseSaveChecksum(gb).port_CalculateBackupSRAMChecksum1(); }
PORT_EX(CalculateBackupSRAMChecksum2, 0, 0) { PauseSaveChecksum(gb).port_CalculateBackupSRAMChecksum2(); }
