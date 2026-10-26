# Handoff

Where the PC port stands and how to carry on. This was written when the work
moved from a cloud session to a local one. Read [PORTING.md](PORTING.md)
for how a routine is ported and [ROADMAP.md](ROADMAP.md) for the long-term
plan. This file covers the current state, the workflow that has worked, and
the open problems.

## The project

The long-term goal is a full PC port of Wario Land 3 in C, with modding,
scripting and expansion on top. The approach:

- `port/` is a native frontend (C + SDL2). It runs the ROM built from the
  disassembly on an SM83 interpreter.
- Routines are ported to C one at a time and registered by label as hooks
  (`PORT(Label)` and friends). With hooks on, the interpreter runs the C
  version instead of the asm.
- Every ported routine must leave **every register, every flag and all
  memory exactly** as the asm does. Native code takes zero emulated time.
- Two tools check this:
  - the **difftest**, which replays real calls from recorded inputs;
  - the **fuzzer** (`--fuzz`), which mutates registers and memory around
    real and synthetic calls and compares the asm and the C.

## Current state

- **Branch:** `claude/relaxed-sagan-aui07s` on `amooose/warioland3-test`. All
  work is pushed and the tests are green.
- **Progress:** 602 / 3491 routines ported (17.2%), 10.7% of the
  instructions. With hooks on, about half of what the game executes runs as
  C. Run `cmake --build build --target progress` and read
  `build/progress.md` for the per-directory numbers and the "ready to port"
  list.
- **Fully ported:** `src/audio`. `src/home` is about 41% ported, and about
  10% of the level engine and the overworld.
- **Tests:** `ctest --test-dir build -j6` runs 6 tests, a difftest and a
  fuzz pass for each of the 3 inputs in `tests/inputs/`.

## Building

**Windows (MSYS2 UCRT64), what the user runs:**

- `update.bat` at the repo root pulls and builds. `update.bat run` also
  starts the game.
- `play.bat` starts the game. Arguments are passed through, for example
  `play.bat --record port\tests\inputs\x.input`.
- Both have `MSYS2_DIR` (default `C:\msys64`) and `RGBDS` paths at the top.

**From a shell (MSYS2 UCRT64 or Linux), in `port/`:**

```sh
cmake -B build -S .            # rerun after adding new .c files (CONFIGURE_DEPENDS usually catches it)
cmake --build build -j8
ctest --test-dir build -j6     # ~1 min
```

A local Claude Code session on Windows usually runs Git Bash, which lacks
the MSYS2 toolchain. Run builds through MSYS2 instead:

```sh
MSYSTEM=UCRT64 CHERE_INVOKING=1 /c/msys64/usr/bin/bash.exe -lc "cd port && cmake --build build -j8"
```

## The batch workflow

Each step that worked in the cloud session:

1. **Pick routines.** In `build/progress.md`, under "Ready to port", look
   for routines whose callees are all native already.
   - Prefer high call counts (hot paths) or high instruction counts.
   - Grouping by asm file keeps shared helpers in one `.c` file.
2. **Read the asm.** Write the C in `src/ported/<same path as the asm>.c`,
   matching that file's helpers and conventions.
   - `engine/level/wario_states_{1,2,3}.c` share small helpers such as
     `st`, `ld_and`, `load_gfx`, `set_oam`, `set_frameset` and `anim`.
   - `home/fade.c` has `fade_begin`, `fade_end` and `unpack_rgb` /
     `pack_cur_rgb`.
3. **Build.**
4. **Fuzz each new routine on every input:** `tools/fuzz_batch.sh A B C ...`
   from `port/`. It prints a line per (routine, input) and the first lines
   of any failure. A batch of about 30 routines takes about 10 minutes.
5. **Fix any FAIL.** The report shows the entry registers and the first
   differences. For more detail, run `./build/wl3 --fuzz --input X
   --fuzz-routine R --fuzz-verbose`.
6. **Run `ctest`.** It fuzzes every routine, so it catches a change to a
   shared helper breaking something else.
7. **Commit and push.** Each commit message lists the routines and the
   verification.

### Pitfalls that bit before

- **`xor a` / `and a` / `cp` flags.** The flags have to be exact even when the
  next instruction is a farcall: hCallFunc pushes and pops af, so the
  caller sees the flags from just before it.
- **Calls and jumps within a ROMX bank.** The asm runs whatever bank is
  mapped. Use `gb_call(gb, ADDR_X)` / `JUMP_TO(ADDR_X)` (or the files'
  `LCALL` / `LJUMP` / `CALL_2B`), not `CALL` / `JUMP`, which abort on a
  bank mismatch. `particles.c`, `collisions.c` and `wario_states_*.c` do
  this.
- **Tail jumps are `PORT_JUMP` + `JUMP` / `JUMP_TO`.** Every exit path of a
  `PORT_JUMP` must set pc (`RET()` on a return path).
- **Anything that reaches `ProcessBlock` from C.** Block functions can read
  the stack, and a C `CALL` pushes a sentinel return address instead of the
  asm's. This is why `CheckAirborneCollision`, `IsOnSlipperyGround`,
  `Func_198c7`, `Func_19b5c` and the `Func_35xx` / `ProcessBlockOnLeft/Right`
  homecall wrappers in `home/object_mechanics.asm` are still asm.
  `collisions.c` gets around it by pushing the asm's own return address and
  jumping (`call_process_block`). Doing the same for these routines would
  unlock them, which is a good small infrastructure task.
- **`dec l` / `inc e` versus `hld` / `hli`.** The 8-bit ones don't carry into
  the high byte, so model them as 8-bit.
- **Unmasked arithmetic.** Fade components can wrap below 0 and then spill
  into neighbouring bits when repacked. Port what the asm does, not what it
  means.
- **Hardware waits.** Routines that wait on LY or VBlank, such as
  `PanIntroBackgroundLayers`, need timed mode. See "Timing" in PORTING.md.

### Fuzzer rules added recently (so their results make sense)

- **Seeding.** Each routine's RNG is seeded by its own bank and address. A
  new port no longer reshuffles the inputs other routines get. Before this,
  every batch "found" old bugs at random.
- **Unrealistic inputs.** These count as inconclusive, not as failures:
  - a push or pop below `wStackBottom` (fuzzed runaway calls overflowing the
    stack);
  - in an untimed check, the asm servicing an interrupt that the routine
    itself enabled (`ei` with one pending). Atomic native code can't match
    that.

## Open issues

1. **`Prologue_WarioFallFromPlane`** (`src/ported/engine/overworld/bank2b_2.c`,
   ported by a parallel agent with `PORT_NESTED` and `dispatch` /
   `run_until`) has a latent mismatch.
   - **How it shows:** it surfaced under certain fuzz inputs. Fuzzing it
     harder on `boot_through_n1` with `--fuzz-variants 600` showed it. The
     address seeding has changed the variants since then, so re-check which
     variant count still triggers it.
   - **Symptom:** the native run ends with `hl=69xx` (a ROM address in bank
     $2b), while the asm finishes the plane-part update (`hl=d8a5`, writes to
     `d8a4..d8a7`). A stack byte just below SP also differs.
   - **Where to look:** probably how `func_ae8bb`'s nested dispatch stops
     (`g_stop`) or resumes.
2. **Held back until (1) is fixed:** `docs/held_back/bank2b_1.c` holds
   `ApplySceneWarioMovementLeft`, `SetObjStateIfAnimationIsFinished` and
   `Func_ac042` to `Func_ac051`. They're ported but not compiled, because at
   the time they changed the fuzz inputs enough to expose (1) in ctest.
   - With address seeding they may be fine to add back. Append them to
     `src/ported/engine/overworld/bank2b_1.c`, fuzz them, and run ctest.
3. **Two locked agent worktrees** remain in `.claude/worktrees/`. They're
   leftovers from the parallel agents and are ignored by git. Remove them
   with `git worktree remove --force <path>` if they're in the way.

## Good next batches

- **`wario_states_1.asm`:** the biggest unported Wario-state file. More
  `SetState_*` routines are ready (see progress.md), and
  `engine/level/wario_states_1.c` already has the helpers.
- **`wario_states_3.asm` / `wario_states_4.asm`:** the `SetState_Zombie*`,
  `SetState_Blind*`, `SetState_Bouncy*` and `SetState_Hot` routines.
- **`update_objects.asm`:**
  - `Func_6284f` (376 calls) and `Func_628a7`;
  - `_CountActiveTransientObjects`;
  - `Func_62d57` and `Func_62e31`.
- **`block_functions.asm`:** `Func_182ec` (1,400 calls), `Func_18064`,
  `EnterDoor`, `EnterPipeUp` and `EnterPipeDown`.
- **Overworld:**
  - `FadePalConfig` (136 instructions, 870 calls, `bank21.asm`);
  - `SetSceneWarioState` (94 instructions, the hub of the cutscene state
    changes);
  - `Func_1d8c2c` (5,000 calls, `bank76.asm`).
- **Infrastructure:** the `ProcessBlock` call trampoline described above,
  then the routines it unlocks.

## Recordings

`tests/inputs/*.input` (plus `.state`) drive both the difftest and the
fuzzer's real seeds. More recordings that cover other areas (later levels,
transformations, the golf minigame, bosses) raise the "tested" counts, so
more routines can be ported with real calls behind them.

To record one, run `play.bat --record port\tests\inputs\name.input`, then
commit both files. Recording and replay run the original asm by default.
