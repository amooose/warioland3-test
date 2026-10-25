# Roadmap

The goal is a complete hand port of Wario Land 3 to C, which can then be
extended beyond what the Game Boy Color allows: widescreen, new levels and
mechanics, and mods and scripting.

The guiding rule: **the game stays playable and verified at every step.**
Nothing is ported blind. Every ported routine is checked against the original
by the difftest, so bugs surface on the routine that caused them instead of
months later in a playtest.

## Scale

From the disassembly: about 109,000 instructions in about 8,500 global
labels (including data labels). Roughly:

| Area | Instructions |
| --- | --- |
| `engine/level` (Wario, objects, rooms, level logic) | 60,000 |
| `engine/overworld` (map, cutscenes) | 22,000 |
| `engine/action_help` | 8,000 |
| `home` (ROM0 core routines) | 4,000 |
| `engine/golf` | 3,300 |
| `engine/pause` | 2,400 |
| `audio` | 2,200 |
| everything else | ~7,000 |

## Phase 0: foundation (done)

- [x] SM83 interpreter, MBC5, CGB memory map, double speed, HDMA
- [x] CGB scanline renderer (BG, window, sprites, priorities, palettes)
- [x] APU (2 square channels with sweep, wave, noise), 48 kHz stereo output
- [x] SDL2 frontend: window, scaling, fullscreen, keyboard, controllers,
      audio-synced pacing, battery saves
- [x] Hook system: replace any routine by its label
- [x] Build generates C constants for every label from the `.sym` file
- [x] Differential tester, verified to catch injected bugs
- [x] Headless mode, scripted input, screenshots, audio capture, tracing
- [x] First ported routines (`home/copy`, `home/joypad`, `home/clear_memory`)

The game boots and plays through the intro, title screen, prologue and into
the first level.

## Phase 1: porting infrastructure (mostly done)

Things that make porting at scale fast and safe:

- [x] **Input recording**: `--record PATH` writes the input script while you
      play, so building coverage for the difftest is just playing the game.
      Replays are bit-exact, across platforms.
- [x] **Save states**: start the game (and the difftest) from any point
      instead of replaying from boot. F5/F7 in game.
- [x] **Constants header**: every constant in the disassembly, evaluated by
      rgbasm itself, as C constants with the same names.
- [x] **Progress report**: what's ported, per directory, and which routines
      are ready to port next (call graph, callee-first, most-called first).
- [x] **CI**: build and run the difftests on every push.
- [ ] **Coverage**: recordings that play through each world, so every
      ported routine is exercised by the tests.

## Phase 2: port the code

Bottom-up, callee-first, one subsystem at a time, each one verified by the
difftest before moving on:

1. `home/` (ROM0): memory helpers, decompression, OAM/sprite building,
   palettes, bank and call helpers.
2. Audio engine.
3. Menus, title, pause, game over, credits and the other small screens.
4. Overworld and cutscenes.
5. Level engine: rooms and scrolling, Wario's states and physics, objects and
   enemies, interactions, transformations, bosses.
6. Golf and the remaining minigames.
7. Interrupt handlers (VBlank, LCD) and the main loop. These come last
   because they drive everything else.

Throughout this phase the ported code still uses the emulated memory
(`wr8(gb, wFoo, ...)`) and the asm register calling convention. That's what
lets ported and unported code coexist.

## Phase 3: become native

Once no game code runs in the interpreter:

- Remove the interpreter from the default build. Keep it in a test build, with
  the ROM, as the reference for the difftest.
- Replace the register calling convention with real C parameters and return
  values, routine by routine.
- Move game state from the emulated WRAM into C structs (Wario, objects,
  level state, save data). Keep a compatibility mapping while it happens so
  the difftest still works.
- Load graphics, levels, text and music from asset files (the disassembly
  already has most of them as PNG and binary files) instead of from the ROM.

## Open decision: slowdown

Native code is faster than the original, so the port has less slowdown than
the Game Boy Color. Most PC ports treat that as an improvement. If exact
original timing turns out to matter (speedrunners, timing-based tricks), an
option can charge native routines the cycles the original would have taken.

## Phase 4: beyond the Game Boy

- **Native renderer**: draw layers and sprites directly instead of emulating
  the PPU. Lifts the 160x144 screen, 40-sprite and 10-per-line limits and
  the palette count. Widescreen and higher internal resolution.
- **Audio**: keep the APU emulation for authenticity, optionally with
  higher-quality output, or replace the sound engine entirely.
- **Quality of life**: rebindable controls, save slots, options menu.

## Phase 5: modding and expansion

Available in layers, from the very start:

1. **Asm and data mods (available now)**: edit the disassembly, graphics,
   level data or text, then rebuild. The port picks up the changes
   automatically.
2. **C mods**: override or hook any routine by label, the same mechanism the
   port itself uses. Can be packaged as plugins once there's a stable API.
3. **Scripting**: embed Lua (or similar) with hooks at well-defined points
   (frame, room load, object spawn and update) and an API over the native
   game state from phase 3 (`wario.x`, `spawn_object(...)`, ...).
4. **Content**: level and room format documentation, then an editor.
   Expansion past the original ROM's size limits follows naturally once data
   loads from files.
