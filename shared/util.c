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

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>
#include <sched.h>
#include <sys/resource.h>
#include <linux/sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/tiocl.h>
#include <termios.h>
#include <stdarg.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <netinet/ip.h>
#include <linux/kd.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <grp.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <linux/magic.h>
#include <limits.h>
#include <langinfo.h>
#include <locale.h>
#include <sys/personality.h>
#include <libgen.h>
#undef basename

#ifdef HAVE_SYS_AUXV_H
#include <sys/auxv.h>
#endif

#include "macro.h"
#include "util.h"
#include "missing.h"
#include "log.h"
#include "path-util.h"
#include "set.h"
#include "hashmap.h"
#include "fileio.h"
#include "utf8.h"


static volatile unsigned cached_columns = 0;
static volatile unsigned cached_lines = 0;

size_t page_size(void) {
        static thread_local size_t pgsz = 0;
        long r;

        if (_likely_(pgsz > 0))
                return pgsz;

        r = sysconf(_SC_PAGESIZE);
        assert(r > 0);

        pgsz = (size_t) r;
        return pgsz;
}

bool streq_ptr(const char *a, const char *b) {

        /* Like streq(), but tries to make sense of NULL pointers */

        if (a && b)
                return streq(a, b);

        if (!a && !b)
                return true;

        return false;
}

char* endswith(const char *s, const char *postfix) {
        size_t sl, pl;

        assert(s);
        assert(postfix);

        sl = strlen(s);
        pl = strlen(postfix);

        if (pl == 0)
                return (char*) s + sl;

        if (sl < pl)
                return NULL;

        if (memcmp(s + sl - pl, postfix, pl) != 0)
                return NULL;

        return (char*) s + sl - pl;
}

int close_nointr(int fd) {
        int r;

        assert(fd >= 0);
        r = close(fd);
        if (r >= 0)
                return r;
        else if (errno == EINTR)
                /*
                 * Just ignore EINTR; a retry loop is the wrong
                 * thing to do on Linux.
                 *
                 * http://lkml.indiana.edu/hypermail/linux/kernel/0509.1/0877.html
                 * https://bugzilla.gnome.org/show_bug.cgi?id=682819
                 * http://utcc.utoronto.ca/~cks/space/blog/unix/CloseEINTR
                 * https://sites.google.com/site/michaelsafyan/software-engineering/checkforeintrwheninvokingclosethinkagain
                 */
                return 0;
        else
                return -errno;
}

int safe_close(int fd) {

        /*
         * Like close_nointr() but cannot fail. Guarantees errno is
         * unchanged. Is a NOP with negative fds passed, and returns
         * -1, so that it can be used in this syntax:
         *
         * fd = safe_close(fd);
         */

        if (fd >= 0) {
                PROTECT_ERRNO;

                /* The kernel might return pretty much any error code
                 * via close(), but the fd will be closed anyway. The
                 * only condition we want to check for here is whether
                 * the fd was invalid at all... */

                assert_se(close_nointr(fd) != -EBADF);
        }

        return -1;
}

void close_many(const int fds[], unsigned n_fd) {
        unsigned i;

        assert(fds || n_fd <= 0);

        for (i = 0; i < n_fd; i++)
                safe_close(fds[i]);
}

int parse_boolean(const char *v) {
        assert(v);

        if (streq(v, "1") || v[0] == 'y' || v[0] == 'Y' || v[0] == 't' || v[0] == 'T' || strcaseeq(v, "on"))
                return 1;
        else if (streq(v, "0") || v[0] == 'n' || v[0] == 'N' || v[0] == 'f' || v[0] == 'F' || strcaseeq(v, "off"))
                return 0;

        return -EINVAL;
}

int parse_pid(const char *s, pid_t* ret_pid) {
        unsigned long ul = 0;
        pid_t pid;
        int r;

        assert(s);
        assert(ret_pid);

        r = safe_atolu(s, &ul);
        if (r < 0)
                return r;

        pid = (pid_t) ul;

        if ((unsigned long) pid != ul)
                return -ERANGE;

        if (pid <= 0)
                return -ERANGE;

        *ret_pid = pid;
        return 0;
}

int parse_uid(const char *s, uid_t* ret_uid) {
        unsigned long ul = 0;
        uid_t uid;
        int r;

        assert(s);
        assert(ret_uid);

        r = safe_atolu(s, &ul);
        if (r < 0)
                return r;

        uid = (uid_t) ul;

        if ((unsigned long) uid != ul)
                return -ERANGE;

        *ret_uid = uid;
        return 0;
}

int safe_atou(const char *s, unsigned *ret_u) {
        char *x = NULL;
        unsigned long l;

        assert(s);
        assert(ret_u);

        errno = 0;
        l = strtoul(s, &x, 0);

        if (!x || x == s || *x || errno)
                return errno > 0 ? -errno : -EINVAL;

        if ((unsigned long) (unsigned) l != l)
                return -ERANGE;

        *ret_u = (unsigned) l;
        return 0;
}

int safe_atoi(const char *s, int *ret_i) {
        char *x = NULL;
        long l;

        assert(s);
        assert(ret_i);

        errno = 0;
        l = strtol(s, &x, 0);

        if (!x || x == s || *x || errno)
                return errno > 0 ? -errno : -EINVAL;

        if ((long) (int) l != l)
                return -ERANGE;

        *ret_i = (int) l;
        return 0;
}

int safe_atollu(const char *s, long long unsigned *ret_llu) {
        char *x = NULL;
        unsigned long long l;

        assert(s);
        assert(ret_llu);

        errno = 0;
        l = strtoull(s, &x, 0);

        if (!x || x == s || *x || errno)
                return errno ? -errno : -EINVAL;

        *ret_llu = l;
        return 0;
}

int safe_atolli(const char *s, long long int *ret_lli) {
        char *x = NULL;
        long long l;

        assert(s);
        assert(ret_lli);

        errno = 0;
        l = strtoll(s, &x, 0);

        if (!x || x == s || *x || errno)
                return errno ? -errno : -EINVAL;

        *ret_lli = l;
        return 0;
}

int safe_atod(const char *s, double *ret_d) {
        char *x = NULL;
        double d = 0;

        assert(s);
        assert(ret_d);

        RUN_WITH_LOCALE(LC_NUMERIC_MASK, "C") {
                errno = 0;
                d = strtod(s, &x);
        }

        if (!x || x == s || *x || errno)
                return errno ? -errno : -EINVAL;

        *ret_d = (double) d;
        return 0;
}

static size_t strcspn_escaped(const char *s, const char *reject) {
        bool escaped = false;
        size_t n;

        for (n=0; s[n]; n++) {
                if (escaped)
                        escaped = false;
                else if (s[n] == '\\')
                        escaped = true;
                else if (strchr(reject, s[n]))
                        break;
        }
        /* if s ends in \, return index of previous char */
        return n - escaped;
}

/* Split a string into words. */
const char* split(const char **state, size_t *l, const char *separator, bool quoted) {
        const char *current;

        current = *state;

        if (!*current) {
                assert(**state == '\0');
                return NULL;
        }

        current += strspn(current, separator);
        if (!*current) {
                *state = current;
                return NULL;
        }

        if (quoted && strchr("\'\"", *current)) {
                char quotechars[2] = {*current, '\0'};

                *l = strcspn_escaped(current + 1, quotechars);
                if (current[*l + 1] == '\0' ||
                    (current[*l + 2] && !strchr(separator, current[*l + 2]))) {
                        /* right quote missing or garbage at the end*/
                        *state = current;
                        return NULL;
                }
                assert(current[*l + 1] == quotechars[0]);
                *state = current++ + *l + 2;
        } else if (quoted) {
                *l = strcspn_escaped(current, separator);
                *state = current + *l;
        } else {
                *l = strcspn(current, separator);
                *state = current + *l;
        }

        return current;
}

int fchmod_umask(int fd, mode_t m) {
        mode_t u;
        int r;

        u = umask(0777);
        r = fchmod(fd, m & (~u)) < 0 ? -errno : 0;
        umask(u);

        return r;
}

char *truncate_nl(char *s) {
        assert(s);

        s[strcspn(s, NEWLINE)] = 0;
        return s;
}

int get_process_comm(pid_t pid, char **name) {
        const char *p;
        int r;

        assert(name);
        assert(pid >= 0);

        p = procfs_file_alloca(pid, "comm");

        r = read_one_line_file(p, name);
        if (r == -ENOENT)
                return -ESRCH;

        return r;
}

int get_process_cmdline(pid_t pid, size_t max_length, bool comm_fallback, char **line) {
        _cleanup_fclose_ FILE *f = NULL;
        char *r = NULL, *k;
        const char *p;
        int c;

        assert(line);
        assert(pid >= 0);

        p = procfs_file_alloca(pid, "cmdline");

        f = fopen(p, "re");
        if (!f)
                return -errno;

        if (max_length == 0) {
                size_t len = 0, allocated = 0;

                while ((c = getc(f)) != EOF) {

                        if (!GREEDY_REALLOC(r, allocated, len+2)) {
                                free(r);
                                return -ENOMEM;
                        }

                        r[len++] = isprint(c) ? c : ' ';
                }

                if (len > 0)
                        r[len-1] = 0;

        } else {
                bool space = false;
                size_t left;

                r = new(char, max_length);
                if (!r)
                        return -ENOMEM;

                k = r;
                left = max_length;
                while ((c = getc(f)) != EOF) {

                        if (isprint(c)) {
                                if (space) {
                                        if (left <= 4)
                                                break;

                                        *(k++) = ' ';
                                        left--;
                                        space = false;
                                }

                                if (left <= 4)
                                        break;

                                *(k++) = (char) c;
                                left--;
                        }  else
                                space = true;
                }

                if (left <= 4) {
                        size_t n = MIN(left-1, 3U);
                        memcpy(k, "...", n);
                        k[n] = 0;
                } else
                        *k = 0;
        }

        /* Kernel threads have no argv[] */
        if (r == NULL || r[0] == 0) {
                _cleanup_free_ char *t = NULL;
                int h;

                free(r);

                if (!comm_fallback)
                        return -ENOENT;

                h = get_process_comm(pid, &t);
                if (h < 0)
                        return h;

                r = strjoin("[", t, "]", NULL);
                if (!r)
                        return -ENOMEM;
        }

        *line = r;
        return 0;
}

int get_process_exe(pid_t pid, char **name) {
        const char *p;
        char *d;
        int r;

        assert(pid >= 0);
        assert(name);

        p = procfs_file_alloca(pid, "exe");

        r = readlink_malloc(p, name);
        if (r < 0)
                return r == -ENOENT ? -ESRCH : r;

        d = endswith(*name, " (deleted)");
        if (d)
                *d = '\0';

        return 0;
}

static int get_process_id(pid_t pid, const char *field, uid_t *uid) {
        _cleanup_fclose_ FILE *f = NULL;
        char line[LINE_MAX];
        const char *p;

        assert(field);
        assert(uid);

        if (pid == 0)
                return getuid();

        p = procfs_file_alloca(pid, "status");
        f = fopen(p, "re");
        if (!f)
                return -errno;

        FOREACH_LINE(line, f, return -errno) {
                char *l;

                l = strstrip(line);

                if (startswith(l, field)) {
                        l += strlen(field);
                        l += strspn(l, WHITESPACE);

                        l[strcspn(l, WHITESPACE)] = 0;

                        return parse_uid(l, uid);
                }
        }

        return -EIO;
}

int get_process_uid(pid_t pid, uid_t *uid) {
        return get_process_id(pid, "Uid:", uid);
}

int get_process_gid(pid_t pid, gid_t *gid) {
        assert_cc(sizeof(uid_t) == sizeof(gid_t));
        return get_process_id(pid, "Gid:", gid);
}

char *strnappend(const char *s, const char *suffix, size_t b) {
        size_t a;
        char *r;

        if (!s && !suffix)
                return strdup("");

        if (!s)
                return strndup(suffix, b);

        if (!suffix)
                return strdup(s);

        assert(s);
        assert(suffix);

        a = strlen(s);
        if (b > ((size_t) -1) - a)
                return NULL;

        r = new(char, a+b+1);
        if (!r)
                return NULL;

        memcpy(r, s, a);
        memcpy(r+a, suffix, b);
        r[a+b] = 0;

        return r;
}

char *strappend(const char *s, const char *suffix) {
        return strnappend(s, suffix, suffix ? strlen(suffix) : 0);
}

int readlinkat_malloc(int fd, const char *p, char **ret) {
        size_t l = 100;
        int r;

        assert(p);
        assert(ret);

        for (;;) {
                char *c;
                ssize_t n;

                c = new(char, l);
                if (!c)
                        return -ENOMEM;

                n = readlinkat(fd, p, c, l-1);
                if (n < 0) {
                        r = -errno;
                        free(c);
                        return r;
                }

                if ((size_t) n < l-1) {
                        c[n] = 0;
                        *ret = c;
                        return 0;
                }

                free(c);
                l *= 2;
        }
}

int readlink_malloc(const char *p, char **ret) {
        return readlinkat_malloc(AT_FDCWD, p, ret);
}

char *strstrip(char *s) {
        char *e;

        /* Drops trailing whitespace. Modifies the string in
         * place. Returns pointer to first non-space character */

        s += strspn(s, WHITESPACE);

        for (e = strchr(s, 0); e > s; e --)
                if (!strchr(WHITESPACE, e[-1]))
                        break;

        *e = 0;

        return s;
}

char *file_in_same_dir(const char *path, const char *filename) {
        char *e, *r;
        size_t k;

        assert(path);
        assert(filename);

        /* This removes the last component of path and appends
         * filename, unless the latter is absolute anyway or the
         * former isn't */

        if (path_is_absolute(filename))
                return strdup(filename);

        if (!(e = strrchr(path, '/')))
                return strdup(filename);

        k = strlen(filename);
        if (!(r = new(char, e-path+1+k+1)))
                return NULL;

        memcpy(r, path, e-path+1);
        memcpy(r+(e-path)+1, filename, k+1);

        return r;
}

char hexchar(int x) {
        static const char table[16] = "0123456789abcdef";

        return table[x & 15];
}

int unhexchar(char c) {

        if (c >= '0' && c <= '9')
                return c - '0';

        if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;

        if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;

        return -EINVAL;
}

char octchar(int x) {
        return '0' + (x & 7);
}

int unoctchar(char c) {

        if (c >= '0' && c <= '7')
                return c - '0';

        return -EINVAL;
}

char decchar(int x) {
        return '0' + (x % 10);
}

int undecchar(char c) {

        if (c >= '0' && c <= '9')
                return c - '0';

        return -EINVAL;
}

char *cescape(const char *s) {
        char *r, *t;
        const char *f;

        assert(s);

        /* Does C style string escaping. */

        r = new(char, strlen(s)*4 + 1);
        if (!r)
                return NULL;

        for (f = s, t = r; *f; f++)

                switch (*f) {

                case '\a':
                        *(t++) = '\\';
                        *(t++) = 'a';
                        break;
                case '\b':
                        *(t++) = '\\';
                        *(t++) = 'b';
                        break;
                case '\f':
                        *(t++) = '\\';
                        *(t++) = 'f';
                        break;
                case '\n':
                        *(t++) = '\\';
                        *(t++) = 'n';
                        break;
                case '\r':
                        *(t++) = '\\';
                        *(t++) = 'r';
                        break;
                case '\t':
                        *(t++) = '\\';
                        *(t++) = 't';
                        break;
                case '\v':
                        *(t++) = '\\';
                        *(t++) = 'v';
                        break;
                case '\\':
                        *(t++) = '\\';
                        *(t++) = '\\';
                        break;
                case '"':
                        *(t++) = '\\';
                        *(t++) = '"';
                        break;
                case '\'':
                        *(t++) = '\\';
                        *(t++) = '\'';
                        break;

                default:
                        /* For special chars we prefer octal over
                         * hexadecimal encoding, simply because glib's
                         * g_strescape() does the same */
                        if ((*f < ' ') || (*f >= 127)) {
                                *(t++) = '\\';
                                *(t++) = octchar((unsigned char) *f >> 6);
                                *(t++) = octchar((unsigned char) *f >> 3);
                                *(t++) = octchar((unsigned char) *f);
                        } else
                                *(t++) = *f;
                        break;
                }

        *t = 0;

        return r;
}

char *cunescape_length_with_prefix(const char *s, size_t length, const char *prefix) {
        char *r, *t;
        const char *f;
        size_t pl;

        assert(s);

        /* Undoes C style string escaping, and optionally prefixes it. */

        pl = prefix ? strlen(prefix) : 0;

        r = new(char, pl+length+1);
        if (!r)
                return NULL;

        if (prefix)
                memcpy(r, prefix, pl);

        for (f = s, t = r + pl; f < s + length; f++) {

                if (*f != '\\') {
                        *(t++) = *f;
                        continue;
                }

                f++;

                switch (*f) {

                case 'a':
                        *(t++) = '\a';
                        break;
                case 'b':
                        *(t++) = '\b';
                        break;
                case 'f':
                        *(t++) = '\f';
                        break;
                case 'n':
                        *(t++) = '\n';
                        break;
                case 'r':
                        *(t++) = '\r';
                        break;
                case 't':
                        *(t++) = '\t';
                        break;
                case 'v':
                        *(t++) = '\v';
                        break;
                case '\\':
                        *(t++) = '\\';
                        break;
                case '"':
                        *(t++) = '"';
                        break;
                case '\'':
                        *(t++) = '\'';
                        break;

                case 's':
                        /* This is an extension of the XDG syntax files */
                        *(t++) = ' ';
                        break;

                case 'x': {
                        /* hexadecimal encoding */
                        int a, b;

                        a = unhexchar(f[1]);
                        b = unhexchar(f[2]);

                        if (a < 0 || b < 0) {
                                /* Invalid escape code, let's take it literal then */
                                *(t++) = '\\';
                                *(t++) = 'x';
                        } else {
                                *(t++) = (char) ((a << 4) | b);
                                f += 2;
                        }

                        break;
                }

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7': {
                        /* octal encoding */
                        int a, b, c;

                        a = unoctchar(f[0]);
                        b = unoctchar(f[1]);
                        c = unoctchar(f[2]);

                        if (a < 0 || b < 0 || c < 0) {
                                /* Invalid escape code, let's take it literal then */
                                *(t++) = '\\';
                                *(t++) = f[0];
                        } else {
                                *(t++) = (char) ((a << 6) | (b << 3) | c);
                                f += 2;
                        }

                        break;
                }

                case 0:
                        /* premature end of string.*/
                        *(t++) = '\\';
                        goto finish;

                default:
                        /* Invalid escape code, let's take it literal then */
                        *(t++) = '\\';
                        *(t++) = *f;
                        break;
                }
        }

finish:
        *t = 0;
        return r;
}

char *cunescape_length(const char *s, size_t length) {
        return cunescape_length_with_prefix(s, length, NULL);
}

char *cunescape(const char *s) {
        assert(s);

        return cunescape_length(s, strlen(s));
}

char *xescape(const char *s, const char *bad) {
        char *r, *t;
        const char *f;

        /* Escapes all chars in bad, in addition to \ and all special
         * chars, in \xFF style escaping. May be reversed with
         * cunescape. */

        r = new(char, strlen(s) * 4 + 1);
        if (!r)
                return NULL;

        for (f = s, t = r; *f; f++) {

                if ((*f < ' ') || (*f >= 127) ||
                    (*f == '\\') || strchr(bad, *f)) {
                        *(t++) = '\\';
                        *(t++) = 'x';
                        *(t++) = hexchar(*f >> 4);
                        *(t++) = hexchar(*f);
                } else
                        *(t++) = *f;
        }

        *t = 0;

        return r;
}

_pure_ static bool ignore_file_allow_backup(const char *filename) {
        assert(filename);

        return
                filename[0] == '.' ||
                streq(filename, "lost+found") ||
                streq(filename, "aquota.user") ||
                streq(filename, "aquota.group") ||
                endswith(filename, ".rpmnew") ||
                endswith(filename, ".rpmsave") ||
                endswith(filename, ".rpmorig") ||
                endswith(filename, ".dpkg-old") ||
                endswith(filename, ".dpkg-new") ||
                endswith(filename, ".swp");
}

bool ignore_file(const char *filename) {
        assert(filename);

        if (endswith(filename, "~"))
                return true;

        return ignore_file_allow_backup(filename);
}

int open_terminal(const char *name, int mode) {
        int fd, r;
        unsigned c = 0;

        /*
         * If a TTY is in the process of being closed opening it might
         * cause EIO. This is horribly awful, but unlikely to be
         * changed in the kernel. Hence we work around this problem by
         * retrying a couple of times.
         *
         * https://bugs.launchpad.net/ubuntu/+source/linux/+bug/554172/comments/245
         */

        assert(!(mode & O_CREAT));

        for (;;) {
                fd = open(name, mode, 0);
                if (fd >= 0)
                        break;

                if (errno != EIO)
                        return -errno;

                /* Max 1s in total */
                if (c >= 20)
                        return -errno;

                usleep(50 * USEC_PER_MSEC);
                c++;
        }

        if (fd < 0)
                return -errno;

        r = isatty(fd);
        if (r < 0) {
                safe_close(fd);
                return -errno;
        }

        if (!r) {
                safe_close(fd);
                return -ENOTTY;
        }

        return fd;
}

void safe_close_pair(int p[]) {
        assert(p);

        if (p[0] == p[1]) {
                /* Special case pairs which use the same fd in both
                 * directions... */
                p[0] = p[1] = safe_close(p[0]);
                return;
        }

        p[0] = safe_close(p[0]);
        p[1] = safe_close(p[1]);
}

ssize_t loop_read(int fd, void *buf, size_t nbytes, bool do_poll) {
        uint8_t *p = buf;
        ssize_t n = 0;

        assert(fd >= 0);
        assert(buf);

        while (nbytes > 0) {
                ssize_t k;

                k = read(fd, p, nbytes);
                if (k < 0 && errno == EINTR)
                        continue;

                if (k < 0 && errno == EAGAIN && do_poll) {

                        /* We knowingly ignore any return value here,
                         * and expect that any error/EOF is reported
                         * via read() */

                        fd_wait_for_event(fd, POLLIN, USEC_INFINITY);
                        continue;
                }

                if (k <= 0)
                        return n > 0 ? n : (k < 0 ? -errno : 0);

                p += k;
                nbytes -= k;
                n += k;
        }

        return n;
}

ssize_t loop_write(int fd, const void *buf, size_t nbytes, bool do_poll) {
        const uint8_t *p = buf;
        ssize_t n = 0;

        assert(fd >= 0);
        assert(buf);

        while (nbytes > 0) {
                ssize_t k;

                k = write(fd, p, nbytes);
                if (k < 0 && errno == EINTR)
                        continue;

                if (k < 0 && errno == EAGAIN && do_poll) {

                        /* We knowingly ignore any return value here,
                         * and expect that any error/EOF is reported
                         * via write() */

                        fd_wait_for_event(fd, POLLOUT, USEC_INFINITY);
                        continue;
                }

                if (k <= 0)
                        return n > 0 ? n : (k < 0 ? -errno : 0);

                p += k;
                nbytes -= k;
                n += k;
        }

        return n;
}

int parse_size(const char *t, off_t base, off_t *size) {

        /* Soo, sometimes we want to parse IEC binary suffxies, and
         * sometimes SI decimal suffixes. This function can parse
         * both. Which one is the right way depends on the
         * context. Wikipedia suggests that SI is customary for
         * hardrware metrics and network speeds, while IEC is
         * customary for most data sizes used by software and volatile
         * (RAM) memory. Hence be careful which one you pick!
         *
         * In either case we use just K, M, G as suffix, and not Ki,
         * Mi, Gi or so (as IEC would suggest). That's because that's
         * frickin' ugly. But this means you really need to make sure
         * to document which base you are parsing when you use this
         * call. */

        struct table {
                const char *suffix;
                unsigned long long factor;
        };

        static const struct table iec[] = {
                { "E", 1024ULL*1024ULL*1024ULL*1024ULL*1024ULL*1024ULL },
                { "P", 1024ULL*1024ULL*1024ULL*1024ULL*1024ULL },
                { "T", 1024ULL*1024ULL*1024ULL*1024ULL },
                { "G", 1024ULL*1024ULL*1024ULL },
                { "M", 1024ULL*1024ULL },
                { "K", 1024ULL },
                { "B", 1 },
                { "", 1 },
        };

        static const struct table si[] = {
                { "E", 1000ULL*1000ULL*1000ULL*1000ULL*1000ULL*1000ULL },
                { "P", 1000ULL*1000ULL*1000ULL*1000ULL*1000ULL },
                { "T", 1000ULL*1000ULL*1000ULL*1000ULL },
                { "G", 1000ULL*1000ULL*1000ULL },
                { "M", 1000ULL*1000ULL },
                { "K", 1000ULL },
                { "B", 1 },
                { "", 1 },
        };

        const struct table *table;
        const char *p;
        unsigned long long r = 0;
        unsigned n_entries, start_pos = 0;

        assert(t);
        assert(base == 1000 || base == 1024);
        assert(size);

        if (base == 1000) {
                table = si;
                n_entries = ELEMENTSOF(si);
        } else {
                table = iec;
                n_entries = ELEMENTSOF(iec);
        }

        p = t;
        do {
                long long l;
                unsigned long long l2;
                double frac = 0;
                char *e;
                unsigned i;

                errno = 0;
                l = strtoll(p, &e, 10);

                if (errno > 0)
                        return -errno;

                if (l < 0)
                        return -ERANGE;

                if (e == p)
                        return -EINVAL;

                if (*e == '.') {
                        e++;
                        if (*e >= '0' && *e <= '9') {
                                char *e2;

                                /* strotoull itself would accept space/+/- */
                                l2 = strtoull(e, &e2, 10);

                                if (errno == ERANGE)
                                        return -errno;

                                /* Ignore failure. E.g. 10.M is valid */
                                frac = l2;
                                for (; e < e2; e++)
                                        frac /= 10;
                        }
                }

                e += strspn(e, WHITESPACE);

                for (i = start_pos; i < n_entries; i++)
                        if (startswith(e, table[i].suffix)) {
                                unsigned long long tmp;
                                if ((unsigned long long) l + (frac > 0) > ULLONG_MAX / table[i].factor)
                                        return -ERANGE;
                                tmp = l * table[i].factor + (unsigned long long) (frac * table[i].factor);
                                if (tmp > ULLONG_MAX - r)
                                        return -ERANGE;

                                r += tmp;
                                if ((unsigned long long) (off_t) r != r)
                                        return -ERANGE;

                                p = e + strlen(table[i].suffix);

                                start_pos = i + 1;
                                break;
                        }

                if (i >= n_entries)
                        return -EINVAL;

        } while (*p);

        *size = r;

        return 0;
}

char* dirname_malloc(const char *path) {
        char *d, *dir, *dir2;

        d = strdup(path);
        if (!d)
                return NULL;
        dir = dirname(d);
        assert(dir);

        if (dir != d) {
                dir2 = strdup(dir);
                free(d);
                return dir2;
        }

        return dir;
}

int dev_urandom(void *p, size_t n) {
        _cleanup_close_ int fd = -1;
        ssize_t k;

        fd = open("/dev/urandom", O_RDONLY|O_CLOEXEC|O_NOCTTY);
        if (fd < 0)
                return errno == ENOENT ? -ENOSYS : -errno;

        k = loop_read(fd, p, n, true);
        if (k < 0)
                return (int) k;
        if ((size_t) k != n)
                return -EIO;

        return 0;
}

void random_bytes(void *p, size_t n) {
        static bool srand_called = false;
        uint8_t *q;
        int r;

        r = dev_urandom(p, n);
        if (r >= 0)
                return;

        /* If some idiot made /dev/urandom unavailable to us, he'll
         * get a PRNG instead. */

        if (!srand_called) {
                unsigned x = 0;

#ifdef HAVE_SYS_AUXV_H
                /* The kernel provides us with a bit of entropy in
                 * auxv, so let's try to make use of that to seed the
                 * pseudo-random generator. It's better than
                 * nothing... */

                void *auxv;

                auxv = (void*) getauxval(AT_RANDOM);
                if (auxv)
                        x ^= *(unsigned*) auxv;
#endif

                x ^= (unsigned) now(CLOCK_REALTIME);
                x ^= (unsigned) gettid();

                srand(x);
                srand_called = true;
        }

        for (q = p; q < (uint8_t*) p + n; q ++)
                *q = rand();
}

int rm_rf_children_dangerous(int fd, bool only_dirs, bool honour_sticky, struct stat *root_dev) {
        DIR *d;
        int ret = 0;

        assert(fd >= 0);

        /* This returns the first error we run into, but nevertheless
         * tries to go on. This closes the passed fd. */

        d = fdopendir(fd);
        if (!d) {
                safe_close(fd);

                return errno == ENOENT ? 0 : -errno;
        }

        for (;;) {
                struct dirent *de;
                bool is_dir, keep_around;
                struct stat st;
                int r;

                errno = 0;
                de = readdir(d);
                if (!de && errno != 0) {
                        if (ret == 0)
                                ret = -errno;
                        break;
                }

                if (!de)
                        break;

                if (streq(de->d_name, ".") || streq(de->d_name, ".."))
                        continue;

                if (de->d_type == DT_UNKNOWN ||
                    honour_sticky ||
                    (de->d_type == DT_DIR && root_dev)) {
                        if (fstatat(fd, de->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0) {
                                if (ret == 0 && errno != ENOENT)
                                        ret = -errno;
                                continue;
                        }

                        is_dir = S_ISDIR(st.st_mode);
                        keep_around =
                                honour_sticky &&
                                (st.st_uid == 0 || st.st_uid == getuid()) &&
                                (st.st_mode & S_ISVTX);
                } else {
                        is_dir = de->d_type == DT_DIR;
                        keep_around = false;
                }

                if (is_dir) {
                        int subdir_fd;

                        /* if root_dev is set, remove subdirectories only, if device is same as dir */
                        if (root_dev && st.st_dev != root_dev->st_dev)
                                continue;

                        subdir_fd = openat(fd, de->d_name,
                                           O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW|O_NOATIME);
                        if (subdir_fd < 0) {
                                if (ret == 0 && errno != ENOENT)
                                        ret = -errno;
                                continue;
                        }

                        r = rm_rf_children_dangerous(subdir_fd, only_dirs, honour_sticky, root_dev);
                        if (r < 0 && ret == 0)
                                ret = r;

                        if (!keep_around)
                                if (unlinkat(fd, de->d_name, AT_REMOVEDIR) < 0) {
                                        if (ret == 0 && errno != ENOENT)
                                                ret = -errno;
                                }

                } else if (!only_dirs && !keep_around) {

                        if (unlinkat(fd, de->d_name, 0) < 0) {
                                if (ret == 0 && errno != ENOENT)
                                        ret = -errno;
                        }
                }
        }

        closedir(d);

        return ret;
}

_pure_ static int is_temporary_fs(struct statfs *s) {
        assert(s);

        return F_TYPE_EQUAL(s->f_type, TMPFS_MAGIC) ||
               F_TYPE_EQUAL(s->f_type, RAMFS_MAGIC);
}

int rm_rf_children(int fd, bool only_dirs, bool honour_sticky, struct stat *root_dev) {
        struct statfs s;

        assert(fd >= 0);

        if (fstatfs(fd, &s) < 0) {
                safe_close(fd);
                return -errno;
        }

        /* We refuse to clean disk file systems with this call. This
         * is extra paranoia just to be sure we never ever remove
         * non-state data */
        if (!is_temporary_fs(&s)) {
                log_error("Attempted to remove disk file system, and we can't allow that.");
                safe_close(fd);
                return -EPERM;
        }

        return rm_rf_children_dangerous(fd, only_dirs, honour_sticky, root_dev);
}

static int rm_rf_internal(const char *path, bool only_dirs, bool delete_root, bool honour_sticky, bool dangerous) {
        int fd, r;
        struct statfs s;

        assert(path);

        /* We refuse to clean the root file system with this
         * call. This is extra paranoia to never cause a really
         * seriously broken system. */
        if (path_equal(path, "/")) {
                log_error("Attempted to remove entire root file system, and we can't allow that.");
                return -EPERM;
        }

        fd = open(path, O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW|O_NOATIME);
        if (fd < 0) {

                if (errno != ENOTDIR)
                        return -errno;

                if (!dangerous) {
                        if (statfs(path, &s) < 0)
                                return -errno;

                        if (!is_temporary_fs(&s)) {
                                log_error("Attempted to remove disk file system, and we can't allow that.");
                                return -EPERM;
                        }
                }

                if (delete_root && !only_dirs)
                        if (unlink(path) < 0 && errno != ENOENT)
                                return -errno;

                return 0;
        }

        if (!dangerous) {
                if (fstatfs(fd, &s) < 0) {
                        safe_close(fd);
                        return -errno;
                }

                if (!is_temporary_fs(&s)) {
                        log_error("Attempted to remove disk file system, and we can't allow that.");
                        safe_close(fd);
                        return -EPERM;
                }
        }

        r = rm_rf_children_dangerous(fd, only_dirs, honour_sticky, NULL);
        if (delete_root) {

                if (honour_sticky && file_is_priv_sticky(path) > 0)
                        return r;

                if (rmdir(path) < 0 && errno != ENOENT) {
                        if (r == 0)
                                r = -errno;
                }
        }

        return r;
}

int rm_rf(const char *path, bool only_dirs, bool delete_root, bool honour_sticky) {
        return rm_rf_internal(path, only_dirs, delete_root, honour_sticky, false);
}

int rm_rf_dangerous(const char *path, bool only_dirs, bool delete_root, bool honour_sticky) {
        return rm_rf_internal(path, only_dirs, delete_root, honour_sticky, true);
}

int chmod_and_chown(const char *path, mode_t mode, uid_t uid, gid_t gid) {
        assert(path);

        /* Under the assumption that we are running privileged we
         * first change the access mode and only then hand out
         * ownership to avoid a window where access is too open. */

        if (mode != (mode_t) -1)
                if (chmod(path, mode) < 0)
                        return -errno;

        if (uid != (uid_t) -1 || gid != (gid_t) -1)
                if (chown(path, uid, gid) < 0)
                        return -errno;

        return 0;
}

int fd_columns(int fd) {
        struct winsize ws = {};

        if (ioctl(fd, TIOCGWINSZ, &ws) < 0)
                return -errno;

        if (ws.ws_col <= 0)
                return -EIO;

        return ws.ws_col;
}

unsigned columns(void) {
        const char *e;
        int c;

        if (_likely_(cached_columns > 0))
                return cached_columns;

        c = 0;
        e = getenv("COLUMNS");
        if (e)
                safe_atoi(e, &c);

        if (c <= 0)
                c = fd_columns(STDOUT_FILENO);

        if (c <= 0)
                c = 80;

        cached_columns = c;
        return c;
}

int fd_lines(int fd) {
        struct winsize ws = {};

        if (ioctl(fd, TIOCGWINSZ, &ws) < 0)
                return -errno;

        if (ws.ws_row <= 0)
                return -EIO;

        return ws.ws_row;
}

unsigned lines(void) {
        const char *e;
        unsigned l;

        if (_likely_(cached_lines > 0))
                return cached_lines;

        l = 0;
        e = getenv("LINES");
        if (e)
                safe_atou(e, &l);

        if (l <= 0)
                l = fd_lines(STDOUT_FILENO);

        if (l <= 0)
                l = 24;

        cached_lines = l;
        return cached_lines;
}

/* intended to be used as a SIGWINCH sighandler */
void columns_lines_cache_reset(int signum) {
        cached_columns = 0;
        cached_lines = 0;
}

bool on_tty(void) {
        static int cached_on_tty = -1;

        if (_unlikely_(cached_on_tty < 0))
                cached_on_tty = isatty(STDOUT_FILENO) > 0;

        return cached_on_tty;
}

int files_same(const char *filea, const char *fileb) {
        struct stat a, b;

        if (stat(filea, &a) < 0)
                return -errno;

        if (stat(fileb, &b) < 0)
                return -errno;

        return a.st_dev == b.st_dev &&
               a.st_ino == b.st_ino;
}

int running_in_chroot(void) {
        int ret;

        ret = files_same("/proc/1/root", "/");
        if (ret < 0)
                return ret;

        return ret == 0;
}

int touch_file(const char *path, usec_t stamp, uid_t uid, gid_t gid, mode_t mode) {
        _cleanup_close_ int fd = -1;
        int r;

        assert(path);

        fd = open(path, O_WRONLY|O_CREAT|O_CLOEXEC|O_NOCTTY, mode > 0 ? mode : 0644);
        if (fd < 0)
                return -errno;

        if (mode > 0) {
                r = fchmod(fd, mode);
                if (r < 0)
                        return -errno;
        }

        if (uid != (uid_t) -1 || gid != (gid_t) -1) {
                r = fchown(fd, uid, gid);
                if (r < 0)
                        return -errno;
        }

        if (stamp != USEC_INFINITY) {
                struct timespec ts[2];

                timespec_store(&ts[0], stamp);
                ts[1] = ts[0];
                r = futimens(fd, ts);
        } else
                r = futimens(fd, NULL);
        if (r < 0)
                return -errno;

        return 0;
}

int touch(const char *path) {
        return touch_file(path, USEC_INFINITY, (uid_t) -1, (gid_t) -1, 0);
}

char *unquote(const char *s, const char* quotes) {
        size_t l;
        assert(s);

        /* This is rather stupid, simply removes the heading and
         * trailing quotes if there is one. Doesn't care about
         * escaping or anything. We should make this smarter one
         * day...*/

        l = strlen(s);
        if (l < 2)
                return strdup(s);

        if (strchr(quotes, s[0]) && s[l-1] == s[0])
                return strndup(s+1, l-2);

        return strdup(s);
}

int wait_for_terminate(pid_t pid, siginfo_t *status) {
        siginfo_t dummy;

        assert(pid >= 1);

        if (!status)
                status = &dummy;

        for (;;) {
                zero(*status);

                if (waitid(P_PID, pid, status, WEXITED) < 0) {

                        if (errno == EINTR)
                                continue;

                        return -errno;
                }

                return 0;
        }
}

bool null_or_empty(struct stat *st) {
        assert(st);

        if (S_ISREG(st->st_mode) && st->st_size <= 0)
                return true;

        if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
                return true;

        return false;
}

bool tty_is_vc(const char *tty) {
        assert(tty);

        if (startswith(tty, "/dev/"))
                tty += 5;

        return vtnr_from_tty(tty) >= 0;
}

int vtnr_from_tty(const char *tty) {
        int i, r;

        assert(tty);

        if (startswith(tty, "/dev/"))
                tty += 5;

        if (!startswith(tty, "tty") )
                return -EINVAL;

        if (tty[3] < '0' || tty[3] > '9')
                return -EINVAL;

        r = safe_atoi(tty+3, &i);
        if (r < 0)
                return r;

        if (i < 0 || i > 63)
                return -EINVAL;

        return i;
}

bool dirent_is_file(const struct dirent *de) {
        assert(de);

        if (ignore_file(de->d_name))
                return false;

        if (de->d_type != DT_REG &&
            de->d_type != DT_LNK &&
            de->d_type != DT_UNKNOWN)
                return false;

        return true;
}

bool dirent_is_file_with_suffix(const struct dirent *de, const char *suffix) {
        assert(de);

        if (de->d_type != DT_REG &&
            de->d_type != DT_LNK &&
            de->d_type != DT_UNKNOWN)
                return false;

        if (ignore_file_allow_backup(de->d_name))
                return false;

        return endswith(de->d_name, suffix);
}

bool nulstr_contains(const char*nulstr, const char *needle) {
        const char *i;

        if (!nulstr)
                return false;

        NULSTR_FOREACH(i, nulstr)
                if (streq(i, needle))
                        return true;

        return false;
}

int fd_wait_for_event(int fd, int event, usec_t t) {

        struct pollfd pollfd = {
                .fd = fd,
                .events = event,
        };

        struct timespec ts;
        int r;

        r = ppoll(&pollfd, 1, t == USEC_INFINITY ? NULL : timespec_store(&ts, t), NULL);
        if (r < 0)
                return -errno;

        if (r == 0)
                return 0;

        return pollfd.revents;
}

int fopen_temporary(const char *path, FILE **_f, char **_temp_path) {
        FILE *f;
        char *t;
        const char *fn;
        size_t k;
        int fd;

        assert(path);
        assert(_f);
        assert(_temp_path);

        t = new(char, strlen(path) + 1 + 6 + 1);
        if (!t)
                return -ENOMEM;

        fn = basename(path);
        k = fn - path;
        memcpy(t, path, k);
        t[k] = '.';
        stpcpy(stpcpy(t+k+1, fn), "XXXXXX");

        fd = mkostemp_safe(t, O_WRONLY|O_CLOEXEC);
        if (fd < 0) {
                free(t);
                return -errno;
        }

        f = fdopen(fd, "we");
        if (!f) {
                unlink(t);
                free(t);
                return -errno;
        }

        *_f = f;
        *_temp_path = t;

        return 0;
}

int get_group_creds(const char **groupname, gid_t *gid) {
        struct group *g;
        gid_t id;

        assert(groupname);

        /* We enforce some special rules for gid=0: in order to avoid
         * NSS lookups for root we hardcode its data. */

        if (streq(*groupname, "root") || streq(*groupname, "0")) {
                *groupname = "root";

                if (gid)
                        *gid = 0;

                return 0;
        }

        if (parse_gid(*groupname, &id) >= 0) {
                errno = 0;
                g = getgrgid(id);

                if (g)
                        *groupname = g->gr_name;
        } else {
                errno = 0;
                g = getgrnam(*groupname);
        }

        if (!g)
                return errno > 0 ? -errno : -ESRCH;

        if (gid)
                *gid = g->gr_gid;

        return 0;
}

int in_gid(gid_t gid) {
        gid_t *gids;
        int ngroups_max, r, i;

        if (getgid() == gid)
                return 1;

        if (getegid() == gid)
                return 1;

        ngroups_max = sysconf(_SC_NGROUPS_MAX);
        assert(ngroups_max > 0);

        gids = alloca(sizeof(gid_t) * ngroups_max);

        r = getgroups(ngroups_max, gids);
        if (r < 0)
                return -errno;

        for (i = 0; i < r; i++)
                if (gids[i] == gid)
                        return 1;

        return 0;
}

int in_group(const char *name) {
        int r;
        gid_t gid;

        r = get_group_creds(&name, &gid);
        if (r < 0)
                return r;

        return in_gid(gid);
}

char *strjoin(const char *x, ...) {
        va_list ap;
        size_t l;
        char *r, *p;

        va_start(ap, x);

        if (x) {
                l = strlen(x);

                for (;;) {
                        const char *t;
                        size_t n;

                        t = va_arg(ap, const char *);
                        if (!t)
                                break;

                        n = strlen(t);
                        if (n > ((size_t) -1) - l) {
                                va_end(ap);
                                return NULL;
                        }

                        l += n;
                }
        } else
                l = 0;

        va_end(ap);

        r = new(char, l+1);
        if (!r)
                return NULL;

        if (x) {
                p = stpcpy(r, x);

                va_start(ap, x);

                for (;;) {
                        const char *t;

                        t = va_arg(ap, const char *);
                        if (!t)
                                break;

                        p = stpcpy(p, t);
                }

                va_end(ap);
        } else
                r[0] = 0;

        return r;
}

bool is_main_thread(void) {
        static thread_local int cached = 0;

        if (_unlikely_(cached == 0))
                cached = getpid() == gettid() ? 1 : -1;

        return cached > 0;
}

int file_is_priv_sticky(const char *p) {
        struct stat st;

        assert(p);

        if (lstat(p, &st) < 0)
                return -errno;

        return
                (st.st_uid == 0 || st.st_uid == getuid()) &&
                (st.st_mode & S_ISVTX);
}

static const char *const log_facility_unshifted_table[LOG_NFACILITIES] = {
        [LOG_FAC(LOG_KERN)] = "kern",
        [LOG_FAC(LOG_USER)] = "user",
        [LOG_FAC(LOG_MAIL)] = "mail",
        [LOG_FAC(LOG_DAEMON)] = "daemon",
        [LOG_FAC(LOG_AUTH)] = "auth",
        [LOG_FAC(LOG_SYSLOG)] = "syslog",
        [LOG_FAC(LOG_LPR)] = "lpr",
        [LOG_FAC(LOG_NEWS)] = "news",
        [LOG_FAC(LOG_UUCP)] = "uucp",
        [LOG_FAC(LOG_CRON)] = "cron",
        [LOG_FAC(LOG_AUTHPRIV)] = "authpriv",
        [LOG_FAC(LOG_FTP)] = "ftp",
        [LOG_FAC(LOG_LOCAL0)] = "local0",
        [LOG_FAC(LOG_LOCAL1)] = "local1",
        [LOG_FAC(LOG_LOCAL2)] = "local2",
        [LOG_FAC(LOG_LOCAL3)] = "local3",
        [LOG_FAC(LOG_LOCAL4)] = "local4",
        [LOG_FAC(LOG_LOCAL5)] = "local5",
        [LOG_FAC(LOG_LOCAL6)] = "local6",
        [LOG_FAC(LOG_LOCAL7)] = "local7"
};

DEFINE_STRING_TABLE_LOOKUP_WITH_FALLBACK(log_facility_unshifted, int, LOG_FAC(~0));

static const char *const log_level_table[] = {
        [LOG_EMERG] = "emerg",
        [LOG_ALERT] = "alert",
        [LOG_CRIT] = "crit",
        [LOG_ERR] = "err",
        [LOG_WARNING] = "warning",
        [LOG_NOTICE] = "notice",
        [LOG_INFO] = "info",
        [LOG_DEBUG] = "debug"
};

DEFINE_STRING_TABLE_LOOKUP_WITH_FALLBACK(log_level, int, LOG_DEBUG);

static const char *const __signal_table[] = {
        [SIGHUP] = "HUP",
        [SIGINT] = "INT",
        [SIGQUIT] = "QUIT",
        [SIGILL] = "ILL",
        [SIGTRAP] = "TRAP",
        [SIGABRT] = "ABRT",
        [SIGBUS] = "BUS",
        [SIGFPE] = "FPE",
        [SIGKILL] = "KILL",
        [SIGUSR1] = "USR1",
        [SIGSEGV] = "SEGV",
        [SIGUSR2] = "USR2",
        [SIGPIPE] = "PIPE",
        [SIGALRM] = "ALRM",
        [SIGTERM] = "TERM",
#ifdef SIGSTKFLT
        [SIGSTKFLT] = "STKFLT",  /* Linux on SPARC doesn't know SIGSTKFLT */
#endif
        [SIGCHLD] = "CHLD",
        [SIGCONT] = "CONT",
        [SIGSTOP] = "STOP",
        [SIGTSTP] = "TSTP",
        [SIGTTIN] = "TTIN",
        [SIGTTOU] = "TTOU",
        [SIGURG] = "URG",
        [SIGXCPU] = "XCPU",
        [SIGXFSZ] = "XFSZ",
        [SIGVTALRM] = "VTALRM",
        [SIGPROF] = "PROF",
        [SIGWINCH] = "WINCH",
        [SIGIO] = "IO",
        [SIGPWR] = "PWR",
        [SIGSYS] = "SYS"
};

DEFINE_PRIVATE_STRING_TABLE_LOOKUP(__signal, int);

const char *signal_to_string(int signo) {
        static thread_local char buf[sizeof("RTMIN+")-1 + DECIMAL_STR_MAX(int) + 1];
        const char *name;

        name = __signal_to_string(signo);
        if (name)
                return name;

        if (signo >= SIGRTMIN && signo <= SIGRTMAX)
                snprintf(buf, sizeof(buf), "RTMIN+%d", signo - SIGRTMIN);
        else
                snprintf(buf, sizeof(buf), "%d", signo);

        return buf;
}

int signal_from_string(const char *s) {
        int signo;
        int offset = 0;
        unsigned u;

        signo = __signal_from_string(s);
        if (signo > 0)
                return signo;

        if (startswith(s, "RTMIN+")) {
                s += 6;
                offset = SIGRTMIN;
        }
        if (safe_atou(s, &u) >= 0) {
                signo = (int) u + offset;
                if (signo > 0 && signo < _NSIG)
                        return signo;
        }
        return -EINVAL;
}

int prot_from_flags(int flags) {

        switch (flags & O_ACCMODE) {

        case O_RDONLY:
                return PROT_READ;

        case O_WRONLY:
                return PROT_WRITE;

        case O_RDWR:
                return PROT_READ|PROT_WRITE;

        default:
                return -EINVAL;
        }
}

char *format_bytes(char *buf, size_t l, off_t t) {
        unsigned i;

        static const struct {
                const char *suffix;
                off_t factor;
        } table[] = {
                { "E", 1024ULL*1024ULL*1024ULL*1024ULL*1024ULL*1024ULL },
                { "P", 1024ULL*1024ULL*1024ULL*1024ULL*1024ULL },
                { "T", 1024ULL*1024ULL*1024ULL*1024ULL },
                { "G", 1024ULL*1024ULL*1024ULL },
                { "M", 1024ULL*1024ULL },
                { "K", 1024ULL },
        };

        for (i = 0; i < ELEMENTSOF(table); i++) {

                if (t >= table[i].factor) {
                        snprintf(buf, l,
                                 "%llu.%llu%s",
                                 (unsigned long long) (t / table[i].factor),
                                 (unsigned long long) (((t*10ULL) / table[i].factor) % 10ULL),
                                 table[i].suffix);

                        goto finish;
                }
        }

        snprintf(buf, l, "%lluB", (unsigned long long) t);

finish:
        buf[l-1] = 0;
        return buf;

}

void* memdup(const void *p, size_t l) {
        void *r;

        assert(p);

        r = malloc(l);
        if (!r)
                return NULL;

        memcpy(r, p, l);
        return r;
}

int fd_inc_sndbuf(int fd, size_t n) {
        int r, value;
        socklen_t l = sizeof(value);

        r = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, &l);
        if (r >= 0 && l == sizeof(value) && (size_t) value >= n*2)
                return 0;

        /* If we have the privileges we will ignore the kernel limit. */

        value = (int) n;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUFFORCE, &value, sizeof(value)) < 0)
                if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) < 0)
                        return -errno;

        return 1;
}

int getenv_for_pid(pid_t pid, const char *field, char **_value) {
        _cleanup_fclose_ FILE *f = NULL;
        char *value = NULL;
        int r;
        bool done = false;
        size_t l;
        const char *path;

        assert(pid >= 0);
        assert(field);
        assert(_value);

        path = procfs_file_alloca(pid, "environ");

        f = fopen(path, "re");
        if (!f)
                return -errno;

        l = strlen(field);
        r = 0;

        do {
                char line[LINE_MAX];
                unsigned i;

                for (i = 0; i < sizeof(line)-1; i++) {
                        int c;

                        c = getc(f);
                        if (_unlikely_(c == EOF)) {
                                done = true;
                                break;
                        } else if (c == 0)
                                break;

                        line[i] = c;
                }
                line[i] = 0;

                if (memcmp(line, field, l) == 0 && line[l] == '=') {
                        value = strdup(line + l + 1);
                        if (!value)
                                return -ENOMEM;

                        r = 1;
                        break;
                }

        } while (!done);

        *_value = value;
        return r;
}

bool in_initrd(void) {
        static int saved = -1;
        struct statfs s;

        if (saved >= 0)
                return saved;

        /* We make two checks here:
         *
         * 1. the flag file /etc/initrd-release must exist
         * 2. the root file system must be a memory file system
         *
         * The second check is extra paranoia, since misdetecting an
         * initrd can have bad bad consequences due the initrd
         * emptying when transititioning to the main systemd.
         */

        saved = access("/etc/initrd-release", F_OK) >= 0 &&
                statfs("/", &s) >= 0 &&
                is_temporary_fs(&s);

        return saved;
}

bool filename_is_safe(const char *p) {

        if (isempty(p))
                return false;

        if (strchr(p, '/'))
                return false;

        if (streq(p, "."))
                return false;

        if (streq(p, ".."))
                return false;

        if (strlen(p) > FILENAME_MAX)
                return false;

        return true;
}

bool string_is_safe(const char *p) {
        const char *t;

        if (!p)
                return false;

        for (t = p; *t; t++) {
                if (*t > 0 && *t < ' ')
                        return false;

                if (strchr("\\\"\'\0x7f", *t))
                        return false;
        }

        return true;
}

void* greedy_realloc(void **p, size_t *allocated, size_t need, size_t size) {
        size_t a, newalloc;
        void *q;

        assert(p);
        assert(allocated);

        if (*allocated >= need)
                return *p;

        newalloc = MAX(need * 2, 64u / size);
        a = newalloc * size;

        /* check for overflows */
        if (a < size * need)
                return NULL;

        q = realloc(*p, a);
        if (!q)
                return NULL;

        *p = q;
        *allocated = newalloc;
        return q;
}

void* greedy_realloc0(void **p, size_t *allocated, size_t need, size_t size) {
        size_t prev;
        uint8_t *q;

        assert(p);
        assert(allocated);

        prev = *allocated;

        q = greedy_realloc(p, allocated, need, size);
        if (!q)
                return NULL;

        if (*allocated > prev)
                memzero(q + prev * size, (*allocated - prev) * size);

        return q;
}

int proc_cmdline(char **ret) {
        int r;

        r = read_one_line_file("/proc/cmdline", ret);
        if (r < 0)
                return r;

        return 1;
}

/* This is much like like mkostemp() but is subject to umask(). */
int mkostemp_safe(char *pattern, int flags) {
        _cleanup_umask_ mode_t u = 077;
        int fd;

        assert(pattern);

        u = umask(u);

        fd = mkostemp(pattern, flags);
        if (fd < 0)
                return -errno;

        return fd;
}

int open_tmpfile(const char *path, int flags) {
        char *p;
        int fd;

        assert(path);

#ifdef O_TMPFILE
        /* Try O_TMPFILE first, if it is supported */
        fd = open(path, flags|O_TMPFILE, S_IRUSR|S_IWUSR);
        if (fd >= 0)
                return fd;
#endif

        /* Fall back to unguessable name + unlinking */
        p = strappenda(path, "/systemd-tmp-XXXXXX");

        fd = mkostemp_safe(p, flags);
        if (fd < 0)
                return fd;

        unlink(p);
        return fd;
}

int fd_warn_permissions(const char *path, int fd) {
        struct stat st;

        if (fstat(fd, &st) < 0)
                return -errno;

        if (st.st_mode & 0111)
                log_warning("Configuration file %s is marked executable. Please remove executable permission bits. Proceeding anyway.", path);

        if (st.st_mode & 0002)
                log_warning("Configuration file %s is marked world-writable. Please remove world writability permission bits. Proceeding anyway.", path);

        if (getpid() == 1 && (st.st_mode & 0044) != 0044)
                log_warning("Configuration file %s is marked world-inaccessible. This has no effect as configuration data is accessible via APIs without restrictions. Proceeding anyway.", path);

        return 0;
}
