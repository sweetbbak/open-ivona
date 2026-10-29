#!/usr/bin/env bash
# Builds host (x86-64) tools from the decompiled engine sources, for checking decompiled code
# against real voice data without an ARM device. Output: build/host/.
#   isef_list LANG.DAT [SECTION]   lists the sections of a lang.dat (ISEF), prints one
# isef.c is written for OpenSSL 1.0.2 (open structs); the host copy is patched to OpenSSL 3's API.
set -euo pipefail
cd "$(dirname "$0")/../.."
out=build/host
mkdir -p "$out"
python3 - "$out/isef_host.c" <<'PY'
import sys
s = open('src/libttsivonavoice/isef.c').read()
s = s.replace("    EVP_CIPHER_CTX ctx;", "    EVP_CIPHER_CTX *ctx;")
s = s.replace("EVP_CIPHER_CTX_cleanup(&r->ctx);", "EVP_CIPHER_CTX_reset(r->ctx);")
s = s.replace("EVP_CIPHER_CTX_init(&r->ctx);", "r->ctx = EVP_CIPHER_CTX_new();")
s = s.replace("&r->ctx", "r->ctx")
s = s.replace("""    r->rsa->n = BN_bin2bn(isef_key, 0x200, NULL);
    r->rsa->e = BN_bin2bn(isef_key + 0x200, 4, NULL);
    if (r->rsa->n == NULL || r->rsa->e == NULL) {""",
              """    if (!RSA_set0_key(r->rsa, BN_bin2bn(isef_key, 0x200, NULL),
                      BN_bin2bn(isef_key + 0x200, 4, NULL), NULL)) {""")
open(sys.argv[1], 'w').write(s)
PY
src=src/libttsivonavoice
# unresolved symbols are only reached by code these tools never run
gcc -w -no-pie -I include -I include/flite -o "$out/isef_list" tools/host/isef_list.c \
    "$out/isef_host.c" tools/host/stubs.c $src/textstream.c $src/flite/cst_alloc.c $src/vector.c \
    $src/charset_dec.c $src/charset_tables.c $src/flite/cst_string.c $src/strutil.c \
    $src/flite/cst_mmap_posix.c -lcrypto -Wl,--unresolved-symbols=ignore-all
echo "built $out/isef_list"
