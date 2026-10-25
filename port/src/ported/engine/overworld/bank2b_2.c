// Port of src/engine/overworld/bank2b_2.asm
#include "ported.h"

typedef void (*Handler)(GB *gb);

// call to a routine in this bank. Plain CALL would abort if the bank was
// switched (a fuzzed wROMBank restored by a farcall, say); the asm then
// just runs whatever is mapped, and so does this.
#define CALL_2B(name) gb_call(gb, ADDR_##name)

// rst JumpTable, with the table at `table` (right after the rst): pushes the
// return address, pops it into hl, a = a * 2, de = a, hl += de, hl = [hl].
// Leaves every register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// Runs the interpreter from the current pc until it returns to ret_addr
// with the stack pointer back at sp (like gb_call, but with the asm's own
// return address on the stack instead of a sentinel, so that code that
// looks at the stack sees what it sees in the asm). Returns false if the
// code pops past sp instead (fuzzed garbage, say): the asm then carries on
// in its caller's frame, so the native caller must stop and leave pc as it
// is.
static bool run_until(GB *gb, uint16_t ret_addr, uint16_t sp)
{
	bool escaped = false;
	gb->call_depth++;
	while (!(gb->cpu.pc == ret_addr && gb->cpu.sp == sp) && !gb->test.crashed) {
		if (gb->cpu.sp > sp) {
			escaped = true;
			break;
		}
		if (gb->test.budget && --gb->test.budget == 0)
			gb->test.out_of_budget(gb);
		int cycles = cpu_step(gb);
		if (!gb->no_tick)
			gb_tick(gb, cycles);
	}
	gb->call_depth--;
	return !escaped;
}

// call target, where ret_addr is the address after the call instruction.
// Returns false if the native caller must stop (see run_until).
static bool call_ret(GB *gb, uint16_t target, uint16_t ret_addr)
{
	uint16_t pc = gb->cpu.pc, sp = gb->cpu.sp;
	gb_push16(gb, ret_addr);
	gb->cpu.pc = target;
	if (!run_until(gb, ret_addr, sp))
		return false;
	gb->cpu.pc = pc;
	return true;
}

// A jumptable with n entries. handlers[i] is the C version of entry i (it
// runs like a call of that entry, up to its ret); NULL means the entry is
// run as is. ret_addr is the return address of the `call` that runs this
// jumptable as a subroutine, or 0 if the jumptable is the tail of the
// routine being run.
// Returns true when done (the subroutine returned, or the tail entry ran up
// to its ret, which the caller then performs with RET()). Returns false if
// execution must continue in the asm at pc, with the stack as the asm has
// it (an entry run as is in tail position, or an index out of range, which
// jumps to garbage); the caller must stop then.
static bool dispatch(GB *gb, uint16_t table, const Handler *handlers, int n, uint16_t ret_addr)
{
	uint16_t sp = gb->cpu.sp;
	if (ret_addr)
		gb_push16(gb, ret_addr); // call
	uint16_t t = jumptable(gb, table);
	int idx = gb->cpu.e >> 1;
	if (idx < n && handlers[idx]) {
		handlers[idx](gb);
		if (ret_addr)
			gb_pop16(gb); // ret
		return true;
	}
	if (idx < n && ret_addr) {
		uint16_t pc = gb->cpu.pc;
		gb->cpu.pc = t;
		if (!run_until(gb, ret_addr, sp))
			return false;
		gb->cpu.pc = pc;
		return true;
	}
	JUMP_TO(t);
	return false;
}

// A routine whose tail is a jumptable (PORT_JUMP).
static void tail_dispatch(GB *gb, uint16_t table, const Handler *handlers, int n)
{
	if (dispatch(gb, table, handlers, n, 0))
		RET();
}

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

// xor a / ld hl, obj_state / call SetSceneObjState
static void clear_obj_state(GB *gb, uint16_t obj_state)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	SET_HL(gb, obj_state);
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

static void stop_music2(GB *gb)
{
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = 0x00;
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

static void wait_frames(GB *gb, uint8_t frames)
{
	gb->cpu.c = frames;
	func_ae1c9(gb);
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

static void wait_wario_state(GB *gb, uint8_t state)
{
	gb->cpu.b = state;
	func_ae1c2(gb);
}

// ld a, state / call SetSceneWarioState / jp Func_ae1ce
static void set_wario_state_and_advance(GB *gb, uint8_t state)
{
	set_wario_state(gb, state);
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

// ld hl, w2d891 / inc [hl] / dec l / ld a, [hl]
static void inc_w2d891_ld_a_w2d890(GB *gb)
{
	SET_HL(gb, w2d891);
	wr8(gb, w2d891, alu_inc(gb, rd8(gb, w2d891)));
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = rd8(gb, REG_HL(gb));
}

// ld hl, w2d891 / inc [hl] / ld c, l / ld b, h / dec l / ld a, [hl]
static void inc_w2d891_ld_bc_a_w2d890(GB *gb)
{
	SET_HL(gb, w2d891);
	wr8(gb, w2d891, alu_inc(gb, rd8(gb, w2d891)));
	gb->cpu.c = gb->cpu.l;
	gb->cpu.b = gb->cpu.h;
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = rd8(gb, REG_HL(gb));
}

// ld hl, obj / ld a, y / ld [hli], a / ld a, x / ld [hl], a
static void set_obj_coord(GB *gb, uint16_t obj, uint8_t y, uint8_t x)
{
	SET_HL(gb, obj);
	gb->cpu.a = y;
	wr8(gb, obj, y);
	SET_HL(gb, obj + 1);
	gb->cpu.a = x;
	wr8(gb, obj + 1, x);
}

// ---------------------------------------------------------------------------
// Prologue

static void prologue_set_fade(GB *gb) // .Func_ae009
{
	wr8(gb, w2d880, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);
	prologue_next(gb);
}

static void prologue_wait60(GB *gb)
{
	gb->cpu.c = 60;
	prologue_delay(gb);
}

static void prologue_set_fade_in_from_black(GB *gb)
{
	gb->cpu.a = 0x01;
	prologue_set_fade(gb);
}

static void prologue_scroll_right_40_pixels(GB *gb)
{
	SET_HL(gb, wSCX);
	wr8(gb, wSCX, alu_inc(gb, rd8(gb, wSCX)));
	gb->cpu.a = rd8(gb, wSCX);
	alu_cp(gb, gb->cpu.a, 40);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	prologue_next(gb);
}

static void prologue_set_fade_to_black(GB *gb)
{
	gb->cpu.a = 0x00;
	prologue_set_fade(gb);
}

static void prologue_func_ae067(GB *gb)
{
	// set Obj3's coordinates to (120, 195)
	set_obj_coord(gb, wSceneObj3YCoord, 195, 120);
	set_obj_state(gb, wSceneObj3State, 0x09);
}

static void prologue_prepare_cave_scene1(GB *gb)
{
	gb->cpu.a = 112;
	wr8(gb, wSCY, 112);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSCX, 0);

	set_obj_coord(gb, wSceneWarioYCoord, 208, 240);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // SCENEWARIO_NONE
	CALL_2B(SetSceneWarioState);

	prologue_func_ae067(gb);

	SET_HL(gb, wSceneObj1);
	gb->cpu.a = 187;
	wr8(gb, wSceneObj1, 187);
	SET_HL(gb, wSceneObj1 + 1);
	wr8(gb, wSceneObj2YCoord, 187);
	gb->cpu.a = 120;
	wr8(gb, wSceneObj1 + 1, 120);
	wr8(gb, wSceneObj2XCoord, 120);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, w2d8b0, 0);
	SET_HL(gb, wSceneObj1State);
	CALL(SetSceneObjState);

	play_music2(gb, MUSIC_MUSIC_BOX_CAVE);

	gb->cpu.a = 0x06;
	prologue_set_fade(gb);
}

// hl = palettes
static void prologue_func_ae0ab(GB *gb)
{
	SET_DE(gb, wTempPals1);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_86a0d;
	CALL(CopyFarBytes);

	SET_DE(gb, ADDR_Data_ac9f5);
	CALL_2B(Func_ac9e9);
	SET_DE(gb, ADDR_Data_ac9ff);
	CALL_2B(Func_ac9e4);
	prologue_next(gb);
}

static void prologue_show_music_box_close_up(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSCY, 0);
	wr8(gb, wSCX, 0);

	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, LCDC_BG_MAP);

	set_obj_coord(gb, wSceneObj8, 80, 80);
	set_obj_state(gb, wSceneObj8State, 0x0d);

	clear_obj_state(gb, wSceneObj3State);

	SET_HL(gb, ADDR_Pals_86a8d);
	SET_DE(gb, wTempOBPals);
	gb->cpu.c = 1 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_86a8d;
	CALL(CopyFarBytes);
	SET_HL(gb, ADDR_Pals_86a0d);
	prologue_func_ae0ab(gb);
}

static void prologue_prepare_cave_scene2(GB *gb)
{
	gb->cpu.a = 112;
	wr8(gb, wSCY, 112);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSCX, 0);

	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, LCDC_BG_MAP);

	set_wario_state(gb, SCENEWARIO_HOLD_MUSIC_BOX);
	prologue_func_ae067(gb);

	set_obj_state(gb, wSceneObj1State, 0x0e);
	clear_obj_state(gb, wSceneObj8State);

	SET_HL(gb, wTempPals2);
	SET_DE(gb, wTempOBPals);
	gb->cpu.b = 1 * PAL_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, ADDR_Pals_869cd);
	prologue_func_ae0ab(gb);
}

// .configure_hdma_transfer: de = source, b = dest, c = length
static void prologue_configure_hdma(GB *gb)
{
	gb->cpu.a = BANK_TempleTilemap;
	wr8(gb, wHDMABank, gb->cpu.a);
	SET_HL(gb, wHDMA);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, wHDMA + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, wHDMA + 1, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wHDMA + 2, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wHDMA + 3, 0);
	gb->cpu.a = gb->cpu.c;
	SET_HL(gb, wHDMA + 4);
	wr8(gb, wHDMA + 4, gb->cpu.a);
	prologue_next(gb);
}

static void prologue_load_temple_tilemap(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wHDMADestVRAMBank, 0);
	SET_DE(gb, ADDR_TempleTilemap);
	gb->cpu.b = (v0BGMap0 >> 8) - 0x80;
	gb->cpu.c = 18 * 2 - 1; // 18 dma_rows
	prologue_configure_hdma(gb);
}

static void prologue_load_temple_attrmap(GB *gb)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, wHDMADestVRAMBank, 1);
	SET_DE(gb, ADDR_TempleAttrmap);
	gb->cpu.b = (v0BGMap0 >> 8) - 0x80;
	gb->cpu.c = 18 * 2 - 1; // 18 dma_rows
	prologue_configure_hdma(gb);
}

static void prologue_load_temple_gfx(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wHDMADestVRAMBank, 0);
	SET_DE(gb, ADDR_TempleGfx);
	gb->cpu.b = (v0Tiles2 >> 8) - 0x80;
	gb->cpu.c = 0x80 - 1; // $80 dma_tiles
	prologue_configure_hdma(gb);
}

static void prologue_prepare_temple_scene(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSCY, 0);
	wr8(gb, wSCX, 0);

	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, LCDC_BG_MAP);

	// place sleeping Wario at (64, 96)
	set_obj_coord(gb, wSceneWarioYCoord, 96, 64);
	set_wario_state(gb, SCENEWARIO_SLEEP);
	play_sfx(gb, SFX_SNORE);
	gb->cpu.a = 0x05;
	prologue_set_fade(gb);
}

static void prologue_func_ae15b(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d014), 0x07);
	if (gb->cpu.a)
		return;
	CALL_2B(PrologueFade);
}

static void prologue_func_ae164(GB *gb)
{
	gb->cpu.a = 0x02;
	prologue_set_fade(gb);
}

static void prologue_func_ae169(GB *gb)
{
	gb->cpu.c = 0x07;
	gb->cpu.a = rd8(gb, wPalConfig1TotalSteps);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C))
		gb->cpu.c = 0x01;
	gb->cpu.a = alu_and(gb, rd8(gb, w2d014), gb->cpu.c);
	if (gb->cpu.a)
		return;
	CALL_2B(PrologueFade);
}

static void prologue_wait30(GB *gb)
{
	gb->cpu.c = 30;
	prologue_delay(gb);
}

static void prologue_func_ae181(GB *gb)
{
	CALL_2B(Func_acaf3);
	// .Func_ae188
	gb->cpu.a = rd8(gb, wCurTextBufferPage);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_wario_state(gb, SCENEWARIO_LOOK_AROUND);
}

static void prologue_func_ae196(GB *gb)
{
	set_wario_state(gb, SCENEWARIO_NOD_ONCE);
	prologue_next(gb);
}

static void prologue_func_ae19e(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, SCENEWARIO_LOOK_BACK);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_obj_state(gb, wSceneObj1State, 0x01);
	prologue_next(gb);
}

static void prologue_func_ae1af(GB *gb)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	clear_obj_state(gb, wSceneObj1State);
	CALL_2B(ClearTextbox);
	prologue_next(gb);
}

static const Handler prologue_cutscene[] = {
	prologue_next,
	prologue_wait60,
	prologue_set_fade_in_from_black,
	NULL, // PrologueFade
	NULL, // Prologue_WarioFallFromPlane
	prologue_scroll_right_40_pixels,
	NULL, // Prologue_WarioWalkToCaveEntrance
	prologue_set_fade_to_black,
	NULL, // PrologueFade
	prologue_prepare_cave_scene1,
	NULL, // PrologueFade
	NULL, // Prologue_WarioEnterCave
	prologue_show_music_box_close_up,
	NULL, // Func_ae1de
	prologue_prepare_cave_scene2,
	NULL, // Prologue_WarioSuckedIntoMusicBox
	prologue_set_fade_to_black,
	NULL, // PrologueFade
	prologue_load_temple_tilemap,
	prologue_load_temple_attrmap,
	prologue_load_temple_gfx,
	prologue_prepare_temple_scene,
	prologue_func_ae15b,
	NULL, // Func_ae55c
	prologue_func_ae164,
	prologue_func_ae169,
	NULL, // Func_ae641
	NULL, // InitTextPrinting
	NULL, // Func_aca9e
	prologue_wait30,
	NULL, // LoadCurTextPageBufferPage
	NULL, // Func_acacd
	NULL, // Func_acae7
	prologue_func_ae181,
	prologue_func_ae196,
	prologue_func_ae19e,
	prologue_func_ae1af,
	NULL, // Func_ae668
	prologue_finish,
};

PORT_JUMP(Prologue)
{
	// call .PlayCutscene
	gb->cpu.a = rd8(gb, w2d013);
	if (!dispatch(gb, ADDR_Prologue + 17, prologue_cutscene, 39, ADDR_Prologue + 3))
		return;
	if (!call_ret(gb, ADDR_UpdatePrologueSceneObjects, ADDR_Prologue + 6) ||
		!call_ret(gb, ADDR_UpdateSceneWario, ADDR_Prologue + 9) ||
		!call_ret(gb, ADDR_Func_ac3fd, ADDR_Prologue + 12))
		return;
	RET();
}

// ---------------------------------------------------------------------------
// Func_ae1de

static void ae1de_wait20(GB *gb) { wait_frames(gb, 20); }
static void ae1de_wait10(GB *gb) { wait_frames(gb, 10); }
static void ae1de_wait90(GB *gb) { wait_frames(gb, 90); }

static void ae1de_func_ae1f5(GB *gb)
{
	// set Obj4's position to (56, 56)
	set_obj_coord(gb, wSceneObj4, 56, 56);
	set_obj_state(gb, wSceneObj4State, 0x0e);
	play_sfx(gb, SFX_104);
	advance_w2d890(gb);
}

static void ae1de_func_ae215(GB *gb)
{
	// set Obj5's position to (120, 48)
	set_obj_coord(gb, wSceneObj5, 48, 120);
	set_obj_state(gb, wSceneObj5State, 0x0e);
	advance_w2d890(gb);
}

static const Handler ae1de_table[] = {
	ae1de_wait20, ae1de_func_ae1f5, ae1de_wait10, ae1de_func_ae215, ae1de_wait90, func_ae1d5,
};

PORT_JUMP(Func_ae1de)
{
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Func_ae1de + 7, ae1de_table, 6);
}

// ---------------------------------------------------------------------------
// Prologue_WarioEnterCave

static void func_ae2f5(GB *gb)
{
	reset_counter_and_advance(gb, w2d8b1); // inc w2d8b0
}

static void func_ae2ed(GB *gb)
{
	set_obj_state(gb, wSceneObj1State, 0x0e);
	func_ae2f5(gb);
}

static void func_ae2fc(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneObj1State), rd8(gb, wSceneObj1State));
	if (gb->cpu.a)
		return;
	func_ae2f5(gb);
}

static void func_ae303(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneObj1State), rd8(gb, wSceneObj1State));
	if (gb->cpu.a)
		return;
	SET_HL(gb, w2d8b1);
	wr8(gb, w2d8b0, 0);
	wr8(gb, w2d8b1, 0);
}

static void func_ae30e(GB *gb)
{
	SET_HL(gb, w2d8b1);
	wr8(gb, w2d8b1, alu_inc(gb, rd8(gb, w2d8b1)));
	gb->cpu.a = rd8(gb, w2d8b1);
	alu_cp(gb, gb->cpu.a, 0x14);
	if (gb->cpu.f & FLAG_C)
		return;
	SET_HL(gb, w2d8b0);
	wr8(gb, w2d8b0, alu_dec(gb, rd8(gb, w2d8b0)));
	wr8(gb, w2d8b0, alu_dec(gb, rd8(gb, w2d8b0)));
}

static void func_ae2fb(GB *gb)
{
	(void)gb;
}

PORT(Func_ae2ed) { func_ae2ed(gb); }
PORT(Func_ae2f5) { func_ae2f5(gb); }
PORT(Func_ae2fb) { func_ae2fb(gb); }
PORT(Func_ae2fc) { func_ae2fc(gb); }
PORT(Func_ae303) { func_ae303(gb); }
PORT(Func_ae30e) { func_ae30e(gb); }

static const Handler ae2dd_table[] = {
	func_ae2fb, func_ae2ed, func_ae303, func_ae2ed, func_ae2fc, func_ae30e,
};

static bool func_ae2dd(GB *gb, uint16_t ret_addr)
{
	gb->cpu.a = rd8(gb, w2d8b0);
	return dispatch(gb, ADDR_Func_ae2dd + 4, ae2dd_table, 6, ret_addr);
}

PORT_JUMP(Func_ae2dd)
{
	if (func_ae2dd(gb, 0))
		RET();
}

static void wario_walk_right(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_WALK_RIGHT);
}

static void enter_cave_wario_move_right_to_64(GB *gb)
{
	CALL_2B(ApplySceneWarioMovementRight);
	alu_cp(gb, gb->cpu.a, 50);
	if (gb->cpu.f & FLAG_Z) {
		// .Func_ae26c
		gb->cpu.a = 0x01;
		wr8(gb, w2d8b0, 0x01);
		return;
	}
	alu_cp(gb, gb->cpu.a, 64);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, REG_HL(gb), gb->cpu.a);
	set_wario_state_and_advance(gb, SCENEWARIO_IDLE_RIGHT);
}

static void enter_cave_func_ae272(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d8b0), rd8(gb, w2d8b0));
	if (gb->cpu.a)
		return;
	advance_w2d890(gb);
}

// place emote on position (Wario_x + 16, Wario_y - 24)
static void show_emote(GB *gb)
{
	SET_HL(gb, wCurSceneObj);
	gb->cpu.a = alu_sub(gb, rd8(gb, wSceneWarioYCoord), 24);
	wr8(gb, wCurSceneObj, gb->cpu.a);
	SET_HL(gb, wCurSceneObj + 1);
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneWarioXCoord), 16);
	wr8(gb, wCurSceneObj + 1, gb->cpu.a);
	set_obj_state(gb, wCurSceneObjState, 0x11);
	play_sfx(gb, SFX_106);
	advance_w2d890(gb);
}

static void enter_cave_show_emote(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x16);
	if (gb->cpu.f & FLAG_C)
		return;
	show_emote(gb);
}

static void wait_emote_animation(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurSceneObjState), rd8(gb, wCurSceneObjState));
	if (gb->cpu.a)
		return;
	advance_w2d890(gb);
}

static void wait26(GB *gb) { wait_frames(gb, 26); }
static void wait30(GB *gb) { wait_frames(gb, 30); }
static void wait38(GB *gb) { wait_frames(gb, 38); }
static void wait40(GB *gb) { wait_frames(gb, 40); }
static void wait46(GB *gb) { wait_frames(gb, 46); }
static void wait60(GB *gb) { wait_frames(gb, 60); }
static void wait74(GB *gb) { wait_frames(gb, 74); }
static void wait120(GB *gb) { wait_frames(gb, 120); }

static void enter_cave_wario_move_right_to_102(GB *gb)
{
	CALL_2B(ApplySceneWarioMovementRight);
	alu_cp(gb, gb->cpu.a, 102);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, REG_HL(gb), gb->cpu.a);
	set_wario_state_and_advance(gb, SCENEWARIO_IDLE_RIGHT);
}

static void enter_cave_func_ae2b8(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x14);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x03;
		wr8(gb, w2d8b0, 0x03);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_C)
		return;
	advance_w2d890(gb);
}

static void enter_cave_wario_look_music_box(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_LOOK_MUSIC_BOX);
}

static void enter_cave_wario_hold_music_box(GB *gb)
{
	wait_wario_state(gb, SCENEWARIO_HOLD_MUSIC_BOX);
}

static const Handler enter_cave_table[] = {
	wario_walk_right,
	enter_cave_wario_move_right_to_64,
	enter_cave_func_ae272,
	enter_cave_show_emote,
	wait_emote_animation,
	wait26,
	wario_walk_right,
	enter_cave_wario_move_right_to_102,
	enter_cave_func_ae2b8,
	enter_cave_wario_look_music_box,
	enter_cave_wario_hold_music_box,
	wait60,
	func_ae1d5,
};

PORT_JUMP(Prologue_WarioEnterCave)
{
	if (!func_ae2dd(gb, ADDR_Prologue_WarioEnterCave + 3))
		return;
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Prologue_WarioEnterCave + 10, enter_cave_table, 13);
}

// ---------------------------------------------------------------------------
// Prologue_WarioSuckedIntoMusicBox

#define SUCKED_ADDR(local) (ADDR_Prologue_WarioSuckedIntoMusicBox + ((local) - 0x631c))

static void sucked_func_ae352(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneObj1State), rd8(gb, wSceneObj1State));
	if (gb->cpu.a)
		return;
	advance_w2d890(gb);
}

static void sucked_func_ae35a(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x14);
	if (gb->cpu.f & FLAG_C)
		return;
	stop_music2(gb);
	advance_w2d890(gb);
}

static void sucked_wario_pickup_music_box(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_PICKUP_MUSIC_BOX);
}

static void sucked_func_ae37c(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C))
		return;
	set_obj_state(gb, wSceneObj3State, 0x0a);
	advance_w2d890(gb);
}

static void sucked_wario_short_hop_back(GB *gb)
{
	SET_HL(gb, wSceneWario);
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x01);
	bool up = gb->cpu.f & FLAG_Z;
	bool down = false;
	if (!up) {
		alu_cp(gb, gb->cpu.a, 0x05);
		down = gb->cpu.f & FLAG_Z;
	}
	if (up || down) {
		uint8_t y = rd8(gb, REG_HL(gb));
		wr8(gb, REG_HL(gb), up ? alu_dec(gb, y) : alu_inc(gb, y)); // y
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		wr8(gb, REG_HL(gb), alu_dec(gb, rd8(gb, REG_HL(gb)))); // x
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x30);
	if (gb->cpu.f & FLAG_C)
		return;
	advance_w2d890(gb);
}

static void sucked_music_box_rise(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d891), 0x07);
	if (gb->cpu.a)
		return;
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_dec(gb, rd8(gb, wSceneObj3YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3YCoord);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 8);
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
	alu_cp(gb, gb->cpu.a, 180);
	if (gb->cpu.f & FLAG_Z) {
		set_obj_state(gb, wSceneObj2State, 0x0c);
		gb->cpu.a = 0x01;
		wr8(gb, w2d150, 0x01);
		return;
	}
	alu_cp(gb, gb->cpu.a, 165);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

static void sucked_func_ae3d8(GB *gb)
{
	clear_obj_state(gb, wSceneObj2State);
	advance_w2d890(gb);
}

static void sucked_func_ae3e2(GB *gb)
{
	// place obj at (Wario_x, Wario_y - 8)
	SET_HL(gb, wCurSceneObj);
	gb->cpu.a = alu_sub(gb, rd8(gb, wSceneWarioYCoord), 8);
	wr8(gb, wCurSceneObj, gb->cpu.a);
	SET_HL(gb, wCurSceneObj + 1);
	gb->cpu.a = rd8(gb, wSceneWarioXCoord);
	wr8(gb, wCurSceneObj + 1, gb->cpu.a);
	set_obj_state(gb, wCurSceneObjState, 0x0e);
	advance_w2d890(gb);
}

static void sucked_func_ae3fa(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurSceneObjState), rd8(gb, wCurSceneObjState));
	if (gb->cpu.a)
		return;
	gb->cpu.a = 0x80;
	wr8(gb, w2d150, 0x80);
	advance_w2d890(gb);
}

static void sucked_func_ae407(GB *gb)
{
	SET_HL(gb, wCurSceneObj);
	gb->cpu.a = alu_sub(gb, rd8(gb, wSceneWarioYCoord), 16);
	wr8(gb, wCurSceneObj, gb->cpu.a);
	SET_HL(gb, wCurSceneObj + 1);
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneWarioXCoord), 2);
	wr8(gb, wCurSceneObj + 1, gb->cpu.a);
	set_obj_state(gb, wCurSceneObjState, 0x15);

	// sets OB palette 0 to all (28, 28, 28)
	const uint16_t color = (28 << B_COLOR_RED) + (28 << B_COLOR_GREEN) + (28 << B_COLOR_BLUE);
	uint16_t hl = wTempOBPals;
	gb->cpu.b = PAL_COLORS;
	do {
		gb->cpu.a = (uint8_t)color;
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = (uint8_t)(color >> 8);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);

	SET_DE(gb, ADDR_Data_ac9ff);
	CALL_2B(Func_ac9e4);
	advance_w2d890(gb);
}

static void sucked_func_ae435(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurSceneObjState);
	alu_cp(gb, gb->cpu.a, 0x16);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // SCENEWARIO_NONE
	CALL_2B(SetSceneWarioState);
	advance_w2d890(gb);
}

static void sucked_func_ae447(GB *gb)
{
	SET_HL(gb, wCurSceneObj + 1);
	gb->cpu.a = rd8(gb, wCurSceneObj); // y
	alu_cp(gb, gb->cpu.a, 181);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = rd8(gb, w2d891);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
		if (gb->cpu.a)
			return;
		if (alu_bit(gb, gb->cpu.b, 2))
			wr8(gb, REG_HL(gb), alu_inc(gb, rd8(gb, REG_HL(gb)))); // x
		gb->cpu.l = alu_dec(gb, gb->cpu.l);
		wr8(gb, REG_HL(gb), alu_dec(gb, rd8(gb, REG_HL(gb)))); // y
		return;
	}
	// .above_181
	wr8(gb, REG_HL(gb), alu_inc(gb, rd8(gb, REG_HL(gb)))); // x
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	wr8(gb, REG_HL(gb), alu_dec(gb, rd8(gb, REG_HL(gb)))); // y
	wr8(gb, REG_HL(gb), alu_dec(gb, rd8(gb, REG_HL(gb))));
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, 168);
	if (gb->cpu.f & FLAG_Z) {
		set_obj_state(gb, wCurSceneObjState, 0x17);
		return;
	}
	alu_cp(gb, gb->cpu.a, 156);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

static void sucked_func_ae476(GB *gb)
{
	set_obj_state(gb, wSceneObj3State, 0x09);
	clear_obj_state(gb, wCurSceneObjState);
	advance_w2d890(gb);
}

static void sucked_func_ae488(GB *gb)
{
	gb->cpu.a = alu_srl(gb, rd8(gb, w2d891));
	if (gb->cpu.f & FLAG_C)
		return;
	SET_HL(gb, wSceneObj3XCoord);
	alu_cp(gb, gb->cpu.a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, wSceneObj3XCoord, alu_inc(gb, rd8(gb, wSceneObj3XCoord)));
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, wSceneObj3XCoord, alu_dec(gb, rd8(gb, wSceneObj3XCoord)));
		wr8(gb, wSceneObj3XCoord, alu_dec(gb, rd8(gb, wSceneObj3XCoord)));
		return;
	}
	wr8(gb, wSceneObj3XCoord, alu_inc(gb, rd8(gb, wSceneObj3XCoord)));
	advance_w2d890(gb);
}

static void play_bump_sfx(GB *gb)
{
	play_sfx(gb, SFX_BUMP);
}

static void sucked_func_ae4a9(GB *gb)
{
	// call .Func_ae4c0 with hl = w2d891
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x14);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x10;
		wr8(gb, w2d891, 0x10);
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, SUCKED_ADDR(0x64d1), REG_DE(gb)); // .data_ae4d1
	gb->cpu.a = rd8(gb, hl);

	SET_HL(gb, wSceneObj3YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj3YCoord));
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 195);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 195;
	wr8(gb, wSceneObj3YCoord, 195);
	play_bump_sfx(gb);
	advance_w2d890(gb);
}

static void sucked_func_ae4e5(GB *gb)
{
	// call .Func_ae4fe with hl = w2d891
	gb->cpu.a = rd8(gb, w2d891);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, SUCKED_ADDR(0x6509), REG_DE(gb)); // .data_ae509
	gb->cpu.a = rd8(gb, hl);
	SET_HL(gb, hl + 1);
	gb->cpu.b = rd8(gb, REG_HL(gb));

	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	SET_HL(gb, wSceneObj3YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj3YCoord));
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 195);
	if (gb->cpu.f & FLAG_Z)
		play_bump_sfx(gb);
	gb->cpu.a = 0x80;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

static const Handler sucked_table[] = {
	sucked_func_ae352,
	sucked_func_ae35a,
	wait30,
	sucked_wario_pickup_music_box,
	sucked_func_ae37c,
	sucked_wario_short_hop_back,
	sucked_music_box_rise,
	wait120,
	sucked_func_ae3d8,
	sucked_func_ae3e2,
	sucked_func_ae3fa,
	sucked_func_ae407,
	sucked_func_ae435,
	wait60,
	sucked_func_ae447,
	sucked_func_ae476,
	sucked_func_ae488,
	wait40,
	sucked_func_ae4a9,
	sucked_func_ae4e5,
	wait120,
	func_ae1d5,
};

static void func_ae523(GB *gb)
{
	SET_HL(gb, w2d150);
	gb->cpu.a = rd8(gb, w2d150);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wSceneWarioAttributes, 0);
		wr8(gb, w2d150, 0);
		return;
	}
	SET_HL(gb, w2d151);
	wr8(gb, w2d151, alu_inc(gb, rd8(gb, w2d151)));
	gb->cpu.a = rd8(gb, w2d151);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, w2d151, 0);
	gb->cpu.c = rd8(gb, w2d150);
	wr8(gb, w2d150, alu_inc(gb, rd8(gb, w2d150)));
	gb->cpu.a = rd8(gb, w2d150);
	alu_cp(gb, gb->cpu.a, 0x09);
	if (!(gb->cpu.f & FLAG_C))
		wr8(gb, w2d150, 0x01);
	gb->cpu.b = 0;
	SET_HL(gb, alu_add16(gb, ADDR_Func_ae523 + (0x654b - 0x6523), REG_BC(gb))); // .data_ae54b
	gb->cpu.a = rd8(gb, REG_HL(gb));
	wr8(gb, wSceneWarioAttributes, gb->cpu.a);
}

PORT(Func_ae523)
{
	func_ae523(gb);
}

PORT_JUMP(Prologue_WarioSuckedIntoMusicBox)
{
	func_ae523(gb);
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Prologue_WarioSuckedIntoMusicBox + 10, sucked_table, 22);
}

// ---------------------------------------------------------------------------
// Func_ae55c, Func_ae641, Func_ae668: bc = w2d891

// ld a, [bc] / cp n: returns true (and the asm returns) if a < n
static bool w2d891_below(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

static void func_ae585(GB *gb)
{
	gb->cpu.a = alu_inc(gb, rd8(gb, w2d14e));
	if (gb->cpu.a)
		return;
	advance_w2d890(gb);
}

static void func_ae58d(GB *gb)
{
	if (w2d891_below(gb, 0x4b))
		return;
	set_wario_state_and_advance(gb, SCENEWARIO_WAKE_UP);
}

static void func_ae599(GB *gb)
{
	if (w2d891_below(gb, 0x14))
		return;
	play_sfx(gb, SFX_GRAB);
	advance_w2d890(gb);
}

static void func_ae5a8(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, SCENEWARIO_SIT_IDLE);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

static void func_ae5b1(GB *gb)
{
	if (w2d891_below(gb, 0x05))
		return;
	gb->cpu.a = alu_sub(gb, rd8(gb, wSceneWarioYCoord), 0x01);
	wr8(gb, wCurSceneObjYCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneWarioXCoord), 0x09);
	wr8(gb, wCurSceneObjXCoord, gb->cpu.a);
	set_obj_state(gb, wCurSceneObjState, 0x14);
	advance_w2d890(gb);
}

static void func_ae5d0(GB *gb)
{
	if (w2d891_below(gb, 0x8c))
		return;
	set_wario_state_and_advance(gb, SCENEWARIO_SIT_STILL);
}

static void func_ae5dc(GB *gb)
{
	if (w2d891_below(gb, 0x10))
		return;
	set_wario_state(gb, SCENEWARIO_IDLE_RIGHT);
	play_sfx(gb, SFX_12A);
	advance_w2d890(gb);
}

static void func_ae5fd(GB *gb)
{
	CALL_2B(ApplySceneWarioMovementRight);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, REG_HL(gb), gb->cpu.a);
	set_wario_state_and_advance(gb, SCENEWARIO_IDLE_RIGHT);
}

static void func_ae60c(GB *gb)
{
	if (w2d891_below(gb, 0x1e))
		return;
	set_wario_state_and_advance(gb, SCENEWARIO_TURN_BACK_RIGHT);
}

static void wait_look_back(GB *gb)
{
	wait_wario_state(gb, SCENEWARIO_LOOK_BACK);
}

static void func_ae61d(GB *gb)
{
	if (w2d891_below(gb, 0x1e))
		return;
	play_music2(gb, MUSIC_THE_TEMPLE);
	advance_w2d890(gb);
}

static void func_ae630(GB *gb)
{
	if (w2d891_below(gb, 0x1e))
		return;
	set_wario_state_and_advance(gb, SCENEWARIO_LOOK_AROUND);
}

PORT(Func_ae585) { func_ae585(gb); }
PORT(Func_ae58d) { func_ae58d(gb); }
PORT(Func_ae599) { func_ae599(gb); }
PORT(Func_ae5a8) { func_ae5a8(gb); }
PORT(Func_ae5b1) { func_ae5b1(gb); }
PORT(Func_ae5d0) { func_ae5d0(gb); }
PORT(Func_ae5dc) { func_ae5dc(gb); }
PORT(Func_ae5f0) { wait60(gb); }
PORT(Func_ae5f5) { wario_walk_right(gb); }
PORT(Func_ae5fd) { func_ae5fd(gb); }
PORT(Func_ae60c) { func_ae60c(gb); }
PORT(Func_ae618) { wait_look_back(gb); }
PORT(Func_ae61d) { func_ae61d(gb); }
PORT(Func_ae630) { func_ae630(gb); }
PORT(Func_ae63c) { wait30(gb); }

static const Handler ae55c_table[] = {
	func_ae585, func_ae58d, func_ae599, func_ae5a8, func_ae5b1, func_ae5d0, func_ae5dc, wait60,
	wario_walk_right, func_ae5fd, func_ae60c, wait_look_back, func_ae61d, func_ae630, wait30, func_ae1d5,
};

PORT_JUMP(Func_ae55c)
{
	inc_w2d891_ld_bc_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Func_ae55c + 9, ae55c_table, 16);
}

static void func_ae652(GB *gb)
{
	wait_wario_state(gb, SCENEWARIO_LOOK_FORWARD);
}

static void func_ae657(GB *gb)
{
	set_wario_state(gb, SCENEWARIO_JUMP_AND_BOW);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneWarioUnk7, 0);
	advance_w2d890(gb);
}

PORT(Func_ae652) { func_ae652(gb); }
PORT(Func_ae657) { func_ae657(gb); }
PORT(Func_ae663) { wait_look_back(gb); }

static const Handler ae641_table[] = {
	func_ae652, func_ae657, wait_look_back, func_ae1d5,
};

PORT_JUMP(Func_ae641)
{
	inc_w2d891_ld_bc_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Func_ae641 + 9, ae641_table, 4);
}

static void func_ae683(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_SNAP_FINGERS);
}

static void func_ae68b(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioFrame);
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_106);
	advance_w2d890(gb);
}

static void func_ae6a6(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_TURN_BACK_TO_LEFT);
}

static void func_ae6ae(GB *gb)
{
	wait_wario_state(gb, SCENEWARIO_IDLE_LEFT);
}

static void func_ae6b3(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_C)
		return;
	set_wario_state_and_advance(gb, SCENEWARIO_WALK_LEFT);
}

static void func_ae6c1(GB *gb)
{
	CALL_2B(ApplySceneWarioMovementLeft);
	alu_cp(gb, gb->cpu.a, 0xf0);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // SCENEWARIO_NONE
	CALL_2B(SetSceneWarioState);
	advance_w2d890(gb);
}

PORT(Func_ae683) { func_ae683(gb); }
PORT(Func_ae68b) { func_ae68b(gb); }
PORT(Func_ae69c) { wait_look_back(gb); }
PORT(Func_ae6a1) { wait30(gb); }
PORT(Func_ae6a6) { func_ae6a6(gb); }
PORT(Func_ae6ae) { func_ae6ae(gb); }
PORT(Func_ae6b3) { func_ae6b3(gb); }
PORT(Func_ae6c1) { func_ae6c1(gb); }
PORT(Func_ae6ce) { wait30(gb); }

static const Handler ae668_table[] = {
	func_ae683, func_ae68b, wait_look_back, wait30, func_ae6a6,
	func_ae6ae, func_ae6b3, func_ae6c1, wait30, func_ae1d5,
};

PORT_JUMP(Func_ae668)
{
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Func_ae668 + 7, ae668_table, 10);
}
