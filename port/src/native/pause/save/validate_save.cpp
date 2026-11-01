// pause/save/validate_save (src/engine/pause/save/validate_save.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::pause {

namespace {

class PauseSaveValidateSave : public AsmModule {
public:
	using AsmModule::AsmModule;

	// hl = 4 check values
	// returns carry set if they don't match
	void check_vals(const uint8_t vals[4])
	{
		uint16_t hl = REG_HL(gb_);
		for (int i = 0; i < 4; i++) {
			cpu_.a = rd8(hl++);
			alu_.cp(cpu_.a, vals[i]);
			if (!(cpu_.f & FLAG_Z)) {
				SET_HL(gb_, hl);
				cpu_.f = (uint8_t)((cpu_.f & FLAG_Z) | FLAG_C); // scf
				return;
			}
		}
		SET_HL(gb_, hl);
		cpu_.f &= FLAG_Z; // scf / ccf
	}

	void port_CheckSaveVals1()
	{
		static const uint8_t vals[4] = {CHECKVAL1_1, CHECKVAL1_2, CHECKVAL1_3, CHECKVAL1_4};
		check_vals(vals);
	}

	void port_CheckSaveVals2()
	{
		static const uint8_t vals[4] = {CHECKVAL2_1, CHECKVAL2_2, CHECKVAL2_3, CHECKVAL2_4};
		check_vals(vals);
	}

	void port_CalculateGameDataChecksum()
	{
		SET_DE(gb_, 0);
		SET_HL(gb_, 0xca00); // STARTOF("Progress WRAM")
		cpu_.b = 0x5b;   // SIZEOF("Progress WRAM")
		gb_call(gb_, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
		// push_wram BANK("WRAM2")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = BANK_wTreasuresCollected;
		wr8(rWBK, cpu_.a);
		SET_HL(gb_, wTreasuresCollected);
		cpu_.b = (uint8_t)((wOWLevel - wTreasuresCollected) + 2);
		gb_call(gb_, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	// ld a, val / ld [s0a790], a / ld [s0a7e0], a / ld [s0afa0], a
	void save_marker(uint8_t val)
	{
		cpu_.a = val;
		wr8(s0a790, val);
		wr8(s0a7e0, val);
		wr8(s0afa0, val);
	}

	// ld a, [wSRAMBank] / push af / ld a, bank / sramswitch
	void push_sram(uint8_t bank)
	{
		cpu_.a = rd8(wSRAMBank);
		push_af();
		cpu_.a = bank;
		sramswitch(bank);
	}

	// pop af / sramswitch
	void pop_sram()
	{
		pop_af();
		sramswitch(cpu_.a);
	}

	// copies SRAM 2 a000-afff to b000-bfff and SRAM 1 to SRAM 3, then stores
	// the backup SRAM checksum with its echoes
	void port_Func_1f14c6()
	{
		save_marker(0x70);
		push_sram(0x02);
		SET_HL(gb_, s2a000);
		SET_DE(gb_, s2b000);
		SET_BC(gb_, 0x1000);
		CALL(CopyHLToDE_BC);
		pop_sram();

		SET_HL(gb_, s0a000);
		SET_BC(gb_, 0x2000);
		do {
			push_sram(0x01);
			cpu_.d = rd8(REG_HL(gb_));
			push_sram(0x03);
			cpu_.a = cpu_.d;
			uint16_t hl = REG_HL(gb_);
			wr8(hl, cpu_.a);
			SET_HL(gb_, hl + 1);
			pop_sram();
			pop_sram();
			cpu_.c = alu_.dec(cpu_.c);
			if (cpu_.c)
				continue;
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b || cpu_.c);

		save_marker(0x71);
		gb_call(gb_, ADDR_CalculateBackupSRAMChecksum2);
		cpu_.a = cpu_.d;
		wr8(s0a79f + 0, cpu_.a);
		cpu_.a = cpu_.e;
		wr8(s0a79f + 1, cpu_.a);
		save_marker(0x72);
		cpu_.a = cpu_.d;
		wr8(s0a7ef + 0, cpu_.a);
		cpu_.a = cpu_.e;
		wr8(s0a7ef + 1, cpu_.a);
		save_marker(0x73);
		cpu_.a = cpu_.d;
		wr8(s0afaf + 0, cpu_.a);
		cpu_.a = cpu_.e;
		wr8(s0afaf + 1, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		save_marker(0);
	}

	// push_wram bank: ldh a, [rWBK] / push af / ld a, bank / ldh [rWBK], a
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

	// Func_1f0d60.GetMatchingChecksum
	// bc = checksum, de = echo 1, hl = echo 2; sets wChecksum (best option),
	// wcee9, wceeb and wChecksumMismatches
	void get_matching_checksum()
	{
		uint8_t b = cpu_.b, c = cpu_.c, d = cpu_.d, e = cpu_.e;
		uint8_t h = cpu_.h, l = cpu_.l;
		enum { M_NONE, M_ECHO1, M_ECHO2, M_ECHOS, M_ALL } r;

		cpu_.a = h;
		alu_.cp(h, b);
		if (cpu_.f & FLAG_Z) {
			alu_.cp(h, d);
			if (cpu_.f & FLAG_Z) {
				// .all_high_bytes_equal
				cpu_.a = l;
				alu_.cp(l, c);
				if (cpu_.f & FLAG_Z) {
					alu_.cp(l, e);
					r = (cpu_.f & FLAG_Z) ? M_ALL : M_ECHO2;
				} else {
					// .check_echos_equality
					alu_.cp(l, e);
					if (cpu_.f & FLAG_Z) {
						r = M_ECHOS;
					} else {
						cpu_.a = c;
						alu_.cp(c, e);
						r = (cpu_.f & FLAG_Z) ? M_ECHO1 : M_NONE;
					}
				}
			} else {
				cpu_.a = c;
				alu_.cp(c, l);
				r = (cpu_.f & FLAG_Z) ? M_ECHO2 : M_NONE;
			}
		} else {
			// .echo2_not_equal
			alu_.cp(h, d);
			if (cpu_.f & FLAG_Z) {
				// .check_lower_echos
				cpu_.a = e;
				alu_.cp(e, l);
				r = (cpu_.f & FLAG_Z) ? M_ECHOS : M_NONE;
			} else {
				cpu_.a = b;
				alu_.cp(b, d);
				if (cpu_.f & FLAG_Z) {
					// .check_lower_echo1
					cpu_.a = c;
					alu_.cp(c, e);
					r = (cpu_.f & FLAG_Z) ? M_ECHO1 : M_NONE;
				} else {
					r = M_NONE;
				}
			}
		}

		switch (r) {
		case M_NONE:
			wr8(wChecksumMismatches, MISMATCH_ORIG | MISMATCH_ECHO1 | MISMATCH_ECHO2);
			wr8(wChecksum + 0, b);
			wr8(wcee9 + 0, d);
			wr8(wceeb + 0, h);
			wr8(wChecksum + 1, c);
			wr8(wcee9 + 1, e);
			wr8(wceeb + 1, l);
			cpu_.a = l;
			break;
		case M_ECHO1:
			wr8(wChecksumMismatches, MISMATCH_ECHO2);
			wr8(wChecksum + 0, b);
			wr8(wceeb + 0, b);
			wr8(wcee9 + 0, h);
			wr8(wChecksum + 1, c);
			wr8(wceeb + 1, c);
			wr8(wcee9 + 1, l);
			cpu_.a = l;
			break;
		case M_ECHO2:
			wr8(wChecksumMismatches, MISMATCH_ECHO1);
			wr8(wChecksum + 0, b);
			wr8(wceeb + 0, b);
			wr8(wcee9 + 0, d);
			wr8(wChecksum + 1, c);
			wr8(wceeb + 1, c);
			wr8(wcee9 + 1, e);
			cpu_.a = e;
			break;
		case M_ECHOS:
			wr8(wChecksumMismatches, MISMATCH_ORIG);
			wr8(wChecksum + 0, d);
			wr8(wceeb + 0, d);
			wr8(wcee9 + 0, b);
			wr8(wChecksum + 1, e);
			wr8(wceeb + 1, e);
			wr8(wcee9 + 1, c);
			cpu_.a = c;
			break;
		case M_ALL:
			cpu_.f = FLAG_Z; // xor a
			wr8(wChecksumMismatches, 0);
			wr8(wChecksum + 0, b);
			wr8(wcee9 + 0, b);
			wr8(wceeb + 0, b);
			wr8(wChecksum + 1, c);
			wr8(wcee9 + 1, c);
			wr8(wceeb + 1, c);
			cpu_.a = c;
			break;
		}
	}

	// Loads bc, de and hl from the checksum at p0 and its echoes at p1 / p2,
	// then call .GetMatchingChecksum.
	void load_checksums(uint16_t p0, uint16_t p1, uint16_t p2)
	{
		cpu_.b = rd8(p0);
		cpu_.c = rd8(p0 + 1);
		cpu_.d = rd8(p1);
		cpu_.e = rd8(p1 + 1);
		cpu_.h = rd8(p2);
		cpu_.l = cpu_.a = rd8(p2 + 1);
		get_matching_checksum();
	}

	// Func_1f0d60.CalculateChecksumAtHL
	void calculate_checksum_at_hl()
	{
		SET_DE(gb_, 0);
		cpu_.b = 0x5b; // SIZEOF("Progress WRAM")
		gb_call(gb_, ADDR_CalculateChecksum);
		push_wram(0x02);
		cpu_.b = 0x11;
		gb_call(gb_, ADDR_CalculateChecksum);
		pop_wram();
	}

	// Whether the calculated checksum in de matches wChecksum, or (with
	// mismatches) wcee9, or (with all three mismatched) wceeb.
	bool checksum_ok()
	{
		cpu_.a = rd8(wChecksum + 0);
		alu_.cp(cpu_.a, cpu_.d);
		if (cpu_.f & FLAG_Z) {
			cpu_.a = rd8(wChecksum + 1);
			alu_.cp(cpu_.a, cpu_.e);
			if (cpu_.f & FLAG_Z)
				return true;
		}
		cpu_.a = alu_.and_(rd8(wChecksumMismatches), rd8(wChecksumMismatches));
		if (!cpu_.a)
			return false;
		cpu_.a = rd8(wcee9 + 0);
		alu_.cp(cpu_.a, cpu_.d);
		if (cpu_.f & FLAG_Z) {
			cpu_.a = rd8(wcee9 + 1);
			alu_.cp(cpu_.a, cpu_.e);
			if (cpu_.f & FLAG_Z)
				return true;
		}
		cpu_.a = rd8(wChecksumMismatches);
		alu_.cp(cpu_.a, MISMATCH_ORIG | MISMATCH_ECHO1 | MISMATCH_ECHO2);
		if (!(cpu_.f & FLAG_Z))
			return false;
		cpu_.a = rd8(wceeb + 0);
		alu_.cp(cpu_.a, cpu_.d);
		if (!(cpu_.f & FLAG_Z))
			return false;
		cpu_.a = rd8(wceeb + 1);
		alu_.cp(cpu_.a, cpu_.e);
		return (cpu_.f & FLAG_Z) != 0;
	}

	// ld hl, wceee / set n, [hl]
	void set_wceee(int n)
	{
		SET_HL(gb_, wceee);
		wr8(wceee, (uint8_t)(rd8(wceee) | 1 << n));
	}

	// xor a / ld hl, addr / ld [hli], a (x4)
	void clear_check_vals(uint16_t addr)
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		for (int i = 0; i < 4; i++)
			wr8(addr + i, 0);
		SET_HL(gb_, addr + 4);
	}

	// marker, clear the check values at addr, then xor a and a 0 marker
	void clear_save(uint8_t marker, uint16_t addr)
	{
		save_marker(marker);
		clear_check_vals(addr);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		save_marker(0);
	}

	void port_Func_1f0d60()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wceee, 0);

		// game data
		SET_HL(gb_, sCheckVals);
		gb_call(gb_, ADDR_CheckSaveVals1);
		bool ok = false;
		if (!(cpu_.f & FLAG_C)) {
			load_checksums(sGameDataChecksum, sGameDataChecksumEcho1, sGameDataChecksumEcho2);
			SET_HL(gb_, sGameData);
			calculate_checksum_at_hl();
			ok = checksum_ok();
		}
		if (ok)
			set_wceee(0);
		else
			clear_save(0x10, sCheckVals); // .clear_check_vals

		// .check_backup_check_vals
		SET_HL(gb_, sBackupCheckVals);
		gb_call(gb_, ADDR_CheckSaveVals1);
		ok = false;
		if (!(cpu_.f & FLAG_C)) {
			load_checksums(sBackupGameDataChecksum, sBackupGameDataChecksumEcho1, sBackupGameDataChecksumEcho2);
			SET_HL(gb_, sBackupGameData);
			calculate_checksum_at_hl();
			ok = checksum_ok();
		}
		if (ok)
			set_wceee(1);
		else
			clear_save(0x20, sBackupCheckVals); // .clear_backup_check_vals

		// .asm_1f0e6e: backup SRAM 1
		load_checksums(s0a79d, s0a7ed, s0afad);
		gb_call(gb_, ADDR_CalculateBackupSRAMChecksum1);
		bool wram_ok = false;
		if (checksum_ok()) {
			// .asm_1f0ec1
			set_wceee(6);
			SET_HL(gb_, s0a000);
			gb_call(gb_, ADDR_CheckSaveVals2);
			ok = false;
			if (!(cpu_.f & FLAG_C)) {
				load_checksums(sWRAMChecksum, sWRAMChecksumEcho1, sWRAMChecksumEcho2);
				SET_HL(gb_, sSaveCounter);
				gb_call(gb_, ADDR_Func_1f0b5b);
				ok = checksum_ok();
			}
			if (ok) {
				set_wceee(2); // .asm_1f0f27
				wram_ok = true;
			} else {
				// .asm_1f0f2f
				clear_save(0x30, s0a000);
				SET_HL(gb_, s0a800);
				gb_call(gb_, ADDR_CheckSaveVals2);
				if (!(cpu_.f & FLAG_C)) {
					load_checksums(sBackup1WRAMChecksum, sBackup1WRAMChecksumEcho1, sBackup1WRAMChecksumEcho2);
					SET_HL(gb_, s0a804);
					gb_call(gb_, ADDR_Func_1f0b5b);
					if (checksum_ok()) {
						set_wceee(3); // .asm_1f0faa
						wram_ok = true;
					}
				}
			}
		}
		if (!wram_ok) {
			// .asm_1f0fb2
			save_marker(0x30);
			clear_check_vals(s0a000);
			save_marker(0x50);
			clear_check_vals(s0a800);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			save_marker(0);
		}

		// .asm_1f0fe2: backup SRAM 2
		load_checksums(s0a79f, s0a7ef, s0afaf);
		gb_call(gb_, ADDR_CalculateBackupSRAMChecksum2);
		if (checksum_ok()) {
			// .asm_1f1035
			set_wceee(7);
			SET_HL(gb_, s0a400);
			gb_call(gb_, ADDR_CheckSaveVals2);
			ok = false;
			if (!(cpu_.f & FLAG_C)) {
				load_checksums(sBackup2WRAMChecksum, sBackup2WRAMChecksumEcho1, sBackup2WRAMChecksumEcho2);
				SET_HL(gb_, s0a404);
				gb_call(gb_, ADDR_Func_1f0b5b);
				ok = checksum_ok();
			}
			if (ok) {
				set_wceee(4); // .asm_1f109b
				return;
			}
			// .asm_1f10a1
			clear_save(0x60, s0a400);
			SET_HL(gb_, s0ac00);
			gb_call(gb_, ADDR_CheckSaveVals2);
			if (!(cpu_.f & FLAG_C)) {
				load_checksums(sBackup3WRAMChecksum, sBackup3WRAMChecksumEcho1, sBackup3WRAMChecksumEcho2);
				SET_HL(gb_, s0ac04);
				gb_call(gb_, ADDR_Func_1f0b5b);
				if (checksum_ok()) {
					set_wceee(5); // .asm_1f111c
					return;
				}
			}
		}
		// .asm_1f1122
		save_marker(0x60);
		clear_check_vals(s0a400);
		save_marker(0x80);
		clear_check_vals(s0ac00);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		save_marker(0);
	}

	// rrca
	uint8_t alu_rrca(uint8_t v)
	{
		cpu_.f = (v & 1) ? FLAG_C : 0;
		return (uint8_t)(v >> 1 | v << 7);
	}

	// ld bc, $600 / .loop: rrca / jr nc, :+ / inc c / : dec b / jr nz, .loop /
	// ld a, c / dec a: returns whether exactly one of a's low 6 bits is set
	bool one_bit_set()
	{
		SET_BC(gb_, 0x600);
		do {
			cpu_.a = alu_.rrca(cpu_.a);
			if (cpu_.f & FLAG_C)
				cpu_.c = alu_.inc(cpu_.c);
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		cpu_.a = alu_.dec(cpu_.c);
		return cpu_.a == 0;
	}

	// Func_1f1246.Func_1f13d7
	void restore_game_data()
	{
		SET_DE(gb_, 0xca00); // STARTOF("Progress WRAM")
		cpu_.b = 0x5b;
		CALL(CopyHLToDE);
		push_wram(0x02);
		SET_DE(gb_, 0xd000);
		cpu_.b = 0x11;
		CALL(CopyHLToDE);
		pop_wram();
	}

	// Func_1f1246.Func_1f13f2
	void restore_wram()
	{
		SET_DE(gb_, wSaveCounter);
		cpu_.b = 0xca;
		CALL(CopyHLToDE);
		push_wram(0x02);
		SET_DE(gb_, 0xd000);
		cpu_.b = 0x11;
		CALL(CopyHLToDE);
		pop_wram();
		push_wram(0x01); // BANK("Level Objects WRAM")
		SET_DE(gb_, 0xd000);  // STARTOF("Level Objects WRAM")
		SET_BC(gb_, 0x014a);  // SIZEOF("Level Objects WRAM")
		CALL(CopyHLToDE_BC);
		pop_wram();
	}

	// Func_1f1246.Func_1f1420: restores the backup SRAM (SRAM2 b000 to a000,
	// SRAM3 to SRAM1) and stores its checksum with the echoes
	void restore_backup_sram()
	{
		save_marker(0x40);
		push_sram(0x02);
		SET_HL(gb_, s2b000);
		SET_DE(gb_, s2a000);
		SET_BC(gb_, 0x1000);
		CALL(CopyHLToDE_BC);
		pop_sram();

		// copy whole SRAM3 to SRAM1
		SET_HL(gb_, s3a000);
		SET_BC(gb_, 0x2000);
		do {
			push_sram(0x03);
			cpu_.d = rd8(REG_HL(gb_));
			push_sram(0x01);
			cpu_.a = cpu_.d;
			uint16_t hl = REG_HL(gb_);
			wr8(hl, cpu_.a);
			SET_HL(gb_, hl + 1);
			pop_sram();
			pop_sram();
			cpu_.c = alu_.dec(cpu_.c);
			if (cpu_.c)
				continue;
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b || cpu_.c);

		save_marker(0x41);
		gb_call(gb_, ADDR_CalculateBackupSRAMChecksum1);
		cpu_.a = cpu_.d;
		wr8(s0a79d + 0, cpu_.a);
		cpu_.a = cpu_.e;
		wr8(s0a79d + 1, cpu_.a);
		save_marker(0x42);
		cpu_.a = cpu_.d;
		wr8(s0a7ed + 0, cpu_.a);
		cpu_.a = cpu_.e;
		wr8(s0a7ed + 1, cpu_.a);
		save_marker(0x43);
		cpu_.a = cpu_.d;
		wr8(s0afad + 0, cpu_.a);
		cpu_.a = cpu_.e;
		wr8(s0afad + 1, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		save_marker(0);
	}

	void port_Func_1f1246()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wceef, 0);
		cpu_.a = alu_.and_(rd8(wceee), 0x3f);
		bool reset_scores = false;
		if (!cpu_.a) {
			reset_scores = true;
		} else {
			// .asm_1f1266
			cpu_.b = cpu_.a;
			cpu_.a = alu_.and_(0x3f, cpu_.b);
			wr8(wceef, cpu_.a);
			if (!one_bit_set()) {
				static const uint16_t srcs[6] = { sGameData, sBackupGameData, sSaveCounter, s0a804, s0a404, s0ac04 };
				for (int i = 0; i < 6; i++) {
					SET_HL(gb_, srcs[i]);
					SET_DE(gb_, s0afe0 + 4 * i);
					cpu_.b = 4;
					CALL(CopyHLToDE);
				}
				cpu_.e = 0x21;
				for (;;) {
					// .asm_1f12c0
					cpu_.e = alu_.dec(cpu_.e);
					if (!cpu_.e)
						break;
					cpu_.a = 0;
					cpu_.f = FLAG_Z; // xor a
					wr8(s0aff8, 0);
					for (int k = 0; k < 6; k++) {
						cpu_.a = rd8(wceef);
						if (!alu_.bit(cpu_.a, k))
							continue;
						uint16_t hl = (uint16_t)(s0afe0 + 4 * k + 3);
						wr8(hl, alu_.sla(rd8(hl)));
						for (int j = 0; j < 3; j++) {
							hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
							wr8(hl, alu_.rl(rd8(hl)));
						}
						SET_HL(gb_, hl);
						if (cpu_.f & FLAG_C) {
							SET_HL(gb_, s0aff8);
							wr8(s0aff8, (uint8_t)(rd8(s0aff8) | 1 << k));
						}
					}
					// .asm_1f1370
					cpu_.a = alu_.and_(rd8(s0aff8), rd8(s0aff8));
					if (!cpu_.a)
						continue;
					wr8(wceef, cpu_.a);
					if (one_bit_set())
						break;
				}
			}
			// .asm_1f1389
			cpu_.a = rd8(wceef);
			int k;
			for (k = 0; k < 6; k++)
				if (alu_.bit(cpu_.a, k))
					break;
			switch (k) {
			case 0:
				SET_HL(gb_, sGameData);
				restore_game_data();
				return;
			case 1:
				SET_HL(gb_, sBackupGameData);
				restore_game_data();
				return;
			case 2:
				SET_HL(gb_, sSaveCounter);
				restore_wram();
				return;
			case 3:
				SET_HL(gb_, s0a804);
				restore_wram();
				return;
			case 4:
				SET_HL(gb_, s0a404);
				restore_wram();
				restore_backup_sram();
				return;
			case 5:
				SET_HL(gb_, s0ac04);
				restore_wram();
				restore_backup_sram();
				return;
			default:
				reset_scores = true;
				break;
			}
		}
		if (reset_scores) {
			// .asm_1f1251
			SET_HL(gb_, wLevelTimeAttackScores);
			cpu_.b = 2 * NUM_LEVELS;
			cpu_.a = (10 << 4) | 10;
			CALL(WriteAToHL_BTimes);
			SET_HL(gb_, wGolfBestScores);
			cpu_.b = 0x4;
			cpu_.a = (10 << 4) | 10;
			CALL(WriteAToHL_BTimes);
		}
	}

	void port_ValidateSaveData()
	{
		push_sram(0x00); // BANK("SRAM0")

		static const struct { uint16_t vals; bool backup_vals1; } checks[6] = {
			{ sCheckVals, true }, { s0a000, false }, { s0a400, false },
			{ sBackupCheckVals, true }, { s0a800, false }, { s0ac00, false },
		};
		bool found = false;
		for (int i = 0; i < 6 && !found; i++) {
			SET_HL(gb_, checks[i].vals);
			gb_call(gb_, checks[i].backup_vals1 ? ADDR_CheckSaveVals1 : ADDR_CheckSaveVals2);
			found = !(cpu_.f & FLAG_C);
		}

		bool ok = false;
		if (found) {
			// .checkvals_ok
			cpu_.b = rd8(s0a790);
			cpu_.c = rd8(s0a7e0);
			cpu_.d = cpu_.a = rd8(s0afa0);
			alu_.cp(cpu_.a, cpu_.b); // s0afa0 == s0a790?
			if (cpu_.f & FLAG_Z) {
				alu_.cp(cpu_.a, cpu_.c); // both branches are the same
				ok = true;
			} else {
				// .asm_1f0d08
				alu_.cp(cpu_.a, cpu_.c); // s0afa0 == s0a7e0?
				if (cpu_.f & FLAG_Z) {
					ok = true;
				} else {
					cpu_.a = cpu_.b;
					alu_.cp(cpu_.a, cpu_.c); // s0a790 == s0a7e0?
					ok = (cpu_.f & FLAG_Z) != 0;
				}
			}
		}
		if (!ok)
			cpu_.a = 0xff; // .asm_1f0d0f
		wr8(wcef1, cpu_.a);

		// .asm_1f0d20
		gb_call(gb_, ADDR_Func_1f0d60);
		gb_call(gb_, ADDR_Func_1f1246);
		pop_sram();
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PauseSaveValidateSave;

PORT_EX(CheckSaveVals1, 0, 0) { PauseSaveValidateSave(gb).port_CheckSaveVals1(); }
PORT_EX(CheckSaveVals2, 0, 0) { PauseSaveValidateSave(gb).port_CheckSaveVals2(); }
PORT_EX(CalculateGameDataChecksum, 0, 0) { PauseSaveValidateSave(gb).port_CalculateGameDataChecksum(); }
PORT_EX(Func_1f14c6, 0, 0) { PauseSaveValidateSave(gb).port_Func_1f14c6(); }
PORT_EX(Func_1f0d60, 0, 0) { PauseSaveValidateSave(gb).port_Func_1f0d60(); }
PORT_EX(Func_1f1246, 0, 0) { PauseSaveValidateSave(gb).port_Func_1f1246(); }
PORT_EX(ValidateSaveData, 0, 0) { PauseSaveValidateSave(gb).port_ValidateSaveData(); }
