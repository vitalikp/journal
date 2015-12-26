
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#cmakedefine _GNU_SOURCE @_GNU_SOURCE@ 
#endif

/* AUDIT available */
#cmakedefine HAVE_AUDIT @HAVE_AUDIT@

/* Define if kmod is available */
#cmakedefine HAVE_KMOD @HAVE_KMOD@

/* Define if blkid is available */
#cmakedefine HAVE_BLKID @HAVE_BLKID@

/* Define to 1  if you have the `name_to_handle_at' function. */
#cmakedefine HAVE_DECL_NAME_TO_HANDLE_AT @HAVE_DECL_NAME_TO_HANDLE_AT@

/* Define to 1 if you have the `setns' function. */
#cmakedefine HAVE_DECL_SETNS @HAVE_DECL_SETNS@

/* Define to 1 if you have the `secure_getenv' function. */
#cmakedefine HAVE_SECURE_GETENV @HAVE_SECURE_GETENV@

/* Define to 1 if you have the `__secure_getenv' function. */
#cmakedefine HAVE___SECURE_GETENV @HAVE___SECURE_GETENV@

/* Define if SELinux is available */
#cmakedefine HAVE_SELINUX @HAVE_SELINUX@

/* Define if SMACK is available */
#cmakedefine HAVE_SMACK @HAVE_SMACK@

/* Define if XZ is available */
#cmakedefine HAVE_XZ @HAVE_XZ@

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


/* Version number of package */
#cmakedefine VERSION "@VERSION@"
