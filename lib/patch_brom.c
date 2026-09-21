/*
 * patch_brom.c -- LD_PRELOAD shim for BROM mode on kernel >= 5.4
 *
 * Kernel 5.4+ CDC ACM driver rejects TIOCGSERIAL / TIOCSSERIAL ioctls
 * (returns EOPNOTSUPP).  The precompiled BROM library treats this as a
 * fatal error.  This shim intercepts the failing ioctls on /dev/ttyACM*
 * and returns fake success so the library can proceed.
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

    if (!__atomic_load_n(&real_ioctl, __ATOMIC_ACQUIRE))
        __atomic_store_n(&real_ioctl, dlsym(RTLD_NEXT, "ioctl"), __ATOMIC_RELEASE);

    ret = real_ioctl(fd, request, arg);
    saved_errno = errno;

    if (ret < 0 && saved_errno == EOPNOTSUPP && is_ttyACM(fd)) {
        switch (request) {
        case TIOCGSERIAL: {
            struct serial_struct *ss = (struct serial_struct *)arg;
            if (ss) {
                memset(ss, 0, sizeof(*ss));
                ss->type      = PORT_UNKNOWN;
                ss->line      = 0;
                ss->port      = 0;
                ss->irq       = 0;
                ss->flags     = ASYNC_SKIP_TEST | ASYNC_LOW_LATENCY;
                ss->baud_base = 115200;
            }
            fprintf(stderr, "[patch_brom] TIOCGSERIAL -> faked OK\n");
            errno = 0;
            return 0;
        }
        case TIOCSSERIAL:
            fprintf(stderr, "[patch_brom] TIOCSSERIAL -> faked OK\n");
            errno = 0;
            return 0;
        default:
            break;
        }
    }

    errno = saved_errno;
    return ret;
}
