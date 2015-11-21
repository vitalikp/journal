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

#include <string.h>
#include <errno.h>

#include "journald-rate-limit.h"
#include "list.h"
#include "util.h"
#include "hashmap.h"

#define POOLS_MAX 5
#define BUCKETS_MAX 127
#define GROUPS_MAX 2047

static const int priority_map[] = {
        [LOG_EMERG]   = 0,
        [LOG_ALERT]   = 0,
        [LOG_CRIT]    = 0,
        [LOG_ERR]     = 1,
        [LOG_WARNING] = 2,
        [LOG_NOTICE]  = 3,
        [LOG_INFO]    = 3,
        [LOG_DEBUG]   = 4
};

typedef struct JournalRateLimitPool JournalRateLimitPool;

struct JournalRateLimitPool {
        usec_t begin;
        unsigned num;
        unsigned suppressed;
};

struct JournalRateLimit {
        usec_t interval;
        unsigned burst;

        JournalRateLimitPool pools[POOLS_MAX];
};

JournalRateLimit *journal_rate_limit_new(usec_t interval, unsigned burst) {
        JournalRateLimit *r;

        assert(interval > 0 || burst == 0);

        r = new0(JournalRateLimit, 1);
        if (!r)
                return NULL;

        r->interval = interval;
        r->burst = burst;

        return r;
}

void journal_rate_limit_free(JournalRateLimit *r) {
        assert(r);

        free(r);
}

static unsigned burst_modulate(unsigned burst, uint64_t available) {
        unsigned k;

        /* Modulates the burst rate a bit with the amount of available
         * disk space */

        k = u64log2(available);

        /* 1MB */
        if (k <= 20)
                return burst;

        burst = (burst * (k-20)) / 4;

        /*
         * Example:
         *
         *      <= 1MB = rate * 1
         *        16MB = rate * 2
         *       256MB = rate * 3
         *         4GB = rate * 4
         *        64GB = rate * 5
         *         1TB = rate * 6
         */

        return burst;
}

int journal_rate_limit_test(JournalRateLimit *r, const char *id, int priority, uint64_t available) {
        JournalRateLimitPool *p;
        unsigned burst;
        usec_t ts;

        assert(id);

        if (!r)
                return 1;

        if (r->interval == 0 || r->burst == 0)
                return 1;

        burst = burst_modulate(r->burst, available);

        ts = now(CLOCK_MONOTONIC);

        p = &r->pools[priority_map[priority]];

        if (p->begin <= 0) {
                p->suppressed = 0;
                p->num = 1;
                p->begin = ts;
                return 1;
        }

        if (p->begin + r->interval < ts) {
                unsigned s;

                s = p->suppressed;
                p->suppressed = 0;
                p->num = 1;
                p->begin = ts;

                return 1 + s;
        }

        if (p->num <= burst) {
                p->num++;
                return 1;
        }

        p->suppressed++;
        return 0;
}
