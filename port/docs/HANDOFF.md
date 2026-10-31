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

- **Branch:** `claude/relaxed-sagan-aui07s` on `amooose/warioland3-test`.
- **Progress (2026-10-31):** 3497 / 3497 routines are ported. While the
  port plays the recordings, the interpreter runs no instructions at all
  (from 169M; see "What still runs as asm"). Only the debug loops, the
  HRAM copies and the RAM blocks listed below are still interpreted, and
  no recording reaches them. Ordinary porting is done: "Ready to port" only lists
  routines that can't or needn't be ported (see "Good next batches"). Run
  `cmake --build build --target progress` and read `build/progress.md` for
  the per-directory numbers.
- **Tests come in two tiers:**
  - **Quick, before every commit:** `ctest --test-dir build -L quick -j6`,
    a few minutes. It runs a difftest and a per-frame trace test
    (`trace_<input>`, see "Per-frame traces") for each input in
    `tests/inputs/`. Also fuzz the routines the commit adds or changes
    with `tools/fuzz_batch.sh`.
  - **Full fuzz, only when something looks wrong:** `ctest -L fuzz`,
    about an hour, so run it in the background. It splits each input's
    fuzz pass into 4 shards (`fuzz_<input>_0` to `_3`, each fuzzing every
    4th hook with `--fuzz-shard I/4`). This is the user's rule since
    2026-10-30.
- **Inputs.** Three are committed. The user's local recordings aren't
  committed, but a local ctest picks them up: `action_help2`, `coin_bug`,
  `boss` (the Hidden Figure fight), `golf`, `yellowbelly`, `gameover`,
  `fresh_save_cutscene`, `anonster` and `credit` (which doesn't reach the
  credits yet).

## Building

**Windows (MSYS2 UCRT64), what the user runs:**

- `build.bat` at the repo root builds. `build.bat run` also starts the
  game. It stops if the game is running, since that locks `wl3.exe`.
- `play.bat` starts the game. Arguments are passed through, for example
  `play.bat --record port\tests\inputs\x.input`.
- `push.bat` pushes the current branch to
  `https://github.com/amooose/warioland3-test` (commits only).
- `build.bat` and `play.bat` have `MSYS2_DIR` (default `C:\msys64`) at the
  top; `build.bat` also has `RGBDS`.

**From a shell (MSYS2 UCRT64 or Linux), in `port/`:**

```sh
cmake -B build -S .            # rerun after adding new .c files (CONFIGURE_DEPENDS usually catches it)
cmake --build build -j8
ctest --test-dir build -j12    # ~17 min
```

A local Claude Code session on Windows usually runs Git Bash, which lacks
the MSYS2 toolchain. Run builds through MSYS2 instead. The login shell
starts in the MSYS2 home directory, so `cd` to an absolute path:

```sh
MSYSTEM=UCRT64 /c/msys64/usr/bin/bash.exe -lc "cd /c/Users/<you>/.../warioland3-test/port && cmake --build build -j8"
```

- If the game is open, `build/wl3.exe` is locked and the link fails.
  Build into a second directory instead (`cmake -B <dir> -S . -G Ninja
  -DRGBDS=C:/rgbds/`), then run `WL3=<dir>/wl3.exe tools/fuzz_batch.sh ...`
  and `ctest --test-dir <dir>`.
- Git may refuse the repo ("dubious ownership") when it's owned by
  Administrators: use `git -c safe.directory=* ...`.
- The labels and constants the port uses from the disassembly are also
  committed in `generated/` (syms.h, sym_table.c with the ROM's SHA-1,
  constants.h), so `-DWL3_BUILD_ROM=OFF` builds without RGBDS. After an asm
  label or constant change, run `cmake --build build --target
  update_generated` and commit the result; the `generated_*` quick tests
  fail until then. At runtime the port reads no .sym file and refuses a ROM
  whose SHA-1 doesn't match the labels it was built with.

## The batch workflow

Each step that worked in the cloud session:

1. **Pick routines.** In `build/progress.md`, under "Ready to port", look
   for routines whose callees are all native already.
   - Prefer high call counts (hot paths) or high instruction counts.
   - Grouping by asm file keeps shared helpers in one `.c` file.
2. **Read the asm.** Write the C in `src/ported/<same path as the asm>.c`,
   matching that file's helpers and conventions.
   - `engine/level/wario_states.h` has the small helpers the Wario state
     ports share, such as `st`, `ld_and`, `clear`, `load_gfx`, `set_oam`,
     `frameset_by_dir` and `anim`. Include it for anything that sets
     Wario's state (`object_interactions.c` does).
   - `home/fade.c` has `fade_begin`, `fade_end` and `unpack_rgb` /
     `pack_cur_rgb`.
3. **Build.**
4. **Fuzz each new routine on every input:** `tools/fuzz_batch.sh A B C ...`
   from `port/`. It replays each input once for the whole list
   (`--fuzz-routine A,B,C`): the replay takes 2-70 s, far longer than
   fuzzing one routine. It prints `R input: PASS (passed/failed/inconclusive,
   cover n/m)` per (routine, input) and the first lines of any failure. A
   batch of 10 routines takes about a minute and a half.
5. **Fix any FAIL.** The report shows the entry registers and the first
   differences. For more detail, run `./build/wl3 --fuzz --input X
   --fuzz-routine R --fuzz-verbose`.
6. **Run `ctest`.** It fuzzes every routine, so it catches a change to a
   shared helper breaking something else.
7. **Commit and push.** Each commit message lists the routines and the
   verification.

### Porting in parallel with agents

- **Local labels:** `PORT_LOCAL(Parent, local)` / `PORT_LOCAL_JUMP` hook
  `Parent.local`, for example an object's `.Update` states reached through
  its update pointer. Fuzz them as `--fuzz-routine Parent.local`.
- **Stashes:** `git stash` is shared by every worktree, so an agent can pop
  another agent's stash. Have agents commit WIP to their own branch instead.
- **Fuzz results:** a routine name that matches no hook prints `NOT FOUND`
  (check for typos and a trailing CR). A PASS with 0 passed checks, or a
  low asm cover, verified little: a missing `RET()` once hid behind an
  all-inconclusive PASS.
- **Commit early:** agents must commit verified work often. A session that
  hits the usage limit leaves everything uncommitted stranded.
- **Few agents:** six large agents at once used up a whole session's usage
  limit before most of their work was verified. Two or three, each with a
  small batch, works better.
- **Session restarts** kill the agents' background tasks. Check their task
  output for "[killed]"; a leftover `wl3.exe` process doesn't mean they're
  still running.

Four agents ported 512 routines at once this way:

- Make a git worktree per agent by hand (`git -c safe.directory=* worktree
  add -b port/<area> ../wl3-wt-<area> HEAD`). The Agent tool's own worktree
  isolation refuses this repo (the ownership check fails).
  - Copy `warioland3.gbc` / `.sym` and the local recordings into the
    worktree.
  - Configure it without the ROM build: `cmake -S . -B build -G Ninja
    -DWL3_BUILD_ROM=OFF -DRGBDS=C:/rgbds/`.
- Give each agent a disjoint set of files (objects; Wario states and
  mechanics; overworld; everything else), and forbid edits to `ported.h`
  and the docs, so the commits cherry-pick cleanly.
- Limit each agent to `cmake --build -j2`, `JOBS=2` fuzzing and
  `ctest -R difftest -j2`: five agents at full parallelism peg the machine.
- Verify after merging with the full ctest here: the worktrees lack the
  local `action_help2` recording, which caught a bug the agents' runs
  missed.
- Before committing, scan for `CALL(X)` / `JUMP(X)` to a routine in the
  caller's own ROMX bank (they abort when a fuzzed bank is mapped; use
  `gb_call` / `JUMP_TO`). A short script over `warioland3.sym` and the
  `PORT(...)` blocks finds them.

### Pitfalls that bit before

- **`--input` runs the asm.** Replaying or recording turns the ported
  routines off unless `--hooks` is given too. Check what the port does
  with `--hooks --input X --watch ...`.
- **Native code takes no emulated time.** Code that reads LY to see how
  late the frame is runs differently. The animated tiles and palettes
  stopped for that reason until the native main loop let V-Blank end
  first. The traces replay LY at `UpdateRoomAnimatedTiles` from the asm,
  so they can't catch this.
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
- **Anything that reaches `ProcessBlock` from C.** Block functions run
  whatever the level's table points at, and a plain `CALL` / `gb_call`
  pushes a sentinel return address instead of the asm's. Use
  `gb_call_ret(gb, addr, ret)` or `FARCALL_RET(Label, ret)`, which push the
  asm's own return address (`ret` = the address after the asm's `call`).
  Get the offset from the ROM rather than by counting bytes: find the
  `call` opcode (`cd lo hi`) inside the routine. `CheckAirborneCollision`,
  `CheckCentreCollision`, `IsOnSlipperyGround` and the `Func_35xx`
  wrappers do this.
- **Local labels** (jumptable targets, data tables after a routine) aren't
  in `syms.h`; take their offsets from `warioland3.sym`, which has them
  (`Func_810fc.PathNone`), and write `ADDR_Routine + offset`.
  `Func_810fc` and `SetSceneWarioState` do this.
- **Jump-table offsets are never verified.** A routine that dispatches
  through its own `rst JumpTable` jumps into its own code, so the difftest
  and the fuzzer always call it inconclusive. A wrong table address passes
  every test and only shows in play. `CoinFunc`'s table was 3 bytes off,
  so coins re-ran their pickup state forever: lag and an endless pickup
  sound. Check each table address against the ROM: the byte before it must
  be `$ef`. Check each `gb_call_ret` return address too: the bytes before
  it must be a `call` to the same routine.
- **Bank changes after `gb_call_ret`.** A callee (a block function, for
  example) can come back with another ROM bank mapped. The asm then returns
  into whatever that bank holds, so check `gb->rom_bank` after the call and
  `JUMP_TO(ret)` when it changed. The routine has to be a `PORT_JUMP` for
  that. `Func_19b5c` and `CheckBottomRightCollision` do this.
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
  - code running from RAM, except the buffers the game builds code in on
    purpose (`wVBlankFunc`, `wLCDFunc`, `wVBlankFuncExtended`), and those
    only when entered the game's way: by the instruction the game enters
    them with (or its routine's native version), then running on or
    returned into from their own calls (`wram_code_ok` in cpu.c). Allowing
    the buffers outright let fuzzed jumps count as real, and 18 checks
    failed;
  - a V-Blank or LCD interrupt serviced in a check: the handlers do real
    work, at moments that differ between the native and the asm run;
  - the asm running the routine's own address range with another bank
    mapped, but only when ROMX code switched the bank under itself or a
    `ret` came back with another bank than its `call` was made in. A
    farcall into another bank's code at the same addresses is fine
    (`Func_8086f` does that).
- **Native recursion.** Under test, `gb_call_ret` stops (unrealistic) when
  its push lands below `wStackBottom`, as the asm would overflow there.
  Before this, a fuzzed jump table leading back into its caller nested
  host stack frames until the host stack overflowed.
- **Early stop.** Once a check is unrealistic the asm run stops (or never
  starts when the native run already was), since the check can only be
  inconclusive. This cut the cutscene dispatchers from minutes to seconds.
  Likewise, when the native run never finishes, an asm run that finishes
  only after turning unrealistic is inconclusive, not a FAIL.
- **Computed jumps.** A `jp hl` to a ROM address with no label is
  unrealistic. Every one the game makes, 14,000+ per recording, goes to a
  label. A fuzzed pointer into the middle of an instruction otherwise ran
  `push bc / ret` into whatever bc held, and native code only promises
  exact registers at its exits.

### Per-frame traces (src/trace.c)

These test the port as a whole, not one routine at a time, and they are
what checks later steps once the interpreter is gone.

```
build/wl3 --headless --input tests/inputs/X.input --trace-write X.trace
build/wl3 --headless --input tests/inputs/X.input --trace-check X.trace
```

- **What's recorded.** Write replays the input in pure asm and records a
  hash of every part of the machine state each game frame, that is, each
  pass through `InitWithoutDemoPowerUpReset.GameLoop`. Check replays it
  with the native routines and compares every frame. At the first
  mismatch it reruns the asm and prints the differing bytes with their
  labels.
- **Timing.** Native code is far faster than the asm, so:
  - Frames are compared per game frame, not per video frame, leaving out
    LY, STAT, the timers, IF and the dead stack below SP.
  - Buttons are fed per game frame.
  - Reads the native code can't get the same value from are replayed from
    the trace:
    - every DIV read;
    - every data read of the stack area, $cf00-$cfff (not push, pop, call
      or ret);
    - the reads listed in `sites[]` in trace.c. Add a site there when a
      trace shows a read like `UpdateRoomAnimatedTiles`'s LY check.
  - A difference in the screen alone is counted but doesn't fail.
- **Stack reads are the original game's bugs.**
  `QueueBlockTilesAlongRow_*` writes one byte past the tile queue, into
  the first attribute queue entry, and reads that byte from the stack:
  its callers' return addresses. The port copies the overrun exactly, but
  native routines don't push the return addresses the asm does, so the
  byte differed. In the original, one tile attribute is wrong until the
  room redraws (`yellowbelly`, game frame 16296, $9929 in VRAM bank 1).
- **Results (2026-10-30).** All 10 recordings match on every game frame,
  more than 200,000 game frames in all.
- **In ctest:** `trace_<input>` writes the trace into `build/traces/` and
  checks it (tools/trace_test.cmake). A trace depends only on the
  recording and the ROM, so it can also be kept next to the recording
  once the interpreter is gone.

### What still runs as asm

`--asm-profile PATH` (with `--trace-check`) counts every instruction the
interpreter runs while the port plays a recording and writes the counts
per label, most run first. It's the real list of work left before the
interpreter can go. The progress report counts routines, and it treats
anything calling `hCallFunc` as blocked.

**State (2026-10-31):** over the 12 recordings the interpreter runs 0
instructions, down from 169M. The profile also lists every interpreted
address (`bank:addr label+offset`), which is how the last ones were found.

**The game runs without the interpreter.** It lives in src/interp.c, built
only with `WL3_INTERPRETER` (ON by default; the difftest, fuzzer, traces,
`--no-hooks` and `--break` need it). With it OFF, cpu.c's
`no_native_routine` stops with the label and the last native routines run
when the game reaches an address without a hook, except the `debug_nop`
loop (`nop / jr :-`), where it stays as time goes on. The calls between
routines still go through emulated addresses and the stack (gb_call_ret,
JUMP_TO): turning those into direct C calls is the next step.

`tools/audit_targets.py <repo> <cc>` checks statically that every constant
jump or call target in the ported code has a hook (24 `AH(...)` hits in
power_ups/ are false positives: bank $7e routines called from bank $79
files). Run it after porting changes; on 2026-10-31 it found 5 local labels
and the `HandleTopBarDPad` loops that only the interpreter could run. Every
recording plays to the end in a build without the interpreter, with the
same per-frame state as the development build with `--hooks`.

- **RAM hooks** (`ram_hook_register` in hooks.h, the code is in
  src/ported/home/ram_code.c) cover what the game runs from RAM:
  - `hCallFunc`, the HRAM trampoline behind every farcall and hcall;
  - `hTransferVirtualOAM`, the OAM DMA routine;
  - the WRAM V-Blank code, including the tile queue copy loops.

  A RAM hook first checks that the bytes are the code it stands for, and
  otherwise leaves them to the interpreter.
- **Copied V-Blank functions.** The screens copy an `X.Func ... X.end`
  (or `X.func ... X.func_end`) block into `wVBlankFunc`. ram_code.c recognises any such block by its
  bytes and runs the native routine of its ROM label, `PORT_LOCAL(X,
  Func)`. Calls inside a block return to `gb->cpu.pc + n`, the block's
  start wherever it runs from. Blocks that switch the ROM bank (the
  overworld's, the title's, two golf ones) are inconclusive in the
  fuzzer at their ROM address, so the traces are what check them.
  `VBlank_1c9f7a.Func` stays asm, because no recording reaches it and the
  fuzzer can't check it.
- **The main loop** halts natively and resumes at
  `InitWithoutDemoPowerUpReset.after_halt`. The fuzzer can't check the
  halt or the interrupt vectors; the traces do.
- **The collision tails** go through `gb_call_ret_unwind`, which stops
  where a block function pops past its caller, as the asm does.

## Open issues

1. **`Prologue_WarioFallFromPlane`** (may be stale) (`src/ported/engine/overworld/bank2b_2.c`,
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
   - **Now:** it passes `--fuzz-variants 600` on `boot_through_n1` and
     every ctest run. The routines that were held back for it
     (`ApplySceneWarioMovementLeft`, `SetObjStateIfAnimationIsFinished`,
     `Func_ac042` to `Func_ac051`) are back in `bank2b_1.c`. The root cause
     was never found, so it may still be there under other inputs.

2. **Coverage gaps.** `UpdateRoomAnimatedPals` (60/104: the palette copy
   loop only runs in levels with palette cycling), `MizzouProjectileFunc`
   (54/59), `SilkPlatformsFunc` (18/26), `TransitionAfterSave` (18/32),
   `Func_850ff` (24/30). Most transformation states (snowman, ball, fan,
   owl, vampire, puffy, crazy, bouncy) are only checked with synthetic
   calls: recordings of those would help most.

## Good next batches

**State (2026-10-30):** porting is done apart from these, which stay in
asm:

- `debug_nop` routines (an endless `jr`): `Func_b4001`,
  `InvalidMusicBoxAction`, `Func_81e44`.
- `PushOAM` and `hCallFuncPointer`: code and data copied to HRAM and run
  there, never at their ROM addresses.
- `Func_af01f` (unreferenced) is ported but can't be checked: its
  Decompress over uncompressed data writes on into the stack and the asm
  returns into garbage. The rest uses `_InitEpilogue`'s helpers.

Routines with 0 passed checks on every input are verified by nothing.
The 2026-10-31 full fuzz run left 14, down from 35. The checker now
carries on natively when a routine jumps into one of its own ported
labels (`CoinFunc` falling into `.Update`), which fixed the dispatchers
and object `Init` code. The 14 left:

- the debug loops (`Func_81e44`, `Func_b4001`, `InvalidMusicBoxAction`);
- the boot code, main loop and V-Blank vector (`InitWithoutDemoPowerUpReset`,
  `.GameLoop`, `VBlankVector`), checked by the traces;
- the V-Blank blocks that switch the ROM bank (`VBlank_1c9eb3.Func`,
  `VBlank_Cutscene.Func`, `VBlank_OWPalTransition.Func`,
  `VBlank_OWScene.Func`, `VBlank_Overworld.Func`, `VBlank_Title.Func`),
  checked by the traces;
- `Func_84b97.Func_84bad` (unrealistic on every seed) and
  `Func_84c0d.Init` (every check sees a V-Blank interrupt).

Rerun that audit after any batch: sum the passed column per routine over
`build/Testing/Temporary/LastTest.log`.

Next:

1. **`ZombieFunc.OnUnderground`** was left in asm because its bank 6
   callees sit inside its own address range. The narrower bank rule may
   make it checkable now.
2. **Recordings** of golf, the bosses (Yellow Belly, Shoot, Scowler,
   Pesce, GKTortoise) and the overworld map events: those routines are
   only checked with synthetic calls.
3. The clean-up plan: per-frame golden traces, then dropping the
   interpreter.
- **Coverage gaps:** `CoinFunc.Update` real seeds are all inconclusive. The
  big state dispatchers (`AnonsterFunc.Func_5041d`, `DollBoyBarrelFunc.Main`)
  only reach about half their code with 200 variants. `--fuzz-variants 2000`
  reaches more, and those runs pass.
- **Progress:** the progress tool counts an object as one routine, so its
  `PORT_LOCAL` update states don't raise the routine count. Use the
  instruction percentage instead.
- `build/progress.md` lists the rest under "Ready to port".

## Recordings

`tests/inputs/*.input` (plus `.state`) drive both the difftest and the
fuzzer's real seeds. More recordings that cover other areas (later levels,
transformations, the golf minigame, bosses) raise the "tested" counts, so
more routines can be ported with real calls behind them.

To record one, run `play.bat --record port\tests\inputs\name.input`, then
commit both files. Recording and replay run the original asm by default.
