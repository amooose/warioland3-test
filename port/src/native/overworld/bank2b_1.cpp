// overworld/bank2b_1 (src/engine/overworld/bank2b_1.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "../home/ow.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank2b1 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// the V-Blank functions copied to wVBlankFunc. The rest is C++
	// (src/native/overworld: SceneCommon, SceneWario, SceneSprites, the scenes).

	// ld hl, counter / xor a / ld [hld], a / inc [hl]
	void reset_and_advance(uint16_t counter)
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(counter, 0);
		SET_HL(gb_, counter - 1);
		wr8(counter - 1, alu_.inc(rd8(counter - 1)));
	}

	// c = frame to wait for
	void wait_and_advance(uint16_t counter)
	{
		cpu_.a = rd8(counter);
		alu_.cp(cpu_.a, cpu_.c);
		if (cpu_.f & FLAG_C)
			return;
		reset_and_advance(counter);
	}

	void port_Func_ad330() { wait_and_advance(w2d891); }
	void port_Func_ad335() { reset_and_advance(w2d891); }
	void port_Func_ad011() { wait_and_advance(w2d014); }
	void port_Func_ad016() { reset_and_advance(w2d014); }

	// unreferenced
	void port_Func_aca63()
	{
		SET_HL(gb_, wOWUIObj1XCoord);
		wr8(wOWUIObj1XCoord, alu_.dec(rd8(wOWUIObj1XCoord)));
	}

	void port_Func_aca68()
	{
		SET_HL(gb_, wOWUIObj1XCoord);
		wr8(wOWUIObj1XCoord, alu_.inc(rd8(wOWUIObj1XCoord)));
	}

	// unreferenced
	void port_DebugSceneWario()
	{
		cpu_.a = rd8(wJoypadPressed);
		cpu_.b = cpu_.a;
		cpu_.a = rd8(wDebugSceneWarioState);
		if (alu_.bit(cpu_.b, B_PAD_RIGHT)) {
			alu_.cp(cpu_.a, NUM_SCENEWARIO_STATES - 1);
			if (cpu_.f & FLAG_C)
				cpu_.a = alu_.inc(cpu_.a);
		} else {
			if (!alu_.bit(cpu_.b, B_PAD_LEFT)) {
				RET();
				return;
			}
			uint8_t a = alu_.and_(cpu_.a, cpu_.a);
			if (a)
				cpu_.a = alu_.dec(cpu_.a);
		}
		wr8(wDebugSceneWarioState, cpu_.a);
		JUMP_TO(ADDR_SetSceneWarioState); // fallthrough
	}

	// unreferenced
	void port_Func_ad329()
	{
		cpu_.a = rd8(wSceneWarioState);
		alu_.cp(cpu_.a, cpu_.b);
		if (!(cpu_.f & FLAG_Z)) {
			RET();
			return;
		}
		JUMP_TO(ADDR_Func_ad335);
	}

	void port_Func_ad33c()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(w2d890, 0);
		wr8(w2d891, 0);
		SET_HL(gb_, w2d891);
		JUMP_TO(ADDR_Func_ad016);
	}

	// The V-Blank functions, as copied to wVBlankFunc (see ram_code.c),
	// plain over memory. They end in hTransferVirtualOAM, which takes a
	// (wVirtualOAM's page).
	void transfer_oam()
	{
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}

	// the HDMA in wHDMA, from wHDMABank (cleared): to the VRAM bank given
	// (or the one mapped)
	void hdma(int vram_bank)
	{
		Memory mem(gb_);
		uint8_t bank = mem[wHDMABank];
		if (!bank)
			return;
		mem[wHDMABank] = 0;
		uint8_t rom = mem[wROMBank];
		bankswitch(bank);
		if (vram_bank >= 0)
			mem[rVBK] = mem[static_cast<uint16_t>(vram_bank)];
		for (int i = 0; i < 5; i++)
			mem[static_cast<uint16_t>(rVDMA_SRC_HIGH + i)] = mem[static_cast<uint16_t>(wHDMA + i)];
		bankswitch(rom);
	}

	// VBlank_PrologueEpilogue.Func: the scroll, the HDMA, a pending
	// character (tile and attribute), the palette configs, the LCDC flags
	// to flip
	void prologue_epilogue_vblank()
	{
		Memory mem(gb_);
		mem[rWBK] = 0x02; // BANK("WRAM2")
		mem[rSCY] = mem[wSCY];
		mem[rSCX] = mem[wSCX];
		hdma(wHDMADestVRAMBank);
		if (uint8_t lo = mem[wPendingCharDest]) {
			mem[wPendingCharDest] = 0;
			uint16_t dest = static_cast<uint16_t>(mem[wPendingCharDest + 1] << 8 | lo);
			mem[dest] = mem[wPendingCharDest + 2]; // wPendingCharTile
			mem[rVBK] = 0x01; // BANK("VRAM1")
			mem[dest] = mem[wPendingCharDest + 3]; // wPendingCharAttr
		}
		mem[rVBK] = 0;
		if (mem[wPalConfig1])
			home::apply_pal_config(gb_, wPalConfig1);
		if (mem[wPalConfig2])
			home::apply_pal_config(gb_, wPalConfig2);
		if (uint8_t flip = mem[wLCDCFlagsToFlip]) {
			mem[rLCDC] = static_cast<uint8_t>(mem[rLCDC] ^ flip);
			mem[wLCDCFlagsToFlip] = 0;
		}
		transfer_oam();
	}

	// _GolfBuilding.func: the scroll and the HDMA
	void golf_building_vblank()
	{
		Memory mem(gb_);
		mem[rWBK] = 2; // BANK("WRAM2")
		mem[rSCY] = mem[wSCY];
		mem[rSCX] = mem[wSCX];
		hdma(-1);
		mem[rVBK] = 0;
		transfer_oam();
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank2b1;

PORT_EX(Func_ad330, 0, 0) { OverworldBank2b1(gb).port_Func_ad330(); }
PORT_EX(Func_ad335, 0, 0) { OverworldBank2b1(gb).port_Func_ad335(); }
PORT_EX(Func_ad011, 0, 0) { OverworldBank2b1(gb).port_Func_ad011(); }
PORT_EX(Func_ad016, 0, 0) { OverworldBank2b1(gb).port_Func_ad016(); }
PORT_EX(Func_aca63, 0, 0) { OverworldBank2b1(gb).port_Func_aca63(); }
PORT_EX(Func_aca68, 0, 0) { OverworldBank2b1(gb).port_Func_aca68(); }
PORT_EX(DebugSceneWario, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_DebugSceneWario(); }
PORT_EX(Func_ad329, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_Func_ad329(); }
PORT_EX(Func_ad33c, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_Func_ad33c(); }
PORT_LOCAL_EX(VBlank_PrologueEpilogue, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { OverworldBank2b1(gb).prologue_epilogue_vblank(); }
PORT_LOCAL_EX(_GolfBuilding, func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { OverworldBank2b1(gb).golf_building_vblank(); }
