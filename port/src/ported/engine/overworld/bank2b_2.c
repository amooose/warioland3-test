// Port of src/engine/overworld/bank2b_2.asm
#include "ported.h"

// call to a routine in this bank. Plain CALL would abort if the bank was
// switched (a fuzzed wROMBank restored by a farcall, say); the asm then
// just runs whatever is mapped, and so does this.
#define CALL_2B(name) gb_call(gb, ADDR_##name)

static void set_wario_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state; // ld a, state
	CALL_2B(SetSceneWarioState);
}

// ld hl, obj_state / ld a, state / call SetSceneObjState
static void set_obj_state(GB *gb, uint16_t obj_state, uint8_t state)
{
	SET_HL(gb, obj_state);
	gb->cpu.a = state;
	CALL(SetSceneObjState);
}

static void play_music2(GB *gb, uint16_t music)
{
	gb->cpu.a = (uint8_t)(music >> 8);
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
	wr8(gb, hMusicID + 1, gb->cpu.a);
}

// ld hl, counter / xor a / ld [hld], a / inc [hl]
static void reset_counter_and_advance(GB *gb, uint16_t counter)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, counter, 0);
	SET_HL(gb, counter - 1);
	wr8(gb, counter - 1, alu_inc(gb, rd8(gb, counter - 1)));
}

// ---------------------------------------------------------------------------
// Prologue sequencing

static void prologue_next(GB *gb)
{
	reset_counter_and_advance(gb, w2d014); // inc w2d013
}

static void prologue_delay(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	prologue_next(gb);
}

PORT(Prologue_Delay)
{
	prologue_delay(gb);
}

PORT(Prologue_Next)
{
	prologue_next(gb);
}

static void prologue_finish(GB *gb)
{
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT(Prologue_Finish)
{
	prologue_finish(gb);
}

// xor a / ld [w2d891], a / inc [w2d890]
static void advance_w2d890(GB *gb)
{
	reset_counter_and_advance(gb, w2d891);
}

// c = number of frames to wait
static void func_ae1c9(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	advance_w2d890(gb);
}

// b = Wario state to wait for
static void func_ae1c2(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

PORT(Func_ae1c2)
{
	func_ae1c2(gb);
}

PORT(Func_ae1c9)
{
	func_ae1c9(gb);
}

PORT(Func_ae1ce)
{
	advance_w2d890(gb);
}

static void func_ae1d5(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, w2d890, 0);
	wr8(gb, w2d891, 0);
	prologue_next(gb);
}

PORT(Func_ae1d5)
{
	func_ae1d5(gb);
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

PORT(Func_af130)
{
	prologue_delay(gb); // same as Prologue_Delay
}

PORT(Func_af135)
{
	prologue_next(gb); // same as Prologue_Next
}

// ---------------------------------------------------------------------------
// Func_af13c: the epilogue

#define EPI(local) (ADDR_Func_af13c + ((local) - 0x713c))
#define EPI_RET (ADDR_Func_af13c + 3) // return address of call .Func_af14c

// ld a, [bc] / cp n: returns true (and the asm returns) if a < n
#define BC_BELOW(n) w2d891_below(gb, (n))

// .Func_af436: the old man's transformation

// .Func_af670: the old man thanks Wario

// .Func_af741: the old man walks away

// .Func_af7af: the transformation into the princess

// .Func_af8bc: the crowd

// ---------------------------------------------------------------------------
// _InitEpilogue

// decompress_tiles0 / decompress_tilemap: decompresses src (in bank) to dest
static void epi_decompress(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, src);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_tiles1 / decompress_attrmap: the same, into VRAM bank 1
static void epi_decompress_vram1(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	epi_decompress(gb, bank, src, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// call DisableLCD / call FillBGMap0_With7f / call ClearVirtualOAM
static void epi_clear_screen(GB *gb)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);
}

// ld a, [wCollectionRow] / ld b, a / and a / ld a, LCDC / jr z / xor b /
// ldh [rLCDC], a / xor a / ld [wCollectionRow], a
static void epi_apply_lcdc(GB *gb)
{
	gb->cpu.a = gb->cpu.b = rd8(gb, wCollectionRow);
	bool z = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
	if (!z)
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.b);
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCollectionRow, 0);
}

// Unreferenced: sets up the epilogue's crowd scene. The difftest can't check
// it: decompressing the uncompressed crowd attrmap runs Decompress over raw
// data, which writes on through WRAM into the stack, and the asm returns
// into garbage. The rest uses the same helpers as _InitEpilogue.
PORT_JUMP(Func_af01f)
{
	epi_clear_screen(gb);
	play_music2(gb, MUSIC_EPILOGUE);

	gb->cpu.a = 0x02;
	wr8(gb, rWBK, 0x02);
	gb->cpu.a = SCREEN_WIDTH_PX;
	wr8(gb, rSCX, gb->cpu.a);
	wr8(gb, wSCX, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);

	SET_HL(gb, ADDR_Pals_86add);
	SET_DE(gb, wTempPals1);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_86add;
	CALL(CopyFarBytes);
	SET_HL(gb, ADDR_Pals_86b1d);
	SET_DE(gb, wTempPals2);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_86b1d;
	CALL(CopyFarBytes);

	epi_decompress_vram1(gb, BANK_FontGfx, ADDR_FontGfx, v1Tiles0);

	// attempting to decompress uncompressed data
	epi_decompress_vram1(gb, BANK_EpilogueCrowdAttrmap, ADDR_EpilogueCrowdAttrmap, v1BGMap0);
	epi_decompress(gb, BANK_EpilogueCrowdTilemap, ADDR_EpilogueCrowdTilemap, v0BGMap0);

	gb->cpu.a = 0x60;
	wr8(gb, wOWUIObj1YCoord, gb->cpu.a);
	SET_HL(gb, wOWUIObj1YCoord + 1);
	wr8(gb, wOWUIObj1YCoord + 1, 0x5c);
	gb->cpu.a = 0xff;
	wr8(gb, wOWUIObj2YCoord, gb->cpu.a);

	set_wario_state(gb, 0x0c);
	static const struct { uint16_t obj, state_addr; uint8_t y, x, state; } objs[] = {
		{wSceneObj10, wSceneObj10State, 0x30, 0x00, 0x1b},
		{wSceneObj11, wSceneObj11State, 0x35, 0x20, 0x1c},
		{wSceneObj12, wSceneObj12State, 0x32, 0x70, 0x1b},
		{wSceneObj13, wSceneObj13State, 0x38, 0xc0, 0x1c},
	};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = objs[i].y;
		wr8(gb, objs[i].obj, gb->cpu.a);
		wr8(gb, objs[i].obj + 1, objs[i].x);
		set_obj_state(gb, objs[i].state_addr, objs[i].state);
	}

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);
	wr8(gb, w2d890, 0);
	wr8(gb, w2d891, 0);
	epi_apply_lcdc(gb);
	JUMP_TO(ADDR_Func_af135);
}
