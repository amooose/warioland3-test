// The save's checksums (src/engine/pause/save/checksum.asm) and checking
// the save on boot (validate_save.asm); see pause.hpp.
#include "pause.hpp"

#include <bit>

namespace wl3::pause {

namespace {

constexpr uint16_t PROGRESS_WRAM = 0xca00; // STARTOF("Progress WRAM")
constexpr uint8_t PROGRESS_WRAM_SIZE = 0x5b;
constexpr uint8_t PROGRESS_AND_LEVEL_WRAM_SIZE = 0xca; // with "Level WRAM"
constexpr uint16_t WRAMX = 0xd000;
constexpr uint8_t TREASURES_SIZE = static_cast<uint8_t>((wOWLevel - wTreasuresCollected) + 2);
constexpr uint8_t LEVEL_OBJECTS_WRAM_BANK = 0x01;
constexpr uint16_t LEVEL_OBJECTS_WRAM_SIZE = 0x14a;
constexpr uint16_t SRAM_SIZE = 0x2000;

constexpr uint8_t CHECK_VALS1[4] = {CHECKVAL1_1, CHECKVAL1_2, CHECKVAL1_3, CHECKVAL1_4};
constexpr uint8_t CHECK_VALS2[4] = {CHECKVAL2_1, CHECKVAL2_2, CHECKVAL2_3, CHECKVAL2_4};

// Where each backup SRAM checksum goes, with its two echoes.
constexpr uint16_t BACKUP_SRAM1_SUMS[3] = {s0a79d, s0a7ed, s0afad};
constexpr uint16_t BACKUP_SRAM2_SUMS[3] = {s0a79f, s0a7ef, s0afaf};

} // namespace

// ---- checksum.asm ----

// CalculateChecksum (and the loop over bc bytes in the backup SRAM
// checksums): count bytes from addr
uint16_t SaveData::sum(uint16_t sum, uint16_t &addr, uint16_t count)
{
	while (count--)
		sum = static_cast<uint16_t>(sum + mem_[addr++]);
	return sum;
}

// CalculateChecksumLong. bug: after the first byte it goes on into
// CalculateChecksum, which counts only the count's high byte (b, where 0
// is 256).
uint16_t SaveData::sum_long(uint16_t total, uint16_t &addr, uint16_t count)
{
	total = static_cast<uint16_t>(total + mem_[addr++]);
	count = static_cast<uint16_t>(count - 1);
	if (!count)
		return total;
	uint8_t b = count >> 8;
	return sum(total, addr, b ? b : 0x100);
}

void SaveData::increment_save_counter()
{
	for (int i = 3; i >= 0; i--)
		if (++mem_[static_cast<uint16_t>(wSaveCounter + i)])
			break;
}

// Func_1f0b5b: a level save (as CalculateWRAMDataChecksum sums WRAM) at addr
uint16_t SaveData::level_data_checksum(uint16_t addr)
{
	uint16_t total = sum(0, addr, PROGRESS_AND_LEVEL_WRAM_SIZE);
	uint8_t wram_bank = mem_[rWBK];
	switch_wram(0x02);
	total = sum(total, addr, 0x11);
	switch_wram(wram_bank);
	wram_bank = mem_[rWBK];
	switch_wram(BANK_wObjects);
	total = sum_long(total, addr, LEVEL_OBJECTS_WRAM_SIZE);
	switch_wram(wram_bank);
	return total;
}

uint16_t SaveData::wram_data_checksum()
{
	uint16_t addr = PROGRESS_WRAM;
	uint16_t total = sum(0, addr, PROGRESS_AND_LEVEL_WRAM_SIZE);
	uint8_t wram_bank = mem_[rWBK];
	switch_wram(BANK_wTreasuresCollected);
	addr = wTreasuresCollected;
	total = sum(total, addr, TREASURES_SIZE);
	switch_wram(wram_bank);
	wram_bank = mem_[rWBK];
	switch_wram(BANK_wObjects);
	addr = wObjects;
	total = sum_long(total, addr, LEVEL_OBJECTS_WRAM_SIZE);
	switch_wram(wram_bank);
	return total;
}

// the whole of an SRAM bank, then half of SRAM bank 2
uint16_t SaveData::backup_sram_checksum(uint8_t bank, uint16_t start, uint16_t sram2)
{
	uint8_t sram_bank = wram_.sram_bank();
	switch_sram(bank);
	uint16_t total = sum(0, start, SRAM_SIZE);
	switch_sram(sram_bank);
	switch_sram(BANK_s2a000);
	total = sum(total, sram2, SRAM_SIZE / 2);
	switch_sram(sram_bank);
	return total;
}

uint16_t SaveData::backup_sram_checksum1()
{
	return backup_sram_checksum(BANK_sLevelBlockObjectMap, sLevelBlockObjectMap, s2a000);
}

uint16_t SaveData::backup_sram_checksum2()
{
	return backup_sram_checksum(BANK_s3a000, s3a000, s2b000);
}

// ---- validate_save.asm ----

bool SaveData::check_vals(uint16_t addr, const uint8_t (&vals)[4]) const
{
	for (int i = 0; i < 4; i++)
		if (mem_[static_cast<uint16_t>(addr + i)] != vals[i])
			return false;
	return true;
}

uint16_t SaveData::game_data_checksum()
{
	uint16_t addr = PROGRESS_WRAM;
	uint16_t total = sum(0, addr, PROGRESS_WRAM_SIZE);
	uint8_t wram_bank = mem_[rWBK];
	switch_wram(BANK_wTreasuresCollected);
	addr = wTreasuresCollected;
	total = sum(total, addr, TREASURES_SIZE);
	switch_wram(wram_bank);
	return total;
}

// the game data saved at addr
uint16_t SaveData::game_data_checksum_at(uint16_t addr)
{
	uint16_t total = sum(0, addr, PROGRESS_WRAM_SIZE);
	uint8_t wram_bank = mem_[rWBK];
	switch_wram(0x02);
	total = sum(total, addr, 0x11);
	switch_wram(wram_bank);
	return total;
}

// From a saved checksum and its two echoes, the one to trust: wChecksum
// gets the best, wcee9 and wceeb the next ones to try, and
// wChecksumMismatches which differ.
void SaveData::match_checksums(uint16_t sum, uint16_t echo1, uint16_t echo2)
{
	const uint8_t b = sum >> 8, c = sum & 0xff;
	const uint8_t d = echo1 >> 8, e = echo1 & 0xff;
	const uint8_t h = echo2 >> 8, l = echo2 & 0xff;
	enum { NO_MATCH, SUM_ECHO1, SUM_ECHO2, ECHOS, ALL } match;
	if (h == b) {
		if (h == d)
			match = l == c ? (l == e ? ALL : SUM_ECHO2) : l == e ? ECHOS : c == e ? SUM_ECHO1 : NO_MATCH;
		else
			match = c == l ? SUM_ECHO2 : NO_MATCH;
	} else if (h == d) {
		match = e == l ? ECHOS : NO_MATCH;
	} else {
		match = b == d && c == e ? SUM_ECHO1 : NO_MATCH;
	}

	uint8_t mismatches;
	uint16_t best, next, last;
	switch (match) {
	case NO_MATCH:
		mismatches = MISMATCH_ORIG | MISMATCH_ECHO1 | MISMATCH_ECHO2;
		best = sum, next = echo1, last = echo2;
		break;
	case SUM_ECHO1: // the checksum and echo 1 agree
		mismatches = MISMATCH_ECHO2;
		best = sum, next = echo2, last = sum;
		break;
	case SUM_ECHO2: // the checksum and echo 2 agree
		mismatches = MISMATCH_ECHO1;
		best = sum, next = echo1, last = sum;
		break;
	case ECHOS: // only the echoes agree
		mismatches = MISMATCH_ORIG;
		best = echo1, next = sum, last = echo1;
		break;
	default:
		mismatches = 0;
		best = next = last = sum;
		break;
	}
	wram_.checksum_mismatches() = mismatches;
	write_sum(wChecksum, best);
	write_sum(wcee9, next);
	write_sum(wceeb, last);
}

// Whether a calculated checksum matches wChecksum, or (with a mismatch)
// wcee9, or (with all three mismatched) wceeb.
bool SaveData::checksum_ok(uint16_t sum) const
{
	if (read_sum(wChecksum) == sum)
		return true;
	uint8_t mismatches = wram_.checksum_mismatches();
	if (!mismatches)
		return false;
	if (read_sum(wcee9) == sum)
		return true;
	return mismatches == (MISMATCH_ORIG | MISMATCH_ECHO1 | MISMATCH_ECHO2) && read_sum(wceeb) == sum;
}

// Func_1f0d60: checks the game data, the level saves and the backup SRAM
// with their copies; wceee gets a bit for each that's good, and the check
// values of the bad ones are cleared.
void SaveData::check()
{
	auto good = wram_.unk_ceee();
	good = 0;

	// game data
	bool ok = check_vals(sCheckVals, CHECK_VALS1);
	if (ok) {
		match_checksums(read_sum(sGameDataChecksum), read_sum(sGameDataChecksumEcho1),
			read_sum(sGameDataChecksumEcho2));
		ok = checksum_ok(game_data_checksum_at(sGameData));
	}
	if (ok)
		good |= 1 << 0;
	else
		clear_save(0x10, sCheckVals);

	ok = check_vals(sBackupCheckVals, CHECK_VALS1);
	if (ok) {
		match_checksums(read_sum(sBackupGameDataChecksum), read_sum(sBackupGameDataChecksumEcho1),
			read_sum(sBackupGameDataChecksumEcho2));
		ok = checksum_ok(game_data_checksum_at(sBackupGameData));
	}
	if (ok)
		good |= 1 << 1;
	else
		clear_save(0x20, sBackupCheckVals);

	// backup SRAM 1, then the level save or its first backup
	match_checksums(read_sum(s0a79d), read_sum(s0a7ed), read_sum(s0afad));
	bool level_ok = false;
	if (checksum_ok(backup_sram_checksum1())) {
		good |= 1 << 6;
		ok = check_vals(s0a000, CHECK_VALS2);
		if (ok) {
			match_checksums(read_sum(sWRAMChecksum), read_sum(sWRAMChecksumEcho1),
				read_sum(sWRAMChecksumEcho2));
			ok = checksum_ok(level_data_checksum(sSaveCounter));
		}
		if (ok) {
			good |= 1 << 2;
			level_ok = true;
		} else {
			clear_save(0x30, s0a000);
			if (check_vals(s0a800, CHECK_VALS2)) {
				match_checksums(read_sum(sBackup1WRAMChecksum), read_sum(sBackup1WRAMChecksumEcho1),
					read_sum(sBackup1WRAMChecksumEcho2));
				if (checksum_ok(level_data_checksum(s0a804))) {
					good |= 1 << 3;
					level_ok = true;
				}
			}
		}
	}
	if (!level_ok) {
		mark(0x30);
		clear_check_vals(s0a000);
		mark(0x50);
		clear_check_vals(s0a800);
		mark(0);
	}

	// backup SRAM 2, then the level save's second or third backup
	match_checksums(read_sum(s0a79f), read_sum(s0a7ef), read_sum(s0afaf));
	if (checksum_ok(backup_sram_checksum2())) {
		good |= 1 << 7;
		ok = check_vals(s0a400, CHECK_VALS2);
		if (ok) {
			match_checksums(read_sum(sBackup2WRAMChecksum), read_sum(sBackup2WRAMChecksumEcho1),
				read_sum(sBackup2WRAMChecksumEcho2));
			ok = checksum_ok(level_data_checksum(s0a404));
		}
		if (ok) {
			good |= 1 << 4;
			return;
		}
		clear_save(0x60, s0a400);
		if (check_vals(s0ac00, CHECK_VALS2)) {
			match_checksums(read_sum(sBackup3WRAMChecksum), read_sum(sBackup3WRAMChecksumEcho1),
				read_sum(sBackup3WRAMChecksumEcho2));
			if (checksum_ok(level_data_checksum(s0ac04))) {
				good |= 1 << 5;
				return;
			}
		}
	}
	mark(0x60);
	clear_check_vals(s0a400);
	mark(0x80);
	clear_check_vals(s0ac00);
	mark(0);
}

// Func_1f1246: loads the newest of the good saves (the one whose save
// counter is highest), or with none, resets the best scores.
void SaveData::restore()
{
	wram_.unk_ceef() = 0;
	uint8_t good = wram_.unk_ceee() & 0x3f;
	if (good) {
		wram_.unk_ceef() = good;
		if (std::popcount(good) != 1) {
			// each one's save counter, compared a bit at a time from the
			// top: the counters with a 1 where another has a 0 stay
			static constexpr uint16_t COUNTERS[6] = {
				sGameData, sBackupGameData, sSaveCounter, s0a804, s0a404, s0ac04};
			for (int i = 0; i < 6; i++)
				copy(COUNTERS[i], static_cast<uint16_t>(s0afe0 + 4 * i), 4);
			for (int bit = 0; bit < 32; bit++) {
				mem_[s0aff8] = 0;
				for (int k = 0; k < 6; k++) {
					if (!(wram_.unk_ceef() & 1 << k))
						continue;
					uint16_t counter = static_cast<uint16_t>(s0afe0 + 4 * k);
					bool carry = false;
					for (int j = 3; j >= 0; j--) {
						uint8_t v = mem_[static_cast<uint16_t>(counter + j)];
						mem_[static_cast<uint16_t>(counter + j)] = static_cast<uint8_t>(v << 1 | carry);
						carry = v & 0x80;
					}
					if (carry)
						mem_[s0aff8] = static_cast<uint8_t>(mem_[s0aff8] | 1 << k);
				}
				uint8_t ones = mem_[s0aff8];
				if (!ones)
					continue;
				wram_.unk_ceef() = ones;
				if (std::popcount(static_cast<uint8_t>(ones & 0x3f)) == 1)
					break;
			}
		}
		uint8_t newest = wram_.unk_ceef();
		int k = 0;
		while (k < 6 && !(newest & 1 << k))
			k++;
		switch (k) {
		case 0:
			restore_game_data(sGameData);
			return;
		case 1:
			restore_game_data(sBackupGameData);
			return;
		case 2:
			restore_wram(sSaveCounter);
			return;
		case 3:
			restore_wram(s0a804);
			return;
		case 4:
			restore_wram(s0a404);
			restore_backup_sram();
			return;
		case 5:
			restore_wram(s0ac04);
			restore_backup_sram();
			return;
		}
	}
	for (int i = 0; i < 2 * NUM_LEVELS; i++)
		mem_[static_cast<uint16_t>(wLevelTimeAttackScores + i)] = (10 << 4) | 10;
	for (int i = 0; i < 4; i++)
		mem_[static_cast<uint16_t>(wGolfBestScores + i)] = (10 << 4) | 10;
}

void SaveData::restore_game_data(uint16_t src)
{
	copy(src, PROGRESS_WRAM, PROGRESS_WRAM_SIZE);
	uint8_t wram_bank = mem_[rWBK];
	switch_wram(0x02);
	copy(static_cast<uint16_t>(src + PROGRESS_WRAM_SIZE), WRAMX, 0x11);
	switch_wram(wram_bank);
}

void SaveData::restore_wram(uint16_t src)
{
	copy(src, wSaveCounter, PROGRESS_AND_LEVEL_WRAM_SIZE);
	src = static_cast<uint16_t>(src + PROGRESS_AND_LEVEL_WRAM_SIZE);
	uint8_t wram_bank = mem_[rWBK];
	switch_wram(0x02);
	copy(src, WRAMX, 0x11);
	switch_wram(wram_bank);
	src = static_cast<uint16_t>(src + 0x11);
	wram_bank = mem_[rWBK];
	switch_wram(LEVEL_OBJECTS_WRAM_BANK);
	copy(src, WRAMX, LEVEL_OBJECTS_WRAM_SIZE);
	switch_wram(wram_bank);
}

// Func_1f14c6: SRAM 2's first half to its second, and SRAM 1 to SRAM 3,
// with the copy's checksum
void SaveData::back_up_sram()
{
	mark(0x70);
	uint8_t sram_bank = wram_.sram_bank();
	switch_sram(0x02);
	copy(s2a000, s2b000, SRAM_SIZE / 2);
	switch_sram(sram_bank);
	copy_sram(0x01, 0x03);
	mark(0x71); // before the checksum, which may count it
	write_sums(0x71, BACKUP_SRAM2_SUMS, backup_sram_checksum2());
}

// Func_1f1246.Func_1f1420: the other way around
void SaveData::restore_backup_sram()
{
	mark(0x40);
	uint8_t sram_bank = wram_.sram_bank();
	switch_sram(0x02);
	copy(s2b000, s2a000, SRAM_SIZE / 2);
	switch_sram(sram_bank);
	copy_sram(0x03, 0x01);
	mark(0x41); // before the checksum, which may count it
	write_sums(0x41, BACKUP_SRAM1_SUMS, backup_sram_checksum1());
}

// a byte at a time, switching banks for each
void SaveData::copy_sram(uint8_t from, uint8_t to)
{
	uint8_t sram_bank = wram_.sram_bank();
	for (uint16_t addr = s0a000; addr < s0a000 + SRAM_SIZE; addr++) {
		switch_sram(from);
		uint8_t v = mem_[addr];
		switch_sram(to);
		mem_[addr] = v;
	}
	switch_sram(sram_bank);
}

void SaveData::copy(uint16_t src, uint16_t dest, uint16_t count)
{
	while (count--)
		mem_[dest++] = mem_[src++];
}

void SaveData::validate()
{
	uint8_t sram_bank = wram_.sram_bank();
	switch_sram(0x00); // BANK("SRAM0")

	// the save's step markers (left by a save cut short), if two agree
	bool found = check_vals(sCheckVals, CHECK_VALS1) || check_vals(s0a000, CHECK_VALS2)
		|| check_vals(s0a400, CHECK_VALS2) || check_vals(sBackupCheckVals, CHECK_VALS1)
		|| check_vals(s0a800, CHECK_VALS2) || check_vals(s0ac00, CHECK_VALS2);
	uint8_t step = 0xff;
	if (found) {
		uint8_t m1 = mem_[s0a790], m2 = mem_[s0a7e0], m3 = mem_[s0afa0];
		if (m3 == m1 || m3 == m2)
			step = m3;
		else if (m1 == m2)
			step = m1;
	}
	wram_.unk_cef1() = step;

	check();
	restore();
	switch_sram(sram_bank);
}

// ---- shared ----

// the save's step markers, which say how far a save got
void SaveData::mark(uint8_t step)
{
	mem_[s0a790] = step;
	mem_[s0a7e0] = step;
	mem_[s0afa0] = step;
}

void SaveData::clear_check_vals(uint16_t addr)
{
	for (int i = 0; i < 4; i++)
		mem_[static_cast<uint16_t>(addr + i)] = 0;
}

void SaveData::clear_save(uint8_t step, uint16_t addr)
{
	mark(step);
	clear_check_vals(addr);
	mark(0);
}

uint16_t SaveData::read_sum(uint16_t addr) const
{
	return static_cast<uint16_t>(mem_[addr] << 8 | mem_[static_cast<uint16_t>(addr + 1)]);
}

void SaveData::write_sum(uint16_t addr, uint16_t sum)
{
	mem_[addr] = static_cast<uint8_t>(sum >> 8);
	mem_[static_cast<uint16_t>(addr + 1)] = static_cast<uint8_t>(sum);
}

// the checksum (its step already marked) and its two echoes, each after
// its step marker
void SaveData::write_sums(uint8_t step, const uint16_t (&dest)[3], uint16_t sum)
{
	for (int i = 0; i < 3; i++) {
		if (i)
			mark(static_cast<uint8_t>(step + i));
		write_sum(dest[i], sum);
	}
	mark(0);
}

void SaveData::switch_sram(uint8_t bank)
{
	wram_.sram_bank() = bank;
	mem_[rRAMB + 0x100] = bank;
}

} // namespace wl3::pause

// Its caller (Start) reads no register after it: ei, then hCGB, then
// InitGameState, FillWhiteBGPal and InitAudio, which set what they use.
PORT_EX(ValidateSaveData, 0, ALL_REGISTERS)
{
	wl3::pause::SaveData(gb).validate();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(CalculateChecksum);
PORT_COVERED(CalculateChecksumLong);
PORT_COVERED(IncrementSaveCounter);
PORT_COVERED(Func_1f0b5b);
PORT_COVERED(CalculateWRAMDataChecksum);
PORT_COVERED(CalculateBackupSRAMChecksum1);
PORT_COVERED(CalculateBackupSRAMChecksum2);
PORT_COVERED(CheckSaveVals1);
PORT_COVERED(CheckSaveVals2);
PORT_COVERED(CalculateGameDataChecksum);
PORT_COVERED(Func_1f14c6);
PORT_COVERED(Func_1f0d60);
PORT_COVERED(Func_1f1246);
