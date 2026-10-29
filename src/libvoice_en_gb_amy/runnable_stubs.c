/*
 * Stand-ins for voice library functions not decompiled yet, so tools/build_runnable.py can link a
 * runnable libvoice_en_gb_amy.so. Not part of the matching build: no function row credits this
 * file, and each stub goes when the real function is written. See docs/notes/runnable-voice.md.
 * Bigger functions are lifted from the binary instead (tools/asm2c.py, "lifted" in
 * tools/build_runnable.py): en_word_pos.
 */

/* en_token_say_as (0x4b09c): SSML <say-as interpret-as=...> tokens, dispatched through tables of
 * handlers (characters, cardinal, ordinal, digits, date, time, fraction, unit, telephone,
 * address, radiostation, flight-code). Only SSML sets "interpret-as", and SSML input is off
 * (docs/notes/ssml-deferred.md), so the original returns 0 here too; done with SSML. */
void *en_token_say_as(void *tok, const char *name)
{
    (void)tok;
    (void)name;
    return 0;
}
