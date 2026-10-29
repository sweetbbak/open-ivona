#!/usr/bin/env bash
# Fetch the Android 4.4 (API 19) ARM emulator system image and extract its /system (bionic
# linker, libc, libm, libdl, liblog, libstdc++) into toolchain/android-19/, so that NDK-built
# programs and the IVONA .so files run unmodified under qemu-user (tools/run_android.sh).
# Idempotent. Checksum: SHA-1 published in Google's sys-img2-1.xml repository manifest.
set -euo pipefail
cd "$(dirname "$0")"
URL=https://dl.google.com/android/repository/sys-img/android/armeabi-v7a-19_r05.zip
SHA1=d1a5fd4f2e1c013c3d3d9bfe7e9db908c3ed56fa
ARCHIVE="dl/$(basename "$URL")"
OUT=android-19
mkdir -p dl
[ -f "$ARCHIVE" ] || { curl -fL -o "$ARCHIVE.part" "$URL" && mv "$ARCHIVE.part" "$ARCHIVE"; }
echo "$SHA1  $ARCHIVE" | sha1sum -c -
[ -x "$OUT/system/bin/linker" ] && { echo "$OUT ready"; exit 0; }
tmp=$(mktemp -d dl/sysimg.XXXX)
unzip -q -o "$ARCHIVE" '*/system.img' -d "$tmp"
img=$(find "$tmp" -name system.img | head -1)
# the image is ext4 (possibly sparse): rdump /bin and /lib with debugfs, no root needed
if file "$img" | grep -q "Android sparse"; then simg2img "$img" "$tmp/raw.img"; img="$tmp/raw.img"; fi
mkdir -p "$OUT/system"
debugfs -R "rdump /bin $OUT/system" "$img" >/dev/null 2>&1
debugfs -R "rdump /lib $OUT/system" "$img" >/dev/null 2>&1
rm -rf "$tmp"
chmod +x "$OUT/system/bin/linker"
echo "$OUT ready"
