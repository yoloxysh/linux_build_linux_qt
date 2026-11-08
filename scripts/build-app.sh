#!/bin/bash
# Cross-compile one of the sample apps for the Zynq board.
#
# Usage: ./scripts/build-app.sh [qt_demo|qt_demo1|qt_demo5]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="${1:-qt_demo5}"
SRC="$ROOT/projects/$APP"

if [[ ! -f "$SRC/CMakeLists.txt" ]]; then
    echo "unknown project: $APP" >&2
    echo "choices: qt_demo qt_demo1 qt_demo5" >&2
    exit 1
fi

if [[ ! -x /opt/qt-5.13.2-arm/bin/qmake ]]; then
    echo "Qt prefix missing. Run: sudo ./scripts/install-toolchain.sh" >&2
    exit 1
fi

BUILD="$SRC/out/build/release"
cmake -S "$SRC" -B "$BUILD" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/toolchain/zynq-linux-armhf.cmake" \
    -DQT_DIR=/opt/qt-5.13.2-arm/lib/cmake/Qt5

cmake --build "$BUILD"
mkdir -p "$SRC/out"
cp -a "$BUILD/qt_demo/qt_demo" "$SRC/out/qt_demo"
file "$SRC/out/qt_demo"
echo "binary: $SRC/out/qt_demo"
