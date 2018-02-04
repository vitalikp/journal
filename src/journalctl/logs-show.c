/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2012 Lennart Poettering

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

#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>

#include "logs-show.h"
#include "log.h"
#include "util.h"
#include "utf8.h"
#include "gunicode.h"
#include "hashmap.h"
#include "fileio.h"
#include "journal-internal.h"

/* up to three lines (each up to 100 characters),
   or 300 characters, whichever is less */
#define PRINT_LINE_THRESHOLD 3
#define PRINT_CHAR_THRESHOLD 300

#define JSON_THRESHOLD 4096

static int parse_field(const void *data, size_t length, const char *field, char **target, size_t *target_size) {
        size_t fl, nl;
        void *buf;

        assert(data);
        assert(field);
        assert(target);
        assert(target_size);

        fl = strlen(field);
        if (length < fl)
                return 0;

        if (memcmp(data, field, fl))
                return 0;

        nl = length - fl;
        buf = malloc(nl+1);
        if (!buf)
                return log_oom();

        memcpy(buf, (const char*) data + fl, nl);
        ((char*)buf)[nl] = 0;

        free(*target);
        *target = buf;
        *target_size = nl;

        return 1;
}

static bool shall_print(const char *p, size_t l, OutputFlags flags) {
        assert(p);

        if (flags & OUTPUT_SHOW_ALL)
                return true;

        if (l >= PRINT_CHAR_THRESHOLD)
                return false;

        if (!utf8_is_printable(p, l))
                return false;

        return true;
}

static char *ascii_ellipsize_mem(const char *s, size_t old_length, size_t new_length, unsigned percent) {
        size_t x;
        char *r;

        assert(s);
        assert(percent <= 100);
        assert(new_length >= 3);

        if (old_length <= 3 || old_length <= new_length)
                return strndup(s, old_length);

        r = new0(char, new_length+1);
        if (!r)
                return NULL;

        x = (new_length * percent) / 100;

        if (x > new_length - 3)
                x = new_length - 3;

        memcpy(r, s, x);
        r[x] = '.';
        r[x+1] = '.';
        r[x+2] = '.';
        memcpy(r + x + 3,
               s + old_length - (new_length - x - 3),
               new_length - x - 3);

        return r;
}

static char *ellipsize_mem(const char *s, size_t old_length, size_t new_length, unsigned percent) {
        size_t x;
        char *e;
        const char *i, *j;
        unsigned k, len, len2;

        assert(s);
        assert(percent <= 100);
        assert(new_length >= 3);

        /* if no multibyte characters use ascii_ellipsize_mem for speed */
        if (ascii_is_valid(s))
                return ascii_ellipsize_mem(s, old_length, new_length, percent);

        if (old_length <= 3 || old_length <= new_length)
                return strndup(s, old_length);

        x = (new_length * percent) / 100;

        if (x > new_length - 3)
                x = new_length - 3;

        k = 0;
        for (i = s; k < x && i < s + old_length; i = utf8_next_char(i)) {
                int c;

                c = utf8_encoded_to_unichar(i);
                if (c < 0)
                        return NULL;
                k += unichar_iswide(c) ? 2 : 1;
        }

        if (k > x) /* last character was wide and went over quota */
                x ++;

        for (j = s + old_length; k < new_length && j > i; ) {
                int c;

                j = utf8_prev_char(j);
                c = utf8_encoded_to_unichar(j);
                if (c < 0)
                        return NULL;
                k += unichar_iswide(c) ? 2 : 1;
        }
        assert(i <= j);

        /* we don't actually need to ellipsize */
        if (i == j)
                return memdup(s, old_length + 1);

        /* make space for ellipsis */
        j = utf8_next_char(j);

        len = i - s;
        len2 = s + old_length - j;
        e = new(char, len + 3 + len2 + 1);
        if (!e)
                return NULL;

        /*
        printf("old_length=%zu new_length=%zu x=%zu len=%u len2=%u k=%u\n",
               old_length, new_length, x, len, len2, k);
        */

        memcpy(e, s, len);
        e[len]   = 0xe2; /* tri-dot ellipsis: … */
        e[len + 1] = 0x80;
        e[len + 2] = 0xa6;

        memcpy(e + len + 3, j, len2 + 1);

        return e;
}

#define ANSI_LIGHTBLUE "\x1B[38;5;153m"
#define ANSI_LIGHTYELLOW "\x1B[38;5;228m"
static bool print_multiline(FILE *f, unsigned prefix, unsigned n_columns, OutputFlags flags, int priority, const char* message, size_t message_len) {
        const char *color_on = "", *color_off = "";
        const char *pos, *end;
        bool ellipsized = false;
        int line = 0;

        if (flags & OUTPUT_COLOR) {
                if (priority <= LOG_ERR) {
                        color_on = ANSI_LIGHTRED_ON;
                        color_off = ANSI_HIGHLIGHT_OFF;
                } else if (priority <= LOG_WARNING) {
                        color_on = ANSI_LIGHTYELLOW;
                        color_off = ANSI_HIGHLIGHT_OFF;
                } else if (priority <= LOG_NOTICE) {
                        color_on = ANSI_LIGHTBLUE;
                        color_off = ANSI_HIGHLIGHT_OFF;
                }
        }

        /* A special case: make sure that we print a newline when
           the message is empty. */
        if (message_len == 0)
                fputs("\n", f);

        for (pos = message;
             pos < message + message_len;
             pos = end + 1, line++) {
                bool continuation = line > 0;
                bool tail_line;
                int len;
                for (end = pos; end < message + message_len && *end != '\n'; end++)
                        ;
                len = end - pos;
                assert(len >= 0);

                /* We need to figure out when we are showing not-last line, *and*
                 * will skip subsequent lines. In that case, we will put the dots
                 * at the end of the line, instead of putting dots in the middle
                 * or not at all.
                 */
                tail_line =
                        line + 1 == PRINT_LINE_THRESHOLD ||
                        end + 1 >= message + PRINT_CHAR_THRESHOLD;

                if (flags & (OUTPUT_FULL_WIDTH | OUTPUT_SHOW_ALL) ||
                    (prefix + len + 1 < n_columns && !tail_line)) {
                        fprintf(f, "%*s%s%.*s%s\n",
                                continuation * prefix, "",
                                color_on, len, pos, color_off);
                        continue;
                }

                /* Beyond this point, ellipsization will happen. */
                ellipsized = true;

                if (prefix < n_columns && n_columns - prefix >= 3) {
                        if (n_columns - prefix > (unsigned) len + 3)
                                fprintf(f, "%*s%s%.*s...%s\n",
                                        continuation * prefix, "",
                                        color_on, len, pos, color_off);
                        else {
                                _cleanup_free_ char *e = NULL;

                                e = ellipsize_mem(pos, len, n_columns - prefix,
                                                  tail_line ? 100 : 90);
                                if (!e)
                                        fprintf(f, "%*s%s%.*s%s\n",
                                                continuation * prefix, "",
                                                color_on, len, pos, color_off);
                                else
                                        fprintf(f, "%*s%s%s%s\n",
                                                continuation * prefix, "",
                                                color_on, e, color_off);
                        }
                } else
                        fputs("...\n", f);

                if (tail_line)
                        break;
        }

        return ellipsized;
}

static char *strip_tab_ansi(char **ibuf, size_t *_isz) {
        const char *i, *begin = NULL;
        enum {
                STATE_OTHER,
                STATE_ESCAPE,
                STATE_BRACKET
        } state = STATE_OTHER;
        char *obuf = NULL;
        size_t osz = 0, isz;
        FILE *f;

        assert(ibuf);
        assert(*ibuf);

        /* Strips ANSI color and replaces TABs by 8 spaces */

        isz = _isz ? *_isz : strlen(*ibuf);

        f = open_memstream(&obuf, &osz);
        if (!f)
                return NULL;

        for (i = *ibuf; i < *ibuf + isz + 1; i++) {

                switch (state) {

                case STATE_OTHER:
                        if (i >= *ibuf + isz) /* EOT */
                                break;
                        else if (*i == '\x1B')
                                state = STATE_ESCAPE;
                        else if (*i == '\t')
                                fputs("        ", f);
                        else
                                fputc(*i, f);
                        break;

                case STATE_ESCAPE:
                        if (i >= *ibuf + isz) { /* EOT */
                                fputc('\x1B', f);
                                break;
                        } else if (*i == '[') {
                                state = STATE_BRACKET;
                                begin = i + 1;
                        } else {
                                fputc('\x1B', f);
                                fputc(*i, f);
                                state = STATE_OTHER;
                        }

                        break;

                case STATE_BRACKET:

                        if (i >= *ibuf + isz || /* EOT */
                            (!(*i >= '0' && *i <= '9') && *i != ';' && *i != 'm')) {
                                fputc('\x1B', f);
                                fputc('[', f);
                                state = STATE_OTHER;
                                i = begin-1;
                        } else if (*i == 'm')
                                state = STATE_OTHER;
                        break;
                }
        }

        if (ferror(f)) {
                fclose(f);
                free(obuf);
                return NULL;
        }

        fclose(f);

        free(*ibuf);
        *ibuf = obuf;

        if (_isz)
                *_isz = osz;

        return obuf;
}

static int output_short(
                FILE *f,
                sd_journal *j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags) {

        int r;
        const void *data;
        size_t length;
        size_t n = 0;
        _cleanup_free_ char *hostname = NULL, *identifier = NULL, *comm = NULL, *pid = NULL, *fake_pid = NULL, *message = NULL, *realtime = NULL, *monotonic = NULL, *priority = NULL;
        size_t hostname_len = 0, identifier_len = 0, comm_len = 0, pid_len = 0, fake_pid_len = 0, message_len = 0, realtime_len = 0, monotonic_len = 0, priority_len = 0;
        int p = LOG_INFO;
        bool ellipsized = false;

        assert(f);
        assert(j);

        /* Set the threshold to one bigger than the actual print
         * threshold, so that if the line is actually longer than what
         * we're willing to print, ellipsization will occur. This way
         * we won't output a misleading line without any indication of
         * truncation.
         */
        sd_journal_set_data_threshold(j, flags & (OUTPUT_SHOW_ALL|OUTPUT_FULL_WIDTH) ? 0 : PRINT_CHAR_THRESHOLD + 1);

        JOURNAL_FOREACH_DATA_RETVAL(j, data, length, r) {

                r = parse_field(data, length, "PRIORITY=", &priority, &priority_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "_HOSTNAME=", &hostname, &hostname_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "SYSLOG_IDENTIFIER=", &identifier, &identifier_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "_COMM=", &comm, &comm_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "_PID=", &pid, &pid_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "SYSLOG_PID=", &fake_pid, &fake_pid_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "_SOURCE_REALTIME_TIMESTAMP=", &realtime, &realtime_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "_SOURCE_MONOTONIC_TIMESTAMP=", &monotonic, &monotonic_len);
                if (r < 0)
                        return r;
                else if (r > 0)
                        continue;

                r = parse_field(data, length, "MESSAGE=", &message, &message_len);
                if (r < 0)
                        return r;
        }

        if (r < 0)
                return r;

        if (!message)
                return 0;

        if (!(flags & OUTPUT_SHOW_ALL))
                strip_tab_ansi(&message, &message_len);

        if (priority_len == 1 && *priority >= '0' && *priority <= '7')
                p = *priority - '0';

        if (mode == OUTPUT_SHORT_MONOTONIC) {
                uint64_t t;
                uuid_t boot_id;

                r = -ENOENT;

                if (monotonic)
                        r = safe_atou64(monotonic, &t);

                if (r < 0)
                        r = sd_journal_get_monotonic_usec(j, &t, &boot_id);

                if (r < 0) {
                        log_error("Failed to get monotonic timestamp: %s", strerror(-r));
                        return r;
                }

                fprintf(f, "[%5llu.%06llu]",
                        (unsigned long long) (t / USEC_PER_SEC),
                        (unsigned long long) (t % USEC_PER_SEC));

                n += 1 + 5 + 1 + 6 + 1;

        } else {
                char buf[64];
                uint64_t x;
                time_t t;
                struct tm tm;

                r = -ENOENT;

                if (realtime)
                        r = safe_atou64(realtime, &x);

                if (r < 0)
                        r = sd_journal_get_realtime_usec(j, &x);

                if (r < 0) {
                        log_error("Failed to get realtime timestamp: %s", strerror(-r));
                        return r;
                }

                t = (time_t) (x / USEC_PER_SEC);

                switch(mode) {
                case OUTPUT_SHORT_ISO:
                        r = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", localtime_r(&t, &tm));
                        break;
                case OUTPUT_SHORT_PRECISE:
                        r = strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localtime_r(&t, &tm));
                        if (r > 0) {
                                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                                         ".%06llu", (unsigned long long) (x % USEC_PER_SEC));
                        }
                        break;
                default:
                        r = strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localtime_r(&t, &tm));
                }

                if (r <= 0) {
                        log_error("Failed to format time.");
                        return -EINVAL;
                }

                fputs(buf, f);
                n += strlen(buf);
        }

        if (hostname && shall_print(hostname, hostname_len, flags)) {
                fprintf(f, " %.*s", (int) hostname_len, hostname);
                n += hostname_len + 1;
        }

        if (identifier && shall_print(identifier, identifier_len, flags)) {
                fprintf(f, " %.*s", (int) identifier_len, identifier);
                n += identifier_len + 1;
        } else if (comm && shall_print(comm, comm_len, flags)) {
                fprintf(f, " %.*s", (int) comm_len, comm);
                n += comm_len + 1;
        } else
                fputc(' ', f);

        if (pid && shall_print(pid, pid_len, flags)) {
                fprintf(f, "[%.*s]", (int) pid_len, pid);
                n += pid_len + 2;
        } else if (fake_pid && shall_print(fake_pid, fake_pid_len, flags)) {
                fprintf(f, "[%.*s]", (int) fake_pid_len, fake_pid);
                n += fake_pid_len + 2;
        }

        if (!(flags & OUTPUT_SHOW_ALL) && !utf8_is_printable(message, message_len)) {
                char bytes[FORMAT_BYTES_MAX];
                fprintf(f, ": [%s blob data]\n", format_bytes(bytes, sizeof(bytes), message_len));
        } else {
                fputs(": ", f);
                ellipsized |=
                        print_multiline(f, n + 2, n_columns, flags, p, message, message_len);
        }

        return ellipsized;
}

static int output_verbose(
                FILE *f,
                sd_journal *j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags) {

        const void *data;
        size_t length;
        _cleanup_free_ char *cursor = NULL;
        uint64_t realtime;
        char ts[FORMAT_TIMESTAMP_MAX + 7];
        int r;

        assert(f);
        assert(j);

        sd_journal_set_data_threshold(j, 0);

        r = sd_journal_get_data(j, "_SOURCE_REALTIME_TIMESTAMP", &data, &length);
        if (r == -ENOENT)
                log_debug("Source realtime timestamp not found");
        else if (r < 0) {
                log_full(r == -EADDRNOTAVAIL ? LOG_DEBUG : LOG_ERR,
                         "Failed to get source realtime timestamp: %s", strerror(-r));
                return r;
        } else {
                _cleanup_free_ char *value = NULL;
                size_t size;

                r = parse_field(data, length, "_SOURCE_REALTIME_TIMESTAMP=", &value, &size);
                if (r < 0)
                        log_debug("_SOURCE_REALTIME_TIMESTAMP invalid: %s", strerror(-r));
                else {
                        r = safe_atou64(value, &realtime);
                        if (r < 0)
                                log_debug("Failed to parse realtime timestamp: %s",
                                          strerror(-r));
                }
        }

        if (r < 0) {
                r = sd_journal_get_realtime_usec(j, &realtime);
                if (r < 0) {
                        log_full(r == -EADDRNOTAVAIL ? LOG_DEBUG : LOG_ERR,
                                 "Failed to get realtime timestamp: %s", strerror(-r));
                        return r;
                }
        }

        r = sd_journal_get_cursor(j, &cursor);
        if (r < 0) {
                log_error("Failed to get cursor: %s", strerror(-r));
                return r;
        }

        fprintf(f, "%s [%s]\n",
                format_timestamp_us(ts, sizeof(ts), realtime),
                cursor);

        JOURNAL_FOREACH_DATA_RETVAL(j, data, length, r) {
                const char *c;
                int fieldlen;
                const char *on = "", *off = "";

                c = memchr(data, '=', length);
                if (!c) {
                        log_error("Invalid field.");
                        return -EINVAL;
                }
                fieldlen = c - (const char*) data;

                if (flags & OUTPUT_COLOR && startswith(data, "MESSAGE=")) {
                        on = ANSI_HIGHLIGHT_ON;
                        off = ANSI_HIGHLIGHT_OFF;
                }

                if (flags & OUTPUT_SHOW_ALL ||
                    (((length < PRINT_CHAR_THRESHOLD) || flags & OUTPUT_FULL_WIDTH)
                     && utf8_is_printable(data, length))) {
                        fprintf(f, "    %s%.*s=", on, fieldlen, (const char*)data);
                        print_multiline(f, 4 + fieldlen + 1, 0, OUTPUT_FULL_WIDTH, 0, c + 1, length - fieldlen - 1);
                        fputs(off, f);
                } else {
                        char bytes[FORMAT_BYTES_MAX];

                        fprintf(f, "    %s%.*s=[%s blob data]%s\n",
                                on,
                                (int) (c - (const char*) data),
                                (const char*) data,
                                format_bytes(bytes, sizeof(bytes), length - (c - (const char *) data) - 1),
                                off);
                }
        }

        if (r < 0)
                return r;

        return 0;
}

static int output_export(
                FILE *f,
                sd_journal *j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags) {

        uuid_t boot_id;
        char sid[33];
        int r;
        usec_t realtime, monotonic;
        _cleanup_free_ char *cursor = NULL;
        const void *data;
        size_t length;

        assert(j);

        sd_journal_set_data_threshold(j, 0);

        r = sd_journal_get_realtime_usec(j, &realtime);
        if (r < 0) {
                log_error("Failed to get realtime timestamp: %s", strerror(-r));
                return r;
        }

        r = sd_journal_get_monotonic_usec(j, &monotonic, &boot_id);
        if (r < 0) {
                log_error("Failed to get monotonic timestamp: %s", strerror(-r));
                return r;
        }

        r = sd_journal_get_cursor(j, &cursor);
        if (r < 0) {
                log_error("Failed to get cursor: %s", strerror(-r));
                return r;
        }

        fprintf(f,
                "__CURSOR=%s\n"
                "__REALTIME_TIMESTAMP="USEC_FMT"\n"
                "__MONOTONIC_TIMESTAMP="USEC_FMT"\n"
                "_BOOT_ID=%s\n",
                cursor,
                realtime,
                monotonic,
                journal_uuid_to_str(boot_id, sid));

        JOURNAL_FOREACH_DATA_RETVAL(j, data, length, r) {

                /* We already printed the boot id, from the data in
                 * the header, hence let's suppress it here */
                if (length >= 9 &&
                    startswith(data, "_BOOT_ID="))
                        continue;

                if (utf8_is_printable_newline(data, length, false))
                        fwrite(data, length, 1, f);
                else {
                        const char *c;
                        uint64_t le64;

                        c = memchr(data, '=', length);
                        if (!c) {
                                log_error("Invalid field.");
                                return -EINVAL;
                        }

                        fwrite(data, c - (const char*) data, 1, f);
                        fputc('\n', f);
                        le64 = htole64(length - (c - (const char*) data) - 1);
                        fwrite(&le64, sizeof(le64), 1, f);
                        fwrite(c + 1, length - (c - (const char*) data) - 1, 1, f);
                }

                fputc('\n', f);
        }

        if (r < 0)
                return r;

        fputc('\n', f);

        return 0;
}

void json_escape(
                FILE *f,
                const char* p,
                size_t l,
                OutputFlags flags) {

        assert(f);
        assert(p);

        if (!(flags & OUTPUT_SHOW_ALL) && l >= JSON_THRESHOLD)

                fputs("null", f);

        else if (!utf8_is_printable(p, l)) {
                bool not_first = false;

                fputs("[ ", f);

                while (l > 0) {
                        if (not_first)
                                fprintf(f, ", %u", (uint8_t) *p);
                        else {
                                not_first = true;
                                fprintf(f, "%u", (uint8_t) *p);
                        }

                        p++;
                        l--;
                }

                fputs(" ]", f);
        } else {
                fputc('\"', f);

                while (l > 0) {
                        if (*p == '"' || *p == '\\') {
                                fputc('\\', f);
                                fputc(*p, f);
                        } else if (*p == '\n')
                                fputs("\\n", f);
                        else if (*p < ' ')
                                fprintf(f, "\\u%04x", *p);
                        else
                                fputc(*p, f);

                        p++;
                        l--;
                }

                fputc('\"', f);
        }
}

static int output_json(
                FILE *f,
                sd_journal *j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags) {

        uint64_t realtime, monotonic;
        _cleanup_free_ char *cursor = NULL;
        const void *data;
        size_t length;
        uuid_t boot_id;
        char sid[33], *k;
        int r;
        Hashmap *h = NULL;
        bool done, separator;

        assert(j);

        sd_journal_set_data_threshold(j, flags & OUTPUT_SHOW_ALL ? 0 : JSON_THRESHOLD);

        r = sd_journal_get_realtime_usec(j, &realtime);
        if (r < 0) {
                log_error("Failed to get realtime timestamp: %s", strerror(-r));
                return r;
        }

        r = sd_journal_get_monotonic_usec(j, &monotonic, &boot_id);
        if (r < 0) {
                log_error("Failed to get monotonic timestamp: %s", strerror(-r));
                return r;
        }

        r = sd_journal_get_cursor(j, &cursor);
        if (r < 0) {
                log_error("Failed to get cursor: %s", strerror(-r));
                return r;
        }

        if (mode == OUTPUT_JSON_PRETTY)
                fprintf(f,
                        "{\n"
                        "\t\"__CURSOR\" : \"%s\",\n"
                        "\t\"__REALTIME_TIMESTAMP\" : \""USEC_FMT"\",\n"
                        "\t\"__MONOTONIC_TIMESTAMP\" : \""USEC_FMT"\",\n"
                        "\t\"_BOOT_ID\" : \"%s\"",
                        cursor,
                        realtime,
                        monotonic,
                        journal_uuid_to_str(boot_id, sid));
        else {
                if (mode == OUTPUT_JSON_SSE)
                        fputs("data: ", f);

                fprintf(f,
                        "{ \"__CURSOR\" : \"%s\", "
                        "\"__REALTIME_TIMESTAMP\" : \""USEC_FMT"\", "
                        "\"__MONOTONIC_TIMESTAMP\" : \""USEC_FMT"\", "
                        "\"_BOOT_ID\" : \"%s\"",
                        cursor,
                        realtime,
                        monotonic,
                        journal_uuid_to_str(boot_id, sid));
        }

        h = hashmap_new(string_hash_func, string_compare_func);
        if (!h)
                return -ENOMEM;

        /* First round, iterate through the entry and count how often each field appears */
        JOURNAL_FOREACH_DATA_RETVAL(j, data, length, r) {
                const char *eq;
                char *n;
                unsigned u;

                if (length >= 9 &&
                    memcmp(data, "_BOOT_ID=", 9) == 0)
                        continue;

                eq = memchr(data, '=', length);
                if (!eq)
                        continue;

                n = strndup(data, eq - (const char*) data);
                if (!n) {
                        r = -ENOMEM;
                        goto finish;
                }

                u = PTR_TO_UINT(hashmap_get(h, n));
                if (u == 0) {
                        r = hashmap_put(h, n, UINT_TO_PTR(1));
                        if (r < 0) {
                                free(n);
                                goto finish;
                        }
                } else {
                        r = hashmap_update(h, n, UINT_TO_PTR(u + 1));
                        free(n);
                        if (r < 0)
                                goto finish;
                }
        }

        if (r < 0)
                return r;

        separator = true;
        do {
                done = true;

                SD_JOURNAL_FOREACH_DATA(j, data, length) {
                        const char *eq;
                        char *kk, *n;
                        size_t m;
                        unsigned u;

                        /* We already printed the boot id, from the data in
                         * the header, hence let's suppress it here */
                        if (length >= 9 &&
                            memcmp(data, "_BOOT_ID=", 9) == 0)
                                continue;

                        eq = memchr(data, '=', length);
                        if (!eq)
                                continue;

                        if (separator) {
                                if (mode == OUTPUT_JSON_PRETTY)
                                        fputs(",\n\t", f);
                                else
                                        fputs(", ", f);
                        }

                        m = eq - (const char*) data;

                        n = strndup(data, m);
                        if (!n) {
                                r = -ENOMEM;
                                goto finish;
                        }

                        u = PTR_TO_UINT(hashmap_get2(h, n, (void**) &kk));
                        if (u == 0) {
                                /* We already printed this, let's jump to the next */
                                free(n);
                                separator = false;

                                continue;
                        } else if (u == 1) {
                                /* Field only appears once, output it directly */

                                json_escape(f, data, m, flags);
                                fputs(" : ", f);

                                json_escape(f, eq + 1, length - m - 1, flags);

                                hashmap_remove(h, n);
                                free(kk);
                                free(n);

                                separator = true;

                                continue;

                        } else {
                                /* Field appears multiple times, output it as array */
                                json_escape(f, data, m, flags);
                                fputs(" : [ ", f);
                                json_escape(f, eq + 1, length - m - 1, flags);

                                /* Iterate through the end of the list */

                                while (sd_journal_enumerate_data(j, &data, &length) > 0) {
                                        if (length < m + 1)
                                                continue;

                                        if (memcmp(data, n, m) != 0)
                                                continue;

                                        if (((const char*) data)[m] != '=')
                                                continue;

                                        fputs(", ", f);
                                        json_escape(f, (const char*) data + m + 1, length - m - 1, flags);
                                }

                                fputs(" ]", f);

                                hashmap_remove(h, n);
                                free(kk);
                                free(n);

                                /* Iterate data fields form the beginning */
                                done = false;
                                separator = true;

                                break;
                        }
                }

        } while (!done);

        if (mode == OUTPUT_JSON_PRETTY)
                fputs("\n}\n", f);
        else if (mode == OUTPUT_JSON_SSE)
                fputs("}\n\n", f);
        else
                fputs(" }\n", f);

        r = 0;

finish:
        while ((k = hashmap_steal_first_key(h)))
                free(k);

        hashmap_free(h);

        return r;
}

static int output_cat(
                FILE *f,
                sd_journal *j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags) {

        const void *data;
        size_t l;
        int r;

        assert(j);
        assert(f);

        sd_journal_set_data_threshold(j, 0);

        r = sd_journal_get_data(j, "MESSAGE", &data, &l);
        if (r < 0) {
                /* An entry without MESSAGE=? */
                if (r == -ENOENT)
                        return 0;

                log_error("Failed to get data: %s", strerror(-r));
                return r;
        }

        assert(l >= 8);

        fwrite((const char*) data + 8, 1, l - 8, f);
        fputc('\n', f);

        return 0;
}

static int (*output_funcs[_OUTPUT_MODE_MAX])(
                FILE *f,
                sd_journal*j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags) = {

        [OUTPUT_SHORT] = output_short,
        [OUTPUT_SHORT_ISO] = output_short,
        [OUTPUT_SHORT_PRECISE] = output_short,
        [OUTPUT_SHORT_MONOTONIC] = output_short,
        [OUTPUT_VERBOSE] = output_verbose,
        [OUTPUT_EXPORT] = output_export,
        [OUTPUT_JSON] = output_json,
        [OUTPUT_JSON_PRETTY] = output_json,
        [OUTPUT_JSON_SSE] = output_json,
        [OUTPUT_CAT] = output_cat
};

int output_journal(
                FILE *f,
                sd_journal *j,
                OutputMode mode,
                unsigned n_columns,
                OutputFlags flags,
                bool *ellipsized) {

        int ret;
        assert(mode >= 0);
        assert(mode < _OUTPUT_MODE_MAX);

        if (n_columns <= 0)
                n_columns = columns();

        ret = output_funcs[mode](f, j, mode, n_columns, flags);
        fflush(stdout);

        if (ellipsized && ret > 0)
                *ellipsized = true;

        return ret;
}

int add_match_this_boot(sd_journal *j) {
        char match[9+32+1] = "_BOOT_ID=";
        uuid_t boot_id;
        int r;

        assert(j);

        r = boot_get_id(&boot_id);
        if (r < 0) {
                log_error("Failed to get boot id: %s", strerror(-r));
                return r;
        }

        journal_uuid_to_str(boot_id, match + 9);
        r = sd_journal_add_match(j, match, strlen(match));
        if (r < 0) {
                log_error("Failed to add match: %s", strerror(-r));
                return r;
        }

        r = sd_journal_add_conjunction(j);
        if (r < 0)
                return r;

        return 0;
}

static const char *const output_mode_table[_OUTPUT_MODE_MAX] = {
        [OUTPUT_SHORT] = "short",
        [OUTPUT_SHORT_ISO] = "short-iso",
        [OUTPUT_SHORT_PRECISE] = "short-precise",
        [OUTPUT_SHORT_MONOTONIC] = "short-monotonic",
        [OUTPUT_VERBOSE] = "verbose",
        [OUTPUT_EXPORT] = "export",
        [OUTPUT_JSON] = "json",
        [OUTPUT_JSON_PRETTY] = "json-pretty",
        [OUTPUT_JSON_SSE] = "json-sse",
        [OUTPUT_CAT] = "cat"
};

DEFINE_STRING_TABLE_LOOKUP(output_mode, OutputMode);
