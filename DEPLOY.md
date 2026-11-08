# 下次部署

在一台新的 x86_64 Linux 上装好这套链，就能交叉编译给 Zynq 板子跑的 Qt 程序。编出来的程序是 32 位 ARM hard-float，动态链接 `/lib/ld-linux-armhf.so.3`，画面走 linuxfb。编译机上不能直接运行。

仓库：https://github.com/yoloxysh/linux_build_linux_qt

## 新机器要满足什么

- x86_64 Ubuntu，能 `sudo`，能访问 Ubuntu 软件源
- 软件源里要有 `gcc-11-arm-linux-gnueabihf` 和 `g++-11-arm-linux-gnueabihf`
- `/opt` 下至少留 400 MB。解压后 Qt 前缀约 90 MB，sysroot 约 270 MB
- 不要改下面两个路径。qmake、CMake 工具链和链接脚本都写死了它们：

| 路径 | 内容 |
| --- | --- |
| `/opt/qt-5.13.2-arm` | Qt 5.13.2，Widgets + linuxfb |
| `/opt/zynq-sysroot` | 板子 rootfs，glibc 2.30 |

板子 glibc 是 2.30。新 Ubuntu 默认的交叉编译器是 GCC 15，启动文件依赖 `GLIBC_2.34`，编出来的程序在板上起不来。安装脚本固定 GCC 11。

## 第一次安装

```bash
git clone https://github.com/yoloxysh/linux_build_linux_qt.git
cd linux_build_linux_qt
sudo ./scripts/install-toolchain.sh
```

脚本只需要跑一次。它会：

1. 用 apt 安装 `gcc-11-arm-linux-gnueabihf`、`g++-11-arm-linux-gnueabihf`、`binutils-arm-linux-gnueabihf`、`cmake`、`ninja-build`
2. 把 `arm-linux-gnueabihf-gcc` / `g++` 指到 GCC 11
3. 把 `toolchain/zynq-sysroot.tgz.part-00`、`part-01`、`part-02` 拼成 `zynq-sysroot.tgz`，解到 `/opt/zynq-sysroot`
4. 把 `toolchain/qt-5.13.2-arm.tgz` 解到 `/opt/qt-5.13.2-arm`
5. 覆盖自定义的 `qmake.conf`
6. 用板子 libc 重编 `libglibc-compat.a` 和 `libstdcxx-compat.a`，放到 `/opt/zynq-sysroot/usr/lib`

成功时最后几行类似：

```text
installed:
  /opt/qt-5.13.2-arm
  /opt/zynq-sysroot
arm-linux-gnueabihf-g++ (Ubuntu 11.x.x) 11.x.x
QT_VERSION:5.13.2
QT_SYSROOT:/opt/zynq-sysroot
```

确认编译器确实是 11，不是 15：

```bash
arm-linux-gnueabihf-g++ --version
```

## 编译示例

```bash
./scripts/build-app.sh qt_demo5
```

可选工程：

| 命令 | 工程 |
| --- | --- |
| `./scripts/build-app.sh qt_demo` | 最小窗口 |
| `./scripts/build-app.sh qt_demo1` | 最小窗口，附文泉驿微米黑 |
| `./scripts/build-app.sh qt_demo5` | 带波形控件的当前工程 |

产物在 `projects/<工程名>/out/qt_demo`。检查：

```bash
file projects/qt_demo5/out/qt_demo
```

应看到 `ELF 32-bit LSB executable, ARM, EABI5`，解释器是 `/lib/ld-linux-armhf.so.3`。

同一台机器以后改了源码，直接再跑 `./scripts/build-app.sh`，不用重装工具链。

## 拷到板子上

程序动态链接这三个库，板子的库目录里要有它们：

- `libQt5Core.so.5`
- `libQt5Gui.so.5`
- `libQt5Widgets.so.5`

文件在编译机的 `/opt/qt-5.13.2-arm/lib`。linuxfb 插件在 `/opt/qt-5.13.2-arm/plugins/platforms/libqlinuxfb.so`。中文缺字时，把 `projects/qt_demo1/fonts/wqy-microhei.ttc` 拷到板子字体目录。

板上启动时指定 linuxfb，例如：

```bash
export QT_QPA_PLATFORM=linuxfb
./qt_demo
```

## 自己的工程

CMake 工程沿用同一个工具链文件。可执行目标名保持 `qt_demo` 时，`build-app.sh` 也能直接编；名字不同就自己调用 cmake：

```bash
cmake -S my_app -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain/zynq-linux-armhf.cmake \
  -DQT_DIR=/opt/qt-5.13.2-arm/lib/cmake/Qt5
cmake --build build
```

工具链已经带上这些约束，不要另配一套编译参数：

- `-march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb`
- `-fno-pie -no-pie`，避开 GCC 11 默认 PIE 对 `GLIBC_2.34` 的依赖
- 兼容库放在 Qt 库之后：`--whole-archive -lglibc-compat -lstdcxx-compat`

`libglibc-compat` 补 `stat64` 和 `__isoc23_*`。`libstdcxx-compat` 补 `std::__throw_bad_array_new_length`。这两份库由 `scripts/build-glibc-compat.sh` 生成，安装脚本已经调用过。sysroot 被换掉后，再跑一次：

```bash
sudo ./scripts/build-glibc-compat.sh
```

## 装失败时看哪里

`apt-get install gcc-11-arm-linux-gnueabihf` 找不到包：这台机器的 Ubuntu 源没有 GCC 11 交叉包。换一台有这个包的 Ubuntu，或者先把这两个包装上再跑脚本。不要改用默认的 `gcc-arm-linux-gnueabihf`，那会落到 GCC 15。

`/opt/qt-5.13.2-arm` 或 `/opt/zynq-sysroot` 已经有旧目录：脚本会往这两个目录上解压。要重装时先移走旧目录，再跑安装脚本。

链接报 `GLIBC_2.34`、`GLIBC_2.38`、`stat64` 或 `__isoc23_strtol` 找不到：编译器不是 GCC 11，或者没走 `toolchain/zynq-linux-armhf.cmake`。用 `arm-linux-gnueabihf-g++ --version` 确认，并带上工具链文件重新配置，不要复用旧的 CMake 缓存。

链接报 `std::__throw_bad_array_new_length` 找不到：兼容库没装进 sysroot。跑 `sudo ./scripts/build-glibc-compat.sh`，确认 `/opt/zynq-sysroot/usr/lib/libstdcxx-compat.a` 存在。

## 这次没有放进仓库的东西

日常交叉编译用不到：

- Qt 5.13.2 源码树，约 2.9 GB
- Qt 构建目录
- 各工程的 `build/`、`.vs/` 和已经编好的 ARM 可执行文件

要重编 Qt 时，下载 [qt-everywhere-src-5.13.2](https://download.qt.io/archive/qt/5.13/5.13.2/single/qt-everywhere-src-5.13.2.tar.xz)，把 `toolchain/mkspecs/linux-arm-gnueabihf-g++/qmake.conf` 覆盖进源码的 `qtbase/mkspecs/linux-arm-gnueabihf-g++/`，再按 `toolchain/qt-configure-options.txt` 配置。prefix 和 sysroot 仍是 `/opt/qt-5.13.2-arm`、`/opt/zynq-sysroot`。

sysroot 压缩包有 111 MB，超过 GitHub 单文件 100 MB 限制，所以仓库里是三个 45 MB 分片。完整的 `toolchain/zynq-sysroot.tgz` 被 git 忽略，只在安装时拼出来。
