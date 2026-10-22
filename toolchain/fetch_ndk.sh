#!/usr/bin/env bash
# Fetch an Android NDK's GCC 4.8 toolchain (the compiler family that built the IVONA .so files)
# and build its sealed podman image localhost/ivona-ndk:<ver>. Idempotent.
# Usage: toolchain/fetch_ndk.sh [r10e|r9d]   (default r10e)
set -euo pipefail
cd "$(dirname "$0")"
VER="${1:-r10e}"
case "$VER" in
  r10e) URL=https://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip
        SHA256=ee5f405f3b57c4f5c3b3b8b5d495ae12b660e03d2112e4ed5c728d349f1e520c ;;
  r9d)  URL=https://dl.google.com/android/ndk/android-ndk-r9d-linux-x86_64.tar.bz2
        SHA256=14e1d183a85caca5e08e5e56e5a5beaa2e3f539755eb7a693efdb0e16ce6b19a ;;
  *) echo "unknown NDK $VER" >&2; exit 1 ;;
esac
ARCHIVE="dl/$(basename "$URL")"
DIR="ndk-$VER"
NDK="$DIR/android-ndk-$VER"
IMAGE="localhost/ivona-ndk:$VER"

mkdir -p dl "$DIR"
[ -f "$ARCHIVE" ] || { curl -fL -o "$ARCHIVE.part" "$URL" && mv "$ARCHIVE.part" "$ARCHIVE"; }
echo "$SHA256  $ARCHIVE" | sha256sum -c -
# toolchain + sysroot + GNU libstdc++ 4.8 (headers and the prebuilt static libs, used both to
# compile C++ TUs and to verify statically linked library code with `dc libmatch`)
PARTS="toolchains/arm-linux-androideabi-4.8 platforms/android-9/arch-arm sources/cxx-stl/gnu-libstdc++/4.8"
if [ ! -d "$NDK/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi" ]; then
    case "$ARCHIVE" in
      *.zip) unzip -q -o "$ARCHIVE" $(for p in $PARTS; do echo "android-ndk-$VER/$p/*"; done) -d "$DIR" ;;
      *) tar -xjf "$ARCHIVE" -C "$DIR" $(for p in $PARTS; do echo "android-ndk-$VER/$p"; done) ;;
    esac
fi
podman image exists "$IMAGE" || podman build --build-arg NDK="$NDK" -t "$IMAGE" -f Containerfile .
podman run --rm --network=none "$IMAGE" arm-linux-androideabi-gcc --version | head -1
