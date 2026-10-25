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

You need CMake (3.16+), a C compiler, SDL2 development files and
[RGBDS 1.0+](https://github.com/gbdev/rgbds/releases) (to build the ROM from
the disassembly). Building the ROM uses the repository's `Makefile`, so on
Windows use MSYS2 or WSL.

```sh
# Debian/Ubuntu: sudo apt install cmake gcc make libsdl2-dev
# macOS:         brew install cmake sdl2 rgbds
cmake -B port/build -S port            # add -DRGBDS=/path/to/rgbds/ if not on PATH
cmake --build port/build
./port/build/wl3
```

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
| F11 | Fullscreen |
| Esc | Quit |

Game controllers are supported through SDL. The game saves to
`warioland3.sav` next to the ROM, in the same format as emulator saves.

## Command line

```
wl3 [options]
  --rom PATH          ROM built from the disassembly
  --sym PATH          symbol file (default: ROM path with .sym)
  --sav PATH          save file (default: ROM path with .sav)
  --scale N           window scale (default 4)
  --no-hooks          run everything in the interpreter
  --input PATH        scripted input: lines of '<frame> <duration> <buttons>'
  --headless          no window or audio
  --frames N          stop after N frames
  --screenshot N:PATH write a PNG of frame N (repeatable)
  --wav PATH          record the audio output
  --difftest          check every ported routine against the original asm
  --trace             print CPU state every frame
  --break LABEL       print recent instructions whenever LABEL is reached
```

## Testing

```sh
ctest --test-dir port/build --output-on-failure
```

This runs the difftest over a scripted playthrough (`tests/difftest.input`).
It fails if any ported routine behaves differently from the original.
