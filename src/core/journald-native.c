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
#include <stddef.h>
#include <sys/epoll.h>

#include "path-util.h"
#include "journald-server.h"
#include "journald-native.h"
#include "journald-kmsg.h"
#include "journald-console.h"
#include "journald-syslog.h"
#include "core/socket.h"

bool valid_user_field(const char *p, size_t l, bool allow_protected) {
        const char *a;

        /* We kinda enforce POSIX syntax recommendations for
           environment variables here, but make a couple of additional
           requirements.

           http://pubs.opengroup.org/onlinepubs/000095399/basedefs/xbd_chap08.html */

        /* No empty field names */
        if (l <= 0)
                return false;

        /* Don't allow names longer than 64 chars */
        if (l > 64)
                return false;

        /* Variables starting with an underscore are protected */
        if (!allow_protected && p[0] == '_')
                return false;

        /* Don't allow digits as first character */
        if (p[0] >= '0' && p[0] <= '9')
                return false;

        /* Only allow A-Z0-9 and '_' */
        for (a = p; a < p + l; a++)
                if ((*a < 'A' || *a > 'Z') &&
                    (*a < '0' || *a > '9') &&
                    *a != '_')
                        return false;

        return true;
}

static bool allow_object_pid(struct ucred *ucred) {
        return ucred && ucred->uid == 0;
}

static int dispatch_message_object(struct iovec *iovec, pid_t object_pid) {
        unsigned n = 0;

        assert(iovec);

        if (!object_pid)
                return 0;

        char o_uid[sizeof("OBJECT_UID=") + DECIMAL_STR_MAX(uid_t)],
             o_gid[sizeof("OBJECT_GID=") + DECIMAL_STR_MAX(gid_t)];

        uid_t object_uid;
        gid_t object_gid;

        char *x;
        int r;
        char *t;

        r = get_process_uid(object_pid, &object_uid);
        if (r >= 0) {
                sprintf(o_uid, "OBJECT_UID="UID_FMT, object_uid);
                IOVEC_SET_STRING(iovec[n++], o_uid);
        }

        r = get_process_gid(object_pid, &object_gid);
        if (r >= 0) {
                sprintf(o_gid, "OBJECT_GID="GID_FMT, object_gid);
                IOVEC_SET_STRING(iovec[n++], o_gid);
        }

        r = get_process_comm(object_pid, &t);
        if (r >= 0) {
                x = strappenda("OBJECT_COMM=", t);
                free(t);
                IOVEC_SET_STRING(iovec[n++], x);
        }

        r = get_process_exe(object_pid, &t);
        if (r >= 0) {
                x = strappenda("OBJECT_EXE=", t);
                free(t);
                IOVEC_SET_STRING(iovec[n++], x);
        }

        r = get_process_cmdline(object_pid, 0, false, &t);
        if (r >= 0) {
                x = strappenda("OBJECT_CMDLINE=", t);
                free(t);
                IOVEC_SET_STRING(iovec[n++], x);
        }

        return n;
}

void server_process_native_message(
                Server *s,
                const void *buffer, size_t buffer_size,
                struct ucred *ucred,
                struct timeval *tv) {

        struct iovec *iovec = NULL;
        unsigned n = 0, j, tn = (unsigned) -1;
        const char *p;
        size_t remaining, m = 0, entry_size = 0;
        int priority = LOG_INFO;
        char *identifier = NULL, *message = NULL;
        pid_t object_pid = 0;

        assert(s);
        assert(buffer || buffer_size == 0);

        p = buffer;
        remaining = buffer_size;

        while (remaining > 0) {
                const char *e, *q;

                e = memchr(p, '\n', remaining);

                if (!e) {
                        /* Trailing noise, let's ignore it, and flush what we collected */
                        log_debug("Received message with trailing noise, ignoring.");
                        break;
                }

                if (e == p) {
                        /* Entry separator */

                        if (entry_size + n + 1 > ENTRY_SIZE_MAX) { /* data + separators + trailer */
                                log_debug("Entry is too big with %u properties and %zu bytes, ignoring.", n, entry_size);
                                continue;
                        }

                        n += dispatch_message_real(&iovec[n], ucred);
                        n += dispatch_message_object(&iovec[n], object_pid);

                        server_dispatch_message(s, iovec, n, m, ucred, tv, priority);
                        n = 0;
                        priority = LOG_INFO;
                        entry_size = 0;

                        p++;
                        remaining--;
                        continue;
                }

                if (*p == '.' || *p == '#') {
                        /* Ignore control commands for now, and
                         * comments too. */
                        remaining -= (e - p) + 1;
                        p = e + 1;
                        continue;
                }

                /* A property follows */

                /* n received properties, +1 for _TRANSPORT */
                if (!GREEDY_REALLOC(iovec, m, n + 1 + N_IOVEC_META_FIELDS + !!object_pid * N_IOVEC_OBJECT_FIELDS)) {
                        log_oom();
                        break;
                }

                q = memchr(p, '=', e - p);
                if (q) {
                        if (valid_user_field(p, q - p, false)) {
                                size_t l;

                                l = e - p;

                                /* If the field name starts with an
                                 * underscore, skip the variable,
                                 * since that indidates a trusted
                                 * field */
                                iovec[n].iov_base = (char*) p;
                                iovec[n].iov_len = l;
                                entry_size += iovec[n].iov_len;
                                n++;

                                /* We need to determine the priority
                                 * of this entry for the rate limiting
                                 * logic */
                                if (l == 10 &&
                                    startswith(p, "PRIORITY=") &&
                                    p[9] >= '0' && p[9] <= '9')
                                        priority = (priority & LOG_FACMASK) | (p[9] - '0');

                                else if (l == 17 &&
                                         startswith(p, "SYSLOG_FACILITY=") &&
                                         p[16] >= '0' && p[16] <= '9')
                                        priority = (priority & LOG_PRIMASK) | ((p[16] - '0') << 3);

                                else if (l == 18 &&
                                         startswith(p, "SYSLOG_FACILITY=") &&
                                         p[16] >= '0' && p[16] <= '9' &&
                                         p[17] >= '0' && p[17] <= '9')
                                        priority = (priority & LOG_PRIMASK) | (((p[16] - '0')*10 + (p[17] - '0')) << 3);

                                else if (l >= 19 &&
                                         startswith(p, "SYSLOG_IDENTIFIER=")) {
                                        char *t;

                                        t = strndup(p + 18, l - 18);
                                        if (t) {
                                                free(identifier);
                                                identifier = t;
                                        }
                                } else if (l >= 8 &&
                                           startswith(p, "MESSAGE=")) {
                                        char *t;

                                        t = strndup(p + 8, l - 8);
                                        if (t) {
                                                free(message);
                                                message = t;
                                        }
                                } else if (l > strlen("OBJECT_PID=") &&
                                           l < strlen("OBJECT_PID=")  + DECIMAL_STR_MAX(pid_t) &&
                                           startswith(p, "OBJECT_PID=") &&
                                           allow_object_pid(ucred)) {
                                        char buf[DECIMAL_STR_MAX(pid_t)];
                                        memcpy(buf, p + strlen("OBJECT_PID="), l - strlen("OBJECT_PID="));
                                        char_array_0(buf);

                                        /* ignore error */
                                        parse_pid(buf, &object_pid);
                                }
                        }

                        remaining -= (e - p) + 1;
                        p = e + 1;
                        continue;
                } else {
                        le64_t l_le;
                        uint64_t l;
                        char *k;

                        if (remaining < e - p + 1 + sizeof(uint64_t) + 1) {
                                log_debug("Failed to parse message, ignoring.");
                                break;
                        }

                        memcpy(&l_le, e + 1, sizeof(uint64_t));
                        l = le64toh(l_le);

                        if (l > DATA_SIZE_MAX) {
                                log_debug("Received binary data block of %"PRIu64" bytes is too large, ignoring.", l);
                                break;
                        }

                        if ((uint64_t) remaining < e - p + 1 + sizeof(uint64_t) + l + 1 ||
                            e[1+sizeof(uint64_t)+l] != '\n') {
                                log_debug("Failed to parse message, ignoring.");
                                break;
                        }

                        k = malloc((e - p) + 1 + l);
                        if (!k) {
                                log_oom();
                                break;
                        }

                        memcpy(k, p, e - p);
                        k[e - p] = '=';
                        memcpy(k + (e - p) + 1, e + 1 + sizeof(uint64_t), l);

                        if (valid_user_field(p, e - p, false)) {
                                iovec[n].iov_base = k;
                                iovec[n].iov_len = (e - p) + 1 + l;
                                entry_size += iovec[n].iov_len;
                                n++;
                        } else
                                free(k);

                        remaining -= (e - p) + 1 + sizeof(uint64_t) + l + 1;
                        p = e + 1 + sizeof(uint64_t) + l + 1;
                }
        }

        if (n <= 0)
                goto finish;

        tn = n++;
        IOVEC_SET_STRING(iovec[tn], "_TRANSPORT=journal");
        entry_size += strlen("_TRANSPORT=journal");

        if (entry_size + n + 1 > ENTRY_SIZE_MAX) { /* data + separators + trailer */
                log_debug("Entry is too big with %u properties and %zu bytes, ignoring.",
                          n, entry_size);
                goto finish;
        }

        if (message) {
                if (s->forward_to_syslog)
                        server_forward_syslog(s, priority, identifier, message, ucred, tv);

                if (s->forward_to_console)
                        server_forward_console(s, priority, identifier, message, ucred);
        }

        n += dispatch_message_real(&iovec[n], ucred);
        n += dispatch_message_object(&iovec[n], object_pid);

        server_dispatch_message(s, iovec, n, m, ucred, tv, priority);

finish:
        for (j = 0; j < n; j++)  {
                if (j == tn)
                        continue;

                if (iovec[j].iov_base < buffer ||
                    (const uint8_t*) iovec[j].iov_base >= (const uint8_t*) buffer + buffer_size)
                        free(iovec[j].iov_base);
        }

        free(iovec);
        free(identifier);
        free(message);
}

int server_open_native_socket(Server*s) {
        int r;

        assert(s);

        s->server.native_fd = socket_open(JOURNAL_RUNDIR "/socket", SOCK_DGRAM);
        if (s->server.native_fd < 0)
        	return -errno;

        r = epollfd_add(s->server.epoll, s->server.native_fd, EPOLLIN, (event_cb)process_datagram, s);
        if (r < 0)
        {
        	log_error("Failed to add native server fd to event loop: %m");
        	return -1;
        }

        return 0;
}
