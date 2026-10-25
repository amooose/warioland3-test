# Wario Land 3 for PC

A native PC port of Wario Land 3, built on top of this disassembly. It is in
progress: the goal is a full hand port to C, done one routine at a time, with
the game fully playable at every step along the way.

## How it works

The game is a mix of native C and the original code:

- **Native hardware layer** (`src/ppu.c`, `src/apu.c`, `src/mem.c`, `src/main.c`):
  the Game Boy Color's screen, sound, memory map, input and save RAM, presented
  through SDL2.
- **SM83 interpreter** (`src/cpu.c`): runs every routine that hasn't been
  ported yet, straight from the ROM this repository builds.
- **Ported routines** (`src/ported/`): hand-written C replacements for asm
  routines, registered by their label from the disassembly. When execution
  reaches a ported routine, the C version runs instead.
- **Differential tester** (`src/difftest.c`): plays the game and, every time a
  ported routine is called, runs both the original asm and the C version from
  the same state and compares the entire machine state afterwards. Porting
  mistakes show up immediately, on the exact call where they happen.

As more routines are ported, less runs in the interpreter. Once everything is
native, the interpreter goes away. See [docs/ROADMAP.md](docs/ROADMAP.md) for
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

To run `wl3.exe` outside the MSYS2 shell, copy `SDL2.dll` from
`C:\msys64\ucrt64\bin` next to it.

The build compiles `warioland3.gbc` from the disassembly first, and then
generates C constants for every label in it (`build/generated/syms.h`), so
ported code always matches the current state of the asm.

No ROM is distributed with this project, and none is needed beyond the one
you build from this repository.

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
```

Headless runs don't read or write the battery save unless `--sav` is given.

## Testing

```sh
ctest --test-dir port/build --output-on-failure
```

This runs the difftest over every input script in `tests/inputs/`. It fails if
any ported routine behaves differently from the original.

```sh
cmake --build port/build --target progress
```

prints how much is ported and which routines are ready to port next
(full report in `port/build/progress.md`).
