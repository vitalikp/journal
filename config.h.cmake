
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#cmakedefine _GNU_SOURCE @_GNU_SOURCE@ 
#endif

/* Maximum host name length */
#define HOST_NAME_MAX 64

/* Define if kmod is available */
#cmakedefine HAVE_KMOD @HAVE_KMOD@

/* Define if blkid is available */
#cmakedefine HAVE_BLKID @HAVE_BLKID@

/* Define to 1 if you have the `secure_getenv' function. */
#cmakedefine HAVE_SECURE_GETENV @HAVE_SECURE_GETENV@

/* Define to 1 if you have the `__secure_getenv' function. */
#cmakedefine HAVE___SECURE_GETENV @HAVE___SECURE_GETENV@

/* Define if XZ is available */
#cmakedefine HAVE_XZ @HAVE_XZ@

/* Define if LZ4 is available */
#cmakedefine HAVE_LZ4 @HAVE_LZ4@

/* The size of `pid_t', as computed by sizeof. */
#cmakedefine SIZEOF_PID_T @SIZEOF_PID_T@

/* The size of `uid_t', as computed by sizeof. */
#cmakedefine SIZEOF_UID_T @SIZEOF_UID_T@

/* The size of `gid_t', as computed by sizeof. */
#cmakedefine SIZEOF_GID_T @SIZEOF_GID_T@

/* The size of `time_t', as computed by sizeof. */
#cmakedefine SIZEOF_TIME_T @SIZEOF_TIME_T@

/* The size of `rlim_t', as computed by sizeof. */
#cmakedefine SIZEOF_RLIM_T @SIZEOF_RLIM_T@

/* Maximum System UID */
#cmakedefine SYSTEM_UID_MAX @SYSTEM_UID_MAX@

/* journal sysconfig directory */
#cmakedefine JOURNAL_SYSCONFDIR "@JOURNAL_SYSCONFDIR@"

/* journal runtime directory */
#cmakedefine JOURNAL_RUNDIR "@JOURNAL_RUNDIR@"

/* journal log directory */
#cmakedefine JOURNAL_LOGDIR "@JOURNAL_LOGDIR@"

/* journal devlog socket runtime path */
#cmakedefine JOURNAL_RUNDEVLOG "@JOURNAL_RUNDEVLOG@"

/* Version number of package */
#cmakedefine VERSION "@VERSION@"
