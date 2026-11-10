// home/header (src/home/header.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeHeader : public AsmModule {
public:
	using AsmModule::AsmModule;

	// rst $28: jumps to entry a of the table that follows the rst
	void port_JumpTable()
	{
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		uint16_t hl = gb_pop16(gb_);
		cpu_.e = cpu_.a;
		cpu_.d = 0x00;
		hl = alu_.add16(hl, REG_DE(gb_));
		cpu_.a = rd8(hl);
		cpu_.h = rd8((uint16_t)(hl + 1));
		cpu_.l = cpu_.a;
		JUMP_TO(REG_HL(gb_));
	}

	// The V-Blank interrupt handler: runs the function in wVBlankFunc when it's
	// enabled, keeping the ROM, SRAM, WRAM and VRAM banks it may switch
	void port_VBlank()
	{
		push_af();
		gb_push16(gb_, REG_BC(gb_));
		gb_push16(gb_, REG_DE(gb_));
		gb_push16(gb_, REG_HL(gb_));
		cpu_.a = rd8(wEnableVBlankFunc);
		alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.a) {
			cpu_.a = rd8(wROMBank);
			push_af();
			cpu_.a = rd8(wSRAMBank);
			push_af();
			cpu_.a = rd8(rWBK);
			push_af();
			cpu_.a = rd8(rVBK);
			push_af();
			// The pushes stay: the function runs under them, 16 bytes lower
			// on the stack, and code that reads below sp sees them
			if (!ram_run_vblank_func(gb_))
				gb_call_ret(gb_, wVBlankFunc, ADDR_VBlank + 0x1b);
			pop_af();
			wr8(rVBK, cpu_.a);
			pop_af();
			wr8(rWBK, cpu_.a);
			pop_af();
			sramswitch(cpu_.a);
			pop_af();
			bankswitch(cpu_.a);
			cpu_.a = TRUE;
			wr8(wVBlankFuncExecuted, cpu_.a);
		}
		uint16_t v = gb_pop16(gb_);
		SET_HL(gb_, v);
		v = gb_pop16(gb_);
		SET_DE(gb_, v);
		v = gb_pop16(gb_);
		SET_BC(gb_, v);
		pop_af();
		// reti
		RET();
		cpu_.ime = true;
	}

	// Where the boot ROM hands over: nop / jp Start
	void port_EntryPoint() { JUMP_TO(ADDR_Start); }

	// The interrupt vectors
	void port_VBlankVector() { JUMP_TO(ADDR_VBlank); }
	void port_LCDVector() { JUMP_TO(wLCDFunc); }

	// reti
	void vector_reti()
	{
		RET();
		cpu_.ime = true;
	}

	void port_TimerVector() { vector_reti(); }
	void port_SerialVector() { vector_reti(); }
	void port_JoypadVector() { vector_reti(); }
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeHeader;

PORT_EX(JumpTable, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_JumpTable(); }
PORT_EX(VBlank, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_VBlank(); }
PORT_EX(EntryPoint, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_EntryPoint(); }
PORT_EX(VBlankVector, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_VBlankVector(); }
PORT_EX(LCDVector, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_LCDVector(); }
PORT_EX(TimerVector, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_TimerVector(); }
PORT_EX(SerialVector, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_SerialVector(); }
PORT_EX(JoypadVector, HOOK_MANUAL_RET, 0) { HomeHeader(gb).port_JoypadVector(); }
