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

- **Faithful version (2026-11-10): tag `faithful-native`** on branch
  `claude/cleanup`. Every routine is native C++ checked against the asm
  (registers, flags and memory where its contract says so), and in play
  native code calls native code directly: the CPU dispatches only the five
  frame-loop hooks (.GameLoop, after_halt, wait_vblank_exec, VBlankVector,
  VBlank), once per frame, and the entries into the loop: EntryPoint at
  boot, and Init / InitWithoutDemoPowerUpReset on a reset (game over,
  saving, the action help's return; the call never returns, so the native
  frames are dropped and the loop restarts from the top). 8 hooks in all,
  399 run directly (`tools/hook_report.sh`, 14 recordings). Kept original-game
  bugs are in `docs/ASM_BUGS.md`; the one asm quirk dropped is the block
  queue's stack read (b70ac05).
- **Modernisation: branch `claude/modernise`** (from `faithful-native`;
  worktree `../warioland3-modernise`). Order: the native frame loop (done,
  below), then the register glue of hooks nothing reaches in play, then
  typed state instead of raw addresses, ROM data as arrays, no bank
  switching, the renderer.
- **Verification in modernisation** is whole-game behaviour: the trace
  check, which already compares every game frame's memory, I/O and screen
  with the asm's at the frame's end and leaves out what only reflects
  elapsed time. The difftest and fuzzer still check the routines that keep
  their asm contract; a routine that drops it leaves their hook table.
- **Native frame loop** (start.cpp `game_loop`): in play the .GameLoop hook
  enters it and it never returns. Each pass: `loop_cb` (the frame's end:
  the frontend saves / loads states there, the trace check compares the
  frame), the frame's logic, then the wait for V-Blank without the CPU:
  idle, taking interrupts as they come (header.cpp `take_interrupt`: V-Blank
  natively, the LCD handler through gb_tail_jump), until V-Blank has run its
  function; then HandleSound. A reset in it unwinds (`in_frame_loop`), and
  a loaded state longjmps out of it to the main loop, which runs from the
  state's pc back into it. The one-pass hooks (.GameLoop with test.active,
  after_halt, wait_vblank_exec) stay for the difftest and for states taken
  mid-frame. The trace check no longer syncs on pc = .GameLoop: its frame
  ends come from loop_cb.
- **Register glue, step 1: calls by address that pass registers.** The
  objects' block checks (hpos_block_*, block_beside: collisions.cpp
  check_block / check_block_beside), their object creation
  (create_object_at_relative_pos) and the temple scene's loads call plain
  functions inside the homecall's bank switch; Overworld::call_a / call_z
  and Object::call_a / call_bc are gone, and move_wario takes only its
  four routines. Their home wrappers (object_mechanics.cpp's homecalls,
  Func_3513 and the like) are now reached only by the difftest and
  fuzzer. Block functions keep their result in BlockFuncs::a_ (from the
  caller's a); hooks hand it over in a (exit_regs, leave) and the plain
  process_block returns it.
- **Register glue, step 1 continued.** Plain in play too: Wario's bottom
  collisions for objects (check_wario_bottom: carts, Muddee's turtle, zip
  lines read the result instead of b), FadeInLevelName (its result, c only
  in the hook), GetMapSideAndLevelForNextTreasure (Events'
  next_treasure_side_level), GetBlockBGPtr (block_bg_ptr), the room
  start's row queues (queue_block_tiles/attributes_along_row), the object
  engine's block checks (ObjectEngine::block maps the wrapper to its
  collision points), and the fade screens' V-Blank palettes (fades.cpp
  apply_vblank: plain unless test.active; VBlank pops every register after
  it, so its exact registers matter only to the hook checks).
  TopBar::animate read a stale hl after the plain UpdateOWAnimation (no
  recording reaches it): it now works out the object's block as the asm
  leaves it. Register reads left in play: block functions' leave() (b, hl
  for the objects after them), the OAM DMA's a (the HRAM routine's input,
  until the renderer), shoot.cpp's latent stale b (ASM_BUGS.md), and the
  wrappers' own glue. **Deferred: compiling the test-only wrappers out**
  (the old stage 5). Which hooks the game can reach by address isn't
  known statically (object update pointers and state tables come from ROM
  data), a wrong guess crashes the game build on an unrecorded path, and
  it removes no code from play; ROM data as arrays (below) makes those
  dispatches native, after which it's a safe cut.
- **Typed RAM.** Memory (native.hpp) has every RAM label by name, generated
  from the disassembly (ram_fields.h: `mem_.timer()` is the byte at wTimer,
  as Wram already had), and the 16-bit fields as words of their byte order:
  `mem_.wario_x()` (WordBE: wWarioXPos, high byte first), wario_y, block_y,
  block_x, block_pointer, bg_pointer, sfx, music, and `cur_obj_y()` /
  `cur_obj_x()` (Word, little-endian); `.hi()` / `.lo()` reach their
  bytes. Converted mechanically (scratch scripts typed1 / typed2):
  exact-label `mem_[wFoo]`, `Memory(gb)[wFoo]`, AsmModule's `rd8(wFoo)` /
  `wr8(wFoo, v);`, and those fields' byte pairs. Left raw: offsets into
  tables and structs (wCurObj's other fields, wHDMA, wTimer's bytes, the
  camera), which want structs next.
- **ROM data by bank (step 3, started).** RomPtr (native.hpp) reads the
  loaded ROM by bank and address (`ROM(gb, Label)`, `RomPtr(gb, bank,
  addr)`), whatever is mapped; RAM addresses still go through the memory
  map. The port ships no ROM content, so ROM data stays views into the
  player's ROM, not generated arrays. `tools/romx_report.sh` (after the
  quick tier) runs every recording's trace check with --romx-report and
  lists each label still read through the bank mapped, with a routine
  reading it (the pc native code last set: approximate). That list is the
  worklist; when it's empty the bank switches in play are bookkeeping
  only, and step 4 (no bank switching) can drop them, once the trace check
  stops comparing that bookkeeping (rom_bank, wROMBank, hCallFunc's bank),
  a verification change to make together with it. HDMA's reads are the
  hardware's and aren't counted (they go with the renderer).
  RomPtr::mapped(gb, addr) stands for "the bank mapped now" in code not
  yet given its bank, and still counts. First converted: load_sprite and
  its callers that name the bank (DrawWario, the OW sprites, the action
  help's). First report (2026-11-10): 1,932 labels, nearly all data read
  through a few generic readers: sprite tables (OAM_*), animated tiles,
  framesets, the audio engine's tables, block maps.
  Converted since: the audio engine, animated tiles, menu and level object
  sprites, decompress / copy / far copies, the state table, the block map
  decompressors, golf, the OW common animations: 1,203 labels left
  (2026-11-10). Under test RomPtr reads through the memory map as the asm
  does, and marks the check unrealistic if its bank isn't the one mapped:
  fuzzed copies can run into the MBC's registers and remap mid-routine
  (a389464 failed the fuzz tier on that; fixed in the next commit).
  Then (134707c): the object and particle framesets and sprites, copy /
  decompress / palette loaders by label (ROM(gb, Label) overloads), the
  room header, block function table, palette cycle, object group data and
  OW scene params: 740 labels left; then f9c6afa: 531; then the Wario and treasure palettes, copy_bc by label and the movement tables (bank $18): 486; then 22e5ed4 (title, pause, golf, Wario animation, object creation): 322; then OW animations, credits, label+offset reads, pause / save maps, the hidden figure: 175; then table word reads, the OW palette module ($21), the clear and save screens, castle palettes: 144; then golf (GOLF(), bank $72): 126; then Memory(gb, code_bank) for the single-bank overworld modules and the cutscenes ($27): 112; then Object(gb, code_bank) for every object class (all but small.cpp single-bank), the treasure collection ($26), the OW side palettes: 97; then AsmModule(gb, code_bank): the object engine ($18), the four Wario state groups and the title screen: 71; then helpers taking the caller's code bank (apply_ow_movement, set_pals, copy_and_apply_ob_pals: mem_.code_bank()), the action help palettes, level music: 52; then Routines(gb, code_bank) (its copy / decompress / palette loads of an address alone read the module's bank), the room and level-map tables (bank $30): 36; then the cutscene loaders, the action help demos' show routines run as their C++ (the interpreter ran them by address), the interactions table, the OW tile updates, the language selection, Perfect and game over screens, the last OW tables: 0 (2026-11-10). The worklist covers what the recordings reach: code they don't can still read through the bank mapped (RomPtr::mapped, Memory without a bank), so step 4 keeps the bank switches those paths need. Pitfall: a module given its code bank must not read its own data inside an in_bank to another bank (group 3 read SwimStraightVelTable, bank $07, that way; the Yellow Belly trace caught it). Memory, Byte and Word can carry a module's code bank: its ROMX reads come from that bank, whatever is mapped (RAM and ROM0 as before). Next batch: the far loaders
  (copy_far_bytes, load_far_tiles, load_far_pals_*, load_far_bg_map) by
  their bank argument, event treasures, the treasure gfx, and the menu /
  Wario animation steppers given their frameset's bank (-1 = the bank
  mapped, for the hooks).
  **Step 4's verification (decided 2026-11-10):** when the worklist is
  empty, the trace check stops comparing the bank bookkeeping (rom_bank,
  wROMBank, hCallFunc's bank), as it already ignores the dead stack and
  wSoundEngineParam1; the bank switches in play are then deleted. The
  hooks keep switching banks under test, where difftest / fuzz compare
  them against the asm.
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
   - The Wario state helpers (`st`, `ld_and`, `clear`, `load_gfx`,
     `set_oam`, `frameset_by_dir`, `anim`...) are now members of
     `AsmModule` (src/native/asm_module.hpp), with the level engine
     (all C++).
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
  whatever the level's table points at, and could in principle pop past
  their return address. No recording's does, and collisions.cpp calls
  ProcessBlock with `gb_call` (a sentinel return address, which the call
  loop flags unrealistic when popped), so nothing above it reads the
  stack: the callers of the collision checks (the Wario states,
  `IsOnSlipperyGround`, the `Func_35xx` wrappers) call them plainly now
  (2026-11-03). `gb_call_ret` / `FARCALL_RET` stay for code that does
  read the stack (the room drawing's queue overrun, HomeJumpRet's
  homejp frames in the object helpers). For those, get the offset from
  the ROM rather than by counting bytes: find the `call` opcode
  (`cd lo hi`) inside the routine.
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
- **The original game's bugs are ported as they are**, and listed in
  docs/ASM_BUGS.md to fix at the end (behind an option, so parity can
  still be tested). Add each one found there, with a `// bug:` comment.
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
- **The collision checks** call ProcessBlock with `gb_call`: a block
  function that pops past its caller (only a fuzzed table does) is
  flagged unrealistic by the call loop, and one that comes back in
  another bank by collisions.cpp. `gb_call_ret_unwind` has no callers
  left.

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

Cleanup (branch claude/cleanup, from tag native-cpp-complete): the
register-level C++ that tools/c2cpp.py made is rewritten module by
module in this style. To find a group's entry points, list which of its
hooks are referenced from outside its asm tree (a scratch script did this
by scanning the .asm and src/native for each label). Internal routines
lose their hooks (PORT_COVERED) and become plain calls, so the fuzzer only
reaches them through the entries. To check them on their own, add
temporary ALL_REGISTERS hooks for the rewritten internals (all memory is
still compared), fuzz with many variants (`--fuzz-variants 3000`), then
remove the hooks. That found an ordering bug in the save module that the
entry's fuzzing had missed: a step marker written before a checksum
that counts it.

Home is a library, so its routines become plain functions (home/memory.hpp:
copy, fill, decompress, the far variants, the OAM and BG map clears,
ClearVRAM/ClearWRAM, DisableLCD; home/pals.hpp: the palette loads and
stores) that `Routines` calls directly. Their hooks stay as thin register
wrappers for register-level callers, leaving exactly the registers the asm
leaves. `home::bankswitch`, `home::hcall` and `home::farcall` switch banks
as home code (hCallFunc) does, so a module calling them from ROMX code
isn't flagged as switching banks under itself. `Routines::in_bank` uses
them too.

When a shared helper stops leaving registers, run the full fuzz tier: a
module entry with an exact contract may have been passing on registers a
helper left. Batch 1 found two (UpdateTempleScene, LoadHiddenFigureGfx).
Neither caller read them, so both now declare ALL_REGISTERS, with the
reason beside the hook.

Cleanup status (2026-11-01, branch claude/cleanup): plain C++ now for
home's memory, palettes, fades, sprites/animations, system/input and
overworld helpers (home/*.hpp); the modules' state table wrappers
(run_state_table); the font loaders and number printers
(engine/font_counts); Action Help's palettes; the cutscenes' loaders
(cutscenes/loaders.cpp, generated from a table) and start; the overworld
state table (no addresses left), its V-Blank setters, level coordinates
and small routines (overworld/small.cpp); the epilogue's state table.
Stays register-level by nature: the header (rst, interrupt vectors),
start.cpp (boot, main loop), the RAM-run V-Blank .Func bodies (and
ApplyPalConfig1/2 they call), the title's LY-timed raster code,
MainStateTable's dispatch, and hooks nothing calls any more (bank2b's
old prologue/epilogue helpers). Left: the level engine (src/native/level,
~17k lines, ~250 hooks reached from outside, mostly through the object
data tables and home/object_mechanics) together with home's level-tied
files (object_mechanics, wario_mechanics, load_room, load_level,
door/floor transitions, room_transition) and bank26's treasure data
(LoadLevelTreasureData, Func_9aab5), which the level and the clear
screen share.

Level engine plan (step 4; started 2026-11-01 with home's Wario
primitives, home/wario.cpp, and the golf objects' animation):
- The entries update_level calls (UpdateWarioStates, UpdateObjects,
  ProcessInteractions, through farcall_ret) are followed by code that
  reloads every register, so their contracts can be ALL_REGISTERS; check
  each asm caller first, as for the state tables.
- ProcessBlock's block functions can unwind past their caller into the
  Wario state or object that ran the collision check (they pop its return
  address); update_level passes real return addresses for that. In plain
  C++, a collision check returns whether it unwound, and its caller
  returns at once, in place of reading the stack.
- Convert a Wario state group (or an object routine set) at a time to
  plain member functions over home::move_wario_x and the other plain
  primitives; check the new internals with temporary ALL_REGISTERS hooks
  and many variants, as for the save data, then remove the hooks; run the
  full fuzz tier for each batch.
- Done (2026-11-01): the four Wario state groups dispatch natively
  (wario_states_1-4.cpp): calls between a group's labels are member
  calls, only labels called from elsewhere keep hooks (69 of 516). The
  conversion is scripted (direct_calls.py in the session scratchpad:
  LCALL/CALL/JUMP to own labels -> member calls, tail jumps out -> calls,
  RET() dropped, the dispatcher a switch); watch for "else RET();" before
  a shared return and for computed jumps (JUMP_TO(target)), which become
  member function pointers. Group 1's dispatcher switches on a (the asm
  doesn't reload wWarioState). object_interactions too:
  ProcessInteractions loops natively, an interaction that jumps to
  .next_obj sets next_obj_ (121 internal hooks gone). Run alone (temporary
  hooks), the handlers that go on to the next object differ from the asm
  in wObjPtr only; ProcessInteractions checks them.
- update_objects is left as it is for now: 60 of its 82 labels are the
  object helpers the objects call by address, it calls the collision
  routines with return addresses that matter, and DoObjectAction jumps
  through the object action tables (targets in other files), so each
  jump needs deciding whether its target keeps a hook. Take it with the
  objects' own cleanup.
- Registers -> plain memory, first pass done (7ad245e, 544 sites): a
  script (locals.py in the session scratchpad) rewrites st / xor_a /
  ld_and / ld_bit / ld_mask / ld_cp + flag test where the a and flags
  they set are dead on every path: both sides of a condition, else
  branches, block ends, labels and returns (a function's end counts as a
  read unless the dispatcher's caller reloads everything), and calls that
  pass a (CALL, LCALL, member calls; FARCALL sets a itself). An
  assignment whose right side reads a is a read. What's left of the
  register helpers is where a value really flows (cpu_.b into
  MoveWario*, b_and() after collision checks...).
- Second pass (b151b7e, 599 sites, redone from the code before the first
  pass): FARCALL keeps the flags (hCallFunc pops af); a table of the
  common callees' register use from their asm; an internal member's exit
  registers are dead when all its calls leave them dead (fixed point);
  "b = n / call MoveWario*" -> home::move_wario_x / _y (20 sites).
  What's still register-level is mostly pinned by the 69 Wario state
  routines and 6 interactions called from other files with exact
  contracts. A cross-file check of their callers (c1b24ab, relax.py)
  relaxed 13 of them; the other 62 carry real data (a collision check's
  result in b, values passed on in a) or have callers outside these
  files. The scripted passes have reached their limit here: what's left
  is rewriting routines by hand to take arguments and return values
  (the collision checks returning their result first, since most of the
  remaining b and flag traffic comes from them). Done at the call sites
  (06c5289: centre_collision() etc., the farcall plus ld a, b / and a).
- collisions.cpp itself, done (2026-11-02): no recording ever unwinds
  (with a print in call_ret's may_unwind path, all 14 recordings ran
  with none: the block functions' push/pop pairs balance), so the checks
  are plain C++ (collisions.hpp: check_centre_collision() etc. return the
  result the asm leaves in b). Each computes its points from Wario's
  position and box into hPos, calls GetBlockPtr and ProcessBlock through
  the emulator (gb_call, no return addresses; the call loop flags a
  fuzzed table that pops past the call, process_block one that comes
  back in another bank) and returns ProcessBlock's a. The block
  functions are still register-level and see b and c, so those are set
  where the asm sets them (c the last offset, b = a after each
  ProcessBlock, b = 2 for MoveWario*), and the CPU ends with the last
  ProcessBlock's registers, as in the asm. Every caller of the 25 hooks
  farcalls or homecalls them, and hCallFunc / homecall pop af, so the
  hooks clobber a and f and fix up only b (plus get_pos_y's hl and de
  in CheckCentreCollision); Func_19720 jumps on to the floor transitions
  and stays exact. AsmModule's centre_collision() etc. call
  level::asm_*_collision() through Routines::in_bank (the farcall's
  memory effects, no hCallFunc interpreted). The 14 routines only this
  file calls lost their hooks (PORT_COVERED), after temporary
  ALL_REGISTERS hooks passed on every recording with full asm cover.
- Collision farcalls (2026-11-03): where the code after a
  FARCALL(Check*Collision) sets a and f before reading them, it calls
  AsmModule's run_*_collision() (the native check, result in b) instead
  of going through hCallFunc (55 sites, 7880f22). The Blind states,
  IsOnSlipperyGround and home's Func_35xx wrappers call the collision
  checks without the asm's return addresses (6a9b1db).
- Block functions as plain C++: tried and backed out (2026-11-03). A
  version with native dispatch (a table of the 138 functions the levels'
  tables point at) and functions returning their result built and
  matched memory once BreakBlock got its block pointer in hl (CreateParticleInBlock's
  Func_ce3e places the debris from h and l). But the objects read
  registers the block functions leave: after an object's collision
  (the home wrappers Func_3513, Func_352b, Func_3543, Func_358b,
  Func_35a3, ProcessBlockOnLeft/Right) MuddeeFunc reads h
  (`set 6, [hl]`, muddee.asm:653), PesceFunc d and e, OctohonFunc hl,
  ZipLineFunc c, and SetObjectFramesetPtr d and e, on paths with no write
  first. Those values come from the block function's exit (hl and de
  from ProcessBlock's jp hl and whatever the function loads), so the
  block functions stay register-level until the objects are plain. Most
  Wario state sites read none of c, d, e, h, l after a collision check
  (checked over the asm, below), but UpdateState_FlatFalling reads de
  (`add hl, de`, wario_states_3.asm:942, after CheckAirborneCollision at
  :863). The attempt is in the session
  scratchpad (plain_attempt/) if it's taken up again with the objects.
- Register liveness over the asm (live.py, session scratchpad): for each
  call site of a routine, which registers the code after it may read
  before writing them. It follows jumps, jumptables (to the dispatcher's
  exit), calls (inlined, farcall restoring a and f), a ret back to every
  caller, including calls of the routine's local labels, and treats an
  object's update function (object_data / object_params references) as
  ending dead, since UpdateObject reloads every register; push / pop
  are transparent. Usage: `python live.py cdehl Routine,Routine...` from
  the repo root. Handy for any contract relaxing.
- Wario's states as plain C++ (2026-11-07): wario_states_1-4.cpp derive
  from `WarioStateBase` (level/wario_state_base.hpp), whose helpers read
  and write memory and return values (the collision checks return the
  result the asm leaves in b, through `in_bank`; the home primitives are
  called directly). The AsmModule helpers that set a and f for a flag
  test are `= delete`d there. Register traffic is left only where a
  register-level callee takes or returns one (CreateParticle's b, the
  floor transitions' c, the b ApplyWalkVelocity_* leaves for MoveWario*,
  Func_1f863's de). Every kept hook clobbers all registers, except five
  reached from inside a collision check, whose register-level callers
  see what they leave: CrouchOrSlide, SetState_Idling and StartFall
  (register-level copies, WarioStates1Exact), SetState_FanStart
  (FanStartExact), SetState_ZombieRecovering (a and f as its xor a). A
  hook others tail into keeps its contract (ResetLevelMusicWarioPals-
  AndState, the block functions), so the hooks it reaches must too: the
  difftests caught the last two. History: first landed as 8340be5, backed
  out in d04f88f when the full fuzz tier failed UpdateLevel, landed again
  once the register map below showed those failures were fuzzed update
  pointers (the fuzzer now flags them).
- The register map (2026-11-07): which object code reads registers it
  didn't set, and whether real play reaches it.
  - At an update function's entry: nothing in real play. The 166 update
    functions the 14 recordings enter (logged at UpdateObject's call)
    read none of b to l before writing them, by the asm liveness from
    each entry (Coin's macro-made moves aside, unresolved). Poisoning
    b to l at every update call changed no frame of any recording. Under
    the fuzzer the same poison failed UpdateLevel, all through fuzzed
    update pointers into the middle of routines: hCallFunc is a direct
    call, so the interpreter's jp hl rule never saw them. The hCallFunc
    hook (ram_code.c) now flags a call to an address with no label as
    unrealistic, and with it the poison fails nothing.
  - Object routines that take a register from their own caller (found
    by the liveness from every object hook's entry, never stored as an
    update function): SilkPlatformsFunc (bcde), AppleFunc (b), BeamFunc,
    SunFlameFunc, MoonStarFunc, ZombieHeadFunc (bc, a movement table),
    TreasureFunc, WallCrackFunc, ClearGateFunc (de, a frameset),
    HandFunc.ApplyOffsetToWario (b, h), Hidden Figure's Func_4cf70 /
    87 / 9e / bf (bc), Jamano's Func_554bc (e, hl), Muddee's .Func_55c11
    (bcde), PillarPlatformFunc and .Func_49d43, Shoot's
    DoPreKickingJumpYMovement, ApplyYOffsetOnKickedWarioBall,
    Func_5725b, the Scowler palette writers (hl). Real update functions
    that read one: ShootFunc .ScoreGoal (a stale b, in ASM_BUGS.md, not
    reachable in play) and ZipLineFunc .Func_49369 (the c its own
    .Func_49418 leaves). Many states read h, which UpdateObject sets to
    the object's page.
  - After a block check: Muddee (h), Pesce (d, e), Octohon (hl), ZipLine
    (c) and SetObjectFramesetPtr callers read what ProcessBlock and the
    block function leave: the block pointer and the block function's
    values, exact as long as the block functions stay register-level.
    Poisoning b to l after every block check the objects make changed
    no frame either, but the recordings reach none of those four
    objects (they reach Anonster, Coin, the bots, Bubble, Para-Goom,
    Silky, Spearhead, Torch embers, Webber, Yellow Belly, Zombie).
  So: the frame before an object need not keep b to l for the update's
  sake. The block functions must stay register-level until Muddee,
  Pesce, Octohon and ZipLine get their values passed, and a recording
  with them would check it.
- Objects, first step (2026-11-07): the objects' block checks through
  home's homecall wrappers (Func_3513, Func_352b, Func_3543, Func_358b,
  Func_35a3, ProcessBlockOnLeft/Right) are `Object` methods returning
  their result (hpos_block_unk4() etc., named after the collision points;
  block_beside(right)), in place of reading gb_->cpu.a after the call:
  146 sites, by a script (only where the next line read a). The routines
  still run register-level, so the registers the block functions leave
  (Muddee's h, Pesce's d/e, Octohon's hl, ZipLine's c) are as before.
  What's left in the objects: arguments passed in registers to home's
  object routines (SET_BC before ApplyObjYMovement, CreateObject*; b for
  DoGroundShake, MoveWario*...), the a/f results of the others
  (IsTreasureCollected's z...), and those block-function reads.
- Objects, second step (2026-11-07): the movement tables, object
  creation and the ground shake are Object methods taking their argument
  (apply_y_movement(table), apply_x_movement(_loop), create_object_at_offset
  / create_object_here(params), ground_shake(frames)): 148 sites, where
  SET_BC (or b) and the call were adjacent, plus three by hand. They set
  the register and call, so nothing changes for code that reads it on.
  About 156 register sites are left in the objects: b for MoveWario*,
  hl / de for DespawnObject and the palette loads, reads of h's page,
  and the registers left by block functions and earlier objects.
- UpdateObjects, plain (2026-11-07): the update loop (UpdateObjects,
  UpdateObject and the temple / floor transition variants), DrawObjects,
  CheckObjectOnScreen, HandleGrabbedObjectCollisions, the object clears
  and InitObjectVars are plain C++; home's UpdateObjSprite is
  home::update_obj_sprite (home/object_mechanics.hpp). Their callers
  read no register after them (live.py), so the entries' hooks clobber
  all, and the ones only the loop calls lost theirs (PORT_COVERED, after
  fuzzing them as temporary ALL_REGISTERS hooks). UpdateObject still
  pushes the object's slot and its caller's return address, calls
  DoObjectAction, the animation and hCallFunc through the emulator with
  the asm's return addresses, and leaves hl (the end of the copy) and a
  and f (the pointer's high byte, the `or` of the bank) for the update
  function: the fuzzer found update functions that read a and f on entry
  (the register map above only poisoned b to l). StartRoom_FromLevelStart
  / _FromTransition, ProcessMultiBlock, DrawLevelObjectsAfterLevelReturn
  and ReturnToLevelFromGolf end on these, and their callers read none of
  b to l either, so they now clobber b to l (the difftests and the fuzz
  tier caught them).
- The shared object states, plain (2026-11-07): falling, thrown, arcs,
  bumps, knock-back, vanishing, the movement tables (_ApplyObj*Movement,
  the set movement, Func_63209's platform) and Func_632ac's count
  (update_objects.cpp, from set_state_by_y_vel to the grabbed-object
  collisions) are plain member functions over table_step() /
  movement_step() (a table's next entry) and block() (a block check
  through home's wrapper, with the asm's return address). Inputs in
  registers stay inputs: bc for Func_62ec3 / Func_62faa (the throw's
  table; the thrown states' entries still jump on into them, as the
  asm's jr does, so each hook checks its own part), b for the arcs, bc
  for the movement tables, de for the set movement, b plus h's page for
  Func_627f0 / Func_62690. The objects reach them through home's homejp
  and homecall wrappers and read nothing after them but h (the
  movement tables leave hl at the position's high byte, the _Loop ones
  e too) and Func_632ac's e, so the hooks clobber the rest; the home
  movement wrappers clobber b and c, VanishObject, ApplyObjSetMovement
  and Func_34fc all.
- Two fuzzer realism rules came out of this (both only fuzzed states
  reach): ProcessBlock flags a block function table entry that isn't one
  of block_functions.asm's labels (a fuzzed table sent a floor check to
  UpdateObjSprite, whose plain version leaves another a), and
  UpdateObject flags an update pointer below $4000 (one pointed at
  MoveWarioUp, which reads the b the asm loop leaves).
- Throwing and held objects, plain (2026-11-07): ThrowObject_Left /
  _Right (now internal: throw_object(right, grab)), the thrown objects
  moving sideways (Func_626da and the like, going on along Func_627f0's
  or Func_62690's table natively), and the picked-up, held and
  charging-throw states (Func_61f54 / 620a6, 621fb / 622bd, 6247b /
  62382). Func_627f0, Func_62690, Func_62d34 and Func_62e0f lost their
  hooks (nothing jumps to them by address now). Deep fuzzing
  (2000 variants) shows one UpdateObjects case on boss.input where the
  native run takes a V-Blank partway through the Hidden Figure objects
  and the asm doesn't: native code runs in less emulated time, so it's
  the timing class above, not a difference in the code; the tier's 200
  variants pass.
- DoObjectAction, plain (2026-11-08): a switch the loop calls directly
  (PORT_COVERED). Its exit f reached the temple's update functions (no
  flag op between in UpdateObject_Temple), but none of the 166 update
  functions the recordings enter reads a or f on entry except through
  the normal loop's `or` (entry liveness with a and f, a variant of the
  scratchpad's entry_reads2.py). update_objects.cpp is plain throughout
  now; registers are left only as hook inputs and the outputs the
  objects read (hl from the movement tables, e from the counts).
- The objects' moves and framesets, plain (2026-11-08): Object's
  move_left() and the rest, the ByVar2 moves, move_by(routine) (home's
  MoveObject* by address: 1 pixel, _Fast 2, _Slow every other frame) at
  the ~35 files' direct calls, and set_frameset() call home's plain
  move_object_x / _y and set_object_frameset_ptr (home/
  object_mechanics.hpp). The asm leaves hl in wCurObj after every move
  that touches it, and the objects that read h on (Pesce, Muddee,
  Wolfenboss, Hidden Figure, the doughnut, potion and rolling rock) now
  use wCurObj's addresses; Zip Line sets b = var2 for MoveWario* itself.
  The home hooks stay register-exact for the register-level callers
  left. (Wolfenboss's MoveObjectUp_Slow site had hl set beforehand for
  the frames the move skips; wCurObj again.)
- The objects' state wrappers, boss hits, MoveWario and particles, plain
  (2026-11-08): the objects call home through Object::call_home(routine)
  (move_by renamed; every routines_.call in objects/), which runs the
  moves, DecrementRemainingBossHits (decrement_boss_hits()) and the 45
  homejp state wrappers plain: object_state() (update_objects.cpp) runs
  the state's C++ with its bank switched around it as homejp and
  HomeJumpRet do, home_jump_ret() and thrown()'s jump skipped
  (ObjectEngine::plain_). Anything else falls through to the call.
  live.py over every wrapper's and MoveWario*'s call sites: no object
  reads a register they leave (only the object tables' false positives,
  HomeJumpRet's a and the uncalled Func_5725b). Object::move_wario(
  routine, n) passes MoveWario*'s b as a parameter (home::move_wario_x /
  _y), and Object::particle(id) runs a plain create_particle() (both
  object sites return straight after). The wrappers' hooks stay for the
  register-level callers.
- The objects' palettes, movement tables and counts, plain (2026-11-08):
  home::set_pals / black_out_pals (home/pals.cpp; the four hooks wrap
  them, a the last byte read, so a source in I/O space reads as the asm
  does) and copy_and_apply_ob_pals back Object::set_bg_pals and the rest.
  The movement tables (apply_x/y_movement(_loop)), ApplyObjSetMovement,
  Func_34fc (platform_y_movement) and the transient counts
  (active_transient_objects, same_transient_objects) call the object
  engine's plain functions inside Object::homecall (the bank switched
  around them as homecall does). Their h is always wCurObj's page, so
  Anonster and Omodonmeka read x() / y(). live.py: the palette routines'
  only object hits are the object tables' paths and rra's carry into a
  dead a.
  The floor transitions are plain too (home/floor_transition.hpp:
  floor_for_y_pos, handle_up/downwards_floor_transition, run by
  call_home; checked under temporary ALL_REGISTERS hooks, 603/603 each).
  They never touch hl, so Omodonmeka's RISE reads wCurObjStateDuration.
- The block functions, plain (2026-11-08), in two commits:
  - 8a8659b: native dispatch. A block function leaves the one it jumps on
    to in next_ (GOTO), dispatch(func) maps an address to its function
    (a switch built from the hook table), and ProcessBlock runs the chain
    itself when bank 6 is mapped (else it sets pc, as the asm's jp hl
    runs whatever's mapped). The hooks stay as entries: dispatch, then
    jump_on() (pc = next_, or the ret).
  - The functions as plain C++: conditions read memory directly
    (mem_[x], word_be for hPos / wWarioXPos / wBlockPtr), without the a
    and f the asm computes on the way. a and f are dead at every jump: no
    block function reads them on entry (entry_reads_af.py over all 269),
    and every chain ends in Free, Solid, Func_1808c, Func_18064 or
    Func_18078 (which set them) or Fire's ret. So the 261 hooks that only
    jump on clobber R_A | R_F, and ProcessBlock, Fire (its ret leaves the
    flags of its last check) and the terminal ones stay exact. b, c, d,
    e, h and l are still set where the asm sets them: the collision
    checks hand the last block function's to their callers (Octohon's
    .asm_4a6eb writes 0 through hl), and they're the register-level
    callees' arguments (Func_e31, CreateParticleInBlock, CopyHLToDE,
    DoGroundShake).
- Object interactions, plain (2026-11-08, d2a307e to the last
  object-interactions commit): ProcessInteractions clobbers every register
  and none of its 84 handlers reads one on entry (entry_reads2.py and
  entry_reads_af.py over the jumptable's targets), so pi_run returns the
  handler (the overlaps and sides worked out in plain C++),
  set_obj_action(action) replaces SetObjAction's b, and the handlers read
  memory directly (switches over the touch states, plain compares for the
  sides and positions, home::move_wario_x / _y for the pushes). Left
  exact on purpose: the five hooks the block functions farcall
  (SetState_OnFire, _ResetStateCounter, SetState_WaterStung,
  SetState_Stung, Func_206eb) and the helpers they share (restore_pos_
  backup, clear, collision_box, load_gfx...), since their register exits
  reach ProcessBlock's callers. What's left: b for CreateParticle and the
  collision checks' result in b.
- The room start, plain (2026-11-08, 86d907b to the queue commit):
  home/object_pos.hpp has the object map positions (Func_cc0, cf8, d3e,
  d81, d8c) as plain functions, their hooks wrapping them with the asm's
  b to l (no caller reads a or f). start_room.cpp's spawning, Wario's
  spawn point, the initial camera scroll, the room drawing,
  ProcessMultiBlock's block walk and both StartRoom_* bodies read and
  write memory directly. Their hooks clobber all registers:
  LevelStateTable sets a and f itself after the state, and nothing reads
  b to l they leave (live.py over every call site; the `pop af` hits
  are the tool's: it treats push / pop as transparent). Kept exact on
  purpose: the calls' return addresses (call_ret) and, around the
  QueueBlock*AlongRow_* farcalls, the push hl and the af hCallFunc
  pushes, since the queue overrun reads the stack. Registers are left
  only as callee arguments (GetNextLevelRoomID's hl, CopyHLToDE*'s hl /
  de / b, SpawnObject's c / de, the queue routines' hl).
- block_queue.cpp, plain (2026-11-08): DespawnObject, Func_ba42, Func_bb2d
  and Func_bb85 over memory (map_loc, home::obj_map_col /
  level_block_to_hpos). DespawnObject's hook leaves the asm's hl / de / c
  and a / f, as DespawnAllObjects walks on with h. Func_bb85 keeps b
  exact: in ProcessMultiBlock's loop it goes on through ProcessBlock to
  the multi-block functions that jump straight to the replace
  (Func_18f32 and the like), whose replace_block writes b as the new
  block (SRAM1 when nothing was queued, 0 after a block, the caller's
  with a DMA pending). The fuzzer found it (ProcessMultiBlock,
  yellowbelly, variant 124) when the hook clobbered b: a register read
  that crosses three routines, which live.py missed.
- camera.cpp, plain (2026-11-08): the scrolls take their distance as a
  parameter (the hooks pass b), every hook clobbers all registers. Its
  callers (update_level.cpp, start_room.cpp) go on to ProcessInteractions,
  Func_8ed9 and Func_bb85, none of which reads a register on entry, and
  live.py finds no read after any call site.
- particles.cpp's update, plain (2026-11-08): UpdateParticles dispatches
  natively (particle_func(id), animate(anim, at)); run(), the animation
  and LoadParticleSprite work on the slot's address, LoadSprite still
  taking hl. All their hooks clobber every register (only each other and
  plain callers reach them; UpdateParticles' loop reloads hl). The fields
  step with the asm's inc c, in the particle's page: the fuzzer enters
  with bc anywhere (ccfe), and 16-bit offsets failed every shard.
  CreateParticle, Func_ce3e and CreateParticleInBlock stay exact (BreakBlock
  calls the last inside the block function chain).
- update_level.cpp, plain (2026-11-08): the level's frame reads and
  writes memory between its calls; UpdateLevel clobbers all registers
  (LevelStateTable sets a and f after). Its calls keep the asm's return
  addresses and its push_wram / SRAM push af stay as stack pushes (the
  bank in a), so every return address sits where the asm's does.
- scroll.cpp, plain (2026-11-08): the edge rows and columns and the
  room's first-drawing queues over memory; the hooks clobber all
  registers (the callers are C), GetNextLevelRoomID stays exact. The
  overrun's read lands on the stack only in
  QueueBlockAttributesAlongRow_OddColumn's half first block (ce35 +
  block * 4 + 3 reaches the stack, the tile tables don't), so only that
  routine still pushes its af (the entry f) and the block map pointer.
  The other pushes left only dead stack, which the difftest and the
  traces ignore.
- load_room.cpp, room_gfx.cpp's room load and start.cpp, plain
  (2026-11-08): LoadRoom and what it calls (LoadRoomGfx, the tiles, tile
  map, palettes, block function table, animated tiles and palettes) and
  UpdateRoomAnimatedPals clobber all registers (their callers are C and
  read none after). The debug asserts still jump to Init with the asm's
  push af on the stack (SP is compared). The game loop and boot code read
  memory only; their calls keep the asm's return addresses (the
  MainStateTable frame is on the stack during the room drawing). The
  fuzzer can't settle GameLoop or InitWithoutDemoPowerUpReset (they run
  whole frames), so the traces cover them. HandleSound stays
  register-level: the audio calls take bc, and UpdateAudio reads c.
- load_level.cpp and decompress_level_data.cpp, plain (2026-11-08): the
  block and object maps' decompression and the objects' reload clobber
  all registers (StartRoom calls them). home/level_map.hpp has
  block_ptr (GetBlockPtr) and block_bg_map_address
  (GetBlockPtrBGMapAddress), whose hooks stay exact for the collisions
  and spawn_edge (their registers reach ProcessBlock); scroll,
  start_room and block_queue call the plain ones. block_ptr reads x
  after writing wBlockPtr's high byte and bank, as the asm does.
- bank26.cpp and home/overworld.cpp, plain (2026-11-08): the level
  treasures clobber all registers (C callers), LoadLevelTreasureData
  leaves pop_wram's a and f. home/ow.hpp has ow_coord_in_tilemap,
  load_level_name, start_hdma, load_far_bg_map, apply_pal_config and
  apply_ow_movement; their hooks stay exact for the asm, and the C++
  callers (scenes, temple, prologue, epilogue, map walk, events,
  cutscenes) call the plain ones (their hooks are all ALL_REGISTERS).
  bank21's ApplyPalConfig* wrap apply_pal_config. door_transition.cpp
  plain (LevelStateTable sets a and f after it).
- The V-Blank functions copied to wVBlankFunc (overworld, OW scene,
  cutscene, prologue/epilogue, golf building) are plain; they end with
  a = HIGH(wVirtualOAM) and a jump to hTransferVirtualOAM, which reads
  only a, so their hooks clobber everything else (ALL_REGISTERS & ~R_A).
- load_objects.cpp's LoadObjectGroupData and LoadHiddenFigureData plain
  (LoadRoom calls the first): the pointer loop returns where it stopped,
  for LoadObjPals (home::load_obj_pals) to read the palettes after it.
  SpawnObject, CreateObject*, DespawnAllObjects keep their register
  interfaces (objects and the block queue pass them registers).
- The other V-Blank functions plain (2026-11-08): golf holes
  (hole_ram.cpp), clear and musical coin screens, language selection,
  save screens, the level's. The V-Blank handler (header.asm) pushes and
  pops af, bc, de and hl around `call wVBlankFunc`, so a function that
  calls hTransferVirtualOAM and returns clobbers everything; one that
  jumps to it keeps a. LevelVBlankFunc stays exact: its jump to the RAM
  queue code at wVBlankFunc + $10 takes hl and bc, and hTransferVirtualOAM's
  dec loop keeps the carry of the shake's add (the fuzzer caught it).
- Pitfall: scratchpad live.py can run for hours and grow to 6-8 GB; four
  at once left 20 MB free and the quick tier failed with SegFault and
  "Not Run". Check free memory before debugging such a failure.
- spawn_edge.cpp plain (2026-11-08): the edge drawing farcalls it and
  reads nothing after; it uses home::block_ptr, obj_map_col and
  level_block_to_hpos, and only SpawnObject still gets c and de. Full
  fuzz tier green through 5d57c81 (56/56).
- Wario mechanics plain (2026-11-08): home/wario.hpp has
  apply_jump_velocity, apply_walk_velocity, trigger_floor_transition,
  recover_from_transformation, is_on_slippery_ground, release_owl and
  trigger_room_transition; floor_transition.hpp update_floor,
  start_floor_transition, handle_floor_transition (Func_1180). The Wario
  states and object interactions call them (no more CALLs of those), so
  the velocity, slippery ground, owl and room transition hooks clobber
  everything (the walk ones keep b). Every asm caller is a hooked C++
  routine (scratchpad callers.py; it must match farcall/homecall too).
  Stay exact: the floor transitions (Func_19720 tail-jumps to them) and
  the recoveries (block functions; SetState_Idling/StartFall read regs).
- Pitfall: a hook may JUMP_TO only where the asm itself tail-jumps; a
  jump to an equivalent routine (TriggerDownwardsFloorTransition to
  HandleDownwardsFloorTransition) fails the fuzzer ("exits somewhere
  other than the asm"): call it instead.
- Also plain: return_level (state tables keep a and f: hooks set them),
  the title raster effects (a and f for TitleStateTable), the musical
  coin flag, _InitPrologue/InitPrologue, and the credits, title, Action
  Help and OW palette transition V-Blank functions. object_mechanics'
  movers, animation, frameset, boss hits and one-way platform hooks
  clobber everything: the objects call them plain (Object::call_home)
  and every asm caller is hooked. Left register-level on purpose:
  header.cpp (interrupts, rst JumpTable), clear_transformation's two
  unreferenced number routines, object_mechanics' homejp/homecall
  wrappers, bank2b_1/2b_2 (register arguments from the prologue).
- Next: block_functions.cpp, load_golf_sprite.cpp, start_room.cpp's
  helpers, bank2b_2.cpp, sprites/system hook glue (by register sites).
- The fuzzer used to abort when native code made a `CALL`/`JUMP` into
  ROMX with another bank mapped (a fuzzed state can map any bank); under
  test that's now an unrealistic check and the call is skipped
  (ported.c). The `boss` input hit it with temporary hooks on
  Func_1b2c0 and Func_1b302 at 1000 variants, with the old code too.
- Home's level-tied files (load_room, load_level, decompress_level_data,
  door/floor/room transitions, wario_mechanics1, object_mechanics,
  object_pos, IsOnSlipperyGround) go with the level code that calls
  them.

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
  `StartOverworldStateAndNewGameParam` and `Func_4628`, called from
  elsewhere, keep exact registers; `LoadHiddenFigureGfx` clobbers them all
  (InitLevel reads none after it). The
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
  object), all clobbering every register. `SaveData` (save_data.cpp) is
  the checksums and the boot-time save validation, plain C++ called
  directly by `SaveScreen`; its one entry is `ValidateSaveData`
  (ALL_REGISTERS: Start reads none after it). The save screen's loaders
  (bank7d.asm) and V-Blank setters are `SaveScreen` methods, and
  `PauseMenuStateTable` (a and f as pop_wram leaves them) calls
  `PauseMenu` directly. Still register-level: the V-Blank functions'
  bodies, run from wVBlankFunc (save/vblank.cpp), and
  DrawLevelObjectsAfterLevelReturn (home code calls it; it farcalls
  level-engine hooks).
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
  VRAM wouldn't return (Object::dec_at); after a SetObjectFramesetPtr, hl
  is wCurObj's again). The bosses' `asm_` hooks that enter with hl set
  (Pesce's, Yellow Belly's) take h's page as a parameter; their internal
  callers pass `wCurObjState & 0xff00`, never the address itself, and a
  `ld [hld]` pair is a 16-bit step (Jamano's Func_554bc with e = 0 writes
  into the page before). Reading h after a home routine is only safe when
  hl was set as the asm sets it before the call: routines that leave hl
  alone (MoveWarioUp, HandleUpwardsFloorTransition without a transition,
  MoveObject*_Slow returning early) pass whatever the hook got through
  (Omodonmeka's .Func_426ee). Every object is C++ now (Anonster,
  Omodonmeka, Shoot and Hidden Figure last): the old
  src/ported/engine/level/objects is gone. Quirks kept: Nobiiru's sting
  step stores its new interaction type as the collision box's left (the
  asm reuses a). The jumptable objects (the rock, Nobiiru, the hand) use
  tables of handlers; the rock's is checked against the asm's dw list.
  Locals the C left to the interpreter (the sparks', the tadpole spawner's)
  are hooked now. Never wrap PORT_EX / PORT_LOCAL_EX in a macro:
  tools/gen.c finds hooks by their text, and a macro's hooks compile but
  are never registered (fuzz_batch reports them NOT FOUND).
- **level engine** (src/native/level/*.cpp): the state table and
  InitLevel (level.cpp), particles, object loading, room graphics, the
  camera, the edge drawing and room-start row queues (scroll.cpp), the
  block queue and edge spawning, room starts and the level's frame.
  Unlike the objects, their callers are still register-level C (Wario's
  states, object interactions, home), so every entry point leaves the
  original's registers and flags; `src/native/flags.hpp` computes the
  flags an instruction leaves. They keep the asm's pushes, and the room
  starts and the frame keep its return addresses (`Routines::call_ret`,
  `farcall_ret`), since the room drawing's queue overrun reads the stack.
  Pitfalls seen: `sla e / sla e / rl d` is block * 4 with bit 7 lost; a
  `ld [hli]` steps across a page where the `inc e` after it doesn't
  (CreateObject_GotPos); `NONE` and `OWL` are macros (constants.h), so
  enum values can't use them. Collisions and the block functions are C++
  too (collisions.cpp, block_functions.cpp): the block functions keep
  every jump between them; the collision checks are plain (see the level
  engine plan).
  The object engine (update_objects.cpp, object_interactions.cpp) was
  converted with tools/c2cpp.py: it turns a register-level C file into a
  class over `AsmModule` (src/native/asm_module.hpp, the C helpers
  as members on the real CPU state, `Alu` in src/native/alu.hpp for the
  flags), with the same logic, then gets reviewed and fuzzed like any
  port. Never wrap PORT_EX in a macro; the tool writes each hook out.
  Every other register-level C file (home/, the overworld leftovers,
  pause/save, golf, action help, title, the small screens) went through
  the same tool into src/native/<subsystem>/ (class named after the
  path, e.g. `HomeFade`). The only C left in src/ported is
  home/ram_code.c, the emulator's RAM-code hooks, which stays C. A
  routine taking a C function pointer becomes a member-function pointer
  (`fade_to_pals1(&HomeFade::darken)`).

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

4. **`UpdateState_Crouching` on `action_help`** (2026-11-07), found with a
   temporary hook on it at `--fuzz-variants 1000`: 1 of 4004 checks fails
   (seed from call 64, frame 6459, variant 836). The asm run writes
   object, scene and audio RAM in WRAM bank 3 (wObj1Var1, wObjDataPtr07,
   wChannel6...) that the native run doesn't, as if it ran off into
   fuzzed code. The code before the plain rewrite (6bcc579) fails the
   same check with the same differences; nothing else does. Never seen at
   the default 200 variants.

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

Toward no registers (2026-11-08): the goal is a game build with no CPU
struct. Order: (1) finish the plain modules, (2) untangle block functions
and objects, (3) direct calls instead of addresses (Routines::call,
farcall, JUMP_TO), (4) the header and main loop as plain code, (5) compile
the hook glue out of the game build, keeping it in `WL3_INTERPRETER`
builds so the fuzzer and difftests still compare against the asm.
Stage 1 is done apart from what depends on later stages:
- Stage 2: load_objects (register arguments and results for object code),
  object_interactions, wario_states_1, particles' CreateParticle,
  collisions, update_objects, block_functions.
- start_room and level_vblank: start_room's pushes are read by the block
  queue overrun; LevelVBlankFunc hands hl and bc to the RAM code.
- Stage 4: header, start, main_state_table.
Stage 2 finding (2026-11-09): poisoning c, d, e, h and l after every block
check passed the full fuzz tier except for BigOctohonFunc, which writes 0
through the hl the last block function left (a wall hit in .asm_4a78b).
Older notes above about Muddee, Pesce, ZipLine and FlatFalling are out of
date: those reads are gone. So block checks now keep only the result in b,
and nothing else. Octohon works out that hl itself (the function of the
block at wBlockPtr: Octohon::last_block_function). Block functions keep the
replacement block and the block to break in members (b_, hl_), handed over
by hooks in leave(). Object spawning and creation (load_objects) take plain
arguments and leave no registers, as do DespawnObject, Func_ba42, Func_bb2d
and the particle creators. Left for stage 3: a as the block result
(ProcessBlock is still called by address). The hooks that were exact only
for the block functions (SetState_OnFire and friends, the recoveries, the
floor transitions) now clobber every register and their bodies are plain.
Stage 3 (2026-11-09): a call by address is fine if nothing passes through
registers, so the work is the calls passing arguments in registers: 173
down to about 20, nearly all boot and audio (stage 4). Still needed: the
Dropped quirk (2026-11-09, the user's decision): start_room, update_level
and scroll.cpp no longer keep the return addresses and pushes that only
QueueBlockAttributesAlongRow_OddColumn's half first block could read
(block about $5e to $71). None of the 14 recordings reach that read, so
the tiers can't show the difference; see ASM_BUGS.md. Older notes in this
file saying those frames are kept describe the earlier state.
Audio (2026-11-09): src/native/audio/audio.hpp has plain entry points,
used by HandleSound and Routines::play_new_music. They pass 0 for the
caller's leftover c, which the original stores in wSoundEngineParam1 (also
wCurSoundID, wTempTrackPriority); every reader writes it first, but the
traces compared it. Since the leftover c varies ($00, $10 in Action
Help), the difftests and traces no longer compare wSoundEngineParam1 (the
user's decision, like the dead stack), and Action Help's volume mod is plain.
Collisions::block still calls GetBlockPtr: ProcessBlock reads its hl and b
(a plain block_ptr call broke the full tier but passed the quick one).
Stage 4 (2026-11-09): the game loop and boot code call routines plainly
(no emulated return addresses; HandleSound, the reset's audio and
UpdateJoypad directly), and engine::run_main_state replaces
MainStateTable's rst JumpTable (the index is wState * 2 in 8 bits). The
frame boundary stays as it was: the GameLoop hook runs one frame and halts
the CPU, so savestates and the traces still sync at .GameLoop.
Stage 5 (no address dispatch in the game build): tools/hook_report.sh
(after the quick tier) replays each recording's trace check with
--hook-report and lists the hooks the game still reaches, with how: by a
native call by address, a jump (JUMP_TO, manual ret, fallthrough) or an
interrupt. First run (2026-11-10, 13 recordings): 415 hooks, 383 by native
call, 35 by jump (the frame loop's parts, the screens' V-Blank functions),
1 interrupt (VBlankVector). A hook nothing reaches in play is fuzz-only:
its register glue can move under WL3_INTERPRETER.
gb_call_native (cpu.c, 2026-11-10): gb_call and hCallFunc run a ROM
routine whose hook returns normally straight from the hook table, with no
CPU dispatch, return address or stack (the report's "direct" column).
Manual-ret hooks, gb_call_ret's explicit return addresses and RAM code still
go through the CPU. After it: 57 hooks CPU-dispatched in play (26 by native
call, 35 by jump, 1 interrupt), 358 direct. A direct call never reaches the
routine's address, so code that keys on PC (the trace's LY site) needs
trace_entry (native.hpp) in the plain function.
V-Blank (2026-11-10): transfer_virtual_oam (native.hpp) runs the OAM DMA
RAM hook directly, and VBlank runs the copied wVBlankFunc's hook directly
(ram_run_vblank_func) when it returns normally. A direct entry must still do
what a CPU dispatch does: the token 4 cycles (the OAM idle counted on them;
an H-Blank DMA moves on them), the depth guard and the step budget. VBlank's
eight pushes stay: the function runs 16 bytes lower under them, and fuzzed
code reads below sp (the one-byte VRAM diff when they were locals). Bulk
regex rewrites: check they didn't rewrite the new wrapper's own fallback
(transfer_virtual_oam calling itself became a tail-call loop that hung the
fuzz tier for hours while the quick tier passed).
gb_tail_jump (cpu.c, 2026-11-10): the asm's jp at a routine's end, from a
hook that returns normally. A ROM target whose hook returns runs directly,
RAM code ram_tail_jump knows (OAM DMA, copied V-Blank functions, the tile
copy loops) runs natively, anything else runs on the CPU at the same sp
until its ret pops the hook's return address (sp is put back, nothing
written). With it LevelVBlankFunc, ProcessLevelTileQueue_VRAM1, every
copied V-Blank .Func, the room/level loaders and ProcessBlock return
normally, and VBlank's call wVBlankFunc is push ret / gb_tail_jump / pop.
Left manual: Func_33f8 / Func_3407 (homejp into object states that end in
jp HomeJumpRet; those states need plain versions first), the frame loop,
Start / Init. Fuzzer rule that came with it: a hook the CPU dispatches during
a check adds its declared clobbers to what isn't compared
(test.nested_clobbers): a non-manual routine's check now runs the RAM code
it jumps to, and fuzzed code there can call clobbering hooks.
To find what reads a register: poison it, relax the contracts, run the
full tier. Stop if the build fails: ctest happily runs the old binary.
Routines nothing references and whose callers' hooks have their own copies
lost their hooks (PORT_COVERED, with a comment for the unreachable ones).
Fuzzing a V-Blank `.Func` label reports 0 checks (all inconclusive): the
quick tier's traces are what covers those.

## Recordings

`tests/inputs/*.input` (plus `.state`) drive both the difftest and the
fuzzer's real seeds. More recordings that cover other areas (later levels,
transformations, the golf minigame, bosses) raise the "tested" counts, so
more routines can be ported with real calls behind them.

To record one, run `play.bat --record port\tests\inputs\name.input`, then
commit both files. Recording and replay run the original asm by default.
