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

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include "journald-server.h"
#include "journald-kmsg.h"
#include "journald-syslog.h"


static void dev_kmsg_record(Server *s, char *p, size_t l) {
        struct iovec iovec[N_IOVEC_META_FIELDS + 7 + N_IOVEC_KERNEL_FIELDS];
        char *message = NULL, *syslog_priority = NULL, *syslog_pid = NULL, *syslog_facility = NULL, *syslog_identifier = NULL, *source_time = NULL;
        int priority, r;
        unsigned n = 0, z = 0, j;
        unsigned long long usec;
        char *identifier = NULL, *pid = NULL, *e, *f, *k;
        uint64_t serial;
        size_t pl;

        assert(s);
        assert(p);

        if (l <= 0)
                return;

        e = memchr(p, ',', l);
        if (!e)
                return;
        *e = 0;

        r = safe_atoi(p, &priority);
        if (r < 0 || priority < 0 || priority > 999)
                return;

        l -= (e - p) + 1;
        p = e + 1;
        e = memchr(p, ',', l);
        if (!e)
                return;
        *e = 0;

        r = safe_atou64(p, &serial);
        if (r < 0)
                return;

        if (s->kernel_seqnum) {
                /* We already read this one? */
                if (serial < *s->kernel_seqnum)
                        return;

                /* Did we lose any? */
                if (serial > *s->kernel_seqnum)
                        server_driver_message(s, "Missed %"PRIu64" kernel messages",
                                              serial - *s->kernel_seqnum);

                /* Make sure we never read this one again. Note that
                 * we always store the next message serial we expect
                 * here, simply because this makes handling the first
                 * message with serial 0 easy. */
                *s->kernel_seqnum = serial + 1;
        }

        l -= (e - p) + 1;
        p = e + 1;
        f = memchr(p, ';', l);
        if (!f)
                return;
        /* Kernel 3.6 has the flags field, kernel 3.5 lacks that */
        e = memchr(p, ',', l);
        if (!e || f < e)
                e = f;
        *e = 0;

        r = safe_atollu(p, &usec);
        if (r < 0)
                return;

        l -= (f - p) + 1;
        p = f + 1;
        e = memchr(p, '\n', l);
        if (!e)
                return;
        *e = 0;

        pl = e - p;
        l -= (e - p) + 1;
        k = e + 1;

        for (j = 0; l > 0 && j < N_IOVEC_KERNEL_FIELDS; j++) {
                char *m;
                /* Meta data fields attached */

                if (*k != ' ')
                        break;

                k ++, l --;

                e = memchr(k, '\n', l);
                if (!e)
                        return;

                *e = 0;

                m = cunescape_length_with_prefix(k, e - k, "_KERNEL_");
                if (!m)
                        break;

                IOVEC_SET_STRING(iovec[n++], m);
                z++;

                l -= (e - k) + 1;
                k = e + 1;
        }

        if (asprintf(&source_time, "_SOURCE_MONOTONIC_TIMESTAMP=%llu", usec) >= 0)
                IOVEC_SET_STRING(iovec[n++], source_time);

        IOVEC_SET_STRING(iovec[n++], "_TRANSPORT=kernel");

        if (asprintf(&syslog_priority, "PRIORITY=%i", priority & LOG_PRIMASK) >= 0)
                IOVEC_SET_STRING(iovec[n++], syslog_priority);

        if (asprintf(&syslog_facility, "SYSLOG_FACILITY=%i", LOG_FAC(priority)) >= 0)
                IOVEC_SET_STRING(iovec[n++], syslog_facility);

        if ((priority & LOG_FACMASK) == LOG_KERN)
                IOVEC_SET_STRING(iovec[n++], "SYSLOG_IDENTIFIER=kernel");
        else {
                pl -= syslog_parse_identifier((const char**) &p, &identifier, &pid);

                if (identifier) {
                        syslog_identifier = strappend("SYSLOG_IDENTIFIER=", identifier);
                        if (syslog_identifier)
                                IOVEC_SET_STRING(iovec[n++], syslog_identifier);
                }

                if (pid) {
                        syslog_pid = strappend("SYSLOG_PID=", pid);
                        if (syslog_pid)
                                IOVEC_SET_STRING(iovec[n++], syslog_pid);
                }
        }

        message = cunescape_length_with_prefix(p, pl, "MESSAGE=");
        if (message)
                IOVEC_SET_STRING(iovec[n++], message);

        server_dispatch_message(s, iovec, n, ELEMENTSOF(iovec), NULL, NULL, NULL, 0, NULL, priority, 0);

finish:
        for (j = 0; j < z; j++)
                free(iovec[j].iov_base);

        free(message);
        free(syslog_priority);
        free(syslog_identifier);
        free(syslog_pid);
        free(syslog_facility);
        free(source_time);
        free(identifier);
        free(pid);
}

static int server_read_dev_kmsg(Server *s) {
        char buffer[8192+1]; /* the kernel-side limit per record is 8K currently */
        ssize_t l;

        assert(s);
        assert(s->dev_kmsg_fd >= 0);

        l = read(s->dev_kmsg_fd, buffer, sizeof(buffer) - 1);
        if (l == 0)
                return 0;
        if (l < 0) {
                /* Old kernels who don't allow reading from /dev/kmsg
                 * return EINVAL when we try. So handle this cleanly,
                 * but don' try to ever read from it again. */
                if (errno == EINVAL) {
                        epollfd_del(s->server.epoll, s->dev_kmsg_fd);
                        return 0;
                }

                if (errno == EAGAIN || errno == EINTR || errno == EPIPE)
                        return 0;

                log_error("Failed to read from kernel: %m");
                return -errno;
        }

        dev_kmsg_record(s, buffer, l);
        return 1;
}

int server_flush_dev_kmsg(Server *s) {
        int r;

        assert(s);

        if (s->dev_kmsg_fd < 0)
                return 0;

        if (!s->dev_kmsg_readable)
                return 0;

        log_debug("Flushing /dev/kmsg...");

        for (;;) {
                r = server_read_dev_kmsg(s);
                if (r < 0)
                        return r;

                if (r == 0)
                        break;
        }

        return 0;
}

static int dispatch_dev_kmsg(int fd, uint32_t revents, void *userdata) {
        Server *s = userdata;

        assert(fd == s->dev_kmsg_fd);
        assert(s);

        if (revents & EPOLLERR)
                log_warning("/dev/kmsg buffer overrun, some messages lost.");

        if (!(revents & EPOLLIN))
                log_error("Got invalid event from epoll for /dev/kmsg: %"PRIx32, revents);

        return server_read_dev_kmsg(s);
}

int server_open_dev_kmsg(Server *s) {
        int r;

        assert(s);

        s->dev_kmsg_fd = open("/dev/kmsg", O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOCTTY);
        if (s->dev_kmsg_fd < 0) {
                log_full(errno == ENOENT ? LOG_DEBUG : LOG_WARNING,
                         "Failed to open /dev/kmsg, ignoring: %m");
                return 0;
        }

        r = epollfd_add(s->server.epoll, s->dev_kmsg_fd, EPOLLIN, (event_cb)dispatch_dev_kmsg, s);
        if (r < 0)
        {
        	/* This will fail with EPERM on older kernels where
			 * /dev/kmsg is not readable. */
			if (errno == -EPERM)
			{
				r = 0;
				goto fail;
			}

        	log_error("Failed to add /dev/kmsg fd to event loop: %m");
        	goto fail;
        }

        s->dev_kmsg_readable = true;

        return 0;

fail:
        s->dev_kmsg_fd = safe_close(s->dev_kmsg_fd);

        return r;
}

int server_open_kernel_seqnum(Server *s) {
        _cleanup_close_ int fd = -1;
        uint64_t *p;

        assert(s);

        /* We store the seqnum we last read in an mmaped file. That
         * way we can just use it like a variable, but it is
         * persistent and automatically flushed at reboot. */

        fd = open(JOURNAL_RUNDIR "/kernel-seqnum", O_RDWR|O_CREAT|O_CLOEXEC|O_NOCTTY|O_NOFOLLOW, 0644);
        if (fd < 0) {
                log_error("Failed to open %s/kernel-seqnum, ignoring: %m", JOURNAL_RUNDIR);
                return 0;
        }

        if (posix_fallocate(fd, 0, sizeof(uint64_t)) < 0) {
                log_error("Failed to allocate sequential number file, ignoring: %m");
                return 0;
        }

        p = mmap(NULL, sizeof(uint64_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED) {
                log_error("Failed to map sequential number file, ignoring: %m");
                return 0;
        }

        s->kernel_seqnum = p;

        return 0;
}
