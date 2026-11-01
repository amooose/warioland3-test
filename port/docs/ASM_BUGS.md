# Bugs kept from the original game

The port reproduces the asm exactly (the difftests and fuzzing compare it
instruction for instruction), so bugs in the original game are ported as
they are. This is the list of them, to fix at the end, once parity testing
is done. A fix should sit behind an option (e.g. a `--fix-bugs` flag or a
build setting) so the parity tests can still run against the asm's
behaviour.

When a port keeps a bug, add it here and mark the code with a `// bug:`
comment.

Status: **confirmed**: the code clearly doesn't do what it means to;
**suspected**: odd, possibly intended; check before fixing. **Visible**:
whether a player can ever see it.

## Level objects

| Where | What | Status | Visible |
|---|---|---|---|
| Shoot, `.interacted_with_ball_wario` (shoot.cpp, `Shoot::interacted_with_ball_wario`) | Meant to compare Shoot's x position with 123 to pick which way to kick Wario. It compares `a`, which holds the low byte of SFX_FAT_WALK (0x3b), so the comparison always passes and Shoot always kicks to the right. | confirmed (marked in the disassembly) | yes, if the kick was meant to go both ways |
| Hidden Figure, `Func_4ce39` (hidden_figure.cpp, `face_laugh_start`) | The face is meant to switch to its laugh state (`.Func_4cf55`) when a hand grabs Wario. It writes the new update function through `h`, which still points at w1d14e's page, not wCurObj. The two bytes land in the w1d1xx page, so the face never laughs. Only the state duration (0x3c) is set. | confirmed | yes: the laugh animation and sound never play |
| Hidden Figure, hand `.Func_4d0c7` vs `.Func_4d3c8` (hidden_figure.cpp, `hand_rest`) | When starting an attack, the left hand clears var2, var3 and the state. The right hand clears var2, var3 and the **substate**: its `ld [hli]` is an `ld [hl]`. | suspected | unknown |
| Nobiiru, sting step (nobiiru.cpp) | Stores the new interaction type as the collision box's left edge (the asm reuses `a`). | suspected | maybe: odd hitbox while stinging |
| Shoot, `.ScoreGoal` (shoot.cpp, `SCORE_GOAL`) | If wShootGoals is already 3 it skips the increment and compares a stale `b`. Unreachable in normal play (Shoot has won by then). | confirmed, latent | no |

## Elsewhere

| Where | What | Status | Visible |
|---|---|---|---|
| `QueueBlockTilesAlongRow_*` (level block queue) | Writes one byte past the tile queue into the first attribute-queue entry, and that byte comes from the stack (a caller's return address). One tile attribute is wrong until the room redraws (e.g. `yellowbelly`, game frame 16296, $9929 in VRAM bank 1). | confirmed | yes, briefly |
| `Func_811cf` (overworld) | `ld [hl], a` writes 0 through hl, which still holds the jumptable's target: it writes the MBC's RAM-bank register. | confirmed, harmless | no |
| `LoadTreasureTiles` (src/engine/overworld/bank26.asm) | Falls through into `.CopyGfx` after the fourth treasure, copying a fifth time. | confirmed (marked in the disassembly) | unknown |
| `CalculateChecksumLong` (src/engine/pause/save/checksum.asm) | Its loop jumps to `CalculateChecksum` instead of `.start`. | confirmed (marked in the disassembly) | unknown; changing it changes save checksums |
