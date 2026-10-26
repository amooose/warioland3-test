# Porting routines to C

This is the guide for hand-porting asm routines, for people and for AI
assistants doing the porting.

## The workflow

1. Pick a routine (see "What to port next" below).
2. Write its C version in `src/ported/`, mirroring the asm file layout:
   `src/home/copy.asm` becomes `src/ported/home/copy.c`.
3. Build and fuzz it against the original:
   ```sh
   cmake --build port/build
   ./port/build/wl3 --fuzz --input port/tests/inputs/boot_through_n1.input --fuzz-routine Label
   ```
   The fuzzer takes real calls of the routine from the replay (or makes a
   synthetic call if the replay never reaches it), then checks hundreds of
   variations of each: random registers, and mutated values in the memory
   the routine reads. Every check runs the asm and the C from the same state
   and compares everything.
4. Fix every `FAIL`, and look at the `asm cover` column: it counts the
   instructions of the original routine that the checks executed. A routine
   is done when it passes with all of them covered (`--fuzz-verbose` lists
   the ones that weren't). An uncovered instruction is code no check
   compared, so a bug there would go unnoticed.
5. Run the replay tests too (`ctest --test-dir port/build`), which check the
   routine in real gameplay, and play the affected part of the game.

## Writing a ported routine

```c
// Port of src/home/clear_memory.asm
#include "ported.h"

// clears whole wVirtualOAM
PORT(ClearVirtualOAM)
{
	SET_HL(gb, wVirtualOAM);
	gb->cpu.b = OAM_SIZE;
	gb->cpu.a = 0;
	CALL(WriteAToHL_BTimes);
	wr8(gb, wVirtualOAMByteSize, 0);
	gb->cpu.f = FLAG_Z; // xor a
}
```

- `PORT(Label)` replaces the asm routine at `Label`. The build finds it
  automatically; there's nothing to register.
- Every label in the disassembly is available as a constant through the
  generated `syms.h`: RAM labels by name (`wVirtualOAM`, `hCallFuncBank`),
  ROM labels as `ADDR_Label` / `BANK_Label`. RAM labels also have
  `BANK_Label` (their WRAM or VRAM bank).
- Every constant (`DEF ... EQU`, `const` lists, everything in
  `src/constants/` and `hardware.inc`) is available through the generated
  `constants.h`, with the value rgbasm computed: `OAM_SIZE`, `rLCDC`,
  `JOYP_GET_BUTTONS`, object and state IDs, and so on.
- Read and write memory with `rd8` / `wr8` / `rd16` / `wr16`. Hardware
  registers are addresses too: `ldh [rLCDC], a` is `wr8(gb, rLCDC, a)`. Never
  index the memory arrays directly: the accessors apply banking and hardware
  side effects exactly like the asm.
- Call other routines with `CALL(Label)` (same bank or ROM0) and
  `FARCALL(Label)` (the `farcall` macro). It doesn't matter whether the
  callee has been ported; unported ones run in the interpreter. Arguments and
  results pass through `gb->cpu`, just like in the asm.
- `bankswitch(gb, bank)` is the `bankswitch` macro.

### Registers and flags

The difftest compares every register and flag at exit, because the callers
are still asm and may rely on any of them. Set the registers to exactly what
the asm leaves in them, including the flags from the last flag-setting
instruction (the existing ports show the usual patterns).

When a register's exit value is provably garbage (no caller reads it), you can
declare it clobbered instead:

```c
PORT_EX(SomeRoutine, 0, R_F | R_B)
{
	...
}
```

Only do this after checking every caller. Once all callers of a routine are
native, its register interface can be replaced with real C parameters and
return values; that cleanup is a later phase (see ROADMAP.md).

### Routines that don't end in `ret`

`PORT()` assumes the routine returns with `ret`; the dispatcher performs that
`ret` after your C function returns. A routine that ends by jumping
elsewhere (`jp Foo`, `jr Foo`, `jp hl`, or falling through into the next
label) is a `PORT_JUMP()`, and ends with `JUMP(Foo)` (or `JUMP_TO(address)`
for computed jumps); paths that return use `RET()`:

```c
PORT_JUMP(Func_e2b)
{
	gb->cpu.a = rd8(gb, wCurObjFlags);
	if (!alu_bit(gb, gb->cpu.a, OBJFLAG_ON_SCREEN_F)) {
		RET();
		return;
	}
	JUMP(Func_e31); // fallthrough
}
```

Don't write a tail jump as `CALL(Foo)`: a call pushes a return address that
the asm doesn't, and code that inspects or pops the stack (Game Boy code
sometimes returns two levels at once that way) then behaves differently.
The fuzzer catches this.

### Fallthrough and shared code

Asm routines often fall through into the next label, or several entry points
share one body (`ClearBGMap0` / `FillBGMap0_With7f` / `FillBGMap0`). Port each
entry point that's called from elsewhere. Shared bodies become a static
helper or a `CALL` to the shared entry point.

### Things that need care

- **Waiting on hardware.** Ported code runs in zero emulated time, so a loop
  that polls `rLY` or `rSTAT` (`wait_hblank`, `wait_not_hblank`, waiting for a
  scanline) must let time pass with `idle()`, as the asm loop would:
  ```c
  while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
  	idle(gb, 32); // wait_hblank
  ```
  The difftest sees that such a routine never finishes with time frozen and
  checks it with time flowing instead, ignoring the registers that only
  reflect elapsed time (LY, DIV, TIMA, IF, the HDMA countdown, the sound
  channels' internal timers). `WaitNextHBlank`, `DisableLCD` and
  `SetWarioPal` are examples. Waiting for an interrupt (`halt`, or a flag an
  interrupt handler sets) is still unsupported: interrupts are off during
  checks, so leave those in asm for now.
- **Interrupt handlers and the main loop** come last, once the routines they
  call are native.
- **Bank-sensitive code.** `CALL` aborts if a ROMX routine is called while
  another bank is mapped, which catches most bank bugs straight away.
- **8-bit arithmetic.** Use `uint8_t` and keep wraparound and carry behaviour
  identical. When the asm uses a carry from `add`/`sub` in a later branch,
  compute it explicitly.

## Timing

Ported routines run in zero emulated time. With them, the game finishes
each frame's work sooner than the original, so it has fewer slowdown
frames and shorter loads: that changes when things happen, not what
happens. It also means a whole playthrough with ported code doesn't match a
pure-asm playthrough frame for frame (the native run reaches the intro about
11 frames sooner, just from clearing RAM at boot), so comparing whole runs
says nothing about correctness. The difftest checks each routine in
isolation instead, which is independent of timing, and it always replays on
the original's timing.

## Coverage

The fuzzer's `asm cover` column is the measure that matters: which
instructions of the original routine were executed and compared. The replay
difftest only checks calls that happen during a recording, so it adds
confidence about real gameplay but doesn't need to reach everything.
Recordings (`--record`) are still welcome for that, and they give the fuzzer
more realistic starting points.

## What to port next

```sh
cmake --build port/build --target progress
```

lists the routines that are ready to port, meaning everything they call is
native already, most-called first (full list in `port/build/progress.md`).
Porting in that order means each new routine only calls native code, and the
most widely used routines get done first.

In general, work outward from small, frequently called leaf routines in `src/home/`
(memory, copying, decompression, OAM, palette helpers), then whole subsystems
from the bottom up, so each new routine's callees are already native. Good
early subsystems are the self-contained ones: decompression, the sprite/OAM
builders, the audio engine (`src/audio/`, `src/home/audio*.asm`). The level
engine (`src/engine/level/`) is more than half of the game's code; take it on
once the home bank is native.

## Debugging

- `--break Label` prints the last 256 instructions every time `Label` is
  reached.
- `--trace` prints the CPU state once per frame.
- `--no-hooks` runs everything in the interpreter, to check whether a bug is
  in ported code at all.
- The interpreter prints its history and exits on an illegal opcode, which
  usually means a bad jump or a wrong ROM bank.
