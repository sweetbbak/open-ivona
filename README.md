# IVONA matching decompilation

This is a decomp.me-style matching decompilation of the IVONA Android TTS engine
(`eng.gbr.amy.1.6.101.3`). Every function is rewritten in C/C++ and compiled with the original
compiler. The harness then proves the result is byte-identical to the shipped binary. For
relocations, it checks that each reference points at the same import, function, data object or
string contents as in the original.

## Make it speak

```
./speak "Hello world" hello.wav
./speak --rate 1.5 --pitch 0.9 --volume 1.2 "Hello world" hello.wav
```

`--rate` (about 0.67 slow .. 4 fast), `--pitch` (0.5 .. 2) and `--volume` (0 .. 1.6) are the
engine's voice parameters; 1 is normal. `--speed` is `tts_streamer_start`'s speed, which only
paces delivery (1 real time, -1 as fast as possible) and does not change the sound.

That's all. The first run downloads what it needs and builds everything; later runs rebuild
the engine only if a source changed. Details: `docs/notes/runnable-engine.md`.

| target | what |
|---|---|
| `libttsivonavoice.so` | the engine (ARM + Thumb, C/C++) |
| `libvoice_en_gb_amy.so` | Amy voice code + data, loaded by the engine |
| `libtiver.so` | voice registry (pilot binary) |

## Toolchain facts (verified)

- **Compiler.** Android NDK r10e `arm-linux-androideabi-4.8` (`.comment` = `GCC: (GNU) 4.8`),
  linked with gold. Every compile runs inside a sealed podman image (`localhost/ivona-ndk:r10e`)
  built from a pinned Debian digest and an NDK archive with a pinned checksum. Builds are
  bit-reproducible.
- **Flags.** NDK armeabi defaults: `-fpic -ffunction-sections -funwind-tables
  -fstack-protector -march=armv5te -mtune=xscale -msoft-float`.
  - Thumb code: `-mthumb -Os`. This is calibrated and matches.
  - ARM code: `-marm -O2`. This is an uncalibrated guess.
- **Library code.** Thumb `libgcc.a`, `libgnustl_static.a` (GNU libstdc++/libsupc++ 4.8) and
  `crtbegin_so.o` are linked in statically. They're identified and verified automatically with
  `dc libmatch`.

## Setup

```sh
toolchain/fetch_ndk.sh r10e      # downloads + verifies NDK, builds the podman image
toolchain/fetch_ndk.sh r9d       # optional second GCC 4.8 build, for comparison
python3 -m venv .venv && .venv/bin/pip install capstone==5.0.3 pyelftools==0.31 pyyaml==6.0.2
./dc selftest                    # proves the matcher end to end (must say PASSED)
```

Splitting needs Ghidra (`/opt/ghidra`, or set `GHIDRA_HOME`). It runs once per binary.

## Commands

All commands take `-b <stem>` (the default is in `config/project.yaml`).

```
./dc split              build symbols/<bin>.funcs.tsv (Ghidra export + exidx + code pointers + calls)
./dc ls [status...]     list functions
./dc asm FUNC           annotated target disassembly (GOT/PC-relative/string literals resolved)
./dc diff FUNC -s SRC   compile SRC, diff FUNC side by side, record status/score
./dc diff FUNC -X "-O2" try extra compiler flags (does not save)
./dc check              re-verify every matched + library function (run before committing)
./dc libmatch           find/verify statically linked NDK library functions
./dc data ADDR NAME [SIZE]   name a data address
./dc dump ADDR N FIELDS emit C initializers for an array of structs (s,p,i,u,h,H,b,B,f)
./dc rename OLD NEW     rename a function/data symbol in the tables and every source file
./dc status             progress table -> docs/progress.md
./dc selftest           link sample code with gold, require 100% + catch planted mutations
```

## How a match is decided

1. The candidate TU is compiled with `-ffunction-sections`. The function's section and its
   relocations are extracted from the `.o`.
2. Bytes that aren't covered by a relocation must be identical. Bytes after the end of the
   candidate must be inter-function padding.
3. Each relocation's linker formula is inverted on the target bytes. That gives where the target
   placed the referenced symbol, which must be the same entity:
   - **Calls.** These go through veneers and the PLT to the same import, or to the function with
     that name in `symbols/<bin>.funcs.tsv`. BL and BLX are interchangeable (interworking).
   - **GOT, GOT_PREL and GOTOFF references, PC-relative literals and absolute pointers.** These
     resolve to the named symbol's address. The address comes from dynsym, the tsv tables, or a
     `FUN_xxxxxxxx`/`D_xxxxxxxx` auto name.
   - **Static data.** The candidate's bytes must equal the target bytes at that address. Pointers
     inside the data are followed and checked the same way, and strings are compared by content
     (gold tail-merges strings). The output section must match too: `const` vs non-`const`
     changes `.data` vs `.data.rel.ro.local`.
   - **`.bss` statics.** Placement must be consistent across every function of the TU. `dc check`
     enforces this.

## Layout

```
target/        original .so files + target.sha256 (verified on every load)
config/        project.yaml (binaries, NDK libraries), <bin>.yaml (toolchain, flags, per-TU overrides)
symbols/       <bin>.funcs.tsv (addr size mode name status src score note), <bin>.data.tsv
src/<bin>/     matching sources, grouped into plausible original TUs
include/       shared headers (common.h: HIDDEN, EXTERN_C)
tools/dc/      the harness (python); tools/ghidra/ headless export script
tests/         selftest sources + driver
toolchain/     fetch_ndk.sh, Containerfile (NDKs are extracted here, gitignored)
bin/agcc       run a tool in the sealed compiler image (DC_NDK=r9d to switch)
docs/          progress.md (generated) and notes/
```

Function status values:
- `todo`, `wip` and `nonmatching`: work states.
- `matched`: verified C/C++.
- `lib`: verified NDK library code, or a linker veneer.
