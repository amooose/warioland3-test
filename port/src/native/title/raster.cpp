// title/raster (src/engine/title/raster.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::title {

namespace {

class TitleRaster : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The title screen's register-level parts (see src/native/title): the
	// raster effects, which wait on rLY as the original does, and the V-Blank
	// function copied to wVBlankFunc.

	// waits until rLY reaches ly, letting time pass
	void wait_ly(uint8_t ly)
	{
		for (;;) {
			cpu_.a = rd8(rLY);
			alu_.cp(cpu_.a, ly);
			if (!(cpu_.f & FLAG_C))
				break;
			idle(32);
		}
	}

	// pans the background of the intro airplane scene
	// each horizontal layer scrolls at different speed
	void port_PanIntroBackgroundLayers()
	{
		cpu_.a = rd8(wceef);
		alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.a)
			return;
		wait_ly(0x5b); // sky
		CALL(WaitNextHBlank);
		cpu_.a = rd8(wIntroBGXOffsetFar);
		wr8(rSCX, cpu_.a);
		wait_ly(0x63); // far layer
		CALL(WaitNextHBlank);
		cpu_.a = rd8(wIntroBGXOffsetCentre);
		wr8(rSCX, cpu_.a);
		wait_ly(0x73); // centre layer
		CALL(WaitNextHBlank);
		cpu_.a = rd8(wIntroBGXOffsetNear);
		wr8(rSCX, cpu_.a);
		wait_ly(0x83); // near layer
		CALL(WaitNextHBlank);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
	}

	void port_AdjustJapaneseTimeAttackDescriptionWindow()
	{
		cpu_.a = rd8(wLanguage);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (cpu_.a)
			return; // return if English

		static const uint8_t lines[] = { 0x2f, 0x38, 0x41, 0x5a, 0x63, 0x6c };
		for (int i = 0; i < 6; i++) {
			for (;;) {
				cpu_.a = rd8(rLY);
				alu_.cp(cpu_.a, lines[i]);
				if (!(cpu_.f & FLAG_C))
					break;
				idle(32);
			}
			CALL(WaitNextHBlank);
			cpu_.a = alu_.add(rd8(rSCY), 0x07);
			wr8(rSCY, cpu_.a);
		}
	}

	// VBlank_Title.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
	// return to base + n, the block's start wherever it runs from.
	void port_VBlank_Title__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_TitleScreenBoxTilemap;
		bankswitch(cpu_.a);
		cpu_.a = rd8(wcee4);
		if (alu_.bit(cpu_.a, 7)) {
			SET_HL(gb_, ADDR_TitleScreenBoxTilemap);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a (VRAM0)
			wr8(rVBK, cpu_.a);
			SET_DE(gb_, v0BGMap0 + 14 * TILEMAP_WIDTH + 4); // debgcoord 4, 14
			gb_push16(gb_, REG_DE(gb_));
			cpu_.b = 12;
			gb_call_ret(gb_, ADDR_CopyHLToDE, (uint16_t)(base + 0x22));
			SET_DE(gb_, gb_pop16(gb_));
			cpu_.a = alu_.add(cpu_.e, TILEMAP_WIDTH);
			cpu_.e = cpu_.a;
			cpu_.a = alu_.adc(cpu_.d, 0);
			cpu_.d = cpu_.a;
			cpu_.b = 12;
			gb_call_ret(gb_, ADDR_CopyHLToDE, (uint16_t)(base + 0x30));
			SET_HL(gb_, wcee4);
			wr8(wcee4, (uint8_t)(rd8(wcee4) & 0x7f));
		}
		// .skip
		pop_af();
		bankswitch(cpu_.a);
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x4b));
		RET();
	}
};

} // namespace

} // namespace wl3::title

using wl3::title::TitleRaster;

PORT_EX(PanIntroBackgroundLayers, 0, 0) { TitleRaster(gb).port_PanIntroBackgroundLayers(); }
PORT_EX(AdjustJapaneseTimeAttackDescriptionWindow, 0, 0) { TitleRaster(gb).port_AdjustJapaneseTimeAttackDescriptionWindow(); }
PORT_LOCAL_EX(VBlank_Title, Func, HOOK_MANUAL_RET, 0) { TitleRaster(gb).port_VBlank_Title__Func(); }
