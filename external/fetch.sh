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
