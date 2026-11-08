#define _GNU_SOURCE
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

extern int __xstat64(int ver, const char *path, struct stat64 *buf);
extern int __lxstat64(int ver, const char *path, struct stat64 *buf);
extern int __fxstat64(int ver, int fd, struct stat64 *buf);
extern int __fxstatat64(int ver, int dirfd, const char *path, struct stat64 *buf, int flags);

int stat64(const char *path, struct stat64 *buf) { return __xstat64(0, path, buf); }
int lstat64(const char *path, struct stat64 *buf) { return __lxstat64(0, path, buf); }
int fstat64(int fd, struct stat64 *buf) { return __fxstat64(0, fd, buf); }
int fstatat64(int dirfd, const char *path, struct stat64 *buf, int flags) {
    return __fxstatat64(0, dirfd, path, buf, flags);
}

long __isoc23_strtol(const char *nptr, char **endptr, int base) {
    return strtol(nptr, endptr, base);
}
unsigned long __isoc23_strtoul(const char *nptr, char **endptr, int base) {
    return strtoul(nptr, endptr, base);
}
long long __isoc23_strtoll(const char *nptr, char **endptr, int base) {
    return strtoll(nptr, endptr, base);
}
unsigned long long __isoc23_strtoull(const char *nptr, char **endptr, int base) {
    return strtoull(nptr, endptr, base);
}
intmax_t __isoc23_strtoimax(const char *nptr, char **endptr, int base) {
    return strtoimax(nptr, endptr, base);
}
uintmax_t __isoc23_strtoumax(const char *nptr, char **endptr, int base) {
    return strtoumax(nptr, endptr, base);
}
int __isoc23_sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}
int __isoc23_vsscanf(const char *s, const char *fmt, va_list ap) {
    return vsscanf(s, fmt, ap);
}
