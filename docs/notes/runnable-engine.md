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

## Stubs (`src/libttsivonavoice/runnable_stubs.c`)

| function | why it's stubbed | effect |
|---|---|---|
| `vorbis_unit_*`, `vorbis_ogg_unit_*` | Tremor deferred (vox codecs 8/12, Windows voices only) | opening such a voice fails; Android voices use SILK (14) |
| `ssml_audio_apply_attrs`, `phoneme_alphabet_id`, `phoneme_convert`, `lex_apply_ssml`, `text_is_ssml` | SSML only, deferred (`ssml-deferred.md`) | unreachable with `IVONA_CERT_SSML=0`; they abort if reached |

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
