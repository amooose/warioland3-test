# Wario Land 3 for PC

A native PC port of Wario Land 3, built on top of this disassembly. It is in
progress: the goal is a full hand port to C, done one routine at a time, with
the game fully playable at every step along the way.

## How it works

The game is a mix of native C and the original code:

- **Native hardware layer** (`src/ppu.c`, `src/apu.c`, `src/mem.c`, `src/main.c`):
  the Game Boy Color's screen, sound, memory map, input and save RAM, presented
  through SDL2.
- **Ported routines** (`src/ported/`): hand-written C replacements for every
  asm routine, registered by their label from the disassembly. The CPU loop
  (`src/cpu.c`) runs the native routine at each address the game reaches.
- **SM83 interpreter** (`src/interp.c`, development builds only): runs the
  original code, for the tools below to compare against.
- **Differential tester** (`src/difftest.c`): plays the game and, every time a
  ported routine is called, runs both the original asm and the C version from
  the same state and compares the entire machine state afterwards. Porting
  mistakes show up immediately, on the exact call where they happen.

Every routine is now native: the game runs without the interpreter (see
"Building just the game" below), which stays in development builds for the
tests. See [docs/ROADMAP.md](docs/ROADMAP.md) for
the plan and [docs/PORTING.md](docs/PORTING.md) for how to port routines.

## Building

You need CMake (3.16+), a C compiler, SDL2 development files, GNU make and
[RGBDS 1.0+](https://github.com/gbdev/rgbds/releases) (to build the ROM from
the disassembly).

### Linux and macOS

```sh
# Debian/Ubuntu: sudo apt install cmake gcc make libsdl2-dev
# macOS:         brew install cmake sdl2 rgbds
cmake -B port/build -S port            # add -DRGBDS=/path/to/rgbds/ if not on PATH
cmake --build port/build
./port/build/wl3
```

### Windows

Building the ROM uses the repository's `Makefile` and Unix tools, so build
from an [MSYS2](https://www.msys2.org/) **UCRT64** shell (not `cmd` or
PowerShell).

1. Install MSYS2, then open "MSYS2 UCRT64" from the Start menu and run:
   ```sh
   pacman -S --needed git make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
       mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2
   ```
2. Download `rgbds-win64.zip` from the
   [RGBDS 1.0.0 release](https://github.com/gbdev/rgbds/releases/tag/v1.0.0)
   and extract it, for example to `C:\rgbds`.
3. Build, from the repository folder (Windows drives are under `/c/`):
   ```sh
   cd /c/Users/you/path/to/warioland3-test
   cmake -B port/build -S port -G Ninja -DRGBDS=/c/rgbds/
   cmake --build port/build
   ./port/build/wl3.exe
   ```

After that, `build.bat` in the repository root rebuilds without opening the
MSYS2 shell (`build.bat run` also starts the game). Edit the paths at its top
if MSYS2 or RGBDS live somewhere other than `C:\msys64` and `C:\rgbds`. From
the MSYS2 shell, `port/build.sh` does the same. Close the game before
building: a running `wl3.exe` can't be replaced.

`push.bat` pushes the current branch's commits to
`https://github.com/amooose/warioland3-test`.

`play.bat` starts the game (double-click it, or pass options:
`play.bat --record my_run.input`). The build also copies `SDL2.dll` next to
`wl3.exe`, so it runs from Explorer too.

The build compiles `warioland3.gbc` from the disassembly first, and then
generates C constants for every label in it (`build/generated/syms.h`), so
ported code always matches the current state of the asm.

### With your own ROM, without the disassembly

The port runs on the original Wario Land 3 ROM, which the disassembly
rebuilds byte for byte. To build just the port, without RGBDS or make:

```sh
cmake -B port/build -S port -DWL3_BUILD_ROM=OFF
cmake --build port/build
```

This uses the label and constant headers committed in `port/generated/`.
Put your ROM next to `wl3` as `warioland3.gbc`, or pass `--rom PATH`. The
port checks its SHA-1 (`bb7877309834441fd03adb7fa65738e5d5b2d7ba`, the
worldwide Game Boy Color release) and refuses any other. Add
`-DWL3_ROM=/path/to/your.gbc` to run the tests too.

After changing labels or constants in the asm, refresh the committed copies
with `cmake --build port/build --target update_generated` (a quick-tier
test fails until they match).

### Building just the game

`-DWL3_INTERPRETER=OFF` leaves out the interpreter and everything that uses
it (the difftest, fuzzer and traces):

```sh
cmake -B port/build -S port -DWL3_BUILD_ROM=OFF -DWL3_INTERPRETER=OFF
cmake --build port/build
```

Recordings made with it use the native routines' timing; replay them in a
development build with `--hooks --input`.

No ROM is distributed with this project.

## Playing

| Key | Button |
| --- | --- |
| Arrow keys | D-pad |
| X | A |
| Z | B |
| Enter | Start |
| Backspace / Right Shift | Select |
| Tab (hold) | Fast forward |
| F5 / F7 | Save / load state |
| F11 | Fullscreen |
| Esc | Quit |

Game controllers are supported through SDL. The game saves to
`warioland3.sav` next to the ROM, in the same format as emulator saves. F5
saves a snapshot of the exact moment to `warioland3.state`, and F7 loads it.

## Recording

```sh
./port/build/wl3 --record my_run.input
```

Records everything you press, until you quit, to `my_run.input`, plus
`my_run.input.state` (the exact starting point, including save data).
Replay it with `--input my_run.input`; replays are exact, on any platform.
Recordings are how the port gets tested: see [tests/README.md](tests/README.md).

Recording and replaying use the original game's timing, with the ported
routines switched off (it plays the same). Ported routines take no emulated
time, so normally the game has less slowdown and shorter loads than the
original, which would shift when recorded button presses land. On the
original's timing, a recording stays valid however much of the game gets
ported.

## Command line

Run `wl3 --help` for the full list. The main ones:

```
  --record PATH         record your input to an input script (plus PATH.state)
  --input PATH          play an input script
  --load-state PATH     start from a save state
  --save-state N:PATH   save a state at frame N
  --headless            no window or audio
  --frames N            stop after running N frames
  --screenshot N:PATH   write a PNG of frame N
  --wav PATH            record the audio output
  --difftest            check every ported routine against the original asm
  --no-hooks            run everything in the interpreter
  --trace               print CPU state every frame
  --break LABEL         print recent instructions whenever LABEL is reached
  --watch LABEL         print the byte at LABEL every frame
  --coverage PATH       write how often each routine runs during an --input
  --fuzz                fuzz every ported routine (seeded from --input)
  --fuzz-routine LABEL  fuzz only this routine
  --fuzz-verbose        list the asm instructions fuzzing never executed
```

Headless runs don't read or write the battery save unless `--sav` is given.

## Testing

```sh
ctest --test-dir port/build --output-on-failure
```

For every input script in `tests/inputs/`, this runs:

- the **difftest**, which replays it and checks each ported routine against
  the original asm on the calls that happen;
- the **fuzzer**, which takes calls from the replay (or synthetic calls for
  routines it never reaches) and checks hundreds of variations of each, with
  random registers and mutated values in the memory the routine reads. It
  also reports how many instructions of each original routine the checks
  executed.

Either fails if any ported routine behaves differently from the original.
To fuzz one routine while porting it:

```sh
./port/build/wl3 --fuzz --input port/tests/inputs/boot_through_n1.input --fuzz-routine Label --fuzz-verbose
```

```sh
cmake --build port/build --target progress
```

prints how much is ported and which routines are ready to port next
(full report in `port/build/progress.md`).
