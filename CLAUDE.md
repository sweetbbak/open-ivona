# Working in this repo (matching decomp of IVONA ARM32 .so files)

Read README.md first. The rules below are what keeps the project deterministic.

## Per-function loop

1. **Pick a function.** Use `./dc ls todo`, preferring leaves and functions whose callees are
   already named. Read it with `./dc asm FUNC`. You can also use Ghidra output. Treat
   `../eng.gbr.amy.1.6.101.3/*.so.c` (retdec) and `../open-ivona` as hints only, never as truth.
2. **Write C/C++.** Put it in the TU it plausibly came from, `src/<bin>/<module>.c|.cpp`.
   - Name the function exactly as in the tsv. Rename first with `./dc rename FUN_xxx good_name`.
     Use the real mangled name for C++ (keep it via the C++ signature) or `extern "C"`.
   - Put a short doc comment above it: address, mode, what it does.
3. **Diff.** Run `./dc diff FUNC -s src/<bin>/<file>`, then iterate until `MATCH`. Status, score
   and src are saved to `symbols/<bin>.funcs.tsv` automatically.
4. **Check before committing.** Run `./dc check` (it must pass) and `./dc status`.

## Rules

- Never edit `target/`. `target.sha256` is checked on every load.
- Never relax the matcher to make a function pass. If the harness is wrong, fix it, then extend
  `tests/selftest.py` so the case stays covered and `./dc selftest` passes.
- Compiler flags live in `config/<bin>.yaml`.
  - Per-TU deviations go in `units:`, each with a comment saying why.
  - For experiments use `-X`, which never saves.
  - Run a different NDK with `DC_NDK=r9d`.
- **Static data referenced by code.** Define it in the TU, using `./dc dump` to generate the
  initializers. The matcher compares its bytes, strings and pointers.
  - A `HIDDEN` extern (include/common.h) only makes sense as a temporary stand-in. It can change
    register and operand order compared with a TU-local definition (seen on GetVoiceDataFunc).
- **`const` matters.** It decides whether data lands in `.data.rel.ro.local`, `.rodata` or `.data`,
  and the matcher checks this.
- **Library code is not hand-written.** Leave `lib` functions to `./dc libmatch`.
- **Thumb-1 output from GCC 4.8 `-Os`:**
  - `adds rX, rY, #0` is a move.
  - `bl __gnu_thumb1_case_*` is a switch.
  - GCC 4.8's reassoc pass merges `i < 0 || i > N` into one unsigned compare. Separate checks in
    the binary usually mean `if (i < 0) ...; size_t u = i; if (u >= N) ...`.
