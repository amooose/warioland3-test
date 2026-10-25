# Test inputs

Every `inputs/*.input` file is a test case: `ctest` replays it and checks
every ported routine that gets called against the original asm, and fuzzes
every ported routine using calls from the replay as starting points.

The fuzzer reaches every branch of the ported routines on its own, so
recordings aren't required for porting. They add realistic gameplay
starting points, and catch problems in how routines work together.

Add coverage by recording yourself playing:

```sh
./port/build/wl3 --record port/tests/inputs/world_n1.input
```

Play the part of the game you want covered, then quit with Esc. That writes
the input script and, next to it, `world_n1.input.state`: the exact machine
state the recording starts from. Commit both. To record a later part of the
game without replaying it from the start, load a save state first
(`--load-state`, or F7 in game) and the recording starts from there.

Recording uses the original game's timing (see "Recording" in the port's
README), so recordings stay valid as more of the game gets ported.

The format is described in `src/input.h`. Scripts can also be written by
hand; ones that start from power-on don't need a state.

States are tied to the exact ROM build. If the disassembly changes the ROM,
recordings need to be redone.
