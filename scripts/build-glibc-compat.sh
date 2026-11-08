#!/bin/bash
# Build libglibc-compat.a against the board sysroot and install it.
# GCC 11 emits stat64 / __isoc23_* symbols that glibc 2.30 does not have.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SYSROOT="${SYSROOT:-/opt/zynq-sysroot}"
SRC="$ROOT/toolchain/glibc-compat/glibc-compat.c"
OUT="${SYSROOT}/usr/lib/libglibc-compat.a"

if [[ ! -d "$SYSROOT" ]]; then
    echo "sysroot not found: $SYSROOT" >&2
    exit 1
fi

arm-linux-gnueabihf-gcc \
    --sysroot="$SYSROOT" \
    -march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb \
    -fno-pie -O2 -c "$SRC" -o /tmp/glibc-compat.o

rm -f "$OUT"
arm-linux-gnueabihf-ar rcs "$OUT" /tmp/glibc-compat.o
echo "installed $OUT"

STDCXX_SRC="$ROOT/toolchain/stdcxx-compat/stdcxx-compat.cpp"
STDCXX_OUT="${SYSROOT}/usr/lib/libstdcxx-compat.a"
arm-linux-gnueabihf-g++ \
    --sysroot="$SYSROOT" \
    -march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb \
    -fno-pie -O2 -c "$STDCXX_SRC" -o /tmp/libstdcxx-compat.o
rm -f "$STDCXX_OUT"
arm-linux-gnueabihf-ar rcs "$STDCXX_OUT" /tmp/libstdcxx-compat.o
echo "installed $STDCXX_OUT"
