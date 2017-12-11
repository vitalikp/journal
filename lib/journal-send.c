/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2011 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <printf.h>

#include "journal.h"
#include "util.h"


#define SNDBUF_SIZE (8*1024*1024)

/* We open a single fd, and we'll share it with the current process,
 * all its threads, and all its subprocesses. This means we need to
 * initialize it atomically, and need to operate on it atomically
 * never assuming we are the only user */

static int journal_fd(void) {
        int fd;
        static int fd_plus_one = 0;

retry:
        if (fd_plus_one > 0)
                return fd_plus_one - 1;

        fd = socket(AF_UNIX, SOCK_DGRAM|SOCK_CLOEXEC, 0);
        if (fd < 0)
                return -errno;

        fd_inc_sndbuf(fd, SNDBUF_SIZE);

        if (!__sync_bool_compare_and_swap(&fd_plus_one, 0, fd+1)) {
                safe_close(fd);
                goto retry;
        }

        return fd;
}

_public_ int sd_journal_print(int priority, const char *format, ...) {
        int r;
        va_list ap;

        va_start(ap, format);
        r = sd_journal_printv(priority, format, ap);
        va_end(ap);

        return r;
}

_public_ int sd_journal_printv(int priority, const char *format, va_list ap) {

        /* FIXME: Instead of limiting things to LINE_MAX we could do a
           C99 variable-length array on the stack here in a loop. */

        char buffer[8 + LINE_MAX], p[11]; struct iovec iov[2];

        assert_return(priority >= 0, -EINVAL);
        assert_return(priority <= 7, -EINVAL);
        assert_return(format, -EINVAL);

        snprintf(p, sizeof(p), "PRIORITY=%i", priority & LOG_PRIMASK);
        char_array_0(p);

        memcpy(buffer, "MESSAGE=", 8);
        vsnprintf(buffer+8, sizeof(buffer) - 8, format, ap);
        char_array_0(buffer);

        zero(iov);
        IOVEC_SET_STRING(iov[0], buffer);
        IOVEC_SET_STRING(iov[1], p);

        return sd_journal_sendv(iov, 2);
}

_printf_(1, 0) static int fill_iovec_sprintf(const char *format, va_list ap, int extra, struct iovec **_iov) {
        PROTECT_ERRNO;
        int r, n = 0, i = 0, j;
        struct iovec *iov = NULL;

        assert(_iov);

        if (extra > 0) {
                n = MAX(extra * 2, extra + 4);
                iov = malloc0(n * sizeof(struct iovec));
                if (!iov) {
                        r = -ENOMEM;
                        goto fail;
                }

                i = extra;
        }

        while (format) {
                struct iovec *c;
                char *buffer;
                va_list aq;

                if (i >= n) {
                        n = MAX(i*2, 4);
                        c = realloc(iov, n * sizeof(struct iovec));
                        if (!c) {
                                r = -ENOMEM;
                                goto fail;
                        }

                        iov = c;
                }

                va_copy(aq, ap);
                if (vasprintf(&buffer, format, aq) < 0) {
                        va_end(aq);
                        r = -ENOMEM;
                        goto fail;
                }
                va_end(aq);

                VA_FORMAT_ADVANCE(format, ap);

                IOVEC_SET_STRING(iov[i++], buffer);

                format = va_arg(ap, char *);
        }

        *_iov = iov;

        return i;

fail:
        for (j = 0; j < i; j++)
                free(iov[j].iov_base);

        free(iov);

        return r;
}

_public_ int sd_journal_send(const char *format, ...) {
        int r, i, j;
        va_list ap;
        struct iovec *iov = NULL;

        va_start(ap, format);
        i = fill_iovec_sprintf(format, ap, 0, &iov);
        va_end(ap);

        if (_unlikely_(i < 0)) {
                r = i;
                goto finish;
        }

        r = sd_journal_sendv(iov, i);

finish:
        for (j = 0; j < i; j++)
                free(iov[j].iov_base);

        free(iov);

        return r;
}

_public_ int sd_journal_sendv(const struct iovec *iov, int n) {
        PROTECT_ERRNO;
        int fd;
        struct iovec *w;
        uint64_t *l;
        int i, j = 0;
        struct sockaddr_un sa = {
                .sun_family = AF_UNIX,
                .sun_path = JOURNAL_RUNDIR "/socket",
        };
        struct msghdr mh = {
                .msg_name = &sa,
                .msg_namelen = offsetof(struct sockaddr_un, sun_path) + strlen(sa.sun_path),
        };
        ssize_t k;
        union {
                struct cmsghdr cmsghdr;
                uint8_t buf[CMSG_SPACE(sizeof(int))];
        } control;
        bool have_syslog_identifier = false;

        assert_return(iov, -EINVAL);
        assert_return(n > 0, -EINVAL);

        w = alloca(sizeof(struct iovec) * n * 5 + 3);
        l = alloca(sizeof(uint64_t) * n);

        for (i = 0; i < n; i++) {
                char *c, *nl;

                if (_unlikely_(!iov[i].iov_base || iov[i].iov_len <= 1))
                        return -EINVAL;

                c = memchr(iov[i].iov_base, '=', iov[i].iov_len);
                if (_unlikely_(!c || c == iov[i].iov_base))
                        return -EINVAL;

                have_syslog_identifier = have_syslog_identifier ||
                        (c == (char *) iov[i].iov_base + 17 &&
                         startswith(iov[i].iov_base, "SYSLOG_IDENTIFIER"));

                nl = memchr(iov[i].iov_base, '\n', iov[i].iov_len);
                if (nl) {
                        if (_unlikely_(nl < c))
                                return -EINVAL;

                        /* Already includes a newline? Bummer, then
                         * let's write the variable name, then a
                         * newline, then the size (64bit LE), followed
                         * by the data and a final newline */

                        w[j].iov_base = iov[i].iov_base;
                        w[j].iov_len = c - (char*) iov[i].iov_base;
                        j++;

                        IOVEC_SET_STRING(w[j++], "\n");

                        l[i] = htole64(iov[i].iov_len - (c - (char*) iov[i].iov_base) - 1);
                        w[j].iov_base = &l[i];
                        w[j].iov_len = sizeof(uint64_t);
                        j++;

                        w[j].iov_base = c + 1;
                        w[j].iov_len = iov[i].iov_len - (c - (char*) iov[i].iov_base) - 1;
                        j++;

                } else
                        /* Nothing special? Then just add the line and
                         * append a newline */
                        w[j++] = iov[i];

                IOVEC_SET_STRING(w[j++], "\n");
        }

        if (!have_syslog_identifier &&
            string_is_safe(program_invocation_short_name)) {

                /* Implicitly add program_invocation_short_name, if it
                 * is not set explicitly. We only do this for
                 * program_invocation_short_name, and nothing else
                 * since everything else is much nicer to retrieve
                 * from the outside. */

                IOVEC_SET_STRING(w[j++], "SYSLOG_IDENTIFIER=");
                IOVEC_SET_STRING(w[j++], program_invocation_short_name);
                IOVEC_SET_STRING(w[j++], "\n");
        }

        fd = journal_fd();
        if (_unlikely_(fd < 0))
                return fd;

        mh.msg_iov = w;
        mh.msg_iovlen = j;

        k = sendmsg(fd, &mh, MSG_NOSIGNAL);
        if (k >= 0)
                return 0;

        /* Fail silently if the journal is not available */
        if (errno == ENOENT)
                return 0;

        if (errno != EMSGSIZE && errno != ENOBUFS)
                return -errno;

        return 0;
}

static int fill_iovec_perror_and_send(const char *message, int skip, struct iovec iov[]) {
        PROTECT_ERRNO;
        size_t n, k;

        k = isempty(message) ? 0 : strlen(message) + 2;
        n = 8 + k + 256 + 1;

        for (;;) {
                char buffer[n];
                char* j;

                errno = 0;
                j = strerror_r(_saved_errno_, buffer + 8 + k, n - 8 - k);
                if (errno == 0) {
                        char error[6 + 10 + 1]; /* for a 32bit value */

                        if (j != buffer + 8 + k)
                                memmove(buffer + 8 + k, j, strlen(j)+1);

                        memcpy(buffer, "MESSAGE=", 8);

                        if (k > 0) {
                                memcpy(buffer + 8, message, k - 2);
                                memcpy(buffer + 8 + k - 2, ": ", 2);
                        }

                        snprintf(error, sizeof(error), "ERRNO=%u", _saved_errno_);
                        char_array_0(error);

                        IOVEC_SET_STRING(iov[skip+0], "PRIORITY=3");
                        IOVEC_SET_STRING(iov[skip+1], buffer);
                        IOVEC_SET_STRING(iov[skip+2], error);

                        return sd_journal_sendv(iov, skip + 3);
                }

                if (errno != ERANGE)
                        return -errno;

                n *= 2;
        }
}

_public_ int sd_journal_perror(const char *message) {
        struct iovec iovec[3];

        return fill_iovec_perror_and_send(message, 0, iovec);
}
