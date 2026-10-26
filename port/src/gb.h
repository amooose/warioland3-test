// Core machine state for the Wario Land 3 PC port.
//
// The port runs the game as a hybrid: routines that have been hand-ported to C
// are registered as hooks (see hooks.h) and run natively, everything else is
// executed by the SM83 interpreter (cpu.c) straight from the ROM that the
// disassembly builds. All code, native or interpreted, shares this one machine
// state, so the two can call each other freely.
#ifndef GB_H
#define GB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GB_SCREEN_W 160
#define GB_SCREEN_H 144

// CPU clock in single-speed mode (T-cycles per second).
#define GB_CLOCK_HZ 4194304
#define GB_CYCLES_PER_LINE 456
#define GB_LINES_PER_FRAME 154
#define GB_CYCLES_PER_FRAME (GB_CYCLES_PER_LINE * GB_LINES_PER_FRAME)

// The cartridge has 32 KiB of battery-backed RAM.
#define GB_SRAM_BANKS 4

#define AUDIO_SAMPLE_RATE 48000
// Enough for a little over one frame of stereo audio.
#define AUDIO_BUFFER_FRAMES 4096

// Interrupt flag bits
enum {
	INT_VBLANK = 1 << 0,
	INT_STAT = 1 << 1,
	INT_TIMER = 1 << 2,
	INT_SERIAL = 1 << 3,
	INT_JOYPAD = 1 << 4,
};

// Flag register bits
enum {
	FLAG_Z = 0x80,
	FLAG_N = 0x40,
	FLAG_H = 0x20,
	FLAG_C = 0x10,
};

// Joypad buttons, as a bitmask (1 = pressed)
enum {
	BTN_RIGHT = 1 << 0,
	BTN_LEFT = 1 << 1,
	BTN_UP = 1 << 2,
	BTN_DOWN = 1 << 3,
	BTN_A = 1 << 4,
	BTN_B = 1 << 5,
	BTN_SELECT = 1 << 6,
	BTN_START = 1 << 7,
};

typedef struct {
	// Registers. Kept as individual bytes; use the REG_* helpers for pairs.
	uint8_t a, f, b, c, d, e, h, l;
	uint16_t sp, pc;
	bool ime;
	int ime_delay; // EI enables interrupts after the next instruction
	bool halted;
	bool stopped;
	bool double_speed;
	bool speed_switch_armed;
} CPU;

typedef struct {
	int lx;  // dot within current line (0..455)
	int mode;
	bool stat_line; // previous state of the combined STAT interrupt line
	bool hdma_active;
	int window_line; // internal window line counter
	// ARGB8888, GB_SCREEN_W * GB_SCREEN_H. Allocated separately so copies
	// of the machine state (the difftest makes many) stay small.
	uint32_t *framebuffer;
	bool frame_ready;
	// Per-pixel BG info for sprite priority while rendering a line.
	uint8_t line_bg_color[GB_SCREEN_W];
	uint8_t line_bg_attr[GB_SCREEN_W];
} PPU;

typedef struct {
	bool on;
	bool dac;
	int length; // remaining length counter
	bool length_enable;
	int volume;
	int env_period, env_timer;
	bool env_up;
	int freq;    // 11-bit frequency value
	int timer;   // frequency timer, counts down in T-cycles
	int duty_pos;
	int duty;
	// Channel 1 sweep
	int sweep_period, sweep_timer, sweep_shift;
	bool sweep_down, sweep_enabled;
	int sweep_shadow;
	// Channel 3 wave
	int wave_pos;
	// Channel 4 noise
	uint16_t lfsr;
} APUChannel;

typedef struct {
	APUChannel ch[4];
	bool power;
	int frame_seq_timer;
	int frame_seq_step;
	// Resampling
	double sample_acc;
	int16_t *buffer; // AUDIO_BUFFER_FRAMES stereo frames, allocated separately
	int buffer_len; // in stereo frames
} APU;

typedef struct GB {
	CPU cpu;
	PPU ppu;
	APU apu;

	uint8_t *rom;
	size_t rom_size;
	int rom_banks;
	int rom_bank;    // currently mapped ROMX bank
	int sram_bank;
	bool sram_enabled;

	uint8_t vram[2][0x2000];
	uint8_t wram[8][0x1000];
	uint8_t sram[GB_SRAM_BANKS][0x2000];
	uint8_t oam[0xa0];
	uint8_t io[0x80];
	uint8_t hram[0x7f];
	uint8_t ie;

	uint8_t bg_pal[64];
	uint8_t obj_pal[64];

	uint16_t div_counter;
	uint8_t joypad; // BTN_* bits

	uint64_t cycles; // total T-cycles executed (in CPU speed units)
	bool sram_dirty;
	int lcd_off_dots; // time since the last frame while the LCD is off

	// Called once per frame at the start of VBlank (or every frame's worth
	// of time while the LCD is off). The frontend presents the framebuffer,
	// paces to 60 Hz, plays audio and polls input from here. Because it is a
	// callback, native code may run across frame boundaries freely.
	void (*frame_cb)(struct GB *gb);
	void *user;
	uint64_t frame_count;

	// Set while the difftest runs a routine in isolation: gb_call then
	// executes without advancing the rest of the hardware.
	bool no_tick;
	// How many gb_call()s are in progress. Save states can only be taken or
	// loaded at depth 0, when no native code is mid-routine.
	int call_depth;

	// Support for the difftest and fuzzer; all zero in normal play.
	struct {
		bool active;    // illegal opcodes set crashed instead of exiting
		bool crashed;
		// Memory accesses left before out_of_budget is called (0: no limit).
		// Stops native code that loops forever on garbage input.
		uint64_t budget;
		void (*out_of_budget)(struct GB *gb);
		// Offsets into this struct of the RAM bytes read, if read_log_cap.
		uint32_t *read_log;
		int read_log_len, read_log_cap;
		// One byte per ROM byte, set when an instruction there executes.
		uint8_t *exec_map;
		// Set when the run does something real game code never does, which
		// a fuzzed input can provoke: a data access to the stack (other than
		// push/pop/call/ret), switching the ROM bank from code running in
		// the switchable bank, or executing RAM other than HRAM. Native and
		// asm can't be compared then.
		bool unrealistic;
		bool in_stack_op;
		// Time flows in this check (see difftest.c): reads of the clock
		// registers then give whatever the moment of the read happens to be.
		bool timed;
		uint16_t stack_lo, stack_hi;
	} test;
} GB;

// Pair helpers
#define REG_AF(gb) ((uint16_t)((gb)->cpu.a << 8 | (gb)->cpu.f))
#define REG_BC(gb) ((uint16_t)((gb)->cpu.b << 8 | (gb)->cpu.c))
#define REG_DE(gb) ((uint16_t)((gb)->cpu.d << 8 | (gb)->cpu.e))
#define REG_HL(gb) ((uint16_t)((gb)->cpu.h << 8 | (gb)->cpu.l))
// Functions rather than macros so the value is evaluated exactly once
// (SET_HL(gb, gb_pop16(gb)) must pop once).
static inline void set_pair(uint8_t *hi, uint8_t *lo, uint16_t v)
{
	*hi = (uint8_t)(v >> 8);
	*lo = (uint8_t)v;
}
#define SET_BC(gb, v) set_pair(&(gb)->cpu.b, &(gb)->cpu.c, (uint16_t)(v))
#define SET_DE(gb, v) set_pair(&(gb)->cpu.d, &(gb)->cpu.e, (uint16_t)(v))
#define SET_HL(gb, v) set_pair(&(gb)->cpu.h, &(gb)->cpu.l, (uint16_t)(v))

// gb.c
bool gb_init(GB *gb, const char *rom_path);
void gb_free(GB *gb);
void gb_reset(GB *gb);
// Run until the PPU finishes a frame.
void gb_run_frame(GB *gb);
// Advance all hardware by the given number of CPU T-cycles.
void gb_tick(GB *gb, int cycles);
bool gb_load_sram(GB *gb, const char *path);
bool gb_save_sram(GB *gb, const char *path);

// mem.c
uint8_t gb_read(GB *gb, uint16_t addr);
void gb_write(GB *gb, uint16_t addr, uint8_t val);
uint16_t gb_read16(GB *gb, uint16_t addr);
// The byte of RAM (WRAM, VRAM, SRAM, OAM, I/O, HRAM) at addr in the current
// banks, or NULL for ROM and unmapped addresses.
uint8_t *gb_ram_byte(GB *gb, uint16_t addr);
void gb_write16(GB *gb, uint16_t addr, uint16_t val);
uint8_t gb_io_read(GB *gb, uint8_t reg);
void gb_io_write(GB *gb, uint8_t reg, uint8_t val);
void gb_push16(GB *gb, uint16_t val);
uint16_t gb_pop16(GB *gb);

// cpu.c
// Execute one instruction (or service an interrupt). Returns T-cycles used.
int cpu_step(GB *gb);
// Call the routine at addr (in the currently mapped bank) and run it to
// completion, through hooks or the interpreter. Used by native code to call
// routines that may or may not have been ported yet.
void gb_call(GB *gb, uint16_t addr);

// Debugging: dump recently executed instructions, stop-and-dump at an address.
void cpu_dump_history(GB *gb, int n);
void cpu_set_breakpoint(int bank, uint16_t addr);

// ppu.c
void ppu_tick(GB *gb, int cycles);
void ppu_lcd_off(GB *gb);
void ppu_hdma_start(GB *gb, uint8_t val);

// apu.c
void apu_reset(GB *gb);
void apu_tick(GB *gb, int cycles);
uint8_t apu_read(GB *gb, uint8_t reg);
void apu_write(GB *gb, uint8_t reg, uint8_t val);

#endif
