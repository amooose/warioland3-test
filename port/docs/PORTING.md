# Porting routines to C

This is the guide for hand-porting asm routines, for people and for AI
assistants doing the porting.

## The workflow

1. Pick a routine (see "What to port next" below).
2. Write its C version in `src/ported/`, mirroring the asm file layout:
   `src/home/copy.asm` becomes `src/ported/home/copy.c`.
3. Build, then run the difftest with input that reaches the routine:
   ```sh
   cmake --build port/build
   ./port/build/wl3 --difftest --frames 2400 --input port/tests/difftest.input
   ```
4. Fix every `FAIL`. Check that the routine was actually called (its `calls`
   column isn't 0); if not, extend the input script until it is.
5. Play the affected part of the game.

A port is only done when the difftest passes with the routine exercised.

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
  ROM labels as `ADDR_Label` / `BANK_Label`. Banked RAM labels also have
  `BANK_Label`.
- Read and write memory with `rd8` / `wr8` / `rd16` / `wr16`, and I/O
  registers with `rdio` / `wrio`. Never index the memory arrays directly: the
  accessors apply banking and hardware side effects exactly like the asm.
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
`ret` after your C function returns. For routines that leave some other way
(tail jumps such as `jp SomeOtherRoutine`, `JumpTable`-style code that
manipulates the stack), use `PORT_EX(Label, HOOK_MANUAL_RET, 0)` and set
`gb->cpu.pc` (and the stack) yourself. A tail jump `jp Foo` is usually best
written as `CALL(Foo)` followed by a normal return, provided that `Foo`
returns with `ret`.

### Fallthrough and shared code

Asm routines often fall through into the next label, or several entry points
share one body (`ClearBGMap0` / `FillBGMap0_With7f` / `FillBGMap0`). Port each
entry point that's called from elsewhere. Shared bodies become a static
helper or a `CALL` to the shared entry point.

### Things that need care

- **Waiting on hardware.** Loops that poll `rLY`, `rSTAT` or wait for an
  interrupt (`halt`) depend on emulated time passing. Ported code runs in zero
  emulated time, so don't port those loops as-is. Leave them in asm for now
  (call them with `CALL`), or port around them. The difftest reports such
  routines as `skipped` because they never finish in an isolated run.
- **Interrupt handlers and the main loop** come last, once the routines they
  call are native.
- **Bank-sensitive code.** `CALL` aborts if a ROMX routine is called while
  another bank is mapped, which catches most bank bugs straight away.
- **8-bit arithmetic.** Use `uint8_t` and keep wraparound and carry behaviour
  identical. When the asm uses a carry from `add`/`sub` in a later branch,
  compute it explicitly.

## Coverage

The difftest only checks calls that actually happen during its run, so
coverage matters as much as the code. `tests/difftest.input` is a scripted
playthrough. Extend it (or add new scripts) as more of the game gets ported.
The summary table lists routines that were never called; treat those as
untested.

## What to port next

Work outward from small, frequently called leaf routines in `src/home/`
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
