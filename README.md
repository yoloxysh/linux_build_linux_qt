# Zynq Qt 5.13.2 交叉编译链

在 x86_64 Linux 上交叉编译 ARM hard-float 程序，目标板是 Zynq，glibc 2.30，画面走 linuxfb。Qt 前缀和 sysroot 都放在 `/opt`，路径写死在 qmake 和 CMake 工具链里，部署时保持这两个路径。

| 路径 | 内容 |
| --- | --- |
| `/opt/qt-5.13.2-arm` | 交叉编译好的 Qt 5.13.2（Widgets、linuxfb，无 OpenGL / X11 / OpenSSL） |
| `/opt/zynq-sysroot` | 板子 rootfs 抽出的 sysroot（libc 2.30、libstdc++） |
| `/usr/bin/arm-linux-gnueabihf-g++` | Ubuntu GCC 11 交叉编译器 |

下次换机器部署，按 [DEPLOY.md](DEPLOY.md) 做。

## 另一台 Linux 上部署

需要 root，磁盘大约 400 MB（解压后）。交叉编译器从 apt 安装，所以目标机要能访问 Ubuntu 软件源，并且是较新的 Ubuntu（这套包名是 `gcc-11-arm-linux-gnueabihf`）。

```bash
git clone https://github.com/yoloxysh/linux_build_linux_qt.git
cd linux_build_linux_qt
sudo ./scripts/install-toolchain.sh
./scripts/build-app.sh qt_demo5
```

`install-toolchain.sh` 会：

1. 安装 `gcc-11-arm-linux-gnueabihf`、`g++-11-arm-linux-gnueabihf`、`cmake`、`ninja-build`
2. 把 `toolchain/zynq-sysroot.tgz.part-*` 拼回 `zynq-sysroot.tgz`，再解到 `/opt/zynq-sysroot`
3. 把 `toolchain/qt-5.13.2-arm.tgz` 解到 `/opt/qt-5.13.2-arm`
4. 用 sysroot 里的 libc 重新编译 `libglibc-compat.a`

编出来的程序是 ELF 32-bit ARM，动态链接 `/lib/ld-linux-armhf.so.3`。拷到板子上跑之前，板子的 `/usr/lib` 里要有对应的 `libQt5Core.so.5`、`libQt5Gui.so.5`、`libQt5Widgets.so.5`。这些库在 `/opt/qt-5.13.2-arm/lib`。

## 为什么必须用 GCC 11

板子 glibc 是 2.30。Ubuntu 默认的 `gcc-arm-linux-gnueabihf` 在新版本上是 GCC 15，它的启动文件依赖 `GLIBC_2.34`，链接出来的程序在板上起不来。所以这里固定 GCC 11，并加了 `-fno-pie -no-pie`。

GCC 11 还会生成两类板子上没有的符号，链接时放在 Qt 库之后，用 `--whole-archive` 拉进来：

- `stat64`、`__isoc23_*`：`toolchain/glibc-compat/glibc-compat.c` 转回 glibc 2.30 的 `__xstat64` 等接口
- `std::__throw_bad_array_new_length`：`toolchain/stdcxx-compat/stdcxx-compat.cpp` 转到板子 libstdc++ 里的 `__cxa_throw_bad_array_new_length`

## 自己改工程

源码在 `projects/`：

- `qt_demo`、`qt_demo1`：Qt Creator 生成的最小窗口
- `qt_demo5`：带波形控件的当前工程

新工程沿用同样的工具链文件：

```bash
cmake -S my_app -B build \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain/zynq-linux-armhf.cmake \
  -DQT_DIR=/opt/qt-5.13.2-arm/lib/cmake/Qt5
cmake --build build
```

## 仓库里有什么

```
toolchain/qt-5.13.2-arm.tgz          编好的 Qt，约 30 MB 压缩
toolchain/zynq-sysroot.tgz.part-*   板子 sysroot，拆成 45 MB 分片（GitHub 单文件上限 100 MB）
toolchain/zynq-linux-armhf.cmake     CMake 交叉工具链
toolchain/mkspecs/.../qmake.conf     针对 glibc 2.30 改过的 mkspec
toolchain/glibc-compat/glibc-compat.c
toolchain/stdcxx-compat/stdcxx-compat.cpp
toolchain/qt-configure-options.txt   当初 configure Qt 的参数
scripts/install-toolchain.sh
scripts/build-app.sh
scripts/build-glibc-compat.sh
projects/qt_demo qt_demo1 qt_demo5
```

Qt 源码树（`/opt/src`，约 2.9 GB）和 Qt 构建目录（`/opt/qt-build-5.13.2`）没有放进来。日常交叉编译用上面的前缀就够了。需要重编 Qt 时，再下载 [qt-everywhere-src-5.13.2](https://download.qt.io/archive/qt/5.13/5.13.2/single/qt-everywhere-src-5.13.2.tar.xz)，把 `toolchain/mkspecs/linux-arm-gnueabihf-g++/qmake.conf` 覆盖进源码的 `qtbase/mkspecs/linux-arm-gnueabihf-g++/`，然后按 `qt-configure-options.txt` 配置。

`projects/qt_demo1/fonts/wqy-microhei.ttc` 是文泉驿微米黑，板上缺中文字体时可以拷到板子的字体目录。
