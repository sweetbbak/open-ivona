#!/usr/bin/env bash
# Reference sources for third-party code embedded in the engine (read-only references, not built).
# CMU Flite 1.4: the engine embeds a modified Flite (cst_val, cst_features, cst_alloc, ...).
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p dl flite-1.4
F=dl/flite-1.4-release.tar.bz2
[ -f $F ] || curl -fL -o $F http://www.festvox.org/flite/packed/flite-1.4/flite-1.4-release.tar.bz2
echo "45c662160aeca6560589f78daf42ab62c6111dd4d244afc28118c4e6f553cd0c  $F" | sha256sum -c -
[ -d flite-1.4/src ] || tar -xjf $F -C flite-1.4 --strip-components=1 \
    flite-1.4-release/src flite-1.4-release/include flite-1.4-release/lang

# OpenSSL 1.0.2h (libcrypto is statically linked; checksum from openssl.org's .sha256) and
# libcurl 7.52.1 (checksum pinned at first download). Built by `dc vendor build <lib>`.
fetch() {  # url sha256 dir [file name, default: the URL basename]
    local f=dl/${4:-$(basename "$1")}
    [ -f "$f" ] || curl -fL -o "$f" "$1"
    echo "$2  $f" | sha256sum -c -
    [ -d "$3" ] || { mkdir -p "$3" && tar -xf "$f" -C "$3" --strip-components=1; }
}
fetch https://www.openssl.org/source/old/1.0.2/openssl-1.0.2h.tar.gz \
    1d4007e53aad94a5b2002fe045ee7bb0b3d98f1a47f8b2bc851dcd1c74332919 openssl-1.0.2h
fetch https://curl.se/download/curl-7.52.1.tar.bz2 \
    d16185a767cb2c1ba3d5b9096ec54e5ec198b213f45864a38b3bda4bbf87389b curl-7.52.1
# PCRE 8.37 (checksum pinned at first download from SourceForge)
fetch https://sourceforge.net/projects/pcre/files/pcre/8.37/pcre-8.37.tar.bz2/download \
    51679ea8006ce31379fb0860e46dd86665d864b5020fc9cd19e71260eef4789d pcre-8.37 pcre-8.37.tar.bz2
# Expat 2.2.0 (GitHub release asset; checksum as published)
fetch https://github.com/libexpat/libexpat/releases/download/R_2_2_0/expat-2.2.0.tar.bz2 \
    d9e50ff2d19b3538bd2127902a89987474e1a4db8e43a66a4d1a712ab9a504ff expat-2.2.0
# cmp v16 (camgunz/cmp MessagePack, ~0x1f1200-0x1f9000 in the engine; GitHub tag archive,
# checksum pinned at first download)
fetch https://github.com/camgunz/cmp/archive/refs/tags/v16.tar.gz \
    2a2026b81c2db43eb27fb66155d1c5f90153e01c537ac0f30535cfb672e06144 cmp-16 cmp-v16.tar.gz
# SILK SDK 1.0.8, fixed-point C (Skype never published it on a site of its own): a copy inside
# fingi/csipsimple, pinned by commit and verified by the git tree hash of the SDK directory.
silk() {  # commit path tree dir
    [ -d "$4" ] && return
    local t; t=$(mktemp -d dl/silk.XXXX)
    git clone -q --filter=blob:none --no-checkout https://github.com/fingi/csipsimple.git "$t"
    git -C "$t" sparse-checkout set "$2"
    git -C "$t" checkout -q "$1"
    [ "$(git -C "$t" rev-parse "$1:$2")" = "$3" ] || { echo "silk: tree hash mismatch" >&2; exit 1; }
    cp -r "$t/$2" "$4"
    rm -rf "$t"
    echo "$4: tree $3 OK"
}
silk a160f718173d79fc9a370386d2df1ad9eb5511e2 jni/silk/sources/SILK_SDK_SRC_FIX_v1.0.8 \
    1c2a30b25b3a32f0c61b51a5b2a438f51c5f2a67 silk-sdk-fix-1.0.8
