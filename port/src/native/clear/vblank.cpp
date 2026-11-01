// clear/vblank (src/engine/clear/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::clear {

namespace {

class ClearVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// are C++, src/native/clear)

	// ld hl, src / ld c, [hl] / ld b, TILE_SIZE - 1 / ld de, src - 2 /
	// .loop: ld a, [de] / ld [hld], a / dec l / dec e / dec e / dec b /
	// jr nz, .loop / ld [hl], c
	void rotate_tile_plane(uint16_t src)
	{
		SET_HL(gb_, src);
		cpu_.c = rd8(src);
		cpu_.b = TILE_SIZE - 1;
		SET_DE(gb_, (uint16_t)(src - 2));
		do {
			cpu_.a = rd8(REG_DE(gb_));
			uint16_t hl = REG_HL(gb_);
			wr8(hl, cpu_.a);
			SET_HL(gb_, (uint16_t)(hl - 1));
			cpu_.l = alu_.dec(cpu_.l);
			cpu_.e = alu_.dec(cpu_.e);
			cpu_.e = alu_.dec(cpu_.e);
			cpu_.b = alu_.dec(cpu_.b);
		} while (!(cpu_.f & FLAG_Z));
		wr8(REG_HL(gb_), cpu_.c);
	}

	// VBlank_ClearScreen.Func, as copied to wVBlankFunc
	void port_VBlank_ClearScreen__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, cpu_.a);
		rotate_tile_plane(v0Tiles2 + 0x71 * TILE_SIZE + (TILE_SIZE - 2));
		rotate_tile_plane(v0Tiles2 + 0x71 * TILE_SIZE + (TILE_SIZE - 1));
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x36));
		RET();
	}

	// ld a, [hli] / ld d, a / ld a, [hli] / ld e, a: the queued address
	uint16_t queue_addr(uint16_t hl)
	{
		cpu_.a = rd8(hl);
		cpu_.d = cpu_.a;
		cpu_.a = rd8((uint16_t)(hl + 1));
		cpu_.e = cpu_.a;
		return (uint16_t)(hl + 2);
	}

	// VBlank_MusicalCoinsScreen.Func, as copied to wVBlankFunc
	void port_VBlank_MusicalCoinsScreen__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = rd8(wcee4);
		if (alu_.bit(cpu_.a, 7)) {
			cpu_.a = alu_.and_(cpu_.a, 0x7f);
			uint16_t hl;
			if (!(cpu_.f & FLAG_Z)) {
				// .asm_d47d9
				cpu_.a = 1; // BANK("VRAM1")
				wr8(rVBK, cpu_.a);
				hl = queue_addr(wBGMapTileVRAM0Queue);
				cpu_.a = rd8(hl);
				wr8(REG_DE(gb_), cpu_.a);
			} else {
				hl = queue_addr(wBGMapTileVRAM0Queue);
				cpu_.a = rd8(hl++);
				wr8(REG_DE(gb_), cpu_.a);
				hl = queue_addr(hl);
				cpu_.a = rd8(hl++);
				wr8(REG_DE(gb_), cpu_.a);
			}
			// .asm_d47e6
			SET_HL(gb_, wcee4);
			wr8(wcee4, (uint8_t)(rd8(wcee4) & 0x7f));
		}
		// .dma_transfer
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x3d));
		RET();
	}
};

} // namespace

} // namespace wl3::clear

using wl3::clear::ClearVblank;

PORT_LOCAL_EX(VBlank_ClearScreen, Func, HOOK_MANUAL_RET, 0) { ClearVblank(gb).port_VBlank_ClearScreen__Func(); }
PORT_LOCAL_EX(VBlank_MusicalCoinsScreen, Func, HOOK_MANUAL_RET, 0) { ClearVblank(gb).port_VBlank_MusicalCoinsScreen__Func(); }
