# Runnable engine build

`tools/build_engine.py` links a working `libttsivonavoice.so` from the decompiled sources. It is
separate from the matching build, so non-matching code is fine as long as it is correct.

```
.venv/bin/python tools/build_engine.py            # -> build/engine/libttsivonavoice.so
.venv/bin/python tools/build_engine.py -D IVONA_CERT_SSML=1   # with SSML input (ssml-deferred.md)
```

## What it does

- **Sources.**
  - It compiles every TU the function table credits, with the TU's own flags plus
    `-DIVONA_NO_CERT_CHECK` (see `cert-bypass.md`). Output goes to `build/engine/obj`.
  - The table, not the directory, decides which TUs go in. Some engine functions are credited to
    voice-library sources, and the superseded `src/libttsivonavoice/flite/cst_phoneset.c` would
    clash.
  - `EXTRA_SRCS` adds the data-only and stub TUs that no row credits:
    - `charset_tables.c`: the 24 code page tables
    - `runnable_stubs.c`
- **Libraries.**
  - It links the vendored archives: OpenSSL, curl, Expat, PCRE, SILK and cmp.
  - `dc vendor build` writes those without a symbol index (`ar rcS`), so the script links
    ranlib'd copies.
- **Exports.**
  - It keeps only the original exports (`config/libttsivonavoice.exports`: `tts_*`, `CIvonaTTS`,
    `android_syslog`).
  - It links with `--gc-sections`, so unreachable code (such as `cst_wave_append_riff`) drops out,
    as it did in the original.
- **Report.** It lists the undefined symbols by table address. When that list is empty, the
  engine is linked.

**Status (2026-10-28).** It links (`build/engine/libttsivonavoice.so`, 2.0 MB) with the same
NEEDED libraries and exported API as the original. **It speaks.** Under qemu with the original
`libvoice_en_gb_amy.so` and Amy's data, it gives:
- "hello world": 1.04 s, a speech-like envelope (/h/ onset, voiced middle, release);
- "The quick brown fox jumps over the lazy dog. It was the 3rd of May, 2016.": 7.0 s.

## Running it (qemu-user + real bionic)

The one command for all of this is `./speak "text" out.wav`: it fetches, builds and runs as
needed. What it does by hand:

```
toolchain/fetch_android_sysroot.sh    # once: Android 4.4 ARM /system -> toolchain/android-19
bin/agcc sh -c 'arm-linux-androideabi-gcc --sysroot=/opt/sysroot -Iinclude \
    -o build/run/tts_run tests/run/tts_run.c -Lbuild/engine -lttsivonavoice'
cd DIR_WITH_libvoice_en_gb_amy.so_vox_en_gb_amy22i_lang_en_gb.dat
tools/run_android.sh -l build/engine -l . build/run/tts_run CERT libvoice_en_gb_amy.so \
    vox_en_gb_amy22i "hello world" out.wav
```

- **Runner.** `tests/run/tts_run.c` is `examples/example.c` with arguments and WAV output. Any
  certificate file works (see `cert-bypass.md`).
- **How it runs.** `tools/run_android.sh` runs `qemu-arm -L toolchain/android-19`, so the
  programs use Android's own linker and libc. It needs two things:
  - **System property area.** bionic aborts without `/dev/__properties__`, and only accepts one
    owned by root. The script writes an empty KitKat property area there and runs inside a user
    namespace (`unshare -r`), where the file looks root-owned.
  - **PID namespace.** KitKat bionic stores only 16 bits of the owner tid in a mutex. With host
    tids above 65535, the dynamic linker's recursive `dlopen` lock deadlocks on its own owner,
    hanging the first `dlopen` (of the voice library). `unshare --pid --fork` keeps tids small.
    This is probably also what hung the earlier qemu attempts.
- **Comparing with the original.** The same setup can run the original engine as a reference,
  once a copy of it gets the certificate check patched (in `build/`, never in `target/`).

## Stubs (`src/libttsivonavoice/runnable_stubs.c`)

| function | why it's stubbed | effect |
|---|---|---|
| `vorbis_unit_*`, `vorbis_ogg_unit_*` | Tremor deferred (vox codecs 8/12, Windows voices only) | opening such a voice fails; Android voices use SILK (14) |
| `ssml_audio_apply_attrs`, `phoneme_alphabet_id`, `phoneme_convert`, `lex_apply_ssml`, `text_is_ssml` | SSML only, deferred (`ssml-deferred.md`) | unreachable with `IVONA_CERT_SSML=0`; they abort if reached |
| `pls_lexicon_parse`, `pls_apply` | PLS not decompiled yet (parser 0x24c58..0x26cb4, applier 0x23788..0x248a0) | lang.dat PLS lexicons load empty and are never applied: Amy's alias lexicons (`pls_lexicon1`, `pls_lexicon2`) are ignored |

Remove a stub when the real function is written.

## Written differently from the binary

- **`ustr_to_charset` (charset.c).**
  - UTF-8, ASCII and ISO-8859-1 behave as in the binary.
  - The binary has a hand-written encoder for each of the other ISO 8859 code pages (2, 5, 9, 14,
    15, 16). Here they use a reverse lookup in the decode tables, plus the binary's one extra
    ISO-8859-2 mapping.
  - Unmapped characters are transliterated, as in the original.
  - The two can still differ on characters a hand-written encoder special-cases.

## Still undefined

`tools/build_engine.py` prints the current list. On the plain-text path (`ssml-deferred.md`), the
SSML-only ones are `pls_*`, `phoneme_*`, `ssml_audio_apply_attrs`, `lex_apply_ssml` and
`text_is_ssml`. They still need a definition to link.

## Host tools (`tools/host/build.sh`)

Some decompiled engine code also builds natively, which makes it possible to check it against
real data without an ARM device:

- `build/host/isef_list LANG.DAT [SECTION]` lists a lang.dat's sections and prints one. The engine's
  `isef.c` is used as is, patched to OpenSSL 3 for the host.

Amy's `lang_en_gb.dat` holds `prm_lex_list` (`prm_pre_lexicon`, `prm_lexicon`,
`prm_post_lexicon`), `pls_lex_list` (`pls_lexicon1`, `pls_lexicon2`: PLS alias lexicons),
`lang_code` "en-GB", `ivona_version` 1.6.101.3 and `lex_data`. So the PLS parser and applier are
needed even for plain text.
