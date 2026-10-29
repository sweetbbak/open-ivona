# Runnable voice library

Goal: `libvoice_en_gb_amy.so` built from source, so that no original binary is loaded at all.
This is step 1 of porting: both a native Linux build and an arm64 Android build need the voice
library as source. The matching decompilation keeps going alongside it.

## How it is checked

Synthesis is deterministic: the same text always gives byte-identical audio. So the decompiled
voice library has to reproduce the original's output exactly, not just sound right.

```
tools/difftest.sh                     # corpus spoken with original and decompiled voice libraries
./speak --voice-lib decomp "text" x.wav
```

- **The test.** `tools/difftest.sh` speaks every line of `tests/run/corpus.txt` with the same
  (decompiled) engine twice: once with `target/libvoice_en_gb_amy.so` and once with the
  decompiled build. The WAVs must be identical.
  - The corpus covers plain words, punctuation and phrasing, numbers, dates, times, money,
    abbreviations, acronyms, symbols, foreign words, the lang.dat PLS words and a long paragraph.
  - Add a line whenever a bug shows up that the corpus missed.
- **Sanity check.** Original against original gives 17/17.
- **Build.** `tools/build_runnable.py -b libvoice_en_gb_amy` writes
  `build/voice/libvoice_en_gb_amy.so`. It keeps the original's four exports (`register_voice`,
  `unregister_voice`, `language_data`, `TTS_ENGINE_1_6`) and reports what is still undefined.
- **Shared sources.** Many of the voice library's functions are compiled from engine sources (the
  embedded Flite, string and vector utilities). Where two credited files define the same
  function, the build weakens the copy the function table does not credit, so the table decides.

## Inventory (2026-10-29)

- **Size.** 1790 functions, 0xd7d40 bytes of code.
- **Library code, verified by `dc libmatch`.** 70.8% of the bytes, the voice library's own copies of:
  - PCRE 8.37 (239 functions)
  - SILK 1.0.8 (70)
  - cmp v16 (166)
  - libgcc
  The same `cmp` leftovers as in the engine remain (byte-order helpers, `cmp_read_object`).
- **IVONA's own code.**
  - 611 functions match; 66 are `wip`.
  - About 550 are `todo`, roughly 260 KB, mostly at 0x28000–0x68000:
    - text analysis, lexicon and letter-to-sound, postlex, prosody (CART, durations, DNN models),
      unit selection, the vox database;
    - a cluster of about 100 small functions at 0xe7000–0xf0000, not identified yet.

## Approach

1. **Link.** Grow the set of functions until the library links. `--gc-sections` keeps only what is
   reachable from `register_voice`, through code and data, so the undefined list grows as data
   tables (lexicon, CART trees, module tables) are defined.
2. **Write functions to match where practical, correctly always.** Functions that already match
   are correct by construction. A non-matching one is accepted when the difftest passes.
3. **Data.** Generate static tables with `./dc dump`, not by hand. `amy_lexicon` (0x3bb67c) is
   the big one.
4. **Stubs.** None until the library links. If one is needed for a step, it goes in
   `src/libvoice_en_gb_amy/runnable_stubs.c` and is listed here.

## Status

- **Links?** No. 10 symbols are undefined on the first link: `amy_lexicon`, the vox database
  loader `FUN_00029dd0`, `FUN_0004b790`, `item_set_breath_acoustic_data`, `named_table_find`,
  `val_dur_stats` and four one-instruction `val_delete_*` wrappers. The list grows as data
  tables are defined.
- **Difftest.** Harness ready.
