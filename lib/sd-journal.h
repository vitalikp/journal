/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#ifndef foosdjournalhfoo
#define foosdjournalhfoo

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

#include <inttypes.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <syslog.h>
#include <systemd/_sd-common.h>

#include "uuid.h"

/* Journal APIs. See sd-journal(3) for more information. */

_SD_BEGIN_DECLARATIONS;

/* Write to daemon */
int sd_journal_print(int priority, const char *format, ...) _sd_printf_(2, 3);
int sd_journal_printv(int priority, const char *format, va_list ap) _sd_printf_(2, 0);
int sd_journal_send(const char *format, ...) _sd_printf_(1, 0) _sd_sentinel_;
int sd_journal_sendv(const struct iovec *iov, int n);
int sd_journal_perror(const char *message);

/* Browse journal stream */

typedef struct sd_journal sd_journal;

/* Open flags */
enum OpenFlags {
        SD_JOURNAL_RUNTIME_ONLY = 1,
        SD_JOURNAL_SYSTEM = 2,
        SD_JOURNAL_CURRENT_USER = 4
};

/* Wakeup event types */
enum {
        SD_JOURNAL_NOP,
        SD_JOURNAL_APPEND,
        SD_JOURNAL_INVALIDATE
};

int sd_journal_open(sd_journal **ret, int flags);
int sd_journal_open_directory(sd_journal **ret, const char *path, int flags);
int sd_journal_open_files(sd_journal **ret, const char **paths, int flags);
void sd_journal_close(sd_journal *j);

int sd_journal_previous(sd_journal *j);
int sd_journal_next(sd_journal *j);

int sd_journal_previous_skip(sd_journal *j, uint64_t skip);
int sd_journal_next_skip(sd_journal *j, uint64_t skip);

int sd_journal_get_realtime_usec(sd_journal *j, uint64_t *ret);
int sd_journal_get_monotonic_usec(sd_journal *j, uint64_t *ret, uuid_t *ret_boot_id);

int sd_journal_set_data_threshold(sd_journal *j, size_t sz);
int sd_journal_get_data_threshold(sd_journal *j, size_t *sz);

int sd_journal_get_data(sd_journal *j, const char *field, const void **data, size_t *l);
int sd_journal_enumerate_data(sd_journal *j, const void **data, size_t *l);
void sd_journal_restart_data(sd_journal *j);

int sd_journal_add_match(sd_journal *j, const void *data, size_t size);
int sd_journal_add_disjunction(sd_journal *j);
int sd_journal_add_conjunction(sd_journal *j);
void sd_journal_flush_matches(sd_journal *j);

int sd_journal_seek_head(sd_journal *j);
int sd_journal_seek_tail(sd_journal *j);
int sd_journal_seek_monotonic_usec(sd_journal *j, uuid_t boot_id, uint64_t usec);
int sd_journal_seek_realtime_usec(sd_journal *j, uint64_t usec);
int sd_journal_seek_cursor(sd_journal *j, const char *cursor);

int sd_journal_get_cursor(sd_journal *j, char **cursor);
int sd_journal_test_cursor(sd_journal *j, const char *cursor);

int sd_journal_get_cutoff_realtime_usec(sd_journal *j, uint64_t *from, uint64_t *to);
int sd_journal_get_cutoff_monotonic_usec(sd_journal *j, const uuid_t boot_id, uint64_t *from, uint64_t *to);

int sd_journal_get_usage(sd_journal *j, uint64_t *bytes);

int sd_journal_query_unique(sd_journal *j, const char *field);
int sd_journal_enumerate_unique(sd_journal *j, const void **data, size_t *l);
void sd_journal_restart_unique(sd_journal *j);

int sd_journal_get_fd(sd_journal *j);
int sd_journal_get_events(sd_journal *j);
int sd_journal_get_timeout(sd_journal *j, uint64_t *timeout_usec);
int sd_journal_process(sd_journal *j);
int sd_journal_wait(sd_journal *j, uint64_t timeout_usec);
int sd_journal_reliable_fd(sd_journal *j);

#define SD_JOURNAL_FOREACH(j)                                           \
        if (sd_journal_seek_head(j) >= 0)                               \
                while (sd_journal_next(j) > 0)

#define SD_JOURNAL_FOREACH_BACKWARDS(j)                                 \
        if (sd_journal_seek_tail(j) >= 0)                               \
                while (sd_journal_previous(j) > 0)

#define SD_JOURNAL_FOREACH_DATA(j, data, l)                             \
        for (sd_journal_restart_data(j); sd_journal_enumerate_data((j), &(data), &(l)) > 0; )

#define SD_JOURNAL_FOREACH_UNIQUE(j, data, l)                           \
        for (sd_journal_restart_unique(j); sd_journal_enumerate_unique((j), &(data), &(l)) > 0; )

_SD_END_DECLARATIONS;

#endif
