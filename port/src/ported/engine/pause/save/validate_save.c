// Port of src/engine/pause/save/validate_save.asm
#include "ported.h"

// hl = 4 check values
// returns carry set if they don't match
static void check_vals(GB *gb, const uint8_t vals[4])
{
	uint16_t hl = REG_HL(gb);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, vals[i]);
		if (!(gb->cpu.f & FLAG_Z)) {
			SET_HL(gb, hl);
			gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
			return;
		}
	}
	SET_HL(gb, hl);
	gb->cpu.f &= FLAG_Z; // scf / ccf
}

PORT(CheckSaveVals1)
{
	static const uint8_t vals[4] = {CHECKVAL1_1, CHECKVAL1_2, CHECKVAL1_3, CHECKVAL1_4};
	check_vals(gb, vals);
}

PORT(CheckSaveVals2)
{
	static const uint8_t vals[4] = {CHECKVAL2_1, CHECKVAL2_2, CHECKVAL2_3, CHECKVAL2_4};
	check_vals(gb, vals);
}

PORT(CalculateGameDataChecksum)
{
	SET_DE(gb, 0);
	SET_HL(gb, 0xca00); // STARTOF("Progress WRAM")
	gb->cpu.b = 0x5b;   // SIZEOF("Progress WRAM")
	gb_call(gb, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
	// push_wram BANK("WRAM2")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wTreasuresCollected;
	wr8(gb, rWBK, gb->cpu.a);
	SET_HL(gb, wTreasuresCollected);
	gb->cpu.b = (uint8_t)((wOWLevel - wTreasuresCollected) + 2);
	gb_call(gb, ADDR_CalculateChecksum); // in-bank: runs whatever is mapped
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// ld a, val / ld [s0a790], a / ld [s0a7e0], a / ld [s0afa0], a
static void save_marker(GB *gb, uint8_t val)
{
	gb->cpu.a = val;
	wr8(gb, s0a790, val);
	wr8(gb, s0a7e0, val);
	wr8(gb, s0afa0, val);
}

// ld a, [wSRAMBank] / push af / ld a, bank / sramswitch
static void push_sram(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = bank;
	sramswitch(gb, bank);
}

// pop af / sramswitch
static void pop_sram(GB *gb)
{
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

// copies SRAM 2 a000-afff to b000-bfff and SRAM 1 to SRAM 3, then stores
// the backup SRAM checksum with its echoes
PORT(Func_1f14c6)
{
	save_marker(gb, 0x70);
	push_sram(gb, 0x02);
	SET_HL(gb, s2a000);
	SET_DE(gb, s2b000);
	SET_BC(gb, 0x1000);
	CALL(CopyHLToDE_BC);
	pop_sram(gb);

	SET_HL(gb, s0a000);
	SET_BC(gb, 0x2000);
	do {
		push_sram(gb, 0x01);
		gb->cpu.d = rd8(gb, REG_HL(gb));
		push_sram(gb, 0x03);
		gb->cpu.a = gb->cpu.d;
		uint16_t hl = REG_HL(gb);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl + 1);
		pop_sram(gb);
		pop_sram(gb);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.c)
			continue;
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b || gb->cpu.c);

	save_marker(gb, 0x71);
	gb_call(gb, ADDR_CalculateBackupSRAMChecksum2);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0a79f + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0a79f + 1, gb->cpu.a);
	save_marker(gb, 0x72);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0a7ef + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0a7ef + 1, gb->cpu.a);
	save_marker(gb, 0x73);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0afaf + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0afaf + 1, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	save_marker(gb, 0);
}

// push_wram bank: ldh a, [rWBK] / push af / ld a, bank / ldh [rWBK], a
static void push_wram(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = bank;
	wr8(gb, rWBK, bank);
}

// pop_wram: pop af / ldh [rWBK], a
static void pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// Func_1f0d60.GetMatchingChecksum
// bc = checksum, de = echo 1, hl = echo 2; sets wChecksum (best option),
// wcee9, wceeb and wChecksumMismatches
static void get_matching_checksum(GB *gb)
{
	uint8_t b = gb->cpu.b, c = gb->cpu.c, d = gb->cpu.d, e = gb->cpu.e;
	uint8_t h = gb->cpu.h, l = gb->cpu.l;
	enum { M_NONE, M_ECHO1, M_ECHO2, M_ECHOS, M_ALL } r;

	gb->cpu.a = h;
	alu_cp(gb, h, b);
	if (gb->cpu.f & FLAG_Z) {
		alu_cp(gb, h, d);
		if (gb->cpu.f & FLAG_Z) {
			// .all_high_bytes_equal
			gb->cpu.a = l;
			alu_cp(gb, l, c);
			if (gb->cpu.f & FLAG_Z) {
				alu_cp(gb, l, e);
				r = (gb->cpu.f & FLAG_Z) ? M_ALL : M_ECHO2;
			} else {
				// .check_echos_equality
				alu_cp(gb, l, e);
				if (gb->cpu.f & FLAG_Z) {
					r = M_ECHOS;
				} else {
					gb->cpu.a = c;
					alu_cp(gb, c, e);
					r = (gb->cpu.f & FLAG_Z) ? M_ECHO1 : M_NONE;
				}
			}
		} else {
			gb->cpu.a = c;
			alu_cp(gb, c, l);
			r = (gb->cpu.f & FLAG_Z) ? M_ECHO2 : M_NONE;
		}
	} else {
		// .echo2_not_equal
		alu_cp(gb, h, d);
		if (gb->cpu.f & FLAG_Z) {
			// .check_lower_echos
			gb->cpu.a = e;
			alu_cp(gb, e, l);
			r = (gb->cpu.f & FLAG_Z) ? M_ECHOS : M_NONE;
		} else {
			gb->cpu.a = b;
			alu_cp(gb, b, d);
			if (gb->cpu.f & FLAG_Z) {
				// .check_lower_echo1
				gb->cpu.a = c;
				alu_cp(gb, c, e);
				r = (gb->cpu.f & FLAG_Z) ? M_ECHO1 : M_NONE;
			} else {
				r = M_NONE;
			}
		}
	}

	switch (r) {
	case M_NONE:
		wr8(gb, wChecksumMismatches, MISMATCH_ORIG | MISMATCH_ECHO1 | MISMATCH_ECHO2);
		wr8(gb, wChecksum + 0, b);
		wr8(gb, wcee9 + 0, d);
		wr8(gb, wceeb + 0, h);
		wr8(gb, wChecksum + 1, c);
		wr8(gb, wcee9 + 1, e);
		wr8(gb, wceeb + 1, l);
		gb->cpu.a = l;
		break;
	case M_ECHO1:
		wr8(gb, wChecksumMismatches, MISMATCH_ECHO2);
		wr8(gb, wChecksum + 0, b);
		wr8(gb, wceeb + 0, b);
		wr8(gb, wcee9 + 0, h);
		wr8(gb, wChecksum + 1, c);
		wr8(gb, wceeb + 1, c);
		wr8(gb, wcee9 + 1, l);
		gb->cpu.a = l;
		break;
	case M_ECHO2:
		wr8(gb, wChecksumMismatches, MISMATCH_ECHO1);
		wr8(gb, wChecksum + 0, b);
		wr8(gb, wceeb + 0, b);
		wr8(gb, wcee9 + 0, d);
		wr8(gb, wChecksum + 1, c);
		wr8(gb, wceeb + 1, c);
		wr8(gb, wcee9 + 1, e);
		gb->cpu.a = e;
		break;
	case M_ECHOS:
		wr8(gb, wChecksumMismatches, MISMATCH_ORIG);
		wr8(gb, wChecksum + 0, d);
		wr8(gb, wceeb + 0, d);
		wr8(gb, wcee9 + 0, b);
		wr8(gb, wChecksum + 1, e);
		wr8(gb, wceeb + 1, e);
		wr8(gb, wcee9 + 1, c);
		gb->cpu.a = c;
		break;
	case M_ALL:
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wChecksumMismatches, 0);
		wr8(gb, wChecksum + 0, b);
		wr8(gb, wcee9 + 0, b);
		wr8(gb, wceeb + 0, b);
		wr8(gb, wChecksum + 1, c);
		wr8(gb, wcee9 + 1, c);
		wr8(gb, wceeb + 1, c);
		gb->cpu.a = c;
		break;
	}
}

// Loads bc, de and hl from the checksum at p0 and its echoes at p1 / p2,
// then call .GetMatchingChecksum.
static void load_checksums(GB *gb, uint16_t p0, uint16_t p1, uint16_t p2)
{
	gb->cpu.b = rd8(gb, p0);
	gb->cpu.c = rd8(gb, p0 + 1);
	gb->cpu.d = rd8(gb, p1);
	gb->cpu.e = rd8(gb, p1 + 1);
	gb->cpu.h = rd8(gb, p2);
	gb->cpu.l = gb->cpu.a = rd8(gb, p2 + 1);
	get_matching_checksum(gb);
}

// Func_1f0d60.CalculateChecksumAtHL
static void calculate_checksum_at_hl(GB *gb)
{
	SET_DE(gb, 0);
	gb->cpu.b = 0x5b; // SIZEOF("Progress WRAM")
	gb_call(gb, ADDR_CalculateChecksum);
	push_wram(gb, 0x02);
	gb->cpu.b = 0x11;
	gb_call(gb, ADDR_CalculateChecksum);
	pop_wram(gb);
}

// Whether the calculated checksum in de matches wChecksum, or (with
// mismatches) wcee9, or (with all three mismatched) wceeb.
static bool checksum_ok(GB *gb)
{
	gb->cpu.a = rd8(gb, wChecksum + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wChecksum + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (gb->cpu.f & FLAG_Z)
			return true;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wChecksumMismatches), rd8(gb, wChecksumMismatches));
	if (!gb->cpu.a)
		return false;
	gb->cpu.a = rd8(gb, wcee9 + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wcee9 + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (gb->cpu.f & FLAG_Z)
			return true;
	}
	gb->cpu.a = rd8(gb, wChecksumMismatches);
	alu_cp(gb, gb->cpu.a, MISMATCH_ORIG | MISMATCH_ECHO1 | MISMATCH_ECHO2);
	if (!(gb->cpu.f & FLAG_Z))
		return false;
	gb->cpu.a = rd8(gb, wceeb + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (!(gb->cpu.f & FLAG_Z))
		return false;
	gb->cpu.a = rd8(gb, wceeb + 1);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	return (gb->cpu.f & FLAG_Z) != 0;
}

// ld hl, wceee / set n, [hl]
static void set_wceee(GB *gb, int n)
{
	SET_HL(gb, wceee);
	wr8(gb, wceee, (uint8_t)(rd8(gb, wceee) | 1 << n));
}

// xor a / ld hl, addr / ld [hli], a (x4)
static void clear_check_vals(GB *gb, uint16_t addr)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, addr + i, 0);
	SET_HL(gb, addr + 4);
}

// marker, clear the check values at addr, then xor a and a 0 marker
static void clear_save(GB *gb, uint8_t marker, uint16_t addr)
{
	save_marker(gb, marker);
	clear_check_vals(gb, addr);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	save_marker(gb, 0);
}

PORT(Func_1f0d60)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wceee, 0);

	// game data
	SET_HL(gb, sCheckVals);
	gb_call(gb, ADDR_CheckSaveVals1);
	bool ok = false;
	if (!(gb->cpu.f & FLAG_C)) {
		load_checksums(gb, sGameDataChecksum, sGameDataChecksumEcho1, sGameDataChecksumEcho2);
		SET_HL(gb, sGameData);
		calculate_checksum_at_hl(gb);
		ok = checksum_ok(gb);
	}
	if (ok)
		set_wceee(gb, 0);
	else
		clear_save(gb, 0x10, sCheckVals); // .clear_check_vals

	// .check_backup_check_vals
	SET_HL(gb, sBackupCheckVals);
	gb_call(gb, ADDR_CheckSaveVals1);
	ok = false;
	if (!(gb->cpu.f & FLAG_C)) {
		load_checksums(gb, sBackupGameDataChecksum, sBackupGameDataChecksumEcho1, sBackupGameDataChecksumEcho2);
		SET_HL(gb, sBackupGameData);
		calculate_checksum_at_hl(gb);
		ok = checksum_ok(gb);
	}
	if (ok)
		set_wceee(gb, 1);
	else
		clear_save(gb, 0x20, sBackupCheckVals); // .clear_backup_check_vals

	// .asm_1f0e6e: backup SRAM 1
	load_checksums(gb, s0a79d, s0a7ed, s0afad);
	gb_call(gb, ADDR_CalculateBackupSRAMChecksum1);
	bool wram_ok = false;
	if (checksum_ok(gb)) {
		// .asm_1f0ec1
		set_wceee(gb, 6);
		SET_HL(gb, s0a000);
		gb_call(gb, ADDR_CheckSaveVals2);
		ok = false;
		if (!(gb->cpu.f & FLAG_C)) {
			load_checksums(gb, sWRAMChecksum, sWRAMChecksumEcho1, sWRAMChecksumEcho2);
			SET_HL(gb, sSaveCounter);
			gb_call(gb, ADDR_Func_1f0b5b);
			ok = checksum_ok(gb);
		}
		if (ok) {
			set_wceee(gb, 2); // .asm_1f0f27
			wram_ok = true;
		} else {
			// .asm_1f0f2f
			clear_save(gb, 0x30, s0a000);
			SET_HL(gb, s0a800);
			gb_call(gb, ADDR_CheckSaveVals2);
			if (!(gb->cpu.f & FLAG_C)) {
				load_checksums(gb, sBackup1WRAMChecksum, sBackup1WRAMChecksumEcho1, sBackup1WRAMChecksumEcho2);
				SET_HL(gb, s0a804);
				gb_call(gb, ADDR_Func_1f0b5b);
				if (checksum_ok(gb)) {
					set_wceee(gb, 3); // .asm_1f0faa
					wram_ok = true;
				}
			}
		}
	}
	if (!wram_ok) {
		// .asm_1f0fb2
		save_marker(gb, 0x30);
		clear_check_vals(gb, s0a000);
		save_marker(gb, 0x50);
		clear_check_vals(gb, s0a800);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		save_marker(gb, 0);
	}

	// .asm_1f0fe2: backup SRAM 2
	load_checksums(gb, s0a79f, s0a7ef, s0afaf);
	gb_call(gb, ADDR_CalculateBackupSRAMChecksum2);
	if (checksum_ok(gb)) {
		// .asm_1f1035
		set_wceee(gb, 7);
		SET_HL(gb, s0a400);
		gb_call(gb, ADDR_CheckSaveVals2);
		ok = false;
		if (!(gb->cpu.f & FLAG_C)) {
			load_checksums(gb, sBackup2WRAMChecksum, sBackup2WRAMChecksumEcho1, sBackup2WRAMChecksumEcho2);
			SET_HL(gb, s0a404);
			gb_call(gb, ADDR_Func_1f0b5b);
			ok = checksum_ok(gb);
		}
		if (ok) {
			set_wceee(gb, 4); // .asm_1f109b
			return;
		}
		// .asm_1f10a1
		clear_save(gb, 0x60, s0a400);
		SET_HL(gb, s0ac00);
		gb_call(gb, ADDR_CheckSaveVals2);
		if (!(gb->cpu.f & FLAG_C)) {
			load_checksums(gb, sBackup3WRAMChecksum, sBackup3WRAMChecksumEcho1, sBackup3WRAMChecksumEcho2);
			SET_HL(gb, s0ac04);
			gb_call(gb, ADDR_Func_1f0b5b);
			if (checksum_ok(gb)) {
				set_wceee(gb, 5); // .asm_1f111c
				return;
			}
		}
	}
	// .asm_1f1122
	save_marker(gb, 0x60);
	clear_check_vals(gb, s0a400);
	save_marker(gb, 0x80);
	clear_check_vals(gb, s0ac00);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	save_marker(gb, 0);
}

// rrca
static uint8_t alu_rrca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 1) ? FLAG_C : 0;
	return (uint8_t)(v >> 1 | v << 7);
}

// ld bc, $600 / .loop: rrca / jr nc, :+ / inc c / : dec b / jr nz, .loop /
// ld a, c / dec a: returns whether exactly one of a's low 6 bits is set
static bool one_bit_set(GB *gb)
{
	SET_BC(gb, 0x600);
	do {
		gb->cpu.a = alu_rrca(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_C)
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	gb->cpu.a = alu_dec(gb, gb->cpu.c);
	return gb->cpu.a == 0;
}

// Func_1f1246.Func_1f13d7
static void restore_game_data(GB *gb)
{
	SET_DE(gb, 0xca00); // STARTOF("Progress WRAM")
	gb->cpu.b = 0x5b;
	CALL(CopyHLToDE);
	push_wram(gb, 0x02);
	SET_DE(gb, 0xd000);
	gb->cpu.b = 0x11;
	CALL(CopyHLToDE);
	pop_wram(gb);
}

// Func_1f1246.Func_1f13f2
static void restore_wram(GB *gb)
{
	SET_DE(gb, wSaveCounter);
	gb->cpu.b = 0xca;
	CALL(CopyHLToDE);
	push_wram(gb, 0x02);
	SET_DE(gb, 0xd000);
	gb->cpu.b = 0x11;
	CALL(CopyHLToDE);
	pop_wram(gb);
	push_wram(gb, 0x01); // BANK("Level Objects WRAM")
	SET_DE(gb, 0xd000);  // STARTOF("Level Objects WRAM")
	SET_BC(gb, 0x014a);  // SIZEOF("Level Objects WRAM")
	CALL(CopyHLToDE_BC);
	pop_wram(gb);
}

// Func_1f1246.Func_1f1420: restores the backup SRAM (SRAM2 b000 to a000,
// SRAM3 to SRAM1) and stores its checksum with the echoes
static void restore_backup_sram(GB *gb)
{
	save_marker(gb, 0x40);
	push_sram(gb, 0x02);
	SET_HL(gb, s2b000);
	SET_DE(gb, s2a000);
	SET_BC(gb, 0x1000);
	CALL(CopyHLToDE_BC);
	pop_sram(gb);

	// copy whole SRAM3 to SRAM1
	SET_HL(gb, s3a000);
	SET_BC(gb, 0x2000);
	do {
		push_sram(gb, 0x03);
		gb->cpu.d = rd8(gb, REG_HL(gb));
		push_sram(gb, 0x01);
		gb->cpu.a = gb->cpu.d;
		uint16_t hl = REG_HL(gb);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl + 1);
		pop_sram(gb);
		pop_sram(gb);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.c)
			continue;
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b || gb->cpu.c);

	save_marker(gb, 0x41);
	gb_call(gb, ADDR_CalculateBackupSRAMChecksum1);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0a79d + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0a79d + 1, gb->cpu.a);
	save_marker(gb, 0x42);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0a7ed + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0a7ed + 1, gb->cpu.a);
	save_marker(gb, 0x43);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, s0afad + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, s0afad + 1, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	save_marker(gb, 0);
}

PORT(Func_1f1246)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wceef, 0);
	gb->cpu.a = alu_and(gb, rd8(gb, wceee), 0x3f);
	bool reset_scores = false;
	if (!gb->cpu.a) {
		reset_scores = true;
	} else {
		// .asm_1f1266
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_and(gb, 0x3f, gb->cpu.b);
		wr8(gb, wceef, gb->cpu.a);
		if (!one_bit_set(gb)) {
			static const uint16_t srcs[6] = { sGameData, sBackupGameData, sSaveCounter, s0a804, s0a404, s0ac04 };
			for (int i = 0; i < 6; i++) {
				SET_HL(gb, srcs[i]);
				SET_DE(gb, s0afe0 + 4 * i);
				gb->cpu.b = 4;
				CALL(CopyHLToDE);
			}
			gb->cpu.e = 0x21;
			for (;;) {
				// .asm_1f12c0
				gb->cpu.e = alu_dec(gb, gb->cpu.e);
				if (!gb->cpu.e)
					break;
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, s0aff8, 0);
				for (int k = 0; k < 6; k++) {
					gb->cpu.a = rd8(gb, wceef);
					if (!alu_bit(gb, gb->cpu.a, k))
						continue;
					uint16_t hl = (uint16_t)(s0afe0 + 4 * k + 3);
					wr8(gb, hl, alu_sla(gb, rd8(gb, hl)));
					for (int j = 0; j < 3; j++) {
						hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
						wr8(gb, hl, alu_rl(gb, rd8(gb, hl)));
					}
					SET_HL(gb, hl);
					if (gb->cpu.f & FLAG_C) {
						SET_HL(gb, s0aff8);
						wr8(gb, s0aff8, (uint8_t)(rd8(gb, s0aff8) | 1 << k));
					}
				}
				// .asm_1f1370
				gb->cpu.a = alu_and(gb, rd8(gb, s0aff8), rd8(gb, s0aff8));
				if (!gb->cpu.a)
					continue;
				wr8(gb, wceef, gb->cpu.a);
				if (one_bit_set(gb))
					break;
			}
		}
		// .asm_1f1389
		gb->cpu.a = rd8(gb, wceef);
		int k;
		for (k = 0; k < 6; k++)
			if (alu_bit(gb, gb->cpu.a, k))
				break;
		switch (k) {
		case 0:
			SET_HL(gb, sGameData);
			restore_game_data(gb);
			return;
		case 1:
			SET_HL(gb, sBackupGameData);
			restore_game_data(gb);
			return;
		case 2:
			SET_HL(gb, sSaveCounter);
			restore_wram(gb);
			return;
		case 3:
			SET_HL(gb, s0a804);
			restore_wram(gb);
			return;
		case 4:
			SET_HL(gb, s0a404);
			restore_wram(gb);
			restore_backup_sram(gb);
			return;
		case 5:
			SET_HL(gb, s0ac04);
			restore_wram(gb);
			restore_backup_sram(gb);
			return;
		default:
			reset_scores = true;
			break;
		}
	}
	if (reset_scores) {
		// .asm_1f1251
		SET_HL(gb, wLevelTimeAttackScores);
		gb->cpu.b = 2 * NUM_LEVELS;
		gb->cpu.a = (10 << 4) | 10;
		CALL(WriteAToHL_BTimes);
		SET_HL(gb, wGolfBestScores);
		gb->cpu.b = 0x4;
		gb->cpu.a = (10 << 4) | 10;
		CALL(WriteAToHL_BTimes);
	}
}
