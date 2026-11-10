// title/raster (src/engine/title/raster.asm): the title screen's raster
// effects, which wait on rLY as the original does, and the V-Blank
// function copied to wVBlankFunc, as plain C++. The raster effects' callers
// are the title's state table (C++, by address), which leaves a and f as
// the asm does: their hooks set them.
#include "../asm_module.hpp"
#include "../flags.hpp"
#include "../home/memory.hpp"
#include "../home/system.hpp"

namespace wl3::title {

namespace {

// waits until rLY reaches ly, letting time pass
void wait_ly(GB *gb, uint8_t ly)
{
	Memory mem(gb);
	while (mem[rLY] < ly)
		home::idle(gb, 32);
}

// PanIntroBackgroundLayers: the intro airplane scene's background, each
// horizontal layer scrolling at its own speed (unless wceef); false if
// skipped
bool pan_intro_background_layers(GB *gb)
{
	Memory mem(gb);
	if (mem.unk_ceef())
		return false;
	static const struct {
		uint8_t ly;
		uint16_t scx;
	} layers[] = {
		{0x5b, wIntroBGXOffsetFar},    // after the sky
		{0x63, wIntroBGXOffsetCentre}, // the far layer
		{0x73, wIntroBGXOffsetNear},   // the centre layer
		{0x83, wSCX},                  // the near layer
	};
	for (const auto &layer : layers) {
		wait_ly(gb, layer.ly);
		home::wait_next_hblank(gb);
		mem[rSCX] = mem[layer.scx];
	}
	return true;
}

// AdjustJapaneseTimeAttackDescriptionWindow: rSCY 7 lower at each of its
// lines (the Japanese text's spacing); false if English. Returns the last
// rSCY before its add.
bool adjust_japanese_time_attack_description_window(GB *gb, uint8_t *last)
{
	Memory mem(gb);
	if (mem.language())
		return false;
	static const uint8_t lines[] = {0x2f, 0x38, 0x41, 0x5a, 0x63, 0x6c};
	for (uint8_t ly : lines) {
		wait_ly(gb, ly);
		home::wait_next_hblank(gb);
		*last = mem[rSCY];
		mem[rSCY] = static_cast<uint8_t>(*last + 0x07);
	}
	return true;
}

class TitleRaster : public AsmModule {
public:
	using AsmModule::AsmModule;

	// VBlank_Title.Func, as copied to wVBlankFunc, plain over memory: the
	// V-Blank handler pushes and pops every register around it. The title
	// box's two tilemap rows (wcee4 bit 7), the scroll, then
	// hTransferVirtualOAM (a = wVirtualOAM's page).
	void port_VBlank_Title__Func()
	{
		uint8_t bank = mem_.rom_bank();
		bankswitch(BANK_TitleScreenBoxTilemap);
		if (mem_.unk_cee4() & 0x80) {
			mem_[rVBK] = 0;
			constexpr uint16_t dest = v0BGMap0 + 14 * TILEMAP_WIDTH + 4; // debgcoord 4, 14
			home::copy(gb_, ROM(gb_, TitleScreenBoxTilemap), dest, 12);
			home::copy(gb_, ROM(gb_, TitleScreenBoxTilemap) + 12, dest + TILEMAP_WIDTH, 12);
			mem_.unk_cee4() &= 0x7f;
		}
		bankswitch(bank);
		mem_[rSCY] = mem_.scy();
		mem_[rSCX] = mem_.scx();
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}
};

} // namespace

} // namespace wl3::title

using namespace wl3::title;
namespace flags = wl3::flags;

// a = wceef and its and, or the last layer's wSCX after WaitNextHBlank's
// and STAT_MODE
PORT_EX(PanIntroBackgroundLayers, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	if (pan_intro_background_layers(gb)) {
		gb->cpu.a = wl3::Memory(gb).scx();
		gb->cpu.f = FLAG_Z | FLAG_H;
	} else {
		gb->cpu.a = wl3::Memory(gb).unk_ceef();
		gb->cpu.f = flags::and_(gb->cpu.a);
	}
}
// a = the language and its and, or the last rSCY and its add
PORT_EX(AdjustJapaneseTimeAttackDescriptionWindow, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	uint8_t last = 0;
	if (adjust_japanese_time_attack_description_window(gb, &last)) {
		gb->cpu.a = static_cast<uint8_t>(last + 0x07);
		gb->cpu.f = flags::add(last, 0x07);
	} else {
		gb->cpu.a = wl3::Memory(gb).language();
		gb->cpu.f = flags::and_(gb->cpu.a);
	}
}
PORT_LOCAL_EX(VBlank_Title, Func, 0, ALL_REGISTERS) { TitleRaster(gb).port_VBlank_Title__Func(); }
