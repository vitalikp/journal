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

#include <sys/signalfd.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
#include <sys/timerfd.h>

#include "journal.h"
#include "fileio.h"
#include "hashmap.h"
#include "journal-file.h"
#include "missing.h"
#include "conf-parser.h"
#include "journal-internal.h"
#include "journal-vacuum.h"
#include "journald-rate-limit.h"
#include "journald-kmsg.h"
#include "journald-syslog.h"
#include "journald-console.h"
#include "journald-native.h"
#include "journald-server.h"
#include "utils.h"


#define USER_JOURNALS_MAX 1024

#define DEFAULT_SYNC_INTERVAL_USEC (5*USEC_PER_MINUTE)
#define DEFAULT_MAX_FILE_USEC USEC_PER_MONTH

#define RECHECK_AVAILABLE_SPACE_USEC (30*USEC_PER_SEC)

static uint64_t available_space(Server *s, bool verbose) {
        struct statvfs ss;
        uint64_t sum = 0, ss_avail = 0, avail = 0;
        int r;
        _cleanup_closedir_ DIR *d = NULL;
        usec_t ts;
        const char *f;
        JournalMetrics *m;

        ts = now(CLOCK_MONOTONIC);

        if (s->cached_available_space_timestamp + RECHECK_AVAILABLE_SPACE_USEC > ts
            && !verbose)
                return s->cached_available_space;

        if (s->system_journal) {
                f = JOURNAL_LOGDIR;
                m = &s->system_metrics;
        } else {
                f = JOURNAL_RUNDIR "/log/";
                m = &s->runtime_metrics;
        }

        assert(m);

        d = opendir(f);
        if (!d)
                return 0;

        if (fstatvfs(dirfd(d), &ss) < 0)
                return 0;

        for (;;) {
                struct stat st;
                struct dirent *de;

                errno = 0;
                de = readdir(d);
                if (!de && errno != 0)
                        return 0;

                if (!de)
                        break;

                if (!endswith(de->d_name, ".journal") &&
                    !endswith(de->d_name, ".journal~"))
                        continue;

                if (fstatat(dirfd(d), de->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0)
                        continue;

                if (!S_ISREG(st.st_mode))
                        continue;

                sum += (uint64_t) st.st_blocks * 512UL;
        }

        ss_avail = ss.f_bsize * ss.f_bavail;

        /* If we reached a high mark, we will always allow this much
         * again, unless usage goes above max_use. This watermark
         * value is cached so that we don't give up space on pressure,
         * but hover below the maximum usage. */

        if (m->min_use < sum)
                m->min_use = sum;

        avail = LESS_BY(ss_avail, m->keep_free);

        s->cached_available_space = LESS_BY(MIN(m->max_use, avail), sum);
        s->cached_available_space_timestamp = ts;

        if (verbose) {
                char    fb1[FORMAT_BYTES_MAX], fb2[FORMAT_BYTES_MAX], fb3[FORMAT_BYTES_MAX],
                        fb4[FORMAT_BYTES_MAX], fb5[FORMAT_BYTES_MAX];

                server_driver_message(s,
                                      "%s journal is using %s (max allowed %s, "
                                      "trying to leave %s free of %s available → current limit %s).",
                                      s->system_journal ? "Permanent" : "Runtime",
                                      format_bytes(fb1, sizeof(fb1), sum),
                                      format_bytes(fb2, sizeof(fb2), m->max_use),
                                      format_bytes(fb3, sizeof(fb3), m->keep_free),
                                      format_bytes(fb4, sizeof(fb4), ss_avail),
                                      format_bytes(fb5, sizeof(fb5), s->cached_available_space + sum));
        }

        return s->cached_available_space;
}

void server_fix_perms(Server *s, JournalFile *f) {
        int r;

        assert(f);

        r = fchmod(f->fd, 0640);
        if (r < 0)
                log_warning("Failed to fix access mode on %s, ignoring: %s", f->path, strerror(-r));
}

static JournalFile* find_journal(Server *s, uid_t uid) {
        _cleanup_free_ char *p = NULL;
        int r;
        JournalFile *f;

        assert(s);

        /* We split up user logs only on /var, not on /run. If the
         * runtime file is open, we write to it exclusively, in order
         * to guarantee proper order as soon as we flush /run to
         * /var and close the runtime file. */

        if (s->runtime_journal)
                return s->runtime_journal;

        if (uid <= SYSTEM_UID_MAX)
                return s->system_journal;

        f = hashmap_get(s->user_journals, UINT32_TO_PTR(uid));
        if (f)
                return f;

        if (asprintf(&p, JOURNAL_LOGDIR "/user-"UID_FMT".journal", uid) < 0)
                return s->system_journal;

        while (hashmap_size(s->user_journals) >= USER_JOURNALS_MAX) {
                /* Too many open? Then let's close one */
                f = hashmap_steal_first(s->user_journals);
                assert(f);
                journal_file_close(f);
        }

        r = journal_file_open_reliably(p, O_RDWR|O_CREAT, 0640, s->compress, &s->system_metrics, s->mmap, NULL, &f);
        if (r < 0)
                return s->system_journal;

        server_fix_perms(s, f);

        r = hashmap_put(s->user_journals, UINT32_TO_PTR(uid), f);
        if (r < 0) {
                journal_file_close(f);
                return s->system_journal;
        }

        return f;
}

static int do_rotate(Server *s, JournalFile **f, const char* name,
                     uint32_t uid) {
        int r;
        assert(s);

        if (!*f)
                return -EINVAL;

        r = journal_file_rotate(f, s->compress);
        if (r < 0)
                if (*f)
                        log_error("Failed to rotate %s: %s",
                                  (*f)->path, strerror(-r));
                else
                        log_error("Failed to create new %s journal: %s",
                                  name, strerror(-r));
        else
                server_fix_perms(s, *f);
        return r;
}

void server_rotate(Server *s) {
        JournalFile *f;
        void *k;
        Iterator i;
        int r;

        log_debug("Rotating...");

        do_rotate(s, &s->runtime_journal, "runtime", 0);
        do_rotate(s, &s->system_journal, "system", 0);

        HASHMAP_FOREACH_KEY(f, k, s->user_journals, i) {
                r = do_rotate(s, &f, "user", PTR_TO_UINT32(k));
                if (r >= 0)
                        hashmap_replace(s->user_journals, k, f);
                else if (!f)
                        /* Old file has been closed and deallocated */
                        hashmap_remove(s->user_journals, k);
        }
}

void server_sync(Server *s) {
        JournalFile *f;
        void *k;
        Iterator i;
        int r;

        if (s->sync_seqnum >= s->seqnum)
                return;

        if (s->system_journal) {
                r = journal_file_set_offline(s->system_journal);
                if (r < 0)
                        log_error("Failed to sync system journal: %s", strerror(-r));
        }

        HASHMAP_FOREACH_KEY(f, k, s->user_journals, i) {
                r = journal_file_set_offline(f);
                if (r < 0)
                        log_error("Failed to sync user journal: %s", strerror(-r));
        }

        s->sync_seqnum = s->seqnum;
        s->sync_time = now(CLOCK_MONOTONIC);
}

static void do_vacuum(Server *s, JournalFile *f, const char* path,
                      JournalMetrics *metrics) {
        int r;

        if (!f)
                return;

        r = journal_directory_vacuum(path, metrics->max_use, s->max_retention_usec, &s->oldest_file_usec);
        if (r < 0 && r != -ENOENT)
                log_error("Failed to vacuum %s: %s", path, strerror(-r));
}

void server_vacuum(Server *s) {
        int r;

        log_debug("Vacuuming...");

        s->oldest_file_usec = 0;

        do_vacuum(s, s->system_journal, JOURNAL_LOGDIR, &s->system_metrics);
        do_vacuum(s, s->runtime_journal, JOURNAL_RUNDIR "/log/", &s->runtime_metrics);

        s->cached_available_space_timestamp = 0;
}

bool shall_try_append_again(JournalFile *f, int r) {

        /* -E2BIG            Hit configured limit
           -EFBIG            Hit fs limit
           -EDQUOT           Quota limit hit
           -ENOSPC           Disk full
           -EHOSTDOWN        Other machine
           -EBUSY            Unclean shutdown
           -EPROTONOSUPPORT  Unsupported feature
           -EBADMSG          Corrupted
           -ENODATA          Truncated
           -ESHUTDOWN        Already archived */

        if (r == -E2BIG || r == -EFBIG || r == -EDQUOT || r == -ENOSPC)
                log_debug("%s: Allocation limit reached, rotating.", f->path);
        else if (r == -EHOSTDOWN)
                log_info("%s: Journal file from other machine, rotating.", f->path);
        else if (r == -EBUSY)
                log_info("%s: Unclean shutdown, rotating.", f->path);
        else if (r == -EPROTONOSUPPORT)
                log_info("%s: Unsupported feature, rotating.", f->path);
        else if (r == -EBADMSG || r == -ENODATA || r == ESHUTDOWN)
                log_warning("%s: Journal file corrupted, rotating.", f->path);
        else
                return false;

        return true;
}

int dispatch_message(Server *s, struct iovec *iovec, struct timeval *tv) {
        unsigned n = 0;
        assert(s);
        assert(iovec);

        char boot_id_field[sizeof("_BOOT_ID=") + 32];
        char source_time[sizeof("_SOURCE_REALTIME_TIMESTAMP=") + DECIMAL_STR_MAX(usec_t)];

        if (tv) {
                sprintf(source_time, "_SOURCE_REALTIME_TIMESTAMP=%llu", (unsigned long long) timeval_load(tv));
                IOVEC_SET_STRING(iovec[n++], source_time);
        }

        /* Note that strictly speaking storing the boot id here is
         * redundant since the entry includes this in-line
         * anyway. However, we need this indexed, too. */
        if (!uuid_is_null(s->server.boot_id)) {
                str_copy(boot_id_field, "_BOOT_ID=", 10);
                journal_uuid_to_str(s->server.boot_id, &boot_id_field[9]);
                IOVEC_SET_STRING(iovec[n++], boot_id_field);
        }

        if (!isempty(s->server.hostname))
                IOVEC_SET_STRING(iovec[n++], strappend("_HOSTNAME=", s->server.hostname));

        return n;
}

static void write_to_journal(Server *s, uid_t realuid, struct iovec *iovec, unsigned n, int priority) {
        JournalFile *f;
        bool vacuumed = false;
        int r;

        assert(s);
        assert(iovec);
        assert(n > 0);

        uid_t uid = 0;

        if (realuid > 0)
				/* Split up strictly by any UID */
				uid = realuid;
		else
				uid = 0;

        f = find_journal(s, uid);
        if (!f)
                return;

        if (journal_file_rotate_suggested(f, s->max_file_usec)) {
                log_debug("%s: Journal header limits reached or header out-of-date, rotating.", f->path);
                server_rotate(s);
                server_vacuum(s);
                vacuumed = true;

                f = find_journal(s, uid);
                if (!f)
                        return;
        }

        r = journal_file_append_entry(f, NULL, iovec, n, &s->seqnum, NULL, NULL);
        if (r >= 0) {
                server_schedule_sync(s, priority);
                return;
        }

        if (vacuumed || !shall_try_append_again(f, r)) {
                size_t size = 0;
                unsigned i;
                for (i = 0; i < n; i++)
                        size += iovec[i].iov_len;

                log_error("Failed to write entry (%d items, %zu bytes), ignoring: %s", n, size, strerror(-r));
                return;
        }

        server_rotate(s);
        server_vacuum(s);

        f = find_journal(s, uid);
        if (!f)
                return;

        log_debug("Retrying write.");
        r = journal_file_append_entry(f, NULL, iovec, n, &s->seqnum, NULL, NULL);
        if (r < 0) {
                size_t size = 0;
                unsigned i;
                for (i = 0; i < n; i++)
                        size += iovec[i].iov_len;

                log_error("Failed to write entry (%d items, %zu bytes) despite vacuuming, ignoring: %s", n, size, strerror(-r));
        } else
                server_schedule_sync(s, priority);
}

int dispatch_message_real(
                struct iovec *iovec,
                struct ucred *ucred) {

        char    pid[sizeof("_PID=") + DECIMAL_STR_MAX(pid_t)],
                uid[sizeof("_UID=") + DECIMAL_STR_MAX(uid_t)],
                gid[sizeof("_GID=") + DECIMAL_STR_MAX(gid_t)];
        unsigned n = 0;
        char *x;
        int r;
        char *t;
        bool owner_valid = false;

        assert(iovec);

        if (ucred) {
                sprintf(pid, "_PID="PID_FMT, ucred->pid);
                IOVEC_SET_STRING(iovec[n++], pid);

                sprintf(uid, "_UID="UID_FMT, ucred->uid);
                IOVEC_SET_STRING(iovec[n++], uid);

                sprintf(gid, "_GID="GID_FMT, ucred->gid);
                IOVEC_SET_STRING(iovec[n++], gid);

                r = get_process_comm(ucred->pid, &t);
                if (r >= 0) {
                        x = strappenda("_COMM=", t);
                        free(t);
                        IOVEC_SET_STRING(iovec[n++], x);
                }

                r = get_process_exe(ucred->pid, &t);
                if (r >= 0) {
                        x = strappenda("_EXE=", t);
                        free(t);
                        IOVEC_SET_STRING(iovec[n++], x);
                }

                r = get_process_cmdline(ucred->pid, 0, false, &t);
                if (r >= 0) {
                        x = strappenda("_CMDLINE=", t);
                        free(t);
                        IOVEC_SET_STRING(iovec[n++], x);
                }
        }

        return n;
}

void server_driver_message(Server *s, const char *format, ...) {
        char buffer[16 + LINE_MAX + 1];
        struct iovec iovec[N_IOVEC_META_FIELDS + 4];
        int n = 0;
        va_list ap;
        struct ucred ucred = {};

        assert(s);
        assert(format);

        IOVEC_SET_STRING(iovec[n++], "PRIORITY=6");
        IOVEC_SET_STRING(iovec[n++], "_TRANSPORT=driver");

        memcpy(buffer, "MESSAGE=", 8);
        va_start(ap, format);
        vsnprintf(buffer + 8, sizeof(buffer) - 8, format, ap);
        va_end(ap);
        char_array_0(buffer);
        IOVEC_SET_STRING(iovec[n++], buffer);

        ucred.pid = getpid();
        ucred.uid = getuid();
        ucred.gid = getgid();

        n += dispatch_message_real(&iovec[n], &ucred);
        n += dispatch_message(s, &iovec[n], NULL);
        write_to_journal(s, ucred.uid, iovec, n, LOG_INFO);
}

void server_dispatch_message(
                Server *s,
                struct iovec *iovec, unsigned n, unsigned m,
                struct ucred *ucred,
                struct timeval *tv,
                int priority) {

        int rl, r;
        char *c;

        assert(s);
        assert(iovec || n == 0);

        if (n == 0)
                return;

        if (LOG_PRI(priority) > s->max_level_store)
                return;

        uid_t realuid = 0;

        if (!ucred)
                goto finish;

        realuid = ucred->uid;

        rl = journal_rate_limit_test(s->rate_limit,
                                     priority & LOG_PRIMASK, available_space(s, false));

        if (rl == 0)
                return;

        /* Write a suppression message if we suppressed something */
        if (rl > 1)
                server_driver_message(s, "Suppressed %u messages from uid %u", rl - 1, realuid);

finish:
        n += dispatch_message(s, &iovec[n], tv);
        write_to_journal(s, realuid, iovec, n, priority);
}


static int system_journal_open(Server *s) {
        int r;
        char *fn;

        if (!s->system_journal &&
            access(JOURNAL_RUNDIR "/flushed", F_OK) >= 0) {

                fn = JOURNAL_LOGDIR "/system.journal";
                r = journal_file_open_reliably(fn, O_RDWR|O_CREAT, 0640, s->compress, &s->system_metrics, s->mmap, NULL, &s->system_journal);

                if (r >= 0)
                        server_fix_perms(s, s->system_journal);
                else if (r < 0) {
                        if (r != -ENOENT && r != -EROFS)
                                log_warning("Failed to open system journal: %s", strerror(-r));

                        r = 0;
                }
        }

        if (!s->runtime_journal) {

                fn = strjoin(JOURNAL_RUNDIR "/log/", "system.journal", NULL);
                if (!fn)
                        return -ENOMEM;

                if (s->system_journal) {

                        /* Try to open the runtime journal, but only
                         * if it already exists, so that we can flush
                         * it into the system journal */

                        r = journal_file_open(fn, O_RDWR, 0640, s->compress, &s->runtime_metrics, s->mmap, NULL, &s->runtime_journal);
                        free(fn);

                        if (r < 0) {
                                if (r != -ENOENT)
                                        log_warning("Failed to open runtime journal: %s", strerror(-r));

                                r = 0;
                        }

                } else {

                        /* OK, we really need the runtime journal, so create
                         * it if necessary. */

                        (void) mkdir(JOURNAL_RUNDIR "/log", 0755);

                        r = journal_file_open_reliably(fn, O_RDWR|O_CREAT, 0640, s->compress, &s->runtime_metrics, s->mmap, NULL, &s->runtime_journal);
                        free(fn);

                        if (r < 0) {
                                log_error("Failed to open runtime journal: %s", strerror(-r));
                                return r;
                        }
                }

                if (s->runtime_journal)
                        server_fix_perms(s, s->runtime_journal);
        }

        available_space(s, true);

        return r;
}

int server_flush_to_var(Server *s) {
        sd_journal *j = NULL;
        char ts[FORMAT_TIMESPAN_MAX];
        usec_t start;
        unsigned n = 0;
        int r;

        assert(s);

        if (!s->runtime_journal)
                return 0;

        system_journal_open(s);

        if (!s->system_journal)
                return 0;

        log_debug("Flushing to /var...");

        start = now(CLOCK_MONOTONIC);

        r = sd_journal_open_directory(&j, JOURNAL_RUNDIR "/log", 0);
        if (r < 0) {
                log_error("Failed to read runtime journal: %s", strerror(-r));
                return r;
        }

        sd_journal_set_data_threshold(j, 0);

        SD_JOURNAL_FOREACH(j) {
                Object *o = NULL;
                JournalFile *f;

                f = j->current_file;
                assert(f && f->current_offset > 0);

                n++;

                r = journal_file_move_to_object(f, OBJECT_ENTRY, f->current_offset, &o);
                if (r < 0) {
                        log_error("Can't read entry: %s", strerror(-r));
                        goto finish;
                }

                r = journal_file_copy_entry(f, s->system_journal, o, f->current_offset, NULL, NULL, NULL);
                if (r >= 0)
                        continue;

                if (!shall_try_append_again(s->system_journal, r)) {
                        log_error("Can't write entry: %s", strerror(-r));
                        goto finish;
                }

                server_rotate(s);
                server_vacuum(s);

                if (!s->system_journal) {
                        log_notice("Didn't flush runtime journal since rotation of system journal wasn't successful.");
                        r = -EIO;
                        goto finish;
                }

                log_debug("Retrying write.");
                r = journal_file_copy_entry(f, s->system_journal, o, f->current_offset, NULL, NULL, NULL);
                if (r < 0) {
                        log_error("Can't write entry: %s", strerror(-r));
                        goto finish;
                }
        }

finish:
        journal_file_post_change(s->system_journal);

        journal_file_close(s->runtime_journal);
        s->runtime_journal = NULL;

        sd_journal_close(j);

        if (r >= 0)
                rm_rf(JOURNAL_RUNDIR "/log", false, true, false);

        server_driver_message(s, "Time spent on flushing to /var is %s for %u entries.", format_timespan(ts, sizeof(ts), now(CLOCK_MONOTONIC) - start, 0), n);

        return r;
}

int process_datagram(int fd, uint32_t events, void *userdata)
{
        Server *s = userdata;

        assert(s);
        assert(fd == s->server.native_fd || fd == s->server.syslog_fd);

        if (events != EPOLLIN) {
                log_error("Got invalid event from epoll for datagram fd: %"PRIx32, events);
                return -EIO;
        }

        for (;;) {
                struct ucred *ucred = NULL;
                struct timeval *tv = NULL;
                struct cmsghdr *cmsg;
                struct iovec iovec;

                uint8_t buf[CMSG_SPACE(sizeof(struct ucred)) +
                            CMSG_SPACE(sizeof(struct timeval))];

                struct msghdr msghdr = {
                        .msg_iov = &iovec,
                        .msg_iovlen = 1,
                        .msg_control = &buf,
                        .msg_controllen = sizeof(buf),
                };

                ssize_t n;
                int v;
                size_t off;

                if (ioctl(fd, SIOCINQ, &v) < 0) {
                        log_error("SIOCINQ failed: %m");
                        return -errno;
                }

                if (!GREEDY_REALLOC(s->buffer, s->buffer_size, LINE_MAX + (size_t) v))
                        return log_oom();

                iovec.iov_base = s->buffer;
                iovec.iov_len = s->buffer_size;

                n = recvmsg(fd, &msghdr, MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
                if (n < 0) {
                        if (errno == EINTR || errno == EAGAIN)
                                return 0;

                        log_error("recvmsg() failed: %m");
                        return -errno;
                }

                off = 0;
                while (off < msghdr.msg_controllen)
                {
                        cmsg = (struct cmsghdr*)&buf[off];
                        if (cmsg->cmsg_level != SOL_SOCKET)
                                continue;

                        switch (cmsg->cmsg_type)
                        {
                                case SCM_CREDENTIALS:
                                        if (cmsg->cmsg_len == CMSG_LEN(sizeof(struct ucred)))
                                                ucred = (struct ucred*) CMSG_DATA(cmsg);
                                        break;
                                case SO_TIMESTAMP:
                                        if (cmsg->cmsg_len == CMSG_LEN(sizeof(struct timeval)))
                                                tv = (struct timeval*) CMSG_DATA(cmsg);
                                        break;
                        }

                        off += CMSG_ALIGN(cmsg->cmsg_len);
                }

                if (fd == s->server.syslog_fd) {
                        if (n > 0) {
                                s->buffer[n] = 0;
                                server_process_syslog_message(s, strstrip(s->buffer), ucred, tv);
                        }

                } else {
                        if (n > 0)
                                server_process_native_message(s, s->buffer, n, ucred, tv);
                }
        }
}

static int dispatch_sigusr1(const struct signalfd_siginfo *si, void *userdata) {
        Server *s = userdata;

        assert(s);

        log_info("Received request to flush runtime journal from PID %"PRIu32, si->ssi_pid);

        touch(JOURNAL_RUNDIR "/flushed");
        server_flush_to_var(s);
        server_sync(s);

        return 0;
}

static int dispatch_sigusr2(const struct signalfd_siginfo *si, void *userdata) {
        Server *s = userdata;

        assert(s);

        log_info("Received request to rotate journal from PID %"PRIu32, si->ssi_pid);
        server_rotate(s);
        server_vacuum(s);

        return 0;
}

static int dispatch_sigterm(const struct signalfd_siginfo *si, void *userdata) {
        Server *s = userdata;

        assert(s);

        if (s->state != SERVER_RUNNING)
                return 0;

        log_received_signal(LOG_INFO, si);

        s->state = SERVER_EXITING;

        return 0;
}

static int setup_signals(Server *s) {
        int r;

        assert(s);

        if (epollfd_signal_add(s->server.epoll, SIGUSR1, (signal_cb)dispatch_sigusr1, s) < 0)
        	return -1;

        if (epollfd_signal_add(s->server.epoll, SIGUSR2, (signal_cb)dispatch_sigusr2, s) < 0)
        	return -1;

        if (epollfd_signal_add(s->server.epoll, SIGTERM, (signal_cb)dispatch_sigterm, s) < 0)
        	return -1;

        if (epollfd_signal_add(s->server.epoll, SIGINT, (signal_cb)dispatch_sigterm, s) < 0)
        	return -1;

        if (epollfd_signal_setup(s->server.epoll) < 0)
        	return -1;

        return 0;
}

static int server_parse_proc_cmdline(Server *s) {
        _cleanup_free_ char *line = NULL;
        const char *w, *state;
        size_t l;
        int r;

        r = proc_cmdline(&line);
        if (r < 0)
                log_warning("Failed to read /proc/cmdline, ignoring: %s", strerror(-r));
        if (r <= 0)
                return 0;

        FOREACH_WORD_QUOTED(w, l, line, state) {
                _cleanup_free_ char *word = NULL;

                word = strndup(w, l);
                if (!word)
                        return -ENOMEM;

                if (startswith(word, "journald.forward_to_syslog=")) {
                        if (parse_bool(word + 35, &s->forward_to_syslog) < 0)
                                log_warning("Failed to parse forward to syslog switch %s. Ignoring.", word + 35);
                } else if (startswith(word, "journald.forward_to_console=")) {
                        if (parse_bool(word + 36, &s->forward_to_console) < 0)
                                log_warning("Failed to parse forward to console switch %s. Ignoring.", word + 36);
                } else if (startswith(word, "journald"))
                        log_warning("Invalid journald parameter. Ignoring.");
        }
        /* do not warn about state here, since probably systemd already did */

        return 0;
}

static int server_parse_config_file(Server *s) {
        assert(s);

        return config_parse(JOURNAL_SYSCONFDIR "/journald.conf",
                            "Journal\0System\0Runtime\0",
                            config_item_perf_lookup, journald_gperf_lookup, s);
}

int server_schedule_sync(Server *s, int priority) {
        int r;

        assert(s);

        if (priority <= LOG_CRIT) {
                /* Immediately sync to disk when this is of priority CRIT, ALERT, EMERG */
                server_sync(s);
                return 0;
        }

        return 0;
}

int server_init(Server *s) {
        int r, fd;

        assert(s);

        zero(*s);
        s->server.syslog_fd = s->server.native_fd = s->server.kmsg_fd;
        s->compress = true;

        s->sync_interval_usec = DEFAULT_SYNC_INTERVAL_USEC;
        s->sync_time = -1;

        s->forward_to_syslog = false;

        s->max_file_usec = DEFAULT_MAX_FILE_USEC;

        s->max_level_store = LOG_DEBUG;
        s->max_level_syslog = LOG_DEBUG;
        s->max_level_kmsg = LOG_NOTICE;
        s->max_level_console = LOG_INFO;

        journal_reset_metrics(&s->system_metrics);
        journal_reset_metrics(&s->runtime_metrics);

        server_parse_config_file(s);
        server_parse_proc_cmdline(s);
        if (!!s->rate_limit_interval ^ !!s->rate_limit_burst) {
                log_debug("Setting both rate limit interval and burst from %llu,%u to 0,0",
                          (long long unsigned) s->rate_limit_interval,
                          s->rate_limit_burst);
                s->rate_limit_interval = s->rate_limit_burst = 0;
        }

        s->user_journals = hashmap_new(trivial_hash_func, trivial_compare_func);
        if (!s->user_journals)
                return log_oom();

        s->mmap = mmap_cache_new();
        if (!s->mmap)
                return log_oom();

        s->state = SERVER_RUNNING;
        if (server_start(&s->server) < 0)
        	return -1;

        r = server_open_syslog_socket(s);
        if (r < 0)
                return r;

        r = server_open_native_socket(s);
        if (r < 0)
                return r;

        r = server_open_dev_kmsg(s);
        if (r < 0)
                return r;

        r = setup_signals(s);
        if (r < 0)
                return r;

        s->rate_limit = journal_rate_limit_new(s->rate_limit_interval, s->rate_limit_burst);
        if (!s->rate_limit)
                return -ENOMEM;

        r = system_journal_open(s);
        if (r < 0)
                return r;

        return 0;
}

void server_done(Server *s) {
        JournalFile *f;
        assert(s);

        if (s->system_journal)
                journal_file_close(s->system_journal);

        if (s->runtime_journal)
                journal_file_close(s->runtime_journal);

        while ((f = hashmap_steal_first(s->user_journals)))
                journal_file_close(f);

        hashmap_free(s->user_journals);

        server_stop(&s->server);

        if (s->rate_limit)
                journal_rate_limit_free(s->rate_limit);

        free(s->buffer);
        free(s->tty_path);

        if (s->mmap)
                mmap_cache_unref(s->mmap);
}
