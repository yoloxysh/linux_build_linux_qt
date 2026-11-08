# Cross toolchain for Zynq boards (armhf, glibc 2.30, linuxfb Qt 5.13.2).
# Install paths are fixed: /opt/qt-5.13.2-arm and /opt/zynq-sysroot.
# The compiler is Ubuntu's GCC 11 cross package, not the default GCC 15.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)

set(CMAKE_SYSROOT /opt/zynq-sysroot)
set(CMAKE_PREFIX_PATH /opt/qt-5.13.2-arm)
set(CMAKE_FIND_ROOT_PATH /opt/qt-5.13.2-arm /opt/zynq-sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Board glibc is 2.30. GCC 11's default PIE startup needs GLIBC_2.34.
# Compat archives must come after libQt5*.so, so they live in
# CMAKE_CXX_STANDARD_LIBRARIES, not in the early linker flags.
set(ZYNQ_ARCH_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb")
set(ZYNQ_COMPAT_FLAGS "-fno-pie -U_TIME_BITS -U_FILE_OFFSET_BITS -D_FILE_OFFSET_BITS=64")

set(CMAKE_C_FLAGS_INIT "${ZYNQ_ARCH_FLAGS} ${ZYNQ_COMPAT_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${ZYNQ_ARCH_FLAGS} ${ZYNQ_COMPAT_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-no-pie -Wl,--no-as-needed -Wl,-rpath-link,/opt/zynq-sysroot/lib:/opt/zynq-sysroot/usr/lib -L/opt/zynq-sysroot/usr/lib -L/opt/zynq-sysroot/lib -Wl,--allow-multiple-definition")
set(CMAKE_SHARED_LINKER_FLAGS_INIT
    "-Wl,--no-as-needed -Wl,-rpath-link,/opt/zynq-sysroot/lib:/opt/zynq-sysroot/usr/lib -L/opt/zynq-sysroot/usr/lib -L/opt/zynq-sysroot/lib -Wl,--allow-multiple-definition")
set(CMAKE_CXX_STANDARD_LIBRARIES
    "-Wl,--whole-archive -lglibc-compat -lstdcxx-compat -Wl,--no-whole-archive")
