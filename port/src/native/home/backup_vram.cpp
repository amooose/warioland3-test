// home/backup_vram (src/home/backup_vram.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeBackupVram : public AsmModule {
public:
	using AsmModule::AsmModule;

	// push_wram: ldh a, [rWBK] / push af / ld a, bank / ldh [rWBK], a
	void push_wram(uint8_t bank)
	{
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = bank;
		wr8(rWBK, bank);
	}

	// pop_wram: pop af / ldh [rWBK], a
	void pop_wram()
	{
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	void copy_bc(uint16_t src, uint16_t dst, uint16_t n)
	{
		SET_HL(gb_, src);
		SET_DE(gb_, dst);
		SET_BC(gb_, n);
		CALL(CopyHLToDE_BC);
	}

	// xor a / ldh [rVBK], a
	void vram0()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		wr8(rVBK, 0);
	}

	void port_SaveBackupVRAM()
	{
		push_wram(BANK_w3d500);
		cpu_.a = rd8(wSCY);
		wr8(w3d500, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(w3d501, cpu_.a);
		SET_HL(gb_, wTempPals1);
		SET_DE(gb_, w3d280);
		cpu_.b = 16 * PAL_SIZE;
		CALL(CopyHLToDE);
		pop_wram();

		vram0();
		push_wram(BANK_w4d000);
		copy_bc(v0Tiles0, w4d000, v0Tiles2 - v0Tiles0);
		pop_wram();

		push_wram(BANK_w5d000);
		copy_bc(v0Tiles2, w5d000, v0End - v0Tiles2);
		pop_wram();

		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, cpu_.a);
		push_wram(BANK_w6d000);
		copy_bc(v1Tiles0, w6d000, v1Tiles2 - v1Tiles0);
		pop_wram();

		push_wram(BANK_w7d000);
		copy_bc(v1Tiles2, w7d000, v1End - v1Tiles2);
		pop_wram();

		vram0();
	}

	void port_LoadBackupVRAM()
	{
		push_wram(0x03);
		cpu_.a = rd8(w3d500);
		wr8(wSCY, cpu_.a);
		cpu_.a = rd8(w3d501);
		wr8(wSCX, cpu_.a);
		SET_HL(gb_, w3d280);
		SET_DE(gb_, wTempPals1);
		cpu_.b = 16 * PAL_SIZE;
		CALL(CopyHLToDE);
		pop_wram();
		vram0();

		push_wram(BANK_w4d000);
		copy_bc(w4d000, v0Tiles0, 0x1000);
		pop_wram();

		push_wram(BANK_w5d000);
		copy_bc(w5d000, v0Tiles2, 0x1000);
		pop_wram();

		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, cpu_.a);
		push_wram(BANK_w6d000);
		copy_bc(w6d000, v1Tiles0, 0x1000);
		pop_wram();

		push_wram(BANK_w7d000);
		copy_bc(w7d000, v1Tiles2, 0x1000);
		pop_wram();
		vram0();
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeBackupVram;

PORT_EX(SaveBackupVRAM, 0, 0) { HomeBackupVram(gb).port_SaveBackupVRAM(); }
PORT_EX(LoadBackupVRAM, 0, 0) { HomeBackupVram(gb).port_LoadBackupVRAM(); }
