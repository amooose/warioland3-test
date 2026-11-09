// overworld/bank2b_2 (src/engine/overworld/bank2b_2.asm): all C++ in
// src/native/overworld (the prologue and epilogue scenes). These helpers'
// callers are all ported and have their own plain copies.
#include "../native.hpp"

PORT_COVERED(Prologue_Delay);
PORT_COVERED(Prologue_Next);
PORT_COVERED(Prologue_Finish);
PORT_COVERED(Func_ae1c2);
PORT_COVERED(Func_ae1c9);
PORT_COVERED(Func_ae1d5);
PORT_COVERED(Func_af130);
PORT_COVERED(Func_af135);

// Unreferenced in the original, so unreachable: nothing to run. Marked for
// the progress report.
PORT_COVERED(Func_af01f);
