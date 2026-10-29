# SSML input: deferred (come back to this)

To get `examples/example.c` running sooner, the runnable build turns SSML input off:
`cert_parse()` under `IVONA_NO_CERT_CHECK` sets `ssml = IVONA_CERT_SSML`, which defaults to 0
(`cert.c`). With `ssml == 0`, `streamer_set_text` always takes `streamer_plain_text`. That
path wraps the text in a one-element `<speak>` DOM, so the SSML parser never runs.

**This is temporary.** Build with `-DIVONA_CERT_SSML=1` once the items below are done, and test
SSML input (the original `ssml = 1`).

## What `ssml = 0` turns off

- SSML input: `streamer_set_text` → `text_is_ssml` / `streamer_parse_ssml` / `ssml_parse`.
- Expat. About 60 `XML_*` / `FUN_0019f…`–`FUN_001ae…` functions on the parse path. Plan:
  verify them with `./dc libmatch` against Expat 2.2.0 (see the vendored table in CLAUDE.md).
- `tts_voice_add_pls` (`voice.c`) and `eng_input_type = application/ssml+xml`
  (`voice_param.c`) fail with `TTS_STATUS_CERTIFICATE_LIMIT`.
- User-lexicon rules written as `<speak>…</speak>` are skipped (`lex_apply.c`).

`synth_iter_next` still runs, because plain text is walked as a `<speak>` DOM too, but only its
`<speak>` and text-node cases.

## Still to do on the SSML path

Written and functional, not matching yet:
- `synth_iter_next` (0x2ef18, 8.9%), `ssml_select_voice` (0x2e084), `ssml_add_lexicon`,
  `ssml_audio_fetch`, `ssml_parse`, `ssml_start_element`, `ssml_end_element`,
  `ssml_chardata`, `ssml_text_expand`, `ssml_apply_dict` / `_prm`, `say_as_lookup`.

Not written. The runnable build has aborting stubs for these (`runnable_stubs.c`); remove each stub
when the function is written:
- `phoneme_alphabet_id` (0x41ae8), `phoneme_convert` (0x41cf4) and its helpers `FUN_000417e0` /
  `FUN_00041540` (phoneme.c).
- `ssml_audio_apply_attrs` (0x27920): clipBegin / clipEnd / soundLevel / speed / repeatCount
  on an `<audio>` wave.
- `text_is_ssml` (0x49e74).
- `lex_apply_ssml` (0x42598, lex_regex.c) and its vector free `FUN_00041edc`.

PLS (`pls_lexicon_parse`, `pls_apply`) is not SSML-only: Amy's lang.dat lists two PLS
lexicons (`pls_lex_list`, loaded by `lang_data_load`), so plain text uses it too. It is stubbed
for now (empty lexicons, no change to the text, see `runnable-engine.md`) and is next to do.
- Expat (see above).
