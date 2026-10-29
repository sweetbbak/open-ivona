/*
 * Stand-ins for voice library functions not decompiled yet, so tools/build_runnable.py can link a
 * runnable libvoice_en_gb_amy.so. Not part of the matching build: no function row credits this
 * file, and each stub goes when the real function is written. See docs/notes/runnable-voice.md.
 */

/* en_word_pos (0x3d908, 0x316c bytes): the homograph rules choosing a word's part of speech
 * from its context (read/read, content, bass, ...). Not written yet: every word gets the
 * default ("0", the lexicon's first entry). */
const char *en_word_pos(const void *word)
{
    (void)word;
    return "0";
}
