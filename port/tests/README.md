# Test inputs

Every `inputs/*.input` file is a difftest case: `ctest` replays it and checks
every ported routine that gets called against the original asm.

Add coverage by recording yourself playing:

```sh
./port/build/wl3 --record port/tests/inputs/world_n1.input
```

Play the part of the game you want covered, then quit with Esc. That writes
the input script and, next to it, `world_n1.input.state`: the exact machine
state the recording starts from. Commit both. To record a later part of the
game without replaying it from the start, load a save state first
(`--load-state`, or F7 in game) and the recording starts from there.

The format is described in `src/input.h`. Scripts can also be written by
hand; ones that start from power-on don't need a state.

States are tied to the exact ROM build. If the disassembly changes the ROM,
recordings need to be redone.
