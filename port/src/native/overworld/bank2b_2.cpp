// overworld/bank2b_2 (src/engine/overworld/bank2b_2.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank2b2 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// call to a routine in this bank. Plain CALL would abort if the bank was
	// switched (a fuzzed wROMBank restored by a farcall, say); the asm then
	// just runs whatever is mapped, and so does this.
	#define CALL_2B(name) gb_call(gb_, ADDR_##name)

	void set_wario_state(uint8_t state)
	{
		cpu_.a = state; // ld a, state
		CALL_2B(SetSceneWarioState);
	}

	// ld hl, obj_state / ld a, state / call SetSceneObjState
	void set_obj_state(uint16_t obj_state, uint8_t state)
	{
		SET_HL(gb_, obj_state);
		cpu_.a = state;
		CALL(SetSceneObjState);
	}

	void play_music2(uint16_t music)
	{
		cpu_.a = (uint8_t)(music >> 8);
		wr8(hMusicIDBackup + 0, cpu_.a);
		wr8(hMusicID + 0, cpu_.a);
		cpu_.a = (uint8_t)music;
		wr8(hMusicIDBackup + 1, cpu_.a);
		wr8(hMusicID + 1, cpu_.a);
	}

	// ld hl, counter / xor a / ld [hld], a / inc [hl]
	void reset_counter_and_advance(uint16_t counter)
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		wr8(counter, 0);
		SET_HL(gb_, counter - 1);
		wr8(counter - 1, alu_.inc(rd8(counter - 1)));
	}

	// ---------------------------------------------------------------------------
	// Prologue sequencing

	void prologue_next()
	{
		reset_counter_and_advance(w2d014); // inc w2d013
	}

	void prologue_delay()
	{
		cpu_.a = rd8(w2d014);
		alu_.cp(cpu_.a, cpu_.c);
		if (cpu_.f & FLAG_C)
			return;
		prologue_next();
	}

	void port_Prologue_Delay()
	{
		prologue_delay();
	}

	void port_Prologue_Next()
	{
		prologue_next();
	}

	void prologue_finish()
	{
		SET_HL(gb_, wSubState);
		wr8(wSubState, alu_.inc(rd8(wSubState)));
	}

	void port_Prologue_Finish()
	{
		prologue_finish();
	}

	// xor a / ld [w2d891], a / inc [w2d890]
	void advance_w2d890()
	{
		reset_counter_and_advance(w2d891);
	}

	// c = number of frames to wait
	void func_ae1c9()
	{
		cpu_.a = rd8(w2d891);
		alu_.cp(cpu_.a, cpu_.c);
		if (cpu_.f & FLAG_C)
			return;
		advance_w2d890();
	}

	// b = Wario state to wait for
	void func_ae1c2()
	{
		cpu_.a = rd8(wSceneWarioState);
		alu_.cp(cpu_.a, cpu_.b);
		if (!(cpu_.f & FLAG_Z))
			return;
		advance_w2d890();
	}

	void port_Func_ae1c2()
	{
		func_ae1c2();
	}

	void port_Func_ae1c9()
	{
		func_ae1c9();
	}

	void port_Func_ae1ce()
	{
		advance_w2d890();
	}

	void func_ae1d5()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		wr8(w2d890, 0);
		wr8(w2d891, 0);
		prologue_next();
	}

	void port_Func_ae1d5()
	{
		func_ae1d5();
	}

	// ---------------------------------------------------------------------------
	// Prologue

	// ---------------------------------------------------------------------------
	// Func_ae1de

	// ---------------------------------------------------------------------------
	// Prologue_WarioEnterCave

	// ---------------------------------------------------------------------------
	// Prologue_WarioSuckedIntoMusicBox

	#define SUCKED_ADDR(local) (ADDR_Prologue_WarioSuckedIntoMusicBox + ((local) - 0x631c))

	// ---------------------------------------------------------------------------
	// Func_ae55c, Func_ae641, Func_ae668: bc = w2d891

	// ---------------------------------------------------------------------------
	// Prologue_WarioFallFromPlane

	// ---------------------------------------------------------------------------
	// Prologue_WarioWalkToCaveEntrance

	// ---------------------------------------------------------------------------
	// The plane parts (Func_ae8bb)

	// Plane part 1: Func_ae924

	#define P1C wPlanePart1Counter
	#define P1A wPlanePart1Action

	// Plane part 2: Func_ae9ee

	#define P2C wPlanePart2Counter
	#define P2A wPlanePart2Action

	// Plane part 3: Func_aeb0d

	#define P3C wPlanePart3Counter
	#define P3A wPlanePart3Action

	// Plane part 4: Func_aec1f

	#define P4C wPlanePart4Counter
	#define P4A wPlanePart4Action

	// Func_ae8bb: all four plane parts, then their sequencing

	// ---------------------------------------------------------------------------
	// Epilogue sequencing

	void port_Func_af130()
	{
		prologue_delay(); // same as Prologue_Delay
	}

	void port_Func_af135()
	{
		prologue_next(); // same as Prologue_Next
	}

	// ---------------------------------------------------------------------------
	// Func_af13c: the epilogue

	#define EPI(local) (ADDR_Func_af13c + ((local) - 0x713c))
	#define EPI_RET (ADDR_Func_af13c + 3) // return address of call .Func_af14c

	// ld a, [bc] / cp n: returns true (and the asm returns) if a < n
	#define BC_BELOW(n) w2d891_below((n))

	// .Func_af436: the old man's transformation

	// .Func_af670: the old man thanks Wario

	// .Func_af741: the old man walks away

	// .Func_af7af: the transformation into the princess

	// .Func_af8bc: the crowd

	// ---------------------------------------------------------------------------
	// _InitEpilogue

	// decompress_tiles0 / decompress_tilemap: decompresses src (in bank) to dest
	void epi_decompress(uint8_t bank, uint16_t src, uint16_t dest)
	{
		cpu_.a = bank;
		wr8(wTempBank, bank);
		SET_HL(gb_, src);
		SET_BC(gb_, dest);
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);
	}

	// decompress_tiles1 / decompress_attrmap: the same, into VRAM bank 1
	void epi_decompress_vram1(uint8_t bank, uint16_t src, uint16_t dest)
	{
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, 1);
		epi_decompress(bank, src, dest);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	// call DisableLCD / call FillBGMap0_With7f / call ClearVirtualOAM
	void epi_clear_screen()
	{
		CALL(DisableLCD);
		CALL(FillBGMap0_With7f);
		CALL(ClearVirtualOAM);
	}

	// ld a, [wCollectionRow] / ld b, a / and a / ld a, LCDC / jr z / xor b /
	// ldh [rLCDC], a / xor a / ld [wCollectionRow], a
	void epi_apply_lcdc()
	{
		cpu_.a = cpu_.b = rd8(wCollectionRow);
		bool z = !alu_.and_(cpu_.a, cpu_.a);
		cpu_.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
		if (!z)
			cpu_.a = alu_.xor_(cpu_.a, cpu_.b);
		wr8(rLCDC, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wCollectionRow, 0);
	}

	// Unreferenced: sets up the epilogue's crowd scene. The difftest can't check
	// it: decompressing the uncompressed crowd attrmap runs Decompress over raw
	// data, which writes on through WRAM into the stack, and the asm returns
	// into garbage. The rest uses the same helpers as _InitEpilogue.
	void port_Func_af01f()
	{
		epi_clear_screen();
		play_music2(MUSIC_EPILOGUE);

		cpu_.a = 0x02;
		wr8(rWBK, 0x02);
		cpu_.a = SCREEN_WIDTH_PX;
		wr8(rSCX, cpu_.a);
		wr8(wSCX, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rSCY, 0);
		wr8(wSCY, 0);

		SET_HL(gb_, ADDR_Pals_86add);
		SET_DE(gb_, wTempPals1);
		cpu_.c = 8 * PAL_SIZE;
		cpu_.b = BANK_Pals_86add;
		CALL(CopyFarBytes);
		SET_HL(gb_, ADDR_Pals_86b1d);
		SET_DE(gb_, wTempPals2);
		cpu_.c = 8 * PAL_SIZE;
		cpu_.b = BANK_Pals_86b1d;
		CALL(CopyFarBytes);

		epi_decompress_vram1(BANK_FontGfx, ADDR_FontGfx, v1Tiles0);

		// attempting to decompress uncompressed data
		epi_decompress_vram1(BANK_EpilogueCrowdAttrmap, ADDR_EpilogueCrowdAttrmap, v1BGMap0);
		epi_decompress(BANK_EpilogueCrowdTilemap, ADDR_EpilogueCrowdTilemap, v0BGMap0);

		cpu_.a = 0x60;
		wr8(wOWUIObj1YCoord, cpu_.a);
		SET_HL(gb_, wOWUIObj1YCoord + 1);
		wr8(wOWUIObj1YCoord + 1, 0x5c);
		cpu_.a = 0xff;
		wr8(wOWUIObj2YCoord, cpu_.a);

		set_wario_state(0x0c);
		static const struct { uint16_t obj, state_addr; uint8_t y, x, state; } objs[] = {
			{wSceneObj10, wSceneObj10State, 0x30, 0x00, 0x1b},
			{wSceneObj11, wSceneObj11State, 0x35, 0x20, 0x1c},
			{wSceneObj12, wSceneObj12State, 0x32, 0x70, 0x1b},
			{wSceneObj13, wSceneObj13State, 0x38, 0xc0, 0x1c},
		};
		for (int i = 0; i < 4; i++) {
			cpu_.a = objs[i].y;
			wr8(objs[i].obj, cpu_.a);
			wr8(objs[i].obj + 1, objs[i].x);
			set_obj_state(objs[i].state_addr, objs[i].state);
		}

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wPalConfig1TotalSteps, 0);
		wr8(wPalConfig2TotalSteps, 0);
		wr8(w2d890, 0);
		wr8(w2d891, 0);
		epi_apply_lcdc();
		JUMP_TO(ADDR_Func_af135);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank2b2;

PORT_EX(Prologue_Delay, 0, 0) { OverworldBank2b2(gb).port_Prologue_Delay(); }
PORT_EX(Prologue_Next, 0, 0) { OverworldBank2b2(gb).port_Prologue_Next(); }
PORT_EX(Prologue_Finish, 0, 0) { OverworldBank2b2(gb).port_Prologue_Finish(); }
PORT_EX(Func_ae1c2, 0, 0) { OverworldBank2b2(gb).port_Func_ae1c2(); }
PORT_EX(Func_ae1c9, 0, 0) { OverworldBank2b2(gb).port_Func_ae1c9(); }
PORT_EX(Func_ae1ce, 0, 0) { OverworldBank2b2(gb).port_Func_ae1ce(); }
PORT_EX(Func_ae1d5, 0, 0) { OverworldBank2b2(gb).port_Func_ae1d5(); }
PORT_EX(Func_af130, 0, 0) { OverworldBank2b2(gb).port_Func_af130(); }
PORT_EX(Func_af135, 0, 0) { OverworldBank2b2(gb).port_Func_af135(); }
PORT_EX(Func_af01f, HOOK_MANUAL_RET, 0) { OverworldBank2b2(gb).port_Func_af01f(); }
