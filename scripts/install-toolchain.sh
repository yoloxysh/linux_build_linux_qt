#!/bin/bash
# Unpack the prebuilt Qt 5.13.2 ARM prefix and the Zynq sysroot into /opt.
# Then install the GCC 11 armhf cross compiler from apt.
#
# Usage: sudo ./scripts/install-toolchain.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QT_TGZ="$ROOT/toolchain/qt-5.13.2-arm.tgz"
SYSROOT_TGZ="$ROOT/toolchain/zynq-sysroot.tgz"
SYSROOT_PARTS=("$ROOT"/toolchain/zynq-sysroot.tgz.part-*)

if [[ "$(id -u)" -ne 0 ]]; then
    echo "run as root: sudo $0" >&2
    exit 1
fi

if [[ ! -f "$QT_TGZ" ]]; then
    echo "missing $QT_TGZ" >&2
    exit 1
fi

if [[ ! -f "$SYSROOT_TGZ" ]]; then
    if [[ ! -f "${SYSROOT_PARTS[0]}" ]]; then
        echo "missing $SYSROOT_TGZ and its part files" >&2
        exit 1
    fi
    echo "joining sysroot parts"
    cat "${SYSROOT_PARTS[@]}" > "$SYSROOT_TGZ"
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y \
    cmake \
    ninja-build \
    pkg-config \
    file \
    build-essential \
    python3 \
    perl \
    gcc-11-arm-linux-gnueabihf \
    g++-11-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf

update-alternatives --install /usr/bin/arm-linux-gnueabihf-gcc arm-linux-gnueabihf-gcc /usr/bin/arm-linux-gnueabihf-gcc-11 110
update-alternatives --install /usr/bin/arm-linux-gnueabihf-g++ arm-linux-gnueabihf-g++ /usr/bin/arm-linux-gnueabihf-g++-11 110

mkdir -p /opt
tar -xzf "$QT_TGZ" -C /opt
tar -xzf "$SYSROOT_TGZ" -C /opt

# Keep the customized mkspec even if the archive is from an older build.
cp -a "$ROOT/toolchain/mkspecs/linux-arm-gnueabihf-g++/qmake.conf" \
    /opt/qt-5.13.2-arm/mkspecs/linux-arm-gnueabihf-g++/qmake.conf

bash "$ROOT/scripts/build-glibc-compat.sh"

echo
echo "installed:"
echo "  /opt/qt-5.13.2-arm"
echo "  /opt/zynq-sysroot"
arm-linux-gnueabihf-g++ --version | head -1
/opt/qt-5.13.2-arm/bin/qmake -query QT_VERSION
/opt/qt-5.13.2-arm/bin/qmake -query QT_SYSROOT
