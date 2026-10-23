/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  July 1999                                        */
/*************************************************************************/
/*                                                                       */
/*  Tokenizer for strings and files                                      */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_TOKENSTREAM_H__
#define _CST_TOKENSTREAM_H__

#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_file.h"

/* IVONA: characters are read through per-encoding callbacks (ASCII or UTF-8), a pushed-back
 * string is read before the input, and tokens may be length limited. */
typedef struct  cst_tokenstream_struct {
    cst_file fd;
    int file_pos;
    int line_number;
    char *string_buffer;
    char *unget;                /* +0x10: pushed back characters (ts_unget) */
    int unget_len;
    int unget_pos;
    int current_char;           /* +0x1c */

    int token_pos;              /* +0x20 */
    int ws_max;
    char *whitespace;
    int prep_max;
    char *prepunctuation; /* +0x30 */
    int max_token_len;          /* +0x34: in characters, -1 for none */
    int token_max;
    char *token;
    int postp_max;              /* +0x40 */
    char *postpunctuation;
    int tail_max;
    char *tail;           /* +0x4c: what followed the token up to white space */

    const char *p_whitespacesymbols;      /* +0x50 */
    const char *p_singlecharsymbols;
    const char *p_prepunctuationsymbols;
    const char *p_postpunctuationsymbols;

    int (*charlen)(int c);                              /* +0x60: bytes of c, 0 for EOF */
    const char *(*member)(const char *set, int c);      /* +0x64 */
    size_t (*span)(const char *s, const char *set);     /* +0x68 */
    int (*decode)(const char *s, int *len);             /* +0x6c */
    int (*encode)(int c, char *out);                    /* +0x70 */
} cst_tokenstream;

/* Public functions for tokenstream manipulation */
cst_tokenstream *ts_open_fd(cst_file fd);
cst_tokenstream *ts_open(const char *filename);
cst_tokenstream *ts_open_string(const char *string);
cst_tokenstream *ts_open_string_utf8(const char *string);
void ts_close(cst_tokenstream *ts);

int ts_eof(cst_tokenstream *ts);
const char *ts_get(cst_tokenstream *ts);

#endif
