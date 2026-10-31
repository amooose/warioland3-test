// Port of src/engine/overworld/bank2b_2.asm
#include "ported.h"

typedef void (*Handler)(GB *gb);

// Set when native code has handed execution back to the asm in the middle
// of a routine (see dispatch()): every native caller up to the hook must
// return at once without touching anything. Handlers return void, so they
// and dispatch() check this after nested calls. Each hook that can set it
// clears it when it returns (PORT_NESTED), so a hook run from inside
// another one's native code doesn't leave it set for the outer one.
static bool g_stop;

// A PORT_JUMP whose native code may stop early (g_stop).
#define PORT_NESTED(name) \
	static void body_##name(GB *gb); \
	PORT_JUMP(name) \
	{ \
		g_stop = false; \
		body_##name(gb); \
		g_stop = false; \
	} \
	static void body_##name(GB *gb)

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
	if (escaped)
		g_stop = true;
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

// Native subroutines that can be called from other native code take the
// return address of the asm `call` (ret_addr), or 0 when they run as the
// hook itself. They keep the stack exactly as the asm has it: enter() is
// the call, leave() the ret. They return false when execution has to go on
// in the asm (see dispatch()); every native caller then stops at once.
static void enter(GB *gb, uint16_t ret_addr)
{
	if (ret_addr)
		gb_push16(gb, ret_addr);
}

static bool leave(GB *gb, uint16_t ret_addr)
{
	if (ret_addr)
		gb_pop16(gb);
	else
		RET();
	return true;
}

// A jumptable with n entries, in tail position of a subroutine entered with
// ret_addr. handlers[i] is the C version of entry i (it runs like a call of
// that entry, up to its ret; the caller then does leave()); NULL means the
// entry is run as is. Returns false if execution must continue in the asm
// at pc, with the stack as the asm has it: an index out of range (which
// jumps to garbage), an entry run as is from the hook itself, or one that
// pops past its frame.
static bool dispatch(GB *gb, uint16_t table, const Handler *handlers, int n, uint16_t ret_addr)
{
	uint16_t sp = gb->cpu.sp;
	uint16_t t = jumptable(gb, table);
	int idx = gb->cpu.e >> 1;
	if (idx < n && handlers[idx]) {
		handlers[idx](gb);
		return !g_stop;
	}
	if (idx < n && ret_addr) {
		uint16_t pc = gb->cpu.pc;
		gb->cpu.pc = t;
		if (!run_until(gb, ret_addr, (uint16_t)(sp + 2)))
			return false;
		gb_push16(gb, ret_addr); // back as it was (same bytes), for leave()
		gb->cpu.pc = pc;
		return true;
	}
	JUMP_TO(t);
	g_stop = true;
	return false;
}

// A hook whose tail is a jumptable (PORT_JUMP).
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

// ld a, bank / ld [wHDMABank], a / ld hl, wHDMA / ld a, d / ld [hli], a /
// ld a, e / ld [hli], a / ld a, b / ld [hli], a / xor a / ld [hli], a /
// ld a, c / ld [hl], a / jp Prologue_Next (or Func_af135, the same)
// de = source, b = dest, c = length
static void configure_hdma(GB *gb, uint8_t bank)
{
	gb->cpu.a = bank;
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
	configure_hdma(gb, BANK_TempleTilemap);
}

static void prologue_load_temple_attrmap(GB *gb)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, wHDMADestVRAMBank, 1);
	SET_DE(gb, ADDR_TempleAttrmap);
	gb->cpu.b = (v0BGMap0 >> 8) - 0x80;
	gb->cpu.c = 18 * 2 - 1; // 18 dma_rows
	configure_hdma(gb, BANK_TempleTilemap);
}

static void prologue_load_temple_gfx(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wHDMADestVRAMBank, 0);
	SET_DE(gb, ADDR_TempleGfx);
	gb->cpu.b = (v0Tiles2 >> 8) - 0x80;
	gb->cpu.c = 0x80 - 1; // $80 dma_tiles
	configure_hdma(gb, BANK_TempleTilemap);
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

PORT_NESTED(Prologue)
{
	// call .PlayCutscene
	enter(gb, ADDR_Prologue + 3);
	gb->cpu.a = rd8(gb, w2d013);
	if (!dispatch(gb, ADDR_Prologue + 17, prologue_cutscene, 39, ADDR_Prologue + 3))
		return;
	leave(gb, ADDR_Prologue + 3);
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

PORT_NESTED(Func_ae1de)
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
	enter(gb, ret_addr);
	gb->cpu.a = rd8(gb, w2d8b0);
	if (!dispatch(gb, ADDR_Func_ae2dd + 4, ae2dd_table, 6, ret_addr))
		return false;
	return leave(gb, ret_addr);
}

PORT_NESTED(Func_ae2dd)
{
	func_ae2dd(gb, 0);
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

PORT_NESTED(Prologue_WarioEnterCave)
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

PORT_NESTED(Prologue_WarioSuckedIntoMusicBox)
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

PORT_NESTED(Func_ae55c)
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

PORT_NESTED(Func_ae641)
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

PORT_NESTED(Func_ae668)
{
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Func_ae668 + 7, ae668_table, 10);
}

// ---------------------------------------------------------------------------
// Prologue_WarioFallFromPlane

static void fall_show_wario_fall(GB *gb)
{
	// set Wario's coordinates to (240, 60)
	set_obj_coord(gb, wSceneWarioYCoord, 60, 240);
	set_wario_state(gb, SCENEWARIO_ON_FIRE);
	play_sfx(gb, SFX_THROW);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, w2d894, 0);
	wr8(gb, w2d892, 0);
	advance_w2d890(gb);
}

// .asm_ae768 (and .asm_ae7c5's tail)
static void fall_next_bounce_step(GB *gb)
{
	SET_HL(gb, w2d892);
	wr8(gb, w2d892, alu_inc(gb, rd8(gb, w2d892)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, w2d893, 0);
}

static void fall_play_bump_sfx_and_show_obj(GB *gb, bool show_obj)
{
	play_bump_sfx(gb);
	if (show_obj) {
		// .asm_ae77f
		SET_HL(gb, wCurSceneObj);
		gb->cpu.a = rd8(gb, wSceneWarioYCoord);
		wr8(gb, wCurSceneObj, gb->cpu.a);
		SET_HL(gb, wCurSceneObj + 1);
		gb->cpu.a = rd8(gb, wSceneWarioXCoord);
		wr8(gb, wCurSceneObj + 1, gb->cpu.a);
		set_obj_state(gb, wCurSceneObjState, 0x10);
	}
	fall_next_bounce_step(gb);
}

// returns true if Wario's y is 96
static bool fall_wario_landed(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioYCoord);
	alu_cp(gb, gb->cpu.a, 96);
	return gb->cpu.f & FLAG_Z;
}

// .asm_ae794
static void fall_wait_4(GB *gb)
{
	SET_HL(gb, w2d893);
	wr8(gb, w2d893, alu_inc(gb, rd8(gb, w2d893)));
	gb->cpu.a = rd8(gb, w2d893);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_C)
		return;
	fall_next_bounce_step(gb);
}

static void fall_do_bounce(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d894), rd8(gb, w2d894));
	if (gb->cpu.a) {
		gb->cpu.a = alu_and(gb, rd8(gb, w2d891), 0x03);
		if (gb->cpu.a) {
			SET_DE(gb, ADDR_Data_aed06);
			SET_HL(gb, wSceneWario);
			CALL(ApplyOWMovement);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z;
				wr8(gb, w2d894, 0);
			}
		}
	}
	// .asm_ae740
	gb->cpu.a = rd8(gb, w2d892);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a) {
		// .asm_ae763
		gb->cpu.a = 0x01;
		wr8(gb, w2d894, 0x01);
		fall_next_bounce_step(gb);
		return;
	}
	int step;
	for (step = 1; step <= 9; step++) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (!gb->cpu.a)
			break;
	}
	switch (step) {
	case 1: // .asm_ae771
		if (!fall_wario_landed(gb))
			return;
		set_wario_state(gb, SCENEWARIO_STUNNED_RIGHT);
		fall_play_bump_sfx_and_show_obj(gb, true);
		return;
	case 2:
	case 5:
		fall_wait_4(gb);
		return;
	case 3: // .asm_ae79e
		set_wario_state(gb, SCENEWARIO_CRASH_TUMBLE_END);
		fall_next_bounce_step(gb);
		return;
	case 4: // .asm_ae7a5
		if (!fall_wario_landed(gb))
			return;
		set_wario_state(gb, SCENEWARIO_CRASH_LAND_RIGHT);
		fall_play_bump_sfx_and_show_obj(gb, true);
		return;
	case 6: // .asm_ae7b5
		if (!fall_wario_landed(gb))
			return;
		set_wario_state(gb, SCENEWARIO_CRASH_LAND_RIGHT);
		fall_play_bump_sfx_and_show_obj(gb, false);
		return;
	case 7: // .asm_ae7c5
		gb->cpu.a = rd8(gb, wSceneWarioState);
		alu_cp(gb, gb->cpu.a, SCENEWARIO_STUNNED_RIGHT);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		fall_next_bounce_step(gb);
		return;
	case 8: // .asm_ae7d4
		SET_HL(gb, w2d893);
		wr8(gb, w2d893, alu_inc(gb, rd8(gb, w2d893)));
		gb->cpu.a = rd8(gb, w2d893);
		alu_cp(gb, gb->cpu.a, 0x3e);
		if (gb->cpu.f & FLAG_C)
			return;
		set_wario_state(gb, SCENEWARIO_IDLE_RIGHT);
		play_sfx(gb, SFX_12A);
		fall_next_bounce_step(gb);
		return;
	case 9: // .asm_ae7f2
		gb->cpu.a = 0x80;
		wr8(gb, w2d892, 0x80);
		return;
	default:
		return;
	}
}

static void fall_wario_bounce(GB *gb)
{
	fall_do_bounce(gb);
	gb->cpu.a = rd8(gb, w2d892);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

static void fall_wario_turn_right(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_TURN_BACK_RIGHT);
}

static void fall_wario_look_around(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_LOOK_AROUND_THEN_TURN);
}

static void wait_idle_right(GB *gb)
{
	wait_wario_state(gb, SCENEWARIO_IDLE_RIGHT);
}

static const Handler fall_table[] = {
	wait74,
	fall_show_wario_fall,
	fall_wario_bounce,
	wait60,
	fall_wario_turn_right,
	wait_look_back,
	wait38,
	fall_wario_look_around,
	wait_idle_right,
	wait46,
	func_ae1d5,
};

static bool func_ae8bb(GB *gb, uint16_t ret_addr);

PORT_NESTED(Prologue_WarioFallFromPlane)
{
	if (!func_ae8bb(gb, ADDR_Prologue_WarioFallFromPlane + 3))
		return;
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Prologue_WarioFallFromPlane + 10, fall_table, 11);
}

// ---------------------------------------------------------------------------
// Prologue_WarioWalkToCaveEntrance

static void walk_wario_move_right_to_120(GB *gb)
{
	CALL_2B(ApplySceneWarioMovementRight);
	alu_cp(gb, gb->cpu.a, 120);
	if (!(gb->cpu.f & FLAG_Z))
		return; // if != 120 keep walking
	wr8(gb, REG_HL(gb), gb->cpu.a);
	set_wario_state_and_advance(gb, SCENEWARIO_IDLE_RIGHT);
}

static void walk_wario_look_up(GB *gb)
{
	set_wario_state_and_advance(gb, SCENEWARIO_LOOK_UP);
}

static void walk_wario_move_right_to_208(GB *gb)
{
	CALL_2B(ApplySceneWarioMovementRight);
	alu_cp(gb, gb->cpu.a, 208);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, REG_HL(gb), gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // SCENEWARIO_NONE
	CALL_2B(SetSceneWarioState);
	advance_w2d890(gb);
}

static const Handler walk_table[] = {
	wait60,
	wario_walk_right,
	walk_wario_move_right_to_120,
	wait60,
	walk_wario_look_up,
	wait_idle_right,
	show_emote,
	wait_emote_animation,
	wait40,
	wario_walk_right,
	walk_wario_move_right_to_208,
	wait40,
	func_ae1d5,
};

PORT_NESTED(Prologue_WarioWalkToCaveEntrance)
{
	inc_w2d891_ld_a_w2d890(gb);
	tail_dispatch(gb, ADDR_Prologue_WarioWalkToCaveEntrance + 7, walk_table, 13);
}

// ---------------------------------------------------------------------------
// The plane parts (Func_ae8bb)

// hl = counter: xor a / ld [hld], a / inc [hl]
static void func_ae915(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, hl, 0);
	hl--;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

PORT(Func_ae915)
{
	func_ae915(gb);
}

// d = y, e = x, c = new state, hl = obj pointer
static void set_scene_obj_coord_and_state(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.l |= 1 << 2; // set 2, l
	gb->cpu.a = gb->cpu.c;
	CALL(SetSceneObjState);
}

PORT(SetSceneObjCoordAndState)
{
	set_scene_obj_coord_and_state(gb);
}

// ld hl, obj / lb de, y, x / ld c, state / call SetSceneObjCoordAndState
static void set_coord_and_state(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint8_t state)
{
	SET_HL(gb, obj);
	gb->cpu.d = y;
	gb->cpu.e = x;
	gb->cpu.c = state;
	set_scene_obj_coord_and_state(gb);
}

// ld de, data / ld hl, obj / call ApplyOWMovement
static void apply_movement(GB *gb, uint16_t obj, uint16_t data)
{
	SET_DE(gb, data);
	SET_HL(gb, obj);
	CALL(ApplyOWMovement);
}

static void part_next(GB *gb, uint16_t counter)
{
	SET_HL(gb, counter);
	func_ae915(gb);
}

static void part_sfx_next(GB *gb, uint16_t counter)
{
	play_sfx(gb, SFX_006);
	part_next(gb, counter);
}

// c = number of frames
static void part_wait(GB *gb, uint16_t counter, uint8_t frames)
{
	gb->cpu.c = frames;
	SET_HL(gb, counter);
	gb->cpu.a = rd8(gb, counter);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	func_ae915(gb);
}

static void part_wait_land(GB *gb, uint16_t y_coord, uint16_t action)
{
	gb->cpu.a = rd8(gb, y_coord);
	alu_cp(gb, gb->cpu.a, 0x62);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_006);
	SET_HL(gb, action);
	wr8(gb, action, alu_inc(gb, rd8(gb, action)));
}

static void part_end(GB *gb, uint16_t action, uint16_t obj_state)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, action, 0);
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

// copy src's coordinates (y first) to dst and set dst's state to $0f
static void part_attach(GB *gb, uint16_t src, uint16_t dst, uint16_t dst_state, uint16_t counter)
{
	gb->cpu.a = rd8(gb, src);
	wr8(gb, dst, gb->cpu.a);
	gb->cpu.a = rd8(gb, src + 1);
	wr8(gb, dst + 1, gb->cpu.a);
	set_obj_state(gb, dst_state, 0x0f);
	part_next(gb, counter);
}

// copy src's coordinates (x first) to dst; when they land, set dst's state
// to $10
static void part_attached_land(GB *gb, uint16_t src, uint16_t dst, uint16_t dst_state, uint16_t action)
{
	gb->cpu.a = rd8(gb, src + 1);
	wr8(gb, dst + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, src);
	wr8(gb, dst, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x62);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_obj_state(gb, dst_state, 0x10);
	play_sfx(gb, SFX_006);
	SET_HL(gb, action);
	wr8(gb, action, alu_inc(gb, rd8(gb, action)));
}

// Plane part 1: Func_ae924

#define P1C wPlanePart1Counter
#define P1A wPlanePart1Action

static void func_ae9d0(GB *gb) { apply_movement(gb, wSceneObj3, ADDR_Data_aee09); }
static void func_ae9da(GB *gb) { apply_movement(gb, wSceneObj3, ADDR_Data_aedba); }
static void func_ae9e4(GB *gb) { apply_movement(gb, wSceneObj3, ADDR_Data_aed6d); }

PORT(Func_ae9d0) { func_ae9d0(gb); }
PORT(Func_ae9da) { func_ae9da(gb); }
PORT(Func_ae9e4) { func_ae9e4(gb); }

static void p1_ae971(GB *gb)
{
	set_coord_and_state(gb, wSceneObj3, 46, 240, 0x06);
	part_next(gb, P1C);
}
static void p1_ae97e(GB *gb)
{
	func_ae9d0(gb);
	part_wait_land(gb, wSceneObj3YCoord, P1A);
}
static void p1_ae983(GB *gb)
{
	func_ae9d0(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P1C);
}
static void p1_ae989(GB *gb) { part_wait(gb, P1C, 0x08); }
static void p1_ae98d(GB *gb)
{
	set_coord_and_state(gb, wSceneObj3, 77, 224, 0x05);
	part_next(gb, P1C);
}
static void p1_ae99a(GB *gb)
{
	func_ae9da(gb);
	part_wait_land(gb, wSceneObj3YCoord, P1A);
}
static void p1_ae99f(GB *gb)
{
	func_ae9da(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P1C);
}
static void p1_ae9a5(GB *gb) { part_wait(gb, P1C, 0x28); }
static void p1_ae9a9(GB *gb)
{
	set_coord_and_state(gb, wSceneObj3, 62, 240, 0x04);
	part_next(gb, P1C);
}
static void p1_ae9b6(GB *gb)
{
	func_ae9e4(gb);
	part_wait_land(gb, wSceneObj3YCoord, P1A);
}
static void p1_ae9bb(GB *gb)
{
	func_ae9e4(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P1C);
}
static void p1_ae9c1(GB *gb) { part_wait(gb, P1C, 0x08); }
static void p1_ae9c5(GB *gb) { part_end(gb, P1A, wSceneObj3State); }

static const Handler p1_table[] = {
	p1_ae971, p1_ae97e, p1_ae983, p1_ae989, p1_ae98d, p1_ae99a, p1_ae99f,
	p1_ae9a5, p1_ae9a9, p1_ae9b6, p1_ae9bb, p1_ae9c1, p1_ae9c5,
};

// ld hl, counter / inc [hl] / dec l / ld a, [hl] / and a / ret z / dec a /
// jumptable
static bool plane_part(GB *gb, uint16_t routine, uint16_t counter, const Handler *table, int n,
	uint16_t ret_addr)
{
	enter(gb, ret_addr);
	SET_HL(gb, counter);
	wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = rd8(gb, REG_HL(gb)); // action
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return leave(gb, ret_addr);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (!dispatch(gb, routine + 10, table, n, ret_addr))
		return false;
	return leave(gb, ret_addr);
}

static bool func_ae924(GB *gb, uint16_t ret_addr)
{
	return plane_part(gb, ADDR_Func_ae924, P1C, p1_table, 13, ret_addr);
}

PORT_NESTED(Func_ae924)
{
	func_ae924(gb, 0);
}

// Plane part 2: Func_ae9ee

#define P2C wPlanePart2Counter
#define P2A wPlanePart2Action

static void p2_aeaef(GB *gb) { apply_movement(gb, wSceneObj4, ADDR_Data_aed6d); }
static void p2_aeaf9(GB *gb) { apply_movement(gb, wSceneObj4, ADDR_Data_aee09); }

static void p2_attached_land(GB *gb)
{
	part_attached_land(gb, wSceneObj4, wSceneObj1, wSceneObj1State, P2A);
}
static void p2_attach(GB *gb)
{
	part_attach(gb, wSceneObj4, wSceneObj1, wSceneObj1State, P2C);
}

static void p2_aea40(GB *gb)
{
	set_coord_and_state(gb, wSceneObj4, 62, 240, 0x02);
	clear_obj_state(gb, wSceneObj1State);
	part_next(gb, P2C);
}
static void p2_aea54(GB *gb)
{
	p2_aeaef(gb);
	p2_attached_land(gb);
}
static void p2_aea59(GB *gb)
{
	p2_aeaef(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P2C);
}
static void p2_aea5f(GB *gb) { part_wait(gb, P2C, 0x04); }
static void p2_aea63(GB *gb)
{
	set_obj_state(gb, wSceneObj4State, 0x12);
	part_next(gb, P2C);
}
static void p2_aea6d(GB *gb) { part_wait(gb, P2C, 0x28); }
static void p2_aea71(GB *gb)
{
	set_coord_and_state(gb, wSceneObj4, 46, 224, 0x08);
	p2_attach(gb);
}
static void p2_aea92(GB *gb)
{
	p2_aeaf9(gb);
	p2_attached_land(gb);
}
static void p2_aeab9(GB *gb)
{
	p2_aeaf9(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P2C);
}
static void p2_aeac0(GB *gb) { part_wait(gb, P2C, 0x08); }
static void p2_aeac5(GB *gb)
{
	set_coord_and_state(gb, wSceneObj4, 46, 244, 0x04);
	p2_attach(gb);
}
static void p2_aead2(GB *gb)
{
	p2_aeaf9(gb); // .Func_aeb03
	part_wait_land(gb, wSceneObj4YCoord, P2A);
}
static void p2_aead8(GB *gb)
{
	p2_aeaf9(gb); // .Func_aeb03
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P2C);
}
static void p2_aeadf(GB *gb) { part_wait(gb, P2C, 0x08); }
static void p2_aeae4(GB *gb) { part_end(gb, P2A, wSceneObj4State); }

static const Handler p2_table[] = {
	p2_aea40, p2_aea54, p2_aea59, p2_aea5f, p2_aea63, p2_aea6d, p2_aea71, p2_aea92,
	p2_aeab9, p2_aeac0, p2_aeac5, p2_aead2, p2_aead8, p2_aeadf, p2_aeae4,
};

static bool func_ae9ee(GB *gb, uint16_t ret_addr)
{
	return plane_part(gb, ADDR_Func_ae9ee, P2C, p2_table, 15, ret_addr);
}

PORT_NESTED(Func_ae9ee)
{
	func_ae9ee(gb, 0);
}

// Plane part 3: Func_aeb0d

#define P3C wPlanePart3Counter
#define P3A wPlanePart3Action

static void p3_aec0b(GB *gb) { apply_movement(gb, wSceneObj5, ADDR_Data_aed6d); }
static void p3_aec15(GB *gb) { apply_movement(gb, wSceneObj5, ADDR_Data_aee09); }
static void p3_aec1a(GB *gb) { apply_movement(gb, wSceneObj5, ADDR_Data_aedba); }

static void p3_aeb5f(GB *gb)
{
	set_coord_and_state(gb, wSceneObj5, 62, 224, 0x03);
	part_attach(gb, wSceneObj5, wSceneObj2, wSceneObj2State, P3C);
}
static void p3_aeb80(GB *gb)
{
	p3_aec0b(gb);
	part_attached_land(gb, wSceneObj5, wSceneObj2, wSceneObj2State, P3A);
}
static void p3_aeba7(GB *gb)
{
	p3_aec0b(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P3C);
}
static void p3_aebad(GB *gb) { part_wait(gb, P3C, 0x08); }
static void p3_aebb1(GB *gb)
{
	set_coord_and_state(gb, wSceneObj5, 46, 240, 0x07);
	part_next(gb, P3C);
}
static void p3_aebbf(GB *gb)
{
	p3_aec15(gb);
	part_wait_land(gb, wSceneObj5YCoord, P3A);
}
static void p3_aebc5(GB *gb)
{
	p3_aec15(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P3C);
}
static void p3_aebcc(GB *gb) { part_wait(gb, P3C, 0x04); }
static void p3_aebd0(GB *gb)
{
	set_obj_state(gb, wSceneObj5State, 0x13);
	part_next(gb, P3C);
}
static void p3_aebdb(GB *gb) { part_wait(gb, P3C, 0x28); }
static void p3_aebe0(GB *gb)
{
	set_coord_and_state(gb, wSceneObj5, 77, 248, 0x05);
	part_next(gb, P3C);
}
static void p3_aebee(GB *gb)
{
	p3_aec1a(gb);
	part_wait_land(gb, wSceneObj5YCoord, P3A);
}
static void p3_aebf4(GB *gb)
{
	p3_aec1a(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P3C);
}
static void p3_aebfb(GB *gb) { part_wait(gb, P3C, 0x08); }
static void p3_aec00(GB *gb) { part_end(gb, P3A, wSceneObj5State); }

static const Handler p3_table[] = {
	p3_aeb5f, p3_aeb80, p3_aeba7, p3_aebad, p3_aebb1, p3_aebbf, p3_aebc5, p3_aebcc,
	p3_aebd0, p3_aebdb, p3_aebe0, p3_aebee, p3_aebf4, p3_aebfb, p3_aec00,
};

static bool func_aeb0d(GB *gb, uint16_t ret_addr)
{
	return plane_part(gb, ADDR_Func_aeb0d, P3C, p3_table, 15, ret_addr);
}

PORT_NESTED(Func_aeb0d)
{
	func_aeb0d(gb, 0);
}

// Plane part 4: Func_aec1f

#define P4C wPlanePart4Counter
#define P4A wPlanePart4Action

static void p4_aecf2(GB *gb) { apply_movement(gb, wSceneObj9, ADDR_Data_aedba); }
static void p4_aecfc(GB *gb) { apply_movement(gb, wSceneObj9, ADDR_Data_aed6d); }

static void p4_aec69(GB *gb)
{
	set_coord_and_state(gb, wSceneObj9, 77, 240, 0x07);
	part_next(gb, P4C);
}
static void p4_aec76(GB *gb)
{
	p4_aecf2(gb);
	part_wait_land(gb, wSceneObj9YCoord, P4A);
}
static void p4_aec7b(GB *gb)
{
	p4_aecf2(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P4C);
}
static void p4_aec81(GB *gb) { part_wait(gb, P4C, 0x04); }
static void p4_aec85(GB *gb)
{
	set_obj_state(gb, wSceneObj9State, 0x13);
	part_next(gb, P4C);
}
static void p4_aec8f(GB *gb) { part_wait(gb, P4C, 0x28); }
static void p4_aec93(GB *gb)
{
	set_coord_and_state(gb, wSceneObj9, 62, 240, 0x08);
	part_attach(gb, wSceneObj9, wSceneObj8, wSceneObj8State, P4C);
}
static void p4_aecb4(GB *gb)
{
	p4_aecfc(gb);
	part_attached_land(gb, wSceneObj9, wSceneObj8, wSceneObj8State, P4A);
}
static void p4_aecdb(GB *gb)
{
	p4_aecfc(gb);
	if (gb->cpu.f & FLAG_Z)
		part_sfx_next(gb, P4C);
}
static void p4_aece2(GB *gb) { part_wait(gb, P4C, 0x08); }
static void p4_aece7(GB *gb) { part_end(gb, P4A, wSceneObj9State); }

static const Handler p4_table[] = {
	p4_aec69, p4_aec76, p4_aec7b, p4_aec81, p4_aec85, p4_aec8f,
	p4_aec93, p4_aecb4, p4_aecdb, p4_aece2, p4_aece7,
};

static bool func_aec1f(GB *gb, uint16_t ret_addr)
{
	return plane_part(gb, ADDR_Func_aec1f, P4C, p4_table, 11, ret_addr);
}

PORT_NESTED(Func_aec1f)
{
	func_aec1f(gb, 0);
}

// Func_ae8bb: all four plane parts, then their sequencing

static void ae8bb_start_part(GB *gb, uint16_t part_counter)
{
	gb->cpu.a = rd8(gb, w2d8a1);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_C)
		return;
	part_next(gb, part_counter);
	part_next(gb, w2d8a1); // .asm_ae912
}

static void ae8bb_ae8d9(GB *gb) { ae8bb_start_part(gb, wPlanePart1Counter); }
static void ae8bb_ae8e8(GB *gb) { ae8bb_start_part(gb, wPlanePart2Counter); }
static void ae8bb_ae8f6(GB *gb) { ae8bb_start_part(gb, wPlanePart3Counter); }
static void ae8bb_ae904(GB *gb) { ae8bb_start_part(gb, wPlanePart4Counter); }
static void ae8bb_ae8d8(GB *gb) { (void)gb; }

static const Handler ae8bb_table[] = {
	ae8bb_ae8d9, ae8bb_ae8e8, ae8bb_ae8f6, ae8bb_ae904, ae8bb_ae8d8,
};

static bool func_ae8bb(GB *gb, uint16_t ret_addr)
{
	enter(gb, ret_addr);
	if (!func_ae924(gb, ADDR_Func_ae8bb + 3) || !func_ae9ee(gb, ADDR_Func_ae8bb + 6) ||
		!func_aeb0d(gb, ADDR_Func_ae8bb + 9) || !func_aec1f(gb, ADDR_Func_ae8bb + 12))
		return false;

	SET_HL(gb, w2d8a1);
	wr8(gb, w2d8a1, alu_inc(gb, rd8(gb, w2d8a1)));
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = rd8(gb, REG_HL(gb)); // w2d8a0
	if (!dispatch(gb, ADDR_Func_ae8bb + 19, ae8bb_table, 5, ret_addr))
		return false;
	return leave(gb, ret_addr);
}

PORT_NESTED(Func_ae8bb)
{
	func_ae8bb(gb, 0);
}

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

// ld hl, palettes / ld de, dest / ld c, 8 palettes / ld b, bank / call CopyFarBytes
static void copy_pals(GB *gb, uint16_t src, uint16_t dest, uint8_t bank)
{
	SET_HL(gb, src);
	SET_DE(gb, dest);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = bank;
	CALL(CopyFarBytes);
}

static void epi_func_af1bd(GB *gb)
{
	CALL_2B(Func_aca09);
	copy_pals(gb, ADDR_Pals_86add, wTempPals1, BANK_Pals_86add);
	copy_pals(gb, ADDR_Pals_86b1d, wTempPals2, BANK_Pals_86b1d);
}

static void epi_func_af1db(GB *gb)
{
	SET_HL(gb, EPI(0x735e)); // .Pals_af35e
	SET_DE(gb, wTempPals2 + PAL_SIZE * 4);
	gb->cpu.b = 3 * PAL_SIZE;
	CALL(CopyHLToDE);
}

static void epi_func_af197(GB *gb)
{
	gb->cpu.c = 0x3c;
	prologue_delay(gb); // Func_af130
}

static void epi_func_af19c(GB *gb)
{
	set_obj_coord(gb, wSceneObj9, 0x60, 0x50);
	set_obj_state(gb, wSceneObj9State, 0x18);
	epi_func_af1bd(gb);
	epi_func_af1db(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);
	prologue_next(gb);
}

// xor a or ld a, 1 / ld [wHDMADestVRAMBank], a / ld de, src / ld b, dest /
// ld c, length / jr .asm_af1f2
static void epi_hdma(GB *gb, uint8_t vram_bank, uint16_t src, uint8_t dest, uint8_t len)
{
	gb->cpu.a = vram_bank;
	if (!vram_bank)
		gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wHDMADestVRAMBank, vram_bank);
	SET_DE(gb, src);
	gb->cpu.b = dest;
	gb->cpu.c = len;
	configure_hdma(gb, BANK_OldMan1Gfx);
}

static void epi_func_af1e7(GB *gb)
{
	epi_hdma(gb, 0, ADDR_OldMan1Gfx, (v0Tiles1 >> 8) - 0x80, 0x40 - 1);
}

static void epi_func_af207(GB *gb)
{
	epi_hdma(gb, 1, ADDR_OldMan2Gfx, (v1Tiles0 >> 8) - 0x80, 0x80 - 1);
}

static void epi_func_af215(GB *gb)
{
	epi_hdma(gb, 0, ADDR_EpilogueCrowdTilemap, (v0BGMap0 >> 8) - 0x80, 18 * 2 - 1);
}

static void epi_func_af222(GB *gb)
{
	epi_hdma(gb, 1, ADDR_EpilogueCrowdAttrmap, (v1BGMap0 >> 8) - 0x80, 18 * 2 - 1);
}

static void epi_func_af230(GB *gb)
{
	epi_hdma(gb, 1, wAttrmap, (v1Tiles1 >> 8) - 0x80, 0x50 - 1);
}

// .Func_af2ab: hl = obj, d = y, e = x, b = state
static void epi_func_af2ab(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint8_t state)
{
	SET_HL(gb, obj);
	gb->cpu.d = y;
	gb->cpu.e = x;
	gb->cpu.b = state;
	gb->cpu.a = y;
	wr8(gb, obj, y);
	gb->cpu.a = x;
	wr8(gb, obj + 1, x);
	SET_HL(gb, obj + 2);
	gb->cpu.l |= 1 << 2; // set 2, l
	gb->cpu.a = gb->cpu.b;
	CALL(SetSceneObjState);
}

static void epi_func_af23e(GB *gb)
{
	gb->cpu.a = SCREEN_WIDTH_PX;
	wr8(gb, rSCX, SCREEN_WIDTH_PX);
	wr8(gb, wSCX, SCREEN_WIDTH_PX);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);
	SET_HL(gb, wOWUIObj1YCoord + 1);
	gb->cpu.a = 0x60;
	wr8(gb, wOWUIObj1YCoord, 0x60);
	wr8(gb, wOWUIObj1YCoord + 1, 0x5c);
	gb->cpu.a = 0xff;
	wr8(gb, wOWUIObj2YCoord, 0xff);
	set_wario_state(gb, 0x0c);

	epi_func_af2ab(gb, wSceneObj10, 0x30, 0x00, 0x1b);
	epi_func_af2ab(gb, wSceneObj11, 0x35, 0x20, 0x1c);
	epi_func_af2ab(gb, wSceneObj12, 0x32, 0x70, 0x1b);
	epi_func_af2ab(gb, wSceneObj13, 0x38, 0xc0, 0x1c);

	copy_pals(gb, ADDR_Pals_86add, wTempPals1, BANK_Pals_86add); // .Func_af2b5
	copy_pals(gb, ADDR_Pals_86b1d, wTempPals2, BANK_Pals_86add);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);
	wr8(gb, w2d890, 0);
	wr8(gb, w2d891, 0);
	prologue_next(gb);
}

static void epi_func_af2bc(GB *gb)
{
	gb->cpu.c = 0x1e;
	prologue_delay(gb); // Func_af130
}

// .asm_af3d7: ld a, b / ld hl, wSceneObj9State / jp SetSceneObjState
static void epi_set_obj9_state_b(GB *gb)
{
	gb->cpu.a = gb->cpu.b;
	SET_HL(gb, wSceneObj9State);
	CALL(SetSceneObjState);
}

// .asm_af3bf
static void epi_af3bf(GB *gb)
{
	gb->cpu.a = rd8(gb, wPalFade7Colour3RedSign);
	alu_cp(gb, gb->cpu.a, 0x01);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_obj_state(gb, wSceneObj9State, 0x0b);
}

// .asm_af3d0: b = state
static void epi_af3d0(GB *gb, uint8_t state)
{
	gb->cpu.b = state;
	gb->cpu.a = 0x01;
	SET_HL(gb, wPalFade7Colour3RedSign);
	alu_cp(gb, gb->cpu.a, rd8(gb, wPalFade7Colour3RedSign));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	epi_set_obj9_state_b(gb);
}

// .asm_af3ea: b = state, a = sign
static void epi_af3ea(GB *gb, uint8_t state, uint8_t sign)
{
	gb->cpu.b = state;
	gb->cpu.a = sign;
	SET_HL(gb, wPalFade7Colour3RedSign);
	alu_cp(gb, gb->cpu.a, rd8(gb, wPalFade7Colour3RedSign));
	if (!(gb->cpu.f & FLAG_Z)) {
		epi_af3bf(gb);
		return;
	}
	epi_set_obj9_state_b(gb);
}

static void epi_func_af376(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wPendingCharDest), rd8(gb, wPendingCharDest));
	if (!gb->cpu.a)
		return;
	gb->cpu.a = rd8(gb, wCurTextLinePos);
	alu_cp(gb, gb->cpu.a, 1);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wLanguage), rd8(gb, wLanguage));
	bool english = gb->cpu.a;
	gb->cpu.a = rd8(gb, wPalFade7Colour3RedUnk1);
	// each entry: value compared, then what to do (0: .asm_af3d0 with
	// state, 1: .asm_af3ea with state and sign)
	static const struct { uint8_t value, kind, state, sign; } jp[] = {
		{0x01, 0, 0x10, 0}, {0x04, 0, 0x0c, 0}, {0x05, 1, 0x0d, 2},
		{0x07, 1, 0x0e, 2}, {0x0a, 1, 0x10, 2}, {0x0b, 1, 0x0c, 3},
	}, en[] = {
		{0x01, 0, 0x10, 0}, {0x04, 0, 0x0c, 0}, {0x05, 0, 0x0d, 0},
		{0x08, 1, 0x0e, 2}, {0x0b, 0, 0x10, 0}, {0x0c, 0, 0x0c, 0},
	};
	const __typeof__(jp[0]) *cases = english ? en : jp;
	for (int i = 0; i < 6; i++) {
		alu_cp(gb, gb->cpu.a, cases[i].value);
		if (gb->cpu.f & FLAG_Z) {
			if (cases[i].kind == 0)
				epi_af3d0(gb, cases[i].state);
			else
				epi_af3ea(gb, cases[i].state, cases[i].sign);
			return;
		}
	}
	epi_af3bf(gb);
}

static void epi_func_af2c1(GB *gb)
{
	CALL_2B(PrintTextWithoutHeader);
	epi_func_af376(gb);
	gb->cpu.a = rd8(gb, wCurTextLine);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	prologue_next(gb);
}

static void epi_func_af2df(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wHDMADestVRAMBank, 0);
	SET_DE(gb, ADDR_PrologueBackgroundGfx);
	gb->cpu.b = (v0Tiles2 >> 8) - 0x80;
	gb->cpu.c = 0x80 - 1;
	configure_hdma(gb, BANK_PrologueBackgroundGfx);
}

static void epi_func_af2f2(GB *gb)
{
	set_obj_coord(gb, wSceneObj9, 0x3c, 0xb2);
	set_obj_state(gb, wSceneObj9State, 0x13);
	set_obj_coord(gb, wOWUIObj1YCoord, 0x3c, 0xc8);
	set_wario_state(gb, 0x1c);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneObj10State, 0);
	wr8(gb, wSceneObj11State, 0);
	wr8(gb, wSceneObj12State, 0);
	wr8(gb, wSceneObj13State, 0);
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	CALL_2B(Func_aca09);
	copy_pals(gb, ADDR_Pals_869cd, wTempPals1, BANK_Pals_869cd);
	copy_pals(gb, ADDR_Pals_86b1d, wTempPals2, BANK_Pals_86b1d);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);
	prologue_next(gb);
}

static void epi_func_af34a(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d014), 0x03);
	if (gb->cpu.a)
		return;
	CALL_2B(PrologueFade);
}

static void epi_func_af353(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_C)
		return;
	prologue_finish(gb);
}

// .asm_af400: b = state to wait for
static void epi_wait_owui_obj1_state(GB *gb, uint8_t state)
{
	gb->cpu.b = state;
	gb->cpu.a = rd8(gb, wOWUIObj1State);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}

static void epi_func_af41c(GB *gb)
{
	FARCALL(Func_84e43);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1TotalSteps), rd8(gb, wPalConfig1TotalSteps));
	if (gb->cpu.a)
		return;
	SET_HL(gb, w2d890);
	wr8(gb, w2d891, 0);
	wr8(gb, w2d890, alu_inc(gb, rd8(gb, w2d890)));
}

// ld hl, w2d890 / ld a, [hli] / inc [hl] / ld b, h / ld c, l
static void epi_ld_a_w2d890_inc_w2d891(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d890);
	SET_HL(gb, w2d891);
	wr8(gb, w2d891, alu_inc(gb, rd8(gb, w2d891)));
	gb->cpu.b = gb->cpu.h;
	gb->cpu.c = gb->cpu.l;
}

// .Func_af436: the old man's transformation

// .Func_af60d's jumptable
static void epi_af643(GB *gb)
{
	SET_HL(gb, wTempleRock5Action);
	wr8(gb, wTempleRock5Action, alu_inc(gb, rd8(gb, wTempleRock5Action)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wTempleRock5Counter, 0);
}
static void epi_af639(GB *gb) { (void)gb; }
static void epi_af63a(GB *gb)
{
	gb->cpu.a = 0x01;
	wr8(gb, wPalFade2Colour4GreenSign, 0x01);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneObj9Unk7, 0);
	epi_af643(gb);
}
static void epi_af64c(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj9YCoord);
	alu_cp(gb, gb->cpu.a, 0x60);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_bump_sfx(gb); // .Func_af667
	epi_af643(gb);
}
static void epi_af657(GB *gb)
{
	SET_HL(gb, wTempleRock5Counter);
	wr8(gb, wTempleRock5Counter, alu_inc(gb, rd8(gb, wTempleRock5Counter)));
	gb->cpu.a = rd8(gb, wTempleRock5Counter);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_C)
		return;
	epi_af643(gb);
}
static void epi_af661(GB *gb)
{
	gb->cpu.a = 0x80;
	wr8(gb, wTempleRock5Action, 0x80);
}

static const Handler epi_af60d_table[] = {
	epi_af639, epi_af63a, epi_af64c, epi_af657, epi_af64c, epi_af657, epi_af64c, epi_af661,
};

static bool epi_func_af60d(GB *gb, uint16_t ret_addr)
{
	enter(gb, ret_addr);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalFade2Colour4GreenSign), rd8(gb, wPalFade2Colour4GreenSign));
	if (gb->cpu.a) {
		SET_HL(gb, wSceneObj9);
		SET_DE(gb, ADDR_Data_aed06);
		CALL(ApplyOWMovement_Mirrored);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z;
			wr8(gb, wPalFade2Colour4GreenSign, 0);
		}
	}
	// .asm_af622
	gb->cpu.a = rd8(gb, wTempleRock5Action);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z)
		return leave(gb, ret_addr);
	if (!dispatch(gb, EPI(0x760d) + 28, epi_af60d_table, 8, ret_addr))
		return false;
	return leave(gb, ret_addr);
}

// .Func_af57b's jumptable
static void epi_af5b2(GB *gb) { (void)gb; }
static void epi_af5b3(GB *gb)
{
	gb->cpu.a = 0x01;
	wr8(gb, wPalFade2Colour2GreenUnk1, 0x01);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneWarioUnk7, 0);
	fall_next_bounce_step(gb); // .asm_af5bc
}
static void epi_af5c5(GB *gb)
{
	gb->cpu.a = rd8(gb, wOWUIObj1YCoord);
	alu_cp(gb, gb->cpu.a, 0x60);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_wario_state(gb, 0x1b);
	play_bump_sfx(gb); // .Func_af604
	fall_next_bounce_step(gb);
}
static void epi_af5df(GB *gb)
{
	gb->cpu.a = rd8(gb, wOWUIObj1State);
	alu_cp(gb, gb->cpu.a, 0x1c);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	fall_next_bounce_step(gb);
}
static void epi_af5e7(GB *gb)
{
	SET_HL(gb, w2d893);
	wr8(gb, w2d893, alu_inc(gb, rd8(gb, w2d893)));
	gb->cpu.a = rd8(gb, w2d893);
	alu_cp(gb, gb->cpu.a, 0x42);
	if (gb->cpu.f & FLAG_C)
		return;
	set_wario_state(gb, 0x0c);
	play_sfx(gb, SFX_12A);
	fall_next_bounce_step(gb);
}
static void epi_af5fe(GB *gb)
{
	gb->cpu.a = 0x80;
	wr8(gb, w2d892, 0x80);
}

static const Handler epi_af57b_table[] = {
	epi_af5b2, epi_af5b3, epi_af5c5, fall_wait_4, epi_af5c5,
	fall_wait_4, epi_af5c5, epi_af5df, epi_af5e7, epi_af5fe,
};

static bool epi_func_af57b(GB *gb, uint16_t ret_addr)
{
	enter(gb, ret_addr);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalFade2Colour2GreenUnk1), rd8(gb, wPalFade2Colour2GreenUnk1));
	if (gb->cpu.a) {
		gb->cpu.a = alu_and(gb, rd8(gb, w2d891), 0x03);
		if (gb->cpu.a) {
			SET_DE(gb, ADDR_Data_aed06);
			SET_HL(gb, wOWUIObj1YCoord);
			CALL(ApplyOWMovement_Mirrored);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z;
				wr8(gb, wPalFade2Colour2GreenUnk1, 0);
			}
		}
	}
	// .asm_af597
	gb->cpu.a = rd8(gb, w2d892);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z)
		return leave(gb, ret_addr);
	if (!dispatch(gb, EPI(0x757b) + 35, epi_af57b_table, 10, ret_addr))
		return false;
	return leave(gb, ret_addr);
}

// .Func_af436's jumptable (bc = w2d891)
static void epi_af466(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wPalFade2Colour2GreenUnk1, 0);
	wr8(gb, w2d892, 0);
	wr8(gb, wTempleRock5Action, 0);
	wr8(gb, wPalFade2Colour4GreenSign, 0);
	advance_w2d890(gb);
}
static void epi_af476(GB *gb)
{
	if (BC_BELOW(0x40))
		return;
	gb->cpu.a = 0x01;
	wr8(gb, w2d892, 0x01);
	play_sfx(gb, SFX_THROW);
	advance_w2d890(gb);
}
static void epi_af48a(GB *gb)
{
	if (BC_BELOW(0x56))
		return;
	gb->cpu.a = 0x01;
	wr8(gb, wTempleRock5Action, 0x01);
	play_sfx(gb, SFX_THROW);
	advance_w2d890(gb);
}
static void epi_af49e(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d892);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wTempleRock5Action);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}
static void epi_af4ad(GB *gb)
{
	if (BC_BELOW(0x3c))
		return;
	set_wario_state_and_advance(gb, 0x05);
}
static void epi_af4b9(GB *gb) { epi_wait_owui_obj1_state(gb, 0x02); }
static void epi_af4be(GB *gb)
{
	if (BC_BELOW(0x28))
		return;
	set_wario_state_and_advance(gb, 0x14);
}
static void epi_af4ca(GB *gb) { epi_wait_owui_obj1_state(gb, 0x0c); }
static void epi_af4cf(GB *gb)
{
	if (BC_BELOW(0x2d))
		return;
	set_wario_state(gb, 0x0b);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneObj9Unk7, 0);
	play_sfx(gb, SFX_GRAB);
	advance_w2d890(gb);
}
static void epi_af4e7(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj9Unk7);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wOWUIObj1State);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}
// .asm_af50a: place Obj9 at OWUIObj1 + (d, e)
static void epi_af50a(GB *gb, uint8_t dy, uint8_t dx)
{
	gb->cpu.d = dy;
	gb->cpu.e = dx;
	SET_HL(gb, wSceneObj9);
	gb->cpu.a = alu_add(gb, rd8(gb, wOWUIObj1YCoord), gb->cpu.d);
	wr8(gb, wSceneObj9, gb->cpu.a);
	SET_HL(gb, wSceneObj9 + 1);
	gb->cpu.a = alu_add(gb, rd8(gb, wOWUIObj1XCoord), gb->cpu.e);
	wr8(gb, wSceneObj9 + 1, gb->cpu.a);
}
static void epi_af4f6(GB *gb)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_Z) {
		epi_af50a(gb, 0xdf, 0xfa);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x06);
	if (gb->cpu.f & FLAG_Z) {
		epi_af50a(gb, 0xe0, 0xfb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x07);
	if (gb->cpu.f & FLAG_Z) {
		epi_af50a(gb, 0xe2, 0xfc);
		advance_w2d890(gb);
	}
}
static void epi_af528(GB *gb)
{
	if (BC_BELOW(0x32))
		return;
	set_wario_state_and_advance(gb, 0x21);
}
static void epi_af534(GB *gb)
{
	gb->cpu.a = rd8(gb, wOWUIObj1Frame);
	alu_cp(gb, gb->cpu.a, 0x0b);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_106);
	advance_w2d890(gb);
}
static void epi_af545(GB *gb) { epi_wait_owui_obj1_state(gb, 0x0a); }
static void epi_af54a(GB *gb)
{
	if (BC_BELOW(0x32))
		return;
	set_wario_state(gb, 0x09);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneObj9Unk7, 0);
	advance_w2d890(gb);
}
static void epi_af55a(GB *gb)
{
	CALL_2B(Func_aca63);
	gb->cpu.a = rd8(gb, wOWUIObj1XCoord);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x04);
	wr8(gb, wSceneObj9XCoord, gb->cpu.a);
	gb->cpu.a = 0xf0;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wOWUIObj1State, 0);
	wr8(gb, wSceneObj9State, 0);
	advance_w2d890(gb);
}
static void epi_af574(GB *gb)
{
	if (BC_BELOW(0x3c))
		return;
	func_ae1d5(gb); // .asm_af413
}

static const Handler epi_af436_table[] = {
	epi_af466, epi_af476, epi_af48a, epi_af49e, epi_af4ad, epi_af4b9, epi_af4be, epi_af4ca, epi_af4cf,
	epi_af4e7, epi_af4f6, epi_af528, epi_af534, epi_af545, epi_af54a, epi_af55a, epi_af574,
};

static void epi_func_af436(GB *gb)
{
	if (!epi_func_af60d(gb, EPI(0x7436) + 3) || !epi_func_af57b(gb, EPI(0x7436) + 6))
		return;
	epi_ld_a_w2d890_inc_w2d891(gb);
	dispatch(gb, EPI(0x7436) + 14, epi_af436_table, 17, EPI_RET);
}

// .Func_af670: the old man thanks Wario

static void epi_af68b(GB *gb)
{
	if (BC_BELOW(0x1e))
		return;
	set_obj_state(gb, wSceneObj1State, 0x01);
	advance_w2d890(gb);
}
static void epi_af69a(GB *gb)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	clear_obj_state(gb, wSceneObj1State);
	stop_music2(gb);
	CALL_2B(ClearTextbox);
	set_obj_state(gb, wSceneObj9State, 0x0f);
	advance_w2d890(gb);
}
static void epi_af6c1(GB *gb)
{
	if (BC_BELOW(0x3c))
		return;
	gb->cpu.a = 0x01;
	wr8(gb, w2d150, 0x01);
	advance_w2d890(gb);
}
static void epi_af6cd(GB *gb)
{
	if (BC_BELOW(0x28))
		return;
	gb->cpu.a = 0x80;
	wr8(gb, w2d150, 0x80);
	SET_HL(gb, wCurSceneObj);
	gb->cpu.a = alu_sub(gb, rd8(gb, wOWUIObj1YCoord), 0x04);
	wr8(gb, wCurSceneObj, gb->cpu.a);
	SET_HL(gb, wCurSceneObj + 1);
	gb->cpu.a = rd8(gb, wOWUIObj1XCoord);
	wr8(gb, wCurSceneObj + 1, gb->cpu.a);
	set_obj_state(gb, wCurSceneObjState, 0x11);
	advance_w2d890(gb);
}
static void epi_af6ee(GB *gb)
{
	if (BC_BELOW(0x14))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	CALL_2B(SetSceneWarioState);
	advance_w2d890(gb);
}
static void epi_af6f9(GB *gb)
{
	if (BC_BELOW(0x5a))
		return;
	advance_w2d890(gb);
}
static void epi_af700(GB *gb)
{
	SET_HL(gb, wCurSceneObj);
	gb->cpu.a = rd8(gb, wCurSceneObj);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = rd8(gb, w2d891);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
		if (gb->cpu.a)
			return;
		wr8(gb, wCurSceneObj, alu_dec(gb, rd8(gb, wCurSceneObj)));
		return;
	}
	// .asm_af711
	wr8(gb, wCurSceneObj, alu_dec(gb, rd8(gb, wCurSceneObj)));
	wr8(gb, wCurSceneObj, alu_dec(gb, rd8(gb, wCurSceneObj)));
	wr8(gb, wCurSceneObj, alu_dec(gb, rd8(gb, wCurSceneObj)));
	gb->cpu.a = rd8(gb, wCurSceneObj);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_C) {
		// .asm_af733
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		wr8(gb, wCurSceneObjState, 0);
		advance_w2d890(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x40);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = rd8(gb, wCurSceneObjState);
	alu_cp(gb, gb->cpu.a, 0x12);
	if (gb->cpu.f & FLAG_Z)
		return;
	set_obj_state(gb, wCurSceneObjState, 0x12);
	set_obj_state(gb, wSceneObj9State, 0x0a);
}
static void epi_af73a(GB *gb)
{
	if (BC_BELOW(0x2d))
		return;
	func_ae1d5(gb); // .asm_af413
}

static const Handler epi_af670_table[] = {
	epi_af68b, epi_af69a, epi_af6c1, epi_af6cd, epi_af6ee, epi_af6f9, epi_af700, epi_af73a,
};

static void epi_func_af670(GB *gb)
{
	func_ae523(gb);
	epi_ld_a_w2d890_inc_w2d891(gb);
	dispatch(gb, EPI(0x7670) + 11, epi_af670_table, 8, EPI_RET);
}

// .Func_af741: the old man walks away

// .Func_af7a7
static void epi_af7a7(GB *gb)
{
	SET_HL(gb, wSceneObj9XCoord);
	wr8(gb, wSceneObj9XCoord, alu_inc(gb, rd8(gb, wSceneObj9XCoord)));
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneObj9XCoord), 0xfe);
}
static void epi_af755(GB *gb)
{
	if (BC_BELOW(0x3c))
		return;
	set_obj_state(gb, wSceneObj9State, 0x08);
	SET_HL(gb, wSceneObj9 + 1);
	gb->cpu.a = 0x60;
	wr8(gb, wSceneObj9, 0x60);
	wr8(gb, wSceneObj9 + 1, 0x80);
	advance_w2d890(gb);
}
static void epi_af76c(GB *gb)
{
	epi_af7a7(gb);
	alu_cp(gb, gb->cpu.a, 0xe0);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}
static void epi_af775(GB *gb)
{
	epi_af7a7(gb);
	SET_HL(gb, wSCX);
	wr8(gb, wSCX, alu_inc(gb, rd8(gb, wSCX)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_obj_state(gb, wSceneObj9State, 0x09);
	advance_w2d890(gb);
}
static void epi_af788(GB *gb)
{
	if (BC_BELOW(0x1e))
		return;
	set_obj_state(gb, wSceneObj9State, 0x06);
	advance_w2d890(gb);
}
static void epi_af797(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj9State);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	advance_w2d890(gb);
}
static void epi_af7a0(GB *gb)
{
	if (BC_BELOW(0x3c))
		return;
	func_ae1d5(gb); // .asm_af413
}

static const Handler epi_af741_table[] = {
	epi_af755, epi_af76c, epi_af775, epi_af788, epi_af797, epi_af7a0,
};

static void epi_func_af741(GB *gb)
{
	epi_ld_a_w2d890_inc_w2d891(gb);
	dispatch(gb, EPI(0x7741) + 8, epi_af741_table, 6, EPI_RET);
}

// .Func_af7af: the transformation into the princess

// .asm_af7ff
static void epi_set_attributes(GB *gb)
{
	wr8(gb, wSceneObj5Attributes, gb->cpu.a);
	wr8(gb, wSceneObj6Attributes, gb->cpu.a);
	wr8(gb, wSceneObj7Attributes, gb->cpu.a);
	wr8(gb, wSceneObj8Attributes, gb->cpu.a);
	wr8(gb, wSceneObj1Attributes, gb->cpu.a);
}
static void epi_af7ff(GB *gb)
{
	epi_set_attributes(gb);
	gb->cpu.a = 0x01;
	wr8(gb, w2d8a0, 0x01);
	advance_w2d890(gb);
}
static void epi_af7ee(GB *gb)
{
	play_music2(gb, MUSIC_EPILOGUE);
	advance_w2d890(gb);
}
static void epi_af7fd(GB *gb)
{
	gb->cpu.a = 0x04;
	epi_af7ff(gb);
}
static void epi_af816(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj1YCoord);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_C)
		return;
	play_sfx(gb, SFX_0EB);
	CALL_2B(Func_aca09);
	CALL_2B(Func_aca25);
	advance_w2d890(gb);
}
// ld a, state / ld hl, wSceneObj9State / call SetSceneObjState /
// ld hl, wSceneObj9XCoord / dec [hl] (times) / .asm_af839
static void epi_af839(GB *gb, uint8_t state, int decs)
{
	set_obj_state(gb, wSceneObj9State, state);
	SET_HL(gb, wSceneObj9XCoord);
	for (int i = 0; i < decs; i++)
		wr8(gb, wSceneObj9XCoord, alu_dec(gb, rd8(gb, wSceneObj9XCoord)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wSceneObj1State, 0);
	epi_func_af1bd(gb);
	advance_w2d890(gb);
}
static void epi_af82d(GB *gb) { epi_af839(gb, 0x02, 1); }
static void epi_af863(GB *gb) { epi_af839(gb, 0x03, 1); }
static void epi_af893(GB *gb) { epi_af839(gb, 0x04, 2); }
// .Func_af843 / .Func_af871
static void epi_show_princess(GB *gb, uint8_t state, uint8_t x)
{
	if (BC_BELOW(0x50))
		return;
	set_obj_state(gb, wSceneObj9State, state);
	gb->cpu.a = x;
	wr8(gb, wSceneObj9XCoord, x);
	epi_func_af1db(gb);
	SET_DE(gb, ADDR_Data_aca04);
	CALL_2B(Func_ac9e4);
	advance_w2d890(gb);
}
static void epi_af843(GB *gb) { epi_show_princess(gb, 0x19, 0x48); }
static void epi_af871(GB *gb) { epi_show_princess(gb, 0x1a, 0x58); }
static void epi_af860(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	epi_af7ff(gb);
}
static void epi_af88e(GB *gb)
{
	gb->cpu.a = 0x03;
	epi_af7ff(gb);
}
static void epi_af8a2(GB *gb)
{
	if (BC_BELOW(0x50))
		return;
	advance_w2d890(gb);
}
static void epi_af8a9(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	epi_set_attributes(gb);
	func_ae1d5(gb); // .asm_af413
}

static const Handler epi_af7af_table[] = {
	epi_af7ee, epi_af7fd, epi_af816, epi_func_af41c, epi_af82d, epi_func_af41c, epi_af843,
	epi_af860, epi_af816, epi_func_af41c, epi_af863, epi_func_af41c, epi_af871, epi_af88e,
	epi_af816, epi_func_af41c, epi_af893, epi_func_af41c, epi_af8a2, epi_af8a9,
};

static void epi_func_af7af(GB *gb)
{
	FARCALL(Func_15848a);
	epi_ld_a_w2d890_inc_w2d891(gb);
	dispatch(gb, EPI(0x77af) + 23, epi_af7af_table, 20, EPI_RET);
}

// .Func_af8bc: the crowd

// ld a, [wGlobalCounter] / and mask / ret nz / ld a, $07 / xor l / ld l, a /
// inc [hl]
static void epi_crowd_step(GB *gb, uint8_t mask)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), mask);
	if (gb->cpu.a)
		return;
	gb->cpu.a = alu_xor(gb, 0x07, gb->cpu.l);
	gb->cpu.l = gb->cpu.a;
	wr8(gb, REG_HL(gb), alu_inc(gb, rd8(gb, REG_HL(gb))));
}

// ld hl, obj_state / ld a, [hl] / cp state / ret nz / then epi_crowd_step;
// returns false if it returned early
static bool epi_crowd_member(GB *gb, uint16_t obj_state, uint8_t state, uint8_t mask)
{
	SET_HL(gb, obj_state);
	gb->cpu.a = rd8(gb, obj_state);
	alu_cp(gb, gb->cpu.a, state);
	if (!(gb->cpu.f & FLAG_Z))
		return false;
	epi_crowd_step(gb, mask);
	return true;
}

static void epi_func_af8bc(GB *gb)
{
	SET_HL(gb, wSceneObj10State);
	gb->cpu.a = rd8(gb, wSceneObj10State);
	alu_cp(gb, gb->cpu.a, 0x1b);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	epi_crowd_step(gb, 0x03); // .Func_af8d0
	epi_crowd_member(gb, wSceneObj11State, 0x1c, 0x07);
	epi_crowd_member(gb, wSceneObj12State, 0x1b, 0x03);
	epi_crowd_member(gb, wSceneObj13State, 0x1c, 0x07);
}

static const Handler epi_table[] = {
	prologue_next, // Func_af135
	epi_func_af197,
	epi_func_af19c,
	NULL, // Func_ac9ca
	epi_func_af7af,
	prologue_set_fade_to_black, // .Func_af2d0
	NULL, // PrologueFade
	epi_func_af1e7,
	epi_func_af207,
	epi_func_af215,
	epi_func_af222,
	epi_func_af230,
	epi_func_af23e,
	NULL, // Func_ac9ca
	epi_func_af741,
	NULL, // InitTextPrinting
	NULL, // Func_aca9e
	epi_func_af2bc,
	NULL, // LoadCurTextPageBufferPage
	NULL, // Func_acacd
	epi_func_af2c1,
	NULL, // Func_acaf3
	epi_func_af670,
	prologue_set_fade_to_black, // .Func_af2d0
	NULL, // PrologueFade
	epi_func_af2df,
	epi_func_af2f2,
	NULL, // Func_ac9ca
	epi_func_af436,
	prologue_set_fade_to_black, // .Func_af2d0
	epi_func_af34a,
	epi_func_af353,
};

PORT_NESTED(Func_af13c)
{
	// call .Func_af14c
	enter(gb, EPI_RET);
	epi_func_af8bc(gb);
	SET_HL(gb, w2d014);
	wr8(gb, w2d014, alu_inc(gb, rd8(gb, w2d014)));
	gb->cpu.a = rd8(gb, w2d013);
	if (!dispatch(gb, EPI(0x714c) + 11, epi_table, 32, EPI_RET))
		return;
	leave(gb, EPI_RET);

	if (!call_ret(gb, ADDR_Func_ac074, ADDR_Func_af13c + 6) ||
		!call_ret(gb, ADDR_UpdateSceneWario, ADDR_Func_af13c + 9) ||
		!call_ret(gb, ADDR_Func_ac080, ADDR_Func_af13c + 12) ||
		!call_ret(gb, ADDR_ClearUnusedVirtualOAM, ADDR_Func_af13c + 15))
		return;
	RET();
}

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

PORT(_InitEpilogue)
{
	epi_clear_screen(gb);
	stop_music2(gb);

	gb->cpu.a = 0x02;
	wr8(gb, rWBK, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);

	CALL_2B(VBlank_PrologueEpilogue);
	CALL_2B(ClearGeneralSceneRAM);
	CALL_2B(ClearSceneObjsRAM);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTempPals1);
	SET_BC(gb, 8 * PAL_SIZE);
	CALL(WriteAToHL_BCTimes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTempPals2);
	SET_BC(gb, 8 * PAL_SIZE);
	CALL(WriteAToHL_BCTimes);

	epi_decompress(gb, BANK_EpilogueGfx, ADDR_EpilogueGfx, v0Tiles0);
	epi_decompress_vram1(gb, BANK_FontGfx, ADDR_FontGfx, v1Tiles0);

	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	SET_HL(gb, v0Tiles1);
	SET_DE(gb, wAttrmap);
	SET_BC(gb, 0x50 * TILE_SIZE);
	CALL(CopyHLToDE_BC);
	SET_HL(gb, ADDR_EpilogueStarGfx);
	SET_DE(gb, v1Tiles1 + TILE_SIZE * 0x40);
	gb->cpu.c = 0x00; // aka $100 bytes
	gb->cpu.b = BANK_EpilogueStarGfx;
	CALL(CopyFarBytes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);

	epi_decompress_vram1(gb, BANK_SpearheadGfx, ADDR_SpearheadGfx, v1Tiles0);
	epi_decompress_vram1(gb, BANK_HammerBotGfx, ADDR_HammerBotGfx, v1Tiles0 + TILE_SIZE * 0x40);
	epi_decompress_vram1(gb, BANK_DoughnuteerGfx, ADDR_DoughnuteerGfx, v1Tiles1);

	epi_decompress_vram1(gb, BANK_EpilogueTransformationAttrmap, ADDR_EpilogueTransformationAttrmap, v1BGMap0);
	epi_decompress(gb, BANK_EpilogueTransformationTilemap, ADDR_EpilogueTransformationTilemap, v0BGMap0);
	epi_decompress_vram1(gb, BANK_PrologueAttrmap, ADDR_PrologueAttrmap, v1BGMap1);
	epi_decompress(gb, BANK_PrologueTilemap, ADDR_PrologueTilemap, v0BGMap1);

	CALL_2B(FillClearedTextBuffer);
	gb->cpu.a = rd8(gb, wLanguage);
	uint16_t text = alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_TextEN_OldManThankYou : ADDR_TextJP_OldManThankYou;
	epi_decompress(gb, BANK_TextJP_OldManThankYou, text, wTextBuffer);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d013, 0);
	wr8(gb, w2d014, 0);
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);

	epi_apply_lcdc(gb);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
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
