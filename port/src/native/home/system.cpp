// Home's input and system routines; see system.hpp.
#include "system.hpp"

#include "memory.hpp"

#include <initializer_list>

namespace wl3::home {

namespace {

constexpr uint16_t HCALL_FUNC = ADDR_InitHRAMCallFunc + 0x0c; // .Func
constexpr uint8_t HCALL_FUNC_SIZE = 0x17; // .end - .Func

} // namespace

void update_joypad(GB *gb)
{
	Memory mem(gb);
	mem[rJOYP] = JOYP_GET_CTRL_PAD;
	uint8_t dpad = static_cast<uint8_t>(~mem[rJOYP] & 0x0f);
	mem[rJOYP] = JOYP_GET_BUTTONS;
	uint8_t buttons = static_cast<uint8_t>(~mem[rJOYP] & 0x0f);
	uint8_t down = static_cast<uint8_t>(dpad << 4 | buttons);
	mem[wJoypadPressed] = static_cast<uint8_t>((mem[wJoypadDown] ^ down) & down);
	mem[wJoypadDown] = down;
	mem[rJOYP] = JOYP_GET_NONE;
}

uint8_t process_dpad_repeat(GB *gb)
{
	Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = BANK_wDPadRepeatCounter;
	auto counter = mem[wDPadRepeatCounter];
	uint8_t added = 0;
	uint8_t down = mem[wJoypadDown] & PAD_CTRL_PAD;
	if (!counter || (mem[wJoypadPressed] & PAD_CTRL_PAD) || !down) {
		counter = 0x10; // a new press: the first repeat's delay
	} else if (!--counter) {
		mem[wJoypadPressed] = static_cast<uint8_t>(mem[wJoypadPressed] | down);
		added = down;
		counter = 0x08;
	}
	mem[rWBK] = wram_bank;
	return added;
}

bool enable_double_speed(GB *gb)
{
	Memory mem(gb);
	if (mem[rSPD] & 0x80)
		return false;
	mem[rSPD] = SPD_PREPARE;
	uint8_t ie = mem[rIE];
	mem[rIE] = 0;
	mem[rJOYP] = JOYP_GET_NONE;
	// stop: with the switch armed, it changes the CPU speed
	if (gb->cpu.speed_switch_armed) {
		gb->cpu.double_speed = !gb->cpu.double_speed;
		gb->cpu.speed_switch_armed = false;
	}
	gb->div_counter = 0;
	while (!(mem[rSPD] & 0x80)) {
	}
	mem[rJOYP] = 0;
	mem[rIF] = 0;
	mem[rIE] = ie;
	return true;
}

void init_hram_call_func(GB *gb)
{
	copy(gb, HCALL_FUNC, hCallFunc, HCALL_FUNC_SIZE);
}

void clear_save_data(GB *gb)
{
	for (uint16_t vals : {sCheckVals, sBackupCheckVals, s0a000, s0a800, s0a400, s0ac00})
		fill(gb, vals, 0, 8);
	Memory(gb).byte(hUnused_fffd) = 0;
}

} // namespace wl3::home

using namespace wl3::home;

PORT_EX(UpdateJoypad, 0, 0)
{
	update_joypad(gb);
	wl3::Memory mem(gb);
	gb->cpu.c = mem[wJoypadDown];
	gb->cpu.b = gb->cpu.c & 0xf0;
	gb->cpu.a = JOYP_GET_NONE;
	gb->cpu.f = static_cast<uint8_t>(FLAG_H | (mem[wJoypadPressed] ? 0 : FLAG_Z)); // and c
}

// a and f are popped by pop_wram (the WRAM bank it read, the entry's flags)
PORT_EX(ProcessDPadRepeat, 0, 0)
{
	uint8_t wram_bank = wl3::Memory(gb)[rWBK];
	if (uint8_t added = process_dpad_repeat(gb))
		gb->cpu.b = added;
	SET_HL(gb, wDPadRepeatCounter);
	gb->cpu.a = wram_bank;
}

PORT_EX(EnableDoubleSpeed, 0, 0)
{
	uint8_t carry = gb->cpu.f & FLAG_C;
	uint8_t spd = wl3::Memory(gb)[rSPD];
	uint8_t ie = wl3::Memory(gb)[rIE];
	if (!enable_double_speed(gb)) {
		gb->cpu.a = spd;
		gb->cpu.f = static_cast<uint8_t>(carry | FLAG_H); // bit 7, set
		return;
	}
	gb->cpu.a = ie; // pop af: with the flags of bit 7 (clear)
	gb->cpu.f = static_cast<uint8_t>(carry | FLAG_Z | FLAG_H);
}

PORT_EX(InitHRAMCallFunc, 0, 0)
{
	init_hram_call_func(gb);
	SET_HL(gb, HCALL_FUNC + HCALL_FUNC_SIZE);
	SET_DE(gb, hCallFunc + HCALL_FUNC_SIZE);
	gb->cpu.a = wl3::Memory(gb)[hCallFunc + HCALL_FUNC_SIZE - 1];
	gb->cpu.b = 0;
	gb->cpu.f = static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

// then on to Init
PORT_EX(ClearSaveData, HOOK_MANUAL_RET, 0)
{
	clear_save_data(gb);
	gb->cpu.a = 0;
	SET_HL(gb, s0ac00 + 8);
	gb->cpu.b = 0;
	gb->cpu.f = FLAG_Z | FLAG_N; // the last fill's dec b, after xor a
	gb->cpu.pc = ADDR_Init;
}
