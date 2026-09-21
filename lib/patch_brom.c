/*
 * patch_brom.c -- LD_PRELOAD shim for BROM mode on kernel >= 5.4
 *
 * Copyright (c) 2026 Badmaneers. All rights reserved.
 *
 * Kernel 5.4+ CDC ACM driver rejects various ioctls on /dev/ttyACM*
 * (returns EOPNOTSUPP), including TIOCGSERIAL/TIOCSSERIAL and serial
 * port setup ioctls used during open().  The precompiled BROM library
 * treats these as fatal errors.  This shim intercepts all failing
 * ioctls on /dev/ttyACM* and returns fake success so the library
 * can proceed.
 *
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <linux/serial.h>

static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;

static int is_ttyACM(int fd)
{
    char buf[256];
    char link[256] = {0};
    ssize_t len;

    snprintf(buf, sizeof(buf), "/proc/self/fd/%d", fd);
    len = readlink(buf, link, sizeof(link) - 1);
    if (len > 0)
        link[len] = '\0';
    return strstr(link, "ttyACM") != NULL;
}

int ioctl(int fd, unsigned long request, ...)
{
    va_list args;
    void *arg;
    int ret, saved_errno;

    va_start(args, request);
    arg = va_arg(args, void *);
    va_end(args);

    if (!__atomic_load_n(&real_ioctl, __ATOMIC_ACQUIRE)) {
        void (*tmp)(void) = dlsym(RTLD_NEXT, "ioctl");
        if (!tmp)
            goto pass_through;
        __atomic_store_n(&real_ioctl, (void*)tmp, __ATOMIC_RELEASE);
    }

    ret = real_ioctl(fd, request, arg);
    saved_errno = errno;

    if (ret < 0 && saved_errno == EOPNOTSUPP && is_ttyACM(fd)) {
        fprintf(stderr, "[patch_brom] ioctl 0x%lx -> EOPNOTSUPP swallowed on ttyACM\n", request);
        errno = 0;
        return 0;
    }

pass_through:
    errno = saved_errno;
    return ret;
}
