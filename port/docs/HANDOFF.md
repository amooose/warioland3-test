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
- **Progress:** 1263 / 3491 routines ported (36.2%), 21.9% of the
  instructions. With hooks on, about half of what the game executes runs as
  C. Run `cmake --build build --target progress` and read
  `build/progress.md` for the per-directory numbers and the "ready to port"
  list.
- **Fully ported:** `src/audio`. `src/home` is about 72% ported (by
  instructions), 29% of the level engine (8% of the objects), 22% of the
  overworld, and 25-35% of the title, pause, clear and action help code.
- **Tests:** `ctest --test-dir build -j6` (about 16 minutes now; the
  fuzz passes grow with every port) runs a difftest and a fuzz pass
  for each input in `tests/inputs/` (8 tests with the 4 inputs there now;
  `action_help2` is a local recording that isn't committed yet).

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
ctest --test-dir build -j6     # ~1 min
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
   from `port/`. It prints a line per (routine, input) and the first lines
   of any failure. A batch of about 30 routines takes about 10 minutes.
5. **Fix any FAIL.** The report shows the entry registers and the first
   differences. For more detail, run `./build/wl3 --fuzz --input X
   --fuzz-routine R --fuzz-verbose`.
6. **Run `ctest`.** It fuzzes every routine, so it catches a change to a
   shared helper breaking something else.
7. **Commit and push.** Each commit message lists the routines and the
   verification.

### Porting in parallel with agents

Four agents ported 512 routines at once this way:

- Make a git worktree per agent by hand (`git -c safe.directory=* worktree
  add -b port/<area> ../wl3-wt-<area> HEAD`). The Agent tool's own worktree
  isolation refuses this repo (the ownership check fails).
- Give each agent a disjoint set of files (objects; Wario states and
  mechanics; overworld; everything else), and forbid edits to `ported.h`
  and the docs, so the commits cherry-pick cleanly.
- Limit each agent to `cmake --build -j2`, `JOBS=1` fuzzing and `ctest -j1`:
  four agents at full parallelism peg the machine.
- Verify after merging with the full ctest here: the worktrees lack the
  local `action_help2` recording, which caught a bug the agents' runs
  missed.
- Before committing, scan for `CALL(X)` / `JUMP(X)` to a routine in the
  caller's own ROMX bank (they abort when a fuzzed bank is mapped; use
  `gb_call` / `JUMP_TO`). A short script over `warioland3.sym` and the
  `PORT(...)` blocks finds them.

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

2. **The fuzzer's step budget.** When the routine never runs on an input,
   the fuzzer makes up a call from the final state; if that call hangs (a
   wait for H-Blank with the LCD off), the seed hits `MAX_STEPS` and every
   variant then gets 20 times that, so one routine runs for minutes. This
   keeps `LoadActionHelpBGPals` and its callers
   (`LoadActionHelpPalsWithWater`, `LoadActionHelpPalsWithoutWater`,
   `Func_1e48bc`) in asm. Fix: cap the variants' budget when the seed hit
   `MAX_STEPS` (difftest.c).
3. **Coverage gaps.** `UpdateRoomAnimatedPals` (60/104: the palette copy
   loop only runs in levels with palette cycling), `MizzouProjectileFunc`
   (54/59), `SilkPlatformsFunc` (18/26), `TransitionAfterSave` (18/32),
   `Func_850ff` (24/30). Most transformation states (snowman, ball, fan,
   owl, vampire, puffy, crazy, bouncy) are only checked with synthetic
   calls: recordings of those would help most.

## Good next batches

- **Wario states left by the last batch:** `SetState_Crazy`,
  `SetState_CrazySpinning`, `SetState_WaterStung`, `SetState_Swallowed`,
  `SetState_FanStart`, `SetState_IceSkatinStart`, `HandleWarioOwlInput`,
  `SetState_SplitHit`, `SetState_TurningIntoSnowman`, `SetState_Launched`,
  `SetState_PuffyInflating`, `SetState_MagicRising`, `UpdateState_BallShot`,
  `ObjInteraction_Rail`, `TurnIntoSmallSnowball`,
  `UpdateState_GettingFlatAirborne`, `SetState_FlatAirborne`, `GetTreasure`,
  `HandleGroundShake`.
- **Objects:** only the entry points of most object functions are ported;
  their local `.Update` code (reached through the update pointer) and the
  big enemies (`FlameBlockTorchUpdate`, `InkFunc`, `DollBoyHammerFunc`,
  `SpearheadFunc`, `ParaGoomFunc`, ...) are next.
- **Action help:** `action_help/power_ups` (316 routines) is barely started
  (`none.c`).
- Not portable as is: `EnableDoubleSpeed` (`stop`), `Func_81feb` (its
  `debug_nop` path loops forever).
- `build/progress.md` lists the rest under "Ready to port".

## Recordings

`tests/inputs/*.input` (plus `.state`) drive both the difftest and the
fuzzer's real seeds. More recordings that cover other areas (later levels,
transformations, the golf minigame, bosses) raise the "tested" counts, so
more routines can be ported with real calls behind them.

To record one, run `play.bat --record port\tests\inputs\name.input`, then
commit both files. Recording and replay run the original asm by default.
