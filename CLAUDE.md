# Wario Land 3 disassembly + PC port

The disassembly builds the ROM with RGBDS (`make`). `port/` is the PC port.
It's a C + SDL2 frontend that runs the ROM on an interpreter, with routines
hand-ported to C one at a time and checked against the asm.

Before working on the port, read:
- `port/docs/HANDOFF.md`: current state, the batch workflow, pitfalls and
  open issues.
- `port/docs/PORTING.md`: how a routine is ported (exact registers, flags
  and memory).

Rules for ported code:
- Every ported routine must match the asm exactly: registers, flags and
  memory.
- Fuzz new routines with `port/tools/fuzz_batch.sh`.
- Run the quick tier, `ctest --test-dir port/build -L quick -j6` (the
  difftests and per-frame traces, a few minutes), before committing.
  Run the full fuzz tier (`-L fuzz`, about an hour) only when something
  looks wrong.
- Never skip or disable a test to get green.
