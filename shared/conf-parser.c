/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

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
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <netinet/ether.h>

#include "conf-parser.h"
#include "util.h"
#include "macro.h"
#include "log.h"
#include "utf8.h"
#include "path-util.h"
#include "set.h"

int log_syntax_internal(int level,
                        const char *file, unsigned line, const char *func,
                        const char *config_file, unsigned config_line,
                        int error, const char *format, ...) {

        _cleanup_free_ char *msg = NULL;
        int r;
        va_list ap;

        va_start(ap, format);
        r = vasprintf(&msg, format, ap);
        va_end(ap);
        if (r < 0)
                return log_oom();

        r = log_struct_internal(level,
                                file, line, func,
                                "CONFIG_FILE=%s", config_file,
                                "CONFIG_LINE=%u", config_line,
                                "ERRNO=%d", error > 0 ? error : EINVAL,
                                "MESSAGE=[%s:%u] %s", config_file, config_line, msg,
                                NULL);

        return r;
}

int config_item_perf_lookup(
                const void *table,
                const char *section,
                const char *lvalue,
                ConfigParserCallback *func,
                int *ltype,
                void **data,
                void *userdata) {

        ConfigPerfItemLookup lookup = (ConfigPerfItemLookup) table;
        const ConfigPerfItem *p;

        assert(table);
        assert(lvalue);
        assert(func);
        assert(ltype);
        assert(data);

        if (!section)
                p = lookup(lvalue, strlen(lvalue));
        else {
                char *key;

                key = strjoin(section, ".", lvalue, NULL);
                if (!key)
                        return -ENOMEM;

                p = lookup(key, strlen(key));
                free(key);
        }

        if (!p)
                return 0;

        *func = p->parse;
        *ltype = p->ltype;
        *data = (uint8_t*) userdata + p->offset;
        return 1;
}

/* Run the user supplied parser for an assignment */
static int next_assignment(const char *filename,
                           unsigned line,
                           ConfigItemLookup lookup,
                           const void *table,
                           const char *section,
                           unsigned section_line,
                           const char *lvalue,
                           const char *rvalue,
                           void *userdata) {

        ConfigParserCallback func = NULL;
        int ltype = 0;
        void *data = NULL;
        int r;

        assert(filename);
        assert(line > 0);
        assert(lookup);
        assert(lvalue);
        assert(rvalue);

        r = lookup(table, section, lvalue, &func, &ltype, &data, userdata);
        if (r < 0)
                return r;

        if (r > 0) {
                if (func)
                        return func(filename, line,
                                    lvalue, ltype, rvalue, data);

                return 0;
        }

        /* Warn about unknown non-extension fields. */
        if (!startswith(lvalue, "X-"))
                log_syntax(LOG_WARNING, filename, line, EINVAL,
                           "Unknown lvalue '%s' in section '%s'", lvalue, section);

        return 0;
}

/* Parse a variable assignment line */
static int parse_line(const char *filename,
                      unsigned line,
                      const char *sections,
                      ConfigItemLookup lookup,
                      const void *table,
                      char **section,
                      unsigned *section_line,
                      bool *section_ignored,
                      char *l,
                      void *userdata) {

        char *e;

        assert(filename);
        assert(line > 0);
        assert(lookup);
        assert(l);

        l = strstrip(l);

        if (!*l)
                return 0;

        if (strchr(COMMENTS "\n", *l))
                return 0;

        if (startswith(l, ".include ")) {

                /* .includes are a bad idea, we only support them here
                 * for historical reasons. They create cyclic include
                 * problems and make it difficult to detect
                 * configuration file changes with an easy
                 * stat(). Better approaches, such as .d/ drop-in
                 * snippets exist.
                 *
                 * Support for them should be eventually removed. */

                 log_syntax(LOG_ERR, filename, line, EBADMSG,
                            ".include not allowed here. Ignoring.");
                 return 0;
        }

        if (*l == '[') {
                size_t k;
                char *n;

                k = strlen(l);
                assert(k > 0);

                if (l[k-1] != ']') {
                        log_syntax(LOG_ERR, filename, line, EBADMSG,
                                   "Invalid section header '%s'", l);
                        return -EBADMSG;
                }

                n = strndup(l+1, k-2);
                if (!n)
                        return -ENOMEM;

                if (sections && !nulstr_contains(sections, n)) {

                        if (!startswith(n, "X-"))
                                log_syntax(LOG_WARNING, filename, line, EINVAL,
                                           "Unknown section '%s'. Ignoring.", n);

                        free(n);
                        free(*section);
                        *section = NULL;
                        *section_line = 0;
                        *section_ignored = true;
                } else {
                        free(*section);
                        *section = n;
                        *section_line = line;
                        *section_ignored = false;
                }

                return 0;
        }

        if (sections && !*section) {

                if (!*section_ignored)
                        log_syntax(LOG_WARNING, filename, line, EINVAL,
                                   "Assignment outside of section. Ignoring.");

                return 0;
        }

        e = strchr(l, '=');
        if (!e) {
                log_syntax(LOG_WARNING, filename, line, EINVAL, "Missing '='.");
                return -EBADMSG;
        }

        *e = 0;
        e++;

        return next_assignment(filename,
                               line,
                               lookup,
                               table,
                               *section,
                               *section_line,
                               strstrip(l),
                               strstrip(e),
                               userdata);
}

/* Go through the file and parse each line */
int config_parse(const char *filename,
                 const char *sections,
                 ConfigItemLookup lookup,
                 const void *table,
                 void *userdata) {

        _cleanup_free_ char *section = NULL, *continuation = NULL;
        _cleanup_fclose_ FILE *f = NULL;
        unsigned line = 0, section_line = 0;
        bool section_ignored = false;
        int r;

        assert(filename);
        assert(lookup);

        f = fopen(filename, "re");
        if (!f) {
                /* Only log on request, except for ENOENT,
                 * since we return 0 to the caller. */
                log_full(errno == ENOENT ? LOG_DEBUG : LOG_ERR,
                         "Failed to open configuration file '%s': %m", filename);
                return errno == ENOENT ? 0 : -errno;
        }

        fd_warn_permissions(filename, fileno(f));

        while (!feof(f)) {
                char l[LINE_MAX], *p, *c = NULL, *e;
                bool escaped = false;

                if (!fgets(l, sizeof(l), f)) {
                        if (feof(f))
                                break;

                        log_error("Failed to read configuration file '%s': %m", filename);
                        return -errno;
                }

                truncate_nl(l);

                if (continuation) {
                        c = strappend(continuation, l);
                        if (!c) {
                                log_oom();
                                return -ENOMEM;
                        }

                        free(continuation);
                        continuation = NULL;
                        p = c;
                } else
                        p = l;

                for (e = p; *e; e++) {
                        if (escaped)
                                escaped = false;
                        else if (*e == '\\')
                                escaped = true;
                }

                if (escaped) {
                        *(e-1) = ' ';

                        if (c)
                                continuation = c;
                        else {
                                continuation = strdup(l);
                                if (!continuation) {
                                        log_oom();
                                        return -ENOMEM;
                                }
                        }

                        continue;
                }

                r = parse_line(filename,
                               ++line,
                               sections,
                               lookup,
                               table,
                               &section,
                               &section_line,
                               &section_ignored,
                               p,
                               userdata);
                free(c);

                if (r < 0) {
                        log_warning("Failed to parse file '%s': %s",
                                    filename, strerror(-r));
                        return r;
                }
        }

        return 0;
}

#define DEFINE_PARSER(type, vartype, conv_func)                         \
        int config_parse_##type(const char *filename,                   \
                                unsigned line,                          \
                                const char *lvalue,                     \
                                int ltype,                              \
                                const char *rvalue,                     \
                                void *data) {                           \
                                                                        \
                vartype *i = data;                                      \
                int r;                                                  \
                                                                        \
                assert(filename);                                       \
                assert(lvalue);                                         \
                assert(rvalue);                                         \
                assert(data);                                           \
                                                                        \
                r = conv_func(rvalue, i);                               \
                if (r < 0)                                              \
                        log_syntax(LOG_ERR, filename, line, -r,   \
                                   "Failed to parse %s value, ignoring: %s", \
                                   #vartype, rvalue);                   \
                                                                        \
                return 0;                                               \
        }

DEFINE_PARSER(unsigned, unsigned, safe_atou)
DEFINE_PARSER(sec, usec_t, parse_sec)

int config_parse_iec_off(const char *filename,
                           unsigned line,
                           const char *lvalue,
                           int ltype,
                           const char *rvalue,
                           void *data) {

        off_t *bytes = data;
        int r;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        assert_cc(sizeof(off_t) == sizeof(uint64_t));

        r = parse_size(rvalue, 1024, bytes);
        if (r < 0)
                log_syntax(LOG_ERR, filename, line, -r, "Failed to parse size value, ignoring: %s", rvalue);

        return 0;
}

int config_parse_bool(const char *filename,
                      unsigned line,
                      const char *lvalue,
                      int ltype,
                      const char *rvalue,
                      void *data) {

        int k;
        bool *b = data;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        k = parse_boolean(rvalue);
        if (k < 0) {
                log_syntax(LOG_ERR, filename, line, -k,
                           "Failed to parse boolean value, ignoring: %s", rvalue);
                return 0;
        }

        *b = !!k;
        return 0;
}

int config_parse_string(
                const char *filename,
                unsigned line,
                const char *lvalue,
                int ltype,
                const char *rvalue,
                void *data) {

        char **s = data, *n;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        if (!utf8_is_valid(rvalue)) {
                log_invalid_utf8(LOG_ERR, filename, line, EINVAL, rvalue);
                return 0;
        }

        if (isempty(rvalue))
                n = NULL;
        else {
                n = strdup(rvalue);
                if (!n)
                        return log_oom();
        }

        free(*s);
        *s = n;

        return 0;
}

int config_parse_path(
                const char *filename,
                unsigned line,
                const char *lvalue,
                int ltype,
                const char *rvalue,
                void *data) {

        char **s = data, *n;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        if (!utf8_is_valid(rvalue)) {
                log_invalid_utf8(LOG_ERR, filename, line, EINVAL, rvalue);
                return 0;
        }

        if (!path_is_absolute(rvalue)) {
                log_syntax(LOG_ERR, filename, line, EINVAL, "Not an absolute path, ignoring: %s", rvalue);
                return 0;
        }

        n = strdup(rvalue);
        if (!n)
                return log_oom();

        path_kill_slashes(n);

        free(*s);
        *s = n;

        return 0;
}

int config_parse_log_level(
                const char *filename,
                unsigned line,
                const char *lvalue,
                int ltype,
                const char *rvalue,
                void *data) {


        int *o = data, x;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        x = log_level_from_string(rvalue);
        if (x < 0) {
                log_syntax(LOG_ERR, filename, line, EINVAL,
                           "Failed to parse log level, ignoring: %s", rvalue);
                return 0;
        }

        *o = (*o & LOG_FACMASK) | x;
        return 0;
}
