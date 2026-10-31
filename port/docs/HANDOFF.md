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

- `build.bat` at the repo root builds the game without the interpreter in
  `port/build-game`; `build.bat dev` builds the development build in
  `port/build` (the one the tests, tools and Claude sessions use). `run`
  after either also starts it. It stops if the game is running, since that
  locks `wl3.exe`.
- `play.bat` starts `port/build-game/wl3.exe`. Arguments are passed through,
  for example `play.bat --record port\tests\inputs\x.input`; with
  `--record`, `--input`, `--hooks`, `--no-hooks` or a test option it runs
  `port/build/wl3.exe` instead, since recordings need the original's timing.
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

## C++ modules (src/native)

On branch `claude/native-c`, the register-exact C is being rewritten, one
self-contained subsystem at a time, as modern C++: classes, locals and
return values, no registers or flags, no return addresses on the emulated
stack. The first is the sound engine (src/native/audio), which replaced
src/ported/audio/engine.c, home/audio.c and home/audio_functions.c.

- **State stays in game memory.** A module keeps its state where the game
  does (the Audio RAM bank), byte for byte, read and written through
  `Byte` / `Word` (native.hpp), which go through the memory map like the
  original's accesses. Save states, the rest of the game and the tests see
  the same bytes. `Channel` and `Track` are views over the channel and track
  structs by their CHANNEL_* / TRACK_* fields.
- **Entry points keep a register contract.** Only the subsystem's entry
  points are hooks (C++ `PORT` / `PORT_EX`). Each leaves the registers the
  original leaves where its callers could see them, and declares the rest
  clobbered. The original's internal routines get `PORT_COVERED(Label);`
  so the progress report counts them.
- **Clobbered registers must be dead.** A clobbered register is only fine if
  nothing reads it before setting it. That was checked by reading the asm
  of every caller chain, and each routine on such a chain carries the same
  clobber mask with a comment: HandleSound, Func_1c8586,
  GolfBuildingStateTable, UnreferencedUnmuteSound and the main loop's
  .after_halt / .wait_vblank_exec. When a caller does read one, give the
  entry the original's exact value instead: ExecuteAudioMod leaves bc / de
  as the original's mod functions do, because action help reads them
  (found by the action_help trace).
- **Verification is the same as for C ports:** the fuzzer checks each entry
  point against the asm (all memory, I/O, the ROM bank and the contract
  registers; the dead stack below SP is ignored), and the traces check
  whole recordings. The traces no longer compare registers at the top of
  the main loop, where none is live.
- **Timing:** a module is one native call where the C ports were many, so
  less emulated time passes per frame. Mid-frame raster timing moves (more
  "screen alone differs" frames in the traces); nothing else does.

Modules so far:

- **audio** (src/native/audio): the sound engine; 13 entry points.
- **golf** (src/native/golf): the whole minigame. `Golf` is the building,
  lobby and clear cutscene; `GolfHole` is a hole (shots, the Para-Goom's
  flight and landing, scrolling, scoring); both derive from `GolfScreen`.
  Entered through the two state tables (`_GolfStateTable`,
  `_GolfBuildingStateTable`, which clobber every register: hCallFunc
  restores af and the main loop follows). What stays register-level C is in
  golf/hole_ram.c: the V-Blank functions copied to wVBlankFunc, and
  HoldGolfWarioLastFrame, which home code calls.
- **title** (src/native/title): the title screen (`Title`: the intro with
  the plane, the start menu, the time attack description) and the game
  over screen (`GameOver`), on a shared `Screen`. Entered through
  `TitleStateTable` (b to l clobbered; a and f as pop_wram leaves them) and
  `_GameOverStateTable`; `StartOverworldState`,
  `StartOverworldStateAndNewGameParam`, `Func_4628` and
  `LoadHiddenFigureGfx`, called from elsewhere, keep exact registers. The
  rLY-timed raster effects and the V-Blank function stay C
  (title/raster.c).
- **screens** (src/native/screens): the GB incompatible notice, the
  language selection, "Perfect" and the credits (`GBIncompatible`,
  `LanguageSelection`, `Perfect`, `Credits` on a shared `Screen`). Their
  state tables are entries that clobber every register: MainStateTable
  returns to the main loop, which reloads them all (`ld a, [...]` right
  after the call). No recording reaches Perfect or the GB incompatible
  notice; the fuzzer's synthetic calls check them. The V-Blank functions
  stay C.
- **overworld, part 1** (src/native/overworld): the outer layer.
  `HandleOverworld` (entry, every register clobbered) with its state table
  and level selection, and init.asm: the map's setup (InitOverworld and its
  `.after_cutscene` entry, the map sides, the scenes, the graphics and BG
  map loading). The map's workings (bank20 to bank76, cutscene/, epilogue/,
  temple/, best_times/, treasure_collection/) are still C, called by
  address; so are Func_803e6 and Func_80bd9 (init.c), which that C calls.
  HandleOverworld dispatches through a jump table, so the fuzzer is always
  inconclusive on it: the traces check it.
- **overworld, part 2**: the treasure collection (`TreasureCollection`,
  bank26.asm's screen): pages, the cursor, links between treasures. Checked
  by `tests/inputs/collection.input` (n1_full_clear, then the top bar and
  the collection: moving, turning pages, closing). bank26.c keeps the
  treasure data routines the map and levels call.
- **overworld, part 3**: the map's common animations (`CommonAnimations`,
  bank76.asm: clouds, sea, cliffs; entries `UpdateCommonOWAnimations` and
  `_LoadCloudAndSeaGfx`, only farcalled, so they clobber every register)
  and the map side palettes (bank21's `_LoadMapSidePals` and
  `_LoadPalsForOWScene`, now `Overworld::load_map_side_pals`).
  Lesson: write rotates as rotates (`rlca`, `rra` through the carry a
  `sla` just set), not shifts. Only the fuzzer's out-of-range inputs show
  the difference; the traces never do.
- **overworld, part 4**: most of bank20, the map itself.
  - `TopBar` (top_bar.cpp, entry `UpdateTopBar`): the Select bar, its
    buttons and their states. Its later states (.CloseTopBar, .Cutscene,
    .Collection, .NextMap, the day/night ones) had never been ported: the
    collection recording ran them on the interpreter (`--asm-profile`
    showed it). Check `--asm-profile` after a module goes in.
  - `BottomBar` (bottom_bar.cpp, entries `HandleBottomBar`,
    `DrawBottomBar`): the magnifying glass bar. .SetVisibleTreasures was
    never ported either. It spins on rDIV until it picks a hidden
    treasure; native code takes no time, so each retry calls `idle()` (as
    ported.h's) to let DIV move.
  - `MapWalk` (map_walk.cpp, entry `Func_80e89`): Wario on a side, by
    w2d013's states (arriving, entering levels, walking paths, the exits,
    the temple hint, Func_8137a's steps).
  - `PathPreview` (path_preview.cpp, entries `Func_81477`, and `Func_814ec`
    and `Func_8150c` that the unreferenced Func_8143d jumps to): B held on
    a level, arrows along its paths.
  - `Scenes` (scenes.cpp, entries `Func_81dce`, `Func_818ad`,
    `Func_82bb8`, `Func_82bda`): every event's scene on the map, set up and
    played (UnlockLevel, Func_82c33's highlight).
  All the entries clobber every register: their callers reload what they
  use. The helpers other C still calls stay C and are called by address:
  Func_810fc, Func_821a1, Func_82208, GetMapLevelCoords, SetOWWarioOAM,
  Func_82111, the event and treasure checks.
  Quirks kept: Func_811cf's `ld [hl], a` writes 0 to its own address, the
  MBC's RAM bank register, since hl is still the jumptable's target; a
  `debug_nop` (an endless loop) marks the run unrealistic.
- **cutscenes** (src/native/cutscenes, bank27's event cutscenes, all
  50). `Cutscenes` hooks `DoCutsceneFunc` and `InitEventCutscene` (both
  clobber every register: their callers, UpdateCutscene and
  _InitCutscene, are annotated `PORT_EX` B..L). Each cutscene is a step
  method (switch on w2d013, frames in w2d014) plus its init and its draw
  routine. Only `PrologueFunc` (a farcall into bank2b's prologue) is
  still called by address, with the registers the jumptable leaves (a and
  hl the target, de the offset, bc w2d014). bank27.c keeps the loaders
  (LoadCutscenesNGfx, LoadCutsceneBGMapsNN, ...), which the C++ calls,
  and VBlank_Cutscene. Helpers: `wait`/`advance`/`end` (WaitCutsceneFunc,
  AdvanceCutsceneFunc, EndCutscene), `place` (y, x, SetSceneObjState),
  `rise` (the item-rises step with Func_9ce1f's whoosh), `animate`/`draw`/
  `next_state_when_done` (Func_9f0ae/0bc/0d0), `animate_all`,
  `draw_shadow` (Func_9fb53), `shake_screen` (Func_9cdcd). Read framesets
  tables at their local labels' addresses (look the offsets up in
  warioland3.sym: guessing them cost two bugs).
- **overworld, part 5**: bank56. `BestTimes` (best_times.cpp, entries
  `_InitBestTimeList`, `_BestTimeList`: the time attack's best time list;
  its V-Blank function stays C as RAM code) and `TempleRocks`
  (temple_rocks.cpp, entry `_HandleTempleShakeAndRocks`: the screen shake
  and the falling and bouncing rocks), and `TempleNotes` (temple_notes.cpp,
  entries `Func_15848a`, the music box notes, and `Func_15868e`, the
  prologue's six objects). All are only farcalled. bank56.c keeps only the
  best time list's V-Blank function (RAM code). Watch for
  names constants.h already defines as macros (`OBJ_STATE`): a C++
  constant of the same name won't compile.
- **overworld, part 6**: bank2d. `SceneObjects` (scene_objects.cpp,
  entries `Func_b4a37`, `Func_b4a3d`, `Func_b4aa9`: the map scene objects'
  per-state animation scripts, as tables of `ObjAnim` rows decoded from
  the asm jumptables, with the few specials: palette, next object, BG
  palette 5, a sound at the end). `MapFunctions` (map_functions.cpp: the
  45 scenes the four sides' events play, entry `_DoOWFunction` and each
  N/W/S/EOWFunc_* hooked with every register clobbered for the fuzzer;
  the shared steps AdvanceOWFunc, WaitOWFunc, WaitOWObjState, EndOWFunc,
  LoadOWSceneLevelName and the falling offsets are covered; the quakes'
  shaking Func_b71fc is a table of scroll steps). The magic seed and the
  snake are one routine each with per-side parameters. `SceneInit`
  (scene_init.cpp, entry `_InitOWObjects`: every scene's objects, tiles
  and graphics by w2d065, the InitOWObjects_* tables and the
  LoadOverworld*Gfx loaders) and `SideAnimations` (side_animations.cpp,
  entries `UpdateMapSideOWAnimations` and `Func_b542e`: the objects'
  idle animations between scenes, Func_b7984 and the like). bank2d.c
  keeps only the unreferenced Func_b4895.
- **overworld, part 7**: bank21 and the rest of bank20. `Palettes`
  (palettes.cpp): the day and night transitions (DayNightTransition,
  Func_84b97, Func_84c0d, OWGreyTransition: each colour stepped in a
  ColourFadeStruct), FadePalConfig and the fades built on it (the level
  name's, _PrologueFade, Func_84e43 ...), and the tile blocks (Func_854ee,
  the frozen sea's columns). FadePalConfig reads and writes in the asm's
  order (a source in wTargetRGB's page sees its own writes) and leaves
  b to e and hl as the asm: _PrologueFade and Func_84e43 are farcalled by
  bank2b's C, and the farcall passes bc, de and hl back. bank21.c keeps
  the RAM V-Blank function and ApplyPalConfig*. `Events` (events.cpp): the
  event and treasure checks (CheckIfEventIsUnlocked and the like, the next
  treasure, the missing-treasure marks, the top bar's selectable
  buttons), called directly by the C++ that used them by address. The
  path helpers (Func_810fc, Func_821a1, Func_82208, Func_82111) and
  SetOWWarioOAM are MapWalk's; bank20.c keeps GetMapLevelCoords (C calls
  it) and unreferenced routines.
- **overworld, part 8**: bank2b, top down (the scenes first; their shared
  helpers, scene Wario and the text printing stay C, called by address,
  until their last C caller goes). `GolfBuilding` (golf_building.cpp,
  entries `_GolfBuilding` and `Func_adbfe`: the building outside, its
  palettes by crayon); its V-Blank function stays C (RAM code).
  `Epilogue` (epilogue.cpp, entries `_InitEpilogue`, `Func_af13c`: the old
  man's thanks, the music boxes, the transformation, the crowd, the gift,
  Wario thrown out of the castle; no recording reaches its later steps, so
  they're checked by the fuzzer alone). `Prologue` (prologue.cpp, entry
  `Prologue`, which PrologueFunc's C farcalls: its registers are left by
  Func_ac3fd, as the asm's; every sub-scene hooked for the fuzzer, the
  plane's four falling parts one table-driven routine). bank2b_2.c keeps
  Prologue_Delay/Next/Finish, the sub-step helpers and Func_af130/5 (C
  still uses them) and the unreferenced Func_af01f.
  `TempleScene` (temple_scene.cpp, entries `_InitTempleScene`,
  `_UpdateTempleScene` and every sub-scene of the hidden figure's reveal:
  the music boxes, the shake and the rocks' drops, Wario's panic run, the
  hands). `SceneWario` (scene_wario.cpp, entries `UpdateSceneWario` and
  `SetSceneWarioState`: the 61 states' animations as a table, each group's
  sprites and tiles). SetSceneWarioState leaves every register as the asm
  (C still calls it); UpdateSceneWario with no state leaves de, hl, a and f
  as JumpTable to .None does (InitTempleScene's C passes them on).
  `SceneSprites` (scene_sprites.cpp: the epilogue's, temple's and
  prologue's object machines, Func_ac0b6, UpdateTempleSceneObjAnimation,
  Func_ac409, as tables; they find the object by UpdateOWAnimation's hl and
  its counter by inc c, as the asm, and call a state address outside the
  scene objects unrealistic). `SceneCommon` (scene_common.cpp: bank2b's
  shared helpers, called directly by the scenes' C++: the text box's
  printing, a character per few frames through wPendingCharDest, its
  pages and clearing by HDMA, the next objective's text, scene Wario's
  walking, clearing the scenes' RAM, the palette configs). Its hooks
  clobber every register: only the fuzzer and _InitPrologue's C call them
  by address, and that C reads no register after them. bank2b_1.c keeps
  the unreferenced routines and the V-Blank functions copied to RAM.
- Testing gotcha: TaskStop on a background test job kills only its outer
  shell; the MSYS bash, ctest, fuzz_batch's sh and their wl3 runs go on and
  hold build/wl3.exe (the next link fails with "Permission denied", and a
  later run then tests the old binary). Wait for them with PowerShell
  (Get-Process ctest / wl3 / sh): Git Bash has no tasklist.
- **pause** (src/native/pause): the pause menu (`PauseMenu`: the options,
  keys and treasures, its state table) and the save screens (`SaveScreen`:
  the progress bar, saving the game or the level in progress to SRAM with
  checksums and backups). Entries: `_PauseMenuStateTable`,
  `_ReturnToPauseMenuFromActionHelp` and `LoadPauseMenuSprite` (hl = the
  object), all clobbering every register. Still C: the checksums and save
  validation (pause/save/*.c), the save screen's loaders (bank7d.c) and
  DrawLevelObjectsAfterLevelReturn (home code calls it).
- **clear** (src/native/clear): the level clear screens (`ClearScreen`):
  treasure and music boxes, the time attack result, the musical coins tally
  with the pencil. The bank-$78 loaders run through `in_bank`. Still C: the
  V-Blank functions, and `_CheckLevelMusicalCoinFlag` (home code calls it
  with registers).
- **action help** (src/native/action_help): the whole screen. `ActionHelp`
  is the screen itself (state table, the power-up shown, the power-up bar,
  input, loaders); the 13 demonstrations are `Demo` subclasses (demo.hpp),
  on shared bases `WaterDemo` (swimming flippers, prince frog's gloves),
  `GrabDemo` (grab gloves, super grab gloves) and `SpecialDemo` (owl, rail,
  vampire, which run in the bank of common2.asm). Each demo is a state
  machine on wActionHelpState; what they show (prompts, Wario's looks,
  enemies, debris, sprites: common1.asm and common2.asm) is in helpers.cpp.
  Entered through `_ActionHelpStateTable` (every register clobbered).
  Still C (action_help/pals.c): the palette loaders, which copy a palette
  per HBlank waiting on rSTAT, and the V-Blank function.
  `tests/inputs/title_demos.input` (no input, 80000 frames from boot) lets
  the title screen's demo cycle through all 13 demonstrations: its trace is
  what checks them.
- **level objects** (src/native/level): the enemies and things in levels
  (src/engine/level/objects), one file each (the simplest share
  objects/small.cpp), each a class over `Object` (object.hpp: wCurObj's
  fields, the home object routines called by address). An object's states
  stay ROM addresses (ADDR_FooFunc + the local label's offset, checked
  against the .sym) written to wCurObjUpdateFunction, and each state is a
  hook. Update functions are only called by UpdateObject's hCallFunc, after
  which UpdateObjectAnimation / UpdateObjectFrame and CheckObjectOnScreen
  load every register before reading one: every hook clobbers all
  registers. A few routines take registers in (TreasureFunc's de,
  PillarPlatformFunc's b) or work through h's page (SmallLeafFunc.Init,
  UnusedFlowerProjectileCommonFunc, HandFunc.ApplyOffsetToWario: h is
  wCurObj's page from their real callers; with h in ROM the asm's writes
  hit the MBC, so the order of the two bytes matters, and set_update
  writes the high byte first, as the asm; a `dec [hl]` through h's page
  tests the value it computed, not a read back, which ROM and blocked
  VRAM wouldn't return; after a SetObjectFramesetPtr, hl is wCurObj's
  again). Quirks kept: Nobiiru's sting
  step stores its new interaction type as the collision box's left (the
  asm reuses a). The jumptable objects (the rock, Nobiiru, the hand) use
  tables of handlers; the rock's is checked against the asm's dw list.
  Locals the C left to the interpreter (the sparks', the tadpole spawner's)
  are hooked now. Never wrap PORT_EX / PORT_LOCAL_EX in a macro:
  tools/gen.c finds hooks by their text, and a macro's hooks compile but
  are never registered (fuzz_batch reports them NOT FOUND).

`src/native/routines.hpp` wraps the register-level C routines a module calls
(DisableLCD, FarDecompress, the golf sprite helpers...): each sets the
registers the routine takes and returns what it returns. `farcall` calls a
routine through hCallFunc; `in_bank` is farcall for code inside a module
(it writes hCallFunc's bank and pointer bytes and switches banks as the
original does). RAM by name: `wram_.timer()` is the byte at wTimer, `hram_.demo_power_up()`
the one at hDemoPowerUp (`src/native/ram.hpp`; names from
`generated/ram_fields.h`, which `gen ram` makes from the .sym file:
wFooBar -> foo_bar, unnamed wc08d -> unk_c08d; `.word()` for a dw). Use
these in module code; `ram(addr)` / `mem_[addr]` remain for I/O registers,
VRAM and computed addresses. `src/native/menu_object.hpp` is the menu_obj_struct
(position, frame, UpdateObjAnim animation, LoadSprite).

Init routines usually start with DisableLCD, which waits for V-Blank, so
the fuzzer's checks of them are inconclusive and only the traces check them
(golf part 1's course menu had its score digits in the wrong rows; only the
golf trace caught it). Make sure a recording reaches each new module.

Next candidates: subsystems with a narrow boundary, like the audio engine
had. Check a candidate's entry points and their callers' register use first.

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

2. **`Func_818ad` on `collection.input`** (the map's scenes, now
   scenes.cpp) fails 1 of 1005 fuzz checks (seed from call 512, variant
   15). The asm run ends with `ime` 0, wTreasuresCollected cleared to
   treasure 0 and wTempBGPals changed, as if it reached a reset or new
   game path. The native run does not. No native sample reaches the
   scene's end (Func_81931), so the divergence is likely in a callee
   reached through fuzzed state (DoOWFunction's bank2d functions). The C
   it replaced failed this routine too, 12 of 1005 (register `c`), since
   collection.input was added after the last full fuzz run. The quick
   tier's traces pass.

3. **Coverage gaps.** `UpdateRoomAnimatedPals` (60/104: the palette copy
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
